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
#include "common_components/platform/string_hash.h"
#include "common_components/tests/test_helper.h"

using namespace common;

namespace common::test {
class BaseStringTest : public common::test::BaseTestWithScope {
};

HWTEST_F_L0(BaseStringTest, ComputeHashForData_TEST0)
{
    const uint8_t data[] = {'a', 'b', 'c'};
    size_t size = sizeof(data) / sizeof(data[0]);
    uint32_t hashSeed = 0;
    uint32_t expectedHash = hashSeed;
    for (uint32_t i = 0; i < size; ++i) {
        expectedHash = (expectedHash << static_cast<uint32_t>(StringHash::HASH_SHIFT)) - expectedHash + data[i];
    }

    EXPECT_EQ(BaseString::ComputeHashForData(data, size, hashSeed), expectedHash);
}

HWTEST_F_L0(BaseStringTest, ComputeHashForData_TEST1)
{
    std::vector<uint8_t> largeData(1000, 'x');
    size_t size = largeData.size();
    uint32_t hashSeed = 0;

    uint32_t result = BaseString::ComputeHashForData(largeData.data(), size, hashSeed);
    EXPECT_NE(result, 0);
}

HWTEST_F_L0(BaseStringTest, ComputeHashcodeUtf8_TEST0)
{
    const uint8_t utf8Data[] = u8"hello";
    size_t utf8Len = sizeof(utf8Data) / sizeof(utf8Data[0]) - 1;

    uint32_t expectedHash = 0;
    for (uint32_t i = 0; i < utf8Len; ++i) {
        expectedHash = (expectedHash << static_cast<uint32_t>(StringHash::HASH_SHIFT)) - expectedHash + utf8Data[i];
    }

    EXPECT_EQ(BaseString::ComputeHashcodeUtf8(utf8Data, utf8Len, true), expectedHash);
}

HWTEST_F_L0(BaseStringTest, ComputeHashcodeUtf8_TEST1)
{
    const uint8_t utf8Data[] = u8"hello";
    size_t utf8Len = sizeof(utf8Data) / sizeof(utf8Data[0]) - 1;

    uint32_t expectedHash = 0;
    for (uint32_t i = 0; i < utf8Len; ++i) {
        expectedHash = (expectedHash << static_cast<uint32_t>(StringHash::HASH_SHIFT)) - expectedHash + utf8Data[i];
    }

    EXPECT_EQ(BaseString::ComputeHashcodeUtf8(utf8Data, utf8Len, false), expectedHash);
}

HWTEST_F_L0(BaseStringTest, IsASCIICharacter_TEST0)
{
    const uint16_t num = 0;
    bool result = BaseString::IsASCIICharacter(num);
    ASSERT_FALSE(result);
}

HWTEST_F_L0(BaseStringTest, IsASCIICharacter_TEST1)
{
    const uint16_t num = 0x7f;
    bool result = BaseString::IsASCIICharacter(num);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(BaseStringTest, CanBeCompressed_TEST0)
{
    uint8_t data[] = {};
    EXPECT_TRUE(BaseString::CanBeCompressed(data, 0));

    uint8_t data1[] = {1, 1, 1, 1, 0};
    ASSERT_FALSE(BaseString::CanBeCompressed(data1, 5));
    ASSERT_TRUE(BaseString::CanBeCompressed(data1, 2));

    uint8_t data2[] = {'a', 'b', 'c', 'd'};
    ASSERT_TRUE(BaseString::CanBeCompressed(data2, 4));

    uint8_t data3[] = {'a', '\0', 'c', 'd'};
    ASSERT_FALSE(BaseString::CanBeCompressed(data3, 4));
}

HWTEST_F_L0(BaseStringTest, CanBeCompressed_TEST1)
{
    uint16_t data[] = {};
    EXPECT_TRUE(BaseString::CanBeCompressed(data, 0));

    uint16_t data1[] = {1, 1, 1, 1, 0};
    ASSERT_FALSE(BaseString::CanBeCompressed(data1, 5));
    ASSERT_TRUE(BaseString::CanBeCompressed(data1, 2));

    uint16_t data2[] = {'a', 'b', 'c', 'd'};
    ASSERT_TRUE(BaseString::CanBeCompressed(data2, 4));

    uint16_t data3[] = {'a', '\0', 'c', 'd'};
    ASSERT_FALSE(BaseString::CanBeCompressed(data3, 4));
}

HWTEST_F_L0(BaseStringTest, IndexOf_TEST0)
{
    const uint8_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs[] = {'a', 'b', 'c'};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 3);

    EXPECT_EQ(BaseString::IndexOf(lhsSp, rhsSp, 0, 4), 0);
}

HWTEST_F_L0(BaseStringTest, IndexOf_TEST1)
{
    const uint8_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs[] = {'c', 'd'};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 2);

    EXPECT_EQ(BaseString::IndexOf(lhsSp, rhsSp, 0, 4), 2);
}

HWTEST_F_L0(BaseStringTest, IndexOf_TEST2)
{
    const uint8_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs[] = {'x', 'y', 'z'};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 3);

    EXPECT_EQ(BaseString::IndexOf(lhsSp, rhsSp, 0, 4), -1);
}

HWTEST_F_L0(BaseStringTest, IndexOf_TEST3)
{
    const uint8_t lhs[] = {'a', 'b', 'a', 'b', 'c'};
    const uint8_t rhs[] = {'a', 'b', 'c'};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 3);

    EXPECT_EQ(BaseString::IndexOf(lhsSp, rhsSp, 0, 4), 2);
}

HWTEST_F_L0(BaseStringTest, IndexOf_TEST4)
{
    const uint8_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs[] = {'a', 'b', 'x'};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 3);

    EXPECT_EQ(BaseString::IndexOf(lhsSp, rhsSp, 0, 4), -1);
}

HWTEST_F_L0(BaseStringTest, CompareStringSpan_TEST0)
{
    const uint8_t lhs[] = {1, 2, 3};
    const uint8_t rhs[] = {1, 2, 3};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 3);
    EXPECT_EQ(CompareStringSpan(lhsSp, rhsSp, 3), 0);
}

HWTEST_F_L0(BaseStringTest, CompareStringSpan_TEST1)
{
    const uint8_t lhs[] = {1, 2, 4};
    const uint8_t rhs[] = {1, 2, 3};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 3);
    EXPECT_EQ(CompareStringSpan(lhsSp, rhsSp, 3), 1);
}

HWTEST_F_L0(BaseStringTest, IsSubStringAtSpan_TEST1)
{
    const uint8_t lhs[] = {'a', 'b', 'c'};
    const uint8_t rhs[] = {'x', 'y'};
    Span<const uint8_t> lhsSp(lhs, 3);
    Span<const uint8_t> rhsSp(rhs, 2);
    ASSERT_FALSE(IsSubStringAtSpan(lhsSp, rhsSp, 1));
}

HWTEST_F_L0(BaseStringTest, IsSubStringAtSpan_TEST2)
{
    const uint8_t lhs[] = {'a', 'b'};
    const uint8_t rhs[] = {'b'};
    Span<const uint8_t> lhsSp(lhs, 2);
    Span<const uint8_t> rhsSp(rhs, 1);
    ASSERT_TRUE(IsSubStringAtSpan(lhsSp, rhsSp, 1));
}

HWTEST_F_L0(BaseStringTest, IndexOf_TEST5)
{
    const uint8_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs[] = {'a', 'b', 'c'};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint16_t> rhsSp(rhs, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp, rhsSp, 0, 4), 0);

    const uint8_t lhs1[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs1[] = {'c', 'd'};
    Span<const uint8_t> lhsSp1(lhs1, 5);
    Span<const uint16_t> rhsSp1(rhs1, 2);
    EXPECT_EQ(BaseString::IndexOf(lhsSp1, rhsSp1, 0, 4), 2);

    const uint8_t lhs2[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs2[] = {'x', 'y', 'z'};
    Span<const uint8_t> lhsSp2(lhs2, 5);
    Span<const uint16_t> rhsSp2(rhs2, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp2, rhsSp2, 0, 4), -1);

    const uint8_t lhs3[] = {'a', 'b', 'a', 'b', 'c'};
    const uint16_t rhs3[] = {'a', 'b', 'c'};
    Span<const uint8_t> lhsSp3(lhs3, 5);
    Span<const uint16_t> rhsSp3(rhs3, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp3, rhsSp3, 0, 4), 2);

    const uint8_t lhs4[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs4[] = {'a', 'b', 'x'};
    Span<const uint8_t> lhsSp4(lhs4, 5);
    Span<const uint16_t> rhsSp4(rhs4, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp4, rhsSp4, 0, 4), -1);
}

HWTEST_F_L0(BaseStringTest, IndexOf_TEST6)
{
    const uint16_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs[] = {'a', 'b', 'c'};
    Span<const uint16_t> lhsSp(lhs, 5);
    Span<const uint16_t> rhsSp(rhs, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp, rhsSp, 0, 4), 0);

    const uint16_t lhs1[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs1[] = {'c', 'd'};
    Span<const uint16_t> lhsSp1(lhs1, 5);
    Span<const uint16_t> rhsSp1(rhs1, 2);
    EXPECT_EQ(BaseString::IndexOf(lhsSp1, rhsSp1, 0, 4), 2);

    const uint16_t lhs2[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs2[] = {'x', 'y', 'z'};
    Span<const uint16_t> lhsSp2(lhs2, 5);
    Span<const uint16_t> rhsSp2(rhs2, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp2, rhsSp2, 0, 4), -1);

    const uint16_t lhs3[] = {'a', 'b', 'a', 'b', 'c'};
    const uint16_t rhs3[] = {'a', 'b', 'c'};
    Span<const uint16_t> lhsSp3(lhs3, 5);
    Span<const uint16_t> rhsSp3(rhs3, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp3, rhsSp3, 0, 4), 2);

    const uint16_t lhs4[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs4[] = {'a', 'b', 'x'};
    Span<const uint16_t> lhsSp4(lhs4, 5);
    Span<const uint16_t> rhsSp4(rhs4, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp4, rhsSp4, 0, 4), -1);
}

HWTEST_F_L0(BaseStringTest, IndexOf_TEST7)
{
    const uint16_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs[] = {'a', 'b', 'c'};
    Span<const uint16_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp, rhsSp, 0, 4), 0);

    const uint16_t lhs1[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs1[] = {'c', 'd'};
    Span<const uint16_t> lhsSp1(lhs1, 5);
    Span<const uint8_t> rhsSp1(rhs1, 2);
    EXPECT_EQ(BaseString::IndexOf(lhsSp1, rhsSp1, 0, 4), 2);

    const uint16_t lhs2[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs2[] = {'x', 'y', 'z'};
    Span<const uint16_t> lhsSp2(lhs2, 5);
    Span<const uint8_t> rhsSp2(rhs2, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp2, rhsSp2, 0, 4), -1);

    const uint16_t lhs3[] = {'a', 'b', 'a', 'b', 'c'};
    const uint8_t rhs3[] = {'a', 'b', 'c'};
    Span<const uint16_t> lhsSp3(lhs3, 5);
    Span<const uint8_t> rhsSp3(rhs3, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp3, rhsSp3, 0, 4), 2);

    const uint16_t lhs4[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs4[] = {'a', 'b', 'x'};
    Span<const uint16_t> lhsSp4(lhs4, 5);
    Span<const uint8_t> rhsSp4(rhs4, 3);
    EXPECT_EQ(BaseString::IndexOf(lhsSp4, rhsSp4, 0, 4), -1);
}

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

HWTEST_F_L0(BaseStringTest, IsSubStringAtSpan_TEST3)
{
    const uint8_t lhs[] = {'a', 'b', 'c'};
    const uint16_t rhs[] = {'x', 'y'};
    Span<const uint8_t> lhsSp(lhs, 3);
    Span<const uint16_t> rhsSp(rhs, 2);
    ASSERT_FALSE(IsSubStringAtSpan(lhsSp, rhsSp, 1));

    const uint8_t lhs1[] = {'a', 'b'};
    const uint16_t rhs1[] = {'b'};
    Span<const uint8_t> lhsSp1(lhs1, 2);
    Span<const uint16_t> rhsSp1(rhs1, 1);
    ASSERT_TRUE(IsSubStringAtSpan(lhsSp1, rhsSp1, 1));
}

HWTEST_F_L0(BaseStringTest, IsSubStringAtSpan_TEST4)
{
    const uint16_t lhs[] = {'a', 'b', 'c'};
    const uint8_t rhs[] = {'x', 'y'};
    Span<const uint16_t> lhsSp(lhs, 3);
    Span<const uint8_t> rhsSp(rhs, 2);
    ASSERT_FALSE(IsSubStringAtSpan(lhsSp, rhsSp, 1));

    const uint16_t lhs1[] = {'a', 'b'};
    const uint8_t rhs1[] = {'b'};
    Span<const uint16_t> lhsSp1(lhs1, 2);
    Span<const uint8_t> rhsSp1(rhs1, 1);
    ASSERT_TRUE(IsSubStringAtSpan(lhsSp1, rhsSp1, 1));
}

HWTEST_F_L0(BaseStringTest, IsSubStringAtSpan_TEST5)
{
    const uint16_t lhs[] = {'a', 'b', 'c'};
    const uint16_t rhs[] = {'x', 'y'};
    Span<const uint16_t> lhsSp(lhs, 3);
    Span<const uint16_t> rhsSp(rhs, 2);
    ASSERT_FALSE(IsSubStringAtSpan(lhsSp, rhsSp, 1));

    const uint16_t lhs1[] = {'a', 'b'};
    const uint16_t rhs1[] = {'b'};
    Span<const uint16_t> lhsSp1(lhs1, 2);
    Span<const uint16_t> rhsSp1(rhs1, 1);
    ASSERT_TRUE(IsSubStringAtSpan(lhsSp1, rhsSp1, 1));
}

HWTEST_F_L0(BaseStringTest, LastIndexOf_TEST5)
{
    const uint8_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs[] = {'c', 'd'};
    Span<const uint8_t> lhsSp(lhs, 5);
    Span<const uint16_t> rhsSp(rhs, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp, rhsSp, 4), 2);

    const uint8_t lhs1[] = {'a', 'b', 'c', 'x', 'e'};
    const uint16_t rhs1[] = {'c', 'd'};
    Span<const uint8_t> lhsSp1(lhs1, 5);
    Span<const uint16_t> rhsSp1(rhs1, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp1, rhsSp1, 4), -1);

    const uint8_t lhs2[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs2[] = {'c', 'x'};
    Span<const uint8_t> lhsSp2(lhs2, 5);
    Span<const uint16_t> rhsSp2(rhs2, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp2, rhsSp2, 4), -1);

    const uint8_t lhs3[] = {'a', 'b', 'a', 'b', 'c'};
    const uint16_t rhs3[] = {'a', 'b'};
    Span<const uint8_t> lhsSp3(lhs3, 5);
    Span<const uint16_t> rhsSp3(rhs3, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp3, rhsSp3, 4), 2);

    const uint8_t lhs4[] = {'a', 'b', 'c', 'd'};
    const uint16_t rhs4[] = {'x', 'y'};
    Span<const uint8_t> lhsSp4(lhs4, 4);
    Span<const uint16_t> rhsSp4(rhs4, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp4, rhsSp4, 3), -1);
}

HWTEST_F_L0(BaseStringTest, LastIndexOf_TEST6)
{
    const uint16_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs[] = {'c', 'd'};
    Span<const uint16_t> lhsSp(lhs, 5);
    Span<const uint16_t> rhsSp(rhs, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp, rhsSp, 4), 2);

    const uint16_t lhs1[] = {'a', 'b', 'c', 'x', 'e'};
    const uint16_t rhs1[] = {'c', 'd'};
    Span<const uint16_t> lhsSp1(lhs1, 5);
    Span<const uint16_t> rhsSp1(rhs1, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp1, rhsSp1, 4), -1);

    const uint16_t lhs2[] = {'a', 'b', 'c', 'd', 'e'};
    const uint16_t rhs2[] = {'c', 'x'};
    Span<const uint16_t> lhsSp2(lhs2, 5);
    Span<const uint16_t> rhsSp2(rhs2, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp2, rhsSp2, 4), -1);

    const uint16_t lhs3[] = {'a', 'b', 'a', 'b', 'c'};
    const uint16_t rhs3[] = {'a', 'b'};
    Span<const uint16_t> lhsSp3(lhs3, 5);
    Span<const uint16_t> rhsSp3(rhs3, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp3, rhsSp3, 4), 2);

    const uint16_t lhs4[] = {'a', 'b', 'c', 'd'};
    const uint16_t rhs4[] = {'x', 'y'};
    Span<const uint16_t> lhsSp4(lhs4, 4);
    Span<const uint16_t> rhsSp4(rhs4, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp4, rhsSp4, 3), -1);
}

HWTEST_F_L0(BaseStringTest, LastIndexOf_TEST7)
{
    const uint16_t lhs[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs[] = {'c', 'd'};
    Span<const uint16_t> lhsSp(lhs, 5);
    Span<const uint8_t> rhsSp(rhs, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp, rhsSp, 4), 2);

    const uint16_t lhs1[] = {'a', 'b', 'c', 'x', 'e'};
    const uint8_t rhs1[] = {'c', 'd'};
    Span<const uint16_t> lhsSp1(lhs1, 5);
    Span<const uint8_t> rhsSp1(rhs1, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp1, rhsSp1, 4), -1);

    const uint16_t lhs2[] = {'a', 'b', 'c', 'd', 'e'};
    const uint8_t rhs2[] = {'c', 'x'};
    Span<const uint16_t> lhsSp2(lhs2, 5);
    Span<const uint8_t> rhsSp2(rhs2, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp2, rhsSp2, 4), -1);

    const uint16_t lhs3[] = {'a', 'b', 'a', 'b', 'c'};
    const uint8_t rhs3[] = {'a', 'b'};
    Span<const uint16_t> lhsSp3(lhs3, 5);
    Span<const uint8_t> rhsSp3(rhs3, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp3, rhsSp3, 4), 2);

    const uint16_t lhs4[] = {'a', 'b', 'c', 'd'};
    const uint8_t rhs4[] = {'x', 'y'};
    Span<const uint16_t> lhsSp4(lhs4, 4);
    Span<const uint8_t> rhsSp4(rhs4, 2);
    EXPECT_EQ(BaseString::LastIndexOf(lhsSp4, rhsSp4, 3), -1);
}
}  // namespace common::test
