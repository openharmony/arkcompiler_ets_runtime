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

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/containers/containers_bitvector.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_api/js_api_bitvector.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/mem/gc_key_stats.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/allocation_inspector.h"
#include "ecmascript/dfx/hprof/heap_sampling.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

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

HWTEST_F_L0(GCTest, ArkToolsHintGC)
{
    Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::CONFIG_DISABLE);
    auto getSizeAfterCreateAndCallHintGC = [this, heap] (size_t &newSize, size_t &finalSize) -> bool {
        {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
            for (int i = 0; i < 2048; i++) {
                [[maybe_unused]] JSHandle<TaggedArray> obj = thread->GetEcmaVM()->GetFactory()->
                    NewTaggedArray(10 * 1024, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
            }
            newSize = heap->GetCommittedSize();
        }
        std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(2))};
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals,
                                                                         6);
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = builtins::BuiltinsArkTools::HintGC(ecmaRuntimeCallInfo);
        finalSize = heap->GetCommittedSize();
        TestHelper::TearDownFrame(thread, prev);

        return result.ToBoolean();
    };
    {
        // Test HintGC() when sensitive.
        heap->CollectGarbage(TriggerGCType::FULL_GC);
        heap->NotifyHighSensitive(true);
        size_t originSize = heap->GetCommittedSize();
        size_t newSize = 0;
        size_t finalSize = 0;
        bool res = getSizeAfterCreateAndCallHintGC(newSize, finalSize);
        EXPECT_FALSE(res);
        EXPECT_TRUE(newSize > originSize);
        EXPECT_TRUE(finalSize == newSize);
        heap->NotifyHighSensitive(false);
    }
    {
#ifdef NDEBUG
        size_t newSize = 0;
        size_t finalSize = 0;
        bool res = getSizeAfterCreateAndCallHintGC(newSize, finalSize);
        EXPECT_TRUE(res);
#endif
    }
}

HWTEST_F_L0(GCTest, LargeOverShootSizeTest)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    size_t originalYoungSize = heap->GetNewSpace()->GetCommittedSize();

    EXPECT_FALSE(heap->GetNewSpace()->CommittedSizeIsLarge());
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    heap->NotifyHighSensitive(true);
    size_t originalCapacity = heap->GetNewSpace()->GetInitialCapacity();
    size_t originalOverShootSize = heap->GetNewSpace()->GetOvershootSize();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 300; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
        size_t newYoungSize = heap->GetNewSpace()->GetCommittedSize();
        EXPECT_TRUE(originalYoungSize < newYoungSize);

        heap->NotifyHighSensitive(false);
        heap->CollectGarbage(TriggerGCType::YOUNG_GC);
        newYoungSize = heap->GetNewSpace()->GetCommittedSize();
        size_t newOverShootSize = heap->GetNewSpace()->GetOvershootSize();
        size_t newCapacity = heap->GetNewSpace()->GetInitialCapacity();
        EXPECT_TRUE(originalYoungSize < newYoungSize);
        EXPECT_TRUE(originalOverShootSize < newOverShootSize);
        EXPECT_TRUE(0 < newOverShootSize);
        EXPECT_TRUE(originalCapacity < newCapacity);
        EXPECT_TRUE(heap->GetNewSpace()->GetMaximumCapacity() == newCapacity);
    }
    originalOverShootSize = heap->GetNewSpace()->GetOvershootSize();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 2049; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t newSize = heap->GetNewSpace()->GetCommittedSize();
    EXPECT_TRUE(originalYoungSize <= newSize);
}

HWTEST_F_L0(GCTest, CheckAndTriggerSharedGCTest001)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    ASSERT_EQ(heap->CheckAndTriggerSharedGC(thread), false);
}

HWTEST_F_L0(GCTest, CheckHugeAndTriggerSharedGCTest001)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    ASSERT_EQ(heap->CheckHugeAndTriggerSharedGC(thread, 1374210560), true);
}

HWTEST_F_L0(GCTest, CheckHugeAndTriggerSharedGCTest002)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    ASSERT_EQ(heap->CheckHugeAndTriggerSharedGC(thread, 1), false);
}

HWTEST_F_L0(GCTest, ObjectExceedMaxHeapSizeTest001)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    ASSERT_EQ(heap->ObjectExceedMaxHeapSize(), false);
}

HWTEST_F_L0(GCTest, CheckAndTriggerHintGCTest001)
{
#ifdef NDEBUG
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::LOW, GCReason::HINT_GC), false);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 4048; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                1024, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        }
    }
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::LOW, GCReason::HINT_GC), true);
#endif
}

HWTEST_F_L0(GCTest, CheckAndTriggerHintGCTest002)
{
#ifdef NDEBUG
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::MIDDLE, GCReason::HINT_GC), false);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 4048; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                1024, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        }
    }
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::MIDDLE, GCReason::HINT_GC), true);
#endif
}

HWTEST_F_L0(GCTest, CheckAndTriggerHintGCTest003)
{
#ifdef NDEBUG
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::HIGH, GCReason::HINT_GC), false);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 1049; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                1024, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        }
    }
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::HIGH, GCReason::HINT_GC), true);
#endif
}

HWTEST_F_L0(GCTest, CheckAndTriggerHintGCTest004)
{
#ifdef NDEBUG
    auto sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::LOW, GCReason::HINT_GC), false);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 4048; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->
                NewSOldSpaceTaggedArray(1024, JSTaggedValue::Undefined());
        }
    }
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::LOW, GCReason::HINT_GC), true);
#endif
}

HWTEST_F_L0(GCTest, CheckAndTriggerHintGCTest005)
{
#ifdef NDEBUG
    auto sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::MIDDLE, GCReason::HINT_GC), false);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 4048; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->
                NewSOldSpaceTaggedArray(1024, JSTaggedValue::Undefined());
        }
    }
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::MIDDLE, GCReason::HINT_GC), true);
#endif
}

HWTEST_F_L0(GCTest, CheckAndTriggerHintGCTest006)
{
#ifdef NDEBUG
    auto sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::HIGH, GCReason::HINT_GC), false);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 2049; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->
                NewSOldSpaceTaggedArray(1024, JSTaggedValue::Undefined());
        }
    }
    ASSERT_EQ(heap->CheckAndTriggerHintGC(MemoryReduceDegree::HIGH, GCReason::HINT_GC), true);
#endif
}

HWTEST_F_L0(GCTest, CalculateIdleDurationTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMarkType(MarkType::MARK_YOUNG);
    ASSERT_EQ(heap->GetMarkType(), MarkType::MARK_YOUNG);
    heap->CalculateIdleDuration();
}

HWTEST_F_L0(GCTest, CalculateIdleDurationTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMarkType(MarkType::MARK_FULL);
    ASSERT_EQ(heap->GetMarkType(), MarkType::MARK_FULL);
    heap->CalculateIdleDuration();
}

HWTEST_F_L0(GCTest, TryTriggerFullMarkBySharedLimitTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_EQ(heap->TryTriggerFullMarkBySharedLimit(), false);
}

HWTEST_F_L0(GCTest, TryTriggerFullMarkBySharedLimitTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::FINISH_MARKING);
    ASSERT_EQ(heap->TryTriggerFullMarkBySharedLimit(), false);
}

HWTEST_F_L0(GCTest, TryTriggerFullMarkBySharedLimitTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    for (size_t i = 0; i < 4; i++) {
        ConcurrentMarker::TryIncreaseTaskCounts();
    }
    ASSERT_EQ(heap->TryTriggerFullMarkBySharedLimit(), true);
}

HWTEST_F_L0(GCTest, CheckAndTriggerTaskFinishedGCTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->CheckAndTriggerTaskFinishedGC();
}

HWTEST_F_L0(GCTest, TryTriggerFullMarkBySharedSizeTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->TryTriggerFullMarkBySharedSize(81579214);
}

HWTEST_F_L0(GCTest, DecreaseNativeBindingSizeTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->DecreaseNativeBindingSize(0);
}

HWTEST_F_L0(GCTest, TriggerConcurrentMarkingTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMarkType(MarkType::MARK_FULL);
    heap->SetIdleTask(IdleTaskType::YOUNG_GC);
    heap->TriggerConcurrentMarking();
}

HWTEST_F_L0(GCTest, TriggerIdleCollectionTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::NO_TASK);
    heap->TriggerIdleCollection(1000);
}

HWTEST_F_L0(GCTest, TriggerIdleCollectionTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::INCREMENTAL_MARK);
    heap->TriggerIdleCollection(1000);
}

HWTEST_F_L0(GCTest, TriggerIdleCollectionTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::FINISH_MARKING);
    heap->TriggerIdleCollection(-1);
}

HWTEST_F_L0(GCTest, TriggerIdleCollectionTest004)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::FINISH_MARKING);
    heap->SetMarkType(MarkType::MARK_FULL);
    heap->TriggerIdleCollection(1000);
}

HWTEST_F_L0(GCTest, TriggerIdleCollectionTest006)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::YOUNG_GC);
    heap->TriggerIdleCollection(1000);
}

HWTEST_F_L0(GCTest, NotifyFinishColdStartTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::YOUNG_GC);
    heap->NotifyFinishColdStart(true);
}

HWTEST_F_L0(GCTest, NotifyFinishColdStartSoonTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->NotifyFinishColdStartSoon();
}

HWTEST_F_L0(GCTest, NeedStopCollectionTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetOnSerializeEvent(true);
    ASSERT_EQ(heap->NeedStopCollection(), true);
}

HWTEST_F_L0(GCTest, ReclaimTest001)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    heap->DisableParallelGC(thread);
    heap->Reclaim(TriggerGCType::YOUNG_GC);
}

HWTEST_F_L0(GCTest, CollectGarbageTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetJSThread()->SetMarkStatus(MarkStatus::READY_TO_MARK);
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);
}

HWTEST_F_L0(GCTest, NeedStopCollectionTest002)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    heap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    ASSERT_EQ(heap->NeedStopCollection(), true);
}

HWTEST_F_L0(GCTest, NeedStopCollectionTest003)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    heap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    heap->GetOldSpace()->SetInitialCapacity(1000);
    ASSERT_EQ(heap->NeedStopCollection(), false);
}

HWTEST_F_L0(GCTest, TriggerIdleCollectionTest007)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::INCREMENTAL_MARK);
    ASSERT_EQ(heap->GetIncrementalMarker()->GetIncrementalGCStates(), IncrementalGCStates::ROOT_SCAN);
    heap->GetIncrementalMarker()->TriggerIncrementalMark(1000);
    heap->TriggerIdleCollection(1000);
}

HWTEST_F_L0(GCTest, CheckAndTriggerSharedGCTest002)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    heap->GetOldSpace()->SetInitialCapacity(100);
    ASSERT_EQ(heap->CheckAndTriggerSharedGC(thread), true);
}

HWTEST_F_L0(GCTest, CheckAndTriggerSharedGCTest003)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    heap->GetOldSpace()->SetInitialCapacity(100);
    thread->SetSharedMarkStatus(SharedMarkStatus::CONCURRENT_MARKING_OR_FINISHED);
    ASSERT_EQ(heap->CheckAndTriggerSharedGC(thread), true);
}

HWTEST_F_L0(GCTest, CheckAndTriggerSharedGCTest004)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    thread->SetSharedMarkStatus(SharedMarkStatus::CONCURRENT_MARKING_OR_FINISHED);
    ASSERT_EQ(heap->CheckAndTriggerSharedGC(thread), false);
}

HWTEST_F_L0(GCTest, CheckHugeAndTriggerSharedGCTest003)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    heap->GetOldSpace()->SetInitialCapacity(100);
    ASSERT_EQ(heap->CheckHugeAndTriggerSharedGC(thread, 1), false);
}

HWTEST_F_L0(GCTest, CheckHugeAndTriggerSharedGCTest004)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    heap->GetOldSpace()->SetInitialCapacity(100);
    thread->SetSharedMarkStatus(SharedMarkStatus::CONCURRENT_MARKING_OR_FINISHED);
    ASSERT_EQ(heap->CheckHugeAndTriggerSharedGC(thread, 1), false);
}

HWTEST_F_L0(GCTest, CheckHugeAndTriggerSharedGCTest005)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    thread->SetSharedMarkStatus(SharedMarkStatus::CONCURRENT_MARKING_OR_FINISHED);
    ASSERT_EQ(heap->CheckHugeAndTriggerSharedGC(thread, 1), false);
}

HWTEST_F_L0(GCTest, CheckOngoingConcurrentMarkingTest001)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    ASSERT_EQ(heap->CheckOngoingConcurrentMarking(), false);
}

HWTEST_F_L0(GCTest, CheckOngoingConcurrentMarkingTest002)
{
    SharedHeap *heap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    ASSERT_EQ(heap->CheckOngoingConcurrentMarking(), false);
}

HWTEST_F_L0(GCTest, SelectGCTypeTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetOldSpace()->SetInitialCapacity(100);
    ASSERT_EQ(heap->SelectGCType(), OLD_GC);
}

HWTEST_F_L0(GCTest, SelectGCTypeTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetOldSpace()->SetMaximumCapacity(1000);
    heap->GetOldSpace()->SetOvershootSize(1000);
    heap->GetNewSpace()->IncreaseCommitted(100000);
    ASSERT_EQ(heap->SelectGCType(), OLD_GC);
}

HWTEST_F_L0(GCTest, CollectGarbageTest008)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetIncrementalMarker()->TriggerIncrementalMark(1000);
    heap->CollectGarbage(TriggerGCType::YOUNG_GC, GCReason::TRIGGER_BY_TASKPOOL);
}

HWTEST_F_L0(GCTest, CollectGarbageTest009)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::REQUEST_DISABLE);
    ASSERT_TRUE(heap->GetConcurrentMarker()->IsRequestDisabled());
    heap->CollectGarbage(TriggerGCType::YOUNG_GC, GCReason::TRIGGER_BY_TASKPOOL);
}

HWTEST_F_L0(GCTest, TryTriggerIdleCollectionTest007)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::NO_TASK);
    heap->TryTriggerIdleCollection();
}

HWTEST_F_L0(GCTest, TryTriggerFullMarkBySharedLimitTest004)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->TryTriggerFullMarkBySharedLimit();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    heap->TryTriggerFullMarkBySharedLimit();
}

HWTEST_F_L0(GCTest, TriggerConcurrentMarkingTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->TriggerConcurrentMarking();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    heap->TriggerConcurrentMarking();
}

HWTEST_F_L0(GCTest, TriggerIdleCollectionTest008)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->TriggerIdleCollection(1000);
    heap->ClearIdleTask();
    heap->TriggerIdleCollection(1000);
}

HWTEST_F_L0(GCTest, TriggerIdleCollectionTest009)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::YOUNG_GC);
    heap->TriggerIdleCollection(5);
}

HWTEST_F_L0(GCTest, NotifyFinishColdStartTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->NotifyPostFork();
    heap->NotifyFinishColdStart(true);
}

HWTEST_F_L0(GCTest, NeedStopCollectionTest004)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetOnSerializeEvent(false);
    heap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    ASSERT_EQ(heap->NeedStopCollection(), true);
}

HWTEST_F_L0(GCTest, TryToGetSuitableSweptRegionTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SparseSpace *space = heap->GetSpaceWithType(MemSpaceType::OLD_SPACE);
    space->FinishFillSweptRegion();
    ASSERT_EQ(space->TryToGetSuitableSweptRegion(100), nullptr);
}

HWTEST_F_L0(GCTest, CalculateGrowingFactorTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMemGrowingType(MemGrowingType::CONSERVATIVE);
    MemController *memController = new MemController(heap);
    memController->CalculateGrowingFactor(0, 0);
}

HWTEST_F_L0(GCTest, CalculateGrowingFactorTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMemGrowingType(MemGrowingType::PRESSURE);
    MemController *memController = new MemController(heap);
    memController->CalculateGrowingFactor(0, 0);
}

HWTEST_F_L0(GCTest, StartCalculationBeforeGCTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    MemController *memController = new MemController(heap);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    memController->StartCalculationBeforeGC();
    memController->StopCalculationAfterGC(TriggerGCType::YOUNG_GC);
}

HWTEST_F_L0(GCTest, StartCalculationBeforeGCTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    MemController *memController = new MemController(heap);
    memController->StopCalculationAfterGC(TriggerGCType::YOUNG_GC);
}

HWTEST_F_L0(GCTest, DryTrunkExpandTest001)
{
    auto trunk = thread->GetEcmaVM()->GetChunk();
    DynChunk *dynChunk = new DynChunk(trunk);
    ASSERT_TRUE(dynChunk->GetAllocatedSize() < 1000);
    dynChunk->SetError();
    ASSERT_EQ(dynChunk->Expand(1000), -1);
}

HWTEST_F_L0(GCTest, DryTrunkInsertTest001)
{
    auto trunk = thread->GetEcmaVM()->GetChunk();
    DynChunk *dynChunk = new DynChunk(trunk);
    ASSERT_EQ(dynChunk->Insert(5, 5), -1);
}

HWTEST_F_L0(GCTest, DryTrunkInsertTest002)
{
    auto trunk = thread->GetEcmaVM()->GetChunk();
    DynChunk *dynChunk = new DynChunk(trunk);
    dynChunk->SetError();
    ASSERT_EQ(dynChunk->Insert(0, 5), -1);
}

HWTEST_F_L0(GCTest, AdvanceAllocationInspectorTest001)
{
    auto counter = new AllocationCounter();
    counter->AdvanceAllocationInspector(100);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto profiler = new HeapSampling(thread->GetEcmaVM(), heap, 10, 3);
    auto inspector = new AllocationInspector(heap, 10, profiler);
    counter->AddAllocationInspector(inspector);
    counter->AdvanceAllocationInspector(0);
}

HWTEST_F_L0(GCTest, InvokeAllocationInspectorTest001)
{
    auto counter = new AllocationCounter();
    counter->InvokeAllocationInspector(10000, 100, 100);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto profiler = new HeapSampling(thread->GetEcmaVM(), heap, 10, 3);
    auto inspector = new AllocationInspector(heap, 10, profiler);
    counter->AddAllocationInspector(inspector);
    counter->InvokeAllocationInspector(10000, 100, 100);
}

HWTEST_F_L0(GCTest, OldSpaceValidCheck)
{
    static constexpr size_t kLength = 10 * 1024;
    static constexpr size_t kCount = 2;
    static constexpr size_t kLimit = 380 * 1024 * 1024;
    instance->GetJSOptions().SetEnableForceGC(false);
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();
    auto array = factory->NewTaggedArray(kLength, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
    heap->ShouldThrowOOMError(true);
    heap->GetOldSpace()->IncreaseLiveObjectSize(kLimit);
    for (size_t i = 0; i < kCount; i++) {
        array = factory->NewTaggedArray(kLength, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        Region *objectRegion = Region::ObjectAddressToRange(*array);
        bool inHeap = false;
        heap->GetOldSpace()->EnumerateRegions([objectRegion, &inHeap](Region *each) {
            if (objectRegion == each) {
                inHeap = true;
            }
        });
        EXPECT_TRUE(inHeap);
    }
}

HWTEST_F_L0(GCTest, DisableSharedConcurrentSweep)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    SharedHeap *sHeap = SharedHeap::GetInstance();
    sHeap->GetSweeper()->ConfigConcurrentSweep(false);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        [[maybe_unused]] JSHandle<EcmaString> key1(factory->NewFromASCII("error1"));
        [[maybe_unused]] JSHandle<EcmaString> key2(factory->NewFromASCII("error2"));
        [[maybe_unused]] JSHandle<EcmaString> msg(factory->NewFromASCII("this is error"));
        [[maybe_unused]] JSHandle<EcmaString> key3(factory->NewFromASCII("error3"));
        [[maybe_unused]] JSHandle<EcmaString> key4(factory->NewFromASCII("error4"));
        [[maybe_unused]] JSHandle<EcmaString> msg2(factory->NewFromASCII("this is error2"));
        auto* newBitSetVector = new std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>>();
        int32_t capacity = 256;
        std::bitset<JSAPIBitVector::BIT_SET_LENGTH> initBitSet;
        newBitSetVector->resize(capacity, initBitSet);
        auto deleter = []([[maybe_unused]] void *env, void *pointer, [[maybe_unused]] void *data) {
            if (pointer == nullptr) {
                return;
            }
            delete reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(pointer);
        };
        [[maybe_unused]] JSHandle<JSNativePointer> pointer = factory->NewSJSNativePointer(newBitSetVector,
                                                                                          deleter,
                                                                                          newBitSetVector);
        const char *filename1 = "__JSPandaFileManagerTest1.pa";
        const char *filename2 = "__JSPandaFileManagerTest2.pa";
        const char *data = R"(
            .function void foo() {}
        )";
        JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
        Parser parser;
        auto res = parser.Parse(data);
        std::unique_ptr<const File> pfPtr1 = pandasm::AsmEmitter::Emit(res.Value());
        std::unique_ptr<const File> pfPtr2 = pandasm::AsmEmitter::Emit(res.Value());
        std::shared_ptr<JSPandaFile> pf1 = pfManager->NewJSPandaFile(pfPtr1.release(), CString(filename1));
        std::shared_ptr<JSPandaFile> pf2 = pfManager->NewJSPandaFile(pfPtr2.release(), CString(filename2));
        pfManager->AddJSPandaFile(pf1);
        pfManager->AddJSPandaFile(pf2);

        JSHandle<ConstantPool> constpool1 = instance->GetFactory()->NewSConstantPool(1);
        JSHandle<ConstantPool> constpool2 = instance->GetFactory()->NewSConstantPool(2);
        constpool1 = Runtime::GetInstance()->AddOrUpdateConstpool(pf1.get(), constpool1, 0);
        constpool2 = Runtime::GetInstance()->AddOrUpdateConstpool(pf2.get(), constpool2, 0);
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    sHeap->WaitGCFinished(thread);
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    sHeap->WaitGCFinished(thread);
    sHeap->GetSweeper()->ConfigConcurrentSweep(true);
};
} // namespace panda::test
