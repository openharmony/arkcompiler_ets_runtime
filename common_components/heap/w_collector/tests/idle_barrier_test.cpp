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

#include "common_components/heap/w_collector/idle_barrier.h"
#include "common_components/heap/heap.h"
#include "common_components/tests/test_helper.h"

using namespace common;

namespace common::test {
class IdleBarrierTest : public common::test::BaseTestWithScope {
};

HWTEST_F_L0(IdleBarrierTest, ReadStruct_TEST0)
{
    Collector& collector = Heap::GetHeap().GetCollector();
    auto idleBarrier = std::make_unique<IdleBarrier>(collector);
    ASSERT_TRUE(idleBarrier != nullptr);

    BaseObject obj;
    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    idleBarrier->ReadStruct(dst, &obj, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(IdleBarrierTest, AtomicWriteRefField_TEST0)
{
    Collector& collector = Heap::GetHeap().GetCollector();
    auto idleBarrier = std::make_unique<IdleBarrier>(collector);
    ASSERT_TRUE(idleBarrier != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<true> oldField(&oldObj);
    MAddress oldAddress = oldField.GetFieldValue();

    BaseObject newObj;
    constexpr size_t newSize = 200;
    newObj.SetSizeForwarded(newSize);
    EXPECT_EQ(newObj.GetSizeForwarded(), newSize);
    RefField<true> newField(&newObj);
    MAddress neWAddress = newField.GetFieldValue();
    EXPECT_NE(oldAddress, neWAddress);

    idleBarrier->AtomicWriteRefField(&oldObj, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(IdleBarrierTest, AtomicWriteRefField_TEST1) {
    Collector& collector = Heap::GetHeap().GetCollector();
    auto idleBarrier = std::make_unique<IdleBarrier>(collector);
    ASSERT_TRUE(idleBarrier != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<true> oldField(&oldObj);
    MAddress oldAddress = oldField.GetFieldValue();

    BaseObject newObj;
    constexpr size_t newSize = 200;
    newObj.SetSizeForwarded(newSize);
    EXPECT_EQ(newObj.GetSizeForwarded(), newSize);
    RefField<true> newField(&newObj);
    MAddress neWAddress = newField.GetFieldValue();
    EXPECT_NE(oldAddress, neWAddress);

    idleBarrier->AtomicWriteRefField(nullptr, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(IdleBarrierTest, AtomicSwapRefField_TEST0)
{
    Collector& collector = Heap::GetHeap().GetCollector();
    auto idleBarrier = std::make_unique<IdleBarrier>(collector);
    ASSERT_TRUE(idleBarrier != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<true> oldField(&oldObj);
    MAddress oldAddress = oldField.GetFieldValue();

    BaseObject newObj;
    constexpr size_t newSize = 200;
    newObj.SetSizeForwarded(newSize);
    EXPECT_EQ(newObj.GetSizeForwarded(), newSize);
    RefField<true> newField(&newObj);
    MAddress neWAddress = newField.GetFieldValue();
    EXPECT_NE(oldAddress, neWAddress);

    BaseObject *retObj = idleBarrier->AtomicSwapRefField(&oldObj, oldField, &newObj, std::memory_order_relaxed);
    ASSERT_TRUE(retObj != nullptr);
    EXPECT_EQ(oldField.GetFieldValue(), newField.GetFieldValue());
}

HWTEST_F_L0(IdleBarrierTest, CompareAndSwapRefField_TEST0)
{
    Collector& collector = Heap::GetHeap().GetCollector();
    auto idleBarrier = std::make_unique<IdleBarrier>(collector);
    ASSERT_TRUE(idleBarrier != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<true> oldField(&oldObj);
    MAddress oldAddress = oldField.GetFieldValue();

    BaseObject newObj;
    constexpr size_t newSize = 200;
    newObj.SetSizeForwarded(newSize);
    EXPECT_EQ(newObj.GetSizeForwarded(), newSize);
    RefField<true> newField(&newObj);
    MAddress neWAddress = newField.GetFieldValue();
    EXPECT_NE(oldAddress, neWAddress);

    bool result = idleBarrier->CompareAndSwapRefField(&oldObj, oldField, &oldObj, &newObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_FALSE(result);
}

HWTEST_F_L0(IdleBarrierTest, CompareAndSwapRefField_TEST1)
{
    Collector& collector = Heap::GetHeap().GetCollector();
    auto idleBarrier = std::make_unique<IdleBarrier>(collector);
    ASSERT_TRUE(idleBarrier != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<true> oldField(&oldObj);

    bool result = idleBarrier->CompareAndSwapRefField(&oldObj, oldField, &oldObj, &oldObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(IdleBarrierTest, CompareAndSwapRefField_TEST2)
{
    Collector& collector = Heap::GetHeap().GetCollector();
    auto idleBarrier = std::make_unique<IdleBarrier>(collector);
    ASSERT_TRUE(idleBarrier != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<true> oldField(&oldObj);
    MAddress oldAddress = oldField.GetFieldValue();

    BaseObject newObj;
    constexpr size_t newSize = 200;
    newObj.SetSizeForwarded(newSize);
    EXPECT_EQ(newObj.GetSizeForwarded(), newSize);
    RefField<true> newField(&newObj);
    MAddress neWAddress = newField.GetFieldValue();
    EXPECT_NE(oldAddress, neWAddress);

    bool result = idleBarrier->CompareAndSwapRefField(&oldObj, newField, &oldObj, &newObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_FALSE(result);
}

HWTEST_F_L0(IdleBarrierTest, WriteRefField_TEST0)
{
    Collector& collector = Heap::GetHeap().GetCollector();
    auto idleBarrier = std::make_unique<IdleBarrier>(collector);
    ASSERT_TRUE(idleBarrier != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<false> oldField(&oldObj);
    MAddress oldAddress = oldField.GetFieldValue();

    BaseObject newObj;
    constexpr size_t newSize = 200;
    newObj.SetSizeForwarded(newSize);
    EXPECT_EQ(newObj.GetSizeForwarded(), newSize);
    RefField<false> newField(&newObj);
    MAddress neWAddress = newField.GetFieldValue();
    EXPECT_NE(oldAddress, neWAddress);

    idleBarrier->WriteRefField(&oldObj, oldField, &newObj);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(IdleBarrierTest, WriteStruct_TEST0)
{
    Collector& collector = Heap::GetHeap().GetCollector();
    auto idleBarrier = std::make_unique<IdleBarrier>(collector);
    ASSERT_TRUE(idleBarrier != nullptr);

    BaseObject obj;
    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);

    idleBarrier->WriteStruct(&obj, dst, size, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(IdleBarrierTest, WriteStaticRef_TEST0)
{
    Collector& collector = Heap::GetHeap().GetCollector();
    auto idleBarrier = std::make_unique<IdleBarrier>(collector);
    ASSERT_TRUE(idleBarrier != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<false> field(&oldObj);
    MAddress oldAddress = field.GetFieldValue();

    BaseObject newObj;
    constexpr size_t newSize = 200;
    newObj.SetSizeForwarded(newSize);
    EXPECT_EQ(newObj.GetSizeForwarded(), newSize);
    idleBarrier->WriteStaticRef(field, &newObj);
    MAddress newAddress = field.GetFieldValue();
    EXPECT_NE(newAddress, oldAddress);
}

HWTEST_F_L0(IdleBarrierTest, CopyStructArray_TEST0)
{
    Collector& collector = Heap::GetHeap().GetCollector();
    auto idleBarrier = std::make_unique<IdleBarrier>(collector);
    ASSERT_TRUE(idleBarrier != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);

    BaseObject newObj;
    constexpr size_t newSize = 200;
    newObj.SetSizeForwarded(newSize);
    EXPECT_EQ(newObj.GetSizeForwarded(), newSize);

    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);

    idleBarrier->CopyStructArray(&oldObj, dst, size, &newObj, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}
}  // namespace common::test