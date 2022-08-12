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
    BuiltinsStubBuilder(CallSignature *callSignature, Environment *env)
        : StubBuilder(callSignature, env) {}
    ~BuiltinsStubBuilder() = default;
    NO_MOVE_SEMANTIC(BuiltinsStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsStubBuilder);
    virtual void GenerateCircuit() = 0;

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

    inline GateRef GetCallArg(GateRef argv, GateRef index)
    {
        return Load(VariableType::JS_ANY(), argv, PtrMul(index, IntPtr(sizeof(JSTaggedType))));
    }

    inline GateRef StringAt(GateRef obj, GateRef index);

    GateRef StringIndexOf(GateRef lhsData, bool lhsIsUtf8, GateRef rhsData, bool rhsIsUtf8,
                          GateRef pos, GateRef max, GateRef rhsCount);
    
    GateRef StringIndexOf(GateRef lhs, GateRef rhs, GateRef pos);

    GateRef GetUtf16Date(GateRef stringData, GateRef index)
    {
        return ZExtInt16ToInt32(Load(VariableType::INT16(), PtrAdd(stringData,
            PtrMul(ChangeInt32ToIntPtr(index), IntPtr(sizeof(uint16_t))))));
    }

    GateRef GetUtf8Date(GateRef stringData, GateRef index)
    {
        return ZExtInt8ToInt32(Load(VariableType::INT8(), PtrAdd(stringData,
            PtrMul(ChangeInt32ToIntPtr(index), IntPtr(sizeof(uint8_t))))));
    }
};

#define DECLARE_BUILTINS_STUB_CLASS(name)                                                           \
    class name##StubBuilder : public BuiltinsStubBuilder {                                          \
    public:                                                                                         \
        explicit name##StubBuilder(CallSignature *callSignature, Environment *env)                  \
            : BuiltinsStubBuilder(callSignature, env) {}                                            \
        ~name##StubBuilder() = default;                                                             \
        NO_MOVE_SEMANTIC(name##StubBuilder);                                                        \
        NO_COPY_SEMANTIC(name##StubBuilder);                                                        \
        void GenerateCircuit() override;                                                            \
                                                                                                    \
    private:                                                                                        \
        void GenerateCircuitImpl(GateRef glue, GateRef nativeCode, GateRef func, GateRef thisValue, \
                                 GateRef numArgs, GateRef argv);                                    \
    };
    BUILTINS_STUB_LIST(DECLARE_BUILTINS_STUB_CLASS)
#undef DECLARE_BUILTINS_STUB_CLASS
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_STUB_H
