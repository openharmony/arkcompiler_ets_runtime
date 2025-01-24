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

#ifndef ECMASCRIPT_PGO_PROFILER_PGO_STATE_H
#define ECMASCRIPT_PGO_PROFILER_PGO_STATE_H

#include <atomic>
#include <string>

#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/mutex.h"

#define PRINT_STATE_CHANGE false

namespace panda::ecmascript::pgo {
class PGOProfiler;
class PGOProfilerManager;
class PGOState {
public:
    PGOState();

    enum class State : uint8_t {
        START,
        SAVE,
        STOP,
    };

    enum class GCState : uint8_t {
        RUNNING,
        WAITING,
        STOP,
    };

    static std::string ToString(State state);
    static std::string ToString(GCState state);
    State GetState() const;
    void SetState(State state);
    GCState GetGCState() const;
    void SetGCState(GCState state);
    bool GCIsWaiting() const;
    bool GCIsRunning() const;
    bool GCIsStop() const;
    void SuspendByGC();
    void ResumeByGC(PGOProfiler* profiler);
    bool StateIsStop() const;
    bool StateIsStart() const;
    void SetStopAndNotify();
    bool SetStartIfStop();
    void WaitDump();
    void WaitGC();
    void NotifyAllDumpWaiters();
    void NotifyAllGCWaiters();
    bool TryChangeState(State expected, State desired);
    void StartDumpBeforeDestroy();

private:
#if PRINT_STATE_CHANGE
    template<typename T>
    void PrintStateChange(T expected, T desired)
    {
        LOG_PGO(INFO) << ToString(expected) << " -> " << ToString(desired);
    }

    template<typename T>
    void PrintStateChange(T original, T expected, T desired)
    {
        LOG_PGO(INFO) << ToString(original) << " -x " << ToString(desired) << ", " << ToString(expected);
    }
#endif

    std::atomic<State> state_ {State::STOP};
    std::atomic<GCState> gcState_ {GCState::STOP};
    ConditionVariable stateCondition_;
    ConditionVariable gcCondition_;
    PGOProfilerManager* manager_ {nullptr};
    std::atomic_bool needRedump_ {false};

protected:
    Mutex stateMutex_;
};
} // namespace panda::ecmascript::pgo

#endif // ECMASCRIPT_PGO_PROFILER_PGO_STATE_H