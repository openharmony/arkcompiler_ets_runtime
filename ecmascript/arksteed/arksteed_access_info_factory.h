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

#ifndef ECMASCRIPT_ARKSTEED_ACCESS_INFO_FACTORY_H
#define ECMASCRIPT_ARKSTEED_ACCESS_INFO_FACTORY_H

#include "ecmascript/arksteed/arksteed_feedback_reader.h"
#include "ecmascript/arksteed/arksteed_pgo_dependency_recorder.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript::arksteed {

class ArkSteedAccessInfoFactory {
public:
    ArkSteedAccessInfoFactory(JSThread *compilerThread, const panda::ecmascript::kungfu::BytecodeInfo &bytecodeInfo,
                              JitCompilationEnv *env)
        : ArkSteedAccessInfoFactory(compilerThread, bytecodeInfo, env, nullptr)
    {}

    ArkSteedAccessInfoFactory(JSThread *compilerThread, const panda::ecmascript::kungfu::BytecodeInfo &bytecodeInfo,
                              JitCompilationEnv *env, ArkSteedHeapBroker *broker)
        : compilerThread_(compilerThread), bytecodeInfo_(bytecodeInfo), env_(env),
          broker_(broker == nullptr ? &ownedBroker_ : broker),
          feedbackReader_(compilerThread_, bytecodeInfo_, broker_)
    {}

    bool TryBuildNamedStoreAccessInfo(int slotIndex, NamedStoreAccessSet *access) const;
    bool TryBuildNamedLoadAccessInfo(int slotIndex, NamedLoadAccessSet *access) const;

private:
    bool ComputeNamedStoreAccessInfo(const NamedAccessFeedback &feedback, NamedStoreAccessSet *access) const;
    bool ComputeNamedLoadAccessInfo(const NamedAccessFeedback &feedback, NamedLoadAccessSet *access) const;
    bool TryMakeNamedStoreAccessInfo(const NamedAccessCaseFeedback &caseFeedback, NamedStoreAccessInfo *info) const;
    bool TryMakeNamedLoadAccessInfo(const NamedAccessCaseFeedback &caseFeedback, NamedLoadAccessInfo *info) const;
    bool RegisterDependencies(const PropertyAccessSet &access) const;

    JSThread *compilerThread_ {nullptr};
    const panda::ecmascript::kungfu::BytecodeInfo &bytecodeInfo_;
    JitCompilationEnv *env_ {nullptr};
    ArkSteedHeapBroker ownedBroker_ {compilerThread_, env_};
    ArkSteedHeapBroker *broker_ {nullptr};
    ArkSteedFeedbackReader feedbackReader_;
    ArkSteedPGODependencyRecorder dependencyRecorder_ {compilerThread_, env_};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ACCESS_INFO_FACTORY_H
