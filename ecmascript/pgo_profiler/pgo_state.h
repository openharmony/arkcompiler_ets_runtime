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

#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript::pgo {
class PGOProfiler;
class PGOState {
public:
    enum class State : uint8_t {
        START,
        PAUSE,
        SAVE,
        STOP,
        FORCE_SAVE_START,
        FORCE_SAVE_PAUSE,
    };

    static std::string ToString(State state);
    State GetState() const;
    void SetState(State state);
    bool IsGcWaiting() const;
    bool IsStop() const;
    bool IsStart() const;
    void SuspendByGC();
    void SetStopIfStartAndNotify();
    void WaitDumpIfStart();
    void WaitDump();
    void SetStopIfSaveAndNotify();
    void SetStartIfStopAndDispatchDumpTask(PGOProfiler* profiler);
    void ResumeByGC(PGOProfiler* profiler);
    void ForceDump(PGOProfiler* profiler);

    bool SetStartIfStop();
    bool SetStopIfStart();
    bool SetStopIfSave();
    bool SetPauseIfStartByGC();
    bool SetStartIfPauseByGC();
    void NotifyAll();
    void IncrementGcCount();
    void DecrementGcCount();
    bool GcCountIsZero() const;
    int GetGcCount() const;

private:
    bool TryTransitionState(State expected, State desired);

    std::atomic<State> state_ {State::STOP};
    Mutex pgoDumpMutex_;
    ConditionVariable pgoDumpCondition_;
    std::atomic_int gcCount_ {0};
};
} // namespace panda::ecmascript::pgo

#endif // ECMASCRIPT_PGO_PROFILER_PGO_STATE_H