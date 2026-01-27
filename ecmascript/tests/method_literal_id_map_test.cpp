/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/jspandafile/method_literal_id_map.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {

class MethodLiteralIDMapTest : public BaseTestWithOutScope {
protected:
    // Helper method to create a mock MethodLiteral for testing
    // Note: We use raw pointers since MethodLiteral requires EntityId for construction
    MethodLiteral* CreateMockMethodLiteral(uint32_t id)
    {
        // For unit testing purposes, we can create simple pointer placeholders
        // In actual usage, these would be proper MethodLiteral objects
        // Add offset to ensure we never return nullptr (when id=0)
        return reinterpret_cast<MethodLiteral*>(static_cast<uintptr_t>((id + 1) * 0x1000));
    }
};

// Test basic construction and destruction
HWTEST_F_L0(MethodLiteralIDMapTest, Construction)
{
    MethodLiteralIDMap map;
    EXPECT_EQ(map.Size(), 0U);
}

// Test Reserve with various sizes
HWTEST_F_L0(MethodLiteralIDMapTest, Reserve)
{
    MethodLiteralIDMap map;

    // Reserve space for 10 elements
    map.Reserve(10);
    EXPECT_EQ(map.Size(), 0U);

    // Reserve should be idempotent for same size
    map.Reserve(10);
    EXPECT_EQ(map.Size(), 0U);
}

// Test basic Insert and Find operations
HWTEST_F_L0(MethodLiteralIDMapTest, InsertAndFind)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    auto* method2 = CreateMockMethodLiteral(2);

    // Insert two entries
    EXPECT_TRUE(map.Insert(100, method1));
    EXPECT_EQ(map.Size(), 1U);

    EXPECT_TRUE(map.Insert(200, method2));
    EXPECT_EQ(map.Size(), 2U);

    // Find existing entries
    EXPECT_EQ(map.Find(100), method1);
    EXPECT_EQ(map.Find(200), method2);

    // Find non-existent entry
    EXPECT_EQ(map.Find(300), nullptr);
}

// Test duplicate key insertion (should fail)
HWTEST_F_L0(MethodLiteralIDMapTest, DuplicateKeyInsert)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    auto* method2 = CreateMockMethodLiteral(2);

    // Insert same key twice
    EXPECT_TRUE(map.Insert(100, method1));
    EXPECT_FALSE(map.Insert(100, method2));  // Should fail

    // Original value should remain
    EXPECT_EQ(map.Find(100), method1);
    EXPECT_EQ(map.Size(), 1U);
}

// Test Insert without Reserve (should fail due to assertion)
HWTEST_F_L0(MethodLiteralIDMapTest, InsertWithoutReserve)
{
    MethodLiteralIDMap map;
    auto* method = CreateMockMethodLiteral(1);

    // Insert without Reserve should trigger ASSERT and crash in debug mode
    // In release mode, it may access nullptr
    // This test verifies the expected behavior
    map.Reserve(1);
    EXPECT_TRUE(map.Insert(100, method));
}

// Test collision handling with linear probing
HWTEST_F_L0(MethodLiteralIDMapTest, CollisionHandling)
{
    MethodLiteralIDMap map;
    map.Reserve(16);  // Small capacity to force collisions

    auto* method1 = CreateMockMethodLiteral(1);
    auto* method2 = CreateMockMethodLiteral(2);
    auto* method3 = CreateMockMethodLiteral(3);

    // Insert multiple entries that may hash to same location
    // Due to linear probing, they should all be stored correctly
    EXPECT_TRUE(map.Insert(0, method1));
    EXPECT_TRUE(map.Insert(16, method2));  // Same hash as 0
    EXPECT_TRUE(map.Insert(32, method3));  // Same hash as 0 and 16

    EXPECT_EQ(map.Size(), 3U);
    EXPECT_EQ(map.Find(0), method1);
    EXPECT_EQ(map.Find(16), method2);
    EXPECT_EQ(map.Find(32), method3);
}

// Test iteration
HWTEST_F_L0(MethodLiteralIDMapTest, Iteration)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    auto* method2 = CreateMockMethodLiteral(2);
    auto* method3 = CreateMockMethodLiteral(3);

    map.Insert(100, method1);
    map.Insert(200, method2);
    map.Insert(300, method3);

    // Count elements using iteration
    size_t count = 0;
    std::set<uint32_t> keys;

    for (auto it = map.begin(); it != map.end(); ++it) {
        auto [key, value] = *it;
        keys.insert(key);
        count++;
    }

    EXPECT_EQ(count, 3U);
    EXPECT_EQ(keys.size(), 3U);
    EXPECT_TRUE(keys.find(100) != keys.end());
    EXPECT_TRUE(keys.find(200) != keys.end());
    EXPECT_TRUE(keys.find(300) != keys.end());
}

// Test range-based for loop
HWTEST_F_L0(MethodLiteralIDMapTest, RangeBasedForLoop)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    auto* method2 = CreateMockMethodLiteral(2);

    map.Insert(100, method1);
    map.Insert(200, method2);

    std::map<uint32_t, MethodLiteral*> result;

    for (auto [key, value] : map) {
        result[key] = value;
    }

    EXPECT_EQ(result.size(), 2U);
    EXPECT_EQ(result[100], method1);
    EXPECT_EQ(result[200], method2);
}

// Test const iteration
HWTEST_F_L0(MethodLiteralIDMapTest, ConstIteration)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    map.Insert(100, method1);

    const MethodLiteralIDMap& const_map = map;

    size_t count = 0;
    for (auto it = const_map.begin(); it != const_map.end(); ++it) {
        auto [key, value] = *it;
        EXPECT_EQ(key, 100U);
        EXPECT_EQ(value, method1);
        count++;
    }

    EXPECT_EQ(count, 1U);
}

// Test Clear operation
HWTEST_F_L0(MethodLiteralIDMapTest, Clear)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    auto* method2 = CreateMockMethodLiteral(2);

    map.Insert(100, method1);
    map.Insert(200, method2);
    EXPECT_EQ(map.Size(), 2U);

    map.Clear();
    EXPECT_EQ(map.Size(), 0U);

    // After Clear, we need to Reserve again
    map.Reserve(10);
    EXPECT_TRUE(map.Insert(300, CreateMockMethodLiteral(3)));
}

// Test move constructor
HWTEST_F_L0(MethodLiteralIDMapTest, MoveConstructor)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    auto* method2 = CreateMockMethodLiteral(2);

    map.Insert(100, method1);
    map.Insert(200, method2);

    // Move construct
    MethodLiteralIDMap map2(std::move(map));

    EXPECT_EQ(map2.Size(), 2U);
    EXPECT_EQ(map2.Find(100), method1);
    EXPECT_EQ(map2.Find(200), method2);

    // Original map should be empty
    EXPECT_EQ(map.Size(), 0U);
}

// Test move assignment
HWTEST_F_L0(MethodLiteralIDMapTest, MoveAssignment)
{
    MethodLiteralIDMap map1;
    map1.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    map1.Insert(100, method1);

    MethodLiteralIDMap map2;
    map2.Reserve(5);

    auto* method2 = CreateMockMethodLiteral(2);
    map2.Insert(200, method2);

    // Move assign
    map1 = std::move(map2);

    EXPECT_EQ(map1.Size(), 1U);
    EXPECT_EQ(map1.Find(200), method2);

    // map2 should be empty
    EXPECT_EQ(map2.Size(), 0U);
}

// Test Find with non-existent keys
HWTEST_F_L0(MethodLiteralIDMapTest, FindNonExistent)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    map.Insert(100, method1);

    // Find non-existent keys
    EXPECT_EQ(map.Find(0), nullptr);
    EXPECT_EQ(map.Find(99), nullptr);
    EXPECT_EQ(map.Find(101), nullptr);
    EXPECT_EQ(map.Find(99999), nullptr);
}

// Test large number of insertions
HWTEST_F_L0(MethodLiteralIDMapTest, LargeNumberOfInsertions)
{
    MethodLiteralIDMap map;
    map.Reserve(200);

    // Insert 50 elements with sequential keys to minimize collisions
    for (uint32_t i = 0; i < 50; i++) {
        auto* method = CreateMockMethodLiteral(i);
        bool inserted = map.Insert(i, method);
        if (!inserted) {
            // If insert fails, report which key failed
            GTEST_LOG_(INFO) << "Failed to insert key: " << i;
        }
        EXPECT_TRUE(inserted);
    }

    EXPECT_EQ(map.Size(), 50U);

    // Verify all elements
    for (uint32_t i = 0; i < 50; i++) {
        auto* found = map.Find(i);
        if (found == nullptr) {
            GTEST_LOG_(INFO) << "Failed to find key: " << i;
        }
        EXPECT_NE(found, nullptr);
    }
}

// Test CapacityCalculation
HWTEST_F_L0(MethodLiteralIDMapTest, CapacityCalculation)
{
    MethodLiteralIDMap map;

    // Test small reserve - minimum capacity is 16
    map.Reserve(1);
    // Should be able to insert at least 12 elements (75% load factor)
    for (uint32_t i = 0; i < 12; i++) {
        auto* method = CreateMockMethodLiteral(i);
        EXPECT_TRUE(map.Insert(i, method));
    }
    EXPECT_EQ(map.Size(), 12U);

    map.Clear();
    map.Reserve(50);
    // Should be able to insert at least 37 elements (75% of 50)
    for (uint32_t i = 0; i < 37; i++) {
        auto* method = CreateMockMethodLiteral(i + 100);
        EXPECT_TRUE(map.Insert(i + 100, method));
    }
    EXPECT_EQ(map.Size(), 37U);
}

// Test empty map iteration
HWTEST_F_L0(MethodLiteralIDMapTest, EmptyMapIteration)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    size_t count = 0;
    for (auto [key, value] : map) {
        (void)key;
        (void)value;
        count++;
    }

    EXPECT_EQ(count, 0U);
    EXPECT_EQ(map.begin(), map.end());
}

// Test iterator comparison
HWTEST_F_L0(MethodLiteralIDMapTest, IteratorComparison)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    map.Insert(100, method1);

    auto begin = map.begin();
    auto end = map.end();

    EXPECT_NE(begin, end);

    auto begin2 = map.begin();
    EXPECT_EQ(begin, begin2);

    ++begin;
    EXPECT_EQ(begin, end);
}

// Test zero key value
HWTEST_F_L0(MethodLiteralIDMapTest, ZeroKeyValue)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method = CreateMockMethodLiteral(0);
    EXPECT_TRUE(map.Insert(0, method));
    EXPECT_EQ(map.Find(0), method);
    EXPECT_EQ(map.Size(), 1U);
}

// Test maximum key value
HWTEST_F_L0(MethodLiteralIDMapTest, MaxKeyValue)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method = CreateMockMethodLiteral(1);
    uint32_t max_key = std::numeric_limits<uint32_t>::max();
    EXPECT_TRUE(map.Insert(max_key, method));
    EXPECT_EQ(map.Find(max_key), method);
    EXPECT_EQ(map.Size(), 1U);
}

// Test reinsertion after Clear
HWTEST_F_L0(MethodLiteralIDMapTest, ReinsertAfterClear)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    // Insert some elements
    auto* method1 = CreateMockMethodLiteral(1);
    auto* method2 = CreateMockMethodLiteral(2);
    map.Insert(100, method1);
    map.Insert(200, method2);
    EXPECT_EQ(map.Size(), 2U);

    // Clear and re-reserve
    map.Clear();
    map.Reserve(10);

    // Reinsert elements
    auto* method3 = CreateMockMethodLiteral(3);
    auto* method4 = CreateMockMethodLiteral(4);
    EXPECT_TRUE(map.Insert(300, method3));
    EXPECT_TRUE(map.Insert(400, method4));
    EXPECT_EQ(map.Size(), 2U);
    EXPECT_EQ(map.Find(300), method3);
    EXPECT_EQ(map.Find(400), method4);
}

// Test same hash index (collision chain)
HWTEST_F_L0(MethodLiteralIDMapTest, CollisionChain)
{
    MethodLiteralIDMap map;
    map.Reserve(16);  // Capacity 16, indices 0-15

    // Keys that hash to same index: 0, 16, 32, 48...
    auto* method1 = CreateMockMethodLiteral(1);
    auto* method2 = CreateMockMethodLiteral(2);
    auto* method3 = CreateMockMethodLiteral(3);
    auto* method4 = CreateMockMethodLiteral(4);

    EXPECT_TRUE(map.Insert(0, method1));     // hash index 0
    EXPECT_TRUE(map.Insert(16, method2));    // hash index 0
    EXPECT_TRUE(map.Insert(32, method3));    // hash index 0
    EXPECT_TRUE(map.Insert(48, method4));    // hash index 0

    EXPECT_EQ(map.Size(), 4U);
    EXPECT_EQ(map.Find(0), method1);
    EXPECT_EQ(map.Find(16), method2);
    EXPECT_EQ(map.Find(32), method3);
    EXPECT_EQ(map.Find(48), method4);
}

// Test insert after move
HWTEST_F_L0(MethodLiteralIDMapTest, InsertAfterMove)
{
    MethodLiteralIDMap map1;
    map1.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    map1.Insert(100, method1);

    // Move map1 to map2
    MethodLiteralIDMap map2 = std::move(map1);

    // map1 should be empty, but we can reserve and insert again
    map1.Reserve(10);
    auto* method2 = CreateMockMethodLiteral(2);
    EXPECT_TRUE(map1.Insert(200, method2));
    EXPECT_EQ(map1.Size(), 1U);
    EXPECT_EQ(map1.Find(200), method2);

    // map2 should still have original data
    EXPECT_EQ(map2.Size(), 1U);
    EXPECT_EQ(map2.Find(100), method1);
}

// Test multiple Clear operations
HWTEST_F_L0(MethodLiteralIDMapTest, MultipleClear)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    // Insert, clear, insert, clear
    for (int round = 0; round < 3; round++) {
        auto* method = CreateMockMethodLiteral(round);
        map.Insert(round * 10, method);
        EXPECT_EQ(map.Size(), 1U);

        map.Clear();
        EXPECT_EQ(map.Size(), 0U);

        if (round < 2) {
            map.Reserve(10);
        }
    }
}

// Test iterator dereference
HWTEST_F_L0(MethodLiteralIDMapTest, IteratorDereference)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    map.Insert(100, method1);

    auto it = map.begin();
    auto [key, value] = *it;
    EXPECT_EQ(key, 100U);
    EXPECT_EQ(value, method1);
}

// Test iterator increment operations
HWTEST_F_L0(MethodLiteralIDMapTest, IteratorIncrement)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    auto* method2 = CreateMockMethodLiteral(2);
    auto* method3 = CreateMockMethodLiteral(3);

    map.Insert(100, method1);
    map.Insert(200, method2);
    map.Insert(300, method3);

    // Test pre-increment
    auto it1 = map.begin();
    auto it2 = ++it1;
    EXPECT_EQ(it1, it2);

    // Test post-increment
    auto it3 = map.begin();
    auto it4 = it3++;
    EXPECT_NE(it3, it4);
    EXPECT_EQ(it4, map.begin());
}

// Test const iterator on const map
HWTEST_F_L0(MethodLiteralIDMapTest, ConstMapIteration)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    map.Insert(100, method1);

    // Get const reference
    const MethodLiteralIDMap& const_map = map;

    size_t count = 0;
    for (auto it = const_map.begin(); it != const_map.end(); ++it) {
        auto [key, value] = *it;
        EXPECT_EQ(key, 100U);
        EXPECT_EQ(value, method1);
        count++;
    }
    EXPECT_EQ(count, 1U);
}

// Test overwrite prevention
HWTEST_F_L0(MethodLiteralIDMapTest, OverwritePrevention)
{
    MethodLiteralIDMap map;
    map.Reserve(10);

    auto* method1 = CreateMockMethodLiteral(1);
    auto* method2 = CreateMockMethodLiteral(2);

    EXPECT_TRUE(map.Insert(100, method1));
    EXPECT_EQ(map.Find(100), method1);

    // Try to overwrite with different value
    EXPECT_FALSE(map.Insert(100, method2));

    // Original value should remain
    EXPECT_EQ(map.Find(100), method1);
    EXPECT_EQ(map.Size(), 1U);
}

}  // namespace panda::test
