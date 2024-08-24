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

HWTEST_F_L0(GCTest, ArkToolsHintGC)
{
    Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::CONFIG_DISABLE);
    auto getSizeAfterCreateAndCallHintGC = [this, heap] (size_t &newSize, size_t &finalSize) -> bool {
        {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
            for (int i = 0; i < 500; i++) {
                [[maybe_unused]] JSHandle<TaggedArray> obj = thread->GetEcmaVM()->GetFactory()->
                    NewTaggedArray(10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
            }
            newSize = heap->GetCommittedSize();
        }

        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 0);
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
        // Test HintGC() when in background.
        heap->CollectGarbage(TriggerGCType::FULL_GC);
        heap->ChangeGCParams(true);
        size_t originSize = heap->GetCommittedSize();
        size_t newSize = 0;
        size_t finalSize = 0;
        bool res = getSizeAfterCreateAndCallHintGC(newSize, finalSize);
        EXPECT_TRUE(res);
        EXPECT_TRUE(newSize > originSize);
        EXPECT_TRUE(finalSize < newSize);
        heap->ChangeGCParams(false);
    }
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
    size_t number = vm->GetConcurrentNativePointerListSize();
    EXPECT_TRUE(number > 0);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    size_t newNumber = vm->GetConcurrentNativePointerListSize();
    EXPECT_TRUE(number > newNumber);
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
        for (int i = 0; i < 100; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t newSize = heap->GetNewSpace()->GetCommittedSize();
    size_t newShootSize = heap->GetNewSpace()->GetOvershootSize();

    EXPECT_TRUE(originalYoungSize <= newSize);
    EXPECT_TRUE(newYoungSize > newSize);
    EXPECT_TRUE(newOverShootSize > newShootSize);
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
    for (size_t i = 0; i < 20480; i++)
    {
        factory->NewTaggedArray(512, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        factory->NewSOldSpaceTaggedArray(512, JSTaggedValue::Hole());
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        [[maybe_unused]] JSHandle<TaggedArray> array = factory->NewTaggedArray(1024, JSTaggedValue::Hole(),
                    MemSpaceType::OLD_SPACE);
        [[maybe_unused]] JSHandle<TaggedArray> sArray = factory->NewSOldSpaceTaggedArray(1024,
                    JSTaggedValue::Hole());
        if (i%340 == 0) {
            heap->CheckAndTriggerGCForIdle(150, CheckIdleGCType::LOOPER);
        }
    }
    heap->CheckAndTriggerGCForIdle(350, CheckIdleGCType::LOOPER);
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
    EXPECT_FALSE(space->AdjustCapacity(size, thread));
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
} // namespace panda::test
