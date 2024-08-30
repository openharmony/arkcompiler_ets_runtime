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

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/stw_young_gc.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

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

HWTEST_F_L0(GCTest, SharedHeapUpdateWorkManagerTest)
{
    constexpr size_t ALLOCATE_COUNT = 10;
    constexpr size_t ALLOCATE_SIZE = 512;
    auto sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t totalThreadNum = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    SharedGCWorkManager *sWorkManager = sHeap->GetWorkManager();
    delete sWorkManager;
    sWorkManager = new SharedGCWorkManager(sHeap, totalThreadNum + 1);
    sHeap->UpdateWorkManager(sWorkManager);
    auto oldSizebase = sHeap->GetOldSpace()->GetHeapObjectSize();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < ALLOCATE_COUNT; i++) {
            factory->NewSOldSpaceTaggedArray(ALLOCATE_SIZE, JSTaggedValue::Undefined());
        }
    }
    size_t oldSizeBefore = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizebase);
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    size_t oldSizeAfter = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizeAfter);
}

HWTEST_F_L0(GCTest, HeapUpdateWorkManagerTest)
{
    constexpr size_t ALLOCATE_COUNT = 10;
    constexpr size_t ALLOCATE_SIZE = 512;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t totalThreadNum = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    WorkManager *workManager = heap->GetWorkManager();
    delete workManager;
    workManager = new WorkManager(heap, totalThreadNum + 1);
    heap->UpdateWorkManager(workManager);
    auto oldSizebase = heap->GetOldSpace()->GetHeapObjectSize();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < ALLOCATE_COUNT; i++) {
            factory->NewOldSpaceTaggedArray(ALLOCATE_SIZE, JSTaggedValue::Undefined());
        }
    }
    size_t oldSizeBefore = heap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizebase);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    size_t oldSizeAfter = heap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizeAfter);
}
}