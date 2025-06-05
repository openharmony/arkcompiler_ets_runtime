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

#include "common_components/heap/collector/trace_collector.h"

#include <new>
#include <iomanip>

#include "common_components/heap/allocator/alloc_buffer.h"
#include "common_interfaces/base/runtime_param.h"

namespace common {
const size_t TraceCollector::MAX_MARKING_WORK_SIZE = 16; // fork task if bigger
const size_t TraceCollector::MIN_MARKING_WORK_SIZE = 8;  // forbid forking task if smaller

// Fill gc roots entry to buckets
void StaticRootTable::RegisterRoots(StaticRootArray* addr, uint32_t size)
{
    std::lock_guard<std::mutex> lock(gcRootsLock_);
    gcRootsBuckets_.insert(std::pair<StaticRootArray*, uint32_t>(addr, size));
    totalRootsCount_ += size;
}

void StaticRootTable::UnregisterRoots(StaticRootArray* addr, uint32_t size)
{
    std::lock_guard<std::mutex> lock(gcRootsLock_);
    auto iter = gcRootsBuckets_.find(addr);
    if (iter != gcRootsBuckets_.end()) {
        gcRootsBuckets_.erase(iter);
        totalRootsCount_ -= size;
    }
}

void StaticRootTable::VisitRoots(const RefFieldVisitor& visitor)
{
    std::lock_guard<std::mutex> lock(gcRootsLock_);
    uint32_t gcRootsSize = 0;
    std::unordered_set<RefField<>*> visitedSet;
    for (auto iter = gcRootsBuckets_.begin(); iter != gcRootsBuckets_.end(); iter++) {
        gcRootsSize = iter->second;
        StaticRootArray* array = iter->first;
        for (uint32_t i = 0; i < gcRootsSize; i++) {
            RefField<>* root = array->content[i];
            // make sure to visit each static root only once time.
            if (visitedSet.find(root) != visitedSet.end()) {
                continue;
            }
            visitedSet.insert(root);
            visitor(*root);
        }
    }
}

class GlobalWorkStackQueue {
public:
    GlobalWorkStackQueue() = default;
    ~GlobalWorkStackQueue() = default;

    void AddWorkStack(TraceCollector::WorkStack &&stack)
    {
        DCHECK_CC(!stack.empty());
        std::lock_guard<std::mutex> guard(mtx_);
        workStacks_.push_back(std::move(stack));
    }

    TraceCollector::WorkStack PopWorkStack()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        while (true) {
            if (!workStacks_.empty()) {
                TraceCollector::WorkStack stack(std::move(workStacks_.back()));
                workStacks_.pop_back();
                return stack;
            }
            if (finished_) {
                return TraceCollector::WorkStack();
            }
            cv_.wait(lock);
        }
    }

    TraceCollector::WorkStack DrainAllWorkStack()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        while (!workStacks_.empty()) {
            TraceCollector::WorkStack stack(std::move(workStacks_.back()));
            workStacks_.pop_back();
            return stack;
        }
        return TraceCollector::WorkStack();
    }

    void NotifyFinish()
    {
        std::lock_guard<std::mutex> guard(mtx_);
        DCHECK_CC(!finished_);
        finished_ = true;
        cv_.notify_all();
    }
private:
    bool finished_ {false};
    std::condition_variable cv_;
    std::mutex mtx_;
    std::vector<TraceCollector::WorkStack> workStacks_;
};

class ConcurrentMarkingTask : public common::Task {
public:
    ConcurrentMarkingTask(uint32_t id, TraceCollector &tc, Taskpool *pool, TaskPackMonitor &monitor,
                          GlobalWorkStackQueue &globalQueue)
        : Task(id), collector_(tc), threadPool_(pool), monitor_(monitor), globalQueue_(globalQueue)
    {}

    // single work task without thread pool
    ConcurrentMarkingTask(uint32_t id, TraceCollector& tc, TaskPackMonitor &monitor,
                          GlobalWorkStackQueue &globalQueue)
        : Task(id), collector_(tc), threadPool_(nullptr), monitor_(monitor), globalQueue_(globalQueue)
    {}

    ~ConcurrentMarkingTask() override
    {
        threadPool_ = nullptr;
    }

    // run concurrent marking task.
    bool Run([[maybe_unused]] uint32_t threadIndex) override
    {
        while (true) {
            TraceCollector::WorkStack workStack = globalQueue_.PopWorkStack();
            if (workStack.empty()) {
                break;
            }
            collector_.ProcessMarkStack(threadIndex, threadPool_, workStack, globalQueue_);
        }
        monitor_.NotifyFinishOne();
        return true;
    }

private:
    TraceCollector &collector_;
    Taskpool *threadPool_;
    TaskPackMonitor &monitor_;
    GlobalWorkStackQueue &globalQueue_;
};

void TraceCollector::TryForkTask(Taskpool *threadPool, WorkStack &workStack, GlobalWorkStackQueue &globalQueue)
{
    size_t size = workStack.size();
    if (size > MIN_MARKING_WORK_SIZE) {
        bool doFork = false;
        size_t newSize = 0;
        if (size > MAX_MARKING_WORK_SIZE) {
            newSize = size >> 1; // give 1/2 the stack to the thread pool as a new work task
            doFork = true;
        } else if (size > MIN_MARKING_WORK_SIZE) {
            constexpr uint8_t shiftForEight = 3;
            newSize = size >> shiftForEight; // give 1/8 the stack to the thread pool as a new work task
            doFork = true;
        }

        if (doFork) {
            WorkStackBuf *hSplit = workStack.split(newSize);
            globalQueue.AddWorkStack(WorkStack(hSplit));
        }
    }
}

void TraceCollector::ProcessMarkStack([[maybe_unused]] uint32_t threadIndex, Taskpool *threadPool,
                                      WorkStack &workStack, GlobalWorkStackQueue &globalQueue)
{
    size_t nNewlyMarked = 0;
    WeakStack weakStack;
    // loop until work stack empty.
    for (;;) {
        ++nNewlyMarked;
        if (workStack.empty()) {
            break;
        }
        // get next object from work stack.
        BaseObject* obj = workStack.back();
        workStack.pop_back();
        auto region = RegionDesc::GetRegionDescAt(reinterpret_cast<MAddress>((void*)obj));
        region->AddLiveByteCount(obj->GetSize());
        [[maybe_unused]] auto beforeSize = workStack.count();
        TraceObjectRefFields(obj, workStack, weakStack);
        DLOG(TRACE, "[tracing] visit finished, workstack size: before=%d, after=%d, newly added=%d",
             beforeSize, workStack.count(), workStack.count() - beforeSize);
        // try to fork new task if needed.
        if (threadPool != nullptr) {
            TryForkTask(threadPool, workStack, globalQueue);
        }
    } // end of mark loop.
    // newly marked statistics.
    markedObjectCount_.fetch_add(nNewlyMarked, std::memory_order_relaxed);
    MergeWeakStack(weakStack);
}

void TraceCollector::MergeWeakStack(WeakStack& weakStack)
{
    std::lock_guard<std::mutex> lock(weakStackLock_);
    globalWeakStack_.insert(weakStack);
}

void TraceCollector::EnumConcurrencyModelRoots(RootSet& rootSet) const
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

void TraceCollector::EnumStaticRoots(RootSet& rootSet) const
{
    const RefFieldVisitor& visitor = [&rootSet, this](RefField<>& root) { EnumRefFieldRoot(root, rootSet); };
    VisitRoots(visitor, true);
}

void TraceCollector::MergeMutatorRoots(WorkStack& workStack)
{
    MutatorManager& mutatorManager = MutatorManager::Instance();
    // hold mutator list lock to freeze mutator liveness, otherwise may access dead mutator fatally
    bool worldStopped = mutatorManager.WorldStopped();
    if (!worldStopped) {
        mutatorManager.MutatorManagementWLock();
    }
    theAllocator_.VisitAllocBuffers([&workStack](AllocationBuffer& buffer) { buffer.MarkStack(workStack); });
    if (!worldStopped) {
        mutatorManager.MutatorManagementWUnlock();
    }
}

void TraceCollector::EnumerateAllRoots(WorkStack& workStack)
{
    OHOS_HITRACE(HITRACE_LEVEL_MAX, "CMCGC::EnumerateAllRoots", "");
    EnumerateAllRootsImpl(GetThreadPool(), workStack);
}

void TraceCollector::TracingImpl(WorkStack& workStack, bool parallel)
{
    OHOS_HITRACE(HITRACE_LEVEL_MAX, "CMCGC::TracingImpl", "");
    if (workStack.empty()) {
        return;
    }

    // enable parallel marking if we have thread pool.
    Taskpool *threadPool = GetThreadPool();
    ASSERT_LOGF(threadPool != nullptr, "thread pool is null");
    if (parallel) { // parallel marking.
        uint32_t parallelCount = GetGCThreadCount(true) - 1;
        uint32_t threadCount = parallelCount + 1;
        TaskPackMonitor monitor(parallelCount, parallelCount);
        GlobalWorkStackQueue globalQueue;
        for (uint32_t i = 0; i < parallelCount; ++i) {
            threadPool->PostTask(std::make_unique<ConcurrentMarkingTask>(0, *this, threadPool, monitor, globalQueue));
        }
        if (!AddConcurrentTracingWork(workStack, globalQueue, static_cast<size_t>(threadCount))) {
            ProcessMarkStack(0, threadPool, workStack, globalQueue);
        }
        while (true) {
            WorkStack stack = globalQueue.DrainAllWorkStack();
            if (stack.empty()) {
                break;
            }
            ProcessMarkStack(0, threadPool, stack, globalQueue);
        }
        globalQueue.NotifyFinish();
        monitor.WaitAllFinished();
    } else {
        // serial marking with a single mark task.
        GlobalWorkStackQueue globalQueue;
        WorkStack stack(std::move(workStack));
        ProcessMarkStack(0, nullptr, stack, globalQueue);
    }
}

bool TraceCollector::AddConcurrentTracingWork(WorkStack& workStack, GlobalWorkStackQueue &globalQueue,
                                              size_t threadCount)
{
    if (workStack.size() <= threadCount * MIN_MARKING_WORK_SIZE) {
        return false; // too less init tasks, which may lead to workload imbalance, add work rejected
    }
    DCHECK_CC(threadCount > 0);
    const size_t chunkSize = std::min(workStack.size() / threadCount + 1, MIN_MARKING_WORK_SIZE);
    // Split the current work stack into work tasks.
    while (!workStack.empty()) {
        WorkStackBuf *hSplit = workStack.split(chunkSize);
        globalQueue.AddWorkStack(WorkStack(hSplit));
    }
    return true;
}

void TraceCollector::TraceRoots(WorkStack& workStack)
{
    OHOS_HITRACE(HITRACE_LEVEL_MAX, "CMCGC::TraceRoots", "");
    COMMON_PHASE_TIMER("TraceRoots");
    VLOG(REPORT, "roots size: %zu", workStack.size());

    ASSERT_LOGF(GetThreadPool() != nullptr, "null thread pool");

    // use fewer threads and lower priority for concurrent mark.
    const uint32_t maxWorkers = GetGCThreadCount(true) - 1;
    VLOG(REPORT, "Concurrent mark with %u threads, workStack: %zu", (maxWorkers + 1), workStack.size());

    {
        COMMON_PHASE_TIMER("Concurrent marking");
        TracingImpl(workStack, maxWorkers > 0);
    }

    {
#ifdef ARK_USE_SATB_BARRIER
        COMMON_PHASE_TIMER("Concurrent re-marking");
        ConcurrentReMark(workStack, maxWorkers > 0);
        ProcessWeakReferences();
#else
        OHOS_HITRACE(HITRACE_LEVEL_MAX, "CMCGC::ReMark[STW]", "");
        if (!BaseRuntime::GetInstance()->GetMutatorManager().WorldStopped()) {
            COMMON_PHASE_TIMER("STW re-marking");
            ScopedStopTheWorld stw("final-mark", true, GCPhase::GC_PHASE_FINAL_MARK);
            EnumerateAllRoots(workStack);
            TracingImpl(workStack, maxWorkers > 0);
            ConcurrentReMark(workStack, maxWorkers > 0);
            ProcessWeakReferences();
        } else {
            if (Heap::GetHeap().GetGCReason() == GC_REASON_YOUNG) {
                LOGF_CHECK(MarkRememberSet(workStack)) << "not cleared\n";
            }
            ProcessWeakReferences();
        }
#endif
    }

    {
        // weakref -> strong ref
        COMMON_PHASE_TIMER("concurrent resurrection");
        DoResurrection(workStack);
    }

    VLOG(REPORT, "mark %zu objects", markedObjectCount_.load(std::memory_order_relaxed));
}

bool TraceCollector::MarkSatbBuffer(WorkStack& workStack)
{
    OHOS_HITRACE(HITRACE_LEVEL_MAX, "CMCGC::MarkSatbBuffer", "");
    COMMON_PHASE_TIMER("MarkSatbBuffer");
    if (!workStack.empty()) {
        workStack.clear();
    }
    constexpr uint64_t maxIterationTime = 120ULL * 1000 * 1000 * 1000; // 2 mins.
    constexpr size_t maxIterationLoopNum = 1000;
    auto visitSatbObj = [this, &workStack]() {
        WorkStack remarkStack;
        auto func = [&remarkStack](Mutator& mutator) {
            const SatbBuffer::TreapNode* node = mutator.GetSatbBufferNode();
            if (node != nullptr) {
                const_cast<SatbBuffer::TreapNode*>(node)->GetObjects(remarkStack);
            }
        };
        MutatorManager::Instance().VisitAllMutators(func);
        SatbBuffer::Instance().GetRetiredObjects(remarkStack);

        while (!remarkStack.empty()) {
            BaseObject* obj = remarkStack.back();
            remarkStack.pop_back();
            if (Heap::IsHeapAddress(obj)) {
                if (!this->MarkObject(obj)) {
                    workStack.push_back(obj);
                    DLOG(TRACE, "satb buffer add obj %p", obj);
                }
            }
        }
    };

    visitSatbObj();
    size_t iterationCnt = 0;
    uint64_t iterationStartTime = TimeUtil::NanoSeconds();
    const uint32_t maxWorkers = GetGCThreadCount(true) - 1;
    do {
        ++iterationCnt;
        if (iterationCnt > maxIterationLoopNum && (TimeUtil::NanoSeconds() - iterationStartTime) > maxIterationTime) {
            ScopedStopTheWorld stw("final-mark", true, GCPhase::GC_PHASE_FINAL_MARK);
            VLOG(REPORT, "MarkSatbBuffer timeouts");
            Taskpool *threadPool = GetThreadPool();
            TracingImpl(workStack, (workStack.size() > MAX_MARKING_WORK_SIZE) && (maxWorkers > 0));
            return workStack.empty();
        }
        if (LIKELY_CC(!workStack.empty())) {
            Taskpool *threadPool = GetThreadPool();
            TracingImpl(workStack, (workStack.size() > MAX_MARKING_WORK_SIZE) && (maxWorkers > 0));
        }
        visitSatbObj();
        if (workStack.empty()) {
            TransitionToGCPhase(GCPhase::GC_PHASE_REMARK_SATB, true);
            visitSatbObj();
        }
    } while (!workStack.empty());
    VLOG(REPORT, "MarkSatbBuffer is finished, takes %zu round and %zu us",
        iterationCnt, (TimeUtil::NanoSeconds() - iterationStartTime) / MICRO_SECOND_TO_NANO_SECOND);
    return true;
}

void TraceCollector::MarkRememberSetImpl(BaseObject* object, WorkStack& workStack)
{
    object->ForEachRefField([this, &workStack, &object](RefField<>& field) {
        BaseObject* targetObj = field.GetTargetObject();
        if (Heap::IsHeapAddress(targetObj)) {
            RegionDesc* region = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(targetObj));
            if (region->IsInYoungSpace() &&
                !region->IsNewObjectSinceTrace(targetObj) &&
                !this->MarkObject(targetObj)) {
                workStack.push_back(targetObj);
                DLOG(TRACE, "remember set trace obj: %p@%p, ref: %p", object, &field, targetObj);
            }
        }
    });
}

bool TraceCollector::MarkRememberSet(WorkStack& workStack)
{
    COMMON_PHASE_TIMER("MarkRememberSet");
    if (!workStack.empty()) {
        workStack.clear();
    }
    auto func = [this, &workStack](BaseObject* object) {
        if (Heap::IsHeapAddress(object)) {
            MarkRememberSetImpl(object, workStack);
        }
    };
    auto visitRSetObj = [this, &workStack, &func]() {
        RegionSpace& space = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
        space.VisitOldSpaceRememberSet(func);
    };
    visitRSetObj();
    const uint32_t maxWorkers = GetGCThreadCount(true) - 1;
    do {
        if (LIKELY_CC(!workStack.empty())) {
            Taskpool *threadPool = GetThreadPool();
            TracingImpl(workStack, (workStack.size() > MAX_MARKING_WORK_SIZE) && (maxWorkers > 0));
        }
        visitRSetObj();
        if (workStack.empty()) {
            TransitionToGCPhase(GCPhase::GC_PHASE_REMARK_SATB, true);
            visitRSetObj();
        }
    } while (!workStack.empty());
    return true;
}

void TraceCollector::ConcurrentReMark(WorkStack& remarkStack, bool parallel)
{
    if (Heap::GetHeap().GetGCReason() == GC_REASON_YOUNG) {
        LOGF_CHECK(MarkRememberSet(remarkStack)) << "not cleared\n";
    }
    LOGF_CHECK(MarkSatbBuffer(remarkStack)) << "not cleared\n";
}

// no need to do resurrection with write barrier and satb-buffer, because write barrier affects only live objects
// which are not to be resurrected.
void TraceCollector::DoResurrection(WorkStack& workStack)
{
    workStack.clear();
    RootVisitor func = [&workStack, this](ObjectRef& ref) {
        RefField<>& refField = reinterpret_cast<RefField<>&>(ref);
        RefField<> tmpField(refField);
        BaseObject* finalizerObj = tmpField.GetTargetObject();
        if (!IsMarkedObject(finalizerObj)) {
            if (!MarkObject(finalizerObj)) {
                DLOG(TRACE, "resurrectable obj @%p:%p", &ref, finalizerObj);
                workStack.push_back(finalizerObj);
            }
        }
        RefField<> newField = GetAndTryTagRefField(finalizerObj);
        if (tmpField.GetFieldValue() != newField.GetFieldValue() &&
            refField.CompareExchange(tmpField.GetFieldValue(), newField.GetFieldValue())) {
            DLOG(FIX, "tag finalizer %p@%p -> %#zx", finalizerObj, &ref, newField.GetFieldValue());
        }
    };
    snapshotFinalizerNum_ = collectorResources_.GetFinalizerProcessor().VisitFinalizers(func);

    size_t resurrectdObjects = 0;
    TraceCollector::WeakStack weakStack;
    while (!workStack.empty()) {
        BaseObject* obj = workStack.back();
        workStack.pop_back();
        auto region = RegionDesc::GetRegionDescAt(reinterpret_cast<MAddress>((void*)obj));
        region->AddLiveByteCount(obj->GetSize());
        // skip if the object already marked.

        ++resurrectdObjects;
        ResurrectObject(obj);

        // try to copy object child refs into work stack.
        if (obj->HasRefField()) {
            TraceObjectRefFields(obj, workStack, weakStack);
        }
    }
    markedObjectCount_.fetch_add(resurrectdObjects, std::memory_order_relaxed);
    MergeWeakStack(weakStack);
    VLOG(REPORT, "resurrected objects %zu", resurrectdObjects);
}

void TraceCollector::Init(const RuntimeParam& param) {}

void TraceCollector::Fini() { Collector::Fini(); }

void TraceCollector::EnumFinalizerProcessorRoots(RootSet& rootSet) const
{
    RootVisitor visitor = [this, &rootSet](ObjectRef& root) { EnumAndTagRawRoot(root, rootSet); };
    collectorResources_.GetFinalizerProcessor().VisitGCRoots(visitor);
}

#if defined(GCINFO_DEBUG) && GCINFO_DEBUG
void TraceCollector::DumpHeap(const CString& tag)
{
    ASSERT_LOGF(MutatorManager::Instance().WorldStopped(), "Not In STW");
    DLOG(FRAGMENT, "DumpHeap %s", tag.Str());
    // dump roots
    DumpRoots(FRAGMENT);
    // dump object contents
    auto dumpVisitor = [](BaseObject* obj) {
        // support Dump
        // obj->DumpObject(FRAGMENT)
    };
    bool ret = Heap::GetHeap().ForEachObject(dumpVisitor, false);
    LOGE_IF(UNLIKELY_CC(!ret)) << "theAllocator.ForEachObject() in DumpHeap() return false.";

    // dump object types
    DLOG(FRAGMENT, "Print Type information");
    std::set<TypeInfo*> classinfoSet;
    auto assembleClassInfoVisitor = [&classinfoSet](BaseObject* obj) {
        TypeInfo* classInfo = obj->GetTypeInfo();
        // No need to check the result of insertion, because there are multiple-insertions.
        (void)classinfoSet.insert(classInfo);
    };
    ret = Heap::GetHeap().ForEachObject(assembleClassInfoVisitor, false);
    LOGE_IF(UNLIKELY_CC(!ret)) << "theAllocator.ForEachObject()#2 in DumpHeap() return false.";

    for (auto it = classinfoSet.begin(); it != classinfoSet.end(); it++) {
        TypeInfo* classInfo = *it;
    }
    DLOG(FRAGMENT, "Dump Allocator");
}

void TraceCollector::DumpRoots(LogType logType)
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}
#endif

void TraceCollector::PreGarbageCollection(bool isConcurrent)
{
    VLOG(REPORT, "Begin GC log. GCReason: %s, Current allocated %s, Current threshold %s",
         g_gcRequests[gcReason_].name, Pretty(Heap::GetHeap().GetAllocatedSize()),
         Pretty(Heap::GetHeap().GetCollector().GetGCStats().GetThreshold()));

    // SatbBuffer should be initialized before concurrent enumeration.
    SatbBuffer::Instance().Init();
    // prepare thread pool.

    GetGCStats().reason = gcReason_;
    GetGCStats().async = !g_gcRequests[gcReason_].IsSyncGC();
    GetGCStats().isConcurrentMark = isConcurrent;
#if defined(GCINFO_DEBUG) && GCINFO_DEBUG
    DumpBeforeGC();
#endif
    OHOS_HITRACE_COUNT(HITRACE_LEVEL_MAX, "ARK_RT_pre_GC_HeapSize",
        Heap::GetHeap().GetAllocatedSize());
}

void TraceCollector::PostGarbageCollection(uint64_t gcIndex)
{
    SatbBuffer::Instance().ReclaimALLPages();
    // release pages in PagePool
    PagePool::Instance().Trim();
    collectorResources_.NotifyGCFinished(gcIndex);

#if defined(GCINFO_DEBUG) && GCINFO_DEBUG
    DumpAfterGC();
#endif
}

void TraceCollector::EnumerateAllRootsImpl(Taskpool *threadPool, RootSet& rootSet)
{
    ASSERT_LOGF(threadPool != nullptr, "thread pool is null");

    const size_t threadCount = static_cast<size_t>(GetGCThreadCount(true));
    RootSet rootSetsInstance[threadCount];
    RootSet* rootSets = rootSetsInstance; // work_around the crash of clang parser

    // Only one root task, no need to post task.
    EnumStaticRoots(rootSets[0]);
    {
        OHOS_HITRACE(HITRACE_LEVEL_MAX, "CMCGC::MergeMutatorRoots", "");
        MergeMutatorRoots(rootSet);
    }

    {
        OHOS_HITRACE(HITRACE_LEVEL_MAX, "CMCGC::PushRootInWorkStack", "");
        WorkStack tempStack = NewWorkStack();
        for (size_t i = 0; i < threadCount; ++i) {
            tempStack.insert(rootSets[i]);
        }
        while (!tempStack.empty()) {
            auto temp = tempStack.back();
            tempStack.pop_back();
            if (!this->MarkObject(temp)) {
                rootSet.push_back(temp);
            }
        }
    }
    VLOG(REPORT, "Total roots: %zu(exclude stack roots)", rootSet.size());
}

void TraceCollector::VisitStaticRoots(const RefFieldVisitor& visitor) const
{
    Heap::GetHeap().VisitStaticRoots(visitor);
}

void TraceCollector::VisitFinalizerRoots(const RootVisitor& visitor) const
{
    collectorResources_.GetFinalizerProcessor().VisitGCRoots(visitor);
}

void TraceCollector::UpdateGCStats()
{
    RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
    GCStats& gcStats = GetGCStats();
    gcStats.Dump();

    size_t oldThreshold = gcStats.heapThreshold;
    size_t oldTargetFootprint = gcStats.targetFootprint;
    size_t recentBytes = space.GetRecentAllocatedSize();
    size_t survivedBytes = space.GetSurvivedSize();
    size_t bytesAllocated = space.GetAllocatedBytes();

    size_t targetSize;
    HeapParam& heapParam = BaseRuntime::GetInstance()->GetHeapParam();
    GCParam& gcParam = BaseRuntime::GetInstance()->GetGCParam();
    if (!gcStats.isYoungGC()) {
        gcStats.shouldRequestYoung = true;
        size_t delta = bytesAllocated * (1.0 / heapParam.heapUtilization - 1.0);
        size_t growBytes = std::min(delta, gcParam.maxGrowBytes);
        growBytes = std::max(growBytes, gcParam.minGrowBytes);
        targetSize = bytesAllocated + growBytes * gcParam.multiplier;
    } else {
        gcStats.shouldRequestYoung = gcStats.collectionRate * gcParam.ygcRateAdjustment >= g_fullGCMeanRate
                                && bytesAllocated <= oldThreshold;
        size_t adjustMaxGrowBytes = gcParam.maxGrowBytes * gcParam.multiplier;
        if (bytesAllocated + adjustMaxGrowBytes < oldTargetFootprint) {
            targetSize = bytesAllocated + adjustMaxGrowBytes;
        } else {
            targetSize = std::max(bytesAllocated, oldTargetFootprint);
        }
    }

    gcStats.targetFootprint = targetSize;
    size_t remainingBytes = recentBytes;
    remainingBytes = std::min(remainingBytes, gcParam.kMaxConcurrentRemainingBytes);
    remainingBytes = std::max(remainingBytes, gcParam.kMinConcurrentRemainingBytes);

    if (UNLIKELY(remainingBytes > gcStats.targetFootprint)) {
        remainingBytes = std::min(gcParam.kMinConcurrentRemainingBytes, gcStats.targetFootprint);
    }
    gcStats.heapThreshold = std::max(gcStats.targetFootprint - remainingBytes, bytesAllocated);
    gcStats.heapThreshold = std::min(gcStats.heapThreshold, gcParam.gcThreshold);

    if (!gcStats.isYoungGC()) {
        g_gcRequests[GC_REASON_HEU].SetMinInterval(gcParam.gcInterval);
    } else {
        g_gcRequests[GC_REASON_YOUNG].SetMinInterval(gcParam.gcInterval);
    }

    LOG_COMMON(INFO) << "allocated bytes " << bytesAllocated << " (survive bytes " << survivedBytes
                     << ", recent-allocated " << recentBytes << "), update target footprint "
                     << oldTargetFootprint << " -> " << gcStats.targetFootprint
                     << ", update gc threshold " << oldThreshold << " -> " << gcStats.heapThreshold;
    OHOS_HITRACE_COUNT(HITRACE_LEVEL_MAX, "ARK_RT_post_GC_HeapSize", Heap::GetHeap().GetAllocatedSize());
}

void TraceCollector::CopyObject(const BaseObject& fromObj, BaseObject& toObj, size_t size) const
{
    uintptr_t from = reinterpret_cast<uintptr_t>(&fromObj);
    uintptr_t to = reinterpret_cast<uintptr_t>(&toObj);
    LOGE_IF(memmove_s(reinterpret_cast<void*>(to), size, reinterpret_cast<void*>(from), size) != EOK) <<
        "memmove_s fail";
#if defined(COMMON_TSAN_SUPPORT)
    Sanitizer::TsanFixShadow(reinterpret_cast<void *>(from), reinterpret_cast<void *>(to), size);
#endif
}

void TraceCollector::RunGarbageCollection(uint64_t gcIndex, GCReason reason)
{
    OHOS_HITRACE(HITRACE_LEVEL_MAX, "CMCGC::RunGarbageCollection", "");
    // prevent other threads stop-the-world during GC.
    // this may be removed in the future.
    ScopedSTWLock stwLock;

    gcReason_ = reason;
    PreGarbageCollection(true);
    LOG_COMMON(INFO) << "[GC] Start " << GetCollectorName() << " " << g_gcRequests[gcReason_].name << " gcIndex="
                     << gcIndex;
    Heap::GetHeap().SetGCReason(reason);
    GCStats& gcStats = GetGCStats();
    gcStats.collectedBytes = 0;
    gcStats.gcStartTime = TimeUtil::NanoSeconds();

    DoGarbageCollection();

    HeapBitmapManager::GetHeapBitmapManager().ClearHeapBitmap();
    reinterpret_cast<RegionSpace&>(theAllocator_).DumpAllRegionStats("region statistics when gc ends");

    if (reason == GC_REASON_OOM) {
        Heap::GetHeap().GetAllocator().ReclaimGarbageMemory(true);
    }

    PostGarbageCollection(gcIndex);
    MutatorManager::Instance().DestroyExpiredMutators();
    gcStats.gcEndTime = TimeUtil::NanoSeconds();

    uint64_t gcTimeNs = gcStats.gcEndTime - gcStats.gcStartTime;
    double rate = (static_cast<double>(gcStats.collectedBytes) / gcTimeNs) * (static_cast<double>(NS_PER_S) / MB);
    {
        std::ostringstream oss;
        const int prec = 3;
        oss << "total gc time: " << Pretty(gcTimeNs / NS_PER_US) << " us, collection rate ";
        oss << std::setprecision(prec) << rate << " MB/s";
        LOG_COMMON(INFO) << oss.str();
    }

    g_gcCount++;
    g_gcTotalTimeUs += (gcTimeNs / NS_PER_US);
    g_gcCollectedTotalBytes += gcStats.collectedBytes;
    gcStats.collectionRate = rate;

    if (!gcStats.isYoungGC()) {
        if (g_fullGCCount == 0) {
            g_fullGCMeanRate = rate;
        } else {
            g_fullGCMeanRate = (g_fullGCMeanRate * g_fullGCCount + rate) / (g_fullGCCount + 1);
        }
        g_fullGCCount++;
    }

    UpdateGCStats();
}

void TraceCollector::CopyFromSpace()
{
    OHOS_HITRACE(HITRACE_LEVEL_MAX, "CMCGC::CopyFromSpace", "");
    TransitionToGCPhase(GCPhase::GC_PHASE_COPY, true);

    RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
    GCStats& stats = GetGCStats();
    stats.liveBytesBeforeGC = space.GetAllocatedBytes();
    stats.fromSpaceSize = space.FromSpaceSize();
    space.CopyFromSpace(GetThreadPool());

    stats.smallGarbageSize = space.FromSpaceSize() - space.ToSpaceSize();
}

void TraceCollector::ExemptFromSpace()
{
    RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
    space.ExemptFromSpace();
}

} // namespace common
