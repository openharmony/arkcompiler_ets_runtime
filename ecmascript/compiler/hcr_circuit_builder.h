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

#ifndef ECMASCRIPT_COMPILER_HCR_CIRCUIT_BUILDER_H
#define ECMASCRIPT_COMPILER_HCR_CIRCUIT_BUILDER_H

#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::kungfu {

GateRef CircuitBuilder::IsSpecial(GateRef x, JSTaggedType type)
{
    auto specialValue = circuit_->GetConstantGate(
        MachineType::I64, type, GateType::TaggedValue());

    return Equal(x, specialValue);
}

inline GateRef CircuitBuilder::IsJSHClass(GateRef obj)
{
    return Int32Equal(GetObjectType(LoadHClass(obj)), Int32(static_cast<int32_t>(JSType::HCLASS)));
}

inline GateRef CircuitBuilder::IsJSFunction(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    GateRef greater = Int32GreaterThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_FUNCTION_FIRST)));
    GateRef less = Int32LessThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_FUNCTION_LAST)));
    return BitAnd(greater, less);
}

GateRef CircuitBuilder::IsJsType(GateRef obj, JSType type)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Equal(objectType, Int32(static_cast<int32_t>(type)));
}

GateRef CircuitBuilder::IsJSObject(GateRef obj)
{
    Label entryPass(env_);
    SubCfgEntry(&entryPass);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    Label heapObj(env_);
    Label exit(env_);
    GateRef isHeapObject = TaggedIsHeapObject(obj);
    BRANCH(isHeapObject, &heapObj, &exit);
    Bind(&heapObj);
    {
        GateRef objectType = GetObjectType(LoadHClass(obj));
        result = BitAnd(
            Int32LessThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT_LAST))),
            Int32GreaterThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::JS_OBJECT_FIRST))));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
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

GateRef CircuitBuilder::AlreadyDeopt(GateRef method)
{
    GateRef extraLiteralInfoOffset = IntPtr(Method::EXTRA_LITERAL_INFO_OFFSET);
    GateRef extraLiteralInfo = Load(VariableType::INT64(), method, extraLiteralInfoOffset);
    return AlreadyDeoptFromExtraLiteralInfo(extraLiteralInfo);
}

GateRef CircuitBuilder::AlreadyDeoptFromExtraLiteralInfo(GateRef callfield)
{
    return NotEqual(Int64And(Int64LSR(callfield, Int64(Method::DeoptTypeBits::START_BIT)),
                             Int64((1LU << Method::DeoptTypeBits::SIZE) - 1)),
                    Int64(0));
}

GateRef CircuitBuilder::IsPrototypeHClass(GateRef hClass)
{
    GateRef bitfield = LoadConstOffset(VariableType::INT32(), hClass, JSHClass::BIT_FIELD_OFFSET);
    return TruncInt32ToInt1(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsPrototypeBit::START_BIT)),
        Int32((1LU << JSHClass::IsPrototypeBit::SIZE) - 1)));
}

GateRef CircuitBuilder::IsJsProxy(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_PROXY)));
}

GateRef CircuitBuilder::IsTreeString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::TREE_STRING)));
}

GateRef CircuitBuilder::IsSlicedString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::SLICED_STRING)));
}

GateRef CircuitBuilder::IsLineString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::LINE_STRING)));
}

GateRef CircuitBuilder::IsConstantString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::CONSTANT_STRING)));
}

GateRef CircuitBuilder::ComputeSizeUtf8(GateRef length)
{
    return PtrAdd(IntPtr(LineEcmaString::DATA_OFFSET), length);
}

GateRef CircuitBuilder::ComputeSizeUtf16(GateRef length)
{
    return PtrAdd(IntPtr(LineEcmaString::DATA_OFFSET), PtrMul(length, IntPtr(sizeof(uint16_t))));
}

GateRef CircuitBuilder::AlignUp(GateRef x, GateRef alignment)
{
    GateRef x1 = PtrAdd(x, PtrSub(alignment, IntPtr(1)));
    return IntPtrAnd(x1, IntPtrNot(PtrSub(alignment, IntPtr(1))));
}

inline GateRef CircuitBuilder::IsDictionaryMode(GateRef object)
{
    GateRef type = GetObjectType(LoadHClass(object));
    return Int32Equal(type, Int32(static_cast<int32_t>(JSType::TAGGED_DICTIONARY)));
}

GateRef CircuitBuilder::IsStableArguments(GateRef hClass)
{
    GateRef objectType = GetObjectType(hClass);
    GateRef isJsArguments = Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_ARGUMENTS)));
    GateRef isStableElements = IsStableElements(hClass);
    return BitAnd(isStableElements, isJsArguments);
}

GateRef CircuitBuilder::IsStableArray(GateRef hClass)
{
    GateRef objectType = GetObjectType(hClass);
    GateRef isJsArray = Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_ARRAY)));
    GateRef isStableElements = IsStableElements(hClass);
    return BitAnd(isStableElements, isJsArray);
}

GateRef CircuitBuilder::IsAOTLiteralInfo(GateRef x)
{
    GateRef objType = GetObjectType(LoadHClass(x));
    GateRef isAOTLiteralInfoObj = Equal(objType,
        Int32(static_cast<int32_t>(JSType::AOT_LITERAL_INFO)));
    return isAOTLiteralInfoObj;
}

GateRef CircuitBuilder::LoadHClass(GateRef object)
{
    GateRef offset = IntPtr(TaggedObject::HCLASS_OFFSET);
    return Load(VariableType::JS_POINTER(), object, offset);
}

GateRef CircuitBuilder::LoadHClassByConstOffset(GateRef object)
{
    return LoadConstOffset(VariableType::JS_POINTER(), object, TaggedObject::HCLASS_OFFSET);
}

GateRef CircuitBuilder::LoadPrototype(GateRef hclass)
{
    return LoadConstOffset(VariableType::JS_POINTER(), hclass, JSHClass::PROTOTYPE_OFFSET);
}

GateRef CircuitBuilder::LoadPrototypeHClass(GateRef object)
{
    GateRef objectHClass = LoadHClassByConstOffset(object);
    GateRef objectPrototype = LoadPrototype(objectHClass);
    return LoadHClass(objectPrototype);
}

GateRef CircuitBuilder::LoadPrototypeOfPrototypeHClass(GateRef object)
{
    GateRef objectHClass = LoadHClassByConstOffset(object);
    GateRef objectPrototype = LoadPrototype(objectHClass);
    GateRef objectPrototypeHClass = LoadHClassByConstOffset(objectPrototype);
    GateRef objectPrototypeOfPrototype = LoadPrototype(objectPrototypeHClass);
    return LoadHClass(objectPrototypeOfPrototype);
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

GateRef CircuitBuilder::IsDictionaryModeByHClass(GateRef hClass)
{
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsDictionaryBit::START_BIT)),
        Int32((1LU << JSHClass::IsDictionaryBit::SIZE) - 1)),
        Int32(0));
}

void CircuitBuilder::StoreHClass(GateRef glue, GateRef object, GateRef hClass)
{
    Store(VariableType::JS_POINTER(), glue, object, IntPtr(TaggedObject::HCLASS_OFFSET), hClass,
          MemoryAttribute::NeedBarrier());
}

void CircuitBuilder::StoreHClassWithoutBarrier(GateRef glue, GateRef object, GateRef hClass)
{
    Store(VariableType::JS_POINTER(), glue, object, IntPtr(TaggedObject::HCLASS_OFFSET), hClass,
          MemoryAttribute::NoBarrier());
}

void CircuitBuilder::StorePrototype(GateRef glue, GateRef hclass, GateRef prototype)
{
    Store(VariableType::JS_POINTER(), glue, hclass, IntPtr(JSHClass::PROTOTYPE_OFFSET), prototype);
}

GateRef CircuitBuilder::GetObjectType(GateRef hClass)
{
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return Int32And(bitfield, Int32((1LU << JSHClass::ObjectTypeBits::SIZE) - 1));
}

inline GateRef CircuitBuilder::CanFastCall(GateRef jsFunc)
{
    GateRef bitFieldOffset = IntPtr(JSFunctionBase::BIT_FIELD_OFFSET);
    GateRef bitField = Load(VariableType::INT32(), jsFunc, bitFieldOffset);
    return Int32NotEqual(
        Int32And(
            Int32LSR(bitField, Int32(JSFunctionBase::IsFastCallBit::START_BIT)),
            Int32((1LU << JSFunctionBase::IsFastCallBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::GetElementsKindByHClass(GateRef hClass)
{
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return Int32And(Int32LSR(bitfield,
        Int32(JSHClass::ElementsKindBits::START_BIT)),
        Int32((1LLU << JSHClass::ElementsKindBits::SIZE) - 1));
}

GateRef CircuitBuilder::HasConstructorByHClass(GateRef hClass)
{
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::HasConstructorBits::START_BIT)),
        Int32((1LU << JSHClass::HasConstructorBits::SIZE) - 1)),
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

GateRef CircuitBuilder::IsStableElements(GateRef hClass)
{
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsStableElementsBit::START_BIT)),
        Int32((1LU << JSHClass::IsStableElementsBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsJSArrayPrototypeModified(GateRef hClass)
{
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsJSArrayPrototypeModifiedBit::START_BIT)),
        Int32((1LU << JSHClass::IsJSArrayPrototypeModifiedBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::HasConstructor(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    return HasConstructorByHClass(hClass);
}

GateRef CircuitBuilder::IsConstructor(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    // decode
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::ConstructorBit::START_BIT)),
                 Int32((1LU << JSHClass::ConstructorBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsClassConstructor(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    return IsClassConstructorWithBitField(bitfield);
}

GateRef CircuitBuilder::IsClassConstructorWithBitField(GateRef bitfield)
{
    auto classBitMask = 1LU << JSHClass::IsClassConstructorOrPrototypeBit::START_BIT;
    auto ctorBitMask = 1LU << JSHClass::ConstructorBit::START_BIT;
    auto mask = Int32(classBitMask | ctorBitMask);
    auto classCtor = Int32And(bitfield, mask);
    return Int32Equal(classCtor, mask);
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

GateRef CircuitBuilder::IsClassPrototype(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    // decode
    return IsClassPrototypeWithBitField(bitfield);
}

GateRef CircuitBuilder::IsClassPrototypeWithBitField(GateRef bitfield)
{
    auto classBitMask = 1LU << JSHClass::IsClassConstructorOrPrototypeBit::START_BIT;
    auto ptBitMask = 1LU << JSHClass::IsPrototypeBit::START_BIT;
    auto mask = Int32(classBitMask | ptBitMask);
    auto classPt = Int32And(bitfield, mask);
    return Int32Equal(classPt, mask);
}

GateRef CircuitBuilder::CreateWeakRef(GateRef x)
{
    return PtrAdd(x, IntPtr(JSTaggedValue::TAG_WEAK));
}

GateRef CircuitBuilder::LoadObjectFromWeakRef(GateRef x)
{
    return PtrAdd(x, IntPtr(-JSTaggedValue::TAG_WEAK));
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

inline GateRef CircuitBuilder::IsDerived(GateRef ctor)
{
    GateRef method = GetMethodFromFunction(ctor);
    GateRef extraLiteralInfoOffset = IntPtr(Method::EXTRA_LITERAL_INFO_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), method, extraLiteralInfoOffset);

    GateRef kind = Int32And(Int32LSR(bitfield, Int32(MethodLiteral::FunctionKindBits::START_BIT)),
                            Int32((1LU << MethodLiteral::FunctionKindBits::SIZE) - 1));
    return Int32Equal(kind, Int32(static_cast<int32_t>(FunctionKind::DERIVED_CONSTRUCTOR)));
}

inline GateRef CircuitBuilder::GetMethodId(GateRef func)
{
    GateRef method = GetMethodFromFunction(func);
    GateRef literalInfoOffset = IntPtr(Method::LITERAL_INFO_OFFSET);
    GateRef literalInfo = Load(VariableType::INT64(), method, literalInfoOffset);
    GateRef methodId = Int64And(Int64LSR(literalInfo, Int64(MethodLiteral::MethodIdBits::START_BIT)),
        Int64((1LLU << MethodLiteral::MethodIdBits::SIZE) - 1));
    return methodId;
}

inline GateRef CircuitBuilder::GetBuiltinsId(GateRef func)
{
    GateRef method = GetMethodFromFunction(func);
    GateRef extraLiteralInfoOffset = IntPtr(Method::EXTRA_LITERAL_INFO_OFFSET);
    GateRef extraLiteralInfo = Load(VariableType::INT64(), method, extraLiteralInfoOffset);
    GateRef builtinsId = Int64And(Int64LSR(extraLiteralInfo, Int64(MethodLiteral::BuiltinIdBits::START_BIT)),
        Int64((1LLU << MethodLiteral::BuiltinIdBits::SIZE) - 1));
    return builtinsId;
}
}
#endif  // ECMASCRIPT_COMPILER_HCR_CIRCUIT_BUILDER_H
