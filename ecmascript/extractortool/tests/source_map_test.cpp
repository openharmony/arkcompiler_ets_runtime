/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "ecmascript/extractortool/tests/source_map_test.h"

#include <cstdio>

#include "ecmascript/extractortool/src/source_map.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class SourceMapTest : public testing::Test {
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

    EcmaVM *instance{nullptr};
    EcmaHandleScope *scope{nullptr};
    JSThread *thread{nullptr};
};

HWTEST_F_L0(SourceMapTest, TranslateUrlPositionBySourceMapTest)
{
    SourceMapFriend sourceMap(sourceMapData);
    int line = 10, column = 5;
    std::string url = "test.js";
    std::string packageName;
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    // if it can't find sources which match url after init, returns false and throw Translate failed
    // e.g. 1. sourceMapData is valid, but url is not valid;
    url = "entry/src/main/ets/pages/Index1.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_TRUE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    // if sourceMapData was used twice, sourceMap should cache url/modularMap
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    url = "entry/src/main/ets/pages/Index.ts";
    line = 0, column = 5;
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    url = "entry/src/main/ets/pages/Index.ts";
    line = 5, column = 0;
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    // e.g. 2. sourceMapData is not valid
    SourceMapFriend sourceMap1("testInvalidSourceMapData");
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap1.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    SourceMapFriend sourceMap2(sourceMapDataWithoutSources);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap2.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    // if mappings is too short(<18), it will throw translate failed
    SourceMapFriend sourceMap3(sourceMapDataWithShortMappings);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap3.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    SourceMapFriend sourceMap4(sourceMapDataWithInvalidMappings1);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap4.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    SourceMapFriend sourceMap5(sourceMapDataWithInvalidMappings2);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap5.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    SourceMapFriend sourceMap6(sourceMapDataWithInvalidMappings3);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap6.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    SourceMapFriend sourceMap7(sourceMapDataWithInvalidMappings4);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap7.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    SourceMapFriend sourceMap8(sourceMapDataWithInvalidMappings5);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap8.TranslateUrlPositionBySourceMap(url, line, column, packageName));

    // if sourceMapData is valid and url is end by ".js", it should return true
    SourceMapFriend sourceMap9(sourceMapDataWithJsSources);
    url = "entry/src/main/ets/pages/Index.js";
    EXPECT_TRUE(sourceMap9.TranslateUrlPositionBySourceMap(url, line, column, packageName));
    EXPECT_TRUE(packageName == "entry");
}

HWTEST_F_L0(SourceMapTest, TranslateUrlPositionBySourceMapTest1)
{
    SourceMapFriend sourceMap(sourceMapData1);
    int line = 10, column = 5;
    std::string url = "entry/src/main/ets/pages/Index.ts";
    std::string packageName;
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column, packageName));
    EXPECT_TRUE(packageName == "library");
}

HWTEST_F_L0(SourceMapTest, TranslateUrlPositionBySourceMapTest2)
{
    SourceMapFriend sourceMap(sourceMapData2);
    int line = 10, column = 5;
    std::string url = "entry/src/main/ets/pages/Index.ts";
    std::string packageName;
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column, packageName));
    EXPECT_TRUE(packageName == "entry");
}

HWTEST_F_L0(SourceMapTest, TranslateUrlPositionBySourceMapTest3)
{
    SourceMapFriend sourceMapFriend(sourceMapData3);
    int line = 10, column = 5;
    std::string url = "entry/src/main/ets/pages/Index.ts";
    std::string packageName;
    EXPECT_FALSE(sourceMapFriend.TranslateUrlPositionBySourceMap(url, line, column, packageName));
    EXPECT_TRUE(packageName == "library");
}

HWTEST_F_L0(SourceMapTest, TranslateUrlPositionBySourceMapTest4)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    int line = 10, column = 5;
    std::string url = "entry/src/main/ets/pages/Index.ts";
    std::string packageName;
    EXPECT_TRUE(sourceMapFriend.TranslateUrlPositionBySourceMap(url, line, column, packageName));
    EXPECT_TRUE(packageName.empty());
}

HWTEST_F_L0(SourceMapTest, Base64CharToIntTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('A'), 0);
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('a'), 26);
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('0'), 52);
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('+'), 62);
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('/'), 63);
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('-'), 64);
}

HWTEST_F_L0(SourceMapTest, ExtractSourceMapDataTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    std::string mappings = ";MAEO,CAAK,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA";
    auto sourceMapDataInstance = std::make_shared<SourceMapData>();
    std::shared_ptr<SourceMapData> &curMapData = sourceMapDataInstance;

    sourceMapFriend.ExtractSourceMapData(mappings, curMapData);
    SourceMapData &data = *curMapData;
    EXPECT_EQ(data.afterPos_.size(), 13);
}

HWTEST_F_L0(SourceMapTest, FindTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    MappingInfo mappingInfo;
    bool isReplaces = true;

    mappingInfo = sourceMapFriend.Find(0, 1, targetMap, isReplaces);
    EXPECT_EQ(mappingInfo.row, 0);
    EXPECT_EQ(mappingInfo.col, 0);
    mappingInfo = sourceMapFriend.Find(1, 1, targetMap, isReplaces);
    EXPECT_EQ(mappingInfo.row, 0);
    EXPECT_EQ(mappingInfo.col, 0);

    std::vector<SourceMapInfo> afterPos;
    SourceMapInfo info;
    info.beforeRow = 1;
    info.beforeColumn = 1;
    info.afterRow = 2;
    info.afterColumn = 2;
    info.sourcesVal = 1;
    info.namesVal = 1;
    afterPos.push_back(info);
    targetMap.afterPos_ = afterPos;
    mappingInfo = sourceMapFriend.Find(3, 3, targetMap, isReplaces);
    EXPECT_EQ(mappingInfo.row, 2);
    EXPECT_EQ(mappingInfo.col, 2);

    mappingInfo = sourceMapFriend.Find(3, 2, targetMap, isReplaces);
    EXPECT_EQ(mappingInfo.row, 2);
    EXPECT_EQ(mappingInfo.col, 2);

    mappingInfo = sourceMapFriend.Find(2, 2, targetMap, isReplaces);
    EXPECT_EQ(mappingInfo.row, 2);
    EXPECT_EQ(mappingInfo.col, 2);
}

HWTEST_F_L0(SourceMapTest, ExtractKeyInfoTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    std::string sourceMap = R"({"key1": "value1", "key2": "value\"2"})";
    std::vector<std::string> sourceKey;
    std::vector<std::string> &sourceKeyInfo = sourceKey;
    sourceMapFriend.ExtractKeyInfo(sourceMap, sourceKeyInfo);
    EXPECT_EQ(sourceKeyInfo[0], "key1");
    EXPECT_EQ(sourceKeyInfo[1], "value1");
    EXPECT_EQ(sourceKeyInfo[2], "key2");
    EXPECT_EQ(sourceKeyInfo[3], "value\"2");
}

HWTEST_F_L0(SourceMapTest, GetPosInfoTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    std::string temp = "005:012:0";
    int32_t start = 6;
    std::string line, column;

    sourceMapFriend.GetPosInfo(temp, start, line, column);
    EXPECT_EQ(line, "05");
    EXPECT_EQ(column, "01");
}

HWTEST_F_L0(SourceMapTest, HandleMappingsTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    std::string mapping = "X;Y";
    std::vector<std::string> result = sourceMapFriend.HandleMappings(mapping);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], "X");
    EXPECT_EQ(result[1], ";");
    EXPECT_EQ(result[2], "Y");
}

HWTEST_F_L0(SourceMapTest, VlqRevCodeTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    std::string vStr;
    std::vector<int32_t> ans;
    EXPECT_TRUE(sourceMapFriend.VlqRevCode(vStr, ans));

    vStr = "A";
    EXPECT_TRUE(sourceMapFriend.VlqRevCode(vStr, ans));
    ASSERT_EQ(ans.size(), 1u);

    vStr = "A=A";
    EXPECT_FALSE(sourceMapFriend.VlqRevCode(vStr, ans));

    vStr = "X";
    EXPECT_TRUE(sourceMapFriend.VlqRevCode(vStr, ans));

    vStr = "A";
    EXPECT_TRUE(sourceMapFriend.VlqRevCode(vStr, ans));
}

HWTEST_F_L0(SourceMapTest, GetLineAndColumnNumbersTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    std::vector<SourceMapInfo> afterPos;
    SourceMapInfo info;
    info.beforeRow = 1;
    info.beforeColumn = 1;
    info.afterRow = 2;
    info.afterColumn = 2;
    info.sourcesVal = 1;
    info.namesVal = 1;
    afterPos.push_back(info);
    targetMap.afterPos_ = afterPos;
    bool isReplaces = true;

    int line = 1;
    int column = 1;
    bool result = sourceMapFriend.GetLineAndColumnNumbers(line, column, targetMap, isReplaces);
    EXPECT_TRUE(result);
    EXPECT_EQ(line, 2);
    EXPECT_EQ(column, 2);
    EXPECT_TRUE(isReplaces);

    line = 5;
    column = 5;
    result = sourceMapFriend.GetLineAndColumnNumbers(line, column, targetMap, isReplaces);
    EXPECT_TRUE(result);
    EXPECT_EQ(line, 5);
    EXPECT_EQ(column, 5);
    EXPECT_FALSE(isReplaces);

    line = 99;
    column = 99;
    result = sourceMapFriend.GetLineAndColumnNumbers(line, column, targetMap, isReplaces);
    EXPECT_TRUE(result);
    EXPECT_EQ(line, 99);
    EXPECT_EQ(column, 99);
    EXPECT_FALSE(isReplaces);
}
}