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
#include "ecmascript/napi/include/jsnapi_expo.h"

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
        options.SetEnableForceGC(false);
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
        heap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
        auto idleGCTrigger = heap->GetIdleGCTrigger();
        idleGCTrigger->SetTriggerGCTaskCallback([idleGCTrigger](TriggerGCData& data) {
            idleGCTrigger->ClearPostGCTask(static_cast<panda::JSNApi::TRIGGER_IDLE_GC_TYPE>(data.second));
        });
    }
};

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
    ASSERT_EQ(trigger->TryTriggerIdleSharedOldGC(), false);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleSharedOldGCTest003)
{
    constexpr size_t LIVE_SIZE = 5242889;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->GetOldSpace()->IncreaseLiveObjectSize(LIVE_SIZE);
    sheap->NotifyHeapAliveSizeAfterGC(1);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    ASSERT_EQ(trigger->TryTriggerIdleSharedOldGC(), false);
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
    // fixme: adapt to cms
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetOldSpace()->SetInitialCapacity(10000);
    heap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    heap->NotifyHeapAliveSizeAfterGC(1);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    ASSERT_EQ(trigger->TryTriggerIdleLocalOldGC(), false);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGCTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_EQ(trigger->TryTriggerIdleLocalOldGC(), false);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGCTest003)
{
    // fixme: adapt to cms
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->Mark();
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK);
    ASSERT_EQ(trigger->TryTriggerIdleLocalOldGC(), false);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGCTest004)
{
    // fixme: adapt to cms
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
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
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_EQ(trigger->TryTriggerIdleLocalOldGC(), false);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGCTest006)
{
    // fixme: adapt to cms
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
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
    // fixme: adapt to cms
    if constexpr (G_USE_CMS_GC) {
        return;
    }
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
    // fixme: adapt to cms
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->NotifyHeapAliveSizeAfterGC(1);
    sheap->GetOldSpace()->SetInitialCapacity(10000);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    auto idleGCTrigger = heap->GetIdleGCTrigger();
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest003)
{
    // fixme: adapt to cms
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->NotifyHeapAliveSizeAfterGC(1);
    heap->GetOldSpace()->SetInitialCapacity(10000);
    heap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    auto idleGCTrigger = heap->GetIdleGCTrigger();
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
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
    // fixme: adapt to cms
    if constexpr (G_USE_CMS_GC) {
        return;
    }
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
    // fixme: adapt to cms
    if constexpr (G_USE_CMS_GC) {
        return;
    }
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
    // fixme: adapt to cms
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->NotifyHeapAliveSizeAfterGC(0);
    heap->GetOldSpace()->SetInitialCapacity(10000);
    heap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
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

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGCTest020)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->NotifyHeapAliveSizeAfterGC(1);
    sheap->GetOldSpace()->SetInitialCapacity(10000);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_PARTIAL_MARK);
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyNeedFreeze001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    bool freeze = false;
    auto callback = [&freeze](bool needNextGC, bool needFreeze) {
        if (needFreeze && !needNextGC) {
            freeze = true;
        }
    };
    Runtime::GetInstance()->SetNotifyNextCompressGCCallback(callback);
    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->NotifyHeapAliveSizeAfterGC(1);
    sheap->GetOldSpace()->SetInitialCapacity(10000);
    sheap->GetOldSpace()->IncreaseLiveObjectSize(5242889);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC);
    sheap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    ASSERT_TRUE(freeze);
}


/**
 * @tc.name: NotifyNextGC001
 * @tc.desc: NotifyNextGC
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, NotifyNextGC001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());
    bool freeze = false;
    bool nextGC = false;
    auto callback = [&freeze, &nextGC](bool needNextGC, bool needFreeze) {
        if (needNextGC) {
            nextGC = true;
            return;
        }
        if (needFreeze && !needNextGC) {
            nextGC = false;
            freeze = true;
            return;
        }
        if (!needNextGC) {
            nextGC = false;
            return;
        }
    };
    Runtime::GetInstance()->SetNotifyNextCompressGCCallback(callback);

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::FULL_GC);
    ASSERT_TRUE(nextGC);
    ASSERT_FALSE(freeze);

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::FULL_GC);
    ASSERT_TRUE(nextGC);
    ASSERT_FALSE(freeze);

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC);
    ASSERT_FALSE(nextGC);
    ASSERT_TRUE(freeze);
}

/**
 * @tc.name: NotifyNextGC002
 * @tc.desc: NotifyNextGC
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, NotifyNextGC002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());
    bool freeze = false;
    bool nextGC = false;
    auto callback = [&freeze, &nextGC](bool needNextGC, bool needFreeze) {
        if (needNextGC) {
            nextGC = true;
            return;
        }
        if (needFreeze && !needNextGC) {
            nextGC = false;
            freeze = true;
            return;
        }
        if (!needNextGC) {
            nextGC = false;
            return;
        }
    };
    Runtime::GetInstance()->SetNotifyNextCompressGCCallback(callback);

    auto oldSpace = heap->GetOldSpace();
    oldSpace->IncreaseLiveObjectSize(100_MB);
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::FULL_GC);
    ASSERT_TRUE(nextGC);
    ASSERT_FALSE(freeze);

    oldSpace->IncreaseLiveObjectSize(100_MB);
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::FULL_GC);
    ASSERT_TRUE(nextGC);
    ASSERT_FALSE(freeze);

    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->GetOldSpace()->IncreaseLiveObjectSize(200_MB);
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC);
    ASSERT_FALSE(nextGC);
    ASSERT_FALSE(freeze);

    sheap->WaitAllTasksFinished(thread);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_FALSE(nextGC);
    ASSERT_TRUE(freeze);
}

/**
 * @tc.name: NotifyNextGC003
 * @tc.desc: NotifyNextGC
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, NotifyNextGC003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());
    bool freeze = false;
    bool nextGC = false;
    auto callback = [&freeze, &nextGC](bool needNextGC, bool needFreeze) {
        if (needNextGC) {
            nextGC = true;
            return;
        }
        if (needFreeze && !needNextGC) {
            nextGC = false;
            freeze = true;
            return;
        }
        if (!needNextGC) {
            nextGC = false;
            return;
        }
    };
    Runtime::GetInstance()->SetNotifyNextCompressGCCallback(callback);

    auto oldSpace = heap->GetOldSpace();
    oldSpace->IncreaseLiveObjectSize(100_MB);
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::FULL_GC);
    ASSERT_TRUE(nextGC);
    ASSERT_FALSE(freeze);

    oldSpace->IncreaseLiveObjectSize(100_MB);
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::FULL_GC);
    ASSERT_TRUE(nextGC);
    ASSERT_FALSE(freeze);

    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->GetOldSpace()->IncreaseLiveObjectSize(200_MB);
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC);
    ASSERT_FALSE(nextGC);
    ASSERT_FALSE(freeze);

    sheap->WaitAllTasksFinished(thread);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_FALSE(nextGC);
    ASSERT_TRUE(freeze);
}

/**
 * @tc.name: NotifyNextGC004
 * @tc.desc: NotifyNextGC
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, NotifyNextGC004)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());
    bool freeze = false;
    bool nextGC = false;
    auto callback = [&freeze, &nextGC](bool needNextGC, bool needFreeze) {
        if (needNextGC) {
            nextGC = true;
            return;
        }
        if (needFreeze && !needNextGC) {
            nextGC = false;
            freeze = true;
            return;
        }
        if (!needNextGC) {
            nextGC = false;
            return;
        }
    };
    Runtime::GetInstance()->SetNotifyNextCompressGCCallback(callback);

    SharedHeap *sheap = SharedHeap::GetInstance();
    sheap->GetOldSpace()->IncreaseLiveObjectSize(200_MB);
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC);
    ASSERT_FALSE(nextGC);
    ASSERT_FALSE(freeze);

    sheap->WaitAllTasksFinished(thread);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_FALSE(nextGC);
    ASSERT_TRUE(freeze);

    freeze = false;
    auto oldSpace = heap->GetOldSpace();
    oldSpace->IncreaseLiveObjectSize(100_MB);
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::FULL_GC);
    ASSERT_TRUE(nextGC);
    ASSERT_FALSE(freeze);

    oldSpace->IncreaseLiveObjectSize(100_MB);
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::FULL_GC);
    ASSERT_TRUE(nextGC);
    ASSERT_FALSE(freeze);
}
/**
 * @tc.name: ExpectedMemoryReclamationSize001
 * @tc.desc: ExpectedMemoryReclamationSize
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, ExpectedMemoryReclamationSize001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());

    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);
    auto oldExceptedSize = idleGCTrigger->GetExpectedMemoryReclamationSize();
    ASSERT_TRUE(oldExceptedSize < 100_MB);

    size_t oldAliveSizeAfterGC = heap->GetHeapAliveSizeAfterGC();
    size_t oldHeapObjSize = heap->GetHeapObjectSize();
    ASSERT_TRUE(oldHeapObjSize >= oldAliveSizeAfterGC);

    size_t oldFragmentSizeAfterGC = heap->GetFragmentSizeAfterGC();
    size_t heapBasicLoss = heap->GetHeapBasicLoss();
    ASSERT_TRUE(oldFragmentSizeAfterGC >= heapBasicLoss);

    heap->GetOldSpace()->IncreaseLiveObjectSize(100_MB);
    size_t newHeapObjSize = heap->GetHeapObjectSize();
    ASSERT_EQ(newHeapObjSize, oldHeapObjSize + 100_MB);
    auto newExceptedSize = idleGCTrigger->GetExpectedMemoryReclamationSize();
    ASSERT_TRUE(newExceptedSize >= 100_MB);
}

/**
 * @tc.name: ExpectedMemoryReclamationSize002
 * @tc.desc: ExpectedMemoryReclamationSize
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, ExpectedMemoryReclamationSize002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());

    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);
    auto oldExceptedSize = idleGCTrigger->GetExpectedMemoryReclamationSize();

    size_t oldAliveSizeAfterGC = heap->GetHeapAliveSizeAfterGC();
    size_t oldHeapObjSize = heap->GetHeapObjectSize();
    ASSERT_TRUE(oldHeapObjSize >= oldAliveSizeAfterGC);

    size_t oldFragmentSizeAfterGC = heap->GetFragmentSizeAfterGC();
    size_t heapBasicLoss = heap->GetHeapBasicLoss();
    ASSERT_TRUE(oldFragmentSizeAfterGC >= heapBasicLoss);

    ASSERT_TRUE(oldExceptedSize == 0);

    ASSERT_TRUE(oldAliveSizeAfterGC <= 100_MB);
    heap->NotifyHeapAliveSizeAfterGC(100_MB);
    auto newExceptedSize = idleGCTrigger->GetExpectedMemoryReclamationSize();
    ASSERT_TRUE(newExceptedSize == 0);
}

/**
 * @tc.name: PossiblePostGCTaskTest001
 * @tc.desc: PossiblePostGCTaskTest
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, PossiblePostGCTaskTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());
    SharedHeap *sheap = SharedHeap::GetInstance();

    sheap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::IDLE>(thread);
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);

    sheap->GetOldSpace()->IncreaseLiveObjectSize(100_MB);
    sheap->NotifyHeapAliveSizeAfterGC(1);

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_PARTIAL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_PARTIAL_MARK));
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_PARTIAL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_PARTIAL_MARK));

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_PARTIAL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_PARTIAL_MARK));
}

/**
 * @tc.name: PossiblePostGCTaskTest002
 * @tc.desc: PossiblePostGCTaskTest
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, PossiblePostGCTaskTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());
    SharedHeap *sheap = SharedHeap::GetInstance();

    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);

    sheap->GetOldSpace()->IncreaseLiveObjectSize(100_MB);
    sheap->NotifyHeapAliveSizeAfterGC(1);

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK));
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK));

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK));
}


/**
 * @tc.name: PossiblePostGCTaskTest003
 * @tc.desc: PossiblePostGCTaskTest
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, PossiblePostGCTaskTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());

    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK));
    idleGCTrigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    ASSERT_FALSE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK));

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 100; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK));

    idleGCTrigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    ASSERT_FALSE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK));
    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK));

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK);
}

/**
 * @tc.name: PossiblePostGCTaskTest004
 * @tc.desc: PossiblePostGCTaskTest
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, PossiblePostGCTaskTest004)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());

    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK));
    idleGCTrigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_FALSE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK));

    heap->GetOldSpace()->IncreaseLiveObjectSize(100_MB);
    heap->NotifyHeapAliveSizeAfterGC(1_MB);

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK));

    idleGCTrigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_FALSE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK));
    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK));

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK);
}

/**
 * @tc.name: PossiblePostGCTaskTest005
 * @tc.desc: PossiblePostGCTaskTest
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, PossiblePostGCTaskTest005)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());

    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 720; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }

    auto newSpace = heap->GetNewSpace();
    newSpace->SetInitialCapacity(1);

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK));
    ASSERT_TRUE(idleGCTrigger->TryTriggerIdleYoungGC());

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK));
}

/**
 * @tc.name: TryTriggerIdleLocalOldGCTest1
 * @tc.desc: TryTriggerIdleLocalOldGCTest1
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGCTest1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());

    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);

    heap->GetOldSpace()->IncreaseLiveObjectSize(100_MB);
    heap->NotifyHeapAliveSizeAfterGC(1_MB);

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK));
    ASSERT_TRUE(idleGCTrigger->TryTriggerIdleLocalOldGC());

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK));
}

/**
 * @tc.name: TryTriggerIdleSharedOldGCTest1
 * @tc.desc: TryTriggerIdleSharedOldGC
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleSharedOldGCTest1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());
    SharedHeap *sheap = SharedHeap::GetInstance();

    sheap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::IDLE>(thread);
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);

    sheap->GetOldSpace()->IncreaseLiveObjectSize(100_MB);
    sheap->NotifyHeapAliveSizeAfterGC(1_MB);

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK));
    ASSERT_TRUE(idleGCTrigger->TryTriggerIdleSharedOldGC());

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK));
}


/**
 * @tc.name: ReachIdleLocalOldGCThresholdsTest1
 * @tc.desc: ReachIdleLocalOldGCThresholdsTest1
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, ReachIdleLocalOldGCThresholdsTest1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());

    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);

    heap->GetOldSpace()->IncreaseLiveObjectSize(100_MB);
    heap->NotifyHeapAliveSizeAfterGC(1_MB);

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK));
    ASSERT_TRUE(idleGCTrigger->ReachIdleLocalOldGCThresholds());
    ASSERT_TRUE(idleGCTrigger->TryTriggerIdleLocalOldGC());

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK));
}


/**
 * @tc.name: ReachIdleSharedGCThresholdsTest1
 * @tc.desc: ReachIdleSharedGCThresholdsTest1
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, ReachIdleSharedGCThresholdsTest1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());
    SharedHeap *sheap = SharedHeap::GetInstance();

    sheap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::IDLE>(thread);
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);

    sheap->GetOldSpace()->IncreaseLiveObjectSize(100_MB);
    sheap->NotifyHeapAliveSizeAfterGC(1_MB);
    sheap->GetOldSpace()->SetInitialCapacity(1);

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK));
    ASSERT_TRUE(idleGCTrigger->ReachIdleSharedGCThresholds());

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK));
}


/**
 * @tc.name: TryPostHandleMarkFinishedTest2
 * @tc.desc: TryPostHandleMarkFinishedTest2
 * @tc.type: FUNC
 */
HWTEST_F_L0(IdleGCTriggerTest, TryPostHandleMarkFinishedTest2)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto idleGCTrigger = const_cast<IdleGCTrigger *>(heap->GetIdleGCTrigger());
    SharedHeap *sheap = SharedHeap::GetInstance();

    sheap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::IDLE>(thread);
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);

    idleGCTrigger->NotifyLooperIdleStart(1, 1);
    idleGCTrigger->NotifyLooperIdleEnd(2);

    heap->GetOldSpace()->IncreaseLiveObjectSize(100_MB);
    heap->NotifyHeapAliveSizeAfterGC(1_MB);

    idleGCTrigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK));
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK));

    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK));
    idleGCTrigger->NotifyLooperIdleStart(10, 1);
    idleGCTrigger->TryPostHandleMarkFinished();
    ASSERT_FALSE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK));

    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK);
    ASSERT_TRUE(idleGCTrigger->IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK));
    idleGCTrigger->NotifyLooperIdleEnd(20);
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->Mark();
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
    heap->GetJSThread()->SetSharedMarkStatus(SharedMarkStatus::READY_TO_CONCURRENT_MARK);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    EXPECT_FALSE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart2)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    EXPECT_FALSE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart3)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->Mark();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    EXPECT_FALSE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart4)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetJSThread()->SetSharedMarkStatus(SharedMarkStatus::READY_TO_CONCURRENT_MARK);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    EXPECT_FALSE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart5)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
    heap->GetConcurrentMarker()->Mark();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    EXPECT_FALSE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart6)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
    heap->GetJSThread()->SetSharedMarkStatus(SharedMarkStatus::READY_TO_CONCURRENT_MARK);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    EXPECT_FALSE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart7)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->Mark();
    heap->GetJSThread()->SetSharedMarkStatus(SharedMarkStatus::READY_TO_CONCURRENT_MARK);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    EXPECT_FALSE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart8)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    EXPECT_TRUE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart9)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    EXPECT_TRUE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart10)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    EXPECT_TRUE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart11)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    EXPECT_TRUE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart12)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    EXPECT_TRUE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart13)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    trigger->SetPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    EXPECT_TRUE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, NotifyLooperIdleStart14)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    trigger->ClearPostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    EXPECT_FALSE(trigger->NotifyLooperIdleStart(1, 1));
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerHandleMarkFinished1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetJSThread()->SetSharedMarkStatus(SharedMarkStatus::CONCURRENT_MARKING_OR_FINISHED);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerHandleMarkFinished();
    EXPECT_FALSE(heap->GetJSThread()->IsReadyToSharedConcurrentMark());
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerHandleMarkFinished2)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetJSThread()->SetSharedMarkStatus(SharedMarkStatus::READY_TO_CONCURRENT_MARK);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerHandleMarkFinished();
    EXPECT_TRUE(heap->GetJSThread()->IsReadyToSharedConcurrentMark());
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerHandleMarkFinished3)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
    heap->SetOnSerializeEvent(true);
    heap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerHandleMarkFinished();
    EXPECT_TRUE(heap->GetJSThread()->IsMarkFinished());
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerHandleMarkFinished4)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->Mark();
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
    heap->SetOnSerializeEvent(true);
    heap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerHandleMarkFinished();
    EXPECT_TRUE(heap->GetJSThread()->IsMarkFinished());
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerHandleMarkFinished5)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->Mark();
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
    heap->SetOnSerializeEvent(false);
    heap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerHandleMarkFinished();
    EXPECT_TRUE(heap->GetJSThread()->IsMarkFinished());
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerLocalConcurrentMark1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->GetJSThread()->SetMarkStatus(MarkStatus::READY_TO_MARK);
    EXPECT_TRUE(heap->GetConcurrentMarker()->IsEnabled());
    EXPECT_TRUE(heap->CheckCanTriggerConcurrentMarking());
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerLocalConcurrentMark(MarkType::MARK_YOUNG);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerLocalConcurrentMark2)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARKING);
    EXPECT_TRUE(heap->GetConcurrentMarker()->IsEnabled());
    EXPECT_FALSE(heap->CheckCanTriggerConcurrentMarking());
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerLocalConcurrentMark(MarkType::MARK_YOUNG);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerLocalConcurrentMark3)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    heap->GetJSThread()->SetMarkStatus(MarkStatus::MARKING);
    EXPECT_FALSE(heap->GetConcurrentMarker()->IsEnabled());
    EXPECT_FALSE(heap->CheckCanTriggerConcurrentMarking());
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerLocalConcurrentMark(MarkType::MARK_YOUNG);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleLocalOldGC1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    thread->SetFullMarkRequest();
    heap->SetOnSerializeEvent(false);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    EXPECT_FALSE(trigger->TryTriggerIdleLocalOldGC());
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleSharedOldGC1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();

    sheap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    sheap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    ASSERT_EQ(trigger->TryTriggerIdleSharedOldGC(), false);
}

HWTEST_F_L0(IdleGCTriggerTest, TryTriggerIdleGC1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    SharedHeap *sheap = SharedHeap::GetInstance();
    IdleGCTrigger *trigger = new IdleGCTrigger(heap, sheap, thread);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CC_GC);
    sheap->NotifyHeapAliveSizeAfterGC(0);
    sheap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->GetJSThread()->SetSharedMarkStatus(SharedMarkStatus::READY_TO_CONCURRENT_MARK);
    EXPECT_FALSE(heap->NeedStopCollection());
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_PARTIAL_MARK);
    trigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
}
}  // namespace panda::test