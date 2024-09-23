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

#include "ecmascript/extractortool/src/source_map.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
const std::string sourceMapData =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": \"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\"\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithoutSources =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [],\n"
        "    \"names\": [],\n"
        "    \"mappings\": \";MAEO,CAAK,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,"
        "CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA;\"\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithShortMappings =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": \"\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithInvalidMappings1 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": pA;\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithInvalidMappings2 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": pAq;ABCD;abcd;0123;0+/-;\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithInvalidMappings3 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": X;\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithInvalidMappings4 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": A;\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithInvalidMappings5 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": ;\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithJsSources =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.js\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.js\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": ;\n"
        "  }\n"
        "}";

namespace panda::ecmascript {
class SourceMapFriend {
public:
    uint32_t Base64CharToInt(char charCode)
    {
        return sourceMap.Base64CharToInt(charCode);
    }

    void SplitSourceMap(const std::string &sourceMapData)
    {
        sourceMap.SplitSourceMap(sourceMapData);
    }

    void ExtractSourceMapData(const std::string &allmappings, std::shared_ptr<SourceMapData> &curMapData)
    {
        sourceMap.ExtractSourceMapData(allmappings, curMapData);
    }

    MappingInfo Find(int32_t row, int32_t col, const SourceMapData &targetMap, bool& isReplaces)
    {
        return sourceMap.Find(row, col, targetMap, isReplaces);
    }

    void ExtractKeyInfo(const std::string &aSourceMap, std::vector<std::string> &sourceKeyInfo)
    {
        sourceMap.ExtractKeyInfo(aSourceMap, sourceKeyInfo);
    }

    void GetPosInfo(const std::string &temp, int32_t start, std::string &line, std::string &column)
    {
        sourceMap.GetPosInfo(temp, start, line, column);
    }

    std::vector<std::string> HandleMappings(const std::string &mapping)
    {
        return sourceMap.HandleMappings(mapping);
    }

    bool VlqRevCode(const std::string &vStr, std::vector<int32_t> &ans)
    {
        return sourceMap.VlqRevCode(vStr, ans);
    }

    bool TranslateUrlPositionBySourceMap(std::string &url, int &line, int &column)
    {
        return sourceMap.TranslateUrlPositionBySourceMap(url, line, column);
    }

    bool GetLineAndColumnNumbers(int &line, int &column, SourceMapData &targetMap, bool& isReplaces)
    {
        return sourceMap.GetLineAndColumnNumbers(line, column, targetMap, isReplaces);
    }

private:
    SourceMap sourceMap;
};
}

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

uint8_t *StringToUint8(const std::string &str)
{
    const auto *data = reinterpret_cast<const uint8_t *>(str.c_str());
    return const_cast<uint8_t *>(data);
}

SourceMap TestInit(const std::string &sourceMapData)
{
    uint8_t *data = StringToUint8(sourceMapData);
    size_t dataSize = sourceMapData.size();
    SourceMap sourceMap;
    sourceMap.Init(const_cast<uint8_t *>(data), dataSize);
    return sourceMap;
}

HWTEST_F_L0(SourceMapTest, TranslateUrlPositionBySourceMapTest)
{
    SourceMap sourceMap;
    int line = 10, column = 5;
    std::string url = "test.js";
    // if url contains ".js" returns true
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    // if it can't find sources which match url after init, returns false and throw Translate failed
    // e.g. 1. sourceMapData is valid, but url is not valid;
    sourceMap = TestInit(sourceMapData);
    url = "entry/src/main/ets/pages/Index1.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    // e.g. 2. sourceMapData is not valid
    sourceMap = TestInit("testInvalidSourceMapData");
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapDataWithoutSources);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_TRUE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    // if mappings is too short(<18), it will throw translate failed
    sourceMap = TestInit(sourceMapDataWithShortMappings);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapData);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_TRUE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    // if sourceMapData was used twice, sourceMap should cache url/modularMap
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapDataWithInvalidMappings1);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapDataWithInvalidMappings2);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapDataWithInvalidMappings3);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapDataWithInvalidMappings4);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapDataWithInvalidMappings5);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapData);
    url = "entry/src/main/ets/pages/Index.ts";
    line = 0, column = 5;
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapData);
    url = "entry/src/main/ets/pages/Index.ts";
    line = 5, column = 0;
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    // if sourceMapData is valid and url is end by ".js", it should return true
    url = "entry/src/main/ets/pages/Index.js";
    sourceMap = TestInit(sourceMapDataWithJsSources);
    EXPECT_TRUE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));
}

HWTEST_F_L0(SourceMapTest, Base64CharToIntTest)
{
    SourceMapFriend sourceMapFriend;
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('A'), 0);
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('a'), 26);
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('0'), 52);
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('+'), 62);
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('/'), 63);
    EXPECT_EQ(sourceMapFriend.Base64CharToInt('-'), 64);
}

void PrintSourceMapData(const SourceMapData &data)
{
    std::cout << "URL: " << data.url_ << std::endl;
    std::cout << "Now Position: (" << data.nowPos_.afterRow << ", " << data.nowPos_.afterColumn << ")" << std::endl;

    std::cout << "Files:" << std::endl;
    for (const auto &file: data.files_) {
        std::cout << "- " << file << std::endl;
    }

    std::cout << "Sources:" << std::endl;
    for (const auto &source: data.sources_) {
        std::cout << "- " << source << std::endl;
    }

    std::cout << "Names:" << std::endl;
    for (const auto &name: data.names_) {
        std::cout << "- " << name << std::endl;
    }

    std::cout << "Mappings:" << std::endl;
    for (const auto &mapping: data.mappings_) {
        std::cout << "- " << mapping << std::endl;
    }

    std::cout << "After Positions:" << std::endl;
    for (const auto &pos: data.afterPos_) {
        std::cout << "- (" << pos.beforeRow << ", " << pos.beforeColumn << ", " << pos.afterRow << ", "
                  << pos.afterColumn << ")" << std::endl;
    }
}

HWTEST_F_L0(SourceMapTest, ExtractSourceMapDataTest)
{
    SourceMapFriend sourceMapFriend;
    std::string mappings = ";MAEO,CAAK,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA";
    auto sourceMapDataInstance = std::make_shared<SourceMapData>();
    std::shared_ptr<SourceMapData> &curMapData = sourceMapDataInstance;

    sourceMapFriend.ExtractSourceMapData(mappings, curMapData);
    SourceMapData &data = *curMapData;
    EXPECT_EQ(data.afterPos_.size(), 13);
}

HWTEST_F_L0(SourceMapTest, FindTest)
{
    // Find Full Covered
    SourceMapFriend sourceMapFriend;
    SourceMapData targetMap;
    MappingInfo mappingInfo;
    bool isReplaces = true;

    mappingInfo = sourceMapFriend.Find(0, 1, targetMap, isReplaces);
    EXPECT_EQ(mappingInfo.row, 0);
    EXPECT_EQ(mappingInfo.col, 0);
    EXPECT_TRUE(isReplaces);
    mappingInfo = sourceMapFriend.Find(1, 1, targetMap, isReplaces);
    EXPECT_EQ(mappingInfo.row, 0);
    EXPECT_EQ(mappingInfo.col, 0);
    EXPECT_TRUE(isReplaces);

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
    EXPECT_TRUE(isReplaces);

    mappingInfo = sourceMapFriend.Find(3, 2, targetMap, isReplaces);
    EXPECT_EQ(mappingInfo.row, 2);
    EXPECT_EQ(mappingInfo.col, 2);
    EXPECT_TRUE(isReplaces);

    mappingInfo = sourceMapFriend.Find(2, 2, targetMap, isReplaces);
    EXPECT_EQ(mappingInfo.row, 2);
    EXPECT_EQ(mappingInfo.col, 2);
    EXPECT_TRUE(isReplaces);
}

HWTEST_F_L0(SourceMapTest, ExtractKeyInfoTest)
{
    // ExtractKeyInfo Full Covered
    SourceMapFriend sourceMapFriend;
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
    SourceMapFriend sourceMapFriend;
    std::string temp = "005:012:0";
    int32_t start = 6;
    std::string line, column;

    sourceMapFriend.GetPosInfo(temp, start, line, column);
    EXPECT_EQ(line, "05");
    EXPECT_EQ(column, "01");
}

HWTEST_F_L0(SourceMapTest, HandleMappingsTest)
{
    SourceMapFriend sourceMapFriend;
    std::string mapping = "X;Y";
    std::vector<std::string> result = sourceMapFriend.HandleMappings(mapping);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], "X");
    EXPECT_EQ(result[1], ";");
    EXPECT_EQ(result[2], "Y");
}

HWTEST_F_L0(SourceMapTest, VlqRevCodeTest)
{
    SourceMapFriend sourceMapFriend;
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
    // GetLineAndColumnNumbers is full covered
    SourceMapFriend sourceMapFriend;
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