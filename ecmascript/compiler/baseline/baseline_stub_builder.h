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

#ifndef ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUB_BUILDER_H

#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/base/config.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/profiler_operation.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/circuit_builder_helper.h"

namespace panda::ecmascript::kungfu {

// currently deprecated
class BaselineStubBuilder : public StubBuilder {
public:
    static_assert(false);
    BaselineStubBuilder(CallSignature *callSignature, Environment *env)
        : StubBuilder(callSignature, env) {}
    ~BaselineStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BaselineStubBuilder);
    NO_COPY_SEMANTIC(BaselineStubBuilder);
    virtual void GenerateCircuit() override = 0;

    inline void SetEnvToFrame(GateRef glue, GateRef frame, GateRef value)
    {
        Store(VariableType::INT64(), glue, frame,
            IntPtr(AsmInterpretedFrame::GetEnvOffset(GetEnvironment()->IsArch32Bit())), value);
    }

    void CheckExceptionWithVar(GateRef acc, GateRef res)
    {
        auto env = GetEnvironment();
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(res), &isException, &notException);
        Bind(&isException);
        {
            Return();
        }
        Bind(&notException);
        {
            DEFVARIABLE(varAcc, VariableType::JS_ANY(), acc);
            varAcc = res;
            Return();
        }
    }

    void CheckException(GateRef acc, GateRef res)
    {
        auto env = GetEnvironment();
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(res), &isException, &notException);
        Bind(&isException);
        {
            (void) acc;
            Return();
        }
        Bind(&notException);
        {
            Return();
        }
    }

    void CheckExceptionWithJump(GateRef acc, GateRef res, Label *jump)
    {
        auto env = GetEnvironment();
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(res), &isException, &notException);
        Bind(&isException);
        {
            Return(acc);
        }
        Bind(&notException);
        {
            Jump(jump);
        }
    }

    void CheckPendingException(GateRef glue, GateRef res, GateRef offset)
    {
        (void)offset;
        auto env = GetEnvironment();
        Label isException(env);
        Label notException(env);
        Branch(HasPendingException(glue), &isException, &notException);
        Bind(&isException);
        {
            Return();
        }
        Bind(&notException);
        {
            (void)res;
            Return();
        }
    }

    template<typename... Args>
    void DispatchBase(GateRef target, GateRef glue, Args... args)
    {
        GetEnvironment()->GetBuilder()->CallBCHandler(glue, target, {glue, args...});
    }

    inline void DispatchLast(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                             GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter)
    {
        GateRef target = PtrMul(IntPtr(BytecodeStubCSigns::ID_ExceptionHandler), IntPtrSize());
        DispatchBase(target, glue, sp, pc, constpool, profileTypeInfo, acc, hotnessCounter);
        Return();
    }

    void Dispatch(GateRef glue, GateRef sp, GateRef pc, GateRef constpool, GateRef profileTypeInfo,
                  GateRef acc, GateRef hotnessCounter, GateRef format)
    {
        GateRef newPc = PtrAdd(pc, format);
        GateRef opcode = Load(VariableType::INT8(), newPc);
        GateRef target = PtrMul(ZExtInt32ToPtr(ZExtInt8ToInt32(opcode)), IntPtrSize());
        DispatchBase(target, glue, sp, newPc, constpool, profileTypeInfo, acc, hotnessCounter);
        Return();
    }

    inline GateRef GetFunctionFromFrame(GateRef frame)
    {
        return Load(VariableType::JS_POINTER(), frame,
            IntPtr(AsmInterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetEnvFromFrame(GateRef frame)
    {
        return Load(VariableType::JS_POINTER(), frame,
            IntPtr(AsmInterpretedFrame::GetEnvOffset(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetAccFromFrame(GateRef frame)
    {
        return Load(VariableType::JS_ANY(), frame,
            IntPtr(AsmInterpretedFrame::GetAccOffset(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetConstpoolFromMethod(GateRef method)
    {
        return Load(VariableType::JS_POINTER(), method, IntPtr(Method::CONSTANT_POOL_OFFSET));
    }

    GateRef GetProfileTypeInfoFromFunction(GateRef function);

    inline GateRef GetHotnessCounterFromMethod(GateRef method)
    {
        GateRef x = Load(VariableType::INT16(), method, IntPtr(Method::LITERAL_INFO_OFFSET));
        return GetEnvironment()->GetBuilder()->SExtInt1ToInt32(x);
    }

    GateRef GetModuleFromFunction(GateRef function);

    GateRef GetHomeObjectFromFunction(GateRef function);

    inline GateRef GetModule(GateRef sp)
    {
        GateRef currentFunc = GetFunctionFromFrame(GetFrame(sp));
        return GetModuleFromFunction(currentFunc);
    }

    inline GateRef GetCurrentFrame(GateRef glue)
    {
        return GetLastLeaveFrame(glue);
    }

    inline GateRef GetFrame(GateRef CurrentSp)
    {
        return PtrSub(CurrentSp, IntPtr(AsmInterpretedFrame::GetSize(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetPcFromFrame(GateRef frame)
    {
        return Load(VariableType::NATIVE_POINTER(), frame,
            IntPtr(AsmInterpretedFrame::GetPcOffset(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetCallSizeFromFrame(GateRef frame)
    {
        return Load(VariableType::NATIVE_POINTER(), frame,
            IntPtr(AsmInterpretedFrame::GetCallSizeOffset(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetThisFromFrame(GateRef frame)
    {
        return Load(VariableType::JS_POINTER(), frame,
            IntPtr(AsmInterpretedFrame::GetThisOffset(GetEnvironment()->IsArch32Bit())));
    }

    GateRef GetNewTarget(GateRef sp)
    {
        GateRef function = Load(VariableType::JS_POINTER(), GetFrame(sp),
            IntPtr(AsmInterpretedFrame::GetFunctionOffset(GetEnvironment()->IsArch32Bit())));
        GateRef method = GetMethodFromFunction(function);
        GateRef callField = GetCallFieldFromMethod(method);
        // ASSERT: callField has "extra" bit.
        GateRef numVregs =
            TruncInt64ToInt32(Int64And(Int64LSR(callField, Int64(MethodLiteral::NumVregsBits::START_BIT)),
                                       Int64((1LLU << MethodLiteral::NumVregsBits::SIZE) - 1)));
        GateRef haveFunc = ZExtInt1ToInt32(Int64NotEqual(Int64And(Int64LSR(callField,
            Int64(MethodLiteral::HaveFuncBit::START_BIT)),
            Int64((1LLU << MethodLiteral::HaveFuncBit::SIZE) - 1)), Int64(0)));
        GateRef idx = ZExtInt32ToPtr(Int32Add(numVregs, haveFunc));
        return Load(VariableType::JS_ANY(), sp, PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), idx));
    }

    GateRef GetStartIdxAndNumArgs(GateRef sp, GateRef restIdx)
    {
        auto env = GetEnvironment();
        Label subEntry(env);
        env->SubCfgEntry(&subEntry);
        DEFVARIABLE(numArgs, VariableType::INT32(), Int32(0));
        GateRef state = PtrSub(sp, IntPtr(AsmInterpretedFrame::GetSize(env->IsArch32Bit())));
        GateRef function = GetFunctionFromFrame(state);
        GateRef method = GetMethodFromJSFunctionOrProxy(function);
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
            numArgs = GetInt32OfTInt(Load(VariableType::JS_ANY(), fp,
                                          IntPtr(-static_cast<int64_t>(sizeof(JSTaggedType)))));
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

    inline void SetVregValue(GateRef glue, GateRef sp, GateRef idx, GateRef val)
    {
        Store(VariableType::INT64(), glue, sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx), val);
    }

    inline GateRef GetVregValue(GateRef sp, GateRef idx)
    {
        return Load(VariableType::JS_ANY(), sp, PtrMul(IntPtr(sizeof(JSTaggedType)), idx));
    }

    GateRef GetResumeModeFromGeneratorObject(GateRef obj);
    GateRef GetResumeModeFromAsyncGeneratorObject(GateRef obj);
    GateRef GetLastLeaveFrame(GateRef glue);
}; // class BaselineStubBuilder
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUB_BUILDER_H
