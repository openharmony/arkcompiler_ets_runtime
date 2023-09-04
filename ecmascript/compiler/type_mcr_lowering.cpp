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

#include "ecmascript/compiler/type_mcr_lowering.h"
#include "ecmascript/compiler/builtins_lowering.h"
#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/subtyping_operator.h"
#include "ecmascript/vtable.h"
#include "ecmascript/message_string.h"
namespace panda::ecmascript::kungfu {

GateRef TypeMCRLowering::VisitGate(GateRef gate)
{
    GateRef glue = acc_.GetGlueFromArgList();
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::PRIMITIVE_TYPE_CHECK:
            LowerPrimitiveTypeCheck(gate);
            break;
        case OpCode::STABLE_ARRAY_CHECK:
            LowerStableArrayCheck(gate);
            break;
        case OpCode::TYPED_ARRAY_CHECK:
            LowerTypedArrayCheck(gate);
            break;
        case OpCode::ECMA_STRING_CHECK:
            LowerEcmaStringCheck(gate);
            break;
        case OpCode::FLATTEN_STRING_CHECK:
            LowerFlattenStringCheck(gate, glue);
            break;
        case OpCode::LOAD_STRING_LENGTH:
            LowerStringLength(gate);
            break;
        case OpCode::LOAD_TYPED_ARRAY_LENGTH:
            LowerLoadTypedArrayLength(gate);
            break;
        case OpCode::OBJECT_TYPE_CHECK:
            LowerObjectTypeCheck(gate);
            break;
        case OpCode::OBJECT_TYPE_COMPARE:
            LowerObjectTypeCompare(gate);
            break;
        case OpCode::INDEX_CHECK:
            LowerIndexCheck(gate);
            break;
        case OpCode::TYPED_CALLTARGETCHECK_OP:
            LowerJSCallTargetCheck(gate);
            break;
        case OpCode::TYPED_CALL_CHECK:
            LowerCallTargetCheck(gate);
            break;
        case OpCode::JSINLINETARGET_TYPE_CHECK:
            LowerJSInlineTargetTypeCheck(gate);
            break;
        case OpCode::TYPE_CONVERT:
            LowerTypeConvert(gate);
            break;
        case OpCode::LOAD_PROPERTY:
            LowerLoadProperty(gate);
            break;
        case OpCode::CALL_GETTER:
            LowerCallGetter(gate, glue);
            break;
        case OpCode::STORE_PROPERTY:
        case OpCode::STORE_PROPERTY_NO_BARRIER:
            LowerStoreProperty(gate);
            break;
        case OpCode::CALL_SETTER:
            LowerCallSetter(gate, glue);
            break;
        case OpCode::LOAD_ARRAY_LENGTH:
            LowerLoadArrayLength(gate);
            break;
        case OpCode::LOAD_ELEMENT:
            LowerLoadElement(gate);
            break;
        case OpCode::STORE_ELEMENT:
            LowerStoreElement(gate, glue);
            break;
        case OpCode::TYPED_CALL_BUILTIN:
            LowerTypedCallBuitin(gate);
            break;
        case OpCode::TYPED_NEW_ALLOCATE_THIS:
            LowerTypedNewAllocateThis(gate, glue);
            break;
        case OpCode::TYPED_SUPER_ALLOCATE_THIS:
            LowerTypedSuperAllocateThis(gate, glue);
            break;
        case OpCode::GET_SUPER_CONSTRUCTOR:
            LowerGetSuperConstructor(gate);
            break;
        case OpCode::COW_ARRAY_CHECK:
            LowerCowArrayCheck(gate, glue);
            break;
        case OpCode::LOAD_GETTER:
            LowerLoadGetter(gate);
            break;
        case OpCode::LOAD_SETTER:
            LowerLoadSetter(gate);
            break;
        case OpCode::INLINE_ACCESSOR_CHECK:
            LowerInlineAccessorCheck(gate);
            break;
        default:
            break;
    }
    return Circuit::NullGate();
}

void TypeMCRLowering::LowerJSCallTargetCheck(GateRef gate)
{
    TypedCallTargetCheckOp Op = acc_.GetTypedCallTargetCheckOp(gate);
    switch (Op) {
        case TypedCallTargetCheckOp::JSCALL_IMMEDIATE_AFTER_FUNC_DEF: {
            LowerJSCallTargetFromDefineFuncCheck(gate);
            break;
        }
        case TypedCallTargetCheckOp::JSCALL: {
            LowerJSCallTargetTypeCheck(gate);
            break;
        }
        case TypedCallTargetCheckOp::JSCALL_FAST: {
            LowerJSFastCallTargetTypeCheck(gate);
            break;
        }
        case TypedCallTargetCheckOp::JSCALLTHIS: {
            LowerJSCallThisTargetTypeCheck(gate);
            break;
        }
        case TypedCallTargetCheckOp::JSCALLTHIS_FAST: {
            LowerJSFastCallThisTargetTypeCheck(gate);
            break;
        }
        case TypedCallTargetCheckOp::JSCALLTHIS_NOGC: {
            LowerJSNoGCCallThisTargetTypeCheck(gate);
            break;
        }
        case TypedCallTargetCheckOp::JSCALLTHIS_FAST_NOGC: {
            LowerJSNoGCFastCallThisTargetTypeCheck(gate);
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void TypeMCRLowering::LowerPrimitiveTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (type.IsIntType()) {
        LowerIntCheck(gate);
    } else if (type.IsDoubleType()) {
        LowerDoubleCheck(gate);
    } else if (type.IsNumberType()) {
        LowerNumberCheck(gate);
    } else if (type.IsBooleanType()) {
        LowerBooleanCheck(gate);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeMCRLowering::LowerIntCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsInt(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTINT);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerDoubleCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsDouble(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTDOUBLE);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerNumberCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsNumber(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTNUMBER);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerBooleanCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsBoolean(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTBOOL);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerStableArrayCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);

    GateRef receiver = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(receiver, frameState);

    GateRef receiverHClass = builder_.LoadConstOffset(
        VariableType::JS_POINTER(), receiver, TaggedObject::HCLASS_OFFSET);
    ArrayMetaDataAccessor accessor = acc_.GetArrayMetaDataAccessor(gate);
    builder_.HClassStableArrayCheck(receiverHClass, frameState, accessor);
    builder_.ArrayGuardianCheck(frameState);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::SetDeoptTypeInfo(BuiltinTypeId id, DeoptType &type, size_t &funcIndex)
{
    type = DeoptType::NOTARRAY;
    switch (id) {
        case BuiltinTypeId::INT8_ARRAY:
            funcIndex = GlobalEnv::INT8_ARRAY_FUNCTION_INDEX;
            break;
        case BuiltinTypeId::UINT8_ARRAY:
            funcIndex = GlobalEnv::UINT8_ARRAY_FUNCTION_INDEX;
            break;
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            funcIndex = GlobalEnv::UINT8_CLAMPED_ARRAY_FUNCTION_INDEX;
            break;
        case BuiltinTypeId::INT16_ARRAY:
            funcIndex = GlobalEnv::INT16_ARRAY_FUNCTION_INDEX;
            break;
        case BuiltinTypeId::UINT16_ARRAY:
            funcIndex = GlobalEnv::UINT16_ARRAY_FUNCTION_INDEX;
            break;
        case BuiltinTypeId::INT32_ARRAY:
            funcIndex = GlobalEnv::INT32_ARRAY_FUNCTION_INDEX;
            break;
        case BuiltinTypeId::UINT32_ARRAY:
            funcIndex = GlobalEnv::UINT32_ARRAY_FUNCTION_INDEX;
            break;
        case BuiltinTypeId::FLOAT32_ARRAY:
            funcIndex = GlobalEnv::FLOAT32_ARRAY_FUNCTION_INDEX;
            break;
        case BuiltinTypeId::FLOAT64_ARRAY:
            funcIndex = GlobalEnv::FLOAT64_ARRAY_FUNCTION_INDEX;
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void TypeMCRLowering::LowerTypedArrayCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    size_t typedArrayFuncIndex = GlobalEnv::TYPED_ARRAY_FUNCTION_INDEX;
    auto deoptType = DeoptType::NOTCHECK;

    auto builtinTypeId = tsManager_->GetTypedArrayBuiltinId(type);
    SetDeoptTypeInfo(builtinTypeId, deoptType, typedArrayFuncIndex);

    GateRef frameState = GetFrameState(gate);
    GateRef glueGlobalEnv = builder_.GetGlobalEnv();
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef receiverHClass = builder_.LoadHClass(receiver);
    GateRef protoOrHclass = builder_.GetGlobalEnvObjHClass(glueGlobalEnv, typedArrayFuncIndex);
    GateRef check = builder_.Equal(receiverHClass, protoOrHclass);
    builder_.DeoptCheck(check, frameState, deoptType);

    if (IsOnHeap()) {
        GateRef isOnHeap = builder_.LoadConstOffset(VariableType::BOOL(), receiver, JSTypedArray::ON_HEAP_OFFSET);
        builder_.DeoptCheck(isOnHeap, frameState, DeoptType::NOTONHEAP);
    }

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerEcmaStringCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(receiver, frameState);
    GateRef isString = builder_.TaggedObjectIsString(receiver);
    builder_.DeoptCheck(isString, frameState, DeoptType::NOTSTRING);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerFlattenStringCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef str = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_POINTER(), str);
    Label isTreeString(&builder_);
    Label exit(&builder_);

    builder_.Branch(builder_.IsTreeString(str), &isTreeString, &exit);
    builder_.Bind(&isTreeString);
    {
        Label isFlat(&builder_);
        Label notFlat(&builder_);
        builder_.Branch(builder_.TreeStringIsFlat(str), &isFlat, &notFlat);
        builder_.Bind(&isFlat);
        {
            result = builder_.GetFirstFromTreeString(str);
            builder_.Jump(&exit);
        }
        builder_.Bind(&notFlat);
        {
            result = LowerCallRuntime(glue, gate, RTSTUB_ID(SlowFlattenString), { str }, true);
            builder_.Jump(&exit);
        }
    }

    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeMCRLowering::LowerStringLength(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef length = builder_.Int32LSR(
        builder_.LoadConstOffset(VariableType::INT32(), receiver, EcmaString::MIX_LENGTH_OFFSET), builder_.Int32(2));

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), length);
}

void TypeMCRLowering::LowerLoadTypedArrayLength(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef length = builder_.LoadConstOffset(VariableType::INT32(), receiver, JSTypedArray::ARRAY_LENGTH_OFFSET);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), length);
}

void TypeMCRLowering::LowerObjectTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateType type = acc_.GetObjectTypeAccessor(gate).GetType();
    if (tsManager_->IsClassInstanceTypeKind(type)) {
        LowerTSSubtypingCheck(gate);
    } else if (tsManager_->IsClassTypeKind(type) ||
               tsManager_->IsObjectTypeKind(type)) {
        LowerSimpleHClassCheck(gate);
    } else {
        LOG_COMPILER(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeMCRLowering::LowerTSSubtypingCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);
    Label levelValid(&builder_);
    Label exit(&builder_);
    GateRef compare = BuildCompareSubTyping(gate, frameState, &levelValid, &exit);
    builder_.DeoptCheck(compare, frameState, DeoptType::INCONSISTENTHCLASS);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerSimpleHClassCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);
    GateRef compare = BuildCompareHClass(gate, frameState);
    builder_.DeoptCheck(compare, frameState, DeoptType::INCONSISTENTHCLASS);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerObjectTypeCompare(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetObjectTypeAccessor(gate).GetType();
    if (tsManager_->IsClassInstanceTypeKind(type)) {
        LowerTSSubtypingCompare(gate);
    } else if (tsManager_->IsClassTypeKind(type) ||
               tsManager_->IsObjectTypeKind(type)) {
        LowerSimpleHClassCompare(gate);
    } else {
        LOG_COMPILER(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeMCRLowering::LowerSimpleHClassCompare(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);
    GateRef compare = BuildCompareHClass(gate, frameState) ;
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), compare);
}

void TypeMCRLowering::LowerTSSubtypingCompare(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);
    Label levelValid(&builder_);
    Label exit(&builder_);
    GateRef compare = BuildCompareSubTyping(gate, frameState, &levelValid, &exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), compare);
}

GateRef TypeMCRLowering::BuildCompareSubTyping(GateRef gate, GateRef frameState, Label *levelValid, Label *exit)
{
    GateRef receiver = acc_.GetValueIn(gate, 0);
    bool isHeapObject = acc_.GetObjectTypeAccessor(gate).IsHeapObject();
    if (!isHeapObject) {
        builder_.HeapObjectCheck(receiver, frameState);
    }

    GateRef aotHCIndex = acc_.GetValueIn(gate, 1);
    ArgumentAccessor argAcc(circuit_);
    GateRef jsFunc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
    JSTaggedValue aotHC = tsManager_->GetValueFromCache(acc_.TryGetValue(aotHCIndex));
    ASSERT(aotHC.IsJSHClass());

    int32_t level = JSHClass::Cast(aotHC.GetTaggedObject())->GetLevel();
    ASSERT(level >= 0);

    GateRef receiverHClass = builder_.LoadConstOffset(
        VariableType::JS_POINTER(), receiver, TaggedObject::HCLASS_OFFSET);
    GateRef supers = LoadSupers(receiverHClass);

    auto hclassIndex = acc_.GetConstantValue(aotHCIndex);
    GateRef aotHCGate = LoadFromConstPool(jsFunc, hclassIndex);

    if (LIKELY(static_cast<uint32_t>(level) < SubtypingOperator::DEFAULT_SUPERS_CAPACITY)) {
        return builder_.Equal(aotHCGate, GetValueFromSupers(supers, level));
    }

    DEFVAlUE(check, (&builder_), VariableType::BOOL(), builder_.False());
    GateRef levelGate = builder_.Int32(level);
    GateRef length = GetLengthFromSupers(supers);

    builder_.Branch(builder_.Int32LessThan(levelGate, length), levelValid, exit,
        BranchWeight::DEOPT_WEIGHT, BranchWeight::ONE_WEIGHT);
    builder_.Bind(levelValid);
    {
        check = builder_.Equal(aotHCGate, GetValueFromSupers(supers, level));
        builder_.Jump(exit);
    }
    builder_.Bind(exit);

    return *check;
}

GateRef TypeMCRLowering::BuildCompareHClass(GateRef gate, GateRef frameState)
{
    GateRef receiver = acc_.GetValueIn(gate, 0);
    bool isHeapObject = acc_.GetObjectTypeAccessor(gate).IsHeapObject();
    if (!isHeapObject) {
        builder_.HeapObjectCheck(receiver, frameState);
    }

    GateRef aotHCIndex = acc_.GetValueIn(gate, 1);
    auto hclassIndex = acc_.GetConstantValue(aotHCIndex);
    ArgumentAccessor argAcc(circuit_);
    GateRef jsFunc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
    GateRef aotHCGate = LoadFromConstPool(jsFunc, hclassIndex);
    GateRef receiverHClass = builder_.LoadConstOffset(
        VariableType::JS_POINTER(), receiver, TaggedObject::HCLASS_OFFSET);
    return builder_.Equal(aotHCGate, receiverHClass);
}

void TypeMCRLowering::LowerIndexCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto deoptType = DeoptType::NOTLEGALIDX;

    GateRef frameState = GetFrameState(gate);
    GateRef length = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    ASSERT(acc_.GetGateType(length).IsNJSValueType());
    // UnsignedLessThan can check both lower and upper bounds
    GateRef lengthCheck = builder_.Int32UnsignedLessThan(index, length);
    builder_.DeoptCheck(lengthCheck, frameState, deoptType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), index);
}

GateRef TypeMCRLowering::LowerCallRuntime(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
                                          bool useLabel)
{
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args, hirGate);
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, dependEntry_, args, hirGate);
        return result;
    }
}

void TypeMCRLowering::LowerTypeConvert(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (rightType.IsNumberType()) {
        GateRef value = acc_.GetValueIn(gate, 0);
        if (leftType.IsDigitablePrimitiveType()) {
            LowerPrimitiveToNumber(gate, value, leftType);
        }
        return;
    }
}

void TypeMCRLowering::LowerPrimitiveToNumber(GateRef dst, GateRef src, GateType srcType)
{
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    if (srcType.IsBooleanType()) {
        Label exit(&builder_);
        Label isTrue(&builder_);
        Label isFalse(&builder_);
        builder_.Branch(builder_.TaggedIsTrue(src), &isTrue, &isFalse);
        builder_.Bind(&isTrue);
        {
            result = IntToTaggedIntPtr(builder_.Int32(1));
            builder_.Jump(&exit);
        }
        builder_.Bind(&isFalse);
        {
            result = IntToTaggedIntPtr(builder_.Int32(0));
            builder_.Jump(&exit);
        }
        builder_.Bind(&exit);
    } else if (srcType.IsUndefinedType()) {
        result = DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
    } else if (srcType.IsBigIntType() || srcType.IsNumberType()) {
        result = src;
    } else if (srcType.IsNullType()) {
        result = IntToTaggedIntPtr(builder_.Int32(0));
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    acc_.ReplaceGate(dst, builder_.GetState(), builder_.GetDepend(), *result);
}

GateRef TypeMCRLowering::LoadFromConstPool(GateRef jsFunc, size_t index)
{
    GateRef constPool = builder_.GetConstPool(jsFunc);
    return builder_.LoadFromTaggedArray(constPool, index);
}

GateRef TypeMCRLowering::GetObjectFromConstPool(GateRef jsFunc, GateRef index)
{
    GateRef constPool = builder_.GetConstPool(jsFunc);
    return builder_.GetValueFromTaggedArray(constPool, index);
}

void TypeMCRLowering::LowerLoadProperty(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: receiver, plr
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsLocal() || plr.IsFunction());

    GateRef result = Circuit::NullGate();
    if (plr.IsNotHole()) {
        ASSERT(plr.IsLocal());
        result = builder_.LoadConstOffset(VariableType::JS_ANY(), receiver, plr.GetOffset());
    } else if (plr.IsLocal()) {
        result = builder_.LoadConstOffset(VariableType::JS_ANY(), receiver, plr.GetOffset());
        result = builder_.ConvertHoleAsUndefined(result);
    } else {
        result = LoadFromVTable(receiver, plr.GetOffset());
    }

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeMCRLowering::LowerCallGetter(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: receiver, plr
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);

    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsAccessor());
    GateRef accessor = LoadFromVTable(receiver, plr.GetOffset());
    GateRef getter = builder_.LoadConstOffset(VariableType::JS_ANY(), accessor, AccessorData::GETTER_OFFSET);

    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    Label callGetter(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.IsSpecial(getter, JSTaggedValue::VALUE_UNDEFINED), &exit, &callGetter);
    builder_.Bind(&callGetter);
    {
        result = CallAccessor(glue, gate, getter, receiver, AccessorMode::GETTER);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    ReplaceHirWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeMCRLowering::LowerStoreProperty(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 3);  // 3: receiver, plr, value
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2); // 2: value
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsLocal());
    auto op = OpCode(acc_.GetOpCode(gate));
    if (op == OpCode::STORE_PROPERTY) {
        builder_.StoreConstOffset(VariableType::JS_ANY(), receiver, plr.GetOffset(), value);
    } else if (op == OpCode::STORE_PROPERTY_NO_BARRIER) {
        builder_.StoreConstOffset(GetVarType(plr), receiver, plr.GetOffset(), value);
    } else {
        UNREACHABLE();
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerCallSetter(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 3);  // 3: receiver, plr, value
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);

    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsAccessor());
    GateRef accessor = LoadFromVTable(receiver, plr.GetOffset());
    GateRef setter = builder_.LoadConstOffset(VariableType::JS_ANY(), accessor, AccessorData::SETTER_OFFSET);

    Label callSetter(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.IsSpecial(setter, JSTaggedValue::VALUE_UNDEFINED), &exit, &callSetter);
    builder_.Bind(&callSetter);
    {
        CallAccessor(glue, gate, setter, receiver, AccessorMode::SETTER, value);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    ReplaceHirWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerLoadArrayLength(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef array = acc_.GetValueIn(gate, 0);
    GateRef result = builder_.LoadConstOffset(VariableType::INT32(), array, JSArray::LENGTH_OFFSET);
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

GateRef TypeMCRLowering::GetElementSize(BuiltinTypeId id)
{
    GateRef elementSize = Circuit::NullGate();
    switch (id) {
        case BuiltinTypeId::INT8_ARRAY:
        case BuiltinTypeId::UINT8_ARRAY:
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            elementSize = builder_.Int32(sizeof(uint8_t));
            break;
        case BuiltinTypeId::INT16_ARRAY:
        case BuiltinTypeId::UINT16_ARRAY:
            elementSize = builder_.Int32(sizeof(uint16_t));
            break;
        case BuiltinTypeId::INT32_ARRAY:
        case BuiltinTypeId::UINT32_ARRAY:
        case BuiltinTypeId::FLOAT32_ARRAY:
            elementSize = builder_.Int32(sizeof(uint32_t));
            break;
        case BuiltinTypeId::FLOAT64_ARRAY:
            elementSize = builder_.Int32(sizeof(double));
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return elementSize;
}

VariableType TypeMCRLowering::GetVariableType(BuiltinTypeId id)
{
    VariableType type = VariableType::JS_ANY();
    switch (id) {
        case BuiltinTypeId::INT8_ARRAY:
        case BuiltinTypeId::UINT8_ARRAY:
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            type = VariableType::INT8();
            break;
        case BuiltinTypeId::INT16_ARRAY:
        case BuiltinTypeId::UINT16_ARRAY:
            type = VariableType::INT16();
            break;
        case BuiltinTypeId::INT32_ARRAY:
        case BuiltinTypeId::UINT32_ARRAY:
            type = VariableType::INT32();
            break;
        case BuiltinTypeId::FLOAT32_ARRAY:
            type = VariableType::FLOAT32();
            break;
        case BuiltinTypeId::FLOAT64_ARRAY:
            type = VariableType::FLOAT64();
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return type;
}

void TypeMCRLowering::LowerLoadElement(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto op = acc_.GetTypedLoadOp(gate);
    switch (op) {
        case TypedLoadOp::ARRAY_LOAD_INT_ELEMENT:
        case TypedLoadOp::ARRAY_LOAD_DOUBLE_ELEMENT:
        case TypedLoadOp::ARRAY_LOAD_OBJECT_ELEMENT:
        case TypedLoadOp::ARRAY_LOAD_TAGGED_ELEMENT:
            LowerArrayLoadElement(gate, ArrayState::PACKED);
            break;
        case TypedLoadOp::ARRAY_LOAD_HOLE_TAGGED_ELEMENT:
            LowerArrayLoadElement(gate, ArrayState::HOLEY);
            break;
        case TypedLoadOp::INT8ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::INT8_ARRAY);
            break;
        case TypedLoadOp::UINT8ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::UINT8_ARRAY);
            break;
        case TypedLoadOp::UINT8CLAMPEDARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::UINT8_CLAMPED_ARRAY);
            break;
        case TypedLoadOp::INT16ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::INT16_ARRAY);
            break;
        case TypedLoadOp::UINT16ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::UINT16_ARRAY);
            break;
        case TypedLoadOp::INT32ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::INT32_ARRAY);
            break;
        case TypedLoadOp::UINT32ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::UINT32_ARRAY);
            break;
        case TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::FLOAT32_ARRAY);
            break;
        case TypedLoadOp::FLOAT64ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::FLOAT64_ARRAY);
            break;
        case TypedLoadOp::STRING_LOAD_ELEMENT:
            LowerStringLoadElement(gate);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void TypeMCRLowering::LowerCowArrayCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    Label notCOWArray(&builder_);
    Label isCOWArray(&builder_);
    builder_.Branch(builder_.IsJsCOWArray(receiver), &isCOWArray, &notCOWArray);
    builder_.Bind(&isCOWArray);
    {
        LowerCallRuntime(glue, gate, RTSTUB_ID(CheckAndCopyArray), {receiver}, true);
        builder_.Jump(&notCOWArray);
    }
    builder_.Bind(&notCOWArray);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

// for JSArray
void TypeMCRLowering::LowerArrayLoadElement(GateRef gate, ArrayState arrayState)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef element = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, JSObject::ELEMENTS_OFFSET);
    GateRef result = builder_.GetValueFromTaggedArray(element, index);
    if (arrayState == ArrayState::HOLEY) {
        result = builder_.ConvertHoleAsUndefined(result);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeMCRLowering::LowerTypedArrayLoadElement(GateRef gate, BuiltinTypeId id)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef elementSize = GetElementSize(id);
    GateRef offset = builder_.PtrMul(index, elementSize);
    VariableType type = GetVariableType(id);

    GateRef result = Circuit::NullGate();
    if (IsOnHeap()) {
        result = BuildOnHeapTypedArrayLoadElement(receiver, offset, type);
    } else {
        Label isByteArray(&builder_);
        Label isArrayBuffer(&builder_);
        Label exit(&builder_);
        result = BuildTypedArrayLoadElement(receiver, offset, type, &isByteArray, &isArrayBuffer, &exit);
    }

    switch (id) {
        case BuiltinTypeId::INT8_ARRAY:
            result = builder_.SExtInt8ToInt32(result);
            break;
        case BuiltinTypeId::UINT8_ARRAY:
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            result = builder_.ZExtInt8ToInt32(result);
            break;
        case BuiltinTypeId::INT16_ARRAY:
            result = builder_.SExtInt16ToInt32(result);
            break;
        case BuiltinTypeId::UINT16_ARRAY:
            result = builder_.ZExtInt16ToInt32(result);
            break;
        case BuiltinTypeId::FLOAT32_ARRAY:
            result = builder_.ExtFloat32ToDouble(result);
            break;
        default:
            break;
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

GateRef TypeMCRLowering::BuildOnHeapTypedArrayLoadElement(GateRef receiver, GateRef offset, VariableType type)
{
    GateRef arrbuffer =
        builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
    GateRef data = builder_.PtrAdd(arrbuffer, builder_.IntPtr(ByteArray::DATA_OFFSET));
    GateRef result = builder_.Load(type, data, offset);
    return result;
}

GateRef TypeMCRLowering::BuildTypedArrayLoadElement(GateRef receiver, GateRef offset, VariableType type,
                                                    Label *isByteArray, Label *isArrayBuffer, Label *exit)
{
    GateRef byteArrayOrArraybuffer =
        builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
    DEFVAlUE(data, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    DEFVAlUE(result, (&builder_), type, builder_.Double(0));

    GateRef isOnHeap = builder_.Load(VariableType::BOOL(), receiver, builder_.IntPtr(JSTypedArray::ON_HEAP_OFFSET));
    builder_.Branch(isOnHeap, isByteArray, isArrayBuffer);
    builder_.Bind(isByteArray);
    {
        data = builder_.PtrAdd(byteArrayOrArraybuffer, builder_.IntPtr(ByteArray::DATA_OFFSET));
        result = builder_.Load(type, *data, offset);
        builder_.Jump(exit);
    }
    builder_.Bind(isArrayBuffer);
    {
        data = builder_.Load(VariableType::JS_POINTER(), byteArrayOrArraybuffer,
                             builder_.IntPtr(JSArrayBuffer::DATA_OFFSET));
        GateRef block = builder_.Load(VariableType::JS_ANY(), *data, builder_.IntPtr(JSNativePointer::POINTER_OFFSET));
        GateRef byteOffset =
            builder_.Load(VariableType::INT32(), receiver, builder_.IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET));
        result = builder_.Load(type, block, builder_.PtrAdd(offset, byteOffset));
        builder_.Jump(exit);
    }
    builder_.Bind(exit);

    return *result;
}

void TypeMCRLowering::LowerStringLoadElement(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);

    GateRef result = builder_.CallStub(glue, gate, CommonStubCSigns::GetCharFromEcmaString, { glue, receiver, index });
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeMCRLowering::LowerStoreElement(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    auto op = acc_.GetTypedStoreOp(gate);
    switch (op) {
        case TypedStoreOp::ARRAY_STORE_ELEMENT:
            LowerArrayStoreElement(gate, glue);
            break;
        case TypedStoreOp::INT8ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::INT8_ARRAY);
            break;
        case TypedStoreOp::UINT8ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::UINT8_ARRAY);
            break;
        case TypedStoreOp::UINT8CLAMPEDARRAY_STORE_ELEMENT:
            LowerUInt8ClampedArrayStoreElement(gate);
            break;
        case TypedStoreOp::INT16ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::INT16_ARRAY);
            break;
        case TypedStoreOp::UINT16ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::UINT16_ARRAY);
            break;
        case TypedStoreOp::INT32ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::INT32_ARRAY);
            break;
        case TypedStoreOp::UINT32ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::UINT32_ARRAY);
            break;
        case TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::FLOAT32_ARRAY);
            break;
        case TypedStoreOp::FLOAT64ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::FLOAT64_ARRAY);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

// for JSArray
void TypeMCRLowering::LowerArrayStoreElement(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);  // 0: receiver
    GateRef index = acc_.GetValueIn(gate, 1);     // 1: index
    GateRef value = acc_.GetValueIn(gate, 2);     // 2: value

    GateRef element = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, JSObject::ELEMENTS_OFFSET);
    builder_.SetValueToTaggedArray(VariableType::JS_ANY(), glue, element, index, value);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

// for JSTypedArray
void TypeMCRLowering::LowerTypedArrayStoreElement(GateRef gate, BuiltinTypeId id)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);

    GateRef elementSize = GetElementSize(id);
    GateRef offset = builder_.PtrMul(index, elementSize);
    switch (id) {
        case BuiltinTypeId::INT8_ARRAY:
        case BuiltinTypeId::UINT8_ARRAY:
            value = builder_.TruncInt32ToInt8(value);
            break;
        case BuiltinTypeId::INT16_ARRAY:
        case BuiltinTypeId::UINT16_ARRAY:
            value = builder_.TruncInt32ToInt16(value);
            break;
        case BuiltinTypeId::FLOAT32_ARRAY:
            value = builder_.TruncDoubleToFloat32(value);
            break;
        default:
            break;
    }

    if (IsOnHeap()) {
        BuildOnHeapTypedArrayStoreElement(receiver, offset, value);
    } else {
        Label isByteArray(&builder_);
        Label isArrayBuffer(&builder_);
        Label exit(&builder_);
        BuildTypedArrayStoreElement(receiver, offset, value, &isByteArray, &isArrayBuffer, &exit);
    }

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::BuildOnHeapTypedArrayStoreElement(GateRef receiver, GateRef offset, GateRef value)
{
    GateRef arrbuffer = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver,
                                                 JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
    GateRef data = builder_.PtrAdd(arrbuffer, builder_.IntPtr(ByteArray::DATA_OFFSET));

    builder_.StoreMemory(MemoryType::ELEMENT_TYPE, VariableType::VOID(), data, offset, value);
}

void TypeMCRLowering::BuildTypedArrayStoreElement(GateRef receiver, GateRef offset, GateRef value,
                                                  Label *isByteArray, Label *isArrayBuffer, Label *exit)
{
    GateRef byteArrayOrArraybuffer = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver,
                                                              JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
    GateRef isOnHeap = builder_.Load(VariableType::BOOL(), receiver, builder_.IntPtr(JSTypedArray::ON_HEAP_OFFSET));
    DEFVAlUE(data, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    builder_.Branch(isOnHeap, isByteArray, isArrayBuffer);
    builder_.Bind(isByteArray);
    {
        data = builder_.PtrAdd(byteArrayOrArraybuffer, builder_.IntPtr(ByteArray::DATA_OFFSET));
        builder_.StoreMemory(MemoryType::ELEMENT_TYPE, VariableType::VOID(), *data, offset, value);
        builder_.Jump(exit);
    }
    builder_.Bind(isArrayBuffer);
    {
        data = builder_.Load(VariableType::JS_POINTER(), byteArrayOrArraybuffer,
                             builder_.IntPtr(JSArrayBuffer::DATA_OFFSET));
        GateRef block = builder_.Load(VariableType::JS_ANY(), *data, builder_.IntPtr(JSNativePointer::POINTER_OFFSET));
        GateRef byteOffset =
            builder_.Load(VariableType::INT32(), receiver, builder_.IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET));
        builder_.StoreMemory(MemoryType::ELEMENT_TYPE, VariableType::VOID(), block,
                             builder_.PtrAdd(offset, byteOffset), value);
        builder_.Jump(exit);
    }
    builder_.Bind(exit);
}

// for UInt8ClampedArray
void TypeMCRLowering::LowerUInt8ClampedArrayStoreElement(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef elementSize = builder_.Int32(sizeof(uint8_t));
    GateRef offset = builder_.PtrMul(index, elementSize);
    GateRef value = acc_.GetValueIn(gate, 2);

    DEFVAlUE(result, (&builder_), VariableType::INT32(), value);
    GateRef topValue = builder_.Int32(static_cast<uint32_t>(UINT8_MAX));
    GateRef bottomValue = builder_.Int32(static_cast<uint32_t>(0));
    Label isOverFlow(&builder_);
    Label notOverFlow(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.Int32GreaterThan(value, topValue), &isOverFlow, &notOverFlow);
    builder_.Bind(&isOverFlow);
    {
        result = topValue;
        builder_.Jump(&exit);
    }
    builder_.Bind(&notOverFlow);
    {
        Label isUnderSpill(&builder_);
        builder_.Branch(builder_.Int32LessThan(value, bottomValue), &isUnderSpill, &exit);
        builder_.Bind(&isUnderSpill);
        {
            result = bottomValue;
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    value = builder_.TruncInt32ToInt8(*result);

    GateRef arrbuffer = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver,
        JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);

    GateRef data = builder_.PtrAdd(arrbuffer, builder_.IntPtr(ByteArray::DATA_OFFSET));

    builder_.StoreMemory(MemoryType::ELEMENT_TYPE, VariableType::VOID(), data, offset, value);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

GateRef TypeMCRLowering::DoubleToTaggedDoublePtr(GateRef gate)
{
    return builder_.DoubleToTaggedDoublePtr(gate);
}

GateRef TypeMCRLowering::ChangeInt32ToFloat64(GateRef gate)
{
    return builder_.ChangeInt32ToFloat64(gate);
}

GateRef TypeMCRLowering::TruncDoubleToInt(GateRef gate)
{
    return builder_.TruncInt64ToInt32(builder_.TruncFloatToInt64(gate));
}

GateRef TypeMCRLowering::IntToTaggedIntPtr(GateRef x)
{
    GateRef val = builder_.SExtInt32ToInt64(x);
    return builder_.ToTaggedIntPtr(val);
}

void TypeMCRLowering::LowerTypedCallBuitin(GateRef gate)
{
    BuiltinLowering lowering(circuit_);
    lowering.LowerTypedCallBuitin(gate);
}

void TypeMCRLowering::LowerJSCallTargetFromDefineFuncCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (tsManager_->IsFunctionTypeKind(type)) {
        GateRef frameState = GetFrameState(gate);
        auto func = acc_.GetValueIn(gate, 0);
        GateRef check = builder_.IsOptimized(func);
        builder_.DeoptCheck(check, frameState, DeoptType::NOTJSCALLTGT);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
    } else {
        LOG_COMPILER(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeMCRLowering::LowerJSCallTargetTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (tsManager_->IsFunctionTypeKind(type)) {
        ArgumentAccessor argAcc(circuit_);
        GateRef frameState = GetFrameState(gate);
        GateRef jsFunc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
        auto func = acc_.GetValueIn(gate, 0);
        auto methodIndex = acc_.GetValueIn(gate, 1);
        GateRef isObj = builder_.TaggedIsHeapObject(func);
        GateRef isOptimized = builder_.IsOptimized(func);
        GateRef funcMethodTarget = builder_.GetMethodFromFunction(func);
        GateRef checkFunc = builder_.BoolAnd(isObj, isOptimized);
        GateRef methodTarget = GetObjectFromConstPool(jsFunc, methodIndex);
        GateRef check = builder_.BoolAnd(checkFunc, builder_.Equal(funcMethodTarget, methodTarget));
        builder_.DeoptCheck(check, frameState, DeoptType::NOTJSCALLTGT);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
    } else {
        LOG_COMPILER(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeMCRLowering::LowerJSFastCallTargetTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (tsManager_->IsFunctionTypeKind(type)) {
        ArgumentAccessor argAcc(circuit_);
        GateRef frameState = GetFrameState(gate);
        GateRef jsFunc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
        auto func = acc_.GetValueIn(gate, 0);
        auto methodIndex = acc_.GetValueIn(gate, 1);
        GateRef isObj = builder_.TaggedIsHeapObject(func);
        GateRef canFastCall = builder_.CanFastCall(func);
        GateRef funcMethodTarget = builder_.GetMethodFromFunction(func);
        GateRef checkFunc = builder_.BoolAnd(isObj, canFastCall);
        GateRef methodTarget = GetObjectFromConstPool(jsFunc, methodIndex);
        GateRef check = builder_.BoolAnd(checkFunc, builder_.Equal(funcMethodTarget, methodTarget));
        builder_.DeoptCheck(check, frameState, DeoptType::NOTJSFASTCALLTGT);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
    } else {
        LOG_COMPILER(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeMCRLowering::LowerJSCallThisTargetTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (tsManager_->IsFunctionTypeKind(type)) {
        GateRef frameState = GetFrameState(gate);
        auto func = acc_.GetValueIn(gate, 0);
        GateRef isObj = builder_.TaggedIsHeapObject(func);
        GateRef isOptimized = builder_.IsOptimizedAndNotFastCall(func);
        GateRef check = builder_.BoolAnd(isObj, isOptimized);
        builder_.DeoptCheck(check, frameState, DeoptType::NOTJSCALLTGT);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
    } else {
        LOG_COMPILER(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeMCRLowering::LowerJSNoGCCallThisTargetTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (tsManager_->IsFunctionTypeKind(type)) {
        GateRef frameState = GetFrameState(gate);
        auto func = acc_.GetValueIn(gate, 0);
        GateRef isObj = builder_.TaggedIsHeapObject(func);
        GateRef isOptimized = builder_.IsOptimizedAndNotFastCall(func);
        GateRef methodId = builder_.GetMethodId(func);
        GateRef checkOptimized = builder_.BoolAnd(isObj, isOptimized);
        GateRef check = builder_.BoolAnd(checkOptimized, builder_.Equal(methodId, acc_.GetValueIn(gate, 1)));
        builder_.DeoptCheck(check, frameState, DeoptType::NOTJSCALLTGT);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
    } else {
        LOG_COMPILER(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeMCRLowering::LowerJSFastCallThisTargetTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (tsManager_->IsFunctionTypeKind(type)) {
        GateRef frameState = GetFrameState(gate);
        auto func = acc_.GetValueIn(gate, 0);
        GateRef isObj = builder_.TaggedIsHeapObject(func);
        GateRef canFastCall = builder_.CanFastCall(func);
        GateRef check = builder_.BoolAnd(isObj, canFastCall);
        builder_.DeoptCheck(check, frameState, DeoptType::NOTJSFASTCALLTGT);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
    } else {
        LOG_COMPILER(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeMCRLowering::LowerJSNoGCFastCallThisTargetTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (tsManager_->IsFunctionTypeKind(type)) {
        GateRef frameState = GetFrameState(gate);
        auto func = acc_.GetValueIn(gate, 0);
        GateRef isObj = builder_.TaggedIsHeapObject(func);
        GateRef canFastCall = builder_.CanFastCall(func);
        GateRef methodId = builder_.GetMethodId(func);
        GateRef checkOptimized = builder_.BoolAnd(isObj, canFastCall);
        GateRef check = builder_.BoolAnd(checkOptimized, builder_.Equal(methodId, acc_.GetValueIn(gate, 1)));
        builder_.DeoptCheck(check, frameState, DeoptType::NOTJSFASTCALLTGT);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
    } else {
        LOG_COMPILER(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeMCRLowering::LowerCallTargetCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);

    BuiltinLowering lowering(circuit_);
    GateRef funcheck = lowering.LowerCallTargetCheck(&env, gate);
    GateRef constId = acc_.GetValueIn(gate, 1); // 1: stub id
    if (acc_.GetConstantValue(constId) != static_cast<uint64_t>(BuiltinsStubCSigns::ID::STRINGIFY)) {
        GateRef check = lowering.CheckPara(gate, funcheck);
        builder_.DeoptCheck(check, frameState, DeoptType::NOTCALLTGT);
    } else {
        builder_.DeoptCheck(funcheck, frameState, DeoptType::NOTCALLTGT);
    }

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerJSInlineTargetTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    auto func = acc_.GetValueIn(gate, 0);
    GateRef isObj = builder_.TaggedIsHeapObject(func);
    GateRef isJsFunc = builder_.IsJSFunction(func);
    GateRef checkFunc = builder_.BoolAnd(isObj, isJsFunc);
    GateRef GetMethodId = builder_.GetMethodId(func);
    GateRef check = builder_.BoolAnd(checkFunc, builder_.Equal(GetMethodId, acc_.GetValueIn(gate, 1)));
    builder_.DeoptCheck(check, frameState, DeoptType::INLINEFAIL);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeMCRLowering::LowerTypedNewAllocateThis(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ArgumentAccessor argAcc(circuit_);
    GateRef frameState = GetFrameState(gate);
    GateRef jsFunc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::FUNC);

    GateRef ctor = acc_.GetValueIn(gate, 0);

    DEFVAlUE(thisObj, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label allocate(&builder_);
    Label exit(&builder_);

    GateRef isBase = builder_.IsBase(ctor);
    builder_.Branch(isBase, &allocate, &exit);
    builder_.Bind(&allocate);
    {
        // add typecheck to detect protoOrHclass is equal with ihclass,
        // if pass typecheck: 1.no need to check whether hclass is valid 2.no need to check return result
        GateRef protoOrHclass = builder_.LoadConstOffset(VariableType::JS_ANY(), ctor,
            JSFunction::PROTO_OR_DYNCLASS_OFFSET);
        GateRef ihclassIndex = acc_.GetValueIn(gate, 1);
        GateRef ihclass = GetObjectFromConstPool(jsFunc, ihclassIndex);
        GateRef check = builder_.Equal(protoOrHclass, ihclass);
        builder_.DeoptCheck(check, frameState, DeoptType::NOTNEWOBJ);

        thisObj = builder_.CallStub(glue, gate, CommonStubCSigns::NewJSObject, { glue, protoOrHclass });
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *thisObj);
}

void TypeMCRLowering::LowerTypedSuperAllocateThis(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef superCtor = acc_.GetValueIn(gate, 0);
    GateRef newTarget = acc_.GetValueIn(gate, 1);

    DEFVAlUE(thisObj, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label allocate(&builder_);
    Label exit(&builder_);

    GateRef isBase = builder_.IsBase(superCtor);
    builder_.Branch(isBase, &allocate, &exit);
    builder_.Bind(&allocate);
    {
        GateRef protoOrHclass = builder_.LoadConstOffset(VariableType::JS_ANY(), newTarget,
            JSFunction::PROTO_OR_DYNCLASS_OFFSET);
        GateRef check = builder_.IsJSHClass(protoOrHclass);
        GateRef frameState = GetFrameState(gate);
        builder_.DeoptCheck(check, frameState, DeoptType::NOTNEWOBJ);

        thisObj = builder_.CallStub(glue, gate, CommonStubCSigns::NewJSObject, { glue, protoOrHclass });
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *thisObj);
}

void TypeMCRLowering::LowerGetSuperConstructor(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef ctor = acc_.GetValueIn(gate, 0);
    GateRef hclass = builder_.LoadHClass(ctor);
    GateRef superCtor = builder_.LoadConstOffset(VariableType::JS_ANY(), hclass, JSHClass::PROTOTYPE_OFFSET);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), superCtor);
}

GateRef TypeMCRLowering::LoadFromVTable(GateRef receiver, size_t index)
{
    GateRef hclass = builder_.LoadConstOffset(
        VariableType::JS_POINTER(), receiver, TaggedObject::HCLASS_OFFSET);
    GateRef vtable = builder_.LoadConstOffset(VariableType::JS_ANY(),
        hclass, JSHClass::VTABLE_OFFSET);

    GateRef itemOwner = builder_.LoadFromTaggedArray(vtable, VTable::TupleItem::OWNER + index);
    GateRef itemOffset = builder_.LoadFromTaggedArray(vtable, VTable::TupleItem::OFFSET + index);
    return builder_.Load(VariableType::JS_ANY(), itemOwner, builder_.TaggedGetInt(itemOffset));
}

VariableType TypeMCRLowering::GetVarType(PropertyLookupResult plr)
{
    if (plr.GetRepresentation() == Representation::DOUBLE) {
        return kungfu::VariableType::FLOAT64();
    } else if (plr.GetRepresentation() == Representation::INT) {
        return kungfu::VariableType::INT32();
    } else {
        return kungfu::VariableType::INT64();
    }
}

GateRef TypeMCRLowering::LoadSupers(GateRef hclass)
{
    return builder_.LoadConstOffset(VariableType::JS_ANY(), hclass, JSHClass::SUPERS_OFFSET);
}

GateRef TypeMCRLowering::GetLengthFromSupers(GateRef supers)
{
    return builder_.LoadConstOffset(VariableType::INT32(), supers, TaggedArray::EXTRACT_LENGTH_OFFSET);
}

GateRef TypeMCRLowering::GetValueFromSupers(GateRef supers, size_t index)
{
    GateRef val = builder_.LoadFromTaggedArray(supers, index);
    return builder_.LoadObjectFromWeakRef(val);
}

GateRef TypeMCRLowering::CallAccessor(GateRef glue, GateRef gate, GateRef function, GateRef receiver,
    AccessorMode mode, GateRef value)
{
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(JSCall));
    GateRef target = builder_.IntPtr(RTSTUB_ID(JSCall));
    GateRef newTarget = builder_.Undefined();
    GateRef argc = builder_.Int64(NUM_MANDATORY_JSFUNC_ARGS + (mode == AccessorMode::SETTER ? 1 : 0));  // 1: value
    std::vector<GateRef> args { glue, argc, function, newTarget, receiver };
    if (mode == AccessorMode::SETTER) {
        args.emplace_back(value);
    }

    return builder_.Call(cs, glue, target, builder_.GetDepend(), args, gate);
}

void TypeMCRLowering::ReplaceHirWithPendingException(GateRef hirGate, GateRef glue, GateRef state, GateRef depend,
                                                     GateRef value)
{
    auto condition = builder_.HasPendingException(glue);
    GateRef ifBranch = builder_.Branch(state, condition);
    GateRef ifTrue = builder_.IfTrue(ifBranch);
    GateRef ifFalse = builder_.IfFalse(ifBranch);
    GateRef eDepend = builder_.DependRelay(ifTrue, depend);
    GateRef sDepend = builder_.DependRelay(ifFalse, depend);

    StateDepend success(ifFalse, sDepend);
    StateDepend exception(ifTrue, eDepend);
    acc_.ReplaceHirWithIfBranch(hirGate, success, exception, value);
}

void TypeMCRLowering::LowerLoadGetter(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: receiver, plr
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);

    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsAccessor());
    GateRef accessor = LoadFromVTable(receiver, plr.GetOffset());
    GateRef getter = builder_.Load(VariableType::JS_ANY(), accessor,
                                   builder_.IntPtr(AccessorData::GETTER_OFFSET));
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), getter);
}

void TypeMCRLowering::LowerLoadSetter(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: receiver, plr
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);

    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsAccessor());
    GateRef accessor = LoadFromVTable(receiver, plr.GetOffset());
    GateRef setter = builder_.Load(VariableType::JS_ANY(),
        accessor, builder_.IntPtr(AccessorData::SETTER_OFFSET));
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), setter);
}

// subtyping check and hclss check
void TypeMCRLowering::LowerInlineAccessorCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef frameState = acc_.GetFrameState(gate);
    builder_.HeapObjectCheck(receiver, frameState);

    GateRef aotHCIndex = acc_.GetValueIn(gate, 1);
    ArgumentAccessor argAcc(circuit_);
    GateRef jsFunc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
    JSTaggedValue aotHC = tsManager_->GetValueFromCache(acc_.TryGetValue(aotHCIndex));
    ASSERT(aotHC.IsJSHClass());

    int32_t level = JSHClass::Cast(aotHC.GetTaggedObject())->GetLevel();
    ASSERT(level >= 0);

    GateRef receiverHClass = builder_.LoadConstOffset(
        VariableType::JS_POINTER(), receiver, TaggedObject::HCLASS_OFFSET);
    GateRef supers = LoadSupers(receiverHClass);

    auto hclassIndex = acc_.GetConstantValue(aotHCIndex);
    GateRef aotHCGate = LoadFromConstPool(jsFunc, hclassIndex);
    GateRef hclassCompare = builder_.Equal(aotHCGate, receiverHClass);
    if (LIKELY(static_cast<uint32_t>(level) < SubtypingOperator::DEFAULT_SUPERS_CAPACITY)) {
        GateRef subtypingCompare = builder_.Equal(aotHCGate, GetValueFromSupers(supers, level));
        GateRef compare = builder_.BoolAnd(hclassCompare, subtypingCompare);
        builder_.DeoptCheck(compare, frameState, DeoptType::INCONSISTENTHCLASS);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
        return;
    }

    Label levelValid(&builder_);
    Label exit(&builder_);
    DEFVAlUE(check, (&builder_), VariableType::BOOL(), builder_.False());
    GateRef levelGate = builder_.Int32(level);
    GateRef length = GetLengthFromSupers(supers);

    builder_.Branch(builder_.Int32LessThan(levelGate, length), &levelValid, &exit);
    builder_.Bind(&levelValid);
    {
        check = builder_.Equal(aotHCGate, GetValueFromSupers(supers, level));
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);

    GateRef compare = builder_.BoolAnd(hclassCompare, *check);
    builder_.DeoptCheck(compare, frameState, DeoptType::INCONSISTENTHCLASS);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}
}  // namespace panda::ecmascript::kungfu
