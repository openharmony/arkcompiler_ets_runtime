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

#include "ecmascript/compiler/type_lowering.h"
#include "ecmascript/compiler/builtins_lowering.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/vtable.h"

namespace panda::ecmascript::kungfu {
void TypeLowering::RunTypeLowering()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        LowerType(gate);
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m" << "=================="
                           << " after type lowering "
                           << "[" << GetMethodName() << "] "
                           << "==================" << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "=========================== End =========================" << "\033[0m";
    }
}

void TypeLowering::LowerType(GateRef gate)
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
            LowerTypedArrayCheck(gate, glue);
            break;
        case OpCode::OBJECT_TYPE_CHECK:
            LowerObjectTypeCheck(gate);
            break;
        case OpCode::INDEX_CHECK:
            LowerIndexCheck(gate);
            break;
        case OpCode::INT32_OVERFLOW_CHECK:
            LowerOverflowCheck(gate);
            break;
        case OpCode::JSCALLTARGET_TYPE_CHECK:
            LowerJSCallTargetTypeCheck(gate);
            break;
        case OpCode::JSCALLTHISTARGET_TYPE_CHECK:
            LowerJSCallThisTargetTypeCheck(gate);
            break;
        case OpCode::TYPED_CALL_CHECK:
            LowerCallTargetCheck(gate);
            break;
        case OpCode::TYPED_BINARY_OP:
            LowerTypedBinaryOp(gate);
            break;
        case OpCode::TYPE_CONVERT:
            LowerTypeConvert(gate);
            break;
        case OpCode::TYPED_UNARY_OP:
            LowerTypedUnaryOp(gate);
            break;
        case OpCode::LOAD_PROPERTY:
            LowerLoadProperty(gate);
            break;
        case OpCode::CALL_GETTER:
            LowerCallGetter(gate, glue);
            break;
        case OpCode::STORE_PROPERTY:
        case OpCode::STORE_PROPERTY_NO_BARRIER:
            LowerStoreProperty(gate, glue);
            break;
        case OpCode::CALL_SETTER:
            LowerCallSetter(gate, glue);
            break;
        case OpCode::LOAD_ARRAY_LENGTH:
            LowerLoadArrayLength(gate);
            break;
        case OpCode::LOAD_CONST_OFFSET:
            LowerLoadConstOffset(gate);
            break;
        case OpCode::LOAD_ELEMENT:
            LowerLoadElement(gate);
            break;
        case OpCode::STORE_ELEMENT:
            LowerStoreElement(gate, glue);
            break;
        case OpCode::HEAP_ALLOC:
            LowerHeapAllocate(gate, glue);
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
        case OpCode::CONVERT:
            LowerConvert(gate);
            break;
        case OpCode::CHECK_AND_CONVERT:
            LowerCheckAndConvert(gate);
            break;
        default:
            break;
    }
}

void TypeLowering::LowerPrimitiveTypeCheck(GateRef gate)
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

void TypeLowering::LowerIntCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsInt(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTINT);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerDoubleCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsDouble(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTDOUBLE);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerNumberCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsNumber(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTNUMBER);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerBooleanCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsBoolean(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTBOOL);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerStableArrayCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);

    GateRef receiver = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(receiver, frameState);

    GateRef receiverHClass = builder_.LoadHClass(receiver);
    builder_.HClassStableArrayCheck(receiverHClass, frameState);
    builder_.ArrayGuardianCheck(frameState);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerTypedArrayCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (tsManager_->IsFloat32ArrayType(type)) {
        LowerFloat32ArrayCheck(gate, glue);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeLowering::LowerFloat32ArrayCheck(GateRef gate, GateRef glue)
{
    GateRef frameState = GetFrameState(gate);

    GateRef glueGlobalEnvOffset = builder_.IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(false));
    GateRef glueGlobalEnv = builder_.Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef receiverHClass = builder_.LoadHClass(receiver);
    GateRef float32ArrayFunction =
        builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::FLOAT32_ARRAY_FUNCTION_INDEX);
    GateRef protoOrHclass =
        builder_.Load(VariableType::JS_ANY(), float32ArrayFunction,
                      builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    GateRef check = builder_.Equal(receiverHClass, protoOrHclass);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTF32ARRAY);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerObjectTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (tsManager_->IsClassInstanceTypeKind(type)) {
        LowerTSSubtypingCheck(gate);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeLowering::LowerTSSubtypingCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    ArgumentAccessor argAcc(circuit_);
    GateRef jsFunc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef aotHCIndex = acc_.GetValueIn(gate, 1);

    Label exit(&builder_);
    builder_.HeapObjectCheck(receiver, frameState);

    DEFVAlUE(check, (&builder_), VariableType::BOOL(), builder_.False());
    {
        JSTaggedValue aotHC = tsManager_->GetHClassFromCache(acc_.TryGetValue(aotHCIndex));
        ASSERT(aotHC.IsJSHClass());

        int32_t level = JSHClass::Cast(aotHC.GetTaggedObject())->GetLevel();
        ASSERT(level >= 0);
        GateRef levelGate = builder_.Int32(level);

        GateRef receiverHC = builder_.LoadHClass(receiver);
        GateRef supers = LoadSupers(receiverHC);
        GateRef length = GetLengthFromSupers(supers);

        // Nextly, consider remove level check by guaranteeing not read illegal addresses.
        Label levelValid(&builder_);
        builder_.Branch(builder_.Int32LessThan(levelGate, length), &levelValid, &exit);
        builder_.Bind(&levelValid);
        {
            GateRef aotHCGate = GetObjectFromConstPool(jsFunc, aotHCIndex);
            check = builder_.Equal(aotHCGate, GetValueFromSupers(supers, levelGate));
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    builder_.DeoptCheck(*check, frameState, DeoptType::INCONSISTENTHCLASS);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerIndexCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (tsManager_->IsArrayTypeKind(type)) {
        LowerArrayIndexCheck(gate);
    } else if (tsManager_->IsFloat32ArrayType(type)) {
        LowerFloat32ArrayIndexCheck(gate);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeLowering::LowerArrayIndexCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef hclassIndex = acc_.GetValueIn(gate, 1);
    GateRef length = acc_.GetGateType(receiver).IsNJSValueType() ? receiver :
            builder_.Load(VariableType::INT32(), receiver, builder_.IntPtr(JSArray::LENGTH_OFFSET));
    GateRef lengthCheck = builder_.Int32UnsignedLessThan(hclassIndex, length);
    GateRef nonNegativeCheck = builder_.Int32LessThanOrEqual(builder_.Int32(0), hclassIndex);
    GateRef check = builder_.BoolAnd(lengthCheck, nonNegativeCheck);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTARRAYIDX);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerFloat32ArrayIndexCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef length = acc_.GetGateType(receiver).IsNJSValueType() ? receiver :
            builder_.Load(VariableType::INT32(), receiver, builder_.IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET));
    GateRef nonNegativeCheck = builder_.Int32LessThanOrEqual(builder_.Int32(0), index);
    GateRef lengthCheck = builder_.Int32UnsignedLessThan(index, length);
    GateRef check = builder_.BoolAnd(nonNegativeCheck, lengthCheck);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTF32ARRAYIDX);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerOverflowCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    auto op = accessor.GetTypedUnOp();
    switch (op) {
        case TypedUnOp::TYPED_INC: {
            LowerTypedIncOverflowCheck(gate);
            break;
        }
        case TypedUnOp::TYPED_DEC: {
            LowerTypedDecOverflowCheck(gate);
            break;
        }
        case TypedUnOp::TYPED_NEG: {
            LowerTypedNegOverflowCheck(gate);
            break;
        }
        default: {
            LOG_COMPILER(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
        }
    }
}

void TypeLowering::LowerTypedIncOverflowCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef max = builder_.Int32(INT32_MAX);
    GateRef rangeCheck = builder_.Int32NotEqual(value, max);
    builder_.DeoptCheck(rangeCheck, frameState, DeoptType::NOTINCOV);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerTypedDecOverflowCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef min = builder_.Int32(INT32_MIN);
    GateRef rangeCheck = builder_.Int32NotEqual(value, min);
    builder_.DeoptCheck(rangeCheck, frameState, DeoptType::NOTDECOV);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerTypedNegOverflowCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef min = builder_.Int32(INT32_MIN);
    GateRef zero = builder_.Int32(0);
    GateRef notMin = builder_.Int32NotEqual(value, min);
    GateRef notZero = builder_.Int32NotEqual(value, zero);
    GateRef rangeCheck = builder_.BoolAnd(notMin, notZero);
    builder_.DeoptCheck(rangeCheck, frameState, DeoptType::NOTNEGOV);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

GateRef TypeLowering::LowerCallRuntime(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
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

void TypeLowering::LowerTypeConvert(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateAccessor acc(circuit_);
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

void TypeLowering::LowerPrimitiveToNumber(GateRef dst, GateRef src, GateType srcType)
{
    Label exit(&builder_);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    if (srcType.IsBooleanType()) {
        Label isTrue(&builder_);
        Label isFalse(&builder_);
        builder_.Branch(builder_.TaggedIsTrue(src), &isTrue, &isFalse);
        builder_.Bind(&isTrue);
        result = IntToTaggedIntPtr(builder_.Int32(1));
        builder_.Jump(&exit);
        builder_.Bind(&isFalse);
        result = IntToTaggedIntPtr(builder_.Int32(0));
        builder_.Jump(&exit);
    } else if (srcType.IsUndefinedType()) {
        result = DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
    } else if (srcType.IsBigIntType() || srcType.IsNumberType()) {
        result = src;
        builder_.Jump(&exit);
    } else if (srcType.IsNullType()) {
        result = IntToTaggedIntPtr(builder_.Int32(0));
        builder_.Jump(&exit);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(dst, builder_.GetState(), builder_.GetDepend(), *result);
}

GateRef TypeLowering::GetObjectFromConstPool(GateRef jsFunc, GateRef index)
{
    GateRef constPool = builder_.GetConstPool(jsFunc);
    return builder_.GetValueFromTaggedArray(constPool, index);
}

void TypeLowering::LowerLoadProperty(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: receiver, plr
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsLocal() || plr.IsFunction());
    GateRef offset = builder_.IntPtr(plr.GetOffset());

    if (plr.IsLocal()) {
        Label returnUndefined(&builder_);
        Label exit(&builder_);
        result = builder_.Load(VariableType::JS_ANY(), receiver, offset);
        builder_.Branch(builder_.IsSpecial(*result, JSTaggedValue::VALUE_HOLE), &returnUndefined, &exit);
        builder_.Bind(&returnUndefined);
        {
            result = builder_.UndefineConstant();
            builder_.Jump(&exit);
        }
        builder_.Bind(&exit);
    } else {
        GateRef vtable = LoadVTable(receiver);
        GateRef itemOwner = GetOwnerFromVTable(vtable, offset);
        GateRef itemOffset = GetOffsetFromVTable(vtable, offset);
        result = builder_.Load(VariableType::JS_ANY(), itemOwner, itemOffset);
    }

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerCallGetter(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: receiver, plr
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);

    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsAccessor());
    GateRef offset = builder_.IntPtr(plr.GetOffset());
    GateRef vtable = LoadVTable(receiver);
    GateRef itemOwner = GetOwnerFromVTable(vtable, offset);
    GateRef itemOffset = GetOffsetFromVTable(vtable, offset);

    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    Label callGetter(&builder_);
    Label exit(&builder_);
    GateRef accessor = builder_.Load(VariableType::JS_ANY(), itemOwner, itemOffset);
    GateRef getter = builder_.Load(VariableType::JS_ANY(), accessor,
                                   builder_.IntPtr(AccessorData::GETTER_OFFSET));
    builder_.Branch(builder_.IsSpecial(getter, JSTaggedValue::VALUE_UNDEFINED), &exit, &callGetter);
    builder_.Bind(&callGetter);
    {
        result = CallAccessor(glue, gate, getter, receiver, AccessorMode::GETTER);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    ReplaceHirWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerStoreProperty(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 3);  // 3: receiver, plr, value
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsLocal());
    GateRef offset = builder_.IntPtr(plr.GetOffset());
    auto op = OpCode(acc_.GetOpCode(gate));
    if (op == OpCode::STORE_PROPERTY) {
        builder_.Store(VariableType::JS_ANY(), glue, receiver, offset, value);
    } else if (op == OpCode::STORE_PROPERTY_NO_BARRIER) {
        builder_.StoreWithNoBarrier(VariableType::JS_ANY(), receiver, offset, value);
    } else {
        UNREACHABLE();
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerCallSetter(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 3);  // 3: receiver, plr, value
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);

    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsAccessor());
    GateRef offset = builder_.IntPtr(plr.GetOffset());
    GateRef vtable = LoadVTable(receiver);
    GateRef itemOwner = GetOwnerFromVTable(vtable, offset);
    GateRef itemOffset = GetOffsetFromVTable(vtable, offset);

    Label callSetter(&builder_);
    Label exit(&builder_);
    GateRef accessor = builder_.Load(VariableType::JS_ANY(), itemOwner, itemOffset);
    GateRef setter = builder_.Load(VariableType::JS_ANY(), accessor, builder_.IntPtr(AccessorData::SETTER_OFFSET));
    builder_.Branch(builder_.IsSpecial(setter, JSTaggedValue::VALUE_UNDEFINED), &exit, &callSetter);
    builder_.Bind(&callSetter);
    {
        CallAccessor(glue, gate, setter, receiver, AccessorMode::SETTER, value);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    ReplaceHirWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerLoadArrayLength(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef array = acc_.GetValueIn(gate, 0);
    GateRef offset = builder_.IntPtr(JSArray::LENGTH_OFFSET);
    GateRef result = builder_.Load(VariableType::JS_ANY(), array, offset);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeLowering::LowerLoadConstOffset(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef offset = builder_.IntPtr(acc_.GetOffset(gate));
    GateRef result = builder_.Load(
        VariableType(acc_.GetMachineType(gate), acc_.GetGateType(gate)), receiver, offset);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeLowering::LowerLoadElement(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto op = acc_.GetTypedLoadOp(gate);
    switch (op) {
        case TypedLoadOp::ARRAY_LOAD_ELEMENT:
            LowerArrayLoadElement(gate);
            break;
        case TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT:
            LowerFloat32ArrayLoadElement(gate);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

// for JSArray
void TypeLowering::LowerArrayLoadElement(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef element = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef res = builder_.GetValueFromTaggedArray(element, index);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), res);
    Label isHole(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsHole(res), &isHole, &exit);
    builder_.Bind(&isHole);
    {
        result = builder_.UndefineConstant();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

// for Float32Array
void TypeLowering::LowerFloat32ArrayLoadElement(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    Label isArrayBuffer(&builder_);
    Label isByteArray(&builder_);
    Label exit(&builder_);
    DEFVAlUE(res, (&builder_), VariableType::FLOAT32(), builder_.Double(0));
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef arrbuffer =
        builder_.Load(VariableType::JS_POINTER(), receiver, builder_.IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET));
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef elementSize = builder_.Int32(4);  // 4: float32 occupy 4 bytes
    GateRef offset = builder_.PtrMul(index, elementSize);
    GateRef byteOffset =
        builder_.Load(VariableType::INT32(), receiver, builder_.IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET));
    // viewed arraybuffer could be JSArrayBuffer or ByteArray
    GateRef isOnHeap = builder_.Load(VariableType::BOOL(), receiver, builder_.IntPtr(JSTypedArray::ON_HEAP_OFFSET));
    builder_.Branch(isOnHeap, &isByteArray, &isArrayBuffer);
    builder_.Bind(&isByteArray);
    {
        GateRef data = builder_.PtrAdd(arrbuffer, builder_.IntPtr(ByteArray::DATA_OFFSET));
        res = builder_.Load(VariableType::FLOAT32(), data, builder_.PtrAdd(offset, byteOffset));
        builder_.Jump(&exit);
    }
    builder_.Bind(&isArrayBuffer);
    {
        GateRef data = builder_.Load(VariableType::JS_POINTER(), arrbuffer,
                                     builder_.IntPtr(JSArrayBuffer::DATA_OFFSET));
        GateRef block = builder_.Load(VariableType::JS_ANY(), data, builder_.IntPtr(JSNativePointer::POINTER_OFFSET));
        res = builder_.Load(VariableType::FLOAT32(), block, builder_.PtrAdd(offset, byteOffset));
        builder_.Jump(&exit);
    }

    builder_.Bind(&exit);
    GateRef result = builder_.Float32ToTaggedDoublePtr(*res);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeLowering::LowerStoreElement(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    auto op = acc_.GetTypedStoreOp(gate);
    switch (op) {
        case TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT:
            LowerFloat32ArrayStoreElement(gate, glue);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

// for Float32Array
void TypeLowering::LowerFloat32ArrayStoreElement(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    Label isArrayBuffer(&builder_);
    Label isByteArray(&builder_);
    Label afterGetValue(&builder_);
    Label exit(&builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef elementSize = builder_.Int32(4);  // 4: float32 occupy 4 bytes
    GateRef offset = builder_.PtrMul(index, elementSize);
    GateRef byteOffset =
        builder_.Load(VariableType::INT32(), receiver, builder_.IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET));

    GateRef value = acc_.GetValueIn(gate, 2);
    GateType valueType = acc_.GetGateType(value);
    DEFVAlUE(storeValue, (&builder_), VariableType::FLOAT32(), builder_.Double(0));
    if (valueType.IsIntType()) {
        storeValue = builder_.TaggedIntPtrToFloat32(value);
    } else if (valueType.IsDoubleType()) {
        storeValue = builder_.TaggedDoublePtrToFloat32(value);
    } else {
        Label valueIsInt(&builder_);
        Label valueIsDouble(&builder_);
        builder_.Branch(builder_.TaggedIsInt(value), &valueIsInt, &valueIsDouble);
        builder_.Bind(&valueIsInt);
        {
            storeValue = builder_.TaggedIntPtrToFloat32(value);
            builder_.Jump(&afterGetValue);
        }
        builder_.Bind(&valueIsDouble);
        {
            storeValue = builder_.TaggedDoublePtrToFloat32(value);
            builder_.Jump(&afterGetValue);
        }
        builder_.Bind(&afterGetValue);
    }
    GateRef arrbuffer =
        builder_.Load(VariableType::JS_POINTER(), receiver, builder_.IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET));
    // viewed arraybuffer could be JSArrayBuffer or ByteArray
    GateRef isOnHeap = builder_.Load(VariableType::BOOL(), receiver, builder_.IntPtr(JSTypedArray::ON_HEAP_OFFSET));
    builder_.Branch(isOnHeap, &isByteArray, &isArrayBuffer);
    builder_.Bind(&isByteArray);
    {
        GateRef data = builder_.PtrAdd(arrbuffer, builder_.IntPtr(ByteArray::DATA_OFFSET));
        builder_.Store(VariableType::VOID(), glue, data, builder_.PtrAdd(offset, byteOffset), *storeValue);
        builder_.Jump(&exit);
    }
    builder_.Bind(&isArrayBuffer);
    {
        GateRef data = builder_.Load(VariableType::JS_POINTER(), arrbuffer,
                                     builder_.IntPtr(JSArrayBuffer::DATA_OFFSET));
        GateRef block = builder_.Load(VariableType::JS_ANY(), data, builder_.IntPtr(JSNativePointer::POINTER_OFFSET));
        builder_.Store(VariableType::VOID(), glue, block, builder_.PtrAdd(offset, byteOffset), *storeValue);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerHeapAllocate(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    auto bit = acc_.TryGetValue(gate);
    switch (bit) {
        case RegionSpaceFlag::IN_YOUNG_SPACE:
            LowerHeapAllocateInYoung(gate, glue);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void TypeLowering::LowerHeapAllocateInYoung(GateRef gate, GateRef glue)
{
    Label success(&builder_);
    Label callRuntime(&builder_);
    Label exit(&builder_);
    auto initialHClass = acc_.GetValueIn(gate, 0);
    auto size = builder_.GetObjectSizeFromHClass(initialHClass);
    auto topOffset = JSThread::GlueData::GetNewSpaceAllocationTopAddressOffset(false);
    auto endOffset = JSThread::GlueData::GetNewSpaceAllocationEndAddressOffset(false);
    auto topAddress = builder_.Load(VariableType::NATIVE_POINTER(), glue, builder_.IntPtr(topOffset));
    auto endAddress = builder_.Load(VariableType::NATIVE_POINTER(), glue, builder_.IntPtr(endOffset));
    auto top = builder_.Load(VariableType::JS_POINTER(), topAddress, builder_.IntPtr(0));
    auto end = builder_.Load(VariableType::JS_POINTER(), endAddress, builder_.IntPtr(0));
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    auto newTop = builder_.PtrAdd(top, size);
    builder_.Branch(builder_.IntPtrGreaterThan(newTop, end), &callRuntime, &success);
    builder_.Bind(&success);
    {
        builder_.Store(VariableType::NATIVE_POINTER(), glue, topAddress, builder_.IntPtr(0), newTop);
        result = top;
        builder_.Jump(&exit);
    }
    builder_.Bind(&callRuntime);
    {
        result = LowerCallRuntime(glue, gate, RTSTUB_ID(AllocateInYoung),  {builder_.ToTaggedInt(size)}, true);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);

    // initialization
    Label afterInitialize(&builder_);
    InitializeWithSpeicalValue(&afterInitialize, *result, glue, builder_.HoleConstant(),
                               builder_.Int32(JSObject::SIZE), builder_.TruncInt64ToInt32(size));
    builder_.Bind(&afterInitialize);
    builder_.Store(VariableType::JS_POINTER(), glue, *result, builder_.IntPtr(0), initialHClass);
    GateRef hashOffset = builder_.IntPtr(ECMAObject::HASH_OFFSET);
    builder_.Store(VariableType::INT64(), glue, *result, hashOffset, builder_.Int64(JSTaggedValue(0).GetRawData()));

    GateRef emptyArray = builder_.GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                         ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    GateRef propertiesOffset = builder_.IntPtr(JSObject::PROPERTIES_OFFSET);
    builder_.Store(VariableType::JS_POINTER(), glue, *result, propertiesOffset, emptyArray);
    GateRef elementsOffset = builder_.IntPtr(JSObject::ELEMENTS_OFFSET);
    builder_.Store(VariableType::JS_POINTER(), glue, *result, elementsOffset, emptyArray);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::InitializeWithSpeicalValue(Label *exit, GateRef object, GateRef glue,
                                              GateRef value, GateRef start, GateRef end)
{
    Label begin(&builder_);
    Label storeValue(&builder_);
    Label endLoop(&builder_);

    DEFVAlUE(startOffset, (&builder_), VariableType::INT32(), start);
    builder_.Jump(&begin);
    builder_.LoopBegin(&begin);
    {
        builder_.Branch(builder_.Int32UnsignedLessThan(*startOffset, end), &storeValue, exit);
        builder_.Bind(&storeValue);
        {
            builder_.Store(VariableType::INT64(), glue, object, builder_.ZExtInt32ToPtr(*startOffset), value);
            startOffset = builder_.Int32Add(*startOffset, builder_.Int32(JSTaggedValue::TaggedTypeSize()));
            builder_.Jump(&endLoop);
        }
        builder_.Bind(&endLoop);
        builder_.LoopEnd(&begin);
    }
}

void TypeLowering::LowerTypedBinaryOp(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    TypedBinOp op = acc_.GetTypedBinaryOp(gate);
    switch (op) {
        case TypedBinOp::TYPED_MOD: {
            LowerTypedMod(gate);
            break;
        }
        default:
            LOG_COMPILER(FATAL) << "unkown TypedBinOp: " << static_cast<int>(op);
            break;
    }
}

void TypeLowering::LowerTypedUnaryOp(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    GateType valueType = accessor.GetTypeValue();
    auto op = accessor.GetTypedUnOp();
    switch (op) {
        case TypedUnOp::TYPED_TONUMBER:
            break;
        case TypedUnOp::TYPED_ISFALSE:
            LowerTypedIsFalse(gate, valueType);
            break;
        case TypedUnOp::TYPED_ISTRUE:
            LowerTypedIsTrue(gate, valueType);
            break;
        default:
            LOG_COMPILER(FATAL) << "unkown TypedUnOp: " << static_cast<int>(op);
            break;
    }
}

void TypeLowering::LowerTypedMod(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberMod(gate);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedIsTrue(GateRef gate, GateType value)
{
    GateRef boolValue = Circuit::NullGate();
    if (value.IsNumberType()) {
        boolValue = ConvertNumberToBool(gate, value);
    } else if (value.IsBooleanType()) {
        boolValue = ConvertBooleanToBool(gate);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    GateRef result = builder_.BooleanToTaggedBooleanPtr(boolValue);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeLowering::LowerTypedIsFalse(GateRef gate, GateType value)
{
    GateRef boolValue = Circuit::NullGate();
    if (value.IsNumberType()) {
        boolValue = ConvertNumberToBool(gate, value);
    } else if (value.IsBooleanType()) {
        boolValue = ConvertBooleanToBool(gate);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    boolValue = builder_.BoolNot(boolValue);

    GateRef result = builder_.BooleanToTaggedBooleanPtr(boolValue);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeLowering::LowerNumberMod(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = ModNumbers(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

GateRef TypeLowering::ConvertNumberToBool(GateRef gate, GateType valueType)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    GateRef value = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::BOOL(), builder_.HoleConstant());
    if (valueType.IsIntType()) {
        // value is int
        GateRef intVal = builder_.GetInt64OfTInt(value);
        result = builder_.BoolNot(builder_.Equal(intVal, builder_.Int64(0)));
    } else if (valueType.IsDoubleType()) {
        // value is double
        GateRef doubleVal = builder_.GetDoubleOfTDouble(value);
        GateRef doubleNotZero = builder_.BoolNot(builder_.Equal(doubleVal, builder_.Double(0)));
        GateRef doubleNotNAN = builder_.BoolNot(builder_.DoubleIsNAN(doubleVal));
        result = builder_.BoolAnd(doubleNotZero, doubleNotNAN);
    } else {
        // value is number and need typecheck in runtime
        Label valueIsInt(&builder_);
        Label valueIsDouble(&builder_);
        Label exit(&builder_);
        builder_.Branch(builder_.TaggedIsInt(value), &valueIsInt, &valueIsDouble);
        builder_.Bind(&valueIsInt);
        {
            GateRef intVal = builder_.GetInt64OfTInt(value);
            result = builder_.BoolNot(builder_.Equal(intVal, builder_.Int64(0)));
            builder_.Jump(&exit);
        }
        builder_.Bind(&valueIsDouble);
        {
            GateRef doubleVal = builder_.GetDoubleOfTDouble(value);
            GateRef doubleNotZero = builder_.BoolNot(builder_.Equal(doubleVal, builder_.Double(0)));
            GateRef doubleNotNAN = builder_.BoolNot(builder_.DoubleIsNAN(doubleVal));
            result = builder_.BoolAnd(doubleNotZero, doubleNotNAN);
            builder_.Jump(&exit);
        }
        builder_.Bind(&exit);
    }
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef TypeLowering::ConvertBooleanToBool(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef result = builder_.TaggedIsTrue(value);
    return result;
}

template<OpCode Op, MachineType Type>
GateRef TypeLowering::BinaryOp(GateRef x, GateRef y)
{
    return builder_.BinaryOp<Op, Type>(x, y);
}

GateRef TypeLowering::DoubleToTaggedDoublePtr(GateRef gate)
{
    return builder_.DoubleToTaggedDoublePtr(gate);
}

GateRef TypeLowering::ChangeInt32ToFloat64(GateRef gate)
{
    return builder_.ChangeInt32ToFloat64(gate);
}

GateRef TypeLowering::TruncDoubleToInt(GateRef gate)
{
    return builder_.TruncInt64ToInt32(builder_.TruncFloatToInt64(gate));
}

GateRef TypeLowering::Int32Mod(GateRef left, GateRef right)
{
    return builder_.BinaryArithmetic(circuit_->Smod(), MachineType::I32, left, right);
}

GateRef TypeLowering::DoubleMod(GateRef left, GateRef right)
{
    return builder_.BinaryArithmetic(circuit_->Fmod(), MachineType::F64, left, right);
}

GateRef TypeLowering::IntToTaggedIntPtr(GateRef x)
{
    GateRef val = builder_.SExtInt32ToInt64(x);
    return builder_.ToTaggedIntPtr(val);
}

GateRef TypeLowering::FastDiv(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    Label leftIsNumber(&builder_);
    Label leftNotNumberOrRightNotNumber(&builder_);
    Label leftIsNumberAndRightIsNumber(&builder_);
    Label leftIsDoubleAndRightIsDouble(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    builder_.Bind(&leftIsNumber);
    {
        Label rightIsNumber(&builder_);
        builder_.Branch(builder_.TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        builder_.Bind(&rightIsNumber);
        {
            Label leftIsInt(&builder_);
            Label leftNotInt(&builder_);
            builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftNotInt);
            builder_.Bind(&leftIsInt);
            {
                doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
                builder_.Jump(&leftIsNumberAndRightIsNumber);
            }
            builder_.Bind(&leftNotInt);
            {
                doubleLeft = builder_.GetDoubleOfTDouble(left);
                builder_.Jump(&leftIsNumberAndRightIsNumber);
            }
        }
    }
    builder_.Bind(&leftNotNumberOrRightNotNumber);
    builder_.Jump(&exit);
    builder_.Bind(&leftIsNumberAndRightIsNumber);
    {
        Label rightIsInt(&builder_);
        Label rightNotInt(&builder_);
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &rightNotInt);
        builder_.Bind(&rightIsInt);
        {
            doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
            builder_.Jump(&leftIsDoubleAndRightIsDouble);
        }
        builder_.Bind(&rightNotInt);
        {
            doubleRight = builder_.GetDoubleOfTDouble(right);
            builder_.Jump(&leftIsDoubleAndRightIsDouble);
        }
    }
    builder_.Bind(&leftIsDoubleAndRightIsDouble);
    {
        Label rightIsZero(&builder_);
        Label rightNotZero(&builder_);
        builder_.Branch(builder_.Equal(*doubleRight, builder_.Double(0.0)), &rightIsZero, &rightNotZero);
        builder_.Bind(&rightIsZero);
        {
            Label leftIsZero(&builder_);
            Label leftNotZero(&builder_);
            Label leftIsZeroOrNan(&builder_);
            Label leftNotZeroAndNotNan(&builder_);
            builder_.Branch(builder_.Equal(*doubleLeft, builder_.Double(0.0)), &leftIsZero, &leftNotZero);
            builder_.Bind(&leftIsZero);
            {
                builder_.Jump(&leftIsZeroOrNan);
            }
            builder_.Bind(&leftNotZero);
            {
                Label leftIsNan(&builder_);
                builder_.Branch(builder_.DoubleIsNAN(*doubleLeft), &leftIsNan, &leftNotZeroAndNotNan);
                builder_.Bind(&leftIsNan);
                {
                    builder_.Jump(&leftIsZeroOrNan);
                }
            }
            builder_.Bind(&leftIsZeroOrNan);
            {
                result = DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
                builder_.Jump(&exit);
            }
            builder_.Bind(&leftNotZeroAndNotNan);
            {
                GateRef intLeftTmp = builder_.CastDoubleToInt64(*doubleLeft);
                GateRef intRightTmp = builder_.CastDoubleToInt64(*doubleRight);
                GateRef flagBit = builder_.Int64And(builder_.Int64Xor(intLeftTmp, intRightTmp),
                                                    builder_.Int64(base::DOUBLE_SIGN_MASK));
                GateRef tmpResult =
                    builder_.Int64Xor(flagBit, builder_.CastDoubleToInt64(builder_.Double(base::POSITIVE_INFINITY)));
                result = DoubleToTaggedDoublePtr(builder_.CastInt64ToFloat64(tmpResult));
                builder_.Jump(&exit);
            }
        }
        builder_.Bind(&rightNotZero);
        {
            result = DoubleToTaggedDoublePtr(
                builder_.BinaryArithmetic(circuit_->Fdiv(), MachineType::F64, *doubleLeft, *doubleRight));
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef TypeLowering::ModNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(intLeft, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVAlUE(intRight, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0));

    Label exit(&builder_);
    Label doFloatOp(&builder_);
    Label doIntOp(&builder_);
    Label leftIsIntRightIsDouble(&builder_);
    Label rightIsInt(&builder_);
    Label rightIsDouble(&builder_);
    Label leftIsInt(&builder_);
    Label leftIsDouble(&builder_);
    bool intOptAccessed = false;

    auto LowerIntOpInt = [&]() -> void {
        builder_.Jump(&doIntOp);
        intOptAccessed = true;
    };
    auto LowerDoubleOpInt = [&]() -> void {
        doubleLeft = builder_.GetDoubleOfTDouble(left);
        doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
        builder_.Jump(&doFloatOp);
    };
    auto LowerIntOpDouble = [&]() -> void {
        doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
        doubleRight = builder_.GetDoubleOfTDouble(right);
        builder_.Jump(&doFloatOp);
    };
    auto LowerDoubleOpDouble = [&]() -> void {
        doubleLeft = builder_.GetDoubleOfTDouble(left);
        doubleRight = builder_.GetDoubleOfTDouble(right);
        builder_.Jump(&doFloatOp);
    };
    auto LowerRightWhenLeftIsInt = [&]() -> void {
        if (rightType.IsIntType()) {
            LowerIntOpInt();
        } else if (rightType.IsDoubleType()) {
            LowerIntOpDouble();
        } else {
            builder_.Branch(builder_.TaggedIsInt(right), &doIntOp, &leftIsIntRightIsDouble);
            intOptAccessed = true;
            builder_.Bind(&leftIsIntRightIsDouble);
            LowerIntOpDouble();
        }
    };
    auto LowerRightWhenLeftIsDouble = [&]() -> void {
        if (rightType.IsIntType()) {
            LowerDoubleOpInt();
        } else if (rightType.IsDoubleType()) {
            LowerDoubleOpDouble();
        } else {
            builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &rightIsDouble);
            builder_.Bind(&rightIsInt);
            LowerDoubleOpInt();
            builder_.Bind(&rightIsDouble);
            LowerDoubleOpDouble();
        }
    };

    if (leftType.IsIntType()) {
        // left is int
        LowerRightWhenLeftIsInt();
    } else if (leftType.IsDoubleType()) {
        // left is double
        LowerRightWhenLeftIsDouble();
    } else {
        // left is number and need typecheck in runtime
        builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftIsDouble);
        builder_.Bind(&leftIsInt);
        LowerRightWhenLeftIsInt();
        builder_.Bind(&leftIsDouble);
        LowerRightWhenLeftIsDouble();
    }
    if (intOptAccessed) {
        builder_.Bind(&doIntOp);
        {
            intLeft = builder_.GetInt32OfTInt(left);
            intRight = builder_.GetInt32OfTInt(right);
            doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
            doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
            Label leftGreaterZero(&builder_);
            builder_.Branch(builder_.Int32GreaterThan(*intLeft, builder_.Int32(0)),
                            &leftGreaterZero, &doFloatOp);
            builder_.Bind(&leftGreaterZero);
            {
                Label rightGreaterZero(&builder_);
                builder_.Branch(builder_.Int32GreaterThan(*intRight, builder_.Int32(0)),
                                &rightGreaterZero, &doFloatOp);
                builder_.Bind(&rightGreaterZero);
                {
                    result = IntToTaggedIntPtr(Int32Mod(*intLeft, *intRight));
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&doFloatOp);
    {
        Label rightNotZero(&builder_);
        Label rightIsZeroOrNanOrLeftIsNanOrInf(&builder_);
        Label rightNotZeroAndNanAndLeftNotNanAndInf(&builder_);
        builder_.Branch(builder_.Equal(*doubleRight, builder_.Double(0.0)), &rightIsZeroOrNanOrLeftIsNanOrInf,
                        &rightNotZero);
        builder_.Bind(&rightNotZero);
        {
            Label rightNotNan(&builder_);
            builder_.Branch(builder_.DoubleIsNAN(*doubleRight), &rightIsZeroOrNanOrLeftIsNanOrInf, &rightNotNan);
            builder_.Bind(&rightNotNan);
            {
                Label leftNotNan(&builder_);
                builder_.Branch(builder_.DoubleIsNAN(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf, &leftNotNan);
                builder_.Bind(&leftNotNan);
                builder_.Branch(builder_.DoubleIsINF(*doubleLeft),
                                &rightIsZeroOrNanOrLeftIsNanOrInf,
                                &rightNotZeroAndNanAndLeftNotNanAndInf);
            }
        }
        builder_.Bind(&rightIsZeroOrNanOrLeftIsNanOrInf);
        {
            result = DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
            builder_.Jump(&exit);
        }
        builder_.Bind(&rightNotZeroAndNanAndLeftNotNanAndInf);
        {
            Label leftNotZero(&builder_);
            Label leftIsZeroOrRightIsInf(&builder_);
            builder_.Branch(builder_.Equal(*doubleLeft, builder_.Double(0.0)), &leftIsZeroOrRightIsInf,
                            &leftNotZero);
            builder_.Bind(&leftNotZero);
            {
                Label rightNotInf(&builder_);
                builder_.Branch(builder_.DoubleIsINF(*doubleRight), &leftIsZeroOrRightIsInf, &rightNotInf);
                builder_.Bind(&rightNotInf);
                {
                    GateRef glue = acc_.GetGlueFromArgList();
                    result = builder_.CallNGCRuntime(
                        glue, RTSTUB_ID(FloatMod), Gate::InvalidGateRef, {*doubleLeft, *doubleRight},
                        Circuit::NullGate());
                    builder_.Jump(&exit);
                }
            }
            builder_.Bind(&leftIsZeroOrRightIsInf);
            {
                result = DoubleToTaggedDoublePtr(*doubleLeft);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef TypeLowering::DivNumbers(GateRef left, GateRef right,
                                 GateType leftType, GateType rightType)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    Label doFloatOp(&builder_);
    Label leftIsIntRightIsInt(&builder_);
    Label leftIsIntRightIsDouble(&builder_);
    Label rightIsInt(&builder_);
    Label rightIsDouble(&builder_);
    Label leftIsInt(&builder_);
    Label leftIsDouble(&builder_);
    Label exit(&builder_);

    auto LowerIntOpInt = [&]() -> void {
        doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
        doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
        builder_.Jump(&doFloatOp);
    };
    auto LowerDoubleOpInt = [&]() -> void {
        doubleLeft = builder_.GetDoubleOfTDouble(left);
        doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
        builder_.Jump(&doFloatOp);
    };
    auto LowerIntOpDouble = [&]() -> void {
        doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
        doubleRight = builder_.GetDoubleOfTDouble(right);
        builder_.Jump(&doFloatOp);
    };
    auto LowerDoubleOpDouble = [&]() -> void {
        doubleLeft = builder_.GetDoubleOfTDouble(left);
        doubleRight = builder_.GetDoubleOfTDouble(right);
        builder_.Jump(&doFloatOp);
    };
    auto LowerRightWhenLeftIsInt = [&]() -> void {
        if (rightType.IsIntType()) {
            LowerIntOpInt();
        } else if (rightType.IsDoubleType()) {
            LowerIntOpDouble();
        } else {
            builder_.Branch(builder_.TaggedIsInt(right), &leftIsIntRightIsInt, &leftIsIntRightIsDouble);
            builder_.Bind(&leftIsIntRightIsInt);
            LowerIntOpInt();
            builder_.Bind(&leftIsIntRightIsDouble);
            LowerIntOpDouble();
        }
    };
    auto LowerRightWhenLeftIsDouble = [&]() -> void {
        if (rightType.IsIntType()) {
            LowerDoubleOpInt();
        } else if (rightType.IsDoubleType()) {
            LowerDoubleOpDouble();
        } else {
            builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &rightIsDouble);
            builder_.Bind(&rightIsInt);
            LowerDoubleOpInt();
            builder_.Bind(&rightIsDouble);
            LowerDoubleOpDouble();
        }
    };

    if (leftType.IsIntType()) {
        // left is int
        LowerRightWhenLeftIsInt();
    } else if (leftType.IsDoubleType()) {
        // left is double
        LowerRightWhenLeftIsDouble();
    } else {
        // left is number and need typecheck in runtime
        builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftIsDouble);
        builder_.Bind(&leftIsInt);
        LowerRightWhenLeftIsInt();
        builder_.Bind(&leftIsDouble);
        LowerRightWhenLeftIsDouble();
    }

    builder_.Bind(&doFloatOp);
    {
        Label rightIsZero(&builder_);
        Label rightNotZero(&builder_);
        builder_.Branch(builder_.Equal(*doubleRight, builder_.Double(0.0)), &rightIsZero, &rightNotZero);
        builder_.Bind(&rightIsZero);
        {
            Label leftIsZero(&builder_);
            Label leftNotZero(&builder_);
            Label leftIsZeroOrNan(&builder_);
            Label leftNotZeroAndNotNan(&builder_);
            builder_.Branch(builder_.Equal(*doubleLeft, builder_.Double(0.0)), &leftIsZero, &leftNotZero);
            builder_.Bind(&leftIsZero);
            {
                builder_.Jump(&leftIsZeroOrNan);
            }
            builder_.Bind(&leftNotZero);
            {
                Label leftIsNan(&builder_);
                builder_.Branch(builder_.DoubleIsNAN(*doubleLeft), &leftIsNan, &leftNotZeroAndNotNan);
                builder_.Bind(&leftIsNan);
                {
                    builder_.Jump(&leftIsZeroOrNan);
                }
            }
            builder_.Bind(&leftIsZeroOrNan);
            {
                result = DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
                builder_.Jump(&exit);
            }
            builder_.Bind(&leftNotZeroAndNotNan);
            {
                GateRef intLeftTmp = builder_.CastDoubleToInt64(*doubleLeft);
                GateRef intRightTmp = builder_.CastDoubleToInt64(*doubleRight);
                GateRef flagBit = builder_.Int64And(builder_.Int64Xor(intLeftTmp, intRightTmp),
                                                    builder_.Int64(base::DOUBLE_SIGN_MASK));
                GateRef tmpResult =
                    builder_.Int64Xor(flagBit, builder_.CastDoubleToInt64(builder_.Double(base::POSITIVE_INFINITY)));
                result = DoubleToTaggedDoublePtr(builder_.CastInt64ToFloat64(tmpResult));
                builder_.Jump(&exit);
            }
        }
        builder_.Bind(&rightNotZero);
        {
            result = DoubleToTaggedDoublePtr(
                builder_.BinaryArithmetic(circuit_->Fdiv(), MachineType::F64, *doubleLeft, *doubleRight));
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef TypeLowering::FastEqual(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);

    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    Label leftEqRight(&builder_);
    Label leftNotEqRight(&builder_);
    Label exit(&builder_);

    builder_.Branch(builder_.Equal(left, right), &leftEqRight, &leftNotEqRight);
    builder_.Bind(&leftEqRight);
    {
        Label leftIsDouble(&builder_);
        Label leftNotDoubleOrLeftNotNan(&builder_);
        builder_.Branch(builder_.TaggedIsDouble(left), &leftIsDouble, &leftNotDoubleOrLeftNotNan);
        builder_.Bind(&leftIsDouble);
        {
            GateRef doubleLeft = builder_.GetDoubleOfTDouble(left);
            Label leftIsNan(&builder_);
            builder_.Branch(builder_.DoubleIsNAN(doubleLeft), &leftIsNan, &leftNotDoubleOrLeftNotNan);
            builder_.Bind(&leftIsNan);
            {
                result = builder_.TaggedFalse();
                builder_.Jump(&exit);
            }
        }
        builder_.Bind(&leftNotDoubleOrLeftNotNan);
        {
            result = builder_.TaggedTrue();
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&leftNotEqRight);
    {
        Label leftIsNumber(&builder_);
        Label leftNotNumberOrLeftNotIntOrRightNotInt(&builder_);
        builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrLeftNotIntOrRightNotInt);
        builder_.Bind(&leftIsNumber);
        {
            Label leftIsInt(&builder_);
            builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftNotNumberOrLeftNotIntOrRightNotInt);
            {
                builder_.Bind(&leftIsInt);
                {
                    Label rightIsInt(&builder_);
                    builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &leftNotNumberOrLeftNotIntOrRightNotInt);
                    builder_.Bind(&rightIsInt);
                    {
                        result = builder_.TaggedFalse();
                        builder_.Jump(&exit);
                    }
                }
            }
        }
        builder_.Bind(&leftNotNumberOrLeftNotIntOrRightNotInt);
        {
            Label rightIsUndefinedOrNull(&builder_);
            Label leftOrRightNotUndefinedOrNull(&builder_);
            builder_.Branch(builder_.TaggedIsUndefinedOrNull(right), &rightIsUndefinedOrNull,
                            &leftOrRightNotUndefinedOrNull);
            builder_.Bind(&rightIsUndefinedOrNull);
            {
                Label leftIsHeapObject(&builder_);
                Label leftNotHeapObject(&builder_);
                builder_.Branch(builder_.TaggedIsHeapObject(left), &leftIsHeapObject, &leftNotHeapObject);
                builder_.Bind(&leftIsHeapObject);
                {
                    result = builder_.TaggedFalse();
                    builder_.Jump(&exit);
                }
                builder_.Bind(&leftNotHeapObject);
                {
                    Label leftIsUndefinedOrNull(&builder_);
                    builder_.Branch(builder_.TaggedIsUndefinedOrNull(left), &leftIsUndefinedOrNull,
                                    &leftOrRightNotUndefinedOrNull);
                    builder_.Bind(&leftIsUndefinedOrNull);
                    {
                        result = builder_.TaggedTrue();
                        builder_.Jump(&exit);
                    }
                }
            }
            builder_.Bind(&leftOrRightNotUndefinedOrNull);
            {
                Label leftIsBoolean(&builder_);
                Label leftNotBooleanOrRightNotSpecial(&builder_);
                builder_.Branch(builder_.TaggedIsBoolean(left), &leftIsBoolean, &leftNotBooleanOrRightNotSpecial);
                builder_.Bind(&leftIsBoolean);
                {
                    Label rightIsSpecial(&builder_);
                    builder_.Branch(builder_.TaggedIsSpecial(right), &rightIsSpecial, &leftNotBooleanOrRightNotSpecial);
                    builder_.Bind(&rightIsSpecial);
                    {
                        result = builder_.TaggedFalse();
                        builder_.Jump(&exit);
                    }
                }
                builder_.Bind(&leftNotBooleanOrRightNotSpecial);
                {
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void TypeLowering::LowerTypedCallBuitin(GateRef gate)
{
    BuiltinLowering lowering(circuit_);
    lowering.LowerTypedCallBuitin(gate);
}

void TypeLowering::LowerJSCallTargetTypeCheck(GateRef gate)
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
        GateRef isJsFunc = builder_.IsJSFunction(func);
        GateRef funcMethodTarget = builder_.GetMethodFromFunction(func);
        GateRef isAot = builder_.HasAotCode(funcMethodTarget);
        GateRef checkFunc = builder_.BoolAnd(isObj, isJsFunc);
        GateRef checkAot = builder_.BoolAnd(checkFunc, isAot);
        GateRef methodTarget = GetObjectFromConstPool(jsFunc, methodIndex);
        GateRef check = builder_.BoolAnd(checkAot, builder_.Equal(funcMethodTarget, methodTarget));
        builder_.DeoptCheck(check, frameState, DeoptType::NOTJSCALLTGT);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeLowering::LowerJSCallThisTargetTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (tsManager_->IsFunctionTypeKind(type)) {
        GateRef frameState = GetFrameState(gate);
        auto func = acc_.GetValueIn(gate, 0);
        GateRef isObj = builder_.TaggedIsHeapObject(func);
        GateRef isJsFunc = builder_.IsJSFunction(func);
        GateRef checkFunc = builder_.BoolAnd(isObj, isJsFunc);
        GateRef funcMethodTarget = builder_.GetMethodFromFunction(func);
        GateRef isAot = builder_.HasAotCode(funcMethodTarget);
        GateRef check = builder_.BoolAnd(checkFunc, isAot);
        builder_.DeoptCheck(check, frameState, DeoptType::NOTJSCALLTGT);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypeLowering::LowerCallTargetCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);

    BuiltinLowering lowering(circuit_);
    GateRef funcheck = lowering.LowerCallTargetCheck(&env, gate);
    GateRef check = lowering.CheckPara(gate, funcheck);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTCALLTGT);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerTypedNewAllocateThis(GateRef gate, GateRef glue)
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
        GateRef protoOrHclass = builder_.Load(VariableType::JS_ANY(), ctor,
                                              builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        GateRef ihclassIndex = acc_.GetValueIn(gate, 1);
        GateRef ihclass = GetObjectFromConstPool(jsFunc, ihclassIndex);
        GateRef check = builder_.Equal(protoOrHclass, ihclass);
        builder_.DeoptCheck(check, frameState);

        thisObj = builder_.CallStub(glue, gate, CommonStubCSigns::NewJSObject, { glue, protoOrHclass });
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    builder_.SetDepend(*thisObj);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *thisObj);
}

void TypeLowering::LowerTypedSuperAllocateThis(GateRef gate, GateRef glue)
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
        GateRef protoOrHclass = builder_.Load(VariableType::JS_ANY(), newTarget,
                                              builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        GateRef check = builder_.IsJSHClass(protoOrHclass);
        GateRef frameState = GetFrameState(gate);
        builder_.DeoptCheck(check, frameState);

        thisObj = builder_.CallStub(glue, gate, CommonStubCSigns::NewJSObject, { glue, protoOrHclass });
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    builder_.SetDepend(*thisObj);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *thisObj);
}

void TypeLowering::LowerGetSuperConstructor(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef ctor = acc_.GetValueIn(gate, 0);
    GateRef hclass = builder_.LoadHClass(ctor);
    GateRef protoOffset = builder_.IntPtr(JSHClass::PROTOTYPE_OFFSET);
    GateRef superCtor = builder_.Load(VariableType::JS_ANY(), hclass, protoOffset);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), superCtor);
}

GateRef TypeLowering::LoadVTable(GateRef object)
{
    GateRef hclass = builder_.LoadHClass(object);
    return builder_.Load(VariableType::JS_ANY(), hclass, builder_.IntPtr(JSHClass::VTABLE_OFFSET));
}

GateRef TypeLowering::GetOwnerFromVTable(GateRef vtable, GateRef offset)
{
    GateRef dataOffset = builder_.PtrAdd(offset, builder_.IntPtr(VTable::TupleItem::OWNER));
    return builder_.GetValueFromTaggedArray(vtable, dataOffset);
}

GateRef TypeLowering::GetOffsetFromVTable(GateRef vtable, GateRef offset)
{
    GateRef dataOffset = builder_.PtrAdd(offset, builder_.IntPtr(VTable::TupleItem::OFFSET));
    return builder_.TaggedGetInt(builder_.GetValueFromTaggedArray(vtable, dataOffset));
}

GateRef TypeLowering::LoadSupers(GateRef hclass)
{
    return builder_.Load(VariableType::JS_ANY(), hclass, builder_.IntPtr(JSHClass::SUPERS_OFFSET));
}

GateRef TypeLowering::GetLengthFromSupers(GateRef supers)
{
    return builder_.Load(VariableType::INT32(), supers, builder_.Int32(TaggedArray::EXTRACT_LENGTH_OFFSET));
}

GateRef TypeLowering::GetValueFromSupers(GateRef supers, GateRef index)
{
    GateRef val = builder_.GetValueFromTaggedArray(supers, index);
    return builder_.LoadObjectFromWeakRef(val);
}

GateRef TypeLowering::CallAccessor(GateRef glue, GateRef gate, GateRef function, GateRef receiver, AccessorMode mode,
                                   GateRef value)
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

void TypeLowering::ReplaceHirWithPendingException(GateRef hirGate, GateRef glue, GateRef state, GateRef depend,
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

void TypeLowering::LowerConvert(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate);
    ValueType srcType = acc_.GetSrcType(gate);
    ValueType dstType = acc_.GetDstType(gate);
    GateRef result = Circuit::NullGate();
    if (srcType == ValueType::BOOL) {
        ASSERT(dstType == ValueType::TAGGED_BOOLEAN);
        result = ConvertBoolToTaggedBoolean(value);
    } else if (srcType == ValueType::INT32) {
        ASSERT(dstType == ValueType::TAGGED_INT || dstType == ValueType::FLOAT64);
        if (dstType == ValueType::TAGGED_INT) {
            result = ConvertInt32ToTaggedInt(value);
        } else {
            result = ConvertInt32ToFloat64(value);
        }
    } else if (srcType == ValueType::FLOAT64) {
        ASSERT(dstType == ValueType::TAGGED_DOUBLE);
        result = ConvertFloat64ToTaggedDouble(value);
    } else {
        ASSERT((srcType == ValueType::TAGGED_BOOLEAN) && (dstType == ValueType::BOOL));
        result = ConvertBooleanToBool(gate);
    }
    acc_.ReplaceGate(gate, Circuit::NullGate(), Circuit::NullGate(), result);
}

GateRef TypeLowering::ConvertTaggedNumberToInt32(GateRef gate, Label *exit)
{
    DEFVAlUE(result, (&builder_), VariableType::INT32(), builder_.Int32(0));
    Label isInt(&builder_);
    Label isDouble(&builder_);
    Label toInt32(&builder_);
    builder_.Branch(builder_.TaggedIsInt(gate), &isInt, &isDouble);
    builder_.Bind(&isInt);
    result = ConvertTaggedIntToInt32(gate);
    builder_.Jump(exit);
    builder_.Bind(&isDouble);
    result = ConvertFloat64ToInt32(ConvertTaggedDoubleToFloat64(gate), &toInt32);
    builder_.Jump(exit);
    builder_.Bind(exit);
    return *result;
}

GateRef TypeLowering::ConvertTaggedNumberToFloat64(GateRef gate, Label *exit)
{
    DEFVAlUE(result, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    Label isInt(&builder_);
    Label isDouble(&builder_);
    builder_.Branch(builder_.TaggedIsInt(gate), &isInt, &isDouble);
    builder_.Bind(&isInt);
    result = ConvertInt32ToFloat64(ConvertTaggedIntToInt32(gate));
    builder_.Jump(exit);
    builder_.Bind(&isDouble);
    result = ConvertTaggedDoubleToFloat64(gate);
    builder_.Jump(exit);
    builder_.Bind(exit);
    return *result;
}

void TypeLowering::LowerCheckAndConvert(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    ValueType srcType = acc_.GetSrcType(gate);
    switch (srcType) {
        case ValueType::FLOAT64:
            LowerCheckFloat64AndConvert(gate);
            break;
        case ValueType::TAGGED_INT:
            LowerCheckTaggedIntAndConvert(gate);
            break;
        case ValueType::TAGGED_DOUBLE:
            LowerCheckTaggedDoubleAndConvert(gate);
            break;
        case ValueType::TAGGED_NUMBER:
            LowerCheckTaggedNumberAndConvert(gate);
            break;
        default:
            UNREACHABLE();
    }
}

void TypeLowering::LowerCheckFloat64AndConvert(GateRef gate)
{
    ASSERT(acc_.GetDstType(gate) == ValueType::INT32);
    Label exit(&builder_);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef result = ConvertFloat64ToInt32(value, &exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeLowering::LowerCheckTaggedIntAndConvert(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsInt(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTINT);
    GateRef result = Circuit::NullGate();
    ValueType dst = acc_.GetDstType(gate);
    ASSERT(dst == ValueType::INT32 || dst == ValueType::FLOAT64);
    if (dst == ValueType::INT32) {
        result = ConvertTaggedIntToInt32(value);
    } else {
        result = ConvertTaggedIntToFloat64(value);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeLowering::LowerCheckTaggedDoubleAndConvert(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsDouble(value);
    Label exit(&builder_);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTDOUBLE);
    GateRef result = Circuit::NullGate();
    ValueType dst = acc_.GetDstType(gate);
    ASSERT(dst == ValueType::INT32 || dst == ValueType::FLOAT64);
    if (dst == ValueType::INT32) {
        result = ConvertTaggedDoubleToInt32(value, &exit);
    } else {
        result = ConvertTaggedDoubleToFloat64(value);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeLowering::LowerCheckTaggedNumberAndConvert(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsNumber(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTNUMBER);
    GateRef result = Circuit::NullGate();
    ValueType dst = acc_.GetDstType(gate);
    ASSERT(dst == ValueType::INT32 || dst == ValueType::FLOAT64);
    Label exit(&builder_);
    if (dst == ValueType::INT32) {
        result = ConvertTaggedNumberToInt32(value, &exit);
    } else {
        result = ConvertTaggedNumberToFloat64(value, &exit);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

GateRef TypeLowering::ConvertBoolToTaggedBoolean(GateRef gate)
{
    return builder_.BooleanToTaggedBooleanPtr(gate);
}

GateRef TypeLowering::ConvertInt32ToFloat64(GateRef gate)
{
    return builder_.ChangeInt32ToFloat64(gate);
}

GateRef TypeLowering::ConvertInt32ToTaggedInt(GateRef gate)
{
    return builder_.Int32ToTaggedPtr(gate);
}

GateRef TypeLowering::ConvertFloat64ToInt32(GateRef gate, Label *exit)
{
    return builder_.DoubleToInt(gate, exit);
}

GateRef TypeLowering::ConvertFloat64ToTaggedDouble(GateRef gate)
{
    return builder_.DoubleToTaggedDoublePtr(gate);
}

GateRef TypeLowering::ConvertTaggedIntToInt32(GateRef gate)
{
    return builder_.GetInt32OfTInt(gate);
}

GateRef TypeLowering::ConvertTaggedIntToFloat64(GateRef gate)
{
    return builder_.ChangeInt32ToFloat64(builder_.GetInt32OfTInt(gate));
}

GateRef TypeLowering::ConvertTaggedDoubleToInt32(GateRef gate, Label *exit)
{
    return builder_.DoubleToInt(builder_.GetDoubleOfTDouble(gate), exit);
}

GateRef TypeLowering::ConvertTaggedDoubleToFloat64(GateRef gate)
{
    return builder_.GetDoubleOfTDouble(gate);
}
}  // namespace panda::ecmascript
