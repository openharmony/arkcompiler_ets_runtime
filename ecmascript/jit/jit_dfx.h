/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JIT_JIT_DFX_H
#define ECMASCRIPT_JIT_JIT_DFX_H

#include <fstream>
#include <map>
#include <atomic>

#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/jit/compile_decision.h"

namespace panda::ecmascript {
using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<uint64_t, std::nano>;

struct JitEventParams {
    std::atomic<int> totalBaselineJitTimes_;
    std::atomic<int> totalFastoptJitTimes_;
    std::atomic<int> jitDeoptTimes_;
    std::atomic<int> longtimeLockTimes_;
    std::atomic<int> singleTimeOnMainThread_;
    std::atomic<int> totalTimeOnMainThread_;
    std::atomic<int> singleTimeOnJitThread_;
    std::atomic<int> totalTimeOnJitThread_;
    std::atomic<int> totalLockHoldingTime_;
    std::atomic<int> maxLockHoldingTime_;
    Clock::time_point start_;
    Clock::time_point blockUIEventstart_;

    JitEventParams() : totalBaselineJitTimes_(0), totalFastoptJitTimes_(0), jitDeoptTimes_(0),
        longtimeLockTimes_(0), singleTimeOnMainThread_(0), totalTimeOnMainThread_(0),
        singleTimeOnJitThread_(0), totalTimeOnJitThread_(0), totalLockHoldingTime_(0),
        maxLockHoldingTime_(0), start_(Clock::now()), blockUIEventstart_(Clock::now()) {}
};

class Method;
class JitDfx {
public:
    using ThreadId = uint32_t;
    static JitDfx *GetInstance();
    void Init(const JSRuntimeOptions &options, std::string &bundleName);
    void EnableDump();
    bool IsEnableDump() const
    {
        return isEnableDump_;
    }

    std::ostream &GetLogFileStream();

    void DumpBytecodeInst(Method *method);
    void TraceJitCode(Method *method, bool isEntry);

    void SetBundleName(CString bundleName)
    {
        bundleName_ = bundleName;
    }

    CString GetBundleName() const
    {
        return bundleName_;
    }

    void SetPidNumber(ThreadId number)
    {
        pidNum_ = number;
    }

    ThreadId GetPidNumber() const
    {
        return pidNum_;
    }

    void RecordSpentTimeAndPrintStatsLogInJsThread(int time)
    {
        SetTotalTimeOnMainThread(time);
        PrintJitStatsLog();
    }

    void RecordSpentTimeAndPrintStatsLogInJitThread(int compilerTime, CString methodName, bool isBaselineJit,
        int mainThreadCompileTime)
    {
        SetTotalTimeOnJitThread(compilerTime);
        if (ReportBlockUIEvent(mainThreadCompileTime)) {
            SetBlockUIEventInfo(methodName, isBaselineJit, mainThreadCompileTime, compilerTime);
        }
        PrintJitStatsLog();
    }

    void SetTotalTimeOnMainThread(int time)
    {
        jitEventParams.totalTimeOnMainThread_.fetch_add(time);
    }

    int GetTotalTimeOnMainThread() const
    {
        return jitEventParams.totalTimeOnMainThread_.load() / CONVERT_TO_MILLISECOND;
    }

    void SetSingleTimeOnMainThread(int time)
    {
        jitEventParams.singleTimeOnMainThread_.store(time);
    }

    int GetSingleTimeOnMainThread() const
    {
        return jitEventParams.singleTimeOnMainThread_.load() / CONVERT_TO_MILLISECOND;
    }

    void SetTotalTimeOnJitThread(int time)
    {
        jitEventParams.totalTimeOnJitThread_.fetch_add(time);
    }

    int GetTotalTimeOnJitThread() const
    {
        return jitEventParams.totalTimeOnJitThread_.load() / CONVERT_TO_MILLISECOND;
    }

    void SetSingleTimeOnJitThread(int time)
    {
        jitEventParams.singleTimeOnJitThread_.store(time);
    }

    int GetSingleTimeOnJitThread() const
    {
        return jitEventParams.singleTimeOnJitThread_.load() / CONVERT_TO_MILLISECOND;
    }

    void SetTriggerCount(CompilerTier tier)
    {
        if (tier.IsBaseLine()) {
            jitEventParams.totalBaselineJitTimes_.fetch_add(1);
        } else {
            jitEventParams.totalFastoptJitTimes_.fetch_add(1);
        }
    }

    int GetTotalBaselineJitCount() const
    {
        return jitEventParams.totalBaselineJitTimes_.load();
    }

    int GetTotalFastoptJitCount() const
    {
        return jitEventParams.totalFastoptJitTimes_.load();
    }

    void SetIsBaselineJit(bool isBaselineJit)
    {
        isBaselineJit_ = isBaselineJit;
    }

    bool GetIsBaselineJit() const
    {
        return isBaselineJit_;
    }

    void SetMethodInfo(CString method)
    {
        methodInfo_ = method;
    }

    CString GetMethodInfo() const
    {
        return methodInfo_;
    }

    void SetLockHoldingTime(int time)
    {
        jitEventParams.totalLockHoldingTime_.fetch_add(time);
        if (time > jitEventParams.maxLockHoldingTime_.load()) {
            jitEventParams.maxLockHoldingTime_.store(time);
        }
        if (time > HOLD_LOCK_LIMIT) {
            jitEventParams.longtimeLockTimes_.fetch_add(1);
        }
    }

    int GetTotalLockHoldingTime() const
    {
        return jitEventParams.totalLockHoldingTime_.load() / CONVERT_TO_MILLISECOND;
    }

    int GetMaxLockHoldingTime() const
    {
        return jitEventParams.maxLockHoldingTime_.load() / CONVERT_TO_MILLISECOND;
    }

    int GetLongtimeLockCount() const
    {
        return jitEventParams.longtimeLockTimes_.load();
    }

    void SetJitDeoptCount()
    {
        jitEventParams.jitDeoptTimes_.fetch_add(1);
    }

    int GetJitDeoptCount() const
    {
        return jitEventParams.jitDeoptTimes_.load();
    }

    void ResetCompilerTime()
    {
        jitEventParams.start_ = Clock::now();
    }

    void ResetBlockUIEventTime()
    {
        jitEventParams.blockUIEventstart_ = Clock::now();
    }

    bool ReportBlockUIEvent(int time)
    {
        return std::chrono::duration_cast<std::chrono::seconds>(Clock::now() -
            jitEventParams.blockUIEventstart_).count() >= MIN_SEND_INTERVAL && (time >= MAX_OCCUPY_MAIN_THREAD_TIME);
    }

    void SetBlockUIEventInfo(CString info, bool isBaselineJit, int mainThreadTime, int jitThreadTime)
    {
        SetMethodInfo(info);
        SetIsBaselineJit(isBaselineJit);
        SetSingleTimeOnMainThread(mainThreadTime);
        SetSingleTimeOnJitThread(jitThreadTime);
        PrintJitBlockUILog();
    }

    void PrintJitStatsLog();
    void PrintJitBlockUILog();

private:
    void OpenLogFile(uint32_t threadId);
    void InitializeRecord();
    void InitializeBlockUIRecord();
    void SendJitStatsEvent() const;
    void SendJitBlockUIEvent() const;
    bool checkUploadConditions() const
    {
        return std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - jitEventParams.start_).count() >=
            MIN_SEND_INTERVAL && (jitEventParams.totalBaselineJitTimes_.load() +
            jitEventParams.totalFastoptJitTimes_.load()) >= MAX_TRIGGER_TIMES;
    }

    static JitDfx instance;
    bool isEnableDump_ {false};
    bool isBaselineJit_ {true};
    std::map<uint32_t, std::ofstream> logFiles_;
    static thread_local uint32_t prefixOffset_;

    CString bundleName_ = "";
    CString methodInfo_ = "";
    ThreadId pidNum_ {0};
    JitEventParams jitEventParams;
    static constexpr int MAX_TRIGGER_TIMES = 100;
    static constexpr int MIN_SEND_INTERVAL = 60; // seconds
    static constexpr int HOLD_LOCK_LIMIT = 1000; // microseconds
    static constexpr int MAX_OCCUPY_MAIN_THREAD_TIME = 3000; // microseconds
    static constexpr int CONVERT_TO_MILLISECOND = 1000;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JIT_JIT_DFX_H
