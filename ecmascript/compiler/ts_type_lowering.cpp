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
#include "ecmascript/llvm_stackmap_parser.h"

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
    EcmaOpcode op = bcBuilder_->PcToOpcode(pc);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
    switch (op) {
        case EcmaOpcode::ADD2_IMM8_V8:
            LowerTypedAdd(gate);
            break;
        case EcmaOpcode::SUB2_IMM8_V8:
            LowerTypedSub(gate);
            break;
        case EcmaOpcode::MUL2_IMM8_V8:
            LowerTypedMul(gate);
            break;
        case EcmaOpcode::DIV2_IMM8_V8:
            LowerTypedDiv(gate);
            break;
        case EcmaOpcode::MOD2_IMM8_V8:
            LowerTypedMod(gate);
            break;
        case EcmaOpcode::LESS_IMM8_V8:
            LowerTypedLess(gate);
            break;
        case EcmaOpcode::LESSEQ_IMM8_V8:
            LowerTypedLessEq(gate);
            break;
        case EcmaOpcode::GREATER_IMM8_V8:
            LowerTypedGreater(gate);
            break;
        case EcmaOpcode::GREATEREQ_IMM8_V8:
            LowerTypedGreaterEq(gate);
            break;
        case EcmaOpcode::EQ_IMM8_V8:
            LowerTypedEq(gate);
            break;
        case EcmaOpcode::NOTEQ_IMM8_V8:
            LowerTypedNotEq(gate);
            break;
        case EcmaOpcode::SHL2_IMM8_V8:
            // lower JS_SHL
            break;
        case EcmaOpcode::SHR2_IMM8_V8:
            // lower JS_SHR
            break;
        case EcmaOpcode::ASHR2_IMM8_V8:
            // lower JS_ASHR
            break;
        case EcmaOpcode::AND2_IMM8_V8:
            // lower JS_AND
            break;
        case EcmaOpcode::OR2_IMM8_V8:
            // lower JS_OR
            break;
        case EcmaOpcode::XOR2_IMM8_V8:
            // lower JS_XOR
            break;
        case EcmaOpcode::EXP_IMM8_V8:
            // lower JS_EXP
            break;
        case EcmaOpcode::TONUMERIC_IMM8:
            // lower ToNumberic
            break;
        case EcmaOpcode::NEG_IMM8:
            // lower JS_NEG
            break;
        case EcmaOpcode::NOT_IMM8:
            // lower JS_NOT
            break;
        case EcmaOpcode::INC_IMM8:
            // lower JS_INC
            break;
        case EcmaOpcode::DEC_IMM8:
            // lower JS_DEC
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

void TSTypeLowering::ReplaceHirToFastPathCfg(GateRef hir, GateRef outir, GateRef state, GateRef depend,
                                             std::vector<GateRef> &unusedGate)
{
    auto uses = acc_.Uses(hir);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        const OpCode op = acc_.GetOpCode(*useIt);
        if (op == OpCode::IF_SUCCESS) {
            auto successUse = acc_.Uses(*useIt).begin();
            acc_.ReplaceStateIn(*successUse, state);
            unusedGate.emplace_back(*useIt);
            ++useIt;
        } else if (op == OpCode::IF_EXCEPTION) {
            auto exceptionUse = acc_.Uses(*useIt).begin();
            unusedGate.emplace_back(*exceptionUse);
            unusedGate.emplace_back(*useIt);
            ++useIt;
        } else if (op == OpCode::RETURN) {
            if (acc_.IsValueIn(useIt)) {
                useIt = acc_.ReplaceIn(useIt, outir);
                continue;
            }
            if (acc_.GetOpCode(acc_.GetIn(*useIt, 0)) != OpCode::IF_EXCEPTION) {
                acc_.ReplaceStateIn(*useIt, state);
                acc_.ReplaceDependIn(*useIt, depend);
                acc_.ReplaceValueIn(*useIt, outir);
            }
            ++useIt;
        } else if (op == OpCode::DEPEND_SELECTOR) {
            useIt = acc_.ReplaceIn(useIt, depend);
        } else {
            useIt = acc_.ReplaceIn(useIt, outir);
        }
    }
}

void TSTypeLowering::DeleteUnusedGate(std::vector<GateRef> &unusedGate)
{
    for (auto &gate : unusedGate) {
        acc_.DeleteGate(gate);
    }
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

void TSTypeLowering::LowerTypedAdd(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberCalculate<TypedBinOp::TYPED_ADD>(gate);
        return;
    }
}

void TSTypeLowering::LowerTypedSub(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberCalculate<TypedBinOp::TYPED_SUB>(gate);
        return;
    }
}

void TSTypeLowering::LowerTypedMul(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberCalculate<TypedBinOp::TYPED_MUL>(gate);
        return;
    }
}

void TSTypeLowering::LowerTypedMod(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberCalculate<TypedBinOp::TYPED_MOD>(gate);
        return;
    }
}

void TSTypeLowering::LowerTypedLess(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberCompare<TypedBinOp::TYPED_LESS>(gate);
        return;
    }
}

void TSTypeLowering::LowerTypedLessEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberCompare<TypedBinOp::TYPED_LESSEQ>(gate);
        return;
    }
}

void TSTypeLowering::LowerTypedGreater(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberCompare<TypedBinOp::TYPED_GREATER>(gate);
        return;
    }
}

void TSTypeLowering::LowerTypedGreaterEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberCompare<TypedBinOp::TYPED_GREATEREQ>(gate);
        return;
    }
}

void TSTypeLowering::LowerTypedDiv(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberCalculate<TypedBinOp::TYPED_DIV>(gate);
        return;
    }
}

void TSTypeLowering::LowerTypedEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberCompare<TypedBinOp::TYPED_EQ>(gate);
        return;
    }
}

void TSTypeLowering::LowerTypedNotEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumberCompare<TypedBinOp::TYPED_NOTEQ>(gate);
        return;
    }
}

template<TypedBinOp Op>
void TSTypeLowering::SpeculateNumberCalculate(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    Label isNumber(&builder_);
    Label notNumber(&builder_);
    Label exit(&builder_);
    GateType numberType = GateType::NumberType();
    std::vector<GateRef> unusedGate;
    unusedGate.emplace_back(gate);
    builder_.Branch(builder_.BoolAnd(builder_.TypeCheck(numberType, left), builder_.TypeCheck(numberType, right)),
                    &isNumber, &notNumber);
    builder_.Bind(&isNumber);
    {
        GateRef result = builder_.NumberBinaryOp<Op>(left, right);
        GateRef state = builder_.GetState();
        GateRef depend = builder_.GetDepend();
        ReplaceHirToFastPathCfg(gate, result, state, depend, unusedGate);
    }
    builder_.Bind(&notNumber);
    // deopt
    GateRef deoptStateIn = builder_.GetState();
    GateRef guard = acc_.GetDep(gate);
    builder_.Deoptimize(deoptStateIn, guard);
    DeleteUnusedGate(unusedGate);
}

template<TypedBinOp Op>
void TSTypeLowering::SpeculateNumberCompare(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    Label isNumber(&builder_);
    Label notNumber(&builder_);
    Label exit(&builder_);
    GateType numberType = GateType::NumberType();
    DEFVAlUE(result, (&builder_), VariableType(MachineType::I64, GateType::BooleanType()), builder_.HoleConstant());
    builder_.Branch(builder_.BoolAnd(builder_.TypeCheck(numberType, left), builder_.TypeCheck(numberType, right)),
                    &isNumber, &notNumber);
    std::map<GateRef, size_t> stateGateMap;
    builder_.Bind(&isNumber);
    {
        result = builder_.NumberBinaryOp<Op>(left, right);
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

void TSTypeLowering::LowerTypeToNumeric(GateRef gate)
{
    GateRef src = acc_.GetValueIn(gate, 0);
    GateType srcType = acc_.GetGateType(src);
    if (srcType.IsPrimitiveType() && !srcType.IsStringType()) {
        LowerPrimitiveTypeToNumber(gate);
        return;
    }
}

void TSTypeLowering::LowerPrimitiveTypeToNumber(GateRef gate)
{
    Label isPrimitive(&builder_);
    Label notPrimitive(&builder_);
    Label exit(&builder_);
    GateRef src = acc_.GetValueIn(gate, 0);
    GateType srcType = acc_.GetGateType(src);
    DEFVAlUE(result, (&builder_), VariableType(MachineType::I64, GateType::NumberType()), builder_.HoleConstant());
    builder_.Branch(builder_.TypeCheck(srcType, src), &isPrimitive, &notPrimitive);
    std::map<GateRef, size_t> stateGateMap;
    builder_.Bind(&isPrimitive);
    {
        result = builder_.PrimitiveToNumber(src, VariableType(MachineType::I64, srcType));
        builder_.Jump(&exit);
    }
    builder_.Bind(&notPrimitive);
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