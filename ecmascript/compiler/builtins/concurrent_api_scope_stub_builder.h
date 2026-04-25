/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CONCURRENT_API_SCOPE_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_CONCURRENT_API_SCOPE_STUB_BUILDER_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/shared_objects/concurrent_api_scope.h"
#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::kungfu {
template<typename Container, ModType modType = ModType::READ>
class ConcurrentApiScopeStubBuilder : public StubBuilder {
public:
    explicit ConcurrentApiScopeStubBuilder(StubBuilder *parent, GateRef glue, GateRef globalEnv,
        GateRef objHandle, SCheckMode mode = SCheckMode::CHECK)
        : StubBuilder(parent, globalEnv), glue_(glue), sharedObj_(objHandle), checkMode_(mode)
    {
        if (checkMode_ == SCheckMode::SKIP) {
            return;
        }
        if constexpr (modType == ModType::READ) {
            CanRead();
        } else {
            CanWrite();
        }
    }
    ~ConcurrentApiScopeStubBuilder() override
    {
        if (checkMode_ == SCheckMode::SKIP) {
            return;
        }
        if constexpr (modType == ModType::READ) {
            ReadDone();
        } else {
            WriteDone();
        }
    }
    NO_MOVE_SEMANTIC(ConcurrentApiScopeStubBuilder);
    NO_COPY_SEMANTIC(ConcurrentApiScopeStubBuilder);
    void GenerateCircuit() override {}
    static constexpr uint32_t WRITE_MOD_MASK = 1 << 31;

private:
    inline void ThrowScopeError()
    {
        int errorCode = containers::ErrorFlag::CONCURRENT_MODIFICATION_ERROR;
        int errorMsg = GET_MESSAGE_STRING_ID(ConcurrentApiScopeError);
        ThrowContainerBusinessError(glue_, errorCode, errorMsg);
    }

    inline GateRef GetModRecord()
    {
        return AtomicLoadAcquireI32(sharedObj_, Int32(Container::MOD_RECORD_OFFSET));
    }

    inline void CanWrite()
    {
        auto env = GetEnvironment();
        Label subentry(env);
        Label exit(env);
        env->SubCfgEntry(&subentry);
        constexpr uint32_t expectedModRecord = 0;
        constexpr uint32_t desiredModRecord = WRITE_MOD_MASK;
        Label throwError(env);
        GateRef oldValue = AtomicCmpXchgI32(sharedObj_, Int32(Container::MOD_RECORD_OFFSET),
            Int32(expectedModRecord), Int32(desiredModRecord));
        BRANCH(Int32Equal(oldValue, Int32(expectedModRecord)), &exit, &throwError);
        Bind(&throwError);
        {
            ThrowScopeError();
            Jump(&exit);
        }
        Bind(&exit);
        env->SubCfgExit();
    }

    inline void WriteDone()
    {
        auto env = GetEnvironment();
        Label subentry(env);
        Label exit(env);
        env->SubCfgEntry(&subentry);
        constexpr uint32_t expectedModRecord = WRITE_MOD_MASK;
        constexpr uint32_t desiredModRecord = 0;
        Label throwError(env);
        GateRef oldValue = AtomicCmpXchgI32(sharedObj_, Int32(Container::MOD_RECORD_OFFSET),
            Int32(expectedModRecord), Int32(desiredModRecord));
        BRANCH(Int32Equal(oldValue, Int32(expectedModRecord)), &exit, &throwError);
        Bind(&throwError);
        {
            ThrowScopeError();
            Jump(&exit);
        }
        Bind(&exit);
        env->SubCfgExit();
    }

    inline void CanRead()
    {
        auto env = GetEnvironment();
        Label subentry(env);
        Label exit(env);
        env->SubCfgEntry(&subentry);
        Label loopHead(env);
        Label loopEnd(env);
        Label tryCAS(env);
        Label throwError(env);

        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            expectModRecord_ = GetModRecord();
            GateRef hasWriter = Int32And(expectModRecord_, Int32(WRITE_MOD_MASK));
            BRANCH(Int32Equal(hasWriter, Int32(0)), &tryCAS, &throwError);
        }
        Bind(&tryCAS);
        {
            desiredModRecord_ = Int32Add(expectModRecord_, Int32(1));
            GateRef oldValue = AtomicCmpXchgI32(sharedObj_, Int32(Container::MOD_RECORD_OFFSET),
                expectModRecord_, desiredModRecord_);
            BRANCH(Int32Equal(oldValue, expectModRecord_), &exit, &loopEnd);
        }
        Bind(&loopEnd);
        {
            LoopEnd(&loopHead);
        }
        Bind(&throwError);
        {
            ThrowScopeError();
            Jump(&exit);
        }
        Bind(&exit);
        env->SubCfgExit();
    }

    inline void ReadDone()
    {
        auto env = GetEnvironment();
        Label subentry(env);
        Label exit(env);
        env->SubCfgEntry(&subentry);
        Label loopHead(env);
        Label loopEnd(env);
        Label updateDesired(env);
        Label checkState(env);
        Label throwError(env);

        GateRef temp = expectModRecord_;
        expectModRecord_ = desiredModRecord_;
        desiredModRecord_ = temp;

        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            GateRef oldValue = AtomicCmpXchgI32(sharedObj_, Int32(Container::MOD_RECORD_OFFSET),
                expectModRecord_, desiredModRecord_);
            BRANCH(Int32Equal(oldValue, expectModRecord_), &exit, &checkState);
        }
        Bind(&checkState);
        {
            expectModRecord_ = GetModRecord();
            GateRef hasWriter = Int32NotEqual(Int32And(expectModRecord_, Int32(WRITE_MOD_MASK)), Int32(0));
            GateRef isZero = Int32Equal(expectModRecord_, Int32(0));
            BRANCH(BitOr(hasWriter, isZero), &throwError, &updateDesired);
        }
        Bind(&updateDesired);
        {
            desiredModRecord_ = Int32Sub(expectModRecord_, Int32(1));
            Jump(&loopEnd);
        }
        Bind(&loopEnd);
        {
            LoopEnd(&loopHead);
        }
        Bind(&throwError);
        {
            ThrowScopeError();
            Jump(&exit);
        }
        Bind(&exit);
        env->SubCfgExit();
    }

    GateRef glue_;
    GateRef sharedObj_;
    SCheckMode checkMode_ = SCheckMode::CHECK;
    GateRef expectModRecord_ = Int32(0);
    GateRef desiredModRecord_ = Int32(0);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CONCURRENT_API_SCOPE_STUB_BUILDER_H
