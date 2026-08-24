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
 * WITHOUT WARRANTIES OR CONDITIONS of ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/dfx/hprof/dynamic_dump.h"
#include "ecmascript/dfx/hprof/file_stream.h"
#include "common_components/heap/heap.h"
#include "ecmascript/base/config.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/runtime_lock.h"
#if defined(ENABLE_DUMP_IN_FAULTLOG)
#include "faultloggerd_client.h"
#endif

#include <unistd.h>
#include <vector>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/runtime.h"

namespace panda::ecmascript {

using common::dump::DumpExecutionMode;
using common::dump::DumpIdentity;
using common::dump::DumpReason;
using common::dump::DumpScope;

class DynamicDump::CrossThreadExecutionScope final {
public:
    explicit CrossThreadExecutionScope(JSThread *thread) : thread_(thread)
    {
        ASSERT(thread_ != nullptr);
        enabledByScope_ = thread_->CheckMultiThread();
        if (enabledByScope_) {
            thread_->SetCrossThreadExecution(true);
        }
    }

    ~CrossThreadExecutionScope()
    {
        if (enabledByScope_) {
            thread_->SetCrossThreadExecution(false);
        }
    }

    NO_COPY_SEMANTIC(CrossThreadExecutionScope);
    NO_MOVE_SEMANTIC(CrossThreadExecutionScope);

private:
    JSThread *thread_;
    bool enabledByScope_ {false};
};

class DynamicDump::CrossThreadExecutionScopes final {
public:
    CrossThreadExecutionScopes(EcmaVM *vm, bool isProcessDump)
    {
        ASSERT(vm != nullptr);
        if (!isProcessDump) {
            scopes_.emplace_back(std::make_unique<CrossThreadExecutionScope>(vm->GetAssociatedJSThread()));
            return;
        }
        Runtime::GetInstance()->GCIterateThreadList([this](JSThread *thread) {
            scopes_.emplace_back(std::make_unique<CrossThreadExecutionScope>(thread));
        });
    }

    NO_COPY_SEMANTIC(CrossThreadExecutionScopes);
    NO_MOVE_SEMANTIC(CrossThreadExecutionScopes);

private:
    std::vector<std::unique_ptr<CrossThreadExecutionScope>> scopes_;
};

class DynamicDump::RuntimeScope final {
public:
    RuntimeScope(EcmaVM *vm, bool runtimeAlreadySuspended, bool isProcessDump) : ownerPid_(getpid())
    {
        if (!runtimeAlreadySuspended) {
            SuspendRuntime();
        }
        PrepareHeaps(vm, isProcessDump, runtimeAlreadySuspended);
    }

    ~RuntimeScope()
    {
        if (getpid() != ownerPid_) {
            // The child inherited guards for parent-owned runtime state.
            // Do not resume threads or unlock copied synchronization objects.
            (void)externalSuspendGuard_.release();
            (void)suspendGuard_.release();
            (void)externalRuntimeLock_.release();
            (void)runtimeLock_.release();
            (void)managedScope_.release();
            return;
        }

        externalSuspendGuard_.reset();
        suspendGuard_.reset();
        externalRuntimeLock_.reset();
        runtimeLock_.reset();
        managedScope_.reset();
    }

private:
    void SuspendRuntime()
    {
        JSThread *current = JSThread::GetCurrent();
        auto &suspensionMutex = SharedHeap::GetInstance()->GetSuspensionRequestMutex();
        if (current == nullptr) {
            externalRuntimeLock_ = std::make_unique<LockHolder>(suspensionMutex);
            externalSuspendGuard_ = std::make_unique<SuspendAllScopeFromExternal>(nullptr);
            return;
        }

        managedScope_ = std::make_unique<ThreadManagedScope<JSThread>>(current);
        runtimeLock_ = std::make_unique<RuntimeLockHolder>(current, suspensionMutex);
        suspendGuard_ = std::make_unique<SuspendAllScope<JSThread>>(current);
    }

    static void PrepareHeaps(EcmaVM *vm, bool isProcessDump, bool fromSharedGC)
    {
        if (g_isEnableCMCGC) {
            common::Heap::GetHeap().WaitForGCFinish();
            return;
        }

        CrossThreadExecutionScopes executionScopes(vm, isProcessDump);
        auto prepareLocalHeaps = [vm, isProcessDump]() {
            if (!isProcessDump) {
                vm->GetHeap()->Prepare();
                return;
            }
            Runtime::GetInstance()->GCIterateThreadList([](JSThread *jsThread) {
                const_cast<Heap *>(jsThread->GetEcmaVM()->GetHeap())->Prepare();
            });
        };
        auto prepareSharedHeap = [vm, fromSharedGC]() {
            if (fromSharedGC) {
                SharedHeap::GetInstance()->PrepareByJSThread(vm->GetAssociatedJSThread(), true);
                return;
            }
            JSThread *current = JSThread::GetCurrent();
            if (current == nullptr) {
                SharedHeap::GetInstance()->Prepare(true);
                return;
            }
            SharedHeap::GetInstance()->PrepareByJSThread(current, true);
        };

        if (fromSharedGC) {
            // The shared-GC OOM route reaches this scope while the shared heap
            // owns the stop-the-world state, so preserve its preparation order.
            prepareSharedHeap();
            prepareLocalHeaps();
        } else {
            prepareLocalHeaps();
            prepareSharedHeap();
        }
        Runtime::GetInstance()->GCIterateThreadList([](JSThread *jsThread) {
            ASSERT(jsThread->IsSuspended() || jsThread->HasLaunchedSuspendAll());
            const_cast<Heap *>(jsThread->GetEcmaVM()->GetHeap())->FillBumpPointerForTlab();
            ASSERT(!jsThread->IsConcurrentCopying());
        });
    }

    pid_t ownerPid_ {-1};
    std::unique_ptr<ThreadManagedScope<JSThread>> managedScope_;
    std::unique_ptr<RuntimeLockHolder> runtimeLock_;
    std::unique_ptr<LockHolder> externalRuntimeLock_;
    std::unique_ptr<SuspendAllScope<JSThread>> suspendGuard_;
    std::unique_ptr<SuspendAllScopeFromExternal> externalSuspendGuard_;
};

DynamicDump::DynamicDump(EcmaVM *vm, const DumpRequest &request)
    : vm_(vm), request_(request)
{
}

bool DynamicDump::IsDynamicOOM() const
{
    return request_.reason == DumpReason::DYNAMIC_LOCAL_OOM ||
           request_.reason == DumpReason::DYNAMIC_SHARED_OOM ||
           request_.reason == DumpReason::DYNAMIC_SHARED_GC_OOM;
}

std::unique_ptr<AbstractDumper> DynamicDump::Create(EcmaVM *vm, const DumpRequest &request)
{
    if (vm == nullptr) {
        LOG_ECMA(ERROR) << "[HybDump][Dyn] Dumper creation failed: VM unavailable";
        return nullptr;
    }
    auto dumper = std::make_unique<DynamicDump>(vm, request);
    LOG_ECMA(INFO) << "[HybDump][Dyn] Dumper created";
    return dumper;
}

DynamicDump::~DynamicDump()
{
    // Keep the runtime suspended until the VM no longer exposes a profiler
    // owned by this dump. Resuming first would let allocation or GC paths race
    // with HeapProfilerInterface::Destroy(vm_).
    // Order: rawHeapDump (uses stream) -> close fd -> snapshot/stringTable -> profiler.
    delete rawHeapDump_;
    rawHeapDump_ = nullptr;
    outputStream_.reset();
    if (outputFd_ >= 0) {
        close(outputFd_);
        outputFd_ = -1;
    }
    snapshot_.reset();
    stringTable_.reset();
    if (heapProfilerOwnership_ == HeapProfilerOwnership::STANDALONE) {
        HeapProfilerInterface::DestroyInstance(heapProfiler_);
    } else if (heapProfilerOwnership_ == HeapProfilerOwnership::VM) {
        HeapProfilerInterface::Destroy(vm_);
    }
    heapProfiler_ = nullptr;
    runtimeScope_.reset();
    LOG_ECMA(INFO) << "[HybDump][Dyn] Dumper destroyed";
}

void DynamicDump::CompleteCrossRuntimeGC()
{
    // XGC resumes every local heap independently and may leave asynchronous
    // reclamation tasks running. Wait for all of them before Shared GC consumes
    // local-to-shared remembered sets; otherwise it can read a slot while its
    // local object is being converted to a FreeObject.
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *jsThread) {
        const_cast<Heap *>(jsThread->GetEcmaVM()->GetHeap())->WaitClearTaskFinished();
    });
}

void DynamicDump::TriggerGC()
{
    JSThread *thread = JSThread::GetCurrent();
    if (thread == nullptr || thread != vm_->GetAssociatedJSThread()) {
        LOG_ECMA(ERROR) << "[HybDump][Dyn] GC failed: current thread does not own target VM";
        return;
    }
    ThreadManagedScope<JSThread> managedScope(thread);
    if (g_isEnableCMCGC) {
        common::BaseRuntime::RequestGC(common::GC_REASON_BACKUP, false, common::GC_TYPE_FULL);
        return;
    }
    vm_->GetHeap()->CollectGarbage(TriggerGCType::FULL_GC);
    SharedHeap *sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    sHeap->GetSweeper()->WaitAllTaskFinished();
}

void DynamicDump::PrepareSession()
{
    if (runtimeScope_ != nullptr) {
        return;
    }
    LOG_ECMA(INFO) << "[HybDump][Dyn] Session prepare begin";
    runtimeScope_ = std::make_unique<RuntimeScope>(vm_, request_.reason == DumpReason::DYNAMIC_SHARED_GC_OOM,
                                                   request_.policy.scope == DumpScope::PROCESS);
    if (!InitializeHeapProfiler()) {
        LOG_ECMA(ERROR) << "[HybDump][Dyn] Session prepare failed: heap profiler unavailable";
        return;
    }
    LOG_ECMA(INFO) << "[HybDump][Dyn] Session prepare end";
}

bool DynamicDump::InitializeHeapProfiler()
{
    if (heapProfiler_ != nullptr) {
        return true;
    }
    if (request_.reason == DumpReason::DYNAMIC_SHARED_GC_OOM) {
        heapProfiler_ = static_cast<HeapProfiler *>(HeapProfilerInterface::CreateNewInstance(vm_));
        if (heapProfiler_ != nullptr) {
            heapProfilerOwnership_ = HeapProfilerOwnership::STANDALONE;
        }
    } else {
        if (IsDynamicOOM() && vm_->GetHeapProfile() != nullptr) {
            LOG_ECMA(ERROR) << "[HybDump][Dyn] Heap profiler creation failed: already active";
            return false;
        }
        bool createHeapProfiler = vm_->GetHeapProfile() == nullptr;
        heapProfiler_ = static_cast<HeapProfiler *>(HeapProfilerInterface::GetInstance(vm_));
        if (createHeapProfiler && heapProfiler_ != nullptr) {
            heapProfilerOwnership_ = HeapProfilerOwnership::VM;
        }
    }
    return heapProfiler_ != nullptr;
}

bool DynamicDump::AcquireOutput()
{
    if (outputStream_ != nullptr || outputFd_ >= 0) {
        return true;
    }
    if (!request_.output.dynamicPath.empty()) {
        outputStream_ = std::make_unique<FileStream>(request_.output.dynamicPath);
        if (!outputStream_->Good()) {
            outputStream_.reset();
            LOG_ECMA(ERROR) << "[HybDump][Dyn] Output file open failed";
            return false;
        }
        return true;
    }
    if (!request_.identity.IsValid()) {
        LOG_ECMA(ERROR) << "[HybDump][Dyn] Output fd acquire failed: invalid dump identity";
        return false;
    }
    LOG_ECMA(INFO) << "[HybDump][Dyn] Output fd acquire begin";
#if defined(ENABLE_DUMP_IN_FAULTLOG)
    FaultLoggerdRequest fdRequest = {};
    fdRequest.type = static_cast<int32_t>(FaultLoggerType::JS_RAW_SNAPSHOT);
    fdRequest.pid = request_.identity.GetPid();
    fdRequest.tid = request_.policy.scope == DumpScope::PROCESS ? DumpIdentity::UNSPECIFIED_ID
                                                                : request_.identity.GetTid();
    fdRequest.time = request_.identity.GetTimestampMillis();
    int fd = RequestFileDescriptorEx(&fdRequest);
#else
    int fd = -1;
#endif
    if (fd < 0) {
        LOG_ECMA(ERROR) << "[HybDump][Dyn] Output fd acquire failed: faultlogger request failed";
        return false;
    }
    LOG_ECMA(INFO) << "[HybDump][Dyn] Output fd acquired: fd=" << fd;
    outputFd_ = fd;
    return true;
}

bool DynamicDump::CreateOutputStream()
{
    if (outputStream_ != nullptr) {
        return true;
    }
    if (outputFd_ < 0) {
        return false;
    }
    outputStream_ = std::make_unique<FileDescriptorStream>(outputFd_);
    outputFd_ = -1;
    return true;
}

DumpSnapShotOption DynamicDump::CreateDumpOption() const
{
    DumpSnapShotOption option;
    bool isDynamicOOM = IsDynamicOOM();
    option.dumpFormat = DumpFormat::BINARY;
    option.isFullGC = false;
    option.isSimplify = isDynamicOOM;
    option.isSync = request_.policy.executionMode == DumpExecutionMode::IN_PROCESS;
    option.isBeforeFill = false;
    option.isDumpOOM = isDynamicOOM;
    option.isForSharedOOM = request_.reason == DumpReason::DYNAMIC_SHARED_OOM ||
                            request_.reason == DumpReason::DYNAMIC_SHARED_GC_OOM;
    option.isProcDump = request_.policy.scope == DumpScope::PROCESS;
    // DynamicDump is only used by the hybrid coordinator. OOM rawheap IDs must
    // therefore be address-resolvable for the static-side XRef records.
    option.isForHybridXRef = isDynamicOOM;
    option.spaceType = request_.oom.spaceType;
    option.heapType = request_.oom.heapType;
    return option;
}

bool DynamicDump::CreateRawHeapDump()
{
    if (rawHeapDump_ != nullptr) {
        return true;
    }
    if (heapProfiler_ == nullptr) {
        LOG_ECMA(ERROR) << "[HybDump][Dyn] Raw heap dumper creation failed: heap profiler unavailable";
        return false;
    }
    if (!CreateOutputStream()) {
        LOG_ECMA(ERROR) << "[HybDump][Dyn] Raw heap dumper creation failed: output unavailable";
        return false;
    }

    EntryIdMap *entryIdMap = heapProfiler_->GetEntryIdMap();
    DumpSnapShotOption option = CreateDumpOption();
    stringTable_ = std::make_unique<StringHashMap>(vm_);
    snapshot_ = std::make_unique<HeapSnapshot>(vm_, stringTable_.get(), option, false, entryIdMap);
    RawHeapDumpCropLevel cropLevel = Runtime::GetInstance()->GetRawHeapDumpCropLevel();
    if (cropLevel == RawHeapDumpCropLevel::LEVEL_V2) {
        rawHeapDump_ = new RawHeapDumpV2(vm_, outputStream_.get(), snapshot_.get(), entryIdMap, option);
    } else {
        rawHeapDump_ = new RawHeapDumpV1(vm_, outputStream_.get(), snapshot_.get(), entryIdMap, option);
    }
    return true;
}

bool DynamicDump::Execute()
{
    if (rawHeapDump_ == nullptr) {
        LOG_ECMA(ERROR) << "[HybDump][Dyn] Dump failed: output is not open";
        return false;
    }
    LOG_ECMA(INFO) << "[HybDump][Dyn] Dump begin";

    rawHeapDump_->BinaryDump();
    LOG_ECMA(INFO) << "[HybDump][Dyn] Dump end: success=true, objects=" << rawHeapDump_->GetObjectCount();
    return true;
}

DumpResult DynamicDump::Dump()
{
    CrossThreadExecutionScopes executionScopes(vm_, request_.policy.scope == DumpScope::PROCESS);
    if (!AcquireOutput() || !CreateRawHeapDump()) {
        return {{0, 0}, false};
    }
    return {{0, 0}, Execute()};
}

}  // namespace panda::ecmascript
