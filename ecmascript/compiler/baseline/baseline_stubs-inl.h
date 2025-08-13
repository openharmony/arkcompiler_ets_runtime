/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUBS_INL_H
#define ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUBS_INL_H

#include "ecmascript/compiler/baseline/baseline_stubs.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_async_generator_object.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;

void BaselineStubBuilder::SetEnvToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame,
          IntPtr(AsmInterpretedFrame::GetEnvOffset(GetEnvironment()->IsArch32Bit())), value);
}

void BaselineStubBuilder::CheckExceptionWithVar(GateRef glue, GateRef sp, GateRef res, GateRef acc)
{
    auto env = GetEnvironment();
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, sp, acc);
        Return(acc);
    }
    Bind(&notException);
    {
        Return(res);
    }
}

void BaselineStubBuilder::CheckException(GateRef glue, GateRef sp, GateRef res)
{
    auto env = GetEnvironment();
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        GateRef frame = GetFrame(sp);
        GateRef acc = GetAccFromFrame(glue, frame);
        DispatchLast(glue, sp, acc);
        Return();
    }
    Bind(&notException);
    {
        Return();
    }
}

void BaselineStubBuilder::CheckExceptionReturn(GateRef glue, GateRef sp, GateRef res)
{
    auto env = GetEnvironment();
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        GateRef frame = GetFrame(sp);
        GateRef acc = GetAccFromFrame(glue, frame);
        DispatchLast(glue, sp, acc);
        Return(acc);
    }
    Bind(&notException);
    {
        Return(res);
    }
}

void BaselineStubBuilder::CheckExceptionWithJump(GateRef glue, GateRef sp, GateRef res, GateRef acc, Label *jump)
{
    auto env = GetEnvironment();
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, sp, acc);
        Return();
    }
    Bind(&notException);
    {
        Jump(jump);
    }
}

void BaselineStubBuilder::CheckExceptionWithJumpAndReturn(GateRef glue, GateRef sp, GateRef res, GateRef acc,
                                                          Label *jump)
{
    auto env = GetEnvironment();
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, sp, acc);
        Return(acc);
    }
    Bind(&notException);
    {
        Jump(jump);
    }
}

void BaselineStubBuilder::CheckPendingException(GateRef glue, GateRef sp, GateRef res, GateRef acc)
{
    auto env = GetEnvironment();
    Label isException(env);
    Label notException(env);
    Branch(HasPendingException(glue), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, sp, acc);
        Return(acc);
    }
    Bind(&notException);
    {
        Return(res);
    }
}

void BaselineStubBuilder::DispatchLast(GateRef glue, GateRef sp, GateRef acc)
{
    // Get baseline exceptionHandler index
    GateRef baselineEHIndex = BaselineStubCSigns::BaselineExceptionHandler;
    CallBaselineStub(glue, baselineEHIndex, { glue, sp, acc });
}

GateRef BaselineStubBuilder::CallBaselineStub(GateRef glue, int index, const std::initializer_list<GateRef>& args)
{
    auto env = GetEnvironment();
    const std::string name = BaselineStubCSigns::GetName(index);
    GateRef result = env->GetBuilder()->CallStub(glue, Circuit::NullGate(), index, args, name.c_str());
    return result;
}

GateRef BaselineStubBuilder::GetFunctionFromFrame(GateRef glue, GateRef frame)
{
    return Load(VariableType::JS_POINTER(), glue, frame,
                IntPtr(AsmInterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef BaselineStubBuilder::GetEnvFromFrame(GateRef glue, GateRef frame)
{
    return Load(VariableType::JS_POINTER(), glue, frame,
                IntPtr(AsmInterpretedFrame::GetEnvOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef BaselineStubBuilder::GetAccFromFrame(GateRef glue, GateRef frame)
{
    return Load(VariableType::JS_ANY(), glue, frame,
                IntPtr(AsmInterpretedFrame::GetAccOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef BaselineStubBuilder::GetConstpoolFromMethod(GateRef glue, GateRef method)
{
    return Load(VariableType::JS_POINTER(), glue, method, IntPtr(Method::CONSTANT_POOL_OFFSET));
}

GateRef BaselineStubBuilder::GetProfileTypeInfoFromFunction(GateRef glue, GateRef function)
{
    GateRef raw = Load(VariableType::JS_POINTER(), glue, function, IntPtr(JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
    return Load(VariableType::JS_POINTER(), glue, raw, IntPtr(ProfileTypeInfoCell::VALUE_OFFSET));
}

GateRef BaselineStubBuilder::GetHotnessCounterFromMethod(GateRef method)
{
    GateRef x = LoadPrimitive(VariableType::INT16(), method, IntPtr(Method::LITERAL_INFO_OFFSET));
    return GetEnvironment()->GetBuilder()->SExtInt1ToInt32(x);
}

GateRef BaselineStubBuilder::GetModuleFromFunction(GateRef glue, GateRef function)
{
    return Load(VariableType::JS_POINTER(), glue, function, IntPtr(JSFunction::ECMA_MODULE_OFFSET));
}

GateRef BaselineStubBuilder::GetHomeObjectFromFunction(GateRef glue, GateRef function)
{
    return Load(VariableType::JS_POINTER(), glue, function, IntPtr(JSFunction::HOME_OBJECT_OFFSET));
}

GateRef BaselineStubBuilder::GetModule(GateRef glue, GateRef sp)
{
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    return GetModuleFromFunction(glue, currentFunc);
}

GateRef BaselineStubBuilder::GetCurrentFrame(GateRef glue)
{
    return GetLastLeaveFrame(glue);
}

GateRef BaselineStubBuilder::GetFrame(GateRef CurrentSp)
{
    return PtrSub(CurrentSp, IntPtr(AsmInterpretedFrame::GetSize(GetEnvironment()->IsArch32Bit())));
}

GateRef BaselineStubBuilder::GetPcFromFrame(GateRef frame)
{
    return LoadPrimitive(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetPcOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef BaselineStubBuilder::GetCallSizeFromFrame(GateRef frame)
{
    return LoadPrimitive(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetCallSizeOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef BaselineStubBuilder::GetThisFromFrame(GateRef glue, GateRef frame)
{
    return Load(VariableType::JS_POINTER(), glue, frame,
                IntPtr(AsmInterpretedFrame::GetThisOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef BaselineStubBuilder::GetNewTarget(GateRef glue, GateRef sp)
{
    GateRef function = Load(VariableType::JS_POINTER(), glue, GetFrame(sp),
                            IntPtr(AsmInterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())));
    GateRef method = GetMethodFromFunction(glue, function);
    GateRef callField = GetCallFieldFromMethod(method);
    // ASSERT: callField has "extra" bit.
    GateRef numVregs =
            TruncInt64ToInt32(Int64And(Int64LSR(callField, Int64(MethodLiteral::NumVregsBits::START_BIT)),
                                       Int64((1LLU << MethodLiteral::NumVregsBits::SIZE) - 1)));
    GateRef haveFunc =
            ZExtInt1ToInt32(Int64NotEqual(Int64And(Int64LSR(callField, Int64(MethodLiteral::HaveFuncBit::START_BIT)),
                Int64((1LLU << MethodLiteral::HaveFuncBit::SIZE) - 1)), Int64(0)));
    GateRef idx = ZExtInt32ToPtr(Int32Add(numVregs, haveFunc));
    return Load(VariableType::JS_ANY(), glue, sp, PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), idx));
}

GateRef BaselineStubBuilder::GetStartIdxAndNumArgs(GateRef glue, GateRef sp, GateRef restIdx)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    DEFVARIABLE(numArgs, VariableType::INT32(), Int32(0));
    GateRef state = PtrSub(sp, IntPtr(AsmInterpretedFrame::GetSize(env->IsArch32Bit())));
    GateRef function = GetFunctionFromFrame(glue, state);
    GateRef method = GetMethodFromJSFunctionOrProxy(glue, function);
    GateRef callField = GetCallFieldFromMethod(method);
    // ASSERT: callField has "extra" bit.
    GateRef numVregs = TruncInt64ToInt32(Int64And(
        Int64LSR(callField, Int64(MethodLiteral::NumVregsBits::START_BIT)),
        Int64((1LLU << MethodLiteral::NumVregsBits::SIZE) - 1)));
    GateRef haveFunc = Int64NotEqual(Int64And(Int64LSR(callField, Int64(MethodLiteral::HaveFuncBit::START_BIT)),
        Int64((1LLU << MethodLiteral::HaveFuncBit::SIZE) - 1)), Int64(0));
    GateRef haveNewTarget = Int64NotEqual(
        Int64And(Int64LSR(callField, Int64(MethodLiteral::HaveNewTargetBit::START_BIT)),
        Int64((1LLU << MethodLiteral::HaveNewTargetBit::SIZE) - 1)), Int64(0));
    GateRef haveThis = Int64NotEqual(Int64And(Int64LSR(callField, Int64(MethodLiteral::HaveThisBit::START_BIT)),
        Int64((1LLU << MethodLiteral::HaveThisBit::SIZE) - 1)), Int64(0));
    GateRef copyArgs = Int32Add(Int32Add(ZExtInt1ToInt32(haveFunc), ZExtInt1ToInt32(haveNewTarget)),
                                ZExtInt1ToInt32(haveThis));
    numArgs = TruncInt64ToInt32(Int64And(Int64LSR(callField, Int64(MethodLiteral::NumArgsBits::START_BIT)),
                                         Int64((1LLU << MethodLiteral::NumArgsBits::SIZE) - 1)));
    GateRef fp = LoadPrimitive(VariableType::NATIVE_POINTER(), state,
        IntPtr(AsmInterpretedFrame::GetFpOffset(env->IsArch32Bit())));
    Label actualEqualDeclared(env);
    Label actualNotEqualDeclared(env);
    Branch(Int32UnsignedGreaterThan(ChangeIntPtrToInt32(PtrSub(fp, sp)),
                                    Int32Mul(Int32Add(Int32Add(numVregs, copyArgs), *numArgs),
                                             Int32(sizeof(JSTaggedType)))),
           &actualNotEqualDeclared, &actualEqualDeclared);
    Bind(&actualNotEqualDeclared);
    {
        numArgs = GetInt32OfTInt(Load(VariableType::JS_ANY(), glue, fp,
            IntPtr(static_cast<int64_t>(-sizeof(JSTaggedType)))));
        Jump(&actualEqualDeclared);
    }
    Bind(&actualEqualDeclared);
    GateRef startIdx = Int32Add(Int32Add(numVregs, copyArgs), restIdx);
    Label numArgsGreater(env);
    Label numArgsNotGreater(env);
    Label exit(env);
    Branch(Int32UnsignedGreaterThan(*numArgs, restIdx), &numArgsGreater, &numArgsNotGreater);
    Bind(&numArgsGreater);
    {
        numArgs = Int32Sub(*numArgs, restIdx);
        Jump(&exit);
    }
    Bind(&numArgsNotGreater);
    {
        numArgs = Int32(0);
        Jump(&exit);
    }
    Bind(&exit);
    // 32: high 32 bits = startIdx, low 32 bits = numArgs
    GateRef ret = Int64Or(Int64LSL(ZExtInt32ToInt64(startIdx), Int64(32)), ZExtInt32ToInt64(*numArgs));
    env->SubCfgExit();
    return ret;
}

void BaselineStubBuilder::SetVregValue(GateRef glue, GateRef sp, GateRef idx, GateRef val)
{
    Store(VariableType::INT64(), glue, sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx), val);
}

GateRef BaselineStubBuilder::GetVregValue(GateRef glue, GateRef sp, GateRef idx)
{
    return Load(VariableType::JS_ANY(), glue, sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx));
}

GateRef BaselineStubBuilder::GetResumeModeFromGeneratorObject(GateRef obj)
{
    GateRef bitfieldOffset = IntPtr(JSGeneratorObject::BIT_FIELD_OFFSET);
    GateRef bitfield = LoadPrimitive(VariableType::INT32(), obj, bitfieldOffset);
    return Int32And(
        Int32LSR(bitfield, Int32(JSGeneratorObject::ResumeModeBits::START_BIT)),
        Int32((1LU << JSGeneratorObject::ResumeModeBits::SIZE) - 1));
}

GateRef BaselineStubBuilder::GetResumeModeFromAsyncGeneratorObject(GateRef obj)
{
    GateRef bitfieldOffset = IntPtr(JSAsyncGeneratorObject::BIT_FIELD_OFFSET);
    GateRef bitfield = LoadPrimitive(VariableType::INT32(), obj, bitfieldOffset);
    return Int32And(
        Int32LSR(bitfield, Int32(JSAsyncGeneratorObject::ResumeModeBits::START_BIT)),
        Int32((1LU << JSAsyncGeneratorObject::ResumeModeBits::SIZE) - 1));
}

GateRef BaselineStubBuilder::GetLastLeaveFrame(GateRef glue)
{
    bool isArch32 = GetEnvironment()->Is32Bit();
    GateRef spOffset = IntPtr(JSThread::GlueData::GetLeaveFrameOffset(isArch32));
    return LoadPrimitive(VariableType::NATIVE_POINTER(), glue, spOffset);
}
}

#endif // ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUBS_INL_H
