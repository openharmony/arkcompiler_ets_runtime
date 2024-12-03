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

#include "ecmascript/compiler/induction_variable_analysis.h"

namespace panda::ecmascript::kungfu {

bool InductionVariableAnalysis::IsIntConstant(GateRef gate) const
{
    if (acc_.GetOpCode(gate) != OpCode::CONSTANT) {
        return false;
    }
    JSTaggedValue value(acc_.GetConstantValue(gate));
    return value.IsInt();
}

bool InductionVariableAnalysis::IsInductionVariable(GateRef gate) const
{
    if (acc_.GetOpCode(gate) != OpCode::VALUE_SELECTOR) {
        return false;
    }
    size_t numValueIn = acc_.GetNumValueIn(gate);
    GateRef startGate = acc_.GetValueIn(gate, 0);
    GateRef valueGate = acc_.GetValueIn(gate, 1);
    if (!IsIntConstant(startGate)) {
        return false;
    }
    if (acc_.GetOpCode(valueGate) != OpCode::TYPED_BINARY_OP) {
        return false;
    }
    TypedBinOp binOp = acc_.GetTypedBinaryOp(valueGate);
    if (binOp != TypedBinOp::TYPED_ADD && binOp != TypedBinOp::TYPED_SUB) {
        return false;
    }
    TypedBinaryAccessor accessor(acc_.TryGetValue(valueGate));
    const ParamType paramType = accessor.GetParamType();
    if (!paramType.IsIntType()) {
        return false;
    }

    for (size_t i = 2; i < numValueIn; i++) { // 2: skip startGate and valueGate
        if (acc_.GetValueIn(gate, i) != valueGate) {
            return false;
        }
    }
    
    // check if value satisfies a = a + x
    if (acc_.GetValueIn(valueGate, 0) != gate && acc_.GetValueIn(valueGate, 1) != gate) {
        return false;
    }
    GateRef stride = acc_.GetValueIn(valueGate, 1);
    if (acc_.GetValueIn(valueGate, 0) != gate) {
        stride = acc_.GetValueIn(valueGate, 0);
    }
    if (!IsIntConstant(stride)) {
        return false;
    }
    return true;
}

std::pair<int32_t, int32_t> InductionVariableAnalysis::GetStartAndStride(GateRef gate) const
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::VALUE_SELECTOR);
    GateRef startGate = acc_.GetValueIn(gate, 0);
    ASSERT(IsIntConstant(startGate));
    auto start = GetIntFromTaggedConstant(startGate);

    GateRef valueGate = acc_.GetValueIn(gate, 1);
    ASSERT(acc_.GetOpCode(valueGate) == OpCode::TYPED_BINARY_OP);
    [[maybe_unused]]TypedBinOp binOp = acc_.GetTypedBinaryOp(valueGate);
    ASSERT(binOp == TypedBinOp::TYPED_ADD || binOp == TypedBinOp::TYPED_SUB);
    TypedBinaryAccessor accessor(acc_.TryGetValue(valueGate));
    [[maybe_unused]]const ParamType paramType = accessor.GetParamType();
    ASSERT(paramType.IsIntType());

    GateRef strideGate = acc_.GetValueIn(valueGate, 1);
    if (acc_.GetValueIn(valueGate, 0) != gate) {
        strideGate = acc_.GetValueIn(valueGate, 0);
    }
    ASSERT(IsIntConstant(strideGate));
    auto stride = GetIntFromTaggedConstant(strideGate);

    // a - xb < c -> a + (-x)b < c
    if (acc_.GetTypedBinaryOp(valueGate) == TypedBinOp::TYPED_SUB) {
        stride = -stride;
    }

    return std::make_pair(start, stride);
}

int32_t InductionVariableAnalysis::GetIntFromTaggedConstant(GateRef gate) const
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::CONSTANT);
    JSTaggedValue value(acc_.GetConstantValue(gate));
    return value.GetInt();
}

bool InductionVariableAnalysis::IsLessOrGreaterCmp(GateRef gate) const
{
    return acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_GREATEREQ ||
        acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_GREATER ||
        acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_LESSEQ ||
        acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_LESS;
}

bool InductionVariableAnalysis::TryGetLoopTimes(const GraphLinearizer::LoopInfo& loop, int32_t& loopTimes) const
{
    if (loop.loopExits == nullptr || loop.loopExits->size() > 1) {
        return false;
    }
    ASSERT(loop.loopExits->size() == 1);
    GateRef loopExit = loop.loopExits->at(0)->GetState();
    ASSERT(acc_.GetOpCode(loopExit) == OpCode::IF_TRUE || acc_.GetOpCode(loopExit) == OpCode::IF_FALSE);
    GateRef conditionJump = acc_.GetState(loopExit);
    GateRef cmp = acc_.GetValueIn(conditionJump);
    if (acc_.GetOpCode(cmp) != OpCode::TYPED_BINARY_OP || !IsLessOrGreaterCmp(cmp)) {
        return false;
    }
    GateRef limitGate = acc_.GetValueIn(cmp, 1);
    if (!IsIntConstant(limitGate)) {
        return false;
    }
    int32_t limit = GetIntFromTaggedConstant(limitGate);

    GateRef selector = acc_.GetValueIn(cmp, 0);
    if (!IsInductionVariable(selector)) {
        return false;
    }

    auto [start, stride] = GetStartAndStride(selector);

    bool cmpFlag = (acc_.GetOpCode(loopExit) == OpCode::IF_TRUE) ^
        (acc_.GetTypedJumpAccessor(conditionJump).GetTypedJumpOp() == TypedJumpOp::TYPED_JEQZ) ^
        (acc_.GetTypedBinaryOp(cmp) == TypedBinOp::TYPED_LESSEQ ||
        acc_.GetTypedBinaryOp(cmp) == TypedBinOp::TYPED_LESS);
    bool equalFlag = (acc_.GetOpCode(loopExit) == OpCode::IF_TRUE) ^
        (acc_.GetTypedJumpAccessor(conditionJump).GetTypedJumpOp() == TypedJumpOp::TYPED_JEQZ) ^
        (acc_.GetTypedBinaryOp(cmp) == TypedBinOp::TYPED_GREATEREQ ||
        acc_.GetTypedBinaryOp(cmp) == TypedBinOp::TYPED_LESSEQ);

    // a + xb >= c -> c - xb <= a
    if (!cmpFlag) {
        std::swap(start, limit);
        stride = -stride;
    }
    // a + xb < c -> a + xb <= c - 1
    if (!equalFlag) {
        limit--;
    }
    if (start > limit) {
        loopTimes = 0;
        return true;
    }
    loopTimes = (limit - start) / stride + 1;
    if (IsLogEnabled() && IsTraced()) {
        LOG_COMPILER(INFO) << "loopTimes: "<< loopTimes << " start: " << start
                           << " stride: " << stride << " limit: " << limit;
    }
    return true;
}

void InductionVariableAnalysis::CollectInductionSelector()
{
    for (const auto &loop : graphLinearizer_.loops_) {
        int32_t loopTimes = 0;
        
        if (TryGetLoopTimes(loop, loopTimes)) {
            ReplaceInductionVariable(loop, loopTimes);
        }
    }
}

void InductionVariableAnalysis::ReplaceInductionVariable(const GraphLinearizer::LoopInfo& loop,
                                                         const int32_t loopTimes)
{
    GateRef loopBegin = loop.loopHead->GetState();
    auto uses = acc_.Uses(loopBegin);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc_.GetOpCode(*it) == OpCode::VALUE_SELECTOR) {
            ASSERT(acc_.GetState(*it) == loopBegin);
            if (!IsInductionVariable(*it)) {
                continue;
            }
            auto [start, stride] = GetStartAndStride(*it);
            int64_t result = start + static_cast<int64_t>(stride) * loopTimes;
            if (result > static_cast<int64_t>(INT_MAX) || result < static_cast<int64_t>(INT_MIN)) {
                return;
            }
            if (IsLogEnabled() && IsTraced()) {
                LOG_COMPILER(INFO) << "result = " << start << " + " << stride << " * "
                                    << loopTimes << " = " << result;
            }
            TryReplaceOutOfLoopUses(*it, loop, static_cast<int32_t>(result));
        }
    }
}

void InductionVariableAnalysis::TryReplaceOutOfLoopUses(GateRef gate,
                                                        const GraphLinearizer::LoopInfo& loop,
                                                        const int32_t result)
{
    ASSERT(IsInductionVariable(gate));
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end();) {
        auto region = graphLinearizer_.GateToRegion(*it);
        if (!loop.loopBodys->TestBit(region->GetId()) && loop.loopHead != region) {
            GateRef constantValue = builder_.Int32(result);
            it = acc_.ReplaceIn(it, constantValue);
        } else {
            it++;
        }
    }
}

void InductionVariableAnalysis::Run()
{
    graphLinearizer_.SetScheduleJSOpcode();
    graphLinearizer_.LinearizeGraph();
    CollectInductionSelector();
    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After Induction Variable Analysis "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

}