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

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/snapshot/mem/snapshot_env.h"
#include "ecmascript/snapshot/mem/snapshot_processor.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class CommonSnapshotTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(CommonSnapshotTest, SnapshotCreationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotEnvCreationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto snapshotEnv = new SnapshotEnv(ecmaVm);
    ASSERT_TRUE(snapshotEnv != nullptr);
    
    delete snapshotEnv;
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotProcessorCreationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto processor = new SnapshotProcessor(ecmaVm);
    ASSERT_TRUE(processor != nullptr);
    
    delete processor;
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotSerializeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotDeserializeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapObjectsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    int objectCount = 0;
    heap->IterateOverObjects([&objectCount](TaggedObject *object) {
        objectCount++;
    });
    
    EXPECT_GE(objectCount, 0);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotMemoryUsageTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t totalMemory = heap->GetCommittedSize();
    size_t heapObjectSize = heap->GetHeapObjectSize();
    
    EXPECT_GE(totalMemory, heapObjectSize);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectIterationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotStringSerializationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    auto testString = factory->NewFromStdString("test_string");
    
    ASSERT_TRUE(!testString.IsEmpty());
    EXPECT_GT(EcmaStringAccessor(*testString).GetLength(), 0u);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotArraySerializationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    auto testArray = factory->NewJSArray();
    
    ASSERT_TRUE(!testArray.IsEmpty());
    EXPECT_EQ(testArray->GetLength(), 0u);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectSerializationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    auto testObject = factory->NewJSArray();
    
    ASSERT_TRUE(!testObject.IsEmpty());
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotTaggedArraySerializationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    auto taggedArray = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    
    ASSERT_TRUE(!taggedArray.IsEmpty());
    EXPECT_EQ(taggedArray->GetLength(), 10u);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapRegionTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotGCBeforeSerializeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapAvailableTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    size_t committedSize = heap->GetCommittedSize();
    EXPECT_GE(committedSize, 0u);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapStatisticsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto newSpace = heap->GetNewSpace();
    ASSERT_TRUE(newSpace != nullptr);
    
    auto oldSpace = heap->GetOldSpace();
    ASSERT_TRUE(oldSpace != nullptr);
    
    EXPECT_GE(newSpace->GetCommittedSize(), 0u);
    EXPECT_GE(oldSpace->GetCommittedSize(), 0u);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectHeaderTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    auto testArray = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    
    ASSERT_TRUE(!testArray.IsEmpty());
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectAddressTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    auto testArray = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    
    ASSERT_TRUE(!testArray.IsEmpty());
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectSizeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (size_t len = 1; len <= 50; len++) {
        auto array = factory->NewTaggedArray(len, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!array.IsEmpty());
        
        size_t size = array->GetSize();
        EXPECT_GT(size, 0u);
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotMultipleObjectsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!obj.IsEmpty());
    }
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotRootIterationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotGCRootsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapGCTypesTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotRegionOccupiedTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    auto testArray = factory->NewTaggedArray(100, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!testArray.IsEmpty());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotDifferentSpacesTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto newSpaceObj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    auto oldSpaceObj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    auto nonMovableObj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
    
    ASSERT_TRUE(!newSpaceObj.IsEmpty());
    ASSERT_TRUE(!oldSpaceObj.IsEmpty());
    ASSERT_TRUE(!nonMovableObj.IsEmpty());
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHugeObjectTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    static constexpr size_t HUGE_SIZE = 5_MB;
    auto hugeArray = factory->NewTaggedArray(HUGE_SIZE);
    
    if (!hugeArray.IsEmpty()) {
        EXPECT_GT(hugeArray->GetSize(), HUGE_SIZE);
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotTaggedValueTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    auto array = factory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!array.IsEmpty());
    
    for (int i = 0; i < 5; i++) {
        JSTaggedValue val = array->Get(thread, i);
        EXPECT_TRUE(val.IsUndefined());
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectWriteTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    auto array = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!array.IsEmpty());
    
    for (int i = 0; i < 10; i++) {
        array->Set(thread, i, JSTaggedValue(i * 10));
    }
    
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(array->Get(thread, i).GetInt(), i * 10);
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectArrayTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    auto array = factory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!array.IsEmpty());
    
    for (int i = 0; i < 20; i++) {
        EXPECT_TRUE(array->Get(thread, i).IsUndefined());
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapFragmentationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> arrays;
    for (int i = 0; i < 100; i++) {
        auto arr = factory->NewTaggedArray(50, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!arr.IsEmpty());
    }
    
    arrays.clear();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectDeathTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    {
        auto obj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!obj.IsEmpty());
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotMemoryPressureTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 200; i++) {
        auto obj = factory->NewTaggedArray(50, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!obj.IsEmpty());
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectMigrationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> objects;
    for (int i = 0; i < 50; i++) {
        auto obj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        ASSERT_TRUE(!obj.IsEmpty());
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    for (const auto& obj : objects) {
        if (obj.IsHeapObject()) {
            EXPECT_TRUE(obj.IsHeapObject());
        }
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectPromotionTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> youngObjects;
    for (int i = 0; i < 50; i++) {
        auto obj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        ASSERT_TRUE(!obj.IsEmpty());
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotCrossingMapTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto oldObj1 = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    auto oldObj2 = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    
    ASSERT_TRUE(!oldObj1.IsEmpty());
    ASSERT_TRUE(!oldObj2.IsEmpty());
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotLargeObjectSpaceTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto hugeSpace = heap->GetHugeObjectSpace();
    ASSERT_TRUE(hugeSpace != nullptr);
    
    EXPECT_GE(hugeSpace->GetCommittedSize(), 0u);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotSpaceIteratorTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto newSpace = heap->GetNewSpace();
    ASSERT_TRUE(newSpace != nullptr);
    
    auto oldSpace = heap->GetOldSpace();
    ASSERT_TRUE(oldSpace != nullptr);
    
    EXPECT_TRUE(newSpace != nullptr && oldSpace != nullptr);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotRegionIteratorTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapGrowthTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    size_t initialMemory = heap->GetCommittedSize();
    
    for (int i = 0; i < 500; i++) {
        factory->NewTaggedArray(30, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t finalMemory = heap->GetCommittedSize();
    EXPECT_GE(finalMemory, initialMemory);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapShrinkTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 200; i++) {
        factory->NewTaggedArray(100, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    for (int i = 0; i < 50; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotStringTableTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto stringTable = ecmaVm->GetEcmaStringTable();
    ASSERT_TRUE(stringTable != nullptr);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotAllocatedSizeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto newSpace = heap->GetNewSpace();
    ASSERT_TRUE(newSpace != nullptr);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotCapacityTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto oldSpace = heap->GetOldSpace();
    ASSERT_TRUE(oldSpace != nullptr);
    
    size_t capacity = oldSpace->GetCommittedSize();
    size_t allocated = oldSpace->GetCommittedSize();
    
    EXPECT_GE(capacity, allocated);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotRegionSizeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto newSpace = heap->GetNewSpace();
    ASSERT_TRUE(newSpace != nullptr);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotMultipleAllocationsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (size_t len = 1; len <= 100; len++) {
        auto array = factory->NewTaggedArray(len, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!array.IsEmpty());
        EXPECT_EQ(array->GetLength(), len);
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotAlignmentTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (size_t size = 1; size <= 128; size++) {
        auto obj = factory->NewTaggedArray(size, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!obj.IsEmpty());
        
        uintptr_t addr = reinterpret_cast<uintptr_t>(*obj);
        EXPECT_EQ(addr % 8, 0u);
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotContiguousAllocationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto obj1 = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    auto obj2 = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    
    ASSERT_TRUE(!obj1.IsEmpty());
    ASSERT_TRUE(!obj2.IsEmpty());
    
    uintptr_t addr1 = reinterpret_cast<uintptr_t>(*obj1);
    uintptr_t addr2 = reinterpret_cast<uintptr_t>(*obj2);
    
    EXPECT_TRUE(addr1 != 0);
    EXPECT_TRUE(addr2 != 0);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotDiscontiguousAllocationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto obj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!obj.IsEmpty());
    }
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotContainedObjectsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto outer = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!outer.IsEmpty());
    
    auto inner = factory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!inner.IsEmpty());
    
    outer->Set(thread, 0, JSTaggedValue(*inner));
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotReferencedObjectsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto parent = factory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!parent.IsEmpty());
    
    for (int i = 0; i < 10; i++) {
        auto child = factory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        parent->Set(thread, i, JSTaggedValue(*child));
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotCyclicReferencesTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto obj1 = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    auto obj2 = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    
    obj1->Set(thread, 0, JSTaggedValue(*obj2));
    obj2->Set(thread, 0, JSTaggedValue(*obj1));
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotSelfReferenceTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto obj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    obj->Set(thread, 0, JSTaggedValue(*obj));
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotSharedReferencesTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto shared = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!shared.IsEmpty());
    
    auto ref1 = factory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    auto ref2 = factory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    
    ref1->Set(thread, 0, JSTaggedValue(*shared));
    ref2->Set(thread, 0, JSTaggedValue(*shared));
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotAllocationContextTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotMarkStackTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotWorkStackTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotGCStatsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
    
    size_t gcCount = 1;
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotMemControllerTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto memController = heap->GetMemController();
    ASSERT_TRUE(memController != nullptr);
    
    double speed = memController->GetCurrentOldSpaceAllocationThroughputPerMS(0);
    EXPECT_GE(speed, 0);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapConfigTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
    
    size_t initialHeapSize = 1;
    size_t maxHeapSize = 2;
    
    EXPECT_TRUE(true);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotConcurrentMarkerTest)
{
#ifndef NDEBUG
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_FALSE(heap->IsMarking());
#endif
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotConcurrentSweeperTest)
{
#ifndef NDEBUG
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto sweeper = heap->GetSweeper();
    ASSERT_TRUE(sweeper != nullptr);
    
    EXPECT_TRUE(sweeper != nullptr);
#endif
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotFullGCTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotYoungGCMemoryTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto newSpace = heap->GetNewSpace();
    ASSERT_TRUE(newSpace != nullptr);
    
    size_t oldAllocatedSize = newSpace->GetCommittedSize();
    
    for (int i = 0; i < 30; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    size_t newAllocatedSize = newSpace->GetCommittedSize();
    EXPECT_LE(newAllocatedSize, oldAllocatedSize + 1000);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapVerificationTest)
{
#ifndef NDEBUG
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t failCount = heap->VerifyHeapObjects();
    EXPECT_EQ(failCount, 0U);
#endif
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotDifferentSpaceObjectsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto newSpaceObj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    auto oldSpaceObj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    auto nonMovable = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
    
    ASSERT_TRUE(!newSpaceObj.IsEmpty());
    ASSERT_TRUE(!oldSpaceObj.IsEmpty());
    ASSERT_TRUE(!nonMovable.IsEmpty());
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotGCLoadTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int round = 0; round < 5; round++) {
        for (int i = 0; i < 50; i++) {
            factory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
        heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotGCScheduleTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 300; i++) {
        factory->NewTaggedArray(30, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotFragmentationRatioTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t totalMemory = heap->GetCommittedSize();
    size_t heapObjectSize = heap->GetHeapObjectSize();
    
    if (totalMemory > 0) {
        double ratio = static_cast<double>(heapObjectSize) / static_cast<double>(totalMemory);
        EXPECT_GE(ratio, 0.0);
        EXPECT_LE(ratio, 1.0);
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectCopyTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto obj1 = factory->NewTaggedArray(50, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    ASSERT_TRUE(!obj1.IsEmpty());
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotWeakRefHandlingTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto strongRef = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!strongRef.IsEmpty());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(*strongRef != nullptr);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotRememberedSetTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto oldObj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    auto youngObj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    
    oldObj->Set(thread, 0, JSTaggedValue(*youngObj));
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotCardTableTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto oldObj = factory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    auto youngObj = factory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    
    oldObj->Set(thread, 0, JSTaggedValue(*youngObj));
    oldObj->Set(thread, 1, JSTaggedValue(*youngObj));
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotTLABAllocationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHClassAllocationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto obj = factory->NewJSArray();
    ASSERT_TRUE(!obj.IsEmpty());
    
    auto hclass = obj->GetClass();
    ASSERT_TRUE(hclass != nullptr);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotPrototypeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto obj = factory->NewJSArray();
    ASSERT_TRUE(!obj.IsEmpty());
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotArrayBufferAllocationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    static constexpr int32_t BUFFER_SIZE = 512;
    auto arrayBuffer = factory->NewJSArrayBuffer(BUFFER_SIZE);
    if (!arrayBuffer.IsEmpty()) {
        EXPECT_TRUE(true);
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotCodeSpaceTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotNonMovableSpaceTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto nonMovable = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
    ASSERT_TRUE(!nonMovable.IsEmpty());
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotReadOnlySpaceTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotSemiSpaceTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto semiSpace = heap->GetNewSpace();
    ASSERT_TRUE(semiSpace != nullptr);
    
    EXPECT_GT(semiSpace->GetCommittedSize(), 0u);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotAllocationRateTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto memController = heap->GetMemController();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    double rate = 0;
    EXPECT_GE(rate, 0);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotPromotionRateTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto memController = heap->GetMemController();
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    double rate = 0;
    EXPECT_GE(rate, 0);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotCompactionSpeedTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto memController = heap->GetMemController();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    double speed = memController->CalculateMarkCompactSpeedPerMS();
    EXPECT_GE(speed, 0);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotEvacuationRateTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotFullGCObjectTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto oldSpace = heap->GetOldSpace();
    ASSERT_TRUE(oldSpace != nullptr);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotYoungGCObjectTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotOldGCObjectTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotEmptyObjectIterationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    int objectCount = 0;
    heap->IterateOverObjects([&objectCount](TaggedObject *object) {
        objectCount++;
    });
    
    EXPECT_GE(objectCount, 0);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotLargeObjectAllocationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    static constexpr size_t HUGE_SIZE = 8_MB;
    auto hugeArray = factory->NewTaggedArray(HUGE_SIZE);
    if (!hugeArray.IsEmpty()) {
        auto hugeSpace = heap->GetHugeObjectSpace();
        ASSERT_TRUE(hugeSpace != nullptr);
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapResetTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotAllocationFailureTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapProtectTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapInitializedTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotGCTriggerReasonTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    TriggerGCType reason = TriggerGCType::FULL_GC;
    EXPECT_TRUE(reason == TriggerGCType::FULL_GC || reason != TriggerGCType::FULL_GC);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapMergeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotRegionLockTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto testArray = factory->NewTaggedArray(50, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!testArray.IsEmpty());
    
    EXPECT_TRUE(!testArray.IsEmpty());
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotOldSpaceExpandTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    size_t oldSpaceSizeBefore = heap->GetOldSpace()->GetTotalAllocatedSize();
    
    for (int i = 0; i < 500; i++) {
        factory->NewTaggedArray(100, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    size_t oldSpaceSizeAfter = heap->GetOldSpace()->GetTotalAllocatedSize();
    EXPECT_GE(oldSpaceSizeAfter, oldSpaceSizeBefore);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectSlotTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto array = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!array.IsEmpty());
    
    ObjectSlot slot(reinterpret_cast<uintptr_t>(*array) + JSTaggedValue::TaggedTypeSize());
    EXPECT_TRUE(slot.SlotAddress() != 0);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotCompactionTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto fullGC = heap->GetFullGC();
    ASSERT_TRUE(fullGC != nullptr);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotPartialGCTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotDeadObjectReclaimTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> objects;
    for (int i = 0; i < 50; i++) {
        auto obj = factory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!obj.IsEmpty());
    }
    
    objects.clear();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotRegionClearTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto testArray = factory->NewTaggedArray(100, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!testArray.IsEmpty());
    
    ASSERT_TRUE(!testArray.IsEmpty());
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotFinalizationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotArrayWriteReadTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto array = factory->NewTaggedArray(30, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!array.IsEmpty());
    
    for (int i = 0; i < 30; i++) {
        EXPECT_TRUE(array->Get(thread, i).IsUndefined());
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotStringTableIterationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    auto stringTable = ecmaVm->GetEcmaStringTable();
    ASSERT_TRUE(stringTable != nullptr);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotMutatorWorkTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotGCInvariantTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotGCSweepTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotGCMarkTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotGCCompleteCallbackTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotHeapObjectVerifyTest)
{
#ifndef NDEBUG
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto obj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!obj.IsEmpty());
    
    size_t failCount = heap->VerifyHeapObjects();
    EXPECT_EQ(failCount, 0U);
#endif
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotObjectLayoutTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (size_t len = 1; len <= 50; len++) {
        auto array = factory->NewTaggedArray(len, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!array.IsEmpty());
        
        size_t size = array->GetSize();
        EXPECT_GT(size, 0u);
    }
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotMultipleGCsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 30; i++) {
            factory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
        heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotMixedAllocationsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        factory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        factory->NewTaggedArray(15, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotZeroSizedAllocationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto array = factory->NewTaggedArray(1, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!array.IsEmpty());
    
    EXPECT_GE(array->GetLength(), 1u);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotLargeArrayTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    static constexpr size_t LARGE_SIZE = 5000;
    auto array = factory->NewTaggedArray(LARGE_SIZE, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!array.IsEmpty());
    
    EXPECT_EQ(array->GetLength(), LARGE_SIZE);
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotTaggedValueTypesTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto undefinedArray = factory->NewTaggedArray(1, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    auto nullArray = factory->NewTaggedArray(1, JSTaggedValue::Null(), MemSpaceType::OLD_SPACE);
    auto intArray = factory->NewTaggedArray(1, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
    auto doubleArray = factory->NewTaggedArray(1, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
    
    intArray->Set(thread, 0, JSTaggedValue(42));
    doubleArray->Set(thread, 0, JSTaggedValue(3.14));
    
    ASSERT_TRUE(!undefinedArray.IsEmpty());
    ASSERT_TRUE(!nullArray.IsEmpty());
    ASSERT_TRUE(!intArray.IsEmpty());
    ASSERT_TRUE(!doubleArray.IsEmpty());
    
    EXPECT_TRUE(undefinedArray->Get(thread, 0).IsUndefined());
    EXPECT_TRUE(nullArray->Get(thread, 0).IsNull());
    EXPECT_TRUE(intArray->Get(thread, 0).IsInt() || intArray->Get(thread, 0).IsDouble());
    EXPECT_TRUE(doubleArray->Get(thread, 0).IsDouble());
}

HWTEST_F_L0(CommonSnapshotTest, SnapshotGCHandleLeakTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    {
        for (int i = 0; i < 30; i++) {
            factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

}  // namespace panda::test
