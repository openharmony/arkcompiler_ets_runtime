/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/napi/include/dfx_jsnapi.h"

#include "ecmascript/base/block_hook_scope.h"
#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/debugger/debugger_api.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/dfx/tracing/tracing.h"
#include "ecmascript/dfx/stackinfo/async_stack_trace.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/dfx/vm_thread_control.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#endif
#if defined(ENABLE_DUMP_IN_FAULTLOG)
#include "faultloggerd_client.h"
#include "uv.h"
#endif

namespace panda {
using ecmascript::CString;
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
using BuiltinsArkTools = ecmascript::builtins::BuiltinsArkTools;
using ecmascript::CpuProfiler;
#endif
using ecmascript::EcmaString;
using ecmascript::JSTaggedValue;
using ecmascript::GCStats;
template<typename T>
using JSHandle = ecmascript::JSHandle<T>;
using ecmascript::FileStream;
using ecmascript::FileDescriptorStream;
using ecmascript::CMap;
using ecmascript::Tracing;
using ecmascript::DumpSnapShotOption;
sem_t g_heapdumpCnt;

void DFXJSNApi::DumpHeapSnapshot([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] const std::string &path,
                                 [[maybe_unused]] const DumpSnapShotOption &dumpOption)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    FileStream stream(path);
    DumpHeapSnapshot(vm, &stream, dumpOption);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap snapshot";
#endif
}

void DFXJSNApi::DumpHeapSnapshot([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] int& fd,
                                 [[maybe_unused]] const DumpSnapShotOption &dumpOption,
                                 [[maybe_unused]] const std::function<void(uint8_t)> &callback)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    FileDescriptorStream stream(fd);
    fd = -1;
    DumpHeapSnapshot(vm, &stream, dumpOption, nullptr, callback);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap snapshot";
#endif
}

// IDE interface.
void DFXJSNApi::DumpHeapSnapshot([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] Stream *stream,
                                 [[maybe_unused]] const DumpSnapShotOption &dumpOption,
                                 [[maybe_unused]] Progress *progress,
                                 [[maybe_unused]] std::function<void(uint8_t)> callback)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));

#if defined (ENABLE_LOCAL_HANDLE_LEAK_DETECT)
#if defined (ENABLE_DUMP_IN_FAULTLOG)
    auto heapProfiler = reinterpret_cast<ecmascript::HeapProfiler *>(heapProfile);
    heapProfiler->SwitchStartLocalHandleLeakDetect();
    if (heapProfiler->IsStartLocalHandleLeakDetect()) {
        int32_t fd = RequestFileDescriptor(static_cast<int32_t>(FaultLoggerType::JS_STACKTRACE));
        if (fd < 0) {
            LOG_ECMA(ERROR) << "[LocalHandleLeakDetect] Failed on request file descriptor";
        } else {
            heapProfiler->SetLeakStackTraceFd(fd);
        }
    }
#endif  // ENABLE_DUMP_IN_FAULTLOG
#endif  // ENABLE_LOCAL_HANDLE_LEAK_DETECT
    heapProfile->DumpHeapSnapshot(stream, dumpOption, progress, callback);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap snapshot";
#endif
}

[[maybe_unused]] static uint8_t killCount = 0;

void DFXJSNApi::DumpCpuProfile([[maybe_unused]] const EcmaVM *vm)
{
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
    vm->GetJSThread()->GetCurrentEcmaContext()->PrintMegaICStat();
#endif
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
#if defined(ENABLE_DUMP_IN_FAULTLOG)
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    // for CpuProfiler kill contral
    if (DFXJSNApi::StopCpuProfilerForColdStart(vm)) {
        return;
    }

    (void)killCount;
    if (DFXJSNApi::CpuProfilerSamplingAnyTime(vm)) {
        killCount++;
        return;
    }
#endif // ECMASCRIPT_SUPPORT_CPUPROFILER
#endif // ENABLE_DUMP_IN_FAULTLOG
#endif // ECMASCRIPT_SUPPORT_SNAPSHOT
}

// kill -39.
void DFXJSNApi::DumpHeapSnapshot([[maybe_unused]] const EcmaVM *vm,
                                 [[maybe_unused]] const DumpSnapShotOption &dumpOption)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
#if defined(ENABLE_DUMP_IN_FAULTLOG)
    sem_wait(&g_heapdumpCnt);
    auto &options = const_cast<EcmaVM *>(vm)->GetJSOptions();
    options.SwitchStartGlobalLeakCheck();
    if (options.EnableGlobalLeakCheck() && options.IsStartGlobalLeakCheck()) {
        int32_t stackTraceFd = RequestFileDescriptor(static_cast<int32_t>(FaultLoggerType::JS_STACKTRACE));
        if (stackTraceFd < 0) {
            options.SwitchStartGlobalLeakCheck();
        } else {
            vm->GetJSThread()->SetStackTraceFd(stackTraceFd);
        }
    }

    // Write in faultlog for heap leak.
    int32_t fd;
    if (dumpOption.isDumpOOM && dumpOption.dumpFormat == DumpFormat::BINARY) {
        fd = RequestFileDescriptor(static_cast<int32_t>(FaultLoggerType::JS_RAW_SNAPSHOT));
    } else {
        fd = RequestFileDescriptor(static_cast<int32_t>(FaultLoggerType::JS_HEAP_SNAPSHOT));
    }
    if (fd < 0) {
        LOG_ECMA(ERROR) << "Write FD failed, fd" << fd;
        return;
    }
    FileDescriptorStream stream(fd);
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));
    heapProfile->DumpHeapSnapshot(&stream, dumpOption);

    sem_post(&g_heapdumpCnt);
#endif // ENABLE_DUMP_IN_FAULTLOG
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap snapshot";
#endif // ECMASCRIPT_SUPPORT_SNAPSHOT
}

// tid = 0: dump all vm; tid != 0: dump tid vm, hidumper.
void DFXJSNApi::DumpHeapSnapshot([[maybe_unused]] const EcmaVM *vm,
                                 [[maybe_unused]] const DumpSnapShotOption &dumpOption, [[maybe_unused]] uint32_t tid)
{
    const int THREAD_COUNT = 1;
    if (vm->IsWorkerThread()) {
        LOG_ECMA(ERROR) << "this is a workthread!";
        return;
    }
    sem_init(&g_heapdumpCnt, 0, THREAD_COUNT);
    uint32_t curTid = vm->GetTid();
    LOG_ECMA(INFO) << "DumpHeapSnapshot tid " << tid << " curTid " << curTid;
    DumpHeapSnapshotWithVm(vm, dumpOption, tid);
}

void DFXJSNApi::DumpHeapSnapshotWithVm([[maybe_unused]] const EcmaVM *vm,
                                       [[maybe_unused]] const DumpSnapShotOption &dumpOption,
                                       [[maybe_unused]] uint32_t tid)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
#if defined(ENABLE_DUMP_IN_FAULTLOG)
    uv_loop_t *loop = reinterpret_cast<uv_loop_t *>(vm->GetLoop());
    if (loop == nullptr || uv_loop_alive(loop) == 0) {
        LOG_ECMA(ERROR) << "loop nullptr or uv_loop_alive dead";
        return;
    }
    struct DumpForSnapShotStruct *dumpStruct = new DumpForSnapShotStruct();
    dumpStruct->vm = vm;
    dumpStruct->dumpOption = dumpOption;
    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        LOG_ECMA(ERROR) << "work nullptr";
        delete dumpStruct;
        return;
    }
    work->data = static_cast<void *>(dumpStruct);

    uint32_t curTid = vm->GetTid();
    int ret = 0;
    if (tid == 0 || tid == curTid) {
        ret = uv_queue_work(loop, work, [](uv_work_t *) {}, [](uv_work_t *work, int32_t) {
            struct DumpForSnapShotStruct *dump = static_cast<struct DumpForSnapShotStruct *>(work->data);
            DFXJSNApi::GetHeapPrepare(dump->vm);
            DumpSnapShotOption dumpOption = dump->dumpOption;
            DumpHeapSnapshot(dump->vm, dumpOption);
            delete dump;
            delete work;
        });
    } else {
        delete dumpStruct;
        delete work;
    }

    // dump worker vm
    const_cast<EcmaVM *>(vm)->EnumerateWorkerVm([&](const EcmaVM *workerVm) -> void {
        uint32_t curTid = workerVm->GetTid();
        LOG_ECMA(INFO) << "DumpHeapSnapshot workthread curTid " << curTid;
        DumpHeapSnapshotWithVm(workerVm, dumpOption, tid);
        return;
    });

    if (ret != 0) {
        LOG_ECMA(ERROR) << "uv_queue_work fail ret " << ret;
        delete dumpStruct;
        delete work;
    }
#endif
#endif
}

void DFXJSNApi::GenerateHeapSnapshotByBinFile([[maybe_unused]] const EcmaVM *vm,
                                              [[maybe_unused]] std::string &inputFilePath,
                                              [[maybe_unused]] std::string &outputPath)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    auto *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(const_cast<EcmaVM *>(vm));
    heapProfile->GenerateHeapSnapshot(inputFilePath, outputPath);
#else
    LOG_ECMA(ERROR) << "Not support GenerateHeapSnapshotByBinFile";
#endif // ECMASCRIPT_SUPPORT_SNAPSHOT
}

// tid = 0: TriggerGC all vm; tid != 0: TriggerGC tid vm
void DFXJSNApi::TriggerGC([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] uint32_t tid)
{
    if (vm->IsWorkerThread()) {
        LOG_ECMA(ERROR) << "this is a workthread!";
        return;
    }
    // triggerGC host vm
    uint32_t curTid = vm->GetTid();
    LOG_ECMA(INFO) << "TriggerGC tid " << tid << " curTid " << curTid;
    if (tid == 0 || tid == curTid) {
        TriggerGCWithVm(vm);
    }
    // triggerGC worker vm
    const_cast<EcmaVM *>(vm)->EnumerateWorkerVm([&](const EcmaVM *workerVm) -> void {
        curTid = workerVm->GetTid();
        LOG_ECMA(INFO) << "TriggerGC tid " << tid << " curTid " << curTid;
        if (tid == 0 || tid == curTid) {
            TriggerGCWithVm(workerVm);
            return;
        }
    });
    // triggerSharedFullGC
    TriggerSharedGCWithVm(vm);
}

void DFXJSNApi::TriggerSharedGCWithVm([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
#if defined(ENABLE_DUMP_IN_FAULTLOG)
    uv_loop_t *loop = reinterpret_cast<uv_loop_t *>(vm->GetLoop());
    if (loop == nullptr) {
        LOG_ECMA(ERROR) << "loop nullptr";
        return;
    }
    if (uv_loop_alive(loop) == 0) {
        LOG_ECMA(ERROR) << "uv_loop_alive dead";
        return;
    }
    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        LOG_ECMA(FATAL) << "DFXJSNApi::TriggerGCWithVm:work is nullptr";
        return;
    }
    work->data = static_cast<void *>(const_cast<EcmaVM *>(vm));
    int ret = uv_queue_work(loop, work, [](uv_work_t *) {}, [](uv_work_t *work, int32_t) {
        EcmaVM *vm = static_cast<EcmaVM *>(work->data);
        ecmascript::SharedHeap* sHeap = ecmascript::SharedHeap::GetInstance();
        JSThread *thread = vm->GetJSThread();
        ecmascript::ThreadManagedScope managedScope(thread);
        sHeap->CollectGarbage<ecmascript::TriggerGCType::SHARED_FULL_GC,
            ecmascript::GCReason::TRIGGER_BY_MEM_TOOLS>(thread);
        delete work;
    });
    if (ret != 0) {
        LOG_ECMA(ERROR) << "uv_queue_work fail ret " << ret;
        delete work;
    }
#endif
#endif
}

void DFXJSNApi::TriggerGCWithVm([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
#if defined(ENABLE_DUMP_IN_FAULTLOG)
    uv_loop_t *loop = reinterpret_cast<uv_loop_t *>(vm->GetLoop());
    if (loop == nullptr) {
        LOG_ECMA(ERROR) << "loop nullptr";
        return;
    }
    if (uv_loop_alive(loop) == 0) {
        LOG_ECMA(ERROR) << "uv_loop_alive dead";
        return;
    }
    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        LOG_ECMA(FATAL) << "DFXJSNApi::TriggerGCWithVm:work is nullptr";
        return;
    }
    work->data = static_cast<void *>(const_cast<EcmaVM *>(vm));
    int ret = uv_queue_work(loop, work, [](uv_work_t *) {}, [](uv_work_t *work, int32_t) {
        EcmaVM *vm = static_cast<EcmaVM *>(work->data);
        ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
        vm->CollectGarbage(ecmascript::TriggerGCType::FULL_GC, ecmascript::GCReason::TRIGGER_BY_MEM_TOOLS);
        delete work;
    });
    if (ret != 0) {
        LOG_ECMA(ERROR) << "uv_queue_work fail ret " << ret;
        delete work;
    }
#endif
#endif
}

void DFXJSNApi::DestroyHeapProfiler([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ecmascript::HeapProfilerInterface::Destroy(const_cast<EcmaVM *>(vm));
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap snapshot";
#endif
}

bool DFXJSNApi::BuildNativeAndJsStackTrace(const EcmaVM *vm, std::string &stackTraceStr)
{
    stackTraceStr = ecmascript::JsStackInfo::BuildJsStackTrace(vm->GetAssociatedJSThread(), true);
    if (stackTraceStr.empty()) {
        return false;
    }
    return true;
}

bool DFXJSNApi::BuildJsStackTrace(const EcmaVM *vm, std::string &stackTraceStr)
{
    stackTraceStr = ecmascript::JsStackInfo::BuildJsStackTrace(vm->GetAssociatedJSThread(), false);
    if (stackTraceStr.empty()) {
        return false;
    }
    return true;
}

bool DFXJSNApi::StartHeapTracking([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] double timeInterval,
                                  [[maybe_unused]] bool isVmMode, [[maybe_unused]] Stream *stream,
                                  [[maybe_unused]] bool traceAllocation, [[maybe_unused]] bool newThread)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ecmascript::base::BlockHookScope blockScope;
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));
    return heapProfile->StartHeapTracking(timeInterval, isVmMode, stream, traceAllocation, newThread);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap tracking";
    return false;
#endif
}

bool DFXJSNApi::UpdateHeapTracking([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] Stream *stream)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ecmascript::base::BlockHookScope blockScope;
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));
    return heapProfile->UpdateHeapTracking(stream);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap tracking";
    return false;
#endif
}

bool DFXJSNApi::StopHeapTracking([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] const std::string &filePath,
                                 [[maybe_unused]] bool newThread)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    FileStream stream(filePath);
    return StopHeapTracking(vm, &stream, nullptr, newThread);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap tracking";
    return false;
#endif
}

bool DFXJSNApi::StopHeapTracking([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] Stream* stream,
                                 [[maybe_unused]] Progress *progress, [[maybe_unused]] bool newThread)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ecmascript::base::BlockHookScope blockScope;
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    bool result = false;
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));
    result = heapProfile->StopHeapTracking(stream, progress, newThread);
    ecmascript::HeapProfilerInterface::Destroy(const_cast<EcmaVM *>(vm));
    return result;
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap tracking";
    return false;
#endif
}

void DFXJSNApi::PrintStatisticResult(const EcmaVM *vm)
{
    ecmascript::GCStats gcstats(vm->GetHeap());
    gcstats.PrintStatisticResult();
}

void DFXJSNApi::StartRuntimeStat(EcmaVM *vm)
{
    vm->GetJSThread()->GetCurrentEcmaContext()->SetRuntimeStatEnable(true);
}

void DFXJSNApi::StopRuntimeStat(EcmaVM *vm)
{
    vm->GetJSThread()->GetCurrentEcmaContext()->SetRuntimeStatEnable(false);
}

size_t DFXJSNApi::GetArrayBufferSize(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return vm->GetHeap()->GetArrayBufferSize();
}

size_t DFXJSNApi::GetHeapTotalSize(const EcmaVM *vm)
{
    return vm->GetHeap()->GetCommittedSize();
}

size_t DFXJSNApi::GetHeapUsedSize(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return vm->GetHeap()->GetLiveObjectSize();
}

size_t DFXJSNApi::GetHeapObjectSize(const EcmaVM *vm)
{
    return vm->GetHeap()->GetHeapObjectSize();
}

size_t DFXJSNApi::GetHeapLimitSize(const EcmaVM *vm)
{
    return vm->GetHeap()->GetHeapLimitSize();
}

size_t DFXJSNApi::GetProcessHeapLimitSize()
{
    return ecmascript::MemMapAllocator::GetInstance()->GetCapacity();
}

size_t DFXJSNApi::GetGCCount(const EcmaVM *vm)
{
    if (vm->IsWorkerThread()) {
        return vm->GetEcmaGCStats()->GetGCCount();
    }
    return vm->GetEcmaGCStats()->GetGCCount() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCCount();
}

size_t DFXJSNApi::GetGCDuration(const EcmaVM *vm)
{
    if (vm->IsWorkerThread()) {
        return vm->GetEcmaGCStats()->GetGCDuration();
    }
    return vm->GetEcmaGCStats()->GetGCDuration() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetGCDuration();
}

size_t DFXJSNApi::GetAccumulatedAllocateSize(const EcmaVM *vm)
{
    if (vm->IsWorkerThread()) {
        return vm->GetEcmaGCStats()->GetAccumulatedAllocateSize();
    }
    return vm->GetEcmaGCStats()->GetAccumulatedAllocateSize() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetAccumulatedAllocateSize();
}

size_t DFXJSNApi::GetAccumulatedFreeSize(const EcmaVM *vm)
{
    if (vm->IsWorkerThread()) {
        return vm->GetEcmaGCStats()->GetAccumulatedFreeSize();
    }
    return vm->GetEcmaGCStats()->GetAccumulatedFreeSize() +
        ecmascript::SharedHeap::GetInstance()->GetEcmaGCStats()->GetAccumulatedFreeSize();
}

size_t DFXJSNApi::GetFullGCLongTimeCount(const EcmaVM *vm)
{
    return vm->GetEcmaGCStats()->GetFullGCLongTimeCount();
}

void DFXJSNApi::GetHeapPrepare(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    const_cast<ecmascript::Heap *>(vm->GetHeap())->GetHeapPrepare();
}

void DFXJSNApi::SetJsDumpThresholds([[maybe_unused]] EcmaVM *vm, [[maybe_unused]] size_t thresholds)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT) && defined(PANDA_TARGET_OHOS) && defined(ENABLE_HISYSEVENT)
    vm->GetHeap()->SetJsDumpThresholds(thresholds);
#else
    LOG_ECMA(ERROR) << "Not support set jsdump thresholds";
#endif
}

void DFXJSNApi::SetAppFreezeFilterCallback([[maybe_unused]] const EcmaVM *vm, AppFreezeFilterCallback cb)
{
    if (ecmascript::Runtime::GetInstance()->GetAppFreezeFilterCallback() == nullptr && cb != nullptr) {
        ecmascript::Runtime::GetInstance()->SetAppFreezeFilterCallback(cb);
    }
}

void DFXJSNApi::NotifyApplicationState(EcmaVM *vm, bool inBackground)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    const_cast<ecmascript::Heap *>(vm->GetHeap())->ChangeGCParams(inBackground);
    ecmascript::Jit::GetInstance()->ChangeTaskPoolState(inBackground);
}

void DFXJSNApi::NotifyIdleStatusControl(const EcmaVM *vm, std::function<void(bool)> callback)
{
    const_cast<ecmascript::Heap *>(vm->GetHeap())->InitializeIdleStatusControl(callback);
}

void DFXJSNApi::NotifyIdleTime(const EcmaVM *vm, int idleMicroSec)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    const_cast<ecmascript::Heap *>(vm->GetHeap())->TriggerIdleCollection(idleMicroSec);
}

void DFXJSNApi::NotifyMemoryPressure(EcmaVM *vm, bool inHighMemoryPressure)
{
    const_cast<ecmascript::Heap *>(vm->GetHeap())->NotifyMemoryPressure(inHighMemoryPressure);
}

void DFXJSNApi::NotifyFinishColdStart(EcmaVM *vm, [[maybe_unused]] bool isConvinced)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
}

void DFXJSNApi::NotifyHighSensitive(EcmaVM *vm, bool isStart)
{
    const_cast<ecmascript::Heap *>(vm->GetHeap())->NotifyHighSensitive(isStart);
}

bool DFXJSNApi::StopCpuProfilerForColdStart([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    bool success = false;
    auto &options = const_cast<EcmaVM *>(vm)->GetJSOptions();
    if (options.EnableCpuProfilerColdStartMainThread()) {
        success = true;
        DFXJSNApi::StopCpuProfilerForFile(vm);
    }

    if (options.EnableCpuProfilerColdStartWorkerThread()) {
        success = true;
        const_cast<EcmaVM *>(vm)->EnumerateWorkerVm([&](const EcmaVM *workerVm) -> void {
            if (workerVm->GetJSThread()->GetIsProfiling()) {
                DFXJSNApi::StopCpuProfilerForFile(workerVm);
            }
        });
    }

    return success;
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler cpu profiler";
    return false;
#endif
}

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
void DFXJSNApi::CpuProfilerAnyTimeMainThread(const EcmaVM *vm)
{
    const uint8_t KILL_COUNT_FACTOR = 2;
    if (killCount % KILL_COUNT_FACTOR == 0) {
        uint8_t fileCount = killCount / KILL_COUNT_FACTOR + 1;
        LOG_ECMA(INFO) << "Start CpuProfiler Any Time Main Thread, killCount = " << killCount;
        std::string fileName = ConvertToStdString(const_cast<EcmaVM *>(vm)->GetBundleName())
                               + "_" + std::to_string(fileCount) + ".cpuprofile";
        if (!BuiltinsArkTools::CreateFile(fileName)) {
            LOG_ECMA(ERROR) << "createFile failed " << fileName;
        } else {
            DFXJSNApi::StartCpuProfilerForFile(vm, fileName, CpuProfiler::INTERVAL_OF_INNER_START);
        }
    } else {
        LOG_ECMA(INFO) << "Stop CpuProfiler Any Time Main Thread, killCount = " << killCount;
        if (vm->GetJSThread()->GetIsProfiling()) {
            DFXJSNApi::StopCpuProfilerForFile(vm);
        }
    }
}
#endif

bool DFXJSNApi::CpuProfilerSamplingAnyTime([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    (void)killCount;
    bool success = false;
    const uint8_t KILL_COUNT_FACTOR = 2;
    auto &options = const_cast<EcmaVM *>(vm)->GetJSOptions();
    if (options.EnableCpuProfilerAnyTimeMainThread()) {
        success = true;
        CpuProfilerAnyTimeMainThread(vm);
    }

    if (options.EnableCpuProfilerAnyTimeWorkerThread()) {
        success = true;
        if (killCount % KILL_COUNT_FACTOR == 0) {
            uint8_t fileCount = killCount / KILL_COUNT_FACTOR + 1;
            LOG_ECMA(INFO) << "Start CpuProfiler Any Time Worker Thread, killCount = " << killCount;
            const_cast<EcmaVM *>(vm)->EnumerateWorkerVm([&](const EcmaVM *workerVm) -> void {
                auto *thread = workerVm->GetAssociatedJSThread();
                std::string fileName = ConvertToStdString(workerVm->GetBundleName()) + "_"
                                       + std::to_string(thread->GetThreadId()) + "_"
                                       + std::to_string(fileCount) + ".cpuprofile";
                if (!BuiltinsArkTools::CreateFile(fileName)) {
                    LOG_ECMA(ERROR) << "createFile failed " << fileName;
                } else {
                    thread->SetCpuProfileName(fileName);
                    thread->SetNeedProfiling(true);
                }
            });
        } else {
            LOG_ECMA(INFO) << "Stop CpuProfiler Any Time Worker Thread, killCount = " << killCount;
            const_cast<EcmaVM *>(vm)->EnumerateWorkerVm([&](const EcmaVM *workerVm) -> void {
                auto *thread = workerVm->GetAssociatedJSThread();
                if (thread->GetIsProfiling()) {
                    DFXJSNApi::StopCpuProfilerForFile(workerVm);
                }
                thread->SetNeedProfiling(false);
            });
        }
    }

    return success;
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler cpu profiler";
    return false;
#endif
}

bool DFXJSNApi::StartCpuProfilerForFile([[maybe_unused]] const EcmaVM *vm,
                                        [[maybe_unused]] const std::string &fileName,
                                        [[maybe_unused]] int interval)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    LOG_ECMA(INFO) << "DFXJSNApi::StartCpuProfilerForFile, vm = " << vm;
    if (interval <= 0) {
        LOG_ECMA(ERROR) << "DFXJSNApi::StartCpuProfilerForFile, interval <= 0";
        return false;
    }
    if (vm == nullptr) {
        LOG_ECMA(ERROR) << "DFXJSNApi::StartCpuProfilerForFile, vm == nullptr";
        return false;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        profiler = new CpuProfiler(vm, interval);
        const_cast<EcmaVM *>(vm)->SetProfiler(profiler);
    }
    return profiler->StartCpuProfilerForFile(fileName);
#else
    LOG_ECMA(ERROR) << "DFXJSNApi::StartCpuProfilerForFile, not support cpu profiler";
    return false;
#endif
}

void DFXJSNApi::StopCpuProfilerForFile([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    LOG_ECMA(INFO) << "DFXJSNApi::StopCpuProfilerForFile, vm = " << vm;
    if (vm == nullptr) {
        LOG_ECMA(ERROR) << "DFXJSNApi::StopCpuProfilerForFile, vm == nullptr";
        return;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        LOG_ECMA(ERROR) << "DFXJSNApi::StopCpuProfilerForFile, profiler == nullptr";
        return;
    }
    bool result = profiler->StopCpuProfilerForFile();
    if (!result) {
        LOG_ECMA(ERROR) << "DFXJSNApi::StopCpuProfilerForFile failed";
        return;
    }
    delete profiler;
    profiler = nullptr;
    const_cast<EcmaVM *>(vm)->SetProfiler(nullptr);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler cpu profiler";
#endif
}

bool DFXJSNApi::StartCpuProfilerForInfo([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] int interval)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    LOG_ECMA(INFO) << "DFXJSNApi::StartCpuProfilerForInfo, vm = " << vm;
    if (interval <= 0) {
        LOG_ECMA(ERROR) << "DFXJSNApi::StartCpuProfilerForInfo, interval <= 0";
        return false;
    }
    if (vm == nullptr) {
        LOG_ECMA(ERROR) << "DFXJSNApi::StartCpuProfilerForInfo, vm == nullptr";
        return false;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        profiler = new CpuProfiler(vm, interval);
        const_cast<EcmaVM *>(vm)->SetProfiler(profiler);
    }
    return profiler->StartCpuProfilerForInfo();
#else
    LOG_ECMA(ERROR) << "DFXJSNApi::StartCpuProfilerForInfo, not support cpu profiler";
    return false;
#endif
}

std::unique_ptr<ProfileInfo> DFXJSNApi::StopCpuProfilerForInfo([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    LOG_ECMA(INFO) << "DFXJSNApi::StopCpuProfilerForInfo, vm = " << vm;
    if (vm == nullptr) {
        LOG_ECMA(ERROR) << "DFXJSNApi::StopCpuProfilerForInfo, vm == nullptr";
        return nullptr;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        LOG_ECMA(ERROR) << "DFXJSNApi::StopCpuProfilerForInfo, profiler == nullptr";
        return nullptr;
    }
    std::unique_ptr<ProfileInfo> profileInfo;
    bool result = profiler->StopCpuProfilerForInfo(profileInfo);
    if (!result) {
        LOG_ECMA(ERROR) << "DFXJSNApi::StopCpuProfilerForInfo failed";
        return nullptr;
    }
    delete profiler;
    profiler = nullptr;
    const_cast<EcmaVM *>(vm)->SetProfiler(nullptr);
    return profileInfo;
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler cpu profiler";
    return nullptr;
#endif
}

void DFXJSNApi::SetCpuSamplingInterval([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] int interval)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (interval < 0) {
        LOG_ECMA(ERROR) << "Sampling interval is illegal";
        return;
    }
    LOG_ECMA(INFO) << "SetCpuProfilerSamplingInterval, Sampling interval is: " << interval;
    if (vm == nullptr) {
        return;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        profiler = new CpuProfiler(vm, interval);
        const_cast<EcmaVM *>(vm)->SetProfiler(profiler);
        return;
    }
    profiler->SetCpuSamplingInterval(interval);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler cpu profiler";
#endif
}

void DFXJSNApi::EnableSeriliazationTimeoutCheck(const EcmaVM *ecmaVM, int32_t threshold)
{
    ecmaVM->GetJsDebuggerManager()->EnableSerializationTimeoutCheck();
    ecmaVM->GetJsDebuggerManager()->SetSerializationCheckThreshold(threshold);
}

void DFXJSNApi::DisableSeriliazationTimeoutCheck(const EcmaVM *ecmaVM)
{
    ecmaVM->GetJsDebuggerManager()->DisableSerializationTimeoutCheck();
}

bool DFXJSNApi::SuspendVM([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ecmascript::VmThreadControl* vmThreadControl = vm->GetAssociatedJSThread()->GetVmThreadControl();
    return vmThreadControl->NotifyVMThreadSuspension();
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler snapshot";
    return false;
#endif
}

void DFXJSNApi::ResumeVM([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ecmascript::VmThreadControl* vmThreadControl = vm->GetAssociatedJSThread()->GetVmThreadControl();
    vmThreadControl->ResumeVM();
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler snapshot";
#endif
}

bool DFXJSNApi::IsSuspended([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ecmascript::VmThreadControl* vmThreadControl = vm->GetAssociatedJSThread()->GetVmThreadControl();
    return vmThreadControl->IsSuspended();
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler snapshot";
    return false;
#endif
}

void DFXJSNApi::TerminateExecution(const EcmaVM *vm)
{
    ecmascript::VmThreadControl* vmThreadControl = vm->GetAssociatedJSThread()->GetVmThreadControl();
    vmThreadControl->RequestTerminateExecution();
}

bool DFXJSNApi::CheckSafepoint([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ecmascript::JSThread* thread = vm->GetJSThread();
    return thread->CheckSafepoint();
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler snapshot";
    return false;
#endif
}

bool DFXJSNApi::BuildJsStackInfoList(const EcmaVM *hostVm, uint32_t tid, std::vector<JsFrameInfo>& jsFrames)
{
    EcmaVM *vm;
    if (hostVm->GetAssociatedJSThread()->GetThreadId() == tid) {
        vm = const_cast<EcmaVM*>(hostVm);
    } else {
        vm = const_cast<EcmaVM*>(hostVm)->GetWorkerVm(tid);
        if (vm == nullptr) {
            return false;
        }
    }
    jsFrames = ecmascript::JsStackInfo::BuildJsStackInfo(vm->GetAssociatedJSThread());
    if (jsFrames.size() > 0) {
        return true;
    }
    return false;
}

//When some objects invoke GetObjectHash, the return result is 0.
//The GetObjectHashCode function is added to rectify the fault.
int32_t DFXJSNApi::GetObjectHash(const EcmaVM *vm, Local<JSValueRef> nativeObject)
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(nativeObject);
    return ecmascript::tooling::DebuggerApi::GetObjectHash(vm, obj);
}

int32_t DFXJSNApi::GetObjectHashCode(const EcmaVM *vm, Local<JSValueRef> nativeObject)
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(nativeObject);
    return ecmascript::tooling::DebuggerApi::GetObjectHashCode(vm, obj);
}

bool DFXJSNApi::StartSampling([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] uint64_t samplingInterval)
{
#if defined(ECMASCRIPT_SUPPORT_HEAPSAMPLING)
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));
    return heapProfile->StartHeapSampling(samplingInterval);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap sampling";
    return false;
#endif
}

const SamplingInfo *DFXJSNApi::GetAllocationProfile([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_HEAPSAMPLING)
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));
    return heapProfile->GetAllocationProfile();
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap sampling";
    return nullptr;
#endif
}

void DFXJSNApi::StopSampling([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_HEAPSAMPLING)
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));
    heapProfile->StopHeapSampling();
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap sampling";
#endif
}

// release or debug hap : aa start -p 'dumpheap'
//                        aa start -p 'profile'
bool DFXJSNApi::StartProfiler(EcmaVM *vm, const ProfilerOption &option, int tid,
                              int32_t instanceId, const DebuggerPostTask &debuggerPostTask, bool isDebugApp)
{
    LOG_ECMA(INFO) << "DFXJSNApi::StartProfiler, type = " << (int)option.profilerType
        << ", tid = " << tid << ", isDebugApp = " << isDebugApp;
    JSNApi::DebugOption debugOption;
    debugOption.libraryPath = option.libraryPath;
    if (option.profilerType == ProfilerType::CPU_PROFILER) {
        debugOption.isDebugMode = false;
        if (JSNApi::NotifyDebugMode(tid, vm, debugOption, instanceId, debuggerPostTask, isDebugApp)) {
            return StartCpuProfilerForInfo(vm, option.interval);
        } else {
            LOG_ECMA(ERROR) << "DFXJSNApi::StartProfiler, NotifyDebugMode failed";
            return false;
        }
    } else {
        debugOption.isDebugMode = true;
        return JSNApi::NotifyDebugMode(tid, vm, debugOption, instanceId, debuggerPostTask, isDebugApp);
    }
}

void DFXJSNApi::ResumeVMById(EcmaVM *hostVm, uint32_t tid)
{
    if (hostVm->GetAssociatedJSThread()->GetThreadId() == tid) {
        ResumeVM(hostVm);
    } else {
        hostVm->ResumeWorkerVm(tid);
    }
}

bool DFXJSNApi::SuspendVMById(EcmaVM *hostVm, uint32_t tid)
{
    bool success = false;
    if (hostVm->GetAssociatedJSThread()->GetThreadId() == tid) {
        success = SuspendVM(hostVm);
        LOG_ECMA(INFO) << "The main thread, SuspendVMById succeeded: " << success;
        return success;
    } else {
        success = hostVm->SuspendWorkerVm(tid);
        LOG_ECMA(INFO) << "The worker thread, SuspendVMById succeeded: " << success;
        return success;
    }
}

bool DFXJSNApi::StartTracing([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] std::string &categories)
{
#if defined(ECMASCRIPT_SUPPORT_TRACING)
    if (vm == nullptr) {
        return false;
    }
    Tracing *tracing = vm->GetTracing();
    if (tracing == nullptr) {
        tracing = new Tracing(vm);
        const_cast<EcmaVM *>(vm)->SetTracing(tracing);
    }
    tracing->StartTracing(categories);
    return true;
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler tracing";
    return false;
#endif
}

std::unique_ptr<std::vector<TraceEvent>> DFXJSNApi::StopTracing([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_TRACING)
    if (vm == nullptr) {
        return nullptr;
    }
    Tracing *tracing = vm->GetTracing();
    if (tracing == nullptr) {
        LOG_ECMA(ERROR) << "StopTracing tracing is nullptr";
        return nullptr;
    }
    auto traceEvents = tracing->StopTracing();
    if (traceEvents == nullptr) {
        LOG_ECMA(ERROR) << "trace events is nullptr";
    }
    delete tracing;
    tracing = nullptr;
    const_cast<EcmaVM *>(vm)->SetTracing(nullptr);
    return traceEvents;
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler tracing";
    return nullptr;
#endif
}

void DFXJSNApi::GetTracingBufferUseage([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] double &percentFull,
                                       [[maybe_unused]] uint32_t &eventCount, [[maybe_unused]] double &value)
{
#if defined(ECMASCRIPT_SUPPORT_TRACING)
    if (vm == nullptr) {
        return;
    }
    ecmascript::Tracing *tracing = vm->GetTracing();
    if (tracing == nullptr) {
        LOG_ECMA(ERROR) << "GetTracingBufferUseage tracing is nullptr";
    } else {
        tracing->GetBufferUseage(percentFull, eventCount, value);
    }
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler tracing";
#endif
}

void DFXJSNApi::TranslateJSStackInfo(const EcmaVM *vm, std::string &url, int32_t &line, int32_t &column,
    std::string &packageName)
{
    auto cb = vm->GetSourceMapTranslateCallback();
    if (cb == nullptr) {
        LOG_ECMA(ERROR) << "Translate failed, callback function is nullptr.";
    } else if (!cb(url, line, column, packageName)) {
        LOG_ECMA(ERROR) << "Translate failed, url: " << url;
    }
}

uint32_t DFXJSNApi::GetCurrentThreadId()
{
    return JSThread::GetCurrentThreadId();
}

void DFXJSNApi::RegisterAsyncDetectCallBack(const EcmaVM *vm)
{
    vm->GetAsyncStackTrace()->RegisterAsyncDetectCallBack();
}
} // namespace panda
