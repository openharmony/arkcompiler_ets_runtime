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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/idle_gc_trigger.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;
using TRIGGER_IDLE_GC_TYPE = panda::JSNApi::TRIGGER_IDLE_GC_TYPE;
using TriggerGCData = std::pair<void*, uint8_t>;
using TriggerGCTaskCallback = std::function<void(TriggerGCData& data)>;

namespace panda::test {
class IdleGCTriggerTest :  public BaseTestWithScope<false> {
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

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStartTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->NotifyLooperIdleStart(1, 1);
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStartTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->Mark();
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_MARK);
    trigger->NotifyLooperIdleStart(1, 1);
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleEndTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->NotifyLooperIdleEnd(1);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerHandleMarkFinishedTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();

    heap->GetConcurrentMarker()->Mark();
    heap->GetConcurrentMarker()->ProcessConcurrentMarkTask(0);
    
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerHandleMarkFinished();
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerLocalConcurrentMarkTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->SetIdleTask(IdleTaskType::NO_TASK);

    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerLocalConcurrentMark();
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerLocalConcurrentMarkTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->SetIdleTask(IdleTaskType::NO_TASK);
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerLocalConcurrentMark();
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerLocalConcurrentMarkTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->SetIdleTask(IdleTaskType::NO_TASK);
    thread->SetMarkStatus(MarkStatus::MARK_FINISHED);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerLocalConcurrentMark();
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleSharedOldGCTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();

    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    ASSERT_EQ(trigger->TryTriggerIdleSharedOldGC(), false);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleSharedOldGCTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->GetOldSpace()->SetInitialCapacity(10000);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    sheap->NotifyHeapAliveSizeAfterGC(1);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    ASSERT_EQ(trigger->TryTriggerIdleSharedOldGC(), true);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleSharedOldGCTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->GetOldSpace()->SetInitialCapacity(10000);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    sheap->NotifyHeapAliveSizeAfterGC(1);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    ASSERT_EQ(trigger->TryTriggerIdleSharedOldGC(), true);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleSharedOldGCTest004)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->SetOnSerializeEvent(true);
    
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    ASSERT_EQ(trigger->TryTriggerIdleSharedOldGC(), false);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGCTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->SetIdleTask(IdleTaskType::NO_TASK);
    heap->GetOldSpace()->SetInitialCapacity(10000);
    heap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    heap->NotifyHeapAliveSizeAfterGC(1);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    ASSERT_EQ(trigger->TryTriggerIdleLocalOldGC(), true);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGCTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_MARK);
    ASSERT_EQ(trigger->TryTriggerIdleLocalOldGC(), true);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGCTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->Mark();
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK);
    ASSERT_EQ(trigger->TryTriggerIdleLocalOldGC(), true);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGCTest004)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->SetIdleTask(IdleTaskType::NO_TASK);
    heap->GetOldSpace()->SetInitialCapacity(10000);
    heap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    heap->NotifyHeapAliveSizeAfterGC(1);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    TriggerGCTaskCallback callback = [](TriggerGCData& data) {
        data.second = 1;
    };
    trigger->SetTriggerGCTaskCallback(callback);
    ASSERT_EQ(trigger->TryTriggerIdleLocalOldGC(), true);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGCTest005)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->Mark();
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_MARK);
    ASSERT_EQ(trigger->TryTriggerIdleLocalOldGC(), true);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGCTest006)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->SetIdleTask(IdleTaskType::FINISH_MARKING);
    heap->GetOldSpace()->SetInitialCapacity(10000);
    heap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    heap->NotifyHeapAliveSizeAfterGC(1);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    TriggerGCTaskCallback callback = [](TriggerGCData& data) {
        data.second = 1;
    };
    trigger->SetTriggerGCTaskCallback(callback);
    ASSERT_EQ(trigger->TryTriggerIdleLocalOldGC(), true);
}

HWTEST_F_L0(IdleGCTriggerTest, ReachIdleLocalOldGCThresholdsTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetNativeAreaAllocator()->IncreaseNativeMemoryUsage(83886100);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    ASSERT_EQ(trigger->ReachIdleLocalOldGCThresholds(), true);
}

HWTEST_F_L0(IdleGCTriggerTest, ReachIdleLocalOldGCThresholdsTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetNativeAreaAllocator()->IncreaseNativeMemoryUsage(1);
    heap->GetOldSpace()->IncreaseCommitted(83886100);
    heap->GetOldSpace()->SetMaximumCapacity(100000);
    heap->GetOldSpace()->SetOvershootSize(100000);
    heap->GetOldSpace()->SetOvershootSize(100000);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    ASSERT_EQ(trigger->ReachIdleLocalOldGCThresholds(), true);
}

HWTEST_F_L0(IdleGCTriggerTest, TryPostHandleMarkFinishedTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->NotifyLooperIdleStart(1, 1);
    trigger->TryPostHandleMarkFinished();
}

HWTEST_F_L0(IdleGCTriggerTest, TryPostHandleMarkFinishedTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryPostHandleMarkFinished();
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->NotifyHeapAliveSizeAfterGC(1);
    sheap->GetOldSpace()->SetInitialCapacity(10000);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->NotifyHeapAliveSizeAfterGC(1);
    heap->GetOldSpace()->SetInitialCapacity(10000);
    heap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_MARK);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest004)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest011)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->SetOnSerializeEvent(true);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest012)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->NotifyHeapAliveSizeAfterGC(0);
    sheap->GetOldSpace()->SetInitialCapacity(10000);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::FULL_GC);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest013)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->NotifyHeapAliveSizeAfterGC(0);
    sheap->GetOldSpace()->SetInitialCapacity(10000);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC);
}

HWTEST_F_L0(IdleGCTriggerTest, ShouldCheckIdleOldGCTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->NotifyHeapAliveSizeAfterGC(0);
    heap->GetOldSpace()->SetInitialCapacity(10000);
    heap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_MARK);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest014)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->NotifyHeapAliveSizeAfterGC(0);
    sheap->GetOldSpace()->SetInitialCapacity(10000);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest015)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest016)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->NotifyHeapAliveSizeAfterGC(1);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5245000);
    sheap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest017)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->NotifyHeapAliveSizeAfterGC(1);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5245000);
    sheap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    sheap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest018)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->NotifyHeapAliveSizeAfterGC(1);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5245000);
    sheap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    sheap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest019)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->NotifyHeapAliveSizeAfterGC(1);
    sheap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    sheap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC);
}

}  // namespace panda::test