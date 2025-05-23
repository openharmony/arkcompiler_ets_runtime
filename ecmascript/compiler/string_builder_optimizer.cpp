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

#include "ecmascript/compiler/string_builder_optimizer.h"

namespace panda::ecmascript::kungfu {

void StringBuilderOptimizer::Run()
{
    graphLinearizer_.SetScheduleJSOpcode();
    graphLinearizer_.LinearizeGraph();
    status_.resize(circuit_->GetMaxGateId() + 1, Status{INVALID_ID, State::UNVISITED}); // 1: max + 1 = size
    VisitGraph();
}

void StringBuilderOptimizer::VisitGraph()
{
    std::vector<GateRef> gateList;
    acc_.GetAllGates(gateList);
    for (auto gate : gateList) {
        FindBuilderBegin(gate);
    }
    currentIndex_ = 0;
    for (auto &builder : stringBuilders_) {
        FindInBuilder(builder.start);
        currentIndex_++;
    }
    FinalizeStringBuilders();
    for (auto gate : concatGates_) {
        StatusTransfer(gate);
    }
}

void StringBuilderOptimizer::FindBuilderBegin(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    if (op == OpCode::STRING_ADD) {
        concatGates_.push_back(gate);
        GateRef right = acc_.GetValueIn(gate, 1);
        if (!IsLiteralString(right)) {
            SetStatus(gate, State::INVALID_OPT);
            return;
        }
        GateRef left = acc_.GetValueIn(gate, 0);
        if (IsLiteralString(left)) {
            if (HasConcatOrPhiUse(left)) {
                SetStatus(gate, State::BEGIN_STRING_BUILDER, stringBuilderCount_);
                stringBuilders_.push_back(StringBuilder{gate, static_cast<int>(stringBuilderCount_), false});
                stringBuilderCount_++;
                return;
            }
            SetStatus(gate, State::INVALID_OPT);
        } else {
            auto leftOp = acc_.GetOpCode(left);
            if (leftOp == OpCode::VALUE_SELECTOR) {
                SetStatus(gate, State::UNVISITED);
            } else if (leftOp == OpCode::STRING_ADD) {
                curStringAddId_ = acc_.GetId(gate);
                if (CheckStringAddUses(left)) {
                    SetStatus(gate, State::UNVISITED);
                } else {
                    SetStatus(gate, State::INVALID_OPT);
                }
            } else {
                SetStatus(gate, State::INVALID_OPT);
            }
        }
    } else if (op == OpCode::VALUE_SELECTOR && PhiInputsAreConcatsOrPhi(gate)) {
        SetStatus(gate, State::UNVISITED);
    } else {
        SetStatus(gate, State::INVALID_OPT);
    }
}

void StringBuilderOptimizer::FindInBuilder(GateRef gate)
{
    toVisit_.clear();
    toVisit_.push_back(gate);
    while (!toVisit_.empty()) {
        GateRef curr = toVisit_.back();
        toVisit_.pop_back();
        auto uses = acc_.Uses(curr);
        for (auto it = uses.begin(); it != uses.end(); it++) {
            GateRef use = *it;
            if (!acc_.IsValueIn(it)) {
                continue;
            }
            VisitGateUse(use);
        }
    }
}

void StringBuilderOptimizer::VisitGateUse(GateRef use)
{
    Status useStatus = GetStatus(use);
    if (useStatus.state == State::INVALID_OPT || useStatus.state == State::IN_STRING_BUILDER) {
        return;
    }

    auto useOpCode = acc_.GetOpCode(use);
    if (useOpCode == OpCode::VALUE_SELECTOR && IsLoopHeader(use)) {
        stringBuilders_[currentIndex_].has_loop_phi = true;
    }

    if (useOpCode == OpCode::VALUE_SELECTOR) {
        UpdateStatus(use, State::IN_STRING_BUILDER);
        toVisit_.push_back(use);
    }

    // if this use is string-add
    // we need to determine whether to set the current use to IN_STRING_BUILDER
    // by judging the type and uses of left child
    if (useOpCode == OpCode::STRING_ADD) {
        GateRef left = acc_.GetValueIn(use, 0);
        auto leftOp = acc_.GetOpCode(left);
        if (leftOp == OpCode::VALUE_SELECTOR) {
            UpdateStatus(use, State::IN_STRING_BUILDER);
            toVisit_.push_back(use);
        } else if (leftOp == OpCode::STRING_ADD) {
            curStringAddId_ = acc_.GetId(use);
            if (CheckStringAddUses(left)) {
                UpdateStatus(use, State::IN_STRING_BUILDER);
                toVisit_.push_back(use);
            } else {
                SetStatus(use, State::INVALID_OPT);
            }
        } else {
            SetStatus(use, State::INVALID_OPT);
        }
    }
}

void StringBuilderOptimizer::FinalizeStringBuilders()
{
    for (uint32_t stringBuilderId = 0; stringBuilderId < stringBuilderCount_; stringBuilderId++) {
        auto& stringBuilder = stringBuilders_[stringBuilderId];
        GateRef start = stringBuilder.start;
        if (!stringBuilder.has_loop_phi) {
            continue;
        }
        toVisit_.clear();
        ends_.clear();

        toVisit_.push_back(start);
        while (!toVisit_.empty()) {
            GateRef curr = toVisit_.back();
            toVisit_.pop_back();
            auto currStatus = GetStatus(curr);
            if (currStatus.state == State::CONFIRMED_IN_STRING_BUILDER) {
                continue;
            }
            if (currStatus.state == State::IN_STRING_BUILDER) {
                UpdateStatus(curr, State::CONFIRMED_IN_STRING_BUILDER);
            }
            bool isEnd = false;
            bool currIsLoopHeader = IsLoopHeader(curr);
            auto uses = acc_.Uses(curr);
            for (auto it = uses.begin(); it != uses.end(); it++) {
                GateRef use = *it;
                if (!acc_.IsValueIn(it)) {
                    continue;
                }
                auto useStatus = GetStatus(use);
                bool useIsInBuilder = useStatus.state == State::IN_STRING_BUILDER;
                bool currAndUseBothInBuilder = useStatus.id == currStatus.id;
                if (useIsInBuilder && currAndUseBothInBuilder) {
                    toVisit_.push_back(use);
                }
                if (currIsLoopHeader && !LoopContains(curr, use)) {
                    isEnd = true;
                }
            }
            if (isEnd) {
                ends_.push_back(curr);
            }
        }

        for (auto &end : ends_) {
            UpdateStatus(end, State::END_STRING_BUILDER);
        }
    }
}

void StringBuilderOptimizer::StatusTransfer(GateRef gate)
{
    Status status = GetStatus(gate);
    switch (status.state) {
        case State::BEGIN_STRING_BUILDER:
            acc_.SetStringStatus(gate, BaseString::BEGIN_STRING_ADD);
            break;
        case State::IN_STRING_BUILDER:
            acc_.SetStringStatus(gate, BaseString::IN_STRING_ADD);
            break;
        case State::CONFIRMED_IN_STRING_BUILDER:
            acc_.SetStringStatus(gate, BaseString::CONFIRMED_IN_STRING_ADD);
            break;
        case State::END_STRING_BUILDER:
            acc_.SetStringStatus(gate, BaseString::END_STRING_ADD);
            break;
        default: {
            acc_.SetStringStatus(gate, BaseString::INVALID_STRING_ADD);
            break;
        }
    }
}

bool StringBuilderOptimizer::HasConcatOrPhiUse(GateRef gate)
{
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        GateRef use = *it;
        if (!acc_.IsValueIn(it)) {
            continue;
        }
        auto op = acc_.GetOpCode(use);
        if ((op == OpCode::STRING_ADD && HasConcatOrPhiUse(use)) || op == OpCode::VALUE_SELECTOR) {
            return true;
        }
    }
    return false;
}

bool StringBuilderOptimizer::CheckStringAddUses(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::STRING_ADD || acc_.IsValueSelector(gate));
    bool canBeOpt = true;
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        GateRef use = *it;
        if (!acc_.IsValueIn(it)) {
            continue;
        }
        auto useOp = acc_.GetOpCode(use);
        if (useOp == OpCode::STRING_ADD) {
            // if this gate has multiple string-add uses
            // or this gate has an invalid use
            // we cannot set current gate to IN_STRING_BUILDER
            if (curStringAddId_ != acc_.GetId(use) || IsInvalidGate(use)) {
                canBeOpt = false;
                break;
            }
            continue;
        }
        // this gate has a phi use, then check the phi use gate
        if (useOp == OpCode::VALUE_SELECTOR && !CheckStringAddUses(use)) {
            return false;
        }
        // this gate has the following types of uses is valid
        if (useOp == OpCode::INTERN_STRING_CHECK || useOp == OpCode::ECMA_STRING_CHECK ||
            acc_.IsFrameValues(use) || acc_.IsValueSelector(use)) {
            continue;
        }
        // all other uses are invalid
        return false;
    }
    return canBeOpt;
}

bool StringBuilderOptimizer::IsLiteralString(GateRef gate)
{
    return acc_.IsConstString(gate) || acc_.GetOpCode(gate) == OpCode::STRING_FROM_SINGLE_CHAR_CODE;
}

bool StringBuilderOptimizer::IsLoopHeader(GateRef gate)
{
    return acc_.GetOpCode(acc_.GetState(gate)) == OpCode::LOOP_BEGIN;
}

bool StringBuilderOptimizer::LoopContains(GateRef loopPhi, GateRef gate)
{
    ASSERT(IsLoopHeader(loopPhi));
    while (gate != acc_.GetStateRoot()) {
        if (gate == loopPhi) {
            return true;
        } else {
            if (acc_.GetStateCount(gate) == 0) {
                return false;
            }
            gate = acc_.GetState(gate, 0);
        }
    }
    return false;
}

bool StringBuilderOptimizer::PhiInputsAreConcatsOrPhi(GateRef phi)
{
    return (acc_.GetOpCode(acc_.GetValueIn(phi, 0)) == OpCode::STRING_ADD ||
            acc_.GetOpCode(acc_.GetValueIn(phi, 0)) == OpCode::VALUE_SELECTOR) &&
           (acc_.GetOpCode(acc_.GetValueIn(phi, 1)) == OpCode::STRING_ADD ||
            acc_.GetOpCode(acc_.GetValueIn(phi, 1)) == OpCode::VALUE_SELECTOR);
}
}  // namespace panda::ecmascript::kungfu
