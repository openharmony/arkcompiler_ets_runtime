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

using DependentGroup = DependentInfos::DependentGroup;
using DependentGroups = DependentInfos::DependentGroups;
class CombinedDependencies {
public:
    void Register(JSHClass *hclass, DependentGroup group);

    void InstallAll(JSThread *thread, JSHandle<JSTaggedValue> jsFunc);

private:
    std::map<JSHClass *, DependentGroups> deps_;  // hclass, groups
};

enum class LazyDeoptDependencyKind : uint32_t {
    STABLE_HCLASS = 0x0ULL, // Once the hClass happended transition, it's not a stable hClass.
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
        combinedDeps->Register(hclass_, DependentGroup::PROTOTYPE_CHECK);
    }

private:
    JSHClass *hclass_ {nullptr};
};

class PUBLIC_API LazyDeoptAllDependencies {
public:
    LazyDeoptAllDependencies() = default;
    ~LazyDeoptAllDependencies()
    {
        Clear();
    }

    bool DependOnStableHClass(JSHClass *hclass);
    bool DependOnStableProtoChain(JSHClass *receiverHClass,
                                  JSHClass *holderHClass);
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

private:
    void RegisterDependency(const LazyDeoptDependency *dependency);
    std::vector<const LazyDeoptDependency *> dependencies_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LAZY_DEOPT_DEPENDENCY_H