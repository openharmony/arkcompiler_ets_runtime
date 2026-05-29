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

#include "ecmascript/arksteed/arksteed_pgo_access_info.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/compiler/lazy_deopt_dependency.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_hclass.h"

namespace panda::ecmascript::arksteed {

class ArkSteedPGODependencyRecorder {
public:
    ArkSteedPGODependencyRecorder(JSThread *compilerThread, JitCompilationEnv *env)
        : compilerThread_(compilerThread), env_(env)
    {}

    bool Install(const PropertyAccessSet &access) const
    {
        if (access.caseCount == 0) {
            return false;
        }
        for (uint32_t i = 0; i < access.caseCount; ++i) {
            if (!Validate(access.cases[i])) {
                return false;
            }
        }
        for (uint32_t i = 0; i < access.caseCount; ++i) {
            if (!Install(access.cases[i])) {
                return false;
            }
        }
        return true;
    }

private:
    bool Install(const PropertyAccessInfo &access) const
    {
        if (access.dependencies.hclassDependency == AccessDependencyKind::HCLASS &&
            !DependOnStableHClass(access.expectedHClass)) {
            return false;
        }
        if (access.dependencies.protoCellDependency == AccessDependencyKind::PROTOTYPE_CELL) {
            bool dependOnFullProtoChain = access.IsNotFound() && !access.HasHolder();
            return DependOnStableProtoChain(access.expectedHClass, access.holder,
                                            access.holderIsReceiver || dependOnFullProtoChain,
                                            access.hasProtoCell);
        }
        return true;
    }

    bool Validate(const PropertyAccessInfo &access) const
    {
        if (access.dependencies.hclassDependency == AccessDependencyKind::HCLASS &&
            !CheckStableHClass(access.expectedHClass)) {
            return false;
        }
        if (access.dependencies.protoCellDependency == AccessDependencyKind::PROTOTYPE_CELL) {
            bool dependOnFullProtoChain = access.IsNotFound() && !access.HasHolder();
            return CheckStableProtoChain(access.expectedHClass, access.holder,
                                         access.holderIsReceiver || dependOnFullProtoChain,
                                         access.hasProtoCell);
        }
        return true;
    }

    bool CheckStableHClass(ArkSteedHClassRef hclass) const
    {
        if (!hclass.IsSafeForCompile() || !hclass.Value().IsJSHClass()) {
            return false;
        }
        return kungfu::LazyDeoptAllDependencies::CheckStableHClass(JSHClass::Cast(hclass.Value().GetTaggedObject()));
    }

    bool CheckStableProtoChain(ArkSteedHClassRef receiverHClass, ArkSteedObjectRef holder,
                               bool holderIsReceiver, bool hasProtoCell) const
    {
        if (!hasProtoCell || !receiverHClass.IsSafeForCompile() || !receiverHClass.Value().IsJSHClass()) {
            return false;
        }
        JSHClass *receiver = JSHClass::Cast(receiverHClass.Value().GetTaggedObject());
        JSHClass *holderHClass = receiver;
        if (!holderIsReceiver) {
            if (!holder.IsSafeForCompile() || !holder.Value().IsHeapObject()) {
                return false;
            }
            holderHClass = holder.Value().GetTaggedObject()->GetClass();
        }
        return kungfu::LazyDeoptAllDependencies::CheckStableProtoChain(compilerThread_, receiver, holderHClass,
                                                                       env_->GetGlobalEnv().GetObject<GlobalEnv>());
    }

    bool DependOnStableHClass(ArkSteedHClassRef hclass) const
    {
        if (!hclass.IsSafeForCompile() || !hclass.Value().IsJSHClass()) {
            return false;
        }
        return env_->GetDependencies()->DependOnStableHClass(JSHClass::Cast(hclass.Value().GetTaggedObject()));
    }

    bool DependOnStableProtoChain(ArkSteedHClassRef receiverHClass, ArkSteedObjectRef holder,
                                  bool holderIsReceiver, bool hasProtoCell) const
    {
        if (!hasProtoCell || !receiverHClass.IsSafeForCompile() || !receiverHClass.Value().IsJSHClass()) {
            return false;
        }
        JSHClass *receiver = JSHClass::Cast(receiverHClass.Value().GetTaggedObject());
        JSHClass *holderHClass = receiver;
        if (!holderIsReceiver) {
            if (!holder.IsSafeForCompile() || !holder.Value().IsHeapObject()) {
                return false;
            }
            holderHClass = holder.Value().GetTaggedObject()->GetClass();
        }
        return env_->GetDependencies()->DependOnStableProtoChain(compilerThread_, receiver, holderHClass,
                                                                 env_->GetGlobalEnv().GetObject<GlobalEnv>());
    }

    JSThread *compilerThread_ {nullptr};
    JitCompilationEnv *env_ {nullptr};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_PGO_DEPENDENCY_RECORDER_H
