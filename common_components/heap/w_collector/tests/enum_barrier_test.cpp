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

#include "common_components/heap/w_collector/enum_barrier.h"
#include "common_components/heap/w_collector/tests/mock_barrier_collector.h"
#include "common_components/mutator/mutator_manager.h"
#include "common_components/tests/test_helper.h"
#include "common_components/heap/heap_manager.h"
#include "common_interfaces/base_runtime.h"

using namespace common;

namespace common::test {
class EnumBarrierTest : public common::test::BaseTestWithScope {
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

HWTEST_F_L0(EnumBarrierTest, ReadRefField_TEST1)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);

    BaseObject* resultObj = enumBarrier->ReadRefField(obj, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, obj);
}

HWTEST_F_L0(EnumBarrierTest, ReadRefField_TEST2)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);

    BaseObject* resultObj = enumBarrier->ReadRefField(nullptr, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, obj);
}

HWTEST_F_L0(EnumBarrierTest, ReadStaticRef_TEST1)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);

    BaseObject* resultObj = enumBarrier->ReadStaticRef(field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, obj);
}

HWTEST_F_L0(EnumBarrierTest, WriteRefField_TEST1)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(addr);
    constexpr size_t oldSize = 100;
    oldObj->SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj->GetSizeForwarded(), oldSize);
    RefField<false> field(oldObj);
    MAddress oldAddress = field.GetFieldValue();

    HeapAddress newAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* newObj = reinterpret_cast<BaseObject*>(newAddr);
    constexpr size_t newSize = 200;
    newObj->SetSizeForwarded(newSize);
    EXPECT_EQ(newObj->GetSizeForwarded(), newSize);
    enumBarrier->WriteRefField(oldObj, field, newObj);
    MAddress newAddress = field.GetFieldValue();
    EXPECT_NE(newAddress, oldAddress);
}

HWTEST_F_L0(EnumBarrierTest, WriteRefField_TEST2)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

    HeapAddress oldAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(oldAddr);
    constexpr size_t oldSize = 100;
    oldObj->SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj->GetSizeForwarded(), oldSize);
    RefField<false> field(nullptr);
    MAddress oldAddress = field.GetFieldValue();

    HeapAddress newAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* newObj = reinterpret_cast<BaseObject*>(newAddr);
    constexpr size_t newSize = 200;
    newObj->SetSizeForwarded(newSize);
    EXPECT_EQ(newObj->GetSizeForwarded(), newSize);
    enumBarrier->WriteRefField(oldObj, field, newObj);
    MAddress newAddress = field.GetFieldValue();
    EXPECT_NE(newAddress, oldAddress);
}

HWTEST_F_L0(EnumBarrierTest, WriteBarrier_TEST1)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

#ifndef ARK_USE_SATB_BARRIER
    constexpr uint64_t TAG_BITS_SHIFT = 48;
    constexpr uint64_t TAG_MARK = 0xFFFFULL << TAG_BITS_SHIFT;
    constexpr uint64_t TAG_SPECIAL = 0x02ULL;
    constexpr uint64_t TAG_BOOLEAN = 0x04ULL;
    constexpr uint64_t TAG_HEAP_OBJECT_MASK = TAG_MARK | TAG_SPECIAL | TAG_BOOLEAN;

    RefField<> field(MAddress(0));
    enumBarrier->WriteBarrier(nullptr, field, nullptr);
    BaseObject *obj = reinterpret_cast<BaseObject *>(TAG_HEAP_OBJECT_MASK);
    enumBarrier->WriteBarrier(obj, field, obj);
    EXPECT_TRUE(obj != nullptr);
#endif
}

HWTEST_F_L0(EnumBarrierTest, WriteBarrier_TEST2)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

#ifdef ARK_USE_SATB_BARRIER
    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> normalField(obj);
    enumBarrier->WriteBarrier(obj, normalField, obj);
    EXPECT_TRUE(obj != nullptr);

    HeapAddress weakAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* weakObj = reinterpret_cast<BaseObject*>(weakAddr);
    RefField<false> weakField(MAddress(0));
    enumBarrier->WriteBarrier(&weakObj, weakField, &weakObj);
    EXPECT_TRUE(weakObj != nullptr);

    HeapAddress nonTaggedAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* nonTaggedObj = reinterpret_cast<BaseObject*>(nonTaggedAddr);
    RefField<false> nonTaggedField(&nonTaggedObj);
    enumBarrier->WriteBarrier(nullptr, nonTaggedField, &nonTaggedObj);
    EXPECT_TRUE(nonTaggedObj != nullptr);
#endif
}

HWTEST_F_L0(EnumBarrierTest, ReadStruct_TEST1)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    enumBarrier->ReadStruct(dst, obj, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(EnumBarrierTest, WriteStruct_TEST1)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

    auto objPtr = std::make_unique<BaseObject>();
    constexpr size_t size = 16;
    auto srcBuffer = std::make_unique<uint8_t[]>(size);
    auto dstBuffer = std::make_unique<uint8_t[]>(size);
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer.get());
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer.get());
    enumBarrier->WriteStruct(objPtr.get(), dst, size, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(EnumBarrierTest, WriteStruct_TEST2)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

    constexpr size_t size = 16;
    auto srcBuffer = std::make_unique<uint8_t[]>(size);
    auto dstBuffer = std::make_unique<uint8_t[]>(size);
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer.get());
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer.get());
    enumBarrier->WriteStruct(nullptr, dst, size, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(EnumBarrierTest, AtomicReadRefField_TEST1)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    constexpr size_t size = 100;
    obj->SetSizeForwarded(size);
    EXPECT_EQ(obj->GetSizeForwarded(), size);
    RefField<true> field(obj);

    BaseObject* resultObj = nullptr;
    resultObj = enumBarrier->AtomicReadRefField(obj, field, std::memory_order_seq_cst);
    ASSERT_TRUE(resultObj != nullptr);
}

HWTEST_F_L0(EnumBarrierTest, AtomicWriteRefField_TEST1)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

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

    enumBarrier->AtomicWriteRefField(oldObj, oldField, newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(EnumBarrierTest, AtomicWriteRefField_TEST2)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

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

    enumBarrier->AtomicWriteRefField(nullptr, oldField, newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(EnumBarrierTest, AtomicSwapRefField_TEST1)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

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

    BaseObject* resultObj = nullptr;
    resultObj = enumBarrier->AtomicSwapRefField(oldObj, oldField, newObj, std::memory_order_relaxed);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(oldField.GetFieldValue(), newField.GetFieldValue());
}

HWTEST_F_L0(EnumBarrierTest, CompareAndSwapRefField_TEST1)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

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
    bool result = enumBarrier->CompareAndSwapRefField(oldObj, oldField, oldObj, newObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(EnumBarrierTest, CompareAndSwapRefField_TEST2)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

    HeapAddress oldAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(oldAddr);
    constexpr size_t oldSize = 100;
    oldObj->SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj->GetSizeForwarded(), oldSize);
    RefField<true> oldField(oldObj);

    bool result = enumBarrier->CompareAndSwapRefField(oldObj, oldField, oldObj, oldObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(EnumBarrierTest, CompareAndSwapRefField_TEST3)
{
    MockCollector collector;
    auto enumBarrier = std::make_unique<EnumBarrier>(collector);
    ASSERT_TRUE(enumBarrier != nullptr);

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

    bool result = enumBarrier->CompareAndSwapRefField(oldObj, newField, oldObj, newObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_FALSE(result);
}

} // namespace common::test
