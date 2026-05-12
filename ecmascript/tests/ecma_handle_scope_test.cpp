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

#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "gtest/gtest.h"

using namespace panda::ecmascript;

namespace panda::test {

class EcmaHandleScopeTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(EcmaHandleScopeTest, NestingLevels_SingleAndSequential)
{
    auto initialLevel = instance->GetOpenHandleScopes();

    // Test single scope
    {
        EcmaHandleScope scope(thread);
        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);
    }

    // Test sequential scopes
    {
        EcmaHandleScope scope1(thread);
        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);
    }
    {
        EcmaHandleScope scope2(thread);
        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);
    }

    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel);
}

HWTEST_F_L0(EcmaHandleScopeTest, NestingLevels_DoubleNested)
{
    auto initialLevel = instance->GetOpenHandleScopes();

    // Test double nested scopes
    {
        EcmaHandleScope outerScope(thread);
        [[maybe_unused]] auto outerHandle =
            EcmaHandleScope::NewHandle(thread, JSTaggedValue::Undefined().GetRawData());
        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);

        {
            EcmaHandleScope innerScope(thread);
            [[maybe_unused]] auto innerHandle =
                EcmaHandleScope::NewHandle(thread, JSTaggedValue::Hole().GetRawData());
            EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 2);
        }

        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);
    }

    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel);
}

HWTEST_F_L0(EcmaHandleScopeTest, NestingLevels_TripleNested)
{
    auto initialLevel = instance->GetOpenHandleScopes();

    // Test triple nested scopes with handle creation
    {
        EcmaHandleScope s1(thread);
        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);

        {
            EcmaHandleScope s2(thread);
            EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 2);

            {
                EcmaHandleScope s3(thread);
                EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 3);

                [[maybe_unused]] auto h1 =
                    EcmaHandleScope::NewHandle(thread, JSTaggedValue::Undefined().GetRawData());
                [[maybe_unused]] auto h2 =
                    EcmaHandleScope::NewHandle(thread, JSTaggedValue::Hole().GetRawData());
            }

            EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 2);

            [[maybe_unused]] auto h3 =
                EcmaHandleScope::NewHandle(thread, JSTaggedValue::True().GetRawData());
        }

        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);
    }

    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel);
}

HWTEST_F_L0(EcmaHandleScopeTest, NestingLevels_Interleaved)
{
    auto initialLevel = instance->GetOpenHandleScopes();

    // Test interleaved scopes
    {
        EcmaHandleScope a1(thread);

        {
            EcmaHandleScope b1(thread);

            {
                EcmaHandleScope c1(thread);
            }

            EcmaHandleScope c2(thread);
        }

        EcmaHandleScope b2(thread);
    }

    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel);
}

HWTEST_F_L0(EcmaHandleScopeTest, ManualHeapAllocation)
{
    auto initialLevel = instance->GetOpenHandleScopes();

    // Create heap-allocated scopes and delete in wrong order (LIFO violation)
    EcmaHandleScope *scope1 = new EcmaHandleScope(thread);
    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);

    EcmaHandleScope *scope2 = new EcmaHandleScope(thread);
    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 2);

    // Delete scope1 first (breaking LIFO order)
    delete scope1;
    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);

    // Delete scope2 second - this triggers scopeLevel_ != GetOpenHandleScopes() detection
    // scope2->scopeLevel_ (initialLevel + 2) != GetOpenHandleScopes() (initialLevel + 1)
    delete scope2;
    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel);
}

HWTEST_F_L0(EcmaHandleScopeTest, CreateHandles)
{
    auto initialLevel = instance->GetOpenHandleScopes();
    EXPECT_GE(initialLevel, 1);  // SetUp() already created a scope

    // Create handles without explicit scope - uses existing scope from SetUp
    uintptr_t h1 = EcmaHandleScope::NewHandle(thread, JSTaggedValue::Undefined().GetRawData());
    EXPECT_NE(h1, 0);
    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel);

    uintptr_t h2 = EcmaHandleScope::NewPrimitiveHandle(thread, JSTaggedValue::True().GetRawData());
    EXPECT_NE(h2, 0);
    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel);

    // Create multiple handles
    constexpr int handleCount = 10;
    for (int i = 0; i < handleCount; i++) {
        uintptr_t handle = EcmaHandleScope::NewHandle(thread, JSTaggedValue::Hole().GetRawData());
        EXPECT_NE(handle, 0);
    }
    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel);

    // Create handles in nested scope
    {
        EcmaHandleScope scope(thread);
        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);

        [[maybe_unused]] uintptr_t h3 =
            EcmaHandleScope::NewHandle(thread, JSTaggedValue::Null().GetRawData());
        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);

        [[maybe_unused]] uintptr_t h4 =
            EcmaHandleScope::NewPrimitiveHandle(thread, JSTaggedValue::Hole().GetRawData());
        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);
    }

    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel);

    // Create handles across scope boundaries
    {
        EcmaHandleScope scope1(thread);
        [[maybe_unused]] uintptr_t h5 =
            EcmaHandleScope::NewHandle(thread, JSTaggedValue::Undefined().GetRawData());
        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);
    }

    uintptr_t hLeaked = EcmaHandleScope::NewHandle(thread, JSTaggedValue::Null().GetRawData());
    EXPECT_NE(hLeaked, 0);
    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel);
}

HWTEST_F_L0(EcmaHandleScopeTest, NestedScopeWithOuterHandle)
{
    auto initialLevel = instance->GetOpenHandleScopes();

    uintptr_t outerHandle = 0;

    {
        EcmaHandleScope outerScope(thread);
        outerHandle = EcmaHandleScope::NewHandle(thread, JSTaggedValue::Undefined().GetRawData());

        {
            EcmaHandleScope innerScope(thread);
            [[maybe_unused]] uintptr_t innerHandle =
                EcmaHandleScope::NewHandle(thread, JSTaggedValue::True().GetRawData());
            EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 2);
        }

        EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel + 1);
    }

    EXPECT_EQ(instance->GetOpenHandleScopes(), initialLevel);

    uintptr_t leakedHandle = EcmaHandleScope::NewHandle(thread, JSTaggedValue::Null().GetRawData());
    EXPECT_NE(leakedHandle, 0);
}

}  // namespace panda::test
