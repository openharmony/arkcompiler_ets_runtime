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
Barrier** Heap::currentBarrierPtr = nullptr;
Barrier* Heap::stwBarrierPtr = nullptr;
HeapAddress Heap::heapStartAddr = 0;
HeapAddress Heap::heapCurrentEnd = 0;

class HeapImpl : public Heap {
public:
    HeapImpl()
        : theSpace(Allocator::CreateAllocator()), collectorResources(collectorProxy),
          collectorProxy(*theSpace, collectorResources), stwBarrier(collectorProxy),
        idleBarrier(collectorProxy), enumBarrier(collectorProxy), traceBarrier(collectorProxy),
        postTraceBarrier(collectorProxy), preforwardBarrier(collectorProxy), copyBarrier(collectorProxy)
    {
        currentBarrier = &stwBarrier;
        stwBarrierPtr = &stwBarrier;
        Heap::currentBarrierPtr = &currentBarrier;
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

    bool IsGcStarted() const override { return collectorResources.IsGcStarted(); }

    void WaitForGCFinish() override { return collectorResources.WaitForGCFinish(); }

    bool IsGCEnabled() const override { return isGCEnabled.load(); }

    void EnableGC(bool val) override { return isGCEnabled.store(val); }

    GCReason GetGCReason() override { return gcReason; }

    void SetGCReason(GCReason reason) override { gcReason = reason; }

    HeapAddress Allocate(size_t size, AllocType allocType, bool allowGC = true) override;

    GCPhase GetGCPhase() const override;
    void SetGCPhase(const GCPhase phase) override;
    Collector& GetCollector() override;
    Allocator& GetAllocator() override;

    size_t GetMaxCapacity() const override;
    size_t GetCurrentCapacity() const override;
    size_t GetUsedPageSize() const override;
    size_t GetAllocatedSize() const override;
    HeapAddress GetStartAddress() const override;
    HeapAddress GetSpaceEndAddress() const override;
    void RegisterStaticRoots(uintptr_t addr, uint32_t) override;
    void UnregisterStaticRoots(uintptr_t addr, uint32_t) override;
    void VisitStaticRoots(const RefFieldVisitor& visitor) override;
    bool ForEachObject(const std::function<void(BaseObject*)>&, bool) override;
    ssize_t GetHeapPhysicalMemorySize() const override;
    void InstallBarrier(const GCPhase phase) override;
    FinalizerProcessor& GetFinalizerProcessor() override;
    CollectorResources& GetCollectorResources() override;
    void RegisterAllocBuffer(AllocationBuffer& buffer) override;
    void StopGCWork() override;
    void TryHeuristicGC() override;
    class ScopedFileHandler {
    public:
        ScopedFileHandler(const char* fileName, const char* mode) { file = fopen(fileName, mode); }
        ~ScopedFileHandler()
        {
            if (file != nullptr) {
                fclose(file);
            }
        }
        FILE* GetFile() const { return file; }

    private:
        FILE* file = nullptr;
    };

private:
    // allocator is actually a subspace in heap
    Allocator* theSpace;

    CollectorResources collectorResources;

    // collector is closely related to barrier. but we do not put barrier inside collector because even without
    // collector (i.e. no-gc), allocator and barrier (interface to access heap) is still needed.
    CollectorProxy collectorProxy;

    Barrier stwBarrier;
    IdleBarrier idleBarrier;
    EnumBarrier enumBarrier;
    TraceBarrier traceBarrier;
    PostTraceBarrier postTraceBarrier;
    PreforwardBarrier preforwardBarrier;
    CopyBarrier copyBarrier;
    Barrier* currentBarrier = nullptr;
    HeuristicGCPolicy heuristicGCPolicy;
    // manage gc roots entry
    StaticRootTable staticRootTable;

    std::atomic<bool> isGCEnabled = { true };
    GCReason gcReason = GCReason::GC_REASON_INVALID;
}; // end class HeapImpl

static ImmortalWrapper<HeapImpl> g_heapInstance;

HeapAddress HeapImpl::Allocate(size_t size, AllocType allocType, bool allowGC)
{
    if (allowGC) {
        return theSpace->Allocate(size, allocType);
    } else {
        return theSpace->AllocateNoGC(size, allocType);
    }
}

bool HeapImpl::ForEachObject(const std::function<void(BaseObject*)>& visitor, bool safe)
{
    {
        ScopedEnterSaferegion enterSaferegion(false);
        this->GetCollectorResources().WaitForGCFinish();
    }
    // Expect no gc in ForEachObj, dfx tools and oom gc should be considered.
    return theSpace->ForEachObject(visitor, safe);
}

void HeapImpl::Init(const RuntimeParam& param)
{
    if (theSpace == nullptr) {
        // Hack impl, since HeapImpl is Immortal, this may happen in multi UT case
        new (this) HeapImpl();
    }
    theSpace->Init(param);
    Heap::GetHeap().EnableGC(param.gcParam.enableGC);
    collectorProxy.Init(param);
    collectorResources.Init();
    heuristicGCPolicy.Init();
}

void HeapImpl::Fini()
{
    collectorResources.Fini();
    collectorProxy.Fini();
    if (theSpace != nullptr) {
        delete theSpace;
        theSpace = nullptr;
    }
}

void HeapImpl::StartRuntimeThreads()
{
    collectorResources.StartRuntimeThreads();
}

void HeapImpl::StopRuntimeThreads()
{
    collectorResources.StopRuntimeThreads();
}

void HeapImpl::TryHeuristicGC()
{
    heuristicGCPolicy.TryHeuristicGC();
}

Collector& HeapImpl::GetCollector() { return collectorProxy.GetCurrentCollector(); }

Allocator& HeapImpl::GetAllocator() { return *theSpace; }

void HeapImpl::InstallBarrier(const GCPhase phase)
{
    if (phase == GCPhase::GC_PHASE_ENUM) {
        currentBarrier = &enumBarrier;
    } else if (phase == GCPhase::GC_PHASE_MARK || phase == GCPhase::GC_PHASE_REMARK_SATB) {
        currentBarrier = &traceBarrier;
    } else if (phase == GCPhase::GC_PHASE_PRECOPY) {
        currentBarrier = &preforwardBarrier;
    } else if (phase == GCPhase::GC_PHASE_COPY || phase == GCPhase::GC_PHASE_FIX) {
        currentBarrier = &copyBarrier;
    } else if (phase == GCPhase::GC_PHASE_IDLE) {
        currentBarrier = &idleBarrier;
    } else if (phase == GCPhase::GC_PHASE_POST_MARK) {
        currentBarrier = &postTraceBarrier;
    }
    DLOG(GCPHASE, "install barrier for gc phase %u", phase);
}

GCPhase HeapImpl::GetGCPhase() const { return collectorProxy.GetGCPhase(); }

void HeapImpl::SetGCPhase(const GCPhase phase) { collectorProxy.SetGCPhase(phase); }

size_t HeapImpl::GetMaxCapacity() const { return theSpace->GetMaxCapacity(); }

size_t HeapImpl::GetCurrentCapacity() const { return theSpace->GetCurrentCapacity(); }

size_t HeapImpl::GetUsedPageSize() const { return theSpace->GetUsedPageSize(); }

size_t HeapImpl::GetAllocatedSize() const { return theSpace->GetAllocatedBytes(); }

HeapAddress HeapImpl::GetStartAddress() const { return theSpace->GetSpaceStartAddress(); }

HeapAddress HeapImpl::GetSpaceEndAddress() const { return theSpace->GetSpaceEndAddress(); }

Heap& Heap::GetHeap() { return *g_heapInstance; }

void HeapImpl::RegisterStaticRoots(uintptr_t addr, uint32_t size)
{
    staticRootTable.RegisterRoots(reinterpret_cast<StaticRootTable::StaticRootArray*>(addr), size);
}

void HeapImpl::UnregisterStaticRoots(uintptr_t addr, uint32_t size)
{
    staticRootTable.UnregisterRoots(reinterpret_cast<StaticRootTable::StaticRootArray*>(addr), size);
}

void HeapImpl::VisitStaticRoots(const RefFieldVisitor& visitor) { staticRootTable.VisitRoots(visitor); }

#if defined(_WIN64)
ssize_t HeapImpl::GetHeapPhysicalMemorySize() const
{
    PROCESS_MEMORY_COUNTERS memCounter;
    HANDLE hProcess = GetCurrentProcess();
    if (GetProcessMemoryInfo(hProcess, &memCounter, sizeof(memCounter))) {
        size_t physicalMemorySize = memCounter.WorkingSetSize;
        if (physicalMemorySize > std::numeric_limits<ssize_t>::max()) {
            LOG_COMMON(ERROR) << "PhysicalMemorySize is too large";
            CloseHandle(hProcess);
            return -2; // -2: Return value of exception.
        }
        CloseHandle(hProcess);
        return static_cast<ssize_t>(physicalMemorySize);
    } else {
        LOG_COMMON(ERROR) << "GetHeapPhysicalMemorySize fail";
        CloseHandle(hProcess);
        return -2; // -2: Return value of exception.
    }
    return 0;
}

#elif defined(__APPLE__)
ssize_t HeapImpl::GetHeapPhysicalMemorySize() const
{
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count) != KERN_SUCCESS)
        return -2;  // -2: Return value of exception.
    return t_info.resident_size;
}

#else
ssize_t HeapImpl::GetHeapPhysicalMemorySize() const
{
    CString smapsFile = CString("/proc/") + CString(GetPid()) + "/smaps";
    ScopedFileHandler fileHandler(smapsFile.Str(), "r");
    FILE* file = fileHandler.GetFile();
    if (file == nullptr) {
        LOG_COMMON(ERROR) << "GetHeapPhysicalMemorySize(): fail to open the file";
        return -1;
    }
    const int bufSize = 256;
    char buf[bufSize] = { '\0' };
    while (fgets(buf, bufSize, file) != nullptr) {
        uint64_t startAddr = 0;
        uint64_t endAddr = 0;
        // expect 2 parameters are both written.
        constexpr int expectResult = 2;
        int ret = sscanf_s(buf, "%lx-%lx rw-p", &startAddr, &endAddr);
        if (ret == expectResult && startAddr <= GetHeap().GetStartAddress() && endAddr >= GetHeap().GetStartAddress()) {
            ssize_t physicalMemorySize = 0;
            uint64_t tmpStartAddr = endAddr;
            do {
                bool getPss = false;
                if (tmpStartAddr != endAddr) {
                    LOG_COMMON(ERROR) << "GetHeapPhysicalMemorySize(): fail to read the file";
                    return -2; // -2: Return value of exception.
                }
                do {
                    (void)fgets(buf, bufSize, file);
                    ssize_t size = 0;
                    if (sscanf_s(buf, "Pss:%zdKB", &size) == 1) {
                        physicalMemorySize += size;
                        getPss = true;
                    }
                } while (sscanf_s(buf, "%lx-%lx", &startAddr, &endAddr) != 2); // expect 2 parameters are both written.
                if (!getPss) {
                    LOG_COMMON(ERROR) << "GetHeapPhysicalMemorySize(): fail to read pss value";
                    return -2; // -2: Return value of exception.
                }
                tmpStartAddr = endAddr;
            } while (endAddr <= GetHeap().GetSpaceEndAddress());
            return physicalMemorySize * KB;
        }
    }
    LOG_COMMON(ERROR) << "GetHeapPhysicalMemorySize fail";
    return -2; // -2: Return value of exception.
}
#endif

FinalizerProcessor& HeapImpl::GetFinalizerProcessor() { return collectorResources.GetFinalizerProcessor(); }

CollectorResources& HeapImpl::GetCollectorResources() { return collectorResources; }

void HeapImpl::StopGCWork() { collectorResources.StopGCWork(); }

void HeapImpl::RegisterAllocBuffer(AllocationBuffer& buffer) { GetAllocator().RegisterAllocBuffer(buffer); }
} // namespace common
