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
#include "common_components/common_runtime/src/common_runtime.h"

#include "common_components/common_runtime/src/heap/heap.h"
#include "common_components/common_runtime/src/heap_manager.h"
#if defined(__linux__) || defined(PANDA_TARGET_OHOS) || defined(__APPLE__)
#endif
#ifdef _WIN64
#include "common_components/common_runtime/src/os/windows/win_module_manager.h"
#endif
#include "common_components/common_runtime/src/common/scoped_object_access.h"
#include "common_components/common_runtime/src/heap/collector/finalizer_processor.h"
#include "common_components/common_runtime/src/heap/heap.h"
#include "common_components/common_runtime/src/log_manager.h"
#include "common_components/common_runtime/src/mutator/mutator.h"
#include "common_components/common_runtime/src/mutator/mutator_manager.h"
namespace panda {
StackGrowConfig ArkCommonRuntime::stackGrowConfig = StackGrowConfig::UNDEF;

// Runtime implementation
Runtime* Runtime::runtime = nullptr;
std::atomic<bool> g_initialized = { false };

void ArkCommonRuntime::CreateAndInit(const RuntimeParam& runtimeParam)
{
    OHOS_HITRACE("CreateAndInit");
    if (runtime != nullptr) {
        // Fatal when support ark finish
        LOG_COMMON(WARN) << "Arkcommon runtime has been initialized and don't need init again.";
        return;
    }
    // need to create runtime
    if (g_initialized.load(std::memory_order_acquire)) {
        return;
    }
    auto arkRuntime = new (std::nothrow) ArkCommonRuntime(runtimeParam);
    LOGF_CHECK(arkRuntime != nullptr) << "new ArkCommonRuntime failed";
    Runtime::runtime = arkRuntime;
    arkRuntime->Init();
    g_initialized.store(true, std::memory_order_release);
}

void ArkCommonRuntime::FiniAndDelete()
{
    OHOS_HITRACE("FiniAndDelete");
    if (Runtime::runtime == nullptr) {
        LOG_COMMON(ERROR) << "Fini called but Arkcommon runtime is not running";
        return;
    }
    auto arkRuntime = reinterpret_cast<ArkCommonRuntime*>(Runtime::runtime);
    arkRuntime->Fini();
    delete arkRuntime;
    Runtime::runtime = nullptr;
    g_initialized.store(false, std::memory_order_release);
}

ArkCommonRuntime::ArkCommonRuntime(const RuntimeParam& runtimeParam) : param_(runtimeParam) {}

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

// LoaderManager must be initialized before ConcurrencyModel because of the reservedStackSize field.
// ConcurrencyModel must be initialized before SignalManager because of the stackGuardCheck field.
// HeapManager must be initialized before HeapManager because of inherited properties of signal handlers.
void ArkCommonRuntime::Init()
{
    PagePool::Instance().Init(param_.heapParam.heapSize * KB / ARK_COMMON_PAGE_SIZE);
#ifdef _WIN64
    winModuleManager = NewAndInit<WinModuleManager>();
#endif
    logManager_ = NewAndInit<LogManager>();
    mutatorManager_ = NewAndInit<MutatorManager>();
#if defined(__linux__) || defined(PANDA_TARGET_OHOS) || defined(__APPLE__)
    // signalManager = NewAndInit<SignalManager>()
#endif
    heapManager_ = NewAndInit<HeapManager>(param_.heapParam);

    LOG_COMMON(INFO) << "Arkcommon runtime started.";
    // Record runtime parameter to report. heap growth value needs to plus 1.
    VLOG(REPORT,
        "Runtime parameter:\n\tHeap size: %zu(KB)\n\tRegion size: %zu(KB)\n\tExemption threshold: %.2f\n\t"
        "Heap utilization: %.2f\n\tHeap growth: %.2f\n\tAllocation rate: %.2f(MB/s)\n\tAlloction wait time: %zuns\n\t"
        "GC Threshold: %zu(KB)\n\tGarbage threshold: %.2f\n\tGC interval: %zums\n\tBackup GC interval: %zus\n\t"
        "Log level: %d\n\tThread stack size: %zu(KB)\n\tArkcommon stack size: %zu(KB)\n\t"
        "Processor number: %d", param_.heapParam.heapSize, param_.heapParam.regionSize,
        param_.heapParam.exemptionThreshold, param_.heapParam.heapUtilization, 1 + param_.heapParam.heapGrowth,
        param_.heapParam.allocationRate, param_.heapParam.allocationWaitTime,
        param_.gcParam.gcThreshold / KB, param_.gcParam.garbageThreshold,
        param_.gcParam.gcInterval / MILLI_SECOND_TO_NANO_SECOND,
        param_.gcParam.backupGCInterval / SECOND_TO_NANO_SECOND);
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

void ArkCommonRuntime::Fini()
{
    // since there might be failure during initialization,
    // here we need to check and call fini.
#ifdef _WIN64
    CheckAndFini<WinModuleManager>(winModuleManager);
#endif
    CheckAndFini<HeapManager>(heapManager_);
    CheckAndFini<MutatorManager>(mutatorManager_);

#if defined(__linux__) || defined(PANDA_TARGET_OHOS) || defined(__APPLE__)
    // CheckAndFini<SignalManager>(signalManager)
#endif
    CheckAndFini<LogManager>(logManager_);
    PagePool::Instance().Fini();

    LOG_COMMON(INFO) << "Arkcommon runtime shutdown.";
}

} // namespace panda
