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

#ifndef ECMASCRIPT_ARKSTEED_PGO_CONTEXT_H
#define ECMASCRIPT_ARKSTEED_PGO_CONTEXT_H

#include <unordered_map>

#include "ecmascript/arksteed/arksteed_access_info_factory.h"
#include "ecmascript/arksteed/arksteed_feedback_reader.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/jit/jit_profiler.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"

namespace panda::ecmascript::arksteed {

class ArkSteedPGOContext {
public:
    ArkSteedPGOContext(JSThread *compilerThread, JitCompilationEnv *env)
        : compilerThread_(compilerThread), env_(env), broker_(compilerThread, env)
    {}

    ArkSteedAccessInfoFactory CreateAccessInfoFactory(
        const panda::ecmascript::kungfu::BytecodeInfo &bytecodeInfo) const
    {
        return ArkSteedAccessInfoFactory(compilerThread_, bytecodeInfo, env_,
                                         const_cast<ArkSteedHeapBroker *>(&broker_));
    }

    ArkSteedHeapBroker::SerializingScope CreateSerializingScope(CString message) const
    {
        return ArkSteedHeapBroker::SerializingScope(&broker_, message);
    }

    ArkSteedHeapBroker *GetBroker()
    {
        return &broker_;
    }

    const ArkSteedHeapBroker *GetBroker() const
    {
        return &broker_;
    }

    void PrepareBytecodeProfiles(Chunk *chunk) const
    {
        if (bytecodeProfilesPrepared_ || env_ == nullptr || chunk == nullptr || !env_->GetJSOptions().IsEnableJITPGO()) {
            return;
        }
        auto profiler = env_->GetPGOProfiler();
        if (profiler == nullptr) {
            bytecodeProfilesPrepared_ = true;
            return;
        }
        JITProfiler *jitProfile = profiler->GetJITProfile();
        if (jitProfile == nullptr) {
            bytecodeProfilesPrepared_ = true;
            return;
        }

        JSHandle<ProfileTypeInfo> profileTypeInfo = env_->GetProfileTypeInfo();
        if (profileTypeInfo.GetAddress() == 0) {
            bytecodeProfilesPrepared_ = true;
            return;
        }

        JSPandaFile *jsPandaFile = env_->GetJSPandaFile();
        MethodLiteral *methodLiteral = env_->GetMethodLiteral();
        if (jsPandaFile == nullptr || methodLiteral == nullptr) {
            bytecodeProfilesPrepared_ = true;
            return;
        }

        ALLOW_DEREF_HANDLE;
        jitProfile->SetCompilationEnv(env_);
        jitProfile->InitChunk(chunk);
        jitProfile->ProfileBytecode(compilerThread_,
                                    profileTypeInfo,
                                    methodLiteral->GetMethodId(),
                                    env_->GetMethodAbcId(),
                                    env_->GetMethodPcStart(),
                                    MethodLiteral::GetCodeSize(jsPandaFile, methodLiteral->GetMethodId()),
                                    jsPandaFile->GetPandaFile()->GetHeader(),
                                    env_->GetJsFunction(),
                                    env_->GetGlobalEnv());
        ResetBinaryOpProfileCache();
        bytecodeProfilesPrepared_ = true;
    }

    pgo::PGOSampleType ReadBinaryOpProfile(uint32_t bcOffset) const
    {
        EnsureBinaryOpProfileCache();
        int32_t key = static_cast<int32_t>(bcOffset);
        auto profile = binaryOpProfileCache_.find(key);
        if (profile == binaryOpProfileCache_.end()) {
            return pgo::PGOSampleType::NoneType();
        }
        return profile->second;
    }

    OperationFeedback ReadOperationFeedback(const panda::ecmascript::kungfu::BytecodeInfo &bytecodeInfo)
    {
        OperationFeedback feedback;
        ArkSteedFeedbackReader reader(compilerThread_, bytecodeInfo, &broker_);
        broker_.GetFeedbackForOperation(reader, &feedback);
        return feedback;
    }

private:
    void EnsureBinaryOpProfileCache() const
    {
        if (binaryOpProfileCacheBuilt_) {
            return;
        }
        binaryOpProfileCacheBuilt_ = true;

        if (env_ == nullptr || !env_->GetJSOptions().IsEnableJITPGO()) {
            return;
        }
        auto profiler = env_->GetPGOProfiler();
        if (profiler == nullptr) {
            return;
        }
        JITProfiler *jitProfile = profiler->GetJITProfile();
        if (jitProfile == nullptr) {
            return;
        }

        auto opTypeMap = jitProfile->GetOpTypeMap();
        if (opTypeMap.empty()) {
            return;
        }
        auto insufficientProfileMap = jitProfile->GetBoolMap();
        binaryOpProfileCache_.reserve(opTypeMap.size());
        for (const auto &[offset, opType] : opTypeMap) {
            if (opType == nullptr) {
                continue;
            }
            auto insufficientProfile = insufficientProfileMap.find(offset);
            if (insufficientProfile != insufficientProfileMap.end() && insufficientProfile->second) {
                continue;
            }
            binaryOpProfileCache_.emplace(offset, *opType);
        }
    }

    void ResetBinaryOpProfileCache() const
    {
        binaryOpProfileCacheBuilt_ = false;
        binaryOpProfileCache_.clear();
    }

    JSThread *compilerThread_ {nullptr};
    JitCompilationEnv *env_ {nullptr};
    mutable ArkSteedHeapBroker broker_;
    mutable bool bytecodeProfilesPrepared_ {false};
    mutable bool binaryOpProfileCacheBuilt_ {false};
    mutable std::unordered_map<int32_t, pgo::PGOSampleType> binaryOpProfileCache_ {};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_PGO_CONTEXT_H
