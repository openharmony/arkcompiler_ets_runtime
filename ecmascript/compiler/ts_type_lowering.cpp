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
#include "ecmascript/compiler/builtins_lowering.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"
#include "ecmascript/ts_types/ts_type.h"

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
        circuit_->PrintAllGatesWithBytecode();
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
            if (acc_.GetOpCode(depend) == OpCode::STATE_SPLIT) {
                auto opcode = acc_.GetByteCodeOpcode(gate);
                std::string bytecodeStr = GetEcmaOpcodeStr(opcode);
                LOG_COMPILER(ERROR) << "[ts_type_lowering][Error] the depend of ["
                                    << "id: " << acc_.GetId(gate) << ", JS_BYTECODE: " << bytecodeStr
                                    << "] should not be STATE_SPLIT after ts type lowring";
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
        EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
        switch (ecmaOpcode) {
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
    GateRef jsFunc = argAcc.GetCommonArgGate(CommonArgIdx::FUNC);
    GateRef newTarget = argAcc.GetCommonArgGate(CommonArgIdx::NEW_TARGET);

    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
    switch (ecmaOpcode) {
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
            LowerTypedAnd(gate);
            break;
        case EcmaOpcode::OR2_IMM8_V8:
            LowerTypedOr(gate);
            break;
        case EcmaOpcode::XOR2_IMM8_V8:
            LowerTypedXor(gate);
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
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
            LowerTypedLdObjByName(gate);
            break;
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
            LowerTypedStObjByName(gate, false);
            break;
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
            LowerTypedStObjByName(gate, true);
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
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
            LowerTypedLdObjByValue(gate, false);
            break;
        case EcmaOpcode::LDTHISBYVALUE_IMM8:
        case EcmaOpcode::LDTHISBYVALUE_IMM16:
            LowerTypedLdObjByValue(gate, true);
            break;
        case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8:
        case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
            LowerTypedNewObjRange(gate);
            break;
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
            LowerTypedSuperCall(gate, jsFunc, newTarget);
            break;
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
            LowerCallThis1Imm8V8V8(gate);
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
                    GateRef stateEntry = circuit_->GetStateRoot();
                    GateRef dependEntry = circuit_->GetDependRoot();
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
    bool expFound = false;
    for (auto tmpUseIt = uses.begin(); tmpUseIt != uses.end(); tmpUseIt++) {
        if (acc_.GetOpCode(*tmpUseIt) == OpCode::IF_EXCEPTION) {
            auto expUses = acc_.Uses(*tmpUseIt);
            for (auto expUseIt = expUses.begin(); expUseIt != expUses.end(); ++expUseIt) {
                if (acc_.GetOpCode(*expUseIt) == OpCode::MERGE && acc_.GetNumIns(*expUseIt) == 1) {
                    expFound = true;
                    break;
                }
            }
        }
    }
    if (expFound) {
        GateRef ifBranch = builder_.Branch(state, builder_.Boolean(false));
        for (auto it = uses.begin(); it != uses.end();) {
            if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS) {
                acc_.SetMetaData(*it, circuit_->IfFalse());
                it = acc_.ReplaceIn(it, ifBranch);
            } else {
                if (acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
                    acc_.SetMetaData(*it, circuit_->IfTrue());
                    it = acc_.ReplaceIn(it, ifBranch);
                } else {
                    it++;
                }
            }
        }
    }

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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedAnd(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_AND>(gate);
    } else {
        acc_.DeleteStateSplitAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedOr(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_OR>(gate);
    } else {
        acc_.DeleteStateSplitAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedXor(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        SpeculateNumbers<TypedBinOp::TYPED_XOR>(gate);
    } else {
        acc_.DeleteStateSplitAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedInc(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsNumberType()) {
        SpeculateNumber<TypedUnOp::TYPED_INC>(gate);
    } else {
        acc_.DeleteStateSplitAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedDec(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsNumberType()) {
        SpeculateNumber<TypedUnOp::TYPED_DEC>(gate);
    } else {
        acc_.DeleteStateSplitAndFrameState(gate);
    }
}

template<TypedBinOp Op>
void TSTypeLowering::SpeculateNumbers(GateRef gate)
{
    AddProfiling(gate);
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    GateType gateType = acc_.GetGateType(gate);
    builder_.PrimitiveTypeCheck(leftType, left);
    builder_.PrimitiveTypeCheck(rightType, right);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.TypedBinaryOp<Op>(left, right, leftType, rightType, gateType);

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

bool TSTypeLowering::NeedInt32OverflowCheck(TypedUnOp op) const
{
    if (op == TypedUnOp::TYPED_INC || op == TypedUnOp::TYPED_DEC ||
        op == TypedUnOp::TYPED_NEG) {
        return true;
    }
    return false;
}

template<TypedUnOp Op>
void TSTypeLowering::SpeculateNumber(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    GateType gateType = acc_.GetGateType(gate);

    builder_.PrimitiveTypeCheck(valueType, value);
    if (valueType.IsIntType() && NeedInt32OverflowCheck(Op)) {
        builder_.Int32OverflowCheck<Op>(value);
    }

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.TypedUnaryOp<Op>(value, valueType, gateType);

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerTypeToNumeric(GateRef gate)
{
    GateRef src = acc_.GetValueIn(gate, 0);
    GateType srcType = acc_.GetGateType(src);
    if (srcType.IsDigitablePrimitiveType()) {
        AddProfiling(gate);
        LowerPrimitiveTypeToNumber(gate);
    } else {
        acc_.DeleteStateSplitAndFrameState(gate);
    }
}

void TSTypeLowering::LowerPrimitiveTypeToNumber(GateRef gate)
{
    GateRef src = acc_.GetValueIn(gate, 0);
    GateType srcType = acc_.GetGateType(src);
    builder_.PrimitiveTypeCheck(srcType, src);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.PrimitiveToNumber(src, VariableType(MachineType::I64, srcType));

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerConditionJump(GateRef gate)
{
    GateRef condition = acc_.GetValueIn(gate, 0);
    GateType conditionType = acc_.GetGateType(condition);
    if (conditionType.IsBooleanType() && IsTrustedType(condition)) {
        AddProfiling(gate);
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
        acc_.DeleteStateSplitAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedNot(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsNumberType()) {
        SpeculateNumber<TypedUnOp::TYPED_NOT>(gate);
    } else {
        acc_.DeleteStateSplitAndFrameState(gate);
    }
}

void TSTypeLowering::LowerTypedLdArrayLength(GateRef gate)
{
    AddProfiling(gate);
    GateRef array = acc_.GetValueIn(gate, 2);
    builder_.ArrayCheck(array);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef loadLength = builder_.LoadArrayLength(array);

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, loadLength, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerTypedLdObjByName(GateRef gate)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint16_t propIndex = acc_.GetConstDataId(acc_.GetValueIn(gate, 1)).GetId();
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    JSHandle<ConstantPool> constantPool(tsManager_->GetConstantPool());
    auto prop = ConstantPool::GetStringFromCache(thread, constantPool.GetTaggedValue(), propIndex);

    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 2); // 2: acc or this object
    GateType receiverType = acc_.GetGateType(receiver);
    if (tsManager_->IsArrayTypeKind(receiverType)) {
        EcmaString *propString = EcmaString::Cast(prop.GetTaggedObject());
        EcmaString *lengthString = EcmaString::Cast(thread->GlobalConstants()->GetLengthString().GetTaggedObject());
        if (propString == lengthString) {
            LowerTypedLdArrayLength(gate);
            return;
        }
    }

    int hclassIndex = tsManager_->GetHClassIndexByInstanceGateType(receiverType);
    if (hclassIndex == -1) { // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    JSTaggedValue hclass = tsManager_->GetHClassFromCache(hclassIndex);

    auto propertyOffset = tsManager_->GetPropertyOffset(hclass, prop);
    if (propertyOffset == -1) { // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);

    GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
    GateRef propertyOffsetGate = builder_.IntPtr(propertyOffset);
    builder_.ObjectTypeCheck(receiverType, receiver, hclassIndexGate);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.LoadProperty(receiver, propertyOffsetGate);

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerTypedStObjByName(GateRef gate, bool isThis)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint16_t propIndex = acc_.GetConstDataId(acc_.GetValueIn(gate, 1)).GetId();
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    JSHandle<ConstantPool> constantPool(tsManager_->GetConstantPool());
    auto prop = ConstantPool::GetStringFromCache(thread, constantPool.GetTaggedValue(), propIndex);

    GateRef receiver = Circuit::NullGate();
    GateRef value = Circuit::NullGate();
    if (isThis) {
        // 4: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 4);
        receiver = acc_.GetValueIn(gate, 3); // 3: this object
        value = acc_.GetValueIn(gate, 2); // 2: acc
    } else {
        // 4: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 4);
        receiver = acc_.GetValueIn(gate, 2); // 2: receiver
        value = acc_.GetValueIn(gate, 3); // 3: acc
    }
    GateType receiverType = acc_.GetGateType(receiver);
    int hclassIndex = tsManager_->GetHClassIndexByInstanceGateType(receiverType);
    if (hclassIndex == -1) { // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    JSTaggedValue hclass = tsManager_->GetHClassFromCache(hclassIndex);

    auto propertyOffset = tsManager_->GetPropertyOffset(hclass, prop);
    if (propertyOffset == -1) { // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);

    GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
    GateRef propertyOffsetGate = builder_.IntPtr(propertyOffset);
    builder_.ObjectTypeCheck(receiverType, receiver, hclassIndexGate);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
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
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);

    if (tsManager_->IsFloat32ArrayType(receiverType)) {
        builder_.TypedArrayCheck(receiverType, receiver);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    GateRef index = acc_.GetValueIn(gate, 0);
    builder_.IndexCheck(receiverType, receiver, index);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = Circuit::NullGate();
    if (tsManager_->IsFloat32ArrayType(receiverType)) {
        result = builder_.LoadElement<TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT>(receiver, index);
    } else {
        UNREACHABLE();
    }

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
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);

    if (tsManager_->IsFloat32ArrayType(receiverType)) {
        builder_.TypedArrayCheck(receiverType, receiver);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    GateRef index = acc_.GetValueIn(gate, 1);
    builder_.IndexCheck(receiverType, receiver, index);
    builder_.PrimitiveTypeCheck(valueType, value);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    if (tsManager_->IsFloat32ArrayType(receiverType)) {
        builder_.StoreElement<TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT>(receiver, index, value);
    } else {
        UNREACHABLE();
    }

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, Circuit::NullGate(), builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerTypedLdObjByValue(GateRef gate, bool isThis)
{
    GateRef receiver = Circuit::NullGate();
    GateRef propKey = Circuit::NullGate();
    if (isThis) {
        // 3: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 3);
        receiver = acc_.GetValueIn(gate, 2); // 2: this object
        propKey = acc_.GetValueIn(gate, 1);
    } else {
        // 3: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 3);
        receiver = acc_.GetValueIn(gate, 1);
        propKey = acc_.GetValueIn(gate, 2);  // 2: the third parameter
    }
    GateType receiverType = acc_.GetGateType(receiver);
    GateType propKeyType = acc_.GetGateType(propKey);
    if (!tsManager_->IsArrayTypeKind(receiverType) || !propKeyType.IsIntType()) { // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);

    builder_.PrimitiveTypeCheck(propKeyType, propKey);
    builder_.StableArrayCheck(receiver);
    builder_.IndexCheck(receiverType, receiver, propKey);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_ELEMENT>(receiver, propKey);

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerTypedIsTrueOrFalse(GateRef gate, bool flag)
{
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    auto value = acc_.GetValueIn(gate, 0);
    auto valueType = acc_.GetGateType(value);
    if ((!valueType.IsNumberType()) && (!valueType.IsBooleanType())) {
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);

    builder_.PrimitiveTypeCheck(valueType, value);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    auto toBool = builder_.TypedUnaryOp<TypedUnOp::TYPED_TOBOOL>(value, valueType, GateType::NJSValue());
    if (!flag) {
        toBool = builder_.BoolNot(toBool);
    }
    auto result = builder_.BooleanToTaggedBooleanPtr(toBool);

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

void TSTypeLowering::LowerTypedNewObjRange(GateRef gate)
{
    GateRef ctor = acc_.GetValueIn(gate, 0);
    GateType ctorType = acc_.GetGateType(ctor);
    if (!tsManager_->IsClassTypeKind(ctorType)) {
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);

    int hclassIndex = tsManager_->GetHClassIndexByClassGateType(ctorType);
    GateRef stateSplit = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(stateSplit) == OpCode::STATE_SPLIT);

    GateRef frameState = acc_.GetFrameState(stateSplit);
    GateRef thisObj = builder_.TypedNewAllocateThis(ctor, builder_.IntPtr(hclassIndex), frameState);

    // call constructor
    size_t range = acc_.GetNumValueIn(gate);
    GateRef envArg = builder_.Undefined();
    GateRef actualArgc = builder_.Int64(range + 2);  // 2:newTaget, this
    std::vector<GateRef> args { glue_, envArg, actualArgc, ctor, ctor, thisObj };
    for (size_t i = 1; i < range; ++i) {  // 1:skip ctor
        args.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef bcIndex = builder_.Int64(acc_.GetBytecodeIndex(gate));
    args.emplace_back(bcIndex);

    GateRef constructGate = builder_.Construct(args);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), constructGate);
}

void TSTypeLowering::LowerTypedSuperCall(GateRef gate, GateRef ctor, GateRef newTarget)
{
    GateType ctorType = acc_.GetGateType(ctor);  // ldfunction in derived constructor get function type
    if (!tsManager_->IsClassTypeKind(ctorType) && !tsManager_->IsFunctionTypeKind(ctorType)) {
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);

    // stateSplit maybe not a STATE_SPLIT
    GateRef stateSplit = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(stateSplit) == OpCode::STATE_SPLIT);

    GateRef frameState = acc_.GetFrameState(stateSplit);
    GateRef superCtor = builder_.GetSuperConstructor(ctor);
    GateRef thisObj = builder_.TypedSuperAllocateThis(superCtor, newTarget, frameState);


    // call constructor
    size_t range = acc_.GetNumValueIn(gate);
    GateRef envArg = builder_.Undefined();
    GateRef actualArgc = builder_.Int64(range + 3);  // 3: ctor, newTaget, this
    std::vector<GateRef> args { glue_, envArg, actualArgc, superCtor, newTarget, thisObj };
    for (size_t i = 0; i < range; ++i) {
        args.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef bcIndex = builder_.Int64(acc_.GetBytecodeIndex(gate));
    args.emplace_back(bcIndex);

    GateRef constructGate = builder_.Construct(args);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), constructGate);
}

void TSTypeLowering::SpeculateCallBuiltin(GateRef gate, BuiltinsStubCSigns::ID id)
{
    GateRef function = acc_.GetValueIn(gate, 2); // 2:function
    GateRef a0 = acc_.GetValueIn(gate, 1);
    builder_.CallTargetCheck(function, builder_.IntPtr(static_cast<int64_t>(id)), a0);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.TypedCallBuiltin(a0, id);

    std::vector<GateRef> removedGate;
    ReplaceHIRGate(gate, result, builder_.GetState(), builder_.GetDepend(), removedGate);
    DeleteGates(gate, removedGate);
}

BuiltinsStubCSigns::ID TSTypeLowering::GetBuiltinId(GateRef func, GateRef receiver)
{
    GateType receiverType = acc_.GetGateType(receiver);
    if (!tsManager_->IsBuiltinMath(receiverType)) {
        return BuiltinsStubCSigns::ID::NONE;
    }
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsBuiltin(funcType)) {
        return BuiltinsStubCSigns::ID::NONE;
    }
    std::string name = tsManager_->GetFuncName(funcType);
    BuiltinsStubCSigns::ID id = BuiltinLowering::GetBuiltinId(name);
    return id;
}

void TSTypeLowering::LowerCallThis1Imm8V8V8(GateRef gate)
{
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef a0 = acc_.GetValueIn(gate, 1); // 1:parameter index
    GateType a0Type = acc_.GetGateType(a0);
    GateRef func = acc_.GetValueIn(gate, 2); // 2:function
    BuiltinsStubCSigns::ID id = GetBuiltinId(func, thisObj);
    if (id != BuiltinsStubCSigns::ID::NONE && a0Type.IsNumberType()) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, id);
    } else {
        acc_.DeleteStateSplitAndFrameState(gate);
    }
}

void TSTypeLowering::AddProfiling(GateRef gate)
{
    if (IsProfiling()) {
        // see stateSplit as a part of JSByteCode if exists
        GateRef maybeStateSplit = acc_.GetDep(gate);
        GateRef current = Circuit::NullGate();
        if (acc_.GetOpCode(maybeStateSplit) == OpCode::STATE_SPLIT) {
            current = maybeStateSplit;
        } else {
            current = gate;
        }

        EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
        auto ecmaOpcodeGate = builder_.Int32(static_cast<uint32_t>(ecmaOpcode));
        GateRef constOpcode = builder_.Int32ToTaggedInt(ecmaOpcodeGate);
        GateRef mode =
            builder_.Int32ToTaggedInt(builder_.Int32(static_cast<int32_t>(OptCodeProfiler::Mode::TYPED_PATH)));
        GateRef profiling = builder_.CallRuntime(glue_, RTSTUB_ID(ProfileOptimizedCode), acc_.GetDep(current),
                                                 {constOpcode, mode});
        acc_.SetDep(current, profiling);
        builder_.SetDepend(acc_.GetDep(gate));  // set gate depend: profiling or STATE_SPLIT
    }
}
}  // namespace panda::ecmascript
