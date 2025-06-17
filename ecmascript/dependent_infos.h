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
    V(PROTOTYPE_CHECK,    1ULL << 0)     \
    V(IS_PROTOTYPE_CHECK, 1ULL << 1)     \
    V(DETECTOR_CHECK,     1ULL << 2)     \
    V(HOTRELOAD_PATCHMAIN, 1ULL << 3)

/*
 * -------------- Structure of DependentInfos --------------
 *
 * Index | Stored Content       | Description
 * ------|----------------------|---------------------------
 *   0   | function0            | Function Slot
 *   1   | collection0          | Collection Slot
 *   2   | function1            | Function Slot
 *   3   | collection1          | Collection Slot
 *   4   | function2            | Function Slot
 *   5   | collection2          | Collection Slot
 *   6   | ...                  | ...
 */

/*
 * DependentInfos are stored on the HClass as (function)-(collection) pairs.
 *
 * When the HClass changes in a way that satisfies collection's requirement,
 * the corresponding paired function is marked for lazy deoptimization.
 */
class DependentInfos : public WeakVector {
public:
    static constexpr uint32_t SLOT_PER_ENTRY = 2;
    static constexpr uint32_t FUNCTION_SLOT_OFFSET = 0;
    static constexpr uint32_t COLLECTION_SLOT_OFFSET = 1;
    static DependentInfos *Cast(TaggedObject *object)
    {
        return static_cast<DependentInfos *>(object);
    }
    enum DependentState : uint32_t {
        #define LAZY_DEOPT_TYPE_MEMBER(name, value) name = (value),
            LAZY_DEOPT_TYPE_LIST(LAZY_DEOPT_TYPE_MEMBER)
        #undef LAZY_DEOPT_TYPE_MEMBER
    };
    using DependentStateCollection = uint32_t;

    static JSHandle<DependentInfos> AppendDependentInfos(JSThread *thread,
                                                         const JSHandle<JSTaggedValue> jsFunc,
                                                         const DependentStateCollection collection,
                                                         const JSHandle<DependentInfos> info);
    // CheckCollectionEffect ensures don't trigger unnecessary lazy deopt.
    // For example, when we need to ensure the IsStable flag of HClass remains valid,
    // modifying the IsPrototype flag of that HClass
    // shouldn't trigger lazy deopt.
    static bool CheckCollectionEffect(DependentStateCollection depCollection, DependentStateCollection collection)
    {
        return (depCollection & collection) > 0;
    }
    static void TriggerLazyDeoptimization(JSHandle<DependentInfos> dependentInfos,
                                          JSThread *thread, DependentStateCollection Collection);
    static void TraceLazyDeoptReason(JSThread *thread, JSHandle<JSFunction> func,
                                     DependentStateCollection collection);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DEPENDENT_INFOS_H