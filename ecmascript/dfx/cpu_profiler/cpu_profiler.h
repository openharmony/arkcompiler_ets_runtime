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

#ifndef ECMASCRIPT_DFX_CPU_PROFILER_CPU_PROFILER_H
#define ECMASCRIPT_DFX_CPU_PROFILER_CPU_PROFILER_H

#include <csignal>

#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_thread.h"

#include "os/mutex.h"

namespace panda::ecmascript {
const int THRESHOLD_GROWTH_FACTORY = 2; // 2:TimeDelta Threshold Growth Factory
const int THRESHOLD_FIXED_INCREMENT = 2000; // 2000:TimeDelta Threshold Fixed Increment
using JSTaggedType = uint64_t;
class SamplesRecord;

struct CurrentProcessInfo {
    uint64_t nowTimeStamp = 0;
    uint64_t tts = 0;
    pid_t pid = 0;
    pthread_t tid = 0;
};

class GcStateScope {
public:
    inline explicit GcStateScope(JSThread *thread)
    {
        thread_ = thread;
        thread_->SetGcState(true);
    }

    inline ~GcStateScope()
    {
        thread_->SetGcState(false);
    }
private:
    JSThread *thread_ = nullptr;
};

class RuntimeStateScope {
public:
    inline explicit RuntimeStateScope(JSThread *thread)
    {
        thread_ = thread;
        oldState_ = thread_->GetRuntimeState();
        thread_->SetRuntimeState(true);
    }

    inline ~RuntimeStateScope()
    {
        thread_->SetRuntimeState(oldState_);
    }
private:
    bool oldState_ = false;
    JSThread *thread_ = nullptr;
};

class CpuProfiler {
public:
    static const int CPUPROFILER_DEFAULT_INTERVAL = 500; // 500:Default Sampling interval 500 microseconds
    static const int INTERVAL_OF_ACTIVE_SAMPLING = 300; // 300:interval of active sampling
    static const int INTERVAL_OF_INNER_START = 100; // 100:interval of inner start(stake in runtime) sampling

    bool InHeaderOrTail(uint64_t pc, uint64_t entryBegin, uint64_t entryDuration, uint64_t headerSize,
                        uint64_t tailSize) const;
    bool IsEntryFrameHeaderOrTail(JSThread *thread, uint64_t pc) const;
    bool GetStackBeforeCallNapi(JSThread *thread);
    void GetStackAfterCallNapi(JSThread *thread);
    bool GetStackCallNapi(JSThread *thread, bool beforeCallNapi);
    static void GetStackSignalHandler(int signal, siginfo_t *siginfo, void *context);

    void StartCpuProfilerForInfo();
    std::unique_ptr<struct ProfileInfo> StopCpuProfilerForInfo();
    void StartCpuProfilerForFile(const std::string &fileName);
    void StopCpuProfilerForFile();
    void SetCpuSamplingInterval(int interval);
    void RecordCallNapiInfo(const std::string &methodAddr);
    void SetBuildNapiStack(bool flag);
    bool GetBuildNapiStack();
    explicit CpuProfiler(const EcmaVM *vm, const int interval = CPUPROFILER_DEFAULT_INTERVAL);
    virtual ~CpuProfiler();

    static CMap<pthread_t, const EcmaVM *> profilerMap_;
private:
    static os::memory::Mutex synchronizationMutex_;

    void GetStack(FrameIterator &it);
    static uint64_t GetPcFromContext(void *context);
    bool IsAddrAtStubOrAot(uint64_t pc) const;
    void SetProfileStart(uint64_t nowTimeStamp);
    void GetCurrentProcessInfo(struct CurrentProcessInfo &currentProcessInfo);
    bool CheckFileName(const std::string &fileName, std::string &absoluteFilePath) const;
    bool isProfiling_ = false;
    bool outToFile_ = false;
    std::string fileName_ = "";
    SamplesRecord *generator_ = nullptr;
    pthread_t tid_ = 0;
    const EcmaVM *vm_ = nullptr;
    uint32_t interval_ = 0;
    uint64_t beforeCallNapiTimeStamp_ = 0;
    std::atomic_bool isBuildNapiStack_ {false};
    bool enableVMTag_ {false};
};

class CallNapiScope {
public:
    inline explicit CallNapiScope(CpuProfiler *profiler)
    {
        profiler_ = profiler;
        profiler_->SetBuildNapiStack(true);
    }

    inline ~CallNapiScope()
    {
        profiler_->SetBuildNapiStack(false);
    }
private:
    CpuProfiler *profiler_ {nullptr};
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_DFX_CPU_PROFILER_CPU_PROFILER_H