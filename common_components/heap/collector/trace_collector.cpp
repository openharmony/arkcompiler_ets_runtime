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
#include "common_components/heap/collector/heuristic_gc_policy.h"
#include "common_interfaces/base/runtime_param.h"
#include <string>

namespace common {
const size_t TraceCollector::MAX_MARKING_WORK_SIZE = 16; // fork task if bigger
const size_t TraceCollector::MIN_MARKING_WORK_SIZE = 8;  // forbid forking task if smaller

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
            WorkStack workStack = globalQueue_.PopWorkStack();
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

class ClearWeakStackTask : public common::Task {
public:
    ClearWeakStackTask(uint32_t id, TraceCollector &tc, Taskpool *pool, TaskPackMonitor &monitor,
                          GlobalWeakStackQueue &globalQueue)
        : Task(id), collector_(tc), threadPool_(pool), monitor_(monitor), globalQueue_(globalQueue)
    {}

    // single work task without thread pool
    ClearWeakStackTask(uint32_t id, TraceCollector& tc, TaskPackMonitor &monitor,
                          GlobalWeakStackQueue &globalQueue)
        : Task(id), collector_(tc), threadPool_(nullptr), monitor_(monitor), globalQueue_(globalQueue)
    {}

    ~ClearWeakStackTask() override
    {
        threadPool_ = nullptr;
    }

    // run concurrent marking task.
    bool Run([[maybe_unused]] uint32_t threadIndex) override
    {
        while (true) {
            WeakStack weakStack = globalQueue_.PopWorkStack();
            if (weakStack.empty()) {
                break;
            }
            collector_.ProcessWeakStack(weakStack);
        }
        monitor_.NotifyFinishOne();
        return true;
    }

private:
    TraceCollector &collector_;
    Taskpool *threadPool_;
    TaskPackMonitor &monitor_;
    GlobalWeakStackQueue &globalQueue_;
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

void TraceCollector::ProcessWeakStack(WeakStack &weakStack)
{
    while (!weakStack.empty()) {
        auto [fieldPointer, offset] = *weakStack.back();
        weakStack.pop_back();
        ASSERT_LOGF(offset % sizeof(RefField<>) == 0, "offset is not aligned");

        RefField<> &field = reinterpret_cast<RefField<>&>(*fieldPointer);
        RefField<> oldField(field);

        BaseObject* targetObj = oldField.GetTargetObject();
        if (!Heap::IsHeapAddress(targetObj) || IsMarkedObject(targetObj) ||
            RegionSpace::IsNewObjectSinceTrace(targetObj)) {
            continue;
        }

        BaseObject* obj = reinterpret_cast<BaseObject*>(reinterpret_cast<uintptr_t>(&field) - offset);
        auto regionType = RegionDesc::InlinedRegionMetaData::GetInlinedRegionMetaData(reinterpret_cast<uintptr_t>(obj))
                              ->GetRegionType();
        // the object might be trimed then forwarded, we need to make sure toField still points to the current object
        if (regionType == RegionDesc::RegionType::FROM_REGION && obj->IsForwarded() &&
            obj->GetSizeForwarded() > offset) {
            BaseObject *toObj = obj->GetForwardingPointer();
            RefField<> &toField = *reinterpret_cast<RefField<>*>(reinterpret_cast<uintptr_t>(toObj) + offset);

            toField.ClearRef(oldField.GetFieldValue());
        }
        field.ClearRef(oldField.GetFieldValue());
    }
}

void TraceCollector::ProcessMarkStack([[maybe_unused]] uint32_t threadIndex, Taskpool *threadPool, WorkStack &workStack,
                                      GlobalWorkStackQueue &globalQueue)
{
    size_t nNewlyMarked = 0;
    WeakStack weakStack;
    auto visitor = CreateTraceObjectRefFieldsVisitor(&workStack, &weakStack);
    WorkStack remarkStack;
    auto fetchFromSatbBuffer = [this, &workStack, &remarkStack]() {
        SatbBuffer::Instance().TryFetchOneRetiredNode(remarkStack);
        while (!remarkStack.empty()) {
            BaseObject *obj = remarkStack.back();
            remarkStack.pop_back();
            if (Heap::IsHeapAddress(obj) && (!MarkObject(obj))) {
                workStack.push_back(obj);
                DLOG(TRACE, "tracing take from satb buffer: obj %p", obj);
            }
        }
    };
    size_t iterationCnt = 0;
    constexpr size_t maxIterationLoopNum = 1000;
    // loop until work stack empty.
    do {
        for (;;) {
            ++nNewlyMarked;
            if (workStack.empty()) {
                break;
            }
            // get next object from work stack.
            BaseObject *obj = workStack.back();
            workStack.pop_back();
            auto region = RegionDesc::GetAliveRegionDescAt(reinterpret_cast<MAddress>((void *)obj));
            region->AddLiveByteCount(obj->GetSize());
            [[maybe_unused]] auto beforeSize = workStack.count();
            TraceObjectRefFields(obj, &visitor);
            DLOG(TRACE, "[tracing] visit finished, workstack size: before=%d, after=%d, newly added=%d", beforeSize,
                 workStack.count(), workStack.count() - beforeSize);
            // try to fork new task if needed.
            if (threadPool != nullptr) {
                TryForkTask(threadPool, workStack, globalQueue);
            }
        }

        // Try some task from satb buffer, bound the loop to make sure it converges in time
        if (++iterationCnt < maxIterationLoopNum) {
            fetchFromSatbBuffer();
            if (workStack.empty()) {
                fetchFromSatbBuffer();
            }
        }
    } while (!workStack.empty());
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

class MergeMutatorRootsScope {
public:
    MergeMutatorRootsScope() : manager_(&MutatorManager::Instance()), worldStopped_(manager_->WorldStopped())
    {
        if (!worldStopped_) {
            manager_->MutatorManagementWLock();
        }
    }

    ~MergeMutatorRootsScope()
    {
        if (!worldStopped_) {
            manager_->MutatorManagementWUnlock();
        }
    }

private:
    MutatorManager *manager_;
    bool worldStopped_;
};

void TraceCollector::MergeAllocBufferRoots(WorkStack& workStack)
{
    // hold mutator list lock to freeze mutator liveness, otherwise may access dead mutator fatally
    MergeMutatorRootsScope lockScope;
    theAllocator_.VisitAllocBuffers([&workStack](AllocationBuffer &buffer) {
        buffer.MarkStack([&workStack](BaseObject *o) { workStack.push_back(o); });
    });
}

void TraceCollector::TracingImpl(WorkStack& workStack, bool parallel, bool Remark)
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, ("CMCGC::TracingImpl_" + std::to_string(workStack.count())).c_str(), "");
    if (workStack.empty()) {
        return;
    }

    // enable parallel marking if we have thread pool.
    Taskpool *threadPool = GetThreadPool();
    ASSERT_LOGF(threadPool != nullptr, "thread pool is null");
    if (parallel) { // parallel marking.
        uint32_t parallelCount = 0;
        // During the STW remark phase, Expect it to utilize all GC threads.
        if (Remark) {
            parallelCount = GetGCThreadCount(true);
        } else {
            parallelCount = GetGCThreadCount(true) - 1;
        }
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

bool TraceCollector::AddWeakStackClearWork(WeakStack &weakStack,
                                           GlobalWeakStackQueue &globalQueue,
                                           size_t threadCount)
{
    if (weakStack.size() <= threadCount * MIN_MARKING_WORK_SIZE) {
        return false; // too less init tasks, which may lead to workload imbalance, add work rejected
    }
    DCHECK_CC(threadCount > 0);
    const size_t chunkSize = std::min(weakStack.size() / threadCount + 1, MIN_MARKING_WORK_SIZE);
    // Split the current work stack into work tasks.
    while (!weakStack.empty()) {
        WeakStackBuf *hSplit = weakStack.split(chunkSize);
        globalQueue.AddWorkStack(WeakStack(hSplit));
    }
    return true;
}

bool TraceCollector::PushRootToWorkStack(RootSet *workStack, BaseObject *obj)
{
    RegionDesc *regionInfo = RegionDesc::GetAliveRegionDescAt(reinterpret_cast<HeapAddress>(obj));
    if (gcReason_ == GCReason::GC_REASON_YOUNG && !regionInfo->IsInYoungSpace()) {
        DLOG(ENUM, "enum: skip old object %p<%p>(%zu)", obj, obj->GetTypeInfo(), obj->GetSize());
        return false;
    }

    // inline MarkObject
    bool marked = regionInfo->MarkObject(obj);
    if (!marked) {
        ASSERT(!regionInfo->IsGarbageRegion());
        DLOG(TRACE, "mark obj %p<%p>(%zu) in region %p(%u)@%#zx, live %u", obj, obj->GetTypeInfo(), obj->GetSize(),
             regionInfo, regionInfo->GetRegionType(), regionInfo->GetRegionStart(), regionInfo->GetLiveByteCount());
        workStack->push_back(obj);
        return true;
    } else {
        return false;
    }
}

void TraceCollector::PushRootsToWorkStack(RootSet *workStack, const CArrayList<BaseObject *> &collectedRoots)
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL,
                 ("CMCGC::PushRootsToWorkStack_" + std::to_string(collectedRoots.size())).c_str(), "");
    for (BaseObject *obj : collectedRoots) {
        PushRootToWorkStack(workStack, obj);
    }
}

void TraceCollector::TraceRoots(const CArrayList<BaseObject *> &collectedRoots)
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::TraceRoots", "");

    WorkStack workStack = NewWorkStack();
    PushRootsToWorkStack(&workStack, collectedRoots);

    if (Heap::GetHeap().GetGCReason() == GC_REASON_YOUNG) {
        OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::PushRootInRSet", "");
        auto func = [this, &workStack](BaseObject *object) { MarkRememberSetImpl(object, workStack); };
        RegionSpace &space = reinterpret_cast<RegionSpace &>(Heap::GetHeap().GetAllocator());
        space.MarkRememberSet(func);
    }

    COMMON_PHASE_TIMER("TraceRoots");
    VLOG(DEBUG, "roots size: %zu", workStack.size());

    ASSERT_LOGF(GetThreadPool() != nullptr, "null thread pool");

    // use fewer threads and lower priority for concurrent mark.
    const uint32_t maxWorkers = GetGCThreadCount(true) - 1;
    VLOG(DEBUG, "Concurrent mark with %u threads, workStack: %zu", (maxWorkers + 1), workStack.size());

    {
        COMMON_PHASE_TIMER("Concurrent marking");
        TracingImpl(workStack, maxWorkers > 0, false);
    }
}

void TraceCollector::Remark()
{
    WorkStack workStack = NewWorkStack();
    const uint32_t maxWorkers = GetGCThreadCount(true) - 1;
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::Remark[STW]", "");
    COMMON_PHASE_TIMER("STW re-marking");
    RemarkAndPreforwardStaticRoots(workStack);
    ConcurrentRemark(workStack, maxWorkers > 0); // Mark enqueue
    TracingImpl(workStack, maxWorkers > 0, true);
    MarkAwaitingJitFort(); // Mark awaiting
    ClearWeakStack(maxWorkers > 0);

    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::TraceRoots END",
        ("mark obejects:" + std::to_string(markedObjectCount_.load(std::memory_order_relaxed))).c_str());
    VLOG(DEBUG, "mark %zu objects", markedObjectCount_.load(std::memory_order_relaxed));
}

void TraceCollector::ClearWeakStack(bool parallel)
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::ProcessGlobalWeakStack", "");
    {
        if (gcReason_ == GC_REASON_YOUNG || globalWeakStack_.empty()) {
            return;
        }
        Taskpool *threadPool = GetThreadPool();
        ASSERT_LOGF(threadPool != nullptr, "thread pool is null");
        if (parallel) {
            uint32_t parallelCount = GetGCThreadCount(true);
            uint32_t threadCount = parallelCount + 1;
            TaskPackMonitor monitor(parallelCount, parallelCount);
            GlobalWeakStackQueue globalQueue;
            for (uint32_t i = 0; i < parallelCount; ++i) {
                threadPool->PostTask(std::make_unique<ClearWeakStackTask>(0, *this, threadPool, monitor, globalQueue));
            }
            if (!AddWeakStackClearWork(globalWeakStack_, globalQueue, static_cast<size_t>(threadCount))) {
                ProcessWeakStack(globalWeakStack_);
            }
            bool exitLoop = false;
            while (!exitLoop) {
                WeakStack stack = globalQueue.DrainAllWorkStack();
                if (stack.empty()) {
                    exitLoop = true;
                }
                ProcessWeakStack(stack);
            }
            globalQueue.NotifyFinish();
            monitor.WaitAllFinished();
        } else {
            ProcessWeakStack(globalWeakStack_);
        }
    }
}


bool TraceCollector::MarkSatbBuffer(WorkStack& workStack)
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::MarkSatbBuffer", "");
    COMMON_PHASE_TIMER("MarkSatbBuffer");
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

        while (!remarkStack.empty()) { // LCOV_EXCL_BR_LINE
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
    return true;
}

void TraceCollector::MarkRememberSetImpl(BaseObject* object, WorkStack& workStack)
{
    object->ForEachRefField([this, &workStack, &object](RefField<>& field) {
        BaseObject* targetObj = field.GetTargetObject();
        if (Heap::IsHeapAddress(targetObj)) {
            RegionDesc* region = RegionDesc::GetAliveRegionDescAt(reinterpret_cast<HeapAddress>(targetObj));
            if (region->IsInYoungSpace() &&
                !region->IsNewObjectSinceTrace(targetObj) &&
                !this->MarkObject(targetObj)) {
                workStack.push_back(targetObj);
                DLOG(TRACE, "remember set trace obj: %p@%p, ref: %p", object, &field, targetObj);
            }
        }
    });
}

void TraceCollector::ConcurrentRemark(WorkStack& remarkStack, bool parallel)
{
    LOGF_CHECK(MarkSatbBuffer(remarkStack)) << "not cleared\n";
}

void TraceCollector::MarkAwaitingJitFort()
{
    reinterpret_cast<RegionSpace&>(theAllocator_).MarkAwaitingJitFort();
}

void TraceCollector::Init(const RuntimeParam& param) {}

void TraceCollector::Fini() { Collector::Fini(); }

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
    // SatbBuffer should be initialized before concurrent enumeration.
    SatbBuffer::Instance().Init();
    // prepare thread pool.

    GCStats& gcStats = GetGCStats();
    gcStats.reason = gcReason_;
    gcStats.async = !g_gcRequests[gcReason_].IsSyncGC();
    gcStats.gcType = gcType_;
    gcStats.isConcurrentMark = isConcurrent;
    gcStats.collectedBytes = 0;
    gcStats.smallGarbageSize = 0;
    gcStats.pinnedGarbageSize = 0;
    gcStats.gcStartTime = TimeUtil::NanoSeconds();
    gcStats.totalSTWTime = 0;
    gcStats.maxSTWTime = 0;
#if defined(GCINFO_DEBUG) && GCINFO_DEBUG
    DumpBeforeGC();
#endif
}

void TraceCollector::PostGarbageCollection(uint64_t gcIndex)
{
    SatbBuffer::Instance().ReclaimALLPages();
    // release pages in PagePool
    PagePool::Instance().Trim();
    collectorResources_.MarkGCFinish(gcIndex);

#if defined(GCINFO_DEBUG) && GCINFO_DEBUG
    DumpAfterGC();
#endif
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
    gcStats.heapThreshold = std::max(gcStats.heapThreshold, 20 * MB); // 20 MB:set 20 MB as min heapThreshold
    gcStats.heapThreshold = std::min(gcStats.heapThreshold, gcParam.gcThreshold);

    UpdateNativeThreshold(gcParam);
    Heap::GetHeap().RecordAliveSizeAfterLastGC(bytesAllocated);
    if (!gcStats.isYoungGC()) {
        Heap::GetHeap().SetRecordHeapObjectSizeBeforeSensitive(bytesAllocated);
    }

    if (!gcStats.isYoungGC()) {
        g_gcRequests[GC_REASON_HEU].SetMinInterval(gcParam.gcInterval);
    } else {
        g_gcRequests[GC_REASON_YOUNG].SetMinInterval(gcParam.gcInterval);
    }
    gcStats.IncreaseAccumulatedFreeSize(bytesAllocated);
    std::ostringstream oss;
    oss << "allocated bytes " << bytesAllocated << " (survive bytes " << survivedBytes
                     << ", recent-allocated " << recentBytes << "), update target footprint "
                     << oldTargetFootprint << " -> " << gcStats.targetFootprint
                     << ", update gc threshold " << oldThreshold << " -> " << gcStats.heapThreshold;
    VLOG(INFO, oss.str().c_str());
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::UpdateGCStats END", (
                    "allocated bytes:" + std::to_string(bytesAllocated) +
                    ";survive bytes:" + std::to_string(survivedBytes) +
                    ";recent allocated:" + std::to_string(recentBytes) +
                    ";update target footprint:" + std::to_string(oldTargetFootprint) +
                    ";new target footprint:" + std::to_string(gcStats.targetFootprint) +
                    ";old gc threshold:" + std::to_string(oldThreshold) +
                    ";new gc threshold:" + std::to_string(gcStats.heapThreshold) +
                    ";native size:" + std::to_string(Heap::GetHeap().GetNotifiedNativeSize()) +
                    ";new native threshold:" + std::to_string(Heap::GetHeap().GetNativeHeapThreshold())
                ).c_str());
    space.DumpAllRegionStats("End GC log", gcReason_, gcType_);
}

void TraceCollector::UpdateNativeThreshold(GCParam& gcParam)
{
    size_t nativeHeapSize = Heap::GetHeap().GetNotifiedNativeSize();
    size_t newNativeHeapThreshold = Heap::GetHeap().GetNotifiedNativeSize();
    if (nativeHeapSize < MAX_NATIVE_SIZE_INC) {
        newNativeHeapThreshold = std::max(nativeHeapSize + gcParam.minGrowBytes,
                                          nativeHeapSize * NATIVE_MULTIPLIER);
    } else {
        newNativeHeapThreshold += MAX_NATIVE_STEP;
    }
    newNativeHeapThreshold = std::min(newNativeHeapThreshold, MAX_GLOBAL_NATIVE_LIMIT);
    Heap::GetHeap().SetNativeHeapThreshold(newNativeHeapThreshold);
    collectorResources_.SetIsNativeGCInvoked(false);
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

void TraceCollector::ReclaimGarbageMemory(GCReason reason)
{
    if (reason != GC_REASON_YOUNG) {
        Heap::GetHeap().GetAllocator().ReclaimGarbageMemory(true);
    } else {
        Heap::GetHeap().GetAllocator().ReclaimGarbageMemory(false);
    }
}

void TraceCollector::RunGarbageCollection(uint64_t gcIndex, GCReason reason, GCType gcType)
{
    gcReason_ = reason;
    gcType_ = gcType;
    auto gcReasonName = std::string(g_gcRequests[gcReason_].name);
    auto currentAllocatedSize = Heap::GetHeap().GetAllocatedSize();
    auto currentThreshold = Heap::GetHeap().GetCollector().GetGCStats().GetThreshold();
    RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
    space.DumpAllRegionStats("Begin GC log", gcReason_, gcType_);
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::RunGarbageCollection", (
                    "GCReason:" + gcReasonName + ";GCType:" + GCTypeToString(gcType) +
                    ";Sensitive:" + std::to_string(static_cast<int>(Heap::GetHeap().GetSensitiveStatus())) +
                    ";Startup:" + std::to_string(static_cast<int>(Heap::GetHeap().GetStartupStatus())) +
                    ";Current Allocated:" + Pretty(currentAllocatedSize) +
                    ";Current Threshold:" + Pretty(currentThreshold) +
                    ";Current Native:" + Pretty(Heap::GetHeap().GetNotifiedNativeSize()) +
                    ";NativeThreshold:" + Pretty(Heap::GetHeap().GetNativeHeapThreshold())
                ).c_str());
    // prevent other threads stop-the-world during GC.
    // this may be removed in the future.
    ScopedSTWLock stwLock;
    PreGarbageCollection(true);
    Heap::GetHeap().SetGCReason(reason);
    GCStats& gcStats = GetGCStats();

    DoGarbageCollection();

    HeapBitmapManager::GetHeapBitmapManager().ClearHeapBitmap();

    ReclaimGarbageMemory(reason);

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
        VLOG(INFO, oss.str().c_str());
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
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::CopyFromSpace", "");
    TransitionToGCPhase(GCPhase::GC_PHASE_COPY, true);
    RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
    GCStats& stats = GetGCStats();
    stats.liveBytesBeforeGC = space.GetAllocatedBytes();
    stats.fromSpaceSize = space.FromSpaceSize();
    space.CopyFromSpace(GetThreadPool());

    stats.smallGarbageSize = space.FromRegionSize() - space.ToSpaceSize();
}

void TraceCollector::ExemptFromSpace()
{
    RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
    space.ExemptFromSpace();
}

} // namespace common
