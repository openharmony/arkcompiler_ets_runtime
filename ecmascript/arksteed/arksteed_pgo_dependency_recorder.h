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

#ifndef ECMASCRIPT_ARKSTEED_PGO_DEPENDENCY_RECORDER_H
#define ECMASCRIPT_ARKSTEED_PGO_DEPENDENCY_RECORDER_H

#include "ecmascript/arksteed/arksteed_heap_broker.h"
#include "ecmascript/arksteed/arksteed_pgo_access_info.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/compiler/lazy_deopt_dependency.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_hclass.h"

namespace panda::ecmascript::arksteed {

class ArkSteedPGODependencyRecorder {
public:
    ArkSteedPGODependencyRecorder(JSThread *compilerThread, JitCompilationEnv *env, const ArkSteedHeapBroker *broker)
        : compilerThread_(compilerThread), env_(env), broker_(broker)
    {}

    bool Install(PropertyAccessSet *access) const
    {
        if (access == nullptr || access->caseCount == 0) {
            return false;
        }
        for (uint32_t i = 0; i < access->caseCount; ++i) {
            if (!Validate(access->cases[i])) {
                return false;
            }
        }
        for (uint32_t i = 0; i < access->caseCount; ++i) {
            if (!Install(&access->cases[i])) {
                return false;
            }
        }
        return true;
    }

    bool InstallStableHClass(ArkSteedHClassRef hclass) const
    {
        return CheckStableHClass(hclass) && DependOnStableHClass(hclass);
    }

    bool InstallArrayDetector() const
    {
        if (!CanUseLazyDeopt()) {
            return false;
        }
        auto *dependencies = env_->GetDependencies();
        return dependencies != nullptr &&
               dependencies->DependOnArrayDetector(env_->GetGlobalEnv().GetObject<GlobalEnv>());
    }

    bool InstallStableProtoChain(ArkSteedHClassRef receiverHClass) const
    {
        if (!CanUseLazyDeopt()) {
            return false;
        }
        JSTaggedValue receiverHClassValue = JSTaggedValue::Undefined();
        if (broker_ == nullptr || !broker_->TryResolveRef(receiverHClass, &receiverHClassValue) ||
            !receiverHClassValue.IsJSHClass()) {
            return false;
        }
        if (receiverHClassValue.IsInSharedHeap()) {
            return true;
        }
        auto *dependencies = env_->GetDependencies();
        return dependencies != nullptr && dependencies->DependOnStableProtoChain(
                                              compilerThread_, JSHClass::Cast(receiverHClassValue.GetTaggedObject()),
                                              nullptr, env_->GetGlobalEnv().GetObject<GlobalEnv>());
    }

    bool InstallNotPrototype(ArkSteedHClassRef receiverHClass) const
    {
        return DependOnNotPrototype(receiverHClass);
    }

private:
    bool Install(PropertyAccessInfo *access) const
    {
        if (access->dependencies.hclassDependency == AccessDependencyKind::HCLASS) {
            bool canAssumeStableHClass = DependOnStableHClass(access->expectedHClass);
            if (CanUseLazyDeopt() && !canAssumeStableHClass) {
                return false;
            }
            access->dependencies.canAssumeStableHClass = canAssumeStableHClass;
        }
        if (access->dependencies.protoChainDependency == AccessDependencyKind::PROTOTYPE_CHAIN) {
            bool dependOnFullProtoChain = access->IsNotFound() && !access->HasHolder();
            access->dependencies.canAssumeStableProtoChain = DependOnStableProtoChain(
                access->expectedHClass, *access, access->holderIsReceiver || dependOnFullProtoChain);
        }
        if (access->dependencies.notPrototypeDependency == AccessDependencyKind::NOT_PROTOTYPE) {
            access->dependencies.canAssumeNotPrototype = DependOnNotPrototype(access->expectedHClass);
        }
        return true;
    }

    bool Validate(const PropertyAccessInfo &access) const
    {
        if (access.dependencies.hclassDependency == AccessDependencyKind::HCLASS &&
            !CheckStableHClass(access.expectedHClass)) {
            return false;
        }
        return true;
    }

    bool CheckStableHClass(ArkSteedHClassRef hclass) const
    {
        JSTaggedValue hclassValue = JSTaggedValue::Undefined();
        if (broker_ == nullptr || !broker_->TryResolveRef(hclass, &hclassValue) || !hclassValue.IsJSHClass()) {
            return false;
        }
        return kungfu::LazyDeoptAllDependencies::CheckStableHClass(JSHClass::Cast(hclassValue.GetTaggedObject()));
    }

    bool TryResolveHolderHClass(const PropertyAccessInfo &access, JSHClass *receiver, bool holderIsReceiver,
                                JSHClass **holderHClass) const
    {
        *holderHClass = receiver;
        if (holderIsReceiver) {
            return true;
        }
        JSTaggedValue holderHClassValue = JSTaggedValue::Undefined();
        if (broker_->TryResolveRef(access.holderHClass, &holderHClassValue) && holderHClassValue.IsJSHClass()) {
            *holderHClass = JSHClass::Cast(holderHClassValue.GetTaggedObject());
            return true;
        }
        return false;
    }

    bool DependOnStableHClass(ArkSteedHClassRef hclass) const
    {
        JSTaggedValue hclassValue = JSTaggedValue::Undefined();
        if (broker_ == nullptr || !broker_->TryResolveRef(hclass, &hclassValue) || !hclassValue.IsJSHClass()) {
            return false;
        }
        if (hclassValue.IsInSharedHeap()) {
            return true;
        }
        if (!CanUseLazyDeopt()) {
            return false;
        }
        auto *dependencies = env_ == nullptr ? nullptr : env_->GetDependencies();
        return dependencies != nullptr &&
               dependencies->DependOnStableHClass(JSHClass::Cast(hclassValue.GetTaggedObject()));
    }

    bool DependOnStableProtoChain(ArkSteedHClassRef receiverHClass, const PropertyAccessInfo &access,
                                  bool holderIsReceiver) const
    {
        if (!CanUseLazyDeopt()) {
            return false;
        }
        JSTaggedValue receiverHClassValue = JSTaggedValue::Undefined();
        if (broker_ == nullptr || !broker_->TryResolveRef(receiverHClass, &receiverHClassValue) ||
            !receiverHClassValue.IsJSHClass()) {
            return false;
        }
        JSHClass *receiver = JSHClass::Cast(receiverHClassValue.GetTaggedObject());
        JSHClass *holderHClass = nullptr;
        if (!TryResolveHolderHClass(access, receiver, holderIsReceiver, &holderHClass)) {
            return false;
        }
        if (receiverHClassValue.IsInSharedHeap()) {
            return true;
        }
        auto *dependencies = env_ == nullptr ? nullptr : env_->GetDependencies();
        return dependencies != nullptr &&
               dependencies->DependOnStableProtoChain(compilerThread_, receiver, holderHClass,
                                                      env_->GetGlobalEnv().GetObject<GlobalEnv>());
    }

    bool DependOnNotPrototype(ArkSteedHClassRef receiverHClass) const
    {
        if (!CanUseLazyDeopt()) {
            return false;
        }
        JSTaggedValue receiverHClassValue = JSTaggedValue::Undefined();
        if (broker_ == nullptr || !broker_->TryResolveRef(receiverHClass, &receiverHClassValue) ||
            !receiverHClassValue.IsJSHClass()) {
            return false;
        }
        JSHClass *receiver = JSHClass::Cast(receiverHClassValue.GetTaggedObject());
        if (receiverHClassValue.IsInSharedHeap()) {
            return !receiver->IsPrototype();
        }
        auto *dependencies = env_ == nullptr ? nullptr : env_->GetDependencies();
        return dependencies != nullptr && dependencies->DependOnNotPrototype(receiver);
    }

    bool CanUseLazyDeopt() const
    {
        return env_ != nullptr && env_->GetJSOptions().IsEnableJitLazyDeopt();
    }

    JSThread *compilerThread_ {nullptr};
    JitCompilationEnv *env_ {nullptr};
    const ArkSteedHeapBroker *broker_ {nullptr};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_PGO_DEPENDENCY_RECORDER_H
