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

#include <string>
#include <cstring>
#include <cstdlib>

#include "ecmascript/compiler/typed_bytecode_lowering.h"
#include "ecmascript/builtin_entries.h"
#include "ecmascript/compiler/builtins_lowering.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/type_info_accessors.h"
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/enum_conversion.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/stackmap/llvm/llvm_stackmap_parser.h"
#include "ecmascript/base/string_helper.h"

namespace panda::ecmascript::kungfu {
bool TypedBytecodeLowering::RunTypedBytecodeLowering()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    ParseOptBytecodeRange();
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
                           << " After TypedBytecodeLowering "
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

void TypedBytecodeLowering::ParseOptBytecodeRange()
{
    std::vector<std::string> splitStrs = base::StringHelper::SplitString(optBCRange_, ",");
    for (const auto &optBCRange : splitStrs) {
        std::vector<std::string> splitRange = base::StringHelper::SplitString(optBCRange, ":");
        if (splitRange.size() == 2) {
            std::vector<int32_t> range;
            std::string start = splitRange[0];
            std::string end = splitRange[1];
            uint32_t startNumber = std::strtoull(start.c_str(), nullptr, 10);
            uint32_t endNumber = std::strtoull(end.c_str(), nullptr, 10);
            range.push_back(static_cast<int32_t>(startNumber));
            range.push_back(static_cast<int32_t>(endNumber));
            optBCRangeList_.push_back(range);
        }
    }
}

bool TypedBytecodeLowering::CheckIsInOptBCIgnoreRange(int32_t index, EcmaOpcode ecmaOpcode)
{
    for (std::vector<int32_t> range : optBCRangeList_) {
        if (index >= range[0] && index <= range[1]) {
            LOG_COMPILER(INFO) << "TypedBytecodeLowering ignore opcode:" << GetEcmaOpcodeStr(ecmaOpcode);
            return true;
        }
    }
    return false;
}

void TypedBytecodeLowering::Lower(GateRef gate)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
    AddBytecodeCount(ecmaOpcode);
    // The order in the switch is referred to in ecmascript/compiler/ecma_opcode_des.h
    int32_t index = GetEcmaOpCodeListIndex(ecmaOpcode);
    if (optBCRangeList_.size() > 0 && CheckIsInOptBCIgnoreRange(index, ecmaOpcode)) {
        DeleteBytecodeCount(ecmaOpcode);
        allNonTypedOpCount_++;
        return;
    }
    switch (ecmaOpcode) {
        case EcmaOpcode::GETITERATOR_IMM8:
        case EcmaOpcode::GETITERATOR_IMM16:
            LowerGetIterator(gate);
            break;
        case EcmaOpcode::CREATEEMPTYOBJECT:
            LowerCreateEmptyObject(gate);
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
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
            LowerCreateObjectWithBuffer(gate);
            break;
        case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8:
        case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
            LowerTypedNewObjRange(gate);
            break;
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
        case EcmaOpcode::EQ_IMM8_V8:
            LowerTypedEqOrNotEq<TypedBinOp::TYPED_EQ>(gate);
            break;
        case EcmaOpcode::NOTEQ_IMM8_V8:
            LowerTypedEqOrNotEq<TypedBinOp::TYPED_NOTEQ>(gate);
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
        case EcmaOpcode::TYPEOF_IMM8:
        case EcmaOpcode::TYPEOF_IMM16:
            LowerTypedTypeOf(gate);
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
        case EcmaOpcode::INSTANCEOF_IMM8_V8:
            LowerInstanceOf(gate);
            break;
        case EcmaOpcode::STRICTNOTEQ_IMM8_V8:
            LowerTypedEqOrNotEq<TypedBinOp::TYPED_STRICTNOTEQ>(gate);
            break;
        case EcmaOpcode::STRICTEQ_IMM8_V8:
            LowerTypedEqOrNotEq<TypedBinOp::TYPED_STRICTEQ>(gate);
            break;
        case EcmaOpcode::ISTRUE:
            LowerTypedIsTrueOrFalse(gate, true);
            break;
        case EcmaOpcode::ISFALSE:
            LowerTypedIsTrueOrFalse(gate, false);
            break;
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
            LowerTypedCallthis3(gate);
            break;
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
            LowerTypedCallthisrange(gate);
            break;
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
            LowerTypedSuperCall(gate);
            break;
        case EcmaOpcode::CALLARG0_IMM8:
            LowerTypedCallArg0(gate);
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
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
            LowerTypedLdObjByName(gate);
            break;
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::DEFINEFIELDBYNAME_IMM8_ID16_V8:
            LowerTypedStObjByName(gate);
            break;
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
        case EcmaOpcode::LDTHISBYVALUE_IMM8:
        case EcmaOpcode::LDTHISBYVALUE_IMM16:
            LowerTypedLdObjByValue(gate);
            break;
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JEQZ_IMM32:
            LowerConditionJump(gate, false);
            break;
        case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
            LowerTypedStObjByValue(gate);
            break;
        case EcmaOpcode::STOWNBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::STOWNBYVALUE_IMM16_V8_V8:
            LowerTypedStOwnByValue(gate);
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
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM16_ID16:
            LowerTypedTryLdGlobalByName(gate);
            break;
        case EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8:
            LowerTypedStOwnByName(gate);
            break;
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
        case EcmaOpcode::JNEZ_IMM32:
            LowerConditionJump(gate, true);
            break;
        case EcmaOpcode::CALLARG1_IMM8_V8:
            LowerTypedCallArg1(gate);
            break;
        case EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8:
            LowerTypedCallInit(gate);
            break;
        default:
            DeleteBytecodeCount(ecmaOpcode);
            allNonTypedOpCount_++;
            break;
    }
}

int32_t TypedBytecodeLowering::GetEcmaOpCodeListIndex(EcmaOpcode ecmaOpCode)
{
    std::vector<EcmaOpcode> opcodeList = GetEcmaCodeListForRange();
    int32_t index = opcodeList.size();
    int32_t size = static_cast<int32_t>(opcodeList.size());
    for (int32_t i = 0; i < size; i++) {
        if (opcodeList[i] == ecmaOpCode) {
            index = i;
            break;
        }
    }
    if (index != size) {
        return index;
    } else {
        return -1;
    }
}

template<TypedBinOp Op>
void TypedBytecodeLowering::LowerTypedBinOp(GateRef gate, bool convertNumberType)
{
    BinOpTypeInfoAccessor tacc(thread_, circuit_, gate, convertNumberType);
    if (tacc.HasNumberType()) {
        SpeculateNumbers<Op>(gate);
    } else if (tacc.HasStringType()) {
        SpeculateStrings<Op>(gate);
    }
}

template<TypedUnOp Op>
void TypedBytecodeLowering::LowerTypedUnOp(GateRef gate)
{
    UnOpTypeInfoAccessor tacc(thread_, circuit_, gate);
    if (tacc.ValueIsNumberType()) {
        SpeculateNumber<Op>(tacc);
    }
}

template<TypedBinOp Op>
void TypedBytecodeLowering::LowerTypedEqOrNotEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetGateType(left);
    GateType rightType = acc_.GetGateType(right);
    GateType gateType = acc_.GetGateType(gate);
    PGOTypeRef pgoType = acc_.TryGetPGOType(gate);

    BinOpTypeInfoAccessor tacc(thread_, circuit_, gate);
    if (tacc.LeftOrRightIsUndefinedOrNullOrHole() || tacc.HasNumberType()) {
        AddProfiling(gate);
        GateRef result = builder_.TypedBinaryOp<Op>(
            left, right, leftType, rightType, gateType, pgoType);
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    } else if (tacc.HasStringType()) {
        SpeculateStrings<Op>(gate);
    }
}

template<TypedBinOp Op>
void TypedBytecodeLowering::SpeculateStrings(GateRef gate)
{
    if (Op == TypedBinOp::TYPED_EQ || Op == TypedBinOp::TYPED_ADD) {
        GateRef left = acc_.GetValueIn(gate, 0);
        GateRef right = acc_.GetValueIn(gate, 1);
        GateType leftType = acc_.GetGateType(left);
        GateType rightType = acc_.GetGateType(right);
        // Only support type is "number" or "string"
        if ((!leftType.IsNumberType() && !leftType.IsStringType()) ||
            (!rightType.IsNumberType() && !rightType.IsStringType())) {
            return ;
        }
        if (leftType.IsNumberType()) {
            left = builder_.NumberToString(left);
            leftType = acc_.GetGateType(left);
            acc_.ReplaceValueIn(gate, left, 0);
        } else if (rightType.IsNumberType()) {
            right = builder_.NumberToString(right);
            rightType = acc_.GetGateType(right);
            acc_.ReplaceValueIn(gate, right, 1);
        }
        AddProfiling(gate);
        if (!TypeInfoAccessor::IsTrustedStringType(thread_, circuit_, chunk_, acc_, left)) {
            builder_.EcmaStringCheck(left);
        }
        if (!TypeInfoAccessor::IsTrustedStringType(thread_, circuit_, chunk_, acc_, right)) {
            builder_.EcmaStringCheck(right);
        }
        GateType gateType = acc_.GetGateType(gate);
        PGOTypeRef pgoType = acc_.TryGetPGOType(gate);
        pgoTypeLog_.CollectGateTypeLogInfo(gate, true);
        GateRef result = builder_.TypedBinaryOp<Op>(left, right, leftType, rightType, gateType, pgoType);
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    }
}

template<TypedBinOp Op>
void TypedBytecodeLowering::SpeculateNumbers(GateRef gate)
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
void TypedBytecodeLowering::SpeculateNumber(const UnOpTypeInfoAccessor &tacc)
{
    AddProfiling(tacc.GetGate());
    pgoTypeLog_.CollectGateTypeLogInfo(tacc.GetGate(), false);
    GateRef value = tacc.GetValue();
    GateType valueType = tacc.FetchNumberType();
    GateRef result = builder_.TypedUnaryOp<Op>(value, valueType, valueType);
    acc_.ReplaceHirAndDeleteIfException(tacc.GetGate(), builder_.GetStateDepend(), result);
}

void TypedBytecodeLowering::LowerTypeToNumeric(GateRef gate)
{
    UnOpTypeInfoAccessor tacc(thread_, circuit_, gate);
    if (tacc.ValueIsNumberType()) {
        AddProfiling(gate);
        LowerPrimitiveTypeToNumber(tacc);
    }
}

void TypedBytecodeLowering::LowerPrimitiveTypeToNumber(const UnOpTypeInfoAccessor &tacc)
{
    GateRef value = tacc.GetValue();
    GateType valueType = tacc.FetchNumberType();
    acc_.SetGateType(value, valueType);
    GateRef result = builder_.PrimitiveToNumber(value,
                                                VariableType(MachineType::I64, valueType));
    acc_.ReplaceHirAndDeleteIfException(tacc.GetGate(), builder_.GetStateDepend(), result);
}

void TypedBytecodeLowering::LowerConditionJump(GateRef gate, bool flag)
{
    ConditionJumpTypeInfoAccessor tacc(thread_, circuit_, gate);
    if (tacc.ValueIsBooleanType() && TypeInfoAccessor::IsTrustedType(acc_, tacc.GetValue())) {
        AddProfiling(gate);
        SpeculateConditionJump(tacc, flag);
    }
}

void TypedBytecodeLowering::SpeculateConditionJump(const ConditionJumpTypeInfoAccessor &tacc, bool flag)
{
    GateRef value = tacc.GetValue();
    GateType valueType = tacc.GetValueGateType();
    uint32_t weight = tacc.GetBranchWeight();
    GateRef jump = Circuit::NullGate();
    if (flag) {
        jump = builder_.TypedConditionJump<TypedJumpOp::TYPED_JNEZ>(value, valueType, weight);
    } else {
        jump = builder_.TypedConditionJump<TypedJumpOp::TYPED_JEQZ>(value, valueType, weight);
    }
    acc_.ReplaceGate(tacc.GetGate(), jump, jump, Circuit::NullGate());
}

void TypedBytecodeLowering::DeleteConstDataIfNoUser(GateRef gate)
{
    auto uses = acc_.Uses(gate);
    if (uses.begin() == uses.end()) {
        builder_.ClearConstantCache(gate);
        acc_.DeleteGate(gate);
    }
}

void TypedBytecodeLowering::LowerTypedLdObjByName(GateRef gate)
{
    DISALLOW_GARBAGE_COLLECTION;
    LoadObjByNameTypeInfoAccessor tacc(thread_, circuit_, gate, chunk_);

    if (TryLowerTypedLdobjBynameFromGloablBuiltin(gate)) {
        return;
    }
    if (TryLowerTypedLdObjByNameForBuiltin(gate)) {
        return;
    }
    if (tacc.TypesIsEmpty() || tacc.HasIllegalType()) {
        return;
    }

    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    size_t typeCount = tacc.GetTypeCount();
    std::vector<Label> loaders;
    std::vector<Label> fails;
    for (size_t i = 0; i < typeCount - 1; ++i) {
        loaders.emplace_back(Label(&builder_));
        fails.emplace_back(Label(&builder_));
    }
    Label exit(&builder_);
    AddProfiling(gate);
    GateRef frameState = acc_.GetFrameState(gate);
    if (tacc.IsMono()) {
        GateRef receiver = tacc.GetReceiver();
        builder_.ObjectTypeCheck(acc_.GetGateType(gate), true, receiver,
                                 builder_.Int32(tacc.GetExpectedHClassIndex(0)), frameState);
        if (tacc.IsReceiverEqHolder(0)) {
            result = BuildNamedPropertyAccess(gate, receiver, receiver, tacc.GetAccessInfo(0).Plr());
        } else {
            builder_.ProtoChangeMarkerCheck(receiver, frameState);
            PropertyLookupResult plr = tacc.GetAccessInfo(0).Plr();
            GateRef plrGate = builder_.Int32(plr.GetData());
            GateRef constpoool = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::CONST_POOL);
            size_t holderHClassIndex = tacc.GetAccessInfo(0).HClassIndex();
            if (LIKELY(!plr.IsAccessor())) {
                result = builder_.MonoLoadPropertyOnProto(receiver, plrGate, constpoool, holderHClassIndex);
            } else {
                result = builder_.MonoCallGetterOnProto(gate, receiver, plrGate, constpoool, holderHClassIndex);
            }
        }
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), *result);
        DeleteConstDataIfNoUser(tacc.GetKey());
        return;
    }
    auto receiverHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), tacc.GetReceiver(),
                                               TaggedObject::HCLASS_OFFSET);
    for (size_t i = 0; i < typeCount; ++i) {
        auto expected = builder_.GetHClassGateFromIndex(gate, tacc.GetExpectedHClassIndex(i));
        if (i != typeCount - 1) {
            builder_.Branch(builder_.Equal(receiverHC, expected), &loaders[i], &fails[i]);
            builder_.Bind(&loaders[i]);
        } else {
            // Deopt if fails at last hclass compare
            builder_.DeoptCheck(builder_.Equal(receiverHC, expected), frameState, DeoptType::INCONSISTENTHCLASS1);
        }

        if (tacc.IsReceiverEqHolder(i)) {
            result = BuildNamedPropertyAccess(gate, tacc.GetReceiver(), tacc.GetReceiver(),
                                              tacc.GetAccessInfo(i).Plr());
            builder_.Jump(&exit);
        } else {
            // prototype change marker check
            builder_.ProtoChangeMarkerCheck(tacc.GetReceiver(), frameState);
            // lookup from receiver for holder
            auto prototype = builder_.LoadConstOffset(VariableType::JS_ANY(), receiverHC, JSHClass::PROTOTYPE_OFFSET);
            // lookup from receiver for holder
            ObjectAccessTypeInfoAccessor::ObjectAccessInfo info = tacc.GetAccessInfo(i);
            auto holderHC = builder_.GetHClassGateFromIndex(gate, info.HClassIndex());
            DEFVALUE(current, (&builder_), VariableType::JS_ANY(), prototype);
            Label loopHead(&builder_);
            Label loadHolder(&builder_);
            Label lookUpProto(&builder_);
            builder_.Jump(&loopHead);

            builder_.LoopBegin(&loopHead);
            builder_.DeoptCheck(builder_.TaggedIsNotNull(*current), frameState, DeoptType::INCONSISTENTHCLASS2);
            auto curHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), *current, TaggedObject::HCLASS_OFFSET);
            builder_.Branch(builder_.Equal(curHC, holderHC), &loadHolder, &lookUpProto);

            builder_.Bind(&lookUpProto);
            current = builder_.LoadConstOffset(VariableType::JS_ANY(), curHC, JSHClass::PROTOTYPE_OFFSET);
            builder_.LoopEnd(&loopHead);

            builder_.Bind(&loadHolder);
            result = BuildNamedPropertyAccess(gate, tacc.GetReceiver(), *current, tacc.GetAccessInfo(i).Plr());
            builder_.Jump(&exit);
        }
        if (i != typeCount - 1) {
            builder_.Bind(&fails[i]);
        }
    }
    builder_.Bind(&exit);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), *result);
    DeleteConstDataIfNoUser(tacc.GetKey());
}

void TypedBytecodeLowering::LowerTypedStObjByName(GateRef gate)
{
    DISALLOW_GARBAGE_COLLECTION;
    StoreObjByNameTypeInfoAccessor tacc(thread_, circuit_, gate, chunk_);
    if (tacc.TypesIsEmpty() || tacc.HasIllegalType()) {
        return;
    }
    size_t typeCount = tacc.GetTypeCount();
    std::vector<Label> loaders;
    std::vector<Label> fails;
    for (size_t i = 0; i < typeCount - 1; ++i) {
        loaders.emplace_back(Label(&builder_));
        fails.emplace_back(Label(&builder_));
    }
    Label exit(&builder_);
    AddProfiling(gate);
    GateRef frameState = Circuit::NullGate();
    auto opcode = acc_.GetByteCodeOpcode(gate);

    // The framestate of Call and Accessor related instructions directives is placed on IR. Using the depend edge to
    // climb up and find the nearest framestate for other instructions
    if (opcode == EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8 ||
        opcode == EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8) {
        frameState = acc_.FindNearestFrameState(builder_.GetDepend());
    } else if (opcode == EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8 ||
               opcode == EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8 ||
               opcode == EcmaOpcode::STTHISBYNAME_IMM8_ID16 ||
               opcode == EcmaOpcode::STTHISBYNAME_IMM16_ID16 ||
               opcode == EcmaOpcode::DEFINEFIELDBYNAME_IMM8_ID16_V8) {
        frameState = acc_.GetFrameState(gate);
    } else {
        UNREACHABLE();
    }

    if (tacc.IsMono()) {
        GateRef receiver = tacc.GetReceiver();
        builder_.ObjectTypeCheck(acc_.GetGateType(gate), true, receiver,
                                 builder_.Int32(tacc.GetExpectedHClassIndex(0)), frameState);
        if (tacc.IsReceiverNoEqNewHolder(0)) {
            builder_.ProtoChangeMarkerCheck(tacc.GetReceiver(), frameState);
            PropertyLookupResult plr = tacc.GetAccessInfo(0).Plr();
            GateRef plrGate = builder_.Int32(plr.GetData());
            GateRef constpool = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::CONST_POOL);
            size_t holderHClassIndex = tacc.GetAccessInfo(0).HClassIndex();
            GateRef value = tacc.GetValue();
            if (tacc.IsHolderEqNewHolder(0)) {
                builder_.MonoStorePropertyLookUpProto(tacc.GetReceiver(), plrGate, constpool, holderHClassIndex, value);
            } else {
                auto propKey = builder_.LoadObjectFromConstPool(argAcc_.GetFrameArgsIn(gate, FrameArgIdx::CONST_POOL),
                                                                tacc.GetKey());
                builder_.MonoStoreProperty(tacc.GetReceiver(), plrGate, constpool, holderHClassIndex, value,
                                           propKey);
            }
        } else if (tacc.IsReceiverEqHolder(0)) {
            BuildNamedPropertyAccess(gate, tacc.GetReceiver(), tacc.GetReceiver(),
                                     tacc.GetValue(), tacc.GetAccessInfo(0).Plr(), tacc.GetExpectedHClassIndex(0));
        }
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
        DeleteConstDataIfNoUser(tacc.GetKey());
        return;
    }

    auto receiverHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), tacc.GetReceiver(),
                                               TaggedObject::HCLASS_OFFSET);
    for (size_t i = 0; i < typeCount; ++i) {
        auto expected = builder_.GetHClassGateFromIndex(gate, tacc.GetExpectedHClassIndex(i));
        if (i != typeCount - 1) {
            builder_.Branch(builder_.Equal(receiverHC, expected),
                &loaders[i], &fails[i]);
            builder_.Bind(&loaders[i]);
        } else {
            builder_.DeoptCheck(builder_.Equal(receiverHC, expected), frameState, DeoptType::INCONSISTENTHCLASS3);
        }
        if (tacc.IsReceiverNoEqNewHolder(i)) {
            builder_.ProtoChangeMarkerCheck(tacc.GetReceiver(), frameState);
            auto prototype = builder_.LoadConstOffset(VariableType::JS_ANY(), receiverHC, JSHClass::PROTOTYPE_OFFSET);
            if (tacc.IsHolderEqNewHolder(i)) {
                // lookup from receiver for holder
                auto holderHC = builder_.GetHClassGateFromIndex(gate, tacc.GetAccessInfo(i).HClassIndex());
                DEFVALUE(current, (&builder_), VariableType::JS_ANY(), prototype);
                Label loopHead(&builder_);
                Label loadHolder(&builder_);
                Label lookUpProto(&builder_);
                builder_.Jump(&loopHead);

                builder_.LoopBegin(&loopHead);
                builder_.DeoptCheck(builder_.TaggedIsNotNull(*current), frameState, DeoptType::INCONSISTENTHCLASS4);
                auto curHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), *current,
                                                      TaggedObject::HCLASS_OFFSET);
                builder_.Branch(builder_.Equal(curHC, holderHC), &loadHolder, &lookUpProto);

                builder_.Bind(&lookUpProto);
                current = builder_.LoadConstOffset(VariableType::JS_ANY(), curHC, JSHClass::PROTOTYPE_OFFSET);
                builder_.LoopEnd(&loopHead);

                builder_.Bind(&loadHolder);
                BuildNamedPropertyAccess(gate, tacc.GetReceiver(), *current, tacc.GetValue(),
                                         tacc.GetAccessInfo(i).Plr());
                builder_.Jump(&exit);
            } else {
                // transition happened
                Label notProto(&builder_);
                Label isProto(&builder_);
                auto newHolderHC = builder_.GetHClassGateFromIndex(gate, tacc.GetAccessInfo(i).HClassIndex());
                builder_.StoreConstOffset(VariableType::JS_ANY(), newHolderHC, JSHClass::PROTOTYPE_OFFSET, prototype);
                builder_.Branch(builder_.IsProtoTypeHClass(receiverHC), &isProto, &notProto,
                    BranchWeight::ONE_WEIGHT, BranchWeight::DEOPT_WEIGHT);
                builder_.Bind(&isProto);
                auto propKey = builder_.LoadObjectFromConstPool(argAcc_.GetFrameArgsIn(gate, FrameArgIdx::CONST_POOL),
                                                                tacc.GetKey());
                builder_.CallRuntime(glue_, RTSTUB_ID(UpdateAOTHClass), Gate::InvalidGateRef,
                    { receiverHC, newHolderHC, propKey }, gate);
                builder_.Jump(&notProto);
                builder_.Bind(&notProto);
                MemoryOrder order = MemoryOrder::NeedBarrierAndAtomic();
                builder_.StoreConstOffset(VariableType::JS_ANY(), tacc.GetReceiver(),
                    TaggedObject::HCLASS_OFFSET, newHolderHC, order);
                if (!tacc.GetAccessInfo(i).Plr().IsInlinedProps()) {
                    auto properties = builder_.LoadConstOffset(VariableType::JS_ANY(), tacc.GetReceiver(),
                                                               JSObject::PROPERTIES_OFFSET);
                    auto capacity =
                        builder_.LoadConstOffset(VariableType::INT32(), properties, TaggedArray::LENGTH_OFFSET);
                    auto index = builder_.Int32(tacc.GetAccessInfo(i).Plr().GetOffset());
                    Label needExtend(&builder_);
                    Label notExtend(&builder_);
                    builder_.Branch(builder_.Int32UnsignedLessThan(index, capacity), &notExtend, &needExtend);
                    builder_.Bind(&notExtend);
                    {
                        BuildNamedPropertyAccess(gate, tacc.GetReceiver(), tacc.GetReceiver(),
                            tacc.GetValue(), tacc.GetAccessInfo(i).Plr());
                        builder_.Jump(&exit);
                    }
                    builder_.Bind(&needExtend);
                    {
                        builder_.CallRuntime(glue_,
                            RTSTUB_ID(PropertiesSetValue),
                            Gate::InvalidGateRef,
                            { tacc.GetReceiver(), tacc.GetValue(), properties, builder_.Int32ToTaggedInt(capacity),
                            builder_.Int32ToTaggedInt(index) }, gate);
                        builder_.Jump(&exit);
                    }
                } else {
                    BuildNamedPropertyAccess(gate, tacc.GetReceiver(), tacc.GetReceiver(),
                        tacc.GetValue(), tacc.GetAccessInfo(i).Plr());
                    builder_.Jump(&exit);
                }
            }
        } else if (tacc.IsReceiverEqHolder(i)) {
            // Local
            BuildNamedPropertyAccess(gate, tacc.GetReceiver(), tacc.GetReceiver(),
                                     tacc.GetValue(), tacc.GetAccessInfo(i).Plr());
            builder_.Jump(&exit);
        } else {
            // find in prototype, same as transition
            UNREACHABLE();
            return;
        }
        if (i != typeCount - 1) {
            // process fastpath for next type
            builder_.Bind(&fails[i]);
        }
    }

    builder_.Bind(&exit);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
    DeleteConstDataIfNoUser(tacc.GetKey());
}

void TypedBytecodeLowering::LowerTypedStOwnByName(GateRef gate)
{
    LowerTypedStObjByName(gate);
}

GateRef TypedBytecodeLowering::BuildNamedPropertyAccess(
    GateRef hir, GateRef receiver, GateRef holder, PropertyLookupResult plr)
{
    GateRef plrGate = builder_.Int32(plr.GetData());
    GateRef result = Circuit::NullGate();
    if (LIKELY(!plr.IsAccessor())) {
        result = builder_.LoadProperty(holder, plrGate, plr.IsFunction());
    } else {
        result = builder_.CallGetter(hir, receiver, holder, plrGate);
    }
    return result;
}

GateRef TypedBytecodeLowering::BuildNamedPropertyAccess(
    GateRef hir, GateRef receiver, GateRef holder, GateRef value, PropertyLookupResult plr,
    uint32_t receiverHClassIndex)
{
    GateRef plrGate = builder_.Int32(plr.GetData());
    GateRef result = Circuit::NullGate();
    if (LIKELY(!plr.IsAccessor())) {
        builder_.StoreProperty(receiver, plrGate, value, receiverHClassIndex);
    } else {
        builder_.CallSetter(hir, receiver, holder, plrGate, value);
    }
    return result;
}

bool TypedBytecodeLowering::TryLowerTypedLdObjByNameForBuiltin(GateRef gate)
{
    LoadBulitinObjTypeInfoAccessor tacc(thread_, circuit_, gate, chunk_);
    // Just supported mono.
    if (tacc.IsMono()) {
        auto builtinsId = tacc.GetBuiltinsTypeId();
        if (builtinsId.has_value()) {
            return TryLowerTypedLdObjByNameForBuiltin(tacc, builtinsId.value());
        }
    }

    // String: primitive string type only
    /* e.g. let s1 = "ABC"; -> OK
     * e.g. let s2 = new String("DEF"); -> Not included, whose type is JSType::JS_PRIMITIVE_REF */
    if (tacc.IsStringType()) {
        return TryLowerTypedLdObjByNameForBuiltin(tacc, BuiltinTypeId::STRING);
    }
    // Array: created via either array literal or new Array(...)
    /* e.g. let a1 = [1, 2, 3]; -> OK
     * e.g. let a2 = new Array(1, 2, 3); -> OK */
    if (tacc.IsArrayType()) {
        return TryLowerTypedLdObjByNameForBuiltin(tacc, BuiltinTypeId::ARRAY);
    }
    // Other valid types: let x = new X(...);
    const auto hclassEntries = thread_->GetBuiltinHClassEntries();
    for (BuiltinTypeId type: BuiltinHClassEntries::BUILTIN_TYPES) {
        if (type == BuiltinTypeId::ARRAY || type == BuiltinTypeId::STRING || !hclassEntries.EntryIsValid(type)) {
            continue; // Checked before or invalid
        }
        if (!tacc.IsBuiltinInstanceType(type)) {
            continue; // Type mismatch
        }
        return TryLowerTypedLdObjByNameForBuiltin(tacc, type);
    }
    return false; // No lowering performed
}

bool TypedBytecodeLowering::TryLowerTypedLdObjByNameForBuiltin(const LoadBulitinObjTypeInfoAccessor &tacc,
                                                               BuiltinTypeId type)
{
    EcmaString *propString = EcmaString::Cast(tacc.GetKeyTaggedValue().GetTaggedObject());
    // (1) get length
    EcmaString *lengthString = EcmaString::Cast(thread_->GlobalConstants()->GetLengthString().GetTaggedObject());
    if (propString == lengthString) {
        if (type == BuiltinTypeId::ARRAY) {
            LowerTypedLdArrayLength(tacc);
            return true;
        }
        if (type == BuiltinTypeId::STRING) {
            LowerTypedLdStringLength(tacc);
            return true;
        }
        if (IsTypedArrayType(type)) {
            LowerTypedLdTypedArrayLength(tacc);
            return true;
        }
    }
    // (2) other functions
    return TryLowerTypedLdObjByNameForBuiltinMethod(tacc, type);
}

bool TypedBytecodeLowering::TryLowerTypedLdobjBynameFromGloablBuiltin(GateRef gate)
{
    LoadBulitinObjTypeInfoAccessor tacc(thread_, circuit_, gate, chunk_);
    GateRef receiver = tacc.GetReceiver();
    if (acc_.GetOpCode(receiver) != OpCode::LOAD_BUILTIN_OBJECT) {
        return false;
    }
    JSHandle<GlobalEnv> globalEnv = thread_->GetEcmaVM()->GetGlobalEnv();
    uint64_t index = acc_.TryGetValue(receiver);
    BuiltinType type = static_cast<BuiltinType>(index);
    if (type == BuiltinType::BT_MATH) {
        auto math = globalEnv->GetMathFunction();
        JSHClass *hclass = math.GetTaggedValue().GetTaggedObject()->GetClass();
        JSTaggedValue key = tacc.GetKeyTaggedValue();
        PropertyLookupResult plr = JSHClass::LookupPropertyInBuiltinHClass(thread_, hclass, key);
        if (!plr.IsFound() || plr.IsAccessor()) {
            return false;
        }
        AddProfiling(gate);
        GateRef plrGate = builder_.Int32(plr.GetData());
        GateRef result = builder_.LoadProperty(receiver, plrGate, plr.IsFunction());
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
        DeleteConstDataIfNoUser(tacc.GetKey());
        return true;
    }
    return false;
}

void TypedBytecodeLowering::LowerTypedLdArrayLength(const LoadBulitinObjTypeInfoAccessor &tacc)
{
    GateRef gate = tacc.GetGate();
    GateRef array = tacc.GetReceiver();
    AddProfiling(gate);
    if (!Uncheck()) {
        ElementsKind kind = acc_.TryGetElementsKind(gate);
        if (!acc_.IsCreateArray(array)) {
            builder_.StableArrayCheck(array, kind, ArrayMetaDataAccessor::Mode::LOAD_LENGTH);
        }
    }

    GateRef result = builder_.LoadArrayLength(array);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TypedBytecodeLowering::LowerTypedLdTypedArrayLength(const LoadBulitinObjTypeInfoAccessor &tacc)
{
    GateRef gate = tacc.GetGate();
    GateRef array = tacc.GetReceiver();
    AddProfiling(gate);
    GateType arrayType = tacc.GetReceiverGateType();
    OnHeapMode onHeap = acc_.TryGetOnHeapMode(gate);
    if (!Uncheck()) {
        builder_.TypedArrayCheck(array, arrayType, TypedArrayMetaDateAccessor::Mode::LOAD_LENGTH, onHeap);
    }
    GateRef result = builder_.LoadTypedArrayLength(array, arrayType, onHeap);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TypedBytecodeLowering::LowerTypedLdStringLength(const LoadBulitinObjTypeInfoAccessor &tacc)
{
    GateRef gate = tacc.GetGate();
    GateRef str = tacc.GetReceiver();
    AddProfiling(gate);
    if (!Uncheck()) {
        builder_.EcmaStringCheck(str);
    }
    GateRef result = builder_.LoadStringLength(str);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

bool TypedBytecodeLowering::TryLowerTypedLdObjByNameForBuiltinMethod(const LoadBulitinObjTypeInfoAccessor &tacc,
                                                                     BuiltinTypeId type)
{
    GateRef gate = tacc.GetGate();
    JSTaggedValue key = tacc.GetKeyTaggedValue();
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
    AddProfiling(gate);
    GateRef receiver = acc_.GetValueIn(gate, 2);
    if (!Uncheck()) {
        // For Array type only: array stability shall be ensured.
        ElementsKind kind = ElementsKind::NONE;
        if (type == BuiltinTypeId::ARRAY) {
            builder_.StableArrayCheck(receiver, ElementsKind::GENERIC, ArrayMetaDataAccessor::CALL_BUILTIN_METHOD);
            kind = tacc.TryGetArrayElementsKind();
        }

        // This check is not required by String, since string is a primitive type.
        if (type != BuiltinTypeId::STRING) {
            builder_.BuiltinPrototypeHClassCheck(receiver, type, kind);
        }
    }
    // Successfully goes to typed path
    GateRef plrGate = builder_.Int32(plr.GetData());
    GateRef prototype = builder_.GetGlobalEnvObj(builder_.GetGlobalEnv(), static_cast<size_t>(*protoField));
    GateRef result = builder_.LoadProperty(prototype, plrGate, plr.IsFunction());
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    return true;
}

bool TypedBytecodeLowering::TryLowerTypedLdObjByIndexForBuiltin(GateRef gate)
{
    LoadBulitinObjTypeInfoAccessor tacc(thread_, circuit_, gate, chunk_);
    GateRef result = Circuit::NullGate();
    if (tacc.IsValidTypedArrayType()) {
        AddProfiling(gate);
        result = LoadTypedArrayByIndex(tacc);
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
        return true;
    }
    return false;
}

void TypedBytecodeLowering::LowerTypedLdObjByIndex(GateRef gate)
{
    if (TryLowerTypedLdObjByIndexForBuiltin(gate)) {
        return;
    }
}

bool TypedBytecodeLowering::TryLowerTypedStObjByIndexForBuiltin(GateRef gate)
{
    StoreBulitinObjTypeInfoAccessor tacc(thread_, circuit_, gate, chunk_);
    if (!tacc.IsBuiltinInstanceType(BuiltinTypeId::FLOAT32_ARRAY) ||
        !tacc.ValueIsNumberType()) {
        return false;
    }
    AddProfiling(gate);
    GateRef receiver = tacc.GetReceiver();
    GateType receiverType = tacc.GetReceiverGateType();
    if (!Uncheck()) {
        OnHeapMode onHeap = tacc.TryGetHeapMode();
        builder_.TypedArrayCheck(receiver, receiverType, TypedArrayMetaDateAccessor::Mode::ACCESS_ELEMENT, onHeap);
    }
    GateRef index = builder_.Int32(tacc.TryConvertKeyToInt());
    GateRef value = tacc.GetValue();
    OnHeapMode onHeap = tacc.TryGetHeapMode();
    auto length = builder_.LoadTypedArrayLength(receiver, receiverType, onHeap);
    if (!Uncheck()) {
        builder_.IndexCheck(length, index);
    }
    builder_.StoreElement<TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT>(receiver, index, value, onHeap);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
    return true;
}

void TypedBytecodeLowering::LowerTypedStObjByIndex(GateRef gate)
{
    if (TryLowerTypedStObjByIndexForBuiltin(gate)) {
        return;
    }
}

bool TypedBytecodeLowering::TryLowerTypedLdObjByValueForBuiltin(GateRef gate)
{
    LoadBulitinObjTypeInfoAccessor tacc(thread_, circuit_, gate, chunk_);
    GateRef result = Circuit::NullGate();
    // Just supported mono.
    if (tacc.IsMono()) {
        if (tacc.IsBuiltinsString()) {
            AddProfiling(gate);
            result = LoadStringByIndex(tacc);
            acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
            return true;
        } else if (tacc.IsBuiltinsArray()) {
            AddProfiling(gate);
            result = LoadJSArrayByIndex(tacc);
            acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
            return true;
        }
    }

    if (!tacc.KeyIsNumberType()) {
        return false;
    }
    if (tacc.IsValidTypedArrayType()) {
        AddProfiling(gate);
        result = LoadTypedArrayByIndex(tacc);
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
        return true;
    }
    return false;
}

void TypedBytecodeLowering::LowerTypedLdObjByValue(GateRef gate)
{
    if (TryLowerTypedLdObjByValueForBuiltin(gate)) {
        return;
    }
}

GateRef TypedBytecodeLowering::LoadStringByIndex(const LoadBulitinObjTypeInfoAccessor &tacc)
{
    GateRef receiver = tacc.GetReceiver();
    GateRef propKey = tacc.GetKey();
    acc_.SetGateType(propKey, GateType::NumberType());
    if (!Uncheck()) {
        builder_.EcmaStringCheck(receiver);
        GateRef length = builder_.LoadStringLength(receiver);
        propKey = builder_.IndexCheck(length, propKey);
        receiver = builder_.FlattenTreeStringCheck(receiver);
    }
    return builder_.LoadElement<TypedLoadOp::STRING_LOAD_ELEMENT>(receiver, propKey);
}

GateRef TypedBytecodeLowering::LoadJSArrayByIndex(const LoadBulitinObjTypeInfoAccessor &tacc)
{
    GateRef receiver = tacc.GetReceiver();
    GateRef propKey = tacc.GetKey();
    acc_.SetGateType(propKey, GateType::NumberType());
    ElementsKind kind = tacc.TryGetArrayElementsKind();
    if (!Uncheck()) {
        if (!acc_.IsCreateArray(receiver)) {
            builder_.StableArrayCheck(receiver, kind, ArrayMetaDataAccessor::Mode::LOAD_ELEMENT);
        }
        GateRef length = builder_.LoadArrayLength(receiver);
        propKey = builder_.IndexCheck(length, propKey);
    }

    GateRef result = Circuit::NullGate();
    if (Elements::IsInt(kind)) {
        // When elementskind switch on, need to add retype for loadInt
        result = builder_.LoadElement<TypedLoadOp::ARRAY_LOAD_INT_ELEMENT>(receiver, propKey);
    } else if (Elements::IsNumber(kind)) {
        // When elementskind switch on, need to add retype for loadNumber
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

GateRef TypedBytecodeLowering::LoadTypedArrayByIndex(const LoadBulitinObjTypeInfoAccessor &tacc)
{
    GateRef receiver = tacc.GetReceiver();
    GateType receiverType = tacc.GetReceiverGateType();
    GateRef propKey = tacc.GetKey();
    OnHeapMode onHeap = tacc.TryGetHeapMode();
    BuiltinTypeId builtinTypeId = tacc.GetTypedArrayBuiltinId();
    if (!Uncheck()) {
        builder_.TypedArrayCheck(receiver, receiverType, TypedArrayMetaDateAccessor::Mode::ACCESS_ELEMENT, onHeap);
        GateRef length = builder_.LoadTypedArrayLength(receiver, receiverType, onHeap);
        propKey = builder_.IndexCheck(length, propKey);
    }
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

void TypedBytecodeLowering::StoreJSArrayByIndex(const StoreBulitinObjTypeInfoAccessor &tacc)
{
    GateRef receiver = tacc.GetReceiver();
    GateRef propKey = tacc.GetKey();
    GateRef value = tacc.GetValue();
    ElementsKind kind = tacc.TryGetArrayElementsKind();
    if (!Uncheck()) {
        if (!acc_.IsCreateArray(receiver)) {
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

void TypedBytecodeLowering::StoreTypedArrayByIndex(const StoreBulitinObjTypeInfoAccessor &tacc)
{
    GateRef receiver = tacc.GetReceiver();
    GateType receiverType = tacc.GetReceiverGateType();
    GateRef propKey = tacc.GetKey();
    GateRef value = tacc.GetValue();
    OnHeapMode onHeap = tacc.TryGetHeapMode();
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

bool TypedBytecodeLowering::TryLowerTypedStObjByValueForBuiltin(GateRef gate)
{
    StoreBulitinObjTypeInfoAccessor tacc(thread_, circuit_, gate, chunk_);
    // Just supported mono.
    if (tacc.IsMono()) {
        if (tacc.IsBuiltinsArray()) {
            AddProfiling(gate);
            StoreJSArrayByIndex(tacc);
            acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
            return true;
        }
    }

    if (!tacc.KeyIsNumberType()) {
        return false;
    }

    if (tacc.IsValidTypedArrayType()) {
        AddProfiling(gate);
        StoreTypedArrayByIndex(tacc);
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), Circuit::NullGate());
        return true;
    }

    return false;
}

void TypedBytecodeLowering::LowerTypedStObjByValue(GateRef gate)
{
    if (TryLowerTypedStObjByValueForBuiltin(gate)) {
        return;
    }
}

void TypedBytecodeLowering::LowerTypedIsTrueOrFalse(GateRef gate, bool flag)
{
    UnOpTypeInfoAccessor tacc(thread_, circuit_, gate);
    if (!tacc.ValueIsPrimitiveNumberType() && !tacc.ValueIsBooleanType()) {
        return;
    }
    AddProfiling(gate);
    GateRef value = tacc.GetValue();
    GateType valueType = tacc.GetValueGateType();
    GateRef result;
    if (!flag) {
        result = builder_.TypedUnaryOp<TypedUnOp::TYPED_ISFALSE>(value, valueType, GateType::TaggedValue());
    } else {
        result = builder_.TypedUnaryOp<TypedUnOp::TYPED_ISTRUE>(value, valueType, GateType::TaggedValue());
    }

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TypedBytecodeLowering::LowerTypedNewObjRange(GateRef gate)
{
    if (TryLowerNewBuiltinConstructor(gate)) {
        return;
    }
    NewObjRangeTypeInfoAccessor tacc(thread_, circuit_, gate);
    if (!tacc.FindHClass()) {
        return;
    }
    AddProfiling(gate);
    GateRef hclassIndex = builder_.IntPtr(tacc.GetHClassIndex());
    GateRef ctor = tacc.GetValue();

    GateRef stateSplit = acc_.GetDep(gate);
    GateRef frameState = acc_.FindNearestFrameState(stateSplit);
    GateRef thisObj = builder_.TypedNewAllocateThis(ctor, hclassIndex, frameState);

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

bool TypedBytecodeLowering::TryLowerNewBuiltinConstructor(GateRef gate)
{
    NewBuiltinCtorTypeInfoAccessor tacc(thread_, circuit_, gate);
    if (!tacc.IsBuiltinModule()) {
        return false;
    }
    GateRef ctor = tacc.GetValue();
    GateRef constructGate = Circuit::NullGate();
    if (tacc.IsBuiltinConstructor(BuiltinTypeId::ARRAY)) {
        if (acc_.GetNumValueIn(gate) <= 2) { // 2: ctor and first arg
            AddProfiling(gate);
            if (!Uncheck()) {
                builder_.ArrayConstructorCheck(ctor);
            }
            constructGate = builder_.BuiltinConstructor(BuiltinTypeId::ARRAY, gate);
        }
    } else if (tacc.IsBuiltinConstructor(BuiltinTypeId::OBJECT)) {
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

void TypedBytecodeLowering::LowerTypedSuperCall(GateRef gate)
{
    SuperCallTypeInfoAccessor tacc(thread_, circuit_, gate);
    if (!tacc.IsClassTypeKind() && !tacc.IsFunctionTypeKind()) {
        return;
    }
    AddProfiling(gate);

    GateRef ctor = tacc.GetCtor();
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

void TypedBytecodeLowering::SpeculateCallBuiltin(GateRef gate, GateRef func, const std::vector<GateRef> &args,
                                                 BuiltinsStubCSigns::ID id, bool isThrow)
{
    if (IS_TYPED_INLINE_BUILTINS_ID(id)) {
        return;
    }
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, func, builder_.IntPtr(static_cast<int64_t>(id)), {args[0]});
    }

    GateRef result = builder_.TypedCallBuiltin(gate, args, id);

    if (isThrow) {
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    } else {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    }
}

void TypedBytecodeLowering::LowerFastCall(GateRef gate, GateRef func,
    const std::vector<GateRef> &argsFastCall, bool isNoGC)
{
    builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
    GateRef result = builder_.TypedFastCall(gate, argsFastCall, isNoGC);
    builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypedBytecodeLowering::LowerCall(GateRef gate, GateRef func,
    const std::vector<GateRef> &args, bool isNoGC)
{
    builder_.StartCallTimer(glue_, gate, {glue_, func, builder_.True()}, true);
    GateRef result = builder_.TypedCall(gate, args, isNoGC);
    builder_.EndCallTimer(glue_, gate, {glue_, func}, true);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

template<class TypeAccessor>
void TypedBytecodeLowering::CheckFastCallThisCallTarget(const TypeAccessor &tacc)
{
    if (noCheck_) {
        return;
    }
    GateRef func = tacc.GetFunc();
    GateRef gate = tacc.GetGate();
    GateType funcType = tacc.GetFuncGateType();
    if (tacc.IsNoGC()) {
        uint32_t methodOffset = tacc.GetFuncMethodOffset();
        builder_.JSNoGCCallThisTargetTypeCheck<TypedCallTargetCheckOp::JSCALLTHIS_FAST_NOGC>(funcType,
            func, builder_.IntPtr(methodOffset), gate);
    } else {
        builder_.JSCallThisTargetTypeCheck<TypedCallTargetCheckOp::JSCALLTHIS_FAST>(funcType,
            func, gate);
    }
}

template<class TypeAccessor>
void TypedBytecodeLowering::CheckCallThisCallTarget(const TypeAccessor &tacc)
{
    if (noCheck_) {
        return;
    }
    GateRef func = tacc.GetFunc();
    GateRef gate = tacc.GetGate();
    GateType funcType = tacc.GetFuncGateType();
    if (tacc.IsNoGC()) {
        auto methodOffset = tacc.GetFuncMethodOffset();
        builder_.JSNoGCCallThisTargetTypeCheck<TypedCallTargetCheckOp::JSCALLTHIS_NOGC>(funcType,
            func, builder_.IntPtr(methodOffset), gate);
    } else {
        builder_.JSCallThisTargetTypeCheck<TypedCallTargetCheckOp::JSCALLTHIS>(funcType,
            func, gate);
    }
}

template<class TypeAccessor>
void TypedBytecodeLowering::CheckThisCallTargetAndLowerCall(const TypeAccessor &tacc,
    const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall)
{
    if (!tacc.FastCallFlagIsVaild()) {
        return;
    }
    GateRef func = tacc.GetFunc();
    GateRef gate = tacc.GetGate();
    bool isNoGC = tacc.IsNoGC();
    if (tacc.CanFastCall()) {
        CheckFastCallThisCallTarget(tacc);
        LowerFastCall(gate, func, argsFastCall, isNoGC);
    } else {
        CheckCallThisCallTarget(tacc);
        LowerCall(gate, func, args, isNoGC);
    }
}

template<class TypeAccessor>
void TypedBytecodeLowering::CheckCallTargetFromDefineFuncAndLowerCall(const TypeAccessor &tacc,
    const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall, bool isNoGC)
{
    GateRef func = tacc.GetFunc();
    GateRef gate = tacc.GetGate();
    GateType funcType = tacc.GetFuncGateType();
    if (!Uncheck()) {
        builder_.JSCallTargetFromDefineFuncCheck(funcType, func, gate);
    }
    if (tacc.CanFastCall()) {
        LowerFastCall(gate, func, argsFastCall, isNoGC);
    } else {
        LowerCall(gate, func, args, isNoGC);
    }
}

template<class TypeAccessor>
void TypedBytecodeLowering::CheckCallTargetAndLowerCall(const TypeAccessor &tacc,
    const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall)
{
    GateRef func = tacc.GetFunc();
    if (IsLoadVtable(func)) {
        CheckThisCallTargetAndLowerCall(tacc, args, argsFastCall); // func = a.foo, func()
    } else {
        bool isNoGC = tacc.IsNoGC();
        auto op = acc_.GetOpCode(func);
        if (!tacc.FastCallFlagIsVaild()) {
            return;
        }
        if (op == OpCode::JS_BYTECODE && (acc_.GetByteCodeOpcode(func) == EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8 ||
                                          acc_.GetByteCodeOpcode(func) == EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8)) {
            CheckCallTargetFromDefineFuncAndLowerCall(tacc, args, argsFastCall, isNoGC);
            return;
        }
        int methodIndex = tacc.GetMethodIndex();
        if (!tacc.MethodOffsetIsVaild() || methodIndex == -1) {
            return;
        }

        GateRef gate = tacc.GetGate();
        GateType funcType = tacc.GetFuncGateType();
        if (tacc.CanFastCall()) {
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

template<EcmaOpcode Op, class TypeAccessor>
void TypedBytecodeLowering::LowerTypedCall(const TypeAccessor &tacc)
{
    uint32_t argc = tacc.GetArgc();
    GateRef gate = tacc.GetGate();
    GateRef actualArgc = Circuit::NullGate();
    switch (Op) {
        case EcmaOpcode::CALLARG0_IMM8: {
            actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
                EcmaOpcode::CALLARG0_IMM8));
            break;
        }
        case EcmaOpcode::CALLARG1_IMM8_V8: {
            actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
                EcmaOpcode::CALLARG1_IMM8_V8));
            break;
        }
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8: {
            actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
                EcmaOpcode::CALLARGS2_IMM8_V8_V8));
            break;
        }
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8: {
            actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
                EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8));
            break;
        }
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8: {
            actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
                EcmaOpcode::CALLRANGE_IMM8_IMM8_V8));
            break;
        }
        default:
            UNREACHABLE();
    }
    if (!tacc.IsHotnessFunc()) {
        return;
    }
    uint32_t len = tacc.GetFunctionTypeLength();
    GateRef func = tacc.GetFunc();
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
    CheckCallTargetAndLowerCall(tacc, args, argsFastCall);
}

void TypedBytecodeLowering::LowerTypedCallArg0(GateRef gate)
{
    CallArg0TypeInfoAccessor tacc(thread_, circuit_, gate);
    if (!tacc.IsFunctionTypeKind()) {
        return;
    }
    LowerTypedCall<EcmaOpcode::CALLARG0_IMM8>(tacc);
}

void TypedBytecodeLowering::LowerTypedCallArg1(GateRef gate)
{
    CallArg1TypeInfoAccessor tacc(thread_, circuit_, gate);
    GateRef func = tacc.GetFunc();
    GateRef a0Value = tacc.GetValue();
    GateType a0Type = tacc.GetValueGateType();
    BuiltinsStubCSigns::ID id = tacc.TryGetPGOBuiltinId();
    if ((IS_TYPED_BUILTINS_MATH_ID(id) && a0Type.IsNumberType())) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, func, { a0Value }, id, false);
    } else if (IS_TYPED_BUILTINS_NUMBER_ID(id)) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, func, { a0Value }, id, true);
    } else {
        if (!tacc.IsFunctionTypeKind()) {
            return;
        }
        LowerTypedCall<EcmaOpcode::CALLARG1_IMM8_V8>(tacc);
    }
}

void TypedBytecodeLowering::LowerTypedCallArg2(GateRef gate)
{
    CallArg2TypeInfoAccessor tacc(thread_, circuit_, gate);
    if (!tacc.IsFunctionTypeKind()) {
        return;
    }
    LowerTypedCall<EcmaOpcode::CALLARGS2_IMM8_V8_V8>(tacc);
}

void TypedBytecodeLowering::LowerTypedCallArg3(GateRef gate)
{
    CallArg3TypeInfoAccessor tacc(thread_, circuit_, gate);
    if (!tacc.IsFunctionTypeKind()) {
        return;
    }
    LowerTypedCall<EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8>(tacc);
}

void TypedBytecodeLowering::LowerTypedCallrange(GateRef gate)
{
    CallRangeTypeInfoAccessor tacc(thread_, circuit_, gate);
    if (!tacc.IsFunctionTypeKind()) {
        return;
    }
    LowerTypedCall<EcmaOpcode::CALLRANGE_IMM8_IMM8_V8>(tacc);
}

bool TypedBytecodeLowering::IsLoadVtable(GateRef func)
{
    auto op = acc_.GetOpCode(func);
    if (op != OpCode::LOAD_PROPERTY || !acc_.IsVtable(func)) {
        return false;
    }
    return true;
}

template<EcmaOpcode Op, class TypeAccessor>
void TypedBytecodeLowering::LowerTypedThisCall(const TypeAccessor &tacc)
{
    uint32_t argc = tacc.GetArgc();
    GateRef gate = tacc.GetGate();
    GateRef actualArgc = Circuit::NullGate();
    switch (Op) {
        case EcmaOpcode::CALLTHIS0_IMM8_V8: {
            actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
                EcmaOpcode::CALLTHIS0_IMM8_V8));
            break;
        }
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8: {
            actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
                EcmaOpcode::CALLTHIS1_IMM8_V8_V8));
            break;
        }
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8: {
            actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
                EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8));
            break;
        }
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8: {
            actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
                EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8));
            break;
        }
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8: {
            actualArgc = builder_.Int64(BytecodeCallArgc::ComputeCallArgc(acc_.GetNumValueIn(gate),
                EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8));
            break;
        }
        default:
            UNREACHABLE();
    }

    uint32_t len = tacc.GetFunctionTypeLength();
    GateRef func = tacc.GetFunc();
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = tacc.GetThisObj();
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
    CheckThisCallTargetAndLowerCall(tacc, args, argsFastCall);
}

void TypedBytecodeLowering::LowerTypedCallthis0(GateRef gate)
{
    CallThis0TypeInfoAccessor tacc(thread_, circuit_, gate);
    BuiltinsStubCSigns::ID id = tacc.TryGetBuiltinId(BuiltinTypeId::ARRAY);
    if (id == BuiltinsStubCSigns::ID::SORT) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, tacc.GetFunc(), { tacc.GetThisObj() }, id, true);
        return;
    }
    BuiltinsStubCSigns::ID pgoFuncId = tacc.TryGetPGOBuiltinId();
    if (IS_TYPED_BUILTINS_ID_CALL_THIS0(pgoFuncId)) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, tacc.GetFunc(), { tacc.GetThisObj() }, pgoFuncId, true);
        return;
    }
    if (!tacc.CanOptimizeAsFastCall()) {
        return;
    }
    LowerTypedThisCall<EcmaOpcode::CALLTHIS0_IMM8_V8>(tacc);
}

void TypedBytecodeLowering::LowerTypedCallthis1(GateRef gate)
{
    CallThis1TypeInfoAccessor tacc(thread_, circuit_, gate);
    BuiltinsStubCSigns::ID id = tacc.TryGetBuiltinId(BuiltinTypeId::MATH);
    if (id == BuiltinsStubCSigns::ID::NONE) {
        id = tacc.TryGetBuiltinId(BuiltinTypeId::JSON);
        if (id != BuiltinsStubCSigns::ID::NONE) {
            AddProfiling(gate);
            SpeculateCallBuiltin(gate, tacc.GetFunc(), { tacc.GetArg0() }, id, true);
            return;
        }
    } else if (tacc.Arg0IsNumberType()) {
            AddProfiling(gate);
            SpeculateCallBuiltin(gate, tacc.GetFunc(), { tacc.GetArg0() }, id, false);
            return;
    }
    if (!tacc.CanOptimizeAsFastCall()) {
        return;
    }
    LowerTypedThisCall<EcmaOpcode::CALLTHIS1_IMM8_V8_V8>(tacc);
}

void TypedBytecodeLowering::LowerTypedCallthis2(GateRef gate)
{
    CallThis2TypeInfoAccessor tacc(thread_, circuit_, gate);
    if (!tacc.CanOptimizeAsFastCall()) {
        return;
    }
    LowerTypedThisCall<EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8>(tacc);
}

void TypedBytecodeLowering::LowerTypedCallthis3(GateRef gate)
{
    CallThis3TypeInfoAccessor tacc(thread_, circuit_, gate);
    BuiltinsStubCSigns::ID id = tacc.TryGetBuiltinId(BuiltinTypeId::STRING);
    if (IS_TYPED_BUILTINS_ID_CALL_THIS3(id)) {
        AddProfiling(gate);
        SpeculateCallBuiltin(gate, tacc.GetFunc(), tacc.GetArgs(), id, true);
        return;
    }
    if (!tacc.CanOptimizeAsFastCall()) {
        return;
    }
    LowerTypedThisCall<EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8>(tacc);
}

void TypedBytecodeLowering::LowerTypedCallthisrange(GateRef gate)
{
    CallThisRangeTypeInfoAccessor tacc(thread_, circuit_, gate);
    if (!tacc.CanOptimizeAsFastCall()) {
        return;
    }
    LowerTypedThisCall<EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8>(tacc);
}

void TypedBytecodeLowering::LowerTypedCallInit(GateRef gate)
{
    // same as callthis0
    LowerTypedCallthis0(gate);
}

void TypedBytecodeLowering::AddProfiling(GateRef gate)
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

        if (acc_.HasFrameState(gate)) {
            // func, pcoffset, opcode, mode
            GateRef func = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::FUNC);
            GateRef bcIndex = builder_.Int32ToTaggedInt(builder_.Int32(acc_.TryGetBcIndex(gate)));
            EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
            auto ecmaOpcodeGate = builder_.Int32(static_cast<uint32_t>(ecmaOpcode));
            GateRef constOpcode = builder_.Int32ToTaggedInt(ecmaOpcodeGate);
            GateRef mode =
                builder_.Int32ToTaggedInt(builder_.Int32(static_cast<int32_t>(OptCodeProfiler::Mode::TYPED_PATH)));
            GateRef profiling = builder_.CallRuntime(glue_, RTSTUB_ID(ProfileOptimizedCode), acc_.GetDep(current),
                { func, bcIndex, constOpcode, mode }, gate);
            acc_.SetDep(current, profiling);
            builder_.SetDepend(acc_.GetDep(gate));  // set gate depend: profiling or STATE_SPLIT
        }
    }
}

void TypedBytecodeLowering::AddBytecodeCount(EcmaOpcode op)
{
    currentOp_ = op;
    if (bytecodeMap_.find(op) != bytecodeMap_.end()) {
        bytecodeMap_[op]++;
    } else {
        bytecodeMap_[op] = 1;
    }
}

void TypedBytecodeLowering::DeleteBytecodeCount(EcmaOpcode op)
{
    bytecodeMap_.erase(op);
}

void TypedBytecodeLowering::AddHitBytecodeCount()
{
    if (bytecodeHitTimeMap_.find(currentOp_) != bytecodeHitTimeMap_.end()) {
        bytecodeHitTimeMap_[currentOp_]++;
    } else {
        bytecodeHitTimeMap_[currentOp_] = 1;
    }
}

void TypedBytecodeLowering::LowerTypedTypeOf(GateRef gate)
{
    // 1: number of value inputs
    TypeOfTypeInfoAccessor tacc(thread_, circuit_, gate);
    if (tacc.IsIllegalType()) {
        return;
    }
    AddProfiling(gate);
    if (!Uncheck()) {
        builder_.TypeOfCheck(tacc.GetValue(), tacc.GetValueGateType());
    }
    GateRef result = builder_.TypedTypeOf(tacc.GetValueGateType());
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TypedBytecodeLowering::LowerGetIterator(GateRef gate)
{
    GetIteratorTypeInfoAccessor tacc(thread_, circuit_, gate);
    BuiltinsStubCSigns::ID id = tacc.TryGetPGOBuiltinId();
    if (id == BuiltinsStubCSigns::ID::NONE) {
        return;
    }
    AddProfiling(gate);
    GateRef obj = tacc.GetCallee();
    SpeculateCallBuiltin(gate, obj, { obj }, id, true);
}

void TypedBytecodeLowering::LowerTypedTryLdGlobalByName(GateRef gate)
{
    if (!enableLoweringBuiltin_) {
        return;
    }
    DISALLOW_GARBAGE_COLLECTION;
    LoadGlobalObjByNameTypeInfoAccessor tacc(thread_, circuit_, gate);
    JSTaggedValue key = tacc.GetKeyTaggedValue();

    BuiltinIndex& builtin = BuiltinIndex::GetInstance();
    auto index = builtin.GetBuiltinIndex(key);
    if (index == builtin.NOT_FOUND) {
        return;
    }
    AddProfiling(gate);
    GateRef result = builder_.LoadBuiltinObject(index);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
    DeleteConstDataIfNoUser(tacc.GetKey());
}

void TypedBytecodeLowering::LowerInstanceOf(GateRef gate)
{
    InstanceOfTypeInfoAccessor tacc(thread_, circuit_, gate, chunk_);
    if (tacc.TypesIsEmpty() || tacc.HasIllegalType()) {
        return;
    }
    AddProfiling(gate);
    size_t typeCount = tacc.GetTypeCount();
    std::vector<GateRef> expectedHCIndexes;
    for (size_t i = 0; i < typeCount; ++i) {
        GateRef temp = builder_.Int32(tacc.GetExpectedHClassIndex(i));
        expectedHCIndexes.emplace_back(temp);
    }
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    // RuntimeCheck -
    // 1. pgo.hclass == ctor.hclass
    // 2. ctor.hclass has a prototype chain up to Function.prototype
    GateRef obj = tacc.GetReceiver();
    GateRef target = tacc.GetTarget();

    builder_.ObjectTypeCheck(acc_.GetGateType(gate), true, target, expectedHCIndexes[0]);
    builder_.ProtoChangeMarkerCheck(target);

    result = builder_.OrdinaryHasInstance(obj, target);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), *result);
}

void TypedBytecodeLowering::LowerCreateEmptyObject(GateRef gate)
{
    AddProfiling(gate);
    GateRef globalEnv = builder_.GetGlobalEnv();
    GateRef hclass = builder_.GetGlobalEnvObjHClass(globalEnv, GlobalEnv::OBJECT_FUNCTION_INDEX);

    JSHandle<JSFunction> objectFunc(tsManager_->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    JSTaggedValue protoOrHClass = objectFunc->GetProtoOrHClass();
    JSHClass *objectHC = JSHClass::Cast(protoOrHClass.GetTaggedObject());
    size_t objectSize = objectHC->GetObjectSize();

    GateRef emptyArray = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    GateRef size = builder_.IntPtr(objectHC->GetObjectSize());

    builder_.StartAllocate();
    GateRef object = builder_.HeapAlloc(glue_, size, GateType::TaggedValue(), RegionSpaceFlag::IN_YOUNG_SPACE);

    // initialization
    for (size_t offset = JSObject::SIZE; offset < objectSize; offset += JSTaggedValue::TaggedTypeSize()) {
        builder_.StoreConstOffset(VariableType::INT64(), object, offset, builder_.Undefined());
    }
    builder_.StoreConstOffset(VariableType::JS_POINTER(), object, JSObject::HCLASS_OFFSET, hclass,
        MemoryOrder::NeedBarrierAndAtomic());
    builder_.StoreConstOffset(VariableType::INT64(), object, JSObject::HASH_OFFSET,
                              builder_.Int64(JSTaggedValue(0).GetRawData()));
    builder_.StoreConstOffset(VariableType::JS_POINTER(), object, JSObject::PROPERTIES_OFFSET, emptyArray,
        MemoryOrder::NoBarrier());
    builder_.StoreConstOffset(VariableType::JS_POINTER(), object, JSObject::ELEMENTS_OFFSET, emptyArray,
        MemoryOrder::NoBarrier());
    GateRef result = builder_.FinishAllocate(object);

    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), result);
}

void TypedBytecodeLowering::LowerTypedStOwnByValue(GateRef gate)
{
    // StOwnByValue is rarely used, so the callruntime solution is used
    AddProfiling(gate);
    return;
}

void TypedBytecodeLowering::LowerCreateObjectWithBuffer(GateRef gate)
{
    CreateObjWithBufferTypeInfoAccessor tacc(thread_, circuit_, gate, recordName_);
    if (!tacc.CanOptimize()) {
        return;
    }
    JSTaggedValue hclassVal = tacc.GetHClass();
    if (hclassVal.IsUndefined()) {
        return;
    }
    JSHandle<JSHClass> newClass(thread_, hclassVal);
    GateRef index = tacc.GetIndex();
    JSHandle<JSObject> objhandle = tacc.GetObjHandle();
    std::vector<uint64_t> inlinedProps;
    auto layout = LayoutInfo::Cast(newClass->GetLayout().GetTaggedObject());
    for (uint32_t i = 0; i < newClass->GetInlinedProperties(); i++) {
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

    AddProfiling(gate);
    auto size = newClass->GetObjectSize();
    std::vector<GateRef> valueIn;
    valueIn.emplace_back(builder_.IntPtr(size));
    valueIn.emplace_back(index);
    valueIn.emplace_back(builder_.Int64(newClass.GetTaggedValue().GetRawData()));
    valueIn.emplace_back(acc_.GetValueIn(gate, 1));
    for (uint32_t i = 0; i < newClass->GetInlinedProperties(); i++) {
        auto attr = layout->GetAttr(i);
        GateRef prop;
        if (attr.IsIntRep()) {
            prop = builder_.Int32(inlinedProps.at(i));
        } else if (attr.IsTaggedRep()) {
            prop = circuit_->NewGate(circuit_->GetMetaBuilder()->Constant(inlinedProps.at(i)),
                                     MachineType::I64, GateType::AnyType());
        } else if (attr.IsDoubleRep()) {
            prop = circuit_->NewGate(circuit_->GetMetaBuilder()->Constant(inlinedProps.at(i)),
                                     MachineType::F64, GateType::NJSValue());
        } else {
            prop = builder_.Int64(inlinedProps.at(i));
        }
        valueIn.emplace_back(prop);
        valueIn.emplace_back(builder_.Int32(newClass->GetInlinedPropertiesOffset(i)));
    }
    GateRef ret = builder_.TypedCreateObjWithBuffer(valueIn);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}
}  // namespace panda::ecmascript
