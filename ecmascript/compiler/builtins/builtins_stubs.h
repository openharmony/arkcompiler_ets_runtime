/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_STUB_H
#define ECMASCRIPT_COMPILER_BUILTINS_STUB_H

#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::kungfu {
class BuiltinsStubBuilder : public StubBuilder {
public:
    BuiltinsStubBuilder(StubBuilder *parent, GateRef globalEnv)
        : StubBuilder(parent, globalEnv) {}
    BuiltinsStubBuilder(CallSignature *callSignature, Environment *env, GateRef globalEnv)
        : StubBuilder(callSignature, env, globalEnv) {}
    BuiltinsStubBuilder(Environment* env, GateRef globalEnv)
        : StubBuilder(env, globalEnv) {}
    ~BuiltinsStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsStubBuilder);
    virtual void GenerateCircuit() override = 0;

    inline GateRef GetGlue(GateRef info)
    {
        return LoadPrimitive(VariableType::NATIVE_POINTER(), info,
            IntPtr(EcmaRuntimeCallInfo::GetThreadOffset(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetNumArgs(GateRef info)
    {
        return LoadPrimitive(VariableType::INT64(), info,
            IntPtr(EcmaRuntimeCallInfo::GetNumArgsOffset(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetFunction(GateRef info)
    {
        return Load(VariableType::JS_ANY(), GetGlue(info), info,
            IntPtr(EcmaRuntimeCallInfo::GetStackArgsOffset(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetNewTarget(GateRef info)
    {
        GateRef newTargetOffset = IntPtr(EcmaRuntimeCallInfo::GetNewTargetOffset(GetEnvironment()->IsArch32Bit()));
        return Load(VariableType::JS_ANY(), GetGlue(info), info, newTargetOffset);
    }

    inline GateRef GetThis(GateRef info)
    {
        GateRef thisOffset = IntPtr(EcmaRuntimeCallInfo::GetThisOffset(GetEnvironment()->IsArch32Bit()));
        return Load(VariableType::JS_ANY(), GetGlue(info), info, thisOffset);
    }

    GateRef GetCallArg0(GateRef numArg);
    GateRef GetCallArg1(GateRef numArg);
    GateRef GetCallArg2(GateRef numArg);
    GateRef GetGlobalEnvFromFunction(GateRef glue, GateRef func);

    inline GateRef GetArgv()
    {
        return PtrArgument(static_cast<size_t>(BuiltinsArgs::ARG0_OR_ARGV));
    }

    GateRef GetArgFromArgv(GateRef glue, GateRef index, GateRef numArgs = Gate::InvalidGateRef, bool needCheck = false);

    GateRef CallSlowPath(GateRef nativeCode, GateRef glue, GateRef thisValue, GateRef numArgs, GateRef func,
                         GateRef newTarget);

    inline GateRef IsNumberYearMonthDay(GateRef year, GateRef month, GateRef day)
    {
        GateRef condition = BitAnd(TaggedIsNumber(year), TaggedIsNumber(month));
        return BitAnd(condition, TaggedIsNumber(day));
    }
};

#define DECLARE_BUILTINS_STUB_CLASS(name)                                                           \
    class name##StubBuilder : public BuiltinsStubBuilder {                                          \
    public:                                                                                         \
        name##StubBuilder(CallSignature *callSignature, Environment *env, GateRef globalEnv)      \
            : BuiltinsStubBuilder(callSignature, env, globalEnv) {}                               \
        ~name##StubBuilder() = default;                                                             \
        NO_MOVE_SEMANTIC(name##StubBuilder);                                                        \
        NO_COPY_SEMANTIC(name##StubBuilder);                                                        \
        void GenerateCircuit() override;                                                            \
                                                                                                    \
    private:                                                                                        \
        void GenerateCircuitImpl(GateRef glue, GateRef nativeCode, GateRef func, GateRef newTarget, \
                                 GateRef thisValue, GateRef numArgs);                               \
    };


#define DECLARE_BUILTINS_STUB_CLASS_DYN(name, type, ...)                                            \
    class type##name##StubBuilder : public BuiltinsStubBuilder {                                    \
    public:                                                                                         \
        type##name##StubBuilder(CallSignature *callSignature, Environment *env, GateRef globalEnv)\
            : BuiltinsStubBuilder(callSignature, env, globalEnv) {}                               \
        ~type##name##StubBuilder() = default;                                                       \
        NO_MOVE_SEMANTIC(type##name##StubBuilder);                                                  \
        NO_COPY_SEMANTIC(type##name##StubBuilder);                                                  \
        void GenerateCircuit() override;                                                            \
                                                                                                    \
    private:                                                                                        \
        void GenerateCircuitImpl(GateRef glue, GateRef nativeCode, GateRef func, GateRef newTarget, \
                                 GateRef thisValue, GateRef numArgs);                               \
    };
    BUILTINS_STUB_LIST(DECLARE_BUILTINS_STUB_CLASS, DECLARE_BUILTINS_STUB_CLASS_DYN, DECLARE_BUILTINS_STUB_CLASS)

#define DECLARE_BUILTINS_STW_COPY_STUB_CLASS(name, base)                                            \
    class name##StubBuilder : public base##StubBuilder {                                            \
    public:                                                                                         \
        name##StubBuilder(CallSignature *callSignature, Environment *env, GateRef globalEnv)        \
            : base##StubBuilder(callSignature, env, globalEnv) {}                                   \
        ~name##StubBuilder() = default;                                                             \
        NO_MOVE_SEMANTIC(name##StubBuilder);                                                        \
        NO_COPY_SEMANTIC(name##StubBuilder);                                                        \
    };

#define DECLARE_BUILTINS_STW_COPY_STUB_CLASS_SECOND(base)                                           \
    DECLARE_BUILTINS_STW_COPY_STUB_CLASS(base##StwCopy, base)

#define DECLARE_BUILTINS_STW_COPY_STUB_CLASS_DYN(name, base, type)                                  \
    class type##name##StubBuilder : public type##base##StubBuilder {                                \
    public:                                                                                         \
        type##name##StubBuilder(CallSignature *callSignature, Environment *env, GateRef globalEnv)  \
            : type##base##StubBuilder(callSignature, env, globalEnv) {}                             \
        ~type##name##StubBuilder() = default;                                                       \
        NO_MOVE_SEMANTIC(type##name##StubBuilder);                                                  \
        NO_COPY_SEMANTIC(type##name##StubBuilder);                                                  \
    };

#define DECLARE_BUILTINS_STW_COPY_STUB_CLASS_DYN_SECOND(base, type, ...)                            \
    DECLARE_BUILTINS_STW_COPY_STUB_CLASS_DYN(base##StwCopy, base, type)

    BUILTINS_STW_COPY_STUB_LIST(DECLARE_BUILTINS_STW_COPY_STUB_CLASS_SECOND, \
        DECLARE_BUILTINS_STW_COPY_STUB_CLASS_DYN_SECOND, DECLARE_BUILTINS_STW_COPY_STUB_CLASS_SECOND)

#undef DECLARE_BUILTINS_STW_COPY_STUB_CLASS_DYN_SECOND
#undef DECLARE_BUILTINS_STW_COPY_STUB_CLASS_DYN
#undef DECLARE_BUILTINS_STW_COPY_STUB_CLASS_SECOND
#undef DECLARE_BUILTINS_STW_COPY_STUB_CLASS
#undef DECLARE_BUILTINS_STUB_CLASS_DYN
#undef DECLARE_BUILTINS_STUB_CLASS
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_STUB_H
