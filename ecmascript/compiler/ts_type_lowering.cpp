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

    VerifyGuard();

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After ts type lowering "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGates(*bcBuilder_);
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

void TSTypeLowering::VerifyGuard() const
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            auto depend = acc_.GetDep(gate);
            if (acc_.GetOpCode(depend) == OpCode::GUARD) {
                std::string bytecodeStr = bcBuilder_->GetBytecodeStr(gate);
                LOG_COMPILER(ERROR) << "[ts_type_lowering][Error] the depend of ["
                                    << "id: " << acc_.GetId(gate) << ", JS_BYTECODE: " << bytecodeStr
                                    << "] should not be GUARD after ts type lowring";
            }
        }
    }
}

bool TSTypeLowering::IsTrustedType(GateRef gate) const
{
    if (acc_.IsConstant(gate)) {
        return true;
    }
    auto op = acc_.GetOpCode(gate);
    if (acc_.IsTypedOperator(gate)) {
        if (op == OpCode::TYPED_BINARY_OP) {
            return !acc_.GetGateType(gate).IsIntType();
        } else {
            return true;
        }
    }
    if (op == OpCode::JS_BYTECODE) {
        auto pc = bcBuilder_->GetJSBytecode(gate);
        EcmaOpcode bc = bcBuilder_->PcToOpcode(pc);
        switch (bc) {
            case EcmaOpcode::ADD2_IMM8_V8:
            case EcmaOpcode::SUB2_IMM8_V8:
            case EcmaOpcode::MUL2_IMM8_V8:
                return !acc_.GetGateType(gate).IsIntType();
            case EcmaOpcode::INC_IMM8:
            case EcmaOpcode::DEC_IMM8:
            case EcmaOpcode::LESS_IMM8_V8:
            case EcmaOpcode::LESSEQ_IMM8_V8:
                return true;
            default:
                break;
        }
    }
    return false;
}

void TSTypeLowering::Lower(GateRef gate)
{
    auto argAcc = ArgumentAccessor(circuit_);
    GateRef thisObj = argAcc.GetCommonArgGate(CommonArgIdx::THIS_OBJECT);

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
            LowerTypedShl(gate);
            break;
        case EcmaOpcode::SHR2_IMM8_V8:
            LowerTypedShr(gate);
            break;
        case EcmaOpcode::ASHR2_IMM8_V8:
            LowerTypedAshr(gate);
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
            LowerTypeToNumeric(gate);
            break;
        case EcmaOpcode::NEG_IMM8:
            LowerTypedNeg(gate);
            break;
        case EcmaOpcode::NOT_IMM8:
            LowerTypedNot(gate);
            break;
        case EcmaOpcode::INC_IMM8:
            LowerTypedInc(gate);
            break;
        case EcmaOpcode::DEC_IMM8:
            LowerTypedDec(gate);
            break;
        case EcmaOpcode::ISTRUE:
            LowerTypedIsTrueOrFalse(gate, true);
            break;
        case EcmaOpcode::ISFALSE:
            LowerTypedIsTrueOrFalse(gate, false);
            break;
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JEQZ_IMM32:
            LowerConditionJump(gate);
            break;
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
            LowerTypedLdObjByName(gate, thisObj);
            break;
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
            LowerTypedLdObjByName(gate, thisObj, true);
            break;
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
            LowerTypedStObjByName(gate, thisObj);
            break;
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
            LowerTypedStObjByName(gate, thisObj, true);
            break;
        case EcmaOpcode::LDOBJBYINDEX_IMM8_IMM16:
        case EcmaOpcode::LDOBJBYINDEX_IMM16_IMM16:
        case EcmaOpcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
            LowerTypedLdObjByIndex(gate);
            break;
        case EcmaOpcode::STOBJBYINDEX_IMM8_V8_IMM16:
        case EcmaOpcode::STOBJBYINDEX_IMM16_V8_IMM16:
        case EcmaOpcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
            LowerTypedStObjByIndex(gate);
            break;
        default:
            break;
    }
}

void TSTypeLowering::DeleteGates(GateRef hir, std::vector<GateRef> &unusedGate)
{
    for (auto &gate : unusedGate) {
        auto uses = acc_.Uses(gate);
        for (auto useIt = uses.begin(); useIt != uses.end(); ++useIt) {
            if (acc_.GetOpCode(gate) == OpCode::IF_EXCEPTION && acc_.GetOpCode(*useIt) == OpCode::MERGE) {
                // handle exception merge has only one input, using state entry and depend entry to replace merge and
                // dependselector.
                if (acc_.GetNumIns(*useIt) == 1) {
                    GateRef stateEntry = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
                    GateRef dependEntry = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
                    auto mergeUses = acc_.Uses(*useIt);
                    for (auto mergeUseIt = mergeUses.begin(); mergeUseIt != uses.end(); ++mergeUseIt) {
                        if (acc_.GetOpCode(*mergeUseIt) == OpCode::DEPEND_SELECTOR) {
                            auto dependSelectorUses = acc_.Uses(*mergeUseIt);
                            acc_.ReplaceIn(*dependSelectorUses.begin(), 0, dependEntry);
                            acc_.DeleteGate(*mergeUseIt);
                            break;
                        }
                    }
                    acc_.ReplaceIn(*useIt, 0, stateEntry);
                } else {
                    acc_.DecreaseIn(useIt);
                }
            }
        }
        acc_.DeleteGate(gate);
    }
    acc_.DeleteGate(hir);
}

void TSTypeLowering::ReplaceHIRGate(GateRef hir, GateRef outir, GateRef state, GateRef depend,
                                    std::vector<GateRef> &unusedGate)
{
    if (outir != Circuit::NullGate()) {
        auto type = acc_.GetGateType(hir);
        if (!type.IsAnyType()) {
            acc_.SetGateType(outir, type);
        }
    }

    std::map<GateRef, size_t> deleteMap;
    auto uses = acc_.Uses(hir);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        const OpCode op = acc_.GetOpCode(*useIt);
        if (op == OpCode::IF_SUCCESS) {
            // success path use fastpath state
            unusedGate.emplace_back(*useIt);
            auto successUse = acc_.Uses(*useIt).begin();
            acc_.ReplaceIn(successUse, state);
            ++useIt;
        } else if (op == OpCode::IF_EXCEPTION) {
            // exception path needs to delete all related nodes
            unusedGate.emplace_back(*useIt);
            auto exceptionUse = acc_.Uses(*useIt);
            auto exceptionUseIt = exceptionUse.begin();
            if (acc_.GetOpCode(*exceptionUseIt) == OpCode::MERGE) {
                auto mergeUse = acc_.Uses(*exceptionUseIt);
                // handle exception->merge->value_selector/depend_selector
                for (auto mergeUseIt = mergeUse.begin(); mergeUseIt != mergeUse.end();) {
                    if (acc_.GetOpCode(*mergeUseIt) == OpCode::VALUE_SELECTOR ||
                        acc_.GetOpCode(*mergeUseIt) == OpCode::DEPEND_SELECTOR) {
                        deleteMap[*mergeUseIt] = exceptionUseIt.GetIndex() + 1;
                    }
                    ++mergeUseIt;
                }
            } else if (acc_.GetOpCode(*exceptionUseIt) == OpCode::RETURN) {
                unusedGate.emplace_back(*exceptionUseIt);
            }
            ++useIt;
        } else if (op == OpCode::RETURN) {
            // replace return valueIn and dependIn
            if (acc_.IsValueIn(useIt)) {
                useIt = acc_.ReplaceIn(useIt, outir);
            } else if (acc_.IsDependIn(useIt)) {
                useIt = acc_.ReplaceIn(useIt, depend);
            } else {
                ++useIt;
            }
        } else if (op == OpCode::DEPEND_SELECTOR) {
            if (acc_.GetOpCode(acc_.GetIn(acc_.GetIn(*useIt, 0), useIt.GetIndex() - 1)) == OpCode::IF_EXCEPTION) {
                ++useIt;
            } else {
                useIt = acc_.ReplaceIn(useIt, depend);
            }
        } else {
            if (acc_.IsValueIn(useIt)) {
                useIt = acc_.ReplaceIn(useIt, outir);
            } else if (acc_.IsDependIn(useIt)) {
                useIt = acc_.ReplaceIn(useIt, depend);
            } else {
                useIt = acc_.ReplaceIn(useIt, state);
            }
        }
    }

    for (auto it = deleteMap.begin(); it != deleteMap.end(); it++) {
        acc_.DecreaseIn(it->first, it->second);
    }
}

void TSTypeLowering::LowerTypedAdd(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_ADD>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedSub(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_SUB>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedMul(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_MUL>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedMod(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_MOD>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedLess(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_LESS>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedLessEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_LESSEQ>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedGreater(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_GREATER>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedGreaterEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_GREATEREQ>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedDiv(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_DIV>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_EQ>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedNotEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_NOTEQ>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedShl(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_SHL>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedShr(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_SHR>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedAshr(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_ASHR>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedInc(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsNumberType()) {
        SpeculateNumber<TypedUnOp::TYPED_INC>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedDec(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsNumberType()) {
        SpeculateNumber<TypedUnOp::TYPED_DEC>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

template<TypedBinOp Op>
void TSTypeLowering::SpeculateNumbers(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    GateType gateType = acc_.GetGateType(gate);
    GateRef check = Circuit::NullGate();
    if (IsTrustedType(left) && IsTrustedType(right)) {
        acc_.DeleteGuardAndFrameState(gate);
    } else if (IsTrustedType(left)) {
        check = builder_.TypeCheck(rightType, right);
    } else if (IsTrustedType(right)) {
        check = builder_.TypeCheck(leftType, left);
    } else {
        check = builder_.BoolAnd(builder_.TypeCheck(leftType, left), builder_.TypeCheck(rightType, right));
    }

    // guard maybe not a GUARD
    GateRef guard = acc_.GetDep(gate);
    if (check != Circuit::NullGate()) {
        ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
        acc_.ReplaceIn(guard, 1, check);
    }

    // Replace the old NumberBinaryOp<Op> with TypedBinaryOp<Op>
    GateRef result = builder_.TypedBinaryOp<Op>(left, right, leftType, rightType, gateType);

    acc_.SetDep(result, guard);
    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

template<TypedUnOp Op>
GateRef TSTypeLowering::AppendOverflowCheck(GateRef typeCheck, GateRef intVal)
{
    GateRef check = typeCheck;
    switch (Op) {
        case TypedUnOp::TYPED_INC: {
            auto max = builder_.Int64(INT32_MAX);
            auto rangeCheck = builder_.Int64NotEqual(intVal, max);
            check = typeCheck != Circuit::NullGate() ? builder_.BoolAnd(typeCheck, rangeCheck) : rangeCheck;
            break;
        }
        case TypedUnOp::TYPED_NEG:
        case TypedUnOp::TYPED_DEC: {
            auto min = builder_.Int64(INT32_MIN);
            auto rangeCheck = builder_.Int64NotEqual(intVal, min);
            check = typeCheck != Circuit::NullGate() ? builder_.BoolAnd(typeCheck, rangeCheck) : rangeCheck;
            break;
        }
        default:
            break;
    }
    return check;
}

template<TypedUnOp Op>
void TSTypeLowering::SpeculateNumber(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    GateType gateType = acc_.GetGateType(gate);
    GateRef check = Circuit::NullGate();
    if (IsTrustedType(value)) {
        if (!valueType.IsIntType()) {
            acc_.DeleteGuardAndFrameState(gate);
        }
    } else {
        check = builder_.TypeCheck(valueType, value);
    }

    if (valueType.IsIntType()) {
        auto intVal = builder_.GetInt64OfTInt(value);
        check = AppendOverflowCheck<Op>(check, intVal);
    }

    // guard maybe not a GUARD
    GateRef guard = acc_.GetDep(gate);
    if (check != Circuit::NullGate()) {
        ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
        acc_.ReplaceIn(guard, 1, check);
    }

    GateRef result = builder_.TypedUnaryOp<Op>(value, valueType, gateType);

    acc_.SetDep(result, guard);
    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerTypeToNumeric(GateRef gate)
{
    GateRef src = acc_.GetValueIn(gate, 0);
    GateType srcType = acc_.GetGateType(src);
    if (srcType.IsPrimitiveType() && !srcType.IsStringType()) {
        LowerPrimitiveTypeToNumber(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerPrimitiveTypeToNumber(GateRef gate)
{
    GateRef src = acc_.GetValueIn(gate, 0);
    GateType srcType = acc_.GetGateType(src);
    GateRef check = Circuit::NullGate();
    if (IsTrustedType(src)) {
        acc_.DeleteGuardAndFrameState(gate);
    } else {
        check = builder_.TypeCheck(srcType, src);
    }

    // guard maybe not a GUARD
    GateRef guard = acc_.GetDep(gate);
    if (check != Circuit::NullGate()) {
        ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
        acc_.ReplaceIn(guard, 1, check);
    }

    GateRef result = builder_.PrimitiveToNumber(src, VariableType(MachineType::I64, srcType));
    acc_.SetDep(result, guard);
    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerConditionJump(GateRef gate)
{
    GateRef condition = acc_.GetValueIn(gate, 0);
    GateType conditionType = acc_.GetGateType(condition);
    if (conditionType.IsBooleanType() && IsTrustedType(condition)) {
        SpeculateConditionJump(gate);
    }
}

void TSTypeLowering::SpeculateConditionJump(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef condition = builder_.IsSpecial(value, JSTaggedValue::VALUE_FALSE);
    GateRef ifBranch = builder_.Branch(acc_.GetState(gate), condition);
    acc_.ReplaceGate(gate, ifBranch, builder_.GetDepend(), Circuit::NullGate());
}

void TSTypeLowering::LowerTypedNeg(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsNumberType()) {
        SpeculateNumber<TypedUnOp::TYPED_NEG>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedNot(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsNumberType()) {
        SpeculateNumber<TypedUnOp::TYPED_NOT>(gate);
    } else {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedLdObjByName(GateRef gate, GateRef thisObj, bool isThis)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto propIndex = acc_.GetBitField(acc_.GetValueIn(gate, 1));
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    auto prop = ConstantPool::GetStringFromCache(thread, constantPool_.GetTaggedValue(), propIndex);

    GateRef receiver = Circuit::NullGate();
    if (isThis) {
        // 2: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 2);
        receiver = thisObj;
    } else {
        // 3: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 3);
        receiver = acc_.GetValueIn(gate, 2);
    }
    GateType receiverType = acc_.GetGateType(receiver);
    int hclassIndex = tsManager_->GetHClassIndex(receiverType);
    if (hclassIndex == -1) { // slowpath
        acc_.DeleteGuardAndFrameState(gate);
        return;
    }
    JSTaggedValue hclass = tsManager_->GetHClassFromCache(hclassIndex-constantPool_->GetCacheLength());

    auto propertyOffset = tsManager_->GetPropertyOffset(hclass, prop);
    if (propertyOffset == -1) { // slowpath
        acc_.DeleteGuardAndFrameState(gate);
        return;
    }

    GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
    GateRef propertyOffsetGate = builder_.IntPtr(propertyOffset);
    GateRef check = builder_.ObjectTypeCheck(receiverType, receiver, hclassIndexGate);

    // guard maybe not a GUARD
    GateRef guard = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
    acc_.ReplaceIn(guard, 1, check);

    acc_.SetDep(guard, builder_.GetDepend());
    builder_.SetDepend(guard);
    GateRef result = builder_.LoadProperty(receiver, propertyOffsetGate);

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerTypedStObjByName(GateRef gate, GateRef thisObj, bool isThis)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto propIndex = acc_.GetBitField(acc_.GetValueIn(gate, 1));
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    auto prop = ConstantPool::GetStringFromCache(thread, constantPool_.GetTaggedValue(), propIndex);

    GateRef receiver = Circuit::NullGate();
    GateRef value = Circuit::NullGate();
    if (isThis) {
        // 3: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 3);
        receiver = thisObj;
        value = acc_.GetValueIn(gate, 2);
    } else {
        // 4: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 4);
        receiver = acc_.GetValueIn(gate, 2);
        value = acc_.GetValueIn(gate, 3);
    }
    GateType receiverType = acc_.GetGateType(receiver);
    int hclassIndex = tsManager_->GetHClassIndex(receiverType);
    if (hclassIndex == -1) { // slowpath
        acc_.DeleteGuardAndFrameState(gate);
        return;
    }
    JSTaggedValue hclass = tsManager_->GetHClassFromCache(hclassIndex-constantPool_->GetCacheLength());

    auto propertyOffset = tsManager_->GetPropertyOffset(hclass, prop);
    if (propertyOffset == -1) { // slowpath
        acc_.DeleteGuardAndFrameState(gate);
        return;
    }

    GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
    GateRef propertyOffsetGate = builder_.IntPtr(propertyOffset);
    GateRef check = builder_.ObjectTypeCheck(receiverType, receiver, hclassIndexGate);

    // guard maybe not a GUARD
    GateRef guard = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
    acc_.ReplaceIn(guard, 1, check);

    acc_.SetDep(guard, builder_.GetDepend());
    builder_.SetDepend(guard);
    builder_.StoreProperty(receiver, propertyOffsetGate, value);

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, Circuit::NullGate(), builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}


void TSTypeLowering::LowerTypedLdObjByIndex(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateType receiverType = acc_.GetGateType(receiver);
    if (!tsManager_->IsFloat32ArrayType(receiverType)) { // slowpath
        acc_.DeleteGuardAndFrameState(gate);
        return;
    }

    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef check = builder_.ObjectTypeCheck(receiverType, receiver, index);

    // guard maybe not a GUARD
    GateRef guard = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
    acc_.ReplaceIn(guard, 1, check);

    acc_.SetDep(guard, builder_.GetDepend());
    builder_.SetDepend(guard);
    GateRef result = builder_.LoadElement(receiver, index);

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerTypedStObjByIndex(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 2);
    GateType receiverType = acc_.GetGateType(receiver);
    GateType valueType = acc_.GetGateType(value);
    if ((!tsManager_->IsFloat32ArrayType(receiverType)) || (!valueType.IsNumberType())) { // slowpath
        acc_.DeleteGuardAndFrameState(gate);
        return;
    }

    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef check = builder_.ObjectTypeCheck(receiverType, receiver, index);

    if (!IsTrustedType(value)) {
        check = builder_.BoolAnd(check, builder_.TypeCheck(valueType, value));
    }

    // guard maybe not a GUARD
    GateRef guard = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
    acc_.ReplaceIn(guard, 1, check);
    acc_.SetDep(guard, builder_.GetDepend());
    builder_.SetDepend(guard);
    builder_.StoreElement(receiver, index, value);

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, Circuit::NullGate(), builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerTypedIsTrueOrFalse(GateRef gate, bool flag)
{
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    auto value = acc_.GetValueIn(gate, 0);
    auto valueType = acc_.GetGateType(value);
    if ((!valueType.IsNumberType()) && (!valueType.IsBooleanType())) {
        acc_.DeleteGuardAndFrameState(gate);
        return;
    }
    auto check = Circuit::NullGate();
    if (IsTrustedType(value)) {
        acc_.DeleteGuardAndFrameState(gate);
        builder_.SetDepend(acc_.GetDep(gate));
    } else {
        check = builder_.TypeCheck(valueType, value);
    }

    // guard maybe not a GUARD
    auto guard = acc_.GetDep(gate);
    if (check != Circuit::NullGate()) {
        ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
        acc_.ReplaceIn(guard, 1, check);
    }
    auto toBool = builder_.TypedUnaryOp<TypedUnOp::TYPED_TOBOOL>(value, valueType, GateType::NJSValue());
    if (!flag) {
        toBool = builder_.BoolNot(toBool);
    }
    auto result = builder_.BooleanToTaggedBooleanPtr(toBool);
    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}
}  // namespace panda::ecmascript
