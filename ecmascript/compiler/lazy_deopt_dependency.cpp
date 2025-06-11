/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/lazy_deopt_dependency.h"

#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/stubs/runtime_stubs.h"

namespace panda::ecmascript::kungfu {

void CombinedDependencies::Register(JSHClass *hclass, DependentGroup group)
{
    // Since the value of deps_ is uint32_t, we don't need to check whether we can find it.
    deps_[hclass] |= static_cast<uint32_t>(group);
}

void CombinedDependencies::Register(uint32_t detectorID, DependentGroup group)
{
    detectorDeps_[detectorID] |= static_cast<uint32_t>(group);
}

void CombinedDependencies::Register(DependentGroup group)
{
    threadDeps_ |= static_cast<uint32_t>(group);
}

void CombinedDependencies::InstallAll(JSThread *thread, JSHandle<JSTaggedValue> jsFunc)
{
    JSMutableHandle<JSHClass> hclass(thread, JSTaggedValue::Undefined());
    for (auto iter : deps_) {
        hclass.Update(JSTaggedValue(iter.first));
        uint32_t groups = iter.second;
        JSHandle<DependentInfos> dependentInfos =
            JSHandle<DependentInfos>::Cast(JSObject::GetOrCreateDependentInfos(thread, hclass));
        JSHandle<DependentInfos> infos = DependentInfos::AppendDependentInfos(thread,
            jsFunc, groups, dependentInfos);
        hclass->SetDependentInfos(thread, infos.GetTaggedValue());
    }
    if (threadDeps_) {
        auto threadDependInfo = thread->GetOrCreateThreadDependentInfo();
        JSHandle<DependentInfos> infos =
            DependentInfos::AppendDependentInfos(thread, jsFunc, threadDeps_, threadDependInfo);
        thread->SetDependentInfo(infos.GetTaggedValue());
    }
    for (auto iter : detectorDeps_) {
        uint32_t detectorID = iter.first;
        uint32_t groups = iter.second;
        JSHandle<DependentInfos> dependentInfos = JSHandle<DependentInfos>::Cast(
            JSObject::GetOrCreateDetectorDependentInfos(thread, detectorID, globalEnv_));
        JSHandle<DependentInfos> infos = DependentInfos::AppendDependentInfos(thread,
            jsFunc, groups, dependentInfos);
        globalEnv_->SetDependentInfos(detectorID, JSHandle<JSTaggedValue>::Cast(infos));
    }
}

// ---------------------------- LazyDeoptAllDependencies ----------------------------

void LazyDeoptAllDependencies::RegisterDependency(const LazyDeoptDependency *dependency)
{
    if (dependency != nullptr) {
        dependencies_.push_back(dependency);
    }
}

bool LazyDeoptAllDependencies::DependOnArrayDetector(GlobalEnv *globalEnv)
{
    return DependOnDetector(GlobalEnv::ArrayPrototypeChangedGuardiansBits::START_BIT, globalEnv);
}

bool LazyDeoptAllDependencies::DependOnDetector(uint32_t detectorID, GlobalEnv *globalEnv)
{
    SetGlobalEnv(globalEnv);
    LazyDeoptDependency *dependency = new DetectorDependency(detectorID, globalEnv);
    if (dependency->IsValid()) {
        RegisterDependency(dependency);
        return true;
    }
    return false;
}

bool LazyDeoptAllDependencies::DependOnNotPrototype(JSHClass *hclass)
{
    LazyDeoptDependency *dependency = new NotPrototypeDependency(hclass);
    if (dependency->IsValid()) {
        RegisterDependency(dependency);
        return true;
    }
    return false;
}

bool LazyDeoptAllDependencies::DependOnStableHClass(JSHClass *hclass)
{
    LazyDeoptDependency *dependency = new StableHClassDependency(hclass);
    if (dependency->IsValid()) {
        RegisterDependency(dependency);
        return true;
    }
    return false;
}

bool LazyDeoptAllDependencies::DependOnStableProtoChain(JSHClass *receiverHClass,
                                                        JSHClass *holderHClass,
                                                        GlobalEnv *globalEnv)
{
    // For "stobjbyname", the holder may not the actual holder.
    // So when receiverHClass == holderHClass,
    // we should check all of the chain of the receiver.
    if (receiverHClass == holderHClass) {
        holderHClass = nullptr;
    }
    JSTaggedValue current;
    if (!receiverHClass->IsCompositeHClass()) {
        if (receiverHClass->IsString()) {
            ASSERT(globalEnv != nullptr);
            current = globalEnv->GetStringPrototype().GetTaggedValue();
            holderHClass = nullptr;
        } else {
            return false;
        }
    } else {
        current = receiverHClass->GetPrototype();
    }
    bool success = true;
    while (current.IsHeapObject()) {
        auto currentHC = current.GetTaggedObject()->GetClass();
        success &= DependOnStableHClass(currentHC);
        // We only need to ensure Stable of the prototype chain
        // from the receiver's prototype to the holder.
        if (currentHC == holderHClass) {
            break;
        }
        current = currentHC->GetPrototype();
    }
    return success;
}

bool LazyDeoptAllDependencies::DependOnHotReloadPatchMain(JSThread *thread)
{
    LazyDeoptDependency *dependency = new HotReloadDependency(thread);
    if (dependency->IsValid()) {
        RegisterDependency(dependency);
        return true;
    }
    return false;
}

bool LazyDeoptAllDependencies::PreInstall(JSThread *thread)
{
    for (auto dep : dependencies_) {
        if (!dep->IsValid()) {
            if (thread->GetEcmaVM()->GetJSOptions().IsEnableLazyDeoptTrace()) {
                LOG_FULL(INFO) << "Lazy Deoptimized Compilation aborted due to invalid dependency";
            }
            return false;
        }
    }
    return true;
}

// static
bool LazyDeoptAllDependencies::Commit(LazyDeoptAllDependencies *dependencies,
                                      JSThread *thread, JSTaggedValue jsFunc)
{
    if (!thread->GetEcmaVM()->GetJSOptions().IsEnableJitLazyDeopt()) {
        return true;
    }
    if (!dependencies->PreInstall(thread)) {
        return false;
    }
    JSHandle<JSTaggedValue> jsFuncHandle(thread, jsFunc);
    CombinedDependencies cbDependencies(dependencies->globalEnv_);
    for (auto dep : dependencies->dependencies_) {
        dep->Install(&cbDependencies);
    }
    cbDependencies.InstallAll(thread, jsFuncHandle);
    RuntimeStubs::TraceLazyDeoptCommitSuccess(thread->GetGlueAddr(), jsFuncHandle);
    return true;
}
}  // namespace panda::ecmascript::kungfu