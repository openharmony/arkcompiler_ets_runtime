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

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"

namespace panda::ecmascript::pgo {
using State = PGOState::State;
using GCState = PGOState::GCState;
PGOState::PGOState()
{
    manager_ = PGOProfilerManager::GetInstance();
}

std::string PGOState::ToString(State state)
{
    switch (state) {
        case State::STOP:
            return "STOP";
        case State::SAVE:
            return "SAVE";
        case State::START:
            return "START";
        default:
            return "UNKNOWN";
    }
}

std::string PGOState::ToString(GCState state)
{
    switch (state) {
        case GCState::STOP:
            return "STOP";
        case GCState::WAITING:
            return "WAITING";
        case GCState::RUNNING:
            return "RUNNING";
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
#if PRINT_STATE_CHANGE
    PrintStateChange(GetState(), state);
#endif
    state_.store(state, std::memory_order_release);
}

bool PGOState::GCIsWaiting() const
{
    GCState gcState = GetGCState();
    return gcState == GCState::WAITING;
}

bool PGOState::GCIsRunning() const
{
    GCState gcState = GetGCState();
    return gcState == GCState::RUNNING;
}

bool PGOState::GCIsStop() const
{
    GCState gcState = GetGCState();
    return gcState == GCState::STOP;
}

bool PGOState::StateIsStop() const
{
    State state = GetState();
    return state == State::STOP;
}

bool PGOState::StateIsStart() const
{
    State state = GetState();
    return state == State::START;
}

bool PGOState::StateIsSave() const
{
    State state = GetState();
    return state == State::SAVE;
}

#if PRINT_STATE_CHANGE
bool PGOState::TryChangeState(State expected, State desired)
{
    auto original = expected;
    if (state_.compare_exchange_strong(expected, desired, std::memory_order_acq_rel)) {
        PrintStateChange(expected, desired);
        return true;
    }
    PrintStateChange(original, expected, desired);
    return false;
}
#endif

#if !PRINT_STATE_CHANGE
bool PGOState::TryChangeState(State expected, State desired)
{
    if (state_.compare_exchange_strong(expected, desired, std::memory_order_acq_rel)) {
        return true;
    }
    return false;
}
#endif

void PGOState::SetGCState(GCState state)
{
    gcState_.store(state, std::memory_order_release);
}

GCState PGOState::GetGCState() const
{
    return gcState_.load(std::memory_order_acquire);
}

void PGOState::SuspendByGC()
{
    LockHolder lock(stateMutex_);
    // possible state: START, SAVE, STOP
    if (StateIsStart()) {
        // possible gc state: STOP
        SetGCState(GCState::WAITING);
        needRedump_ = true;
        WaitDump();
    }
    // possible gc state: STOP, WAITING
    SetGCState(GCState::RUNNING);
}

void PGOState::ResumeByGC(PGOProfiler* profiler)
{
    LockHolder lock(stateMutex_);
    // possible state: START, SAVE, STOP
    SetGCState(GCState::STOP);
    if (needRedump_) {
        manager_->TryDispatchDumpTask(profiler);
        needRedump_ = false;
    }
    NotifyAllGCWaiters();
}

bool PGOState::SetStartIfStop()
{
    LockHolder lock(stateMutex_);
    // possible state: STOP, SAVE
    // possible gc state: STOP, WAITING, RUNNING
    if (GCIsStop() && TryChangeState(State::STOP, State::START)) {
        return true;
    }
    return false;
}

void PGOState::SetStopAndNotify()
{
    LockHolder lock(stateMutex_);
    // possible state: START, SAVE
    SetState(State::STOP);
    NotifyAllDumpWaiters();
}

void PGOState::SetSaveAndNotify()
{
    LockHolder lock(stateMutex_);
    // possible state: START
    SetState(State::SAVE);
    NotifyAllDumpWaiters();
}

void PGOState::StartDumpBeforeDestroy([[maybe_unused]] JSThread *thread)
{
    LockHolder lock(stateMutex_);
    // possible state: STOP, SAVE, START
    // may notify after change to STOP and SAVE, we need to make sure state is STOP
    while (!StateIsStop()) {
        WaitDump();
    }
    // possible gc state: STOP, WAITING, RUNNING
    if (!GCIsStop()) {
        WaitGC();
    }
    SetState(State::START);
}

void PGOState::WaitDump()
{
    stateCondition_.Wait(&stateMutex_);
}

void PGOState::NotifyAllDumpWaiters()
{
    stateCondition_.SignalAll();
}

void PGOState::WaitGC()
{
    gcCondition_.Wait(&stateMutex_);
}

void PGOState::NotifyAllGCWaiters()
{
    gcCondition_.SignalAll();
}
} // namespace panda::ecmascript::pgo
