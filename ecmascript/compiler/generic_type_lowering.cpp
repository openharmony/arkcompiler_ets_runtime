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
#include "ecmascript/compiler/generic_type_lowering.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::kungfu {
void GenericTypeLowering::Run()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        switch (op) {
            case OpCode::GET_CONSTPOOL:
                LowerGetConstPool(gate);
                break;
            case OpCode::STATE_SPLIT:
                DeleteStateSplit(gate);
                break;
            case OpCode::ARRAY_GUARDIAN_CHECK:
                LowerArrayGuardianCheck(gate);
                break;
            case OpCode::HCLASS_STABLE_ARRAY_CHECK:
                LowerHClassStableArrayCheck(gate);
                break;
            case OpCode::HEAP_OBJECT_CHECK:
                LowerHeapObjectCheck(gate);
                break;
            case OpCode::LOAD_CONST_OFFSET:
                LowerLoadConstOffset(gate);
                break;
            case OpCode::STORE_CONST_OFFSET:
                LowerStoreConstOffset(gate);
                break;
            case OpCode::CONVERT_HOLE_AS_UNDEFINED:
                LowerConvertHoleAsUndefined(gate);
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

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << " ";
        LOG_COMPILER(INFO) << "\033[34m" << "================="
                           << " After generic type Lowering "
                           << "[" << GetMethodName() << "] "
                           << "=================" << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "=========================== End ===========================" << "\033[0m";
    }
}

void GenericTypeLowering::LowerConvertHoleAsUndefined(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    Label returnUndefined(&builder_);
    Label exit(&builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), receiver);

    builder_.Branch(builder_.TaggedIsHole(*result), &returnUndefined, &exit);
    builder_.Bind(&returnUndefined);
    {
        result = builder_.UndefineConstant();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void GenericTypeLowering::LowerLoadConstOffset(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef offset = builder_.IntPtr(acc_.GetOffset(gate));
    VariableType type = VariableType(acc_.GetMachineType(gate), acc_.GetGateType(gate));
    GateRef result = builder_.Load(type, receiver, offset);
    acc_.ReplaceGate(gate, Circuit::NullGate(), builder_.GetDepend(), result);
}

void GenericTypeLowering::LowerStoreConstOffset(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef offset = builder_.IntPtr(acc_.GetOffset(gate));
    VariableType type = VariableType(acc_.GetMachineType(gate), acc_.GetGateType(gate));
    builder_.Store(type, glue_, receiver, offset, value);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void GenericTypeLowering::LowerHeapObjectCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);

    GateRef heapObjectCheck = builder_.TaggedIsHeapObject(receiver);
    builder_.DeoptCheck(heapObjectCheck, frameState, DeoptType::NOTHEAPOBJECT);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void GenericTypeLowering::LowerGetConstPool(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef jsFunc = acc_.GetValueIn(gate, 0); // 0: this object
    GateRef newGate = builder_.GetConstPoolFromFunction(jsFunc);

    acc_.UpdateAllUses(gate, newGate);

    // delete old gate
    acc_.DeleteGate(gate);
}

void GenericTypeLowering::DeleteStateSplit(GateRef gate)
{
    auto depend = acc_.GetDep(gate);
    acc_.ReplaceGate(gate, Circuit::NullGate(), depend, Circuit::NullGate());
}

void GenericTypeLowering::LowerArrayGuardianCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef frameState = acc_.GetFrameState(gate);
    GateRef guardiansOffset = builder_.IntPtr(JSThread::GlueData::GetStableArrayElementsGuardiansOffset(false));
    GateRef check = builder_.Load(VariableType::BOOL(), glue_, guardiansOffset);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTSARRAY);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void GenericTypeLowering::LowerHClassStableArrayCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef hclass = acc_.GetValueIn(gate, 0);

    GateRef check = builder_.IsIsStableElementsByHClass(hclass);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTSARRAY);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void GenericTypeLowering::LowerConvert(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate);
    ValueType srcType = acc_.GetSrcType(gate);
    ValueType dstType = acc_.GetDstType(gate);
    GateRef result = Circuit::NullGate();

    switch (srcType) {
        case ValueType::BOOL:
            ASSERT(dstType == ValueType::TAGGED_BOOLEAN);
            result = ConvertBoolToTaggedBoolean(value);
            break;
        case ValueType::INT32:
            if (dstType == ValueType::TAGGED_INT) {
                result = ConvertInt32ToTaggedInt(value);
            } else if (dstType == ValueType::FLOAT64) {
                result = ConvertInt32ToFloat64(value);
            } else {
                ASSERT(dstType == ValueType::BOOL);
                result = builder_.NotEqual(value, builder_.Int32(0));
            }
            break;
        case ValueType::FLOAT64:
            if (dstType == ValueType::TAGGED_DOUBLE) {
                result = ConvertFloat64ToTaggedDouble(value);
            } else {
                ASSERT(dstType == ValueType::BOOL);
                result = ConvertFloat64ToBool(value);
            }
            break;
        case ValueType::TAGGED_BOOLEAN:
            ASSERT((dstType == ValueType::BOOL));
            result = ConvertTaggedBooleanToBool(value);
            break;
        case ValueType::TAGGED_INT:
            ASSERT((dstType == ValueType::INT32));
            result = ConvertTaggedIntToInt32(value);
            break;
        case ValueType::TAGGED_DOUBLE:
            ASSERT((dstType == ValueType::FLOAT64));
            result = ConvertTaggedDoubleToFloat64(value);
            break;
        default:
            LOG_COMPILER(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    acc_.ReplaceGate(gate, Circuit::NullGate(), Circuit::NullGate(), result);
}

GateRef GenericTypeLowering::ConvertTaggedNumberToBool(GateRef gate, Label *exit)
{
    DEFVAlUE(result, (&builder_), VariableType::BOOL(), builder_.Boolean(false));
    Label isInt(&builder_);
    Label isDouble(&builder_);
    Label toInt32(&builder_);
    builder_.Branch(builder_.TaggedIsInt(gate), &isInt, &isDouble);
    builder_.Bind(&isInt);
    {
        GateRef intVal = builder_.GetInt64OfTInt(gate);
        result = builder_.NotEqual(intVal, builder_.Int64(0));
    }
    builder_.Jump(exit);
    builder_.Bind(&isDouble);
    {
        GateRef doubleVal = builder_.GetDoubleOfTDouble(gate);
        result = ConvertFloat64ToBool(doubleVal);
    }
    builder_.Jump(exit);
    builder_.Bind(exit);
    return *result;
}

GateRef GenericTypeLowering::ConvertTaggedNumberToInt32(GateRef gate, Label *exit)
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

GateRef GenericTypeLowering::ConvertTaggedNumberToFloat64(GateRef gate, Label *exit)
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

void GenericTypeLowering::LowerCheckAndConvert(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto frameState = acc_.GetFrameState(gate);
    ValueType srcType = acc_.GetSrcType(gate);
    switch (srcType) {
        case ValueType::FLOAT64:
            LowerCheckFloat64AndConvert(gate);
            break;
        case ValueType::TAGGED_INT:
            LowerCheckTaggedIntAndConvert(gate, frameState);
            break;
        case ValueType::TAGGED_DOUBLE:
            LowerCheckTaggedDoubleAndConvert(gate, frameState);
            break;
        case ValueType::TAGGED_BOOLEAN:
            LowerCheckTaggedBoolAndConvert(gate, frameState);
            break;
        case ValueType::TAGGED_NUMBER:
            LowerCheckTaggedNumberAndConvert(gate, frameState);
            break;
        default:
            UNREACHABLE();
    }
}

void GenericTypeLowering::LowerCheckFloat64AndConvert(GateRef gate)
{
    ASSERT(acc_.GetDstType(gate) == ValueType::INT32);
    Label exit(&builder_);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef result = ConvertFloat64ToInt32(value, &exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void GenericTypeLowering::LowerCheckTaggedIntAndConvert(GateRef gate, GateRef frameState)
{
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

void GenericTypeLowering::LowerCheckTaggedDoubleAndConvert(GateRef gate, GateRef frameState)
{
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

void GenericTypeLowering::LowerCheckTaggedNumberAndConvert(GateRef gate, GateRef frameState)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsNumber(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTNUMBER);
    GateRef result = Circuit::NullGate();
    ValueType dst = acc_.GetDstType(gate);
    Label exit(&builder_);
    if (dst == ValueType::INT32) {
        result = ConvertTaggedNumberToInt32(value, &exit);
    } else if (dst == ValueType::FLOAT64) {
        result = ConvertTaggedNumberToFloat64(value, &exit);
    } else {
        ASSERT(dst == ValueType::BOOL);
        result = ConvertTaggedNumberToBool(value, &exit);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void GenericTypeLowering::LowerCheckTaggedBoolAndConvert(GateRef gate, GateRef frameState)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsBoolean(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTBOOL);
    GateRef result = Circuit::NullGate();
    ASSERT(acc_.GetDstType(gate) == ValueType::BOOL);
    result = ConvertTaggedBooleanToBool(value);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

GateRef GenericTypeLowering::ConvertTaggedBooleanToBool(GateRef value)
{
    return builder_.TaggedIsTrue(value);
}

GateRef GenericTypeLowering::ConvertBoolToTaggedBoolean(GateRef gate)
{
    return builder_.BooleanToTaggedBooleanPtr(gate);
}

GateRef GenericTypeLowering::ConvertInt32ToFloat64(GateRef gate)
{
    return builder_.ChangeInt32ToFloat64(gate);
}

GateRef GenericTypeLowering::ConvertInt32ToTaggedInt(GateRef gate)
{
    return builder_.Int32ToTaggedPtr(gate);
}

GateRef GenericTypeLowering::ConvertFloat64ToInt32(GateRef gate, Label *exit)
{
    return builder_.DoubleToInt(gate, exit);
}

GateRef GenericTypeLowering::ConvertFloat64ToBool(GateRef gate)
{
    GateRef doubleNotZero = builder_.DoubleNotEqual(gate, builder_.Double(0.0));
    GateRef doubleNotNAN = builder_.BoolNot(builder_.DoubleIsNAN(gate));
    return builder_.BoolAnd(doubleNotZero, doubleNotNAN);
}

GateRef GenericTypeLowering::ConvertFloat64ToTaggedDouble(GateRef gate)
{
    return builder_.DoubleToTaggedDoublePtr(gate);
}

GateRef GenericTypeLowering::ConvertTaggedIntToInt32(GateRef gate)
{
    return builder_.GetInt32OfTInt(gate);
}

GateRef GenericTypeLowering::ConvertTaggedIntToFloat64(GateRef gate)
{
    return builder_.ChangeInt32ToFloat64(builder_.GetInt32OfTInt(gate));
}

GateRef GenericTypeLowering::ConvertTaggedDoubleToInt32(GateRef gate, Label *exit)
{
    return builder_.DoubleToInt(builder_.GetDoubleOfTDouble(gate), exit);
}

GateRef GenericTypeLowering::ConvertTaggedDoubleToFloat64(GateRef gate)
{
    return builder_.GetDoubleOfTDouble(gate);
}
}  // namespace panda::ecmascript
