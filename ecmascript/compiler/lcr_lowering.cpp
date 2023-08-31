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
#include "ecmascript/compiler/lcr_lowering.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::kungfu {

GateRef LCRLowering::VisitGate(GateRef gate)
{
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
        case OpCode::GET_GLOBAL_ENV:
            LowerGetGlobalEnv(gate);
            break;
        case OpCode::GET_GLOBAL_ENV_OBJ:
            LowerGetGlobalEnvObj(gate);
            break;
        case OpCode::GET_GLOBAL_ENV_OBJ_HCLASS:
            LowerGetGlobalEnvObjHClass(gate);
            break;
        case OpCode::GET_GLOBAL_CONSTANT_VALUE:
            LowerGetGlobalConstantValue(gate);
            break;
        case OpCode::HEAP_ALLOC:
            LowerHeapAllocate(gate);
            break;
        case OpCode::INT32_CHECK_RIGHT_IS_ZERO:
            LowerInt32CheckRightIsZero(gate);
            break;
        case OpCode::FLOAT64_CHECK_RIGHT_IS_ZERO:
            LowerFloat64CheckRightIsZero(gate);
            break;
        case OpCode::VALUE_CHECK_NEG_OVERFLOW:
            LowerValueCheckNegOverflow(gate);
            break;
        case OpCode::OVERFLOW_CHECK:
            LowerOverflowCheck(gate);
            break;
        case OpCode::INT32_UNSIGNED_UPPER_BOUND_CHECK:
            LowerInt32UnsignedUpperBoundCheck(gate);
            break;
        case OpCode::INT32_DIV_WITH_CHECK:
            LowerInt32DivWithCheck(gate);
            break;
        case OpCode::LEX_VAR_IS_HOLE_CHECK:
            LowerLexVarIsHoleCheck(gate);
            break;
        case OpCode::STORE_MEMORY:
            LowerStoreMemory(gate);
            break;
        case OpCode::CHECK_AND_CONVERT:
            LowerCheckAndConvert(gate);
            break;
        default:
            break;
    }
    return Circuit::NullGate();
}

void LCRLowering::LowerConvertHoleAsUndefined(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    Label returnUndefined(&builder_);
    Label exit(&builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), receiver);

    builder_.Branch(builder_.TaggedIsHole(*result), &returnUndefined, &exit, 1, BranchWeight::DEOPT_WEIGHT);
    builder_.Bind(&returnUndefined);
    {
        result = builder_.UndefineConstant();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void LCRLowering::LowerLoadConstOffset(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef offset = builder_.IntPtr(acc_.GetOffset(gate));
    VariableType type = VariableType(acc_.GetMachineType(gate), acc_.GetGateType(gate));
    GateRef result = builder_.Load(type, receiver, offset);
    acc_.ReplaceGate(gate, Circuit::NullGate(), builder_.GetDepend(), result);
}

void LCRLowering::LowerStoreConstOffset(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef offset = builder_.IntPtr(acc_.GetOffset(gate));
    VariableType type = VariableType(acc_.GetMachineType(gate), acc_.GetGateType(gate));
    builder_.Store(type, glue_, receiver, offset, value);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void LCRLowering::LowerHeapObjectCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);

    GateRef heapObjectCheck = builder_.TaggedIsHeapObject(receiver);
    builder_.DeoptCheck(heapObjectCheck, frameState, DeoptType::NOTHEAPOBJECT);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void LCRLowering::LowerGetConstPool(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef jsFunc = acc_.GetValueIn(gate, 0); // 0: this object
    GateRef newGate = builder_.GetConstPoolFromFunction(jsFunc);

    acc_.UpdateAllUses(gate, newGate);

    // delete old gate
    acc_.DeleteGate(gate);
}

void LCRLowering::DeleteStateSplit(GateRef gate)
{
    auto depend = acc_.GetDep(gate);
    auto frameState = acc_.GetFrameState(gate);
    acc_.DeleteGateIfNoUse(frameState);
    acc_.ReplaceGate(gate, Circuit::NullGate(), depend, Circuit::NullGate());
}

void LCRLowering::LowerArrayGuardianCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef frameState = acc_.GetFrameState(gate);
    GateRef guardiansOffset = builder_.IntPtr(JSThread::GlueData::GetStableArrayElementsGuardiansOffset(false));
    GateRef check = builder_.Load(VariableType::BOOL(), glue_, guardiansOffset);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTSARRAY);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void LCRLowering::LowerHClassStableArrayCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef hclass = acc_.GetValueIn(gate, 0);

    GateRef check = Circuit::NullGate();
    GateRef stableCheck = builder_.IsStableElements(hclass);
    ArrayMetaDataAccessor accessor = acc_.GetArrayMetaDataAccessor(gate);
    ElementsKind kind = accessor.GetElementsKind();
    if (accessor.IsLoadElement() && !Elements::IsHole(kind)) {
        GateRef elementsKindCheck = builder_.Equal(builder_.Int32(static_cast<int32_t>(kind)),
                                                   builder_.GetElementsKindByHClass(hclass));
        check = builder_.BoolAnd(stableCheck, elementsKindCheck);
    } else {
        check = stableCheck;
    }
    builder_.DeoptCheck(check, frameState, DeoptType::NOTSARRAY);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

StateDepend LCRLowering::LowerConvert(StateDepend stateDepend, GateRef gate)
{
    Environment env(stateDepend.State(), stateDepend.Depend(), {}, circuit_, &builder_);
    GateRef value = acc_.GetValueIn(gate);
    ValueType dstType = acc_.GetDstType(gate);
    GateRef result = Circuit::NullGate();
    Label exit(&builder_);
    switch (acc_.GetSrcType(gate)) {
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
        case ValueType::UINT32:
            if (dstType == ValueType::TAGGED_NUMBER) {
                result = ConvertUInt32ToTaggedNumber(value, &exit);
            } else if (dstType == ValueType::FLOAT64) {
                result = ConvertUInt32ToFloat64(value);
            } else {
                ASSERT(dstType == ValueType::BOOL);
                result = builder_.NotEqual(value, builder_.Int32(0));
            }
            break;
        case ValueType::FLOAT64:
            if (dstType == ValueType::TAGGED_DOUBLE) {
                result = ConvertFloat64ToTaggedDouble(value);
            } else if (dstType == ValueType::INT32) {
                result = ConvertFloat64ToInt32(value, &exit);
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
            break;
    }
    acc_.ReplaceGate(gate, Circuit::NullGate(), Circuit::NullGate(), result);
    return builder_.GetStateDepend();
}

GateRef LCRLowering::ConvertTaggedNumberToBool(GateRef gate, Label *exit)
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

GateRef LCRLowering::ConvertTaggedNumberToInt32(GateRef gate, Label *exit)
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

GateRef LCRLowering::ConvertTaggedNumberToFloat64(GateRef gate, Label *exit)
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

void LCRLowering::LowerCheckAndConvert(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    ValueType srcType = acc_.GetSrcType(gate);
    Label exit(&builder_);
    switch (srcType) {
        case ValueType::UINT32:
            LowerCheckUInt32AndConvert(gate, frameState);
            break;
        case ValueType::TAGGED_INT:
            LowerCheckTaggedIntAndConvert(gate, frameState);
            break;
        case ValueType::TAGGED_DOUBLE:
            LowerCheckTaggedDoubleAndConvert(gate, frameState, &exit);
            break;
        case ValueType::TAGGED_BOOLEAN:
            LowerCheckTaggedBoolAndConvert(gate, frameState);
            break;
        case ValueType::TAGGED_NUMBER:
            LowerCheckTaggedNumberAndConvert(gate, frameState, &exit);
            break;
        case ValueType::BOOL:
            LowerCheckSupportAndConvert(gate, frameState);
            break;
        default:
            UNREACHABLE();
    }
}

void LCRLowering::LowerCheckUInt32AndConvert(GateRef gate, GateRef frameState)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef upperBound = builder_.Int32(INT32_MAX);
    GateRef check = builder_.Int32UnsignedLessThanOrEqual(value, upperBound);
    builder_.DeoptCheck(check, frameState, DeoptType::INT32OVERFLOW);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), value);
}

void LCRLowering::LowerCheckTaggedIntAndConvert(GateRef gate, GateRef frameState)
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

void LCRLowering::LowerCheckTaggedDoubleAndConvert(GateRef gate, GateRef frameState, Label *exit)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsDouble(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTDOUBLE);
    GateRef result = Circuit::NullGate();
    ValueType dst = acc_.GetDstType(gate);
    ASSERT(dst == ValueType::INT32 || dst == ValueType::FLOAT64);
    if (dst == ValueType::INT32) {
        result = ConvertTaggedDoubleToInt32(value, exit);
    } else {
        result = ConvertTaggedDoubleToFloat64(value);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void LCRLowering::LowerCheckTaggedNumberAndConvert(GateRef gate, GateRef frameState, Label *exit)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsNumber(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTNUMBER);
    GateRef result = Circuit::NullGate();
    ValueType dst = acc_.GetDstType(gate);
    if (dst == ValueType::INT32) {
        result = ConvertTaggedNumberToInt32(value, exit);
    } else if (dst == ValueType::FLOAT64) {
        result = ConvertTaggedNumberToFloat64(value, exit);
    } else {
        ASSERT(dst == ValueType::BOOL);
        result = ConvertTaggedNumberToBool(value, exit);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void LCRLowering::LowerCheckSupportAndConvert(GateRef gate, GateRef frameState)
{
    ValueType dstType = acc_.GetDstType(gate);
    ASSERT(dstType == ValueType::INT32 || dstType == ValueType::FLOAT64);
    bool support = acc_.IsConvertSupport(gate);
    GateRef value = acc_.GetValueIn(gate, 0);

    GateRef result = Circuit::NullGate();
    if (dstType == ValueType::INT32) {
        builder_.DeoptCheck(builder_.Boolean(support), frameState, DeoptType::NOTINT);
        result = builder_.BooleanToInt32(value);
    } else {
        builder_.DeoptCheck(builder_.Boolean(support), frameState, DeoptType::NOTDOUBLE);
        result = builder_.BooleanToFloat64(value);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void LCRLowering::LowerCheckTaggedBoolAndConvert(GateRef gate, GateRef frameState)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsBoolean(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTBOOL);
    GateRef result = Circuit::NullGate();
    ASSERT(acc_.GetDstType(gate) == ValueType::BOOL);
    result = ConvertTaggedBooleanToBool(value);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

GateRef LCRLowering::ConvertTaggedBooleanToBool(GateRef value)
{
    return builder_.TaggedIsTrue(value);
}

GateRef LCRLowering::ConvertBoolToTaggedBoolean(GateRef gate)
{
    return builder_.BooleanToTaggedBooleanPtr(gate);
}

GateRef LCRLowering::ConvertInt32ToFloat64(GateRef gate)
{
    return builder_.ChangeInt32ToFloat64(gate);
}

GateRef LCRLowering::ConvertUInt32ToFloat64(GateRef gate)
{
    return builder_.ChangeUInt32ToFloat64(gate);
}

GateRef LCRLowering::ConvertInt32ToTaggedInt(GateRef gate)
{
    return builder_.Int32ToTaggedPtr(gate);
}

GateRef LCRLowering::ConvertUInt32ToTaggedNumber(GateRef gate, Label *exit)
{
    Label isOverFlow(&builder_);
    Label notOverFlow(&builder_);
    GateRef upperBound = builder_.Int32(INT32_MAX);
    DEFVAlUE(taggedVal, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    builder_.Branch(builder_.Int32UnsignedLessThanOrEqual(gate, upperBound), &notOverFlow, &isOverFlow);
    builder_.Bind(&notOverFlow);
    taggedVal = builder_.Int32ToTaggedPtr(gate);
    builder_.Jump(exit);
    builder_.Bind(&isOverFlow);
    taggedVal = builder_.DoubleToTaggedDoublePtr(builder_.ChangeUInt32ToFloat64(gate));
    builder_.Jump(exit);
    builder_.Bind(exit);
    return *taggedVal;
}

GateRef LCRLowering::ConvertFloat64ToInt32(GateRef gate, Label *exit)
{
    return builder_.DoubleToInt(gate, exit);
}

GateRef LCRLowering::ConvertFloat64ToBool(GateRef gate)
{
    GateRef doubleNotZero = builder_.DoubleNotEqual(gate, builder_.Double(0.0));
    GateRef doubleNotNAN = builder_.BoolNot(builder_.DoubleIsNAN(gate));
    return builder_.BoolAnd(doubleNotZero, doubleNotNAN);
}

GateRef LCRLowering::ConvertFloat64ToTaggedDouble(GateRef gate)
{
    return builder_.DoubleToTaggedDoublePtr(gate);
}

GateRef LCRLowering::ConvertTaggedIntToInt32(GateRef gate)
{
    return builder_.GetInt32OfTInt(gate);
}

GateRef LCRLowering::ConvertTaggedIntToFloat64(GateRef gate)
{
    return builder_.ChangeInt32ToFloat64(builder_.GetInt32OfTInt(gate));
}

GateRef LCRLowering::ConvertTaggedDoubleToInt32(GateRef gate, Label *exit)
{
    return builder_.DoubleToInt(builder_.GetDoubleOfTDouble(gate), exit);
}

GateRef LCRLowering::ConvertTaggedDoubleToFloat64(GateRef gate)
{
    return builder_.GetDoubleOfTDouble(gate);
}

void LCRLowering::LowerGetGlobalEnv(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glueGlobalEnvOffset = builder_.IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(false));
    GateRef glueGlobalEnv = builder_.Load(VariableType::JS_POINTER(), glue_, glueGlobalEnvOffset);
    acc_.ReplaceGate(gate, Circuit::NullGate(), builder_.GetDepend(), glueGlobalEnv);
}

void LCRLowering::LowerGetGlobalEnvObj(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef globalEnv = acc_.GetValueIn(gate, 0);
    size_t index = acc_.GetIndex(gate);
    GateRef offset = builder_.IntPtr(GlobalEnv::HEADER_SIZE + JSTaggedValue::TaggedTypeSize() * index);
    GateRef object = builder_.Load(VariableType::JS_ANY(), globalEnv, offset);
    acc_.ReplaceGate(gate, Circuit::NullGate(), builder_.GetDepend(), object);
}

void LCRLowering::LowerGetGlobalEnvObjHClass(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef globalEnv = acc_.GetValueIn(gate, 0);
    size_t index = acc_.GetIndex(gate);
    GateRef offset = builder_.IntPtr(GlobalEnv::HEADER_SIZE + JSTaggedValue::TaggedTypeSize() * index);
    GateRef object = builder_.Load(VariableType::JS_ANY(), globalEnv, offset);
    auto hclass = builder_.Load(VariableType::JS_POINTER(), object,
                                builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    acc_.ReplaceGate(gate, Circuit::NullGate(), builder_.GetDepend(), hclass);
}

void LCRLowering::LowerGetGlobalConstantValue(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    size_t index = acc_.GetIndex(gate);
    GateRef gConstAddr = builder_.Load(VariableType::JS_POINTER(), glue_,
        builder_.IntPtr(JSThread::GlueData::GetGlobalConstOffset(false)));
    GateRef constantIndex = builder_.IntPtr(JSTaggedValue::TaggedTypeSize() * index);
    GateRef result = builder_.Load(VariableType::JS_POINTER(), gConstAddr, constantIndex);
    acc_.ReplaceGate(gate, Circuit::NullGate(), builder_.GetDepend(), result);
}

void LCRLowering::LowerHeapAllocate(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto flag = acc_.TryGetValue(gate);
    switch (flag) {
        case RegionSpaceFlag::IN_YOUNG_SPACE:
            HeapAllocateInYoung(gate);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void LCRLowering::HeapAllocateInYoung(GateRef gate)
{
    Label success(&builder_);
    Label callRuntime(&builder_);
    Label exit(&builder_);
    GateRef size = acc_.GetValueIn(gate, 0);
    size_t topOffset = JSThread::GlueData::GetNewSpaceAllocationTopAddressOffset(false);
    size_t endOffset = JSThread::GlueData::GetNewSpaceAllocationEndAddressOffset(false);
    GateRef topAddress = builder_.Load(VariableType::NATIVE_POINTER(), glue_, builder_.IntPtr(topOffset));
    GateRef endAddress = builder_.Load(VariableType::NATIVE_POINTER(), glue_, builder_.IntPtr(endOffset));
    GateRef top = builder_.Load(VariableType::JS_POINTER(), topAddress, builder_.IntPtr(0));
    GateRef end = builder_.Load(VariableType::JS_POINTER(), endAddress, builder_.IntPtr(0));

    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    GateRef newTop = builder_.PtrAdd(top, size);
    builder_.Branch(builder_.IntPtrGreaterThan(newTop, end), &callRuntime, &success);
    builder_.Bind(&success);
    {
        builder_.Store(VariableType::NATIVE_POINTER(), glue_, topAddress, builder_.IntPtr(0), newTop);
        result = top;
        builder_.Jump(&exit);
    }
    builder_.Bind(&callRuntime);
    {
        result = builder_.CallRuntime(glue_, RTSTUB_ID(AllocateInYoung), Gate::InvalidGateRef,
                                      {builder_.ToTaggedInt(size)}, gate);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void LCRLowering::LowerInt32CheckRightIsZero(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef right = acc_.GetValueIn(gate, 0);
    GateRef rightNotZero = builder_.Int32NotEqual(right, builder_.Int32(0));
    builder_.DeoptCheck(rightNotZero, frameState, DeoptType::MODZERO);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void LCRLowering::LowerFloat64CheckRightIsZero(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef right = acc_.GetValueIn(gate, 0);
    GateRef rightNotZero = builder_.DoubleNotEqual(right, builder_.Double(0.0));
    builder_.DeoptCheck(rightNotZero, frameState, DeoptType::DIVZERO);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void LCRLowering::LowerLexVarIsHoleCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef valueIsNotHole = builder_.TaggedIsNotHole(value);
    builder_.DeoptCheck(valueIsNotHole, frameState, DeoptType::LEXVARISHOLE);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void LCRLowering::LowerValueCheckNegOverflow(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef valueNotZero = builder_.NotEqual(value, builder_.Int32(0));
    builder_.DeoptCheck(valueNotZero, frameState, DeoptType::NOTNEGOV);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void LCRLowering::LowerOverflowCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef result = acc_.GetValueIn(gate, 0);
    GateRef condition = builder_.BoolNot(builder_.ExtractValue(MachineType::I1, result, builder_.Int32(1)));
    builder_.DeoptCheck(condition, frameState, DeoptType::NOTINT);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void LCRLowering::LowerInt32UnsignedUpperBoundCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef upperBound = acc_.GetValueIn(gate, 1);
    GateRef condition = builder_.Int32UnsignedLessThanOrEqual(value, upperBound);
    builder_.DeoptCheck(condition, frameState, DeoptType::NOTINT);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void LCRLowering::LowerInt32DivWithCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateRef result = Circuit::NullGate();
    GateRef rightGreaterZero = builder_.Int32GreaterThan(right, builder_.Int32(0));
    GateRef rightLessZero = builder_.Int32LessThan(right, builder_.Int32(0));
    GateRef leftNotZero = builder_.Int32NotEqual(left, builder_.Int32(0));
    GateRef condition = builder_.BoolOr(rightGreaterZero, builder_.BoolAnd(rightLessZero, leftNotZero));
    builder_.DeoptCheck(condition, frameState, DeoptType::DIVZERO);
    result = builder_.BinaryArithmetic(circuit_->Sdiv(), MachineType::I32, left, right, GateType::NJSValue());
    GateRef truncated = builder_.BinaryArithmetic(circuit_->Mul(),
        MachineType::I32, result, right, GateType::NJSValue());
    GateRef overCheck = builder_.Int32Equal(truncated, left);
    builder_.DeoptCheck(overCheck, frameState, DeoptType::NOTINT);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void LCRLowering::LowerStoreMemory(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);
    builder_.Store(VariableType::VOID(), glue_, receiver, index, value);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void LCRLowering::InitializeWithSpeicalValue(Label *exit, GateRef object, GateRef glue,
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
}  // namespace panda::ecmascript
