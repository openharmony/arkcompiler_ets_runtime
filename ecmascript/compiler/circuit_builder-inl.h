/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_INL_H
#define ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_INL_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/method.h"

namespace panda::ecmascript::kungfu {
// constant
GateRef CircuitBuilder::True()
{
    return TruncInt32ToInt1(Int32(1));
}

GateRef CircuitBuilder::False()
{
    return TruncInt32ToInt1(Int32(0));
}

GateRef CircuitBuilder::Undefined()
{
    return UndefineConstant();
}

GateRef CircuitBuilder::Equal(GateRef x, GateRef y)
{
    auto xType = acc_.GetMachineType(x);
    switch (xType) {
        case ARCH:
        case FLEX:
        case I1:
        case I8:
        case I16:
        case I32:
        case I64:
            return BinaryCmp(circuit_->Icmp(static_cast<uint64_t>(ICmpCondition::EQ)), x, y);
        case F32:
        case F64:
            return BinaryCmp(circuit_->Fcmp(static_cast<uint64_t>(FCmpCondition::OEQ)), x, y);
        default:
            UNREACHABLE();
    }
}

GateRef CircuitBuilder::NotEqual(GateRef x, GateRef y)
{
    auto xType = acc_.GetMachineType(x);
    switch (xType) {
        case ARCH:
        case FLEX:
        case I1:
        case I8:
        case I16:
        case I32:
        case I64:
            return BinaryCmp(circuit_->Icmp(static_cast<uint64_t>(ICmpCondition::NE)), x, y);
        case F32:
        case F64:
            return BinaryCmp(circuit_->Fcmp(static_cast<uint64_t>(FCmpCondition::ONE)), x, y);
        default:
            UNREACHABLE();
    }
}

// memory
GateRef CircuitBuilder::Load(VariableType type, GateRef base, GateRef offset)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef val = PtrAdd(base, offset);
    GateRef result = GetCircuit()->NewGate(GetCircuit()->Load(), type.GetMachineType(),
                                           { depend, val }, type.GetGateType());
    label->SetDepend(result);
    return result;
}

// Js World
// cast operation
GateRef CircuitBuilder::GetInt64OfTInt(GateRef x)
{
    GateRef tagged = ChangeTaggedPointerToInt64(x);
    return Int64And(tagged, Int64(~JSTaggedValue::TAG_MARK));
}

GateRef CircuitBuilder::GetInt32OfTInt(GateRef x)
{
    return TruncInt64ToInt32(GetInt64OfTInt(x));
}

GateRef CircuitBuilder::TaggedCastToIntPtr(GateRef x)
{
    ASSERT(cmpCfg_ != nullptr);
    return cmpCfg_->Is32Bit() ? GetInt32OfTInt(x) : GetInt64OfTInt(x);
}

GateRef CircuitBuilder::GetDoubleOfTDouble(GateRef x)
{
    GateRef tagged = ChangeTaggedPointerToInt64(x);
    GateRef val = Int64Sub(tagged, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    return CastInt64ToFloat64(val);
}

GateRef CircuitBuilder::GetDoubleOfTNumber(GateRef x)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label isInt(env_);
    Label isDouble(env_);
    Label exit(env_);
    DEFVAlUE(result, env_, VariableType::FLOAT64(), Double(0));
    Branch(TaggedIsInt(x), &isInt, &isDouble);
    Bind(&isInt);
    {
        result = ChangeInt32ToFloat64(GetInt32OfTInt(x));
        Jump(&exit);
    }
    Bind(&isDouble);
    {
        result = GetDoubleOfTDouble(x);
        Jump(&exit);
    }
    Bind(&exit);
    GateRef ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::Int8Equal(GateRef x, GateRef y)
{
    return Equal(x, y);
}

GateRef CircuitBuilder::Int32NotEqual(GateRef x, GateRef y)
{
    return NotEqual(x, y);
}

GateRef CircuitBuilder::Int64NotEqual(GateRef x, GateRef y)
{
    return NotEqual(x, y);
}

GateRef CircuitBuilder::DoubleEqual(GateRef x, GateRef y)
{
    return Equal(x, y);
}

GateRef CircuitBuilder::DoubleNotEqual(GateRef x, GateRef y)
{
    return NotEqual(x, y);
}

GateRef CircuitBuilder::Int64Equal(GateRef x, GateRef y)
{
    return Equal(x, y);
}

GateRef CircuitBuilder::Int32Equal(GateRef x, GateRef y)
{
    return Equal(x, y);
}

GateRef CircuitBuilder::IntPtrGreaterThan(GateRef x, GateRef y)
{
    return env_->Is32Bit() ? Int32GreaterThan(x, y) : Int64GreaterThan(x, y);
}

template<OpCode Op, MachineType Type>
GateRef CircuitBuilder::BinaryOp(GateRef x, GateRef y)
{
    if (Op == OpCode::ADD) {
        return BinaryArithmetic(circuit_->Add(), Type, x, y);
    } else if (Op == OpCode::SUB) {
        return BinaryArithmetic(circuit_->Sub(), Type, x, y);
    } else if (Op == OpCode::MUL) {
        return BinaryArithmetic(circuit_->Mul(), Type, x, y);
    }
    UNREACHABLE();
    return Circuit::NullGate();
}

GateRef CircuitBuilder::IntPtrLSR(GateRef x, GateRef y)
{
    auto ptrSize = env_->Is32Bit() ? MachineType::I32 : MachineType::I64;
    return BinaryArithmetic(circuit_->Lsr(), ptrSize, x, y);
}

GateRef CircuitBuilder::IntPtrLSL(GateRef x, GateRef y)
{
    auto ptrSize = env_->Is32Bit() ? MachineType::I32 : MachineType::I64;
    return BinaryArithmetic(circuit_->Lsl(), ptrSize, x, y);
}

GateRef CircuitBuilder::IntPtrOr(GateRef x, GateRef y)
{
    auto ptrsize = env_->Is32Bit() ? MachineType::I32 : MachineType::I64;
    return BinaryArithmetic(circuit_->Or(), ptrsize, x, y);
}

GateRef CircuitBuilder::IntPtrDiv(GateRef x, GateRef y)
{
    return env_->Is32Bit() ? Int32Div(x, y) : Int64Div(x, y);
}

GateRef CircuitBuilder::Int64ToTaggedPtr(GateRef x)
{
    return GetCircuit()->NewGate(circuit_->Int64ToTagged(),
        MachineType::I64, { x }, GateType::TaggedValue());
}

GateRef CircuitBuilder::Int32ToTaggedPtr(GateRef x)
{
    GateRef val = SExtInt32ToInt64(x);
    return Int64ToTaggedPtr(Int64Or(val, Int64(JSTaggedValue::TAG_INT)));
}

GateRef CircuitBuilder::Int32ToTaggedInt(GateRef x)
{
    GateRef val = SExtInt32ToInt64(x);
    return Int64Or(val, Int64(JSTaggedValue::TAG_INT));
}

// bit operation
GateRef CircuitBuilder::IsSpecial(GateRef x, JSTaggedType type)
{
    auto specialValue = circuit_->GetConstantGate(
        MachineType::I64, type, GateType::TaggedValue());

    return Equal(x, specialValue);
}

GateRef CircuitBuilder::TaggedIsInt(GateRef x)
{
    x = ChangeTaggedPointerToInt64(x);
    return Equal(Int64And(x, Int64(JSTaggedValue::TAG_MARK)),
                 Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::TaggedIsDouble(GateRef x)
{
    return BoolAnd(TaggedIsNumber(x), BoolNot(TaggedIsInt(x)));
}

GateRef CircuitBuilder::TaggedIsObject(GateRef x)
{
    x = ChangeTaggedPointerToInt64(x);
    return Equal(Int64And(x, Int64(JSTaggedValue::TAG_MARK)),
                 Int64(JSTaggedValue::TAG_OBJECT));
}

GateRef CircuitBuilder::TaggedIsNumber(GateRef x)
{
    return BoolNot(TaggedIsObject(x));
}

GateRef CircuitBuilder::TaggedIsNumeric(GateRef x)
{
    return BoolOr(TaggedIsNumber(x), TaggedIsBigInt(x));
}

GateRef CircuitBuilder::DoubleIsINF(GateRef x)
{
    GateRef infinity = Double(base::POSITIVE_INFINITY);
    GateRef negativeInfinity = Double(-base::POSITIVE_INFINITY);
    GateRef diff1 = DoubleEqual(x, infinity);
    GateRef diff2 = DoubleEqual(x, negativeInfinity);
    return BoolOr(diff1, diff2);
}

GateRef CircuitBuilder::TaggedIsHole(GateRef x)
{
    return Equal(x, HoleConstant());
}

GateRef CircuitBuilder::TaggedIsNotHole(GateRef x)
{
    return NotEqual(x, HoleConstant());
}

GateRef CircuitBuilder::TaggedIsUndefined(GateRef x)
{
    return Equal(x, UndefineConstant());
}

GateRef CircuitBuilder::TaggedIsException(GateRef x)
{
    return Equal(x, ExceptionConstant());
}

GateRef CircuitBuilder::TaggedIsSpecial(GateRef x)
{
    return BoolOr(
        Equal(Int64And(ChangeTaggedPointerToInt64(x), Int64(JSTaggedValue::TAG_SPECIAL_MASK)),
            Int64(JSTaggedValue::TAG_SPECIAL)),
        TaggedIsHole(x));
}

inline GateRef CircuitBuilder::IsJSHClass(GateRef obj)
{
    return Int32Equal(GetObjectType(LoadHClass(obj)), Int32(static_cast<int32_t>(JSType::HCLASS)));
}

GateRef CircuitBuilder::TaggedIsHeapObject(GateRef x)
{
    x = ChangeTaggedPointerToInt64(x);
    return Equal(Int64And(x, Int64(JSTaggedValue::TAG_HEAPOBJECT_MASK)), Int64(0));
}

GateRef CircuitBuilder::TaggedIsAsyncGeneratorObject(GateRef x)
{
    GateRef isHeapObj = TaggedIsHeapObject(x);
    GateRef objType = GetObjectType(LoadHClass(x));
    GateRef isAsyncGeneratorObj = Equal(objType,
        Int32(static_cast<int32_t>(JSType::JS_ASYNC_GENERATOR_OBJECT)));
    return LogicAnd(isHeapObj, isAsyncGeneratorObj);
}

GateRef CircuitBuilder::TaggedIsGeneratorObject(GateRef x)
{
    GateRef isHeapObj = TaggedIsHeapObject(x);
    GateRef objType = GetObjectType(LoadHClass(x));
    GateRef isAsyncGeneratorObj = Equal(objType,
        Int32(static_cast<int32_t>(JSType::JS_GENERATOR_OBJECT)));
    return LogicAnd(isHeapObj, isAsyncGeneratorObj);
}

GateRef CircuitBuilder::TaggedIsPropertyBox(GateRef x)
{
    return LogicAnd(TaggedIsHeapObject(x),
        IsJsType(x, JSType::PROPERTY_BOX));
}

GateRef CircuitBuilder::TaggedIsWeak(GateRef x)
{
    return LogicAnd(TaggedIsHeapObject(x),
        Equal(Int64And(ChangeTaggedPointerToInt64(x), Int64(JSTaggedValue::TAG_WEAK)), Int64(1)));
}

GateRef CircuitBuilder::TaggedIsPrototypeHandler(GateRef x)
{
    return LogicAnd(TaggedIsHeapObject(x),
        IsJsType(x, JSType::PROTOTYPE_HANDLER));
}

GateRef CircuitBuilder::TaggedIsTransitionHandler(GateRef x)
{
    return LogicAnd(TaggedIsHeapObject(x),
        IsJsType(x, JSType::TRANSITION_HANDLER));
}

GateRef CircuitBuilder::TaggedIsStoreTSHandler(GateRef x)
{
    return LogicAnd(TaggedIsHeapObject(x),
        IsJsType(x, JSType::STORE_TS_HANDLER));
}

GateRef CircuitBuilder::TaggedIsTransWithProtoHandler(GateRef x)
{
    return LogicAnd(TaggedIsHeapObject(x),
        IsJsType(x, JSType::TRANS_WITH_PROTO_HANDLER));
}

GateRef CircuitBuilder::TaggedIsUndefinedOrNull(GateRef x)
{
    return BoolOr(TaggedIsUndefined(x), TaggedIsNull(x));
}

GateRef CircuitBuilder::TaggedIsTrue(GateRef x)
{
    return Equal(x, TaggedTrue());
}

GateRef CircuitBuilder::TaggedIsFalse(GateRef x)
{
    return Equal(x, TaggedFalse());
}

GateRef CircuitBuilder::TaggedIsNull(GateRef x)
{
    return Equal(x, NullConstant());
}

GateRef CircuitBuilder::TaggedIsBoolean(GateRef x)
{
    return BoolOr(TaggedIsFalse(x), TaggedIsTrue(x));
}

GateRef CircuitBuilder::IsAOTLiteralInfo(GateRef x)
{
    GateRef isHeapObj = TaggedIsHeapObject(x);
    GateRef objType = GetObjectType(LoadHClass(x));
    GateRef isAOTLiteralInfoObj = Equal(objType,
        Int32(static_cast<int32_t>(JSType::AOT_LITERAL_INFO)));
    return LogicAnd(isHeapObj, isAOTLiteralInfoObj);
}

GateRef CircuitBuilder::TaggedGetInt(GateRef x)
{
    x = ChangeTaggedPointerToInt64(x);
    return TruncInt64ToInt32(Int64And(x, Int64(~JSTaggedValue::TAG_MARK)));
}

GateRef CircuitBuilder::ToTaggedInt(GateRef x)
{
    return Int64Or(x, Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::ToTaggedIntPtr(GateRef x)
{
    return Int64ToTaggedPtr(Int64Or(x, Int64(JSTaggedValue::TAG_INT)));
}

GateRef CircuitBuilder::DoubleToTaggedDoublePtr(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    return Int64ToTaggedPtr(Int64Add(val, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET)));
}

GateRef CircuitBuilder::BooleanToTaggedBooleanPtr(GateRef x)
{
    auto val = ZExtInt1ToInt64(x);
    return Int64ToTaggedPtr(Int64Or(val, Int64(JSTaggedValue::TAG_BOOLEAN_MASK)));
}

GateRef CircuitBuilder::Float32ToTaggedDoublePtr(GateRef x)
{
    GateRef val = ExtFloat32ToDouble(x);
    return DoubleToTaggedDoublePtr(val);
}

GateRef CircuitBuilder::TaggedDoublePtrToFloat32(GateRef x)
{
    GateRef val = GetDoubleOfTDouble(x);
    return TruncDoubleToFloat32(val);
}

GateRef CircuitBuilder::TaggedIntPtrToFloat32(GateRef x)
{
    GateRef val = GetInt32OfTInt(x);
    return ChangeInt32ToFloat32(val);
}

GateRef CircuitBuilder::DoubleToTaggedDouble(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    return Int64Add(val, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
}

GateRef CircuitBuilder::DoubleIsNAN(GateRef x)
{
    GateRef diff = Equal(x, x);
    return Equal(SExtInt1ToInt32(diff), Int32(0));
}

GateRef CircuitBuilder::DoubleToTagged(GateRef x)
{
    GateRef val = CastDoubleToInt64(x);
    acc_.SetGateType(val, GateType::TaggedValue());
    return Int64Add(val, Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
}

GateRef CircuitBuilder::TaggedTrue()
{
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_TRUE, GateType::TaggedValue());
}

GateRef CircuitBuilder::TaggedFalse()
{
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_FALSE, GateType::TaggedValue());
}

GateRef CircuitBuilder::GetValueFromTaggedArray(GateRef array, GateRef index)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    return Load(VariableType::JS_ANY(), array, dataOffset);
}

void CircuitBuilder::SetValueToTaggedArray(VariableType valType, GateRef glue,
                                           GateRef array, GateRef index, GateRef val)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    Store(valType, glue, array, dataOffset, val);
}

GateRef CircuitBuilder::GetGlobalConstantString(ConstantIndex index)
{
    return PtrMul(IntPtr(sizeof(JSTaggedValue)), IntPtr(static_cast<int>(index)));
}

// object operation
GateRef CircuitBuilder::LoadHClass(GateRef object)
{
    GateRef offset = IntPtr(TaggedObject::HCLASS_OFFSET);
    return Load(VariableType::JS_POINTER(), object, offset);
}

void CircuitBuilder::StoreHClass(GateRef glue, GateRef object, GateRef hClass)
{
    Store(VariableType::JS_POINTER(), glue, object, IntPtr(TaggedObject::HCLASS_OFFSET), hClass);
}

GateRef CircuitBuilder::IsJsType(GateRef obj, JSType type)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Equal(objectType, Int32(static_cast<int32_t>(type)));
}

inline GateRef CircuitBuilder::IsDictionaryMode(GateRef object)
{
    GateRef type = GetObjectType(LoadHClass(object));
    return Int32Equal(type, Int32(static_cast<int32_t>(JSType::TAGGED_DICTIONARY)));
}

GateRef CircuitBuilder::GetObjectType(GateRef hClass)
{
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return Int32And(bitfield, Int32((1LU << JSHClass::ObjectTypeBits::SIZE) - 1));
}

GateRef CircuitBuilder::IsDictionaryModeByHClass(GateRef hClass)
{
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsDictionaryBit::START_BIT)),
        Int32((1LU << JSHClass::IsDictionaryBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsDictionaryElement(GateRef hClass)
{
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::DictionaryElementBits::START_BIT)),
        Int32((1LU << JSHClass::DictionaryElementBits::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsClassConstructor(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::ClassConstructorBit::START_BIT)),
        Int32((1LU << JSHClass::ClassConstructorBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsClassPrototype(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    // decode
    return NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::ClassPrototypeBit::START_BIT)),
        Int32((1LU << JSHClass::ClassPrototypeBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsExtensible(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::ExtensibleBit::START_BIT)),
        Int32((1LU << JSHClass::ExtensibleBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::TaggedObjectIsEcmaObject(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return BoolAnd(
        Int32LessThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::ECMA_OBJECT_LAST))),
        Int32GreaterThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::ECMA_OBJECT_FIRST))));
}

GateRef CircuitBuilder::IsJSObject(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    auto ret = BoolAnd(
        Int32LessThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT_LAST))),
        Int32GreaterThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT_FIRST))));
    return LogicAnd(TaggedIsHeapObject(obj), ret);
}

GateRef CircuitBuilder::TaggedObjectBothAreString(GateRef x, GateRef y)
{
    return BoolAnd(IsJsType(x, JSType::STRING), IsJsType(y, JSType::STRING));
}

GateRef CircuitBuilder::IsCallableFromBitField(GateRef bitfield)
{
    return NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::CallableBit::START_BIT)),
            Int32((1LU << JSHClass::CallableBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsCallable(GateRef obj)
{
    GateRef hClass = LoadHClass(obj);
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return IsCallableFromBitField(bitfield);
}

GateRef CircuitBuilder::BothAreString(GateRef x, GateRef y)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label bothAreHeapObjet(env_);
    Label bothAreStringType(env_);
    Label exit(env_);
    DEFVAlUE(result, env_, VariableType::BOOL(), False());
    Branch(BoolAnd(TaggedIsHeapObject(x), TaggedIsHeapObject(y)), &bothAreHeapObjet, &exit);
    Bind(&bothAreHeapObjet);
    {
        Branch(TaggedObjectBothAreString(x, y), &bothAreStringType, &exit);
        Bind(&bothAreStringType);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetObjectSizeFromHClass(GateRef hClass)
{
    // NOTE: check for special case of string and TAGGED_ARRAY
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef objectSizeInWords = Int32And(Int32LSR(bitfield,
        Int32(JSHClass::ObjectSizeInWordsBits::START_BIT)),
        Int32((1LU << JSHClass::ObjectSizeInWordsBits::SIZE) - 1));
    return PtrMul(ZExtInt32ToPtr(objectSizeInWords), IntPtr(JSTaggedValue::TaggedTypeSize()));
}

template<TypedBinOp Op>
GateRef CircuitBuilder::TypedBinaryOp(GateRef x, GateRef y, GateType xType, GateType yType, GateType gateType)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto numberBinaryOp = TypedBinaryOperator(MachineType::I64, Op, xType, yType,
                                              {currentControl, currentDepend, x, y}, gateType);
    currentLabel->SetControl(numberBinaryOp);
    currentLabel->SetDepend(numberBinaryOp);
    return numberBinaryOp;
}

template<TypedUnOp Op>
GateRef CircuitBuilder::TypedUnaryOp(GateRef x, GateType xType, GateType gateType)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto machineType = (Op == TypedUnOp::TYPED_TOBOOL) ? MachineType::I1 : MachineType::I64;
    auto numberUnaryOp = TypedUnaryOperator(machineType, Op, xType, {currentControl, currentDepend, x}, gateType);
    currentLabel->SetControl(numberUnaryOp);
    currentLabel->SetDepend(numberUnaryOp);
    return numberUnaryOp;
}

template<TypedLoadOp Op>
GateRef CircuitBuilder::LoadElement(GateRef receiver, GateRef index)
{
    auto opIdx = static_cast<uint64_t>(Op);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(GetCircuit()->LoadElement(opIdx), MachineType::ANYVALUE,
                                     { currentControl, currentDepend, receiver, index }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

template<TypedStoreOp Op>
GateRef CircuitBuilder::StoreElement(GateRef receiver, GateRef index, GateRef value)
{
    auto opIdx = static_cast<uint64_t>(Op);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(GetCircuit()->StoreElement(opIdx), MachineType::NOVALUE,
                                     { currentControl, currentDepend, receiver, index, value }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

// Number operator
template<TypedBinOp Op>
GateRef CircuitBuilder::NumberBinaryOp(GateRef x, GateRef y)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto numberBinaryOp = TypedBinaryOperator(MachineType::I64, Op,
                                              GateType::NumberType(), GateType::NumberType(),
                                              {currentControl, currentDepend, x, y}, GateType::AnyType());
    currentLabel->SetControl(numberBinaryOp);
    currentLabel->SetDepend(numberBinaryOp);
    return numberBinaryOp;
}

GateRef CircuitBuilder::PrimitiveToNumber(GateRef x, VariableType type)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto numberconvert = TypeConvert(MachineType::I64, type.GetGateType(), GateType::NumberType(),
                                     {currentControl, currentDepend, x});
    currentLabel->SetControl(numberconvert);
    currentLabel->SetDepend(numberconvert);
    return numberconvert;
}

GateRef CircuitBuilder::LogicAnd(GateRef x, GateRef y)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label exit(env_);
    Label isX(env_);
    Label notX(env_);
    DEFVAlUE(result, env_, VariableType::BOOL(), x);
    Branch(x, &isX, &notX);
    Bind(&isX);
    {
        result = y;
        Jump(&exit);
    }
    Bind(&notX);
    {
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::LogicOr(GateRef x, GateRef y)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label exit(env_);
    Label isX(env_);
    Label notX(env_);
    DEFVAlUE(result, env_, VariableType::BOOL(), x);
    Branch(x, &isX, &notX);
    Bind(&isX);
    {
        Jump(&exit);
    }
    Bind(&notX);
    {
        result = y;
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

int CircuitBuilder::NextVariableId()
{
    return env_->NextVariableId();
}

void CircuitBuilder::HandleException(GateRef result, Label *success, Label *fail, Label *exit)
{
    Branch(Equal(result, ExceptionConstant()), fail, success);
    Bind(fail);
    {
        Jump(exit);
    }
}

void CircuitBuilder::HandleException(GateRef result, Label *success, Label *fail, Label *exit, GateRef exceptionVal)
{
    Branch(Equal(result, exceptionVal), fail, success);
    Bind(fail);
    {
        Jump(exit);
    }
}

void CircuitBuilder::SubCfgEntry(Label *entry)
{
    ASSERT(env_ != nullptr);
    env_->SubCfgEntry(entry);
}

void CircuitBuilder::SubCfgExit()
{
    ASSERT(env_ != nullptr);
    env_->SubCfgExit();
}

GateRef CircuitBuilder::Return(GateRef value)
{
    auto control = GetCurrentLabel()->GetControl();
    auto depend = GetCurrentLabel()->GetDepend();
    return Return(control, depend, value);
}

GateRef CircuitBuilder::Return()
{
    auto control = GetCurrentLabel()->GetControl();
    auto depend = GetCurrentLabel()->GetDepend();
    return ReturnVoid(control, depend);
}

void CircuitBuilder::Bind(Label *label)
{
    label->Bind();
    env_->SetCurrentLabel(label);
}

void CircuitBuilder::Bind(Label *label, bool justSlowPath)
{
    if (!justSlowPath) {
        label->Bind();
        env_->SetCurrentLabel(label);
    }
}

Label *CircuitBuilder::GetCurrentLabel() const
{
    return GetCurrentEnvironment()->GetCurrentLabel();
}

GateRef CircuitBuilder::GetState() const
{
    return GetCurrentLabel()->GetControl();
}

GateRef CircuitBuilder::GetDepend() const
{
    return GetCurrentLabel()->GetDepend();
}

void CircuitBuilder::SetDepend(GateRef depend)
{
    GetCurrentLabel()->SetDepend(depend);
}

void CircuitBuilder::SetState(GateRef state)
{
    GetCurrentLabel()->SetControl(state);
}

// ctor is base but not builtin
inline GateRef CircuitBuilder::IsBase(GateRef ctor)
{
    GateRef method = GetMethodFromFunction(ctor);
    GateRef extraLiteralInfoOffset = IntPtr(Method::EXTRA_LITERAL_INFO_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), method, extraLiteralInfoOffset);

    GateRef kind = Int32And(Int32LSR(bitfield, Int32(MethodLiteral::FunctionKindBits::START_BIT)),
                            Int32((1LU << MethodLiteral::FunctionKindBits::SIZE) - 1));
    return Int32LessThanOrEqual(kind, Int32(static_cast<int32_t>(FunctionKind::CLASS_CONSTRUCTOR)));
}

inline GateRef CircuitBuilder::TypedCallBuiltin(GateRef x, BuiltinsStubCSigns::ID id)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef idGate = Int8(static_cast<int8_t>(id));
    auto numberMathOp = TypedCallOperator(MachineType::I64, {currentControl, currentDepend, x, idGate});
    currentLabel->SetControl(numberMathOp);
    currentLabel->SetDepend(numberMathOp);
    return numberMathOp;
}

void Label::Seal()
{
    return impl_->Seal();
}

void Label::Bind()
{
    impl_->Bind();
}

void Label::MergeAllControl()
{
    impl_->MergeAllControl();
}

void Label::MergeAllDepend()
{
    impl_->MergeAllDepend();
}

void Label::AppendPredecessor(const Label *predecessor)
{
    impl_->AppendPredecessor(predecessor->GetRawLabel());
}

std::vector<Label> Label::GetPredecessors() const
{
    std::vector<Label> labels;
    for (auto rawlabel : impl_->GetPredecessors()) {
        labels.emplace_back(Label(rawlabel));
    }
    return labels;
}

void Label::SetControl(GateRef control)
{
    impl_->SetControl(control);
}

void Label::SetPreControl(GateRef control)
{
    impl_->SetPreControl(control);
}

void Label::MergeControl(GateRef control)
{
    impl_->MergeControl(control);
}

GateRef Label::GetControl() const
{
    return impl_->GetControl();
}

GateRef Label::GetDepend() const
{
    return impl_->GetDepend();
}

void Label::SetDepend(GateRef depend)
{
    return impl_->SetDepend(depend);
}

Label Environment::GetLabelFromSelector(GateRef sel)
{
    Label::LabelImpl *rawlabel = phiToLabels_[sel];
    return Label(rawlabel);
}

void Environment::AddSelectorToLabel(GateRef sel, Label label)
{
    phiToLabels_[sel] = label.GetRawLabel();
}

Label::LabelImpl *Environment::NewLabel(Environment *env, GateRef control)
{
    auto impl = new Label::LabelImpl(env, control);
    rawLabels_.emplace_back(impl);
    return impl;
}

void Environment::SubCfgEntry(Label *entry)
{
    if (currentLabel_ != nullptr) {
        GateRef control = currentLabel_->GetControl();
        GateRef depend = currentLabel_->GetDepend();
        stack_.push(currentLabel_);
        currentLabel_ = entry;
        currentLabel_->SetControl(control);
        currentLabel_->SetDepend(depend);
    }
}

void Environment::SubCfgExit()
{
    if (currentLabel_ != nullptr) {
        GateRef control = currentLabel_->GetControl();
        GateRef depend = currentLabel_->GetDepend();
        if (!stack_.empty()) {
            currentLabel_ = stack_.top();
            currentLabel_->SetControl(control);
            currentLabel_->SetDepend(depend);
            stack_.pop();
        }
    }
}

GateRef Environment::GetInput(size_t index) const
{
    return inputList_.at(index);
}

// only for int32
template<TypedUnOp Op>
GateRef CircuitBuilder::Int32OverflowCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    ASSERT(acc_.HasFrameState(currentDepend));
    auto frameState = acc_.GetFrameState(currentDepend);

    uint64_t value = TypedUnaryAccessor::ToValue(GateType::Empty(), Op);
    GateRef ret = GetCircuit()->NewGate(circuit_->Int32OverflowCheck(value),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}
} // namespace panda::ecmascript::kungfu

#endif