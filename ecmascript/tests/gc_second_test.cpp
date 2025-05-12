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
#include <chrono>
#include <thread>

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/mem/idle_gc_trigger.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/tests/ecma_test_common.h"
#include "ecmascript/napi/include/jsnapi_expo.h"
#include "ecmascript/mem/free_object_list.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/free_object_set.h"
#include "ecmascript/mem/shared_mem_controller.h"
#include "ecmascript/mem/mem_controller_utils.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/incremental_marker.h"

using namespace panda;

using namespace panda::ecmascript;
using TRIGGER_IDLE_GC_TYPE = panda::JSNApi::TRIGGER_IDLE_GC_TYPE;

namespace panda::test {
class GCTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
        heap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    }
};

HWTEST_F_L0(GCTest, NativeGCTestConcurrentMarkDisabled)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    // Disable concurrent mark.
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    size_t oldNativeSize = heap->GetNativeBindingSize();
    EcmaTestCommon::GcCommonCase(thread, heap, false);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::OLD_GC);
    auto newNativeSize = heap->GetNativeBindingSize();
    EXPECT_EQ(newNativeSize - oldNativeSize, 0UL);
}

HWTEST_F_L0(GCTest, NonNewSpaceNativeGCTestConcurrentMarkDisabled)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    // Disable concurrent mark.
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    size_t oldNativeSize = heap->GetNativeBindingSize();
    EcmaTestCommon::GcCommonCase(thread, heap);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::OLD_GC);
    auto newNativeSize = heap->GetNativeBindingSize();
    EXPECT_EQ(newNativeSize - oldNativeSize, 0UL);
}

HWTEST_F_L0(GCTest, ArkToolsForceFullGC)
{
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);
    size_t originalHeapSize = thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    size_t newSize = originalHeapSize;
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        for (int i = 0; i < 10; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> obj = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(1024 * 1024);
        }
        newSize = thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    }
    EXPECT_TRUE(newSize > originalHeapSize);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 0);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    [[maybe_unused]] JSTaggedValue result1 = builtins::BuiltinsArkTools::ForceFullGC(ecmaRuntimeCallInfo);

    ASSERT_TRUE(thread->GetEcmaVM()->GetHeap()->GetCommittedSize() < newSize);
}

HWTEST_F_L0(GCTest, ColdStartForceExpand)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    size_t originalHeapSize = heap->GetCommittedSize();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    heap->NotifyPostFork();
    heap->NotifyFinishColdStartSoon();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 500; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t expandHeapSize = thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    usleep(10000000);
    size_t newSize = EcmaTestCommon::GcCommonCase(thread);
    EXPECT_TRUE(originalHeapSize < expandHeapSize);
    EXPECT_TRUE(expandHeapSize > newSize);
}

HWTEST_F_L0(GCTest, HighSensitiveForceExpand)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    size_t originalHeapSize = heap->GetCommittedSize();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    heap->NotifyHighSensitive(true);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 500; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t expandHeapSize = thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->NotifyHighSensitive(false);
    size_t newSize = EcmaTestCommon::GcCommonCase(thread);
    EXPECT_TRUE(originalHeapSize < expandHeapSize);
    EXPECT_TRUE(expandHeapSize > newSize);
}

HWTEST_F_L0(GCTest, HighSensitiveExceedMaxHeapSize)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->NotifyHighSensitive(true);
    // First allocate about 250M TaggedArray, not reach max heap size
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 16 * 1000; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    // Continue allocate about 250M TaggedArray, now reach max heap size, must trigger gc to avoid OOM
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 10 * 1000; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t commitSize = thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->NotifyHighSensitive(false);
    EXPECT_TRUE(commitSize < thread->GetEcmaVM()->GetEcmaParamConfiguration().GetMaxHeapSize());
}

HWTEST_F_L0(GCTest, ColdStartNoConcurrentMark)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->NotifyPostFork();
    heap->NotifyHighSensitive(true);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 500; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    EXPECT_FALSE(heap->HandleExitHighSensitiveEvent());
    heap->NotifyHighSensitive(false);
    EXPECT_FALSE(heap->HandleExitHighSensitiveEvent());
    heap->FinishStartupEvent();

    heap->NotifyHighSensitive(true);
    heap->NotifyHighSensitive(false);
    EXPECT_TRUE(heap->HandleExitHighSensitiveEvent());
}

HWTEST_F_L0(GCTest, ColdStartGCRestrainInternal)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->NotifyPostFork();
    heap->NotifyFinishColdStartSoon();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    if (!heap->OnStartupEvent()) {
        StartupStatus startupStatus = heap->GetStartupStatus();
        EXPECT_TRUE(startupStatus == StartupStatus::JUST_FINISH_STARTUP);
    }
}

HWTEST_F_L0(GCTest, ColdStartGCRestrainExternal)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->NotifyPostFork();
    heap->NotifyFinishColdStartSoon();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    heap->NotifyFinishColdStart(true);
    EXPECT_FALSE(heap->OnStartupEvent());
    StartupStatus startupStatus = heap->GetStartupStatus();
    EXPECT_TRUE(startupStatus == StartupStatus::JUST_FINISH_STARTUP);
}

HWTEST_F_L0(GCTest, CallbackTask)
{
    auto vm = thread->GetEcmaVM();
    Heap *heap = const_cast<Heap *>(vm->GetHeap());
    auto factory = vm->GetFactory();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        for (int i = 0; i < 10; i++) {
            // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
            void *externalPointer = malloc(10);
            [[maybe_unused]] JSHandle<JSNativePointer> nativePointer = factory->NewJSNativePointer(
                externalPointer, []([[maybe_unused]] void *env, void* pointer, [[maybe_unused]] void* data) {
                if (pointer != nullptr) {
                    free(pointer);
                }
            },
            nullptr, false, 10, Concurrent::YES);
        }
    }
    size_t number = heap->concurrentNativePointerList_.size();
    EXPECT_TRUE(number > 0);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    size_t newNumber = heap->concurrentNativePointerList_.size();
    EXPECT_TRUE(number > newNumber);
}

HWTEST_F_L0(GCTest, RecomputeLimitsTest)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto oldCapacity = heap->GetOldSpace()->GetInitialCapacity();
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_FALSE(heap->IsConcurrentFullMark());
    EXPECT_FALSE(heap->IsFullMarkRequested());
    auto newCapacity = heap->GetOldSpace()->GetInitialCapacity();
    EXPECT_NE(newCapacity, oldCapacity);
    double gcSpeed = heap->GetMemController()->CalculateMarkCompactSpeedPerMS();
    double mutatorSpeed = heap->GetMemController()->GetCurrentOldSpaceAllocationThroughputPerMS();
    size_t oldSpaceSize = heap->GetOldSpace()->GetHeapObjectSize() + heap->GetHugeObjectSpace()->GetHeapObjectSize() +
        heap->GetHugeMachineCodeSpace()->GetHeapObjectSize();
    size_t newSpaceCapacity = heap->GetNewSpace()->GetInitialCapacity();
    double growingFactor =  heap->GetMemController()->CalculateGrowingFactor(gcSpeed, mutatorSpeed);
    size_t maxOldSpaceCapacity = heap->GetOldSpace()->GetMaximumCapacity() - newSpaceCapacity;
    auto newOldSpaceLimit = heap->GetMemController()->CalculateAllocLimit(oldSpaceSize, MIN_OLD_SPACE_LIMIT,
        maxOldSpaceCapacity, newSpaceCapacity, growingFactor);
    EXPECT_EQ(newCapacity, newOldSpaceLimit);
}

HWTEST_F_L0(GCTest, GlobalNativeSizeLargerThanLimitTest)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto ret = heap->GlobalNativeSizeLargerThanLimit();
    EXPECT_FALSE(ret);
    heap->GetNativeAreaAllocator()->IncreaseNativeMemoryUsage(300*1000*1000);
    ret = heap->GlobalNativeSizeLargerThanLimit();
    EXPECT_TRUE(ret);
}
#ifdef NDEBUG
HWTEST_F_L0(GCTest, IdleGCTriggerTest)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = heap->GetIdleGCTrigger();
    auto sHeap = SharedHeap::GetInstance();
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    int baseLocalGCCount = heap->GetEcmaGCStats()->GetGCCount();
    int baseSharedGCCount = sHeap->GetEcmaGCStats()->GetGCCount();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Apply for some memory that cannot be released to simulate the actual situation
    for (int i = 0; i < 5120; i++) {
        factory->NewTaggedArray(1024, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        factory->NewSOldSpaceTaggedArray(1024, JSTaggedValue::Hole());
    }
    for (size_t i = 0; i < 10240; i++)
    {
        factory->NewTaggedArray(512, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        factory->NewSOldSpaceTaggedArray(512, JSTaggedValue::Hole());
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        [[maybe_unused]] JSHandle<TaggedArray> array = factory->NewTaggedArray(1024, JSTaggedValue::Hole(),
                    MemSpaceType::OLD_SPACE);
        [[maybe_unused]] JSHandle<TaggedArray> sArray = factory->NewSOldSpaceTaggedArray(1024,
                    JSTaggedValue::Hole());
        if (i%340 == 0) {
            idleGCTrigger->NotifyVsyncIdleStart();
        }
    }
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::FULL_GC);
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC);
    int afterLocalGCCount = heap->GetEcmaGCStats()->GetGCCount();
    int afterSharedGCCount = sHeap->GetEcmaGCStats()->GetGCCount();
    EXPECT_TRUE(afterLocalGCCount - baseLocalGCCount < 10);
    EXPECT_TRUE(afterSharedGCCount - baseSharedGCCount < 10);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
}
#endif  // #ifndef NDEBUG

HWTEST_F_L0(GCTest, AdjustCapacity)
{
#if defined(PANDA_TARGET_ARM64)
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SemiSpace * space = heap->GetNewSpace();

    EXPECT_EQ(space->GetSurvivalObjectSize(), 0);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 300; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    EXPECT_GT(space->GetSurvivalObjectSize(), 0);

    EXPECT_FALSE(space->AdjustCapacity(0, thread));
    size_t size = space->GetInitialCapacity() * GROW_OBJECT_SURVIVAL_RATE / 2;
    EXPECT_FALSE(space->AdjustCapacity(size, thread));

    space->SetInitialCapacity(space->GetSurvivalObjectSize() / GROW_OBJECT_SURVIVAL_RATE - 1);
    size = space->GetSurvivalObjectSize() / GROW_OBJECT_SURVIVAL_RATE - 1;
    size_t oldMaxCapacity = space->GetMaximumCapacity();
    space->SetMaximumCapacity(space->GetInitialCapacity());
    EXPECT_TRUE(space->AdjustCapacity(size, thread));
    space->SetMaximumCapacity(oldMaxCapacity);
    EXPECT_TRUE(space->AdjustCapacity(size, thread));
#endif
}

HWTEST_F_L0(GCTest, NativeMemAllocInSensitive)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    heap->NotifyHighSensitive(true);
    for (size_t i = 0; i < 20; i++) {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        factory->NewJSArrayBuffer(300 * 1024 * 1024); // 300MB
    }
    EXPECT_TRUE(heap->GetGlobalNativeSize() < 1 * 1024 * 1024* 1024); // 1GB
}

HWTEST_F_L0(GCTest, RecordAllocationForIdleTest001)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    SharedMemController *controller = new SharedMemController(heap);
    controller->RecordAllocationForIdle();
    controller->RecordAllocationForIdle();
}

HWTEST_F_L0(GCTest, RecordAllocationForIdleTest002)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    SharedMemController *controller = new SharedMemController(heap);
    controller->RecordAllocationForIdle();
    size_t before = heap->GetHeapObjectSize();
    heap->ReclaimForAppSpawn();
    size_t after = heap->GetHeapObjectSize();
    ASSERT_NE(before, after);
    controller->RecordAllocationForIdle();
}

HWTEST_F_L0(GCTest, PrintGCStatisticTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    int prop = 1 << 15;
    heap->GetEcmaVM()->GetJSOptions().SetArkProperties(prop);
    ASSERT_EQ(heap->GetEcmaVM()->GetJSOptions().EnableGCTracer(), true);
    GCStats *stats = new GCStats(heap);
    stats->PrintGCStatistic();

    prop = 1 << 14;
    heap->GetEcmaVM()->GetJSOptions().SetArkProperties(prop);
    ASSERT_EQ(heap->GetEcmaVM()->GetJSOptions().EnableGCTracer(), false);
    stats->PrintGCStatistic();
}

HWTEST_F_L0(GCTest, GCReasonToStringTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    GCStats *stats = new GCStats(heap);
    ASSERT_EQ(strcmp(stats->GCReasonToString(GCReason::SWITCH_BACKGROUND), "Switch to background"), 0);
    ASSERT_EQ(strcmp(stats->GCReasonToString(GCReason::EXTERNAL_TRIGGER), "Externally triggered"), 0);
    ASSERT_EQ(strcmp(stats->GCReasonToString(GCReason::WORKER_DESTRUCTION), "Worker Destruction"), 0);
    ASSERT_EQ(strcmp(stats->GCReasonToString(GCReason::TRIGGER_BY_ARKUI), "Trigger by ArkUI"), 0);
    ASSERT_EQ(strcmp(stats->GCReasonToString(GCReason::TRIGGER_BY_ABILITY), "Trigger by AbilityRuntime"), 0);
    ASSERT_EQ(strcmp(stats->GCReasonToString(GCReason::TRIGGER_BY_MEM_TOOLS), "Trigger by Mem tools"), 0);
    ASSERT_EQ(strcmp(stats->GCReasonToString(GCReason::TRIGGER_BY_TASKPOOL), "Trigger by taskPool"), 0);
}

HWTEST_F_L0(GCTest, PrintGCMemoryStatisticTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMarkType(MarkType::MARK_YOUNG);
    GCStats *stats = new GCStats(heap);
    stats->RecordStatisticBeforeGC(TriggerGCType::YOUNG_GC, GCReason::TRIGGER_BY_ARKUI);
    stats->PrintGCMemoryStatistic();
}

HWTEST_F_L0(GCTest, CheckIfNeedPrintTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMarkType(MarkType::MARK_YOUNG);
    GCStats *stats = new GCStats(heap);
    stats->SetRecordData(RecordData::YOUNG_COUNT, 1);
    stats->PrintStatisticResult();
}

HWTEST_F_L0(GCTest, PrintGCSummaryStatisticTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMarkType(MarkType::MARK_YOUNG);
    GCStats *stats = new GCStats(heap);
    stats->PrintStatisticResult();
}

HWTEST_F_L0(GCTest, CalculateGrowingFactorTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMemGrowingType(MemGrowingType::CONSERVATIVE);
    auto controller = new MemController(heap);
    controller->CalculateGrowingFactor(1, 1);
}

HWTEST_F_L0(GCTest, CalculateGrowingFactorTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMemGrowingType(MemGrowingType::PRESSURE);
    auto controller = new MemController(heap);
    controller->CalculateGrowingFactor(1, 1);
}

HWTEST_F_L0(GCTest, CalculateGrowingFactorTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMemGrowingType(MemGrowingType::CONSERVATIVE);
    auto controller = new MemController(heap);
    controller->CalculateGrowingFactor(1, 0);
}

HWTEST_F_L0(GCTest, StopCalculationAfterGCTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto controller = new MemController(heap);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    controller->StartCalculationBeforeGC();
    controller->StopCalculationAfterGC(TriggerGCType::YOUNG_GC);
}

HWTEST_F_L0(GCTest, RecordAllocationForIdleTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto controller = new MemController(heap);
    controller->RecordAllocationForIdle();
}

HWTEST_F_L0(GCTest, TryTriggerIdleCollectionTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::YOUNG_GC);
    heap->TryTriggerIdleCollection();
}

HWTEST_F_L0(GCTest, WaitAllTasksFinishedTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARKING);
    heap->WaitAllTasksFinished();
}

HWTEST_F_L0(GCTest, WaitAllTasksFinishedTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARKING);
    heap->GetConcurrentMarker()->Mark();
    heap->WaitAllTasksFinished();
}

HWTEST_F_L0(GCTest, ChangeGCParamsTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetOldSpace()->IncreaseLiveObjectSize(2098000);
    heap->ChangeGCParams(true);
}

HWTEST_F_L0(GCTest, ChangeGCParamsTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetOldSpace()->IncreaseLiveObjectSize(2098000);
    heap->GetOldSpace()->IncreaseCommitted(31457300);
    heap->ChangeGCParams(true);
}

HWTEST_F_L0(GCTest, ChangeGCParamsTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetOldSpace()->IncreaseLiveObjectSize(2098000);
    heap->ChangeGCParams(true);
}

HWTEST_F_L0(GCTest, ChangeGCParamsTest004)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMemGrowingType(MemGrowingType::HIGH_THROUGHPUT);
    heap->ChangeGCParams(true);
}

HWTEST_F_L0(GCTest, IncrementMarkerTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetIncrementalMarker()->TriggerIncrementalMark(100);
    heap->GetIncrementalMarker()->TriggerIncrementalMark(100);
}

HWTEST_F_L0(GCTest, IncrementMarkerTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetIncrementalMarker()->TriggerIncrementalMark(0);
}

HWTEST_F_L0(GCTest, IncrementMarkerTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetIncrementalMarker()->SetMarkingFinished(true);
    heap->GetIncrementalMarker()->TriggerIncrementalMark(100);
}

HWTEST_F_L0(GCTest, IncrementMarkerTest004)
{
    thread->GetEcmaVM()->GetJSOptions().SetArkProperties(0);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetIncrementalMarker()->SetMarkingFinished(true);
    heap->GetIncrementalMarker()->TriggerIncrementalMark(100);
}

} // namespace panda::test
