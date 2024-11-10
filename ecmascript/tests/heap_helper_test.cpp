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

HWTEST_F_L0(HeapTest, TryTriggerIdleCollectionTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto helper = new HeapTestHelper(heap);
    helper->SetIdleGCState(true);
    helper->GetHeap()->SetIdleTask(IdleTaskType::NO_TASK);
    helper->GetHeap()->GetJSThread()->SetMarkStatus(MarkStatus::READY_TO_MARK);
    helper->GetHeap()->CollectGarbage(TriggerGCType::EDEN_GC, GCReason::TRIGGER_BY_TASKPOOL);
    helper->GetHeap()->TryTriggerIdleCollection();
}

HWTEST_F_L0(HeapTest, TryTriggerIncrementalMarkingTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto helper = new HeapTestHelper(heap);
    helper->SetIdleGCState(true);
    helper->GetHeap()->SetIdleTask(IdleTaskType::NO_TASK);
    helper->GetHeap()->GetJSThread()->SetMarkStatus(MarkStatus::READY_TO_MARK);
    helper->GetHeap()->CollectGarbage(TriggerGCType::EDEN_GC, GCReason::TRIGGER_BY_TASKPOOL);
    helper->GetHeap()->TryTriggerIncrementalMarking();
}

HWTEST_F_L0(HeapTest, TryTriggerIncrementalMarkingTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto helper = new HeapTestHelper(heap);
    helper->SetIdleGCState(true);
    helper->GetHeap()->GetOldSpace()->IncreaseLiveObjectSize(467664896);
    helper->GetHeap()->SetIdleTask(IdleTaskType::NO_TASK);
    helper->GetHeap()->GetJSThread()->SetMarkStatus(MarkStatus::READY_TO_MARK);
    helper->GetHeap()->CollectGarbage(TriggerGCType::EDEN_GC, GCReason::TRIGGER_BY_TASKPOOL);
    helper->GetHeap()->TryTriggerIncrementalMarking();
}

}