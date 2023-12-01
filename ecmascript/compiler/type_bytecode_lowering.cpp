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

#include "ecmascript/compiler/type_bytecode_lowering.h"
#include "ecmascript/builtin_entries.h"
#include "ecmascript/compiler/builtins_lowering.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/enum_conversion.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"

namespace panda::ecmascript::kungfu {
bool TypeBytecodeLowering::RunTypeBytecodeLowering()
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

    if (IsTypeLogEnabled()) {
        pgoTypeLog_.PrintPGOTypeLog();
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After TypeBytecodeLowering "
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

bool TypeBytecodeLowering::IsTrustedType(GateRef gate) const
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

bool TypeBytecodeLowering::IsTrustedStringType(GateRef gate) const
{
    auto op = acc_.GetOpCode(gate);
    if (op == OpCode::LOAD_ELEMENT) {
        return acc_.GetTypedLoadOp(gate) == TypedLoadOp::STRING_LOAD_ELEMENT;
    }
    if (op == OpCode::JS_BYTECODE) {
        EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
        switch (ecmaOpcode) {
            case EcmaOpcode::LDA_STR_ID16:
                return true;
            case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
            case EcmaOpcode::LDOBJBYVALUE_IMM16_V8: {
                GateRef receiver = acc_.GetValueIn(gate, 1);
                GateRef propKey = acc_.GetValueIn(gate, 2);
                GateType receiverType = acc_.GetGateType(receiver);
                GateType propKeyType = acc_.GetGateType(propKey);
                return propKeyType.IsNumberType() && receiverType.IsStringType();
            }
            default:
                break;
        }
    }
    return false;
}

void TypeBytecodeLowering::Lower(GateRef gate)
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
            LowerTypedBinOp<TypedBinOp::TYPED_DIV>(gate);
            break;
        case EcmaOpcode::MOD2_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_MOD>(gate);
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
            LowerTypedEqOrNotEq<TypedBinOp::TYPED_EQ>(gate);
            break;
        case EcmaOpcode::STRICTEQ_IMM8_V8:
            LowerTypedEqOrNotEq<TypedBinOp::TYPED_STRICTEQ>(gate);
            break;
        case EcmaOpcode::NOTEQ_IMM8_V8:
            LowerTypedEqOrNotEq<TypedBinOp::TYPED_NOTEQ>(gate);
            break;
        case EcmaOpcode::STRICTNOTEQ_IMM8_V8:
            LowerTypedEqOrNotEq<TypedBinOp::TYPED_STRICTNOTEQ>(gate);
            break;
        case EcmaOpcode::SHL2_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_SHL>(gate);
            break;
        case EcmaOpcode::SHR2_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_SHR>(gate);
            break;
        case EcmaOpcode::ASHR2_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_ASHR>(gate);
            break;
        case EcmaOpcode::AND2_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_AND>(gate);
            break;
        case EcmaOpcode::OR2_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_OR>(gate);
            break;
        case EcmaOpcode::XOR2_IMM8_V8:
            LowerTypedBinOp<TypedBinOp::TYPED_XOR>(gate);
            break;
        case EcmaOpcode::TONUMERIC_IMM8:
            LowerTypeToNumeric(gate);
            break;
        case EcmaOpcode::NEG_IMM8:
            LowerTypedUnOp<TypedUnOp::TYPED_NEG>(gate);
            break;
        case EcmaOpcode::NOT_IMM8:
            LowerTypedUnOp<TypedUnOp::TYPED_NOT>(gate);
            break;
        case EcmaOpcode::INC_IMM8:
            LowerTypedUnOp<TypedUnOp::TYPED_INC>(gate);
            break;
        case EcmaOpcode::DEC_IMM8:
            LowerTypedUnOp<TypedUnOp::TYPED_DEC>(gate);
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
        case EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8:
            LowerTypedStOwnByName(gate);
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
        case EcmaOpcode::TYPEOF_IMM8:
        case EcmaOpcode::TYPEOF_IMM16:
            LowerTypedTypeOf(gate);
            break;
        case EcmaOpcode::GETITERATOR_IMM8:
        case EcmaOpcode::GETITERATOR_IMM16:
            LowerGetIterator(gate);
            break;
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM16_ID16:
            if (enableLoweringBuiltin_) {
                LowerTypedTryLdGlobalByName(gate);
            }
            break;
        case EcmaOpcode::INSTANCEOF_IMM8_V8:
            LowerInstanceOf(gate);
            break;
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
            LowerCreateObjectWithBuffer(gate);
            break;
        default:
            DeleteBytecodeCount(ecmaOpcode);
            allNonTypedOpCount_++;
            break;
    }
}

template<TypedBinOp Op>
void TypeBytecodeLowering::LowerTypedBinOp(GateRef gate, bool convertNumberType)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (HasNumberType(gate, left, right, convertNumberType)) {
        SpeculateNumbers<Op>(gate);
    } else if (HasStringType(gate, left, right)) {
        SpeculateStrings<Op>(gate);
    }
}

template<TypedUnOp Op>
void TypeBytecodeLowering::LowerTypedUnOp(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    if (HasNumberType(gate, value)) {
        SpeculateNumber<Op>(gate);
    }
}

template<TypedBinOp Op>
void TypeBytecodeLowering::LowerTypedEqOrNotEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    GateType gateType = acc_.GetGateType(gate);
    PGOTypeRef pgoType = acc_.TryGetPGOType(gate);
    if (acc_.IsUndefinedOrNull(left) || acc_.IsUndefinedOrNull(right) || HasNumberType(gate, left, right, false)) {
        AddProfiling(gate);
        GateRef result = builder_.TypedBinaryOp<Op>(
            left, right, leftType, rightType, gateType, pgoType);
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    } else if (HasStringType(gate, left, right)) {
        SpeculateStrings<Op>(gate);
    }
}

bool TypeBytecodeLowering::HasNumberType(GateRef gate, GateRef value) const
{
    GateType valueType = acc_.GetGateType(value);
    const PGOSampleType *sampleType = acc_.TryGetPGOType(gate).GetPGOSampleType();
    if (sampleType->IsNumber() ||
        (sampleType->IsNone() && valueType.IsPrimitiveNumberType())) {
        return true;
    }
    return false;
}

bool TypeBytecodeLowering::HasNumberType(GateRef gate, GateRef left, GateRef right, bool convertNumberType) const
{
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);

    const PGOSampleType *sampleType = acc_.TryGetPGOType(gate).GetPGOSampleType();
    if (sampleType->IsNumber()) {
        return true;
    } else if (convertNumberType && sampleType->IsNone() && leftType.IsPrimitiveNumberType() &&
               rightType.IsPrimitiveNumberType()) {
        return true;
    } else if (!convertNumberType && sampleType->IsNone() && leftType.IsNumberType() && rightType.IsNumberType()) {
        return true;
    } else {
        return false;
    }
    return false;
}

bool TypeBytecodeLowering::HasStringType([[maybe_unused]] GateRef gate, GateRef left, GateRef right) const
{
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    const PGOSampleType *sampleType = acc_.TryGetPGOType(gate).GetPGOSampleType();
    if (sampleType->IsString()) {
        return true;
    } else if (sampleType->IsNone() && leftType.IsStringType() && rightType.IsStringType()) {
        return true;
    }
    return false;
}

template<TypedBinOp Op>
void TypeBytecodeLowering::SpeculateStrings(GateRef gate)
{
    if (Op == TypedBinOp::TYPED_EQ || Op == TypedBinOp::TYPED_ADD) {
        AddProfiling(gate);
        GateRef left = acc_.GetValueIn(gate, 0);
        GateRef right = acc_.GetValueIn(gate, 1);
        if (!IsTrustedStringType(left)) {
            builder_.EcmaStringCheck(left);
        }
        if (!IsTrustedStringType(right)) {
            builder_.EcmaStringCheck(right);
        }
        GateType leftType = acc_.GetGateType(left);
        GateType rightType = acc_.GetGateType(right);
        GateType gateType = acc_.GetGateType(gate);
        PGOTypeRef pgoType = acc_.TryGetPGOType(gate);
        pgoTypeLog_.CollectGateTypeLogInfo(gate, true);

        GateRef result = builder_.TypedBinaryOp<Op>(left, right, leftType, rightType, gateType, pgoType);
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    }
}

template<TypedBinOp Op>
void TypeBytecodeLowering::SpeculateNumbers(GateRef gate)
{
    AddProfiling(gate);
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    GateType gateType = acc_.GetGateType(gate);
    PGOTypeRef pgoType = acc_.TryGetPGOType(gate);
    pgoTypeLog_.CollectGateTypeLogInfo(gate, true);

    GateRef result = builder_.TypedBinaryOp<Op>(left, right, leftType, rightType, gateType, pgoType);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

template<TypedUnOp Op>
void TypeBytecodeLowering::SpeculateNumber(GateRef gate)
{
    AddProfiling(gate);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    GateType gateType = acc_.GetGateType(gate);
    pgoTypeLog_.CollectGateTypeLogInfo(gate, false);

    const PGOSampleType *sampleType = acc_.TryGetPGOType(gate).GetPGOSampleType();
    if (sampleType->IsNumber()) {
        if (sampleType->IsInt()) {
            gateType = GateType::IntType();
        } else if (sampleType->IsDouble()) {
            gateType = GateType::DoubleType();
        } else {
            gateType = GateType::NumberType();
        }
        valueType = gateType;
    }

    GateRef result = builder_.TypedUnaryOp<Op>(value, valueType, gateType);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TypeBytecodeLowering::LowerTypeToNumeric(GateRef gate)
{
    GateRef src = acc_.GetValueIn(gate, 0);
    if (HasNumberType(gate, src)) {
        AddProfiling(gate);
        LowerPrimitiveTypeToNumber(gate);
    }
}

void TypeBytecodeLowering::LowerPrimitiveTypeToNumber(GateRef gate)
{
    GateRef src = acc_.GetValueIn(gate, 0);
    GateType srcType = acc_.GetGateType(src);

    GateRef result = builder_.PrimitiveToNumber(src, VariableType(MachineType::I64, srcType));

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TypeBytecodeLowering::LowerConditionJump(GateRef gate, bool flag)
{
    GateRef condition = acc_.GetValueIn(gate, 0);
    GateType conditionType = acc_.GetGateType(condition);
    if (conditionType.IsBooleanType() && IsTrustedType(condition)) {
        AddProfiling(gate);
        SpeculateConditionJump(gate, flag);
    }
}

void TypeBytecodeLowering::SpeculateConditionJump(GateRef gate, bool flag)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(value);
    GateRef jump = Circuit::NullGate();
    const PGOSampleType *sampleType = acc_.TryGetPGOType(value).GetPGOSampleType();
    if (flag) {
        jump = builder_.TypedConditionJump<TypedJumpOp::TYPED_JNEZ>(value, valueType, sampleType->GetWeight());
    } else {
        jump = builder_.TypedConditionJump<TypedJumpOp::TYPED_JEQZ>(value, valueType, sampleType->GetWeight());
    }
    acc_.ReplaceGate(gate, jump, jump, Circuit::NullGate());
}

void TypeBytecodeLowering::DeleteConstDataIfNoUser(GateRef gate)
{
    auto uses = acc_.Uses(gate);
    if (uses.begin() == uses.end()) {
        builder_.ClearConstantCache(gate);
        acc_.DeleteGate(gate);
    }
}

void TypeBytecodeLowering::LowerTypedLdObjByName(GateRef gate)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto constData = acc_.GetValueIn(gate, 1); // 1: valueIn 1
    uint16_t keyIndex = acc_.GetConstantValue(constData);
    JSTaggedValue key = tsManager_->GetStringFromConstantPool(keyIndex);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 2); // 2: acc or this object

    // deal builtins and array
    GateType tsreceiverType = acc_.GetGateType(receiver);
    tsreceiverType = tsManager_->TryNarrowUnionType(tsreceiverType);
    if (TryLowerTypedLdObjByNameForBuiltin(gate, tsreceiverType, key)) {
        return;
    }

    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    ChunkVector<std::pair<ProfileTyper, ProfileTyper>> types(chunk_);
    FetchPGORWTypesDual(gate, types);
    // get types from pgo

    if (types.size() == 0) {
        return;
    }

    std::vector<Label> loaders;
    std::vector<Label> fails;
    std::vector<PGOObjectAccessHelper> accessHelpers;
    std::vector<PGOObjectAccessInfo> accessInfos;
    std::vector<PGOObjectAccessHelper> checkerHelpers;
    std::vector<PGOObjectAccessInfo> checkerInfos;
    for (size_t i = 0; i < types.size() - 1; ++i) {
        loaders.emplace_back(Label(&builder_));
        fails.emplace_back(Label(&builder_));
    }
    Label exit(&builder_);

    for (size_t i = 0; i < types.size(); ++i) {
        ProfileTyper receiverType = types[i].first;
        ProfileTyper holderType = types[i].second;
        if (receiverType == holderType) {
            accessHelpers.emplace_back(PGOObjectAccessHelper(tsManager_, AccessMode::LOAD,
                receiver, receiverType, key, Circuit::NullGate()));
            accessInfos.emplace_back(PGOObjectAccessInfo(receiverType));
            if (!accessHelpers[i].ComputeForClassInstance(accessInfos[i])) {
                return;
            }
            checkerHelpers.emplace_back(accessHelpers[i]);
            checkerInfos.emplace_back(accessInfos[i]);
        } else {
            accessHelpers.emplace_back(PGOObjectAccessHelper(tsManager_, AccessMode::LOAD,
                receiver, holderType, key, Circuit::NullGate()));
            accessInfos.emplace_back(PGOObjectAccessInfo(holderType));
            if (!accessHelpers[i].ComputeForClassInstance(accessInfos[i])) {
                return;
            }
            checkerHelpers.emplace_back(PGOObjectAccessHelper(tsManager_, AccessMode::LOAD,
                receiver, receiverType, key, Circuit::NullGate()));
            checkerInfos.emplace_back(PGOObjectAccessInfo(receiverType));
            checkerHelpers[i].ComputeForClassInstance(checkerInfos[i]);
            if (checkerInfos[i].HClassIndex() == -1) {
                return;
            }
        }
    }

    AddProfiling(gate);

    auto receiverHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, TaggedObject::HCLASS_OFFSET);
    for (size_t i = 0; i < types.size(); ++i) {
        ProfileTyper receiverType = types[i].first;
        ProfileTyper holderType = types[i].second;
        auto expected = builder_.GetHClassGateFromIndex(gate, checkerInfos[i].HClassIndex());
        if (i != types.size() - 1) {
            builder_.Branch(builder_.Equal(receiverHC, expected), &loaders[i], &fails[i]);
            builder_.Bind(&loaders[i]);
        } else {
            // Deopt if fails at last hclass compare
            builder_.DeoptCheck(builder_.Equal(receiverHC, expected),
                acc_.FindNearestFrameState(builder_.GetDepend()), DeoptType::INCONSISTENTHCLASS);
        }
        if (receiverType == holderType) {
            // Local
            result = BuildNamedPropertyAccess(gate, accessHelpers[i], accessInfos[i].Plr());
            builder_.Jump(&exit);
        } else {
            // prototype change marker check
            auto prototype = builder_.LoadConstOffset(VariableType::JS_ANY(), receiverHC, JSHClass::PROTOTYPE_OFFSET);
            auto protoHClass =
                builder_.LoadConstOffset(VariableType::JS_POINTER(), prototype, TaggedObject::HCLASS_OFFSET);
            auto marker =
                builder_.LoadConstOffset(VariableType::JS_ANY(), protoHClass, JSHClass::PROTO_CHANGE_MARKER_OFFSET);
            builder_.ProtoChangeMarkerCheck(marker, acc_.FindNearestFrameState(builder_.GetDepend()));

            // lookup from receiver for holder
            auto holderHC = builder_.GetHClassGateFromIndex(gate, accessInfos[i].HClassIndex());
            DEFVALUE(current, (&builder_), VariableType::JS_ANY(), prototype);
            Label loopHead(&builder_);
            Label loadHolder(&builder_);
            Label lookUpProto(&builder_);
            builder_.Jump(&loopHead);

            builder_.LoopBegin(&loopHead);
            auto curHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), *current, TaggedObject::HCLASS_OFFSET);
            builder_.Branch(builder_.Equal(curHC, holderHC), &loadHolder, &lookUpProto);

            builder_.Bind(&lookUpProto);
            current = builder_.LoadConstOffset(VariableType::JS_ANY(), curHC, JSHClass::PROTOTYPE_OFFSET);
            builder_.LoopEnd(&loopHead);

            builder_.Bind(&loadHolder);
            accessHelpers[i].SetHolder(*current);
            result = BuildNamedPropertyAccess(gate, accessHelpers[i], accessInfos[i].Plr());
            builder_.Jump(&exit);
        }
        if (i != types.size() - 1) {
            builder_.Bind(&fails[i]);
        }
    }

    builder_.Bind(&exit);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), *result);
    DeleteConstDataIfNoUser(constData);
}

void TypeBytecodeLowering::LowerTypedStObjByName(GateRef gate, bool isThis)
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

    std::vector<std::tuple<ProfileTyper, ProfileTyper, ProfileTyper>> types;
    // get types from pgo

    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate).GetPGORWOpType();
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        types.emplace_back(std::make_tuple(
            std::make_pair(temp.GetReceiverRootType(), temp.GetReceiverType()),
            std::make_pair(temp.GetHoldRootType(), temp.GetHoldType()),
            std::make_pair(temp.GetHoldTraRootType(), temp.GetHoldTraType())
        ));
    }
    // get types from pgo

    if (types.size() == 0) {
        return;
    }

    std::vector<Label> loaders;
    std::vector<Label> fails;
    std::vector<PGOObjectAccessHelper> accessHelpers;
    std::vector<PGOObjectAccessInfo> accessInfos;
    std::vector<PGOObjectAccessHelper> checkerHelpers;
    std::vector<PGOObjectAccessInfo> checkerInfos;
    for (size_t i = 0; i < types.size() - 1; ++i) {
        loaders.emplace_back(Label(&builder_));
        fails.emplace_back(Label(&builder_));
    }
    Label exit(&builder_);

    for (size_t i = 0; i < types.size(); ++i) {
        ProfileTyper receiverType = std::get<0>(types[i]);
        ProfileTyper holderType = std::get<1>(types[i]);
        ProfileTyper newHolderType = std::get<2>(types[i]);
        if (receiverType != newHolderType) {
            // transition happened ==> slowpath
            accessHelpers.emplace_back(PGOObjectAccessHelper(tsManager_, AccessMode::STORE,
                receiver, newHolderType, key, value));
            accessInfos.emplace_back(PGOObjectAccessInfo(newHolderType));
            if (!accessHelpers[i].ComputeForClassInstance(accessInfos[i])) {
                return;
            }
            checkerHelpers.emplace_back(PGOObjectAccessHelper(tsManager_, AccessMode::STORE,
                receiver, receiverType, key, value));
            checkerInfos.emplace_back(PGOObjectAccessInfo(receiverType));
            checkerHelpers[i].ComputeForClassInstance(checkerInfos[i]);
            if (checkerInfos[i].HClassIndex() == -1) {
                return;
            }
        } else if (receiverType == holderType) {
            accessHelpers.emplace_back(PGOObjectAccessHelper(tsManager_, AccessMode::STORE,
                receiver, receiverType, key, value));
            accessInfos.emplace_back(PGOObjectAccessInfo(receiverType));
            if (!accessHelpers[i].ComputeForClassInstance(accessInfos[i])) {
                return;
            }
            checkerHelpers.emplace_back(accessHelpers[i]);
            checkerInfos.emplace_back(accessInfos[i]);
        } else {
            UNREACHABLE();
            return;
        }
    }

    AddProfiling(gate);

    auto receiverHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, TaggedObject::HCLASS_OFFSET);
    for (size_t i = 0; i < types.size(); ++i) {
        ProfileTyper receiverType = std::get<0>(types[i]);
        ProfileTyper holderType = std::get<1>(types[i]);
        ProfileTyper newHolderType = std::get<2>(types[i]);
        auto expected = builder_.GetHClassGateFromIndex(gate, checkerInfos[i].HClassIndex());
        if (i != types.size() - 1) {
            builder_.Branch(builder_.Equal(receiverHC, expected),
                &loaders[i], &fails[i]);
            builder_.Bind(&loaders[i]);
        } else {
            builder_.DeoptCheck(builder_.Equal(receiverHC, expected),
                acc_.FindNearestFrameState(builder_.GetDepend()), DeoptType::INCONSISTENTHCLASS);
        }
        if (receiverType != newHolderType) {
            // prototype change marker check
            auto prototype = builder_.LoadConstOffset(VariableType::JS_ANY(), receiverHC, JSHClass::PROTOTYPE_OFFSET);
            auto protoHClass =
                builder_.LoadConstOffset(VariableType::JS_POINTER(), prototype, TaggedObject::HCLASS_OFFSET);
            auto marker =
                builder_.LoadConstOffset(VariableType::JS_ANY(), protoHClass, JSHClass::PROTO_CHANGE_MARKER_OFFSET);
            auto hasChanged = builder_.GetHasChanged(marker);
            builder_.DeoptCheck(builder_.BoolNot(hasChanged), acc_.FindNearestFrameState(builder_.GetDepend()),
                DeoptType::PROTOTYPECHANGED);

            // transition happened
            Label notProto(&builder_);
            Label isProto(&builder_);
            auto newHolderHC = builder_.GetHClassGateFromIndex(gate, accessInfos[i].HClassIndex());
            builder_.StoreConstOffset(VariableType::JS_ANY(), newHolderHC, JSHClass::PROTOTYPE_OFFSET, prototype);
            builder_.Branch(builder_.IsProtoTypeHClass(receiverHC), &isProto, &notProto,
                BranchWeight::ONE_WEIGHT, BranchWeight::DEOPT_WEIGHT);
            builder_.Bind(&isProto);
            auto propKey = builder_.LoadObjectFromConstPool(argAcc_.GetFrameArgsIn(gate, FrameArgIdx::FUNC), constData);
            builder_.CallRuntime(glue_, RTSTUB_ID(UpdateHClass), Gate::InvalidGateRef,
                { receiverHC, newHolderHC, propKey }, gate);
            builder_.Jump(&notProto);
            builder_.Bind(&notProto);
            MemoryOrder order = MemoryOrder::Create(MemoryOrder::MEMORY_ORDER_RELEASE);
            builder_.StoreConstOffset(VariableType::JS_ANY(), receiver,
                TaggedObject::HCLASS_OFFSET, newHolderHC, order);
            if (!accessInfos[i].Plr().IsInlinedProps()) {
                auto properties =
                    builder_.LoadConstOffset(VariableType::JS_ANY(), receiver, JSObject::PROPERTIES_OFFSET);
                auto capacity = builder_.LoadConstOffset(VariableType::INT32(), properties, TaggedArray::LENGTH_OFFSET);
                auto index = builder_.Int32(accessInfos[i].Plr().GetOffset());
                Label needExtend(&builder_);
                Label notExtend(&builder_);
                builder_.Branch(builder_.Int32UnsignedLessThan(index, capacity), &notExtend, &needExtend);
                builder_.Bind(&notExtend);
                {
                    BuildNamedPropertyAccess(gate, accessHelpers[i], accessInfos[i].Plr());
                    builder_.Jump(&exit);
                }
                builder_.Bind(&needExtend);
                {
                    builder_.CallRuntime(glue_,
                        RTSTUB_ID(PropertiesSetValue),
                        Gate::InvalidGateRef,
                        { receiver, value, properties, builder_.Int32ToTaggedInt(capacity),
                          builder_.Int32ToTaggedInt(index) }, gate);
                    builder_.Jump(&exit);
                }
            } else {
                BuildNamedPropertyAccess(gate, accessHelpers[i], accessInfos[i].Plr());
                builder_.Jump(&exit);
            }
        } else if (receiverType == holderType) {
            // Local
            BuildNamedPropertyAccess(gate, accessHelpers[i], accessInfos[i].Plr());
            builder_.Jump(&exit);
        } else {
            // find in prototype, same as transition
            UNREACHABLE();
            return;
        }
        if (i != types.size() - 1) {
            // process fastpath for next type
            builder_.Bind(&fails[i]);
        }
    }

    builder_.Bind(&exit);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
    DeleteConstDataIfNoUser(constData);
}

void TypeBytecodeLowering::LowerTypedStOwnByName(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    auto constData = acc_.GetValueIn(gate, 0);
    uint16_t propIndex = acc_.GetConstantValue(constData);
    auto key = tsManager_->GetStringFromConstantPool(propIndex);

    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);

    std::vector<std::tuple<ProfileTyper, ProfileTyper, ProfileTyper>> types;
    // get types from pgo

    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate).GetPGORWOpType();
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        types.emplace_back(std::make_tuple(
            std::make_pair(temp.GetReceiverRootType(), temp.GetReceiverType()),
            std::make_pair(temp.GetHoldRootType(), temp.GetHoldType()),
            std::make_pair(temp.GetHoldTraRootType(), temp.GetHoldTraType())
        ));
    }
    // get types from pgo

    if (types.size() == 0) {
        return;
    }

    std::vector<Label> loaders;
    std::vector<Label> fails;
    std::vector<PGOObjectAccessHelper> accessHelpers;
    std::vector<PGOObjectAccessInfo> accessInfos;
    std::vector<PGOObjectAccessHelper> checkerHelpers;
    std::vector<PGOObjectAccessInfo> checkerInfos;
    for (size_t i = 0; i < types.size() - 1; ++i) {
        loaders.emplace_back(Label(&builder_));
        fails.emplace_back(Label(&builder_));
    }
    Label exit(&builder_);

    for (size_t i = 0; i < types.size(); ++i) {
        ProfileTyper receiverType = std::get<0>(types[i]);
        ProfileTyper holderType = std::get<1>(types[i]);
        ProfileTyper newHolderType = std::get<2>(types[i]);
        if (receiverType != newHolderType) {
            // transition happened ==> slowpath
            accessHelpers.emplace_back(PGOObjectAccessHelper(tsManager_, AccessMode::STORE,
                receiver, newHolderType, key, value));
            accessInfos.emplace_back(PGOObjectAccessInfo(newHolderType));
            if (!accessHelpers[i].ComputeForClassInstance(accessInfos[i])) {
                return;
            }
            checkerHelpers.emplace_back(PGOObjectAccessHelper(tsManager_, AccessMode::STORE,
                receiver, receiverType, key, value));
            checkerInfos.emplace_back(PGOObjectAccessInfo(receiverType));
            checkerHelpers[i].ComputeForClassInstance(checkerInfos[i]);
            if (checkerInfos[i].HClassIndex() == -1) {
                return;
            }
        } else if (receiverType == holderType) {
            accessHelpers.emplace_back(PGOObjectAccessHelper(tsManager_, AccessMode::STORE,
                receiver, receiverType, key, value));
            accessInfos.emplace_back(PGOObjectAccessInfo(receiverType));
            if (!accessHelpers[i].ComputeForClassInstance(accessInfos[i])) {
                return;
            }
            checkerHelpers.emplace_back(accessHelpers[i]);
            checkerInfos.emplace_back(accessInfos[i]);
        } else {
            UNREACHABLE();
            return;
        }
    }

    AddProfiling(gate);

    auto receiverHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, TaggedObject::HCLASS_OFFSET);
    for (size_t i = 0; i < types.size(); ++i) {
        ProfileTyper receiverType = std::get<0>(types[i]);
        ProfileTyper holderType = std::get<1>(types[i]);
        ProfileTyper newHolderType = std::get<2>(types[i]);
        auto expected = builder_.GetHClassGateFromIndex(gate, checkerInfos[i].HClassIndex());
        if (i != types.size() - 1) {
            builder_.Branch(builder_.Equal(receiverHC, expected),
                &loaders[i], &fails[i]);
            builder_.Bind(&loaders[i]);
        } else {
            builder_.DeoptCheck(builder_.Equal(receiverHC, expected),
                acc_.FindNearestFrameState(builder_.GetDepend()), DeoptType::INCONSISTENTHCLASS);
        }
        if (receiverType != newHolderType) {
            // prototype change marker check
            auto prototype = builder_.LoadConstOffset(VariableType::JS_ANY(), receiverHC, JSHClass::PROTOTYPE_OFFSET);
            auto protoHClass =
                builder_.LoadConstOffset(VariableType::JS_POINTER(), prototype, TaggedObject::HCLASS_OFFSET);
            auto marker =
                builder_.LoadConstOffset(VariableType::JS_ANY(), protoHClass, JSHClass::PROTO_CHANGE_MARKER_OFFSET);
            auto hasChanged = builder_.GetHasChanged(marker);
            builder_.DeoptCheck(builder_.BoolNot(hasChanged), acc_.FindNearestFrameState(builder_.GetDepend()),
                DeoptType::PROTOTYPECHANGED);

            // transition happened
            Label notProto(&builder_);
            Label isProto(&builder_);
            auto newHolderHC = builder_.GetHClassGateFromIndex(gate, accessInfos[i].HClassIndex());
            builder_.StoreConstOffset(VariableType::JS_ANY(), newHolderHC, JSHClass::PROTOTYPE_OFFSET, prototype);
            builder_.Branch(builder_.IsProtoTypeHClass(receiverHC), &isProto, &notProto,
                BranchWeight::ONE_WEIGHT, BranchWeight::DEOPT_WEIGHT);
            builder_.Bind(&isProto);
            auto propKey = builder_.LoadObjectFromConstPool(argAcc_.GetFrameArgsIn(gate, FrameArgIdx::FUNC), constData);
            builder_.CallRuntime(glue_, RTSTUB_ID(UpdateHClass), Gate::InvalidGateRef,
                { receiverHC, newHolderHC, propKey }, gate);
            builder_.Jump(&notProto);
            builder_.Bind(&notProto);
            MemoryOrder order = MemoryOrder::Create(MemoryOrder::MEMORY_ORDER_RELEASE);
            builder_.StoreConstOffset(VariableType::JS_ANY(), receiver,
                TaggedObject::HCLASS_OFFSET, newHolderHC, order);
            if (!accessInfos[i].Plr().IsInlinedProps()) {
                auto properties =
                    builder_.LoadConstOffset(VariableType::JS_ANY(), receiver, JSObject::PROPERTIES_OFFSET);
                auto capacity = builder_.LoadConstOffset(VariableType::INT32(), properties, TaggedArray::LENGTH_OFFSET);
                auto index = builder_.Int32(accessInfos[i].Plr().GetOffset());
                Label needExtend(&builder_);
                Label notExtend(&builder_);
                builder_.Branch(builder_.Int32UnsignedLessThan(index, capacity), &notExtend, &needExtend);
                builder_.Bind(&notExtend);
                {
                    BuildNamedPropertyAccess(gate, accessHelpers[i], accessInfos[i].Plr());
                    builder_.Jump(&exit);
                }
                builder_.Bind(&needExtend);
                {
                    builder_.CallRuntime(glue_,
                        RTSTUB_ID(PropertiesSetValue),
                        Gate::InvalidGateRef,
                        { receiver, value, properties, builder_.Int32ToTaggedInt(capacity),
                          builder_.Int32ToTaggedInt(index) }, gate);
                    builder_.Jump(&exit);
                }
            } else {
                BuildNamedPropertyAccess(gate, accessHelpers[i], accessInfos[i].Plr());
                builder_.Jump(&exit);
            }
        } else if (receiverType == holderType) {
            // Local
            BuildNamedPropertyAccess(gate, accessHelpers[i], accessInfos[i].Plr());
            builder_.Jump(&exit);
        } else {
            // find in prototype, same as transition
            UNREACHABLE();
            return;
        }
        if (i != types.size() - 1) {
            // process fastpath for next type
            builder_.Bind(&fails[i]);
        }
    }

    builder_.Bind(&exit);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
    DeleteConstDataIfNoUser(constData);
}

GateRef TypeBytecodeLowering::BuildNamedPropertyAccess(
    GateRef hir, PGOObjectAccessHelper accessHelper, PropertyLookupResult plr)
{
    GateRef receiver = accessHelper.GetReceiver();
    GateRef holder = accessHelper.GetHolder();
    GateRef plrGate = builder_.Int32(plr.GetData());
    GateRef result = Circuit::NullGate();

    AccessMode mode = accessHelper.GetAccessMode();
    switch (mode) {
        case AccessMode::LOAD: {
            if (LIKELY(!plr.IsAccessor())) {
                result = builder_.LoadProperty(holder, plrGate, plr.IsFunction());
            } else {
                result = builder_.CallGetter(hir, receiver, holder, plrGate);
            }
        }
            break;
        case AccessMode::STORE: {
            GateRef value = accessHelper.GetValue();
            if (LIKELY(!plr.IsAccessor())) {
                builder_.StoreProperty(receiver, plrGate, value);
            } else {
                builder_.CallSetter(hir, receiver, holder, plrGate, value);
            }
        }
            break;
        default:
            break;
    }

    return result;
}

bool TypeBytecodeLowering::TryLowerTypedLdObjByNameForBuiltin(GateRef gate, GateType receiverType, JSTaggedValue key)
{
    ChunkVector<ProfileType> types(chunk_);
    FetchBuiltinsTypes(gate, types);
    // Just supported mono.
    if (types.size() == 1) {
        if (types[0].IsBuiltinsString()) {
            return TryLowerTypedLdObjByNameForBuiltin(gate, key, BuiltinTypeId::STRING);
        }
        if (types[0].IsBuiltinsArray()) {
            return TryLowerTypedLdObjByNameForBuiltin(gate, key, BuiltinTypeId::ARRAY);
        }
    }

    // String: primitive string type only
    // e.g. let s1 = "ABC"; // OK
    //      let s2 = new String("DEF"); // Not included, whose type is JSType::JS_PRIMITIVE_REF
    if (receiverType.IsStringType()) {
        return TryLowerTypedLdObjByNameForBuiltin(gate, key, BuiltinTypeId::STRING);
    }
    // Array: created via either array literal or new Array(...)
    // e.g. let a1 = [1, 2, 3]; // OK
    //      let a2 = new Array(1, 2, 3); // OK
    if (tsManager_->IsArrayTypeKind(receiverType) ||
        tsManager_->IsBuiltinInstanceType(BuiltinTypeId::ARRAY, receiverType)) {
        return TryLowerTypedLdObjByNameForBuiltin(gate, key, BuiltinTypeId::ARRAY);
    }
    // Other valid types: let x = new X(...);
    const auto hclassEntries = thread_->GetBuiltinHClassEntries();
    for (BuiltinTypeId type: BuiltinHClassEntries::BUILTIN_TYPES) {
        if (type == BuiltinTypeId::ARRAY || type == BuiltinTypeId::STRING || !hclassEntries.EntryIsValid(type)) {
            continue; // Checked before or invalid
        }
        if (!tsManager_->IsBuiltinInstanceType(type, receiverType)) {
            continue; // Type mismatch
        }
        return TryLowerTypedLdObjByNameForBuiltin(gate, key, type);
    }
    return false; // No lowering performed
}

bool TypeBytecodeLowering::TryLowerTypedLdObjByNameForBuiltin(GateRef gate, JSTaggedValue key, BuiltinTypeId type)
{
    EcmaString *propString = EcmaString::Cast(key.GetTaggedObject());
    // (1) get length
    EcmaString *lengthString = EcmaString::Cast(thread_->GlobalConstants()->GetLengthString().GetTaggedObject());
    if (propString == lengthString) {
        if (type == BuiltinTypeId::ARRAY) {
            LowerTypedLdArrayLength(gate);
            return true;
        }
        if (type == BuiltinTypeId::STRING) {
            LowerTypedLdStringLength(gate);
            return true;
        }
        if (IsTypedArrayType(type)) {
            LowerTypedLdTypedArrayLength(gate);
            return true;
        }
    }
    // (2) other functions
    return false;
}

void TypeBytecodeLowering::LowerTypedLdArrayLength(GateRef gate)
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

bool TypeBytecodeLowering::IsCreateArray(GateRef gate)
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

void TypeBytecodeLowering::LowerTypedLdTypedArrayLength(GateRef gate)
{
    AddProfiling(gate);
    GateRef array = acc_.GetValueIn(gate, 2);
    GateType arrayType = acc_.GetGateType(array);
    arrayType = tsManager_->TryNarrowUnionType(arrayType);
    OnHeapMode onHeap = acc_.TryGetOnHeapMode(gate);
    if (!Uncheck()) {
        builder_.TypedArrayCheck(array, arrayType, TypedArrayMetaDateAccessor::Mode::LOAD_LENGTH, onHeap);
    }
    GateRef result = builder_.LoadTypedArrayLength(array, arrayType, onHeap);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TypeBytecodeLowering::LowerTypedLdStringLength(GateRef gate)
{
    AddProfiling(gate);
    GateRef str = acc_.GetValueIn(gate, 2);
    if (!Uncheck()) {
        builder_.EcmaStringCheck(str);
    }
    GateRef result = builder_.LoadStringLength(str);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

bool TypeBytecodeLowering::TryLowerTypedLdObjByNameForBuiltinMethod(GateRef gate, JSTaggedValue key, BuiltinTypeId type)
{
    AddProfiling(gate);
    std::optional<GlobalEnvField> protoField = ToGlobelEnvPrototypeField(type);
    if (!protoField.has_value()) {
        return false;
    }
    size_t protoFieldIndex = static_cast<size_t>(*protoField);
    JSHandle<GlobalEnv> globalEnv = thread_->GetEcmaVM()->GetGlobalEnv();
    JSHClass *prototypeHClass = globalEnv->GetGlobalEnvObjectByIndex(protoFieldIndex)->GetTaggedObject()->GetClass();
    PropertyLookupResult plr = JSHClass::LookupPropertyInBuiltinPrototypeHClass(thread_, prototypeHClass, key);
    // Unable to handle accessor at the moment
    if (!plr.IsFound() || plr.IsAccessor()) {
        return false;
    }
    GateRef receiver = acc_.GetValueIn(gate, 2);
    if (!Uncheck()) {
        // For Array type only: array stability shall be ensured.
        if (type == BuiltinTypeId::ARRAY) {
            builder_.StableArrayCheck(receiver, ElementsKind::GENERIC, ArrayMetaDataAccessor::CALL_BUILTIN_METHOD);
        }
        // This check is not required by String, since string is a primitive type.
        if (type != BuiltinTypeId::STRING) {
            builder_.BuiltinPrototypeHClassCheck(receiver, type);
        }
    }
    // Successfully goes to typed path
    GateRef plrGate = builder_.Int32(plr.GetData());
    GateRef prototype = builder_.GetGlobalEnvObj(builder_.GetGlobalEnv(), static_cast<size_t>(*protoField));
    GateRef result = builder_.LoadProperty(prototype, plrGate, plr.IsFunction());
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    return true;
}

void TypeBytecodeLowering::LowerTypedLdObjByIndex(GateRef gate)
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
        OnHeapMode onHeapMode = acc_.TryGetOnHeapMode(gate);
        result = LoadTypedArrayByIndex(receiver, index, onHeapMode);
    } else {
        return; // slowpath
    }
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TypeBytecodeLowering::LowerTypedStObjByIndex(GateRef gate)
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
            OnHeapMode onHeap = acc_.TryGetOnHeapMode(gate);
            builder_.TypedArrayCheck(receiver, receiverType, TypedArrayMetaDateAccessor::Mode::ACCESS_ELEMENT, onHeap);
        }
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    GateRef index = acc_.GetValueIn(gate, 1);
    uint32_t indexValue = static_cast<uint32_t>(acc_.GetConstantValue(index));
    index = builder_.Int32(indexValue);
    OnHeapMode onHeap = acc_.TryGetOnHeapMode(gate);
    auto length = builder_.LoadTypedArrayLength(receiver, receiverType, onHeap);
    if (!Uncheck()) {
        builder_.IndexCheck(length, index);
    }

    if (tsManager_->IsBuiltinInstanceType(BuiltinTypeId::FLOAT32_ARRAY, receiverType)) {
        builder_.StoreElement<TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT>(receiver, index, value, onHeap);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
}

void TypeBytecodeLowering::LowerTypedLdObjByValue(GateRef gate, bool isThis)
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
    ChunkVector<ProfileType> types(chunk_);
    FetchBuiltinsTypes(gate, types);
    GateRef result = Circuit::NullGate();
    // Just supported mono.
    if (types.size() == 1) {
        if (types[0].IsBuiltinsString()) {
            AddProfiling(gate);
            acc_.SetGateType(propKey, GateType::NumberType());
            result = LoadStringByIndex(receiver, propKey);
            acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
            return;
        } else if (types[0].IsBuiltinsArray()) {
            AddProfiling(gate);
            ElementsKind kind = acc_.TryGetArrayElementsKind(gate);
            acc_.SetGateType(propKey, GateType::NumberType());
            result = LoadJSArrayByIndex(receiver, propKey, kind);
            acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
            return;
        }
    }

    GateType receiverType = acc_.GetGateType(receiver);
    GateType propKeyType = acc_.GetGateType(propKey);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if (!propKeyType.IsNumberType()) {
        return; // slowpath
    }
    if (tsManager_->IsValidTypedArrayType(receiverType)) {
        AddProfiling(gate);
        OnHeapMode onHeapMode = acc_.TryGetOnHeapMode(gate);
        result = LoadTypedArrayByIndex(receiver, propKey, onHeapMode);
    } else {
        return; // slowpath
    }
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

GateRef TypeBytecodeLowering::LoadStringByIndex(GateRef receiver, GateRef propKey)
{
    if (!Uncheck()) {
        builder_.EcmaStringCheck(receiver);
        GateRef length = builder_.LoadStringLength(receiver);
        propKey = builder_.IndexCheck(length, propKey);
        receiver = builder_.FlattenTreeStringCheck(receiver);
    }
    return builder_.LoadElement<TypedLoadOp::STRING_LOAD_ELEMENT>(receiver, propKey);
}

GateRef TypeBytecodeLowering::LoadJSArrayByIndex(GateRef receiver, GateRef propKey, ElementsKind kind)
{
    if (!Uncheck()) {
        if (!IsCreateArray(receiver)) {
            builder_.StableArrayCheck(receiver, kind, ArrayMetaDataAccessor::Mode::LOAD_ELEMENT);
        }
        GateRef length = builder_.LoadArrayLength(receiver);
        propKey = builder_.IndexCheck(length, propKey);
    }

    GateRef result = Circuit::NullGate();
    if (Elements::IsInt(kind)) {
        result = builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_INT_ELEMENT>(receiver, propKey);
    } else if (Elements::IsNumber(kind)) {
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

GateRef TypeBytecodeLowering::LoadTypedArrayByIndex(GateRef receiver, GateRef propKey, OnHeapMode onHeap)
{
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if (!Uncheck()) {
        builder_.TypedArrayCheck(receiver, receiverType, TypedArrayMetaDateAccessor::Mode::ACCESS_ELEMENT, onHeap);
        GateRef length = builder_.LoadTypedArrayLength(receiver, receiverType, onHeap);
        propKey = builder_.IndexCheck(length, propKey);
    }
    auto builtinTypeId = tsManager_->GetTypedArrayBuiltinId(receiverType);
    switch (builtinTypeId) {
        case BuiltinTypeId::INT8_ARRAY:
            return builder_.LoadElement<TypedLoadOp::INT8ARRAY_LOAD_ELEMENT>(receiver, propKey, onHeap);
        case BuiltinTypeId::UINT8_ARRAY:
            return builder_.LoadElement<TypedLoadOp::UINT8ARRAY_LOAD_ELEMENT>(receiver, propKey, onHeap);
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            return builder_.LoadElement<TypedLoadOp::UINT8CLAMPEDARRAY_LOAD_ELEMENT>(receiver, propKey, onHeap);
        case BuiltinTypeId::INT16_ARRAY:
            return builder_.LoadElement<TypedLoadOp::INT16ARRAY_LOAD_ELEMENT>(receiver, propKey, onHeap);
        case BuiltinTypeId::UINT16_ARRAY:
            return builder_.LoadElement<TypedLoadOp::UINT16ARRAY_LOAD_ELEMENT>(receiver, propKey, onHeap);
        case BuiltinTypeId::INT32_ARRAY:
            return builder_.LoadElement<TypedLoadOp::INT32ARRAY_LOAD_ELEMENT>(receiver, propKey, onHeap);
        case BuiltinTypeId::UINT32_ARRAY:
            return builder_.LoadElement<TypedLoadOp::UINT32ARRAY_LOAD_ELEMENT>(receiver, propKey, onHeap);
        case BuiltinTypeId::FLOAT32_ARRAY:
            return builder_.LoadElement<TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT>(receiver, propKey, onHeap);
        case BuiltinTypeId::FLOAT64_ARRAY:
            return builder_.LoadElement<TypedLoadOp::FLOAT64ARRAY_LOAD_ELEMENT>(receiver, propKey, onHeap);
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }

    return Circuit::NullGate();
}

void TypeBytecodeLowering::StoreJSArrayByIndex(GateRef receiver, GateRef propKey, GateRef value, ElementsKind kind)
{
    if (!Uncheck()) {
        if (!IsCreateArray(receiver)) {
            builder_.StableArrayCheck(receiver, kind, ArrayMetaDataAccessor::Mode::STORE_ELEMENT);
        }
        GateRef length = builder_.LoadArrayLength(receiver);
        builder_.IndexCheck(length, propKey);
        builder_.COWArrayCheck(receiver);

        if (Elements::IsObject(kind)) {
            GateRef frameState = acc_.FindNearestFrameState(builder_.GetDepend());
            builder_.HeapObjectCheck(value, frameState);
        }
    }
    builder_.StoreElement<TypedStoreOp::ARRAY_STORE_ELEMENT>(receiver, propKey, value);
}


void TypeBytecodeLowering::StoreTypedArrayByIndex(GateRef receiver, GateRef propKey, GateRef value, OnHeapMode onHeap)
{
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if (!Uncheck()) {
        builder_.TypedArrayCheck(receiver, receiverType, TypedArrayMetaDateAccessor::Mode::ACCESS_ELEMENT, onHeap);
        GateRef length = builder_.LoadTypedArrayLength(receiver, receiverType, onHeap);
        propKey = builder_.IndexCheck(length, propKey);
    }

    auto builtinTypeId = tsManager_->GetTypedArrayBuiltinId(receiverType);
    switch (builtinTypeId) {
        case BuiltinTypeId::INT8_ARRAY:
            builder_.StoreElement<TypedStoreOp::INT8ARRAY_STORE_ELEMENT>(receiver, propKey, value, onHeap);
            break;
        case BuiltinTypeId::UINT8_ARRAY:
            builder_.StoreElement<TypedStoreOp::UINT8ARRAY_STORE_ELEMENT>(receiver, propKey, value, onHeap);
            break;
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            builder_.StoreElement<TypedStoreOp::UINT8CLAMPEDARRAY_STORE_ELEMENT>(receiver, propKey, value, onHeap);
            break;
        case BuiltinTypeId::INT16_ARRAY:
            builder_.StoreElement<TypedStoreOp::INT16ARRAY_STORE_ELEMENT>(receiver, propKey, value, onHeap);
            break;
        case BuiltinTypeId::UINT16_ARRAY:
            builder_.StoreElement<TypedStoreOp::UINT16ARRAY_STORE_ELEMENT>(receiver, propKey, value, onHeap);
            break;
        case BuiltinTypeId::INT32_ARRAY:
            builder_.StoreElement<TypedStoreOp::INT32ARRAY_STORE_ELEMENT>(receiver, propKey, value, onHeap);
            break;
        case BuiltinTypeId::UINT32_ARRAY:
            builder_.StoreElement<TypedStoreOp::UINT32ARRAY_STORE_ELEMENT>(receiver, propKey, value, onHeap);
            break;
        case BuiltinTypeId::FLOAT32_ARRAY:
            builder_.StoreElement<TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT>(receiver, propKey, value, onHeap);
            break;
        case BuiltinTypeId::FLOAT64_ARRAY:
            builder_.StoreElement<TypedStoreOp::FLOAT64ARRAY_STORE_ELEMENT>(receiver, propKey, value, onHeap);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void TypeBytecodeLowering::LowerTypedStObjByValue(GateRef gate)
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
        OnHeapMode onHeapMode = acc_.TryGetOnHeapMode(gate);
        StoreTypedArrayByIndex(receiver, propKey, value, onHeapMode);
    } else {
        return;
    }

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
}

void TypeBytecodeLowering::LowerTypedIsTrueOrFalse(GateRef gate, bool flag)
{
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    auto value = acc_.GetValueIn(gate, 0);
    auto valueType = acc_.GetGateType(value);
    if ((!valueType.IsPrimitiveNumberType()) && (!valueType.IsBooleanType())) {
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

void TypeBytecodeLowering::LowerTypedNewObjRange(GateRef gate)
{
    GateRef ctor = acc_.GetValueIn(gate, 0);
    GateType ctorType = acc_.GetGateType(ctor);
    GlobalTSTypeRef ctorGT = ctorType.GetGTRef();
    if (ctorGT.IsBuiltinModule()) {
        if (TryLowerNewBuiltinConstructor(gate)) {
            return;
        }
    }

    auto sampleType = acc_.TryGetPGOType(gate).GetPGOSampleType();
    if (!sampleType->IsProfileType()) {
        return;
    }
    auto type = std::make_pair(sampleType->GetProfileType(), sampleType->GetProfileType());

    PGOTypeManager *ptManager = thread_->GetCurrentEcmaContext()->GetPTManager();
    int hclassIndex = static_cast<int>(ptManager->GetHClassIndexByProfileType(type));
    if (hclassIndex == -1) {
        return;
    }

    AddProfiling(gate);

    GateRef stateSplit = acc_.GetDep(gate);

    GateRef frameState = acc_.FindNearestFrameState(stateSplit);
    GateRef thisObj = builder_.TypedNewAllocateThis(ctor, builder_.IntPtr(hclassIndex), frameState);

    // call constructor
    size_t range = acc_.GetNumValueIn(gate);
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(range,
        EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8));
    std::vector<GateRef> args { glue_, actualArgc, ctor, ctor, thisObj };
    for (size_t i = 1; i < range; ++i) {  // 1:skip ctor
        args.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef constructGate = builder_.Construct(gate, args);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), constructGate);
}

bool TypeBytecodeLowering::TryLowerNewBuiltinConstructor(GateRef gate)
{
    GateRef ctor = acc_.GetValueIn(gate, 0);
    GateType ctorType = acc_.GetGateType(ctor);
    GlobalTSTypeRef ctorGT = ctorType.GetGTRef();
    GateRef constructGate = Circuit::NullGate();
    if (tsManager_->IsBuiltinConstructor(BuiltinTypeId::ARRAY, ctorGT)) {
        if (acc_.GetNumValueIn(gate) <= 2) { // 2: ctor and first arg
            AddProfiling(gate);
            if (!Uncheck()) {
                builder_.ArrayConstructorCheck(ctor);
            }
            constructGate = builder_.BuiltinConstructor(BuiltinTypeId::ARRAY, gate);
        }
    } else if (tsManager_->IsBuiltinConstructor(BuiltinTypeId::OBJECT, ctorGT)) {
        AddProfiling(gate);
        if (!Uncheck()) {
            builder_.ObjectConstructorCheck(ctor);
        }
        constructGate = builder_.BuiltinConstructor(BuiltinTypeId::OBJECT, gate);
    }
    if (constructGate == Circuit::NullGate()) {
        return false;
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), constructGate);
    return true;
}

void TypeBytecodeLowering::LowerTypedSuperCall(GateRef gate)
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

void TypeBytecodeLowering::SpeculateCallBuiltin(GateRef gate, GateRef func, const std::vector<GateRef> &args,
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

BuiltinsStubCSigns::ID TypeBytecodeLowering::GetBuiltinId(BuiltinTypeId id, GateRef func)
{
    GateType funcType = acc_.GetGateType(func);
    if (!tsManager_->IsBuiltinObjectMethod(id, funcType)) {
        return BuiltinsStubCSigns::ID::NONE;
    }
    std::string name = tsManager_->GetFuncName(funcType);
    BuiltinsStubCSigns::ID stubId = BuiltinsStubCSigns::GetBuiltinId(name);
    return stubId;
}

BuiltinsStubCSigns::ID TypeBytecodeLowering::GetPGOBuiltinId(GateRef gate)
{
    PGOTypeRef sampleType = acc_.TryGetPGOType(gate);
    if (sampleType.GetPGOSampleType()->IsNone()) {
        return BuiltinsStubCSigns::ID::NONE;
    }
    if (sampleType.GetPGOSampleType()->GetProfileType().IsBuiltinFunctionId()) {
        return static_cast<BuiltinsStubCSigns::ID>(sampleType.GetPGOSampleType()->GetProfileType().GetId());
    }
    return BuiltinsStubCSigns::ID::NONE;
}

void TypeBytecodeLowering::CheckCallTargetFromDefineFuncAndLowerCall(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
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

void TypeBytecodeLowering::LowerFastCall(GateRef gate, GateRef func,
    const std::vector<GateRef> &argsFastCall, bool isNoGC)
{
    builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
    GateRef result = builder_.TypedFastCall(gate, argsFastCall, isNoGC);
    builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeBytecodeLowering::LowerCall(GateRef gate, GateRef func,
    const std::vector<GateRef> &args, bool isNoGC)
{
    builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
    GateRef result = builder_.TypedCall(gate, args, isNoGC);
    builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeBytecodeLowering::CheckCallTargetAndLowerCall(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
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

void TypeBytecodeLowering::LowerTypedCallArg0(GateRef gate)
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

void TypeBytecodeLowering::LowerTypedCallArg1(GateRef gate)
{
    GateRef func = acc_.GetValueIn(gate, 1);
    GateRef a0Value = acc_.GetValueIn(gate, 0);
    GateType a0Type = acc_.GetGateType(a0Value);
    BuiltinsStubCSigns::ID id = GetPGOBuiltinId(gate);
    if ((IS_TYPED_BUILTINS_MATH_ID(id) && a0Type.IsNumberType())) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, func, { a0Value }, id, false);
    } else if (IS_TYPED_BUILTINS_NUMBER_ID(id)) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, func, { a0Value }, id, true);
    } else {
        GateType funcType = acc_.GetGateType(func);
        if (!tsManager_->IsFunctionTypeKind(funcType)) {
            return;
        }
        GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
            EcmaOpcode::CALLARG1_IMM8_V8));
        LowerTypedCall(gate, func, actualArgc, funcType, 1);
    }
}

void TypeBytecodeLowering::LowerTypedCallArg2(GateRef gate)
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

void TypeBytecodeLowering::LowerTypedCallArg3(GateRef gate)
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

void TypeBytecodeLowering::LowerTypedCallrange(GateRef gate)
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

void TypeBytecodeLowering::LowerTypedCall(
    GateRef gate, GateRef func, GateRef actualArgc, GateType funcType, uint32_t argc)
{
    GlobalTSTypeRef funcGt = funcType.GetGTRef();
    if (!tsManager_->IsHotnessFunc(funcGt)) {
        return;
    }
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

bool TypeBytecodeLowering::IsLoadVtable(GateRef func)
{
    auto op = acc_.GetOpCode(func);
    if (op != OpCode::LOAD_PROPERTY || !acc_.IsVtable(func)) {
        return false;
    }
    return true;
}

bool TypeBytecodeLowering::CanOptimizeAsFastCall(GateRef func)
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

void TypeBytecodeLowering::CheckFastCallThisCallTarget(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
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

void TypeBytecodeLowering::CheckCallThisCallTarget(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
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

void TypeBytecodeLowering::CheckThisCallTargetAndLowerCall(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
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

void TypeBytecodeLowering::LowerTypedCallthis0(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef func = acc_.GetValueIn(gate, 1);
    BuiltinsStubCSigns::ID id = GetBuiltinId(BuiltinTypeId::ARRAY, func);
    if (id == BuiltinsStubCSigns::ID::SORT) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, func, { thisObj }, id, true);
        return;
    }
    BuiltinsStubCSigns::ID pgoFuncId = GetPGOBuiltinId(gate);
    if (IS_TYPED_BUILTINS_ID_CALL_THIS0(pgoFuncId)) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, func, { thisObj }, pgoFuncId, true);
        return;
    }
    if (!CanOptimizeAsFastCall(func)) {
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLTHIS0_IMM8_V8));
    LowerTypedThisCall(gate, func, actualArgc, 0);
}

void TypeBytecodeLowering::LowerTypedCallthis1(GateRef gate)
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
            return;
        }
    } else if (a0Type.IsNumberType()) {
            AddProfiling(gate);
            SpeculateCallBuiltin(gate, func, { a0 }, id, false);
            return;
    }
    if (!CanOptimizeAsFastCall(func)) {
        return;
    }
    GateRef actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
        EcmaOpcode::CALLTHIS1_IMM8_V8_V8));
    LowerTypedThisCall(gate, func, actualArgc, 1);
}

void TypeBytecodeLowering::LowerTypedCallthis2(GateRef gate)
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

void TypeBytecodeLowering::LowerTypedCallthis3(GateRef gate)
{
    // 5: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 5);
    GateRef func = acc_.GetValueIn(gate, 4); // 4: func
    BuiltinsStubCSigns::ID id = GetBuiltinId(BuiltinTypeId::STRING, func);
    if (IS_TYPED_BUILTINS_ID_CALL_THIS3(id)) {
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

void TypeBytecodeLowering::LowerTypedThisCall(GateRef gate, GateRef func, GateRef actualArgc, uint32_t argc)
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


void TypeBytecodeLowering::LowerTypedCallthisrange(GateRef gate)
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

void TypeBytecodeLowering::AddProfiling(GateRef gate)
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

bool TypeBytecodeLowering::CheckDuplicatedBuiltinType(ChunkVector<ProfileType> &types, ProfileType newType)
{
    for (auto &type : types) {
        if (type.GetBuiltinsId() == newType.GetBuiltinsId()) {
            return true;
        }
    }
    return false;
}

void TypeBytecodeLowering::FetchBuiltinsTypes(GateRef gate, ChunkVector<ProfileType> &types)
{
    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate).GetPGORWOpType();
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsBuiltinsType()) {
            if (CheckDuplicatedBuiltinType(types, temp.GetReceiverType())) {
                continue;
            }
            types.emplace_back(temp.GetReceiverType());
        }
    }
}

void TypeBytecodeLowering::FetchPGORWTypesDual(GateRef gate, ChunkVector<std::pair<ProfileTyper, ProfileTyper>> &types)
{
    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate).GetPGORWOpType();
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsBuiltinsType()) {
            continue;
        }
        types.emplace_back(std::make_pair(
            std::make_pair(temp.GetReceiverRootType(), temp.GetReceiverType()),
            std::make_pair(temp.GetHoldRootType(), temp.GetHoldType())
        ));
    }
}

void TypeBytecodeLowering::AddBytecodeCount(EcmaOpcode op)
{
    currentOp_ = op;
    if (bytecodeMap_.find(op) != bytecodeMap_.end()) {
        bytecodeMap_[op]++;
    } else {
        bytecodeMap_[op] = 1;
    }
}

void TypeBytecodeLowering::DeleteBytecodeCount(EcmaOpcode op)
{
    bytecodeMap_.erase(op);
}

void TypeBytecodeLowering::AddHitBytecodeCount()
{
    if (bytecodeHitTimeMap_.find(currentOp_) != bytecodeHitTimeMap_.end()) {
        bytecodeHitTimeMap_[currentOp_]++;
    } else {
        bytecodeHitTimeMap_[currentOp_] = 1;
    }
}

void TypeBytecodeLowering::LowerTypedTypeOf(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetGateType(gate);
    if (!valueType.IsDigitablePrimitiveType() && !valueType.IsStringType() && !valueType.IsSymbolType()) {
        if (!tsManager_->IsFunctionTypeKind(valueType) && !tsManager_->IsObjectTypeKind(valueType) &&
            !tsManager_->IsClassTypeKind(valueType) && !tsManager_->IsClassInstanceTypeKind(valueType) &&
            !tsManager_->IsArrayTypeKind(valueType)) {
            return;
        }
    }
    AddProfiling(gate);
    if (!Uncheck()) {
        builder_.TypeOfCheck(value, valueType);
    }
    GateRef result = builder_.TypedTypeOf(valueType);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TypeBytecodeLowering::LowerGetIterator(GateRef gate)
{
    BuiltinsStubCSigns::ID id = GetPGOBuiltinId(gate);
    if (id == BuiltinsStubCSigns::ID::NONE) {
        return;
    }
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef obj = acc_.GetValueIn(gate, 0);
    AddProfiling(gate);
    SpeculateCallBuiltin(gate, obj, { obj }, id, true);
}

void TypeBytecodeLowering::LowerTypedTryLdGlobalByName(GateRef gate)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto value = acc_.GetValueIn(gate, 1);
    auto idx = acc_.GetConstantValue(value);
    auto key = tsManager_->GetStringFromConstantPool(idx);
    BuiltinIndex& builtin = BuiltinIndex::GetInstance();
    auto index = builtin.GetBuiltinIndex(key);
    if (index == builtin.NOT_FOUND) {
        return;
    }
    AddProfiling(gate);
    GateRef result = builder_.LoadBuiltinObject(static_cast<uint64_t>(builtin.GetBuiltinBoxOffset(key)));
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    DeleteConstDataIfNoUser(value);
}

void TypeBytecodeLowering::LowerInstanceOf(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef obj = acc_.GetValueIn(gate, 1);     // 1: the second parameter
    GateRef target = acc_.GetValueIn(gate, 2);  // 2: the third parameter

    // CompilerCheck - pgo.hclass having [hasInstance] property
    // if true -> slowthPath
    // if false -> lowering
    JSHandle<GlobalEnv> globalEnv = thread_->GetEcmaVM()->GetGlobalEnv();
    auto hasInstanceEnvIndex = static_cast<size_t>(GlobalEnvField::HASINSTANCE_SYMBOL_INDEX);
    JSTaggedValue key = globalEnv->GetGlobalEnvObjectByIndex(hasInstanceEnvIndex).GetTaggedValue();
    ChunkVector<std::pair<ProfileTyper, ProfileTyper>> types(chunk_);
    FetchPGORWTypesDual(gate, types);

    if (types.size() == 0) {
        return;
    }

    // Instanceof does not support Poly for now
    ASSERT(types.size() == 1);

    ChunkVector<PGOObjectAccessHelper> accessHelpers(chunk_);
    ChunkVector<PGOObjectAccessInfo> accessInfos(chunk_);
    ChunkVector<PGOObjectAccessHelper> checkerHelpers(chunk_);
    ChunkVector<PGOObjectAccessInfo> checkerInfos(chunk_);

    for (size_t i = 0; i < types.size(); ++i) {
        ProfileTyper targetPgoType = types[i].first;
        accessHelpers.emplace_back(PGOObjectAccessHelper(tsManager_, AccessMode::LOAD,
            target, targetPgoType, key, Circuit::NullGate()));
        accessInfos.emplace_back(PGOObjectAccessInfo(targetPgoType));
        // If @@hasInstance is found on prototype Chain of ctor -> slowpath
        // Future work: pgo support Symbol && Dump Object.defineProperty && FunctionPrototypeHclass
        // Current temporary solution:
        // Try searching @@hasInstance in pgo dump stage,
        // If found, pgo dump no types and we go slowpath in aot
        accessHelpers[i].ComputeForClassInstance(accessInfos[i]);
        if (accessInfos[i].HClassIndex() == -1 || !accessHelpers[i].ClassInstanceIsCallable(accessInfos[i])) {
            return;
        }
        checkerHelpers.emplace_back(accessHelpers[i]);
        checkerInfos.emplace_back(accessInfos[i]);
    }

    AddProfiling(gate);

    std::vector<GateRef> expectedHCIndexes;
    for (size_t i = 0; i < types.size(); ++i) {
        GateRef temp = builder_.Int32(checkerInfos[i].HClassIndex());
        expectedHCIndexes.emplace_back(temp);
    }
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    // RuntimeCheck -
    // 1. pgo.hclass == ctor.hclass
    // 2. ctor.hclass has a prototype chain up to Function.prototype
    builder_.ObjectTypeCheck(acc_.GetGateType(gate), true, target, expectedHCIndexes[0]);
    auto ctorHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), target, TaggedObject::HCLASS_OFFSET);
    auto prototype = builder_.LoadConstOffset(VariableType::JS_ANY(), ctorHC, JSHClass::PROTOTYPE_OFFSET);
    auto protoHClass =
        builder_.LoadConstOffset(VariableType::JS_POINTER(), prototype, TaggedObject::HCLASS_OFFSET);
    auto marker =
        builder_.LoadConstOffset(VariableType::JS_ANY(), protoHClass, JSHClass::PROTO_CHANGE_MARKER_OFFSET);
    builder_.ProtoChangeMarkerCheck(marker, acc_.FindNearestFrameState(builder_.GetDepend()));

    result = builder_.OrdinaryHasInstance(obj, target);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), *result);
}

void TypeBytecodeLowering::LowerCreateObjectWithBuffer(GateRef gate)
{
    auto sampleType = acc_.TryGetPGOType(gate).GetPGODefineOpType();
    auto type = std::make_pair(sampleType->GetProfileType(), sampleType->GetProfileType());

    PGOTypeManager *ptManager = thread_->GetCurrentEcmaContext()->GetPTManager();
    int hclassIndex = static_cast<int>(ptManager->GetHClassIndexByProfileType(type));
    if (hclassIndex == -1) {
        return;
    }
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: number of value ins
    GateRef index = acc_.GetValueIn(gate, 0);
    auto imm = acc_.GetConstantValue(index);
    JSTaggedValue obj = ConstantPool::GetLiteralFromCache<ConstPoolType::OBJECT_LITERAL>(
        tsManager_->GetEcmaVM()->GetJSThread(), tsManager_->GetConstantPool().GetTaggedValue(), imm, recordName_);
    JSHandle<JSObject> objhandle = JSHandle<JSObject>(thread_, obj);
    JSHandle<TaggedArray> properties = JSHandle<TaggedArray>(thread_, objhandle->GetProperties());
    if (properties->GetLength() > 0) {
        return;
    }
    JSHandle<TaggedArray> elements = JSHandle<TaggedArray>(thread_, objhandle->GetElements());
    if (elements->GetLength() > 0) {
        return;
    }
    std::vector<uint64_t> inlinedProps;
    JSHandle<JSHClass> newClass = JSHandle<JSHClass>(thread_, ptManager->QueryHClass(type.first, type.second));
    JSHandle<JSHClass> oldClass = JSHandle<JSHClass>(thread_, objhandle->GetClass());
    if (oldClass->GetInlinedProperties() != newClass->GetInlinedProperties()) {
        return;
    }
    auto layout = LayoutInfo::Cast(newClass->GetLayout().GetTaggedObject());
    for (uint32_t i = 0; i < oldClass->GetInlinedProperties(); i++) {
        auto attr = layout->GetAttr(i);
        JSTaggedValue value = objhandle->GetPropertyInlinedProps(i);
        if ((!attr.IsTaggedRep()) || value.IsUndefinedOrNull() ||
            value.IsNumber() || value.IsBoolean() || value.IsException()) {
            auto converted = JSObject::ConvertValueWithRep(attr, value);
            if (!converted.first) {
                return;
            }
            inlinedProps.emplace_back(converted.second.GetRawData());
        } else {
            return;
        }
    }

    GateRef jsFunc = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::FUNC);
    GateRef oldObj = builder_.GetObjectFromConstPool(glue_, gate, jsFunc,
        builder_.TruncInt64ToInt32(index), ConstPoolType::OBJECT_LITERAL);
    GateRef hclass = builder_.LoadConstOffset(VariableType::JS_POINTER(), oldObj, JSObject::HCLASS_OFFSET);
    GateRef emptyArray = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    builder_.StartAllocate();
    auto size = newClass->GetObjectSize();
    GateRef newObj = builder_.HeapAlloc(builder_.IntPtr(size),
        GateType::TaggedValue(), RegionSpaceFlag::IN_YOUNG_SPACE);
    builder_.StoreConstOffset(VariableType::JS_POINTER(), newObj, JSObject::HCLASS_OFFSET, hclass);
    builder_.StoreConstOffset(VariableType::INT64(), newObj,
        JSObject::HASH_OFFSET, builder_.Int64(JSTaggedValue(0).GetRawData()));
    builder_.StoreConstOffset(VariableType::JS_POINTER(), newObj, JSObject::PROPERTIES_OFFSET, emptyArray);
    builder_.StoreConstOffset(VariableType::JS_POINTER(), newObj, JSObject::ELEMENTS_OFFSET, emptyArray);
    for (uint32_t i = 0; i < newClass->GetInlinedProperties(); i++) {
        builder_.StoreConstOffset(VariableType::INT64(), newObj, newClass->GetInlinedPropertiesOffset(i),
            builder_.Int64(inlinedProps.at(i)));
    }
    builder_.FinishAllocate();
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), newObj);
}
}  // namespace panda::ecmascript
