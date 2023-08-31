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
#include "ecmascript/object_factory.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/stw_young_gc.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class GCTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        scope = new EcmaHandleScope(thread);
        auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
        heap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
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
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    size_t oldNativeSize = heap->GetNativeBindingSize();
    size_t newNativeSize = heap->GetNativeBindingSize();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
        [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(newData,
            NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1 * 1024 * 1024);
        newNativeSize = heap->GetNativeBindingSize();
        EXPECT_EQ(newNativeSize - oldNativeSize, 1UL * 1024 * 1024);

        auto newData1 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
        [[maybe_unused]] JSHandle<JSNativePointer> obj2 = factory->NewJSNativePointer(newData1,
            NativeAreaAllocator::FreeBufferFunc, nullptr, false, 1 * 1024 * 1024);

        EXPECT_TRUE(newNativeSize - oldNativeSize > 0);
        EXPECT_TRUE(newNativeSize - oldNativeSize <= 2 * 1024 *1024);
        for (int i = 0; i < 300; i++) {
            auto newData2 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1024);
            // malloc size is smaller to avoid test fail in the small devices.
            [[maybe_unused]] JSHandle<JSNativePointer> obj3 = factory->NewJSNativePointer(newData2,
                NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1 * 1024 * 1024);
        }
        // Old GC should be trigger here, so the size should be reduced.
        EXPECT_TRUE(newNativeSize - oldNativeSize < 256 * 1024 *1024);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    newNativeSize = heap->GetNativeBindingSize();
    EXPECT_EQ(newNativeSize - oldNativeSize, 0UL);
}

HWTEST_F_L0(GCTest, NativeGCTestConcurrentMarkDisabled)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    // Disable concurrent mark.
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    size_t oldNativeSize = heap->GetNativeBindingSize();
    size_t newNativeSize = heap->GetNativeBindingSize();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
        [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(newData,
            NativeAreaAllocator::FreeBufferFunc, nullptr, false, 1 * 1024 * 1024);
        newNativeSize = heap->GetNativeBindingSize();
        EXPECT_EQ(newNativeSize - oldNativeSize, 1UL * 1024 * 1024);

        auto newData1 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
        [[maybe_unused]] JSHandle<JSNativePointer> obj2 = factory->NewJSNativePointer(newData1,
            NativeAreaAllocator::FreeBufferFunc, nullptr, false, 1 * 1024 * 1024);

        EXPECT_TRUE(newNativeSize - oldNativeSize > 0);
        EXPECT_TRUE(newNativeSize - oldNativeSize <= 2 * 1024 *1024);
        for (int i = 0; i < 20; i++) {
            auto newData2 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
            [[maybe_unused]] JSHandle<JSNativePointer> obj3 = factory->NewJSNativePointer(newData2,
                NativeAreaAllocator::FreeBufferFunc, nullptr, false, 1 * 1024 * 1024);
        }
        // Young GC should be trigger here, so the size should be reduced.
        EXPECT_TRUE(newNativeSize - oldNativeSize < 22 * 1024 *1024);
    }
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::OLD_GC);
    newNativeSize = heap->GetNativeBindingSize();
    EXPECT_EQ(newNativeSize - oldNativeSize, 0UL);
}

HWTEST_F_L0(GCTest, NonNewSpaceNativeGCTestConcurrentMarkDisabled)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    // Disable concurrent mark.
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    size_t oldNativeSize = heap->GetNativeBindingSize();
    size_t newNativeSize = heap->GetNativeBindingSize();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
        [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(newData,
            NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1 * 1024 * 1024);
        newNativeSize = heap->GetNativeBindingSize();
        EXPECT_EQ(newNativeSize - oldNativeSize, 1UL * 1024 * 1024);

        auto newData1 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
        [[maybe_unused]] JSHandle<JSNativePointer> obj2 = factory->NewJSNativePointer(newData1,
            NativeAreaAllocator::FreeBufferFunc, nullptr, false, 1 * 1024 * 1024);

        EXPECT_TRUE(newNativeSize - oldNativeSize > 0);
        EXPECT_TRUE(newNativeSize - oldNativeSize <= 2 * 1024 *1024);
        for (int i = 0; i < 300; i++) {
            auto newData2 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1024);
            // malloc size is smaller to avoid test fail in the small devices.
            [[maybe_unused]] JSHandle<JSNativePointer> obj3 = factory->NewJSNativePointer(newData2,
                NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1 * 1024 * 1024);
        }
        // Old GC should be trigger here, so the size should be reduced.
        EXPECT_TRUE(newNativeSize - oldNativeSize < 256 * 1024 *1024);
    }
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::OLD_GC);
    newNativeSize = heap->GetNativeBindingSize();
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

HWTEST_F_L0(GCTest, NoFullConcurrentMarkOldGCTrigger)
{
#ifdef NDEBUG
    auto heap = const_cast<Heap *>(instance->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        for (int i = 0; i < 1024 * 2; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> obj =
                instance->GetFactory()->NewTaggedArray(1024, JSTaggedValue::Hole(), MemSpaceType::NON_MOVABLE);
        }
        EXPECT_EQ(heap->GetOldSpace()->GetOvershootSize(),
                  instance->GetEcmaParamConfiguration().GetOldSpaceOvershootSize());
        EXPECT_TRUE(heap->GetOldGCRequested());
        heap->CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_LIMIT);
        EXPECT_TRUE(thread->HasPendingException());
        JSType errorType = thread->GetException().GetTaggedObject()->GetClass()->GetObjectType();
        EXPECT_EQ(errorType, JSType::JS_OOM_ERROR);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(heap->GetNonMovableSpace()->GetHeapObjectSize() < MAX_NONMOVABLE_LIVE_OBJ_SIZE);
    EXPECT_EQ(heap->GetOldSpace()->GetOvershootSize(), 0);
#endif
}

}  // namespace panda::test
