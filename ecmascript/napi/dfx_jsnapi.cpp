/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/tooling/interface/file_stream.h"
#include "ecmascript/tooling/interface/stream.h"

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#endif
#if defined(ENABLE_DUMP_IN_FAULTLOG)
#include "include/faultloggerd_client.h"
#endif

namespace panda {
using ecmascript::CString;
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
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

void DFXJSNApi::DumpHeapSnapshot(const EcmaVM *vm, int dumpFormat,
                                 const std::string &path, bool isVmMode, bool isPrivate)
{
    FileStream stream(path);
    DumpHeapSnapshot(vm, dumpFormat, &stream, nullptr, isVmMode, isPrivate);
}

void DFXJSNApi::DumpHeapSnapshot(const EcmaVM *vm, int dumpFormat, Stream *stream, Progress *progress,
                                 bool isVmMode, bool isPrivate)
{
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));
    heapProfile->DumpHeapSnapshot(ecmascript::DumpFormat(dumpFormat), stream, progress, isVmMode, isPrivate);
    ecmascript::HeapProfilerInterface::Destroy(const_cast<EcmaVM *>(vm));
}

void DFXJSNApi::DumpHeapSnapshot([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] int dumpFormat,
                                 [[maybe_unused]] bool isVmMode, [[maybe_unused]] bool isPrivate)
{
#if defined(ENABLE_DUMP_IN_FAULTLOG)
    // Write in faultlog for heap leak.
    int32_t fd = RequestFileDescriptor(static_cast<int32_t>(FaultLoggerType::JS_HEAP_SNAPSHOT));
    if (fd < 0) {
        LOG_ECMA(ERROR) << "Write FD failed, fd" << fd;
        return;
    }
    FileDescriptorStream stream(fd);
    DumpHeapSnapshot(vm, dumpFormat, &stream, nullptr, isVmMode, isPrivate);
#endif
}

bool DFXJSNApi::BuildNativeAndJsStackTrace(const EcmaVM *vm, std::string &stackTraceStr)
{
    LOG_ECMA(INFO) <<"BuildJsStackInfoList start";
    std::vector<JsFrameInfo> jf = ecmascript::JsStackInfo::BuildJsStackInfo(vm->GetAssociatedJSThread());
    LOG_ECMA(INFO) <<"BuildJsStackInfoList JsFrameInfo";
    for (uint32_t i = 0; i < jf.size(); ++i) {
        std::string functionname = jf[i].functionname;
        std::string filename = jf[i].filename;
        std::string lines = jf[i].lines;
        std::string nativepointer = jf[i].nativepointer;
        LOG_ECMA(INFO) << "BuildJsStackInfoList functionname: " << functionname;
        LOG_ECMA(INFO) << "BuildJsStackInfoList filenaem: " << filename;
        LOG_ECMA(INFO) << "BuildJsStackInfoList lines: " << lines;
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

bool DFXJSNApi::StartHeapTracking(const EcmaVM *vm, double timeInterval, bool isVmMode,
                                  Stream *stream, bool traceAllocation)
{
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));
    return heapProfile->StartHeapTracking(timeInterval, isVmMode, stream, traceAllocation);
}

bool DFXJSNApi::StopHeapTracking(const EcmaVM *vm, const std::string &filePath)
{
    FileStream stream(filePath);
    return StopHeapTracking(vm, &stream, nullptr);
}

bool DFXJSNApi::StopHeapTracking(const EcmaVM *vm, Stream* stream, Progress *progress)
{
    bool result = false;
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(
        const_cast<EcmaVM *>(vm));
    result = heapProfile->StopHeapTracking(stream, progress);
    ecmascript::HeapProfilerInterface::Destroy(const_cast<EcmaVM *>(vm));
    return result;
}

void DFXJSNApi::PrintStatisticResult(const EcmaVM *vm)
{
    ecmascript::GCStats gcstats(vm->GetHeap());
    gcstats.PrintStatisticResult(true);
}

void DFXJSNApi::StartRuntimeStat(EcmaVM *vm)
{
    vm->SetRuntimeStatEnable(true);
}

void DFXJSNApi::StopRuntimeStat(EcmaVM *vm)
{
    vm->SetRuntimeStatEnable(false);
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

void DFXJSNApi::NotifyMemoryPressure(EcmaVM *vm, bool inHighMemoryPressure)
{
    const_cast<ecmascript::Heap *>(vm->GetHeap())->NotifyMemoryPressure(inHighMemoryPressure);
}
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
void DFXJSNApi::StartCpuProfilerForFile(const EcmaVM *vm, const std::string &fileName)
{
    if (vm == nullptr) {
        return;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        profiler = new CpuProfiler(vm);
        const_cast<EcmaVM *>(vm)->SetProfiler(profiler);
    }
    profiler->StartCpuProfilerForFile(fileName);
}

void DFXJSNApi::StopCpuProfilerForFile(const EcmaVM *vm)
{
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
}

void DFXJSNApi::StartCpuProfilerForInfo(const EcmaVM *vm)
{
    if (vm == nullptr) {
        return;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        profiler = new CpuProfiler(vm);
        const_cast<EcmaVM *>(vm)->SetProfiler(profiler);
    }
    profiler->StartCpuProfilerForInfo();
}

std::unique_ptr<ProfileInfo> DFXJSNApi::StopCpuProfilerForInfo(const EcmaVM *vm)
{
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
}

void DFXJSNApi::SetCpuSamplingInterval(const EcmaVM *vm, int interval)
{
    if (vm == nullptr) {
        return;
    }
    CpuProfiler *profiler = vm->GetProfiler();
    if (profiler == nullptr) {
        return;
    }
    profiler = new CpuProfiler(vm);
    const_cast<EcmaVM *>(vm)->SetProfiler(profiler);
    profiler->SetCpuSamplingInterval(interval);
}
#endif

bool DFXJSNApi::SuspendVM(const EcmaVM *vm)
{
    ecmascript::VmThreadControl* vmThreadControl = vm->GetAssociatedJSThread()->GetVmThreadControl();
    return vmThreadControl->NotifyVMThreadSuspension();
}

void DFXJSNApi::ResumeVM(const EcmaVM *vm)
{
    ecmascript::VmThreadControl* vmThreadControl = vm->GetAssociatedJSThread()->GetVmThreadControl();
    vmThreadControl->ResumeVM();
}

bool DFXJSNApi::IsSuspended(const EcmaVM *vm)
{
    ecmascript::VmThreadControl* vmThreadControl = vm->GetAssociatedJSThread()->GetVmThreadControl();
    return vmThreadControl->IsSuspended();
}

bool DFXJSNApi::CheckSafepoint(const EcmaVM *vm)
{
    ecmascript::JSThread* thread = vm->GetJSThread();
    return  thread->CheckSafepoint();
}

std::vector<JsFrameInfo> DFXJSNApi::BuildJsStackInfoList(const EcmaVM *vm)
{
    std::vector<JsFrameInfo> jsframeinfo = ecmascript::JsStackInfo::BuildJsStackInfo(vm->GetAssociatedJSThread());
    return jsframeinfo;
}
} // namespace panda
