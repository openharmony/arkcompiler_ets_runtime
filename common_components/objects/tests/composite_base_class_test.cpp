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

#include "common_components/tests/test_helper.h"
#include "common_interfaces/objects/composite_base_class.h"
#include "common_interfaces/objects/base_object.h"

#include <vector>
#include <memory>
#include <functional>

namespace common {
class CompositeBaseClassTest : public test::BaseTestWithScope {
protected:
    static CompositeBaseClass* MockAllocator()
    {
        void* memory = ::operator new(sizeof(uint64_t) * 16);

        auto* baseClass = reinterpret_cast<BaseClass*>(memory);
        baseClass->SetObjectType(CommonType::LINE_STRING);
        baseClass->ClearBitField();

        return reinterpret_cast<CompositeBaseClass*>(baseClass);
    }

    void SetUp() override
    {
        roots_ = std::make_unique<BaseClassRoots>();
    }

    void TearDown() override
    {
        roots_.reset();
    }

    std::unique_ptr<BaseClassRoots> roots_;
};

HWTEST_F_L0(CompositeBaseClassTest, InitializeOnce)
{
    auto allocator = []() -> CompositeBaseClass* {
        return CompositeBaseClassTest::MockAllocator();
    };

    roots_->InitializeCompositeBaseClass(allocator);
    roots_->InitializeCompositeBaseClass(allocator);

    EXPECT_TRUE(true);
}

HWTEST_F_L0(CompositeBaseClassTest, CreateAndGetType)
{
    auto allocator = []() -> CompositeBaseClass* {
        return CompositeBaseClassTest::MockAllocator();
    };

    roots_->InitializeCompositeBaseClass(allocator);

    auto* baseClass = roots_->GetBaseClass(CommonType::LINE_STRING);
    ASSERT_NE(baseClass, nullptr);
    EXPECT_EQ(baseClass->GetObjectType(), CommonType::LINE_STRING);
}

HWTEST_F_L0(CompositeBaseClassTest, GetBaseClassReturnsCorrectType)
{
    auto allocator = []() -> CompositeBaseClass* {
        return CompositeBaseClassTest::MockAllocator();
    };

    roots_->InitializeCompositeBaseClass(allocator);

    auto* lineString = roots_->GetBaseClass(CommonType::LINE_STRING);
    auto* slicedString = roots_->GetBaseClass(CommonType::SLICED_STRING);
    auto* treeString = roots_->GetBaseClass(CommonType::TREE_STRING);

    ASSERT_NE(lineString, nullptr);
    ASSERT_NE(slicedString, nullptr);
    ASSERT_NE(treeString, nullptr);

    EXPECT_EQ(lineString->GetObjectType(), CommonType::LINE_STRING);
    EXPECT_EQ(slicedString->GetObjectType(), CommonType::SLICED_STRING);
    EXPECT_EQ(treeString->GetObjectType(), CommonType::TREE_STRING);
}

HWTEST_F_L0(CompositeBaseClassTest, IterateCompositeBaseClass)
{
    auto allocator = []() -> CompositeBaseClass* {
        return CompositeBaseClassTest::MockAllocator();
    };

    roots_->InitializeCompositeBaseClass(allocator);

    std::vector<CompositeBaseClass*> visited;

    roots_->IterateCompositeBaseClass([&visited](RefField<>& field) {
        auto* ptr = reinterpret_cast<CompositeBaseClass*>(const_cast<void*>(static_cast<const void*>(&field)));
        visited.push_back(ptr);
    });

    EXPECT_EQ(visited.size(), 3);
}
}