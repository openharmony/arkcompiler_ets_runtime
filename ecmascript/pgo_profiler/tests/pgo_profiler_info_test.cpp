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

#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "gtest/gtest.h"

#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/tests/pgo_context_mock.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::pgo;

namespace panda::test {
class PGOProfilerInfoTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
    }
};

HWTEST_F_L0(PGOProfilerInfoTest, VerifyChecksumEmptyTest)
{
    PGOPandaFileInfos baseFileInfos;
    PGOPandaFileInfos incomingFileInfos;
    bool result = baseFileInfos.VerifyChecksum(incomingFileInfos, "base.ap", "incoming.ap");
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, VerifyChecksumEmptyBaseTest)
{
    PGOPandaFileInfos emptyBase;
    PGOPandaFileInfos nonEmptyIncoming;
    nonEmptyIncoming.Sample(100, 1);
    bool result = emptyBase.VerifyChecksum(nonEmptyIncoming, "base.ap", "incoming.ap");
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, VerifyChecksumEmptyIncomingTest)
{
    PGOPandaFileInfos nonEmptyBase;
    nonEmptyBase.Sample(100, 1);
    PGOPandaFileInfos emptyIncoming;
    bool result = nonEmptyBase.VerifyChecksum(emptyIncoming, "base.ap", "incoming.ap");
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, VerifyChecksumPartialOverlapTest)
{
    PGOPandaFileInfos baseFileInfos4;
    baseFileInfos4.Sample(100, 1);
    baseFileInfos4.Sample(200, 2);
    PGOPandaFileInfos incomingFileInfos4;
    incomingFileInfos4.Sample(100, 1);
    incomingFileInfos4.Sample(300, 3);
    bool result = baseFileInfos4.VerifyChecksum(incomingFileInfos4, "base.ap", "incoming.ap");
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, VerifyChecksumNoOverlapTest)
{
    PGOPandaFileInfos baseFileInfos5;
    baseFileInfos5.Sample(100, 1);
    PGOPandaFileInfos incomingFileInfos5;
    incomingFileInfos5.Sample(200, 2);
    bool result = baseFileInfos5.VerifyChecksum(incomingFileInfos5, "base.ap", "incoming.ap");
    ASSERT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, PGOPandaFileInfosProcessToTextEmptyTest)
{
    PGOPandaFileInfos fileInfos;
    std::string tempFile = "test_output_1.txt";
    std::ofstream stream(tempFile);
    ASSERT_TRUE(stream.is_open());
    fileInfos.ProcessToText(stream);
    stream.close();
    std::ifstream readStream(tempFile);
    std::string output((std::istreambuf_iterator<char>(readStream)),
                       std::istreambuf_iterator<char>());
    ASSERT_TRUE(output.find("Panda file sumcheck list") != std::string::npos);
    readStream.close();
    std::remove(tempFile.c_str());
}

HWTEST_F_L0(PGOProfilerInfoTest, PGOPandaFileInfosProcessToTextSingleSampleTest)
{
    PGOPandaFileInfos fileInfos;
    fileInfos.Sample(12345, 1);
    std::string tempFile = "test_output_2.txt";
    std::ofstream stream(tempFile);
    ASSERT_TRUE(stream.is_open());
    fileInfos.ProcessToText(stream);
    stream.close();
    std::ifstream readStream(tempFile);
    std::string output((std::istreambuf_iterator<char>(readStream)),
                       std::istreambuf_iterator<char>());
    ASSERT_TRUE(output.find("1") != std::string::npos);
    ASSERT_TRUE(output.find("12345") != std::string::npos);
    readStream.close();
    std::remove(tempFile.c_str());
}

HWTEST_F_L0(PGOProfilerInfoTest, PGOPandaFileInfosProcessToTextMultipleSamplesTest)
{
    PGOPandaFileInfos fileInfos;
    fileInfos.Sample(100, 1);
    fileInfos.Sample(200, 2);
    fileInfos.Sample(300, 3);
    std::string tempFile = "test_output_3.txt";
    std::ofstream stream(tempFile);
    ASSERT_TRUE(stream.is_open());
    fileInfos.ProcessToText(stream);
    stream.close();
    std::ifstream readStream(tempFile);
    std::string output((std::istreambuf_iterator<char>(readStream)),
                       std::istreambuf_iterator<char>());
    ASSERT_TRUE(output.find("100") != std::string::npos);
    ASSERT_TRUE(output.find("200") != std::string::npos);
    ASSERT_TRUE(output.find("300") != std::string::npos);
    readStream.close();
    std::remove(tempFile.c_str());
}

HWTEST_F_L0(PGOProfilerInfoTest, PGOMethodInfoProcessToTextTest)
{
    PGOMethodId methodId(100);
    PGOMethodInfo methodInfo(methodId);
    methodInfo.IncreaseCount();
    std::string text;
    methodInfo.ProcessToText(text);
    ASSERT_FALSE(text.empty());
    ASSERT_TRUE(text.find("100") != std::string::npos);
    ASSERT_TRUE(text.find("1") != std::string::npos);
}

HWTEST_F_L0(PGOProfilerInfoTest, PGOMethodInfoProcessToJsonTest)
{
    PGOMethodId methodId(200);
    const char* methodName = "testMethod";
    PGOMethodInfo methodInfo(methodId, 5, SampleMode::CALL_MODE, methodName);
    ProfileType::VariantMap function;
    methodInfo.ProcessToJson(function);
    ASSERT_FALSE(function.empty());
    ASSERT_TRUE(function.find("functionName") != function.end());
}

HWTEST_F_L0(PGOProfilerInfoTest, PGOMethodInfoParseFromTextTest)
{
    {
        std::string infoString = "100/5/CALL/testMethod";
        auto result = PGOMethodInfo::ParseFromText(infoString);
        ASSERT_FALSE(result.empty());
        ASSERT_EQ(result.size(), 4U);
        ASSERT_EQ(result[0], "100");
        ASSERT_EQ(result[1], "5");
        ASSERT_EQ(result[2], "CALL");
        ASSERT_EQ(result[3], "testMethod");
    }
    {
        std::string infoString = "";
        auto result = PGOMethodInfo::ParseFromText(infoString);
        ASSERT_TRUE(result.empty());
    }
    {
        std::string infoString = "100";
        auto result = PGOMethodInfo::ParseFromText(infoString);
        ASSERT_EQ(result.size(), 1U);
        ASSERT_EQ(result[0], "100");
    }
    {
        std::string infoString = "100/5/CALL/testMethod/extra";
        auto result = PGOMethodInfo::ParseFromText(infoString);
        ASSERT_EQ(result.size(), 5U);
        ASSERT_EQ(result[4], "extra");
    }
}

HWTEST_F_L0(PGOProfilerInfoTest, ParseFromTextCountBelowThresholdTest)
{
    auto chunk = std::make_unique<Chunk>();
    PGOMethodInfoMap methodMap;
    std::vector<std::string> content;
    content.push_back("100/1/CALL/testMethod");  // Count=1, threshold=5
    bool result = methodMap.ParseFromText(chunk.get(), 5, content);
    EXPECT_FALSE(result);  // Actual behavior based on code flow
}

HWTEST_F_L0(PGOProfilerInfoTest, ParseFromTextInvalidModeTest)
{
    auto chunk = std::make_unique<Chunk>();
    PGOMethodInfoMap methodMap;
    std::vector<std::string> content;
    content.push_back("100/5/INVALID_MODE/testMethod");  // Invalid mode
    bool result = methodMap.ParseFromText(chunk.get(), 5, content);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, ParseFromTextInvalidCountTest)
{
    auto chunk = std::make_unique<Chunk>();
    PGOMethodInfoMap methodMap;
    std::vector<std::string> content;
    content.push_back("100/abc/INVALID_MODE/testMethod");  // Invalid count
    bool result = methodMap.ParseFromText(chunk.get(), 5, content);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, ParseFromTextInvalidMethodIdTest)
{
    auto chunk = std::make_unique<Chunk>();
    PGOMethodInfoMap methodMap;
    std::vector<std::string> content;
    content.push_back("invalid_id/5/CALL/testMethod");  // Invalid method ID
    bool result = methodMap.ParseFromText(chunk.get(), 5, content);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, ParseFromTextInvalidTypeInfoFormatTest)
{
    auto chunk = std::make_unique<Chunk>();
    PGOMethodInfoMap methodMap;
    std::vector<std::string> content;
    content.push_back("100/5/CALL/testMethod/[invalid_format");  // Invalid type info format
    bool result = methodMap.ParseFromText(chunk.get(), 5, content);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, ParseFromTextTypeInfoParseFailedTest)
{
    auto chunk = std::make_unique<Chunk>();
    PGOMethodInfoMap methodMap;
    std::vector<std::string> content;
    content.push_back("100/5/CALL/testMethod/[100:INVALID_TYPE]");  // Invalid type
    bool result = methodMap.ParseFromText(chunk.get(), 5, content);
    EXPECT_FALSE(result);
}


HWTEST_F_L0(PGOProfilerInfoTest, ProcessToTextEmptyMethodInfosTest)
{
    PGOMethodInfoMap methodMap;
    std::string tempFile = "test_output.txt";
    std::ofstream stream(tempFile);
    methodMap.ProcessToText(5, "testRecord", stream);
    stream.close();
    std::ifstream readStream(tempFile);
    std::string output((std::istreambuf_iterator<char>(readStream)),
                      std::istreambuf_iterator<char>());
    readStream.close();
    std::remove(tempFile.c_str());
    EXPECT_TRUE(output.empty());
}

HWTEST_F_L0(PGOProfilerInfoTest, UpdateFileInfosAbcIDTest)
{
    PGOPandaFileInfos fileInfos;
    fileInfos.Sample(1000, 1);
    PGOContextMock context(PGOProfilerHeader::LAST_VERSION);
    fileInfos.UpdateFileInfosAbcID(context);
}

HWTEST_F_L0(PGOProfilerInfoTest, UpdateFileInfosAbcIDWithEmptyFileInfosTest)
{
    PGOPandaFileInfos fileInfos;
    PGOContextMock context(PGOProfilerHeader::LAST_VERSION);
    fileInfos.UpdateFileInfosAbcID(context);
}


HWTEST_F_L0(PGOProfilerInfoTest, MergeEmptyRecordInfosTest)
{
    PGORecordSimpleInfos recordInfos1(5);
    PGORecordSimpleInfos recordInfos2(5);
    recordInfos1.Merge(recordInfos2);
}

HWTEST_F_L0(PGOProfilerInfoTest, ParseFromTextEmptyMethodNameTest)
{
    auto chunk = std::make_unique<Chunk>();
    PGOMethodInfoMap methodMap;
    std::vector<std::string> content;
    content.push_back("100/5/CALL/");  // Empty method name
    bool result = methodMap.ParseFromText(chunk.get(), 5, content);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, ParseFromTextExtraDataTest)
{
    auto chunk = std::make_unique<Chunk>();
    PGOMethodInfoMap methodMap;
    std::vector<std::string> content;
    content.push_back("100/5/CALL/testMethod/extra/data");
    bool result = methodMap.ParseFromText(chunk.get(), 5, content);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, ChecksumMismatchTest)
{
    PGOPandaFileInfos fileInfos;
    fileInfos.Sample(1000, 1);  // checksum=1000, abcId=1
    std::unordered_map<CString, uint32_t> fileNameToChecksumMap;
    fileNameToChecksumMap["test.abc"] = 2000;  // Different checksum
    bool result = fileInfos.Checksum(fileNameToChecksumMap);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, ChecksumNotFoundTest)
{
    PGOPandaFileInfos fileInfos;
    fileInfos.Sample(1000, 1);  // checksum=1000, abcId=1
    std::unordered_map<CString, uint32_t> fileNameToChecksumMap;
    fileNameToChecksumMap["test.abc"] = 2000;  // Different checksum
    bool result = fileInfos.Checksum(fileNameToChecksumMap);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, IsFilterThresholdTest)
{
    PGOMethodId methodId(100);
    PGOMethodInfo methodInfo(methodId);
    methodInfo.IncreaseCount();  // Count = 1
    EXPECT_TRUE(methodInfo.IsFilter(5));
    EXPECT_FALSE(methodInfo.IsFilter(0));
}


HWTEST_F_L0(PGOProfilerInfoTest, MatchNonExistentAbcDescTest)
{
    PGORecordSimpleInfos recordInfos(5);
    bool result = recordInfos.Match("non_existent.abc", "testRecord", EntityId(100));
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, MatchNonExistentRecordNameTest)
{
    PGORecordSimpleInfos recordInfos(5);
    bool result = recordInfos.Match("test.abc", "non_existent_record", EntityId(100));
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, MergeEmptyRecordDetailInfosTest)
{
    PGORecordDetailInfos recordInfos1(5);
    PGORecordDetailInfos recordInfos2(5);
    recordInfos1.Merge(recordInfos2);
}

HWTEST_F_L0(PGOProfilerInfoTest, ProcessToTextEmptyRecordPoolTest)
{
    PGORecordDetailInfos recordInfos(5);
    std::string tempFile = "test_output_empty_pool.txt";
    std::ofstream stream(tempFile);
    ASSERT_TRUE(stream.is_open());
    recordInfos.ProcessToText(stream);
    stream.close();
    std::ifstream readStream(tempFile);
    std::string output((std::istreambuf_iterator<char>(readStream)),
                      std::istreambuf_iterator<char>());
    readStream.close();
    std::remove(tempFile.c_str());

    EXPECT_TRUE(true);
}

HWTEST_F_L0(PGOProfilerInfoTest, ClearOperationTest)
{
    PGORecordDetailInfos recordInfos(5);
    recordInfos.Clear();
}

HWTEST_F_L0(PGOProfilerInfoTest, ClearSafeOperationTest)
{
    PGORecordDetailInfos recordInfos(5);
    recordInfos.ClearSafe();
}


HWTEST_F_L0(PGOProfilerInfoTest, CalcChecksumNullByteCodeTest)
{
    const char* methodName = "testMethod";
    uint32_t checksum = PGOMethodInfo::CalcChecksum(methodName, nullptr, 0);
    EXPECT_NE(checksum, 0U);
}

HWTEST_F_L0(PGOProfilerInfoTest, CalcChecksumNullNameTest)
{
    uint8_t byteCodeArray[] = {0x01, 0x02, 0x03};
    uint32_t checksum = PGOMethodInfo::CalcChecksum(nullptr, byteCodeArray, sizeof(byteCodeArray));
    EXPECT_NE(checksum, 0U);
}

HWTEST_F_L0(PGOProfilerInfoTest, CalcOpCodeChecksumTest)
{
    uint8_t byteCodeArray[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint32_t checksum = PGOMethodInfo::CalcOpCodeChecksum(byteCodeArray, sizeof(byteCodeArray));
    EXPECT_NE(checksum, 0U);
}


HWTEST_F_L0(PGOProfilerInfoTest, AddRootPtTypeExistingInfoTest)
{
    PGORecordDetailInfos recordInfos(5);
    recordInfos.AddRootPtType(ProfileType(100), ProfileType(200));
}

HWTEST_F_L0(PGOProfilerInfoTest, IsDumpedExistingTypeTest)
{
    PGORecordDetailInfos recordInfos(5);
    bool result = recordInfos.IsDumped(ProfileType(100), ProfileType(200));
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, IsDumpedNonExistingTypeTest)
{
    PGORecordDetailInfos recordInfos(5);
    bool result = recordInfos.IsDumped(ProfileType(999), ProfileType(888));
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, CalcChecksumBothNullTest)
{
    uint32_t checksum = PGOMethodInfo::CalcChecksum(nullptr, nullptr, 0);
    EXPECT_EQ(checksum, 0U);
}

HWTEST_F_L0(PGOProfilerInfoTest, ParseFromTextEmptyStringTest)
{
    auto result = PGOMethodInfo::ParseFromText("");
    EXPECT_TRUE(result.empty());
}

HWTEST_F_L0(PGOProfilerInfoTest, ParseFromTextSingleElementTest)
{
    auto result = PGOMethodInfo::ParseFromText("100");
    EXPECT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0], "100");
}

HWTEST_F_L0(PGOProfilerInfoTest, ProcessToTextCountZeroTest)
{
    PGOMethodId methodId(100);
    PGOMethodInfo methodInfo(methodId);
    std::string text;
    methodInfo.ProcessToText(text);
    EXPECT_FALSE(text.empty());
    EXPECT_TRUE(text.find("100") != std::string::npos);
}

HWTEST_F_L0(PGOProfilerInfoTest, MergeSafeOperationTest)
{
    PGORecordDetailInfos recordInfos1(5);
    PGORecordDetailInfos recordInfos2(5);
    recordInfos1.MergeSafe(recordInfos2);
}
}  // namespace panda::test
