/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/type_info_accessors.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_fields.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/ts_types/ts_type_accessor.h"

namespace panda::ecmascript::kungfu {
JSTaggedValue TypeInfoAccessor::GetStringFromConstantPool(const JSThread *thread, uint32_t methodId, uint32_t index)
{
    return thread->GetCurrentEcmaContext()->GetTSManager()->GetStringFromConstantPool(methodId, index);
}

// TypeTrusted means the type of gate is already PrimitiveTypeCheck-passed,
// or the gate is constant and no need to check.
bool TypeInfoAccessor::IsTrustedType(GateAccessor acc, GateRef gate)
{
    if (acc.IsConstant(gate)) {
        return true;
    }
    auto op = acc.GetOpCode(gate);
    if (acc.IsTypedOperator(gate)) {
        if (op == OpCode::TYPED_BINARY_OP) {
            return !acc.GetGateType(gate).IsIntType();
        } else {
            return true;
        }
    }
    if (op == OpCode::JS_BYTECODE) {
        EcmaOpcode ecmaOpcode = acc.GetByteCodeOpcode(gate);
        switch (ecmaOpcode) {
            case EcmaOpcode::ADD2_IMM8_V8:
            case EcmaOpcode::SUB2_IMM8_V8:
            case EcmaOpcode::MUL2_IMM8_V8:
                return !acc.GetGateType(gate).IsIntType();
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

bool TypeInfoAccessor::IsTrustedStringType(
    const JSThread *thread, Circuit *circuit, Chunk *chunk, GateAccessor acc, GateRef gate)
{
    auto op = acc.GetOpCode(gate);
    if (op == OpCode::LOAD_ELEMENT) {
        return acc.GetTypedLoadOp(gate) == TypedLoadOp::STRING_LOAD_ELEMENT;
    }
    if (op == OpCode::JS_BYTECODE) {
        EcmaOpcode ecmaOpcode = acc.GetByteCodeOpcode(gate);
        switch (ecmaOpcode) {
            case EcmaOpcode::LDA_STR_ID16:
                return true;
            case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
            case EcmaOpcode::LDOBJBYVALUE_IMM16_V8: {
                LoadBulitinObjTypeInfoAccessor tacc(thread, circuit, gate, chunk);
                if (tacc.IsMono()) {
                    return tacc.IsBuiltinsString();
                }
                break;
            }
            default:
                break;
        }
    }
    return false;
}

BinOpTypeInfoAccessor::BinOpTypeInfoAccessor(const JSThread *thread, Circuit *circuit,
                                             GateRef gate, bool convertNumberType)
    : TypeInfoAccessor(thread, circuit, gate), convertNumberType_(convertNumberType)
{
    left_ = acc_.GetValueIn(gate, 0);  // 0: left
    right_ = acc_.GetValueIn(gate, 1);  // 1: right
}

bool BinOpTypeInfoAccessor::HasNumberType() const
{
    GateType leftType = acc_.GetGateType(left_);
    GateType rightType = acc_.GetGateType(right_);

    const PGOSampleType *sampleType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
    if (sampleType->IsNumber()) {
        return true;
    } else if (convertNumberType_ && sampleType->IsNone() && leftType.IsPrimitiveNumberType() &&
               rightType.IsPrimitiveNumberType()) {
        return true;
    } else if (!convertNumberType_ && sampleType->IsNone() && leftType.IsNumberType() && rightType.IsNumberType()) {
        return true;
    } else {
        return false;
    }
    return false;
}

bool BinOpTypeInfoAccessor::HasStringType() const
{
    GateType leftType = acc_.GetGateType(left_);
    GateType rightType = acc_.GetGateType(right_);
    const PGOSampleType *sampleType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
    if (sampleType->IsString()) {
        return true;
    } else if (sampleType->IsNone() && leftType.IsStringType() && rightType.IsStringType()) {
        return true;
    }
    return false;
}

UnOpTypeInfoAccessor::UnOpTypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : TypeInfoAccessor(thread, circuit, gate)
{
    value_ = acc_.GetValueIn(gate, 0); // 0: value
}

bool UnOpTypeInfoAccessor::ValueIsNumberType() const
{
    const PGOSampleType *sampleType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
    if (sampleType->IsNumber() ||
        (sampleType->IsNone() && ValueIsPrimitiveNumberType())) {
        return true;
    }
    return false;
}

bool NewObjRangeTypeInfoAccessor::FindHClass()
{
    auto sampleType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
    if (!sampleType->IsProfileType()) {
        return false;
    }
    auto type = std::make_pair(sampleType->GetProfileType(), sampleType->GetProfileType());
    hclassIndex_ = static_cast<int>(ptManager_->GetHClassIndexByProfileType(type));
    if (hclassIndex_ == -1) {
        return false;
    }
    return ptManager_->QueryHClass(type.first, type.second).IsJSHClass();
}

bool TypeOfTypeInfoAccessor::IsIllegalType() const
{
    GateType valueType = GetValueGateType();
    if (!valueType.IsDigitablePrimitiveType() && !valueType.IsStringType() && !valueType.IsSymbolType()) {
        if (!tsManager_->IsFunctionTypeKind(valueType) && !tsManager_->IsObjectTypeKind(valueType) &&
            !tsManager_->IsClassTypeKind(valueType) && !tsManager_->IsClassInstanceTypeKind(valueType) &&
            !tsManager_->IsArrayTypeKind(valueType)) {
            return true;
        }
    }
    return false;
}

SuperCallTypeInfoAccessor::SuperCallTypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : TypeInfoAccessor(thread, circuit, gate)
{
    ctor_ = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::FUNC);
}

BuiltinsStubCSigns::ID CallTypeInfoAccessor::TryGetPGOBuiltinId() const
{
    PGOTypeRef sampleType = acc_.TryGetPGOType(gate_);
    if (sampleType.GetPGOSampleType()->IsNone()) {
        return BuiltinsStubCSigns::ID::NONE;
    }
    if (sampleType.GetPGOSampleType()->GetProfileType().IsBuiltinFunctionId()) {
        return static_cast<BuiltinsStubCSigns::ID>(sampleType.GetPGOSampleType()->GetProfileType().GetId());
    }
    return BuiltinsStubCSigns::ID::NONE;
}

BuiltinsStubCSigns::ID CallTypeInfoAccessor::TryGetBuiltinId(BuiltinTypeId id) const
{
    GateType funcType = acc_.GetGateType(func_);
    if (!tsManager_->IsBuiltinObjectMethod(id, funcType)) {
        return BuiltinsStubCSigns::ID::NONE;
    }
    std::string name = tsManager_->GetFuncName(funcType);
    BuiltinsStubCSigns::ID stubId = BuiltinsStubCSigns::GetBuiltinId(name);
    return stubId;
}

GetIteratorTypeInfoAccessor::GetIteratorTypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : CallTypeInfoAccessor(thread, circuit, gate)
{
    argc_ = 0; // 0: number of argc
    func_ = acc_.GetValueIn(gate, 0); // 1: func
}

CallArg0TypeInfoAccessor::CallArg0TypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : CallTypeInfoAccessor(thread, circuit, gate)
{
    argc_ = 0; // 0: number of argc
    func_ = acc_.GetValueIn(gate, 0); // 0: func
}

CallArg1TypeInfoAccessor::CallArg1TypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : CallTypeInfoAccessor(thread, circuit, gate)
{
    argc_ = 1; // 1: number of argc
    value_ = acc_.GetValueIn(gate, 0); // 0: value
    func_ = acc_.GetValueIn(gate, 1); // 1: func
}

CallArg2TypeInfoAccessor::CallArg2TypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : CallTypeInfoAccessor(thread, circuit, gate)
{
    argc_ = 2; // 2: number of argc
    func_ = acc_.GetValueIn(gate, 2); // 2: func
}

CallArg3TypeInfoAccessor::CallArg3TypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : CallTypeInfoAccessor(thread, circuit, gate)
{
    argc_ = 3; // 3: number of argc
    func_ = acc_.GetValueIn(gate, 3); // 3: func
}

CallRangeTypeInfoAccessor::CallRangeTypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : CallTypeInfoAccessor(thread, circuit, gate)
{
    size_t numArgs = acc_.GetNumValueIn(gate);
    constexpr size_t callTargetIndex = 1; // acc
    argc_ = numArgs - callTargetIndex;
    func_ = acc_.GetValueIn(gate, argc_);
}

bool CallThisTypeInfoAccessor::CanOptimizeAsFastCall()
{
    GateType funcType = acc_.GetGateType(func_);
    if (!tsManager_->IsFunctionTypeKind(funcType)) {
        return false;
    }
    auto op = acc_.GetOpCode(func_);
    if (op != OpCode::LOAD_PROPERTY || !acc_.IsVtable(func_)) {
        return false;
    }
    return true;
}

CallThis0TypeInfoAccessor::CallThis0TypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : CallThisTypeInfoAccessor(thread, circuit, gate)
{
    argc_ = 0; // 0: number of argc
    func_ = acc_.GetValueIn(gate, 1); // 1: func
}

CallThis1TypeInfoAccessor::CallThis1TypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : CallThisTypeInfoAccessor(thread, circuit, gate)
{
    argc_ = 1; // 1: number of argc
    func_ = acc_.GetValueIn(gate, 2); // 2: func
    a0_ = acc_.GetValueIn(gate, 1); // 1: arg0
}

CallThis2TypeInfoAccessor::CallThis2TypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : CallThisTypeInfoAccessor(thread, circuit, gate)
{
    argc_ = 2; // 2: number of argc
    func_ = acc_.GetValueIn(gate, 3); // 3: func
}

CallThis3TypeInfoAccessor::CallThis3TypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : CallThisTypeInfoAccessor(thread, circuit, gate)
{
    argc_ = 3; // 3: number of argc
    func_ = acc_.GetValueIn(gate, 3); // 3: func
    a0_ = acc_.GetValueIn(gate, 1); // 1: arg0
    a1_ = acc_.GetValueIn(gate, 2); // 2: arg1
    a2_ = acc_.GetValueIn(gate, 3); // 3: arg2
}

CallThisRangeTypeInfoAccessor::CallThisRangeTypeInfoAccessor(const JSThread *thread, Circuit *circuit, GateRef gate)
    : CallThisTypeInfoAccessor(thread, circuit, gate)
{
    constexpr size_t fixedInputsNum = 1;
    constexpr size_t callTargetIndex = 1;  // 1: acc
    ASSERT(acc_.GetNumValueIn(gate) - fixedInputsNum >= 0);
    size_t numIns = acc_.GetNumValueIn(gate);
    argc_ = numIns - callTargetIndex - fixedInputsNum;
    func_ = acc_.GetValueIn(gate, numIns - callTargetIndex); // acc
}

InlineTypeInfoAccessor::InlineTypeInfoAccessor(
    const JSThread *thread, Circuit *circuit, GateRef gate, GateRef receiver, CallKind kind)
    : TypeInfoAccessor(thread, circuit, gate), receiver_(receiver), kind_(kind)
{
    if (IsCallAccessor()) {
        plr_ = GetAccessorPlr();
    }
}

GlobalTSTypeRef InlineTypeInfoAccessor::GetAccessorFuncGT() const
{
    GateType receiverType = acc_.GetGateType(receiver_);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    GlobalTSTypeRef classInstanceGT = receiverType.GetGTRef();
    GlobalTSTypeRef classGT = tsManager_->GetClassType(classInstanceGT);
    TSTypeAccessor tsTypeAcc(tsManager_, classGT);
    GateRef constData = acc_.GetValueIn(gate_, 1);
    uint16_t propIndex = acc_.GetConstantValue(constData);
    auto methodOffset = acc_.TryGetMethodOffset(gate_);
    auto prop = tsManager_->GetStringFromConstantPool(methodOffset, propIndex);
    GlobalTSTypeRef funcGT = tsTypeAcc.GetAccessorGT(prop, IsCallSetter());
    return funcGT;
}

PropertyLookupResult InlineTypeInfoAccessor::GetAccessorPlr() const
{
    GateRef constData = acc_.GetValueIn(gate_, 1);
    uint16_t propIndex = acc_.GetConstantValue(constData);
    auto methodOffset = acc_.TryGetMethodOffset(gate_);
    auto prop = tsManager_->GetStringFromConstantPool(methodOffset, propIndex);
    // PGO currently does not support call, so GT is still used to support inline operations.
    // However, the original GT solution cannot support accessing the property of prototype, so it is filtered here
    if (EcmaStringAccessor(prop).ToStdString() == "prototype") {
        return PropertyLookupResult();
    }

    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate_).GetPGORWOpType();
    if (pgoTypes->GetCount() != 1) {
        return PropertyLookupResult();
    }
    auto pgoType = pgoTypes->GetObjectInfo(0);
    ProfileTyper receiverType = std::make_pair(pgoType.GetReceiverRootType(), pgoType.GetReceiverType());
    ProfileTyper holderType = std::make_pair(pgoType.GetHoldRootType(), pgoType.GetHoldType());

    PGOTypeManager *ptManager = thread_->GetCurrentEcmaContext()->GetPTManager();
    JSHClass *hclass = nullptr;
    if (receiverType == holderType) {
        int hclassIndex = static_cast<int>(ptManager->GetHClassIndexByProfileType(receiverType));
        if (hclassIndex == -1) {
            return PropertyLookupResult();
        }
        hclass = JSHClass::Cast(ptManager->QueryHClass(receiverType.first, receiverType.second).GetTaggedObject());
    } else {
        int hclassIndex = static_cast<int>(ptManager->GetHClassIndexByProfileType(holderType));
        if (hclassIndex == -1) {
            return PropertyLookupResult();
        }
        hclass = JSHClass::Cast(ptManager->QueryHClass(holderType.first, holderType.second).GetTaggedObject());
    }

    PropertyLookupResult plr = JSHClass::LookupPropertyInPGOHClass(thread_, hclass, prop);
    return plr;
}

uint32_t InlineTypeInfoAccessor::GetCallMethodId() const
{
    uint32_t methodOffset = 0;
    if (IsNormalCall() && IsValidCallMethodId()) {
        methodOffset = GetFuncMethodOffsetFromPGO();
        if (methodOffset == base::PGO_POLY_INLINE_REP) {
            methodOffset = 0;
            return methodOffset;
        }
    }
    if (methodOffset == 0) {
        if (IsFunctionTypeKind()) {
            auto funcGT = GetReceiverGT().GetGTRef();
            methodOffset = tsManager_->GetFuncMethodOffset(funcGT);
        } else if (IsClassInstanceTypeKind()) {
            auto funcGT = GetAccessorFuncGT();
            methodOffset = tsManager_->GetFuncMethodOffset(funcGT);
        }
    }
    return methodOffset;
}

JSTaggedValue ObjectAccessTypeInfoAccessor::GetKeyTaggedValue() const
{
    uint16_t index = acc_.GetConstantValue(key_);
    auto methodOffset = acc_.TryGetMethodOffset(GetGate());
    return TypeInfoAccessor::GetStringFromConstantPool(thread_, methodOffset, index);
}

bool ObjAccByNameTypeInfoAccessor::GeneratePlr(ProfileTyper type, ObjectAccessInfo &info, JSTaggedValue key) const
{
    int hclassIndex = static_cast<int>(ptManager_->GetHClassIndexByProfileType(type));
    if (hclassIndex == -1) {
        return false;
    }
    JSHClass *hclass = JSHClass::Cast(ptManager_->QueryHClass(type.first, type.second).GetTaggedObject());
    PropertyLookupResult plr = JSHClass::LookupPropertyInPGOHClass(thread_, hclass, key);
    info.Set(hclassIndex, plr);

    if (mode_ == AccessMode::LOAD) {
        return plr.IsFound();
    }

    return (plr.IsFound() && !plr.IsFunction());
}

LoadObjByNameTypeInfoAccessor::LoadObjByNameTypeInfoAccessor(const JSThread *thread, Circuit *circuit,
                                                             GateRef gate, Chunk *chunk)
    : ObjAccByNameTypeInfoAccessor(thread, circuit, gate, chunk, AccessMode::LOAD), types_(chunk_)
{
    key_ = acc_.GetValueIn(gate, 1); // 1: key
    receiver_ = acc_.GetValueIn(gate, 2); // 2: receiver
    FetchPGORWTypesDual();
    hasIllegalType_ = !GenerateObjectAccessInfo();
}

void LoadObjByNameTypeInfoAccessor::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate_).GetPGORWOpType();
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsBuiltinsType()) {
            continue;
        }
        types_.emplace_back(std::make_pair(
            std::make_pair(temp.GetReceiverRootType(), temp.GetReceiverType()),
            std::make_pair(temp.GetHoldRootType(), temp.GetHoldType())
        ));
    }
}

bool LoadObjByNameTypeInfoAccessor::GenerateObjectAccessInfo()
{
    JSTaggedValue key = GetKeyTaggedValue();
    for (size_t i = 0; i < types_.size(); ++i) {
        ProfileTyper receiverType = types_[i].first;
        ProfileTyper holderType = types_[i].second;
        if (receiverType == holderType) {
            ObjectAccessInfo info;
            if (!GeneratePlr(receiverType, info, key)) {
                return false;
            }
            accessInfos_.emplace_back(info);
            checkerInfos_.emplace_back(info);
        } else {
            ObjectAccessInfo accInfo;
            if (!GeneratePlr(holderType, accInfo, key)) {
                return false;
            }
            accessInfos_.emplace_back(accInfo);
            ObjectAccessInfo checkInfo;
            GeneratePlr(receiverType, checkInfo, key);
            if (checkInfo.HClassIndex() == -1) {
                return false;
            }
            checkerInfos_.emplace_back(checkInfo);
        }
    }
    return true;
}

StoreObjByNameTypeInfoAccessor::StoreObjByNameTypeInfoAccessor(const JSThread *thread, Circuit *circuit,
                                                               GateRef gate, Chunk *chunk)
    : ObjAccByNameTypeInfoAccessor(thread, circuit, gate, chunk, AccessMode::STORE), types_(chunk_)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::DEFINEFIELDBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8: {
            key_ = acc_.GetValueIn(gate, 1); // 1: key
            receiver_ = acc_.GetValueIn(gate, 2); // 2: receiver
            value_ = acc_.GetValueIn(gate, 3); // 3: value
            break;
        }
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16: {
            key_ = acc_.GetValueIn(gate, 1); // 1: key
            receiver_ = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
            value_ = acc_.GetValueIn(gate, 2); // 2: value
            break;
        }
        case EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8: {
            key_ = acc_.GetValueIn(gate, 0); // 0: key
            receiver_ = acc_.GetValueIn(gate, 1); // 1: receiver
            value_ = acc_.GetValueIn(gate, 2); // 2: value
            break;
        }
        default:
            UNREACHABLE();
    }

    FetchPGORWTypesDual();
    hasIllegalType_ = !GenerateObjectAccessInfo();
}

void StoreObjByNameTypeInfoAccessor::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate_).GetPGORWOpType();
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        types_.emplace_back(std::make_tuple(
            std::make_pair(temp.GetReceiverRootType(), temp.GetReceiverType()),
            std::make_pair(temp.GetHoldRootType(), temp.GetHoldType()),
            std::make_pair(temp.GetHoldTraRootType(), temp.GetHoldTraType())
        ));
    }
}

bool StoreObjByNameTypeInfoAccessor::GenerateObjectAccessInfo()
{
    JSTaggedValue key = GetKeyTaggedValue();
    for (size_t i = 0; i < types_.size(); ++i) {
        ProfileTyper receiverType = std::get<0>(types_[i]);
        ProfileTyper holderType = std::get<1>(types_[i]);
        ProfileTyper newHolderType = std::get<2>(types_[i]);

        if (receiverType != newHolderType) {
            // transition happened ==> slowpath
            ObjectAccessInfo newHolderInfo;
            if (!GeneratePlr(newHolderType, newHolderInfo, key)) {
                return false;
            }
            accessInfos_.emplace_back(newHolderInfo);

            ObjectAccessInfo receiverInfo;
            GeneratePlr(receiverType, receiverInfo, key);
            if (receiverInfo.HClassIndex() == -1) {
                return false;
            }
            checkerInfos_.emplace_back(receiverInfo);
        } else if (receiverType == holderType) {
            ObjectAccessInfo receiverInfo;
            if (!GeneratePlr(receiverType, receiverInfo, key)) {
                return false;
            }
            accessInfos_.emplace_back(receiverInfo);
            checkerInfos_.emplace_back(receiverInfo);
        } else {
            UNREACHABLE();
        }
    }
    return true;
}

InstanceOfTypeInfoAccessor::InstanceOfTypeInfoAccessor(const JSThread *thread, Circuit *circuit,
                                                       GateRef gate, Chunk *chunk)
    : ObjAccByNameTypeInfoAccessor(thread, circuit, gate, chunk, AccessMode::LOAD), types_(chunk_)
{
    receiver_ = acc_.GetValueIn(gate, 1); // 2: receiver
    target_ = acc_.GetValueIn(gate, 2);  // 2: the third parameter

    FetchPGORWTypesDual();
    hasIllegalType_ = !GenerateObjectAccessInfo();
}

JSTaggedValue InstanceOfTypeInfoAccessor::GetKeyTaggedValue() const
{
    JSHandle<GlobalEnv> globalEnv = thread_->GetEcmaVM()->GetGlobalEnv();
    auto hasInstanceEnvIndex = static_cast<size_t>(GlobalEnvField::HASINSTANCE_SYMBOL_INDEX);
    return globalEnv->GetGlobalEnvObjectByIndex(hasInstanceEnvIndex).GetTaggedValue();
}

void InstanceOfTypeInfoAccessor::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate_).GetPGORWOpType();
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsBuiltinsType()) {
            continue;
        }
        types_.emplace_back(std::make_pair(
            std::make_pair(temp.GetReceiverRootType(), temp.GetReceiverType()),
            std::make_pair(temp.GetHoldRootType(), temp.GetHoldType())
        ));
    }
}

bool InstanceOfTypeInfoAccessor::ClassInstanceIsCallable(ProfileTyper type) const
{
    int hclassIndex = static_cast<int>(ptManager_->GetHClassIndexByProfileType(type));
    if (hclassIndex == -1) {
        return false;
    }

    JSHClass *hclass = JSHClass::Cast(ptManager_->QueryHClass(type.first, type.second).GetTaggedObject());

    return hclass->IsCallable();
}

bool InstanceOfTypeInfoAccessor::GenerateObjectAccessInfo()
{
    // Instanceof does not support Poly for now
    if (!IsMono()) {
        return false;
    }
    JSTaggedValue key = GetKeyTaggedValue();
    for (size_t i = 0; i < types_.size(); ++i) {
        ProfileTyper targetPgoType = types_[i].first;
        ObjectAccessInfo targetInfo;
        // If @@hasInstance is found on prototype Chain of ctor -> slowpath
        // Future work: pgo support Symbol && Dump Object.defineProperty && FunctionPrototypeHclass
        // Current temporary solution:
        // Try searching @@hasInstance in pgo dump stage,
        // If found, pgo dump no types and we go slowpath in aot
        GeneratePlr(targetPgoType, targetInfo, key);
        if (targetInfo.HClassIndex() == -1 || !ClassInstanceIsCallable(targetPgoType)) {
            return false;
        }
        accessInfos_.emplace_back(targetInfo);
        checkerInfos_.emplace_back(targetInfo);
    }
    return true;
}

LoadBulitinObjTypeInfoAccessor::LoadBulitinObjTypeInfoAccessor(const JSThread *thread, Circuit *circuit,
                                                               GateRef gate, Chunk *chunk)
    : AccBuiltinObjTypeInfoAccessor(thread, circuit, gate, chunk, AccessMode::LOAD)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8: {
            receiver_ = acc_.GetValueIn(gate, 1); // 1: receiver
            key_ = acc_.GetValueIn(gate, 2);  // 2: key
            break;
        }
        case EcmaOpcode::LDTHISBYVALUE_IMM8:
        case EcmaOpcode::LDTHISBYVALUE_IMM16: {
            receiver_ = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
            key_ = acc_.GetValueIn(gate, 1); // 1: key
            break;
        }
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16: {
            key_ = acc_.GetValueIn(gate, 1); // 1: key
            receiver_ = acc_.GetValueIn(gate, 2); // 2: receiver
            break;
        }
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16: {
            key_ = acc_.GetValueIn(gate, 1); // 1: key
            receiver_ = argAcc_.GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
            break;
        }
        case EcmaOpcode::LDOBJBYINDEX_IMM8_IMM16:
        case EcmaOpcode::LDOBJBYINDEX_IMM16_IMM16:
        case EcmaOpcode::WIDE_LDOBJBYINDEX_PREF_IMM32: {
            key_ = acc_.GetValueIn(gate, 0);
            receiver_ = acc_.GetValueIn(gate, 1);
            break;
        }
        default:
            UNREACHABLE();
    }
    FetchBuiltinsTypes();
}

bool AccBuiltinObjTypeInfoAccessor::IsAllString() const
{
    if (types_.empty()) {
        return false;
    }
    for (auto type : types_) {
        if (!type.IsBuiltinsString()) {
            return false;
        }
    }
    return true;
}

void AccBuiltinObjTypeInfoAccessor::FetchBuiltinsTypes()
{
    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate_).GetPGORWOpType();
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsBuiltinsType()) {
            if (CheckDuplicatedBuiltinType(temp.GetReceiverType())) {
                continue;
            }
            types_.emplace_back(temp.GetReceiverType());
        }
    }
}

bool AccBuiltinObjTypeInfoAccessor::CheckDuplicatedBuiltinType(ProfileType newType) const
{
    for (auto &type : types_) {
        if (type.GetBuiltinsId() == newType.GetBuiltinsId()) {
            return true;
        }
    }
    return false;
}

StoreBulitinObjTypeInfoAccessor::StoreBulitinObjTypeInfoAccessor(const JSThread *thread, Circuit *circuit,
                                                                 GateRef gate, Chunk *chunk)
    : AccBuiltinObjTypeInfoAccessor(thread, circuit, gate, chunk, AccessMode::STORE)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::STOBJBYINDEX_IMM8_V8_IMM16:
        case EcmaOpcode::STOBJBYINDEX_IMM16_V8_IMM16:
        case EcmaOpcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32: {
            key_ = acc_.GetValueIn(gate, 0); // 0: key
            receiver_ = acc_.GetValueIn(gate, 1);  // 1: receiver
            value_ = acc_.GetValueIn(gate, 2); // 2: value
            break;
        }
        case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8: {
            receiver_ = acc_.GetValueIn(gate, 1);  // 1: receiver
            key_ = acc_.GetValueIn(gate, 2);   // 2: key
            value_ = acc_.GetValueIn(gate, 3);     // 3: value
            break;
        }
        default:
            UNREACHABLE();
    }
    FetchBuiltinsTypes();
}

CreateObjWithBufferTypeInfoAccessor::CreateObjWithBufferTypeInfoAccessor(const JSThread *thread, Circuit *circuit,
                                                                         GateRef gate, const CString &recordName)
    : TypeInfoAccessor(thread, circuit, gate), recordName_(recordName), objHandle_(thread, JSTaggedValue::Undefined())
{
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: number of value ins
    index_ = acc_.GetValueIn(gate, 0);
    Init();
}

void CreateObjWithBufferTypeInfoAccessor::Init()
{
    auto imm = acc_.GetConstantValue(index_);
    auto methodOffset = acc_.TryGetMethodOffset(GetGate());
    JSTaggedValue cp = tsManager_->GetConstantPool(methodOffset);
    JSTaggedValue unsharedCp = thread_->GetCurrentEcmaContext()->FindUnsharedConstpool(cp);
    JSTaggedValue obj = ConstantPool::GetLiteralFromCache<ConstPoolType::OBJECT_LITERAL>(
        tsManager_->GetEcmaVM()->GetJSThread(), unsharedCp, imm, recordName_);
    objHandle_ = JSHandle<JSObject>(thread_, obj);
}

JSTaggedValue CreateObjWithBufferTypeInfoAccessor::GetHClass() const
{
    auto sampleType = acc_.TryGetPGOType(gate_).GetPGODefineOpType();
    auto type = std::make_pair(sampleType->GetProfileType(), sampleType->GetProfileType());
    int hclassIndex = static_cast<int>(ptManager_->GetHClassIndexByProfileType(type));

    JSHClass *oldClass = objHandle_->GetClass();
    if (hclassIndex == -1) {
        if (objHandle_->ElementsAndPropertiesIsEmpty()) {
            return JSTaggedValue(oldClass);
        }
        return JSTaggedValue::Undefined();
    }
    JSHClass *newClass = JSHClass::Cast(ptManager_->QueryHClass(type.first, type.second).GetTaggedObject());
    if (oldClass->GetInlinedProperties() != newClass->GetInlinedProperties()) {
        return JSTaggedValue::Undefined();
    }
    return JSTaggedValue(newClass);
}
}  // namespace panda::ecmascript::kungfu
