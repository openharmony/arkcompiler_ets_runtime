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

#include "common_interfaces/base_runtime.h"

#include "common_components/base_runtime/base_runtime_param.h"
#include "common_components/base_runtime/hooks.h"
#include "common_components/common/page_pool.h"
#include "common_components/heap/allocator/region_desc.h"
#include "common_components/heap/collector/heuristic_gc_policy.h"
#include "common_components/heap/heap.h"
#include "common_components/heap/heap_manager.h"
#include "common_components/mutator/mutator_manager.h"
#include "common_interfaces/objects/composite_base_class.h"
#include "common_components/objects/string_table_internal.h"
#include "common_interfaces/objects/base_string_table.h"
#include "common_interfaces/thread/thread_state_transition.h"

namespace panda::ecmascript {
class TaggedObject;
}

namespace common {
using panda::ecmascript::TaggedObject;

std::mutex BaseRuntime::vmCreationLock_;
BaseRuntime *BaseRuntime::baseRuntimeInstance_ = nullptr;
bool BaseRuntime::initialized_ = false;

BaseRuntime *BaseRuntime::GetInstance()
{
    if (UNLIKELY_CC(baseRuntimeInstance_ == nullptr)) {
        std::unique_lock<std::mutex> lock(vmCreationLock_);
        if (baseRuntimeInstance_ == nullptr) {
            baseRuntimeInstance_ = new BaseRuntime();
        }
    }
    return baseRuntimeInstance_;
}

void BaseRuntime::DestroyInstance()
{
    std::unique_lock<std::mutex> lock(vmCreationLock_);
    delete baseRuntimeInstance_;
    baseRuntimeInstance_ = nullptr;
}

template<typename T>
inline T* NewAndInit()
{
    T* temp = new (std::nothrow) T();
    LOGF_CHECK(temp != nullptr) << "NewAndInit failed";
    temp->Init();
    return temp;
}

template<typename T, typename A>
inline T* NewAndInit(A arg)
{
    T* temp = new (std::nothrow) T();
    LOGF_CHECK(temp != nullptr) << "NewAndInit failed";
    temp->Init(arg);
    return temp;
}

template<typename T>
inline void CheckAndFini(T*& module)
{
    if (module != nullptr) {
        module->Fini();
    }

    delete module;
    module = nullptr;
}

void BaseRuntime::Init()
{
    Init(BaseRuntimeParam::DefaultRuntimeParam());
}

void BaseRuntime::Init(const RuntimeParam &param)
{
    std::unique_lock<std::mutex> lock(vmCreationLock_);
    if (initialized_) {
        LOG_COMMON(FATAL) << "BaseRuntime has been initialized and don't need init again.";
        return;
    }

    param_ = param;

    PagePool::Instance().Init(param_.heapParam.heapSize * KB / COMMON_PAGE_SIZE);
    mutatorManager_ = NewAndInit<MutatorManager>();
    heapManager_ = NewAndInit<HeapManager>(param_);
    baseClassRoots_ = NewAndInit<BaseClassRoots>();
    stringTable_ = NewAndInit<BaseStringTableImpl>();
    LOG_COMMON(INFO) << "Arkcommon runtime started.";
    // Record runtime parameter to report. heap growth value needs to plus 1.
    VLOG(REPORT, "Runtime parameter:\n\tHeap size: %zu(KB)\n\tRegion size: %zu(KB)\n\tExemption threshold: %.2f\n\t"
        "Heap utilization: %.2f\n\tHeap growth: %.2f\n\tAllocation rate: %.2f(MB/s)\n\tAlloction wait time: %zuns\n\t"
        "GC Threshold: %zu(KB)\n\tGarbage threshold: %.2f\n\tGC interval: %zums\n\tBackup GC interval: %zus\n\t"
        "Log level: %d\n\tThread stack size: %zu(KB)\n\tArkcommon stack size: %zu(KB)\n\t"
        "Processor number: %d", param_.heapParam.heapSize, param_.heapParam.regionSize,
        param_.heapParam.exemptionThreshold, param_.heapParam.heapUtilization, 1 + param_.heapParam.heapGrowth,
        param_.heapParam.allocationRate, param_.heapParam.allocationWaitTime,
        param_.gcParam.gcThreshold / KB, param_.gcParam.garbageThreshold,
        param_.gcParam.gcInterval / MILLI_SECOND_TO_NANO_SECOND,
        param_.gcParam.backupGCInterval / SECOND_TO_NANO_SECOND);

    initialized_ = true;
}

void BaseRuntime::Fini()
{
    std::unique_lock<std::mutex> lock(vmCreationLock_);
    if (!initialized_) {
        LOG_COMMON(FATAL) << "BaseRuntime has been initialized and don't need init again.";
        return;
    }

    {
        // since there might be failure during initialization,
        // here we need to check and call fini.
        CheckAndFini<HeapManager>(heapManager_);
        CheckAndFini<MutatorManager>(mutatorManager_);
        CheckAndFini<BaseClassRoots>(baseClassRoots_);
        CheckAndFini<BaseStringTableImpl>(reinterpret_cast<BaseStringTableImpl*&>(stringTable_));
        PagePool::Instance().Fini();
    }

    LOG_COMMON(INFO) << "Arkcommon runtime shutdown.";
    initialized_ = false;
}

void BaseRuntime::PreFork(ThreadHolder *holder)
{
    // Need appspawn space and compress gc.
    RequestGC(GcType::APPSPAWN);
    {
        ThreadNativeScope scope(holder);
        HeapManager::StopRuntimeThreads();
    }
}

void BaseRuntime::PostFork()
{
    HeapManager::StartRuntimeThreads();
#ifdef ENABLE_COLD_STARTUP_GC_POLICY
    StartupStatusManager::OnAppStartup();
#endif
}

void BaseRuntime::WriteBarrier(void* obj, void* field, void* ref)
{
    Heap::GetBarrier().WriteBarrier(reinterpret_cast<BaseObject*>(obj),
        *reinterpret_cast<RefField<>*>(field), reinterpret_cast<BaseObject*>(ref));
}

void* BaseRuntime::ReadBarrier(void* obj, void* field)
{
    return reinterpret_cast<void*>(Heap::GetBarrier().ReadRefField(reinterpret_cast<BaseObject*>(obj),
        *reinterpret_cast<RefField<false>*>(field)));
}

void* BaseRuntime::ReadBarrier(void* field)
{
    return reinterpret_cast<void*>(Heap::GetBarrier().ReadStaticRef(*reinterpret_cast<RefField<false>*>(field)));
}

void* BaseRuntime::AtomicReadBarrier(void* obj, void* field, std::memory_order order)
{
    return reinterpret_cast<void*>(Heap::GetBarrier().AtomicReadRefField(reinterpret_cast<BaseObject*>(obj),
        *reinterpret_cast<RefField<true>*>(field), order));
}

void BaseRuntime::RequestGC(GcType type)
{
    switch (type) {
        case GcType::SYNC: {
            HeapManager::RequestGC(GC_REASON_USER, false);
            break;
        }
        case GcType::ASYNC: {
            HeapManager::RequestGC(GC_REASON_USER, true);
            break;
        }
        case GcType::FULL: {
            HeapManager::RequestGC(GC_REASON_BACKUP, false);
            break;
        }
        case GcType::APPSPAWN: {
            HeapManager::RequestGC(GC_REASON_APPSPAWN, false);
            break;
        }
    }
}

void BaseRuntime::WaitForGCFinish() { Heap::GetHeap().WaitForGCFinish(); }

bool BaseRuntime::ForEachObj(HeapVisitor& visitor, bool safe)
{
    return Heap::GetHeap().ForEachObject(visitor, safe);
}
}  // namespace common
