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

#ifndef ECMASCRIPT_DEPENDENT_INFOS_H
#define ECMASCRIPT_DEPENDENT_INFOS_H

#include "ecmascript/weak_vector.h"

namespace panda::ecmascript {
#define LAZY_DEOPT_TYPE_LIST(V)          \
    V(PROTOTYPE_CHECK, 1ULL << 0)

/*
 * -------------- Structure of DependentInfos --------------
 *
 * Index | Stored Content       | Description
 * ------|----------------------|---------------------------
 *   0   | function0            | Function Slot
 *   1   | group0               | Group Slot
 *   2   | function1            | Function Slot
 *   3   | group1               | Group Slot
 *   4   | function2            | Function Slot
 *   5   | group2               | Group Slot
 *   6   | ...                  | ...
 */

/*
 * DependentInfos are stored on the HClass as (function)-(group) pairs.
 *
 * When the HClass changes in a way that satisfies group's requirement,
 * the corresponding paired function is marked for lazy deoptimization.
 */
class DependentInfos : public WeakVector {
public:
    static constexpr uint32_t SLOT_PER_ENTRY = 2;
    static constexpr uint32_t FUNCTION_SLOT_OFFSET = 0;
    static constexpr uint32_t GROUP_SLOT_OFFSET = 1;
    static DependentInfos *Cast(TaggedObject *object)
    {
        return static_cast<DependentInfos *>(object);
    }
    enum DependentGroup : uint32_t {
        #define LAZY_DEOPT_TYPE_MEMBER(name, value) name = (value),
            LAZY_DEOPT_TYPE_LIST(LAZY_DEOPT_TYPE_MEMBER)
        #undef LAZY_DEOPT_TYPE_MEMBER
    };
    using DependentGroups = uint32_t;

    static JSHandle<DependentInfos> AppendDependentInfos(JSThread *thread,
                                                         const JSHandle<JSTaggedValue> jsFunc,
                                                         const DependentGroups groups,
                                                         const JSHandle<DependentInfos> info);
    // CheckGroupsEffect ensures don't trigger unnecessary lazy deopt.
    // For example, when we need to ensure the IsStable flag of HClass remains valid,
    // modifying the IsPrototype flag of that HClass
    // shouldn't trigger lazy deopt.
    static bool CheckGroupsEffect(DependentGroups depGroups, DependentGroups groups)
    {
        return (depGroups & groups) > 0;
    }
    static void DeoptimizeGroups(JSHandle<DependentInfos> dependentInfos,
                                 JSThread *thread, DependentGroups groups);
    static void TraceLazyDeoptReason(JSThread *thread, JSHandle<JSFunction> func,
                                     DependentGroups groups);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DEPENDENT_INFOS_H