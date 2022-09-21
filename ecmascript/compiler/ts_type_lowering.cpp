/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/ts_type_lowering.h"

namespace panda::ecmascript::kungfu {
void TSTypeLowering::RunTSTypeLowering()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            Lower(gate);
        }
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "================== ts type lowering print all gates Start==================";
        circuit_->PrintAllGates(*bcBuilder_);
    }
}

void TSTypeLowering::Lower(GateRef gate)
{
    auto pc = bcBuilder_->GetJSBytecode(gate);
    EcmaBytecode op = static_cast<EcmaBytecode>(*pc);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
    switch (op) {
        case ADD2DYN_PREF_V8:
            LowerTypeAdd2Dyn(gate);
            break;
        case SUB2DYN_PREF_V8:
            LowerTypeSub2Dyn(gate);
            break;
        case MUL2DYN_PREF_V8:
            LowerTypeMul2Dyn(gate);
            break;
        case LESSDYN_PREF_V8:
            LowerTypeLess2Dyn(gate);
            break;
        case LESSEQDYN_PREF_V8:
            LowerTypeLessEq2Dyn(gate);
            break;
        case TONUMERIC_PREF_V8:
            // lower ToNumberic
            break;
        default:
            break;
    }
}

void TSTypeLowering::RebuildSlowpathCfg(GateRef hir, std::map<GateRef, size_t> &stateGateMap)
{
    acc_.ReplaceStateIn(hir, builder_.GetState());
    acc_.ReplaceDependIn(hir, builder_.GetDepend());
    auto uses = acc_.Uses(hir);
    GateRef stateGate = Circuit::NullGate();
    for (auto useIt = uses.begin(); useIt != uses.end(); ++useIt) {
        const OpCode op = acc_.GetOpCode(*useIt);
        if (op == OpCode::IF_SUCCESS) {
            stateGate = *useIt;
            builder_.SetState(*useIt);
            break;
        }
    }
    auto nextUses = acc_.Uses(stateGate);
    for (auto it = nextUses.begin(); it != nextUses.end(); ++it) {
        if (it.GetOpCode().IsState()) {
            stateGateMap[*it] = it.GetIndex();
        }
    }
    builder_.SetDepend(hir);
}

void TSTypeLowering::GenerateSuccessMerge(std::vector<GateRef> &successControl)
{
    GateRef stateMerge = builder_.GetState();
    GateRef dependSelect = builder_.GetDepend();
    successControl.emplace_back(stateMerge);
    successControl.emplace_back(dependSelect);
}

void TSTypeLowering::ReplaceHirToFastPathCfg(GateRef hir, GateRef outir, const std::vector<GateRef> &successControl)
{
    auto uses = acc_.Uses(hir);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        const OpCode op = acc_.GetOpCode(*useIt);
        if (op == OpCode::JS_BYTECODE && useIt.GetIndex() == 1) {
            acc_.ReplaceStateIn(*useIt, successControl[0]);
            useIt = acc_.ReplaceIn(useIt, successControl[1]);
        } else if (op == OpCode::RETURN) {
            if (acc_.IsValueIn(useIt)) {
                useIt = acc_.ReplaceIn(useIt, outir);
                continue;
            }
            if (acc_.GetOpCode(acc_.GetIn(*useIt, 0)) != OpCode::IF_EXCEPTION) {
                acc_.ReplaceStateIn(*useIt, successControl[0]);
                acc_.ReplaceDependIn(*useIt, successControl[1]);
                acc_.ReplaceValueIn(*useIt, outir);
            }
            ++useIt;
        } else if (op == OpCode::IF_SUCCESS || op == OpCode::IF_EXCEPTION) {
            ++useIt;
        } else if (op == OpCode::VALUE_SELECTOR) {
            if (*useIt != outir) {
                useIt = acc_.ReplaceIn(useIt, outir);
            } else {
                ++useIt;
            }
        } else if (op == OpCode::DEPEND_SELECTOR) {
            if (*useIt != successControl[1]) {
                useIt = acc_.ReplaceIn(useIt, successControl[1]);
            } else {
                ++useIt;
            }
        } else {
            useIt = acc_.ReplaceIn(useIt, outir);
        }
    }
}

void TSTypeLowering::LowerTypeAdd2Dyn(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberAdd(gate);
        return;
    }
}

void TSTypeLowering::SpeculateNumberAdd(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    Label isNumber(&builder_);
    Label notNumber(&builder_);
    Label exit(&builder_);
    GateType numberType = GateType::NumberType();
    DEFVAlUE(result, (&builder_), VariableType(MachineType::I64, numberType), builder_.HoleConstant());
    builder_.Branch(builder_.BoolAnd(builder_.TypeCheck(numberType, left),
                                     builder_.TypeCheck(numberType, right)),
                                     &isNumber, &notNumber);
    std::map<GateRef, size_t> stateGateMap;
    builder_.Bind(&isNumber);
    {
        result = builder_.NumberAdd(left, right);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notNumber);
    {
        // slowpath
        result = gate;
        RebuildSlowpathCfg(gate, stateGateMap);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    for (auto [state, index] : stateGateMap) {
        acc_.ReplaceIn(state, index, builder_.GetState());
    }
    std::vector<GateRef> successControl;
    GenerateSuccessMerge(successControl);
    ReplaceHirToFastPathCfg(gate, *result, successControl);
}

void TSTypeLowering::LowerTypeSub2Dyn(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberSub(gate);
        return;
    }
}

void TSTypeLowering::SpeculateNumberSub(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    Label isNumber(&builder_);
    Label notNumber(&builder_);
    Label exit(&builder_);
    GateType numberType = GateType::NumberType();
    DEFVAlUE(result, (&builder_), VariableType(MachineType::I64, numberType), builder_.HoleConstant());
    builder_.Branch(builder_.BoolAnd(builder_.TypeCheck(numberType, left),
                                     builder_.TypeCheck(numberType, right)),
                                     &isNumber, &notNumber);
    std::map<GateRef, size_t> stateGateMap;
    builder_.Bind(&isNumber);
    {
        result = builder_.NumberSub(left, right);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notNumber);
    {
        // slowpath
        result = gate;
        RebuildSlowpathCfg(gate, stateGateMap);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    for (auto [state, index] : stateGateMap) {
        acc_.ReplaceIn(state, index, builder_.GetState());
    }
    std::vector<GateRef> successControl;
    GenerateSuccessMerge(successControl);
    ReplaceHirToFastPathCfg(gate, *result, successControl);
}

void TSTypeLowering::LowerTypeMul2Dyn(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberMul(gate);
        return;
    }
}

void TSTypeLowering::SpeculateNumberMul(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    Label isNumber(&builder_);
    Label notNumber(&builder_);
    Label exit(&builder_);
    GateType numberType = GateType::NumberType();
    DEFVAlUE(result, (&builder_), VariableType(MachineType::I64, numberType), builder_.HoleConstant());
    builder_.Branch(builder_.BoolAnd(builder_.TypeCheck(numberType, left),
                                     builder_.TypeCheck(numberType, right)),
                                     &isNumber, &notNumber);
    std::map<GateRef, size_t> stateGateMap;
    builder_.Bind(&isNumber);
    {
        result = builder_.NumberMul(left, right);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notNumber);
    {
        // slowpath
        result = gate;
        RebuildSlowpathCfg(gate, stateGateMap);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    for (auto [state, index] : stateGateMap) {
        acc_.ReplaceIn(state, index, builder_.GetState());
    }
    std::vector<GateRef> successControl;
    GenerateSuccessMerge(successControl);
    ReplaceHirToFastPathCfg(gate, *result, successControl);
}

void TSTypeLowering::LowerTypeLess2Dyn(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberLess(gate);
        return;
    }
}

void TSTypeLowering::SpeculateNumberLess(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    Label isNumber(&builder_);
    Label notNumber(&builder_);
    Label exit(&builder_);
    GateType numberType = GateType::NumberType();
    DEFVAlUE(result, (&builder_), VariableType(MachineType::I64, GateType::BooleanType()), builder_.HoleConstant());
    builder_.Branch(builder_.BoolAnd(builder_.TypeCheck(numberType, left),
                                     builder_.TypeCheck(numberType, right)),
                                     &isNumber, &notNumber);
    std::map<GateRef, size_t> stateGateMap;
    builder_.Bind(&isNumber);
    {
        result = builder_.NumberLess(left, right);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notNumber);
    {
        // slowpath
        result = gate;
        RebuildSlowpathCfg(gate, stateGateMap);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    for (auto [state, index] : stateGateMap) {
        acc_.ReplaceIn(state, index, builder_.GetState());
    }
    std::vector<GateRef> successControl;
    GenerateSuccessMerge(successControl);
    ReplaceHirToFastPathCfg(gate, *result, successControl);
}

void TSTypeLowering::LowerTypeLessEq2Dyn(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberLessEq(gate);
        return;
    }
}

void TSTypeLowering::SpeculateNumberLessEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    Label isNumber(&builder_);
    Label notNumber(&builder_);
    Label exit(&builder_);
    GateType numberType = GateType::NumberType();
    DEFVAlUE(result, (&builder_), VariableType(MachineType::I64, GateType::BooleanType()), builder_.HoleConstant());
    builder_.Branch(builder_.BoolAnd(builder_.TypeCheck(numberType, left),
                                     builder_.TypeCheck(numberType, right)),
                                     &isNumber, &notNumber);
    std::map<GateRef, size_t> stateGateMap;
    builder_.Bind(&isNumber);
    {
        result = builder_.NumberLessthanOrEq(left, right);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notNumber);
    {
        // slowpath
        result = gate;
        RebuildSlowpathCfg(gate, stateGateMap);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    for (auto [state, index] : stateGateMap) {
        acc_.ReplaceIn(state, index, builder_.GetState());
    }
    std::vector<GateRef> successControl;
    GenerateSuccessMerge(successControl);
    ReplaceHirToFastPathCfg(gate, *result, successControl);
}
}  // namespace panda::ecmascript