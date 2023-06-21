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

#include "ecmascript/compiler/ts_hcr_lowering.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/builtins_lowering.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"
#include "ecmascript/ts_types/ts_type.h"

namespace panda::ecmascript::kungfu {
bool TSHCRLowering::RunTSHCRLowering()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            Lower(gate);
            allJSBcCount_++;
        }
    }

    bool success = true;
    double typeHitRate = 0.0;
    auto allTypedOpCount = allJSBcCount_ - allNonTypedOpCount_;
    if (allTypedOpCount != 0) {
        typeHitRate = static_cast<double>(hitTypedOpCount_) / static_cast<double>(allTypedOpCount);
        auto typeThreshold = tsManager_->GetTypeThreshold();
        if (typeHitRate <= typeThreshold) {
            success = false;
        }
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After TSHCRlowering "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << " =========================== End typeHitRate: "
                           << std::to_string(typeHitRate)
                           << " ===========================" << "\033[0m";
    }

    return success;
}

bool TSHCRLowering::IsTrustedType(GateRef gate) const
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

void TSHCRLowering::Lower(GateRef gate)
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
            LowerTypedStObjByValue(gate);
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
        case EcmaOpcode::CREATEEMPTYARRAY_IMM8:
        case EcmaOpcode::CREATEEMPTYARRAY_IMM16:
            LowerTypedCreateEmptyArray(gate);
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
            allNonTypedOpCount_++;
            break;
    }
}

template<TypedBinOp Op>
void TSHCRLowering::LowerTypedBinOp(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (HasNumberType(gate, left, right)) {
        SpeculateNumbers<Op>(gate);
    }
}

void TSHCRLowering::LowerTypedMod(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (HasNumberType(gate, left, right)) {
        SpeculateNumbers<TypedBinOp::TYPED_MOD>(gate);
    }
}

void TSHCRLowering::LowerTypedDiv(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (HasNumberType(gate, left, right)) {
        SpeculateNumbers<TypedBinOp::TYPED_DIV>(gate);
    }
}

void TSHCRLowering::LowerTypedStrictEq(GateRef gate)
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
    }
}

void TSHCRLowering::LowerTypedShl(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (HasNumberType(gate, left, right)) {
        SpeculateNumbers<TypedBinOp::TYPED_SHL>(gate);
    }
}

void TSHCRLowering::LowerTypedShr(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (HasNumberType(gate, left, right)) {
        SpeculateNumbers<TypedBinOp::TYPED_SHR>(gate);
    }
}

void TSHCRLowering::LowerTypedAshr(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (HasNumberType(gate, left, right)) {
        SpeculateNumbers<TypedBinOp::TYPED_ASHR>(gate);
    }
}

void TSHCRLowering::LowerTypedAnd(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (HasNumberType(gate, left, right)) {
        SpeculateNumbers<TypedBinOp::TYPED_AND>(gate);
    }
}

void TSHCRLowering::LowerTypedOr(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (HasNumberType(gate, left, right)) {
        SpeculateNumbers<TypedBinOp::TYPED_OR>(gate);
    }
}

void TSHCRLowering::LowerTypedXor(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (HasNumberType(gate, left, right)) {
        SpeculateNumbers<TypedBinOp::TYPED_XOR>(gate);
    }
}

void TSHCRLowering::LowerTypedInc(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsNumberType()) {
        SpeculateNumber<TypedUnOp::TYPED_INC>(gate);
    }
}

void TSHCRLowering::LowerTypedDec(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsNumberType()) {
        SpeculateNumber<TypedUnOp::TYPED_DEC>(gate);
    }
}

bool TSHCRLowering::HasNumberType(GateRef gate, GateRef left, GateRef right) const
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
void TSHCRLowering::SpeculateNumbers(GateRef gate)
{
    AddProfiling(gate);
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    GateType gateType = acc_.GetGateType(gate);
    PGOSampleType sampleType = acc_.TryGetPGOType(gate);

    GateRef result = builder_.TypedBinaryOp<Op>(left, right, leftType, rightType, gateType, sampleType);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

template<TypedUnOp Op>
void TSHCRLowering::SpeculateNumber(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    GateType gateType = acc_.GetGateType(gate);

    GateRef result = builder_.TypedUnaryOp<Op>(value, valueType, gateType);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSHCRLowering::LowerTypeToNumeric(GateRef gate)
{
    GateRef src = acc_.GetValueIn(gate, 0);
    GateType srcType = acc_.GetGateType(src);
    if (srcType.IsNumberType()) {
        AddProfiling(gate);
        LowerPrimitiveTypeToNumber(gate);
    }
}

void TSHCRLowering::LowerPrimitiveTypeToNumber(GateRef gate)
{
    GateRef src = acc_.GetValueIn(gate, 0);
    GateType srcType = acc_.GetGateType(src);

    GateRef result = builder_.PrimitiveToNumber(src, VariableType(MachineType::I64, srcType));

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSHCRLowering::LowerConditionJump(GateRef gate, bool flag)
{
    GateRef condition = acc_.GetValueIn(gate, 0);
    GateType conditionType = acc_.GetGateType(condition);
    if (conditionType.IsBooleanType() && IsTrustedType(condition)) {
        AddProfiling(gate);
        SpeculateConditionJump(gate, flag);
    }
}

void TSHCRLowering::SpeculateConditionJump(GateRef gate, bool flag)
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

void TSHCRLowering::LowerTypedNeg(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsNumberType()) {
        SpeculateNumber<TypedUnOp::TYPED_NEG>(gate);
    }
}

void TSHCRLowering::LowerTypedNot(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsNumberType()) {
        SpeculateNumber<TypedUnOp::TYPED_NOT>(gate);
    }
}

void TSHCRLowering::LowerTypedLdArrayLength(GateRef gate)
{
    AddProfiling(gate);
    GateRef array = acc_.GetValueIn(gate, 2);
    builder_.StableArrayCheck(array);

    GateRef result = builder_.LoadArrayLength(array);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSHCRLowering::DeleteConstDataIfNoUser(GateRef gate)
{
    auto uses = acc_.Uses(gate);
    if (uses.begin() == uses.end()) {
        builder_.ClearConstantCache(gate);
        acc_.DeleteGate(gate);
    }
}

void TSHCRLowering::LowerTypedLdObjByName(GateRef gate)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto constData = acc_.GetValueIn(gate, 1); // 1: valueIn 1
    uint16_t propIndex = acc_.GetConstantValue(constData);
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    auto prop = tsManager_->GetStringFromConstantPool(propIndex);

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
    if (tsManager_->IsClassInstanceTypeKind(receiverType)) {
        int hclassIndex = tsManager_->GetHClassIndexByInstanceGateType(receiverType);
        if (hclassIndex == -1) { // slowpath
            return;
        }
        JSHClass *hclass = JSHClass::Cast(tsManager_->GetHClassFromCache(hclassIndex).GetTaggedObject());
        if (!hclass->HasTSSubtyping()) {  // slowpath
            return;
        }

        PropertyLookupResult plr = JSHClass::LookupProperty(thread, hclass, prop);
        if (!plr.IsFound()) {  // slowpath
            return;
        }

        AddProfiling(gate);

        GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
        builder_.ObjectTypeCheck(receiverType, receiver, hclassIndexGate);

        GateRef pfrGate = builder_.Int32(plr.GetData());
        GateRef result = Circuit::NullGate();
        if (LIKELY(!plr.IsAccessor())) {
            result = builder_.LoadProperty(receiver, pfrGate, plr.IsVtable());
            if (UNLIKELY(IsVerifyVTbale())) {
                AddVTableLoadVerifer(gate, result);
            }
        } else {
            result = builder_.CallGetter(gate, receiver, pfrGate);
        }

        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
        DeleteConstDataIfNoUser(constData);
        return;
    }

    int hclassIndex = tsManager_->GetConstructorHClassIndexByClassGateType(receiverType);
    if (hclassIndex == -1) { // slowpath
        return;
    }
    JSHClass *hclass = JSHClass::Cast(tsManager_->GetHClassFromCache(hclassIndex).GetTaggedObject());

    PropertyLookupResult plr = JSHClass::LookupProperty(thread, hclass, prop);
    if (!plr.IsFound() || !plr.IsLocal() || plr.IsAccessor()) {  // slowpath
        return;
    }
    AddProfiling(gate);
    GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
    builder_.ObjectTypeCheck(receiverType, receiver, hclassIndexGate);

    GateRef pfrGate = builder_.Int32(plr.GetData());
    GateRef result = builder_.LoadProperty(receiver, pfrGate, false);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    DeleteConstDataIfNoUser(constData);
}

void TSHCRLowering::LowerTypedStObjByName(GateRef gate, bool isThis)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto constData = acc_.GetValueIn(gate, 1); // 1: valueIn 1
    uint16_t propIndex = acc_.GetConstantValue(constData);
    auto thread = tsManager_->GetEcmaVM()->GetJSThread();
    auto prop = tsManager_->GetStringFromConstantPool(propIndex);

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
    if (tsManager_->IsClassInstanceTypeKind(receiverType)) {
        int hclassIndex = tsManager_->GetHClassIndexByInstanceGateType(receiverType);
        if (hclassIndex == -1) { // slowpath
            return;
        }
        JSHClass *hclass = JSHClass::Cast(tsManager_->GetHClassFromCache(hclassIndex).GetTaggedObject());
        if (!hclass->HasTSSubtyping()) {  // slowpath
            return;
        }

        PropertyLookupResult plr = JSHClass::LookupProperty(thread, hclass, prop);
        if (!plr.IsFound() || plr.IsFunction()) {  // slowpath
            return;
        }

        AddProfiling(gate);

        GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
        builder_.ObjectTypeCheck(receiverType, receiver, hclassIndexGate);

        GateRef pfrGate = builder_.Int32(plr.GetData());
        if (LIKELY(plr.IsLocal())) {
            GateRef store = builder_.StoreProperty(receiver, pfrGate, value);
            if (UNLIKELY(IsVerifyVTbale())) {
                AddVTableStoreVerifer(gate, store, isThis);
            }
        } else {
            builder_.CallSetter(gate, receiver, pfrGate, value);
        }

        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
        DeleteConstDataIfNoUser(constData);
        return;
    }
    int hclassIndex = tsManager_->GetConstructorHClassIndexByClassGateType(receiverType);
    if (hclassIndex == -1) { // slowpath
        return;
    }
    JSHClass *hclass = JSHClass::Cast(tsManager_->GetHClassFromCache(hclassIndex).GetTaggedObject());

    PropertyLookupResult plr = JSHClass::LookupProperty(thread, hclass, prop);
    if (!plr.IsFound() || !plr.IsLocal() || plr.IsAccessor()) {  // slowpath
        return;
    }
    AddProfiling(gate);
    GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
    builder_.ObjectTypeCheck(receiverType, receiver, hclassIndexGate);

    GateRef pfrGate = builder_.Int32(plr.GetData());
    builder_.StoreProperty(receiver, pfrGate, value);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
    DeleteConstDataIfNoUser(constData);
}

void TSHCRLowering::LowerTypedLdObjByIndex(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if (!tsManager_->IsFloat32ArrayType(receiverType)) { // slowpath
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

    GateRef result = Circuit::NullGate();
    if (tsManager_->IsFloat32ArrayType(receiverType)) {
        result = builder_.LoadElement<TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT>(receiver, index);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSHCRLowering::LowerTypedStObjByIndex(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 2);
    GateType receiverType = acc_.GetGateType(receiver);
    GateType valueType = acc_.GetGateType(value);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if ((!tsManager_->IsFloat32ArrayType(receiverType)) || (!valueType.IsNumberType())) { // slowpath
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

    if (tsManager_->IsFloat32ArrayType(receiverType)) {
        builder_.StoreElement<TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT>(receiver, index, value);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
}

void TSHCRLowering::LowerTypedLdObjByValue(GateRef gate, bool isThis)
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
    if (!tsManager_->IsArrayTypeKind(receiverType) || !propKeyType.IsNumberType()) {  // slowpath
        return;
    }

    AddProfiling(gate);

    builder_.StableArrayCheck(receiver);
    GateRef length = builder_.LoadArrayLength(receiver);
    propKey = builder_.IndexCheck(receiverType, length, propKey);
    GateRef result = builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_ELEMENT>(receiver, propKey);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSHCRLowering::LowerTypedStObjByValue(GateRef gate)
{
    ASSERT(acc_.GetNumValueIn(gate) == 4);        // 4: num of value ins
    GateRef receiver = acc_.GetValueIn(gate, 1);  // 1: receiver
    GateRef propKey = acc_.GetValueIn(gate, 2);   // 2: key
    GateRef value = acc_.GetValueIn(gate, 3);     // 3: value
    GateType receiverType = acc_.GetGateType(receiver);
    GateType propKeyType = acc_.GetGateType(propKey);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if (!tsManager_->IsArrayTypeKind(receiverType) || !propKeyType.IsNumberType()) {  // slowpath
        return;
    }

    AddProfiling(gate);
    builder_.StableArrayCheck(receiver);
    GateRef length = builder_.LoadArrayLength(receiver);
    builder_.IndexCheck(receiverType, length, propKey);
    builder_.StoreElement<TypedStoreOp::ARRAY_STORE_ELEMENT>(receiver, propKey, value);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
}

void TSHCRLowering::LowerTypedIsTrueOrFalse(GateRef gate, bool flag)
{
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    auto value = acc_.GetValueIn(gate, 0);
    auto valueType = acc_.GetGateType(value);
    if ((!valueType.IsNumberType()) && (!valueType.IsBooleanType())) {
        return;
    }

    AddProfiling(gate);
    GateRef result;
    if (!flag) {
        result = builder_.TypedUnaryOp<TypedUnOp::TYPED_ISFALSE>(value, valueType, GateType::TaggedValue());
    } else {
        result = builder_.TypedUnaryOp<TypedUnOp::TYPED_ISTRUE>(value, valueType, GateType::TaggedValue());
    }

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSHCRLowering::LowerTypedNewObjRange(GateRef gate)
{
    GateRef ctor = acc_.GetValueIn(gate, 0);
    GateType ctorType = acc_.GetGateType(ctor);
    if (!tsManager_->IsClassTypeKind(ctorType)) {
        return;
    }

    AddProfiling(gate);

    int hclassIndex = tsManager_->GetHClassIndexByClassGateType(ctorType);
    GateRef stateSplit = acc_.GetDep(gate);

    GateRef frameState = acc_.FindNearestFrameState(stateSplit);
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

void TSHCRLowering::LowerTypedSuperCall(GateRef gate)
{
    GateRef ctor = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::FUNC);
    GateType ctorType = acc_.GetGateType(ctor);  // ldfunction in derived constructor get function type
    if (!tsManager_->IsClassTypeKind(ctorType) && !tsManager_->IsFunctionTypeKind(ctorType)) {
        return;
    }

    AddProfiling(gate);

    // stateSplit maybe not a STATE_SPLIT
    GateRef stateSplit = acc_.GetDep(gate);

    GateRef frameState = acc_.FindNearestFrameState(stateSplit);
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

void TSHCRLowering::SpeculateCallBuiltin(GateRef gate, GateRef func, GateRef a0, BuiltinsStubCSigns::ID id)
{
    builder_.CallTargetCheck(func, builder_.IntPtr(static_cast<int64_t>(id)), a0);
    GateRef result = builder_.TypedCallBuiltin(gate, a0, id);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSHCRLowering::SpeculateCallThis3Builtin(GateRef gate, BuiltinsStubCSigns::ID id)
{
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef a0 = acc_.GetValueIn(gate, 1);  // 1: the first-para
    GateRef a1 = acc_.GetValueIn(gate, 2);  // 2: the third-para
    GateRef a2 = acc_.GetValueIn(gate, 3);  // 3: the fourth-para
    GateRef result = builder_.TypedCallThis3Builtin(gate, thisObj, a0, a1, a2, id);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

BuiltinsStubCSigns::ID TSHCRLowering::GetBuiltinId(BuiltinTypeId id, GateRef func)
{
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsBuiltinObject(id, funcType)) {
        return BuiltinsStubCSigns::ID::NONE;
    }
    std::string name = tsManager_->GetFuncName(funcType);
    BuiltinsStubCSigns::ID stubId = BuiltinsStubCSigns::GetBuiltinId(name);
    return stubId;
}

void TSHCRLowering::CheckCallTargetAndLowerCall(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
    GateType funcType, const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall)
{
    if (IsLoadVtable(func)) {
        if (tsManager_->CanFastCall(funcGt)) {
            builder_.JSFastCallThisTargetTypeCheck(funcType, func);
            builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
            GateRef result = builder_.TypedFastCall(gate, argsFastCall);
            builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
            acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
        } else {
            builder_.JSCallThisTargetTypeCheck(funcType, func);
            builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
            GateRef result = builder_.TypedCall(gate, args);
            builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
            acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
        }
    } else {
        auto op = acc_.GetOpCode(func);
        if (!tsManager_->FastCallFlagIsVaild(funcGt)) {
            return;
        }
        if (op == OpCode::JS_BYTECODE && (acc_.GetByteCodeOpcode(func) == EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8 ||
                                          acc_.GetByteCodeOpcode(func) == EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8)) {
            if (tsManager_->CanFastCall(funcGt)) {
                builder_.JSCallTargetFromDefineFuncCheck(funcType, func);
                builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
                GateRef result = builder_.TypedFastCall(gate, argsFastCall);
                builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
                acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
            } else {
                builder_.JSCallTargetFromDefineFuncCheck(funcType, func);
                builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
                GateRef result = builder_.TypedCall(gate, args);
                builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
                acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
            }
            return;
        }
        int methodIndex = tsManager_->GetMethodIndex(funcGt);
        if (!tsManager_->MethodOffsetIsVaild(funcGt) || methodIndex == -1) {
            return;
        }
        if (tsManager_->CanFastCall(funcGt)) {
            builder_.JSFastCallTargetTypeCheck(funcType, func, builder_.IntPtr(methodIndex));
            builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
            GateRef result = builder_.TypedFastCall(gate, argsFastCall);
            builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
            acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
        } else {
            builder_.JSCallTargetTypeCheck(funcType, func, builder_.IntPtr(methodIndex));
            builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
            GateRef result = builder_.TypedCall(gate, args);
            builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
            acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
        }
    }
}

void TSHCRLowering::LowerTypedCallArg0(GateRef gate)
{
    GateRef func = acc_.GetValueIn(gate, 0);
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLARG0_IMM8));
    LowerTypedCall(gate, func, actualArgc, funcType, 0);
}

void TSHCRLowering::LowerTypedCallArg1(GateRef gate)
{
    GateRef func = acc_.GetValueIn(gate, 1);
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        return;
    }
    GateRef a0Value = acc_.GetValueIn(gate, 0);
    GateType a0Type = acc_.GetGateType(a0Value);
    BuiltinsStubCSigns::ID id = GetBuiltinId(BuiltinTypeId::MATH, func);
    if (id != BuiltinsStubCSigns::ID::NONE && a0Type.IsNumberType()) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, func, a0Value, id);
    } else {
        GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
            EcmaOpcode::CALLARG1_IMM8_V8));
        LowerTypedCall(gate, func, actualArgc, funcType, 1);
    }
}

void TSHCRLowering::LowerTypedCallArg2(GateRef gate)
{
    GateRef func = acc_.GetValueIn(gate, 2); // 2:function
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLARGS2_IMM8_V8_V8));
    LowerTypedCall(gate, func, actualArgc, funcType, 2); // 2: 2 params
}

void TSHCRLowering::LowerTypedCallArg3(GateRef gate)
{
    GateRef func = acc_.GetValueIn(gate, 3); // 3:function
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8));
    LowerTypedCall(gate, func, actualArgc, funcType, 3); // 3: 3 params
}

void TSHCRLowering::LowerTypedCallrange(GateRef gate)
{
    std::vector<GateRef> vec;
    std::vector<GateRef> vec1;
    size_t numArgs = acc_.GetNumValueIn(gate);
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLRANGE_IMM8_IMM8_V8));
    const size_t callTargetIndex = 1; // acc
    size_t argc = numArgs - callTargetIndex;
    GateRef func = acc_.GetValueIn(gate, argc);
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        return;
    }
    LowerTypedCall(gate, func, actualArgc, funcType, argc);
}

void TSHCRLowering::LowerTypedCall(GateRef gate, GateRef func, GateRef actualArgc, GateType funcType, uint32_t argc)
{
    GlobalTSTypeRef funcGt = funcType.GetGTRef();
    uint32_t len = tsManager_->GetFunctionTypeLength(funcGt);
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    std::vector<GateRef> argsFastCall { glue_, func, thisObj};
    std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj };
    for (uint32_t i = 0; i < argc; i++) {
        GateRef value = acc_.GetValueIn(gate, i);
        argsFastCall.emplace_back(value);
        args.emplace_back(value);
    }
    for (uint32_t i = argc; i < len; i++) {
        argsFastCall.emplace_back(builder_.Undefined());
        args.emplace_back(builder_.Undefined());
    }
    CheckCallTargetAndLowerCall(gate, func, funcGt, funcType, args, argsFastCall);
}

bool TSHCRLowering::IsLoadVtable(GateRef func)
{
    auto op = acc_.GetOpCode(func);
    if (op != OpCode::LOAD_PROPERTY || !acc_.IsVtable(func)) {
        return false;
    }
    return true;
}

bool TSHCRLowering::CanOptimizeAsFastCall(GateRef func)
{
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        return false;
    }
    auto op = acc_.GetOpCode(func);
    if (op != OpCode::LOAD_PROPERTY || !acc_.IsVtable(func)) {
        return false;
    }
    return true;
}

void TSHCRLowering::CheckThisCallTargetAndLowerCall(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
    GateType funcType, const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall)
{
    if (!tsManager_->FastCallFlagIsVaild(funcGt)) {
        return;
    }
    if (tsManager_->CanFastCall(funcGt)) {
        builder_.JSFastCallThisTargetTypeCheck(funcType, func);
        builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
        GateRef result = builder_.TypedFastCall(gate, argsFastCall);
        builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    } else {
        builder_.JSCallThisTargetTypeCheck(funcType, func);
        builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
        GateRef result = builder_.TypedCall(gate, args);
        builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    }
}

void TSHCRLowering::LowerTypedCallthis0(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef func = acc_.GetValueIn(gate, 1);
    if (!CanOptimizeAsFastCall(func)) {
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLTHIS0_IMM8_V8));
    LowerTypedThisCall(gate, func, actualArgc, 1);
}

void TSHCRLowering::LowerTypedCallthis1(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef a0 = acc_.GetValueIn(gate, 1); // 1:parameter index
    GateType a0Type = acc_.GetGateType(a0);
    GateRef func = acc_.GetValueIn(gate, 2); // 2:function
    BuiltinsStubCSigns::ID id = GetBuiltinId(BuiltinTypeId::MATH, func);
    if (id != BuiltinsStubCSigns::ID::NONE && a0Type.IsNumberType()) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, func, a0, id);
    } else {
        if (!CanOptimizeAsFastCall(func)) {
            return;
        }
        GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
            EcmaOpcode::CALLTHIS1_IMM8_V8_V8));
        LowerTypedThisCall(gate, func, actualArgc, 1);
    }
}

void TSHCRLowering::LowerTypedCallthis2(GateRef gate)
{
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    GateRef func = acc_.GetValueIn(gate, 3);  // 3: func
    if (!CanOptimizeAsFastCall(func)) {
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8));
    LowerTypedThisCall(gate, func, actualArgc, 2); // 2: 2 params
}

void TSHCRLowering::LowerTypedCallthis3(GateRef gate)
{
    // 5: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 5);
    GateRef func = acc_.GetValueIn(gate, 4); // 4: func
    BuiltinsStubCSigns::ID id = GetBuiltinId(BuiltinTypeId::STRING, func);
    if (id == BuiltinsStubCSigns::ID::LocaleCompare) {
        AddProfiling(gate);
        SpeculateCallThis3Builtin(gate, id);
        return;
    }

    if (!CanOptimizeAsFastCall(func)) {
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8));
    LowerTypedThisCall(gate, func, actualArgc, 3); // 3: 3 params
}

void TSHCRLowering::LowerTypedThisCall(GateRef gate, GateRef func, GateRef actualArgc, uint32_t argc)
{
    GateType funcType = acc_.GetGateType(func);
    GlobalTSTypeRef funcGt = funcType.GetGTRef();
    uint32_t len = tsManager_->GetFunctionTypeLength(funcGt);
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    std::vector<GateRef> argsFastCall { glue_, func, thisObj};
    std::vector<GateRef> args { glue_, actualArgc, func, newTarget, thisObj };
    for (uint32_t i = 0; i < argc; i++) {
        GateRef value = acc_.GetValueIn(gate, i + 1);
        argsFastCall.emplace_back(value);
        args.emplace_back(value);
    }
    for (uint32_t i = argc; i < len; i++) {
        argsFastCall.emplace_back(builder_.Undefined());
        args.emplace_back(builder_.Undefined());
    }
    CheckThisCallTargetAndLowerCall(gate, func, funcGt, funcType, args, argsFastCall);
}


void TSHCRLowering::LowerTypedCallthisrange(GateRef gate)
{
    // this
    size_t fixedInputsNum = 1;
    ASSERT(acc_.GetNumValueIn(gate) - fixedInputsNum >= 0);
    size_t numIns = acc_.GetNumValueIn(gate);
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8));
    const size_t callTargetIndex = 1;  // 1: acc
    GateRef func = acc_.GetValueIn(gate, numIns - callTargetIndex); // acc
    if (!CanOptimizeAsFastCall(func)) {
        return;
    }
    LowerTypedThisCall(gate, func, actualArgc, numIns - callTargetIndex - fixedInputsNum);
}

void TSHCRLowering::LowerTypedCreateEmptyArray(GateRef gate)
{
    // in the future, the type of the elements in the array will be obtained through pgo,
    // and the type will be used to determine whether to create a typed-array.
    GateRef emptyArray = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    GateRef array = builder_.CreateArray(emptyArray, true);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), array);
}

void TSHCRLowering::AddProfiling(GateRef gate)
{
    hitTypedOpCount_++;
    if (IsTraceBC()) {
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
        GateRef typedPath = builder_.Int32ToTaggedInt(builder_.Int32(1));
        GateRef traceGate = builder_.CallRuntime(glue_, RTSTUB_ID(DebugAOTPrint), acc_.GetDep(current),
                                                 { constOpcode, typedPath }, gate);
        acc_.SetDep(current, traceGate);
        builder_.SetDepend(acc_.GetDep(gate));  // set gate depend: trace or STATE_SPLIT
    }

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

void TSHCRLowering::AddVTableLoadVerifer(GateRef gate, GateRef value)
{
    GateRef receiver = acc_.GetValueIn(gate, 2);  // 2: receiver
    GateRef key = acc_.GetValueIn(gate, 1);       // 1: key

    GateRef verifier = builder_.CallRuntime(glue_, RTSTUB_ID(VerifyVTableLoading), acc_.GetDep(gate),
                                            { receiver, key, value }, gate);
    acc_.SetDep(gate, verifier);
}

void TSHCRLowering::AddVTableStoreVerifer(GateRef gate, GateRef store, bool isThis)
{
    GateRef key = acc_.GetValueIn(gate, 1);
    GateRef receiver = Circuit::NullGate();
    GateRef value = Circuit::NullGate();
    if (isThis) {
        receiver = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
        value = acc_.GetValueIn(gate, 2);  // 2: acc
    } else {
        receiver = acc_.GetValueIn(gate, 2);  // 2: receiver
        value = acc_.GetValueIn(gate, 3);     // 3: acc
    }

    GateRef verifier = builder_.CallRuntime(glue_, RTSTUB_ID(VerifyVTableStoring), store,
                                            { receiver, key, value }, gate);
    acc_.SetDep(gate, verifier);
}
}  // namespace panda::ecmascript
