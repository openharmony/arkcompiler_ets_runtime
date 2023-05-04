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
#include "ecmascript/compiler/bytecodes.h"
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
            case EcmaOpcode::GREATER_IMM8_V8:
            case EcmaOpcode::GREATEREQ_IMM8_V8:
            case EcmaOpcode::EQ_IMM8_V8:
            case EcmaOpcode::NOTEQ_IMM8_V8:
            case EcmaOpcode::STRICTEQ_IMM8_V8:
            case EcmaOpcode::STRICTNOTEQ_IMM8_V8:
            case EcmaOpcode::ISTRUE:
            case EcmaOpcode::ISFALSE:
                return true;
            default:
                break;
        }
    }
    return false;
}

void TSTypeLowering::Lower(GateRef gate)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
    switch (ecmaOpcode) {
        case EcmaOpcode::ADD2_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_ADD>(gate);
            break;
        case EcmaOpcode::SUB2_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_SUB>(gate);
            break;
        case EcmaOpcode::MUL2_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_MUL>(gate);
            break;
        case EcmaOpcode::DIV2_IMM8_V8:
            LowerTypedDiv(gate);
            break;
        case EcmaOpcode::MOD2_IMM8_V8:
            LowerTypedMod(gate);
            break;
        case EcmaOpcode::LESS_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_LESS>(gate);
            break;
        case EcmaOpcode::LESSEQ_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_LESSEQ>(gate);
            break;
        case EcmaOpcode::GREATER_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_GREATER>(gate);
            break;
        case EcmaOpcode::GREATEREQ_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_GREATEREQ>(gate);
            break;
        case EcmaOpcode::EQ_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_EQ>(gate);
            break;
        case EcmaOpcode::STRICTEQ_IMM8_V8:
            LowerTypedStrictEq(gate);
            break;
        case EcmaOpcode::NOTEQ_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_NOTEQ>(gate);
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
            LowerConditionJump(gate, false);
            break;
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
        case EcmaOpcode::JNEZ_IMM32:
            LowerConditionJump(gate, true);
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
        case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
            acc_.DeleteStateSplitAndFrameState(gate);
            break;
        case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8:
        case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
            LowerTypedNewObjRange(gate);
            break;
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
            LowerTypedSuperCall(gate);
            break;
        case EcmaOpcode::CALLARG0_IMM8:
            LowerTypedCallArg0(gate);
            break;
        case EcmaOpcode::CALLARG1_IMM8_V8:
            LowerTypedCallArg1(gate);
            break;
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
            LowerTypedCallArg2(gate);
            break;
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
            LowerTypedCallArg3(gate);
            break;
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
            LowerTypedCallrange(gate);
            break;
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
            LowerTypedCallthis0(gate);
            break;
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
            LowerTypedCallthis1(gate);
            break;
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
            LowerTypedCallthis2(gate);
            break;
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
            LowerTypedCallthis3(gate);
            break;
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
            LowerTypedCallthisrange(gate);
            break;
        default:
            break;
    }
}

template<TypedBinOp Op>
void TSTypeLowering::LowerTypedBinOp(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (HasNumberType(gate, left, right)) {
        SpeculateNumbers<Op>(gate);
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

void TSTypeLowering::LowerTypedStrictEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    GateType gateType = acc_.GetGateType(gate);
    PGOSampleType sampleType = acc_.TryGetPGOType(gate);
    if (acc_.IsConstantUndefined(left) || acc_.IsConstantUndefined(right) || HasNumberType(gate, left, right)) {
        GateRef result = builder_.TypedBinaryOp<TypedBinOp::TYPED_STRICTEQ>(
            left, right, leftType, rightType, gateType, sampleType);
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
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

bool TSTypeLowering::HasNumberType(GateRef gate, GateRef left, GateRef right) const
{
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);

    PGOSampleType sampleType = acc_.TryGetPGOType(gate);
    if (sampleType.IsNumber() ||
        (sampleType.IsNone() && leftType.IsNumberType() && rightType.IsNumberType())) {
        return true;
    }
    return false;
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
    PGOSampleType sampleType = acc_.TryGetPGOType(gate);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.TypedBinaryOp<Op>(left, right, leftType, rightType, gateType, sampleType);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

bool TSTypeLowering::NeedInt32OverflowCheck(TypedUnOp op) const
{
    if (op == TypedUnOp::TYPED_NEG) {
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

    if (valueType.IsIntType() && NeedInt32OverflowCheck(Op)) {
        builder_.Int32OverflowCheck<Op>(value);
    }

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.TypedUnaryOp<Op>(value, valueType, gateType);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
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

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.PrimitiveToNumber(src, VariableType(MachineType::I64, srcType));

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSTypeLowering::LowerConditionJump(GateRef gate, bool flag)
{
    GateRef condition = acc_.GetValueIn(gate, 0);
    GateType conditionType = acc_.GetGateType(condition);
    if (conditionType.IsBooleanType() && IsTrustedType(condition)) {
        AddProfiling(gate);
        SpeculateConditionJump(gate, flag);
    }
}

void TSTypeLowering::SpeculateConditionJump(GateRef gate, bool flag)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    GateRef jump = Circuit::NullGate();
    if (flag) {
        jump = builder_.TypedConditionJump<TypedJumpOp::TYPED_JNEZ>(value, valueType);
    } else {
        jump = builder_.TypedConditionJump<TypedJumpOp::TYPED_JEQZ>(value, valueType);
    }
    acc_.ReplaceGate(gate, jump, jump, Circuit::NullGate());
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
    builder_.StableArrayCheck(array);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.LoadArrayLength(array);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSTypeLowering::DeleteConstDataIfNoUser(GateRef gate)
{
    auto uses = acc_.Uses(gate);
    if (uses.begin() == uses.end()) {
        acc_.DeleteGate(gate);
    }
}

void TSTypeLowering::LowerTypedLdObjByName(GateRef gate)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto constData = acc_.GetValueIn(gate, 1); // 1: valueIn 1
    uint16_t propIndex = acc_.GetConstDataId(constData).GetId();
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    JSHandle<ConstantPool> constantPool(tsManager_->GetConstantPool());
    auto prop = ConstantPool::GetStringFromCache(thread, constantPool.GetTaggedValue(), propIndex);

    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 2); // 2: acc or this object
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
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
    JSHClass *hclass = JSHClass::Cast(tsManager_->GetHClassFromCache(hclassIndex).GetTaggedObject());
    if (!hclass->HasTSSubtyping()) {  // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    PropertyLookupResult plr = JSHClass::LookupProperty(thread, hclass, prop);
    if (!plr.IsFound()) {  // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);

    GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
    builder_.ObjectTypeCheck(receiverType, receiver, hclassIndexGate);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef pfrGate = builder_.Int32(plr.GetData());
    GateRef result = Circuit::NullGate();
    if (LIKELY(!plr.IsAccessor())) {
        result = builder_.LoadProperty(receiver, pfrGate, plr.IsVtable());
    } else {
        result = builder_.CallGetter(gate, receiver, pfrGate);
    }

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    DeleteConstDataIfNoUser(constData);
}

void TSTypeLowering::LowerTypedStObjByName(GateRef gate, bool isThis)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto constData = acc_.GetValueIn(gate, 1); // 1: valueIn 1
    uint16_t propIndex = acc_.GetConstDataId(constData).GetId();
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    JSHandle<ConstantPool> constantPool(tsManager_->GetConstantPool());
    auto prop = ConstantPool::GetStringFromCache(thread, constantPool.GetTaggedValue(), propIndex);

    GateRef receiver = Circuit::NullGate();
    GateRef value = Circuit::NullGate();
    if (isThis) {
        // 3: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 3);
        receiver = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
        value = acc_.GetValueIn(gate, 2); // 2: acc
    } else {
        // 4: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 4);
        receiver = acc_.GetValueIn(gate, 2); // 2: receiver
        value = acc_.GetValueIn(gate, 3); // 3: acc
    }
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    int hclassIndex = tsManager_->GetHClassIndexByInstanceGateType(receiverType);
    if (hclassIndex == -1) { // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    JSHClass *hclass = JSHClass::Cast(tsManager_->GetHClassFromCache(hclassIndex).GetTaggedObject());
    if (!hclass->HasTSSubtyping()) {  // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    PropertyLookupResult plr = JSHClass::LookupProperty(thread, hclass, prop);
    if (!plr.IsFound() || plr.IsFunction()) {  // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);

    GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
    builder_.ObjectTypeCheck(receiverType, receiver, hclassIndexGate);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef pfrGate = builder_.Int32(plr.GetData());
    if (LIKELY(plr.IsLocal())) {
        builder_.StoreProperty(receiver, pfrGate, value);
    } else {
        builder_.CallSetter(gate, receiver, pfrGate, value);
    }

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
    DeleteConstDataIfNoUser(constData);
}

void TSTypeLowering::LowerTypedLdObjByIndex(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
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
    uint32_t indexValue = static_cast<uint32_t>(acc_.GetConstantValue(index));
    index = builder_.Int32(indexValue);
    builder_.IndexCheck(receiverType, receiver, index);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = Circuit::NullGate();
    if (tsManager_->IsFloat32ArrayType(receiverType)) {
        result = builder_.LoadElement<TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT>(receiver, index);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSTypeLowering::LowerTypedStObjByIndex(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 2);
    GateType receiverType = acc_.GetGateType(receiver);
    GateType valueType = acc_.GetGateType(value);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
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
    uint32_t indexValue = static_cast<uint32_t>(acc_.GetConstantValue(index));
    index = builder_.Int32(indexValue);
    builder_.IndexCheck(receiverType, receiver, index);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    if (tsManager_->IsFloat32ArrayType(receiverType)) {
        builder_.StoreElement<TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT>(receiver, index, value);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
}

void TSTypeLowering::LowerTypedLdObjByValue(GateRef gate, bool isThis)
{
    GateRef receiver = Circuit::NullGate();
    GateRef propKey = Circuit::NullGate();
    if (isThis) {
        // 2: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 2);
        receiver = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
        propKey = acc_.GetValueIn(gate, 1);
    } else {
        // 3: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 3);
        receiver = acc_.GetValueIn(gate, 1);
        propKey = acc_.GetValueIn(gate, 2);  // 2: the third parameter
    }
    GateType receiverType = acc_.GetGateType(receiver);
    GateType propKeyType = acc_.GetGateType(propKey);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if (!tsManager_->IsArrayTypeKind(receiverType) || !propKeyType.IsNumberType()) { // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);

    builder_.StableArrayCheck(receiver);
    GateRef length = builder_.LoadArrayLength(receiver);
    builder_.IndexCheck(receiverType, length, propKey);
    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_ELEMENT>(receiver, propKey);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSTypeLowering::LowerTypedStObjByValue(GateRef gate)
{
    ASSERT(acc_.GetNumValueIn(gate) == 4);  // 4: num of value ins
    GateRef receiver = acc_.GetValueIn(gate, 1);    // 1: receiver
    GateRef propKey = acc_.GetValueIn(gate, 2);  // 2: key
    GateRef value = acc_.GetValueIn(gate, 3);   // 3: value
    GateType receiverType = acc_.GetGateType(receiver);
    GateType propKeyType = acc_.GetGateType(propKey);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if (!tsManager_->IsArrayTypeKind(receiverType) || !propKeyType.IsNumberType()) { // slowpath
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }

    AddProfiling(gate);
    builder_.StableArrayCheck(receiver);
    GateRef length = builder_.LoadArrayLength(receiver);
    builder_.IndexCheck(receiverType, length, propKey);
    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    builder_.StoreElement<TypedStoreOp::ARRAY_STORE_ELEMENT>(receiver, propKey, value);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
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

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);

    GateRef result;
    if (!flag) {
        result = builder_.TypedUnaryOp<TypedUnOp::TYPED_ISFALSE>(value, valueType, GateType::TaggedValue());
    } else {
        result = builder_.TypedUnaryOp<TypedUnOp::TYPED_ISTRUE>(value, valueType, GateType::TaggedValue());
    }

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
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
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(range, EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8));
    std::vector<GateRef> args { glue_, actualArgc, ctor, ctor, thisObj };
    for (size_t i = 1; i < range; ++i) {  // 1:skip ctor
        args.emplace_back(acc_.GetValueIn(gate, i));
    }

    GateRef constructGate = builder_.Construct(gate, args);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), constructGate);
}

void TSTypeLowering::LowerTypedSuperCall(GateRef gate)
{
    GateRef ctor = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::FUNC);
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
    GateRef newTarget = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::NEW_TARGET);
    GateRef thisObj = builder_.TypedSuperAllocateThis(superCtor, newTarget, frameState);

    // call constructor
    size_t range = acc_.GetNumValueIn(gate);
    GateRef actualArgc = builder_.Int64(range + 3);  // 3: ctor, newTaget, this
    std::vector<GateRef> args { glue_, actualArgc, superCtor, newTarget, thisObj };
    for (size_t i = 0; i < range; ++i) {
        args.emplace_back(acc_.GetValueIn(gate, i));
    }

    GateRef constructGate = builder_.Construct(gate, args);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), constructGate);
}

void TSTypeLowering::SpeculateCallBuiltin(GateRef gate, GateRef func, GateRef a0, BuiltinsStubCSigns::ID id)
{
    builder_.CallTargetCheck(func, builder_.IntPtr(static_cast<int64_t>(id)), a0);

    ASSERT(acc_.GetOpCode(acc_.GetDep(gate)) == OpCode::STATE_SPLIT);
    GateRef result = builder_.TypedCallBuiltin(gate, a0, id);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

BuiltinsStubCSigns::ID TSTypeLowering::GetBuiltinId(GateRef func)
{
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsBuiltinMath(funcType)) {
        return BuiltinsStubCSigns::ID::NONE;
    }
    std::string name = tsManager_->GetFuncName(funcType);
    BuiltinsStubCSigns::ID id = BuiltinsStubCSigns::GetBuiltinId(name);
    return id;
}

void TSTypeLowering::LowerTypedCallArg0(GateRef gate)
{
    GateRef func = acc_.GetValueIn(gate, 0);
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GlobalTSTypeRef funcGt = funcType.GetGTRef();
    uint32_t len = tsManager_->GetFunctionTypeLength(funcGt);
    if (len != 0) {
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLARG0_IMM8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    if (IsLoadVtable(func)) {
        builder_.JSCallThisTargetTypeCheck(funcType, func);
        std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj };

        GateRef result = builder_.TypedAotCall(gate, args);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    } else {
        int methodIndex = tsManager_->GetMethodIndex(funcGt);
        if (methodIndex == -1) {
            acc_.DeleteStateSplitAndFrameState(gate);
            return;
        }
        builder_.JSCallTargetTypeCheck(funcType, func, builder_.IntPtr(methodIndex));
        std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj };

        GateRef result = builder_.TypedAotCall(gate, args);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    }
}

void TSTypeLowering::LowerTypedCallArg1(GateRef gate)
{
    GateRef func = acc_.GetValueIn(gate, 1);
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GlobalTSTypeRef funcGt = funcType.GetGTRef();
    uint32_t len = tsManager_->GetFunctionTypeLength(funcGt);
    if (len != 1) { // 1: 1 params
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GateRef a0Value = acc_.GetValueIn(gate, 0);
    GateType a0Type = acc_.GetGateType(a0Value);
    BuiltinsStubCSigns::ID id = GetBuiltinId(func);
    if (id != BuiltinsStubCSigns::ID::NONE && a0Type.IsNumberType()) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, func, a0Value, id);
    } else {
        GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
            EcmaOpcode::CALLARG1_IMM8_V8));
        GateRef newTarget = builder_.Undefined();
        GateRef thisObj = builder_.Undefined();
        if (IsLoadVtable(func)) {
            builder_.JSCallThisTargetTypeCheck(funcType, func);
            std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj, a0Value };
            GateRef result = builder_.TypedAotCall(gate, args);
            acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
        } else {
            int methodIndex = tsManager_->GetMethodIndex(funcGt);
            if (methodIndex == -1) {
                acc_.DeleteStateSplitAndFrameState(gate);
                return;
            }
            builder_.JSCallTargetTypeCheck(funcType, func, builder_.IntPtr(methodIndex));
            std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj, a0Value };
            GateRef result = builder_.TypedAotCall(gate, args);
            acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
        }
    }
}

void TSTypeLowering::LowerTypedCallArg2(GateRef gate)
{
    GateRef func = acc_.GetValueIn(gate, 2); // 2:function
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GlobalTSTypeRef funcGt = funcType.GetGTRef();
    uint32_t len = tsManager_->GetFunctionTypeLength(funcGt);
    if (len != 2) { // 2: 2 params
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLARGS2_IMM8_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef a0 = acc_.GetValueIn(gate, 0);
    GateRef a1 = acc_.GetValueIn(gate, 1); // 1:first parameter
    if (IsLoadVtable(func)) {
        builder_.JSCallThisTargetTypeCheck(funcType, func);
        std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj, a0, a1 };
        GateRef result = builder_.TypedAotCall(gate, args);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    } else {
        int methodIndex = tsManager_->GetMethodIndex(funcGt);
        if (methodIndex == -1) {
            acc_.DeleteStateSplitAndFrameState(gate);
            return;
        }
        builder_.JSCallTargetTypeCheck(funcType, func, builder_.IntPtr(methodIndex));
        std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj, a0, a1 };
        GateRef result = builder_.TypedAotCall(gate, args);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    }
}

void TSTypeLowering::LowerTypedCallArg3(GateRef gate)
{
    GateRef func = acc_.GetValueIn(gate, 3); // 3:function
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GlobalTSTypeRef funcGt = funcType.GetGTRef();
    uint32_t len = tsManager_->GetFunctionTypeLength(funcGt);
    if (len != 3) { // 3: 3 params
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef a0 = acc_.GetValueIn(gate, 0);
    GateRef a1 = acc_.GetValueIn(gate, 1);
    GateRef a2 = acc_.GetValueIn(gate, 2);
    if (IsLoadVtable(func)) {
        builder_.JSCallThisTargetTypeCheck(funcType, func);
        std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj, a0, a1, a2 };
        GateRef result = builder_.TypedAotCall(gate, args);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    } else {
        int methodIndex = tsManager_->GetMethodIndex(funcGt);
        if (methodIndex == -1) {
            acc_.DeleteStateSplitAndFrameState(gate);
            return;
        }
        builder_.JSCallTargetTypeCheck(funcType, func, builder_.IntPtr(methodIndex));
        std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj, a0, a1, a2 };
        GateRef result = builder_.TypedAotCall(gate, args);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    }
}

void TSTypeLowering::LowerTypedCallrange(GateRef gate)
{
    std::vector<GateRef> vec;
    size_t numArgs = acc_.GetNumValueIn(gate);
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLRANGE_IMM8_IMM8_V8));
    const size_t callTargetIndex = 1; // acc
    size_t argc = numArgs - callTargetIndex;
    GateRef func = acc_.GetValueIn(gate, argc);
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GlobalTSTypeRef funcGt = funcType.GetGTRef();
    uint32_t len = tsManager_->GetFunctionTypeLength(funcGt);
    if (len != static_cast<uint32_t>(argc)) {
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    vec.emplace_back(glue_);
    vec.emplace_back(actualArgc);
    vec.emplace_back(func);
    vec.emplace_back(newTarget);
    vec.emplace_back(thisObj);
    for (size_t i = 0; i < argc; i++) {
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    if (IsLoadVtable(func)) {
        builder_.JSCallThisTargetTypeCheck(funcType, func);
        GateRef result = builder_.TypedAotCall(gate, vec);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    } else {
        int methodIndex = tsManager_->GetMethodIndex(funcGt);
        if (methodIndex == -1) {
            acc_.DeleteStateSplitAndFrameState(gate);
            return;
        }
        builder_.JSCallTargetTypeCheck(funcType, func, builder_.IntPtr(methodIndex));
        GateRef result = builder_.TypedAotCall(gate, vec);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    }
}

bool TSTypeLowering::IsLoadVtable(GateRef func)
{
    auto op = acc_.GetOpCode(func);
    if (op != OpCode::LOAD_PROPERTY || !acc_.IsVtable(func)) {
        return false;
    }
    return true;
}

bool TSTypeLowering::CanOptimizeAsFastCall(GateRef func, uint32_t len)
{
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        return false;
    }
    GlobalTSTypeRef funcGt = funcType.GetGTRef();
    uint32_t length = tsManager_->GetFunctionTypeLength(funcGt);
    if (len != length) {
        return false;
    }
    auto op = acc_.GetOpCode(func);
    if (op != OpCode::LOAD_PROPERTY || !acc_.IsVtable(func)) {
        return false;
    }
    return true;
}

void TSTypeLowering::LowerTypedCallthis0(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef func = acc_.GetValueIn(gate, 1);
    if (!CanOptimizeAsFastCall(func, 0)) {
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GateType funcType = acc_.GetGateType(func);
    builder_.JSCallThisTargetTypeCheck(funcType, func);
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLTHIS0_IMM8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj };

    GateRef result = builder_.TypedAotCall(gate, args);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TSTypeLowering::LowerTypedCallthis1(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);

    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef a0 = acc_.GetValueIn(gate, 1); // 1:parameter index
    GateType a0Type = acc_.GetGateType(a0);
    GateRef func = acc_.GetValueIn(gate, 2); // 2:function
    BuiltinsStubCSigns::ID id = GetBuiltinId(func);
    if (id != BuiltinsStubCSigns::ID::NONE && a0Type.IsNumberType()) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, func, a0, id);
    } else {
        if (!CanOptimizeAsFastCall(func, 1)) {
            acc_.DeleteStateSplitAndFrameState(gate);
            return;
        }
        GateType funcType = acc_.GetGateType(func);
        builder_.JSCallThisTargetTypeCheck(funcType, func);
        GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
            EcmaOpcode::CALLTHIS1_IMM8_V8_V8));
        GateRef newTarget = builder_.Undefined();
        std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj, a0 };

        GateRef result = builder_.TypedAotCall(gate, args);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    }
}

void TSTypeLowering::LowerTypedCallthis2(GateRef gate)
{
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    GateRef func = acc_.GetValueIn(gate, 3);  // 3: func
    if (!CanOptimizeAsFastCall(func, 2)) { // 2: 2 params
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GateType funcType = acc_.GetGateType(func);
    builder_.JSCallThisTargetTypeCheck(funcType, func);
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef a0Value = acc_.GetValueIn(gate, 1);
    GateRef a1Value = acc_.GetValueIn(gate, 2);
    std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj, a0Value, a1Value };

    GateRef result = builder_.TypedAotCall(gate, args);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TSTypeLowering::LowerTypedCallthis3(GateRef gate)
{
    // 5: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 5);
    GateRef func = acc_.GetValueIn(gate, 4); // 4: func
    if (!CanOptimizeAsFastCall(func, 3)) { // 3: 3 params
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GateType funcType = acc_.GetGateType(func);
    builder_.JSCallThisTargetTypeCheck(funcType, func);
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef a0Value = acc_.GetValueIn(gate, 1);
    GateRef a1Value = acc_.GetValueIn(gate, 2);
    GateRef a2Value = acc_.GetValueIn(gate, 3);
    std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj, a0Value, a1Value, a2Value };

    GateRef result = builder_.TypedAotCall(gate, args);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TSTypeLowering::LowerTypedCallthisrange(GateRef gate)
{
    std::vector<GateRef> vec;
    // this
    size_t fixedInputsNum = 1;
    ASSERT(acc_.GetNumValueIn(gate) - fixedInputsNum >= 0);
    size_t numIns = acc_.GetNumValueIn(gate);
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8));
    const size_t callTargetIndex = 1;  // 1: acc
    GateRef func = acc_.GetValueIn(gate, numIns - callTargetIndex); // acc
    if (!CanOptimizeAsFastCall(func, numIns - 2)) { // 2 :func and thisobj
        acc_.DeleteStateSplitAndFrameState(gate);
        return;
    }
    GateType funcType = acc_.GetGateType(func);
    builder_.JSCallThisTargetTypeCheck(funcType, func);
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef newTarget = builder_.Undefined();
    vec.emplace_back(glue_);
    vec.emplace_back(actualArgc);
    vec.emplace_back(func);
    vec.emplace_back(newTarget);
    vec.emplace_back(thisObj);
    // add common args
    for (size_t i = fixedInputsNum; i < numIns - callTargetIndex; i++) {
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef result = builder_.TypedAotCall(gate, vec);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
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
                                                 { constOpcode, mode }, gate);
        acc_.SetDep(current, profiling);
        builder_.SetDepend(acc_.GetDep(gate));  // set gate depend: profiling or STATE_SPLIT
    }
}
}  // namespace panda::ecmascript
