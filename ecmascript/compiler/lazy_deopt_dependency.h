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

#ifndef ECMASCRIPT_COMPILER_LAZY_DEOPT_DEPENDENCY_H
#define ECMASCRIPT_COMPILER_LAZY_DEOPT_DEPENDENCY_H

#include "ecmascript/common.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/dependent_infos.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript::kungfu {

using DependentState = DependentInfos::DependentState;
using DependentStateCollection = DependentInfos::DependentStateCollection;
class CombinedDependencies {
public:
    CombinedDependencies(GlobalEnv *globalEnv)
    {
        globalEnv_ = globalEnv;
    }
    NO_COPY_SEMANTIC(CombinedDependencies);
    NO_MOVE_SEMANTIC(CombinedDependencies);

    void Register(JSHClass *hclass, DependentState state);
    void Register(uint32_t detectorID, DependentState state);
    void Register(DependentState state);

    void InstallAll(JSThread *thread, JSHandle<JSTaggedValue> jsFunc);

private:
    std::map<JSHClass *, DependentStateCollection> deps_;        // hclass, collection
    std::map<uint32_t, DependentStateCollection> detectorDeps_;  // detectorID_, collection
    GlobalEnv *globalEnv_;
    DependentStateCollection threadDeps_ {0};
};

enum class LazyDeoptDependencyKind : uint32_t {
    STABLE_HCLASS,          // Once the HClass undergoes transition, "isStable" bit remains false permanently
    NOT_PROTOTYPE_HCLASS,   // Once the HClass becomes prototype, "isPrototype" bit remains true permanently
    DETECTOR,
    NOT_HOT_RELOAD_PATCHMAIN,
};

class LazyDeoptDependency {
public:
    explicit LazyDeoptDependency(LazyDeoptDependencyKind kind) : kind_(kind) {}
    virtual ~LazyDeoptDependency() = default;

    virtual bool IsValid() const = 0;
    virtual void Install(CombinedDependencies *combinedDeps) const = 0;
private:
    [[maybe_unused]]LazyDeoptDependencyKind kind_;
};

class DetectorDependency final : public LazyDeoptDependency {
public:
    DetectorDependency (uint32_t detectorID, GlobalEnv *globalEnv)
        : LazyDeoptDependency(LazyDeoptDependencyKind::DETECTOR),
          detectorID_(detectorID), globalEnv_(globalEnv) {}

    bool IsValid() const override
    {
        return globalEnv_->GetDetectorValue(detectorID_);
    }

    void Install(CombinedDependencies *combinedDeps) const override
    {
        ASSERT(IsValid());
        combinedDeps->Register(detectorID_, DependentState::DETECTOR_CHECK);
    }
    
private:
    uint32_t detectorID_;
    GlobalEnv *globalEnv_;
};

class NotPrototypeDependency final : public LazyDeoptDependency {
public:
    NotPrototypeDependency(JSHClass *hclass)
        : LazyDeoptDependency(LazyDeoptDependencyKind::NOT_PROTOTYPE_HCLASS),
          hclass_(hclass) {}

    bool IsValid() const override
    {
        return !hclass_->IsPrototype();
    }

    void Install(CombinedDependencies *combinedDeps) const override
    {
        ASSERT(IsValid());
        combinedDeps->Register(hclass_, DependentState::IS_PROTOTYPE_CHECK);
    }

private:
    JSHClass *hclass_ {nullptr};
};

class StableHClassDependency final : public LazyDeoptDependency {
public:
    StableHClassDependency(JSHClass *hclass)
        : LazyDeoptDependency(LazyDeoptDependencyKind::STABLE_HCLASS),
          hclass_(hclass) {}

    bool IsValid() const override
    {
        return !hclass_->IsDictionaryMode() && hclass_->IsStable();
    }

    void Install(CombinedDependencies *combinedDeps) const override
    {
        ASSERT(IsValid());
        combinedDeps->Register(hclass_, DependentState::PROTOTYPE_CHECK);
    }

private:
    JSHClass *hclass_ {nullptr};
};

class NotHotReloadDependency final : public LazyDeoptDependency {
public:
    NotHotReloadDependency(JSThread *thread)
        : LazyDeoptDependency(LazyDeoptDependencyKind::NOT_HOT_RELOAD_PATCHMAIN), thread_(thread) {}

    bool IsValid() const override
    {
        return thread_->GetStageOfHotReload() != StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN;
    }

    void Install(CombinedDependencies *combinedDeps) const override
    {
        ASSERT(IsValid());
        combinedDeps->Register(DependentState::HOTRELOAD_PATCHMAIN);
    }

private:
    JSThread *thread_ {nullptr};
};

class PUBLIC_API LazyDeoptAllDependencies {
public:
    LazyDeoptAllDependencies() = default;
    ~LazyDeoptAllDependencies()
    {
        Clear();
    }

    bool DependOnArrayDetector(GlobalEnv *globalEnv);
    bool DependOnDetector(uint32_t detectorID, GlobalEnv *globalEnv);
    bool DependOnNotPrototype(JSHClass *hclass);
    bool DependOnStableHClass(JSHClass *hclass);
    bool DependOnStableProtoChain(JSHClass *receiverHClass,
                                  JSHClass *holderHClass,
                                  GlobalEnv *globalEnv = nullptr);
    bool DependOnNotHotReloadPatchMain(JSThread *thread);
    bool PreInstall(JSThread *thread);
    PUBLIC_API static bool Commit(LazyDeoptAllDependencies *dependencies,
                                  JSThread *thread, JSTaggedValue jsFunc);

    void Clear()
    {
        for (auto& dep : dependencies_) {
            delete dep;
        }
        dependencies_.clear();
    }

    void SetGlobalEnv(GlobalEnv *globalEnv)
    {
        globalEnv_ = globalEnv;
    }

    GlobalEnv* GetGlobalEnv()
    {
        return globalEnv_;
    }
private:
    void RegisterDependency(const LazyDeoptDependency *dependency);
    std::vector<const LazyDeoptDependency *> dependencies_;
    GlobalEnv *globalEnv_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LAZY_DEOPT_DEPENDENCY_H