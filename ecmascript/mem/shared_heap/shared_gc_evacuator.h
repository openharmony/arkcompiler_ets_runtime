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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_EVACUATOR_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_EVACUATOR_H

#include "ecmascript/mem/heap-inl.h"

namespace panda::ecmascript {
class SharedGCEvacuator {
public:
    explicit SharedGCEvacuator(SharedHeap *heap) : sHeap_(heap), objectFieldCSetVisitor_(this) {}
    ~SharedGCEvacuator() = default;
    NO_COPY_SEMANTIC(SharedGCEvacuator);
    NO_MOVE_SEMANTIC(SharedGCEvacuator);
    void Evacuate();
private:
    void EvacuateRegions();
    void UpdateReference();
    bool UpdateObjectSlot(ObjectSlot slot);
    void UpdateObjectSlotRoot(ObjectSlot slot);
    void ProcessObjectField(TaggedObject *object, JSHClass *hclass);
    void PostParallelTasks();
    void WaitFinished();
    SharedTlabAllocator *GetOrCreateTlab(uint32_t index)
    {
        SharedTlabAllocator *tlab = sTlabs_[index];
        if UNLIKELY(tlab == nullptr) {
            tlab = new SharedTlabAllocator(sHeap_);
            sTlabs_[index] = tlab;
        }
        return tlab;
    }

    int CalculateParallelThreadNum();

    class UpdateRootVisitor final : public RootVisitor {
    public:
        UpdateRootVisitor() = default;
        ~UpdateRootVisitor() = default;

        void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override;
        void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override;
        void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                     uintptr_t baseOldObject) override;
    private:
        void UpdateObjectSlotRoot(ObjectSlot slot);
    };

    class ObjectFieldCSetVisitor final : public BaseObjectVisitor<ObjectFieldCSetVisitor> {
    public:
        explicit ObjectFieldCSetVisitor(SharedGCEvacuator *evacuator) : evacuator_(evacuator) {}
        ~ObjectFieldCSetVisitor() override = default;

        void VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end, VisitObjectArea area) override;
    private:
        SharedGCEvacuator *evacuator_ {nullptr};
    };

    class Workload {
    public:
        Workload(SharedGCEvacuator *evacuator, Region *region) : evacuator_(evacuator), region_(region) {}
        virtual ~Workload() = default;
        virtual void Process(uint32_t threadIndex) = 0;
    protected:
        SharedGCEvacuator *evacuator_ {nullptr};
        Region *region_ {nullptr};
    };

    void AddWorkload(std::unique_ptr<Workload> workload)
    {
        workloads_.emplace_back(std::move(workload));
        totalTasks_++;
    }

    std::unique_ptr<Workload> GetWorkload()
    {
        std::unique_ptr<Workload> workload;
        size_t idx = taskIter_.fetch_add(1U, std::memory_order_relaxed);
        if (idx < totalTasks_) {
            workload = std::move(workloads_[idx]);
        }
        return workload;
    }

    void ProcessWorkloads(uint32_t threadIndex)
    {
        std::unique_ptr<Workload> workload = GetWorkload();
        while (workload != nullptr) {
            workload->Process(threadIndex);
            workload = GetWorkload();
        }
        if (threadIndex != MAIN_THREAD_INDEX) {
            LockHolder holder(lock_);
            if (--parallel_ <= 0) {
                condition_.SignalAll();
            }
        } else {
            WaitFinished();
            workloads_.clear();
            taskIter_ = 0;
            totalTasks_ = 0;
        }
    }

    class EvacuateRegionWorkload : public Workload {
    public:
        EvacuateRegionWorkload(SharedGCEvacuator *evacuator, Region *region, SharedHeap *sHeap, bool inHeapProfiler)
            : Workload(evacuator, region) {}
        void Process(uint32_t threadIndex) override;
    private:
        SharedHeap *sHeap_ {nullptr};
        bool inHeapProfiler_ {false};
    };

    class UpdateLocalReferenceWorkload : public Workload {
    public:
        UpdateLocalReferenceWorkload(SharedGCEvacuator *evacuator, Region *region) : Workload(evacuator, region) {}
        void Process(uint32_t threadIndex) override;
    };

    class UpdateSharedReferenceWorkload : public Workload {
    public:
        UpdateSharedReferenceWorkload(SharedGCEvacuator *evacuator, Region *region) : Workload(evacuator, region) {}
        void Process(uint32_t threadIndex) override;
    };

    class UpdateToRegionReferenceWorkload : public Workload {
    public:
        UpdateToRegionReferenceWorkload(SharedGCEvacuator *evacuator, Region *region) : Workload(evacuator, region) {}
        void Process(uint32_t threadIndex) override;
    };

    class ParallelTask : public common::Task {
    public:
        ParallelTask(int32_t id, SharedGCEvacuator *evacuator) : common::Task(id), evacuator_(evacuator) {};
        ~ParallelTask() override = default;
        NO_COPY_SEMANTIC(ParallelTask);
        NO_MOVE_SEMANTIC(ParallelTask);

        bool Run(uint32_t threadIndex) override;

    private:
        SharedGCEvacuator *evacuator_ {nullptr};
    };

    SharedHeap *sHeap_ {nullptr};
    std::array<SharedTlabAllocator*, common::MAX_TASKPOOL_THREAD_NUM + 1> sTlabs_ {};
    Mutex lock_;
    ConditionVariable condition_;
    std::vector<std::unique_ptr<Workload>> workloads_;
    std::atomic<size_t> taskIter_ {0};
    size_t totalTasks_ {0};
    UpdateRootVisitor rootVisitor_;
    ObjectFieldCSetVisitor objectFieldCSetVisitor_;
    int parallel_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_EVACUATOR_H
