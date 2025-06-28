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

#ifndef COMMON_COMPONENTS_HEAP_COLLECTOR_TRACE_COLLECTOR_H
#define COMMON_COMPONENTS_HEAP_COLLECTOR_TRACE_COLLECTOR_H

#include <cstdint>
#include <map>

#include "common_components/heap/collector/collector.h"
#include "common_components/heap/collector/collector_resources.h"
#include "common_components/common/mark_work_stack.h"
#include "common_components/heap/allocator/region_space.h"
#include "common_components/heap/collector/copy_data_manager.h"
#include "common_components/mutator/mutator_manager.h"

namespace common {
class GlobalWorkStackQueue;

// number of nanoseconds in a microsecond.
constexpr uint64_t NS_PER_US = 1000;
constexpr uint64_t NS_PER_S = 1000000000;

// prefetch distance for mark.
#define MACRO_MARK_PREFETCH_DISTANCE 16    // this macro is used for check when pre-compiling.
constexpr int MARK_PREFETCH_DISTANCE = 16; // when it is changed, remember to change MACRO_MARK_PREFETCH_DISTANCE.

// Small queue implementation, for prefetching.
#define COMMON_MAX_PREFETCH_QUEUE_SIZE_LOG 5UL
#define COMMON_MAX_PREFETCH_QUEUE_SIZE (1UL << COMMON_MAX_PREFETCH_QUEUE_SIZE_LOG)
#if COMMON_MAX_PREFETCH_QUEUE_SIZE <= MACRO_MARK_PREFETCH_DISTANCE
#error Prefetch queue size must be strictly greater than prefetch distance.
#endif

class PrefetchQueue {
public:
    explicit PrefetchQueue(size_t d) : elems_{ nullptr }, distance_(d), tail_(0), head_(0) {}
    ~PrefetchQueue() {}
    inline void Add(BaseObject* objaddr)
    {
        size_t t = tail_;
        elems_[t] = objaddr;
        tail_ = (t + 1) & (COMMON_MAX_PREFETCH_QUEUE_SIZE - 1UL);

        __builtin_prefetch(reinterpret_cast<void*>(objaddr), 0, PREFETCH_LOCALITY);
    }

    inline BaseObject* Remove()
    {
        size_t h = head_;
        BaseObject* objaddr = elems_[h];
        head_ = (h + 1) & (COMMON_MAX_PREFETCH_QUEUE_SIZE - 1UL);

        return objaddr;
    }

    inline size_t Length() const { return (tail_ - head_) & (COMMON_MAX_PREFETCH_QUEUE_SIZE - 1UL); }

    inline bool Empty() const { return head_ == tail_; }

    inline bool Full() const { return Length() == distance_; }

private:
    static constexpr int PREFETCH_LOCALITY = 3;
    BaseObject* elems_[COMMON_MAX_PREFETCH_QUEUE_SIZE];
    size_t distance_;
    size_t tail_;
    size_t head_;
}; // End of small queue implementation

// For managing gc roots
class StaticRootTable {
public:
    struct StaticRootArray {
        RefField<>* content[0];
    };

    StaticRootTable() { totalRootsCount_ = 0; }
    ~StaticRootTable() = default;
    void VisitRoots(const RefFieldVisitor& visitor);

private:
    std::mutex gcRootsLock_;                         // lock gcRootsBuckets
    std::map<StaticRootArray*, uint32_t> gcRootsBuckets_; // record gc roots entry of CFile
    size_t totalRootsCount_;
};

class MarkingWork;
class ConcurrentMarkingWork;

class TraceCollector : public Collector {
    friend MarkingWork;
    friend ConcurrentMarkingWork;

public:
    explicit TraceCollector(Allocator& allocator, CollectorResources& resources)
        : Collector(), theAllocator_(allocator), collectorResources_(resources)
    {}

    ~TraceCollector() override = default;
    virtual void PreGarbageCollection(bool isConcurrent);
    virtual void PostGarbageCollection(uint64_t gcIndex);

    // Types, so that we don't confuse root sets and working stack.
    // The policy is: we simply `push_back` into root set,
    // but we use Enqueue to add into work stack.
    using RootSet = MarkStack<BaseObject*>;
    using WorkStack = MarkStack<BaseObject*>;
    using WorkStackBuf = MarkStackBuffer<BaseObject*>;
    using WeakStack = MarkStack<RefField<>*>;

    void Init(const RuntimeParam& param) override;
    void Fini() override;

#if defined(GCINFO_DEBUG) && GCINFO_DEBUG
    void DumpRoots(LogType logType);
    void DumpHeap(const CString& tag);
    void DumpBeforeGC()
    {
        if (ENABLE_LOG(FRAGMENT)) {
            if (MutatorManager::Instance().WorldStopped()) {
                DumpHeap("before_gc");
            } else {
                ScopedStopTheWorld stw("dump-heap-before-gc");
                DumpHeap("before_gc");
            }
        }
    }

    void DumpAfterGC()
    {
        if (ENABLE_LOG(FRAGMENT)) {
            if (MutatorManager::Instance().WorldStopped()) {
                DumpHeap("after_gc");
            } else {
                ScopedStopTheWorld stw("dump-heap-after-gc");
                DumpHeap("after_gc");
            }
        }
    }
#endif

    bool ShouldIgnoreRequest(GCRequest& request) override { return request.ShouldBeIgnored(); }

    void ProcessMarkStack(uint32_t threadIndex, Taskpool *threadPool, WorkStack &workStack,
                          GlobalWorkStackQueue &globalQueue);

    void TryForkTask(Taskpool *threadPool, WorkStack &workStack, GlobalWorkStackQueue &globalQueue);

    // live but not resurrected object.
    bool IsMarkedObject(const BaseObject* obj) const { return RegionSpace::IsMarkedObject(obj); }

    // live or resurrected object.
    inline bool IsSurvivedObject(const BaseObject* obj) const
    {
        return RegionSpace::IsMarkedObject(obj) || RegionSpace::IsResurrectedObject(obj);
    }

    inline bool IsToObject(const BaseObject* obj) const
    {
        return RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj))->IsToRegion();
    }

    inline bool IsToVersion(const BaseObject* obj) const
    {
        return obj->IsToVersion();
    }

    virtual bool MarkObject(BaseObject* obj, size_t cellCount = 0) const
    {
        bool marked = RegionSpace::MarkObject(obj);
        if (!marked) {
            reinterpret_cast<RegionSpace&>(theAllocator_).CountLiveObject(obj);
            if (!fixReferences_ && RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj))->IsFromRegion()) {
                DLOG(TRACE, "marking tag w-obj %p<cls %p>+%zu", obj, obj->GetTypeInfo(), obj->GetSize());
            }
        }
        return marked;
    }

    virtual void EnumRefFieldRoot(RefField<>& ref, RootSet& rootSet) const {}
    virtual void TraceObjectRefFields(BaseObject* obj, WorkStack& workStack, WeakStack& weakStack)
    {
        LOG_COMMON(FATAL) << "Unresolved fatal";
        UNREACHABLE_CC();
    }

    inline bool IsResurrectedObject(const BaseObject* obj) const { return RegionSpace::IsResurrectedObject(obj); }

    Allocator& GetAllocator() const { return theAllocator_; }

    bool IsHeapMarked() const { return collectorResources_.IsHeapMarked(); }

    void SetHeapMarked(bool value) { collectorResources_.SetHeapMarked(value); }

    void SetGcStarted(bool val) { collectorResources_.SetGcStarted(val); }

    void RunGarbageCollection(uint64_t, GCReason) override;

    void TransitionToGCPhase(const GCPhase phase, const bool)
    {
        MutatorManager::Instance().TransitionAllMutatorsToGCPhase(phase);
    }

    GCStats& GetGCStats() override { return collectorResources_.GetGCStats(); }

    virtual void UpdateGCStats();

    static const size_t MAX_MARKING_WORK_SIZE;
    static const size_t MIN_MARKING_WORK_SIZE;

    void CopyObject(const BaseObject& fromObj, BaseObject& toObj, size_t size) const;

protected:
    virtual BaseObject* CopyObjectAfterExclusive(BaseObject* obj) = 0;
    virtual void CopyFromSpace();
    virtual void ExemptFromSpace();

    virtual void DoGarbageCollection() = 0;

    void RequestGCInternal(GCReason reason, bool async) override { collectorResources_.RequestGC(reason, async); }
    void MergeWeakStack(WeakStack& weakStack);
    void UpdateNativeThreshold(GCParam& gcParam);

    Allocator& theAllocator_;

    // A collectorResources provides the resources that the tracing collector need,
    // such as gc thread/threadPool, gc task queue.
    // Also provides the resource access interfaces, such as invokeGC, waitGC.
    // This resource should be singleton and shared for multi-collectors
    CollectorResources& collectorResources_;

    WeakStack globalWeakStack_;
    std::mutex weakStackLock_;

    uint32_t snapshotFinalizerNum_ = 0;

    // reason for current GC.
    GCReason gcReason_ = GC_REASON_USER;

    // indicate whether to fix references (including global roots and reference fields).
    // this member field is useful for optimizing concurrent copying gc.
    bool fixReferences_ = false;

    std::atomic<size_t> markedObjectCount_ = { 0 };

    void ResetBitmap(bool heapMarked)
    {
        // if heap is marked and tracing result will be used during next gc, we should not reset liveInfo.
    }

    uint32_t GetGCThreadCount(const bool isConcurrent) const
    {
        return collectorResources_.GetGCThreadCount(isConcurrent);
    }

    inline WorkStack NewWorkStack() const
    {
        WorkStack workStack = WorkStack();
        return workStack;
    }

    inline void SetGCReason(const GCReason reason) { gcReason_ = reason; }

    Taskpool *GetThreadPool() const { return collectorResources_.GetThreadPool(); }
    // enum all roots.
    void EnumerateAllRootsImpl(Taskpool *threadPool, RootSet& rootSet);

    // let finalizerProcessor process finalizers, and mark resurrected if in stw gc
    virtual void ProcessWeakReferences() {}
    virtual void ProcessStringTable() {}

    virtual void ProcessFinalizers() {}
    virtual void RemarkAndPreforwardStaticRoots(WorkStack& workStack)
    {
        LOG_COMMON(FATAL) << "Unresolved fatal";
        UNREACHABLE_CC();
    }

    void MergeAllocBufferRoots(WorkStack& workStack);
    void EnumerateAllRoots(WorkStack& workStack);
    void PushRootInWorkStack(RootSet *dst, RootSet *src);

    void TraceRoots(WorkStack& workStack);
    void Remark(WorkStack& workStack);
    bool MarkSatbBuffer(WorkStack& workStack);

    // concurrent marking.
    void TracingImpl(WorkStack& workStack, bool parallel);

    bool AddConcurrentTracingWork(WorkStack& workStack, GlobalWorkStackQueue &globalQueue, size_t threadCount);
private:
    void MarkRememberSetImpl(BaseObject* object, WorkStack& workStack);
    void ConcurrentRemark(WorkStack& remarkStack, bool parallel);
    void MarkAwaitingJitFort();
    void EnumMutatorRoot(ObjectPtr& obj, RootSet& rootSet) const;
    void EnumConcurrencyModelRoots(RootSet& rootSet) const;
    void EnumStaticRoots(RootSet& rootSet) const;
};
} // namespace common

#endif  // COMMON_COMPONENTS_HEAP_COLLECTOR_TRACE_COLLECTOR_H
