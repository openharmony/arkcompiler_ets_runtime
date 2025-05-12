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

#ifndef ECMASCRIPT_COMPILER_INTERPRETER_STUB_INL_H
#define ECMASCRIPT_COMPILER_INTERPRETER_STUB_INL_H

#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"

namespace panda::ecmascript::kungfu {
void InterpreterStubBuilder::SetVregValue(GateRef glue, GateRef sp, GateRef idx, GateRef val)
{
    Store(VariableType::INT64(), glue, sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx), val);
}

inline GateRef InterpreterStubBuilder::GetVregValue(GateRef glue, GateRef sp, GateRef idx)
{
    return Load(VariableType::JS_ANY(), glue, sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx));
}

GateRef InterpreterStubBuilder::ReadInst8_0(GateRef pc)
{
    return LoadPrimitive(VariableType::INT8(), pc, IntPtr(1));  // 1 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_1(GateRef pc)
{
    return LoadPrimitive(VariableType::INT8(), pc, IntPtr(2));  // 2 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_2(GateRef pc)
{
    return LoadPrimitive(VariableType::INT8(), pc, IntPtr(3));  // 3 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_3(GateRef pc)
{
    return LoadPrimitive(VariableType::INT8(), pc, IntPtr(4));  // 4 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_4(GateRef pc)
{
    return LoadPrimitive(VariableType::INT8(), pc, IntPtr(5));  // 5 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_5(GateRef pc)
{
    return LoadPrimitive(VariableType::INT8(), pc, IntPtr(6));  // 6 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_6(GateRef pc)
{
    return LoadPrimitive(VariableType::INT8(), pc, IntPtr(7));  // 7 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_7(GateRef pc)
{
    return LoadPrimitive(VariableType::INT8(), pc, IntPtr(8));  // 8 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_8(GateRef pc)
{
    return LoadPrimitive(VariableType::INT8(), pc, IntPtr(9));  // 9 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_9(GateRef pc)
{
    return LoadPrimitive(VariableType::INT8(), pc, IntPtr(10));  // 10 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst4_0(GateRef pc)
{
    return Int8And(LoadPrimitive(VariableType::INT8(), pc, IntPtr(1)), Int8(0xf));
}

GateRef InterpreterStubBuilder::ReadInst4_1(GateRef pc)
{
    // 1 : skip 1 byte of bytecode
    return Int8And(
        Int8LSR(LoadPrimitive(VariableType::INT8(), pc, IntPtr(1)), Int8(4)), Int8(0xf));  // 4: read 4 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst4_2(GateRef pc)
{
    // 2 : skip 1 byte of bytecode
    return Int8And(LoadPrimitive(VariableType::INT8(), pc, IntPtr(2)), Int8(0xf));
}

GateRef InterpreterStubBuilder::ReadInst4_3(GateRef pc)
{
    // 2 : skip 1 byte of bytecode
    return Int8And(
        Int8LSR(LoadPrimitive(VariableType::INT8(), pc, IntPtr(2)), Int8(4)), Int8(0xf));  // 4 : 4 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInstSigned8_0(GateRef pc)
{
    GateRef x = LoadPrimitive(VariableType::INT8(), pc, IntPtr(1));  // 1 : skip 1 byte of bytecode
    return GetEnvironment()->GetBuilder()->SExtInt1ToInt32(x);
}

GateRef InterpreterStubBuilder::ReadInstSigned16_0(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        GateRef currentInst = LoadPrimitive(VariableType::INT16(), pc, IntPtr(1));  // 1 : skip 1 byte of bytecode
        return GetEnvironment()->GetBuilder()->SExtInt16ToInt32(currentInst);
    }
    /* 2 : skip 8 bits of opcode and 8 bits of low bits */
    GateRef currentInst = LoadPrimitive(VariableType::INT8(), pc, IntPtr(2));
    GateRef currentInst1 = GetEnvironment()->GetBuilder()->SExtInt1ToInt32(currentInst);
    GateRef currentInst2 = Int32LSL(currentInst1, Int32(8));  // 8 : set as high 8 bits
    return Int32Add(currentInst2, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStubBuilder::ReadInstSigned32_0(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        GateRef x = LoadPrimitive(VariableType::INT32(), pc, IntPtr(1));  // 1 : skip 1 byte of bytecode
        return GetEnvironment()->GetBuilder()->SExtInt1ToInt32(x);
    }
    /* 4 : skip 8 bits of opcode and 24 bits of low bits */
    GateRef x = LoadPrimitive(VariableType::INT8(), pc, IntPtr(4));
    GateRef currentInst = GetEnvironment()->GetBuilder()->SExtInt1ToInt32(x);
    GateRef currentInst1 = Int32LSL(currentInst, Int32(8));  // 8 : set as high 8 bits
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, Int32(8));  // 8 : set as high 8 bits
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_1(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, Int32(8));  // 8 : set as high 8 bits
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_0(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT16(), pc, IntPtr(1));  // 1 : skip 1 byte of bytecode
    }
    /* 2 : skip 8 bits of opcode and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_1(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_0(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_1(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT16(), pc, IntPtr(2));  // 2 : skip 2 bytes of bytecode
    }
    /* 3 : skip 8 bits of opcode, 8 bits of prefix and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_2(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 2 : skip 8 bits of opcode and 8 bits of prefix */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_1(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_2(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT16(), pc, IntPtr(3));  // 3 : skip 3 bytes of bytecode
    }
    /* 4 : skip 8 bits of opcode, first parameter of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_3(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 3 : skip 8 bits of opcode and first parameter of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_2(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_3(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT16(), pc, IntPtr(4));  // 4 : skip 4 bytes of bytecode
    }
    /* 5 : skip 8 bits of opcode, 8 bits of prefix, first parameter of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_4(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 4 : skip 8 bits of opcode, 8 bits of prefix and first parameter of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_3(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_4(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT16(), pc, IntPtr(5));  // 5 : skip 5 bytes of bytecode
    }
    /* 7 : skip 8 bits of opcode, 8 bits of prefix, first 2 parameters of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_5(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 6 : skip 8 bits of opcode, 8 bits of prefix and first 2 parameters of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_4(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_5(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT16(), pc, IntPtr(6));  // 6 : skip 6 bytes of bytecode
    }
    /* 7 : skip 8 bits of opcode, 8 bits of prefix, first 2 parameters of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_6(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 6 : skip 8 bits of opcode, 8 bits of prefix and first 2 parameters of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_5(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_6(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT16(), pc, IntPtr(7));  // 7 : skip 7 bytes of bytecode
    }
    /* 7 : skip 8 bits of opcode, 8 bits of prefix, first 2 parameters of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_7(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 6 : skip 8 bits of opcode, 8 bits of prefix and first 2 parameters of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_6(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_7(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT16(), pc, IntPtr(8));  // 8 : skip 8 bytes of bytecode
    }
    /* 7 : skip 8 bits of opcode, 8 bits of prefix, first 2 parameters of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_8(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 6 : skip 8 bits of opcode, 8 bits of prefix and first 2 parameters of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_9(pc)));
}

GateRef InterpreterStubBuilder::GetFrame(GateRef CurrentSp)
{
    return PtrSub(CurrentSp, IntPtr(AsmInterpretedFrame::GetSize(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetPcFromFrame(GateRef frame)
{
    return LoadPrimitive(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetPcOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetFunctionFromFrame(GateRef glue, GateRef frame)
{
    return Load(VariableType::JS_POINTER(), glue, frame,
        IntPtr(AsmInterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetNewTarget(GateRef glue, GateRef sp)
{
    GateRef function = Load(VariableType::JS_POINTER(), glue, GetFrame(sp),
        IntPtr(AsmInterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())));
    GateRef method = GetMethodFromFunction(glue, function);
    GateRef callField = GetCallFieldFromMethod(method);
    // ASSERT: callField has "extra" bit.
    GateRef numVregs = TruncInt64ToInt32(Int64And(Int64LSR(callField, Int64(MethodLiteral::NumVregsBits::START_BIT)),
        Int64((1LLU << MethodLiteral::NumVregsBits::SIZE) - 1)));
    GateRef haveFunc = ZExtInt1ToInt32(Int64NotEqual(Int64And(Int64LSR(callField,
        Int64(MethodLiteral::HaveFuncBit::START_BIT)),
        Int64((1LLU << MethodLiteral::HaveFuncBit::SIZE) - 1)), Int64(0)));
    GateRef idx = ZExtInt32ToPtr(Int32Add(numVregs, haveFunc));
    return Load(VariableType::JS_ANY(), glue, sp, PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), idx));
}

GateRef InterpreterStubBuilder::GetThisFromFrame(GateRef glue, GateRef frame)
{
    return Load(VariableType::JS_POINTER(), glue, frame,
        IntPtr(AsmInterpretedFrame::GetThisOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetCallSizeFromFrame(GateRef frame)
{
    return LoadPrimitive(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetCallSizeOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetAccFromFrame(GateRef glue, GateRef frame)
{
    return Load(VariableType::JS_ANY(), glue, frame,
        IntPtr(AsmInterpretedFrame::GetAccOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetEnvFromFrame(GateRef glue, GateRef frame)
{
    return Load(VariableType::JS_POINTER(), glue, frame,
        IntPtr(AsmInterpretedFrame::GetEnvOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetEnvFromFunction(GateRef glue, GateRef function)
{
    return Load(VariableType::JS_POINTER(), glue, function, IntPtr(JSFunction::LEXICAL_ENV_OFFSET));
}

GateRef InterpreterStubBuilder::GetProfileTypeInfoFromFunction(GateRef glue, GateRef function)
{
    GateRef raw = Load(VariableType::JS_POINTER(), glue, function, IntPtr(JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
    return Load(VariableType::JS_POINTER(), glue, raw, IntPtr(ProfileTypeInfoCell::VALUE_OFFSET));
}

GateRef InterpreterStubBuilder::GetModuleFromFunction(GateRef glue, GateRef function)
{
    return Load(VariableType::JS_POINTER(), glue, function, IntPtr(JSFunction::ECMA_MODULE_OFFSET));
}

GateRef InterpreterStubBuilder::GetSendableEnvFromModule(GateRef glue, GateRef module)
{
    return Load(VariableType::JS_POINTER(), glue, module, IntPtr(SourceTextModule::SENDABLE_ENV_OFFSET));
}

GateRef InterpreterStubBuilder::GetHomeObjectFromFunction(GateRef glue, GateRef function)
{
    return Load(VariableType::JS_POINTER(), glue, function, IntPtr(JSFunction::HOME_OBJECT_OFFSET));
}

GateRef InterpreterStubBuilder::GetConstpoolFromMethod(GateRef glue, GateRef method)
{
    return Load(VariableType::JS_POINTER(), glue, method, IntPtr(Method::CONSTANT_POOL_OFFSET));
}

GateRef InterpreterStubBuilder::GetModule(GateRef glue, GateRef sp)
{
    GateRef currentFunc = GetFunctionFromFrame(glue, GetFrame(sp));
    return GetModuleFromFunction(glue, currentFunc);
}

GateRef InterpreterStubBuilder::GetResumeModeFromGeneratorObject(GateRef obj)
{
    GateRef bitfieldOffset = IntPtr(JSGeneratorObject::BIT_FIELD_OFFSET);
    GateRef bitfield = LoadPrimitive(VariableType::INT32(), obj, bitfieldOffset);
    return Int32And(
        Int32LSR(bitfield, Int32(JSGeneratorObject::ResumeModeBits::START_BIT)),
        Int32((1LU << JSGeneratorObject::ResumeModeBits::SIZE) - 1));
}

GateRef InterpreterStubBuilder::GetResumeModeFromAsyncGeneratorObject(GateRef obj)
{
    GateRef bitfieldOffset = IntPtr(JSAsyncGeneratorObject::BIT_FIELD_OFFSET);
    GateRef bitfield = LoadPrimitive(VariableType::INT32(), obj, bitfieldOffset);
    return Int32And(
        Int32LSR(bitfield, Int32(JSAsyncGeneratorObject::ResumeModeBits::START_BIT)),
        Int32((1LU << JSAsyncGeneratorObject::ResumeModeBits::SIZE) - 1));
}

void InterpreterStubBuilder::SetPcToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame,
        IntPtr(AsmInterpretedFrame::GetPcOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStubBuilder::SetCallSizeToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::NATIVE_POINTER(), glue, frame,
          IntPtr(AsmInterpretedFrame::GetCallSizeOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStubBuilder::SetAccToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame,
          IntPtr(AsmInterpretedFrame::GetAccOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStubBuilder::SetEnvToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame,
          IntPtr(AsmInterpretedFrame::GetEnvOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStubBuilder::SetFunctionToFrame(GateRef glue, GateRef frame, GateRef value)
{
    Store(VariableType::INT64(), glue, frame,
          IntPtr(AsmInterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())), value);
}

void InterpreterStubBuilder::SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value);
}

void InterpreterStubBuilder::SetFrameState(GateRef glue, GateRef sp, GateRef function, GateRef acc,
    GateRef env, GateRef pc, GateRef prev, GateRef type)
{
    GateRef state = GetFrame(sp);
    SetFunctionToFrame(glue, state, function);
    SetAccToFrame(glue, state, acc);
    SetEnvToFrame(glue, state, env);
    SetPcToFrame(glue, state, pc);
    GateRef prevOffset = IntPtr(AsmInterpretedFrame::GetBaseOffset(GetEnvironment()->IsArch32Bit()));
    Store(VariableType::NATIVE_POINTER(), glue, state, prevOffset, prev);
    GateRef frameTypeOffset = PtrAdd(prevOffset, IntPtr(
        InterpretedFrameBase::GetTypeOffset(GetEnvironment()->IsArch32Bit())));
    Store(VariableType::INT64(), glue, state, frameTypeOffset, type);
}

GateRef InterpreterStubBuilder::GetCurrentSpFrame(GateRef glue)
{
    bool isArch32 = GetEnvironment()->Is32Bit();
    GateRef spOffset = IntPtr(JSThread::GlueData::GetCurrentFrameOffset(isArch32));
    return LoadPrimitive(VariableType::NATIVE_POINTER(), glue, spOffset);
}

void InterpreterStubBuilder::SetCurrentSpFrame(GateRef glue, GateRef value)
{
    GateRef spOffset = IntPtr(JSThread::GlueData::GetCurrentFrameOffset(GetEnvironment()->Is32Bit()));
    Store(VariableType::NATIVE_POINTER(), glue, glue, spOffset, value);
}

GateRef InterpreterStubBuilder::GetLastLeaveFrame(GateRef glue)
{
    bool isArch32 = GetEnvironment()->Is32Bit();
    GateRef spOffset = IntPtr(JSThread::GlueData::GetLeaveFrameOffset(isArch32));
    return LoadPrimitive(VariableType::NATIVE_POINTER(), glue, spOffset);
}

void InterpreterStubBuilder::SetLastLeaveFrame(GateRef glue, GateRef value)
{
    GateRef spOffset = IntPtr(JSThread::GlueData::GetLeaveFrameOffset(GetEnvironment()->Is32Bit()));
    Store(VariableType::NATIVE_POINTER(), glue, glue, spOffset, value);
}

GateRef InterpreterStubBuilder::CheckStackOverflow(GateRef glue, GateRef sp)
{
    GateRef frameBaseOffset = IntPtr(JSThread::GlueData::GetFrameBaseOffset(GetEnvironment()->IsArch32Bit()));
    GateRef frameBase = LoadPrimitive(VariableType::NATIVE_POINTER(), glue, frameBaseOffset);
    return Int64UnsignedLessThanOrEqual(sp,
        PtrAdd(frameBase, IntPtr(JSThread::RESERVE_STACK_SIZE * sizeof(JSTaggedType))));
}

GateRef InterpreterStubBuilder::PushArg(GateRef glue, GateRef sp, GateRef value)
{
    GateRef newSp = PtrSub(sp, IntPtr(sizeof(JSTaggedType)));
    // 0 : skip 0 byte of bytecode
    Store(VariableType::INT64(), glue, newSp, IntPtr(0), value);
    return newSp;
}

GateRef InterpreterStubBuilder::PushUndefined(GateRef glue, GateRef sp, GateRef num)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    DEFVARIABLE(newSp, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label pushUndefinedBegin(env);
    Label pushUndefinedAgain(env);
    Label pushUndefinedEnd(env);
    BRANCH(Int32LessThan(*i, num), &pushUndefinedBegin, &pushUndefinedEnd);
    LoopBegin(&pushUndefinedBegin);
    newSp = PushArg(glue, *newSp, Undefined());
    i = Int32Add(*i, Int32(1));  // 1 : set as high 1 bits
    BRANCH(Int32LessThan(*i, num), &pushUndefinedAgain, &pushUndefinedEnd);
    Bind(&pushUndefinedAgain);
    LoopEnd(&pushUndefinedBegin);
    Bind(&pushUndefinedEnd);
    auto ret = *newSp;
    env->SubCfgExit();
    return ret;
}

GateRef InterpreterStubBuilder::PushRange(GateRef glue, GateRef sp, GateRef array, GateRef startIndex, GateRef endIndex)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    DEFVARIABLE(newSp, VariableType::NATIVE_POINTER(), sp);
    DEFVARIABLE(i, VariableType::INT32(), endIndex);
    Label pushArgsBegin(env);
    Label pushArgsAgain(env);
    Label pushArgsEnd(env);
    BRANCH(Int32GreaterThanOrEqual(*i, startIndex), &pushArgsBegin, &pushArgsEnd);
    LoopBegin(&pushArgsBegin);
    GateRef arg = GetVregValue(glue, array, ZExtInt32ToPtr(*i));
    newSp = PushArg(glue, *newSp, arg);
    i = Int32Sub(*i, Int32(1));  // 1 : set as high 1 bits
    BRANCH(Int32GreaterThanOrEqual(*i, startIndex), &pushArgsAgain, &pushArgsEnd);
    Bind(&pushArgsAgain);
    LoopEnd(&pushArgsBegin);
    Bind(&pushArgsEnd);
    auto ret = *newSp;
    env->SubCfgExit();
    return ret;
}

GateRef InterpreterStubBuilder::GetStartIdxAndNumArgs(GateRef glue, GateRef sp, GateRef restIdx)
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
    GateRef numVregs = TruncInt64ToInt32(Int64And(Int64LSR(callField, Int64(MethodLiteral::NumVregsBits::START_BIT)),
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
    BRANCH(Int32UnsignedGreaterThan(ChangeIntPtrToInt32(PtrSub(fp, sp)),
                                    Int32Mul(Int32Add(Int32Add(numVregs, copyArgs), *numArgs),
                                             Int32(sizeof(JSTaggedType)))),
           &actualNotEqualDeclared, &actualEqualDeclared);
    Bind(&actualNotEqualDeclared);
    {
        numArgs = GetInt32OfTInt(LoadPrimitive(VariableType::JS_ANY(), fp, IntPtr(-sizeof(JSTaggedType))));
        Jump(&actualEqualDeclared);
    }
    Bind(&actualEqualDeclared);
    GateRef startIdx = Int32Add(Int32Add(numVregs, copyArgs), restIdx);
    Label numArgsGreater(env);
    Label numArgsNotGreater(env);
    Label exit(env);
    BRANCH(Int32UnsignedGreaterThan(*numArgs, restIdx), &numArgsGreater, &numArgsNotGreater);
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

GateRef InterpreterStubBuilder::GetCurrentFrame(GateRef glue)
{
    return GetLastLeaveFrame(glue);
}

void InterpreterStubBuilder::UpdateProfileTypeInfoCellToFunction(GateRef glue, GateRef function,
                                                                 GateRef profileTypeInfo, GateRef slotId)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    Label profileTypeInfoNotUndefined(env);
    Label slotValueUpdate(env);
    Label slotValueNotUndefined(env);
    Label slotValueNotHole(env);
    Label profileTypeInfoEnd(env);
    NewObjectStubBuilder newBuilder(this);
    BRANCH(TaggedIsUndefined(profileTypeInfo), &profileTypeInfoEnd, &profileTypeInfoNotUndefined);
    Bind(&profileTypeInfoNotUndefined);
    {
        GateRef slotValue = GetValueFromTaggedArray(glue, profileTypeInfo, slotId);
        BRANCH_UNLIKELY(TaggedIsUndefined(slotValue), &slotValueUpdate, &slotValueNotUndefined);
        Bind(&slotValueUpdate);
        {
            GateRef newProfileTypeInfoCell = newBuilder.NewProfileTypeInfoCell(glue, Undefined());
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, newProfileTypeInfoCell,
                                  MemoryAttribute::NeedNotShareBarrier());
            SetRawProfileTypeInfoToFunction(glue, function, newProfileTypeInfoCell,
                                            MemoryAttribute::NeedNotShareBarrier());
            Jump(&profileTypeInfoEnd);
        }
        Bind(&slotValueNotUndefined);
        BRANCH_UNLIKELY(TaggedIsHole(slotValue), &profileTypeInfoEnd, &slotValueNotHole);
        Bind(&slotValueNotHole);
        {
            UpdateProfileTypeInfoCellType(glue, slotValue);
            SetRawProfileTypeInfoToFunction(glue, function, slotValue, MemoryAttribute::NeedNotShareBarrier());
            TryToJitReuseCompiledFunc(glue, function, slotValue);
        }
        Jump(&profileTypeInfoEnd);
    }
    Bind(&profileTypeInfoEnd);

    env->SubCfgExit();
}

GateRef InterpreterStubBuilder::ReadInst32_0(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT32(), pc, IntPtr(1));  // 1 : skip 1 byte of bytecode
    }
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_3(pc));
    GateRef currentInst1 = Int32LSL(currentInst, Int32(8));  // 8 : set as high 8 bits
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, Int32(8));  // 8 : set as high 8 bits
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_1(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, Int32(8));  // 8 : set as high 8 bits
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStubBuilder::ReadInst32_1(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT32(), pc, IntPtr(2));  // 2 : skip 2 bytes of bytecode
    }
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_4(pc));
    GateRef currentInst1 = Int32LSL(currentInst, Int32(8));  // 8 : set as high 8 bits
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_3(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, Int32(8));  // 8 : set as high 8 bits
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, Int32(8));  // 8 : set as high 8 bits
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_1(pc)));
}

GateRef InterpreterStubBuilder::ReadInst32_2(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT32(), pc, IntPtr(3));  // 3 : skip 3 bytes of bytecode
    }
    GateRef currentInst = ZExtInt8ToInt32(ReadInst8_5(pc));
    GateRef currentInst1 = Int32LSL(currentInst, Int32(8));  // 8 : set as high 8 bits
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_4(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, Int32(8));  // 8 : set as high 8 bits
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_3(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, Int32(8));  // 8 : set as high 8 bits
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_2(pc)));
}

GateRef InterpreterStubBuilder::ReadInst64_0(GateRef pc)
{
    if (GetEnvironment()->IsAArch64() || GetEnvironment()->IsAmd64()) {
        return LoadPrimitive(VariableType::INT64(), pc, IntPtr(1));  // 1 : skip 1 byte of bytecode
    }
    GateRef currentInst = ZExtInt8ToInt64(ReadInst8_7(pc));
    GateRef currentInst1 = Int64LSL(currentInst, Int64(8));  // 8 : set as high 8 bits
    GateRef currentInst2 = Int64Add(currentInst1, ZExtInt8ToInt64(ReadInst8_6(pc)));
    GateRef currentInst3 = Int64LSL(currentInst2, Int64(8));  // 8 : set as high 8 bits
    GateRef currentInst4 = Int64Add(currentInst3, ZExtInt8ToInt64(ReadInst8_5(pc)));
    GateRef currentInst5 = Int64LSL(currentInst4, Int64(8));  // 8 : set as high 8 bits
    GateRef currentInst6 = Int64Add(currentInst5, ZExtInt8ToInt64(ReadInst8_4(pc)));
    GateRef currentInst7 = Int64LSL(currentInst6, Int64(8));  // 8 : set as high 8 bits
    GateRef currentInst8 = Int64Add(currentInst7, ZExtInt8ToInt64(ReadInst8_3(pc)));
    GateRef currentInst9 = Int64LSL(currentInst8, Int64(8));  // 8 : set as high 8 bits
    GateRef currentInst10 = Int64Add(currentInst9, ZExtInt8ToInt64(ReadInst8_2(pc)));
    GateRef currentInst11 = Int64LSL(currentInst10, Int64(8));  // 8 : set as high 8 bits
    GateRef currentInst12 = Int64Add(currentInst11, ZExtInt8ToInt64(ReadInst8_1(pc)));
    GateRef currentInst13 = Int64LSL(currentInst12, Int64(8));  // 8 : set as high 8 bits
    return Int64Add(currentInst13, ZExtInt8ToInt64(ReadInst8_0(pc)));
}

template<typename... Args>
void InterpreterStubBuilder::DispatchBase(GateRef target, GateRef glue, Args... args)
{
    GetEnvironment()->GetBuilder()->CallBCHandler(glue, target, {glue, args...});
}

void InterpreterStubBuilder::Dispatch(GateRef glue, GateRef sp, GateRef pc, GateRef constpool, GateRef profileTypeInfo,
    GateRef acc, GateRef hotnessCounter, GateRef format)
{
    GateRef newPc = PtrAdd(pc, format);
    GateRef opcode = LoadZeroOffsetPrimitive(VariableType::INT8(), newPc);
    GateRef target = PtrMul(ZExtInt32ToPtr(ZExtInt8ToInt32(opcode)), IntPtrSize());
    DispatchBase(target, glue, sp, newPc, constpool, profileTypeInfo, acc, hotnessCounter);
    Return();
}

void InterpreterStubBuilder::DispatchLast(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
    GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter)
{
    GateRef target = PtrMul(IntPtr(BytecodeStubCSigns::ID_ExceptionHandler), IntPtrSize());
    DispatchBase(target, glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);
    Return();
}

void InterpreterStubBuilder::DispatchDebugger(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
    GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter)
{
    GateRef opcode = LoadZeroOffsetPrimitive(VariableType::INT8(), pc);
    GateRef target = PtrMul(ZExtInt32ToPtr(ZExtInt8ToInt32(opcode)), IntPtrSize());
    auto args = { glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter };
    GetEnvironment()->GetBuilder()->CallBCDebugger(glue, target, args);
    Return();
}

void InterpreterStubBuilder::DispatchDebuggerLast(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
    GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter)
{
    GateRef target = PtrMul(IntPtr(BytecodeStubCSigns::ID_ExceptionHandler), IntPtrSize());
    auto args = { glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter };
    GetEnvironment()->GetBuilder()->CallBCDebugger(glue, target, args);
    Return();
}

GateRef InterpreterStubBuilder::GetHotnessCounterFromMethod(GateRef method)
{
    GateRef x = LoadPrimitive(VariableType::INT16(), method, IntPtr(Method::LITERAL_INFO_OFFSET));
    return GetEnvironment()->GetBuilder()->SExtInt1ToInt32(x);
}

void InterpreterStubBuilder::DispatchWithId(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
    GateRef profileTypeInfo, GateRef acc,
    GateRef hotnessCounter, GateRef index)
{
    GateRef target = PtrMul(index, IntPtrSize());
    DispatchBase(target, glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);
    Return();
}

#define DISPATCH_LAST(acc)                                                                  \
    DispatchLast(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter)
#define DISPATCH(acc)                                                                       \
    Dispatch(glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter, offset)
void InterpreterStubBuilder::CheckException(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                                            GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter,
                                            GateRef res, GateRef offset)
{
    auto env = GetEnvironment();
    Label isException(env);
    Label notException(env);
    BRANCH(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST(acc);
    }
    Bind(&notException);
    {
        DISPATCH(acc);
    }
}

void InterpreterStubBuilder::CheckPendingException(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                                                   GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter,
                                                   GateRef res, GateRef offset)
{
    auto env = GetEnvironment();
    Label isException(env);
    Label notException(env);
    BRANCH(HasPendingException(glue), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST(acc);
    }
    Bind(&notException);
    {
        DISPATCH(res);
    }
}

void InterpreterStubBuilder::CheckExceptionWithVar(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                                                   GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter,
                                                   GateRef res, GateRef offset)
{
    auto env = GetEnvironment();
    Label isException(env);
    Label notException(env);
    BRANCH(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST(acc);
    }
    Bind(&notException);
    {
        DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
        varAcc = res;
        DISPATCH(*varAcc);
    }
}

void InterpreterStubBuilder::CheckExceptionWithJump(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                                                    GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter,
                                                    GateRef res, Label *jump)
{
    auto env = GetEnvironment();
    Label isException(env);
    Label notException(env);
    BRANCH(TaggedIsException(res), &isException, &notException);
    Bind(&isException);
    {
        DISPATCH_LAST(acc);
    }
    Bind(&notException);
    {
        Jump(jump);
    }
}

GateRef InterpreterToolsStubBuilder::GetStringId(const StringIdInfo &info)
{
    if (info.GetStringIdType() == StringIdInfo::StringIdType::STRING_ID) {
        return info.GetStringId();
    }
    GateRef stringId;
    switch (info.GetOffset()) {
        case StringIdInfo::Offset::BYTE_0: {
            if (info.GetLength() == StringIdInfo::Length::BITS_16) {
                stringId = ZExtInt16ToInt32(ReadInst16_0(info.GetPc()));
            } else {
                std::abort();
            }
            break;
        }
        case StringIdInfo::Offset::BYTE_1: {
            if (info.GetLength() == StringIdInfo::Length::BITS_16) {
                stringId = ZExtInt16ToInt32(ReadInst16_1(info.GetPc()));
            } else if (info.GetLength() == StringIdInfo::Length::BITS_32) {
                stringId = ReadInst32_1(info.GetPc());
            } else {
                std::abort();
            }
            break;
        }
        case StringIdInfo::Offset::BYTE_2: {
            if (info.GetLength() == StringIdInfo::Length::BITS_16) {
                stringId = ZExtInt16ToInt32(ReadInst16_2(info.GetPc()));
            } else {
                std::abort();
            }
            break;
        }
        default: {
            std::abort();
            break;
        }
    }
    return stringId;
}
#undef DISPATCH_LAST
#undef DISPATCH
} //  namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_INTERPRETER_STUB_INL_H
