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

#include <signal.h>
#include "ecmascript/base/config.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/linear_space.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/shared_heap/shared_space.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/work_manager.h"
#include "ecmascript/taskpool/taskpool.h"

namespace panda::ecmascript {
struct JsHeapDumpWork;
class ConcurrentMarker;
class ConcurrentSweeper;
class EcmaVM;
class FullGC;
class GCStats;
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
class SharedConcurrentSweeper;
class SharedGC;
class SharedGCMarker;
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

enum AppSensitiveStatus : uint8_t {
    NORMAL_SCENE,
    ENTER_HIGH_SENSITIVE,
    EXIT_HIGH_SENSITIVE,
};

enum class VerifyKind {
    VERIFY_PRE_GC,
    VERIFY_POST_GC,
    VERIFY_CONCURRENT_MARK_YOUNG,
    VERIFY_EVACUATE_YOUNG,
    VERIFY_CONCURRENT_MARK_FULL,
    VERIFY_EVACUATE_OLD,
    VERIFY_EVACUATE_FULL,
    VERIFY_SHARED_RSET_POST_FULL_GC,
    VERIFY_PRE_SHARED_GC,
    VERIFY_POST_SHARED_GC
};

class BaseHeap {
public:
    BaseHeap(const EcmaParamConfiguration &config) : config_(config) {}
    virtual ~BaseHeap() = default;
    NO_COPY_SEMANTIC(BaseHeap);
    NO_MOVE_SEMANTIC(BaseHeap);

    virtual bool IsMarking() const = 0;

    virtual bool IsReadyToMark() const = 0;

    virtual bool NeedStopCollection() = 0;

    virtual void TryTriggerConcurrentMarking() = 0;

    virtual void TryTriggerIdleCollection() = 0;

    virtual void TryTriggerIncrementalMarking() = 0;

    virtual bool OldSpaceExceedCapacity(size_t size) const = 0;

    virtual bool OldSpaceExceedLimit() const = 0;

    virtual inline size_t GetCommittedSize() const = 0;

    virtual inline size_t GetHeapObjectSize() const = 0;

    virtual void ChangeGCParams(bool inBackground) = 0;

    virtual const GlobalEnvConstants *GetGlobalConst() const = 0;

    virtual GCStats *GetEcmaGCStats() = 0;

    void SetMarkType(MarkType markType)
    {
        markType_ = markType;
    }

    bool IsFullMark() const
    {
        return markType_ == MarkType::MARK_FULL;
    }

    bool IsConcurrentFullMark() const
    {
        return markType_ == MarkType::MARK_FULL;
    }

    TriggerGCType GetGCType() const
    {
        return gcType_;
    }

    bool IsAlive(TaggedObject *object) const;

    bool ContainObject(TaggedObject *object) const;

    void SetOnSerializeEvent(bool isSerialize)
    {
        onSerializeEvent_ = isSerialize;
        if (!onSerializeEvent_ && !InSensitiveStatus()) {
            TryTriggerIncrementalMarking();
            TryTriggerIdleCollection();
            TryTriggerConcurrentMarking();
        }
    }

    bool GetOnSerializeEvent() const
    {
        return onSerializeEvent_;
    }

    bool GetOldGCRequested()
    {
        return oldGCRequested_;
    }

    EcmaParamConfiguration GetEcmaParamConfiguration() const
    {
        return config_;
    }

    NativeAreaAllocator *GetNativeAreaAllocator() const
    {
        return nativeAreaAllocator_;
    }

    HeapRegionAllocator *GetHeapRegionAllocator() const
    {
        return heapRegionAllocator_;
    }

    void ShouldThrowOOMError(bool shouldThrow)
    {
        shouldThrowOOMError_ = shouldThrow;
    }

    bool IsInBackground() const
    {
        return inBackground_;
    }

    // ONLY used for heap verification.
    bool IsVerifying() const
    {
        return isVerifying_;
    }

    // ONLY used for heap verification.
    void SetVerifying(bool verifying)
    {
        isVerifying_ = verifying;
    }

    void NotifyHeapAliveSizeAfterGC(size_t size)
    {
        heapAliveSizeAfterGC_ = size;
    }

    size_t GetHeapAliveSizeAfterGC() const
    {
        return heapAliveSizeAfterGC_;
    }

    bool InSensitiveStatus() const
    {
        return sensitiveStatus_.load(std::memory_order_relaxed) == AppSensitiveStatus::ENTER_HIGH_SENSITIVE
            || onStartupEvent_;
    }

    AppSensitiveStatus GetSensitiveStatus() const
    {
        return sensitiveStatus_.load(std::memory_order_relaxed);
    }

    bool onStartUpEvent() const
    {
        return onStartupEvent_;
    }

    void SetSensitiveStatus(AppSensitiveStatus status)
    {
        sensitiveStatus_.store(status, std::memory_order_release);;
    }

    bool CASSensitiveStatus(AppSensitiveStatus expect, AppSensitiveStatus status)
    {
        return sensitiveStatus_.compare_exchange_strong(expect, status, std::memory_order_seq_cst);
    }

    void NotifyPostFork()
    {
        LockHolder holder(finishColdStartMutex_);
        onStartupEvent_ = true;
        LOG_GC(INFO) << "SmartGC: enter app cold start";
    }

    // Whether should verify heap during gc.
    bool ShouldVerifyHeap() const
    {
        return shouldVerifyHeap_;
    }

    void ThrowOutOfMemoryErrorForDefault(JSThread *thread, size_t size, std::string functionName,
        bool NonMovableObjNearOOM = false);

    uint32_t GetMaxMarkTaskCount() const
    {
        return maxMarkTaskCount_;
    }

    bool CheckCanDistributeTask();
    void IncreaseTaskCount();
    void ReduceTaskCount();
    void WaitRunningTaskFinished();
    void WaitClearTaskFinished();
    void ThrowOutOfMemoryError(JSThread *thread, size_t size, std::string functionName,
        bool NonMovableObjNearOOM = false);

protected:
    void FatalOutOfMemoryError(size_t size, std::string functionName);

    const EcmaParamConfiguration config_;
    MarkType markType_ {MarkType::MARK_YOUNG};
    TriggerGCType gcType_ {TriggerGCType::YOUNG_GC};
    // Region allocators.
    NativeAreaAllocator *nativeAreaAllocator_ {nullptr};
    HeapRegionAllocator *heapRegionAllocator_ {nullptr};

    size_t heapAliveSizeAfterGC_ {0};
    size_t globalSpaceAllocLimit_ {0};
    // parallel marker task count.
    uint32_t runningTaskCount_ {0};
    uint32_t maxMarkTaskCount_ {0};
    Mutex waitTaskFinishedMutex_;
    ConditionVariable waitTaskFinishedCV_;
    Mutex waitClearTaskFinishedMutex_;
    ConditionVariable waitClearTaskFinishedCV_;
    bool clearTaskFinished_ {true};
    bool inBackground_ {false};
    bool shouldThrowOOMError_ {false};
    bool oldGCRequested_ {false};
    bool onSerializeEvent_ {false};
    std::atomic<AppSensitiveStatus> sensitiveStatus_ {AppSensitiveStatus::NORMAL_SCENE};
    bool onStartupEvent_ {false};
    Mutex finishColdStartMutex_;
    // ONLY used for heap verification.
    bool shouldVerifyHeap_ {false};
    bool isVerifying_ {false};
};

class SharedHeap : public BaseHeap {
public:
    SharedHeap(const EcmaParamConfiguration &config) : BaseHeap(config) {}
    virtual ~SharedHeap() = default;

    static SharedHeap* GetInstance()
    {
        EcmaParamConfiguration config(false, DEFAULT_HEAP_SIZE);
        static SharedHeap *shareHeap = new SharedHeap(config);
        return shareHeap;
    }

    void Initialize(NativeAreaAllocator *nativeAreaAllocator, HeapRegionAllocator *heapRegionAllocator,
        const JSRuntimeOptions &option);

    void PostInitialization(const GlobalEnvConstants *globalEnvConstants, const JSRuntimeOptions &option);

    void EnableParallelGC(JSRuntimeOptions &option);
    void DisableParallelGC();
    class ParallelMarkTask : public Task {
    public:
        ParallelMarkTask(int32_t id, SharedHeap *heap)
            : Task(id), sHeap_(heap) {};
        ~ParallelMarkTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(ParallelMarkTask);
        NO_MOVE_SEMANTIC(ParallelMarkTask);

    private:
        SharedHeap *sHeap_ {nullptr};
    };

    class AsyncClearTask : public Task {
    public:
        AsyncClearTask(int32_t id, SharedHeap *heap)
            : Task(id), sHeap_(heap) {}
        ~AsyncClearTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(AsyncClearTask);
        NO_MOVE_SEMANTIC(AsyncClearTask);
    private:
        SharedHeap *sHeap_;
    };
    bool IsMarking() const override
    {
        LOG_FULL(ERROR) << "SharedHeap IsMarking() not support yet";
        return false;
    }

    bool IsReadyToMark() const override
    {
        return true;
    }

    bool NeedStopCollection() override
    {
        LOG_FULL(ERROR) << "SharedHeap NeedStopCollection() not support yet";
        return onSerializeEvent_;
    }

    bool CheckAndTriggerOldGC(JSThread *thread, size_t size = 0);

    void TryTriggerConcurrentMarking() override
    {
        LOG_FULL(ERROR) << "SharedHeap TryTriggerConcurrentMarking() not support yet";
        return;
    }

    void TryTriggerIdleCollection() override
    {
        LOG_FULL(ERROR) << "SharedHeap TryTriggerIdleCollection() not support yet";
        return;
    }

    void TryTriggerIncrementalMarking() override
    {
        LOG_FULL(ERROR) << "SharedHeap TryTriggerIncrementalMarking() not support yet";
        return;
    }

    bool OldSpaceExceedCapacity(size_t size) const override
    {
        size_t totalSize = sOldSpace_->GetCommittedSize() + sHugeObjectSpace_->GetCommittedSize() + size;
        return totalSize >= sOldSpace_->GetMaximumCapacity() + sOldSpace_->GetOutOfMemoryOvershootSize();
    }

    bool OldSpaceExceedLimit() const override
    {
        size_t totalSize = sOldSpace_->GetHeapObjectSize() + sHugeObjectSpace_->GetHeapObjectSize();
        return totalSize >= sOldSpace_->GetInitialCapacity();
    }

    SharedConcurrentSweeper *GetSweeper() const
    {
        return sSweeper_;
    }

    bool IsParallelGCEnabled() const
    {
        return parallelGC_;
    }

    SharedOldSpace *GetOldSpace() const
    {
        return sOldSpace_;
    }

    SharedNonMovableSpace *GetNonMovableSpace() const
    {
        return sNonMovableSpace_;
    }

    SharedHugeObjectSpace *GetHugeObjectSpace() const
    {
        return sHugeObjectSpace_;
    }

    SharedReadOnlySpace *GetReadOnlySpace() const
    {
        return sReadOnlySpace_;
    }

    void CollectGarbage(JSThread *thread, TriggerGCType gcType, GCReason reason);

    void SetMaxMarkTaskCount(uint32_t maxTaskCount)
    {
        maxMarkTaskCount_ = maxTaskCount;
    }

    inline size_t GetCommittedSize() const override
    {
        size_t result = sOldSpace_->GetCommittedSize() +
            sHugeObjectSpace_->GetCommittedSize() +
            sNonMovableSpace_->GetCommittedSize() +
            sReadOnlySpace_->GetCommittedSize();
        return result;
    }

    inline size_t GetHeapObjectSize() const override
    {
        size_t result = sOldSpace_->GetHeapObjectSize() +
            sHugeObjectSpace_->GetHeapObjectSize() +
            sNonMovableSpace_->GetHeapObjectSize() +
            sReadOnlySpace_->GetCommittedSize();
        return result;
    }

    void ChangeGCParams([[maybe_unused]]bool inBackground) override
    {
        LOG_FULL(ERROR) << "SharedHeap ChangeGCParams() not support yet";
        return;
    }

    GCStats *GetEcmaGCStats() override
    {
        LOG_FULL(ERROR) << "SharedHeap GetEcmaGCStats() not support yet";
        return nullptr;
    }

    inline void SetGlobalEnvConstants(const GlobalEnvConstants *globalEnvConstants)
    {
        globalEnvConstants_ = globalEnvConstants;
    }

    inline const GlobalEnvConstants *GetGlobalConst() const override
    {
        return globalEnvConstants_;
    }

    SharedSparseSpace *GetSpaceWithType(MemSpaceType type) const
    {
        switch (type) {
            case MemSpaceType::SHARED_OLD_SPACE:
                return sOldSpace_;
            case MemSpaceType::SHARED_NON_MOVABLE:
                return sNonMovableSpace_;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
                break;
        }
    }

    void Prepare();
    void Reclaim();
    void PostGCMarkingTask();

    SharedGCWorkManager *GetWorkManager() const
    {
        return sWorkManager_;
    }

    SharedGCMarker *GetSharedGCMarker() const
    {
        return sharedGCMarker_;
    }

    void PrepareRecordRegionsForReclaim();

    template<class Callback>
    void EnumerateOldSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateOldSpaceRegionsWithRecord(const Callback &cb) const;

    inline TaggedObject *AllocateClassClass(JSHClass *hclass, size_t size);

    inline TaggedObject *AllocateNonMovableOrHugeObject(JSThread *thread, JSHClass *hclass);

    inline TaggedObject *AllocateNonMovableOrHugeObject(JSThread *thread, JSHClass *hclass, size_t size);

    inline TaggedObject *AllocateOldOrHugeObject(JSThread *thread, JSHClass *hclass);

    inline TaggedObject *AllocateOldOrHugeObject(JSThread *thread, JSHClass *hclass, size_t size);

    inline TaggedObject *AllocateOldOrHugeObject(JSThread *thread, size_t size);

    inline TaggedObject *AllocateHugeObject(JSThread *thread, JSHClass *hclass, size_t size);

    inline TaggedObject *AllocateHugeObject(JSThread *thread, size_t size);

    inline TaggedObject *AllocateReadOnlyOrHugeObject(JSThread *thread, JSHClass *hclass);

    inline TaggedObject *AllocateReadOnlyOrHugeObject(JSThread *thread, JSHClass *hclass, size_t size);

    size_t VerifyHeapObjects(VerifyKind verifyKind) const;
private:
    void ReclaimRegions();

    bool parallelGC_ {true};
    const GlobalEnvConstants *globalEnvConstants_ {nullptr};
    SharedOldSpace *sOldSpace_ {nullptr};
    SharedNonMovableSpace *sNonMovableSpace_ {nullptr};
    SharedReadOnlySpace *sReadOnlySpace_ {nullptr};
    SharedHugeObjectSpace *sHugeObjectSpace_ {nullptr};
    SharedGCWorkManager *sWorkManager_ {nullptr};
    SharedConcurrentSweeper *sSweeper_ {nullptr};
    SharedGC *sharedGC_ {nullptr};
    SharedGCMarker *sharedGCMarker_ {nullptr};
};

class Heap : public BaseHeap {
public:
    explicit Heap(EcmaVM *ecmaVm);
    virtual ~Heap() = default;
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

    HugeMachineCodeSpace *GetHugeMachineCodeSpace() const
    {
        return hugeMachineCodeSpace_;
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

    const GlobalEnvConstants *GetGlobalConst() const override
    {
        return thread_->GlobalConstants();
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
    inline TaggedObject *AllocateHugeMachineCodeObject(size_t size);
    // Snapshot
    inline uintptr_t AllocateSnapshotSpace(size_t size);

    /*
     * GC triggers.
     */
    void CollectGarbage(TriggerGCType gcType, GCReason reason = GCReason::OTHER);
    bool CheckAndTriggerOldGC(size_t size = 0);
    bool CheckAndTriggerHintGC();
    TriggerGCType SelectGCType() const;
    /*
     * Parallel GC related configurations and utilities.
     */

    void PostParallelGCTask(ParallelGCTaskPhase taskPhase);

    bool IsParallelGCEnabled() const
    {
        return parallelGC_;
    }
    void ChangeGCParams(bool inBackground) override;

    GCStats *GetEcmaGCStats() override;
    
    JSObjectResizingStrategy *GetJSObjectResizingStrategy();

    void TriggerIdleCollection(int idleMicroSec);
    void NotifyMemoryPressure(bool inHighMemoryPressure);

    void TryTriggerConcurrentMarking() override;
    void AdjustBySurvivalRate(size_t originalNewSpaceSize);
    void TriggerConcurrentMarking();
    bool CheckCanTriggerConcurrentMarking();

    void TryTriggerIdleCollection() override;
    void TryTriggerIncrementalMarking() override;
    void CalculateIdleDuration();
    void UpdateWorkManager(WorkManager *workManager);
    /*
     * Wait for existing concurrent marking tasks to be finished (if any).
     * Return true if there's ongoing concurrent marking.
     */
    bool CheckOngoingConcurrentMarking();

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

    size_t CalculateLinearSpaceOverShoot()
    {
        return oldSpace_->GetMaximumCapacity() - oldSpace_->GetInitialCapacity();
    }

    inline size_t GetCommittedSize() const override;

    inline size_t GetHeapObjectSize() const override;

    size_t GetRegionCachedSize() const
    {
        return activeSemiSpace_->GetInitialCapacity();
    }

    size_t GetLiveObjectSize() const;

    inline uint32_t GetHeapObjectCount() const;

    size_t GetPromotedSize() const
    {
        return promotedSize_;
    }

    size_t GetArrayBufferSize() const;

    size_t GetHeapLimitSize() const;

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

    bool IsEmptyIdleTask()
    {
        return idleTask_ == IdleTaskType::NO_TASK;
    }

    void NotifyFinishColdStart(bool isMainThread = true);

    void NotifyFinishColdStartSoon();

    void NotifyHighSensitive(bool isStart);

    void HandleExitHighSensitiveEvent();

    bool ObjectExceedMaxHeapSize() const;

    bool NeedStopCollection() override;

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

    size_t VerifyHeapObjects(VerifyKind verifyKind = VerifyKind::VERIFY_PRE_GC) const;
    size_t VerifyOldToNewRSet(VerifyKind verifyKind = VerifyKind::VERIFY_PRE_GC) const;
    void StatisticHeapObject(TriggerGCType gcType) const;
    void StatisticHeapDetail() const;
    void PrintHeapInfo(TriggerGCType gcType) const;

    bool OldSpaceExceedCapacity(size_t size) const override
    {
        size_t totalSize = oldSpace_->GetCommittedSize() + hugeObjectSpace_->GetCommittedSize() + size;
        return totalSize >= oldSpace_->GetMaximumCapacity() + oldSpace_->GetOvershootSize() +
               oldSpace_->GetOutOfMemoryOvershootSize();
    }

    bool OldSpaceExceedLimit() const override
    {
        size_t totalSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetHeapObjectSize();
        return totalSize >= oldSpace_->GetInitialCapacity() + oldSpace_->GetOvershootSize();
    }

    void AdjustSpaceSizeForAppSpawn();

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

    void SetHeapMode(HeapMode mode)
    {
        mode_ = mode;
    }

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

    void TryTriggerFullMarkByNativeSize();

    bool IsMarking() const override
    {
        return thread_->IsMarking();
    }

    bool IsReadyToMark() const override
    {
        return thread_->IsReadyToMark();
    }

    bool IsYoungGC() const
    {
        return gcType_ == TriggerGCType::YOUNG_GC;
    }

    void CheckNonMovableSpaceOOM();
    std::tuple<uint64_t, uint8_t *, int, kungfu::CalleeRegAndOffsetVec> CalCallSiteInfo(uintptr_t retAddr) const;

private:
    static constexpr int IDLE_TIME_LIMIT = 10;  // if idle time over 10ms we can do something
    static constexpr int ALLOCATE_SIZE_LIMIT = 100_KB;
    static constexpr int IDLE_MAINTAIN_TIME = 500;
    static constexpr int BACKGROUND_GROW_LIMIT = 2_MB;
    // Threadshold that HintGC will actually trigger GC.
    static constexpr double SURVIVAL_RATE_THRESHOLD = 0.5;
    void RecomputeLimits();
    void AdjustOldSpaceLimit();
    // record lastRegion for each space, which will be used in ReclaimRegions()
    void PrepareRecordRegionsForReclaim();
    void DumpHeapSnapshotBeforeOOM(bool isFullGC, size_t size, std::string functionName, bool NonMovableObjNearOOM);
    inline void ReclaimRegions(TriggerGCType gcType);
    inline size_t CalculateCommittedCacheSize();
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

    class FinishColdStartTask : public Task {
    public:
        FinishColdStartTask(int32_t id, Heap *heap)
            : Task(id), heap_(heap) {}
        ~FinishColdStartTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(FinishColdStartTask);
        NO_MOVE_SEMANTIC(FinishColdStartTask);
    private:
        Heap *heap_;
    };

    class RecursionScope {
    public:
        explicit RecursionScope(Heap* heap) : heap_(heap)
        {
            if (heap_->recursionDepth_++ != 0) {
                LOG_GC(FATAL) << "Recursion in HeapCollectGarbage Constructor, depth: " << heap_->recursionDepth_;
            }
        }
        ~RecursionScope()
        {
            if (--heap_->recursionDepth_ != 0) {
                LOG_GC(FATAL) << "Recursion in HeapCollectGarbage Destructor, depth: " << heap_->recursionDepth_;
            }
        }
    private:
        Heap* heap_ {nullptr};
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
    HugeMachineCodeSpace *hugeMachineCodeSpace_ {nullptr};
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

    bool parallelGC_ {true};
    bool fullGCRequested_ {false};
    bool fullMarkRequested_ {false};
    bool oldSpaceLimitAdjusted_ {false};
    bool enableIdleGC_ {false};
    HeapMode mode_ { HeapMode::NORMAL };

    /*
     * The memory controller providing memory statistics (by allocations and coleections),
     * which is used for GC heuristics.
     */
    MemController *memController_ {nullptr};
    size_t promotedSize_ {0};
    size_t semiSpaceCopiedSize_ {0};
    size_t nativeBindingSize_{0};
    size_t globalSpaceNativeLimit_ {0};
    MemGrowingType memGrowingtype_ {MemGrowingType::HIGH_THROUGHPUT};

    // parallel evacuator task number.
    uint32_t maxEvacuateTaskCount_ {0};
    Mutex waitTaskFinishedMutex_;
    ConditionVariable waitTaskFinishedCV_;

    // Application status

    IdleNotifyStatusCallback notifyIdleStatusCallback {nullptr};

    IdleTaskType idleTask_ {IdleTaskType::NO_TASK};
    float idlePredictDuration_ {0.0f};
    double idleTaskFinishTime_ {0.0};
    int32_t recursionDepth_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_H
