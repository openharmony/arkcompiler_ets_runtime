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

#include "ecmascript/pgo_profiler/pgo_state.h"

#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"

namespace panda::ecmascript::pgo {
using State = PGOState::State;

std::string PGOState::ToString(State state)
{
    switch (state) {
        case State::STOP:
            return "STOP";
        case State::SAVE:
            return "SAVE";
        case State::PAUSE:
            return "PAUSE";
        case State::START:
            return "START";
        case State::FORCE_SAVE_START:
            return "FORCE_SAVE_START";
        case State::FORCE_SAVE_PAUSE:
            return "FORCE_SAVE_PAUSE";
        default:
            return "UNKNOWN";
    }
}

State PGOState::GetState() const
{
    return state_.load(std::memory_order_acquire);
}

void PGOState::SetState(State state)
{
    state_.store(state, std::memory_order_release);
}

bool PGOState::SetStartIfStop()
{
    return TryTransitionState(State::STOP, State::START);
}

bool PGOState::SetStopIfStart()
{
    return TryTransitionState(State::START, State::STOP);
}

bool PGOState::SetPauseIfStartByGC()
{
    State expected = GetState();
    State desired;

    if (expected == State::START) {
        desired = State::PAUSE;
    } else if (expected == State::FORCE_SAVE_START) {
        desired = State::FORCE_SAVE_PAUSE;
    } else if (expected == State::PAUSE || expected == State::FORCE_SAVE_PAUSE) {
        return true;
    } else {
        return false;
    }

    return TryTransitionState(expected, desired);
}

bool PGOState::SetStartIfPauseByGC()
{
    State expected = GetState();
    State desired;

    if (expected == State::PAUSE) {
        desired = State::START;
    } else if (expected == State::FORCE_SAVE_PAUSE) {
        desired = State::FORCE_SAVE_START;
    } else {
        return false;
    }

    return TryTransitionState(expected, desired);
}

bool PGOState::IsGcWaiting() const
{
    State state = GetState();
    return state == State::PAUSE || state == State::FORCE_SAVE_PAUSE;
}

bool PGOState::IsStop() const
{
    State state = GetState();
    return state == State::STOP;
}

bool PGOState::IsStart() const
{
    State state = GetState();
    return state == State::START;
}

bool PGOState::TryTransitionState(State expected, State desired)
{
    if (state_.compare_exchange_strong(expected, desired, std::memory_order_acq_rel)) {
        LOG_PGO(DEBUG) << ToString(expected) << " -> " << ToString(desired);
        return true;
    }
    return false;
}

bool PGOState::SetStopIfSave()
{
    return TryTransitionState(State::SAVE, State::STOP);
}

void PGOState::SuspendByGC()
{
    LockHolder lock(pgoDumpMutex_);
    if (SetPauseIfStartByGC() && GcCountIsZero()) {
        WaitDump();
    }
    IncrementGcCount();
}

void PGOState::ResumeByGC(PGOProfiler* profiler)
{
    LockHolder lock(pgoDumpMutex_);
    DecrementGcCount();
    if (!GcCountIsZero()) {
        return;
    }
    if (SetStartIfPauseByGC()) {
        profiler->DispatchPGODumpTask();
    }
}

void PGOState::SetStartIfStopAndDispatchDumpTask(PGOProfiler* profiler)
{
    LockHolder lock(pgoDumpMutex_);
    if (!GcCountIsZero()) {
        return;
    }
    if (SetStartIfStop()) {
        profiler->DispatchPGODumpTask();
    }
}

void PGOState::SetStopIfStartAndNotify()
{
    LockHolder lock(pgoDumpMutex_);
    SetStopIfStart();
    NotifyAll();
}

void PGOState::WaitDumpIfStart()
{
    LockHolder lock(pgoDumpMutex_);
    if (IsStart()) {
        WaitDump();
    }
}

void PGOState::SetStopIfSaveAndNotify()
{
    LockHolder lock(pgoDumpMutex_);
    SetStopIfSave();
    NotifyAll();
}

void PGOState::WaitDump()
{
    pgoDumpCondition_.Wait(&pgoDumpMutex_);
}

void PGOState::NotifyAll()
{
    pgoDumpCondition_.SignalAll();
}

void PGOState::IncrementGcCount()
{
    gcCount_++;
}

void PGOState::DecrementGcCount()
{
    gcCount_--;
}

bool PGOState::GcCountIsZero() const
{
    return gcCount_ == 0;
}

int PGOState::GetGcCount() const
{
    return gcCount_;
}

void PGOState::ForceDump(PGOProfiler* profiler)
{
    LockHolder lock(pgoDumpMutex_);
    auto state = GetState();
    if (state == State::START) {
        SetState(State::FORCE_SAVE_START);
        WaitDump();
    } else if (state == State::STOP) {
        SetState(State::FORCE_SAVE_START);
    } else if (state == State::PAUSE) {
        SetState(State::FORCE_SAVE_PAUSE);
        WaitDump();
    }
    profiler->DispatchPGODumpTask();
    WaitDump();
}
} // namespace panda::ecmascript::pgo
