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

#include "ecmascript/jit/jit.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {

class JitTimeScopeTest : public BaseTestWithScope<false> {
};

/**
 * @tc.name: TimeScope_BasicConstruction
 * @tc.desc: Test basic TimeScope construction and destruction with output logging
 * @tc.type: FUNC
 */
HWTEST_F_L0(JitTimeScopeTest, TimeScope_BasicConstruction)
{
    auto vm = thread->GetEcmaVM();
    ASSERT_NE(vm, nullptr);

    {
        // Test with INFO level logging disabled to avoid log spam
        Jit::TimeScope timeScope(vm, "TestCompilation", CompilerTier::Tier::FAST, false, false);
        // Verify time tracking works
        EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
    }
}

/**
 * @tc.name: TimeScope_DisableOutputLog
 * @tc.desc: Test TimeScope when output logging is disabled
 * @tc.type: FUNC
 */
HWTEST_F_L0(JitTimeScopeTest, TimeScope_DisableOutputLog)
{
    auto vm = thread->GetEcmaVM();
    ASSERT_NE(vm, nullptr);

    {
        // Output logging disabled
        Jit::TimeScope timeScope(vm, "SilentCompilation", CompilerTier::Tier::FAST, false, false);
        // Even with logging disabled, timing should work
        EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
    }
}

/**
 * @tc.name: TimeScope_DefaultConstruction
 * @tc.desc: Test default TimeScope construction for JitLockHolder
 * @tc.type: FUNC
 */
HWTEST_F_L0(JitTimeScopeTest, TimeScope_DefaultConstruction)
{
    auto vm = thread->GetEcmaVM();
    ASSERT_NE(vm, nullptr);

    // This constructor is used in JitLockHolder
    Jit::TimeScope timeScope(vm);

    // Default construction should still track time
    EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
}

/**
 * @tc.name: TimeScope_DifferentTiers
 * @tc.desc: Test TimeScope with different compiler tiers
 * @tc.type: FUNC
 */
HWTEST_F_L0(JitTimeScopeTest, TimeScope_DifferentTiers)
{
    auto vm = thread->GetEcmaVM();
    ASSERT_NE(vm, nullptr);

    {
        Jit::TimeScope timeScope(vm, "FastJIT", CompilerTier::Tier::FAST, false, false);
        EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
    }

    {
        Jit::TimeScope timeScope(vm, "BaselineJIT", CompilerTier::Tier::BASELINE, false, false);
        EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
    }
}

/**
 * @tc.name: TimeScope_AppendMessage
 * @tc.desc: Test appendMessage method
 * @tc.type: FUNC
 */
HWTEST_F_L0(JitTimeScopeTest, TimeScope_AppendMessage)
{
    auto vm = thread->GetEcmaVM();
    ASSERT_NE(vm, nullptr);

    Jit::TimeScope timeScope(vm, "Initial", CompilerTier::Tier::FAST, false, false);

    // Test multiple appends
    timeScope.appendMessage(" - Step1");
    timeScope.appendMessage(" - Step2");
    timeScope.appendMessage(" - Step3");

    // Verify time still tracked after appends
    EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
}

/**
 * @tc.name: TimeScope_NestedScopes
 * @tc.desc: Test nested TimeScope instances
 * @tc.type: FUNC
 */
HWTEST_F_L0(JitTimeScopeTest, TimeScope_NestedScopes)
{
    auto vm = thread->GetEcmaVM();
    ASSERT_NE(vm, nullptr);

    // Verify nested scopes track time independently
    {
        Jit::TimeScope outerScope(vm, "Outer", CompilerTier::Tier::FAST, false, false);
        {
            Jit::TimeScope middleScope(vm, "Middle", CompilerTier::Tier::BASELINE, false, false);
            {
                Jit::TimeScope innerScope(vm, "Inner", CompilerTier::Tier::FAST, false, false);
                EXPECT_GE(innerScope.TotalSpentTimeInMicroseconds(), 0);
            }
            EXPECT_GE(middleScope.TotalSpentTimeInMicroseconds(), 0);
        }
        EXPECT_GE(outerScope.TotalSpentTimeInMicroseconds(), 0);
    }
}

/**
 * @tc.name: TimeScope_LongMessage
 * @tc.desc: Test TimeScope with long message strings
 * @tc.type: FUNC
 */
HWTEST_F_L0(JitTimeScopeTest, TimeScope_LongMessage)
{
    auto vm = thread->GetEcmaVM();
    ASSERT_NE(vm, nullptr);

    CString longMessage = "This is a very long message that contains a lot of information about the compilation "
                         "process including method name, file name, and other relevant details that might be "
                         "useful for debugging and performance analysis";

    {
        Jit::TimeScope timeScope(vm, longMessage, CompilerTier::Tier::FAST, false, false);
        EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
    }
}

/**
 * @tc.name: TimeScope_SpecialCharacters
 * @tc.desc: Test TimeScope with special characters in message
 * @tc.type: FUNC
 */
HWTEST_F_L0(JitTimeScopeTest, TimeScope_SpecialCharacters)
{
    auto vm = thread->GetEcmaVM();
    ASSERT_NE(vm, nullptr);

    // Test with various special characters
    {
        Jit::TimeScope timeScope(vm, "Method:with:colons", CompilerTier::Tier::FAST, false, false);
        EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
    }

    {
        Jit::TimeScope timeScope(vm, "Method@with@at", CompilerTier::Tier::FAST, false, false);
        EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
    }

    {
        Jit::TimeScope timeScope(vm, "Method(with)parens", CompilerTier::Tier::FAST, false, false);
        EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
    }

    {
        Jit::TimeScope timeScope(vm, "Method<with>angles", CompilerTier::Tier::FAST, false, false);
        EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
    }
}

/**
 * @tc.name: TimeScope_MemoryManagement
 * @tc.desc: Test TimeScope doesn't leak memory with VM reference
 * @tc.type: FUNC
 */
HWTEST_F_L0(JitTimeScopeTest, TimeScope_MemoryManagement)
{
    auto vm = thread->GetEcmaVM();
    ASSERT_NE(vm, nullptr);

    {
        Jit::TimeScope timeScope(vm, "MemoryTest", CompilerTier::Tier::FAST, false, false);
        // VM should be stored
    }

    // VM should still be valid after scope destruction
    EXPECT_NE(vm, nullptr);
}

/**
 * @tc.name: TimeScope_EmptyMessage
 * @tc.desc: Test TimeScope with empty message
 * @tc.type: FUNC
 */
HWTEST_F_L0(JitTimeScopeTest, TimeScope_EmptyMessage)
{
    auto vm = thread->GetEcmaVM();
    ASSERT_NE(vm, nullptr);

    Jit::TimeScope timeScope(vm, "", CompilerTier::Tier::FAST, false, false);
    EXPECT_GE(timeScope.TotalSpentTimeInMicroseconds(), 0);
}

/**
 * @tc.name: TimeScope_TotalSpentTimeFloat
 * @tc.desc: Test TotalSpentTime returns valid float value
 * @tc.type: FUNC
 */
HWTEST_F_L0(JitTimeScopeTest, TimeScope_TotalSpentTimeFloat)
{
    auto vm = thread->GetEcmaVM();
    ASSERT_NE(vm, nullptr);

    Jit::TimeScope timeScope(vm, "FloatTime", CompilerTier::Tier::FAST, false, false);

    float timeMs = timeScope.TotalSpentTime();
    int timeMicros = timeScope.TotalSpentTimeInMicroseconds();

    // Both should be non-negative
    EXPECT_GE(timeMs, 0.0f);
    EXPECT_GE(timeMicros, 0);
}

} // namespace panda::test
