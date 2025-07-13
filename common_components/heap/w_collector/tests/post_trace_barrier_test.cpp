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

#include "common_components/heap/w_collector/post_trace_barrier.h"
#include "common_components/heap/w_collector/tests/mock_barrier_collector.h"
#include "common_components/mutator/mutator_manager.h"
#include "common_components/tests/test_helper.h"
#include "common_components/heap/heap_manager.h"
#include "common_interfaces/base_runtime.h"

using namespace common;

namespace common::test {
class PostTraceBarrierTest : public ::testing::Test {
protected:
    static void SetUpTestCase()
    {
        BaseRuntime::GetInstance()->Init();
    }

    static void TearDownTestCase() {}

    void SetUp() override
    {
        MutatorManager::Instance().CreateRuntimeMutator(ThreadType::ARK_PROCESSOR);
    }

    void TearDown() override
    {
        MutatorManager::Instance().DestroyRuntimeMutator(ThreadType::ARK_PROCESSOR);
    }
};

HWTEST_F_L0(PostTraceBarrierTest, ReadRefField_TEST1)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);

    BaseObject *resultObj = postTraceBarrier->ReadRefField(obj, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, obj);
}

HWTEST_F_L0(PostTraceBarrierTest, ReadRefField_TEST2)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);

    BaseObject *resultObj = postTraceBarrier->ReadRefField(nullptr, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, obj);
}

HWTEST_F_L0(PostTraceBarrierTest, ReadStaticRef_TEST1)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);

    BaseObject *resultObj = postTraceBarrier->ReadStaticRef(field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, obj);
}

HWTEST_F_L0(PostTraceBarrierTest, ReadStringTableStaticRef_TEST1)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    RefField<false> field(nullptr);

    BaseObject* resultObj = postTraceBarrier->ReadStringTableStaticRef(field);
    ASSERT_TRUE(resultObj == nullptr);
}

HWTEST_F_L0(PostTraceBarrierTest, WriteRefField_TEST1)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(oldObj);
    MAddress oldAddr = field.GetFieldValue();

    HeapAddress newObjAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* newObj = reinterpret_cast<BaseObject*>(newObjAddr);
    postTraceBarrier->WriteRefField(oldObj, field, newObj);
    MAddress newAddr = field.GetFieldValue();

    EXPECT_NE(oldAddr, newAddr);
    EXPECT_EQ(newAddr, reinterpret_cast<MAddress>(newObj));
}

HWTEST_F_L0(PostTraceBarrierTest, ReadStruct_TEST1)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    postTraceBarrier->ReadStruct(dst, obj, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(PostTraceBarrierTest, WriteStruct_TEST1)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    postTraceBarrier->WriteStruct(obj, dst, size, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(PostTraceBarrierTest, AtomicReadRefField_TEST1)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<true> field(obj);

    BaseObject* result = postTraceBarrier->AtomicReadRefField(obj, field, std::memory_order_seq_cst);
    EXPECT_EQ(result, obj);
}

HWTEST_F_L0(PostTraceBarrierTest, AtomicWriteRefField_TEST1)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress oldAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(oldAddr);
    constexpr size_t oldSize = 100;
    oldObj->SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj->GetSizeForwarded(), oldSize);
    RefField<true> oldField(oldObj);
    MAddress oldAddress = oldField.GetFieldValue();

    HeapAddress newAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* newObj = reinterpret_cast<BaseObject*>(newAddr);
    constexpr size_t newSize = 200;
    newObj->SetSizeForwarded(newSize);
    EXPECT_EQ(newObj->GetSizeForwarded(), newSize);

    postTraceBarrier->AtomicWriteRefField(oldObj, oldField, newObj, std::memory_order_relaxed);

    EXPECT_NE(oldField.GetFieldValue(), oldAddress);
    EXPECT_EQ(oldField.GetFieldValue(), reinterpret_cast<MAddress>(newObj));
}

HWTEST_F_L0(PostTraceBarrierTest, AtomicWriteRefField_TEST2)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress oldAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(oldAddr);
    constexpr size_t oldSize = 100;
    oldObj->SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj->GetSizeForwarded(), oldSize);
    RefField<true> oldField(oldObj);
    MAddress oldAddress = oldField.GetFieldValue();

    HeapAddress newAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* newObj = reinterpret_cast<BaseObject*>(newAddr);
    constexpr size_t newSize = 200;
    newObj->SetSizeForwarded(newSize);
    EXPECT_EQ(newObj->GetSizeForwarded(), newSize);

    postTraceBarrier->AtomicWriteRefField(nullptr, oldField, newObj, std::memory_order_relaxed);

    EXPECT_NE(oldField.GetFieldValue(), oldAddress);
    EXPECT_EQ(oldField.GetFieldValue(), reinterpret_cast<MAddress>(newObj));
}

HWTEST_F_L0(PostTraceBarrierTest, AtomicSwapRefField_TEST1)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress oldAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(oldAddr);
    RefField<true> field(oldObj);
    HeapAddress newAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* newObj = reinterpret_cast<BaseObject*>(newAddr);
    BaseObject* result = postTraceBarrier->AtomicSwapRefField(
        oldObj, field, newObj, std::memory_order_relaxed);
    EXPECT_EQ(result, oldObj);
    EXPECT_EQ(field.GetFieldValue(), reinterpret_cast<MAddress>(newObj));
}

HWTEST_F_L0(PostTraceBarrierTest, CompareAndSwapRefField_TEST1)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress oldAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(oldAddr);
    constexpr size_t oldSize = 100;
    oldObj->SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj->GetSizeForwarded(), oldSize);
    RefField<true> oldField(oldObj);
    MAddress oldAddress = oldField.GetFieldValue();

    HeapAddress newAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* newObj = reinterpret_cast<BaseObject*>(newAddr);
    constexpr size_t newSize = 200;
    newObj->SetSizeForwarded(newSize);
    EXPECT_EQ(newObj->GetSizeForwarded(), newSize);
    RefField<true> newField(newObj);
    MAddress neWAddress = newField.GetFieldValue();
    EXPECT_NE(oldAddress, neWAddress);

    bool result = postTraceBarrier->CompareAndSwapRefField(
        oldObj, oldField, oldObj, newObj, std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(PostTraceBarrierTest, CompareAndSwapRefField_TEST2)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress oldAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(oldAddr);
    constexpr size_t oldSize = 100;
    oldObj->SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj->GetSizeForwarded(), oldSize);
    RefField<true> oldField(oldObj);

    MAddress initialAddress = oldField.GetFieldValue();

    bool result = postTraceBarrier->CompareAndSwapRefField(
        oldObj, oldField, oldObj, oldObj, std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
    EXPECT_EQ(oldField.GetFieldValue(), initialAddress);
}

HWTEST_F_L0(PostTraceBarrierTest, CompareAndSwapRefField_TEST3)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress oldAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(oldAddr);
    constexpr size_t oldSize = 100;
    oldObj->SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj->GetSizeForwarded(), oldSize);
    RefField<true> oldField(oldObj);
    MAddress oldAddress = oldField.GetFieldValue();

    HeapAddress newAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* newObj = reinterpret_cast<BaseObject*>(newAddr);
    constexpr size_t newSize = 200;
    newObj->SetSizeForwarded(newSize);
    EXPECT_EQ(newObj->GetSizeForwarded(), newSize);
    RefField<true> newField(newObj);
    MAddress neWAddress = newField.GetFieldValue();
    EXPECT_NE(oldAddress, neWAddress);

    bool result = postTraceBarrier->CompareAndSwapRefField(oldObj, newField, oldObj, newObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_FALSE(result);
}

HWTEST_F_L0(PostTraceBarrierTest, CopyStructArray_TEST1)
{
    MockCollector collector;
    auto postTraceBarrier = std::make_unique<PostTraceBarrier>(collector);
    ASSERT_TRUE(postTraceBarrier != nullptr);

    HeapAddress oldAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(oldAddr);
    constexpr size_t oldSize = 100;
    oldObj->SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj->GetSizeForwarded(), oldSize);

    HeapAddress newAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* newObj = reinterpret_cast<BaseObject*>(newAddr);
    constexpr size_t newSize = 200;
    newObj->SetSizeForwarded(newSize);
    EXPECT_EQ(newObj->GetSizeForwarded(), newSize);

    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);

    postTraceBarrier->CopyStructArray(oldObj, dst, size, newObj, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}
}  // namespace common::test
