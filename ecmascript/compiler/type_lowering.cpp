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
    auto op = OpCode(acc_.GetOpCode(gate));
    switch (op) {
        case OpCode::PRIMITIVE_TYPE_CHECK:
            LowerPrimitiveTypeCheck(gate);
            break;
        case OpCode::ARRAY_CHECK:
            LowerArrayCheck(gate, glue);
            break;
        case OpCode::STABLE_ARRAY_CHECK:
            LowerStableArrayCheck(gate, glue);
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
            LowerLoadProperty(gate, glue);
            break;
        case OpCode::STORE_PROPERTY:
            LowerStoreProperty(gate, glue);
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
        case OpCode::HEAP_ALLOC:
            LowerHeapAllocate(gate, glue);
            break;
        case OpCode::TYPED_CALL:
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

void TypeLowering::LowerArrayCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);

    GateRef glueGlobalEnvOffset = builder_.IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(false));
    GateRef glueGlobalEnv = builder_.Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef receiverHClass = builder_.LoadHClass(receiver);
    GateRef arrayFunction =
        builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    GateRef protoOrHclass =
        builder_.Load(VariableType::JS_ANY(), arrayFunction,
                      builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    GateRef hcalssCheck = builder_.Equal(receiverHClass, protoOrHclass);
    builder_.DeoptCheck(hcalssCheck, frameState, DeoptType::NOTARRAY);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerStableArrayCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);

    GateRef glueGlobalEnvOffset = builder_.IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(false));
    GateRef glueGlobalEnv = builder_.Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef receiverHClass = builder_.LoadHClass(receiver);
    GateRef arrayFunction =
        builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    GateRef protoOrHclass =
        builder_.Load(VariableType::JS_ANY(), arrayFunction,
                      builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    GateRef hcalssCheck = builder_.Equal(receiverHClass, protoOrHclass);
    GateRef guardiansOffset = builder_.IntPtr(JSThread::GlueData::GetStableArrayElementsGuardiansOffset(false));
    GateRef guardians = builder_.Load(VariableType(MachineType::I64, GateType::BooleanType()), glue, guardiansOffset);
    GateRef guardiansCheck = builder_.Equal(guardians, builder_.TaggedTrue());
    GateRef check = builder_.BoolAnd(hcalssCheck, guardiansCheck);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTSARRAY);

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
        LowerClassInstanceCheck(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerClassInstanceCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    ArgumentAccessor argAcc(circuit_);
    GateRef jsFunc = argAcc.GetCommonArgGate(CommonArgIdx::FUNC);
    auto receiver = acc_.GetValueIn(gate, 0);
    auto receiverHClass = builder_.LoadHClass(receiver);
    auto hclassOffset = acc_.GetValueIn(gate, 1);
    GateRef hclass = GetObjectFromConstPool(jsFunc, hclassOffset);
    GateRef check = builder_.Equal(receiverHClass, hclass);
    builder_.DeoptCheck(check, frameState, DeoptType::WRONGHCLASS);

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
    GateRef hclassIndex = builder_.GetInt32OfTInt(acc_.GetValueIn(gate, 1));
    GateRef length =
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
    GateRef index = builder_.GetInt32OfTInt(acc_.GetValueIn(gate, 1));
    GateRef length =
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
    GateRef intVal = builder_.GetInt64OfTInt(value);
    GateRef max = builder_.Int64(INT32_MAX);
    GateRef rangeCheck = builder_.Int64NotEqual(intVal, max);
    builder_.DeoptCheck(rangeCheck, frameState, DeoptType::NOTINCOV);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerTypedDecOverflowCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef intVal = builder_.GetInt64OfTInt(value);
    GateRef min = builder_.Int64(INT32_MIN);
    GateRef rangeCheck = builder_.Int64NotEqual(intVal, min);
    builder_.DeoptCheck(rangeCheck, frameState, DeoptType::NOTDECOV);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerTypedNegOverflowCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef intVal = builder_.GetInt64OfTInt(value);
    GateRef min = builder_.Int64(INT32_MIN);
    GateRef zero = builder_.Int64(0);
    GateRef notMin = builder_.Int64NotEqual(intVal, min);
    GateRef notZero = builder_.Int64NotEqual(intVal, zero);
    GateRef rangeCheck = builder_.BoolAnd(notMin, notZero);
    builder_.DeoptCheck(rangeCheck, frameState, DeoptType::NOTNEGOV);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

GateRef TypeLowering::LowerCallRuntime(GateRef glue, int index, const std::vector<GateRef> &args, bool useLabel)
{
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args);
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, dependEntry_, args);
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

GateRef TypeLowering::GetConstPool(GateRef jsFunc)
{
    GateRef method = builder_.GetMethodFromFunction(jsFunc);
    return builder_.Load(VariableType::JS_ANY(), method, builder_.IntPtr(Method::CONSTANT_POOL_OFFSET));
}

GateRef TypeLowering::GetObjectFromConstPool(GateRef jsFunc, GateRef index)
{
    GateRef constPool = GetConstPool(jsFunc);
    return builder_.GetValueFromTaggedArray(constPool, index);
}

void TypeLowering::LowerLoadProperty(GateRef gate, [[maybe_unused]] GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    Label hole(&builder_);
    Label exit(&builder_);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef offset = acc_.GetValueIn(gate, 1);
    result = builder_.Load(VariableType::JS_ANY(), receiver, offset);
    // simplify the process, need to query the vtable to complete the whole process later
    builder_.Branch(builder_.IsSpecial(*result, JSTaggedValue::VALUE_HOLE), &hole, &exit);
    builder_.Bind(&hole);
    {
        result = builder_.UndefineConstant();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerStoreProperty(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef offset = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);
    builder_.Store(VariableType::JS_ANY(), glue, receiver, offset, value);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerLoadArrayLength(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef array = acc_.GetValueIn(gate, 0);
    GateRef offset = builder_.IntPtr(JSArray::LENGTH_OFFSET);
    GateRef result = builder_.Load(VariableType::JS_ANY(), array, offset);
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
            UNREACHABLE();
    }
}

// for JSArray
void TypeLowering::LowerArrayLoadElement(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef element =
        builder_.Load(VariableType::JS_POINTER(), receiver, builder_.IntPtr(JSObject::ELEMENTS_OFFSET));
    GateRef index = acc_.GetValueIn(gate, 1);
    index = builder_.GetInt32OfTInt(index);
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
    index = builder_.GetInt32OfTInt(index);
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
        result = LowerCallRuntime(glue, RTSTUB_ID(AllocateInYoung),  {builder_.ToTaggedInt(size)}, true);
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
        case TypedBinOp::TYPED_ADD: {
            LowerTypedAdd(gate);
            break;
        }
        case TypedBinOp::TYPED_SUB: {
            LowerTypedSub(gate);
            break;
        }
        case TypedBinOp::TYPED_MUL: {
            LowerTypedMul(gate);
            break;
        }
        case TypedBinOp::TYPED_MOD: {
            LowerTypedMod(gate);
            break;
        }
        case TypedBinOp::TYPED_LESS: {
            LowerTypedLess(gate);
            break;
        }
        case TypedBinOp::TYPED_LESSEQ: {
            LowerTypedLessEq(gate);
            break;
        }
        case TypedBinOp::TYPED_GREATER: {
            LowerTypedGreater(gate);
            break;
        }
        case TypedBinOp::TYPED_GREATEREQ: {
            LowerTypedGreaterEq(gate);
            break;
        }
        case TypedBinOp::TYPED_DIV: {
            LowerTypedDiv(gate);
            break;
        }
        case TypedBinOp::TYPED_EQ: {
            LowerTypedEq(gate);
            break;
        }
        case TypedBinOp::TYPED_NOTEQ: {
            LowerTypedNotEq(gate);
            break;
        }
        case TypedBinOp::TYPED_SHL: {
            LowerTypedShl(gate);
            break;
        }
        case TypedBinOp::TYPED_SHR: {
            LowerTypedShr(gate);
            break;
        }
        case TypedBinOp::TYPED_ASHR: {
            LowerTypedAshr(gate);
            break;
        }
        case TypedBinOp::TYPED_AND: {
            LowerTypedAnd(gate);
            break;
        }
        case TypedBinOp::TYPED_OR: {
            LowerTypedOr(gate);
            break;
        }
        case TypedBinOp::TYPED_XOR: {
            LowerTypedXor(gate);
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
        case TypedUnOp::TYPED_NEG:
            LowerTypedNeg(gate, valueType);
            break;
        case TypedUnOp::TYPED_NOT:
            LowerTypedNot(gate, valueType);
            break;
        case TypedUnOp::TYPED_INC:
            LowerTypedInc(gate, valueType);
            break;
        case TypedUnOp::TYPED_DEC:
            LowerTypedDec(gate, valueType);
            break;
        case TypedUnOp::TYPED_TOBOOL:
            LowerTypedToBool(gate, valueType);
            break;
        default:
            LOG_COMPILER(FATAL) << "unkown TypedUnOp: " << static_cast<int>(op);
            break;
    }
}

void TypeLowering::LowerTypedAdd(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberAdd(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedSub(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberSub(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedMul(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberMul(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedMod(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberMod(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedLess(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberLess(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedLessEq(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberLessEq(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedGreater(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberGreater(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedGreaterEq(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberGreaterEq(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedDiv(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberDiv(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedEq(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberEq(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedNotEq(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberNotEq(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedShl(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberShl(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedShr(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberShr(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedAshr(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberAshr(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedAnd(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberAnd(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedOr(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberOr(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedXor(GateRef gate)
{
    auto leftType = acc_.GetLeftType(gate);
    auto rightType = acc_.GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberXor(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedInc(GateRef gate, GateType value)
{
    if (value.IsNumberType()) {
        LowerNumberInc(gate, value);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedDec(GateRef gate, GateType value)
{
    if (value.IsNumberType()) {
        LowerNumberDec(gate, value);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedNeg(GateRef gate, GateType value)
{
    if (value.IsNumberType()) {
        LowerNumberNeg(gate, value);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedToBool(GateRef gate, GateType value)
{
    if (value.IsNumberType()) {
        LowerNumberToBool(gate, value);
    } else if (value.IsBooleanType()) {
        LowerBooleanToBool(gate);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerTypedNot(GateRef gate, GateType value)
{
    if (value.IsNumberType()) {
        LowerNumberNot(gate, value);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerNumberAdd(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CalculateNumbers<OpCode::ADD>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberSub(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CalculateNumbers<OpCode::SUB>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberMul(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CalculateNumbers<OpCode::MUL>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
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

void TypeLowering::LowerNumberLess(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_LESS>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberLessEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_LESSEQ>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberGreater(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_LESS>(right, left, rightType, leftType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberGreaterEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_LESSEQ>(right, left, rightType, leftType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberDiv(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = DivNumbers(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_EQ>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberNotEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_NOTEQ>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberShl(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = ShiftNumber<OpCode::LSL>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberShr(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = ShiftNumber<OpCode::LSR>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberAshr(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = ShiftNumber<OpCode::ASR>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberAnd(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = LogicalNumbers<OpCode::AND>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberOr(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = LogicalNumbers<OpCode::OR>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberXor(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = LogicalNumbers<OpCode::XOR>(left, right, leftType, rightType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberInc(GateRef gate, GateType valueType)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = MonocularNumber<TypedUnOp::TYPED_INC>(value, valueType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberDec(GateRef gate, GateType valueType)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = MonocularNumber<TypedUnOp::TYPED_DEC>(value, valueType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberNeg(GateRef gate, GateType valueType)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    if (valueType.IsIntType()) {
        // value is int
        GateRef intVal = builder_.GetInt64OfTInt(value);
        GateRef res = BinaryOp<OpCode::SUB, MachineType::I64>(builder_.Int64(0), intVal);
        result = builder_.ToTaggedIntPtr(res);
    } else if (valueType.IsDoubleType()) {
        // value is double
        GateRef doubleVal = builder_.GetDoubleOfTDouble(value);
        GateRef res = BinaryOp<OpCode::SUB, MachineType::F64>(builder_.Double(0), doubleVal);
        result = DoubleToTaggedDoublePtr(res);
    } else {
        // value is number and need typecheck in runtime
        Label valueIsInt(&builder_);
        Label valueIsDouble(&builder_);
        Label exit(&builder_);
        builder_.Branch(builder_.TaggedIsInt(value), &valueIsInt, &valueIsDouble);
        builder_.Bind(&valueIsInt);
        {
            GateRef intVal = builder_.GetInt64OfTInt(value);
            Label overflow(&builder_);
            Label notOverflow(&builder_);
            GateRef min = builder_.Int64(INT32_MIN);
            builder_.Branch(builder_.Int64Equal(intVal, min), &overflow, &notOverflow);
            builder_.Bind(&overflow);
            {
                GateRef res = builder_.Double(static_cast<double>(JSTaggedValue::TAG_INT32_INC_MAX));
                result = builder_.DoubleToTaggedDoublePtr(res);
                builder_.Jump(&exit);
            }
            builder_.Bind(&notOverflow);
            {
                GateRef res = BinaryOp<OpCode::SUB, MachineType::I64>(builder_.Int64(0), intVal);
                result = builder_.ToTaggedIntPtr(res);
                builder_.Jump(&exit);
            }
        }
        builder_.Bind(&valueIsDouble);
        {
            GateRef doubleVal = builder_.GetDoubleOfTDouble(value);
            GateRef res = BinaryOp<OpCode::SUB, MachineType::F64>(builder_.Double(0), doubleVal);
            result = DoubleToTaggedDoublePtr(res);
            builder_.Jump(&exit);
        }
        builder_.Bind(&exit);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberToBool(GateRef gate, GateType valueType)
{
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
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerBooleanToBool(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef result = builder_.TaggedIsTrue(value);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypeLowering::LowerNumberNot(GateRef gate, GateType valueType)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = MonocularNumber<TypedUnOp::TYPED_NOT>(value, valueType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

template<OpCode Op>
GateRef TypeLowering::CalculateNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
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
            Label overflow(&builder_);
            Label notOverflow(&builder_);
            Label notOverflowOrUnderflow(&builder_);
            // handle left is int and right is int
            GateRef res = BinaryOp<Op, MachineType::I64>(builder_.GetInt64OfTInt(left),
                                                         builder_.GetInt64OfTInt(right));
            GateRef max = builder_.Int64(INT32_MAX);
            GateRef min = builder_.Int64(INT32_MIN);
            builder_.Branch(builder_.Int64GreaterThan(res, max), &overflow, &notOverflow);
            builder_.Bind(&notOverflow);
            {
                builder_.Branch(builder_.Int64LessThan(res, min), &overflow, &notOverflowOrUnderflow);
            }
            builder_.Bind(&overflow);
            {
                doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
                doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
                builder_.Jump(&doFloatOp);
            }
            builder_.Bind(&notOverflowOrUnderflow);
            {
                result = builder_.ToTaggedIntPtr(res);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&doFloatOp);
    {
        // Other situations
        auto res = BinaryOp<Op, MachineType::F64>(*doubleLeft, *doubleRight);
        result = DoubleToTaggedDoublePtr(res);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<OpCode Op>
GateRef TypeLowering::ShiftNumber(GateRef left, GateRef right,
                                  GateType leftType, GateType rightType)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(intLeft, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVAlUE(intRight, (&builder_), VariableType::INT32(), builder_.Int32(0));

    Label exit(&builder_);
    Label doIntOp(&builder_);
    Label leftIsIntRightIsInt(&builder_);
    Label leftIsIntRightIsDouble(&builder_);
    Label rightIsInt(&builder_);
    Label rightIsDouble(&builder_);
    Label leftIsInt(&builder_);
    Label leftIsDouble(&builder_);
    Label overflow(&builder_);
    Label notOverflow(&builder_);

    auto LowerIntOpInt = [&]() -> void {
        intLeft = builder_.GetInt32OfTInt(left);
        intRight = builder_.GetInt32OfTInt(right);
        builder_.Jump(&doIntOp);
    };
    auto LowerDoubleOpInt = [&]() -> void {
        intLeft = TruncDoubleToInt(builder_.GetDoubleOfTDouble(left));
        intRight = builder_.GetInt32OfTInt(right);
        builder_.Jump(&doIntOp);
    };
    auto LowerIntOpDouble = [&]() -> void {
        intLeft = builder_.GetInt32OfTInt(left);
        intRight = TruncDoubleToInt(builder_.GetDoubleOfTDouble(right));
        builder_.Jump(&doIntOp);
    };
    auto LowerDoubleOpDouble = [&]() -> void {
        intLeft = TruncDoubleToInt(builder_.GetDoubleOfTDouble(left));
        intRight = TruncDoubleToInt(builder_.GetDoubleOfTDouble(right));
        builder_.Jump(&doIntOp);
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
    builder_.Bind(&doIntOp);
    {
        GateRef res = Circuit::NullGate();
        GateRef shift = builder_.Int32And(*intRight, builder_.Int32(0x1f));
        switch (Op) {
            case OpCode::LSL: {
                res = builder_.Int32LSL(*intLeft, shift);
                result = IntToTaggedIntPtr(res);
                builder_.Jump(&exit);
                break;
            }
            case OpCode::LSR: {
                res = builder_.Int32LSR(*intLeft, shift);
                auto condition = builder_.Int32UnsignedGreaterThan(res, builder_.Int32(INT32_MAX));
                builder_.Branch(condition, &overflow, &notOverflow);
                builder_.Bind(&overflow);
                {
                    result = builder_.DoubleToTaggedDoublePtr(builder_.ChangeUInt32ToFloat64(res));
                    builder_.Jump(&exit);
                }
                builder_.Bind(&notOverflow);
                {
                    result = IntToTaggedIntPtr(res);
                    builder_.Jump(&exit);
                }
                break;
            }
            case OpCode::ASR: {
                res = builder_.Int32ASR(*intLeft, shift);
                result = IntToTaggedIntPtr(res);
                builder_.Jump(&exit);
                break;
            }
            default:
                UNREACHABLE();
                break;
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<OpCode Op>
GateRef TypeLowering::LogicalNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(intLeft, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVAlUE(intRight, (&builder_), VariableType::INT32(), builder_.Int32(0));

    Label exit(&builder_);
    Label doIntOp(&builder_);
    Label leftIsIntRightIsInt(&builder_);
    Label leftIsIntRightIsDouble(&builder_);
    Label rightIsInt(&builder_);
    Label rightIsDouble(&builder_);
    Label leftIsInt(&builder_);
    Label leftIsDouble(&builder_);

    auto LowerIntOpInt = [&]() -> void {
        intLeft = builder_.GetInt32OfTInt(left);
        intRight = builder_.GetInt32OfTInt(right);
        builder_.Jump(&doIntOp);
    };
    auto LowerDoubleOpInt = [&]() -> void {
        intLeft = TruncDoubleToInt(builder_.GetDoubleOfTDouble(left));
        intRight = builder_.GetInt32OfTInt(right);
        builder_.Jump(&doIntOp);
    };
    auto LowerIntOpDouble = [&]() -> void {
        intLeft = builder_.GetInt32OfTInt(left);
        intRight = TruncDoubleToInt(builder_.GetDoubleOfTDouble(right));
        builder_.Jump(&doIntOp);
    };
    auto LowerDoubleOpDouble = [&]() -> void {
        intLeft = TruncDoubleToInt(builder_.GetDoubleOfTDouble(left));
        intRight = TruncDoubleToInt(builder_.GetDoubleOfTDouble(right));
        builder_.Jump(&doIntOp);
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
    builder_.Bind(&doIntOp);
    {
        GateRef res = Circuit::NullGate();
        switch (Op) {
            case OpCode::AND: {
                res = builder_.Int32And(*intLeft, *intRight);
                break;
            }
            case OpCode::OR: {
                res = builder_.Int32Or(*intLeft, *intRight);
                break;
            }
            case OpCode::XOR: {
                res = builder_.Int32Xor(*intLeft, *intRight);
                break;
            }
            default:
                UNREACHABLE();
                break;
        }
        result = IntToTaggedIntPtr(res);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<TypedBinOp Op>
GateRef TypeLowering::CompareNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
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
    bool floatOptAccessed = false;

    auto LowerIntOpInt = [&]() -> void {
        builder_.Jump(&doIntOp);
        intOptAccessed = true;
    };
    auto LowerDoubleOpInt = [&]() -> void {
        doubleLeft = builder_.GetDoubleOfTDouble(left);
        doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
        builder_.Jump(&doFloatOp);
        floatOptAccessed = true;
    };
    auto LowerIntOpDouble = [&]() -> void {
        doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
        doubleRight = builder_.GetDoubleOfTDouble(right);
        builder_.Jump(&doFloatOp);
        floatOptAccessed = true;
    };
    auto LowerDoubleOpDouble = [&]() -> void {
        doubleLeft = builder_.GetDoubleOfTDouble(left);
        doubleRight = builder_.GetDoubleOfTDouble(right);
        builder_.Jump(&doFloatOp);
        floatOptAccessed = true;
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
            GateRef intLeft = builder_.GetInt32OfTInt(left);
            GateRef intRight = builder_.GetInt32OfTInt(right);
            auto cmp = builder_.ZExtInt1ToInt64(CompareInt<Op>(intLeft, intRight));
            auto res = builder_.Int64Or(cmp, builder_.Int64(JSTaggedValue::TAG_BOOLEAN_MASK));
            result = builder_.Int64ToTaggedPtr(res);
            builder_.Jump(&exit);
        }
    }
    if (floatOptAccessed) {
        builder_.Bind(&doFloatOp);
        {
            // Other situations
            auto cmp = builder_.ZExtInt1ToInt64(CompareDouble<Op>(*doubleLeft, *doubleRight));
            auto res = builder_.Int64Or(cmp, builder_.Int64(JSTaggedValue::TAG_BOOLEAN_MASK));
            result = builder_.Int64ToTaggedPtr(res);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<TypedBinOp Op>
GateRef TypeLowering::CompareInt(GateRef left, GateRef right)
{
    GateRef condition = Circuit::NullGate();
    switch (Op) {
        case TypedBinOp::TYPED_LESS:
            condition = builder_.Int32LessThan(left, right);
            break;
        case TypedBinOp::TYPED_LESSEQ:
            condition = builder_.Int32LessThanOrEqual(left, right);
            break;
        case TypedBinOp::TYPED_GREATER:
            condition = builder_.Int32GreaterThan(left, right);
            break;
        case TypedBinOp::TYPED_GREATEREQ:
            condition = builder_.Int32GreaterThanOrEqual(left, right);
            break;
        case TypedBinOp::TYPED_EQ:
            condition = builder_.Int32Equal(left, right);
            break;
        case TypedBinOp::TYPED_NOTEQ:
            condition = builder_.Int32NotEqual(left, right);
            break;
        default:
            break;
    }
    return condition;
}

template<TypedBinOp Op>
GateRef TypeLowering::CompareDouble(GateRef left, GateRef right)
{
    GateRef condition = Circuit::NullGate();
    switch (Op) {
        case TypedBinOp::TYPED_LESS:
            condition = builder_.DoubleLessThan(left, right);
            break;
        case TypedBinOp::TYPED_LESSEQ:
            condition = builder_.DoubleLessThanOrEqual(left, right);
            break;
        case TypedBinOp::TYPED_GREATER:
            condition = builder_.DoubleGreaterThan(left, right);
            break;
        case TypedBinOp::TYPED_GREATEREQ:
            condition = builder_.DoubleGreaterThanOrEqual(left, right);
            break;
        case TypedBinOp::TYPED_EQ:
            condition = builder_.DoubleEqual(left, right);
            break;
        case TypedBinOp::TYPED_NOTEQ:
            condition = builder_.DoubleNotEqual(left, right);
            break;
        default:
            break;
    }
    return condition;
}

template<TypedUnOp Op>
GateRef TypeLowering::MonocularNumber(GateRef value, GateType valueType)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(doubleVal, (&builder_), VariableType::FLOAT64(), builder_.Double(0));

    Label exit(&builder_);
    Label doFloatOp(&builder_);
    Label doIntOp(&builder_);
    Label valueIsInt(&builder_);
    Label valueIsDouble(&builder_);
    bool intOptAccessed = false;
    bool floatOptAccessed = false;

    auto LowerIntOp = [&]() -> void {
        builder_.Jump(&doIntOp);
        intOptAccessed = true;
    };
    auto LowerDoubleOp = [&]() -> void {
        doubleVal = builder_.GetDoubleOfTDouble(value);
        builder_.Jump(&doFloatOp);
        floatOptAccessed = true;
    };

    if (valueType.IsIntType()) {
        // value is int
        // no need to consider if value will overflow or underflow,
        // since guard has already checked that.
        GateRef intVal = builder_.GetInt64OfTInt(value);
        GateRef res = Circuit::NullGate();
        switch (Op) {
            case TypedUnOp::TYPED_INC: {
                res = builder_.Int64Add(intVal, builder_.Int64(1));
                result = builder_.ToTaggedIntPtr(res);
                builder_.Jump(&exit);
                break;
            }
            case TypedUnOp::TYPED_DEC: {
                res = builder_.Int64Sub(intVal, builder_.Int64(1));
                result = builder_.ToTaggedIntPtr(res);
                builder_.Jump(&exit);
                break;
            }
            case TypedUnOp::TYPED_NOT: {
                res = builder_.Int64Not(intVal);
                result = builder_.ToTaggedIntPtr(res);
                builder_.Jump(&exit);
                break;
            }
            default:
                break;
        }
    } else if (valueType.IsDoubleType()) {
        // value is double
        LowerDoubleOp();
    } else {
        // value is number and need typecheck in runtime
        builder_.Branch(builder_.TaggedIsInt(value), &valueIsInt, &valueIsDouble);
        builder_.Bind(&valueIsInt);
        LowerIntOp();
        builder_.Bind(&valueIsDouble);
        LowerDoubleOp();
    }
    if (intOptAccessed) {
        builder_.Bind(&doIntOp);
        {
            GateRef intVal = builder_.GetInt64OfTInt(value);
            GateRef res = Circuit::NullGate();
            switch (Op) {
                case TypedUnOp::TYPED_INC: {
                    Label overflow(&builder_);
                    Label notOverflow(&builder_);
                    GateRef max = builder_.Int64(INT32_MAX);
                    builder_.Branch(builder_.Int64Equal(intVal, max), &overflow, &notOverflow);
                    builder_.Bind(&overflow);
                    {
                        res = builder_.Double(JSTaggedValue::TAG_INT32_INC_MAX);
                        result = DoubleToTaggedDoublePtr(res);
                        builder_.Jump(&exit);
                    }
                    builder_.Bind(&notOverflow);
                    {
                        res = builder_.Int64Add(intVal, builder_.Int64(1));
                        result = builder_.ToTaggedIntPtr(res);
                        builder_.Jump(&exit);
                    }
                    break;
                }
                case TypedUnOp::TYPED_DEC: {
                    Label underflow(&builder_);
                    Label notUnderflow(&builder_);
                    GateRef min = builder_.Int64(INT32_MIN);
                    builder_.Branch(builder_.Int64Equal(intVal, min), &underflow, &notUnderflow);
                    builder_.Bind(&underflow);
                    {
                        res = builder_.Double(static_cast<double>(JSTaggedValue::TAG_INT32_DEC_MIN));
                        result = DoubleToTaggedDoublePtr(res);
                        builder_.Jump(&exit);
                    }
                    builder_.Bind(&notUnderflow);
                    {
                        res = builder_.Int64Sub(intVal, builder_.Int64(1));
                        result = builder_.ToTaggedIntPtr(res);
                        builder_.Jump(&exit);
                    }
                    break;
                }
                case TypedUnOp::TYPED_NOT: {
                    res = builder_.Int64Not(intVal);
                    result = builder_.ToTaggedIntPtr(res);
                    builder_.Jump(&exit);
                    break;
                }
                default:
                    break;
            }
        }
    }
    if (floatOptAccessed) {
        builder_.Bind(&doFloatOp);
        {
            auto res = Circuit::NullGate();
            switch (Op) {
                case TypedUnOp::TYPED_INC: {
                    res = builder_.DoubleAdd(*doubleVal, builder_.Double(1.0));
                    result = DoubleToTaggedDoublePtr(res);
                    break;
                }
                case TypedUnOp::TYPED_DEC: {
                    res = builder_.DoubleSub(*doubleVal, builder_.Double(1.0));
                    result = DoubleToTaggedDoublePtr(res);
                    break;
                }
                case TypedUnOp::TYPED_NOT: {
                    res = builder_.TruncFloatToInt64(*doubleVal);
                    result = builder_.ToTaggedIntPtr(builder_.Int64Not(res));
                    break;
                }
                default:
                    break;
            }
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
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

GateRef TypeLowering::Less(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    Label leftIsInt(&builder_);
    Label leftOrRightNotInt(&builder_);
    Label leftLessRight(&builder_);
    Label leftGreaterEqRight(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    builder_.Bind(&leftIsInt);
    {
        Label rightIsInt(&builder_);
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        builder_.Bind(&rightIsInt);
        {
            GateRef intLeft = builder_.GetInt32OfTInt(left);
            GateRef intRight = builder_.GetInt32OfTInt(right);
            builder_.Branch(builder_.Int32LessThan(intLeft, intRight), &leftLessRight, &leftGreaterEqRight);
        }
    }
    builder_.Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(&builder_);
        builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &exit);
        builder_.Bind(&leftIsNumber);
        {
            Label rightIsNumber(&builder_);
            builder_.Branch(builder_.TaggedIsNumber(right), &rightIsNumber, &exit);
            builder_.Bind(&rightIsNumber);
            {
                // fast path
                DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
                DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
                Label leftIsInt1(&builder_);
                Label leftNotInt1(&builder_);
                Label exit1(&builder_);
                Label exit2(&builder_);
                Label rightIsInt1(&builder_);
                Label rightNotInt1(&builder_);
                builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                builder_.Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
                    builder_.Jump(&exit1);
                }
                builder_.Bind(&leftNotInt1);
                {
                    doubleLeft = builder_.GetDoubleOfTDouble(left);
                    builder_.Jump(&exit1);
                }
                builder_.Bind(&exit1);
                builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                builder_.Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
                    builder_.Jump(&exit2);
                }
                builder_.Bind(&rightNotInt1);
                {
                    doubleRight = builder_.GetDoubleOfTDouble(right);
                    builder_.Jump(&exit2);
                }
                builder_.Bind(&exit2);
                builder_.Branch(builder_.DoubleLessThan(*doubleLeft, *doubleRight), &leftLessRight,
                                &leftGreaterEqRight);
            }
        }
    }
    builder_.Bind(&leftLessRight);
    {
        result = builder_.TaggedTrue();
        builder_.Jump(&exit);
    }
    builder_.Bind(&leftGreaterEqRight);
    {
        result = builder_.TaggedFalse();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef TypeLowering::LessEq(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label leftIsInt(&builder_);
    Label leftOrRightNotInt(&builder_);
    Label leftLessEqRight(&builder_);
    Label leftGreaterRight(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    builder_.Bind(&leftIsInt);
    {
        Label rightIsInt(&builder_);
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        builder_.Bind(&rightIsInt);
        {
            GateRef intLeft = builder_.GetInt32OfTInt(left);
            GateRef intRight = builder_.GetInt32OfTInt(right);
            builder_.Branch(builder_.Int32LessThanOrEqual(intLeft, intRight), &leftLessEqRight, &leftGreaterRight);
        }
    }
    builder_.Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(&builder_);
        builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &exit);
        builder_.Bind(&leftIsNumber);
        {
            Label rightIsNumber(&builder_);
            builder_.Branch(builder_.TaggedIsNumber(right), &rightIsNumber, &exit);
            builder_.Bind(&rightIsNumber);
            {
                // fast path
                DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
                DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
                Label leftIsInt1(&builder_);
                Label leftNotInt1(&builder_);
                Label exit1(&builder_);
                Label exit2(&builder_);
                Label rightIsInt1(&builder_);
                Label rightNotInt1(&builder_);
                builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                builder_.Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
                    builder_.Jump(&exit1);
                }
                builder_.Bind(&leftNotInt1);
                {
                    doubleLeft = builder_.GetDoubleOfTDouble(left);
                    builder_.Jump(&exit1);
                }
                builder_.Bind(&exit1);
                builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                builder_.Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
                    builder_.Jump(&exit2);
                }
                builder_.Bind(&rightNotInt1);
                {
                    doubleRight = builder_.GetDoubleOfTDouble(right);
                    builder_.Jump(&exit2);
                }
                builder_.Bind(&exit2);
                builder_.Branch(builder_.DoubleLessThanOrEqual(*doubleLeft, *doubleRight), &leftLessEqRight,
                                &leftGreaterRight);
            }
        }
    }
    builder_.Bind(&leftLessEqRight);
    {
        result = builder_.TaggedTrue();
        builder_.Jump(&exit);
    }
    builder_.Bind(&leftGreaterRight);
    {
        result = builder_.TaggedFalse();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
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
                        glue, RTSTUB_ID(FloatMod), Gate::InvalidGateRef, {*doubleLeft, *doubleRight});
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

void TypeLowering::LowerCallTargetCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);

    BuiltinLowering lowering(circuit_);
    GateRef funcheck = lowering.LowerCallTargetCheck(&env, gate);
    GateRef paracheck = lowering.CheckPara(gate);
    GateRef check = builder_.BoolAnd(paracheck, funcheck);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTCALLTGT);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerTypedNewAllocateThis(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ArgumentAccessor argAcc(circuit_);
    GateRef jsFunc = argAcc.GetCommonArgGate(CommonArgIdx::FUNC);

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
        GateRef frameState = GetFrameState(gate);
        builder_.DeoptCheck(check, frameState);

        NewObjectStubBuilder stubBuilder(&env);
        stubBuilder.SetGule(glue);
        stubBuilder.NewJSObject(&thisObj, &exit, protoOrHclass);
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

        NewObjectStubBuilder stubBuilder(&env);
        stubBuilder.SetGule(glue);
        stubBuilder.NewJSObject(&thisObj, &exit, protoOrHclass);
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
}  // namespace panda::ecmascript
