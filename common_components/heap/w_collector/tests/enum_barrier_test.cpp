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

#include "common_components/heap/heap.h"
#include "common_components/heap/w_collector/enum_barrier.h"
#include "common_components/tests/test_helper.h"

using namespace common;

namespace common::test {
class EnumBarrierTest : public common::test::BaseTestWithScope {
protected:
    void SetUp() override
    {
        Collector& collector = Heap::GetHeap().GetCollector();
        enumBarrier_ = std::make_unique<EnumBarrier>(collector);
    }

    void TearDown() override {}

    std::unique_ptr<EnumBarrier> enumBarrier_ {nullptr};
};

HWTEST_F_L0(EnumBarrierTest, ReadRefField_TEST1) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject* resultObj = enumBarrier_->ReadRefField(&obj, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(EnumBarrierTest, ReadRefField_TEST2) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject* resultObj = enumBarrier_->ReadRefField(nullptr, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(EnumBarrierTest, ReadStaticRef_TEST1) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject* resultObj = enumBarrier_->ReadStaticRef(field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(EnumBarrierTest, WriteRefField_TEST1) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

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
    enumBarrier_->WriteRefField(&oldObj, field, &newObj);
    enumBarrier_->WriteBarrier(&oldObj, field, &newObj);
    MAddress newAddress = field.GetFieldValue();
    EXPECT_NE(newAddress, oldAddress);
}

HWTEST_F_L0(EnumBarrierTest, ReadStruct_TEST1) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

    BaseObject obj;
    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    enumBarrier_->ReadStruct(dst, &obj, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(EnumBarrierTest, WriteStaticRef_TEST1) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

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
    enumBarrier_->WriteStaticRef(field, &newObj);
    MAddress newAddress = field.GetFieldValue();
    EXPECT_NE(newAddress, oldAddress);
}

HWTEST_F_L0(EnumBarrierTest, WriteStruct_TEST1) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

    auto objPtr = std::make_unique<BaseObject>();
    constexpr size_t size = 16;
    auto srcBuffer = std::make_unique<uint8_t[]>(size);
    auto dstBuffer = std::make_unique<uint8_t[]>(size);
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer.get());
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer.get());
    enumBarrier_->WriteStruct(objPtr.get(), dst, size, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(EnumBarrierTest, AtomicReadRefField_TEST1) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

    BaseObject obj;
    constexpr size_t size = 100;
    obj.SetSizeForwarded(size);
    EXPECT_EQ(obj.GetSizeForwarded(), size);
    RefField<true> field(&obj);

    BaseObject* resultObj = nullptr;
    resultObj = enumBarrier_->AtomicReadRefField(&obj, field, std::memory_order_seq_cst);
    ASSERT_TRUE(resultObj != nullptr);
}

HWTEST_F_L0(EnumBarrierTest, AtomicWriteRefField_TEST1) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

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

    enumBarrier_->AtomicWriteRefField(&oldObj, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(EnumBarrierTest, AtomicWriteRefField_TEST2) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

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

    enumBarrier_->AtomicWriteRefField(nullptr, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(EnumBarrierTest, AtomicSwapRefField_TEST1) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

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

    BaseObject* resultObj = nullptr;
    resultObj = enumBarrier_->AtomicSwapRefField(&oldObj, oldField, &newObj, std::memory_order_relaxed);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(oldField.GetFieldValue(), newField.GetFieldValue());
}

HWTEST_F_L0(EnumBarrierTest, CompareAndSwapRefField_TEST1) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

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

    bool result = enumBarrier_->CompareAndSwapRefField(&oldObj, oldField, &oldObj, &newObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_FALSE(result);
}

HWTEST_F_L0(EnumBarrierTest, CompareAndSwapRefField_TEST2) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<true> oldField(&oldObj);

    bool result = enumBarrier_->CompareAndSwapRefField(&oldObj, oldField, &oldObj, &oldObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(EnumBarrierTest, CompareAndSwapRefField_TEST3) {
    ASSERT_TRUE(enumBarrier_ != nullptr);

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

    bool result = enumBarrier_->CompareAndSwapRefField(&oldObj, newField, &oldObj, &newObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_FALSE(result);
}

} // namespace common::test