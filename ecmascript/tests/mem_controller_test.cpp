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

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/mem_controller_utils.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class MemControllerTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(MemControllerTest, AllocationVerify)
{
#ifdef NDEBUG
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    auto memController = heap->GetMemController();

    heap->CollectGarbage(TriggerGCType::FULL_GC);

    for (int i = 0; i < 1024; i++) {
        // old space object
        [[maybe_unused]] auto oldArray = objectFactory->NewTaggedArray(128, JSTaggedValue::Undefined(),
                                                                       MemSpaceType::OLD_SPACE);
    }
    sleep(5);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    double mutatorSpeed1 = memController->GetCurrentOldSpaceAllocationThroughputPerMS(0);
    for (int i = 0; i < 1024; i++) {
        // old space object
        [[maybe_unused]] auto oldArray = objectFactory->NewTaggedArray(128, JSTaggedValue::Undefined(),
                                                                       MemSpaceType::OLD_SPACE);
    }
    sleep(10);

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    double mutatorSpeed2 = memController->GetCurrentOldSpaceAllocationThroughputPerMS(0);
    ASSERT_TRUE(mutatorSpeed2 < mutatorSpeed1);
#endif
}

HWTEST_F_L0(MemControllerTest, VerifyMutatorSpeed)
{
#ifdef NDEBUG
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    auto memController = heap->GetMemController();

    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    size_t oldSpaceAllocatedSizeBefore = memController->GetOldSpaceAllocAccumulatedSize();
    size_t nonMovableSpaceAllocatedSizeBefore = memController->GetNonMovableSpaceAllocAccumulatedSize();
    double allocDurationBefore = memController->GetAllocTimeMs();
    sleep(1);

    // new space object
    [[maybe_unused]] auto newArray =
        objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    // old space object
    auto oldArray = objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    // non movable object
    auto nonMovableArray = objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
    // huge space object
    static constexpr size_t SIZE = 1_MB;
    auto hugeArray = objectFactory->NewTaggedArray(SIZE);

    auto newSpace = heap->GetNewSpace();
    size_t newSpaceAllocBytesSinceGC = newSpace->GetAllocatedSizeSinceGC(newSpace->GetTop());
    ASSERT_EQ(newSpaceAllocBytesSinceGC, TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), 2));
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    newSpace = heap->GetNewSpace();
    ASSERT_EQ(newSpace->GetAllocatedSizeSinceGC(newSpace->GetTop()), static_cast<size_t>(0));

    size_t oldSpaceAllocatedSizeAfter = memController->GetOldSpaceAllocAccumulatedSize();
    size_t nonMovableSpaceAllocatedSizeAfter = memController->GetNonMovableSpaceAllocAccumulatedSize();
    double allocDurationAfter = memController->GetAllocTimeMs();

    size_t hugeObjectAllocSizeInLastGC = memController->GetHugeObjectAllocSizeSinceGC();

    ASSERT_TRUE(allocDurationAfter - allocDurationBefore > 1000);
    ASSERT_TRUE(oldSpaceAllocatedSizeAfter - oldSpaceAllocatedSizeBefore
                == oldArray->ComputeSize(JSTaggedValue::TaggedTypeSize(), 2));
    ASSERT_TRUE(nonMovableSpaceAllocatedSizeAfter - nonMovableSpaceAllocatedSizeBefore
                == nonMovableArray->ComputeSize(JSTaggedValue::TaggedTypeSize(), 2));
    // The allocated size of huge object must be larger than the object size.
    ASSERT_TRUE(hugeObjectAllocSizeInLastGC > hugeArray->ComputeSize(JSTaggedValue::TaggedTypeSize(), SIZE));
#endif
}

HWTEST_F_L0(MemControllerTest, CalculateMarkCompactSpeedPerMSTest)
{
#ifdef NDEBUG
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto memController = heap->GetMemController();
    auto compactSpeed = memController->CalculateMarkCompactSpeedPerMS();
    EXPECT_TRUE(compactSpeed == 0);
#endif
}

HWTEST_F_L0(MemControllerTest, StartCalculationBeforeGC)
{
    JSNApi::InitHybridVMEnv(thread->GetEcmaVM());
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto memController = heap->GetMemController();
    auto objectFactory = ecmaVm->GetFactory();

    // huge space object
    static constexpr size_t SIZE = 1_MB;
    objectFactory->NewTaggedArray(SIZE);

    double allocTimeMsBefore = memController->GetAllocTimeMs();
    size_t oldSpaceSizeBefore = memController->GetOldSpaceAllocAccumulatedSize();
    size_t nonMovableSpaceSizeBefore = memController->GetNonMovableSpaceAllocAccumulatedSize();
    size_t codeSpaceSizeBefore = memController->GetCodeSpaceAllocAccumulatedSize();
    {
        [[maybe_unused]] auto newArray =
            objectFactory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        // old space object
        [[maybe_unused]] auto oldArray =
            objectFactory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        // non movable object
        [[maybe_unused]] auto nonMovableArray =
            objectFactory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
    }

    sleep(1);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    memController->StartCalculationBeforeGC();
    memController->CheckLowAllocationUsageState();

    double allocTimeMsAfter = memController->GetAllocTimeMs();
    size_t oldSpaceSizeAfter = memController->GetOldSpaceAllocAccumulatedSize();
    size_t nonMovableSpaceSizeAfter = memController->GetNonMovableSpaceAllocAccumulatedSize();
    size_t codeSpaceSizeAfter = memController->GetCodeSpaceAllocAccumulatedSize();

    EXPECT_TRUE(allocTimeMsAfter - allocTimeMsBefore > 1000);
    EXPECT_TRUE(oldSpaceSizeAfter > oldSpaceSizeBefore);
    EXPECT_TRUE(nonMovableSpaceSizeAfter > nonMovableSpaceSizeBefore);
    EXPECT_TRUE(codeSpaceSizeAfter == codeSpaceSizeBefore);
}

HWTEST_F_L0(MemControllerTest, StopCalculationAfterGC)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    auto memController = heap->GetMemController();
    heap->CollectGarbage(TriggerGCType::FULL_GC);

    [[maybe_unused]] auto newArray =
        objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    [[maybe_unused]] auto oldArray =
        objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    [[maybe_unused]] auto nonMovableArray =
        objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
    static constexpr size_t SIZE = 1_MB;
    [[maybe_unused]] auto hugeArray = objectFactory->NewTaggedArray(SIZE);

    heap->CollectGarbage(TriggerGCType::YOUNG_GC);

    double allocDurationSinceGc = memController->GetAllocDurationSinceGc();
    size_t codeSpaceAllocSizeSinceGC = memController->GetCodeSpaceAllocAccumulatedSize();
    EXPECT_EQ(allocDurationSinceGc, static_cast<double>(0.0));
    EXPECT_EQ(codeSpaceAllocSizeSinceGC, static_cast<size_t>(0));
    size_t hugeObjectSize = heap->GetHugeObjectSpace()->GetHeapObjectSize();
    size_t hugeAllocSizeSinceGC = memController->GetHugeObjectAllocSizeSinceGC();
    EXPECT_EQ(hugeAllocSizeSinceGC, hugeObjectSize);
    double markCompactSpeed = memController->CalculateMarkCompactSpeedPerMS();
    EXPECT_GE(markCompactSpeed, 0);
}

HWTEST_F_L0(MemControllerTest, MemControllerUtilsTest001)
{
    base::GCRingBuffer<BytesAndDuration, 10> buffer;
    BytesAndDuration initial = {1_GB + 1, 1};
    double timeMs = 1;
    double result = MemControllerUtils::CalculateAverageSpeed(buffer, initial, timeMs);
    EXPECT_GE(result, 1_GB);
}

HWTEST_F_L0(MemControllerTest, MemControllerUtilsTest002)
{
    base::GCRingBuffer<BytesAndDuration, 10> buffer;
    BytesAndDuration initial = {0, 0};
    double timeMs = 0;
    double result = MemControllerUtils::CalculateAverageSpeed(buffer, initial, timeMs);
    EXPECT_GE(result, 0);
}

HWTEST_F_L0(MemControllerTest, MemControllerUtilsTest003)
{
    base::GCRingBuffer<BytesAndDuration, 10> buffer;
    BytesAndDuration initial = {1, 1_GB + 1};
    double timeMs = 1;
    double result = MemControllerUtils::CalculateAverageSpeed(buffer, initial, timeMs);
    EXPECT_GE(result, 1);
}

HWTEST_F_L0(MemControllerTest, MemControllerUtilsTest004)
{
    base::GCRingBuffer<BytesAndDuration, 10> buffer;
    BytesAndDuration initial = {1_GB / 2, 1};
    double timeMs = 1;
    double result = MemControllerUtils::CalculateAverageSpeed(buffer, initial, timeMs);
    EXPECT_GE(result, 1_GB / 2);
}

HWTEST_F_L0(MemControllerTest, MemControllerUtilsTest005)
{
    base::GCRingBuffer<BytesAndDuration, 10> buffer;
    double result = MemControllerUtils::CalculateAverageSpeed(buffer);
    EXPECT_GE(result, 0);
}

}  // namespace panda::test
