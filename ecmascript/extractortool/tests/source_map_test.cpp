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

HWTEST_F_L0(SourceMapTest, GetInitStatusDefaultTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::NOT_EXECUTED);
}

HWTEST_F_L0(SourceMapTest, SetAndGetInitStatusTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);

    sourceMapFriend.SetInitStatus(InitStatus::IN_EXECUTED);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::IN_EXECUTED);

    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::EXECUTED_SUCCESSFULLY);

    sourceMapFriend.SetInitStatus(InitStatus::NOT_EXECUTED);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::NOT_EXECUTED);
}

HWTEST_F_L0(SourceMapTest, InitStatusTransitionTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);

    // NOT_EXECUTED -> IN_EXECUTED -> EXECUTED_SUCCESSFULLY
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::NOT_EXECUTED);
    sourceMapFriend.SetInitStatus(InitStatus::IN_EXECUTED);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::IN_EXECUTED);
    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::EXECUTED_SUCCESSFULLY);
}

HWTEST_F_L0(SourceMapTest, InitStatusThreeStatesDistinctTest)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);

    sourceMapFriend.SetInitStatus(InitStatus::NOT_EXECUTED);
    InitStatus s1 = sourceMapFriend.GetInitStatus();

    sourceMapFriend.SetInitStatus(InitStatus::IN_EXECUTED);
    InitStatus s2 = sourceMapFriend.GetInitStatus();

    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);
    InitStatus s3 = sourceMapFriend.GetInitStatus();

    // Three states must be distinct
    EXPECT_NE(s1, s2);
    EXPECT_NE(s2, s3);
    EXPECT_NE(s1, s3);
}

/**
 * @tc.name: ExtractFileName_001
 * @tc.desc: Test ExtractFileName extracts URL from stack frame with parentheses and colons
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, ExtractFileName_001)
{
    // Format: at funcName (url:line:column)
    std::string stackLine = "at handleClick (entry/src/main/ets/pages/Index.ts:10:5)";
    std::string fileName = SourceMapFriend::ExtractFileName(stackLine);
    EXPECT_EQ(fileName, "entry/src/main/ets/pages/Index.ts");
}

/**
 * @tc.name: ExtractFileName_002
 * @tc.desc: Test ExtractFileName with no parentheses returns original string
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, ExtractFileName_002)
{
    std::string stackLine = "TypeError: Cannot read property";
    std::string fileName = SourceMapFriend::ExtractFileName(stackLine);
    EXPECT_EQ(fileName, stackLine);
}

/**
 * @tc.name: ExtractFileName_003
 * @tc.desc: Test ExtractFileName with no second colon returns original string
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, ExtractFileName_003)
{
    std::string stackLine = "at func (nocolon)";
    std::string fileName = SourceMapFriend::ExtractFileName(stackLine);
    EXPECT_EQ(fileName, stackLine);
}

/**
 * @tc.name: ExtractFileName_004
 * @tc.desc: Test ExtractFileName with empty string
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, ExtractFileName_004)
{
    std::string stackLine = "";
    std::string fileName = SourceMapFriend::ExtractFileName(stackLine);
    EXPECT_EQ(fileName, "");
}

/**
 * @tc.name: ExtractStackInfo_001
 * @tc.desc: Test ExtractStackInfo splits stack string by newlines
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, ExtractStackInfo_001)
{
    std::string stackStr = "line1\nline2\nline3";
    std::vector<std::string> res;
    SourceMapFriend::ExtractStackInfo(stackStr, res);
    ASSERT_EQ(res.size(), 3u);
    EXPECT_EQ(res[0], "line1");
    EXPECT_EQ(res[1], "line2");
    EXPECT_EQ(res[2], "line3");
}

/**
 * @tc.name: ExtractStackInfo_002
 * @tc.desc: Test ExtractStackInfo with empty string
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, ExtractStackInfo_002)
{
    std::string stackStr = "";
    std::vector<std::string> res;
    SourceMapFriend::ExtractStackInfo(stackStr, res);
    EXPECT_TRUE(res.empty());
}

/**
 * @tc.name: ExtractStackInfo_003
 * @tc.desc: Test ExtractStackInfo with single line (no newline)
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, ExtractStackInfo_003)
{
    std::string stackStr = "only one line";
    std::vector<std::string> res;
    SourceMapFriend::ExtractStackInfo(stackStr, res);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0], "only one line");
}

/**
 * @tc.name: FindWithKey_001
 * @tc.desc: Test Find with key returns MappingInfo with sources for valid input
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, FindWithKey_001)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    // sources_ format: 6 spaces + quote + url + quote (as parsed by eager path)
    targetMap.sources_ = "      \"entry/src/main/ets/pages/Index.ets\"";
    std::vector<SourceMapInfo> afterPos;
    SourceMapInfo info;
    info.beforeRow = 1;
    info.beforeColumn = 1;
    info.afterRow = 2;
    info.afterColumn = 2;
    afterPos.push_back(info);
    targetMap.afterPos_ = afterPos;

    MappingInfo result = sourceMapFriend.FindWithKey(3, 3, targetMap, "entry/src/main/ets/pages/Index.ts");
    EXPECT_EQ(result.row, 2);
    EXPECT_EQ(result.col, 2);
    EXPECT_FALSE(result.sources.empty());
}

/**
 * @tc.name: FindWithKey_002
 * @tc.desc: Test Find with key returns original row/col/key when row<1 or col<1
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, FindWithKey_002)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    targetMap.sources_ = "      \"entry/src/main/ets/pages/Index.ets\"";

    MappingInfo result = sourceMapFriend.FindWithKey(0, 5, targetMap, "testKey");
    EXPECT_EQ(result.row, 0);
    EXPECT_EQ(result.col, 5);
    EXPECT_EQ(result.sources, "testKey");

    result = sourceMapFriend.FindWithKey(5, 0, targetMap, "testKey2");
    EXPECT_EQ(result.row, 5);
    EXPECT_EQ(result.col, 0);
    EXPECT_EQ(result.sources, "testKey2");
}

/**
 * @tc.name: FindWithKey_003
 * @tc.desc: Test Find with key returns original when afterPos_ or sources_ is empty
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, FindWithKey_003)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    targetMap.sources_ = "      \"entry/src/main/ets/pages/Index.ets\"";
    // afterPos_ is empty

    MappingInfo result = sourceMapFriend.FindWithKey(5, 5, targetMap, "myKey");
    EXPECT_EQ(result.row, 5);
    EXPECT_EQ(result.col, 5);
    EXPECT_EQ(result.sources, "myKey");
}

/**
 * @tc.name: FindWithKey_004
 * @tc.desc: Test Find with key ending in .js returns sources without binary search
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, FindWithKey_004)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    targetMap.sources_ = "      \"entry/src/main/ets/pages/Index.ets\"";
    std::vector<SourceMapInfo> afterPos;
    SourceMapInfo info;
    info.beforeRow = 1;
    info.beforeColumn = 1;
    info.afterRow = 2;
    info.afterColumn = 2;
    afterPos.push_back(info);
    targetMap.afterPos_ = afterPos;

    // Key ending with .js: row/col unchanged, sources extracted
    MappingInfo result = sourceMapFriend.FindWithKey(3, 3, targetMap, "entry/src/main/ets/pages/Index.js");
    EXPECT_EQ(result.row, 3);
    EXPECT_EQ(result.col, 3);
    EXPECT_FALSE(result.sources.empty());
}

/**
 * @tc.name: FindWithKey_005
 * @tc.desc: Test Find with key when row exceeds max afterRow
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, FindWithKey_005)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    targetMap.sources_ = "      \"entry/src/main/ets/pages/Index.ets\"";
    std::vector<SourceMapInfo> afterPos;
    SourceMapInfo info;
    info.beforeRow = 1;
    info.beforeColumn = 1;
    info.afterRow = 2;
    info.afterColumn = 2;
    afterPos.push_back(info);
    targetMap.afterPos_ = afterPos;

    // row=100 exceeds max afterRow=2, row-- -> 99 > 2, returns (row+1, col+1, key) = (100, 5, key)
    MappingInfo result = sourceMapFriend.FindWithKey(100, 5, targetMap, "entry/src/main/ets/pages/Index.ts");
    EXPECT_EQ(result.row, 100);
    EXPECT_EQ(result.col, 5);
    EXPECT_EQ(result.sources, "entry/src/main/ets/pages/Index.ts");
}

/**
 * @tc.name: GetPackageName_001
 * @tc.desc: Test GetPackageName with entry-package-info format
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, GetPackageName_001)
{
    SourceMapData targetMap;
    targetMap.packageName_ = "    \"entry-package-info\": \"entry|1.0.0\"";
    targetMap.isPackageInfo_ = false;

    std::string packageName;
    SourceMapFriend::GetPackageName(targetMap, packageName);
    EXPECT_EQ(packageName, "entry");
}

/**
 * @tc.name: GetPackageName_002
 * @tc.desc: Test GetPackageName with package-info format (isPackageInfo_=true)
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, GetPackageName_002)
{
    SourceMapData targetMap;
    targetMap.packageName_ = "    \"package-info\": \"library|2.0.0\"";
    targetMap.isPackageInfo_ = true;

    std::string packageName;
    SourceMapFriend::GetPackageName(targetMap, packageName);
    EXPECT_EQ(packageName, "library");
}

/**
 * @tc.name: GetPackageName_003
 * @tc.desc: Test GetPackageName with empty packageName_ leaves output unchanged
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, GetPackageName_003)
{
    SourceMapData targetMap;
    targetMap.packageName_ = "";

    std::string packageName = "original";
    SourceMapFriend::GetPackageName(targetMap, packageName);
    EXPECT_EQ(packageName, "original");
}

/**
 * @tc.name: GetPackageName_004
 * @tc.desc: Test GetPackageName with no pipe character leaves output unchanged
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, GetPackageName_004)
{
    SourceMapData targetMap;
    targetMap.packageName_ = "    \"entry-package-info\": \"entryNoPipe\"";
    targetMap.isPackageInfo_ = false;

    std::string packageName = "original";
    SourceMapFriend::GetPackageName(targetMap, packageName);
    EXPECT_EQ(packageName, "original");
}

/**
 * @tc.name: GetSourceInfo_001
 * @tc.desc: Test GetSourceInfo with packageName containing pipe
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, GetSourceInfo_001)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    targetMap.sources_ = "      \"entry/src/main/ets/pages/Index.ets\"";
    targetMap.packageName_ = "    \"entry-package-info\": \"entry|1.0.0\"";
    targetMap.isPackageInfo_ = false;
    std::vector<SourceMapInfo> afterPos;
    SourceMapInfo info;
    info.beforeRow = 5;
    info.beforeColumn = 10;
    info.afterRow = 2;
    info.afterColumn = 2;
    afterPos.push_back(info);
    targetMap.afterPos_ = afterPos;

    std::string sourceInfo = sourceMapFriend.GetSourceInfo("3", "3", targetMap,
        "entry/src/main/ets/pages/Index.ts");
    EXPECT_FALSE(sourceInfo.empty());
    EXPECT_NE(sourceInfo.find("entry"), std::string::npos);
    EXPECT_NE(sourceInfo.find("6"), std::string::npos);   // row = beforeRow+1 = 6
    EXPECT_NE(sourceInfo.find("11"), std::string::npos);  // col = beforeColumn+1 = 11
}

/**
 * @tc.name: GetSourceInfo_002
 * @tc.desc: Test GetSourceInfo without packageName (empty)
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, GetSourceInfo_002)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    targetMap.sources_ = "      \"entry/src/main/ets/pages/Index.ets\"";
    targetMap.packageName_ = "";
    std::vector<SourceMapInfo> afterPos;
    SourceMapInfo info;
    info.beforeRow = 5;
    info.beforeColumn = 10;
    info.afterRow = 2;
    info.afterColumn = 2;
    afterPos.push_back(info);
    targetMap.afterPos_ = afterPos;

    std::string sourceInfo = sourceMapFriend.GetSourceInfo("3", "3", targetMap,
        "entry/src/main/ets/pages/Index.ts");
    EXPECT_FALSE(sourceInfo.empty());
    // Without package name, format is (sources:row:col)
    EXPECT_NE(sourceInfo.find("("), std::string::npos);
    EXPECT_NE(sourceInfo.find("6"), std::string::npos);
}

/**
 * @tc.name: GetSourceInfo_003
 * @tc.desc: Test GetSourceInfo with package-info (isPackageInfo_=true)
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, GetSourceInfo_003)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    targetMap.sources_ = "      \"entry/src/main/ets/pages/Index.ets\"";
    targetMap.packageName_ = "    \"package-info\": \"library|2.0.0\"";
    targetMap.isPackageInfo_ = true;
    std::vector<SourceMapInfo> afterPos;
    SourceMapInfo info;
    info.beforeRow = 5;
    info.beforeColumn = 10;
    info.afterRow = 2;
    info.afterColumn = 2;
    afterPos.push_back(info);
    targetMap.afterPos_ = afterPos;

    std::string sourceInfo = sourceMapFriend.GetSourceInfo("3", "3", targetMap,
        "entry/src/main/ets/pages/Index.ts");
    EXPECT_FALSE(sourceInfo.empty());
    EXPECT_NE(sourceInfo.find("library"), std::string::npos);
}

/**
 * @tc.name: SetInitStatus_NO_SOURCEMAP_001
 * @tc.desc: Test SetInitStatus with NO_SOURCEMAP state
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, SetInitStatus_NO_SOURCEMAP_001)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);

    sourceMapFriend.SetInitStatus(InitStatus::NO_SOURCEMAP);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::NO_SOURCEMAP);

    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::EXECUTED_SUCCESSFULLY);
}

/**
 * @tc.name: TranslateBySourceMap_001
 * @tc.desc: Test TranslateBySourceMap preserves lines without parentheses
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, TranslateBySourceMap_001)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);

    std::string stackStr = "TypeError: Cannot read property";
    std::string result = sourceMapFriend.TranslateBySourceMap(stackStr);
    EXPECT_NE(result.find("TypeError: Cannot read property"), std::string::npos);
}

/**
 * @tc.name: TranslateBySourceMap_002
 * @tc.desc: Test TranslateBySourceMap with unknown URL falls back to raw stack
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, TranslateBySourceMap_002)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);

    // No eager data loaded, URL not found in eagerSourceMaps_
    std::string stackStr = "at func (unknown/file.js:10:5)";
    std::string result = sourceMapFriend.TranslateBySourceMap(stackStr);
    // Unknown URL line is appended as-is, result not empty
    EXPECT_NE(result.find("unknown/file.js"), std::string::npos);
}

/**
 * @tc.name: TranslateBySourceMap_003
 * @tc.desc: Test TranslateBySourceMap with mixed stack lines
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, TranslateBySourceMap_003)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);

    std::string stackStr = "TypeError: Cannot read property\nat func (unknown.js:10:5)";
    std::string result = sourceMapFriend.TranslateBySourceMap(stackStr);
    // First line (no parens) preserved, second line (unknown URL) preserved
    EXPECT_NE(result.find("TypeError: Cannot read property"), std::string::npos);
    EXPECT_NE(result.find("unknown.js"), std::string::npos);
}

/**
 * @tc.name: TranslateBySourceMap_NoSourceMap_001
 * @tc.desc: Test TranslateBySourceMap returns NOT_FOUNDMAP prefix when InitStatus=NO_SOURCEMAP
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, TranslateBySourceMap_NoSourceMap_001)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    sourceMapFriend.SetInitStatus(InitStatus::NO_SOURCEMAP);

    std::string stackStr = "at func (entry/src/main/ets/pages/Index.ts:10:5)";
    std::string result = sourceMapFriend.TranslateBySourceMap(stackStr);
    EXPECT_NE(result.find("Cannot get SourceMap info"), !std::string::npos);
}

/**
 * @tc.name: SplitSourceMapEager_001
 * @tc.desc: Test SplitSourceMap(string) eager path populates eagerSourceMaps
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, SplitSourceMapEager_001)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);

    // Populate eager data using the existing test source map data
    sourceMapFriend.SplitSourceMapEager(sourceMapData);

    // Verify by calling TranslateUrlPositionBySourceMap which checks eager path first
    std::string url = "entry/src/main/ets/pages/Index.ts";
    int line = 10;
    int column = 5;
    std::string packageName;
    // Should not crash; eager path is checked before zero-copy path
    sourceMapFriend.TranslateUrlPositionBySourceMap(url, line, column, packageName);
}

/**
 * @tc.name: GetLineAndColumnNumbersEager_001
 * @tc.desc: Test GetLineAndColumnNumbers with url+packageName (eager path)
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, GetLineAndColumnNumbersEager_001)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    targetMap.sources_ = "      \"entry/src/main/ets/pages/Index.ets\"";
    targetMap.packageName_ = "    \"entry-package-info\": \"entry|1.0.0\"";
    targetMap.isPackageInfo_ = false;
    std::vector<SourceMapInfo> afterPos;
    SourceMapInfo info;
    info.beforeRow = 5;
    info.beforeColumn = 10;
    info.afterRow = 2;
    info.afterColumn = 2;
    afterPos.push_back(info);
    targetMap.afterPos_ = afterPos;

    int line = 3;
    int column = 3;
    std::string url = "entry/src/main/ets/pages/Index.ts";
    std::string packageName;

    bool result = sourceMapFriend.GetLineAndColumnNumbersEager(line, column, targetMap, url, packageName);
    EXPECT_TRUE(result);
    EXPECT_EQ(line, 6);    // beforeRow + 1 = 5 + 1
    EXPECT_EQ(column, 11); // beforeColumn + 1 = 10 + 1
}

/**
 * @tc.name: GetLineAndColumnNumbersEager_002
 * @tc.desc: Test GetLineAndColumnNumbers eager returns false when Find returns row=0
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, GetLineAndColumnNumbersEager_002)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    SourceMapData targetMap;
    targetMap.sources_ = "      \"entry/src/main/ets/pages/Index.ets\"";
    targetMap.afterPos_.clear();

    int line = 0;
    int column = 5;
    std::string url = "entry/src/main/ets/pages/Index.ts";
    std::string packageName;

    bool result = sourceMapFriend.GetLineAndColumnNumbersEager(line, column, targetMap, url, packageName);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: ExtractSourceMapDataShared_001
 * @tc.desc: Test ExtractSourceMapData with shared_ptr<SourceMapData> overload
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, ExtractSourceMapDataShared_001)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    auto curMapData = std::make_shared<SourceMapData>();
    std::string mappings = ";MAEO,CAAK,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA";

    sourceMapFriend.ExtractSourceMapDataShared(mappings, curMapData);
    EXPECT_FALSE(curMapData->afterPos_.empty());
}

/**
 * @tc.name: ExtractSourceMapDataShared_002
 * @tc.desc: Test ExtractSourceMapData with null shared_ptr does not crash
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, ExtractSourceMapDataShared_002)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);
    std::shared_ptr<SourceMapData> curMapData = nullptr;
    std::string mappings = "A";

    // Should not crash when curMapData is null
    sourceMapFriend.ExtractSourceMapDataShared(mappings, curMapData);
}

/**
 * @tc.name: MappingInfo_sources_001
 * @tc.desc: Test MappingInfo sources field default and assignment
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, MappingInfo_sources_001)
{
    MappingInfo info;
    EXPECT_TRUE(info.sources.empty());

    info.sources = "entry/src/main/ets/pages/Index.ets";
    EXPECT_EQ(info.sources, "entry/src/main/ets/pages/Index.ets");
}

/**
 * @tc.name: SourceMapData_isPackageInfo_001
 * @tc.desc: Test SourceMapData isPackageInfo_ field default and assignment
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, SourceMapData_isPackageInfo_001)
{
    SourceMapData data;
    EXPECT_FALSE(data.isPackageInfo_);

    data.isPackageInfo_ = true;
    EXPECT_TRUE(data.isPackageInfo_);

    data.isPackageInfo_ = false;
    EXPECT_FALSE(data.isPackageInfo_);
}

/**
 * @tc.name: SetInitStatus_MultiHapScenario_001
 * @tc.desc: Test InitStatus state across multiple hap iterations (success/failure)
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, SetInitStatus_MultiHapScenario_001)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);

    // Simulate multiple hap iterations in js_runtime SourceMapInit:
    // hap1: ReadSourceMapData success -> EXECUTED_SUCCESSFULLY
    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::EXECUTED_SUCCESSFULLY);

    // hap2: ReadSourceMapData failure -> NO_SOURCEMAP
    sourceMapFriend.SetInitStatus(InitStatus::NO_SOURCEMAP);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::NO_SOURCEMAP);

    // hap3: ReadSourceMapData success -> EXECUTED_SUCCESSFULLY, explicitly reset
    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::EXECUTED_SUCCESSFULLY);

    // hap4: ReadSourceMapData failure -> NO_SOURCEMAP
    sourceMapFriend.SetInitStatus(InitStatus::NO_SOURCEMAP);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::NO_SOURCEMAP);
}

/**
 * @tc.name: InitStatus_AllStates_001
 * @tc.desc: Test all InitStatus states transition correctly
 * @tc.type: FUNC
 */
HWTEST_F_L0(SourceMapTest, InitStatus_AllStates_001)
{
    SourceMapFriend sourceMapFriend(sourceMapData4);

    // Simulate full SourceMapInit lifecycle
    sourceMapFriend.SetInitStatus(InitStatus::IN_EXECUTED);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::IN_EXECUTED);

    // ReadSourceMapData fails for first hap
    sourceMapFriend.SetInitStatus(InitStatus::NO_SOURCEMAP);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::NO_SOURCEMAP);

    // ReadSourceMapData succeeds for second hap
    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::EXECUTED_SUCCESSFULLY);

    // Init completes
    sourceMapFriend.SetInitStatus(InitStatus::EXECUTED_SUCCESSFULLY);
    EXPECT_EQ(sourceMapFriend.GetInitStatus(), InitStatus::EXECUTED_SUCCESSFULLY);
}
}  // namespace panda::test
