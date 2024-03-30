/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <cstdio>

#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JsStackInfoTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        instance->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(JsStackInfoTest, BuildJsStackTrace)
{
    std::string stack = JsStackInfo::BuildJsStackTrace(thread, false);
    ASSERT_TRUE(!stack.empty());
}

HWTEST_F_L0(JsStackInfoTest, FrameCheckTest)
{
    uintptr_t frame[22];
    frame[0] = reinterpret_cast<uintptr_t>(FrameType::OPTIMIZED_FRAME);
    frame[1] = reinterpret_cast<uintptr_t>(FrameType::OPTIMIZED_ENTRY_FRAME);
    frame[2] = reinterpret_cast<uintptr_t>(FrameType::OPTIMIZED_JS_FUNCTION_FRAME);
    frame[3] = reinterpret_cast<uintptr_t>(FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME);
    frame[4] = reinterpret_cast<uintptr_t>(FrameType::ASM_BRIDGE_FRAME);
    frame[5] = reinterpret_cast<uintptr_t>(FrameType::LEAVE_FRAME);
    frame[6] = reinterpret_cast<uintptr_t>(FrameType::LEAVE_FRAME_WITH_ARGV);
    frame[7] = reinterpret_cast<uintptr_t>(FrameType::BUILTIN_CALL_LEAVE_FRAME);
    frame[8] = reinterpret_cast<uintptr_t>(FrameType::INTERPRETER_FRAME);
    frame[9] = reinterpret_cast<uintptr_t>(FrameType::ASM_INTERPRETER_FRAME);
    frame[10] = reinterpret_cast<uintptr_t>(FrameType::INTERPRETER_CONSTRUCTOR_FRAME);
    frame[11] = reinterpret_cast<uintptr_t>(FrameType::BUILTIN_FRAME);
    frame[12] = reinterpret_cast<uintptr_t>(FrameType::BUILTIN_FRAME_WITH_ARGV);
    frame[13] = reinterpret_cast<uintptr_t>(FrameType::BUILTIN_ENTRY_FRAME);
    frame[14] = reinterpret_cast<uintptr_t>(FrameType::INTERPRETER_BUILTIN_FRAME);
    frame[15] = reinterpret_cast<uintptr_t>(FrameType::INTERPRETER_FAST_NEW_FRAME);
    frame[16] = reinterpret_cast<uintptr_t>(FrameType::INTERPRETER_ENTRY_FRAME);
    frame[17] = reinterpret_cast<uintptr_t>(FrameType::ASM_INTERPRETER_ENTRY_FRAME);
    frame[18] = reinterpret_cast<uintptr_t>(FrameType::ASM_INTERPRETER_BRIDGE_FRAME);
    frame[19] = reinterpret_cast<uintptr_t>(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME);
    frame[20] = reinterpret_cast<uintptr_t>(FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME);
    frame[21] = reinterpret_cast<uintptr_t>(FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME);

    for (i = 0; i < 22; i++) {
        bool ret1 = ArkFrameCheck(frame[i]);
        if (i == 1 || i == 17) {
            EXPECT_TRUE(ret1 == true);
        } else {
            EXPECT_TRUE(ret1 == false);
        }
        bool ret2 = IsFunctionFrame(frame[i]);
        if (i == 2 || i == 3 || i == 8 || i == 9 || i ==10 || i == 15) {
            EXPECT_TRUE(ret2 == true);
        } else {
            EXPECT_FALSE(ret2 == false);
        }
    }
}

HWTEST_F_L0(JsStackInfoTest, TranslateByteCodePc)
{
    std::vector<MethodInfo> vec = {
        {0, 0, 24},
        {1, 24, 30},
        {2, 54, 56},
        {3, 120, 60}
    };
    uintptr_t realPc = 115;

    auto ret = TranslateByteCodePc(realPc, vec);
    EXPECT_TRUE(ret != std::nullopt);

    vec.clear();
    ret = TranslateByteCodePc(realPc, vec);
    EXPECT_TRUE(ret == std::nullopt);

    vec.push_back({2, 54, 56});
    ret = TranslateByteCodePc(realPc, vec);
    EXPECT_TRUE(ret != std::nullopt);
}

HWTEST_F_L0(JsStackInfoTest, GetArkNativeFrameInfo)
{
    uintptr_t pc = 0;
    uintptr_t fp = 0;
    uintptr_t sp = 0;
    size_t size = 22;
    JsFrame jsFrame[22];
    bool ret = GetArkNativeFrameInfo(getpid(), &pc, &fp, &sp, &jsFrame, &size);
    EXPECT_FALSE(ret);
    EXPECT_TRUE(size == 22);

    pc = 1234;
    fp = 62480;
    sp = 62496;
    bool ret = GetArkNativeFrameInfo(getpid(), &pc, &fp, &sp, &jsFrame, &size);
    EXPECT_FALSE(ret);
    EXPECT_TRUE(size == 22);
}
}  // namespace panda::test
