/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/dead_code_elimination.h"

namespace panda::ecmascript::kungfu {

GateRef DeadCodeElimination::VisitGate(GateRef gate)
{
    auto opcode = acc_.GetOpCode(gate);
    switch (opcode) {
        case OpCode::SWITCH_BRANCH:
        case OpCode::IF_BRANCH:
            return EliminateBranch(gate);
        case OpCode::MERGE:
        case OpCode::LOOP_BEGIN:
            return EliminateMergeAndLoopBegin(gate);
        case OpCode::DEPEND_SELECTOR:
            return EliminateDependSelector(gate);
        case OpCode::IF_EXCEPTION:
            return EliminateIfException(gate);
        case OpCode::LOOP_EXIT:
            return EliminateLoopExit(gate);
        default :
            return EliminateGate(gate);
    }
    return Circuit::NullGate();
}

GateRef DeadCodeElimination::StateIsDead(GateRef gate)
{
    auto state = acc_.GetState(gate);
    if (acc_.IsDead(state)) {
        return state;
    }
    return Circuit::NullGate();
}

GateRef DeadCodeElimination::EliminateDependSelector(GateRef gate)
{
    GateRef state = StateIsDead(gate);
    if (state != Circuit::NullGate() && acc_.IsDead(state)) {
        return state;
    }
    auto stateInput = acc_.GetState(gate);
    size_t dependCount = acc_.GetDependCount(gate);
    GateRef result = Circuit::NullGate();
    for (size_t i = 0; i < dependCount; i++) {
        auto depend = acc_.GetDep(gate, i);
        if (acc_.IsDead(depend)) {
            acc_.ReplaceStateIn(stateInput, deadGate_, i);
            visitor_->ReVisitGate(stateInput);
            result = gate;
        }
    }
    return result;
}

GateRef DeadCodeElimination::EliminateIfException(GateRef gate)
{
    GateRef state = StateIsDead(gate);
    if (state != Circuit::NullGate() && acc_.IsDead(state)) {
        return state;
    }
    GateRef depend = acc_.GetDep(gate);
    if (acc_.IsDead(depend)) {
        return deadGate_;
    }
    return Circuit::NullGate();
}

GateRef DeadCodeElimination::EliminateLoopExit(GateRef gate)
{
    GateRef state = StateIsDead(gate);
    if (state != Circuit::NullGate() && acc_.IsDead(state)) {
        return DeleteLoopExit(gate);
    }
    return Circuit::NullGate();
}

GateRef DeadCodeElimination::EliminateBranch(GateRef gate)
{
    GateRef state = StateIsDead(gate);
    if (state != Circuit::NullGate() && acc_.IsDead(state)) {
        return state;
    }
    GateRef value = acc_.GetValueIn(gate, 0);
    if (acc_.IsDead(value)) {
        auto uses = acc_.Uses(gate);
        for (auto it = uses.begin(); it != uses.end(); it++) {
            if (acc_.IsIfOrSwitchRelated(*it)) {
                ReplaceGate(*it, acc_.GetState(gate));
                return deadGate_;
            }
        }
        UNREACHABLE();
    }
    return gate;
}

void DeadCodeElimination::DecreaseAllSelectors(GateRef gate, size_t count)
{
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc_.IsSelector(*it)) {
            acc_.DecreaseIn(*it, count + 1);
        }
    }
}
GateRef DeadCodeElimination::EliminateMergeAndLoopBegin(GateRef gate)
{
    if (acc_.GetOpCode(gate) == OpCode::LOOP_BEGIN) {
        auto loopEntry = acc_.GetIn(gate, 0);
        if (acc_.IsDead(loopEntry)) {
            return deadGate_;
        }
    }
    size_t count = 0;
    size_t inputCount = acc_.GetNumIns(gate);
    for (size_t i = 0; i < inputCount; i++) {
        auto input = acc_.GetIn(gate, count);
        if (acc_.IsDead(input)) {
            acc_.DecreaseIn(gate, count);
            DecreaseAllSelectors(gate, count);
        } else {
            count++;
        }
    }
    if (count == 0) {
        return deadGate_;
    } else if (count == 1) {
        auto uses = acc_.Uses(gate);
        for (auto it = uses.begin(); it != uses.end(); it++) {
            if (acc_.IsSelector(*it)) {
                TryFindAndDeleteLoopExit(*it);
                auto selectorInput = acc_.GetIn(*it, 1);
                ReplaceGate(*it, selectorInput);
            }
        }
        return acc_.GetIn(gate, 0);
    }
    if (count < inputCount) {
        auto uses = acc_.Uses(gate);
        for (auto it = uses.begin(); it != uses.end(); it++) {
            if (acc_.IsSelector(*it)) {
                visitor_->ReVisitGate(*it);
            }
        }
        return gate;
    }
    return Circuit::NullGate();
}

void DeadCodeElimination::TryFindAndDeleteLoopExit(GateRef gate)
{
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc_.GetOpCode(*it) == OpCode::LOOP_EXIT_VALUE || acc_.GetOpCode(*it) == OpCode::LOOP_EXIT_DEPEND) {
            GateRef loopExit = acc_.GetState(gate);
            DeleteLoopExit(loopExit);
        }
    }
}
GateRef DeadCodeElimination::DeleteLoopExit(GateRef gate)
{
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc_.GetOpCode(*it) == OpCode::LOOP_EXIT_VALUE) {
            ReplaceGate(*it, acc_.GetValueIn(*it));
        } else if (acc_.GetOpCode(*it) == OpCode::LOOP_EXIT_DEPEND) {
            ReplaceGate(*it, acc_.GetDep(*it));
        }
    }
    return acc_.GetState(gate);
}

GateRef DeadCodeElimination::EliminateGate(GateRef gate)
{
    if (acc_.GetStateCount(gate) == 1) {
        return StateIsDead(gate);
    }
    return Circuit::NullGate();
}

}