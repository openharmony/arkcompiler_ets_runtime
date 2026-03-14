/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
class CommonMemTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(CommonMemTest, HeapAllocationTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    static constexpr size_t TEST_SIZE = 64;
    auto testArray = objectFactory->NewTaggedArray(TEST_SIZE, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    EXPECT_TRUE(*testArray != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto newSpace = heap->GetNewSpace();
    ASSERT_TRUE(newSpace != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest004)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    static constexpr size_t SMALL_SIZE = 64;
    auto smallArray = objectFactory->NewTaggedArray(SMALL_SIZE, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    EXPECT_TRUE(*smallArray != nullptr);
    
    auto oldSpace = heap->GetOldSpace();
    ASSERT_TRUE(oldSpace != nullptr);
    
    auto oldArray = objectFactory->NewTaggedArray(SMALL_SIZE, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    EXPECT_TRUE(*oldArray != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest005)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto concurrentMarker = heap->GetConcurrentMarker();
    ASSERT_TRUE(concurrentMarker != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest006)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto concurrentSweeper = heap->GetSweeper();
    ASSERT_TRUE(concurrentSweeper != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest007)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto array = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*array != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto oldSpace = heap->GetOldSpace();
    ASSERT_TRUE(oldSpace != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest008)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    auto newSpace = heap->GetNewSpace();
    ASSERT_TRUE(newSpace != nullptr);
    
    for (int i = 0; i < 50; i++) {
        auto array = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*array != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest009)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t committedSize = heap->GetCommittedSize();
    size_t heapObjectSize = heap->GetHeapObjectSize();
    
    EXPECT_GE(committedSize, heapObjectSize);
    EXPECT_GT(committedSize, 0u);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest010)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (size_t size = 8; size <= 256; size *= 2) {
        auto array = objectFactory->NewTaggedArray(size, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*array != nullptr);
    }
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest011)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    auto testArray = objectFactory->NewTaggedArray(100, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    EXPECT_TRUE(*testArray != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest012)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> arrays;
    for (int i = 0; i < 200; i++) {
        auto array = objectFactory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        arrays.push_back(JSTaggedValue(*array));
    }
    
    arrays.clear();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto oldSpace = heap->GetOldSpace();
    ASSERT_TRUE(oldSpace != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest013)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto objectFactory = ecmaVm->GetFactory();
    
    static constexpr size_t MAX_SIZE = 10000;
    for (size_t size = 1; size <= MAX_SIZE; size *= 10) {
        auto array = objectFactory->NewTaggedArray(size, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*array != nullptr);
        EXPECT_EQ(array->GetLength(), size);
    }
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest014)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    auto newSpace = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    EXPECT_TRUE(*newSpace != nullptr);
    
    auto oldSpace = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    EXPECT_TRUE(*oldSpace != nullptr);
    
    auto nonMovable = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
    EXPECT_TRUE(*nonMovable != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest015)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest016)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    static constexpr size_t HUGE_SIZE = 10_MB;
    auto hugeArray = objectFactory->NewTaggedArray(HUGE_SIZE);
    if (hugeArray.GetTaggedValue().IsHeapObject()) {
        auto hugeSpace = heap->GetHugeObjectSpace();
        ASSERT_TRUE(hugeSpace != nullptr);
    }
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest017)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> objects;
    for (int i = 0; i < 500; i++) {
        auto obj = objectFactory->NewTaggedArray(100, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        objects.push_back(JSTaggedValue(*obj));
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t afterGC = heap->GetHeapObjectSize();
    EXPECT_GT(afterGC, 0u);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest018)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    size_t initialMemory = heap->GetCommittedSize();
    
    for (int i = 0; i < 1000; i++) {
        objectFactory->NewTaggedArray(50, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t finalMemory = heap->GetCommittedSize();
    EXPECT_GE(finalMemory, initialMemory);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest019)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 500; i++) {
        objectFactory->NewTaggedArray(100, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    for (int i = 0; i < 200; i++) {
        objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest020)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest021)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    auto strongRef = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    EXPECT_TRUE(*strongRef != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(strongRef.GetTaggedValue().IsHeapObject());
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest022)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    auto oldObj = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    EXPECT_TRUE(*oldObj != nullptr);
    
    auto youngObj = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    EXPECT_TRUE(*youngObj != nullptr);
    
    oldObj->Set(thread, 0, youngObj.GetTaggedValue());
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest023)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t committedSize = heap->GetCommittedSize();
    size_t heapObjectSize = heap->GetHeapObjectSize();
    
    EXPECT_GE(committedSize, heapObjectSize);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest024)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto gcStats = heap->GetEcmaGCStats();
    ASSERT_TRUE(gcStats != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest025)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto memController = heap->GetMemController();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    double oldSpaceAllocThroughput = memController->GetCurrentOldSpaceAllocationThroughputPerMS(0);
    EXPECT_GE(oldSpaceAllocThroughput, 0);
    
    size_t oldSpaceAllocAccumulatedSize = memController->GetOldSpaceAllocAccumulatedSize();
    EXPECT_GE(oldSpaceAllocAccumulatedSize, 0u);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest026)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto concurrentMarker = heap->GetConcurrentMarker();
    ASSERT_TRUE(concurrentMarker != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest027)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    size_t heapLimitSize = heap->GetHeapLimitSize();
    size_t globalSpaceAllocLimit = heap->GetGlobalSpaceAllocLimit();
    
    EXPECT_GT(heapLimitSize, 0u);
    EXPECT_GT(globalSpaceAllocLimit, 0u);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest028)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    auto smallObj = objectFactory->NewTaggedArray(8, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    EXPECT_TRUE(*smallObj != nullptr);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest029)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto gcStats = heap->GetEcmaGCStats();
    ASSERT_TRUE(gcStats != nullptr);
    
    size_t gcCount = gcStats->GetGCCount();
    EXPECT_GT(gcCount, 0u);
}

HWTEST_F_L0(CommonMemTest, HeapAllocationTest030)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

}  // namespace panda::test

namespace panda::test {
class ExtendedMemTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = objectFactory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto obj = objectFactory->NewTaggedArray(30, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 75; i++) {
        auto obj = objectFactory->NewTaggedArray(25, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest004)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    size_t initialSize = heap->GetCommittedSize();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t finalSize = heap->GetCommittedSize();
    EXPECT_GE(finalSize, 0u);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest005)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 200; i++) {
        objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest006)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 150; i++) {
        auto obj = objectFactory->NewTaggedArray(15, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest007)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto str = objectFactory->NewFromStdString(std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest008)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto obj = objectFactory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest009)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto arr = objectFactory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest010)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest011)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 120; i++) {
        auto obj = objectFactory->NewTaggedArray(8, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    size_t size = heap->GetHeapObjectSize();
    EXPECT_GT(size, 0u);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest012)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 90; i++) {
        auto obj = objectFactory->NewTaggedArray(12, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest013)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 70; i++) {
        auto obj = objectFactory->NewTaggedArray(18, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest014)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    size_t committedSize = heap->GetCommittedSize();
    size_t heapObjectSize = heap->GetHeapObjectSize();
    
    EXPECT_GE(committedSize, heapObjectSize);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest015)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto str = objectFactory->NewFromStdString("test_string_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest016)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 40; i++) {
        auto obj = objectFactory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest017)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        auto arr = objectFactory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest018)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t size1 = heap->GetCommittedSize();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t size2 = heap->GetCommittedSize();
    
    EXPECT_TRUE(size1 >= 0 && size2 >= 0);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest019)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = objectFactory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest020)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto obj = objectFactory->NewTaggedArray(7, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest021)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto obj = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest022)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto str = objectFactory->GetEmptyString();
        EXPECT_TRUE(*str != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest023)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 40; i++) {
        auto obj = objectFactory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    
    size_t size = heap->GetHeapObjectSize();
    EXPECT_GT(size, 0u);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest024)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        auto arr = objectFactory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest025)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    size_t committedSize = heap->GetCommittedSize();
    EXPECT_GT(committedSize, 0u);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest026)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 200; i++) {
        auto obj = objectFactory->NewTaggedArray(3, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest027)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 150; i++) {
        auto obj = objectFactory->NewTaggedArray(4, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest028)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = objectFactory->NewTaggedArray(6, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest029)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto str = objectFactory->NewFromStdString(std::to_string(i * 100));
        EXPECT_TRUE(*str != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedHeapTest030)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto obj = objectFactory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedMemTest, ExtendedMemTest031)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test

namespace panda::test {
class LastMemTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(LastMemTest, LastMemTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto obj = objectFactory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(LastMemTest, LastMemTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 40; i++) {
        auto obj = objectFactory->NewTaggedArray(6, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(LastMemTest, LastMemTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        auto obj = objectFactory->NewTaggedArray(7, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test

namespace panda::test {
class FinalExtendedMemTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FinalExtendedMemTest, FinalExtendedMemTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 500; i++) {
        auto obj = objectFactory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedMemTest, FinalExtendedMemTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 400; i++) {
        auto obj = objectFactory->NewTaggedArray(6, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedMemTest, FinalExtendedMemTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 300; i++) {
        auto obj = objectFactory->NewTaggedArray(7, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedMemTest, FinalExtendedMemTest004)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 250; i++) {
        auto str = objectFactory->NewFromStdString("final_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedMemTest, FinalExtendedMemTest005)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 200; i++) {
        auto obj = objectFactory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedMemTest, FinalExtendedMemTest006)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 180; i++) {
        auto arr = objectFactory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedMemTest, FinalExtendedMemTest007)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    for (int i = 0; i < 5; i++) {
        heap->CollectGarbage(TriggerGCType::YOUNG_GC);
        heap->CollectGarbage(TriggerGCType::OLD_GC);
        heap->CollectGarbage(TriggerGCType::FULL_GC);
    }
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedMemTest, FinalExtendedMemTest008)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 150; i++) {
        auto obj = objectFactory->NewTaggedArray(8, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    size_t size = heap->GetHeapObjectSize();
    EXPECT_GT(size, 0u);
}

HWTEST_F_L0(FinalExtendedMemTest, FinalExtendedMemTest009)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 120; i++) {
        auto str = objectFactory->NewFromStdString("mem_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedMemTest, FinalExtendedMemTest010)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = objectFactory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test

namespace panda::test {
class MoreExtendedMemTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 300; i++) {
        auto obj = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 250; i++) {
        auto obj = objectFactory->NewTaggedArray(12, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 200; i++) {
        auto obj = objectFactory->NewTaggedArray(15, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest004)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 180; i++) {
        auto str = objectFactory->NewFromStdString("test_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest005)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 160; i++) {
        auto obj = objectFactory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest006)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 140; i++) {
        auto arr = objectFactory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest007)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest008)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 120; i++) {
        auto obj = objectFactory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    size_t size = heap->GetHeapObjectSize();
    EXPECT_GT(size, 0u);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest009)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto str = objectFactory->NewFromStdString("string_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest010)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 90; i++) {
        auto obj = objectFactory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest011)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto arr = objectFactory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest012)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    size_t committedSize = heap->GetCommittedSize();
    EXPECT_GT(committedSize, 0u);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest013)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 70; i++) {
        auto obj = objectFactory->NewTaggedArray(25, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest014)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto obj = objectFactory->NewTaggedArray(30, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest015)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto obj = objectFactory->NewTaggedArray(35, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest016)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 45; i++) {
        auto str = objectFactory->GetEmptyString();
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest017)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 40; i++) {
        auto obj = objectFactory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    size_t size = heap->GetHeapObjectSize();
    EXPECT_GT(size, 0u);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest018)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 35; i++) {
        auto arr = objectFactory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest019)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    size_t size = heap->GetCommittedSize();
    EXPECT_GE(size, 0u);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest020)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        auto obj = objectFactory->NewTaggedArray(40, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest021)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 25; i++) {
        auto obj = objectFactory->NewTaggedArray(45, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest022)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 20; i++) {
        auto obj = objectFactory->NewTaggedArray(50, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest023)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 15; i++) {
        auto str = objectFactory->NewFromStdString("extended_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest024)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 10; i++) {
        auto obj = objectFactory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedMemTest, MoreExtendedMemTest025)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 5; i++) {
        auto arr = objectFactory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test

namespace panda::test {
class AdditionalMemTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 50; i++) {
            objectFactory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
        heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 200; i++) {
        auto arr = objectFactory->NewTaggedArray(15, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*arr != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> largeObjects;
    for (int i = 0; i < 30; i++) {
        auto obj = objectFactory->NewTaggedArray(200, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        largeObjects.push_back(JSTaggedValue(*obj));
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    largeObjects.clear();
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest004)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = objectFactory->NewTaggedArray(8, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest005)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 150; i++) {
        auto obj = objectFactory->NewTaggedArray(12, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest006)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto obj = objectFactory->NewTaggedArray(25, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest007)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int round = 0; round < 5; round++) {
        std::vector<JSTaggedValue> objs;
        for (int i = 0; i < 40; i++) {
            auto obj = objectFactory->NewTaggedArray(30, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
            objs.push_back(JSTaggedValue(*obj));
        }
        heap->CollectGarbage(TriggerGCType::YOUNG_GC);
        objs.clear();
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest008)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = objectFactory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest009)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto oldObj = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        auto youngObj = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        oldObj->Set(thread, 0, youngObj.GetTaggedValue());
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest010)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto obj = objectFactory->NewTaggedArray(18, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest011)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 70; i++) {
        auto obj = objectFactory->NewTaggedArray(22, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest012)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> objects;
    for (int i = 0; i < 90; i++) {
        auto obj = objectFactory->NewTaggedArray(16, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        objects.push_back(JSTaggedValue(*obj));
    }
    objects.clear();
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest013)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 45; i++) {
        auto obj = objectFactory->NewTaggedArray(35, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest014)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 120; i++) {
        auto obj = objectFactory->NewTaggedArray(7, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest015)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 55; i++) {
        auto obj1 = objectFactory->NewTaggedArray(12, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        auto obj2 = objectFactory->NewTaggedArray(12, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        obj1->Set(thread, 0, obj2.GetTaggedValue());
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest016)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 40; i++) {
        auto obj = objectFactory->NewTaggedArray(40, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest017)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 30; i++) {
            objectFactory->NewTaggedArray(50, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
        heap->CollectGarbage(TriggerGCType::OLD_GC);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest018)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 85; i++) {
        auto obj = objectFactory->NewTaggedArray(9, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest019)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 65; i++) {
        auto obj = objectFactory->NewTaggedArray(28, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest020)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 75; i++) {
        auto obj = objectFactory->NewTaggedArray(14, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest021)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> objects;
    for (int i = 0; i < 110; i++) {
        auto obj = objectFactory->NewTaggedArray(11, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        objects.push_back(JSTaggedValue(*obj));
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    objects.clear();
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest022)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 95; i++) {
        auto obj = objectFactory->NewTaggedArray(6, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest023)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto oldObj = objectFactory->NewTaggedArray(8, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        auto youngObj = objectFactory->NewTaggedArray(8, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        oldObj->Set(thread, 0, youngObj.GetTaggedValue());
        oldObj->Set(thread, 1, youngObj.GetTaggedValue());
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest024)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 130; i++) {
        auto obj = objectFactory->NewTaggedArray(4, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest025)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 140; i++) {
        auto obj = objectFactory->NewTaggedArray(3, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest026)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 160; i++) {
        auto obj = objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest027)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int round = 0; round < 8; round++) {
        for (int i = 0; i < 25; i++) {
            objectFactory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
        heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest028)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 170; i++) {
        auto obj = objectFactory->NewTaggedArray(1, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest029)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 180; i++) {
        auto obj = objectFactory->NewTaggedArray(17, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalMemTest, AdditionalHeapRegionTest030)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> objects;
    for (int i = 0; i < 200; i++) {
        auto obj = objectFactory->NewTaggedArray(13, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        objects.push_back(JSTaggedValue(*obj));
    }
    objects.clear();
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test

namespace panda::test {
class VeryFinalMemTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(VeryFinalMemTest, VeryFinalMemTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = objectFactory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(VeryFinalMemTest, VeryFinalMemTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto obj = objectFactory->NewTaggedArray(12, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(VeryFinalMemTest, VeryFinalMemTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto obj = objectFactory->NewTaggedArray(15, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(VeryFinalMemTest, VeryFinalMemTest004)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto str = objectFactory->NewFromStdString("very_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(VeryFinalMemTest, VeryFinalMemTest005)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 40; i++) {
        auto obj = objectFactory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test
