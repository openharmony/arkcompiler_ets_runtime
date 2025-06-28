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

#include "common_components/heap/heap.h"

#include "common_components/heap/collector/collector_proxy.h"
#include "common_components/heap/collector/collector_resources.h"
#include "common_components/heap/collector/heuristic_gc_policy.h"
#include "common_components/heap/w_collector/idle_barrier.h"
#include "common_components/heap/w_collector/enum_barrier.h"
#include "common_components/heap/w_collector/trace_barrier.h"
#include "common_components/heap/w_collector/remark_barrier.h"
#include "common_components/heap/w_collector/post_trace_barrier.h"
#include "common_components/heap/w_collector/preforward_barrier.h"
#include "common_components/heap/w_collector/copy_barrier.h"
#include "common_components/mutator/mutator_manager.h"
#include "common_interfaces/base_runtime.h"
#if defined(_WIN64)
#include <windows.h>
#include <psapi.h>
#undef ERROR
#endif
#if defined(__APPLE__)
#include <mach/mach.h>
#endif
namespace common {
std::atomic<Barrier*>* Heap::currentBarrierPtr_ = nullptr;
Barrier* Heap::stwBarrierPtr_ = nullptr;
HeapAddress Heap::heapStartAddr_ = 0;
HeapAddress Heap::heapCurrentEnd_ = 0;

class HeapImpl : public Heap {
public:
    HeapImpl()
        : theSpace_(Allocator::CreateAllocator()), collectorResources_(collectorProxy_),
          collectorProxy_(*theSpace_, collectorResources_), stwBarrier_(collectorProxy_),
        idleBarrier_(collectorProxy_), enumBarrier_(collectorProxy_), traceBarrier_(collectorProxy_),
        remarkBarrier_(collectorProxy_), postTraceBarrier_(collectorProxy_), preforwardBarrier_(collectorProxy_),
        copyBarrier_(collectorProxy_)
    {
        currentBarrier_.store(&stwBarrier_, std::memory_order_relaxed);
        stwBarrierPtr_ = &stwBarrier_;
        Heap::currentBarrierPtr_ = &currentBarrier_;
        RunType::InitRunTypeMap();
    }

    ~HeapImpl() override = default;
    void Init(const RuntimeParam& param) override;
    void Fini() override;
    void StartRuntimeThreads() override;
    void StopRuntimeThreads() override;

    bool IsSurvivedObject(const BaseObject* obj) const override
    {
        return RegionSpace::IsMarkedObject(obj) || RegionSpace::IsResurrectedObject(obj);
    }

    bool IsGcStarted() const override { return collectorResources_.IsGcStarted(); }

    void WaitForGCFinish() override { return collectorResources_.WaitForGCFinish(); }

    bool IsGCEnabled() const override { return isGCEnabled_.load(); }

    void EnableGC(bool val) override { return isGCEnabled_.store(val); }

    GCReason GetGCReason() override { return gcReason_; }

    void SetGCReason(GCReason reason) override { gcReason_ = reason; }

    HeapAddress Allocate(size_t size, AllocType allocType, bool allowGC = true) override;

    GCPhase GetGCPhase() const override;
    void SetGCPhase(const GCPhase phase) override;
    Collector& GetCollector() override;
    Allocator& GetAllocator() override;

    size_t GetMaxCapacity() const override;
    size_t GetCurrentCapacity() const override;
    size_t GetUsedPageSize() const override;
    size_t GetAllocatedSize() const override;
    size_t GetRemainHeapSize() const override;
    size_t GetAccumulatedAllocateSize() const override;
    size_t GetAccumulatedFreeSize() const override;
    HeapAddress GetStartAddress() const override;
    HeapAddress GetSpaceEndAddress() const override;
    void VisitStaticRoots(const RefFieldVisitor& visitor) override;
    bool ForEachObject(const std::function<void(BaseObject*)>&, bool) override;
    void InstallBarrier(const GCPhase phase) override;
    FinalizerProcessor& GetFinalizerProcessor() override;
    CollectorResources& GetCollectorResources() override;
    void RegisterAllocBuffer(AllocationBuffer& buffer) override;
    void StopGCWork() override;
    void TryHeuristicGC() override;
    void NotifyNativeAllocation(size_t bytes) override;
    void NotifyNativeFree(size_t bytes) override;
    void NotifyNativeReset(size_t oldBytes, size_t newBytes) override;
    size_t GetNotifiedNativeSize() override;
    void SetNativeHeapThreshold(size_t newThreshold) override;
    size_t GetNativeHeapThreshold() override;

private:
    // allocator is actually a subspace in heap
    Allocator* theSpace_;

    CollectorResources collectorResources_;

    // collector is closely related to barrier. but we do not put barrier inside collector because even without
    // collector (i.e. no-gc), allocator and barrier (interface to access heap) is still needed.
    CollectorProxy collectorProxy_;

    Barrier stwBarrier_;
    IdleBarrier idleBarrier_;
    EnumBarrier enumBarrier_;
    TraceBarrier traceBarrier_;
    RemarkBarrier remarkBarrier_;
    PostTraceBarrier postTraceBarrier_;
    PreforwardBarrier preforwardBarrier_;
    CopyBarrier copyBarrier_;
    std::atomic<Barrier*> currentBarrier_ = nullptr;
    HeuristicGCPolicy heuristicGCPolicy_;
    // manage gc roots entry
    StaticRootTable staticRootTable_;

    std::atomic<bool> isGCEnabled_ = { true };

    GCReason gcReason_ = GCReason::GC_REASON_INVALID;
}; // end class HeapImpl

static ImmortalWrapper<HeapImpl> g_heapInstance;

HeapAddress HeapImpl::Allocate(size_t size, AllocType allocType, bool allowGC)
{
    if (allowGC) {
        return theSpace_->Allocate(size, allocType);
    } else {
        return theSpace_->AllocateNoGC(size, allocType);
    }
}

bool HeapImpl::ForEachObject(const std::function<void(BaseObject*)>& visitor, bool safe)
{
    {
        ScopedEnterSaferegion enterSaferegion(false);
        this->GetCollectorResources().WaitForGCFinish();
    }
    // Expect no gc in ForEachObj, dfx tools and oom gc should be considered.
    return theSpace_->ForEachObject(visitor, safe);
}

void HeapImpl::Init(const RuntimeParam& param)
{
    if (theSpace_ == nullptr) {
        // Hack impl, since HeapImpl is Immortal, this may happen in multi UT case
        new (this) HeapImpl();
    }
    theSpace_->Init(param);
    Heap::GetHeap().EnableGC(param.gcParam.enableGC);
    collectorProxy_.Init(param);
    collectorResources_.Init();
    heuristicGCPolicy_.Init();
}

void HeapImpl::Fini()
{
    collectorResources_.Fini();
    collectorProxy_.Fini();
    if (theSpace_ != nullptr) {
        delete theSpace_;
        theSpace_ = nullptr;
    }
}

void HeapImpl::StartRuntimeThreads()
{
    collectorResources_.StartRuntimeThreads();
}

void HeapImpl::StopRuntimeThreads()
{
    collectorResources_.StopRuntimeThreads();
}

void HeapImpl::TryHeuristicGC()
{
    heuristicGCPolicy_.TryHeuristicGC();
}

void HeapImpl::NotifyNativeAllocation(size_t bytes)
{
    heuristicGCPolicy_.NotifyNativeAllocation(bytes);
}

void HeapImpl::NotifyNativeFree(size_t bytes)
{
    heuristicGCPolicy_.NotifyNativeFree(bytes);
}

void HeapImpl::NotifyNativeReset(size_t oldBytes, size_t newBytes)
{
    heuristicGCPolicy_.NotifyNativeFree(oldBytes);
    heuristicGCPolicy_.NotifyNativeAllocation(newBytes);
}

size_t HeapImpl::GetNotifiedNativeSize()
{
    return heuristicGCPolicy_.GetNotifiedNativeSize();
}

void HeapImpl::SetNativeHeapThreshold(size_t newThreshold)
{
    heuristicGCPolicy_.SetNativeHeapThreshold(newThreshold);
}

size_t HeapImpl::GetNativeHeapThreshold()
{
    return heuristicGCPolicy_.GetNativeHeapThreshold();
}

Collector& HeapImpl::GetCollector() { return collectorProxy_.GetCurrentCollector(); }

Allocator& HeapImpl::GetAllocator() { return *theSpace_; }

void HeapImpl::InstallBarrier(const GCPhase phase)
{
    if (phase == GCPhase::GC_PHASE_ENUM) {
        currentBarrier_.store(&enumBarrier_, std::memory_order_relaxed);
    } else if (phase == GCPhase::GC_PHASE_MARK) {
        currentBarrier_.store(&traceBarrier_, std::memory_order_relaxed);
    } else if (phase == GCPhase::GC_PHASE_PRECOPY) {
        currentBarrier_.store(&preforwardBarrier_, std::memory_order_relaxed);
    } else if (phase == GCPhase::GC_PHASE_COPY || phase == GCPhase::GC_PHASE_FIX) {
        currentBarrier_.store(&copyBarrier_, std::memory_order_relaxed);
    } else if (phase == GCPhase::GC_PHASE_IDLE) {
        currentBarrier_.store(&idleBarrier_, std::memory_order_relaxed);
    } else if (phase == GCPhase::GC_PHASE_POST_MARK) {
        currentBarrier_.store(&postTraceBarrier_, std::memory_order_relaxed);
    } else if (phase == GCPhase::GC_PHASE_FINAL_MARK ||
               phase == GCPhase::GC_PHASE_REMARK_SATB) {
        currentBarrier_ = &remarkBarrier_;
    }
    DLOG(GCPHASE, "install barrier for gc phase %u", phase);
}

GCPhase HeapImpl::GetGCPhase() const { return collectorProxy_.GetGCPhase(); }

void HeapImpl::SetGCPhase(const GCPhase phase) { collectorProxy_.SetGCPhase(phase); }

size_t HeapImpl::GetMaxCapacity() const { return theSpace_->GetMaxCapacity(); }

size_t HeapImpl::GetCurrentCapacity() const { return theSpace_->GetCurrentCapacity(); }

size_t HeapImpl::GetUsedPageSize() const { return theSpace_->GetUsedPageSize(); }

size_t HeapImpl::GetAllocatedSize() const { return theSpace_->GetAllocatedBytes(); }

size_t HeapImpl::GetRemainHeapSize() const { return theSpace_->GetMaxCapacity() - theSpace_->GetUsedPageSize(); }

size_t HeapImpl::GetAccumulatedAllocateSize() const
{
    return collectorResources_.GetGCStats().GetAccumulatedFreeSize() + theSpace_->GetUsedPageSize();
}

size_t HeapImpl::GetAccumulatedFreeSize() const { return collectorResources_.GetGCStats().GetAccumulatedFreeSize(); }

HeapAddress HeapImpl::GetStartAddress() const { return theSpace_->GetSpaceStartAddress(); }

HeapAddress HeapImpl::GetSpaceEndAddress() const { return theSpace_->GetSpaceEndAddress(); }

Heap& Heap::GetHeap() { return *g_heapInstance; }

void HeapImpl::VisitStaticRoots(const RefFieldVisitor& visitor) { staticRootTable_.VisitRoots(visitor); }

FinalizerProcessor& HeapImpl::GetFinalizerProcessor() { return collectorResources_.GetFinalizerProcessor(); }

CollectorResources& HeapImpl::GetCollectorResources() { return collectorResources_; }

void HeapImpl::StopGCWork() { collectorResources_.StopGCWork(); }

void HeapImpl::RegisterAllocBuffer(AllocationBuffer& buffer) { GetAllocator().RegisterAllocBuffer(buffer); }
} // namespace common
