/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class AssertScopeTest : public BaseTestWithOutScope {
};

HWTEST_F_L0(AssertScopeTest, AssertScopeChecker)
{
#if !defined(NDEBUG)
    {
        EXPECT_TRUE(AllowHeapAlloc::IsAllowed());
        EXPECT_TRUE(AllowGarbageCollection::IsAllowed());
        [[maybe_unused]] DisallowGarbageCollection noGc;  // set dis-allow gc
        {
            [[maybe_unused]] AllowGarbageCollection allowGc;  // set allow gc
            EXPECT_TRUE(AllowGarbageCollection::IsAllowed());

            [[maybe_unused]] DisAllowHeapAlloc noHeapAlloc;  // set dis-allow alloc
            {
                [[maybe_unused]] AllowHeapAlloc heapAlloc;  // set allow alloc
                EXPECT_TRUE(AllowHeapAlloc::IsAllowed());
            }
            EXPECT_FALSE(AllowHeapAlloc::IsAllowed());
        }
        // allow alloc
        EXPECT_TRUE(AllowHeapAlloc::IsAllowed());
        // dis-allow gc
        EXPECT_FALSE(AllowGarbageCollection::IsAllowed());
    }
    // allow gc
    EXPECT_TRUE(AllowGarbageCollection::IsAllowed());
#endif
    EXPECT_TRUE(AllowHeapAlloc::IsAllowed());
    EXPECT_TRUE(AllowGarbageCollection::IsAllowed());
}

HWTEST_F_L0(AssertScopeTest, AssertScopeChecker_LocalToShareWeakRef)
{
#ifndef NDEBUG
    EXPECT_FALSE(AllowLocalToShareWeakRefHandle::IsAllowed());
    {
        [[maybe_unused]] AllowLocalToShareWeakRefHandle weakRef;  // set allow local-to-share weak ref
        EXPECT_TRUE(AllowLocalToShareWeakRefHandle::IsAllowed());
        {
            [[maybe_unused]] DisAllowLocalToShareWeakRefHandle noWeakRef;  // set dis-allow local-to-share weak ref
            EXPECT_FALSE(AllowLocalToShareWeakRefHandle::IsAllowed());
        }
        EXPECT_TRUE(AllowLocalToShareWeakRefHandle::IsAllowed());
    }
    EXPECT_FALSE(AllowLocalToShareWeakRefHandle::IsAllowed());
#endif
}
}  // namespace panda::test
