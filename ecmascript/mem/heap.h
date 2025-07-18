/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "common_components/taskpool/task.h"
#include "ecmascript/base/config.h"
#include "ecmascript/daemon/daemon_thread.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_object_resizing_strategy.h"
#include "ecmascript/mem/linear_space.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/shared_heap/shared_space.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/work_manager.h"
#include "ecmascript/napi/include/jsnapi_expo.h"

namespace panda::test {
class GCTest_CallbackTask_Test;
class HProfTestHelper;
class HeapTestHelper;
}

namespace panda::ecmascript {
class ConcurrentMarker;
class ConcurrentSweeper;
class EcmaVM;
class FullGC;
class GCStats;
class GCKeyStats;
class HeapRegionAllocator;
class HeapTracker;
#if !WIN_OR_MAC_OR_IOS_PLATFORM
class HeapProfilerInterface;
class HeapProfiler;
#endif
class IncrementalMarker;
class JSNativePointer;
class Marker;
class UnifiedGC;
class UnifiedGCMarker;
class MemController;
class IdleGCTrigger;
class NativeAreaAllocator;
class ParallelEvacuator;
class PartialGC;
class RSetWorkListHandler;
class SharedConcurrentMarker;
class SharedConcurrentSweeper;
class SharedGC;
class SharedGCEvacuator;
class SharedGCMarkerBase;
class SharedGCMarker;
class SharedFullGC;
class SharedGCMovableMarker;
class ThreadLocalAllocationBuffer;
class JSThread;
class DaemonThread;
class GlobalEnvConstants;
class SharedMemController;
class IdleGCTrigger;

enum ThreadType : uint8_t;

using namespace panda;
using IdleNotifyStatusCallback = std::function<void(bool)>;
using FinishGCListener = void (*)(void *);
using GCListenerId = std::vector<std::pair<FinishGCListener, void *>>::const_iterator;
using Clock = std::chrono::high_resolution_clock;
using AppFreezeFilterCallback = std::function<bool(const int32_t pid, const bool needDecreaseQuota)>;
using BytesAndDuration = std::pair<uint64_t, double>;
using MemoryReduceDegree = panda::JSNApi::MemoryReduceDegree;
using NativePointerList = CVector<JSTaggedValue>;
enum class IdleTaskType : uint8_t {
    NO_TASK,
    YOUNG_GC,
    FINISH_MARKING,
    INCREMENTAL_MARK
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

enum class StartupStatus : uint8_t {
    BEFORE_STARTUP,
    ON_STARTUP,
    JUST_FINISH_STARTUP,
    FINISH_STARTUP
};

enum class VerifyKind {
    VERIFY_PRE_GC,
    VERIFY_POST_GC,
    VERIFY_MARK_YOUNG,
    VERIFY_EVACUATE_YOUNG,
    VERIFY_MARK_FULL,
    VERIFY_EVACUATE_OLD,
    VERIFY_EVACUATE_FULL,
    VERIFY_SHARED_RSET_POST_FULL_GC,
    VERIFY_PRE_SHARED_GC,
    VERIFY_POST_SHARED_GC,
    VERIFY_SHARED_GC_MARK,
    VERIFY_SHARED_GC_SWEEP,
    VERIFY_END,
};

enum class SharedHeapOOMSource {
    NORMAL_ALLOCATION,
    DESERIALIZE,
    SHARED_GC,
};

class BaseHeap {
public:
    BaseHeap(const EcmaParamConfiguration &config) : config_(config) {}
    virtual ~BaseHeap() = default;
    NO_COPY_SEMANTIC(BaseHeap);
    NO_MOVE_SEMANTIC(BaseHeap);

    virtual void Destroy() = 0;

    virtual bool IsMarking() const = 0;

    virtual bool IsReadyToConcurrentMark() const = 0;

    virtual bool NeedStopCollection() = 0;

    virtual void SetSensitiveStatus(AppSensitiveStatus status) = 0;

    virtual AppSensitiveStatus GetSensitiveStatus() const = 0;

    virtual bool FinishStartupEvent() = 0;

    virtual bool OnStartupEvent() const = 0;

    virtual void NotifyPostFork() = 0;

    virtual void TryTriggerIdleCollection() = 0;

    virtual void TryTriggerIncrementalMarking() = 0;

    /*
     * Wait for existing concurrent marking tasks to be finished (if any).
     * Return true if there's ongoing concurrent marking.
     */
    virtual bool CheckOngoingConcurrentMarking() = 0;

    virtual bool OldSpaceExceedCapacity(size_t size) const = 0;

    virtual bool OldSpaceExceedLimit() const = 0;

    virtual inline size_t GetCommittedSize() const = 0;

    virtual inline size_t GetHeapObjectSize() const = 0;

    virtual inline size_t GetRegionCount() const = 0;

    virtual void ChangeGCParams(bool inBackground) = 0;

    virtual const GlobalEnvConstants *GetGlobalConst() const = 0;

    virtual GCStats *GetEcmaGCStats() = 0;

    virtual bool ObjectExceedMaxHeapSize() const = 0;

    virtual void UpdateHeapStatsAfterGC(TriggerGCType gcType) = 0;

    MarkType GetMarkType() const
    {
        return markType_;
    }

    void SetMarkType(MarkType markType)
    {
        markType_ = markType;
    }

    bool IsYoungMark() const
    {
        return markType_ == MarkType::MARK_YOUNG;
    }

    bool IsFullMark() const
    {
        return markType_ == MarkType::MARK_FULL;
    }

    bool IsConcurrentFullMark() const
    {
        return markType_ == MarkType::MARK_FULL;
    }

    void SetGCType(TriggerGCType gcType)
    {
        gcType_ = gcType;
    }

    TriggerGCType GetGCType() const
    {
        return gcType_;
    }

    bool PUBLIC_API IsAlive(TaggedObject *object) const;

    bool ContainObject(TaggedObject *object) const;

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

    void ShouldForceThrowOOMError()
    {
        shouldForceThrowOOMError_ = true;
    }

    void SetCanThrowOOMError(bool canThrow)
    {
        canThrowOOMError_ = canThrow;
    }

    bool CanThrowOOMError()
    {
        return canThrowOOMError_;
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

    void SetGCState(bool inGC)
    {
        inGC_.store(inGC, std::memory_order_relaxed);
    }

    bool InGC() const
    {
        return inGC_.load(std::memory_order_relaxed);
    }

    void NotifyHeapAliveSizeAfterGC(size_t size)
    {
        heapAliveSizeAfterGC_ = size;
    }

    size_t GetHeapAliveSizeAfterGC() const
    {
        return heapAliveSizeAfterGC_;
    }

    size_t GetFragmentSizeAfterGC() const
    {
        return fragmentSizeAfterGC_;
    }

    size_t GetHeapBasicLoss() const
    {
        return heapBasicLoss_;
    }

    size_t GetGlobalSpaceAllocLimit() const
    {
        return globalSpaceAllocLimit_;
    }

    // Whether should verify heap during gc.
    bool ShouldVerifyHeap() const
    {
        return shouldVerifyHeap_;
    }

    bool EnablePageTagThreadId() const
    {
        return enablePageTagThreadId_;
    }

    void ThrowOutOfMemoryErrorForDefault(JSThread *thread, size_t size, std::string functionName,
        bool NonMovableObjNearOOM = false);

    uint32_t GetMaxMarkTaskCount() const
    {
        return maxMarkTaskCount_;
    }

    bool InSensitiveStatus() const
    {
        return GetSensitiveStatus() == AppSensitiveStatus::ENTER_HIGH_SENSITIVE || OnStartupEvent();
    }

    void OnAllocateEvent(EcmaVM *ecmaVm, TaggedObject* address, size_t size);
    inline void SetHClassAndDoAllocateEvent(JSThread *thread, TaggedObject *object, JSHClass *hclass,
                                            [[maybe_unused]] size_t size);
    bool CheckCanDistributeTask();
    void IncreaseTaskCount();
    void ReduceTaskCount();
    void WaitRunningTaskFinished();
    uint32_t GetRunningTaskCount();
    void WaitClearTaskFinished();
    void ThrowOutOfMemoryError(JSThread *thread, size_t size, std::string functionName,
        bool NonMovableObjNearOOM = false);
    void SetMachineCodeOutOfMemoryError(JSThread *thread, size_t size, std::string functionName);

#ifndef NDEBUG
    bool TriggerCollectionOnNewObjectEnabled() const
    {
        return triggerCollectionOnNewObject_;
    };

    void EnableTriggerCollectionOnNewObject()
    {
        triggerCollectionOnNewObject_ = true;
    }

    void DisableTriggerCollectionOnNewObject()
    {
        triggerCollectionOnNewObject_ = false;
    }
#endif

protected:
    void FatalOutOfMemoryError(size_t size, std::string functionName);

    inline TaggedObject *FastAllocateYoungInTlabForCMC(JSThread *thread, size_t size) const;
    inline TaggedObject *FastAllocateOldInTlabForCMC(JSThread *thread, size_t size) const;
    inline TaggedObject *AllocateYoungForCMC(JSThread *thread, size_t size) const;
    inline TaggedObject *AllocateOldForCMC(JSThread *thread, size_t size) const;

    enum class HeapType {
        LOCAL_HEAP,
        SHARED_HEAP,
        INVALID,
    };

    class RecursionScope {
    public:
        explicit RecursionScope(BaseHeap* heap, HeapType heapType) : heap_(heap), heapType_(heapType)
        {
            if (heap_->recursionDepth_++ != 0) {
                LOG_GC(FATAL) << "Recursion in HeapCollectGarbage(isShared=" << static_cast<int>(heapType_)
                              << ") Constructor, depth: " << heap_->recursionDepth_;
            }
            heap_->SetGCState(true);
        }
        ~RecursionScope()
        {
            if (--heap_->recursionDepth_ != 0) {
                LOG_GC(FATAL) << "Recursion in HeapCollectGarbage(isShared=" << static_cast<int>(heapType_)
                              << ") Destructor, depth: " << heap_->recursionDepth_;
            }
            heap_->SetGCState(false);
        }
    private:
        BaseHeap *heap_ {nullptr};
        HeapType heapType_ {HeapType::INVALID};
    };

    static constexpr double TRIGGER_SHARED_CONCURRENT_MARKING_OBJECT_LIMIT_RATE = 0.75;

    const EcmaParamConfiguration config_;
    MarkType markType_ {MarkType::MARK_YOUNG};
    TriggerGCType gcType_ {TriggerGCType::YOUNG_GC};
    Mutex gcCollectGarbageMutex_;
    // Region allocators.
    NativeAreaAllocator *nativeAreaAllocator_ {nullptr};
    HeapRegionAllocator *heapRegionAllocator_ {nullptr};

    size_t heapAliveSizeAfterGC_ {0};
    size_t globalSpaceAllocLimit_ {0};
    size_t globalSpaceConcurrentMarkLimit_ {0};
    size_t heapBasicLoss_ {1_MB};
    size_t fragmentSizeAfterGC_ {0};
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
    // Diffs from `shouldThrowOOMError_`, this is set due to allocating region failed during GC, and thus make
    // MemMapAllocator infinite to complete this GC. After GC, if this flag is set, we MUST throw OOM force.
    bool shouldForceThrowOOMError_ {false};
    bool canThrowOOMError_ {true};
    bool oldGCRequested_ {false};
    // ONLY used for heap verification.
    bool shouldVerifyHeap_ {false};
    bool isVerifying_ {false};
    bool enablePageTagThreadId_ {false};
    std::atomic_bool inGC_ {false};
    int32_t recursionDepth_ {0};
#ifndef NDEBUG
    bool triggerCollectionOnNewObject_ {true};
#endif
};

class SharedHeap : public BaseHeap {
public:
    SharedHeap(const EcmaParamConfiguration &config) : BaseHeap(config) {}
    virtual ~SharedHeap() = default;

    static void CreateNewInstance();
    static SharedHeap *GetInstance();
    static void DestroyInstance();

    void Initialize(NativeAreaAllocator *nativeAreaAllocator, HeapRegionAllocator *heapRegionAllocator,
        const JSRuntimeOptions &option, DaemonThread *dThread);

    void Destroy() override;

    void PostInitialization(const GlobalEnvConstants *globalEnvConstants, const JSRuntimeOptions &option);

    void EnableParallelGC(JSRuntimeOptions &option);

    void DisableParallelGC(JSThread *thread);

    void AdjustGlobalSpaceAllocLimit();

    inline void OnMoveEvent(uintptr_t address, TaggedObject* forwardAddress, size_t size);

    void ResetLargeCapacity();

    class ParallelMarkTask : public common::Task {
    public:
        ParallelMarkTask(int32_t id, SharedHeap *heap, SharedParallelMarkPhase taskPhase)
            : common::Task(id), sHeap_(heap), taskPhase_(taskPhase) {};
        ~ParallelMarkTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(ParallelMarkTask);
        NO_MOVE_SEMANTIC(ParallelMarkTask);

    private:
        SharedHeap *sHeap_ {nullptr};
        SharedParallelMarkPhase taskPhase_;
    };

    class AsyncClearTask : public common::Task {
    public:
        AsyncClearTask(int32_t id, SharedHeap *heap, TriggerGCType type)
            : common::Task(id), sHeap_(heap), gcType_(type) {}
        ~AsyncClearTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(AsyncClearTask);
        NO_MOVE_SEMANTIC(AsyncClearTask);
    private:
        SharedHeap *sHeap_;
        TriggerGCType gcType_;
    };
    bool IsMarking() const override
    {
        LOG_FULL(ERROR) << "SharedHeap IsMarking() not support yet";
        return false;
    }

    bool IsReadyToConcurrentMark() const override;

    bool NeedStopCollection() override;

    void SetSensitiveStatus(AppSensitiveStatus status) override
    {
        LockHolder lock(smartGCStats_.sensitiveStatusMutex_);
        smartGCStats_.sensitiveStatus_ = status;
        if (!InSensitiveStatus()) {
            smartGCStats_.sensitiveStatusCV_.Signal();
        }
    }

    // This should be called when holding lock of sensitiveStatusMutex_.
    AppSensitiveStatus GetSensitiveStatus() const override
    {
        return smartGCStats_.sensitiveStatus_;
    }

    StartupStatus GetStartupStatus() const
    {
        return smartGCStats_.startupStatus_;
    }

    bool IsJustFinishStartup() const
    {
        return smartGCStats_.startupStatus_ == StartupStatus::JUST_FINISH_STARTUP;
    }

    bool CancelJustFinishStartupEvent()
    {
        LockHolder lock(smartGCStats_.sensitiveStatusMutex_);
        if (!IsJustFinishStartup()) {
            return false;
        }
        smartGCStats_.startupStatus_ = StartupStatus::FINISH_STARTUP;
        return true;
    }

    bool FinishStartupEvent() override
    {
        LockHolder lock(smartGCStats_.sensitiveStatusMutex_);
        if (!OnStartupEvent()) {
            return false;
        }
        smartGCStats_.startupStatus_ = StartupStatus::JUST_FINISH_STARTUP;
        if (!InSensitiveStatus()) {
            smartGCStats_.sensitiveStatusCV_.Signal();
        }
        return true;
    }

    // This should be called when holding lock of sensitiveStatusMutex_.
    bool OnStartupEvent() const override
    {
        return smartGCStats_.startupStatus_ == StartupStatus::ON_STARTUP;
    }

    void NotifyPostFork() override
    {
        LockHolder lock(smartGCStats_.sensitiveStatusMutex_);
        smartGCStats_.startupStatus_ = StartupStatus::ON_STARTUP;
    }

    void WaitSensitiveStatusFinished()
    {
        LockHolder lock(smartGCStats_.sensitiveStatusMutex_);
        while (InSensitiveStatus() && !smartGCStats_.forceGC_) {
            smartGCStats_.sensitiveStatusCV_.Wait(&smartGCStats_.sensitiveStatusMutex_);
        }
    }

    bool ObjectExceedMaxHeapSize() const override;

    bool ObjectExceedJustFinishStartupThresholdForGC() const;

    bool ObjectExceedJustFinishStartupThresholdForCM() const;

    bool CheckIfNeedStopCollectionByStartup();

    void TryAdjustSpaceOvershootByConfigSize();

    bool CheckAndTriggerSharedGC(JSThread *thread);

    bool CheckHugeAndTriggerSharedGC(JSThread *thread, size_t size);

    bool HasCSetRegions()
    {
        return sOldSpace_->GetCollectSetRegionCount() > 0;
    }

    void TryTriggerLocalConcurrentMarking();

    // Called when all vm is destroyed, and try to destroy daemon thread.
    void WaitAllTasksFinishedAfterAllJSThreadEliminated();

    void WaitAllTasksFinished(JSThread *thread);

    void StartConcurrentMarking(TriggerGCType gcType, MarkReason markReason);         // In daemon thread

    // Use JSThread instead of DaemonThread to check if IsReadyToSharedConcurrentMark, to avoid an atomic load.
    bool CheckCanTriggerConcurrentMarking(JSThread *thread);

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

    void UpdateWorkManager(SharedGCWorkManager *sWorkManager);

    bool CheckOngoingConcurrentMarking() override;

    bool OldSpaceExceedCapacity(size_t size) const override
    {
        size_t totalSize = sOldSpace_->GetCommittedSize() + size;
        return totalSize >= sOldSpace_->GetMaximumCapacity() + sOldSpace_->GetOutOfMemoryOvershootSize();
    }

    bool OldSpaceExceedLimit() const override
    {
        return sOldSpace_->GetHeapObjectSize() >= sOldSpace_->GetInitialCapacity();
    }

    SharedConcurrentMarker *GetConcurrentMarker() const
    {
        return sConcurrentMarker_;
    }

    SharedGCEvacuator *GetSharedGCEvacuator() const
    {
        return sEvacuator_;
    }

    SharedConcurrentSweeper *GetSweeper() const
    {
        return sSweeper_;
    }

    UnifiedGC *GetUnifiedGC() const
    {
        return unifiedGC_;
    }

    bool IsParallelGCEnabled() const
    {
        return parallelGC_;
    }

    SharedOldSpace *GetOldSpace() const
    {
        return sOldSpace_;
    }

    SharedOldSpace *GetCompressSpace() const
    {
        return sCompressSpace_;
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

    SharedAppSpawnSpace *GetAppSpawnSpace() const
    {
        return sAppSpawnSpace_;
    }

    void SetForceGC(bool forceGC)
    {
        LockHolder lock(smartGCStats_.sensitiveStatusMutex_);
        smartGCStats_.forceGC_ = forceGC;
        if (smartGCStats_.forceGC_) {
            smartGCStats_.sensitiveStatusCV_.Signal();
        }
    }

    inline void TryTriggerConcurrentMarking(JSThread *thread);

    template<TriggerGCType gcType, MarkReason markReason>
    void TriggerConcurrentMarking(JSThread *thread);

    template<TriggerGCType gcType, GCReason gcReason>
    void CollectGarbage(JSThread *thread);

    template<GCReason gcReason>
    void CompressCollectGarbageNotWaiting(JSThread *thread);

    template<TriggerGCType gcType, GCReason gcReason>
    void PostGCTaskForTest(JSThread *thread);

    void CollectGarbageNearOOM(JSThread *thread);
    // Only means the main body of SharedGC is finished, i.e. if parallel_gc is enabled, this flags will be set
    // to true even if sweep_task and clear_task is running asynchronously
    void NotifyGCCompleted();            // In daemon thread

    // Called when all vm is destroyed, and try to destroy daemon thread
    void WaitGCFinishedAfterAllJSThreadEliminated();

    void WaitGCFinished(JSThread *thread);

    void DaemonCollectGarbage(TriggerGCType gcType, GCReason reason);

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

    inline size_t GetRegionCount() const override
    {
        size_t result = sOldSpace_->GetRegionCount() +
            sHugeObjectSpace_->GetRegionCount() +
            sNonMovableSpace_->GetRegionCount() +
            sReadOnlySpace_->GetRegionCount();
        return result;
    }

    void ResetNativeSizeAfterLastGC()
    {
        nativeSizeAfterLastGC_.store(0, std::memory_order_relaxed);
    }

    void IncNativeSizeAfterLastGC(size_t size)
    {
        nativeSizeAfterLastGC_.fetch_add(size, std::memory_order_relaxed);
    }

    size_t GetNativeSizeAfterLastGC() const
    {
        return nativeSizeAfterLastGC_.load(std::memory_order_relaxed);
    }

    size_t GetNativeSizeTriggerSharedGC() const
    {
        return incNativeSizeTriggerSharedGC_;
    }

    size_t GetNativeSizeTriggerSharedCM() const
    {
        return incNativeSizeTriggerSharedCM_;
    }

    void ChangeGCParams([[maybe_unused]]bool inBackground) override
    {
        LOG_FULL(ERROR) << "SharedHeap ChangeGCParams() not support yet";
        return;
    }

    GCStats *GetEcmaGCStats() override
    {
        return sGCStats_;
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

    void Prepare(bool inTriggerGCThread);
    void Reclaim(TriggerGCType gcType);
    void PostGCMarkingTask(SharedParallelMarkPhase sharedTaskPhase);
    void CompactHeapBeforeFork(JSThread *thread);
    void ReclaimForAppSpawn();

    SharedGCWorkManager *GetWorkManager() const
    {
        return sWorkManager_;
    }

    SharedGCMarker *GetSharedGCMarker() const
    {
        return sharedGCMarker_;
    }

    SharedGCMovableMarker *GetSharedGCMovableMarker() const
    {
        return sharedGCMovableMarker_;
    }
    inline void SwapOldSpace();

    SharedMemController *GetSharedMemController() const
    {
        return sharedMemController_;
    }

    void PrepareRecordRegionsForReclaim();

    template<class Callback>
    void EnumerateOldSpaceRegions(const Callback &cb) const;

    template<class Callback>
    void EnumerateOldSpaceRegionsWithRecord(const Callback &cb) const;

    template<class Callback>
    void IterateOverObjects(const Callback &cb) const;

    inline TaggedObject *AllocateClassClass(JSThread *thread, JSHClass *hclass, size_t size);

    inline TaggedObject *AllocateNonMovableOrHugeObject(JSThread *thread, JSHClass *hclass);

    inline TaggedObject *AllocateNonMovableOrHugeObject(JSThread *thread, JSHClass *hclass, size_t size);

    inline TaggedObject *AllocateNonMovableOrHugeObject(JSThread *thread, size_t size);

    inline TaggedObject *AllocateOldOrHugeObject(JSThread *thread, JSHClass *hclass);

    inline TaggedObject *AllocateOldOrHugeObject(JSThread *thread, JSHClass *hclass, size_t size);

    inline TaggedObject *AllocateOldOrHugeObject(JSThread *thread, size_t size);

    inline TaggedObject *AllocateOldOrHugeObjectNoGC(JSThread *thread, size_t size);

    inline TaggedObject *AllocateHugeObject(JSThread *thread, JSHClass *hclass, size_t size);

    inline TaggedObject *AllocateHugeObject(JSThread *thread, size_t size);

    inline TaggedObject *AllocateReadOnlyOrHugeObject(JSThread *thread, JSHClass *hclass);

    inline TaggedObject *AllocateReadOnlyOrHugeObject(JSThread *thread, JSHClass *hclass, size_t size);

    inline TaggedObject *AllocateSNonMovableTlab(JSThread *thread, size_t size);

    inline TaggedObject *AllocateSOldTlab(JSThread *thread, size_t size);

    size_t VerifyHeapObjects(VerifyKind verifyKind) const;

    inline void MergeToOldSpaceSync(SharedLocalSpace *localSpace);

    void DumpHeapSnapshotBeforeOOM(bool isFullGC, JSThread *thread, SharedHeapOOMSource source);

    inline void ProcessSharedNativeDelete(const WeakRootVisitor& visitor);
    inline void PushToSharedNativePointerList(JSNativePointer* pointer);
    inline void IteratorNativePointerList(WeakVisitor &visitor);

    void UpdateHeapStatsAfterGC(TriggerGCType gcType) override;

    class SharedGCScope {
    public:
        SharedGCScope();
        ~SharedGCScope();
    };

    bool InHeapProfiler() const
    {
        return inHeapProfiler_;
    }

    void CheckInHeapProfiler();

    void StartUnifiedGCMark(TriggerGCType gcType, GCReason gcReason);
    template<TriggerGCType gcType, GCReason gcReason>
    bool TriggerUnifiedGCMark(JSThread *thread) const;
    void SetGCThreadQosPriority(common::PriorityMode mode);

private:
    void ProcessAllGCListeners();
    void CollectGarbageFinish(bool inDaemon, TriggerGCType gcType);

    void MoveOldSpaceToAppspawn();

    void ReclaimRegions(TriggerGCType type);

    void ForceCollectGarbageWithoutDaemonThread(TriggerGCType gcType, GCReason gcReason, JSThread *thread);
    inline TaggedObject *AllocateInSOldSpace(JSThread *thread, size_t size);
    inline void InvokeSharedNativePointerCallbacks();
    struct SharedHeapSmartGCStats {
        /**
         * For SmartGC.
         * For daemon thread, it check these status before trying to collect garbage, and wait until finish.
         * It need that check-wait events is atomic, so use a Mutex/CV.
        */
        Mutex sensitiveStatusMutex_;
        ConditionVariable sensitiveStatusCV_;
        AppSensitiveStatus sensitiveStatus_ {AppSensitiveStatus::NORMAL_SCENE};
        StartupStatus startupStatus_ {StartupStatus::BEFORE_STARTUP};
        // If the SharedHeap is almost OOM and a collect is failed, cause a GC with GCReason::ALLOCATION_FAILED,
        // must do GC at once even in sensitive status.
        bool forceGC_ {false};
    };

    SharedHeapSmartGCStats smartGCStats_;

    static SharedHeap *instance_;

    GCStats *sGCStats_ {nullptr};

    bool localFullMarkTriggered_ {false};

    bool optionalLogEnabled_ {false};

    bool parallelGC_ {true};

    // Only means the main body of SharedGC is finished, i.e. if parallel_gc is enabled, this flags will be set
    // to true even if sweep_task and clear_task is running asynchronously
    bool gcFinished_ {true};
    Mutex waitGCFinishedMutex_;
    ConditionVariable waitGCFinishedCV_;

    DaemonThread *dThread_ {nullptr};
    const GlobalEnvConstants *globalEnvConstants_ {nullptr};
    SharedOldSpace *sOldSpace_ {nullptr};
    SharedOldSpace *sCompressSpace_ {nullptr};
    SharedNonMovableSpace *sNonMovableSpace_ {nullptr};
    SharedReadOnlySpace *sReadOnlySpace_ {nullptr};
    SharedHugeObjectSpace *sHugeObjectSpace_ {nullptr};
    SharedAppSpawnSpace *sAppSpawnSpace_ {nullptr};
    SharedGCWorkManager *sWorkManager_ {nullptr};
    SharedConcurrentMarker *sConcurrentMarker_ {nullptr};
    SharedConcurrentSweeper *sSweeper_ {nullptr};
    SharedGC *sharedGC_ {nullptr};
    SharedFullGC *sharedFullGC_ {nullptr};
    SharedGCEvacuator *sEvacuator_ {nullptr};
    SharedGCMarker *sharedGCMarker_ {nullptr};
    SharedGCMovableMarker *sharedGCMovableMarker_ {nullptr};
    SharedMemController *sharedMemController_ {nullptr};
    UnifiedGC *unifiedGC_ {nullptr};
    size_t growingFactor_ {0};
    size_t growingStep_ {0};
    size_t incNativeSizeTriggerSharedCM_ {0};
    size_t incNativeSizeTriggerSharedGC_ {0};
    size_t fragmentationLimitForSharedFullGC_ {0};
    std::atomic<size_t> spaceOvershoot_ {0};
    std::atomic<size_t> nativeSizeAfterLastGC_ {0};
    bool inHeapProfiler_ {false};
    NativePointerList sharedNativePointerList_;
    std::mutex sNativePointerListMutex_;
};

class Heap : public BaseHeap {
public:
    explicit Heap(EcmaVM *ecmaVm);
    virtual ~Heap() = default;
    NO_COPY_SEMANTIC(Heap);
    NO_MOVE_SEMANTIC(Heap);
    void Initialize();
    void Destroy() override;
    void Prepare();
    void UnifiedGCPrepare();
    void GetHeapPrepare();
    void ResetLargeCapacity();
    void Resume(TriggerGCType gcType);
    void ResumeForAppSpawn();
    void CompactHeapBeforeFork();
    void DisableParallelGC();
    void EnableParallelGC();
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT) && defined(PANDA_TARGET_OHOS) && defined(ENABLE_HISYSEVENT)
    void SetJsDumpThresholds(size_t thresholds) const;
#endif

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

    OldSpace *GetCompressSpace() const
    {
        return compressSpace_;
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

    Marker *GetCompressGCMarker() const
    {
        return compressGCMarker_;
    }

    UnifiedGCMarker *GetUnifiedGCMarker() const
    {
        return unifiedGCMarker_;
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

    WorkNode *&GetMarkingObjectLocalBuffer()
    {
        return sharedGCData_.sharedConcurrentMarkingLocalBuffer_;
    }

    IdleGCTrigger *GetIdleGCTrigger() const
    {
        return idleGCTrigger_;
    }

    void SetRSetWorkListHandler(RSetWorkListHandler *handler)
    {
        ASSERT((sharedGCData_.rSetWorkListHandler_ == nullptr) != (handler == nullptr));
        sharedGCData_.rSetWorkListHandler_ = handler;
    }

    void ProcessSharedGCMarkingLocalBuffer();

    void ProcessSharedGCRSetWorkList();

    const GlobalEnvConstants *GetGlobalConst() const override;

    MemController *GetMemController() const
    {
        return memController_;
    }

    inline void RecordOrResetObjectSize(size_t objectSize)
    {
        recordObjectSize_ = objectSize;
    }

    inline size_t GetRecordObjectSize() const
    {
        return recordObjectSize_;
    }

    inline void RecordOrResetNativeSize(size_t nativeSize)
    {
        recordNativeSize_ = nativeSize;
    }

    inline size_t GetRecordNativeSize() const
    {
        return recordNativeSize_;
    }

    /*
     * For object allocations.
     */

    // Young
    inline TaggedObject *AllocateInYoungSpace(size_t size);
    inline TaggedObject *AllocateYoungOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateYoungOrHugeObject(JSHClass *hclass, size_t size);
    inline TaggedObject *AllocateYoungOrHugeObject(size_t size);
    inline TaggedObject *AllocateReadOnlyOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateReadOnlyOrHugeObject(JSHClass *hclass, size_t size);
    inline TaggedObject *AllocateReadOnlyOrHugeObject(size_t size);
    inline uintptr_t AllocateYoungSync(size_t size);
    inline TaggedObject *TryAllocateYoungGeneration(JSHClass *hclass, size_t size);
    // Old
    inline TaggedObject *AllocateOldOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateOldOrHugeObject(JSHClass *hclass, size_t size);
    inline TaggedObject *AllocateOldOrHugeObject(size_t size);
    // Non-movable
    inline TaggedObject *AllocateNonMovableOrHugeObject(JSHClass *hclass);
    inline TaggedObject *AllocateNonMovableOrHugeObject(JSHClass *hclass, size_t size);
    inline TaggedObject *AllocateClassClass(JSHClass *hclass, size_t size);
    // Huge
    inline TaggedObject *AllocateHugeObject(size_t size);
    inline TaggedObject *AllocateHugeObject(JSHClass *hclass, size_t size);
    // Machine code
    inline TaggedObject *AllocateMachineCodeObject(JSHClass *hclass, size_t size, MachineCodeDesc *desc = nullptr);
    inline TaggedObject *AllocateHugeMachineCodeObject(size_t size, MachineCodeDesc *desc = nullptr);
    // Snapshot
    inline uintptr_t AllocateSnapshotSpace(size_t size);

    // shared non movable space tlab
    inline TaggedObject *AllocateSharedNonMovableSpaceFromTlab(JSThread *thread, size_t size);
    // shared old space tlab
    inline TaggedObject *AllocateSharedOldSpaceFromTlab(JSThread *thread, size_t size);

    void ResetTlab();
    void FillBumpPointerForTlab();
    /*
     * GC triggers.
     */
    void CollectGarbage(TriggerGCType gcType, GCReason reason = GCReason::OTHER);
    void ProcessGCCallback();
    bool CheckAndTriggerOldGC(size_t size = 0);
    bool CheckAndTriggerHintGC(MemoryReduceDegree degree, GCReason reason = GCReason::OTHER);
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

    GCKeyStats *GetEcmaGCKeyStats();

    JSObjectResizingStrategy *GetJSObjectResizingStrategy();

    void TriggerIdleCollection(int idleMicroSec);
    void NotifyMemoryPressure(bool inHighMemoryPressure);

    void TryTriggerConcurrentMarking(MarkReason markReason = MarkReason::OTHER);
    void AdjustBySurvivalRate(size_t originalNewSpaceSize);
    void TriggerConcurrentMarking(MarkReason markReason = MarkReason::OTHER);
    bool CheckCanTriggerConcurrentMarking();

    void TryTriggerIdleCollection() override;
    void TryTriggerIncrementalMarking() override;
    void CalculateIdleDuration();
    void UpdateWorkManager(WorkManager *workManager);

    bool CheckOngoingConcurrentMarking() override
    {
        return CheckOngoingConcurrentMarkingImpl(ThreadType::JS_THREAD, MAIN_THREAD_INDEX,
                                                 "Heap::CheckOngoingConcurrentMarking");
    }
    bool DaemonCheckOngoingConcurrentMarking()
    {
        return CheckOngoingConcurrentMarkingImpl(ThreadType::DAEMON_THREAD, DAEMON_THREAD_INDEX,
                                                 "Heap::DaemonCheckOngoingConcurrentMarking");
    }

    inline void SwapNewSpace();
    inline void SwapOldSpace();

    inline bool MoveYoungRegion(Region *region);
    inline bool MoveYoungRegionToOld(Region *region);
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

    inline void NotifyRecordMemorySize();

    inline size_t GetRegionCount() const override;

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

    void SetOnSerializeEvent(bool isSerialize)
    {
        onSerializeEvent_ = isSerialize;
        if (!onSerializeEvent_ && !InSensitiveStatus()) {
            TryTriggerIncrementalMarking();
            TryTriggerIdleCollection();
            TryTriggerConcurrentMarking(MarkReason::EXIT_SERIALIZE);
        }
    }

    bool GetOnSerializeEvent() const
    {
        return onSerializeEvent_;
    }

    void NotifyFinishColdStart(bool isMainThread = true);

    void NotifyFinishColdStartSoon();

    void NotifyWarmStartup();

    void NotifyHighSensitive(bool isStart);

    bool HandleExitHighSensitiveEvent();

    bool ObjectExceedMaxHeapSize() const override;

    bool ObjectExceedHighSensitiveThresholdForCM() const;

    bool ObjectExceedJustFinishStartupThresholdForGC() const;

    bool ObjectExceedJustFinishStartupThresholdForCM() const;

    void TryIncreaseNewSpaceOvershootByConfigSize();

    void TryIncreaseOvershootByConfigSize();

    bool CheckIfNeedStopCollectionByStartup();

    bool CheckIfNeedStopCollectionByHighSensitive();

    bool NeedStopCollection() override;

    void SetSensitiveStatus(AppSensitiveStatus status) override
    {
        sHeap_->SetSensitiveStatus(status);
        smartGCStats_.sensitiveStatus_.store(status, std::memory_order_release);
    }

    AppSensitiveStatus GetSensitiveStatus() const override
    {
        return smartGCStats_.sensitiveStatus_.load(std::memory_order_acquire);
    }

    void SetRecordHeapObjectSizeBeforeSensitive(size_t objSize)
    {
        recordObjSizeBeforeSensitive_ = objSize;
    }

    size_t GetRecordHeapObjectSizeBeforeSensitive() const
    {
        return recordObjSizeBeforeSensitive_;
    }

    void SetNearGCInSensitive(bool flag)
    {
        nearGCInSensitive_ = flag;
    }

    bool IsNearGCInSensitive()
    {
        return nearGCInSensitive_;
    }

    bool CASSensitiveStatus(AppSensitiveStatus expect, AppSensitiveStatus status)
    {
        return smartGCStats_.sensitiveStatus_.compare_exchange_strong(expect, status, std::memory_order_seq_cst);
    }

    StartupStatus GetStartupStatus() const
    {
        ASSERT(smartGCStats_.startupStatus_.load(std::memory_order_relaxed) == sHeap_->GetStartupStatus());
        return smartGCStats_.startupStatus_.load(std::memory_order_relaxed);
    }

    bool IsJustFinishStartup() const
    {
        return GetStartupStatus() == StartupStatus::JUST_FINISH_STARTUP;
    }

    bool CancelJustFinishStartupEvent()
    {
        if (!IsJustFinishStartup()) {
            return false;
        }
        TryIncreaseOvershootByConfigSize();
        smartGCStats_.startupStatus_.store(StartupStatus::FINISH_STARTUP, std::memory_order_release);
        sHeap_->CancelJustFinishStartupEvent();
        return true;
    }

    bool FinishStartupEvent() override
    {
        if (!OnStartupEvent()) {
            LOG_GC(WARN) << "SmartGC: app cold start last status is not JUST_FINISH_STARTUP, just return false";
            return false;
        }
        TryIncreaseOvershootByConfigSize();
        smartGCStats_.startupStatus_.store(StartupStatus::JUST_FINISH_STARTUP, std::memory_order_release);
        sHeap_->FinishStartupEvent();
        return true;
    }

    bool OnStartupEvent() const override
    {
        return GetStartupStatus() == StartupStatus::ON_STARTUP;
    }

    void NotifyPostFork() override
    {
        sHeap_->NotifyPostFork();
        smartGCStats_.startupStatus_.store(StartupStatus::ON_STARTUP, std::memory_order_relaxed);
        LOG_GC(INFO) << "SmartGC: enter app cold start";
        size_t localFirst = config_.GetMaxHeapSize();
        size_t localSecond = config_.GetMaxHeapSize() * JUST_FINISH_STARTUP_LOCAL_THRESHOLD_RATIO;
        auto sharedHeapConfig = sHeap_->GetEcmaParamConfiguration();
        size_t sharedFirst = sHeap_->GetOldSpace()->GetInitialCapacity();
        size_t sharedSecond = sharedHeapConfig.GetMaxHeapSize()
                            * JUST_FINISH_STARTUP_SHARED_THRESHOLD_RATIO
                            * JUST_FINISH_STARTUP_SHARED_CONCURRENT_MARK_RATIO;
        LOG_GC(INFO) << "SmartGC: startup GC restrain, "
            << "phase 1 threshold: local " << localFirst / 1_MB << "MB, shared " << sharedFirst / 1_MB << "MB; "
            << "phase 2 threshold: local " << localSecond / 1_MB << "MB, shared " << sharedSecond / 1_MB << "MB";
    }

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
    inline bool InHeapProfiler();

    inline void OnMoveEvent(uintptr_t address, TaggedObject* forwardAddress, size_t size);

    // add allocationInspector to each space
    void AddAllocationInspectorToAllSpaces(AllocationInspector *inspector);

    // clear allocationInspector from each space
    void ClearAllocationInspectorFromAllSpaces();

    /*
     * Funtions used by heap verification.
     */

    template<class Callback>
    void IterateOverObjects(const Callback &cb, bool isSimplify = false) const;

    size_t VerifyHeapObjects(VerifyKind verifyKind = VerifyKind::VERIFY_PRE_GC) const;
    size_t VerifyOldToNewRSet(VerifyKind verifyKind = VerifyKind::VERIFY_PRE_GC) const;
    void StatisticHeapObject(TriggerGCType gcType) const;
    void StatisticHeapDetail();
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

    static bool ShouldMoveToRoSpace(JSHClass *hclass, TaggedObject *object);

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
    void DecreaseNativeBindingSize(size_t size);
    void ResetNativeBindingSize()
    {
        nativeBindingSize_ = 0;
    }

    size_t GetNativeBindingSize() const
    {
        if (g_isEnableCMCGC) {
            return common::BaseRuntime::GetNotifiedNativeSize();
        }
        return nativeBindingSize_;
    }

    size_t GetGlobalSpaceNativeLimit() const
    {
        return globalSpaceNativeLimit_;
    }

    size_t GetNativeBindingSizeAfterLastGC() const
    {
        return nativeBindingSizeAfterLastGC_;
    }

    size_t GetGlobalNativeSize() const
    {
        return GetNativeBindingSize() + nativeAreaAllocator_->GetNativeMemoryUsage();
    }

    void ResetNativeSizeAfterLastGC()
    {
        nativeSizeAfterLastGC_ = 0;
        nativeBindingSizeAfterLastGC_= nativeBindingSize_;
    }

    void IncNativeSizeAfterLastGC(size_t size)
    {
        nativeSizeAfterLastGC_ += size;
    }

    bool GlobalNativeSizeLargerToTriggerGC() const
    {
        auto incNativeBindingSizeAfterLastGC = nativeBindingSize_ > nativeBindingSizeAfterLastGC_ ?
            nativeBindingSize_ - nativeBindingSizeAfterLastGC_ : 0;
        return GetGlobalNativeSize() > nativeSizeTriggerGCThreshold_ &&
            nativeSizeAfterLastGC_ + incNativeBindingSizeAfterLastGC > incNativeSizeTriggerGC_;
    }

    bool GlobalNativeSizeLargerThanLimit() const
    {
        size_t overshoot = InSensitiveStatus() ? nativeSizeOvershoot_ : 0;
        return GetGlobalNativeSize() >= globalSpaceNativeLimit_ + overshoot;
    }

    bool GlobalNativeSizeLargerThanLimitForIdle() const
    {
        return GetGlobalNativeSize() >= static_cast<size_t>(globalSpaceNativeLimit_ *
            IDLE_SPACE_SIZE_LIMIT_RATE);
    }

    void TryTriggerFullMarkOrGCByNativeSize();

    void TryTriggerFullMarkBySharedSize(size_t size);

    bool TryTriggerFullMarkBySharedLimit();

    void CheckAndTriggerTaskFinishedGC();

    bool IsMarking() const override;

    bool IsReadyToConcurrentMark() const override;

    bool IsYoungGC() const
    {
        return gcType_ == TriggerGCType::YOUNG_GC;
    }

    bool IsUnifiedGC() const
    {
        return gcType_ == TriggerGCType::UNIFIED_GC;
    }

    void CheckNonMovableSpaceOOM();
    void DumpHeapSnapshotBeforeOOM(bool isFullGC = true);
    std::tuple<uint64_t, uint8_t *, int, kungfu::CalleeRegAndOffsetVec> CalCallSiteInfo(uintptr_t retAddr) const;
    MachineCode *GetMachineCodeObject(uintptr_t pc) const;
    void SetMachineCodeObject(uintptr_t start, uintptr_t end, uintptr_t address) const;

    PUBLIC_API GCListenerId AddGCListener(FinishGCListener listener, void *data);
    PUBLIC_API void RemoveGCListener(GCListenerId listenerId);
    void ProcessGCListeners();

    inline void ProcessNativeDelete(const WeakRootVisitor& visitor);
    inline void ProcessReferences(const WeakRootVisitor& visitor);
    inline void PushToNativePointerList(JSNativePointer* pointer, bool isConcurrent);
    inline void RemoveFromNativePointerList(const JSNativePointer* pointer);
    inline void ClearNativePointerList();
    inline void IteratorNativePointerList(WeakVisitor &vistor);

    size_t GetNativePointerListSize() const
    {
        return nativePointerList_.size();
    }

    size_t GetHeapAliveSizeExcludesYoungAfterGC() const
    {
        return heapAliveSizeExcludesYoungAfterGC_;
    }

    void UpdateHeapStatsAfterGC(TriggerGCType gcType) override;

private:
    void CollectGarbageImpl(TriggerGCType gcType, GCReason reason = GCReason::OTHER);

    static constexpr int MIN_JSDUMP_THRESHOLDS = 85;
    static constexpr int MAX_JSDUMP_THRESHOLDS = 95;
    static constexpr int IDLE_TIME_LIMIT = 10;  // if idle time over 10ms we can do something
    static constexpr int ALLOCATE_SIZE_LIMIT = 100_KB;
    static constexpr int IDLE_MAINTAIN_TIME = 500;
    static constexpr int BACKGROUND_GROW_LIMIT = 2_MB;
    // Threadshold that HintGC will actually trigger GC.
    static constexpr double SURVIVAL_RATE_THRESHOLD = 0.5;
    static constexpr size_t NEW_ALLOCATED_SHARED_OBJECT_SIZE_LIMIT = DEFAULT_SHARED_HEAP_SIZE / 10; // 10 : ten times.
    static constexpr size_t INIT_GLOBAL_SPACE_NATIVE_SIZE_LIMIT = 100_MB;
    void RecomputeLimits();
    void AdjustOldSpaceLimit();
    // record lastRegion for each space, which will be used in ReclaimRegions()
    void PrepareRecordRegionsForReclaim();
    inline void ReclaimRegions(TriggerGCType gcType);
    inline size_t CalculateCommittedCacheSize();
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT) && defined(PANDA_TARGET_OHOS) && defined(ENABLE_HISYSEVENT)
    uint64_t GetCurrentTickMillseconds();
    void ThresholdReachedDump();
#endif
    void CleanCallback();
    void IncreasePendingAsyncNativeCallbackSize(size_t bindingSize)
    {
        pendingAsyncNativeCallbackSize_ += bindingSize;
    }
    void DecreasePendingAsyncNativeCallbackSize(size_t bindingSize)
    {
        pendingAsyncNativeCallbackSize_ -= bindingSize;
    }
    bool CheckOngoingConcurrentMarkingImpl(ThreadType threadType, int threadIndex,
                                           [[maybe_unused]] const char* traceName);
    class ParallelGCTask : public common::Task {
    public:
        ParallelGCTask(int32_t id, Heap *heap, ParallelGCTaskPhase taskPhase)
            : common::Task(id), heap_(heap), taskPhase_(taskPhase) {};
        ~ParallelGCTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(ParallelGCTask);
        NO_MOVE_SEMANTIC(ParallelGCTask);

    private:
        Heap *heap_ {nullptr};
        ParallelGCTaskPhase taskPhase_;
    };

    class AsyncClearTask : public common::Task {
    public:
        AsyncClearTask(int32_t id, Heap *heap, TriggerGCType type)
            : common::Task(id), heap_(heap), gcType_(type) {}
        ~AsyncClearTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(AsyncClearTask);
        NO_MOVE_SEMANTIC(AsyncClearTask);
    private:
        Heap *heap_;
        TriggerGCType gcType_;
    };

    class FinishColdStartTask : public common::Task {
    public:
        FinishColdStartTask(int32_t id, Heap *heap)
            : common::Task(id), heap_(heap) {}
        ~FinishColdStartTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(FinishColdStartTask);
        NO_MOVE_SEMANTIC(FinishColdStartTask);
    private:
        Heap *heap_;
    };

    class FinishGCRestrainTask : public common::Task {
    public:
        FinishGCRestrainTask(int32_t id, Heap *heap)
            : common::Task(id), heap_(heap) {}
        ~FinishGCRestrainTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(FinishGCRestrainTask);
        NO_MOVE_SEMANTIC(FinishGCRestrainTask);
    private:
        Heap *heap_;
    };

    class DeleteCallbackTask : public common::Task {
    public:
        DeleteCallbackTask(int32_t id, std::vector<NativePointerCallbackData> &callbacks) : common::Task(id)
        {
            std::swap(callbacks, nativePointerCallbacks_);
        }
        ~DeleteCallbackTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(DeleteCallbackTask);
        NO_MOVE_SEMANTIC(DeleteCallbackTask);

    private:
        std::vector<NativePointerCallbackData> nativePointerCallbacks_ {};
    };

    struct MainLocalHeapSmartGCStats {
        /**
         * For SmartGC.
         * For main js thread, it check these status everytime when trying to
         * collect garbage(e.g. in JSThread::CheckSafePoint), and skip if need, so std::atomic is almost enough.
        */
        std::atomic<AppSensitiveStatus> sensitiveStatus_ {AppSensitiveStatus::NORMAL_SCENE};
        std::atomic<StartupStatus> startupStatus_ {StartupStatus::BEFORE_STARTUP};
    };

    // Some data used in SharedGC is also need to store in local heap, e.g. the temporary local mark stack.
    struct SharedGCLocalStoragePackedData {
        /**
         * During SharedGC concurrent marking, barrier will push shared object to mark stack for marking,
         * in LocalGC can just push non-shared object to WorkNode for MAIN_THREAD_INDEX, but in SharedGC, only can
         * either use a global lock for DAEMON_THREAD_INDEX's WorkNode, or push to a local WorkNode, and push to global
         * in remark.
         * If the heap is destructed before push this node to global, check and try to push remain object as well.
        */
        WorkNode *sharedConcurrentMarkingLocalBuffer_ {nullptr};
        /**
         * Recording the local_to_share rset used in SharedGC concurrentMark,
         * which lifecycle is in one SharedGC.
         * Before mutate this local heap(e.g. LocalGC::Evacuate), should make sure the RSetWorkList is all processed,
         * other the SharedGC concurrentMark will visitor the incorrect local_to_share bit.
         * Before destroying local heap, RSetWorkList should be done as well.
        */
        RSetWorkListHandler *rSetWorkListHandler_ {nullptr};
    };

    EcmaVM *ecmaVm_ {nullptr};
    JSThread *thread_ {nullptr};

    SharedHeap *sHeap_ {nullptr};
    MainLocalHeapSmartGCStats smartGCStats_;

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
    // tlab for shared non movable space
    ThreadLocalAllocationBuffer *sNonMovableTlab_ {nullptr};
    // tlab for shared old space
    ThreadLocalAllocationBuffer *sOldTlab_ {nullptr};
    /*
     * Garbage collectors collecting garbage in different scopes.
     */

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
    Marker *compressGCMarker_ {nullptr};
    UnifiedGCMarker *unifiedGCMarker_ {nullptr};

    // Work manager managing the tasks mostly generated in the GC mark phase.
    WorkManager *workManager_ {nullptr};

    SharedGCLocalStoragePackedData sharedGCData_;

    bool onSerializeEvent_ {false};
    bool parallelGC_ {true};
    bool fullGCRequested_ {false};
    bool fullMarkRequested_ {false};
    bool oldSpaceLimitAdjusted_ {false};
    bool enableIdleGC_ {false};
    std::atomic_bool isCSetClearing_ {false};
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
    size_t nativeSizeTriggerGCThreshold_ {0};
    size_t incNativeSizeTriggerGC_ {0};
    size_t nativeSizeOvershoot_ {0};
    size_t asyncClearNativePointerThreshold_ {0};
    size_t nativeSizeAfterLastGC_ {0};
    size_t heapAliveSizeExcludesYoungAfterGC_ {0};
    size_t nativeBindingSizeAfterLastGC_ {0};
    size_t newAllocatedSharedObjectSize_ {0};
    // recordObjectSize_ & recordNativeSize_:
    // Record memory before taskpool start, used to determine trigger GC or not after task finish.
    size_t recordObjectSize_ {0};
    size_t recordNativeSize_ {0};
    // Record heap object size before enter sensitive status
    size_t recordObjSizeBeforeSensitive_ {0};
    bool nearGCInSensitive_ {false};

    size_t pendingAsyncNativeCallbackSize_ {0};
    MemGrowingType memGrowingtype_ {MemGrowingType::HIGH_THROUGHPUT};

    // parallel evacuator task number.
    uint32_t maxEvacuateTaskCount_ {0};

    uint64_t startupDurationInMs_ {0};

    Mutex setNewSpaceOvershootSizeMutex_;

    // Application status

    IdleNotifyStatusCallback notifyIdleStatusCallback {nullptr};

    IdleTaskType idleTask_ {IdleTaskType::NO_TASK};
    float idlePredictDuration_ {0.0f};
    double idleTaskFinishTime_ {0.0};

    /*
     * The listeners which are called at the end of GC
     */
    std::vector<std::pair<FinishGCListener, void *>> gcListeners_;

    IdleGCTrigger *idleGCTrigger_ {nullptr};

    bool hasOOMDump_ {false};

    NativePointerList nativePointerList_;
    NativePointerList concurrentNativePointerList_;

    friend panda::test::HProfTestHelper;
    friend panda::test::GCTest_CallbackTask_Test;
    friend panda::test::HeapTestHelper;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_H
