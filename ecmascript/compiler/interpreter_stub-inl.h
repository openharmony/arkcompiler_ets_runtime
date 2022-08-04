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
#include "ecmascript/global_env.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"

namespace panda::ecmascript::kungfu {
void InterpreterStubBuilder::SetVregValue(GateRef glue, GateRef sp, GateRef idx, GateRef val)
{
    Store(VariableType::INT64(), glue, sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx), val);
}

inline GateRef InterpreterStubBuilder::GetVregValue(GateRef sp, GateRef idx)
{
    return Load(VariableType::JS_ANY(), sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx));
}

GateRef InterpreterStubBuilder::ReadInst8_0(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(1));  // 1 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_1(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(2));  // 2 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_2(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(3));  // 3 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_3(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(4));  // 4 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_4(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(5));  // 5 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_5(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(6));  // 6 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_6(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(7));  // 7 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_7(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(8));  // 8 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst8_8(GateRef pc)
{
    return Load(VariableType::INT8(), pc, IntPtr(9));  // 9 : skip 1 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst4_0(GateRef pc)
{
    return Int8And(Load(VariableType::INT8(), pc, IntPtr(1)), Int8(0xf));
}

GateRef InterpreterStubBuilder::ReadInst4_1(GateRef pc)
{
    // 1 : skip 1 byte of bytecode
    return Int8And(
        Int8LSR(Load(VariableType::INT8(), pc, IntPtr(1)), Int8(4)), Int8(0xf));  // 4: read 4 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInst4_2(GateRef pc)
{
    // 2 : skip 1 byte of bytecode
    return Int8And(Load(VariableType::INT8(), pc, IntPtr(2)), Int8(0xf));
}

GateRef InterpreterStubBuilder::ReadInst4_3(GateRef pc)
{
    // 2 : skip 1 byte of bytecode
    return Int8And(
        Int8LSR(Load(VariableType::INT8(), pc, IntPtr(2)), Int8(4)), Int8(0xf));  // 4 : read 4 byte of bytecode
}

GateRef InterpreterStubBuilder::ReadInstSigned8_0(GateRef pc)
{
    GateRef x = Load(VariableType::INT8(), pc, IntPtr(1));  // 1 : skip 1 byte of bytecode
    return GetEnvironment()->GetBuilder()->UnaryArithmetic(OpCode(OpCode::SEXT_TO_INT32), x);
}

GateRef InterpreterStubBuilder::ReadInstSigned16_0(GateRef pc)
{
    /* 2 : skip 8 bits of opcode and 8 bits of low bits */
    GateRef currentInst = Load(VariableType::INT8(), pc, IntPtr(2));
    GateRef currentInst1 = GetEnvironment()->GetBuilder()->UnaryArithmetic(
        OpCode(OpCode::SEXT_TO_INT32), currentInst);
    GateRef currentInst2 = Int32LSL(currentInst1, Int32(8));  // 8 : set as high 8 bits
    return Int32Add(currentInst2, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStubBuilder::ReadInstSigned32_0(GateRef pc)
{
    /* 4 : skip 8 bits of opcode and 24 bits of low bits */
    GateRef x = Load(VariableType::INT8(), pc, IntPtr(4));
    GateRef currentInst = GetEnvironment()->GetBuilder()->UnaryArithmetic(OpCode(OpCode::SEXT_TO_INT32), x);
    GateRef currentInst1 = Int32LSL(currentInst, Int32(8));  // 8 : set as high 8 bits
    GateRef currentInst2 = Int32Add(currentInst1, ZExtInt8ToInt32(ReadInst8_2(pc)));
    GateRef currentInst3 = Int32LSL(currentInst2, Int32(8));  // 8 : set as high 8 bits
    GateRef currentInst4 = Int32Add(currentInst3, ZExtInt8ToInt32(ReadInst8_1(pc)));
    GateRef currentInst5 = Int32LSL(currentInst4, Int32(8));  // 8 : set as high 8 bits
    return Int32Add(currentInst5, ZExtInt8ToInt32(ReadInst8_0(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_0(GateRef pc)
{
    /* 2 : skip 8 bits of opcode and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_1(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_0(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_1(GateRef pc)
{
    /* 3 : skip 8 bits of opcode, 8 bits of prefix and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_2(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 2 : skip 8 bits of opcode and 8 bits of prefix */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_1(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_2(GateRef pc)
{
    /* 4 : skip 8 bits of opcode, first parameter of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_3(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 3 : skip 8 bits of opcode and first parameter of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_2(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_3(GateRef pc)
{
    /* 5 : skip 8 bits of opcode, 8 bits of prefix, first parameter of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_4(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 4 : skip 8 bits of opcode, 8 bits of prefix and first parameter of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_3(pc)));
}

GateRef InterpreterStubBuilder::ReadInst16_5(GateRef pc)
{
    /* 7 : skip 8 bits of opcode, 8 bits of prefix, first 2 parameters of 16 bits and 8 bits of low bits */
    GateRef currentInst1 = ZExtInt8ToInt16(ReadInst8_6(pc));
    GateRef currentInst2 = Int16LSL(currentInst1, Int16(8));  // 8 : set as high 8 bits
    /* 6 : skip 8 bits of opcode, 8 bits of prefix and first 2 parameters of 16 bits */
    return Int16Add(currentInst2, ZExtInt8ToInt16(ReadInst8_5(pc)));
}

GateRef InterpreterStubBuilder::GetFrame(GateRef CurrentSp)
{
    return PtrSub(CurrentSp, IntPtr(AsmInterpretedFrame::GetSize(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetPcFromFrame(GateRef frame)
{
    return Load(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetPcOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetFunctionFromFrame(GateRef frame)
{
    return Load(VariableType::JS_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetCallSizeFromFrame(GateRef frame)
{
    return Load(VariableType::NATIVE_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetCallSizeOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetAccFromFrame(GateRef frame)
{
    return Load(VariableType::JS_ANY(), frame,
        IntPtr(AsmInterpretedFrame::GetAccOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetEnvFromFrame(GateRef frame)
{
    return Load(VariableType::JS_POINTER(), frame,
        IntPtr(AsmInterpretedFrame::GetEnvOffset(GetEnvironment()->IsArch32Bit())));
}

GateRef InterpreterStubBuilder::GetEnvFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::LEXICAL_ENV_OFFSET));
}

GateRef InterpreterStubBuilder::GetProfileTypeInfoFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::PROFILE_TYPE_INFO_OFFSET));
}

GateRef InterpreterStubBuilder::GetModuleFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::ECMA_MODULE_OFFSET));
}

GateRef InterpreterStubBuilder::GetConstpoolFromFunction(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::CONSTANT_POOL_OFFSET));
}

// only use for fast new, not universal API
GateRef InterpreterStubBuilder::GetThisObjectFromFastNewFrame(GateRef prevSp)
{
    auto idx = AsmInterpretedFrame::ReverseIndex::THIS_OBJECT_REVERSE_INDEX;
    return Load(VariableType::JS_ANY(), prevSp, IntPtr(idx * sizeof(JSTaggedType)));
}

GateRef InterpreterStubBuilder::GetResumeModeFromGeneratorObject(GateRef obj)
{
    GateRef bitfieldOffset = IntPtr(JSGeneratorObject::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), obj, bitfieldOffset);
    return Int32And(
        Int32LSR(bitfield, Int32(JSGeneratorObject::ResumeModeBits::START_BIT)),
        Int32((1LU << JSGeneratorObject::ResumeModeBits::SIZE) - 1));
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

void InterpreterStubBuilder::SetConstantPoolToFunction(GateRef glue, GateRef function, GateRef value)
{
    Store(VariableType::INT64(), glue, function,
          IntPtr(JSFunction::CONSTANT_POOL_OFFSET), value);
}

void InterpreterStubBuilder::SetResolvedToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef bitfield = GetFunctionBitFieldFromJSFunction(function);
    GateRef mask = Int32(
        ~(((1<<JSFunction::ResolvedBits::SIZE) - 1) << JSFunction::ResolvedBits::START_BIT));
    GateRef result = Int32Or(Int32And(bitfield, mask),
        Int32LSL(ZExtInt1ToInt32(value), Int32(JSFunction::ResolvedBits::START_BIT)));
    Store(VariableType::INT32(), glue, function, IntPtr(JSFunction::BIT_FIELD_OFFSET), result);
}

void InterpreterStubBuilder::SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value);
}

void InterpreterStubBuilder::SetModuleToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, function, offset, value);
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
    return Load(VariableType::NATIVE_POINTER(), glue, spOffset);
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
    return Load(VariableType::NATIVE_POINTER(), glue, spOffset);
}

void InterpreterStubBuilder::SetLastLeaveFrame(GateRef glue, GateRef value)
{
    GateRef spOffset = IntPtr(JSThread::GlueData::GetLeaveFrameOffset(GetEnvironment()->Is32Bit()));
    Store(VariableType::NATIVE_POINTER(), glue, glue, spOffset, value);
}

GateRef InterpreterStubBuilder::CheckStackOverflow(GateRef glue, GateRef sp)
{
    GateRef frameBaseOffset = IntPtr(JSThread::GlueData::GetFrameBaseOffset(GetEnvironment()->IsArch32Bit()));
    GateRef frameBase = Load(VariableType::NATIVE_POINTER(), glue, frameBaseOffset);
    return Int64UnsignedLessThanOrEqual(sp,
        PtrAdd(frameBase, IntPtr(JSThread::RESERVE_STACK_SIZE * sizeof(JSTaggedType))));
}

GateRef InterpreterStubBuilder::PushArg(GateRef glue, GateRef sp, GateRef value)
{
    GateRef newSp = PointerSub(sp, IntPtr(sizeof(JSTaggedType)));
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
    Branch(Int32LessThan(*i, num), &pushUndefinedBegin, &pushUndefinedEnd);
    LoopBegin(&pushUndefinedBegin);
    newSp = PushArg(glue, *newSp, Int64(JSTaggedValue::VALUE_UNDEFINED));
    i = Int32Add(*i, Int32(1));  // 1 : set as high 1 bits
    Branch(Int32LessThan(*i, num), &pushUndefinedAgain, &pushUndefinedEnd);
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
    Branch(Int32GreaterThanOrEqual(*i, startIndex), &pushArgsBegin, &pushArgsEnd);
    LoopBegin(&pushArgsBegin);
    GateRef arg = GetVregValue(array, ChangeInt32ToIntPtr(*i));
    newSp = PushArg(glue, *newSp, arg);
    i = Int32Sub(*i, Int32(1));  // 1 : set as high 1 bits
    Branch(Int32GreaterThanOrEqual(*i, startIndex), &pushArgsAgain, &pushArgsEnd);
    Bind(&pushArgsAgain);
    LoopEnd(&pushArgsBegin);
    Bind(&pushArgsEnd);
    auto ret = *newSp;
    env->SubCfgExit();
    return ret;
}

GateRef InterpreterStubBuilder::GetStartIdxAndNumArgs(GateRef sp, GateRef restIdx)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    DEFVARIABLE(numArgs, VariableType::INT32(), Int32(0));
    GateRef state = PtrSub(sp, IntPtr(AsmInterpretedFrame::GetSize(env->IsArch32Bit())));
    GateRef function = GetFunctionFromFrame(state);
    GateRef method = GetMethodFromJSFunction(function);
    GateRef callField = GetCallFieldFromMethod(method);
    // ASSERT: callField has "extra" bit.
    GateRef numVregs = TruncInt64ToInt32(Int64And(Int64LSR(callField, Int64(JSMethod::NumVregsBits::START_BIT)),
        Int64((1LLU << JSMethod::NumVregsBits::SIZE) - 1)));
    GateRef haveFunc = Int64NotEqual(Int64And(Int64LSR(callField, Int64(JSMethod::HaveFuncBit::START_BIT)),
        Int64((1LLU << JSMethod::HaveFuncBit::SIZE) - 1)), Int64(0));
    GateRef haveNewTarget = Int64NotEqual(Int64And(Int64LSR(callField, Int64(JSMethod::HaveNewTargetBit::START_BIT)),
        Int64((1LLU << JSMethod::HaveNewTargetBit::SIZE) - 1)), Int64(0));
    GateRef haveThis = Int64NotEqual(Int64And(Int64LSR(callField, Int64(JSMethod::HaveThisBit::START_BIT)),
        Int64((1LLU << JSMethod::HaveThisBit::SIZE) - 1)), Int64(0));
    GateRef copyArgs = Int32Add(Int32Add(ZExtInt1ToInt32(haveFunc), ZExtInt1ToInt32(haveNewTarget)),
                                ZExtInt1ToInt32(haveThis));
    numArgs = TruncInt64ToInt32(Int64And(Int64LSR(callField, Int64(JSMethod::NumArgsBits::START_BIT)),
                                         Int64((1LLU << JSMethod::NumArgsBits::SIZE) - 1)));
    GateRef fp = Load(VariableType::NATIVE_POINTER(), state,
                      IntPtr(AsmInterpretedFrame::GetFpOffset(env->IsArch32Bit())));
    Label actualEqualDeclared(env);
    Label actualNotEqualDeclared(env);
    Branch(Int32UnsignedGreaterThan(ChangeIntPtrToInt32(PtrSub(fp, sp)),
                                    Int32Mul(Int32Add(Int32Add(numVregs, copyArgs), *numArgs),
                                             Int32(sizeof(JSTaggedType)))),
           &actualNotEqualDeclared, &actualEqualDeclared);
    Bind(&actualNotEqualDeclared);
    {
        numArgs = TaggedCastToInt32(Load(VariableType::JS_ANY(), fp, IntPtr(-sizeof(JSTaggedType))));
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

GateRef InterpreterStubBuilder::NewArgumentsList(GateRef glue, GateRef sp, GateRef startIdx, GateRef numArgs)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label exit(env);
    Label setHClass(env);
    GateRef arraySize = ComputeTaggedArraySize(ChangeInt32ToIntPtr(numArgs));
    GateRef argumentsList = AllocateInYoung(glue, arraySize);
    Branch(TaggedIsException(argumentsList), &exit, &setHClass);
    Bind(&setHClass);
    GateRef arrayClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                ConstantIndex::ARRAY_CLASS_INDEX);
    StoreHClass(glue, argumentsList, arrayClass);
    Store(VariableType::INT32(), glue, argumentsList, IntPtr(TaggedArray::LENGTH_OFFSET), numArgs);
    // skip InitializeTaggedArrayWithSpeicalValue due to immediate setting arguments
    Label setArgumentsBegin(env);
    Label setArgumentsAgain(env);
    Label setArgumentsEnd(env);
    Branch(Int32UnsignedLessThan(*i, numArgs), &setArgumentsBegin, &setArgumentsEnd);
    LoopBegin(&setArgumentsBegin);
    GateRef argument = GetVregValue(sp, ChangeInt32ToIntPtr(Int32Add(startIdx, *i)));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, argumentsList, *i, argument);
    i = Int32Add(*i, Int32(1));
    Branch(Int32UnsignedLessThan(*i, numArgs), &setArgumentsAgain, &setArgumentsEnd);
    Bind(&setArgumentsAgain);
    LoopEnd(&setArgumentsBegin);
    Bind(&setArgumentsEnd);
    Jump(&exit);
    Bind(&exit);
    env->SubCfgExit();
    return argumentsList;
}

GateRef InterpreterStubBuilder::NewArgumentsObj(GateRef glue, GateRef argumentsList, GateRef numArgs)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef argumentsClass = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                               GlobalEnv::ARGUMENTS_CLASS);
    GateRef argumentsObj = NewJSObject(glue, argumentsClass);
    Label exit(env);
    Label setArgumentsObjProperties(env);
    Branch(TaggedIsException(argumentsObj), &exit, &setArgumentsObjProperties);
    Bind(&setArgumentsObjProperties);
    SetPropertyInlinedProps(glue, argumentsObj, argumentsClass, IntToTaggedNGC(numArgs),
                            Int32(JSArguments::LENGTH_INLINE_PROPERTY_INDEX));
    SetElementsArray(VariableType::JS_ANY(), glue, argumentsObj, argumentsList);
    GateRef arrayProtoValuesFunction = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                                         GlobalEnv::ARRAY_PROTO_VALUES_FUNCTION_INDEX);
    SetPropertyInlinedProps(glue, argumentsObj, argumentsClass, arrayProtoValuesFunction,
                            Int32(JSArguments::ITERATOR_INLINE_PROPERTY_INDEX));
    GateRef accessorCaller = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                               GlobalEnv::ARGUMENTS_CALLER_ACCESSOR);
    SetPropertyInlinedProps(glue, argumentsObj, argumentsClass, accessorCaller,
                            Int32(JSArguments::CALLER_INLINE_PROPERTY_INDEX));
    GateRef accessorCallee = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                               GlobalEnv::ARGUMENTS_CALLEE_ACCESSOR);
    SetPropertyInlinedProps(glue, argumentsObj, argumentsClass, accessorCallee,
                            Int32(JSArguments::CALLEE_INLINE_PROPERTY_INDEX));
    Jump(&exit);
    Bind(&exit);
    env->SubCfgExit();
    return argumentsObj;
}

GateRef InterpreterStubBuilder::GetCurrentFrame(GateRef glue)
{
    return GetLastLeaveFrame(glue);
}

GateRef InterpreterStubBuilder::ReadInst32_0(GateRef pc)
{
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
    GateRef opcode = Load(VariableType::INT8(), newPc);
    GateRef target = PtrMul(ChangeInt32ToIntPtr(ZExtInt8ToInt32(opcode)), IntPtrSize());
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
    GateRef opcode = Load(VariableType::INT8(), pc);
    GateRef target = PtrMul(ChangeInt32ToIntPtr(ZExtInt8ToInt32(opcode)), IntPtrSize());
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

GateRef InterpreterStubBuilder::GetObjectFromConstPool(GateRef constpool, GateRef index)
{
    return GetValueFromTaggedArray(VariableType::JS_ANY(), constpool, index);
}

GateRef InterpreterStubBuilder::FunctionIsResolved(GateRef object)
{
    GateRef bitfield = GetFunctionBitFieldFromJSFunction(object);
    // decode
    return Int32NotEqual(
        Int32And(
            Int32LSR(bitfield, Int32(JSFunction::ResolvedBits::START_BIT)),
            Int32((1LU << JSFunction::ResolvedBits::SIZE) - 1)),
        Int32(0));
}

GateRef InterpreterStubBuilder::GetHotnessCounterFromMethod(GateRef method)
{
    auto env = GetEnvironment();
    GateRef x = Load(VariableType::INT16(), method,
                     IntPtr(JSMethod::GetHotnessCounterOffset(env->IsArch32Bit())));
    return GetEnvironment()->GetBuilder()->UnaryArithmetic(OpCode(OpCode::SEXT_TO_INT32), x);
}

void InterpreterStubBuilder::SetHotnessCounter(GateRef glue, GateRef method, GateRef value)
{
    auto env = GetEnvironment();
    GateRef newValue = env->GetBuilder()->UnaryArithmetic(OpCode(OpCode::TRUNC_TO_INT16), value);
    Store(VariableType::INT16(), glue, method,
          IntPtr(JSMethod::GetHotnessCounterOffset(env->IsArch32Bit())), newValue);
}

void InterpreterStubBuilder::DispatchWithId(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                                     GateRef profileTypeInfo, GateRef acc,
                                     GateRef hotnessCounter, int index)
{
    GateRef target = PtrMul(IntPtr(index), IntPtrSize());
    DispatchBase(target, glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);
    Return();
}
} //  namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_INTERPRETER_STUB_INL_H
