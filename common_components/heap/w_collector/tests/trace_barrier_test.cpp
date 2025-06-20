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
#include "common_components/heap/w_collector/trace_barrier.h"
#include "common_components/tests/test_helper.h"

using namespace common;

namespace common::test {
class TraceBarrierTest : public BaseTestWithScope {
protected:
    void SetUp() override
    {
        Collector &collector = Heap::GetHeap().GetCollector();
        traceBarrier_ = std::make_unique<TraceBarrier>(collector);
    }

    void TearDown() override
    {}

    std::unique_ptr<TraceBarrier> traceBarrier_{nullptr};
};

HWTEST_F_L0(TraceBarrierTest, ReadRefField_TEST1)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject *resultObj = traceBarrier_->ReadRefField(&obj, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(TraceBarrierTest, ReadRefField_TEST2)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject *resultObj = traceBarrier_->ReadRefField(nullptr, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(TraceBarrierTest, ReadStaticRef_TEST1)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

    BaseObject obj;
    RefField<false> field(&obj);

    BaseObject *resultObj = traceBarrier_->ReadStaticRef(field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, &obj);
}

HWTEST_F_L0(TraceBarrierTest, ReadStruct_TEST1)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

    BaseObject obj;
    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    traceBarrier_->ReadStruct(dst, &obj, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(TraceBarrierTest, WriteRefField_TEST1)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<false> field(&oldObj);
    BaseObject *target = field.GetTargetObject();
    EXPECT_TRUE(target != nullptr);
    MAddress oldAddress = field.GetFieldValue();

    BaseObject newObj;
    constexpr size_t newSize = 200;
    newObj.SetSizeForwarded(newSize);
    EXPECT_EQ(newObj.GetSizeForwarded(), newSize);
    traceBarrier_->WriteRefField(&oldObj, field, &newObj);
    MAddress newAddress = field.GetFieldValue();
    EXPECT_NE(newAddress, oldAddress);
}

HWTEST_F_L0(TraceBarrierTest, WriteRefField_TEST2)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<> field(MAddress(0));
    BaseObject *target = field.GetTargetObject();
    EXPECT_TRUE(target == nullptr);

    BaseObject newObj;
    constexpr size_t newSize = 200;
    newObj.SetSizeForwarded(newSize);
    EXPECT_EQ(newObj.GetSizeForwarded(), newSize);
    RefField<false> newField(&newObj);
    traceBarrier_->WriteRefField(&oldObj, field, &newObj);
    MAddress newAddress = field.GetFieldValue();
    EXPECT_EQ(newField.GetFieldValue(), newAddress);
}

HWTEST_F_L0(TraceBarrierTest, WriteBarrier_TEST1)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

#ifndef ARK_USE_SATB_BARRIER
    constexpr uint64_t TAG_BITS_SHIFT = 48;
    constexpr uint64_t TAG_MARK = 0xFFFFULL << TAG_BITS_SHIFT;
    constexpr uint64_t TAG_SPECIAL = 0x02ULL;
    constexpr uint64_t TAG_BOOLEAN = 0x04ULL;
    constexpr uint64_t TAG_HEAP_OBJECT_MASK = TAG_MARK | TAG_SPECIAL | TAG_BOOLEAN;

    RefField<> field(MAddress(0));
    traceBarrier_->WriteBarrier(nullptr, field, nullptr);
    BaseObject *obj = reinterpret_cast<BaseObject *>(TAG_HEAP_OBJECT_MASK);
    traceBarrier_->WriteBarrier(obj, field, obj);
    EXPECT_TRUE(obj != nullptr);
#endif
}

HWTEST_F_L0(TraceBarrierTest, WriteStaticRef_TEST1)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

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
    traceBarrier_->WriteStaticRef(field, &newObj);
    MAddress newAddress = field.GetFieldValue();
    EXPECT_NE(newAddress, oldAddress);
}

HWTEST_F_L0(TraceBarrierTest, AtomicReadRefField_TEST1)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

    BaseObject obj;
    constexpr size_t size = 100;
    obj.SetSizeForwarded(size);
    EXPECT_EQ(obj.GetSizeForwarded(), size);
    RefField<true> field(&obj);

    BaseObject *resultObj = nullptr;
    resultObj = traceBarrier_->AtomicReadRefField(&obj, field, std::memory_order_seq_cst);
    ASSERT_TRUE(resultObj != nullptr);
}

HWTEST_F_L0(TraceBarrierTest, AtomicWriteRefField_TEST1)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

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

    traceBarrier_->AtomicWriteRefField(&oldObj, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(TraceBarrierTest, AtomicWriteRefField_TEST2)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

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

    traceBarrier_->AtomicWriteRefField(nullptr, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(TraceBarrierTest, AtomicSwapRefField_TEST1)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

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
    resultObj = traceBarrier_->AtomicSwapRefField(&oldObj, oldField, &newObj, std::memory_order_relaxed);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(oldField.GetFieldValue(), newField.GetFieldValue());
}

HWTEST_F_L0(TraceBarrierTest, CompareAndSwapRefField_TEST1)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

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

    bool result = traceBarrier_->CompareAndSwapRefField(
        &oldObj, oldField, &oldObj, &newObj, std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(TraceBarrierTest, CompareAndSwapRefField_TEST2)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

    BaseObject oldObj;
    constexpr size_t oldSize = 100;
    oldObj.SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj.GetSizeForwarded(), oldSize);
    RefField<true> oldField(&oldObj);

    bool result = traceBarrier_->CompareAndSwapRefField(
        &oldObj, oldField, &oldObj, &oldObj, std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(TraceBarrierTest, CompareAndSwapRefField_TEST3)
{
    ASSERT_TRUE(traceBarrier_ != nullptr);

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

    bool result = traceBarrier_->CompareAndSwapRefField(
        &oldObj, newField, &oldObj, &newObj, std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_FALSE(result);
}
}  // namespace common::test