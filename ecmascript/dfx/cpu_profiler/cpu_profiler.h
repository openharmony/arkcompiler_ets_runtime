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

#ifndef ECMASCRIPT_CPU_PROFILER_H
#define ECMASCRIPT_CPU_PROFILER_H

#include <csignal>

#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_thread.h"

#include "os/mutex.h"

namespace panda::ecmascript {
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

class CpuProfiler {
public:
    bool ParseMethodInfo(Method *method, FrameHandler &frameHandler, bool isCallNapi);
    void GetNativeStack(FrameHandler &frameHandler, char *functionName, size_t size);
    void GetFrameStack(FrameHandler &frameHandler);
    void IsNeedAndGetStack(JSThread *thread);
    void GetStackBeforeCallNapi(JSThread *thread);
    static void GetStackSignalHandler(int signal, siginfo_t *siginfo, void *context);

    void StartCpuProfilerForInfo();
    std::unique_ptr<struct ProfileInfo> StopCpuProfilerForInfo();
    void StartCpuProfilerForFile(const std::string &fileName);
    void StopCpuProfilerForFile();
    void SetCpuSamplingInterval(int interval);
    std::string GetProfileName() const;
    explicit CpuProfiler(const EcmaVM *vm);
    virtual ~CpuProfiler();

    static CMap<pthread_t, const EcmaVM *> profilerMap_;
private:
    static os::memory::Mutex synchronizationMutex_;

    bool IsAddrAtStub(void *context);
    bool CheckFrameType(JSThread *thread, JSTaggedType *sp);
    void SetProfileStart(uint64_t nowTimeStamp);
    void GetCurrentProcessInfo(struct CurrentProcessInfo &currentProcessInfo);
    bool CheckFileName(const std::string &fileName, std::string &absoluteFilePath) const;
    bool CheckAndCopy(char *dest, size_t length, const char *src) const;
    bool GetFrameStackCallNapi(FrameHandler &frameHandler);

    bool isProfiling_ = false;
    bool outToFile_ = false;
    int interval_ = 500; // 500:Sampling interval 500 microseconds
    std::string fileName_ = "";
    SamplesRecord *generator_ = nullptr;
    pthread_t tid_ = 0;
    const EcmaVM *vm_ = nullptr;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_CPU_PROFILE_H