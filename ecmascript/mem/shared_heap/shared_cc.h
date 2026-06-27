/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CC_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CC_H

#include <memory>
#include <vector>

#include "ecmascript/daemon/daemon_thread.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/shared_heap/shared_gc_marker.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"
#include "ecmascript/mem/tlab_allocator.h"

namespace panda::ecmascript {
class SharedHeap;
class SharedCCEvacuator;
class SharedCCCopyTask;

class SharedCCRootVisitor final : public RootVisitor {
public:
    explicit SharedCCRootVisitor(SharedCCEvacuator *evacuator) : evacuator_(evacuator) {}
    ~SharedCCRootVisitor() override = default;
    NO_COPY_SEMANTIC(SharedCCRootVisitor);
    NO_MOVE_SEMANTIC(SharedCCRootVisitor);

    void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override;
    void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override;
    void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                 uintptr_t baseOldObject) override;

private:
    void UpdateObjectSlotRoot(ObjectSlot slot);
    SharedCCEvacuator *evacuator_;
};

struct ThreadWorkload {
    JSThread *thread{nullptr};
    std::vector<Region*> localRegions;
    std::atomic<int> nextIndex_{-1};
    int remainItems_{0};
    Mutex mutex_;
    ConditionVariable cv_;
};

struct EvacuatorEntry {
    JSThread *thread{nullptr};
    SharedCCEvacuator *evacuator{nullptr};
};

class SharedCC {
public:
    explicit SharedCC(SharedHeap *heap);
    ~SharedCC() = default;

    NO_COPY_SEMANTIC(SharedCC);
    NO_MOVE_SEMANTIC(SharedCC);

    void Trigger(GCReason gcReason);
    void RunPhases(GCReason gcReason);

    SharedHeap *GetHeap() const
    {
        return sHeap_;
    }
    bool IsRunning() const
    {
        return ccRunning_;
    }
    PUBLIC_API void WaitFinished();

    void NotifyMainThreadReady();
    void PrepareNewThread(JSThread *thread);
    void SkipThreadWorkload(JSThread *thread);

private:
    // GC Phases (called from RunPhases)
    void ConcurrentMark();
    void ReMarkAndPrepare(GCReason gcReason);
    void FinalizeAndReclaim();
    void Finish();
    void VerifyHeap();

    // Copy & Reference Update
    void ParallelCopy();
    void UpdateReferences();
    void UpdateRoot();
    void OnCopyFinished();
    void OnUpdateFinished();
    void WaitUpdateFinished();
    void ProcessWeakReference();

    // Preparation
    void PrepareAllThreads();
    void WaitMainThreadReady();
    void PrepareForCopy();
    void CollectUpdateRegions();
    void SuspendIdleThreads();

    // Thread State Management
    void EnterSharedGCScope();
    void ExitSharedGCScope();
    void RestoreThreadStates();

    // Evacuators
    void InstallSharedCCEvacuators();
    void FinalizeSharedCCEvacuators();
    void FinalizeCopy();

    // String Table
    void PostStringTableSweepTask();
    void WaitStringTableSweep();
    void FinishConcurrentStringTableSweep();
    void SetStringTableCopyOrSweeping(bool enabled);

    // Helpers
    void UpdateRecordWeakReference(SharedCCEvacuator &evacuator);
    void LogThreadStatesBeforeCopy();
    void EstimatePostCCSize();
    int CalculateCopyThreadNum();
    int CalculateUpdateThreadNum();
    SharedTlabAllocator *GetTlabAllocator(uint32_t threadIndex)
    {
        return tlabAllocators_[threadIndex].get();
    }

    SharedHeap *sHeap_{nullptr};
    DaemonThread *dThread_{nullptr};
    SharedConcurrentMarker *marker_{nullptr};

    bool ccRunning_{false};
    std::vector<std::unique_ptr<SharedTlabAllocator>> tlabAllocators_;
    std::atomic<size_t> runningTaskCount_{0};
    Mutex waitMutex_;
    ConditionVariable waitCV_;

    std::vector<Region*> copyTasks_{};
    std::atomic<size_t> taskIter_{0};

    std::vector<ThreadWorkload*> threadWorkloads_;
    std::vector<Region*> sharedWorkloads_;
    std::atomic<size_t> sharedIter_{0};

    Mutex evacuatorsMutex_;
    std::vector<EvacuatorEntry> evacuators_;

    friend class SharedCCCopyTask;
    friend class SharedCCUpdateTask;
};

class SharedCCCopyTask : public common::Task {
public:
    SharedCCCopyTask(std::vector<Region*> &tasks, std::atomic<size_t> &taskIter,
                     std::atomic<size_t> &runningTaskCount, SharedCC *cc)
        : Task(0), tasks_(tasks), taskIter_(taskIter), runningTaskCount_(runningTaskCount),
          cc_(cc), totalSize_(tasks_.size()) {}
    ~SharedCCCopyTask() override = default;

    bool Run(uint32_t threadIndex) override;

    NO_COPY_SEMANTIC(SharedCCCopyTask);
    NO_MOVE_SEMANTIC(SharedCCCopyTask);

private:
    Region *GetNextTask();
    std::vector<Region*> &tasks_;
    std::atomic<size_t> &taskIter_;
    std::atomic<size_t> &runningTaskCount_;
    SharedCC *cc_{nullptr};
    size_t totalSize_{0};
};

class SharedCCUpdateTask : public common::Task {
public:
    SharedCCUpdateTask(SharedCC *cc, std::atomic<size_t> &runningTaskCount)
        : Task(0), cc_(cc), runningTaskCount_(runningTaskCount) {}
    ~SharedCCUpdateTask() override = default;

    bool Run(uint32_t threadIndex) override;

    NO_COPY_SEMANTIC(SharedCCUpdateTask);
    NO_MOVE_SEMANTIC(SharedCCUpdateTask);

private:
    SharedCC *cc_{nullptr};
    std::atomic<size_t> &runningTaskCount_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CC_H
