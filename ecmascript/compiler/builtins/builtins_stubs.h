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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_STUB_H
#define ECMASCRIPT_COMPILER_BUILTINS_STUB_H

#include "ecmascript/base/config.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"

namespace panda::ecmascript::kungfu {
class BuiltinsStubBuilder : public StubBuilder {
public:
    explicit BuiltinsStubBuilder(StubBuilder *parent)
        :StubBuilder(parent) {}
    BuiltinsStubBuilder(CallSignature *callSignature, Environment *env)
        : StubBuilder(callSignature, env) {}
    ~BuiltinsStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsStubBuilder);
    virtual void GenerateCircuit() override = 0;

    inline GateRef GetGlue(GateRef info)
    {
        return Load(VariableType::NATIVE_POINTER(), info,
            IntPtr(EcmaRuntimeCallInfo::GetThreadOffset(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetNumArgs(GateRef info)
    {
        return Load(VariableType::INT64(), info,
            IntPtr(EcmaRuntimeCallInfo::GetNumArgsOffset(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetFunction(GateRef info)
    {
        return Load(VariableType::JS_ANY(), info,
            IntPtr(EcmaRuntimeCallInfo::GetStackArgsOffset(GetEnvironment()->IsArch32Bit())));
    }

    inline GateRef GetNewTarget(GateRef info)
    {
        GateRef newTargetOffset = IntPtr(EcmaRuntimeCallInfo::GetNewTargetOffset(GetEnvironment()->IsArch32Bit()));
        return Load(VariableType::JS_ANY(), info, newTargetOffset);
    }

    inline GateRef GetThis(GateRef info)
    {
        GateRef thisOffset = IntPtr(EcmaRuntimeCallInfo::GetThisOffset(GetEnvironment()->IsArch32Bit()));
        return Load(VariableType::JS_ANY(), info, thisOffset);
    }

    inline GateRef GetCallArg0(GateRef numArg)
    {
        auto env = GetEnvironment();
        Label subentry(env);
        env->SubCfgEntry(&subentry);
        DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
        Label isValid(env);
        Label exit(env);
        Branch(Int32GreaterThan(TruncPtrToInt32(numArg), Int32(0)), &isValid, &exit);
        Bind(&isValid);
        {
            result = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG0_OR_ARGV));
            Jump(&exit);
        }
        Bind(&exit);
        auto res = *result;
        env->SubCfgExit();
        return res;
    }

    inline GateRef GetCallArg1(GateRef numArg)
    {
        auto env = GetEnvironment();
        Label subentry(env);
        env->SubCfgEntry(&subentry);
        DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
        Label isValid(env);
        Label exit(env);
        Branch(Int32GreaterThan(TruncPtrToInt32(numArg), Int32(1)), &isValid, &exit);
        Bind(&isValid);
        {
            result = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG1));
            Jump(&exit);
        }
        Bind(&exit);
        auto res = *result;
        env->SubCfgExit();
        return res;
    }

    inline GateRef GetCallArg2(GateRef numArg)
    {
        auto env = GetEnvironment();
        Label subentry(env);
        env->SubCfgEntry(&subentry);
        DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
        Label isValid(env);
        Label exit(env);
        // 2: 2 args
        Branch(Int32GreaterThan(TruncPtrToInt32(numArg), Int32(2)), &isValid, &exit);
        Bind(&isValid);
        {
            result = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG2));
            Jump(&exit);
        }
        Bind(&exit);
        auto res = *result;
        env->SubCfgExit();
        return res;
    }

    inline GateRef GetArgv()
    {
        return PtrArgument(static_cast<size_t>(BuiltinsArgs::ARG0_OR_ARGV));
    }

    // not check whether index is valid, if not sure, invoke GetArg
    inline GateRef GetArgNCheck(GateRef index)
    {
        GateRef argv = GetArgv();
        return Load(VariableType::JS_ANY(), argv, PtrMul(index, IntPtr(JSTaggedValue::TaggedTypeSize())));
    }

    GateRef GetArg(GateRef numArgs, GateRef index);

    GateRef CallSlowPath(GateRef nativeCode, GateRef glue, GateRef thisValue, GateRef numArgs, GateRef func,
                         GateRef newTarget, const char* comment = nullptr);

    inline GateRef IsNumberYearMonthDay(GateRef year, GateRef month, GateRef day)
    {
        GateRef condition = BoolAnd(TaggedIsNumber(year), TaggedIsNumber(month));
        return BoolAnd(condition, TaggedIsNumber(day));
    }
};

#define DECLARE_BUILTINS_STUB_CLASS(name)                                                           \
    class name##StubBuilder : public BuiltinsStubBuilder {                                          \
    public:                                                                                         \
        name##StubBuilder(CallSignature *callSignature, Environment *env)                           \
            : BuiltinsStubBuilder(callSignature, env) {}                                            \
        ~name##StubBuilder() = default;                                                             \
        NO_MOVE_SEMANTIC(name##StubBuilder);                                                        \
        NO_COPY_SEMANTIC(name##StubBuilder);                                                        \
        void GenerateCircuit() override;                                                            \
                                                                                                    \
    private:                                                                                        \
        void GenerateCircuitImpl(GateRef glue, GateRef nativeCode, GateRef func, GateRef newTarget, \
                                 GateRef thisValue, GateRef numArgs);                               \
    };
    BUILTINS_STUB_LIST(DECLARE_BUILTINS_STUB_CLASS)
#undef DECLARE_BUILTINS_STUB_CLASS
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_STUB_H
