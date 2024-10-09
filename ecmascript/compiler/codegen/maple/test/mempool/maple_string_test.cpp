/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include <iostream>
#include "maple_string.h"
#include "securec.h"

using namespace maple;
using namespace std;
namespace {
TEST(find_FUNC, t01)
{
    MemPoolCtrler memPoolCtrler1, memPoolCtrler2;
    MemPool memPool1(memPoolCtrler1, "pn1"), memPool2(memPoolCtrler2, "pn2");
    const char *testStr = "ms_obj";
    const char *findCase1 = "ob";
    MapleString ms_obj(testStr, &memPool1), findCase2("ob", &memPool2);
    size_t ans;

    ans = ms_obj.find(findCase1, 0); // 3
    EXPECT_EQ(ans, 3);
    ans = ms_obj.find(nullptr, 0); // inf
    EXPECT_EQ(std::string::npos, ans);
    ans = ms_obj.find(findCase1, 8000); // inf
    EXPECT_EQ(std::string::npos, ans);
    ans = ms_obj.find(findCase1, 4); // inf
    EXPECT_EQ(std::string::npos, ans);
    ans = ms_obj.find(findCase2, 0); // 3
    EXPECT_EQ(ans, 3);
    ans = ms_obj.find(findCase2, 8000); // inf
    EXPECT_EQ(std::string::npos, ans);
    ans = ms_obj.find(findCase2, 4); // inf
    EXPECT_EQ(std::string::npos, ans);
}

TEST(find_last_of_FUNC, t02)
{
    // Search for the last occurrence of the target string
    MemPoolCtrler memPoolCtrler1;
    MemPool memPool1(memPoolCtrler1, "pn1");
    const char *testStr = "ms_obj_ABCDDABC";
    const char *findCase1 = "ABC";
    const char *findCase2 = "ABCD";
    MapleString ms_obj(testStr, &memPool1);
    size_t ans;

    ans = ms_obj.find_last_of(findCase1, 0);
    EXPECT_EQ(ans, 12);
    ans = ms_obj.find_last_of(nullptr, 0);
    EXPECT_EQ(std::string::npos, ans);
    ans = ms_obj.find_last_of(findCase1, 13);
    EXPECT_EQ(std::string::npos, ans);
    ans = ms_obj.find_last_of(findCase2, 0);
    EXPECT_EQ(ans, 7);
    ans = ms_obj.find_last_of("XY", 0);
    EXPECT_EQ(std::string::npos, ans);
}

TEST(find_FUNC, t03)
{
    MemPoolCtrler memPoolCtrler1;
    MemPool memPool1(memPoolCtrler1, "pn1");
    const char *testStr = "ms_ob_obj";
    const char *findCase1 = "obj";
    MapleString ms_obj(testStr, &memPool1);
    size_t ans;
    ans = ms_obj.find(findCase1, 0, 2); // 3
    EXPECT_EQ(ans, 3);
    ans = ms_obj.find(findCase1, 0, 3); // 6
    EXPECT_EQ(ans, 6);
    ans = ms_obj.find(findCase1, 0, 8000); // inf
    EXPECT_EQ(ans, std::string::npos);
    ans = ms_obj.find(nullptr, 0, 1); // inf
    EXPECT_EQ(ans, std::string::npos);
}

TEST(find_FUNC, t04)
{ // test method:find(char c, size_t pos)
    MemPoolCtrler memPoolCtrler1;
    MemPool memPool1(memPoolCtrler1, "pn1");
    const char *testStr = "ms_obj";
    char ch = 'o';
    MapleString ms_obj(testStr, &memPool1);
    size_t ans;
    ans = ms_obj.find(ch, 0); // 3
    EXPECT_EQ(ans, 3);
    ans = ms_obj.find(ch, 900);
    EXPECT_EQ(ans, std::string::npos);
    ans = ms_obj.find('x', 0);
    EXPECT_EQ(ans, std::string::npos);
}

TEST(substr_FUNC, t05)
{
    MemPoolCtrler memPoolCtrler1, memPoolCtrler2;
    MemPool memPool1(memPoolCtrler1, "pn1"), memPool2(memPoolCtrler2, "pn2");
    const char *testStr = "ms_obj";
    MapleString ms_obj(testStr, &memPool1), ans(testStr, &memPool2);
    ans = ms_obj.substr(0, 2);
    ASSERT_STREQ(ans.c_str(), "ms");
}

TEST(Operator_Check_MergeFUNC, t06)
{ // operator==
    MemPoolCtrler memPoolCtrler1, memPoolCtrler2, memPoolCtrler3;
    MemPool memPool1(memPoolCtrler1, "pn1"), memPool2(memPoolCtrler2, "pn2"), memPool3(memPoolCtrler3, "pn3");
    const char *testStr1 = "ms_obj1";
    const char *testStr2 = "ms_obj2";
    const char *testStr3 = "ms_obj";
    MapleString ms_obj1(testStr1, &memPool1), ms_obj2(testStr2, &memPool2), ms_obj3(testStr3, &memPool3);
    EXPECT_EQ((ms_obj1 == ms_obj2), false);
    EXPECT_EQ((ms_obj1 == ms_obj3), false);
    EXPECT_EQ((testStr1 == ms_obj1), true);
    EXPECT_EQ((testStr3 == ms_obj1), false);
    EXPECT_EQ((ms_obj1 == testStr1), true);
    EXPECT_EQ((ms_obj1 == testStr3), false);
}

TEST(insert_FUNC, t07)
{ // test method:insert(size_t pos, size_t n, char c)
    MemPoolCtrler memPoolCtrler1;
    MemPool memPool1(memPoolCtrler1, "pn1");
    const char *testStr1 = "ms_obj1";
    MapleString ms_obj1(testStr1, &memPool1);

    ASSERT_STREQ((ms_obj1.insert(9000, 2, 'a')).c_str(), "ms_obj1");
    ASSERT_STREQ((ms_obj1.insert(3, 2, 'a')).c_str(), "ms_aaobj1");
}

TEST(insert_FUNC, t08)
{
    MemPoolCtrler memPoolCtrler1, memPoolCtrler2;
    MemPool memPool1(memPoolCtrler1, "pn1"), memPool2(memPoolCtrler2, "pn2");
    const char *testStr1 = "ms_obj1";
    const char *testStr2 = "MS_OBJ2";
    MapleString ms_obj1(testStr1, &memPool1), ms_obj2(testStr2, &memPool2);

    // test method: insert(size_t pos, const char *s, size_t n)
    ASSERT_STREQ((ms_obj1.insert(1, nullptr, 2)).c_str(), "ms_obj1");
    ASSERT_STREQ((ms_obj1.insert(9000, "ABC", 2)).c_str(), "ms_obj1");
    ASSERT_STREQ((ms_obj1.insert(1, "ABC", 2)).c_str(), "mABs_obj1");

    // test method: insert(size_t pos, const char *s)
    ASSERT_STREQ((ms_obj1.insert(1, nullptr)).c_str(), "mABs_obj1");
    ASSERT_STREQ((ms_obj1.insert(9000, "ABC")).c_str(), "mABs_obj1");
    ASSERT_STREQ((ms_obj1.insert(1, "xyz")).c_str(), "mxyzABs_obj1");

    // test method: insert(size_t pos, const MapleString &str)
    ASSERT_STREQ((ms_obj1.insert(9000, ms_obj2)).c_str(), "mxyzABs_obj1");
    ASSERT_STREQ((ms_obj1.insert(2, ms_obj2)).c_str(), "mxMS_OBJ2yzABs_obj1");
    ASSERT_STREQ((ms_obj1.insert(20, ms_obj2)).c_str(), "mxMS_OBJ2yzABs_obj1");
}

TEST(append_FUNC, t01)
{
    MemPoolCtrler memPoolCtrler1, memPoolCtrler2, memPoolCtrler3;
    MemPool memPool1(memPoolCtrler1, "pn1"), memPool2(memPoolCtrler2, "pn2"), memPool3(memPoolCtrler3, "pn3");
    const char *testStr1 = "ms_obj1";
    const char *testStr2 = "";
    const char *testStr3 = "MS_OBJ3";
    const std::string testStr4 = "";
    const std::string testStr5 = "STR5";
    MapleString ms_obj1(testStr1, &memPool1), ms_obj2(testStr2, &memPool2), ms_obj3(testStr3, &memPool3);

    // test method:append(const MapleString &str)
    ASSERT_STREQ((ms_obj1.append(ms_obj2)).c_str(), "ms_obj1");
    ASSERT_STREQ((ms_obj1.append(ms_obj3)).c_str(), "ms_obj1MS_OBJ3");
    // test method:append(const std::string &str)
    ASSERT_STREQ((ms_obj1.append(testStr4)).c_str(), "ms_obj1MS_OBJ3");
    ASSERT_STREQ((ms_obj1.append(testStr5)).c_str(), "ms_obj1MS_OBJ3STR5");
    // test method:append(const char *s)
    ASSERT_STREQ((ms_obj1.append(nullptr)).c_str(), "ms_obj1MS_OBJ3STR5");
    ASSERT_STREQ((ms_obj1.append(testStr3)).c_str(), "ms_obj1MS_OBJ3STR5MS_OBJ3");
    // test method:append(const char *s, size_t n)
    ASSERT_STREQ((ms_obj1.append(nullptr, 2)).c_str(), "ms_obj1MS_OBJ3STR5MS_OBJ3");
    ASSERT_STREQ((ms_obj1.append(testStr3, 3)).c_str(), "ms_obj1MS_OBJ3STR5MS_OBJ3MS_");
}

}  // namespace
