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

class DependentInfos : public WeakVector {
public:
    static constexpr uint32_t SLOT_PER_ENTRY = 2;
    static constexpr uint32_t METHOD_SLOT_OFFSET = 0;
    static constexpr uint32_t GROUP_SLOT_OFFSET = 1;
    static DependentInfos *Cast(TaggedObject *object)
    {
        return static_cast<DependentInfos *>(object);
    }
    enum DependentGroup : uint32_t {
        #define LAZY_DEOPT_TYPE_MEMBER(name, value) name = value,
            LAZY_DEOPT_TYPE_LIST(LAZY_DEOPT_TYPE_MEMBER)
        #undef LAZY_DEOPT_TYPE_MEMBER
    };
    using DependentGroups = uint32_t;

    static JSHandle<DependentInfos> AppendDependentInfos(JSThread *thread,
                                                         const JSHandle<JSTaggedValue> jsFunc,
                                                         const DependentGroups groups,
                                                         const JSHandle<DependentInfos> info);
    static void DeoptimizeGroups(JSHandle<DependentInfos> dependentInfos,
                                 JSThread *thread, DependentGroups groups);
    static void TraceLazyDeoptReason(JSThread *thread, JSHandle<JSFunction> func,
                                     DependentGroups groups);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DEPENDENT_INFOS_H