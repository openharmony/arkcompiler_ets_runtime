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

#include "common_components/base/c_string.h"
#include "common_components/tests/test_helper.h"

using namespace common;
namespace common::test {
class CStringTest : public common::test::BaseTestWithScope {
};

HWTEST_F_L0(CStringTest, ParseTimeFromEnvTest)
{
    CString env;

    env = "s";
    EXPECT_EQ(CString::ParseTimeFromEnv(env), static_cast<size_t>(0));

    env = "5s";
    EXPECT_EQ(CString::ParseTimeFromEnv(env), static_cast<size_t>(5 * 1000UL * 1000 * 1000));

    env = "10ns";
    EXPECT_EQ(CString::ParseTimeFromEnv(env), static_cast<size_t>(10));

    env = "20us";
    EXPECT_EQ(CString::ParseTimeFromEnv(env), static_cast<size_t>(20 * 1000));

    env = "30ms";
    EXPECT_EQ(CString::ParseTimeFromEnv(env), static_cast<size_t>(30 * 1000 * 1000));

    env = "40xyz";
    EXPECT_EQ(CString::ParseTimeFromEnv(env), static_cast<size_t>(0));

    env = "abcms";
    EXPECT_EQ(CString::ParseTimeFromEnv(env), static_cast<size_t>(0));
}

HWTEST_F_L0(CStringTest, ParseNumFromEnvTest)
{
    CString env;

    env = "123 456";
    EXPECT_EQ(CString::ParseNumFromEnv(env), 123456);

    env = "-123";
    EXPECT_EQ(CString::ParseNumFromEnv(env), -123);

    env = "abc";
    EXPECT_EQ(CString::ParseNumFromEnv(env), 0);

    env = "";
    EXPECT_EQ(CString::ParseNumFromEnv(env), 0);

    env = "+123";
    EXPECT_EQ(CString::ParseNumFromEnv(env), 0);
}

HWTEST_F_L0(CStringTest, ParsePosNumFromEnvTest)
{
    CString env;

    env = "123";
    EXPECT_EQ(CString::ParsePosNumFromEnv(env), static_cast<size_t>(123));

    env = "+456";
    EXPECT_EQ(CString::ParsePosNumFromEnv(env), static_cast<size_t>(456));

    env = "";
    EXPECT_EQ(CString::ParsePosNumFromEnv(env), static_cast<size_t>(0));

    env = "0";
    EXPECT_EQ(CString::ParsePosNumFromEnv(env), static_cast<size_t>(0));

    env = "12a3";
    EXPECT_EQ(CString::ParsePosNumFromEnv(env), static_cast<size_t>(0));

    env = "-123";
    EXPECT_EQ(CString::ParsePosNumFromEnv(env), static_cast<size_t>(0));
}

HWTEST_F_L0(CStringTest, ParsePosDecFromEnvTest)
{
    CString env;

    env = "123.45";
    EXPECT_EQ(CString::ParsePosDecFromEnv(env), 123.45);

    env = "+456.78";
    EXPECT_EQ(CString::ParsePosDecFromEnv(env), 456.78);

    env = "";
    EXPECT_EQ(CString::ParsePosDecFromEnv(env), 0.0);

    env = "0";
    EXPECT_EQ(CString::ParsePosDecFromEnv(env), 0.0);

    env = "12.3.4";
    EXPECT_EQ(CString::ParsePosDecFromEnv(env), 0.0);

    env = "-123.45";
    EXPECT_EQ(CString::ParsePosDecFromEnv(env), 0.0);
}

HWTEST_F_L0(CStringTest, WhileLoopCoverageTest)
{
    CString str = "hello world hello panda";
    CString target = "hello";
    CString replacement = "hi";
    str.ReplaceAll(replacement, target);
    EXPECT_STREQ(str.Str(), "hillo world hillo panda");

    CString str1 = "hello world";
    CString target1 = "notfound";
    CString replacement1 = "replace";
    str1.ReplaceAll(replacement1, target1);
    EXPECT_STREQ(str1.Str(), "hello world");
}

HWTEST_F_L0(CStringTest, RemoveBlankSpaceTest)
{
    CString emptyStr = "";
    CString result1 = emptyStr.RemoveBlankSpace();
    EXPECT_STREQ(result1.Str(), "");

    CString noSpaceStr = "HelloWorld";
    CString result2 = noSpaceStr.RemoveBlankSpace();
    EXPECT_STREQ(result2.Str(), "HelloWorld");

    CString withSpaceStr = "This is a test string";
    CString result3 = withSpaceStr.RemoveBlankSpace();
    EXPECT_STREQ(result3.Str(), "Thisisateststring");

    CString allSpaceStr = "    ";
    CString result4 = allSpaceStr.RemoveBlankSpace();
    EXPECT_STREQ(result4.Str(), "");
}

HWTEST_F_L0(CStringTest, ParseSizeFromEnvTest)
{
    EXPECT_EQ(CString::ParseSizeFromEnv("1"), static_cast<size_t>(0));
    EXPECT_EQ(CString::ParseSizeFromEnv("abcgb"), static_cast<size_t>(0));
    EXPECT_EQ(CString::ParseSizeFromEnv("1024kb"), static_cast<size_t>(1024));
    EXPECT_EQ(CString::ParseSizeFromEnv("5mb"), static_cast<size_t>(5 * 1024));
    EXPECT_EQ(CString::ParseSizeFromEnv("2GB"), static_cast<size_t>(2 * 1024 * 1024));
    EXPECT_EQ(CString::ParseSizeFromEnv("10tb"), static_cast<size_t>(0));
}

HWTEST_F_L0(CStringTest, IsPosDecimalTest)
{
    EXPECT_FALSE(CString::IsPosDecimal(""));
    EXPECT_FALSE(CString::IsPosDecimal("+"));
    EXPECT_FALSE(CString::IsPosDecimal("0"));
    EXPECT_FALSE(CString::IsPosDecimal("abc"));
    EXPECT_FALSE(CString::IsPosDecimal("-1.5"));
    EXPECT_FALSE(CString::IsPosDecimal("0"));
    EXPECT_TRUE(CString::IsPosDecimal("123.45"));
}

HWTEST_F_L0(CStringTest, IsNumberTest)
{
    EXPECT_FALSE(CString::IsNumber(""));
    EXPECT_FALSE(CString::IsNumber("abc"));
    EXPECT_FALSE(CString::IsNumber("12a3"));

    EXPECT_TRUE(CString::IsNumber("123"));
    EXPECT_TRUE(CString::IsNumber("-456"));
}

HWTEST_F_L0(CStringTest, IsPosNumberTest)
{
    EXPECT_FALSE(CString::IsPosNumber(""));
    EXPECT_FALSE(CString::IsPosNumber("+"));
    EXPECT_FALSE(CString::IsPosNumber("0"));
    EXPECT_FALSE(CString::IsPosNumber("abc"));
    EXPECT_FALSE(CString::IsPosNumber("-123"));

    EXPECT_TRUE(CString::IsPosNumber("123"));
    EXPECT_TRUE(CString::IsPosNumber("+123"));
}

HWTEST_F_L0(CStringTest, SubStrTest)
{
    CString str("abcdef");
    EXPECT_EQ(std::string(str.SubStr(2, 3).Str()), "cde");
    EXPECT_EQ(std::string(str.SubStr(2, 0).Str()), "");
    EXPECT_EQ(std::string(str.SubStr(4, 3).Str()), "");
    EXPECT_EQ(std::string(str.SubStr(10).Str()), "");
    EXPECT_EQ(std::string(str.SubStr(2).Str()), "cdef");
}

HWTEST_F_L0(CStringTest, SplitTest)
{
    CString emptySource("");
    auto tokens = CString::Split(emptySource, ',');
    EXPECT_TRUE(tokens.empty());

    CString normalSource("a,b,c");
    tokens = CString::Split(normalSource, ',');
    ASSERT_EQ(tokens.size(), static_cast<size_t>(3));
    EXPECT_EQ(std::string(tokens[0].Str()), "a");
    EXPECT_EQ(std::string(tokens[1].Str()), "b");
    EXPECT_EQ(std::string(tokens[2].Str()), "c");
}

HWTEST_F_L0(CStringTest, FIndandRfindTest)
{
    CString str("hello world");

    EXPECT_EQ(str.Find("hello", 20), -1);
    EXPECT_EQ(str.Find("world", 6), 6);

    EXPECT_EQ(str.Find('h', 20), -1);
    EXPECT_EQ(str.Find('o', 4), 4);

    EXPECT_EQ(str.RFind("xyz"), -1);
    CString multiStr("abababa");
    EXPECT_EQ(multiStr.RFind("aba"), 4);
}

HWTEST_F_L0(CStringTest, TruncateAndInsertTest)
{
    CString str("hello world");

    EXPECT_EQ(&str.Truncate(20), &str);
    EXPECT_EQ(&str.Insert(20, "abc"), &str);
    EXPECT_EQ(&str.Insert(0, ""), &str);
}

HWTEST_F_L0(CStringTest, EnsureSpaceTest)
{
    CString str("hello");

    str.EnsureSpace(0);
    EXPECT_EQ(str.Length(), static_cast<size_t>(5));
    str.EnsureSpace(5);
    EXPECT_EQ(str.Length(), static_cast<size_t>(5));
}

HWTEST_F_L0(CStringTest, ConstructTest)
{
    CString str1(0, 'a');
    EXPECT_EQ(str1.Length(), static_cast<size_t>(0));
    EXPECT_EQ(str1.Str()[0], '\0');

    CString str2(20, 'b');
    EXPECT_EQ(str2.Length(), static_cast<size_t>(20));

    CString src("test string");
    CString dst;
    dst = src;
    EXPECT_STREQ(dst.Str(), "test string");
    EXPECT_EQ(dst.Length(), strlen("test string"));

    CString another("new content");
    another = src;
    EXPECT_STREQ(another.Str(), "test string");

    CString emptyStr;
    CString nonEmptyStr("abc");
    nonEmptyStr = emptyStr;
    EXPECT_EQ(nonEmptyStr.Length(), static_cast<size_t>(0));
    EXPECT_EQ(nonEmptyStr.Str()[0], '\0');
}

HWTEST_F_L0(CStringTest, FormatStringBasicTest)
{
    CString result = CString::FormatString("Hello, %s!", "World");
    EXPECT_STREQ(result.Str(), "Hello, World!");
}

HWTEST_F_L0(CStringTest, FormatString_InvalidArguments_ReturnsError)
{
    CString result = CString::FormatString("%n", nullptr);
    EXPECT_STREQ(result.Str(), "invalid arguments for FormatString");
}

HWTEST_F_L0(CStringTest, InsertMiddle_Success)
{
    CString str("helloworld");
    EXPECT_STREQ(str.Insert(5, ", ").Str(), "hello, world");
}

HWTEST_F_L0(CStringTest, TruncateValidIndex_Success)
{
    CString str("hello world");
    EXPECT_STREQ(str.Truncate(5).Str(), "hello");
    EXPECT_EQ(str.Length(), static_cast<size_t>(5));
}

HWTEST_F_L0(CStringTest, GetStr_NonEmptyString_ReturnsCorrect)
{
    CString str("test string");
    EXPECT_STREQ(str.GetStr(), "test string");
}

HWTEST_F_L0(CStringTest, CombineWithEmptyString_ReturnsOriginal)
{
    CString str("original");
    CString emptyStr;
    CString combined = str.Combine(emptyStr);
    EXPECT_STREQ(combined.Str(), "original");
    EXPECT_EQ(combined.Length(), str.Length());
}

HWTEST_F_L0(CStringTest, CombineWithEmptyCStr_ReturnsOriginal)
{
    CString str("original");
    const char* emptyCStr = "";
    CString combined = str.Combine(emptyCStr);
    EXPECT_STREQ(combined.Str(), "original");
    EXPECT_EQ(combined.Length(), str.Length());
}

HWTEST_F_L0(CStringTest, AppendNullptr_NoChange)
{
    CString str("original");
    str.Append(nullptr, 5);
    EXPECT_STREQ(str.Str(), "original");
    EXPECT_EQ(str.Length(), strlen("original"));
}

HWTEST_F_L0(CStringTest, AppendZeroLength_NoChange)
{
    CString str("test");
    CString emptyStr;

    str.Append(emptyStr, 0);

    EXPECT_STREQ(str.Str(), "test");
    EXPECT_EQ(str.Length(), strlen("test"));
}

HWTEST_F_L0(CStringTest, AppendSelf_ValidResult)
{
    CString str("abc");
    str.Append(str.Str(), str.Length());
    EXPECT_STREQ(str.Str(), "abcabc");
    EXPECT_EQ(str.Length(), strlen("abcabc"));
}

HWTEST_F_L0(CStringTest, AppendEmptyCString_NoChange)
{
    CString str("original");
    CString emptyStr;
    str.Append(emptyStr);
    EXPECT_STREQ(str.Str(), "original");
    EXPECT_EQ(str.Length(), strlen("original"));
}

HWTEST_F_L0(CStringTest, AppendEmptyCStringZeroLength_NoChange)
{
    CString str("test");
    CString emptyStr;

    str.Append(emptyStr, 0);

    EXPECT_STREQ(str.Str(), "test");
    EXPECT_EQ(str.Length(), strlen("test"));
}

HWTEST_F_L0(CStringTest, AppendValidCString_CorrectResult)
{
    CString str("hello");
    CString addStr(" world!");
    str.Append(addStr, strlen(addStr.Str()));
    EXPECT_STREQ(str.Str(), "hello world!");
    EXPECT_EQ(str.Length(), strlen("hello world!"));
}

HWTEST_F_L0(CStringTest, EnsureMultipleCalls_CapacityGrowsCorrectly)
{
    CString str("initial");
    char* firstPtr = str.GetStr();

    str.EnsureSpace(16);
    char* secondPtr = str.GetStr();

    EXPECT_NE(firstPtr, secondPtr);

    str.EnsureSpace(100);
    char* thirdPtr = str.GetStr();

    EXPECT_NE(secondPtr, thirdPtr);
    EXPECT_EQ(str.Length(), strlen("initial"));
    EXPECT_STREQ(str.SubStr(0, str.Length()).Str(), "initial");
}

HWTEST_F_L0(CStringTest, CStringSubscriptOperatorTest)
{
    const CString constStr("hello world");
    EXPECT_EQ(constStr[0], 'h');

    CString mutableStr("mutable");
    mutableStr[0] = 'M';
    EXPECT_EQ(mutableStr[0], 'M');
}
}