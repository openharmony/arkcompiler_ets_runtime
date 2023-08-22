/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/dfx/hprof/file_stream.h"

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#endif
#if defined(ENABLE_DUMP_IN_FAULTLOG)
#include "faultloggerd_client.h"
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
using ecmascript::Stream;
using ecmascript::CMap;

void DFXJSNApi::DumpHeapSnapshot([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] int dumpFormat,
                                 [[maybe_unused]] const std::string &path, [[maybe_unused]] bool isVmMode,
                                 [[maybe_unused]] bool isPrivate, [[maybe_unused]] bool captureNumericValue)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    FileStream stream(path);
    DumpHeapSnapshot(vm, dumpFormat, &stream, nullptr, isVmMode, isPrivate, captureNumericValue);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap snapshot";
#endif
}

void DFXJSNApi::DumpHeapSnapshot([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] int dumpFormat,
                                 [[maybe_unused]] Stream *stream, [[maybe_unused]] Progress *progress,
                                 [[maybe_unused]] bool isVmMode, [[maybe_unused]] bool isPrivate,
                                 [[maybe_unused]] bool captureNumericValue)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ecmascript::base::BlockHookScope blockScope;
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));
    heapProfile->DumpHeapSnapshot(ecmascript::DumpFormat(dumpFormat), stream, progress,
                                  isVmMode, isPrivate, captureNumericValue);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap snapshot";
#endif
}

[[maybe_unused]] static uint8_t killCount = 0;

void DFXJSNApi::DumpHeapSnapshot([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] int dumpFormat,
                                 [[maybe_unused]] bool isVmMode, [[maybe_unused]] bool isPrivate,
                                 [[maybe_unused]] bool captureNumericValue)
{
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
    int32_t fd = RequestFileDescriptor(static_cast<int32_t>(FaultLoggerType::JS_HEAP_SNAPSHOT));
    if (fd < 0) {
        LOG_ECMA(ERROR) << "Write FD failed, fd" << fd;
        return;
    }
    FileDescriptorStream stream(fd);
    DumpHeapSnapshot(vm, dumpFormat, &stream, nullptr, isVmMode, isPrivate, captureNumericValue);
#endif // ENABLE_DUMP_IN_FAULTLOG
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap snapshot";
#endif // ECMASCRIPT_SUPPORT_SNAPSHOT
}

void DFXJSNApi::DestroyHeapProfiler([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    ecmascript::HeapProfilerInterface::Destroy(const_cast<EcmaVM *>(vm));
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler heap snapshot";
#endif
}

bool DFXJSNApi::BuildNativeAndJsStackTrace(const EcmaVM *vm, std::string &stackTraceStr)
{
    LOG_ECMA(INFO) <<"BuildJsStackInfoList start";
    std::vector<JsFrameInfo> jf = ecmascript::JsStackInfo::BuildJsStackInfo(vm->GetAssociatedJSThread());
    LOG_ECMA(INFO) <<"BuildJsStackInfoList JsFrameInfo";
    for (uint32_t i = 0; i < jf.size(); ++i) {
        std::string functionname = jf[i].functionName;
        std::string filename = jf[i].fileName;
        std::string pos = jf[i].pos;
        uintptr_t *nativepointer = jf[i].nativePointer;
        LOG_ECMA(INFO) << "BuildJsStackInfoList functionname: " << functionname;
        LOG_ECMA(INFO) << "BuildJsStackInfoList filenaem: " << filename;
        LOG_ECMA(INFO) << "BuildJsStackInfoList pos: " << pos;
        LOG_ECMA(INFO) << "BuildJsStackInfoList nativepointer: " << nativepointer;
    }
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
    return vm->GetHeap()->GetArrayBufferSize();
}

size_t DFXJSNApi::GetHeapTotalSize(const EcmaVM *vm)
{
    return vm->GetHeap()->GetCommittedSize();
}

size_t DFXJSNApi::GetHeapUsedSize(const EcmaVM *vm)
{
    return vm->GetHeap()->GetHeapObjectSize();
}

void DFXJSNApi::NotifyApplicationState(EcmaVM *vm, bool inBackground)
{
    const_cast<ecmascript::Heap *>(vm->GetHeap())->ChangeGCParams(inBackground);
}

void DFXJSNApi::NotifyIdleStatusControl(const EcmaVM *vm, std::function<void(bool)> callback)
{
    const_cast<ecmascript::Heap *>(vm->GetHeap())->InitializeIdleStatusControl(callback);
}

void DFXJSNApi::NotifyIdleTime(const EcmaVM *vm, int idleMicroSec)
{
    const_cast<ecmascript::Heap *>(vm->GetHeap())->TriggerIdleCollection(idleMicroSec);
}

void DFXJSNApi::NotifyMemoryPressure(EcmaVM *vm, bool inHighMemoryPressure)
{
    const_cast<ecmascript::Heap *>(vm->GetHeap())->NotifyMemoryPressure(inHighMemoryPressure);
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

bool DFXJSNApi::CpuProfilerSamplingAnyTime([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    (void)killCount;
    bool success = false;
    const uint8_t KILL_COUNT_FACTOR = 2;
    auto &options = const_cast<EcmaVM *>(vm)->GetJSOptions();
    if (options.EnableCpuProfilerAnyTimeMainThread()) {
        success = true;
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

void DFXJSNApi::StartCpuProfilerForFile([[maybe_unused]] const EcmaVM *vm,
                                        [[maybe_unused]] const std::string &fileName,
                                        [[maybe_unused]] int interval)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (vm == nullptr) {
        return;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        profiler = new CpuProfiler(vm, interval);
        const_cast<EcmaVM *>(vm)->SetProfiler(profiler);
    }
    profiler->StartCpuProfilerForFile(fileName);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler cpu profiler";
#endif
}

void DFXJSNApi::StopCpuProfilerForFile([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (vm == nullptr) {
        return;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        return;
    }
    profiler->StopCpuProfilerForFile();
    delete profiler;
    profiler = nullptr;
    const_cast<EcmaVM *>(vm)->SetProfiler(nullptr);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler cpu profiler";
#endif
}

void DFXJSNApi::StartCpuProfilerForInfo([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] int interval)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (vm == nullptr) {
        return;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        profiler = new CpuProfiler(vm, interval);
        const_cast<EcmaVM *>(vm)->SetProfiler(profiler);
    }
    profiler->StartCpuProfilerForInfo();
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler cpu profiler";
#endif
}

std::unique_ptr<ProfileInfo> DFXJSNApi::StopCpuProfilerForInfo([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (vm == nullptr) {
        return nullptr;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        return nullptr;
    }
    auto profile = profiler->StopCpuProfilerForInfo();
    if (profile == nullptr) {
        LOG_DEBUGGER(ERROR) << "Transfer CpuProfiler::StopCpuProfilerImpl is failure";
    }
    delete profiler;
    profiler = nullptr;
    const_cast<EcmaVM *>(vm)->SetProfiler(nullptr);
    return profile;
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler cpu profiler";
    return nullptr;
#endif
}

void DFXJSNApi::SetCpuSamplingInterval([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] int interval)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
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

bool DFXJSNApi::StartProfiler(EcmaVM *vm, const ProfilerOption &option, int32_t instanceId,
                              const DebuggerPostTask &debuggerPostTask)
{
    JSNApi::DebugOption debugOption;
    debugOption.libraryPath = option.libraryPath;
    if (option.profilerType == ProfilerType::CPU_PROFILER) {
        debugOption.isDebugMode = false;
        if (JSNApi::StartDebugger(vm, debugOption, instanceId, debuggerPostTask)) {
            StartCpuProfilerForInfo(vm, option.interval);
            return true;
        } else {
            LOG_ECMA(ERROR) << "DFXJSNApi:Failed to StartDebugger";
            return false;
        }
    } else {
        debugOption.isDebugMode = true;
        return JSNApi::StartDebugger(vm, debugOption, instanceId, debuggerPostTask);
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
} // namespace panda
