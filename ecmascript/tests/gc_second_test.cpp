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
#include "ecmascript/tests/ecma_test_common.h"
#include "ecmascript/napi/include/jsnapi_expo.h"

using namespace panda;

using namespace panda::ecmascript;
using TRIGGER_IDLE_GC_TYPE = panda::JSNApi::TRIGGER_IDLE_GC_TYPE;

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

HWTEST_F_L0(GCTest, NativeGCTestConcurrentMarkDisabled)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    // Disable concurrent mark.
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    size_t oldNativeSize = heap->GetNativeBindingSize();
    EcmaTestCommon::GcCommonCase(thread, heap, false);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::OLD_GC);
    auto newNativeSize = heap->GetNativeBindingSize();
    EXPECT_EQ(newNativeSize - oldNativeSize, 0UL);
}

HWTEST_F_L0(GCTest, NonNewSpaceNativeGCTestConcurrentMarkDisabled)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    // Disable concurrent mark.
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    size_t oldNativeSize = heap->GetNativeBindingSize();
    EcmaTestCommon::GcCommonCase(thread, heap);
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::OLD_GC);
    auto newNativeSize = heap->GetNativeBindingSize();
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

HWTEST_F_L0(GCTest, ColdStartForceExpand)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    size_t originalHeapSize = heap->GetCommittedSize();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    heap->NotifyPostFork();
    heap->NotifyFinishColdStartSoon();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 500; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t expandHeapSize = thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    usleep(2500000);
    size_t newSize = EcmaTestCommon::GcCommonCase(thread);
    EXPECT_TRUE(originalHeapSize < expandHeapSize);
    EXPECT_TRUE(expandHeapSize > newSize);
}

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

HWTEST_F_L0(GCTest, ColdStartNoConcurrentMark)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->NotifyPostFork();
    heap->NotifyHighSensitive(true);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 500; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    EXPECT_FALSE(heap->HandleExitHighSensitiveEvent());
    heap->NotifyHighSensitive(false);
    EXPECT_FALSE(heap->HandleExitHighSensitiveEvent());
    heap->FinishStartupEvent();

    heap->NotifyHighSensitive(true);
    heap->NotifyHighSensitive(false);
    EXPECT_TRUE(heap->HandleExitHighSensitiveEvent());
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
    size_t number = heap->concurrentNativePointerList_.size();
    EXPECT_TRUE(number > 0);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    size_t newNumber = heap->concurrentNativePointerList_.size();
    EXPECT_TRUE(number > newNumber);
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
    auto idleGCTrigger = heap->GetIdleGCTrigger();
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
    for (size_t i = 0; i < 10240; i++)
    {
        factory->NewTaggedArray(512, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        factory->NewSOldSpaceTaggedArray(512, JSTaggedValue::Hole());
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        [[maybe_unused]] JSHandle<TaggedArray> array = factory->NewTaggedArray(1024, JSTaggedValue::Hole(),
                    MemSpaceType::OLD_SPACE);
        [[maybe_unused]] JSHandle<TaggedArray> sArray = factory->NewSOldSpaceTaggedArray(1024,
                    JSTaggedValue::Hole());
        if (i%340 == 0) {
            idleGCTrigger->NotifyVsyncIdleStart();
        }
    }
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::FULL_GC);
    idleGCTrigger->TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC);
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
