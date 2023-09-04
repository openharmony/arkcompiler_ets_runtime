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
#include "ecmascript/compiler/circuit.h"
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
    acc_.EliminateRedundantPhi();

    if (IsTypeLogEnabled()) {
        pgoTypeLog_.PrintPGOTypeLog();
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
        for (auto a : bytecodeMap_) {
            if (bytecodeHitTimeMap_.find(a.first) != bytecodeHitTimeMap_.end()) {
                double rate = static_cast<double>(bytecodeHitTimeMap_[a.first]) / static_cast<double>(a.second);
                LOG_COMPILER(INFO) << "\033[34m" << " =========================== End opHitRate: "
                                   << GetEcmaOpcodeStr(a.first) << " rate: " << std::to_string(rate)
                                   << "(" << std::to_string(bytecodeHitTimeMap_[a.first])
                                   << " / " << std::to_string(a.second) << ")"
                                   << " ===========================" << "\033[0m";
            } else {
                LOG_COMPILER(INFO) << "\033[34m" << " =========================== End opHitRate: "
                                   << GetEcmaOpcodeStr(a.first) << " rate: " << std::to_string(0)
                                   << "(" << std::to_string(0)
                                   << " / " << std::to_string(a.second) << ")"
                                   << " ===========================" << "\033[0m";
            }
        }
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
    AddBytecodeCount(ecmaOpcode);
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
            DeleteBytecodeCount(ecmaOpcode);
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
    if (HasNumberType(gate, value)) {
        SpeculateNumber<TypedUnOp::TYPED_INC>(gate);
    }
}

void TSHCRLowering::LowerTypedDec(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    if (HasNumberType(gate, value)) {
        SpeculateNumber<TypedUnOp::TYPED_DEC>(gate);
    }
}

bool TSHCRLowering::HasNumberType(GateRef gate, GateRef value) const
{
    GateType valueType = acc_.GetGateType(value);
    PGOSampleType sampleType = acc_.TryGetPGOType(gate);
    if (sampleType.IsNumber() ||
        (sampleType.IsNone() && valueType.IsNumberType())) {
        return true;
    }
    return false;
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
    pgoTypeLog_.CollectGateTypeLogInfo(gate, true);

    GateRef result = builder_.TypedBinaryOp<Op>(left, right, leftType, rightType, gateType, sampleType);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

template<TypedUnOp Op>
void TSHCRLowering::SpeculateNumber(GateRef gate)
{
    AddProfiling(gate);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    GateType gateType = acc_.GetGateType(gate);
    pgoTypeLog_.CollectGateTypeLogInfo(gate, false);

    PGOSampleType sampleType = acc_.TryGetPGOType(gate);
    if (sampleType.IsNumber()) {
        if (sampleType.IsInt()) {
            gateType = GateType::IntType();
        } else if (sampleType.IsDouble()) {
            gateType = GateType::DoubleType();
        } else {
            gateType = GateType::NumberType();
        }
        valueType = gateType;
    }

    GateRef result = builder_.TypedUnaryOp<Op>(value, valueType, gateType);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSHCRLowering::LowerTypeToNumeric(GateRef gate)
{
    GateRef src = acc_.GetValueIn(gate, 0);
    if (HasNumberType(gate, src)) {
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
    auto branchKind = BranchKind::NORMAL_BRANCH;
    PGOSampleType sampleType = acc_.TryGetPGOType(value);
    if (sampleType.IsLikely()) {
        branchKind = BranchKind::TRUE_BRANCH;
    } else if (sampleType.IsUnLikely()) {
        branchKind = BranchKind::FALSE_BRANCH;
    } else if (sampleType.IsStrongLikely()) {
        branchKind = BranchKind::STRONG_TRUE_BRANCH;
    } else if (sampleType.IsStrongUnLikely()) {
        branchKind = BranchKind::STRONG_FALSE_BRANCH;
    }
    if (flag) {
        jump = builder_.TypedConditionJump<TypedJumpOp::TYPED_JNEZ>(value, valueType, branchKind);
    } else {
        jump = builder_.TypedConditionJump<TypedJumpOp::TYPED_JEQZ>(value, valueType, branchKind);
    }
    acc_.ReplaceGate(gate, jump, jump, Circuit::NullGate());
}

void TSHCRLowering::LowerTypedNeg(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    if (HasNumberType(gate, value)) {
        SpeculateNumber<TypedUnOp::TYPED_NEG>(gate);
    }
}

void TSHCRLowering::LowerTypedNot(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    if (HasNumberType(gate, value)) {
        SpeculateNumber<TypedUnOp::TYPED_NOT>(gate);
    }
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
    uint16_t keyIndex = acc_.GetConstantValue(constData);
    JSTaggedValue key = tsManager_->GetStringFromConstantPool(keyIndex);

    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 2); // 2: acc or this object
    LowerNamedAccess(gate, receiver, AccessMode::LOAD, key, Circuit::NullGate());
    DeleteConstDataIfNoUser(constData);
}

void TSHCRLowering::LowerTypedStObjByName(GateRef gate, bool isThis)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto constData = acc_.GetValueIn(gate, 1); // 1: valueIn 1
    uint16_t keyIndex = acc_.GetConstantValue(constData);
    JSTaggedValue key = tsManager_->GetStringFromConstantPool(keyIndex);

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
    LowerNamedAccess(gate, receiver, AccessMode::STORE, key, value);
    DeleteConstDataIfNoUser(constData);
}

void TSHCRLowering::LowerNamedAccess(GateRef gate, GateRef receiver, AccessMode accessMode, JSTaggedValue key,
                                     GateRef value)
{
    DISALLOW_GARBAGE_COLLECTION;
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if (accessMode == AccessMode::LOAD && TryLowerTypedLdObjByNameForBuiltin(gate, receiverType, key)) {
        return;
    }

    ObjectAccessHelper accessHelper(tsManager_, accessMode, receiver, receiverType, key, value);
    ChunkVector<ObjectAccessInfo> infos(circuit_->chunk());
    bool continuation = accessHelper.Compute(infos);
    if (!continuation) {
        return;  // slowpath
    }

    ASSERT(!infos.empty());
    AddProfiling(gate);

    // If all elements of the array are objects, and receiver is one of the elements,
    // no HeapObjectCheck is required.
    bool isHeapObject = acc_.IsHeapObjectFromElementsKind(receiver);

    // monomorphic
    if (infos.size() == 1) {
        int hclassIndex = infos[0].HClassIndex();
        PropertyLookupResult plr = infos[0].Plr();
        if (!Uncheck()) {
            GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
            builder_.ObjectTypeCheck(infos[0].Type(), isHeapObject, receiver, hclassIndexGate);
        }

        GateRef result = BuildNamedPropertyAccess(gate, accessHelper, plr);
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
        return;
    }

    // polymorphic
    size_t size = infos.size();
    GateRef fallthroughState = builder_.GetState();
    GateRef fallthroughDepend = builder_.GetDepend();
    std::vector<GateRef> values(size + 1, Circuit::NullGate());   // +1: state for value selector
    std::vector<GateRef> depends(size + 1, Circuit::NullGate());  // +1: state for depend selector
    std::vector<GateRef> states(size, Circuit::NullGate());
    for (size_t i = 0; i < size; ++i) {
        GateType type = infos[i].Type();
        int hclassIndex = infos[i].HClassIndex();
        PropertyLookupResult plr = infos[i].Plr();
        GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);

        builder_.SetState(fallthroughState);
        builder_.SetDepend(fallthroughDepend);
        if (i == size - 1) {
            builder_.ObjectTypeCheck(type, isHeapObject, receiver, hclassIndexGate);
            fallthroughState = Circuit::NullGate();
            fallthroughDepend = Circuit::NullGate();
        } else {
            GateRef compare = builder_.ObjectTypeCompare(type, isHeapObject, receiver, hclassIndexGate);
            GateRef branch = builder_.Branch(builder_.GetState(), compare);
            GateRef ifTrue = builder_.IfTrue(branch);
            GateRef ifFalse = builder_.IfFalse(branch);
            GateRef tDepend = builder_.DependRelay(ifTrue, builder_.GetDepend());
            fallthroughState = ifFalse;
            fallthroughDepend = builder_.DependRelay(ifFalse, builder_.GetDepend());
            builder_.SetState(ifTrue);
            builder_.SetDepend(tDepend);
        }

        values[i + 1] = BuildNamedPropertyAccess(gate, accessHelper, plr);
        depends[i + 1] = builder_.GetDepend();
        states[i] = builder_.GetState();
    }
    ASSERT(fallthroughState == Circuit::NullGate());
    ASSERT(fallthroughDepend == Circuit::NullGate());
    GateRef mergeState = circuit_->NewGate(circuit_->Merge(size), states);
    depends[0] = mergeState;
    values[0] = mergeState;
    GateRef dependSelector = circuit_->NewGate(circuit_->DependSelector(size), depends);
    GateRef result = accessHelper.IsLoading() ?
        circuit_->NewGate(circuit_->ValueSelector(size), MachineType::I64, size + 1, values.data(), GateType::AnyType())
        : Circuit::NullGate();
    acc_.ReplaceHirAndDeleteIfException(gate, StateDepend(mergeState, dependSelector), result);
}

GateRef TSHCRLowering::BuildNamedPropertyAccess(GateRef hir, ObjectAccessHelper accessHelper, PropertyLookupResult plr)
{
    GateRef receiver = accessHelper.GetReceiver();
    GateRef plrGate = builder_.Int32(plr.GetData());
    GateRef result = Circuit::NullGate();

    AccessMode mode = accessHelper.GetAccessMode();
    switch (mode) {
        case AccessMode::LOAD: {
            if (LIKELY(!plr.IsAccessor())) {
                result = builder_.LoadProperty(receiver, plrGate, plr.IsFunction());
                if (UNLIKELY(IsVerifyVTbale())) {
                    BuildNamedPropertyAccessVerifier(hir, receiver, mode, result);
                }
            } else {
                result = builder_.CallGetter(hir, receiver, plrGate);
            }
        }
            break;
        case AccessMode::STORE: {
            GateRef value = accessHelper.GetValue();
            if (LIKELY(plr.IsLocal())) {
                builder_.StoreProperty(receiver, plrGate, value);
                if (UNLIKELY(IsVerifyVTbale())) {
                    BuildNamedPropertyAccessVerifier(hir, receiver, mode, value);
                }
            } else {
                builder_.CallSetter(hir, receiver, plrGate, value);
            }
        }
            break;
        default:
            break;
    }

    return result;
}

void TSHCRLowering::BuildNamedPropertyAccessVerifier(GateRef gate, GateRef receiver, AccessMode mode, GateRef value)
{
    GateRef constData = acc_.GetValueIn(gate, 1);
    uint16_t keyIndex = acc_.GetConstantValue(constData);
    GateRef func = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::FUNC);
    GateRef constPool = builder_.GetConstPool(func);
    GateRef key =  builder_.GetValueFromTaggedArray(constPool, builder_.Int32(keyIndex));
    int stubId = mode == AccessMode::LOAD ? RTSTUB_ID(VerifyVTableLoading) : RTSTUB_ID(VerifyVTableStoring);
    builder_.CallRuntime(glue_, stubId, builder_.GetDepend(), { receiver, key, value }, gate);
}

bool TSHCRLowering::TryLowerTypedLdObjByNameForBuiltin(GateRef gate, GateType receiverType, JSTaggedValue key)
{
    EcmaString *propString = EcmaString::Cast(key.GetTaggedObject());
    EcmaString *lengthString = EcmaString::Cast(thread_->GlobalConstants()->GetLengthString().GetTaggedObject());
    if (propString == lengthString) {
        if (tsManager_->IsArrayTypeKind(receiverType)) {
            LowerTypedLdArrayLength(gate);
            return true;
        } else if (tsManager_->IsValidTypedArrayType(receiverType)) {
            LowerTypedLdTypedArrayLength(gate);
            return true;
        } else if (receiverType.IsStringType()) {
            LowerTypedLdStringLength(gate);
            return true;
        }
    }
    return TryLowerTypedLdObjByNameForBuiltinMethod(gate, receiverType, key);
}

bool TSHCRLowering::IsCreateArray(GateRef gate)
{
    if (acc_.GetOpCode(gate) != OpCode::JS_BYTECODE) {
        return false;
    }
    EcmaOpcode ecmaop = acc_.GetByteCodeOpcode(gate);
    switch (ecmaop) {
        case EcmaOpcode::CREATEEMPTYARRAY_IMM8:
        case EcmaOpcode::CREATEEMPTYARRAY_IMM16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
            return true;
        default:
            return false;
    }
    UNREACHABLE();
    return false;
}

void TSHCRLowering::LowerTypedLdArrayLength(GateRef gate)
{
    AddProfiling(gate);
    GateRef array = acc_.GetValueIn(gate, 2);
    if (!Uncheck()) {
        ElementsKind kind = acc_.TryGetElementsKind(gate);
        if (!IsCreateArray(array)) {
            builder_.StableArrayCheck(array, kind, ArrayMetaDataAccessor::Mode::LOAD_LENGTH);
        }
    }

    GateRef result = builder_.LoadArrayLength(array);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSHCRLowering::LowerTypedLdTypedArrayLength(GateRef gate)
{
    AddProfiling(gate);
    GateRef array = acc_.GetValueIn(gate, 2);
    GateType arrayType = acc_.GetGateType(array);
    arrayType = tsManager_->TryNarrowUnionType(arrayType);
    if (!Uncheck()) {
        builder_.TypedArrayCheck(arrayType, array);
    }
    GateRef result = builder_.LoadTypedArrayLength(arrayType, array);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TSHCRLowering::LowerTypedLdStringLength(GateRef gate)
{
    AddProfiling(gate);
    GateRef str = acc_.GetValueIn(gate, 2);
    if (!Uncheck()) {
        builder_.EcmaStringCheck(str);
    }
    GateRef result = builder_.LoadStringLength(str);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

bool TSHCRLowering::TryLowerTypedLdObjByNameForBuiltinMethod(GateRef gate, GateType receiverType, JSTaggedValue key)
{
    JSHandle<GlobalEnv> globalEnv = thread_->GetEcmaVM()->GetGlobalEnv();
    if (receiverType.IsStringType()) {
        JSHClass *stringPhc = globalEnv->GetStringPrototype()->GetTaggedObject()->GetClass();
        PropertyLookupResult plr = JSHClass::LookupPropertyInBuiltinPrototypeHClass(thread_, stringPhc, key);
        // Unable to handle accessor at the moment
        if (!plr.IsFound() || plr.IsAccessor()) {
            return false;
        }
        AddProfiling(gate);
        GateRef str = acc_.GetValueIn(gate, 2);
        if (!Uncheck()) {
            builder_.EcmaStringCheck(str);
        }
        GateRef plrGate = builder_.Int32(plr.GetData());
        GateRef strPrototype = builder_.GetGlobalEnvObj(builder_.GetGlobalEnv(), GlobalEnv::STRING_PROTOTYPE_INDEX);
        GateRef result = builder_.LoadProperty(strPrototype, plrGate, plr.IsFunction());
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
        return true;
    }
    return false;
}

void TSHCRLowering::LowerTypedLdObjByIndex(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    GateRef result = Circuit::NullGate();
    if (tsManager_->IsValidTypedArrayType(receiverType)) {
        AddProfiling(gate);
        GateRef index = acc_.GetValueIn(gate, 0);
        uint32_t indexValue = static_cast<uint32_t>(acc_.GetConstantValue(index));
        index = builder_.Int32(indexValue);
        result = LoadTypedArrayByIndex(receiver, index);
    } else {
        return; // slowpath
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
    if ((!tsManager_->IsBuiltinInstanceType(BuiltinTypeId::FLOAT32_ARRAY, receiverType)) ||
        (!valueType.IsNumberType())) { // slowpath
        return;
    }

    AddProfiling(gate);

    if (tsManager_->IsBuiltinInstanceType(BuiltinTypeId::FLOAT32_ARRAY, receiverType)) {
        if (!Uncheck()) {
            builder_.TypedArrayCheck(receiverType, receiver);
        }
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    GateRef index = acc_.GetValueIn(gate, 1);
    uint32_t indexValue = static_cast<uint32_t>(acc_.GetConstantValue(index));
    index = builder_.Int32(indexValue);
    auto length = builder_.LoadTypedArrayLength(receiverType, receiver);
    if (!Uncheck()) {
        builder_.IndexCheck(receiverType, length, index);
    }

    if (tsManager_->IsBuiltinInstanceType(BuiltinTypeId::FLOAT32_ARRAY, receiverType)) {
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
    if (!propKeyType.IsNumberType()) {
        return; // slowpath
    }

    GateRef result = Circuit::NullGate();
    if (receiverType.IsStringType()) {
        AddProfiling(gate);
        result = LoadStringByIndex(receiver, propKey);
    } else if (tsManager_->IsArrayTypeKind(receiverType)) {
        AddProfiling(gate);
        ElementsKind kind = acc_.TryGetArrayElementsKind(gate);
        result = LoadJSArrayByIndex(receiver, propKey, kind);
    } else if (tsManager_->IsValidTypedArrayType(receiverType)) {
        AddProfiling(gate);
        result = LoadTypedArrayByIndex(receiver, propKey);
    } else {
        return; // slowpath
    }
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

GateRef TSHCRLowering::LoadStringByIndex(GateRef receiver, GateRef propKey)
{
    if (!Uncheck()) {
        GateType receiverType = acc_.GetGateType(receiver);
        receiverType = tsManager_->TryNarrowUnionType(receiverType);
        builder_.EcmaStringCheck(receiver);
        GateRef length = builder_.LoadStringLength(receiver);
        propKey = builder_.IndexCheck(receiverType, length, propKey);
        receiver = builder_.FlattenStringCheck(receiver);
    }
    return builder_.LoadElement<TypedLoadOp::STRING_LOAD_ELEMENT>(receiver, propKey);
}

GateRef TSHCRLowering::LoadJSArrayByIndex(GateRef receiver, GateRef propKey, ElementsKind kind)
{
    if (!Uncheck()) {
        GateType receiverType = acc_.GetGateType(receiver);
        receiverType = tsManager_->TryNarrowUnionType(receiverType);
        if (!IsCreateArray(receiver)) {
            builder_.StableArrayCheck(receiver, kind, ArrayMetaDataAccessor::Mode::LOAD_ELEMENT);
        }
        GateRef length = builder_.LoadArrayLength(receiver);
        propKey = builder_.IndexCheck(receiverType, length, propKey);
    }

    GateRef result = Circuit::NullGate();
    if (Elements::IsInt(kind)) {
        result = builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_INT_ELEMENT>(receiver, propKey);
    } else if (Elements::IsDouble(kind)) {
        result = builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_DOUBLE_ELEMENT>(receiver, propKey);
    } else if (Elements::IsObject(kind)) {
        result = builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_OBJECT_ELEMENT>(receiver, propKey);
    } else if (!Elements::IsHole(kind)) {
        result = builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_TAGGED_ELEMENT>(receiver, propKey);
    } else {
        result = builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_HOLE_TAGGED_ELEMENT>(receiver, propKey);
    }
    return result;
}

GateRef TSHCRLowering::LoadTypedArrayByIndex(GateRef receiver, GateRef propKey)
{
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if (!Uncheck()) {
        builder_.TypedArrayCheck(receiverType, receiver);
        GateRef length = builder_.LoadTypedArrayLength(receiverType, receiver);
        propKey = builder_.IndexCheck(receiverType, length, propKey);
    }
    auto builtinTypeId = tsManager_->GetTypedArrayBuiltinId(receiverType);
    switch (builtinTypeId) {
        case BuiltinTypeId::INT8_ARRAY:
            return builder_.LoadElement<TypedLoadOp::INT8ARRAY_LOAD_ELEMENT>(receiver, propKey);
        case BuiltinTypeId::UINT8_ARRAY:
            return builder_.LoadElement<TypedLoadOp::UINT8ARRAY_LOAD_ELEMENT>(receiver, propKey);
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            return builder_.LoadElement<TypedLoadOp::UINT8CLAMPEDARRAY_LOAD_ELEMENT>(receiver, propKey);
        case BuiltinTypeId::INT16_ARRAY:
            return builder_.LoadElement<TypedLoadOp::INT16ARRAY_LOAD_ELEMENT>(receiver, propKey);
        case BuiltinTypeId::UINT16_ARRAY:
            return builder_.LoadElement<TypedLoadOp::UINT16ARRAY_LOAD_ELEMENT>(receiver, propKey);
        case BuiltinTypeId::INT32_ARRAY:
            return builder_.LoadElement<TypedLoadOp::INT32ARRAY_LOAD_ELEMENT>(receiver, propKey);
        case BuiltinTypeId::UINT32_ARRAY:
            return builder_.LoadElement<TypedLoadOp::UINT32ARRAY_LOAD_ELEMENT>(receiver, propKey);
        case BuiltinTypeId::FLOAT32_ARRAY:
            return builder_.LoadElement<TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT>(receiver, propKey);
        case BuiltinTypeId::FLOAT64_ARRAY:
            return builder_.LoadElement<TypedLoadOp::FLOAT64ARRAY_LOAD_ELEMENT>(receiver, propKey);
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }

    return Circuit::NullGate();
}

void TSHCRLowering::StoreJSArrayByIndex(GateRef receiver, GateRef propKey, GateRef value, ElementsKind kind)
{
    if (!Uncheck()) {
        GateType receiverType = acc_.GetGateType(receiver);
        receiverType = tsManager_->TryNarrowUnionType(receiverType);
        if (!IsCreateArray(receiver)) {
            builder_.StableArrayCheck(receiver, kind, ArrayMetaDataAccessor::Mode::STORE_ELEMENT);
        }
        GateRef length = builder_.LoadArrayLength(receiver);
        builder_.IndexCheck(receiverType, length, propKey);
        builder_.COWArrayCheck(receiver);

        if (Elements::IsObject(kind)) {
            GateRef frameState = acc_.FindNearestFrameState(builder_.GetDepend());
            builder_.HeapObjectCheck(value, frameState);
        }
    }
    builder_.StoreElement<TypedStoreOp::ARRAY_STORE_ELEMENT>(receiver, propKey, value);
}


void TSHCRLowering::StoreTypedArrayByIndex(GateRef receiver, GateRef propKey, GateRef value)
{
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if (!Uncheck()) {
        builder_.TypedArrayCheck(receiverType, receiver);
        GateRef length = builder_.LoadTypedArrayLength(receiverType, receiver);
        propKey = builder_.IndexCheck(receiverType, length, propKey);
    }

    auto builtinTypeId = tsManager_->GetTypedArrayBuiltinId(receiverType);
    switch (builtinTypeId) {
        case BuiltinTypeId::INT8_ARRAY:
            builder_.StoreElement<TypedStoreOp::INT8ARRAY_STORE_ELEMENT>(receiver, propKey, value);
            break;
        case BuiltinTypeId::UINT8_ARRAY:
            builder_.StoreElement<TypedStoreOp::UINT8ARRAY_STORE_ELEMENT>(receiver, propKey, value);
            break;
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            builder_.StoreElement<TypedStoreOp::UINT8CLAMPEDARRAY_STORE_ELEMENT>(receiver, propKey, value);
            break;
        case BuiltinTypeId::INT16_ARRAY:
            builder_.StoreElement<TypedStoreOp::INT16ARRAY_STORE_ELEMENT>(receiver, propKey, value);
            break;
        case BuiltinTypeId::UINT16_ARRAY:
            builder_.StoreElement<TypedStoreOp::UINT16ARRAY_STORE_ELEMENT>(receiver, propKey, value);
            break;
        case BuiltinTypeId::INT32_ARRAY:
            builder_.StoreElement<TypedStoreOp::INT32ARRAY_STORE_ELEMENT>(receiver, propKey, value);
            break;
        case BuiltinTypeId::UINT32_ARRAY:
            builder_.StoreElement<TypedStoreOp::UINT32ARRAY_STORE_ELEMENT>(receiver, propKey, value);
            break;
        case BuiltinTypeId::FLOAT32_ARRAY:
            builder_.StoreElement<TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT>(receiver, propKey, value);
            break;
        case BuiltinTypeId::FLOAT64_ARRAY:
            builder_.StoreElement<TypedStoreOp::FLOAT64ARRAY_STORE_ELEMENT>(receiver, propKey, value);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
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
    if (!propKeyType.IsNumberType()) {
        return; // slowpath
    }

    if (tsManager_->IsArrayTypeKind(receiverType)) {
        AddProfiling(gate);
        ElementsKind kind = acc_.TryGetArrayElementsKind(gate);
        StoreJSArrayByIndex(receiver, propKey, value, kind);
    } else if (tsManager_->IsValidTypedArrayType(receiverType)) {
        AddProfiling(gate);
        StoreTypedArrayByIndex(receiver, propKey, value);
    } else {
        return;
    }

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

void TSHCRLowering::SpeculateCallBuiltin(GateRef gate, GateRef func, const std::vector<GateRef> &args,
                                         BuiltinsStubCSigns::ID id, bool isThrow)
{
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, func, builder_.IntPtr(static_cast<int64_t>(id)), args[0]);
    }

    GateRef result = builder_.TypedCallBuiltin(gate, args, id);

    if (isThrow) {
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    } else {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    }
}

BuiltinsStubCSigns::ID TSHCRLowering::GetBuiltinId(BuiltinTypeId id, GateRef func)
{
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsBuiltinObjectMethod(id, funcType)) {
        return BuiltinsStubCSigns::ID::NONE;
    }
    std::string name = tsManager_->GetFuncName(funcType);
    BuiltinsStubCSigns::ID stubId = BuiltinsStubCSigns::GetBuiltinId(name);
    return stubId;
}

void TSHCRLowering::CheckCallTargetFromDefineFuncAndLowerCall(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
    GateType funcType, const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall, bool isNoGC)
{
    if (!Uncheck()) {
        builder_.JSCallTargetFromDefineFuncCheck(funcType, func, gate);
    }
    if (tsManager_->CanFastCall(funcGt)) {
        LowerFastCall(gate, func, argsFastCall, isNoGC);
    } else {
        LowerCall(gate, func, args, isNoGC);
    }
}

void TSHCRLowering::LowerFastCall(GateRef gate, GateRef func,
    const std::vector<GateRef> &argsFastCall, bool isNoGC)
{
    builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
    GateRef result = builder_.TypedFastCall(gate, argsFastCall, isNoGC);
    builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TSHCRLowering::LowerCall(GateRef gate, GateRef func,
    const std::vector<GateRef> &args, bool isNoGC)
{
    builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
    GateRef result = builder_.TypedCall(gate, args, isNoGC);
    builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TSHCRLowering::CheckCallTargetAndLowerCall(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
    GateType funcType, const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall)
{
    if (IsLoadVtable(func)) {
        CheckThisCallTargetAndLowerCall(gate, func, funcGt, funcType, args, argsFastCall); // func = a.foo, func()
    } else {
        bool isNoGC = tsManager_->IsNoGC(funcGt);
        auto op = acc_.GetOpCode(func);
        if (!tsManager_->FastCallFlagIsVaild(funcGt)) {
            return;
        }
        if (op == OpCode::JS_BYTECODE && (acc_.GetByteCodeOpcode(func) == EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8 ||
                                          acc_.GetByteCodeOpcode(func) == EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8)) {
            CheckCallTargetFromDefineFuncAndLowerCall(gate, func, funcGt, funcType, args, argsFastCall, isNoGC);
            return;
        }
        int methodIndex = tsManager_->GetMethodIndex(funcGt);
        if (!tsManager_->MethodOffsetIsVaild(funcGt) || methodIndex == -1) {
            return;
        }
        if (tsManager_->CanFastCall(funcGt)) {
            if (!Uncheck()) {
                builder_.JSCallTargetTypeCheck<TypedCallTargetCheckOp::JSCALL_FAST>(funcType,
                    func, builder_.IntPtr(methodIndex), gate);
            }
            LowerFastCall(gate, func, argsFastCall, isNoGC);
        } else {
            if (!Uncheck()) {
                builder_.JSCallTargetTypeCheck<TypedCallTargetCheckOp::JSCALL>(funcType,
                    func, builder_.IntPtr(methodIndex), gate);
            }
            LowerCall(gate, func, args, isNoGC);
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
    if (IS_TYPED_BUILTINS_MATH_ID(id) && a0Type.IsNumberType()) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, func, { a0Value }, id, false);
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

void TSHCRLowering::CheckFastCallThisCallTarget(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
                                                GateType funcType, bool isNoGC)
{
    if (noCheck_) {
        return;
    }
    if (isNoGC) {
        auto methodOffset = tsManager_->GetFuncMethodOffset(funcGt);
        builder_.JSNoGCCallThisTargetTypeCheck<TypedCallTargetCheckOp::JSCALLTHIS_FAST_NOGC>(funcType,
            func, builder_.IntPtr(methodOffset), gate);
    } else {
        builder_.JSCallThisTargetTypeCheck<TypedCallTargetCheckOp::JSCALLTHIS_FAST>(funcType,
            func, gate);
    }
}

void TSHCRLowering::CheckCallThisCallTarget(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
    GateType funcType, bool isNoGC)
{
    if (noCheck_) {
        return;
    }
    if (isNoGC) {
        auto methodOffset = tsManager_->GetFuncMethodOffset(funcGt);
        builder_.JSNoGCCallThisTargetTypeCheck<TypedCallTargetCheckOp::JSCALLTHIS_NOGC>(funcType,
            func, builder_.IntPtr(methodOffset), gate);
    } else {
        builder_.JSCallThisTargetTypeCheck<TypedCallTargetCheckOp::JSCALLTHIS>(funcType,
            func, gate);
    }
}

void TSHCRLowering::CheckThisCallTargetAndLowerCall(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
    GateType funcType, const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall)
{
    if (!tsManager_->FastCallFlagIsVaild(funcGt)) {
        return;
    }
    bool isNoGC = tsManager_->IsNoGC(funcGt);
    if (tsManager_->CanFastCall(funcGt)) {
        CheckFastCallThisCallTarget(gate, func, funcGt, funcType, isNoGC);
        LowerFastCall(gate, func, argsFastCall, isNoGC);
    } else {
        CheckCallThisCallTarget(gate, func, funcGt, funcType, isNoGC);
        LowerCall(gate, func, args, isNoGC);
    }
}

void TSHCRLowering::LowerTypedCallthis0(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef func = acc_.GetValueIn(gate, 1);
    BuiltinsStubCSigns::ID id = GetBuiltinId(BuiltinTypeId::ARRAY, func);
    if (id == BuiltinsStubCSigns::ID::SORT) {
        AddProfiling(gate);
        GateRef thisObj = acc_.GetValueIn(gate, 0);
        SpeculateCallBuiltin(gate, func, { thisObj }, id, true);
        return;
    }
    if (!CanOptimizeAsFastCall(func)) {
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLTHIS0_IMM8_V8));
    LowerTypedThisCall(gate, func, actualArgc, 0);
}

void TSHCRLowering::LowerTypedCallthis1(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef a0 = acc_.GetValueIn(gate, 1); // 1:parameter index
    GateType a0Type = acc_.GetGateType(a0);
    GateRef func = acc_.GetValueIn(gate, 2); // 2:function
    BuiltinsStubCSigns::ID id = GetBuiltinId(BuiltinTypeId::MATH, func);
    if (id == BuiltinsStubCSigns::ID::NONE) {
        id = GetBuiltinId(BuiltinTypeId::JSON, func);
        if (id != BuiltinsStubCSigns::ID::NONE) {
            AddProfiling(gate);
            SpeculateCallBuiltin(gate, func, { a0 }, id, true);
        }
    } else {
        if (a0Type.IsNumberType()) {
            AddProfiling(gate);
            SpeculateCallBuiltin(gate, func, { a0 }, id, false);
        } else {
            if (!CanOptimizeAsFastCall(func)) {
                return;
            }
            GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
                EcmaOpcode::CALLTHIS1_IMM8_V8_V8));
            LowerTypedThisCall(gate, func, actualArgc, 1);
        }
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
        GateRef thisObj = acc_.GetValueIn(gate, 0);
        GateRef a0 = acc_.GetValueIn(gate, 1);  // 1: the first-para
        GateRef a1 = acc_.GetValueIn(gate, 2);  // 2: the third-para
        GateRef a2 = acc_.GetValueIn(gate, 3);  // 3: the fourth-para
        SpeculateCallBuiltin(gate, func, { thisObj, a0, a1, a2 }, id, true);
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

void TSHCRLowering::AddProfiling(GateRef gate)
{
    hitTypedOpCount_++;
    AddHitBytecodeCount();
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

void TSHCRLowering::AddBytecodeCount(EcmaOpcode op)
{
    currentOp_ = op;
    if (bytecodeMap_.find(op) != bytecodeMap_.end()) {
        bytecodeMap_[op]++;
    } else {
        bytecodeMap_[op] = 1;
    }
}

void TSHCRLowering::DeleteBytecodeCount(EcmaOpcode op)
{
    bytecodeMap_.erase(op);
}

void TSHCRLowering::AddHitBytecodeCount()
{
    if (bytecodeHitTimeMap_.find(currentOp_) != bytecodeHitTimeMap_.end()) {
        bytecodeHitTimeMap_[currentOp_]++;
    } else {
        bytecodeHitTimeMap_[currentOp_] = 1;
    }
}
}  // namespace panda::ecmascript
