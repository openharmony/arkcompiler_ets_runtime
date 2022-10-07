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
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m" << "================ After ts type lowering ================" << "\033[0m";
        circuit_->PrintAllGates(*bcBuilder_);
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
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

void TSTypeLowering::DeleteGates(std::vector<GateRef> &unusedGate)
{
    for (auto &gate : unusedGate) {
        acc_.DeleteGate(gate);
    }
}

void TSTypeLowering::ReplaceHIRGate(GateRef hir, GateRef outir, GateRef state, GateRef depend,
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
            ASSERT(acc_.GetOpCode(*exceptionUse) == OpCode::RETURN);
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
    GateType numberType = GateType::NumberType();
    GateRef check = builder_.BoolAnd(builder_.TypeCheck(numberType, left),
                                     builder_.TypeCheck(numberType, right));
    GateRef guard = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
    ASSERT(acc_.IsInGateNull(guard, 1));
    acc_.NewIn(guard, 1, check);
    GateRef result = builder_.NumberBinaryOp<Op>(left, right);
    acc_.SetDep(result, guard);
    std::vector<GateRef> removedGate{gate};
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(removedGate);
}

template<TypedBinOp Op>
void TSTypeLowering::SpeculateNumberCompare(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType numberType = GateType::NumberType();
    GateRef check = builder_.BoolAnd(builder_.TypeCheck(numberType, left),
                                     builder_.TypeCheck(numberType, right));
    GateRef guard = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
    ASSERT(acc_.IsInGateNull(guard, 1));
    acc_.NewIn(guard, 1, check);
    GateRef result = builder_.NumberBinaryOp<Op>(left, right);
    acc_.SetDep(result, guard);
    std::vector<GateRef> removedGate{gate};
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(removedGate);
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
    GateRef src = acc_.GetValueIn(gate, 0);
    GateType srcType = acc_.GetGateType(src);
    GateRef check = builder_.TypeCheck(srcType, src);
    GateRef guard = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
    ASSERT(acc_.IsInGateNull(guard, 1));
    acc_.NewIn(guard, 1, check);
    GateRef result = builder_.PrimitiveToNumber(src, VariableType(MachineType::I64, srcType));
    acc_.SetDep(result, guard);
    std::vector<GateRef> removedGate{gate};
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(removedGate);
}
}  // namespace panda::ecmascript