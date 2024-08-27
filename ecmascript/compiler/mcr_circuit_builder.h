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

#ifndef ECMASCRIPT_COMPILER_MCR_CIRCUIT_BUILDER_H
#define ECMASCRIPT_COMPILER_MCR_CIRCUIT_BUILDER_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/method.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::kungfu {

// bit operation

GateRef CircuitBuilder::TaggedIsInt(GateRef x)
{
    x = ChangeTaggedPointerToInt64(x);
    return Equal(Int64And(x, Int64(JSTaggedValue::TAG_MARK)),
                 Int64(JSTaggedValue::TAG_INT));
}

GateRef CircuitBuilder::TaggedIsDouble(GateRef x)
{
    x = ChangeTaggedPointerToInt64(x);
    x = Int64And(x, Int64(JSTaggedValue::TAG_MARK));
    auto left = NotEqual(x, Int64(JSTaggedValue::TAG_INT));
    auto right = NotEqual(x, Int64(JSTaggedValue::TAG_OBJECT));
    return BitAnd(left, right);
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
    return LogicOrBuilder(env_).Or(TaggedIsNumber(x)).Or(TaggedIsBigInt(x)).Done();
}

GateRef CircuitBuilder::TaggedObjectIsString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return BitAnd(
        Int32LessThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::STRING_LAST))),
        Int32GreaterThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::STRING_FIRST))));
}

GateRef CircuitBuilder::TaggedObjectIsShared(GateRef obj)
{
    GateRef bitfield = Load(VariableType::INT32(), LoadHClass(obj), IntPtr(JSHClass::BIT_FIELD_OFFSET));
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::IsJSSharedBit::START_BIT)),
                 Int32((1LU << JSHClass::IsJSSharedBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::TaggedObjectIsEcmaObject(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return BitAnd(
        Int32LessThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::ECMA_OBJECT_LAST))),
        Int32GreaterThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::ECMA_OBJECT_FIRST))));
}

GateRef CircuitBuilder::TaggedObjectIsByteArray(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::BYTE_ARRAY)));
}

GateRef CircuitBuilder::TaggedObjectIsMap(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_MAP)));
}

GateRef CircuitBuilder::TaggedObjectIsDataView(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_DATA_VIEW)));
}

GateRef CircuitBuilder::IsSpecialSlicedString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    GateRef isSlicedString = Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::SLICED_STRING)));
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    Label isSlicedStr(env_);
    BRANCH_CIR2(isSlicedString, &isSlicedStr, &exit);
    Bind(&isSlicedStr);
    {
        GateRef hasBackingStore = LoadConstOffset(VariableType::INT32(), obj, SlicedString::BACKING_STORE_FLAG);
        result = Int32Equal(hasBackingStore, Int32(EcmaString::HAS_BACKING_STORE));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TaggedIsBigInt(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    BRANCH_CIR2(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = Int32Equal(GetObjectType(LoadHClass(obj)),
                            Int32(static_cast<int32_t>(JSType::BIGINT)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TaggedIsString(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    BRANCH_CIR2(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = TaggedObjectIsString(obj);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TaggedIsStringIterator(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = Int32Equal(GetObjectType(LoadHClass(obj)),
                            Int32(static_cast<int32_t>(JSType::JS_STRING_ITERATOR)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TaggedIsSharedObj(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    BRANCH_CIR2(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = TaggedObjectIsShared(obj);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TaggedIsSymbol(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    BRANCH_CIR2(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef objType = GetObjectType(LoadHClass(obj));
        result = Equal(objType, Int32(static_cast<int32_t>(JSType::SYMBOL)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TaggedIsStringOrSymbol(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    BRANCH_CIR2(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = TaggedObjectIsString(obj);
        Label isString(env_);
        Label notString(env_);
        BRANCH_CIR2(*result, &exit, &notString);
        Bind(&notString);
        {
            GateRef objType = GetObjectType(LoadHClass(obj));
            result = Equal(objType, Int32(static_cast<int32_t>(JSType::SYMBOL)));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TaggedIsProtoChangeMarker(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    BRANCH_CIR2(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef objType = GetObjectType(LoadHClass(obj));
        result = Equal(objType, Int32(static_cast<int32_t>(JSType::PROTO_CHANGE_MARKER)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TaggedObjectIsJSMap(GateRef obj)
{
    GateRef objType = GetObjectType(LoadHClass(obj));
    return Equal(objType, Int32(static_cast<int32_t>(JSType::JS_MAP)));
}

GateRef CircuitBuilder::TaggedObjectIsJSSet(GateRef obj)
{
    GateRef objType = GetObjectType(LoadHClass(obj));
    return Equal(objType, Int32(static_cast<int32_t>(JSType::JS_SET)));
}

GateRef CircuitBuilder::TaggedObjectIsJSDate(GateRef obj)
{
    GateRef objType = GetObjectType(LoadHClass(obj));
    return Equal(objType, Int32(static_cast<int32_t>(JSType::JS_DATE)));
}

GateRef CircuitBuilder::TaggedObjectIsTypedArray(GateRef obj)
{
    GateRef jsType = GetObjectType(LoadHClass(obj));
    return BitAnd(Int32GreaterThan(jsType, Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY_FIRST))),
                  Int32GreaterThanOrEqual(Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY_LAST)), jsType));
}

GateRef CircuitBuilder::TaggedObjectIsJSArray(GateRef obj)
{
    GateRef objType = GetObjectType(LoadHClass(obj));
    return Equal(objType, Int32(static_cast<int32_t>(JSType::JS_ARRAY)));
}

GateRef CircuitBuilder::TaggedIsBoundFunction(GateRef obj)
{
    GateRef objType = GetObjectType(LoadHClass(obj));
    return Equal(objType, Int32(static_cast<int32_t>(JSType::JS_BOUND_FUNCTION)));
}

inline GateRef CircuitBuilder::JudgeAotAndFastCall(GateRef jsFunc, JudgeMethodType type)
{
    GateRef bitFieldOffset = IntPtr(JSFunctionBase::BIT_FIELD_OFFSET);
    GateRef bitField = Load(VariableType::INT32(), jsFunc, bitFieldOffset);
    switch (type) {
        case JudgeMethodType::HAS_AOT: {
            return Int32NotEqual(
                Int32And(
                    Int32LSR(bitField, Int32(JSFunctionBase::IsCompiledCodeBit::START_BIT)),
                    Int32((1U << JSFunctionBase::IsCompiledCodeBit::SIZE) - 1)),
                Int32(0));
        }
        case JudgeMethodType::HAS_AOT_FASTCALL: {
            return Int32Equal(
                Int32And(
                    bitField,
                    Int32(JSFunctionBase::COMPILED_CODE_FASTCALL_BITS << JSFunctionBase::IsCompiledCodeBit::START_BIT)),
                Int32(JSFunctionBase::COMPILED_CODE_FASTCALL_BITS << JSFunctionBase::IsCompiledCodeBit::START_BIT));
        }
        case JudgeMethodType::HAS_AOT_NOTFASTCALL: {
            GateRef fastCallField =
                Int32And(bitField,
                    Int32(JSFunctionBase::COMPILED_CODE_FASTCALL_BITS << JSFunctionBase::IsCompiledCodeBit::START_BIT));
            GateRef hasAot = Int32(1U << JSFunctionBase::IsCompiledCodeBit::START_BIT);
            return Int32Equal(fastCallField, hasAot);
        }
        default:
            UNREACHABLE();
    }
}

GateRef CircuitBuilder::BothAreString(GateRef x, GateRef y)
{
    return LogicAndBuilder(env_).And(TaggedIsHeapObject(x)).And(TaggedIsHeapObject(y))
        .And(TaggedObjectIsString(x)).And(TaggedObjectIsString(y)).Done();
}

GateRef CircuitBuilder::TaggedIsHole(GateRef x)
{
    return Equal(x, HoleConstant());
}

GateRef CircuitBuilder::TaggedIsNullPtr(GateRef x)
{
    return Equal(x, NullPtrConstant());
}

GateRef CircuitBuilder::IsSpecialHole(GateRef x)
{
    return Equal(x, SpecialHoleConstant());
}

GateRef CircuitBuilder::IsNotSpecialHole(GateRef x)
{
    return NotEqual(x, SpecialHoleConstant());
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
    return LogicOrBuilder(env_)
        .Or(TaggedIsHole(x))
        .Or(Equal(Int64And(ChangeTaggedPointerToInt64(x), Int64(JSTaggedValue::TAG_SPECIAL_MASK)),
                  Int64(JSTaggedValue::TAG_SPECIAL)))
        .Done();
}

GateRef CircuitBuilder::TaggedIsHeapObject(GateRef x)
{
    x = ChangeTaggedPointerToInt64(x);
    auto t = Int64And(x, Int64(JSTaggedValue::TAG_HEAPOBJECT_MASK), GateType::Empty(), "checkHeapObject");
    return Equal(t, Int64(0), "checkHeapObject");
}

GateRef CircuitBuilder::TaggedIsAsyncGeneratorObject(GateRef x)
{
    return LogicAndBuilder(env_).And(TaggedIsHeapObject(x)).And(IsJsType(x, JSType::JS_ASYNC_GENERATOR_OBJECT)).Done();
}

GateRef CircuitBuilder::TaggedIsJSGlobalObject(GateRef x)
{
    return LogicAndBuilder(env_).And(TaggedIsHeapObject(x)).And(IsJsType(x, JSType::JS_GLOBAL_OBJECT)).Done();
}

GateRef CircuitBuilder::TaggedIsGeneratorObject(GateRef x)
{
    return LogicAndBuilder(env_).And(TaggedIsHeapObject(x)).And(IsJsType(x, JSType::JS_GENERATOR_OBJECT)).Done();
}

GateRef CircuitBuilder::TaggedIsJSArray(GateRef x)
{
    return LogicAndBuilder(env_).And(TaggedIsHeapObject(x)).And(IsJsType(x, JSType::JS_ARRAY)).Done();
}

GateRef CircuitBuilder::TaggedIsPropertyBox(GateRef x)
{
    return LogicAndBuilder(env_).And(TaggedIsHeapObject(x)).And(IsJsType(x, JSType::PROPERTY_BOX)).Done();
}

GateRef CircuitBuilder::TaggedIsWeak(GateRef x)
{
    return LogicAndBuilder(env_)
        .And(TaggedIsHeapObject(x))
        .And(Equal(Int64And(ChangeTaggedPointerToInt64(x), Int64(JSTaggedValue::TAG_WEAK)), Int64(1)))
        .Done();
}

GateRef CircuitBuilder::TaggedIsPrototypeHandler(GateRef x)
{
    return LogicAndBuilder(env_).And(TaggedIsHeapObject(x)).And(IsJsType(x, JSType::PROTOTYPE_HANDLER)).Done();
}

GateRef CircuitBuilder::TaggedIsTransitionHandler(GateRef x)
{
    return LogicAndBuilder(env_).And(TaggedIsHeapObject(x)).And(IsJsType(x, JSType::TRANSITION_HANDLER)).Done();
}

GateRef CircuitBuilder::TaggedIsStoreTSHandler(GateRef x)
{
    return LogicAndBuilder(env_).And(TaggedIsHeapObject(x)).And(IsJsType(x, JSType::STORE_TS_HANDLER)).Done();
}

GateRef CircuitBuilder::TaggedIsTransWithProtoHandler(GateRef x)
{
    return LogicAndBuilder(env_).And(TaggedIsHeapObject(x)).And(IsJsType(x, JSType::TRANS_WITH_PROTO_HANDLER)).Done();
}

GateRef CircuitBuilder::TaggedIsUndefinedOrNull(GateRef x)
{
    x = ChangeTaggedPointerToInt64(x);
    GateRef heapObjMask = Int64(JSTaggedValue::TAG_HEAPOBJECT_MASK);
    GateRef tagSpecial = Int64(JSTaggedValue::TAG_SPECIAL);
    GateRef andGate = Int64And(x, heapObjMask);
    GateRef result = Equal(andGate, tagSpecial);
    return result;
}

GateRef CircuitBuilder::TaggedIsUndefinedOrNullOrHole(GateRef x)
{
    return BitOr(TaggedIsHole(x), TaggedIsUndefinedOrNull(x));
}

GateRef CircuitBuilder::TaggedIsNotUndefinedAndNull(GateRef x)
{
    x = ChangeTaggedPointerToInt64(x);
    GateRef heapObjMask = Int64(JSTaggedValue::TAG_HEAPOBJECT_MASK);
    GateRef tagSpecial = Int64(JSTaggedValue::TAG_SPECIAL);
    GateRef andGate = Int64And(x, heapObjMask);
    GateRef result = NotEqual(andGate, tagSpecial);
    return result;
}

GateRef CircuitBuilder::TaggedIsNotUndefinedAndNullAndHole(GateRef x)
{
    return BitAnd(TaggedIsNotHole(x), TaggedIsNotUndefinedAndNull(x));
}

GateRef CircuitBuilder::TaggedIsUndefinedOrHole(GateRef x)
{
    GateRef isUndefined = TaggedIsUndefined(x);
    GateRef isHole = TaggedIsHole(x);
    GateRef result = BitOr(isHole, isUndefined);
    return result;
}

GateRef CircuitBuilder::TaggedTrue()
{
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_TRUE, GateType::TaggedValue());
}

GateRef CircuitBuilder::TaggedFalse()
{
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_FALSE, GateType::TaggedValue());
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

GateRef CircuitBuilder::TaggedIsNotNull(GateRef x)
{
    return NotEqual(x, NullConstant());
}

GateRef CircuitBuilder::TaggedIsBoolean(GateRef x)
{
    return BitOr(TaggedIsFalse(x), TaggedIsTrue(x));
}

GateRef CircuitBuilder::TaggedGetInt(GateRef x)
{
    x = ChangeTaggedPointerToInt64(x);
    return TruncInt64ToInt32(Int64And(x, Int64(~JSTaggedValue::TAG_MARK)));
}

inline GateRef CircuitBuilder::TypedCallBuiltin(GateRef hirGate, const std::vector<GateRef> &args,
                                                BuiltinsStubCSigns::ID id, bool isSideEffect)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();

    std::vector<GateRef> inList { currentControl, currentDepend };
    inList.insert(inList.end(), args.begin(), args.end());
    inList.push_back(Int16(static_cast<int16_t>(id)));
    AppendFrameArgs(inList, hirGate);

    auto builtinOp = TypedCallOperator(hirGate, MachineType::I64, inList, isSideEffect);
    currentLabel->SetControl(builtinOp);
    currentLabel->SetDepend(builtinOp);
    return builtinOp;
}

template<TypedBinOp Op>
GateRef CircuitBuilder::TypedBinaryOp(GateRef x, GateRef y, ParamType paramType)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    uint64_t value = TypedBinaryAccessor::ToValue(paramType, Op);
    auto numberBinaryOp = GetCircuit()->NewGate(circuit_->TypedBinaryOp(value),
        MachineType::I64, {currentControl, currentDepend, x, y}, GateType::AnyType());
    currentLabel->SetControl(numberBinaryOp);
    currentLabel->SetDepend(numberBinaryOp);
    return numberBinaryOp;
}

template<TypedCallTargetCheckOp Op>
GateRef CircuitBuilder::JSNoGCCallThisTargetTypeCheck(GateRef func, GateRef methodId, GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.GetFrameState(gate);
    uint64_t value = TypedCallTargetCheckAccessor::ToValue(Op);
    GateRef ret = GetCircuit()->NewGate(circuit_->TypedCallTargetCheckOp(value), MachineType::I1,
        {currentControl, currentDepend, func, methodId, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

template<TypedCallTargetCheckOp Op>
GateRef CircuitBuilder::JSCallTargetTypeCheck(GateRef func, GateRef methodIndex, GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.GetFrameState(gate);
    uint64_t value = TypedCallTargetCheckAccessor::ToValue(Op);
    GateRef ret = GetCircuit()->NewGate(circuit_->TypedCallTargetCheckOp(value), MachineType::I1,
        {currentControl, currentDepend, func, methodIndex, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

template<TypedUnOp Op>
GateRef CircuitBuilder::TypedUnaryOp(GateRef x, ParamType paramType)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    uint64_t value = TypedUnaryAccessor::ToValue(paramType, Op);
    auto numberUnaryOp = GetCircuit()->NewGate(circuit_->TypedUnaryOp(value),
        MachineType::I64, {currentControl, currentDepend, x}, GateType::AnyType());
    currentLabel->SetControl(numberUnaryOp);
    currentLabel->SetDepend(numberUnaryOp);
    return numberUnaryOp;
}

template<TypedJumpOp Op>
GateRef CircuitBuilder::TypedConditionJump(GateRef x, ParamType paramType, uint32_t weight)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto machineType = MachineType::NOVALUE;
    auto jumpOp = TypedConditionJump(machineType, Op, weight, paramType, {currentControl, currentDepend, x});
    currentLabel->SetControl(jumpOp);
    currentLabel->SetDepend(jumpOp);
    return jumpOp;
}

template <TypedLoadOp Op>
GateRef CircuitBuilder::LoadElement(GateRef receiver, GateRef index, OnHeapMode onHeap)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    LoadElementAccessor accessor(Op, onHeap);
    auto ret = GetCircuit()->NewGate(GetCircuit()->LoadElement(accessor.ToValue()), MachineType::I64,
                                     {currentControl, currentDepend, receiver, index}, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

template <TypedStoreOp Op>
GateRef CircuitBuilder::StoreElement(GateRef receiver, GateRef index, GateRef value, OnHeapMode onHeap)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    StoreElementAccessor accessor(Op, onHeap);
    auto ret = GetCircuit()->NewGate(GetCircuit()->StoreElement(accessor.ToValue()), MachineType::NOVALUE,
                                     {currentControl, currentDepend, receiver, index, value}, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::PrimitiveToNumber(GateRef x, ParamType paramType)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();

    auto numberconvert = TypeConvert(MachineType::I64, paramType, GateType::NumberType(),
                                     {currentControl, currentDepend, x});
    currentLabel->SetControl(numberconvert);
    currentLabel->SetDepend(numberconvert);
    return numberconvert;
}

GateRef CircuitBuilder::LoadFromTaggedArray(GateRef array, size_t index)
{
    auto dataOffset = TaggedArray::DATA_OFFSET + index * JSTaggedValue::TaggedTypeSize();
    return LoadConstOffset(VariableType::JS_ANY(), array, dataOffset);
}

GateRef CircuitBuilder::StoreToTaggedArray(GateRef array, size_t index, GateRef value)
{
    auto dataOffset = TaggedArray::DATA_OFFSET + index * JSTaggedValue::TaggedTypeSize();
    return StoreConstOffset(VariableType::JS_ANY(), array, dataOffset, value);
}

GateRef CircuitBuilder::TreeStringIsFlat(GateRef string)
{
    GateRef second = GetSecondFromTreeString(string);
    GateRef len = GetLengthFromString(second);
    return Int32Equal(len, Int32(0));
}

GateRef CircuitBuilder::GetFirstFromTreeString(GateRef string)
{
    GateRef offset = IntPtr(TreeEcmaString::FIRST_OFFSET);
    return Load(VariableType::JS_POINTER(), string, offset);
}

GateRef CircuitBuilder::GetSecondFromTreeString(GateRef string)
{
    GateRef offset = IntPtr(TreeEcmaString::SECOND_OFFSET);
    return Load(VariableType::JS_POINTER(), string, offset);
}

GateRef CircuitBuilder::GetValueFromTaggedArray(GateRef array, GateRef index)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    return Load(VariableType::JS_ANY(), array, dataOffset);
}

GateRef CircuitBuilder::GetValueFromTaggedArray(VariableType valType, GateRef array, GateRef index)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    return Load(valType, array, dataOffset);
}

GateRef CircuitBuilder::GetValueFromJSArrayWithElementsKind(VariableType type, GateRef array, GateRef index)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    return Load(type, array, dataOffset);
}

void CircuitBuilder::SetValueToTaggedArray(VariableType valType, GateRef glue,
                                           GateRef array, GateRef index, GateRef val)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    Store(valType, glue, array, dataOffset, val);
}
}

#endif  // ECMASCRIPT_COMPILER_MCR_CIRCUIT_BUILDER_H
