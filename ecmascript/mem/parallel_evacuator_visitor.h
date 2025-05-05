/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_PARALLEL_EVACUATOR_VISITOR_H
#define ECMASCRIPT_MEM_PARALLEL_EVACUATOR_VISITOR_H

#include "ecmascript/mem/parallel_evacuator.h"

namespace panda::ecmascript {
template<TriggerGCType gcType>
class SlotUpdateRangeVisitor final : public BaseObjectVisitor<SlotUpdateRangeVisitor<gcType>> {
public:
    explicit SlotUpdateRangeVisitor(ParallelEvacuator *evacuator);
    ~SlotUpdateRangeVisitor() override = default;

    void VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end, VisitObjectArea area) override;
private:
    void UpdateSlot(ObjectSlot slot, Region *rootRegion);

    ParallelEvacuator *evacuator_ {nullptr};
};

template<TriggerGCType gcType>
class NewToOldEvacuationVisitor {
public:
    explicit NewToOldEvacuationVisitor(Heap *heap, std::unordered_set<JSTaggedType> *set,
        ParallelEvacuator *evacuator);

    ~NewToOldEvacuationVisitor() = default;

    NO_COPY_SEMANTIC(NewToOldEvacuationVisitor);
    NO_MOVE_SEMANTIC(NewToOldEvacuationVisitor);

    void operator()(void *mem);

    void UpdateTrackInfo(TaggedObject *header, JSHClass *klass);

private:
    bool pgoEnabled_ {false};
    std::shared_ptr<PGOProfiler> pgoProfiler_;
    std::unordered_set<JSTaggedType> *trackSet_ {nullptr};
    SlotUpdateRangeVisitor<gcType> slotUpdateRangeVisitor_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_EVACUATOR_VISITOR_H
