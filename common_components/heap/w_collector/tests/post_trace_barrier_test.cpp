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
#include "common_components/heap/w_collector/post_trace_barrier.h"
#include "common_components/tests/test_helper.h"

using namespace common;

namespace common::test {
class PostTraceBarrierTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Collector &collector = Heap::GetHeap().GetCollector();
        postTraceBarrier_ = std::make_unique<PostTraceBarrier>(collector);
    }

    void TearDown() override
    {}

    std::unique_ptr<PostTraceBarrier> postTraceBarrier_{nullptr};
};

HWTEST_F_L0(PostTraceBarrierTest, ReadRefField_TEST1)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject *resultObj = postTraceBarrier_->ReadRefField(&obj, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(PostTraceBarrierTest, ReadRefField_TEST2)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject *resultObj = postTraceBarrier_->ReadRefField(nullptr, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(PostTraceBarrierTest, ReadStaticRef_TEST1)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject *resultObj = postTraceBarrier_->ReadStaticRef(field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(PostTraceBarrierTest, WriteRefField_TEST1)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

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
    postTraceBarrier_->WriteRefField(&oldObj, field, &newObj);
    postTraceBarrier_->WriteBarrier(&oldObj, field, &newObj);
    MAddress newAddress = field.GetFieldValue();
    EXPECT_NE(newAddress, oldAddress);
}

HWTEST_F_L0(PostTraceBarrierTest, ReadStruct_TEST1)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

    BaseObject obj;
    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    postTraceBarrier_->ReadStruct(dst, &obj, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(PostTraceBarrierTest, WriteStaticRef_TEST1)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

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
    postTraceBarrier_->WriteStaticRef(field, &newObj);
    MAddress newAddress = field.GetFieldValue();
    EXPECT_NE(newAddress, oldAddress);
}

HWTEST_F_L0(PostTraceBarrierTest, WriteStruct_TEST1)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

    BaseObject obj;
    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    postTraceBarrier_->WriteStruct(&obj, dst, size, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(PostTraceBarrierTest, AtomicReadRefField_TEST1)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

    BaseObject obj;
    constexpr size_t size = 100;
    obj.SetSizeForwarded(size);
    EXPECT_EQ(obj.GetSizeForwarded(), size);
    RefField<true> field(&obj);

    BaseObject *resultObj = nullptr;
    resultObj = postTraceBarrier_->AtomicReadRefField(&obj, field, std::memory_order_seq_cst);
    ASSERT_TRUE(resultObj != nullptr);
}

HWTEST_F_L0(PostTraceBarrierTest, AtomicWriteRefField_TEST1)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

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

    postTraceBarrier_->AtomicWriteRefField(&oldObj, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(PostTraceBarrierTest, AtomicWriteRefField_TEST2)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

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

    postTraceBarrier_->AtomicWriteRefField(nullptr, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(PostTraceBarrierTest, AtomicSwapRefField_TEST1)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

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

    BaseObject *resultObj = nullptr;
    resultObj = postTraceBarrier_->AtomicSwapRefField(&oldObj, oldField, &newObj, std::memory_order_relaxed);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(oldField.GetFieldValue(), newField.GetFieldValue());
}

HWTEST_F_L0(PostTraceBarrierTest, CompareAndSwapRefField_TEST1)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

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

    bool result = postTraceBarrier_->CompareAndSwapRefField(
        &oldObj, oldField, &oldObj, &newObj, std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PostTraceBarrierTest, CompareAndSwapRefField_TEST2)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<true> oldField(&oldObj);

    bool result = postTraceBarrier_->CompareAndSwapRefField(
        &oldObj, oldField, &oldObj, &oldObj, std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PostTraceBarrierTest, CompareAndSwapRefField_TEST3)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

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

    bool result = postTraceBarrier_->CompareAndSwapRefField(
        &oldObj, newField, &oldObj, &newObj, std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_FALSE(result);
}

HWTEST_F_L0(PostTraceBarrierTest, CopyStructArray_TEST1)
{
    ASSERT_TRUE(postTraceBarrier_ != nullptr);

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

    postTraceBarrier_->CopyStructArray(&oldObj, dst, size, &newObj, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}
}  // namespace common::test