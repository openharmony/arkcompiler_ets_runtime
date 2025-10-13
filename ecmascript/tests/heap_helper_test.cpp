/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;

namespace panda::test {

class HeapTest :  public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
    }
};

class HeapTestHelper {
public:
    explicit HeapTestHelper(Heap *heap) : heap_(heap) {}

    ~HeapTestHelper() = default;

    void SetIdleGCState(bool flag)
    {
        ASSERT_NE(heap_, nullptr);
        heap_->enableIdleGC_ = flag;
    }

    Heap *GetHeap()
    {
        return heap_;
    }

private:
    Heap *heap_{nullptr};
};

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking1)
{
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = true;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), false);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking2)
{
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->SetIdleTask(IdleTaskType::FINISH_MARKING);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), false);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), false);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking3)
{
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->SetIdleTask(IdleTaskType::YOUNG_GC);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);

    uint32_t length = 1000;
    heap->GetOldSpace()->SetInitialCapacity(static_cast<size_t>(length));
    heap->SetFullMarkRequestedState(true);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), true);
    EXPECT_EQ(heap->GetMarkType(), MarkType::MARK_FULL);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking4)
{
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->SetIdleTask(IdleTaskType::YOUNG_GC);
    heap->SetFullMarkRequestedState(false);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);
    EXPECT_EQ(heap->IsFullMarkRequested(), false);

    heap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    EXPECT_EQ(heap->InSensitiveStatus(), true);
    heap->SetRecordHeapObjectSizeBeforeSensitive(1000000);
    EXPECT_EQ(heap->ObjectExceedHighSensitiveThresholdForCM(), false);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), false);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking5)
{
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->SetIdleTask(IdleTaskType::YOUNG_GC);
    heap->SetFullMarkRequestedState(false);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);
    EXPECT_EQ(heap->IsFullMarkRequested(), false);

    heap->NotifyPostFork();
    EXPECT_EQ(heap->FinishStartupEvent(), true);
    EXPECT_EQ(heap->IsJustFinishStartup(), true);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), false);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking6)
{
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->SetIdleTask(IdleTaskType::YOUNG_GC);
    heap->SetFullMarkRequestedState(false);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);
    EXPECT_EQ(heap->IsFullMarkRequested(), false);

    heap->GetOldSpace()->SetInitialCapacity(1000);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), true);
    EXPECT_EQ(heap->GetMarkType(), MarkType::MARK_FULL);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking7)
{
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->SetIdleTask(IdleTaskType::YOUNG_GC);
    heap->SetFullMarkRequestedState(false);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);
    EXPECT_EQ(heap->IsFullMarkRequested(), false);

    heap->GetNewSpace()->IncreaseCommitted(10000000);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), true);
    EXPECT_EQ(heap->GetMarkType(), MarkType::MARK_YOUNG);
    EXPECT_EQ(heap->GetEcmaGCStats()->GetMarkReason(), MarkReason::ALLOCATION_LIMIT);
    g_isEnableCMCGC = temp;
}
}