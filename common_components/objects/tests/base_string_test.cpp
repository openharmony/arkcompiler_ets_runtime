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

#include "common_interfaces/objects/base_string.h"
#include "common_components/tests/test_helper.h"

using namespace common;

namespace common::test {
class BaseStringTest : public common::test::BaseTestWithScope {
};

HWTEST_F_L0(BaseStringTest, IsUtf8EqualsUtf16_TEST0)
{
    const uint8_t utf8_01[] = {0xF0, 0xE0, 0xC0};
    const uint16_t utf16_01[] = {'h', 'e', 'l', 'l', 'o'};
    EXPECT_FALSE(BaseString::IsUtf8EqualsUtf16(utf8_01, 3, utf16_01, 5));

    const uint8_t utf8_02[] = {0xF0, 0x90, 0x80, 0x80};
    const uint16_t utf16_02[] = {0xD801, 0xDC00};
    EXPECT_FALSE(BaseString::IsUtf8EqualsUtf16(utf8_02, 4, utf16_02, 2));

    const uint8_t utf8_03[] = {0xF0, 0x90, 0x80, 0x80};
    const uint16_t utf16_03[] = {0xD800, 0xDC01};
    EXPECT_FALSE(BaseString::IsUtf8EqualsUtf16(utf8_03, 4, utf16_03, 2));

    const uint8_t utf8_04[] = {0xE0, 0xA0, 0x80};
    const uint16_t utf16_04[] = {0x0801};
    EXPECT_FALSE(BaseString::IsUtf8EqualsUtf16(utf8_04, 3, utf16_04, 1));

    const uint8_t utf8_05[] = {0xC3, 0xA9};
    const uint16_t utf16_05[] = {0x00EA};
    EXPECT_FALSE(BaseString::IsUtf8EqualsUtf16(utf8_05, 2, utf16_05, 1));

    const uint8_t utf8_06[] = {'A'};
    const uint16_t utf16_06[] = {'B'};
    EXPECT_FALSE(BaseString::IsUtf8EqualsUtf16(utf8_06, 1, utf16_06, 1));

    const uint8_t utf8_07[] = {0xF0, 0x90, 0x80, 0x80};
    const uint16_t utf16_07[] = {0xD800};
    EXPECT_FALSE(BaseString::IsUtf8EqualsUtf16(utf8_07, 4, utf16_07, 1));

    const uint8_t utf8_08[] = {0xF0, 0x8F, 0xBF, 0xBF};
    const uint16_t utf16_08[] = {0xFFFE};
    EXPECT_FALSE(BaseString::IsUtf8EqualsUtf16(utf8_08, 4, utf16_08, 1));
}

HWTEST_F_L0(BaseStringTest, LastIndexOf_TEST0)
{
    const uint8_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs[] = {'c', 'd'};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 2);

    EXPECT_EQ(BaseString::LastIndexOf(lhsSp, rhsSp, 4), 2);
}

HWTEST_F_L0(BaseStringTest, LastIndexOf_TEST1)
{
    const uint8_t lhs[] = {'a', 'b', 'c', 'x', 'e'};
    const uint8_t rhs[] = {'c', 'd'};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 2);

    EXPECT_EQ(BaseString::LastIndexOf(lhsSp, rhsSp, 4), -1);
}

HWTEST_F_L0(BaseStringTest, LastIndexOf_TEST2)
{
    const uint8_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs[] = {'c', 'x'};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 2);

    EXPECT_EQ(BaseString::LastIndexOf(lhsSp, rhsSp, 4), -1);
}

HWTEST_F_L0(BaseStringTest, LastIndexOf_TEST3)
{
    const uint8_t lhs[] = {'a', 'b', 'a', 'b', 'c'};
    const uint8_t rhs[] = {'a', 'b'};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 2);

    EXPECT_EQ(BaseString::LastIndexOf(lhsSp, rhsSp, 4), 2);
}

HWTEST_F_L0(BaseStringTest, LastIndexOf_TEST4)
{
    const uint8_t lhs[] = {'a', 'b', 'c', 'd'};
    const uint8_t rhs[] = {'x', 'y'};
    Span<const uint8_t> lhsSp(lhs, 4);
    Span<const uint8_t> rhsSp(rhs, 2);

    EXPECT_EQ(BaseString::LastIndexOf(lhsSp, rhsSp, 3), -1);
}
}  // namespace common::test
