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

#ifndef ECMASCRIPT_MEM_HEAP_H
#define ECMASCRIPT_MEM_HEAP_H

#include "ecmascript/base/config.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/linear_space.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/work_manager.h"
#include "ecmascript/taskpool/taskpool.h"

namespace panda::ecmascript {
class ConcurrentMarker;
class ConcurrentSweeper;
class EcmaVM;
class FullGC;
class HeapRegionAllocator;
class HeapTracker;
#if !WIN_OR_MAC_OR_IOS_PLATFORM
class HeapProfilerInterface;
class HeapProfiler;
#endif
class IncrementalMarker;
class JSNativePointer;
class Marker;
class MemController;
class NativeAreaAllocator;
class ParallelEvacuator;
class PartialGC;
class STWYoungGC;

using IdleNotifyStatusCallback = std::function<void(bool)>;

enum class IdleTaskType : uint8_t {
    NO_TASK,
    YOUNG_GC,
    FINISH_MARKING,
    INCREMENTAL_MARK
};

enum class MarkType : uint8_t {
    MARK_YOUNG,
    MARK_FULL
};

enum class MemGrowingType : uint8_t {
    HIGH_THROUGHPUT,
    CONSERVATIVE,
    PRESSURE
};

enum class HeapMode {
    NORMAL,
    SPAWN,
    SHARE,
};

class Heap {
public:
    explicit Heap(EcmaVM *ecmaVm);
    ~Heap() = default;
    NO_COPY_SEMANTIC(Heap);
    NO_MOVE_SEMANTIC(Heap);
    void Initialize();
    void Destroy();
    void Prepare();
    void Resume(TriggerGCType gcType);
    void ResumeForAppSpawn();
    void CompactHeapBeforeFork();
    void DisableParallelGC();
    void EnableParallelGC();
    // fixme: Rename NewSpace to YoungSpace.
    // This is the active young generation space that the new objects are allocated in
    // or copied into (from the other semi space) during semi space GC.
    SemiSpace *GetNewSpace() const
    {
        return activeSemiSpace_;
    }

    /*
     * Return the original active space where the objects are to be evacuated during semi space GC.
     * This should be invoked only in the evacuation phase of semi space GC.
     * fixme: Get rid of this interface or make it safe considering the above implicit limitation / requirement.
     */
    SemiSpace *GetFromSpaceDuringEvacuation() const
    {
        return inactiveSemiSpace_;
    }

    OldSpace *GetOldSpace() const
    {
        return oldSpace_;
    }

    NonMovableSpace *GetNonMovableSpace() const
    {
        return nonMovableSpace_;
    }

    HugeObjectSpace *GetHugeObjectSpace() const
    {
        return hugeObjectSpace_;
    }

    MachineCodeSpace *GetMachineCodeSpace() const
    {
        return machineCodeSpace_;
    }

    SnapshotSpace *GetSnapshotSpace() const
    {
        return snapshotSpace_;
    }

    ReadOnlySpace *GetReadOnlySpace() const
    {
        return readOnlySpace_;
    }

    AppSpawnSpace *GetAppSpawnSpace() const
    {
        return appSpawnSpace_;
    }

    SparseSpace *GetSpaceWithType(MemSpaceType type) const
    {
        switch (type) {
            case MemSpaceType::OLD_SPACE:
                return oldSpace_;
            case MemSpaceType::NON_MOVABLE:
                return nonMovableSpace_;
            case MemSpaceType::MACHINE_CODE_SPACE:
                return machineCodeSpace_;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
                break;
        }
    }

    STWYoungGC *GetSTWYoungGC() const
    {
        return stwYoungGC_;
    }

    PartialGC *GetPartialGC() const
    {
        return partialGC_;
    }

    FullGC *GetFullGC() const
    {
        return fullGC_;
    }

    ConcurrentSweeper *GetSweeper() const
    {
        return sweeper_;
    }

    ParallelEvacuator *GetEvacuator() const
    {
        return evacuator_;
    }

    ConcurrentMarker *GetConcurrentMarker() const
    {
        return concurrentMarker_;
    }

    IncrementalMarker *GetIncrementalMarker() const
    {
        return incrementalMarker_;
    }

    Marker *GetNonMovableMarker() const
    {
        return nonMovableMarker_;
    }

    Marker *GetSemiGCMarker() const
    {
        return semiGCMarker_;
    }

    Marker *GetCompressGCMarker() const
    {
        return compressGCMarker_;
    }

    EcmaVM *GetEcmaVM() const
    {
        return ecmaVm_;
    }

    JSThread *GetJSThread() const
    {
        return thread_;
    }

    WorkManager *GetWorkManager() const
    {
        return workManager_;
    }

    MemController *GetMemController() const
    {
        return memController_;
    }

    /*
     * For object allocations.
     */

    // Young
    inline TaggedObject *AllocateYoungOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateYoungOrHugeObject(JSHClass *hclass, size_t size);
    inline TaggedObject *AllocateReadOnlyOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateReadOnlyOrHugeObject(JSHClass *hclass, size_t size);
    inline TaggedObject *AllocateYoungOrHugeObject(size_t size);
    inline uintptr_t AllocateYoungSync(size_t size);
    inline TaggedObject *TryAllocateYoungGeneration(JSHClass *hclass, size_t size);
    // Old
    inline TaggedObject *AllocateOldOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateOldOrHugeObject(JSHClass *hclass, size_t size);
    // Non-movable
    inline TaggedObject *AllocateNonMovableOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateNonMovableOrHugeObject(JSHClass *hclass, size_t size);
    inline TaggedObject *AllocateClassClass(JSHClass *hclass, size_t size);
    // Huge
    inline TaggedObject *AllocateHugeObject(JSHClass *hclass, size_t size);
    inline TaggedObject *AllocateHugeObject(size_t size);
    // Machine code
    inline TaggedObject *AllocateMachineCodeObject(JSHClass *hclass, size_t size);
    // Snapshot
    inline uintptr_t AllocateSnapshotSpace(size_t size);

    NativeAreaAllocator *GetNativeAreaAllocator() const
    {
        return nativeAreaAllocator_;
    }

    HeapRegionAllocator *GetHeapRegionAllocator() const
    {
        return heapRegionAllocator_;
    }

    /*
     * GC triggers.
     */

    void CollectGarbage(TriggerGCType gcType, GCReason reason = GCReason::OTHER);

    bool CheckAndTriggerOldGC(size_t size = 0);
    TriggerGCType SelectGCType() const;
    /*
     * Parallel GC related configurations and utilities.
     */

    void PostParallelGCTask(ParallelGCTaskPhase taskPhase);

    bool IsParallelGCEnabled() const
    {
        return parallelGC_;
    }
    void ChangeGCParams(bool inBackground);
    void TriggerIdleCollection(int idleMicroSec);
    void NotifyMemoryPressure(bool inHighMemoryPressure);
    bool CheckCanDistributeTask();

    void WaitRunningTaskFinished();

    void TryTriggerConcurrentMarking();
    void AdjustBySurvivalRate(size_t originalNewSpaceSize);
    void TriggerConcurrentMarking();

    void TryTriggerIdleCollection();
    void TryTriggerIncrementalMarking();
    void CalculateIdleDuration();

    /*
     * Wait for existing concurrent marking tasks to be finished (if any).
     * Return true if there's ongoing concurrent marking.
     */
    bool CheckOngoingConcurrentMarking();

    /*
     * Functions invoked during GC.
     */

    void SetMarkType(MarkType markType)
    {
        markType_ = markType;
    }

    bool IsFullMark() const
    {
        return markType_ == MarkType::MARK_FULL;
    }

    inline void SwapNewSpace();
    inline void SwapOldSpace();

    inline bool MoveYoungRegionSync(Region *region);
    inline void MergeToOldSpaceSync(LocalSpace *localSpace);

    template<class Callback>
    void EnumerateOldSpaceRegions(const Callback &cb, Region *region = nullptr) const;

    template<class Callback>
    void EnumerateNonNewSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateNonNewSpaceRegionsWithRecord(const Callback &cb) const;

    template<class Callback>
    void EnumerateNewSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateSnapshotSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateNonMovableRegions(const Callback &cb) const;

    template<class Callback>
    inline void EnumerateRegions(const Callback &cb) const;

    inline void ClearSlotsRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd);

    void WaitAllTasksFinished();
    void WaitConcurrentMarkingFinished();

    MemGrowingType GetMemGrowingType() const
    {
        return memGrowingtype_;
    }

    void SetMemGrowingType(MemGrowingType memGrowingType)
    {
        memGrowingtype_ = memGrowingType;
    }

    inline size_t GetCommittedSize() const;

    inline size_t GetHeapObjectSize() const;

    inline uint32_t GetHeapObjectCount() const;

    size_t GetPromotedSize() const
    {
        return promotedSize_;
    }

    size_t GetArrayBufferSize() const;

    uint32_t GetMaxMarkTaskCount() const
    {
        return maxMarkTaskCount_;
    }

    uint32_t GetMaxEvacuateTaskCount() const
    {
        return maxEvacuateTaskCount_;
    }

    /*
     * Receive callback function to control idletime.
     */
    inline void InitializeIdleStatusControl(std::function<void(bool)> callback);

    void DisableNotifyIdle()
    {
        if (notifyIdleStatusCallback != nullptr) {
            notifyIdleStatusCallback(true);
        }
    }

    void EnableNotifyIdle()
    {
        if (enableIdleGC_ && notifyIdleStatusCallback != nullptr) {
            notifyIdleStatusCallback(false);
        }
    }

    void SetIdleTask(IdleTaskType task)
    {
        idleTask_ = task;
    }

    void ClearIdleTask();

#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    void StartHeapTracking()
    {
        WaitAllTasksFinished();
    }

    void StopHeapTracking()
    {
        WaitAllTasksFinished();
    }
#endif
    void OnAllocateEvent(TaggedObject* address, size_t size);
    void OnMoveEvent(uintptr_t address, TaggedObject* forwardAddress, size_t size);
    void AddToKeptObjects(JSHandle<JSTaggedValue> value) const;
    void ClearKeptObjects() const;

    // add allocationInspector to each space
    void AddAllocationInspectorToAllSpaces(AllocationInspector *inspector);

    // clear allocationInspector from each space
    void ClearAllocationInspectorFromAllSpaces();

    /*
     * Funtions used by heap verification.
     */

    template<class Callback>
    void IterateOverObjects(const Callback &cb) const;

    bool IsAlive(TaggedObject *object) const;
    bool ContainObject(TaggedObject *object) const;

    size_t VerifyHeapObjects() const;
    size_t VerifyOldToNewRSet() const;
    void StatisticHeapObject(TriggerGCType gcType) const;
    void PrintHeapInfo(TriggerGCType gcType) const;

    bool OldSpaceExceedCapacity(size_t size) const
    {
        size_t totalSize = oldSpace_->GetCommittedSize() + hugeObjectSpace_->GetCommittedSize() + size;
        return totalSize >= oldSpace_->GetMaximumCapacity() + oldSpace_->GetOvershootSize() +
               oldSpace_->GetOutOfMemoryOvershootSize();
    }

    bool OldSpaceExceedLimit() const
    {
        size_t totalSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetHeapObjectSize();
        return totalSize >= oldSpace_->GetInitialCapacity() + oldSpace_->GetOvershootSize();
    }

    void AdjustSpaceSizeForAppSpawn();
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    bool IsVerifying() const
    {
        return isVerifying_;
    }
#endif
    static bool ShouldMoveToRoSpace(JSHClass *hclass, TaggedObject *object)
    {
        return hclass->IsString() && !Region::ObjectAddressToRange(object)->InHugeObjectSpace();
    }

    bool IsFullMarkRequested() const
    {
        return fullMarkRequested_;
    }

    void SetFullMarkRequestedState(bool fullMarkRequested)
    {
        fullMarkRequested_ = fullMarkRequested;
    }

    void ShouldThrowOOMError(bool shouldThrow)
    {
        shouldThrowOOMError_ = shouldThrow;
    }

    void SetHeapMode(HeapMode mode)
    {
        mode_ = mode;
    }

    void ThrowOutOfMemoryError(size_t size, std::string functionName, bool NonMovableObjNearOOM = false);

    void IncreaseNativeBindingSize(size_t size);
    void IncreaseNativeBindingSize(JSNativePointer *object);
    void ResetNativeBindingSize()
    {
        nativeBindingSize_ = 0;
    }

    size_t GetNativeBindingSize() const
    {
        return nativeBindingSize_;
    }

    size_t GetGlobalNativeSize() const
    {
        return GetNativeBindingSize() + nativeAreaAllocator_->GetNativeMemoryUsage();
    }

    bool GlobalNativeSizeLargerThanLimit() const
    {
        return GetGlobalNativeSize() >= globalSpaceNativeLimit_;
    }

    void NotifyHeapAliveSizeAfterGC(size_t size)
    {
        heapAliveSizeAfterGC_ = size;
    }

    size_t GetHeapAliveSizeAfterGC() const
    {
        return heapAliveSizeAfterGC_;
    }

    bool IsInBackground() const
    {
        return inBackground_;
    }

    bool IsYoungGC() const
    {
        return gcType_ == TriggerGCType::YOUNG_GC;
    }

    bool GetOldGCRequested()
    {
        return oldGCRequested_;
    }

    void CheckNonMovableSpaceOOM()
    {
        if (nonMovableSpace_->GetHeapObjectSize() > MAX_NONMOVABLE_LIVE_OBJ_SIZE) {
            ThrowOutOfMemoryError(nonMovableSpace_->GetHeapObjectSize(), "Heap::CheckNonMovableSpaceOOM", true);
        }
    }
private:
    static constexpr int IDLE_TIME_LIMIT = 10;  // if idle time over 10ms we can do something
    static constexpr int ALLOCATE_SIZE_LIMIT = 100_KB;
    static constexpr int IDLE_MAINTAIN_TIME = 500;
    static constexpr int BACKGROUND_GROW_LIMIT = 2_MB;
    void FatalOutOfMemoryError(size_t size, std::string functionName);
    void RecomputeLimits();
    void AdjustOldSpaceLimit();
    // record lastRegion for each space, which will be used in ReclaimRegions()
    void PrepareRecordRegionsForReclaim();
    void IncreaseTaskCount();
    void ReduceTaskCount();
    void WaitClearTaskFinished();
    void InvokeWeakNodeNativeFinalizeCallback();
    inline void ReclaimRegions(TriggerGCType gcType);

    class ParallelGCTask : public Task {
    public:
        ParallelGCTask(int32_t id, Heap *heap, ParallelGCTaskPhase taskPhase)
            : Task(id), heap_(heap), taskPhase_(taskPhase) {};
        ~ParallelGCTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(ParallelGCTask);
        NO_MOVE_SEMANTIC(ParallelGCTask);

    private:
        Heap *heap_ {nullptr};
        ParallelGCTaskPhase taskPhase_;
    };

    class AsyncClearTask : public Task {
    public:
        AsyncClearTask(int32_t id, Heap *heap, TriggerGCType type)
            : Task(id), heap_(heap), gcType_(type) {}
        ~AsyncClearTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(AsyncClearTask);
        NO_MOVE_SEMANTIC(AsyncClearTask);
    private:
        Heap *heap_;
        TriggerGCType gcType_;
    };

    EcmaVM *ecmaVm_ {nullptr};
    JSThread *thread_ {nullptr};

    /*
     * Heap spaces.
     */

    /*
     * Young generation spaces where most new objects are allocated.
     * (only one of the spaces is active at a time in semi space GC).
     */
    SemiSpace *activeSemiSpace_ {nullptr};
    SemiSpace *inactiveSemiSpace_ {nullptr};

    // Old generation spaces where some long living objects are allocated or promoted.
    OldSpace *oldSpace_ {nullptr};
    OldSpace *compressSpace_ {nullptr};
    ReadOnlySpace *readOnlySpace_ {nullptr};
    AppSpawnSpace *appSpawnSpace_ {nullptr};
    // Spaces used for special kinds of objects.
    NonMovableSpace *nonMovableSpace_ {nullptr};
    MachineCodeSpace *machineCodeSpace_ {nullptr};
    HugeObjectSpace *hugeObjectSpace_ {nullptr};
    SnapshotSpace *snapshotSpace_ {nullptr};

    /*
     * Garbage collectors collecting garbage in different scopes.
     */

    /*
     * Semi sapce GC which collects garbage only in young spaces.
     * This is however optional for now because the partial GC also covers its functionality.
     */
    STWYoungGC *stwYoungGC_ {nullptr};

    /*
     * The mostly used partial GC which collects garbage in young spaces,
     * and part of old spaces if needed determined by GC heuristics.
     */
    PartialGC *partialGC_ {nullptr};

    // Full collector which collects garbage in all valid heap spaces.
    FullGC *fullGC_ {nullptr};

    // Concurrent marker which coordinates actions of GC markers and mutators.
    ConcurrentMarker *concurrentMarker_ {nullptr};

    // Concurrent sweeper which coordinates actions of sweepers (in spaces excluding young semi spaces) and mutators.
    ConcurrentSweeper *sweeper_ {nullptr};

    // Parallel evacuator which evacuates objects from one space to another one.
    ParallelEvacuator *evacuator_ {nullptr};

    // Incremental marker which coordinates actions of GC markers in idle time.
    IncrementalMarker *incrementalMarker_ {nullptr};

    /*
     * Different kinds of markers used by different collectors.
     * Depending on the collector algorithm, some markers can do simple marking
     *  while some others need to handle object movement.
     */
    Marker *nonMovableMarker_ {nullptr};
    Marker *semiGCMarker_ {nullptr};
    Marker *compressGCMarker_ {nullptr};

    // Work manager managing the tasks mostly generated in the GC mark phase.
    WorkManager *workManager_ {nullptr};

    MarkType markType_ {MarkType::MARK_YOUNG};

    bool parallelGC_ {true};
    bool fullGCRequested_ {false};
    bool oldGCRequested_ {false};
    bool fullMarkRequested_ {false};
    bool oldSpaceLimitAdjusted_ {false};
    bool shouldThrowOOMError_ {false};
    bool runningNativeFinalizeCallbacks_ {false};
    bool enableIdleGC_ {false};
    HeapMode mode_ { HeapMode::NORMAL };

    size_t globalSpaceAllocLimit_ {0};
    size_t promotedSize_ {0};
    size_t semiSpaceCopiedSize_ {0};
    size_t nativeBindingSize_{0};
    size_t globalSpaceNativeLimit_ {0};
    MemGrowingType memGrowingtype_ {MemGrowingType::HIGH_THROUGHPUT};
    TriggerGCType gcType_ {TriggerGCType::YOUNG_GC};

    bool clearTaskFinished_ {true};
    os::memory::Mutex waitClearTaskFinishedMutex_;
    os::memory::ConditionVariable waitClearTaskFinishedCV_;
    uint32_t runningTaskCount_ {0};
    // parallel marker task number.
    uint32_t maxMarkTaskCount_ {0};
    // parallel evacuator task number.
    uint32_t maxEvacuateTaskCount_ {0};
    os::memory::Mutex waitTaskFinishedMutex_;
    os::memory::ConditionVariable waitTaskFinishedCV_;

    /*
     * The memory controller providing memory statistics (by allocations and coleections),
     * which is used for GC heuristics.
     */
    MemController *memController_ {nullptr};

    // Region allocators.
    NativeAreaAllocator *nativeAreaAllocator_ {nullptr};
    HeapRegionAllocator *heapRegionAllocator_ {nullptr};

    // Application status
    bool inBackground_ {false};

    IdleNotifyStatusCallback notifyIdleStatusCallback {nullptr};

    IdleTaskType idleTask_ {IdleTaskType::NO_TASK};
    float idlePredictDuration_ {0.0f};
    size_t heapAliveSizeAfterGC_ {0};
    double idleTaskFinishTime_ {0.0};
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    bool isVerifying_ {false};
#endif
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_H
