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

#ifndef ECMASCRIPT_NAPI_INCLUDE_DFX_JSNAPI_H
#define ECMASCRIPT_NAPI_INCLUDE_DFX_JSNAPI_H

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ecmascript/common.h"
#include "ecmascript/dfx/hprof/file_stream.h"

#include "libpandabase/macros.h"

namespace panda {
namespace ecmascript {
class EcmaVM;
class Stream;
class Progress;
struct ProfileInfo;
struct JsFrameInfo;
struct SamplingInfo;
}
class DFXJSNApi;
using EcmaVM = ecmascript::EcmaVM;
using Stream = ecmascript::Stream;
using Progress = ecmascript::Progress;
using ProfileInfo = ecmascript::ProfileInfo;
using JsFrameInfo = ecmascript::JsFrameInfo;
using SamplingInfo = ecmascript::SamplingInfo;
using DebuggerPostTask = std::function<void(std::function<void()> &&)>;

class PUBLIC_API DFXJSNApi {
public:
    // progress pointer is used to report the object number for IDE.
    // isVmMode means the internal class in vm is visible. isPrivate means the number and string is not visible.
    static void DumpHeapSnapshot(const EcmaVM *vm, int dumpFormat, const std::string &path, bool isVmMode = true,
                                 bool isPrivate = false, bool captureNumericValue = false);
    static void DumpHeapSnapshot(const EcmaVM *vm, int dumpFormat, Stream *stream, Progress *progress = nullptr,
                                 bool isVmMode = true, bool isPrivate = false, bool captureNumericValue = false);
    static void DumpHeapSnapshot(const EcmaVM *vm, int dumpFormat, bool isVmMode = true, bool isPrivate = false,
                                 bool captureNumericValue = false);
    static void DestroyHeapProfiler(const EcmaVM *vm);

    static bool BuildNativeAndJsStackTrace(const EcmaVM *vm, std::string &stackTraceStr);
    static bool BuildJsStackTrace(const EcmaVM *vm, std::string &stackTraceStr);
    static bool StartHeapTracking(const EcmaVM *vm, double timeInterval, bool isVmMode = true,
                                  Stream *stream = nullptr, bool traceAllocation = false, bool newThread = true);
    static bool UpdateHeapTracking(const EcmaVM *vm, Stream *stream);
    static bool StopHeapTracking(const EcmaVM *vm, const std::string &filePath, bool newThread = true);
    static bool StopHeapTracking(const EcmaVM *vm, Stream *stream, Progress *progress = nullptr, bool newThread = true);
    static void PrintStatisticResult(const EcmaVM *vm);
    static void StartRuntimeStat(EcmaVM *vm);
    static void StopRuntimeStat(EcmaVM *vm);
    static size_t GetArrayBufferSize(const EcmaVM *vm);
    static size_t GetHeapTotalSize(const EcmaVM *vm);
    static size_t GetHeapUsedSize(const EcmaVM *vm);
    static void NotifyApplicationState(EcmaVM *vm, bool inBackground);
    static void NotifyIdleStatusControl(const EcmaVM *vm, std::function<void(bool)> callback);
    static void NotifyIdleTime(const EcmaVM *vm, int idleMicroSec);
    static void NotifyMemoryPressure(EcmaVM *vm, bool inHighMemoryPressure);
    static bool BuildJsStackInfoList(const EcmaVM *hostVm, uint32_t tid, std::vector<JsFrameInfo>& jsFrames);

    // cpuprofiler
    static bool StopCpuProfilerForColdStart(const EcmaVM *vm);
    static bool CpuProfilerSamplingAnyTime(const EcmaVM *vm);
    static void StartCpuProfilerForFile(const EcmaVM *vm, const std::string &fileName,
                                        int interval = 500); // 500:Default Sampling interval 500 microseconds
    static void StopCpuProfilerForFile(const EcmaVM *vm);
    static void StartCpuProfilerForInfo(const EcmaVM *vm,
                                        int interval = 500); // 500:Default Sampling interval 500 microseconds
    static std::unique_ptr<ProfileInfo> StopCpuProfilerForInfo(const EcmaVM *vm);

    enum class PUBLIC_API ProfilerType : uint8_t { CPU_PROFILER, HEAP_PROFILER };

    struct ProfilerOption {
        const char *libraryPath;
        int interval = 500; // 500:Default Sampling interval 500 microseconds
        ProfilerType profilerType = ProfilerType::CPU_PROFILER;
    };
    static bool StartProfiler(EcmaVM *vm, const ProfilerOption &option, int32_t instanceId,
        const DebuggerPostTask &debuggerPostTask);
    static void SetCpuSamplingInterval(const EcmaVM *vm, int interval);
    static bool StartSampling(const EcmaVM *vm, uint64_t samplingInterval);
    static const SamplingInfo *GetAllocationProfile(const EcmaVM *vm);
    static void StopSampling(const EcmaVM *vm);

    static void ResumeVM(const EcmaVM *vm);
    static bool SuspendVM(const EcmaVM *vm);
    static bool IsSuspended(const EcmaVM *vm);
    static bool CheckSafepoint(const EcmaVM *vm);
    static void ResumeVMById(EcmaVM *vm, uint32_t tid);
    static bool SuspendVMById(EcmaVM *vm, uint32_t tid);
};
}
#endif