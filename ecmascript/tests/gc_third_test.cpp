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

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class GCTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        options.SetEnableEdenGC(true);
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
        size_t originSize = heap->GetCommittedSize();
        size_t newSize = 0;
        size_t finalSize = 0;
        bool res = getSizeAfterCreateAndCallHintGC(newSize, finalSize);
        EXPECT_TRUE(res);
        EXPECT_TRUE(newSize > originSize);
        EXPECT_TRUE(finalSize < newSize);
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
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 500; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t newYoungSize = heap->GetNewSpace()->GetCommittedSize();
    size_t originalOverShootSize = heap->GetNewSpace()->GetOvershootSize();
    EXPECT_TRUE(heap->GetNewSpace()->CommittedSizeIsLarge());
    EXPECT_TRUE(originalYoungSize < newYoungSize);

    heap->NotifyHighSensitive(false);
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    newYoungSize = heap->GetNewSpace()->GetCommittedSize();
    size_t newOverShootSize = heap->GetNewSpace()->GetOvershootSize();

    EXPECT_TRUE(originalYoungSize < newYoungSize);
    EXPECT_TRUE(originalOverShootSize < newOverShootSize);
    EXPECT_TRUE(0 < newOverShootSize);

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

HWTEST_F_L0(GCTest, CalculateIdleDurationTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMarkType(MarkType::MARK_EDEN);
    ASSERT_EQ(heap->GetMarkType(), MarkType::MARK_EDEN);
    heap->CalculateIdleDuration();
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

HWTEST_F_L0(GCTest, TriggerIdleCollectionTest005)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetIdleTask(IdleTaskType::FINISH_MARKING);
    heap->SetMarkType(MarkType::MARK_EDEN);
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
    heap->Reclaim(TriggerGCType::EDEN_GC);
}

HWTEST_F_L0(GCTest, CollectGarbageTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->CollectGarbage(TriggerGCType::EDEN_GC, GCReason::IDLE);
}

HWTEST_F_L0(GCTest, CollectGarbageTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMarkType(MarkType::MARK_FULL);
    ASSERT_EQ(heap->GetMarkType(), MarkType::MARK_FULL);
    heap->CollectGarbage(TriggerGCType::EDEN_GC, GCReason::IDLE);
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

} // namespace panda::test
