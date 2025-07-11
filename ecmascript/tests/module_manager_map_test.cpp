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

#include "ecmascript/module/module_manager_map.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {

class ModuleManagerMapTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        instance->SetEnableForceGC(false);
    }

protected:
    ModuleManagerMap<int> map;
};

// --- GCRoot Tests ---

HWTEST_F_L0(ModuleManagerMapTest, GCRoot)
{
    // Test default constructor
    GCRoot root;
    ASSERT_EQ(root.Read().GetRawData(), JSTaggedValue::Hole().GetRawData());

    // Test regular and move constructor
    {
        JSTaggedValue value(42);
        GCRoot root1(value);

        GCRoot root2(std::move(root1));
        ASSERT_EQ(root2.Read().GetRawData(), value.GetRawData());
    }

    // Test move assignment
    {
        JSTaggedValue value1(42);

        JSTaggedValue value2(84);

        GCRoot root1(value1);
        GCRoot root2(value2);

        root2 = std::move(root1);
        ASSERT_EQ(root2.Read().GetRawData(), value1.GetRawData());
    }

    // Test read barrier
    {
        JSTaggedValue value(42);
        GCRoot root(value);

        JSTaggedValue read_value = root.Read();
        ASSERT_EQ(read_value.GetRawData(), value.GetRawData());
    }

    // Test Hash er
    {
        JSTaggedValue value1(42);
        JSTaggedValue value2(42);
        JSTaggedValue value3(84);

        GCRoot root1(value1);
        GCRoot root2(value2);
        GCRoot root3(value3);

        GCRoot::GCRootHash hasher;

        // Same values should have same hash
        ASSERT_EQ(hasher(root1), hasher(root2));

        // Different values should have different hashes
        ASSERT_NE(hasher(root1), hasher(root3));

        // Template version should work with raw types
        ASSERT_EQ(hasher(root1), hasher(value1.GetRawData()));
    }
}

HWTEST_F_L0(ModuleManagerMapTest, InsertAndFind)
{
    JSTaggedValue value(42);

    map.Insert(1, value);

    auto result = map.Find(1);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->GetRawData(), value.GetRawData());
}

HWTEST_F_L0(ModuleManagerMapTest, EmplaceAndFind)
{
    JSTaggedValue value(42);

    map.Emplace(1, value);

    auto result = map.Find(1);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->GetRawData(), value.GetRawData());
}

HWTEST_F_L0(ModuleManagerMapTest, EmplaceDoesNotOverwrite)
{
    JSTaggedValue value1(42);
    JSTaggedValue value2(84);

    map.Insert(1, value1);
    map.Emplace(1, value2);  // Should not overwrite

    auto result = map.Find(1);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->GetRawData(), value1.GetRawData());  // Should still be original value
}

HWTEST_F_L0(ModuleManagerMapTest, InsertOverwrites)
{
    JSTaggedValue value1(42);
    JSTaggedValue value2(84);

    map.Insert(1, value1);
    map.Insert(1, value2);  // Should overwrite

    auto result = map.Find(1);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->GetRawData(), value2.GetRawData());  // Should be new value
}

HWTEST_F_L0(ModuleManagerMapTest, FindNonExistent)
{
    auto result = map.Find(999);
    ASSERT_FALSE(result.has_value());
}

HWTEST_F_L0(ModuleManagerMapTest, ForEach)
{
    JSTaggedValue value1(42);
    JSTaggedValue value2(84);

    map.Insert(1, value1);
    map.Insert(2, value2);

    std::vector<std::pair<int, JSTaggedValue>> collected;
    map.ForEach([&](auto it) { collected.emplace_back(it->first, it->second.Read()); });

    ASSERT_EQ(collected.size(), 2U);

    // Sort for deterministic comparison
    std::sort(collected.begin(), collected.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

    ASSERT_EQ(collected[0].first, 1);
    ASSERT_EQ(collected[0].second.GetRawData(), value1.GetRawData());
    ASSERT_EQ(collected[1].first, 2);
    ASSERT_EQ(collected[1].second.GetRawData(), value2.GetRawData());
}

HWTEST_F_L0(ModuleManagerMapTest, Erase)
{
    JSTaggedValue value(42);

    map.Insert(1, value);
    ASSERT_TRUE(map.Find(1).has_value());

    map.Erase(1);
    ASSERT_FALSE(map.Find(1).has_value());
    ASSERT_EQ(map.Size(), 0U);
}

HWTEST_F_L0(ModuleManagerMapTest, Size)
{
    ASSERT_EQ(map.Size(), 0U);

    map.Insert(1, JSTaggedValue(42));
    ASSERT_EQ(map.Size(), 1U);

    map.Insert(2, JSTaggedValue(84));
    ASSERT_EQ(map.Size(), 2U);

    map.Erase(1);
    ASSERT_EQ(map.Size(), 1U);
}

HWTEST_F_L0(ModuleManagerMapTest, Clear)
{
    map.Insert(1, JSTaggedValue(42));
    map.Insert(2, JSTaggedValue(84));

    ASSERT_EQ(map.Size(), 2U);

    map.Clear();

    ASSERT_EQ(map.Size(), 0U);
    ASSERT_FALSE(map.Find(1).has_value());
    ASSERT_FALSE(map.Find(2).has_value());
}

HWTEST_F_L0(ModuleManagerMapTest, TemplateKeyType)
{
    ModuleManagerMap<std::string> string_map;
    JSTaggedValue value(42);

    string_map.Insert("test_key", value);

    auto result = string_map.Find("test_key");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->GetRawData(), value.GetRawData());

    ASSERT_FALSE(string_map.Find("non_existent").has_value());
}
}  // namespace panda::test
