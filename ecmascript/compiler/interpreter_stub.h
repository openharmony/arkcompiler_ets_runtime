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

#ifndef ECMASCRIPT_COMPILER_INTERPRETER_STUB_H
#define ECMASCRIPT_COMPILER_INTERPRETER_STUB_H

#include "ecmascript/base/config.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/profiler_operation.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"

namespace panda::ecmascript::kungfu {
class StringIdInfo {
public:
    enum class Offset : uint8_t {
        BYTE_0,
        BYTE_1,
        BYTE_2,
        INVALID,
    };
    enum class Length : uint8_t {
        BITS_16,
        BITS_32,
        INVALID,
    };
    enum class StringIdType : uint8_t {
        STRING_ID,
        STRING_ID_INFO,
    };

    StringIdInfo() : constpool(0), pc(0), offset(Offset::INVALID),
          length(Length::INVALID), stringId(0), stringIdType(StringIdType::STRING_ID_INFO) {}

    StringIdInfo(GateRef inputConstpool, GateRef inputPc, Offset inputOffset, Length inputLength)
        : constpool(inputConstpool), pc(inputPc), offset(inputOffset),
          length(inputLength), stringId(0), stringIdType(StringIdType::STRING_ID_INFO) {}

    explicit StringIdInfo(GateRef inputConstpool, GateRef inputStringId)
        : constpool(inputConstpool), pc(0), offset(Offset::INVALID),
          length(Length::INVALID), stringId(inputStringId), stringIdType(StringIdType::STRING_ID) {}

    GateRef GetConstantPool() const
    {
        return constpool;
    }

    GateRef GetPc() const
    {
        return pc;
    }

    Offset GetOffset() const
    {
        return offset;
    }

    Length GetLength() const
    {
        return length;
    }

    GateRef GetStringId() const
    {
        return stringId;
    }

    StringIdType GetStringIdType() const
    {
        return stringIdType;
    }

    bool IsValid() const
    {
        if (stringIdType == StringIdType::STRING_ID_INFO) {
            return (constpool != 0) && (pc != 0) && (offset != Offset::INVALID) && (length != Length::INVALID);
        }
        return stringId != 0;
    }

private:
    GateRef constpool { 0 };
    GateRef pc { 0 };
    Offset offset { Offset::INVALID };
    Length length { Length::INVALID };
    GateRef stringId { 0 };
    StringIdType stringIdType { StringIdType::STRING_ID_INFO };
};

class InterpreterStubBuilder : public StubBuilder {
public:
    InterpreterStubBuilder(CallSignature *callSignature, Environment *env)
        : StubBuilder(callSignature, env) {}
    ~InterpreterStubBuilder() override = default;
    NO_MOVE_SEMANTIC(InterpreterStubBuilder);
    NO_COPY_SEMANTIC(InterpreterStubBuilder);
    virtual void GenerateCircuit() override = 0;

    inline void SetVregValue(GateRef glue, GateRef sp, GateRef idx, GateRef val);
    inline GateRef GetVregValue(GateRef glue, GateRef sp, GateRef idx);
    inline GateRef GetVregValueFromArray(GateRef glue, GateRef array, GateRef idx);
    inline GateRef ReadInst4_0(GateRef pc);
    inline GateRef ReadInst4_1(GateRef pc);
    inline GateRef ReadInst4_2(GateRef pc);
    inline GateRef ReadInst4_3(GateRef pc);
    inline GateRef ReadInst8_0(GateRef pc);
    inline GateRef ReadInst8_1(GateRef pc);
    inline GateRef ReadInst8_2(GateRef pc);
    inline GateRef ReadInst8_3(GateRef pc);
    inline GateRef ReadInst8_4(GateRef pc);
    inline GateRef ReadInst8_5(GateRef pc);
    inline GateRef ReadInst8_6(GateRef pc);
    inline GateRef ReadInst8_7(GateRef pc);
    inline GateRef ReadInst8_8(GateRef pc);
    inline GateRef ReadInst8_9(GateRef pc);
    inline GateRef ReadInst16_0(GateRef pc);
    inline GateRef ReadInst16_1(GateRef pc);
    inline GateRef ReadInst16_2(GateRef pc);
    inline GateRef ReadInst16_3(GateRef pc);
    inline GateRef ReadInst16_4(GateRef pc);
    inline GateRef ReadInst16_5(GateRef pc);
    inline GateRef ReadInst16_6(GateRef pc);
    inline GateRef ReadInst16_7(GateRef pc);
    inline GateRef ReadInstSigned8_0(GateRef pc);
    inline GateRef ReadInstSigned16_0(GateRef pc);
    inline GateRef ReadInstSigned32_0(GateRef pc);
    inline GateRef ReadInst32_0(GateRef pc);
    inline GateRef ReadInst32_1(GateRef pc);
    inline GateRef ReadInst32_2(GateRef pc);
    inline GateRef ReadInst64_0(GateRef pc);

    inline GateRef GetFrame(GateRef frame);
    inline GateRef GetCurrentSpFrame(GateRef glue);
    inline GateRef GetLastLeaveFrame(GateRef glue);
    inline GateRef GetCurrentFrame(GateRef glue);
    inline GateRef GetPcFromFrame(GateRef frame);
    inline GateRef GetCallSizeFromFrame(GateRef frame);
    inline GateRef GetFunctionFromFrame(GateRef glue, GateRef frame);
    inline GateRef GetNewTarget(GateRef glue, GateRef sp);
    inline GateRef GetThisFromFrame(GateRef glue, GateRef frame);
    inline GateRef GetAccFromFrame(GateRef glue, GateRef frame);
    inline GateRef GetEnvFromFrame(GateRef glue, GateRef frame);
    inline GateRef GetEnvFromFunction(GateRef glue, GateRef frame);
    inline GateRef GetConstpoolFromMethod(GateRef glue, GateRef function);
    inline GateRef GetModule(GateRef glue, GateRef sp);
    inline GateRef GetProfileTypeInfoFromFunction(GateRef glue, GateRef function);
    inline GateRef GetModuleFromFunction(GateRef glue, GateRef function);
    inline GateRef GetSendableEnvFromModule(GateRef glue, GateRef module);
    inline GateRef GetHomeObjectFromFunction(GateRef glue, GateRef function);
    inline GateRef GetResumeModeFromGeneratorObject(GateRef obj);
    inline GateRef GetResumeModeFromAsyncGeneratorObject(GateRef obj);
    inline GateRef GetHotnessCounterFromMethod(GateRef method);

    inline void SetCurrentSpFrame(GateRef glue, GateRef sp);
    inline void SetLastLeaveFrame(GateRef glue, GateRef sp);
    inline void SetPcToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetCallSizeToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetFunctionToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetAccToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetEnvToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value);
    inline void SetFrameState(GateRef glue, GateRef sp, GateRef function, GateRef acc,
                              GateRef env, GateRef pc, GateRef prev, GateRef type);

    inline void UpdateProfileTypeInfoCellToFunction(GateRef glue, GateRef function,
                                                    GateRef profileTypeInfo, GateRef slotId);

    inline void CheckException(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
		               GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter,
			       GateRef res, GateRef offset);
    inline void CheckPendingException(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
		                      GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter,
			              GateRef res, GateRef offset);
    inline void CheckExceptionWithJump(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
		                       GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter,
			               GateRef res, Label *jump);
    inline void CheckExceptionWithVar(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
		                      GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter,
			              GateRef res, GateRef offset);

    inline GateRef CheckStackOverflow(GateRef glue, GateRef sp);
    inline GateRef PushArg(GateRef glue, GateRef sp, GateRef value);
    inline GateRef PushUndefined(GateRef glue, GateRef sp, GateRef num);
    inline GateRef PushRange(GateRef glue, GateRef sp, GateRef array, GateRef startIndex, GateRef endIndex);
    inline GateRef GetStartIdxAndNumArgs(GateRef glue, GateRef sp, GateRef restIdx);
    inline void Dispatch(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                         GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter, GateRef format);
    inline void DispatchWithId(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                               GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter, GateRef index);
    inline void DispatchLast(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                             GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter);
    inline void DispatchDebugger(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                                 GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter);
    inline void DispatchDebuggerLast(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,
                                     GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter);

    template <bool needPrint>
    void DebugPrintInstruction();
private:
    template<typename... Args>
    void DispatchBase(GateRef target, GateRef glue, Args... args);
};

class InterpreterToolsStubBuilder : private InterpreterStubBuilder {
public:
    InterpreterToolsStubBuilder(CallSignature *callSignature, Environment *env)
        : InterpreterStubBuilder(callSignature, env) {}
    ~InterpreterToolsStubBuilder() override = default;
    NO_MOVE_SEMANTIC(InterpreterToolsStubBuilder);
    NO_COPY_SEMANTIC(InterpreterToolsStubBuilder);
    void GenerateCircuit() override {}

    inline GateRef GetStringId(const StringIdInfo &info);
};

#define DECLARE_HANDLE_STUB_CLASS(name)                                                         \
    class name##StubBuilder : public InterpreterStubBuilder {                                   \
    public:                                                                                     \
        explicit name##StubBuilder(CallSignature *callSignature, Environment *env)              \
            : InterpreterStubBuilder(callSignature, env)                                        \
        {                                                                                       \
            env->GetCircuit()->SetFrameType(FrameType::ASM_INTERPRETER_FRAME);                  \
        }                                                                                       \
        ~name##StubBuilder() = default;                                                         \
        NO_MOVE_SEMANTIC(name##StubBuilder);                                                    \
        NO_COPY_SEMANTIC(name##StubBuilder);                                                    \
        void GenerateCircuit() override;                                                        \
                                                                                                \
    protected:                                                                                  \
        void GenerateCircuitImpl(GateRef glue, GateRef sp, GateRef pc, GateRef constpool,       \
                                 GateRef profileTypeInfo, GateRef acc, GateRef hotnessCounter,  \
                                 ProfileOperation callback);                                    \
    };
    INTERPRETER_BC_STUB_LIST(DECLARE_HANDLE_STUB_CLASS)
    ASM_INTERPRETER_BC_HELPER_STUB_LIST(DECLARE_HANDLE_STUB_CLASS)

#define DECLARE_HANDLE_PROFILE_STUB_CLASS(name, base, ...)                         \
    class name##StubBuilder : public base##StubBuilder {                           \
    public:                                                                        \
        explicit name##StubBuilder(CallSignature *callSignature, Environment *env) \
            : base##StubBuilder(callSignature, env)                                \
        {                                                                          \
        }                                                                          \
        ~name##StubBuilder() = default;                                            \
        NO_MOVE_SEMANTIC(name##StubBuilder);                                       \
        NO_COPY_SEMANTIC(name##StubBuilder);                                       \
        void GenerateCircuit() override;                                           \
    };
    ASM_INTERPRETER_BC_PROFILER_STUB_LIST(DECLARE_HANDLE_PROFILE_STUB_CLASS)
#undef DECLARE_HANDLE_PROFILE_STUB_CLASS
#define DECLARE_HANDLE_JIT_PROFILE_STUB_CLASS(name, base, ...)                     \
    class name##StubBuilder : public base##StubBuilder {                           \
    public:                                                                        \
        explicit name##StubBuilder(CallSignature *callSignature, Environment *env) \
            : base##StubBuilder(callSignature, env)                                \
        {                                                                          \
        }                                                                          \
        ~name##StubBuilder() = default;                                            \
        NO_MOVE_SEMANTIC(name##StubBuilder);                                       \
        NO_COPY_SEMANTIC(name##StubBuilder);                                       \
        void GenerateCircuit() override;                                           \
    };
    ASM_INTERPRETER_BC_JIT_PROFILER_STUB_LIST(DECLARE_HANDLE_JIT_PROFILE_STUB_CLASS)
#undef DECLARE_HANDLE_PROFILE_STUB_CLASS
#undef DECLARE_HANDLE_STUB_CLASS
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_INTERPRETER_STUB_H
