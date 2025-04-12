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

#ifndef ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUBS_H
#define ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUBS_H

#include "ecmascript/compiler/baseline/baseline_compiler_builtins.h"
#include "ecmascript/compiler/baseline/baseline_stub_csigns.h"
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/base/config.h"
#include "ecmascript/compiler/profiler_operation.h"

namespace panda::ecmascript::kungfu {

class BaselineStubBuilder : public StubBuilder {
public:
    BaselineStubBuilder(CallSignature *callSignature, Environment *env)
        : StubBuilder(callSignature, env) {}
    ~BaselineStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BaselineStubBuilder);
    NO_COPY_SEMANTIC(BaselineStubBuilder);
    virtual void GenerateCircuit() override = 0;

    inline void SetEnvToFrame(GateRef glue, GateRef frame, GateRef value);
    inline void CheckExceptionWithVar(GateRef glue, GateRef sp, GateRef res, GateRef acc);
    inline void CheckException(GateRef glue, GateRef sp, GateRef res);
    inline void CheckExceptionReturn(GateRef glue, GateRef sp, GateRef res);
    inline void CheckExceptionWithJump(GateRef glue, GateRef sp, GateRef res, GateRef acc, Label *jump);
    inline void CheckExceptionWithJumpAndReturn(GateRef glue, GateRef sp, GateRef res, GateRef acc, Label *jump);
    inline void CheckPendingException(GateRef glue, GateRef sp, GateRef res, GateRef acc);
    inline void DispatchLast(GateRef glue, GateRef sp, GateRef acc);
    inline GateRef GetFunctionFromFrame(GateRef glue, GateRef frame);
    inline GateRef GetEnvFromFrame(GateRef glue, GateRef frame);
    inline GateRef GetAccFromFrame(GateRef glue, GateRef frame);
    inline GateRef GetConstpoolFromMethod(GateRef glue, GateRef method);
    inline GateRef GetProfileTypeInfoFromFunction(GateRef glue, GateRef function);
    inline GateRef GetHotnessCounterFromMethod(GateRef method);
    inline GateRef GetModuleFromFunction(GateRef glue, GateRef function);
    inline GateRef GetHomeObjectFromFunction(GateRef glue, GateRef function);
    inline GateRef GetModule(GateRef glue, GateRef sp);
    inline GateRef GetCurrentFrame(GateRef glue);
    inline GateRef GetFrame(GateRef CurrentSp);
    inline GateRef GetPcFromFrame(GateRef frame);
    inline GateRef GetCallSizeFromFrame(GateRef frame);
    inline GateRef GetThisFromFrame(GateRef glue, GateRef frame);
    inline GateRef GetNewTarget(GateRef glue, GateRef sp);
    inline GateRef GetStartIdxAndNumArgs(GateRef glue, GateRef sp, GateRef restIdx);
    inline void SetVregValue(GateRef glue, GateRef sp, GateRef idx, GateRef val);
    inline GateRef GetVregValue(GateRef glue, GateRef sp, GateRef idx);
    inline GateRef GetResumeModeFromGeneratorObject(GateRef obj);
    inline GateRef GetResumeModeFromAsyncGeneratorObject(GateRef obj);
    inline GateRef GetLastLeaveFrame(GateRef glue);
    inline GateRef CallBaselineStub(GateRef glue, int index, const std::initializer_list<GateRef>& args);
    inline void UpdateProfileTypeInfoCellToFunction(GateRef glue, GateRef function,
                                                    GateRef profileTypeInfo, GateRef slotId);
};

#define DECLARE_STUB_CLASS(name)                                                   \
    class name##StubBuilder : public BaselineStubBuilder {                         \
    public:                                                                        \
        name##StubBuilder(CallSignature *callSignature, Environment *env)          \
            : BaselineStubBuilder(callSignature, env)                              \
        {                                                                          \
            env->GetCircuit()->SetFrameType(FrameType::BASELINE_BUILTIN_FRAME);    \
        }                                                                          \
        ~name##StubBuilder() = default;                                            \
        NO_MOVE_SEMANTIC(name##StubBuilder);                                       \
        NO_COPY_SEMANTIC(name##StubBuilder);                                       \
        void GenerateCircuit() override;                                           \
    };
    BASELINE_COMPILER_BUILTIN_LIST(DECLARE_STUB_CLASS)
#undef DECLARE_STUB_CLASS
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BASELINE_BASELINE_STUBS_H
