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

#include "common_components/heap/w_collector/preforward_barrier.h"
#include "common_components/heap/w_collector/tests/mock_barrier_collector.h"
#include "common_components/mutator/mutator_manager.h"
#include "common_components/tests/test_helper.h"
#include "common_components/heap/heap_manager.h"
#include "common_interfaces/base_runtime.h"

using namespace common;

namespace common::test {
class PreforwardBarrierTest : public BaseTestWithScope {
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

HWTEST_F_L0(PreforwardBarrierTest, ReadRefField_TEST1)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);

    BaseObject* resultObj = preforwardBarrier->ReadRefField(obj, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, obj);
}

HWTEST_F_L0(PreforwardBarrierTest, ReadRefField_TEST2)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);

    BaseObject* resultObj = preforwardBarrier->ReadRefField(nullptr, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, obj);
}

HWTEST_F_L0(PreforwardBarrierTest, ReadRefField_TEST3)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj, true);

    BaseObject* resultObj = preforwardBarrier->ReadRefField(obj, field);
    ASSERT_TRUE(resultObj != nullptr);
    constexpr uint64_t TAG_WEAK = 0x01ULL;
    BaseObject* newObj = reinterpret_cast<BaseObject*>(reinterpret_cast<uintptr_t>(obj) | TAG_WEAK);
    EXPECT_EQ(resultObj, newObj);
}

HWTEST_F_L0(PreforwardBarrierTest, ReadRefField_TEST4)
{
    MockCollectorUnmovableTest collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);

    BaseObject* resultObj = preforwardBarrier->ReadRefField(obj, field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, obj);
}

HWTEST_F_L0(PreforwardBarrierTest, ReadRefField_TEST5)
{
    MockCollectorUnmovableTest collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj, true);

    BaseObject* resultObj = preforwardBarrier->ReadRefField(obj, field);
    ASSERT_TRUE(resultObj != nullptr);
    constexpr uint64_t TAG_WEAK = 0x01ULL;
    BaseObject* newObj = reinterpret_cast<BaseObject*>(reinterpret_cast<uintptr_t>(obj) | TAG_WEAK);
    EXPECT_EQ(resultObj, newObj);
}


HWTEST_F_L0(PreforwardBarrierTest, ReadRefField_TEST6)
{
    MockCollectorForwardTest collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);

    BaseObject* resultObj = preforwardBarrier->ReadRefField(obj, field);
    ASSERT_TRUE(resultObj != nullptr);
}

HWTEST_F_L0(PreforwardBarrierTest, ReadRefField_TEST7)
{
    MockCollectorForwardTest collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj, true);

    BaseObject* resultObj = preforwardBarrier->ReadRefField(obj, field);
    ASSERT_TRUE(resultObj != nullptr);
}

HWTEST_F_L0(PreforwardBarrierTest, ReadStaticRef_TEST1)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);

    BaseObject* resultObj = preforwardBarrier->ReadStaticRef(field);
    ASSERT_TRUE(resultObj != nullptr);
    EXPECT_EQ(resultObj, obj);
}

HWTEST_F_L0(PreforwardBarrierTest, ReadStringTableStaticRef_TEST1)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    RefField<false> field(nullptr);

    BaseObject* resultObj = preforwardBarrier->ReadStringTableStaticRef(field);
    ASSERT_TRUE(resultObj == nullptr);
}

HWTEST_F_L0(PreforwardBarrierTest, ReadStruct_TEST1)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    preforwardBarrier->ReadStruct(dst, obj, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(PreforwardBarrierTest, ReadStruct_TEST2)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    constexpr size_t size = 16;
    uint8_t srcBuffer[size] = {};
    uint8_t dstBuffer[size] = {};
    srcBuffer[0] = 1;
    HeapAddress src = reinterpret_cast<HeapAddress>(srcBuffer);
    HeapAddress dst = reinterpret_cast<HeapAddress>(dstBuffer);
    preforwardBarrier->ReadStruct(dst, nullptr, src, size);
    EXPECT_EQ(dstBuffer[0], 1);
    EXPECT_EQ(srcBuffer[0], dstBuffer[0]);
}

HWTEST_F_L0(PreforwardBarrierTest, AtomicReadRefField_TEST1)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    constexpr size_t size = 100;
    obj->SetSizeForwarded(size);
    EXPECT_EQ(obj->GetSizeForwarded(), size);
    RefField<true> field(obj);

    BaseObject* resultObj = nullptr;
    resultObj = preforwardBarrier->AtomicReadRefField(obj, field, std::memory_order_seq_cst);
    ASSERT_TRUE(resultObj != nullptr);
}

HWTEST_F_L0(PreforwardBarrierTest, AtomicWriteRefField_TEST1)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

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

    preforwardBarrier->AtomicWriteRefField(oldObj, oldField, newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(PreforwardBarrierTest, AtomicWriteRefField_TEST2)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

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

    preforwardBarrier->AtomicWriteRefField(nullptr, oldField, newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(PreforwardBarrierTest, CompareAndSwapRefField_TEST1)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

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
    bool result = preforwardBarrier->CompareAndSwapRefField(oldObj, oldField, oldObj, newObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PreforwardBarrierTest, CompareAndSwapRefField_TEST2)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

    HeapAddress oldAddr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject* oldObj = reinterpret_cast<BaseObject*>(oldAddr);
    constexpr size_t oldSize = 100;
    oldObj->SetSizeForwarded(oldSize);
    EXPECT_EQ(oldObj->GetSizeForwarded(), oldSize);
    RefField<true> oldField(oldObj);

    bool result = preforwardBarrier->CompareAndSwapRefField(oldObj, oldField, oldObj, oldObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PreforwardBarrierTest, CompareAndSwapRefField_TEST3)
{
    MockCollector collector;
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

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

    bool result = preforwardBarrier->CompareAndSwapRefField(oldObj, newField, oldObj, newObj,
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    ASSERT_FALSE(result);
}
}  // namespace common::test
