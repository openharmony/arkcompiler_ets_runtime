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

#include "common_components/heap/w_collector/copy_barrier.h"
#include "common_components/heap/heap.h"
#include "common_components/tests/test_helper.h"

using namespace common;

namespace common::test {
class CopyBarrierTest : public common::test::BaseTestWithScope {
};

HWTEST_F_L0(CopyBarrierTest, ReadStruct_TEST1) {
    Collector& collector = Heap::GetHeap().GetCollector();
    auto copyBarrier = std::make_unique<CopyBarrier>(collector);
    ASSERT_TRUE(copyBarrier != nullptr);

    BaseObject obj;
    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    copyBarrier->ReadStruct(dst, &obj, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(CopyBarrierTest, ReadStruct_TEST2) {
    Collector& collector = Heap::GetHeap().GetCollector();
    auto copyBarrier = std::make_unique<CopyBarrier>(collector);
    ASSERT_TRUE(copyBarrier != nullptr);

    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    copyBarrier->ReadStruct(dst, nullptr, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(CopyBarrierTest, AtomicWriteRefField_TEST1) {
    Collector& collector = Heap::GetHeap().GetCollector();
    auto copyBarrier = std::make_unique<CopyBarrier>(collector);
    ASSERT_TRUE(copyBarrier != nullptr);

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

    copyBarrier->AtomicWriteRefField(&oldObj, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(CopyBarrierTest, AtomicWriteRefField_TEST2) {
    Collector& collector = Heap::GetHeap().GetCollector();
    auto copyBarrier = std::make_unique<CopyBarrier>(collector);
    ASSERT_TRUE(copyBarrier != nullptr);

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

    copyBarrier->AtomicWriteRefField(nullptr, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(CopyBarrierTest, ReadRefField_TEST1) {
    Collector& collector = Heap::GetHeap().GetCollector();
    auto copyBarrier = std::make_unique<CopyBarrier>(collector);
    ASSERT_TRUE(copyBarrier != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject* resultObj = copyBarrier->ReadRefField(&obj, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(CopyBarrierTest, ReadRefField_TEST2) {
    Collector& collector = Heap::GetHeap().GetCollector();
    auto copyBarrier = std::make_unique<CopyBarrier>(collector);
    ASSERT_TRUE(copyBarrier != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject* resultObj = copyBarrier->ReadRefField(nullptr, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(CopyBarrierTest, ReadStaticRef_TEST1) {
    Collector& collector = Heap::GetHeap().GetCollector();
    auto copyBarrier = std::make_unique<CopyBarrier>(collector);
    ASSERT_TRUE(copyBarrier != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject* resultObj = copyBarrier->ReadStaticRef(field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(CopyBarrierTest, AtomicReadRefField_TEST1) {
    Collector& collector = Heap::GetHeap().GetCollector();
    auto copyBarrier = std::make_unique<CopyBarrier>(collector);
    ASSERT_TRUE(copyBarrier != nullptr);

    BaseObject obj;
    RefField<true> field(&obj);

    BaseObject* resultObj = nullptr;
    resultObj = copyBarrier->AtomicReadRefField(&obj, field, std::memory_order_seq_cst);
    ASSERT_TRUE(resultObj != nullptr);
}
}  // namespace common::test