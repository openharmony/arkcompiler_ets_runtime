/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class FrameTest : public testing::Test {
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
    }

    void TearDown() override
    {
    }
};

/*
 * @tc.name: GetArkJSHeapCrashInfoTest
 * @tc.desc: Test API get_ark_js_heap_crash_info
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(FrameTest, GetArkJSHeapCrashInfoTest)
{
    if (sizeof(uintptr_t) == sizeof(uint32_t)) {  // 32 bit
        // The frame structure is different between 32 bit and 64 bit.
        // Skip 32 bit because there is no ArkJS Heap.
        return;
    }
    uint8_t bytecode[] = {
        12,  // 12: HandleMul2Imm8V8
        34,  // 34: HandleDecImm8
        56,  // 56: HandleStobjbyvalueImm8V8V8
        78,  // 78: HandleJmpImm16
        90,  // 90: HandleJstrictequndefinedImm8
        254, // 254: HandleThrow
        8,   // 8: HandleThrowIfsupernotcorrectcallPrefImm16
    };
    JSTaggedType frame[9];  // 9: size of AsmInterpretedFrame
    frame[0] = JSTaggedValue::Hole().GetRawData();  // 0: function
    frame[1] = JSTaggedValue::Hole().GetRawData();  // 1: thisObj
    frame[2] = JSTaggedValue::Hole().GetRawData();  // 2: acc
    frame[3] = JSTaggedValue::Hole().GetRawData();  // 3: env
    frame[4] = static_cast<JSTaggedType>(0);  // 4: callSize
    frame[5] = static_cast<JSTaggedType>(0);  // 5: fp
    frame[6] = reinterpret_cast<JSTaggedType>(&bytecode[2]);  // 6: pc
    frame[7] = static_cast<JSTaggedType>(0);  // 7: base.prev
    frame[8] = static_cast<JSTaggedType>(FrameType::ASM_INTERPRETER_FRAME);  // 8: base.type
    uintptr_t x20 = reinterpret_cast<uintptr_t>(&bytecode[5]);  // 5: HandleThrow offset
    uintptr_t fp = reinterpret_cast<uintptr_t>(&frame[9]);  // 9: bottom of frame
    char buf[256];  // 256: buf size
    int result = get_ark_js_heap_crash_info(getpid(), &x20, &fp, 0, &buf[0], 256);  // 256: buf size
    EXPECT_EQ(result, 1);
    EXPECT_EQ(std::string(buf), std::string("RegisterBytecode:HandleThrow/HandleThrowIfsupernotcorrectcallPrefImm16") +
                                std::string(" FrameBytecode:HandleStobjbyvalueImm8V8V8"));
}
}  // namespace panda::ecmascript
