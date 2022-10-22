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
#include "ecmascript/deoptimizer.h"
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
        circuit_->PrintAllGates(*bcBuilder_);
        LOG_COMPILER(INFO) << "\033[34m" << "=========================== End =========================" << "\033[0m";
    }
}

void TypeLowering::LowerType(GateRef gate)
{
    ArgumentAccessor argAcc(circuit_);
    GateRef glue = argAcc.GetCommonArgGate(CommonArgIdx::GLUE);
    auto op = OpCode::Op(acc_.GetOpCode(gate));
    switch (op) {
        case OpCode::TYPE_CHECK:
            LowerTypeCheck(gate);
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
        case OpCode::OBJECT_TYPE_CHECK:
            LowerObjectTypeCheck(gate, glue);
            break;
        case OpCode::LOAD_PROPERTY:
            LowerLoadProperty(gate, glue);
            break;
        case OpCode::STORE_PROPERTY:
            LowerStoreProperty(gate, glue);
            break;
        case OpCode::LOAD_ELEMENT:
            LowerLoadElement(gate);
            break;
        case OpCode::STORE_ELEMENT:
            LowerStoreElement(gate, glue);
            break;
        default:
            break;
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
        if (leftType.IsPrimitiveType() && !leftType.IsStringType()) {
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
        UNREACHABLE();
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(dst, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerTypeCheck(GateRef gate)
{
    auto type = GateType(static_cast<uint32_t>(acc_.GetBitField(gate)));
    if (type.IsIntType()) {
        LowerIntCheck(gate);
    } else if (type.IsDoubleType()) {
        LowerDoubleCheck(gate);
    } else if (type.IsNumberType()) {
        LowerNumberCheck(gate);
    } else {
        UNREACHABLE();
    }
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

void TypeLowering::LowerObjectTypeCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    auto type = GateType(static_cast<uint32_t>(acc_.GetBitField(gate)));
    if (tsManager_->IsFloat32ArrayType(type)) {
        LowerFloat32ArrayCheck(gate, glue);
    } else if (tsManager_->IsClassInstanceTypeKind(type)) {
        LowerClassInstanceCheck(gate, glue);
    } else {
        UNREACHABLE();
    }
}

void TypeLowering::LowerClassInstanceCheck(GateRef gate, [[maybe_unused]] GateRef glue)
{
    ArgumentAccessor argAcc(circuit_);
    GateRef jsFunc = argAcc.GetCommonArgGate(CommonArgIdx::FUNC);
    auto receiver = acc_.GetValueIn(gate, 0);
    auto receiverHClass = builder_.LoadHClass(receiver);
    auto hclassOffset = acc_.GetValueIn(gate, 1);
    GateRef hclass = GetObjectFromConstPool(jsFunc, hclassOffset);
    GateRef check = builder_.Equal(receiverHClass, hclass);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), check);
}

void TypeLowering::LowerFloat32ArrayCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    bool is32Bit = builder_.GetCompilationConfig()->Is32Bit();
    GateRef glueGlobalEnvOffset = builder_.IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(is32Bit));
    GateRef glueGlobalEnv = builder_.Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);

    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef receiverHClass = builder_.LoadHClass(receiver);

    GateRef float32ArrayFunction =
        builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::FLOAT32_ARRAY_FUNCTION_INDEX);
    GateRef protoOrHclass =
        builder_.Load(VariableType::JS_ANY(), float32ArrayFunction,
                      builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    GateRef typeCheck = builder_.Equal(receiverHClass, protoOrHclass);

    auto bitfield = acc_.GetBitField(acc_.GetValueIn(gate, 1));
    GateRef index = builder_.Int32(bitfield);
    GateRef length =
            builder_.Load(VariableType::INT32(), receiver, builder_.IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET));
    GateRef lengthCheck = builder_.Int32UnsignedLessThan(index, length);

    GateRef check = builder_.BoolAnd(typeCheck, lengthCheck);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), check);
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

// for Float32Array
void TypeLowering::LowerLoadElement(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef arrbuffer =
        builder_.Load(VariableType::JS_POINTER(), receiver, builder_.IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET));
    GateRef data = builder_.Load(VariableType::JS_POINTER(), arrbuffer, builder_.IntPtr(JSArrayBuffer::DATA_OFFSET));
    GateRef block = builder_.Load(VariableType::JS_ANY(), data, builder_.IntPtr(JSNativePointer::POINTER_OFFSET));

    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef elementSize = builder_.Int32(4);  // 4: float32 occupy 4 bytes
    GateRef offset = builder_.PtrMul(index, elementSize);
    GateRef byteOffset =
        builder_.Load(VariableType::INT32(), receiver, builder_.IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET));

    GateRef res = builder_.Load(VariableType::FLOAT32(), block, builder_.PtrAdd(offset, byteOffset));
    GateRef result = builder_.Float32ToTaggedDoublePtr(res);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

// for Float32Array
void TypeLowering::LowerStoreElement(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef arrbuffer =
        builder_.Load(VariableType::JS_POINTER(), receiver, builder_.IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET));
    GateRef data = builder_.Load(VariableType::JS_POINTER(), arrbuffer, builder_.IntPtr(JSArrayBuffer::DATA_OFFSET));
    GateRef block = builder_.Load(VariableType::JS_ANY(), data, builder_.IntPtr(JSNativePointer::POINTER_OFFSET));

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
        Label exit(&builder_);
        builder_.Branch(builder_.TaggedIsInt(value), &valueIsInt, &valueIsDouble);
        builder_.Bind(&valueIsInt);
        {
            storeValue = builder_.TaggedIntPtrToFloat32(value);
            builder_.Jump(&exit);
        }
        builder_.Bind(&valueIsDouble);
        {
            storeValue = builder_.TaggedDoublePtrToFloat32(value);
            builder_.Jump(&exit);
        }
        builder_.Bind(&exit);
    }
    builder_.Store(VariableType::VOID(), glue, block, builder_.PtrAdd(offset, byteOffset), *storeValue);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypeLowering::LowerIntCheck(GateRef gate)
{
    auto value = acc_.GetValueIn(gate, 0);
    auto typeCheck = builder_.TaggedIsInt(value);
    acc_.UpdateAllUses(gate, typeCheck);
    acc_.DeleteGate(gate);
}

void TypeLowering::LowerDoubleCheck(GateRef gate)
{
    auto value = acc_.GetValueIn(gate, 0);
    auto typeCheck = builder_.TaggedIsDouble(value);
    acc_.UpdateAllUses(gate, typeCheck);
    acc_.DeleteGate(gate);
}

void TypeLowering::LowerNumberCheck(GateRef gate)
{
    auto value = acc_.GetValueIn(gate, 0);
    auto typeCheck = builder_.TaggedIsNumber(value);
    acc_.UpdateAllUses(gate, typeCheck);
    acc_.DeleteGate(gate);
}

void TypeLowering::LowerTypedBinaryOp(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto opGate = acc_.GetValueIn(gate, 2);
    auto op = static_cast<TypedBinOp>(acc_.GetBitField(opGate));
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
        default:
            break;
    }
}

void TypeLowering::LowerTypedUnaryOp(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto bitfield = acc_.GetBitField(gate);
    auto temp = bitfield >>  CircuitBuilder::OPRAND_TYPE_BITS;
    auto op = static_cast<TypedUnOp>(bitfield ^ (temp << CircuitBuilder::OPRAND_TYPE_BITS));
    switch (op) {
        case TypedUnOp::TYPED_TONUMBER:
            break;
        case TypedUnOp::TYPED_NEG:
            LowerTypedNeg(gate);
            break;
        case TypedUnOp::TYPED_NOT:
            LowerTypedNot(gate);
            break;
        case TypedUnOp::TYPED_INC:
            LowerTypedInc(gate);
            break;
        case TypedUnOp::TYPED_DEC:
            LowerTypedDec(gate);
            break;
        default:
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
    return;
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
    return;
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
    return;
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
    return;
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
    return;
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
    return;
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
    return;
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
    return;
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
    return;
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
    return;
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
    return;
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
    return;
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
    return;
}

void TypeLowering::LowerTypedInc(GateRef gate)
{
    auto value = acc_.GetLeftType(gate);
    if (value.IsNumberType()) {
        LowerNumberInc(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedDec(GateRef gate)
{
    auto value = acc_.GetLeftType(gate);
    if (value.IsNumberType()) {
        LowerNumberDec(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedNeg(GateRef gate)
{
    auto value = acc_.GetLeftType(gate);
    if (value.IsNumberType()) {
        LowerNumberNeg(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedNot(GateRef gate)
{
    auto value = acc_.GetLeftType(gate);
    if (value.IsNumberType()) {
        LowerNumberNot(gate);
    } else {
        UNREACHABLE();
    }
    return;
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

void TypeLowering::LowerNumberInc(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetLeftType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = MonocularNumber<TypedUnOp::TYPED_INC>(value, valueType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberDec(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetLeftType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = MonocularNumber<TypedUnOp::TYPED_DEC>(value, valueType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberNeg(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetLeftType(gate);
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

void TypeLowering::LowerNumberNot(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateType valueType = acc_.GetLeftType(gate);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = MonocularNumber<TypedUnOp::TYPED_NOT>(value, valueType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

template<OpCode::Op Op>
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

template<OpCode::Op Op>
GateRef TypeLowering::ShiftNumber(GateRef left, GateRef right, GateType leftType, GateType rightType)
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
        GateRef shift = builder_.Int32And(*intRight, builder_.Int32(0x1f));
        switch (Op) {
            case OpCode::LSL: {
                res = builder_.Int32LSL(*intLeft, shift);
                break;
            }
            case OpCode::LSR: {
                res = builder_.Int32LSR(*intLeft, shift);
                break;
            }
            case OpCode::ASR: {
                res = builder_.Int32ASR(*intLeft, shift);
                break;
            }
            default:
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

template<OpCode::Op Op>
GateRef TypeLowering::FastAddOrSubOrMul(GateRef left, GateRef right)
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
    Label leftIsNumber(&builder_);
    Label rightIsNumber(&builder_);
    Label leftIsIntRightIsDouble(&builder_);
    Label rightIsInt(&builder_);
    Label rightIsDouble(&builder_);
    builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &exit);
    builder_.Bind(&leftIsNumber);
    {
        builder_.Branch(builder_.TaggedIsNumber(right), &rightIsNumber, &exit);
        builder_.Bind(&rightIsNumber);
        {
            Label leftIsInt(&builder_);
            Label leftIsDouble(&builder_);
            builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            builder_.Bind(&leftIsInt);
            {
                builder_.Branch(builder_.TaggedIsInt(right), &doIntOp, &leftIsIntRightIsDouble);
                builder_.Bind(&leftIsIntRightIsDouble);
                {
                    doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
                    doubleRight = builder_.GetDoubleOfTDouble(right);
                    builder_.Jump(&doFloatOp);
                }
            }
            builder_.Bind(&leftIsDouble);
            {
                builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                builder_.Bind(&rightIsInt);
                {
                    doubleLeft = builder_.GetDoubleOfTDouble(left);
                    doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
                    builder_.Jump(&doFloatOp);
                }
                builder_.Bind(&rightIsDouble);
                {
                    doubleLeft = builder_.GetDoubleOfTDouble(left);
                    doubleRight = builder_.GetDoubleOfTDouble(right);
                    builder_.Jump(&doFloatOp);
                }
            }
        }
    }
    builder_.Bind(&doIntOp);
    {
        Label overflow(&builder_);
        Label notOverflow(&builder_);
        // handle left is int and right is int
        GateRef res = BinaryOp<Op, MachineType::I64>(builder_.GetInt64OfTInt(left),
                                                     builder_.GetInt64OfTInt(right));
        GateRef max = builder_.Int64(INT32_MAX);
        GateRef min = builder_.Int64(INT32_MIN);
        Label greaterZero(&builder_);
        Label notGreaterZero(&builder_);
        builder_.Branch(builder_.Int32GreaterThan(builder_.GetInt32OfTInt(left), builder_.Int32(0)),
                        &greaterZero, &notGreaterZero);
        builder_.Bind(&greaterZero);
        {
            builder_.Branch(builder_.Int64GreaterThan(res, max), &overflow, &notOverflow);
        }
        builder_.Bind(&notGreaterZero);
        {
            Label lessZero(&builder_);
            builder_.Branch(builder_.Int32LessThan(builder_.GetInt32OfTInt(left), builder_.Int32(0)),
                            &lessZero, &notOverflow);
            builder_.Bind(&lessZero);
            builder_.Branch(builder_.Int64LessThan(res, min), &overflow, &notOverflow);
        }
        builder_.Bind(&overflow);
        {
            GateRef newDoubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
            GateRef newDoubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
            GateRef middleRet = BinaryOp<Op, MachineType::F64>(newDoubleLeft, newDoubleRight);
            result = DoubleToTaggedDoublePtr(middleRet);
            builder_.Jump(&exit);
        }
        builder_.Bind(&notOverflow);
        {
            result = builder_.ToTaggedIntPtr(res);
            builder_.Jump(&exit);
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

template<OpCode::Op Op, MachineType Type>
GateRef TypeLowering::BinaryOp(GateRef x, GateRef y)
{
    return builder_.BinaryArithmetic(OpCode(Op), Type, x, y);
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
    return BinaryOp<OpCode::SMOD, MachineType::I32>(left, right);
}

GateRef TypeLowering::DoubleMod(GateRef left, GateRef right)
{
    return BinaryOp<OpCode::FMOD, MachineType::F64>(left, right);
}

GateRef TypeLowering::IntToTaggedIntPtr(GateRef x)
{
    GateRef val = builder_.SExtInt32ToInt64(x);
    return builder_.ToTaggedIntPtr(val);
}

GateRef TypeLowering::DoubleIsINF(GateRef x)
{
    GateRef infinity = builder_.Double(base::POSITIVE_INFINITY);
    GateRef negativeInfinity = builder_.Double(-base::POSITIVE_INFINITY);
    GateRef isInfinity = builder_.Equal(x, infinity);
    GateRef isNegativeInfinity = builder_.Equal(x, negativeInfinity);
    return builder_.BoolOr(builder_.Equal(builder_.SExtInt1ToInt32(isInfinity), builder_.Int32(1)),
                           builder_.Equal(builder_.SExtInt1ToInt32(isNegativeInfinity), builder_.Int32(1)));
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
            result = DoubleToTaggedDoublePtr(BinaryOp<OpCode::FDIV, MachineType::F64>(*doubleLeft, *doubleRight));
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
                builder_.Branch(DoubleIsINF(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf,
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
                builder_.Branch(DoubleIsINF(*doubleRight), &leftIsZeroOrRightIsInf, &rightNotInf);
                builder_.Bind(&rightNotInf);
                {
                    ArgumentAccessor argAcc(circuit_);
                    auto glue = argAcc.GetCommonArgGate(CommonArgIdx::GLUE);
                    result = builder_.CallNGCRuntime(
                        glue, RTSTUB_ID(AotFloatMod), Gate::InvalidGateRef, { *doubleLeft, *doubleRight });
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

GateRef TypeLowering::DivNumbers(GateRef left, GateRef right, GateType leftType, GateType rightType)
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
            result = DoubleToTaggedDoublePtr(BinaryOp<OpCode::FDIV, MachineType::F64>(*doubleLeft, *doubleRight));
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
}  // namespace panda::ecmascript