/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_PARALLEL_EVACUATOR_H
#define ECMASCRIPT_MEM_PARALLEL_EVACUATOR_H

#include <atomic>
#include <memory>

#include "ecmascript/js_hclass.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/tlab_allocator.h"
#include "ecmascript/taskpool/task.h"

#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
class ParallelEvacuator {
public:
    explicit ParallelEvacuator(Heap *heap) : heap_(heap) {}
    ~ParallelEvacuator() = default;
    void Initialize();
    void Finalize();
    void Evacuate();

    size_t GetPromotedSize() const
    {
        return promotedSize_;
    }

    size_t GetEdenToYoungSize() const
    {
        return edenToYoungSize_;
    }
private:
    class EvacuationTask : public Task {
    public:
        EvacuationTask(int32_t id, uint32_t idOrder, ParallelEvacuator *evacuator);
        ~EvacuationTask() override;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(EvacuationTask);
        NO_MOVE_SEMANTIC(EvacuationTask);

    private:
        uint32_t idOrder_;
        ParallelEvacuator *evacuator_;
        TlabAllocator *allocator_ {nullptr};
    };

    class UpdateReferenceTask : public Task {
    public:
        UpdateReferenceTask(int32_t id, ParallelEvacuator *evacuator) : Task(id), evacuator_(evacuator) {};
        ~UpdateReferenceTask() override = default;

        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(UpdateReferenceTask);
        NO_MOVE_SEMANTIC(UpdateReferenceTask);

    private:
        ParallelEvacuator *evacuator_;
    };

    class Workload {
    public:
        Workload(ParallelEvacuator *evacuator, Region *region) : evacuator_(evacuator), region_(region) {};
        virtual ~Workload() = default;
        virtual bool Process(bool isMain) = 0;
        inline Region *GetRegion() const
        {
            return region_;
        }

        inline ParallelEvacuator *GetEvacuator() const
        {
            return evacuator_;
        }
    protected:
        ParallelEvacuator *evacuator_;
        Region *region_;
    };

    class AcquireItem {
    public:
        AcquireItem() = default;
        AcquireItem(AcquireItem&& other) noexcept {(void)other;};
        bool TryAcquire();
    private:
        std::atomic<bool> acquire_{false};
    };

    class WorkloadSet {
    public:
        inline void Add(std::unique_ptr<Workload> workload);
        inline size_t GetWorkloadCount() const;
        inline bool HasRemaningWorkload() const;
        inline bool FetchSubAndCheckWorkloadCount(size_t finishedCount);
        void PrepareWorkloads();
        std::optional<size_t> GetNextIndex();
        std::unique_ptr<ParallelEvacuator::Workload> TryGetWorkload(size_t index);
        void Clear();
    private:
        using WorkItem = std::pair<AcquireItem, std::unique_ptr<Workload>>;
        std::vector<WorkItem> workloads_;
        std::vector<size_t> indexList_;
        std::atomic<size_t> indexCursor_ = 0;
        std::atomic<size_t> remainingWorkloadNum_ = 0;
    };
    
    class EvacuateWorkload : public Workload {
    public:
        EvacuateWorkload(ParallelEvacuator *evacuator, Region *region) : Workload(evacuator, region) {}
        ~EvacuateWorkload() override = default;
        bool Process(bool isMain) override;
    };

    class UpdateRSetWorkload : public Workload {
    public:
        UpdateRSetWorkload(ParallelEvacuator *evacuator, Region *region, bool isEdenGC)
            : Workload(evacuator, region), isEdenGC_(isEdenGC) {}
        ~UpdateRSetWorkload() override = default;
        bool Process(bool isMain) override;
    private:
        bool isEdenGC_;
    };

    class UpdateNewToEdenRSetWorkload : public Workload {
    public:
        UpdateNewToEdenRSetWorkload(ParallelEvacuator *evacuator, Region *region) : Workload(evacuator, region) {}
        ~UpdateNewToEdenRSetWorkload() override = default;
        bool Process(bool isMain) override;
    };

    class UpdateNewRegionWorkload : public Workload {
    public:
        UpdateNewRegionWorkload(ParallelEvacuator *evacuator, Region *region, bool isYoungGC)
            : Workload(evacuator, region), isYoungGC_(isYoungGC) {}
        ~UpdateNewRegionWorkload() override = default;
        bool Process(bool isMain) override;
    private:
        bool isYoungGC_;
    };

    class UpdateAndSweepNewRegionWorkload : public Workload {
    public:
        UpdateAndSweepNewRegionWorkload(ParallelEvacuator *evacuator, Region *region, bool isYoungGC)
            : Workload(evacuator, region), isYoungGC_(isYoungGC) {}
        ~UpdateAndSweepNewRegionWorkload() override = default;
        bool Process(bool isMain) override;
    private:
        bool isYoungGC_;
    };

    template <typename WorkloadCallback>
    void DrainWorkloads(WorkloadSet &workloadSet, WorkloadCallback callback);

    std::unordered_set<JSTaggedType> &ArrayTrackInfoSet(uint32_t threadIndex)
    {
        return arrayTrackInfoSets_[threadIndex];
    }

    TaggedObject* UpdateAddressAfterEvacation(TaggedObject *oldTrackInfo);

    void UpdateTrackInfo();

    bool ProcessWorkloads(bool isMain = false);

    void EvacuateSpace();
    bool EvacuateSpace(TlabAllocator *allocation, uint32_t threadIndex, uint32_t idOrder, bool isMain = false);
    void UpdateRecordWeakReferenceInParallel(uint32_t idOrder);
    void EvacuateRegion(TlabAllocator *allocator, Region *region, std::unordered_set<JSTaggedType> &trackSet);
    template<bool SetEdenObject>
    inline void SetObjectFieldRSet(TaggedObject *object, JSHClass *cls);
    template<bool SetEdenObject>
    inline void SetObjectRSet(ObjectSlot slot, Region *region);

    inline void UpdateLocalToShareRSet(TaggedObject *object, JSHClass *cls);
    inline void SetLocalToShareRSet(ObjectSlot slot, Region *region);

    inline bool IsWholeRegionEvacuate(Region *region);
    inline bool WholeRegionEvacuate(Region *region);
    void VerifyValue(TaggedObject *object, ObjectSlot slot);
    void VerifyHeapObject(TaggedObject *object);

    void UpdateReference();
    void UpdateRoot();
    template<TriggerGCType gcType>
    void UpdateWeakReferenceOpt();
    template<bool IsEdenGC>
    void UpdateRSet(Region *region);
    void UpdateNewToEdenRSetReference(Region *region);
    template<TriggerGCType gcType>
    void UpdateNewRegionReference(Region *region);
    template<TriggerGCType gcType>
    void UpdateAndSweepNewRegionReference(Region *region);
    template<TriggerGCType gcType>
    void UpdateNewObjectField(TaggedObject *object, JSHClass *cls);

    template<typename Callback>
    inline bool VisitBodyInObj(TaggedObject *root, ObjectSlot start, ObjectSlot end, Callback callback);
    inline bool UpdateForwardedOldToNewObjectSlot(TaggedObject *object, ObjectSlot &slot, bool isWeak);
    template<bool IsEdenGC>
    inline bool UpdateOldToNewObjectSlot(ObjectSlot &slot);
    inline bool UpdateNewToEdenObjectSlot(ObjectSlot &slot);
    inline void UpdateObjectSlot(ObjectSlot &slot);
    template<TriggerGCType gcType>
    inline void UpdateObjectSlotOpt(ObjectSlot &slot);
    inline void UpdateWeakObjectSlot(TaggedObject *object, ObjectSlot &slot);
    template<TriggerGCType gcType>
    inline bool UpdateWeakObjectSlotOpt(JSTaggedValue value, ObjectSlot &slot);

    inline int CalculateEvacuationThreadNum();
    inline int CalculateUpdateThreadNum();
    void WaitFinished();

    Heap *heap_;
    TlabAllocator *allocator_ {nullptr};

    uintptr_t waterLine_ = 0;
    std::unordered_set<JSTaggedType> arrayTrackInfoSets_[MAX_TASKPOOL_THREAD_NUM + 1];
    uint32_t evacuateTaskNum_ = 0;
    std::atomic_int parallel_ = 0;
    Mutex mutex_;
    ConditionVariable condition_;
    std::atomic<size_t> promotedSize_ = 0;
    std::atomic<size_t> edenToYoungSize_ = 0;
    WorkloadSet evacuateWorkloadSet_;
    WorkloadSet updateWorkloadSet_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_EVACUATOR_H
