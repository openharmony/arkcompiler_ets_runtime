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

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;

namespace panda::test {
class EcmaStringEqualsTest : public BaseTestWithScope<false> {
    public:
        static bool IsUtf8EqualsUtf16UT(const uint8_t *utf8Data, size_t utf8Len,
                                        const uint16_t *utf16Data, uint32_t utf16Len)
        {
            return  EcmaString::IsUtf8EqualsUtf16(utf8Data, utf8Len, utf16Data, utf16Len);
        }
};

/*
* @tc.name: IsUtf8EqualsUtf16
* @tc.desc: Test a function that compares whether an array of UTF8 characters
* is equal to an array of UTF16 characters
* @tc.type: FUNC
*/
HWTEST_F_L0(EcmaStringEqualsTest, IsUtf8EqualsUtf16)
{
    // Test case 1: ASCII characters
    const uint8_t utf8_01[] = "hello";  // "hello" in ASCII is valid UTF-8
    const uint16_t utf16_01[] = {'h', 'e', 'l', 'l', 'o'};
    EXPECT_TRUE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_01, 5, utf16_01, 5));

    // Test case 2: 2-byte UTF-8 sequences
    const uint8_t utf8_02[] = {0xC3, 0xA9, 0xC3, 0xA8}; // "éè" in UTF-8
    const uint16_t utf16_02[] = {0x00E9, 0x00E8}; // "éè" in UTF-16
    EXPECT_TRUE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_02, 4, utf16_02, 2));

    // Test case 3: 3-byte UTF-8 sequences
    const uint8_t utf8_03[] = {0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87}; // "中文" in UTF-8
    const uint16_t utf16_03[] = {0x4E2D, 0x6587}; // "中文" in UTF-16
    EXPECT_TRUE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_03, 6, utf16_03, 2));

    // Test case 4: 4-byte UTF-8 sequences
    const uint8_t utf8_04[] = {0xF0, 0x9F, 0x98, 0x81}; // 😁 in UTF-8
    const uint16_t utf16_04[] = {0xD83D, 0xDE01}; // 😁 in UTF-16 (surrogate pair)
    EXPECT_TRUE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_04, 4, utf16_04, 2));

    // Test case 5: UTF-16 edge cases (empty strings)
    const uint8_t *utf8_05 = nullptr;
    const uint16_t *utf16_05 = nullptr;
    EXPECT_TRUE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_05, 0, utf16_05, 0));

    // Test case 6: UTF-8 shorter than UTF-16
    const uint8_t utf8_06[] = "test"; // "test" in ASCII
    const uint16_t utf16_06[] = {'t', 'e', 's', 't', '!', '!'};
    EXPECT_FALSE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_06, 4, utf16_06, 6));

    // Test case 7: UTF-8 longer than UTF-16
    const uint8_t utf8_07[] = {0xF0, 0x9F, 0x98, 0x81, 0xF0, 0x9F, 0x98, 0x81}; // 😁😁 in UTF-8
    const uint16_t utf16_07[] = {0xD83D, 0xDE01}; // 😁 in UTF-16 (surrogate pair)
    EXPECT_FALSE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_07, 8, utf16_07, 2));

    // Test case 8: Incomplete surrogate pair in UTF-16
    const uint8_t utf8_08[] = {0xF0, 0x9F, 0x92, 0xA9}; // 😩 in UTF-8
    const uint16_t utf16_08[] = {0xD83D}; // Missing low surrogate
    EXPECT_FALSE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_08, 4, utf16_08, 1));

    // Test case 9: Truncated UTF-8 multi-byte character
    const uint8_t utf8_09[] = {0xE3, 0x81}; // Truncated "あ" (Japanese 'a')
    const uint16_t utf16_09[] = {0x3042}; // Full "あ"
    EXPECT_FALSE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_09, 2, utf16_09, 1));

    // Test case 10: Longer UTF-8 sequence matching shorter UTF-16
    const uint8_t utf8_10[] = {0xC2, 0xA3, 0xC2, 0xA3}; // "££" in UTF-8
    const uint16_t utf16_10[] = {0x00A3}; // Single "£"
    EXPECT_FALSE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_10, 4, utf16_10, 1));

    // Test case 11: Handling noncharacters in both UTF-8 and UTF-16
    const uint8_t utf8_11[] = {0xEF, 0xBF, 0xBE}; // UTF-8 noncharacter U+FFFE
    const uint16_t utf16_11[] = {0xFFFE}; // UTF-16 noncharacter
    EXPECT_TRUE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_11, 3, utf16_11, 1));

    // Test case 12: Empty UTF-8 and non-empty UTF-16
    const uint8_t *utf8_12 = nullptr; // Empty UTF-8
    const uint16_t utf16_12[] = {0x0061}; // "a"
    EXPECT_FALSE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_12, 0, utf16_12, 1));

    // Test case 13: Non-empty UTF-8 and empty UTF-16
    const uint8_t utf8_13[] = {0x61}; // "a"
    const uint16_t *utf16_13 = nullptr; // Empty UTF-16
    EXPECT_FALSE(EcmaStringEqualsTest::IsUtf8EqualsUtf16UT(utf8_13, 1, utf16_13, 0));
}
}