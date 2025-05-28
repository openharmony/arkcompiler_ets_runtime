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

#include "ecmascript/base/json_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class JsonHelperTest : public BaseTestWithScope<false> {
};

#if ENABLE_NEXT_OPTIMIZATION
/**
 * @tc.name: IsFastValueToQuotedString
 * @tc.desc: Check basic Ascii characters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonHelperTest, IsFastValueToQuotedString_001)
{
    Span<const uint8_t> sp1(reinterpret_cast<const uint8_t*>("Hello World"), 11);
    Span<const uint8_t> sp2(reinterpret_cast<const uint8_t*>("0123456789"), 10);
    Span<const uint8_t> sp3(reinterpret_cast<const uint8_t*>("!#$%&'()*+,-./:;=?@[]^_`{|}~"), 28);
    EXPECT_TRUE(JsonHelper::IsFastValueToQuotedString(sp1));
    EXPECT_TRUE(JsonHelper::IsFastValueToQuotedString(sp2));
    EXPECT_TRUE(JsonHelper::IsFastValueToQuotedString(sp3));
}

/**
 * @tc.name: IsFastValueToQuotedString
 * @tc.desc: Check Ascii characters need to escape, including Control characters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonHelperTest, IsFastValueToQuotedString_002)
{
    Span<const uint8_t> sp1(reinterpret_cast<const uint8_t*>("\""), 1);
    Span<const uint8_t> sp2(reinterpret_cast<const uint8_t*>("\\"), 1);
    Span<const uint8_t> sp3(reinterpret_cast<const uint8_t*>("\b"), 1);
    Span<const uint8_t> sp4(reinterpret_cast<const uint8_t*>("\f"), 1);
    Span<const uint8_t> sp5(reinterpret_cast<const uint8_t*>("\n"), 1);
    Span<const uint8_t> sp6(reinterpret_cast<const uint8_t*>("\r"), 1);
    Span<const uint8_t> sp7(reinterpret_cast<const uint8_t*>("\t"), 1);
    EXPECT_FALSE(JsonHelper::IsFastValueToQuotedString(sp1));
    EXPECT_FALSE(JsonHelper::IsFastValueToQuotedString(sp2));
    EXPECT_FALSE(JsonHelper::IsFastValueToQuotedString(sp3));
    EXPECT_FALSE(JsonHelper::IsFastValueToQuotedString(sp4));
    EXPECT_FALSE(JsonHelper::IsFastValueToQuotedString(sp5));
    EXPECT_FALSE(JsonHelper::IsFastValueToQuotedString(sp6));
    EXPECT_FALSE(JsonHelper::IsFastValueToQuotedString(sp7));


    for (uint8_t c = 0; c < 32; c++) {
        EXPECT_FALSE(JsonHelper::IsFastValueToQuotedString(
          Span<const uint8_t>(reinterpret_cast<const uint8_t*>(&c), 1)));
    }
}

/**
 * @tc.name: IsFastValueToQuotedString
 * @tc.desc: Check mixed content
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonHelperTest, IsFastValueToQuotedString_003)
{
    Span<const uint8_t> sp1(reinterpret_cast<const uint8_t*>("Hello\nWorld"), 11);
    Span<const uint8_t> sp2(reinterpret_cast<const uint8_t*>("Test\"Quote"), 10);
    Span<const uint8_t> sp3(reinterpret_cast<const uint8_t*>("Test\\BackSlash"), 14);

    EXPECT_FALSE(JsonHelper::IsFastValueToQuotedString(sp1));
    EXPECT_FALSE(JsonHelper::IsFastValueToQuotedString(sp2));
    EXPECT_FALSE(JsonHelper::IsFastValueToQuotedString(sp3));
}
#endif
}
