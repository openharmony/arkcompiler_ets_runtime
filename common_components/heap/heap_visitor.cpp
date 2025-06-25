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

#include "common_interfaces/heap/heap_visitor.h"

#include "common_components/base_runtime/hooks.h"
#include "common_components/mutator/mutator.h"

namespace common {
void VisitRoots(const RefFieldVisitor &visitor)
{
    VisitDynamicGlobalRoots(visitor);
    VisitDynamicLocalRoots(visitor);
    VisitBaseRoots(visitor);
}

void VisitWeakRoots(const WeakRefFieldVisitor &visitor)
{
    VisitDynamicWeakGlobalRoots(visitor);
    VisitDynamicWeakLocalRoots(visitor);
}

void VisitGlobalRoots(const RefFieldVisitor &visitor)
{
    VisitDynamicGlobalRoots(visitor);
    VisitBaseRoots(visitor);
}

void VisitWeakGlobalRoots(const WeakRefFieldVisitor &visitor)
{
    VisitDynamicWeakGlobalRoots(visitor);
}

// Visit specific mutator's root.
void VisitMutatorRoot(const RefFieldVisitor &visitor, Mutator &mutator)
{
    if (mutator.GetEcmaVMPtr()) {
        VisitDynamicThreadRoot(visitor, mutator.GetEcmaVMPtr());
    }
}

void VisitWeakMutatorRoot(const WeakRefFieldVisitor &visitor, Mutator &mutator)
{
    if (mutator.GetEcmaVMPtr()) {
        VisitDynamicWeakThreadRoot(visitor, mutator.GetEcmaVMPtr());
    }
}
}  // namespace common
