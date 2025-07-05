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

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/global_env.h"

namespace panda::ecmascript::kungfu {
ParamType TypeInfoAccessor::PGOSampleTypeToParamType() const
{
    if (pgoType_.IsPGOSampleType()) {
        auto sample = pgoType_.GetPGOSampleType();
        if (sample->IsInt()) {
            return ParamType::IntType();
        } else if (sample->IsIntOverFlow()) {
            return ParamType::IntOverflowType();
        } else if (sample->IsDouble()) {
            return ParamType::DoubleType();
        } else if (sample->IsString()) {
            return ParamType::StringType();
        } else if (sample->IsInternString()) {
            return ParamType::InternStringType();
        } else if (sample->IsBigInt()) {
            return ParamType::BigIntType();
        } else if (sample->IsBoolean()) {
            return ParamType::BooleanType();
        } else if (sample->IsNumber()) {
            return ParamType::NumberType();
        } else if (sample->IsNumberOrString()) {
            return ParamType::StringType();
        }
    }
    return ParamType::AnyType();
}

ParamType TypeInfoAccessor::PGOBuiltinTypeToParamType(ProfileType pgoType)
{
    if (pgoType.IsBuiltinsType()) {
        return ParamType(static_cast<uint32_t>(pgoType.GetBuiltinsType()), true);
    }
    return ParamType::AnyType();
}

// TypeTrusted means the type of gate is already PrimitiveTypeCheck-passed,
// or the gate is constant and no need to check.
bool TypeInfoAccessor::IsTrustedBooleanType(GateAccessor acc, GateRef gate)
{
    if (acc.IsConstant(gate) && acc.GetGateType(gate).IsBooleanType()) {
        return true;
    }
    auto op = acc.GetOpCode(gate);
    if (op == OpCode::TYPED_BINARY_OP) {
        return TypedBinaryAccessor(acc.TryGetValue(gate)).IsTrustedBooleanType();
    }
    if (op == OpCode::TYPED_UNARY_OP) {
        return TypedUnaryAccessor(acc.TryGetValue(gate)).IsTrustedBooleanType();
    }
    if (op == OpCode::JS_BYTECODE) {
        EcmaOpcode ecmaOpcode = acc.GetByteCodeOpcode(gate);
        switch (ecmaOpcode) {
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
            case EcmaOpcode::CALLRUNTIME_ISFALSE_PREF_IMM8:
            case EcmaOpcode::CALLRUNTIME_ISTRUE_PREF_IMM8:
                return true;
            default:
                break;
        }
    }
    return false;
}

bool TypeInfoAccessor::IsTrustedNumberType(GateAccessor acc, GateRef gate)
{
    if (acc.IsConstant(gate) && acc.GetGateType(gate).IsNumberType()) {
        return true;
    }
    auto op = acc.GetOpCode(gate);
    if (op == OpCode::TYPED_BINARY_OP) {
        return TypedBinaryAccessor(acc.TryGetValue(gate)).IsTrustedNumberType();
    }
    if (op == OpCode::TYPED_UNARY_OP) {
        return TypedUnaryAccessor(acc.TryGetValue(gate)).IsTrustedNumberType();
    }
    if (op == OpCode::TYPE_CONVERT) {
        // typeconvert only use in tomumeric
        return true;
    }
    if (op == OpCode::LOAD_ELEMENT) {
        auto loadOp = acc.GetTypedLoadOp(gate);
        switch (loadOp) {
            case TypedLoadOp::INT8ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::UINT8ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::UINT8CLAMPEDARRAY_LOAD_ELEMENT:
            case TypedLoadOp::INT16ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::UINT16ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::INT32ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::UINT32ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::FLOAT64ARRAY_LOAD_ELEMENT:
                return true;
            default:
                return false;
        }
    }

    if (op == OpCode::JS_BYTECODE) {
        EcmaOpcode ecmaOpcode = acc.GetByteCodeOpcode(gate);
        switch (ecmaOpcode) {
            case EcmaOpcode::ADD2_IMM8_V8:
            case EcmaOpcode::SUB2_IMM8_V8:
            case EcmaOpcode::MUL2_IMM8_V8:
            case EcmaOpcode::DIV2_IMM8_V8:
            case EcmaOpcode::MOD2_IMM8_V8:
            case EcmaOpcode::SHL2_IMM8_V8:
            case EcmaOpcode::SHR2_IMM8_V8:
            case EcmaOpcode::ASHR2_IMM8_V8:
            case EcmaOpcode::AND2_IMM8_V8:
            case EcmaOpcode::OR2_IMM8_V8:
            case EcmaOpcode::XOR2_IMM8_V8:
            case EcmaOpcode::INC_IMM8:
            case EcmaOpcode::DEC_IMM8:
            case EcmaOpcode::NEG_IMM8:
            case EcmaOpcode::NOT_IMM8:
                return acc.TryGetPGOType(gate).GetPGOSampleType()->HasNumber();
            default:
                break;
        }
    }
    return false;
}

bool TypeInfoAccessor::IsTrustedStringType(
    const CompilationEnv *env, Circuit *circuit, Chunk *chunk, GateAccessor acc, GateRef gate)
{
    auto op = acc.GetOpCode(gate);
    if (op == OpCode::LOAD_ELEMENT) {
        return acc.GetTypedLoadOp(gate) == TypedLoadOp::STRING_LOAD_ELEMENT;
    }
    if (op == OpCode::NUMBER_TO_STRING) {
        return true;
    }
    if (op == OpCode::TYPED_BINARY_OP) {
        return TypedBinaryAccessor(acc.TryGetValue(gate)).IsTrustedStringType();
    }

    if (op == OpCode::JS_BYTECODE) {
        EcmaOpcode ecmaOpcode = acc.GetByteCodeOpcode(gate);
        switch (ecmaOpcode) {
            case EcmaOpcode::LDA_STR_ID16:
                return true;
            case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
            case EcmaOpcode::LDOBJBYVALUE_IMM16_V8: {
                LoadBuiltinObjTypeInfoAccessor tacc(env, circuit, gate, chunk);
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

bool TypeInfoAccessor::IsTrustedNotSameType(const CompilationEnv *env, Circuit *circuit, Chunk *chunk,
                                            GateAccessor acc, GateRef left, GateRef right)
{
    bool leftIsBoolean = IsTrustedBooleanType(acc, left);
    bool rightIsBoolean = IsTrustedBooleanType(acc, right);
    bool leftIsNumber = IsTrustedNumberType(acc, left);
    bool rightIsNumber = IsTrustedNumberType(acc, right);
    bool leftIsString = IsTrustedStringType(env, circuit, chunk, acc, left);
    bool rightIsString = IsTrustedStringType(env, circuit, chunk, acc, right);
    if (leftIsBoolean) {
        return rightIsNumber || rightIsString;
    }

    if (leftIsNumber) {
        return rightIsBoolean || rightIsString;
    }

    if (leftIsString) {
        return rightIsBoolean || rightIsNumber;
    }

    return false;
}

BuiltinsStubCSigns::ID TypeInfoAccessor::TryGetPGOBuiltinMethodId() const
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

bool NewObjRangeTypeInfoAccessor::AotAccessorStrategy::FindHClass() const
{
    auto sampleType = parent_.pgoType_.GetPGOSampleType();
    if (!sampleType->IsProfileType()) {
        return false;
    }
    ProfileType profileType = sampleType->GetProfileType();
    auto type = std::make_pair(profileType, profileType);
    parent_.hclassIndex_ = static_cast<int>(parent_.ptManager_->GetHClassIndexByProfileType(type));
    if (parent_.hclassIndex_ == -1) {
        return false;
    }
    profileType.UpdateKind(pgo::ProfileType::Kind::TransitionClassId);
    auto transType = std::make_pair(profileType, profileType);
    parent_.traHClassIndex_ = static_cast<int>(parent_.ptManager_->GetHClassIndexByProfileType(transType));
    if (parent_.traHClassIndex_ == -1) {
        return parent_.ptManager_->QueryHClass(type.first, type.second).IsJSHClass();
    }
    return parent_.ptManager_->QueryHClass(transType.first, transType.second).IsJSHClass();
}

bool NewObjRangeTypeInfoAccessor::JitAccessorStrategy::FindHClass() const
{
    // For JIT, it handles Builtin using SampleType. For handling others, it uses DefType.
    // Therefore, when it's SampleType, it can directly return.
    if (parent_.pgoType_.IsPGOSampleType()) {
        return false;
    }
    auto sampleType = parent_.pgoType_.GetPGODefineOpType();
    if (sampleType->IsNone()) {
        return false;
    }
    JSHClass *hclass = sampleType->GetReceiver();
    bool result = hclass != nullptr;
    if (result) {
        parent_.hclassIndex_ = parent_.ptManager_->RecordAndGetHclassIndexForJIT(hclass);
    }
    return result;
}

JSTaggedValue NewObjRangeTypeInfoAccessor::AotAccessorStrategy::GetHClass() const
{
    auto sampleType = parent_.pgoType_.GetPGOSampleType();
    ASSERT(sampleType->IsProfileType());
    ProfileType profileType = sampleType->GetProfileType();
    auto type = std::make_pair(profileType, profileType);
    parent_.hclassIndex_ = static_cast<int>(parent_.ptManager_->GetHClassIndexByProfileType(type));
    ASSERT(parent_.hclassIndex_ != -1);
    profileType.UpdateKind(pgo::ProfileType::Kind::TransitionClassId);
    auto transType = std::make_pair(profileType, profileType);
    parent_.traHClassIndex_ = static_cast<int>(parent_.ptManager_->GetHClassIndexByProfileType(transType));
    if (parent_.traHClassIndex_ == -1) {
        return parent_.ptManager_->QueryHClass(type.first, type.second);
    }
    return parent_.ptManager_->QueryHClass(transType.first, transType.second);
}

JSTaggedValue NewObjRangeTypeInfoAccessor::JitAccessorStrategy::GetHClass() const
{
    auto sampleType = parent_.pgoType_.GetPGODefineOpType();
    ASSERT(!sampleType->IsNone());
    JSHClass *hclass = sampleType->GetReceiver();
    parent_.hclassIndex_ = parent_.ptManager_->RecordAndGetHclassIndexForJIT(hclass);
    return JSTaggedValue(sampleType->GetReceiver());
}

bool TypeOfTypeInfoAccessor::IsIllegalType() const
{
    return true;
}

SuperCallTypeInfoAccessor::SuperCallTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                     const JSPandaFile *jsPandaFile,
                                                     const CallMethodFlagMap *callMethodFlagMap)
    : TypeInfoAccessor(env, circuit, gate), jsPandaFile_(jsPandaFile), callMethodFlagMap_(callMethodFlagMap)
{
    ctor_ = circuit->GetArgumentAccessor()->GetFrameArgsIn(gate, FrameArgIdx::FUNC);
}

GetIteratorTypeInfoAccessor::GetIteratorTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                         const JSPandaFile *jsPandaFile,
                                                         const CallMethodFlagMap *callMethodFlagMap)
    : CallTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
{
    argc_ = 0; // 0: number of argc
    func_ = acc_.GetValueIn(gate, 0); // 1: func
}

CallArg0TypeInfoAccessor::CallArg0TypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                   const JSPandaFile *jsPandaFile,
                                                   const CallMethodFlagMap *callMethodFlagMap)
    : CallTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
{
    argc_ = 0; // 0: number of argc
    func_ = acc_.GetValueIn(gate, 0); // 0: func
}

CallArg1TypeInfoAccessor::CallArg1TypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                   const JSPandaFile *jsPandaFile,
                                                   const CallMethodFlagMap *callMethodFlagMap)
    : CallTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
{
    argc_ = 1; // 1: number of argc
    value_ = acc_.GetValueIn(gate, 0); // 0: value
    func_ = acc_.GetValueIn(gate, 1); // 1: func
}

CallArg2TypeInfoAccessor::CallArg2TypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                   const JSPandaFile *jsPandaFile,
                                                   const CallMethodFlagMap *callMethodFlagMap)
    : CallTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
{
    argc_ = 2; // 2: number of argc
    func_ = acc_.GetValueIn(gate, 2); // 2: func
}

CallArg3TypeInfoAccessor::CallArg3TypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                   const JSPandaFile *jsPandaFile,
                                                   const CallMethodFlagMap *callMethodFlagMap)
    : CallTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
{
    argc_ = 3; // 3: number of argc
    func_ = acc_.GetValueIn(gate, 3); // 3: func
}

CallRangeTypeInfoAccessor::CallRangeTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                     const JSPandaFile *jsPandaFile,
                                                     const CallMethodFlagMap *callMethodFlagMap)
    : CallTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
{
    size_t numArgs = acc_.GetNumValueIn(gate);
    constexpr size_t callTargetIndex = 1; // acc
    ASSERT(numArgs > 0);
    argc_ = numArgs - callTargetIndex;
    func_ = acc_.GetValueIn(gate, argc_);
}

bool CallThisTypeInfoAccessor::CanOptimizeAsFastCall()
{
    auto profileType = acc_.TryGetPGOType(gate_).GetPGOSampleType();
    auto op = acc_.GetOpCode(func_);
    if (!IsValidCallMethodId()) {
        return false;
    }
    if (!profileType->IsNone()) {
        if (profileType->IsProfileTypeNone() ||
            (op != OpCode::LOAD_PROPERTY && op != OpCode::MONO_LOAD_PROPERTY_ON_PROTO)) {
            return false;
        }
        return true;
    }
    return false;
}

CallThis0TypeInfoAccessor::CallThis0TypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                     const JSPandaFile *jsPandaFile,
                                                     const CallMethodFlagMap *callMethodFlagMap)
    : CallThisTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
{
    argc_ = 0; // 0: number of argc
    func_ = acc_.GetValueIn(gate, 1); // 1: func
}

CallThis1TypeInfoAccessor::CallThis1TypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                     const JSPandaFile *jsPandaFile,
                                                     const CallMethodFlagMap *callMethodFlagMap)
    : CallThisTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
{
    argc_ = 1; // 1: number of argc
    func_ = acc_.GetValueIn(gate, 2); // 2: func
    a0_ = acc_.GetValueIn(gate, 1); // 1: arg0
}

CallThis2TypeInfoAccessor::CallThis2TypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                     const JSPandaFile *jsPandaFile,
                                                     const CallMethodFlagMap *callMethodFlagMap)
    : CallThisTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
{
    argc_ = 2; // 2: number of argc
    func_ = acc_.GetValueIn(gate, 3); // 3: func
    a0_ = acc_.GetValueIn(gate, 1); // 1: arg0
    a1_ = acc_.GetValueIn(gate, 2); // 2: arg1
}

CallThis3TypeInfoAccessor::CallThis3TypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                     const JSPandaFile *jsPandaFile,
                                                     const CallMethodFlagMap *callMethodFlagMap)
    : CallThisTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
{
    argc_ = 3; // 3: number of argc
    func_ = acc_.GetValueIn(gate, 4); // 4: func
    a0_ = acc_.GetValueIn(gate, 1); // 1: arg0
    a1_ = acc_.GetValueIn(gate, 2); // 2: arg1
    a2_ = acc_.GetValueIn(gate, 3); // 3: arg2
}

CallThisRangeTypeInfoAccessor::CallThisRangeTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                             const JSPandaFile *jsPandaFile,
                                                             const CallMethodFlagMap *callMethodFlagMap)
    : CallThisTypeInfoAccessor(env, circuit, gate, jsPandaFile, callMethodFlagMap)
{
    constexpr size_t fixedInputsNum = 1;
    constexpr size_t callTargetIndex = 1;  // 1: acc
    size_t numIns = acc_.GetNumValueIn(gate);
    ASSERT(numIns >= fixedInputsNum + callTargetIndex);
    argc_ = numIns - callTargetIndex - fixedInputsNum;
    func_ = acc_.GetValueIn(gate, numIns - callTargetIndex); // acc
}

InlineTypeInfoAccessor::InlineTypeInfoAccessor(
    const CompilationEnv *env, Circuit *circuit, GateRef gate, GateRef receiver, CallKind kind)
    : TypeInfoAccessor(env, circuit, gate), receiver_(receiver), kind_(kind)
{
    if (IsCallAccessor()) {
        if (IsAot()) {
            plr_ = GetAccessorPlr();
        } else {
            plr_ = GetAccessorPlrInJIT();
        }
    }
    thisObj_ = Circuit::NullGate();
}
bool InlineTypeInfoAccessor::InitPropAndCheck(JSTaggedValue& prop) const
{
    GateRef constData = acc_.GetValueIn(gate_, 1);
    uint16_t propIndex = acc_.GetConstantValue(constData);
    auto methodOffset = acc_.TryGetMethodOffset(gate_);
    prop = compilationEnv_->GetStringFromConstantPool(methodOffset, propIndex);
    if (prop.IsUndefined()) {
        return false;
    }
    // PGO currently does not support call, so GT is still used to support inline operations.
    // However, the original GT solution cannot support accessing the property of prototype, so it is filtered here
    if (EcmaStringAccessor(prop).ToStdString(compilationEnv_->GetJSThread()) == "prototype") {
        return false;
    }
    return true;
}

PropertyLookupResult InlineTypeInfoAccessor::GetAccessorPlr() const
{
    JSTaggedValue prop = JSTaggedValue::Undefined();
    if (!InitPropAndCheck(prop)) {
        return PropertyLookupResult();
    }

    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate_).GetPGORWOpType();
    if (pgoTypes->GetCount() != 1) {
        return PropertyLookupResult();
    }
    auto pgoType = pgoTypes->GetObjectInfo(0);
    ProfileTyper holderType = std::make_pair(pgoType.GetHoldRootType(), pgoType.GetHoldType());

    PGOTypeManager *ptManager = compilationEnv_->GetPTManager();

    int hclassIndex = static_cast<int>(ptManager->GetHClassIndexByProfileType(holderType));
    if (hclassIndex == -1) {
        return PropertyLookupResult();
    }
    JSTaggedValue hclass = ptManager->QueryHClass(holderType.first, holderType.second);
    if (!hclass.IsJSHClass()) {
        return PropertyLookupResult();
    }

    PropertyLookupResult plr = JSHClass::LookupPropertyInPGOHClass(compilationEnv_->GetJSThread(),
        JSHClass::Cast(hclass.GetTaggedObject()), prop);
    return plr;
}


PropertyLookupResult InlineTypeInfoAccessor::GetAccessorPlrInJIT() const
{
    JSTaggedValue prop = JSTaggedValue::Undefined();
    if (!InitPropAndCheck(prop)) {
        return PropertyLookupResult();
    }

    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate_).GetPGORWOpType();
    if (pgoTypes->GetCount() != 1) {
        return PropertyLookupResult();
    }
    if (!pgoTypes->GetObjectInfo(0).IsJITClassType()) {
        return PropertyLookupResult();
    }
    auto pgoType = pgoTypes->GetObjectInfo(0);
    JSHClass* receiverType = pgoType.GetReceiverHclass();
    JSHClass* holderType = pgoType.GetHolderHclass();
    JSHClass *hclass = nullptr;
    if (receiverType == holderType) {
        hclass = receiverType;
    } else {
        hclass = holderType;
    }

    PropertyLookupResult plr = JSHClass::LookupPropertyInPGOHClass(compilationEnv_->GetJSThread(), hclass, prop);
    return plr;
}

uint32_t InlineTypeInfoAccessor::GetCallMethodId() const
{
    uint32_t methodOffset = 0;
    if ((IsNormalCall() || IsSuperCall()) && IsValidCallMethodId()) {
        methodOffset = GetFuncMethodOffsetFromPGO();
    }
    if (IsCallAccessor()) {
        const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate_).GetPGORWOpType();
        auto pgoType = pgoTypes->GetObjectInfo(0);
        if (pgoType.GetAccessorMethod().GetProfileType().IsValidCallMethodId()) {
            return pgoType.GetAccessorMethod().GetProfileType().GetCallMethodId();
        }
    }
    return methodOffset;
}

JSTaggedValue ObjectAccessTypeInfoAccessor::GetKeyTaggedValue() const
{
    uint16_t index = acc_.GetConstantValue(key_);
    auto methodOffset = acc_.TryGetMethodOffset(GetGate());
    return compilationEnv_->GetStringFromConstantPool(methodOffset, index);
}

bool ObjAccByNameTypeInfoAccessor::GeneratePlr(ProfileTyper type, ObjectAccessInfo &info, JSTaggedValue key) const
{
    int hclassIndex = static_cast<int>(ptManager_->GetHClassIndexByProfileType(type));
    if (hclassIndex == -1) {
        return false;
    }
    JSTaggedValue hclass = ptManager_->QueryHClass(type.first, type.second);
    if (!hclass.IsJSHClass()) {
        return false;
    }
    PropertyLookupResult plr = JSHClass::LookupPropertyInPGOHClass(compilationEnv_->GetJSThread(),
        JSHClass::Cast(hclass.GetTaggedObject()), key);
    info.Set(hclassIndex, plr);

    if (mode_ == AccessMode::LOAD) {
        return plr.IsFound();
    }

    return (plr.IsFound() && !plr.IsFunction());
}

bool ObjAccByNameTypeInfoAccessor::GeneratePlrInJIT(JSHClass* hclass, ObjectAccessInfo &info, JSTaggedValue key) const
{
    PropertyLookupResult plr = JSHClass::LookupPropertyInPGOHClass(compilationEnv_->GetJSThread(), hclass, key);
    int hclassIndex = ptManager_->RecordAndGetHclassIndexForJIT(hclass);
    info.Set(hclassIndex, plr);

    if (mode_ == AccessMode::LOAD) {
        // For not found ic
        if (hclass == nullptr) {
            return true;
        }
        return plr.IsFound();
    }

    return (plr.IsFound() && !plr.IsFunction());
}

void StorePrivatePropertyTypeInfoAccessor::AotAccessorStrategy::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGORWOpType();
    if (IsMegaType(pgoTypes)) {
        return;
    }
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        parent_.types_.emplace_back(std::make_tuple(std::make_pair(temp.GetReceiverRootType(), temp.GetReceiverType()),
                                                    std::make_pair(temp.GetHoldRootType(), temp.GetHoldType()),
                                                    std::make_pair(temp.GetHoldTraRootType(), temp.GetHoldTraType())));
    }
}

void StorePrivatePropertyTypeInfoAccessor::JitAccessorStrategy::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGORWOpType();
    if (IsMegaType(pgoTypes)) {
        return;
    }
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsJITClassType()) {
            parent_.jitTypes_.emplace_back(temp);
        }
    }
}

JSTaggedValue StorePrivatePropertyTypeInfoAccessor::GetKeyTaggedValue() const
{
    if (types_.empty()) {
        return JSTaggedValue::Hole();
    }

    ProfileTyper receiverType = std::get<0>(types_.at(0));
    auto slotIndex = acc_.GetConstantValue(slotIndex_);
    ProfileTypeTuple type = std::make_tuple(receiverType.first, receiverType.second, slotIndex);
    auto privateId = ptManager_->GetSymbolIdByProfileType(type);
    if (!privateId) {
        return JSTaggedValue::Hole();
    }

    AOTSnapshot& snapshot = ptManager_->GetAOTSnapshot();
    auto symbolInfo = snapshot.GetSymbolInfo();
    auto symbol = ConstantPool::GetSymbolFromSymbolInfo(compilationEnv_->GetJSThread(), symbolInfo, *privateId);
    return symbol;
}

bool StorePrivatePropertyTypeInfoAccessor::AotAccessorStrategy::GenerateObjectAccessInfo()
{
    JSTaggedValue key = parent_.GetKeyTaggedValue();
    if (key.IsHole()) {
        parent_.isAccessor_ = true;
        return true;
    }

    if (parent_.types_.size() == 0) {
        return false;
    }
    ProfileTyper receiverType = std::get<0>(parent_.types_.at(0));
    ProfileTyper holderType = std::get<1>(parent_.types_.at(0));
    if (receiverType == holderType) {
        ObjectAccessInfo receiverInfo;
        if (!parent_.GeneratePlr(receiverType, receiverInfo, key)) {
            return false;
        }
        parent_.accessInfos_.emplace_back(receiverInfo);
        parent_.checkerInfos_.emplace_back(receiverInfo);
    } else {
        UNREACHABLE();
    }

    return true;
}

bool StorePrivatePropertyTypeInfoAccessor::JitAccessorStrategy::GenerateObjectAccessInfo()
{
    JSTaggedValue key = parent_.GetKeyTaggedValue();
    if (key.IsHole()) {
        parent_.isAccessor_ = true;
        return true;
    }

    if (parent_.jitTypes_.size() == 0) {
        return false;
    }
    JSHClass *receiverType = parent_.jitTypes_[0].GetReceiverHclass();
    JSHClass *holderType = parent_.jitTypes_[0].GetHolderHclass();
    if (receiverType->IsJsPrimitiveRef() || holderType->IsJsPrimitiveRef()) {
        return false;
    }

    if (receiverType == holderType) {
        ObjectAccessInfo receiverInfo;
        if (!parent_.GeneratePlrInJIT(receiverType, receiverInfo, key)) {
            return false;
        }
        parent_.accessInfos_.emplace_back(receiverInfo);
        parent_.checkerInfos_.emplace_back(receiverInfo);
    } else {
        UNREACHABLE();
    }

    return true;
}

void LoadPrivatePropertyTypeInfoAccessor::AotAccessorStrategy::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGORWOpType();
    if (IsMegaType(pgoTypes)) {
        return;
    }
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        parent_.types_.emplace_back(std::make_pair(std::make_pair(temp.GetReceiverRootType(), temp.GetReceiverType()),
                                                   std::make_pair(temp.GetHoldRootType(), temp.GetHoldType())));
    }
}

void LoadPrivatePropertyTypeInfoAccessor::JitAccessorStrategy::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGORWOpType();
    if (IsMegaType(pgoTypes)) {
        return;
    }
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsJITClassType()) {
            parent_.jitTypes_.emplace_back(temp);
        }
    }
}


JSTaggedValue LoadPrivatePropertyTypeInfoAccessor::GetKeyTaggedValue() const
{
    if (types_.empty()) {
        return JSTaggedValue::Hole();
    }

    ProfileTyper receiverType = types_.at(0).first;
    auto slotIndex = acc_.GetConstantValue(slotIndex_);
    ProfileTypeTuple type = std::make_tuple(receiverType.first, receiverType.second, slotIndex);
    auto privateId = ptManager_->GetSymbolIdByProfileType(type);
    if (!privateId) {
        return JSTaggedValue::Hole();
    }

    AOTSnapshot& snapshot = ptManager_->GetAOTSnapshot();
    auto symbolInfo = snapshot.GetSymbolInfo();
    auto symbol = ConstantPool::GetSymbolFromSymbolInfo(compilationEnv_->GetJSThread(), symbolInfo, *privateId);
    return symbol;
}

bool LoadPrivatePropertyTypeInfoAccessor::AotAccessorStrategy::GenerateObjectAccessInfo()
{
    JSTaggedValue key = parent_.GetKeyTaggedValue();
    if (key.IsHole()) {
        parent_.isAccessor_ = true;
        return true;
    }

    if (parent_.types_.size() == 0) {
        return false;
    }
    ProfileTyper receiverType = parent_.types_.at(0).first;
    ProfileTyper holderType = parent_.types_.at(0).second;
    if (receiverType == holderType) {
        ObjectAccessInfo receiverInfo;
        if (!parent_.GeneratePlr(receiverType, receiverInfo, key)) {
            return false;
        }
        parent_.accessInfos_.emplace_back(receiverInfo);
        parent_.checkerInfos_.emplace_back(receiverInfo);
    } else {
        UNREACHABLE();
    }

    return true;
}

bool LoadPrivatePropertyTypeInfoAccessor::JitAccessorStrategy::GenerateObjectAccessInfo()
{
    JSTaggedValue key = parent_.GetKeyTaggedValue();
    if (key.IsHole()) {
        parent_.isAccessor_ = true;
        return true;
    }

    if (parent_.jitTypes_.size() == 0) {
        return false;
    }
    JSHClass *receiver = parent_.jitTypes_[0].GetReceiverHclass();
    JSHClass *holder = parent_.jitTypes_[0].GetHolderHclass();
    // case: r.toFixed() => HeapObjectCheck Deopt
    if (receiver->IsJsPrimitiveRef() || holder->IsJsPrimitiveRef()) {
        return false;
    }

    if (receiver == holder) {
        ObjectAccessInfo receiverInfo;
        if (!parent_.GeneratePlrInJIT(receiver, receiverInfo, key)) {
            return false;
        }
        parent_.accessInfos_.emplace_back(receiverInfo);
        parent_.checkerInfos_.emplace_back(receiverInfo);
    } else {
        UNREACHABLE();
    }

    return true;
}

LoadObjPropertyTypeInfoAccessor::LoadObjPropertyTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit,
                                                                 GateRef gate, Chunk *chunk, bool isByValue)
    : ObjAccByNameTypeInfoAccessor(env, circuit, gate, chunk, AccessMode::LOAD), types_(chunk_), jitTypes_(chunk_),
      isByValue_(isByValue)
{
    if (isByValue_) {
        receiver_ = acc_.GetValueIn(gate, 1); // 1: receiver
        key_ = acc_.GetValueIn(gate, 2);      // 2: key
        ASSERT(!IsAot());
    } else {
        key_ = acc_.GetValueIn(gate, 1);      // 1: key
        receiver_ = acc_.GetValueIn(gate, 2); // 2: receiver
    }
    if (IsAot()) {
        strategy_ = chunk_->New<AotAccessorStrategy>(*this);
    } else {
        strategy_ = chunk_->New<JitAccessorStrategy>(*this);
    }
    strategy_->FetchPGORWTypesDual();
    hasIllegalType_ = !strategy_->GenerateObjectAccessInfo();
}

void LoadObjPropertyTypeInfoAccessor::AotAccessorStrategy::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGORWOpType();
    if (IsMegaType(pgoTypes)) {
        return;
    }
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsBuiltinsType()) {
            continue;
        }
        parent_.types_.emplace_back(std::make_pair(std::make_pair(temp.GetReceiverRootType(), temp.GetReceiverType()),
                                                   std::make_pair(temp.GetHoldRootType(), temp.GetHoldType())));
    }
}

void LoadObjPropertyTypeInfoAccessor::JitAccessorStrategy::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGORWOpType();
    if (IsMegaType(pgoTypes)) {
        return;
    }
    if (parent_.isByValue_) {
        parent_.name_ = pgoTypes->GetName();
        parent_.nameIdx_ = pgoTypes->GetNameIdx();
    }
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsJITClassType()) {
            parent_.jitTypes_.emplace_back(temp);
        }
    }
}

bool LoadObjPropertyTypeInfoAccessor::AotAccessorStrategy::GenerateObjectAccessInfo()
{
    JSTaggedValue key = JSTaggedValue::Undefined();
    if (parent_.isByValue_) {
        key = parent_.name_.GetTaggedValue();
    } else {
        key = parent_.GetKeyTaggedValue();
    }
    if (key.IsUndefined()) {
        return false;
    }
    if (parent_.types_.size() == 0) {
        return false;
    }
    for (size_t i = 0; i < parent_.types_.size(); ++i) {
        ProfileTyper receiverType = parent_.types_[i].first;
        ProfileTyper holderType = parent_.types_[i].second;
        if (receiverType == holderType) {
            ObjectAccessInfo info;
            if (!parent_.GeneratePlr(receiverType, info, key)) {
                return false;
            }
            parent_.accessInfos_.emplace_back(info);
            parent_.checkerInfos_.emplace_back(info);
        } else {
            ObjectAccessInfo accInfo;
            if (!parent_.GeneratePlr(holderType, accInfo, key)) {
                return false;
            }
            parent_.accessInfos_.emplace_back(accInfo);
            ObjectAccessInfo checkInfo;
            parent_.GeneratePlr(receiverType, checkInfo, key);
            if (checkInfo.HClassIndex() == -1) {
                return false;
            }
            parent_.checkerInfos_.emplace_back(checkInfo);
        }
    }
    return true;
}

bool LoadObjPropertyTypeInfoAccessor::JitAccessorStrategy::GenerateObjectAccessInfo()
{
    JSTaggedValue key = JSTaggedValue::Undefined();
    if (parent_.isByValue_) {
        key = parent_.name_.GetTaggedValue();
    } else {
        key = parent_.GetKeyTaggedValue();
    }
    if (key.IsUndefined()) {
        return false;
    }
    if (parent_.jitTypes_.size() == 0) {
        return false;
    }
    for (size_t i = 0; i < parent_.jitTypes_.size(); ++i) {
        JSHClass *receiver = parent_.jitTypes_[i].GetReceiverHclass();
        JSHClass *holder = parent_.jitTypes_[i].GetHolderHclass();
        PrimitiveType primitiveType = parent_.jitTypes_[i].GetPrimitiveType();
        // case: r.toFixed() => HeapObjectCheck Deopt
        ASSERT(receiver != nullptr);
        if ((receiver->IsJsPrimitiveRef() ||
            (holder != nullptr && holder->IsJsPrimitiveRef())) &&
            !parent_.compilationEnv_->SupportHeapConstant()) {
            return false;
        }
        if (receiver == holder) {
            ObjectAccessInfo info;
            if (!parent_.GeneratePlrInJIT(receiver, info, key)) {
                return false;
            }
            info.SetPrimitiveType(primitiveType);
            parent_.accessInfos_.emplace_back(info);
            parent_.checkerInfos_.emplace_back(info);
        } else {
            ObjectAccessInfo accInfo;
            if (!parent_.GeneratePlrInJIT(holder, accInfo, key)) {
                return false;
            }
            accInfo.SetPrimitiveType(primitiveType);
            parent_.accessInfos_.emplace_back(accInfo);
            ObjectAccessInfo checkInfo;
            parent_.GeneratePlrInJIT(receiver, checkInfo, key);
            if (checkInfo.HClassIndex() == -1) {
                return false;
            }
            checkInfo.SetPrimitiveType(primitiveType);
            parent_.checkerInfos_.emplace_back(checkInfo);
        }
    }
    return true;
}

StoreObjByNameTypeInfoAccessor::StoreObjByNameTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit,
                                                               GateRef gate, Chunk *chunk)
    : ObjAccByNameTypeInfoAccessor(env, circuit, gate, chunk, AccessMode::STORE), types_(chunk_), jitTypes_(chunk)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::DEFINEFIELDBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8:
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
            receiver_ = circuit->GetArgumentAccessor()->GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
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

    if (IsAot()) {
        strategy_ = chunk_->New<AotAccessorStrategy>(*this);
    } else {
        strategy_ = chunk_->New<JitAccessorStrategy>(*this);
    }
    strategy_->FetchPGORWTypesDual();
    hasIllegalType_ = !strategy_->GenerateObjectAccessInfo();
}

void StoreObjByNameTypeInfoAccessor::AotAccessorStrategy::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGORWOpType();
    if (IsMegaType(pgoTypes)) {
        return;
    }
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        parent_.types_.emplace_back(std::make_tuple(
            std::make_pair(temp.GetReceiverRootType(), temp.GetReceiverType()),
            std::make_pair(temp.GetHoldRootType(), temp.GetHoldType()),
            std::make_pair(temp.GetHoldTraRootType(), temp.GetHoldTraType())
        ));
    }
}

void StoreObjByNameTypeInfoAccessor::JitAccessorStrategy::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGORWOpType();
    if (IsMegaType(pgoTypes)) {
        return;
    }
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsJITClassType()) {
            parent_.jitTypes_.emplace_back(temp);
        }
    }
}

bool StoreObjByNameTypeInfoAccessor::AotAccessorStrategy::GenerateObjectAccessInfo()
{
    JSTaggedValue key = parent_.GetKeyTaggedValue();
    if (parent_.types_.size() == 0) {
        return false;
    }
    for (size_t i = 0; i < parent_.types_.size(); ++i) {
        ProfileTyper receiverType = std::get<0>(parent_.types_[i]);
        ProfileTyper holderType = std::get<1>(parent_.types_[i]);
        ProfileTyper newHolderType = std::get<2>(parent_.types_[i]);

        if (receiverType != newHolderType) {
            // transition happened ==> slowpath
            ObjectAccessInfo newHolderInfo;
            if (!parent_.GeneratePlr(newHolderType, newHolderInfo, key)) {
                return false;
            }
            parent_.accessInfos_.emplace_back(newHolderInfo);

            ObjectAccessInfo receiverInfo;
            parent_.GeneratePlr(receiverType, receiverInfo, key);
            if (receiverInfo.HClassIndex() == -1) {
                return false;
            }
            parent_.checkerInfos_.emplace_back(receiverInfo);
        } else if (receiverType == holderType) {
            ObjectAccessInfo receiverInfo;
            if (!parent_.GeneratePlr(receiverType, receiverInfo, key)) {
                return false;
            }
            parent_.accessInfos_.emplace_back(receiverInfo);
            parent_.checkerInfos_.emplace_back(receiverInfo);
        } else {
            UNREACHABLE();
        }
    }
    return true;
}

bool StoreObjByNameTypeInfoAccessor::JitAccessorStrategy::GenerateObjectAccessInfo()
{
    JSTaggedValue key = parent_.GetKeyTaggedValue();
    if (key.IsUndefined()) {
        return false;
    }
    if (parent_.jitTypes_.size() == 0) {
        return false;
    }
    for (size_t i = 0; i < parent_.jitTypes_.size(); ++i) {
        JSHClass* receiverType = parent_.jitTypes_[i].GetReceiverHclass();
        JSHClass* holderType = parent_.jitTypes_[i].GetHolderHclass();
        if (receiverType->IsJsPrimitiveRef() || holderType->IsJsPrimitiveRef()) {
            return false;
        }
        JSHClass* newHolderType = parent_.jitTypes_[i].GetHolderTraHclass();

        if (receiverType != newHolderType) {
            // transition happened ==> slowpath
            ObjectAccessInfo newHolderInfo;
            if (!parent_.GeneratePlrInJIT(newHolderType, newHolderInfo, key)) {
                return false;
            }
            parent_.accessInfos_.emplace_back(newHolderInfo);

            ObjectAccessInfo receiverInfo;
            parent_.GeneratePlrInJIT(receiverType, receiverInfo, key);
            if (receiverInfo.HClassIndex() == -1) {
                return false;
            }
            parent_.checkerInfos_.emplace_back(receiverInfo);
        } else if (receiverType == holderType) {
            ObjectAccessInfo receiverInfo;
            if (!parent_.GeneratePlrInJIT(receiverType, receiverInfo, key)) {
                return false;
            }
            parent_.accessInfos_.emplace_back(receiverInfo);
            parent_.checkerInfos_.emplace_back(receiverInfo);
        } else {
            UNREACHABLE();
        }
    }
    return true;
}

InstanceOfTypeInfoAccessor::InstanceOfTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit, GateRef gate,
                                                       Chunk *chunk)
    : ObjAccByNameTypeInfoAccessor(env, circuit, gate, chunk, AccessMode::LOAD), types_(chunk_), jitTypes_(chunk)
{
    receiver_ = acc_.GetValueIn(gate, 1); // 2: receiver
    target_ = acc_.GetValueIn(gate, 2);   // 2: the third parameter
    if (IsAot()) {
        strategy_ = chunk_->New<AotAccessorStrategy>(*this);
    } else {
        strategy_ = chunk_->New<JitAccessorStrategy>(*this);
    }
    strategy_->FetchPGORWTypesDual();
    hasIllegalType_ = !strategy_->GenerateObjectAccessInfo();
}

JSTaggedValue InstanceOfTypeInfoAccessor::GetKeyTaggedValue() const
{
    JSHandle<GlobalEnv> globalEnv = compilationEnv_->GetGlobalEnv();
    auto hasInstanceEnvIndex = static_cast<size_t>(GlobalEnvField::HASINSTANCE_SYMBOL_INDEX);
    return globalEnv->GetGlobalEnvObjectByIndex(hasInstanceEnvIndex).GetTaggedValue();
}

void InstanceOfTypeInfoAccessor::AotAccessorStrategy::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGORWOpType();
    if (IsMegaType(pgoTypes)) {
        return;
    }
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsBuiltinsType()) {
            continue;
        }
        parent_.types_.emplace_back(std::make_pair(std::make_pair(temp.GetReceiverRootType(), temp.GetReceiverType()),
                                                   std::make_pair(temp.GetHoldRootType(), temp.GetHoldType())));
    }
}

void InstanceOfTypeInfoAccessor::JitAccessorStrategy::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGORWOpType();
    if (IsMegaType(pgoTypes)) {
        return;
    }
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsJITClassType()) {
            parent_.jitTypes_.emplace_back(temp);
        }
    }
}


bool InstanceOfTypeInfoAccessor::AotAccessorStrategy::ClassInstanceIsCallable(ProfileTyper type) const
{
    int hclassIndex = static_cast<int>(parent_.ptManager_->GetHClassIndexByProfileType(type));
    if (hclassIndex == -1) {
        return false;
    }

    JSTaggedValue hclass = parent_.ptManager_->QueryHClass(type.first, type.second);
    if (!hclass.IsJSHClass()) {
        return false;
    }
    return JSHClass::Cast(hclass.GetTaggedObject())->IsCallable();
}

bool InstanceOfTypeInfoAccessor::JitAccessorStrategy::ClassInstanceIsCallable(JSHClass *hclass) const
{
    return hclass->IsCallable();
}


bool InstanceOfTypeInfoAccessor::AotAccessorStrategy::GenerateObjectAccessInfo()
{
    // Instanceof does not support Poly for now
    if (!IsMono()) {
        return false;
    }
    if (parent_.types_.size() == 0) {
        return false;
    }
    JSTaggedValue key = parent_.GetKeyTaggedValue();
    for (size_t i = 0; i < parent_.types_.size(); ++i) {
        ProfileTyper targetPgoType = parent_.types_[i].first;
        ObjectAccessInfo targetInfo;
        // If @@hasInstance is found on prototype Chain of ctor -> slowpath
        // Future work: pgo support Symbol && Dump Object.defineProperty && FunctionPrototypeHclass
        // Current temporary solution:
        // Try searching @@hasInstance in pgo dump stage,
        // If found, pgo dump no types and we go slowpath in aot
        parent_.GeneratePlr(targetPgoType, targetInfo, key);
        if (targetInfo.HClassIndex() == -1 || !ClassInstanceIsCallable(targetPgoType)) {
            return false;
        }
        parent_.accessInfos_.emplace_back(targetInfo);
        parent_.checkerInfos_.emplace_back(targetInfo);
    }
    return true;
}

bool InstanceOfTypeInfoAccessor::JitAccessorStrategy::GenerateObjectAccessInfo()
{
    // Instanceof does not support Poly for now
    if (!IsMono()) {
        return false;
    }
    JSTaggedValue key = parent_.GetKeyTaggedValue();
    if (key.IsUndefined()) {
        return false;
    }
    if (parent_.jitTypes_.size() == 0) {
        return false;
    }
    for (size_t i = 0; i < parent_.jitTypes_.size(); ++i) {
        JSHClass *receiver = parent_.jitTypes_[i].GetReceiverHclass();
        if (receiver->IsJsPrimitiveRef()) {
            return false;
        }
        ObjectAccessInfo targetInfo;
        // If @@hasInstance is found on prototype Chain of ctor -> slowpath
        // Future work: pgo support Symbol && Dump Object.defineProperty && FunctionPrototypeHclass
        // Current temporary solution:
        // Try searching @@hasInstance in pgo dump stage,
        // If found, pgo dump no types and we go slowpath in aot
        parent_.GeneratePlrInJIT(receiver, targetInfo, key);
        if (targetInfo.HClassIndex() == -1 || !ClassInstanceIsCallable(receiver)) {
            return false;
        }
        parent_.accessInfos_.emplace_back(targetInfo);
        parent_.checkerInfos_.emplace_back(targetInfo);
    }
    return true;
}

void LoadBuiltinObjTypeInfoAccessor::AotAccessorStrategy::FetchPGORWTypesDual()
{
}

void LoadBuiltinObjTypeInfoAccessor::JitAccessorStrategy::FetchPGORWTypesDual()
{
    const PGORWOpType *pgoTypes = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGORWOpType();
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsBuiltinsType()) {
            parent_.jitTypes_.emplace_back(temp);
        }
    }
}

LoadBuiltinObjTypeInfoAccessor::LoadBuiltinObjTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit,
                                                               GateRef gate, Chunk *chunk)
    : AccBuiltinObjTypeInfoAccessor(env, circuit, gate, chunk, AccessMode::LOAD), jitTypes_(chunk_)
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
            receiver_ = circuit->GetArgumentAccessor()->GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
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
            receiver_ = circuit->GetArgumentAccessor()->GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
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

    if (IsAot()) {
        strategy_ = chunk_->New<AotAccessorStrategy>(*this);
    } else {
        strategy_ = chunk_->New<JitAccessorStrategy>(*this);
    }
    strategy_->FetchPGORWTypesDual();
    FetchBuiltinsTypes();
}

bool AccBuiltinObjTypeInfoAccessor::IsStringMonoBuiltins() const
{
    for (auto type : types_) {
        if (!type.IsBuiltinsString()) {
            return false;
        }
    }
    return true;
}

bool AccBuiltinObjTypeInfoAccessor::IsMonoBuiltins() const
{
    if (types_.size() == 0) {
        return false;
    }

    if (IsStringMonoBuiltins()) {
        return true;
    }

    for (size_t i = 0; i < types_.size(); i++) {
        if (!types_[i].IsBuiltinsType()) {
            return false;
        }
        if (types_[i].GetBuiltinsType() != types_[0].GetBuiltinsType()) {
            return false;
        }
    }
    return true;
}

void AccBuiltinObjTypeInfoAccessor::FetchBuiltinsTypes()
{
    const PGORWOpType *pgoTypes = acc_.TryGetPGOType(gate_).GetPGORWOpType();
    if (IsMegaType(pgoTypes)) {
        return;
    }
    for (uint32_t i = 0; i < pgoTypes->GetCount(); ++i) {
        auto temp = pgoTypes->GetObjectInfo(i);
        if (temp.GetReceiverType().IsBuiltinsType()) {
            if (CheckDuplicatedBuiltinType(temp.GetReceiverType())) {
                continue;
            }
            types_.emplace_back(temp.GetReceiverType());
        } else if (temp.GetReceiverType().IsGlobalsType()) {
            types_.emplace_back(temp.GetReceiverType());
        } else if (temp.GetReceiverType().IsInvalidType()) {
            types_.emplace_back(temp.GetReceiverType());
        }
    }
}

bool AccBuiltinObjTypeInfoAccessor::CheckDuplicatedBuiltinType(ProfileType newType) const
{
    for (auto &type : types_) {
        if (type.IsBuiltinsType() && type.GetBuiltinsType() == newType.GetBuiltinsType()) {
            if (type.IsBuiltinsArray()) {
                // When array elementsKind switch on, we should check elementsKind too.
                return (type.GetElementsKindBeforeTransition() == newType.GetElementsKindBeforeTransition() &&
                    type.GetElementsKindAfterTransition() == newType.GetElementsKindAfterTransition());
            }
            return true;
        }
    }
    return false;
}

StoreBuiltinObjTypeInfoAccessor::StoreBuiltinObjTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit,
                                                                 GateRef gate, Chunk *chunk)
    : AccBuiltinObjTypeInfoAccessor(env, circuit, gate, chunk, AccessMode::STORE)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16:
        case EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16:
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

CreateObjWithBufferTypeInfoAccessor::CreateObjWithBufferTypeInfoAccessor(const CompilationEnv *env, Circuit *circuit,
                                                                         GateRef gate, const CString &recordName,
                                                                         Chunk *chunk)
    : TypeInfoAccessor(env, circuit, gate), recordName_(recordName)
{
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: number of value ins
    index_ = acc_.GetValueIn(gate, 0);
    if (IsAot()) {
        strategy_ = chunk->New<AotAccessorStrategy>(*this);
    } else {
        strategy_ = chunk->New<JitAccessorStrategy>(*this);
    }
}

JSTaggedValue CreateObjWithBufferTypeInfoAccessor::GetObject() const
{
    auto imm = acc_.GetConstantValue(index_);
    auto methodOffset = acc_.TryGetMethodOffset(GetGate());
    JSTaggedValue unsharedCp = compilationEnv_->FindOrCreateUnsharedConstpool(methodOffset);
    if (unsharedCp.IsUndefined()) {
        return JSTaggedValue::Undefined();
    }
    return compilationEnv_->GetObjectLiteralFromCache(unsharedCp, imm, recordName_);
}

JSTaggedValue CreateObjWithBufferTypeInfoAccessor::AotAccessorStrategy::GetHClass() const
{
    JSTaggedValue obj = parent_.GetObject();
    if (obj.IsUndefined()) {
        return JSTaggedValue::Undefined();
    }
    auto sampleType = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGODefineOpType();
    auto type = std::make_pair(sampleType->GetProfileType(), sampleType->GetProfileType());
    int hclassIndex = static_cast<int>(parent_.ptManager_->GetHClassIndexByProfileType(type));

    JSObject *jsObj = JSObject::Cast(obj);
    JSHClass *oldClass = jsObj->GetClass();
    JSThread *thread = parent_.compilationEnv_->GetJSThread();
    if (hclassIndex == -1) {
        if (jsObj->ElementsAndPropertiesIsEmpty(thread)) {
            return JSTaggedValue(oldClass);
        }
        return JSTaggedValue::Undefined();
    }
    JSTaggedValue newClass = parent_.ptManager_->QueryHClass(type.first, type.second);
    if (!newClass.IsJSHClass()) {
        return JSTaggedValue::Undefined();
    }
    if (oldClass->GetInlinedProperties() != JSHClass::Cast(newClass.GetTaggedObject())->GetInlinedProperties()) {
        return JSTaggedValue::Undefined();
    }
    return newClass;
}

JSTaggedValue CreateObjWithBufferTypeInfoAccessor::JitAccessorStrategy::GetHClass() const
{
    JSTaggedValue obj = parent_.GetObject();
    if (obj.IsUndefined()) {
        return JSTaggedValue::Undefined();
    }
    auto sampleType = parent_.acc_.TryGetPGOType(parent_.gate_).GetPGODefineOpType();
    if (sampleType->IsNone()) {
        return JSTaggedValue::Undefined();
    }
    JSObject *jsObj = JSObject::Cast(obj);
    JSHClass *oldClass = jsObj->GetClass();
    JSHClass *newClass = sampleType->GetReceiver();
    JSThread *thread = parent_.compilationEnv_->GetJSThread();
    if (newClass == nullptr) {
        if (jsObj->ElementsAndPropertiesIsEmpty(thread)) {
            return JSTaggedValue(oldClass);
        }
        return JSTaggedValue::Undefined();
    }
    if (oldClass->GetInlinedProperties() != newClass->GetInlinedProperties()) {
        return JSTaggedValue::Undefined();
    }
    return JSTaggedValue(newClass);
}

}  // namespace panda::ecmascript::kungfu
