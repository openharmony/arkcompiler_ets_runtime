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
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/serializer/serialize_chunk.h"
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

HWTEST_F_L0(GCTest, FullGCOne)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto heap = thread->GetEcmaVM()->GetHeap();
    auto fullGc = heap->GetFullGC();
    fullGc->RunPhases();
    auto oldSizebase = heap->GetOldSpace()->GetHeapObjectSize();
    size_t oldSizeBefore = 0;
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 1024; i++) {
            factory->NewTaggedArray(512, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
        oldSizeBefore = heap->GetOldSpace()->GetHeapObjectSize();
        EXPECT_TRUE(oldSizeBefore > oldSizebase);
    }
    fullGc->RunPhases();
    auto oldSizeAfter = heap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizeAfter);
}

HWTEST_F_L0(GCTest, ChangeGCParams)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::HIGH_THROUGHPUT);
#if !ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    EXPECT_TRUE(heap->GetConcurrentMarker()->IsEnabled());
    uint32_t markTaskNum = heap->GetMaxMarkTaskCount();
#endif
    EXPECT_TRUE(heap->GetSweeper()->ConcurrentSweepEnabled());
    uint32_t evacuateTaskNum = heap->GetMaxEvacuateTaskCount();

    auto partialGc = heap->GetPartialGC();
    partialGc->RunPhases();
    heap->ChangeGCParams(true);
    heap->Prepare();
#if !ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    uint32_t markTaskNumBackground = heap->GetMaxMarkTaskCount();
    EXPECT_TRUE(markTaskNum > markTaskNumBackground);
    EXPECT_FALSE(heap->GetConcurrentMarker()->IsEnabled());
#endif
    uint32_t evacuateTaskNumBackground = heap->GetMaxEvacuateTaskCount();
    EXPECT_TRUE(evacuateTaskNum > evacuateTaskNumBackground);
    EXPECT_FALSE(heap->GetSweeper()->ConcurrentSweepEnabled());
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::CONSERVATIVE);

    partialGc->RunPhases();
    heap->ChangeGCParams(false);
    heap->Prepare();
#if !ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    uint32_t markTaskNumForeground = heap->GetMaxMarkTaskCount();
    EXPECT_EQ(markTaskNum, markTaskNumForeground);
    EXPECT_TRUE(heap->GetConcurrentMarker()->IsEnabled());
#endif
    uint32_t evacuateTaskNumForeground = heap->GetMaxEvacuateTaskCount();
    EXPECT_EQ(evacuateTaskNum, evacuateTaskNumForeground);
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::HIGH_THROUGHPUT);
    EXPECT_TRUE(heap->GetSweeper()->ConcurrentSweepEnabled());
}

HWTEST_F_L0(GCTest, ConfigDisable)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::CONFIG_DISABLE);
    heap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::CONFIG_DISABLE);

    EXPECT_FALSE(heap->GetConcurrentMarker()->IsEnabled());
    EXPECT_FALSE(heap->GetSweeper()->ConcurrentSweepEnabled());

    auto partialGc = heap->GetPartialGC();
    partialGc->RunPhases();
    heap->ChangeGCParams(false);
    heap->Prepare();

    EXPECT_FALSE(heap->GetConcurrentMarker()->IsEnabled());
    EXPECT_FALSE(heap->GetSweeper()->ConcurrentSweepEnabled());
}

HWTEST_F_L0(GCTest, NotifyMemoryPressure)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::HIGH_THROUGHPUT);
#if !ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    uint32_t markTaskNum = heap->GetMaxMarkTaskCount();
#endif
    uint32_t evacuateTaskNum = heap->GetMaxEvacuateTaskCount();

    auto partialGc = heap->GetPartialGC();
    partialGc->RunPhases();
    heap->ChangeGCParams(true);
    heap->NotifyMemoryPressure(true);
    heap->Prepare();
#if !ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    uint32_t markTaskNumBackground = heap->GetMaxMarkTaskCount();
    EXPECT_TRUE(markTaskNum > markTaskNumBackground);
    EXPECT_FALSE(heap->GetConcurrentMarker()->IsEnabled());
#endif
    uint32_t evacuateTaskNumBackground = heap->GetMaxEvacuateTaskCount();
    EXPECT_TRUE(evacuateTaskNum > evacuateTaskNumBackground);
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::PRESSURE);
    EXPECT_FALSE(heap->GetSweeper()->ConcurrentSweepEnabled());

    partialGc->RunPhases();
    heap->ChangeGCParams(false);
    heap->Prepare();
#if !ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    uint32_t markTaskNumForeground = heap->GetMaxMarkTaskCount();
    EXPECT_EQ(markTaskNum, markTaskNumForeground);
#endif
    uint32_t evacuateTaskNumForeground = heap->GetMaxEvacuateTaskCount();
    EXPECT_EQ(evacuateTaskNum, evacuateTaskNumForeground);
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::PRESSURE);

    heap->NotifyMemoryPressure(false);
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::CONSERVATIVE);
}

HWTEST_F_L0(GCTest, NativeBindingCheckGCTest)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    size_t oldNativeSize = heap->GetNativeBindingSize();
    EcmaTestCommon::GcCommonCase(thread, heap);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    auto newNativeSize = heap->GetNativeBindingSize();
    EXPECT_EQ(newNativeSize - oldNativeSize, 0UL);
}

HWTEST_F_L0(GCTest, SharedGC)
{
#ifdef NDEBUG
    constexpr size_t ALLOCATE_COUNT = 100;
    constexpr size_t ALLOCATE_SIZE = 512;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
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
    auto oldSizeAfter = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizeAfter);
    EXPECT_EQ(oldSizebase, oldSizeAfter);
#endif
}

HWTEST_F_L0(GCTest, SharedFullGC)
{
    constexpr size_t ALLOCATE_COUNT = 100;
    constexpr size_t ALLOCATE_SIZE = 512;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    auto oldSizebase = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizebase > 0);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < ALLOCATE_COUNT; i++) {
            factory->NewSOldSpaceTaggedArray(ALLOCATE_SIZE, JSTaggedValue::Undefined());
        }
    }
    size_t oldSizeBefore = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizebase);
    EXPECT_TRUE(oldSizeBefore > TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), ALLOCATE_SIZE));
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    auto oldSizeAfter = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizeAfter);
}

HWTEST_F_L0(GCTest, SharedFullGCInAppspawn)
{
    constexpr size_t ALLOCATE_COUNT = 10;
    constexpr size_t ALLOCATE_SIZE = 512;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto sHeap = SharedHeap::GetInstance();
    sHeap->CompactHeapBeforeFork(thread);
    EXPECT_TRUE(sHeap->GetOldSpace()->GetHeapObjectSize() == 0);
    auto oldSizebase = sHeap->GetOldSpace()->GetHeapObjectSize();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < ALLOCATE_COUNT; i++) {
            factory->NewSOldSpaceTaggedArray(ALLOCATE_SIZE, JSTaggedValue::Undefined());
        }
    }
    size_t oldSizeBefore = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizebase);
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    sHeap->GetAppSpawnSpace()->IterateOverMarkedObjects([](TaggedObject *object) {
        Region *objectRegion = Region::ObjectAddressToRange(object);
        EXPECT_TRUE(objectRegion->InSharedAppSpawnSpace());
    });
    auto oldSizeAfter = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizeAfter);
    EXPECT_EQ(oldSizebase, oldSizeAfter);
}

HWTEST_F_L0(GCTest, SharedGCSuspendAll)
{
    EXPECT_TRUE(thread->IsInRunningState());
    {
        SuspendAllScope suspendScope(thread);
        EXPECT_TRUE(!thread->IsInRunningState());
    }
    EXPECT_TRUE(thread->IsInRunningState());
}

HWTEST_F_L0(GCTest, SerializeGCCheck)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t index = 0;
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        JSHandle<EcmaString> key1(factory->NewFromASCII("error1"));
        JSHandle<EcmaString> key2(factory->NewFromASCII("error2"));
        JSHandle<EcmaString> msg(factory->NewFromASCII("this is error"));
        auto chunk = std::make_unique<SerializationChunk>();
        chunk->Emplace(reinterpret_cast<JSTaggedType>(key1.GetTaggedValue().GetTaggedObject()));
        chunk->Emplace(reinterpret_cast<JSTaggedType>(key2.GetTaggedValue().GetTaggedObject()));
        chunk->Emplace(reinterpret_cast<JSTaggedType>(msg.GetTaggedValue().GetTaggedObject()));
        index = Runtime::GetInstance()->PushSerializationRoot(thread, std::move(chunk));
    }
    auto sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    Runtime::GetInstance()->RemoveSerializationRoot(thread, index);
};

HWTEST_F_L0(GCTest, StatisticHeapDetailTest)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 1024; i++) {
            factory->NewTaggedArray(128, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        }
        for (int i = 0; i < 1024; i++) {
            factory->NewTaggedArray(128, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
        for (int i = 0; i < 1024; i++) {
            factory->NewTaggedArray(128, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        }
    }
    heap->StatisticHeapDetail();
};

HWTEST_F_L0(GCTest, Destroy)
{
    EcmaParamConfiguration config;
    auto sHeap = new SharedHeap(config);
    EXPECT_TRUE(sHeap != nullptr);
    sHeap->Destroy();
    delete sHeap;
    sHeap = nullptr;

    // EcmaVM ecmaVm;
    auto heap = new Heap(thread->GetEcmaVM());
    EXPECT_TRUE(heap != nullptr);
    heap->Destroy();
    delete heap;
    heap = nullptr;
};

}  // namespace panda::test
