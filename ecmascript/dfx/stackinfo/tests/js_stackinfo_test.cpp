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

    for (int i = 0; i < 22; i++) {
        bool ret1 = ArkFrameCheck(frame[i]);
        if (i == 1 || i == 17) {
            EXPECT_TRUE(ret1 == true);
        } else {
            EXPECT_TRUE(ret1 == false);
        }
        bool ret2 = IsFunctionFrame(frame[i]);
        if (i == 2 || i == 3 || i == 8 || i == 9 || i == 10 || i == 15) {
            EXPECT_TRUE(ret2 == true);
        } else {
            EXPECT_TRUE(ret2 == false);
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
    if (sizeof(uintptr_t) == sizeof(uint32_t)) {  // 32 bit
        // The frame structure is different between 32 bit and 64 bit.
        // Skip 32 bit because there is no ArkJS Heap.
        return;
    }

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

    JSTaggedType frame1[3];  // 3: size of ASM_INTERPRETER_ENTRY_FRAME
    frame1[0] = pc;  // 0: pc
    frame1[1] = 62480;  // 1: base.prev
    frame1[2] = static_cast<JSTaggedType>(FrameType::ASM_INTERPRETER_ENTRY_FRAME);  // 2: base.type
    uintptr_t fp_frame1 = reinterpret_cast<uintptr_t>(&frame3[3]);  // 3: bottom of frame

    JSTaggedType frame2[9];  // 9: size of AsmInterpretedFrame
    frame2[0] = JSTaggedValue::Hole().GetRawData();  // 0: function
    frame2[1] = JSTaggedValue::Hole().GetRawData();  // 1: thisObj
    frame2[2] = JSTaggedValue::Hole().GetRawData();  // 2: acc
    frame2[3] = JSTaggedValue::Hole().GetRawData();  // 3: env
    frame2[4] = static_cast<JSTaggedType>(0);  // 4: callSize
    frame2[5] = static_cast<JSTaggedType>(0);  // 5: fp
    frame2[6] = reinterpret_cast<JSTaggedType>(&bytecode[2]);  // 6: pc
    frame2[7] = fp_frame1;  // 7: base.prev
    frame2[8] = static_cast<JSTaggedType>(FrameType::ASM_INTERPRETER_FRAME);  // 8: base.type
    uintptr_t fp_frame2 = reinterpret_cast<uintptr_t>(&frame[9]);  // 9: bottom of frame

    JSTaggedType frame3[6];  // 6: size of BUILTIN_FRAME
    frame3[0] = JSTaggedValue::Hole().GetRawData();  // 0: stackArgs
    frame3[1] = JSTaggedValue::Hole().GetRawData();  // 1:numArgs
    frame3[2] = JSTaggedValue::Hole().GetRawData();  // 2: thread
    frame3[3] = JSTaggedValue::Hole().GetRawData();  // 3: returnAddr
    frame3[4] = fp_frame2;  // 4: prevFp
    frame3[5] = static_cast<JSTaggedType>(FrameType::BUILTIN_FRAME);  // 5: type
    uintptr_t fp_frame3 = reinterpret_cast<uintptr_t>(&frame1[6]);  // 6: bottom of frame
    fp = fp_frame3;
    bool ret = GetArkNativeFrameInfo(getpid(), &pc, &fp, &sp, &jsFrame, &size);
    EXPECT_TRUE(ret);
    EXPECT_TRUE(sp = &fp_frame1);
    EXPECT_TRUE(pc = 1234);
    EXPECT_TRUE(fp = 62480);
}
}  // namespace panda::test
