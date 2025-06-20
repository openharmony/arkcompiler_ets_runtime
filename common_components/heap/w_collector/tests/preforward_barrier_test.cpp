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
#include "common_components/heap/heap.h"
#include "common_components/tests/test_helper.h"

using namespace common;

namespace common::test {
class PreforwardBarrierTest : public BaseTestWithScope {};

HWTEST_F_L0(PreforwardBarrierTest, AtomicWriteRefField_TEST1)
{
    Collector &collector = Heap::GetHeap().GetCollector();
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

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

    preforwardBarrier->AtomicWriteRefField(&oldObj, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}

HWTEST_F_L0(PreforwardBarrierTest, AtomicWriteRefField_TEST2)
{
    Collector &collector = Heap::GetHeap().GetCollector();
    auto preforwardBarrier = std::make_unique<PreforwardBarrier>(collector);
    ASSERT_TRUE(preforwardBarrier != nullptr);

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

    preforwardBarrier->AtomicWriteRefField(nullptr, oldField, &newObj, std::memory_order_relaxed);
    EXPECT_EQ(oldField.GetFieldValue(), neWAddress);
}
}  // namespace common::test
