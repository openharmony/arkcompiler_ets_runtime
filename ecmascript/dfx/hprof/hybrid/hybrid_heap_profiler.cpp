/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <csignal>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/dfx/hprof/hybrid/hybrid_heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/dfx/hprof/hybrid/hybrid_heap_snapshot.h"
#include "ecmascript/dfx/hprof/stream.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/runtime.h"
#include "ecmascript/runtime_lock.h"
#if defined(ENABLE_DUMP_IN_FAULTLOG)
#include "faultloggerd_client.h"
#endif

namespace panda::ecmascript {
HybridHeapProfiler *HybridHeapProfiler::GetInstance()
{
    if (!Runtime::HasInstance()) {
        LOG_ECMA(ERROR) << "[HybridHeapDump] HybridHeapProfiler::GetInstance: Runtime instance not available";
        return nullptr;
    }
    EcmaVM *mainVm = Runtime::GetInstance()->GetMainThread()->GetEcmaVM();
    if (mainVm == nullptr) {
        LOG_ECMA(ERROR) << "[HybridHeapDump] HybridHeapProfiler::GetInstance: Main EcmaVM not available";
        return nullptr;
    }
#ifdef PANDA_JS_ETS_HYBRID_MODE
    return mainVm->GetOrNewHybridHeapProfiler();
#endif
    return nullptr;
}

HybridHeapProfiler::HybridHeapProfiler(EcmaVM *vm)
    : mainVm_(vm),
      stringTable_(vm)
{
#ifdef PANDA_JS_ETS_HYBRID_MODE
    auto *crossVmOp = vm->GetCrossVMOperator();
    if (crossVmOp != nullptr) {
        stsInterface_ = crossVmOp->GetSTSVMInterface();
    }
#endif  // PANDA_JS_ETS_HYBRID_MODE
}

HybridHeapProfiler::JSSuspendAllScope::JSSuspendAllScope(EcmaVM *vm, bool enabled)
    : vm_(vm), enabled_(enabled)
{
    if (enabled_) {
        ASSERT(vm_ != nullptr);
        thread_ = vm_->GetAssociatedJSThread();
        locker_ = std::make_unique<RuntimeLockHolder>(
            thread_, SharedHeap::GetInstance()->GetSuspensionRequestMutex());
        suspendScope_ = std::make_unique<SuspendAllScope<JSThread>>(thread_);
    }
}

HybridHeapProfiler::JSSuspendAllScope::~JSSuspendAllScope()
{
    if (enabled_) {
        suspendScope_.reset();
        locker_.reset();
    }
}

void HybridHeapProfiler::JSForceFullGC(const EcmaVM *vm)
{
    ThreadManagedScope managedScope(vm->GetAssociatedJSThread());
    [[maybe_unused]] bool heapClean = false;
    if (vm->IsInitialized()) {
        const_cast<Heap *>(vm->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);
        heapClean = true;
    }
    SharedHeap *sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(vm->GetAssociatedJSThread());
    sHeap->GetSweeper()->WaitAllTaskFinished();
    ASSERT(heapClean);
}

void HybridHeapProfiler::ForceFullGC(EcmaVM *vm, const DumpSnapShotOption &dumpOption)
{
    if (!dumpOption.isFullGC) {
        return;
    }
    if (dumpOption.dumpDynamicHeap && dumpOption.dumpStaticHeap) {
        LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapProfiler::ForceFullGC: trigger XGC";
        stsInterface_->TriggerXGCAndWait();
    }
    if (dumpOption.dumpDynamicHeap) {
        LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapProfiler::ForceFullGC: trigger Dynamic ArkTS FullGC";
        JSForceFullGC(vm);
    }
    if (dumpOption.dumpStaticHeap) {
        LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapProfiler::ForceFullGC: trigger Static ArkTS FullGC";
        stsInterface_->EtsForceFullGC();
    }
}

void HybridHeapProfiler::WaitForJSGCFinish(const EcmaVM *vm, const DumpSnapShotOption &dumpOption)
{
    if (!dumpOption.dumpDynamicHeap) {
        return;
    }
    JSThread *thread = vm->GetAssociatedJSThread();
    const_cast<Heap *>(vm->GetHeap())->Prepare();
    SharedHeap::GetInstance()->PrepareByJSThread(thread, true);
    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(thread->IsSuspended() || thread->HasLaunchedSuspendAll());
        const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->FillBumpPointerForTlab();
    });
}

static void HybridWaitProcess(pid_t pid)
{
    time_t startTime = time(nullptr);
    constexpr int dumpTimeOut = 300;
    constexpr int defaultSleepTime = 100000;
    while (time(nullptr) <= startTime + dumpTimeOut) {
        int status = 0;
        pid_t p = waitpid(pid, &status, WNOHANG);
        if (p < 0) {
            LOG_ECMA(ERROR) << "[HybridHeapDump] HybridWaitProcess wait failed";
            return;
        }
        if (p == pid) {
            LOG_ECMA(INFO) << "[HybridHeapDump] HybridWaitProcess child completed";
            return;
        }
        usleep(defaultSleepTime);
    }
    LOG_ECMA(ERROR) << "[HybridHeapDump] HybridWaitProcess timeout, killing child after "
                    << dumpTimeOut << "s";
    kill(pid, SIGKILL);
}

bool HybridHeapProfiler::BuildAndSerializeSnapshot(EcmaVM *vm, Stream *stream, const DumpSnapShotOption &dumpOption)
{
    NativeAreaAllocator *allocator = vm != nullptr
        ? vm->GetNativeAreaAllocator()
        : mainVm_->GetNativeAreaAllocator();
    auto *snapshot = new HybridHeapSnapshot(vm, stsInterface_, &entryIdMap_, &stringTable_,
                                            dumpOption.isSimplify, dumpOption.dumpDynamicHeap,
                                            dumpOption.dumpStaticHeap, allocator);
    bool result = snapshot->BuildHybridSnapshot();
    LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapProfiler::BuildHybridSnapshot, result = " << result
                   << ", nodeCount = " << snapshot->GetNodeCount();
    if (result && dumpOption.isSync) {
        UpdateEntryIdMap(snapshot);
    }
    if (result) {
        LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapProfiler::BuildAndSerializeSnapshot: serializing snapshot";
        result = HeapSnapshotJSONSerializer::Serialize(snapshot, stream);
    }
    delete snapshot;
    return result;
}

pid_t HybridHeapProfiler::ForkAndPerformDump(EcmaVM *vm, Stream *stream, const DumpSnapShotOption &dumpOption)
{
    LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapProfiler::ForkAndPerformDump: forking process to perform dump";
    pid_t pid = fork();
    if (pid < 0) {
        LOG_ECMA(ERROR) << "[HybridHeapDump] ForkAndPerformDump fork failed: " << strerror(errno);
        return -1;
    }
    if (pid == 0) {
        if (dumpOption.dumpDynamicHeap) {
            vm->GetAssociatedJSThread()->SetCrossThreadExecution(true);
        }
        prctl(PR_SET_NAME, reinterpret_cast<unsigned long>("dump_process"), 0, 0, 0);
        BuildAndSerializeSnapshot(vm, stream, dumpOption);
        _exit(0);
    }
    return pid;
}

bool HybridHeapProfiler::Dump(EcmaVM *vm, Stream *stream, DumpSnapShotOption &dumpOption)
{
    if (!HasSTSInterface()) {
        LOG_ECMA(ERROR) << "[HybridHeapDump] HybridHeapProfiler::Dump: stsInterface is null, abort";
        return false;
    }
    dumpOption.dumpDynamicHeap = vm != nullptr;
    dumpOption.dumpStaticHeap = stsInterface_->IsCurrentThreadAttached();
    LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapProfiler::Dump: "
                   << " dumpDynamic = " << dumpOption.dumpDynamicHeap
                   << ", dumpStatic = " << dumpOption.dumpStaticHeap;
    if (!dumpOption.dumpDynamicHeap && !dumpOption.dumpStaticHeap) {
        return false;
    }

    std::unique_ptr<FileDescriptorStream> ownedStream;
    if (stream == nullptr) {
        int32_t fd = AcquireDumpStream(dumpOption);
        if (fd < 0) {
            return false;
        }
        ownedStream = std::make_unique<FileDescriptorStream>(fd);
        stream = ownedStream.get();
    }

    ForceFullGC(vm, dumpOption);
    JSSuspendAllScope jsSuspendScope(vm, dumpOption.dumpDynamicHeap);
    WaitForJSGCFinish(vm, dumpOption);
    EtsSuspendAllScope etsSuspendScope(stsInterface_, dumpOption.dumpStaticHeap);
    if (dumpOption.isSync) {
        return BuildAndSerializeSnapshot(vm, stream, dumpOption);
    }
    if (!SetAppFreezeFilter()) {
        return false;
    }
    if (dumpOption.isBeforeFill) {
        FillIdMap(vm, dumpOption);
    }
    pid_t pid = -1;
    pid = ForkAndPerformDump(vm, stream, dumpOption);
    if (pid < 0) {
        return false;
    }
    std::thread waitThread(&HybridWaitProcess, pid);
    waitThread.detach();
    return true;
}

int32_t HybridHeapProfiler::AcquireDumpStream(const DumpSnapShotOption &dumpOption)
{
#if defined(ENABLE_DUMP_IN_FAULTLOG)
    FaultLoggerType faultLoggerType = FaultLoggerType::JS_HEAP_SNAPSHOT;
    if (dumpOption.dumpDynamicHeap && dumpOption.dumpStaticHeap) {
        faultLoggerType = FaultLoggerType::HYBRID_JS_HEAP_SNAPSHOT;
    } else if (dumpOption.dumpStaticHeap) {
        faultLoggerType = FaultLoggerType::STATIC_JS_HEAP_SNAPSHOT;
    }
    int32_t fd = RequestFileDescriptor(static_cast<int32_t>(faultLoggerType));
    if (fd < 0) {
        LOG_ECMA(ERROR) << "[HybridHeapDump] RequestFileDescriptor failed, fd: " << fd;
    }
    return fd;
#else
    return -1;
#endif
}

bool HybridHeapProfiler::SetAppFreezeFilter()
{
    AppFreezeFilterCallback callback = Runtime::GetInstance()->GetAppFreezeFilterCallback();
    if (callback != nullptr) {
        std::string unused;
        if (!callback(getpid(), false, unused)) {
            LOG_ECMA(ERROR) << "[HybridHeapDump] failed to set appfreeze filter";
            return false;
        }
    }
    return true;
}

void HybridHeapProfiler::UpdateEntryIdMap(HybridHeapSnapshot *snapshot)
{
    EntryIdMap newIdMap;
    auto nodes = snapshot->GetNodes();
    for (auto node : *nodes) {
        auto addr = node->GetAddress();
        auto [idExist, sequenceId] = entryIdMap_.FindId(addr);
        if (idExist) {
            newIdMap.InsertId(addr, sequenceId);
        }
    }
    entryIdMap_.GetIdMap()->swap(*newIdMap.GetIdMap());
}

void HybridHeapProfiler::FillIdMap(EcmaVM *vm, const DumpSnapShotOption &dumpOption)
{
    EntryIdMap newEntryIdMap;

    // Dynamic: iterate SharedHeap + LocalHeap
    if (dumpOption.dumpDynamicHeap) {
        SharedHeap *sHeap = SharedHeap::GetInstance();
        if (sHeap != nullptr) {
            sHeap->IterateOverObjects([&](TaggedObject *obj) {
                JSTaggedType addr = (JSTaggedValue(obj)).GetRawData();
                auto [idExist, sequenceId] = entryIdMap_.FindId(addr);
                newEntryIdMap.InsertId(addr, sequenceId);
            });
            sHeap->GetReadOnlySpace()->IterateOverObjects([&](TaggedObject *obj) {
                JSTaggedType addr = (JSTaggedValue(obj)).GetRawData();
                auto [idExist, sequenceId] = entryIdMap_.FindId(addr);
                newEntryIdMap.InsertId(addr, sequenceId);
            });
        }
        auto *heap = vm->GetHeap();
        if (heap != nullptr) {
            heap->IterateOverObjects([&](TaggedObject *obj) {
                JSTaggedType addr = (JSTaggedValue(obj)).GetRawData();
                auto [idExist, sequenceId] = entryIdMap_.FindId(addr);
                newEntryIdMap.InsertId(addr, sequenceId);
            });
        }
    }

    // Static: iterate through STSVMInterface
    if (dumpOption.dumpStaticHeap) {
        stsInterface_->IterateEtsObjects([&](uint64_t addr) {
            auto [idExist, sequenceId] = entryIdMap_.FindId(addr);
            newEntryIdMap.InsertId(addr, sequenceId);
        });
    }

    entryIdMap_.GetIdMap()->swap(*newEntryIdMap.GetIdMap());
}
}  // namespace panda::ecmascript
