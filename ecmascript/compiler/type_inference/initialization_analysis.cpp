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

#include "ecmascript/compiler/type_inference/initialization_analysis.h"
#include "ecmascript/ts_types/ts_type_accessor.h"

namespace panda::ecmascript::kungfu {
void InitializationAnalysis::Run()
{
    if (classType_.IsAnyType()) {
        return;
    }

    CollectThisEscapedInfo(thisObject_);
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            Analyse(gate);
        }
    }
    MarkHasAnalysedInitialization();

    if (IsLogEnabled()) {
        Print();
    }
}

void InitializationAnalysis::Analyse(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::JS_BYTECODE);
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8: {
            CollectInitializationType(gate, ThisUsage::INDEFINITE_THIS);
            CollectInitializationInfo(gate, ThisUsage::INDEFINITE_THIS);
            break;
        }
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16: {
            CollectInitializationType(gate, ThisUsage::DEFINITE_THIS);
            CollectInitializationInfo(gate, ThisUsage::DEFINITE_THIS);
            break;
        }
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        case EcmaOpcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
        case EcmaOpcode::SUPERCALLSPREAD_IMM8_V8: {
            MarkSuperClass();
            CollectThisEscapedInfo(gate);
            break;
        }
        default: {
            break;
        }
    }
}

void InitializationAnalysis::CollectInitializationType(GateRef gate, ThisUsage thisUsage)
{
    GateRef value = acc_.GetValueIn(gate, 3);  // 3: index of value
    GateType valueType = acc_.GetGateType(value);
    if (valueType.IsAnyType()) {
        return;
    }
    if (thisUsage == ThisUsage::INDEFINITE_THIS) {
        GateRef receiver = acc_.GetValueIn(gate, 2);  // 2: index of receiver
        GateType receiverType = acc_.GetGateType(receiver);
        auto receiverGT = receiverType.GetGTRef();
        if (tsManager_->IsClassInstanceTypeKind(receiverType)) {
            receiverGT = tsManager_->GetClassType(receiverType);
        }
        if (!CheckIsThisObject(receiver) && receiverGT != classType_.GetGTRef()) {
            return;
        }
    }

    uint16_t index = acc_.GetConstantValue(acc_.GetValueIn(gate, 1));  // 1: stringId
    JSTaggedValue propKey = tsManager_->GetStringFromConstantPool(index);

    if (valueType.IsNumberType()) {
        valueType = GateType::NumberType();
    }

    TSTypeAccessor typeAccessor(tsManager_, classType_);
    typeAccessor.UpdateNonStaticProp(propKey, valueType.GetGTRef());
}

void InitializationAnalysis::CollectInitializationInfo(GateRef gate, ThisUsage thisUsage)
{
    if (thisUsage == ThisUsage::INDEFINITE_THIS) {
        GateRef receiver = acc_.GetValueIn(gate, 2);  // 2: index of receiver
        if (!CheckIsThisObject(receiver)) {
            return;
        }
    }

    uint16_t index = acc_.GetConstantValue(acc_.GetValueIn(gate, 1));  // 1: stringId
    if (!CheckSimpleCFG(gate, index)) {
        return;
    }
    JSTaggedValue propKey = tsManager_->GetStringFromConstantPool(index);
    TSTypeAccessor typeAccessor(tsManager_, classType_);
    typeAccessor.MarkPropertyInitialized(propKey);
}

bool InitializationAnalysis::CheckIsThisObject(GateRef receiver) const
{
    return IsThisFromArg(receiver) || IsThisFromSuperCall(receiver);
}

bool InitializationAnalysis::IsThisFromSuperCall(GateRef gate) const
{
    auto op = acc_.GetOpCode(gate);
    if (op != OpCode::JS_BYTECODE) {
        return false;
    }
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        case EcmaOpcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
        case EcmaOpcode::SUPERCALLSPREAD_IMM8_V8: {
            return true;
        }
        default: {
            break;
        }
    }
    return false;
}

bool InitializationAnalysis::CheckSimpleCFG(GateRef gate, const uint16_t index) const
{
    while (!acc_.IsStateRoot(gate)) {
        if (CheckSimpleGate(gate, index)) {
            return false;
        }
        gate = acc_.GetState(gate);
    }
    return true;
}

bool InitializationAnalysis::CheckSimpleGate(GateRef gate, const uint16_t index) const
{
    OpCode opCode = acc_.GetOpCode(gate);
    switch (opCode) {
        case OpCode::IF_TRUE:
        case OpCode::IF_FALSE: {
            return true;
        }
        case OpCode::JS_BYTECODE: {
            return CheckSimpleJSGate(gate, index);
        }
        default: {
            break;
        }
    }
    return false;
}

bool InitializationAnalysis::CheckSimpleJSGate(GateRef gate, const uint16_t index) const
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::JS_BYTECODE);
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16: {
            return CheckLdObjByName(gate, index, ThisUsage::INDEFINITE_THIS);
        }
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16: {
            return CheckLdObjByName(gate, index, ThisUsage::DEFINITE_THIS);
        }
        case EcmaOpcode::LDTHISBYVALUE_IMM8:
        case EcmaOpcode::LDTHISBYVALUE_IMM16: {
            return true;
        }
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
        case EcmaOpcode::LDOBJBYINDEX_IMM8_IMM16:
        case EcmaOpcode::LDOBJBYINDEX_IMM16_IMM16:
        case EcmaOpcode::WIDE_LDOBJBYINDEX_PREF_IMM32: {
            return CheckLdObjByIndexOrValue(gate);
        }
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
        case EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16: {
            return false;
        }
        default: {
            return CheckThisAsValueIn(gate);
        }
    }
    return false;
}

bool InitializationAnalysis::CheckLdObjByName(GateRef gate, const uint16_t index, ThisUsage thisUsage) const
{
    if (thisUsage == ThisUsage::INDEFINITE_THIS) {
        GateRef receiver = acc_.GetValueIn(gate, 2);  // 2: index of receiver
        if (!CheckIsThisObject(receiver)) {
            return false;
        }
    }

    auto constData = acc_.GetValueIn(gate, 1); // 1: stringId
    uint16_t stringId = acc_.GetConstantValue(constData);
    return stringId == index;
}

bool InitializationAnalysis::CheckLdObjByIndexOrValue(GateRef gate) const
{
    GateRef receiver = acc_.GetValueIn(gate, 1);  // 1: index of receiver
    return CheckIsThisObject(receiver);
}

void InitializationAnalysis::MarkSuperClass()
{
    TSTypeAccessor typeAccessor(tsManager_, classType_);
    typeAccessor.MarkClassHasSuperCallInConstructor();
}

void InitializationAnalysis::StoreThisObject()
{
    ArgumentAccessor argAcc(circuit_);
    thisObject_ = argAcc.GetCommonArgGate(CommonArgIdx::THIS_OBJECT);
    auto type = acc_.GetGateType(thisObject_);
    if (tsManager_->IsClassInstanceTypeKind(type)) {
        classType_ = GateType(tsManager_->GetClassType(type));
    }
}

bool InitializationAnalysis::CheckThisAsValueIn(GateRef gate) const
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::JS_BYTECODE);
    uint32_t numIns = acc_.GetNumValueIn(gate);
    for (uint32_t i = 0; i < numIns; ++i) {
        if (CheckIsThisObject(acc_.GetValueIn(gate, i))) {
            return true;
        }
    }
    return false;
}

void InitializationAnalysis::CollectThisEscapedInfo(GateRef gate)
{
    ASSERT(gate != Circuit::NullGate());
    std::vector<GateRef> valueUses;
    acc_.GetValueUses(gate, valueUses);
    for (const auto useGate : valueUses) {
        if (!HasEscapedThis(useGate)) {
            continue;
        }
        TSTypeAccessor typeAccessor(tsManager_, classType_);
        typeAccessor.MarkClassHasEscapedThisInConstructor();
        return;
    }
}

bool InitializationAnalysis::HasEscapedThis(GateRef gate) const
{
    if (acc_.GetOpCode(gate) != OpCode::JS_BYTECODE) {
        return false;
    }
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
        case EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
        case EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16: {
            return false;
        }
        default: {
            break;
        }
    }
    return true;
}

void InitializationAnalysis::MarkHasAnalysedInitialization()
{
    TSTypeAccessor typeAccessor(tsManager_, classType_);
    typeAccessor.MarkClassHasAnalysedInitialization();
}

bool InitializationAnalysis::CheckCall() const
{
    TSTypeAccessor typeAccessor(tsManager_, classType_);
    return typeAccessor.ClassHasEscapedThisInConstructor();
}

void InitializationAnalysis::Print() const
{
    LOG_COMPILER(INFO) << " ";
    LOG_COMPILER(INFO) << "\033[34m" << "================="
                       << " After initialization analysis "
                       << "[" << GetMethodName() << "] "
                       << "=================" << "\033[0m";
    TSTypeAccessor typeAccessor(tsManager_, classType_);
    LOG_COMPILER(INFO) << "In the constructor of class " << typeAccessor.GetClassTypeName()
                       << " of record " << std::string(recordName_)
                       << ", the following propertiess will be initialized.";
    LOG_COMPILER(INFO) << (typeAccessor.GetInitializedProperties());
    if (typeAccessor.ClassHasEscapedThisInConstructor()) {
        LOG_COMPILER(INFO) << "This-object will be called by some functions in the constructor.";
    }
    LOG_COMPILER(INFO) << "\033[34m" << "=========================== End ===========================" << "\033[0m";
}
}  // namespace panda::ecmascript
