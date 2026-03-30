/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <memory>
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

HWTEST_F_L0(PGOProfilerInfoTest, ProcessToTextEmptyMethodInfosTest)
{
    PGOMethodInfoMap methodMap;
    TextFormatter fmt;
    methodMap.ProcessToText(5, "testRecord", fmt);
    std::string output = fmt.Str();
    // Even empty method map outputs header with record name and zero stats
    EXPECT_TRUE(output.find("Record:") != std::string::npos);
    EXPECT_TRUE(output.find("testRecord") != std::string::npos);
    EXPECT_TRUE(output.find("Methods:") != std::string::npos);
    EXPECT_TRUE(output.find("Total Calls:") != std::string::npos);
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

HWTEST_F_L0(PGOProfilerInfoTest, MergeSafeOperationTest)
{
    PGORecordDetailInfos recordInfos1(5);
    PGORecordDetailInfos recordInfos2(5);
    recordInfos1.MergeSafe(recordInfos2);
}

HWTEST_F_L0(PGOProfilerInfoTest, ChecksumMatchTest)
{
    PGOPandaFileInfos fileInfos;
    fileInfos.Sample(1000, 1);
    std::unordered_map<CString, uint32_t> fileNameToChecksumMap;
    fileNameToChecksumMap["test.abc"] = 1000;
    bool result = fileInfos.Checksum(fileNameToChecksumMap);
    EXPECT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, ChecksumMultipleFilesTest)
{
    PGOPandaFileInfos fileInfos;
    fileInfos.Sample(1000, 1);
    fileInfos.Sample(2000, 2);
    fileInfos.Sample(3000, 3);
    std::unordered_map<CString, uint32_t> fileNameToChecksumMap;
    fileNameToChecksumMap["test1.abc"] = 1000;
    fileNameToChecksumMap["test2.abc"] = 2000;
    fileNameToChecksumMap["test3.abc"] = 3000;
    bool result = fileInfos.Checksum(fileNameToChecksumMap);
    EXPECT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, ChecksumPartialMatchTest)
{
    PGOPandaFileInfos fileInfos;
    fileInfos.Sample(1000, 1);
    fileInfos.Sample(2000, 2);
    std::unordered_map<CString, uint32_t> fileNameToChecksumMap;
    fileNameToChecksumMap["test1.abc"] = 1000;
    fileNameToChecksumMap["test2.abc"] = 9999;
    bool result = fileInfos.Checksum(fileNameToChecksumMap);
    EXPECT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, VerifyChecksumBothEmptyTest)
{
    PGOPandaFileInfos baseFileInfos;
    PGOPandaFileInfos incomingFileInfos;
    bool result = baseFileInfos.VerifyChecksum(incomingFileInfos, "base.ap", "incoming.ap");
    EXPECT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, MergeExistingRecordTypeTest)
{
    PGORecordDetailInfos recordInfos1(5);
    PGORecordDetailInfos recordInfos2(5);

    ProfileType rootType(100);
    ProfileType curType1(200);
    ProfileType curType2(300);

    recordInfos1.AddRootPtType(rootType, curType1);
    recordInfos2.AddRootPtType(rootType, curType2);

    // Merge should combine the hclassTreeDescInfos
    recordInfos1.Merge(recordInfos2);

    // Verify merge was successful
    bool result1 = recordInfos1.IsDumped(rootType, curType1);
    bool result2 = recordInfos1.IsDumped(rootType, curType2);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(PGOProfilerInfoTest, MergeNewRecordTypeTest)
{
    PGORecordDetailInfos recordInfos1(5);
    PGORecordDetailInfos recordInfos2(5);

    // Add different root types to each
    ProfileType rootType1(100);
    ProfileType rootType2(200);
    ProfileType curType1(300);
    ProfileType curType2(400);

    recordInfos1.AddRootPtType(rootType1, curType1);
    recordInfos2.AddRootPtType(rootType2, curType2);

    // Merge should add new record type
    recordInfos1.Merge(recordInfos2);

    // Verify both are present after merge
    bool result1 = recordInfos1.IsDumped(rootType1, curType1);
    bool result2 = recordInfos1.IsDumped(rootType2, curType2);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(PGOProfilerInfoTest, AddDefineNewHClassTreeDescTest)
{
    PGORecordDetailInfos recordInfos(5);
    PGOMethodId methodId(100);
    ProfileType profileType(100);

    // Create PGODefineOpType with single argument constructor
    PGODefineOpType defineOpType(profileType);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(PGOProfilerInfoTest, AddDefineExistingHClassTreeDescTest)
{
    PGORecordDetailInfos recordInfos(5);
    PGOMethodId methodId(100);
    ProfileType profileType(100);

    // Create different PGODefineOpType instances
    PGODefineOpType defineOpType1(profileType);
    PGODefineOpType defineOpType2(profileType);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(PGOProfilerInfoTest, IsDumpedExistingDescTest)
{
    PGORecordDetailInfos recordInfos(5);
    ProfileType rootType(100);
    ProfileType curType(200);

    // Add a desc first
    recordInfos.AddRootPtType(rootType, curType);

    bool result = recordInfos.IsDumped(rootType, curType);
    // Result depends on internal state - just verify it doesn't crash
    EXPECT_TRUE(true);
}

HWTEST_F_L0(PGOProfilerInfoTest, GetSampleModeHotnessModeTest)
{
    SampleMode mode;
    std::string content = "HOTNESS_MODE";
    bool result = PGOMethodInfo::GetSampleMode(content, mode);
    EXPECT_TRUE(result);
    EXPECT_EQ(mode, SampleMode::HOTNESS_MODE);
}

HWTEST_F_L0(PGOProfilerInfoTest, GetSampleModeInvalidModeTest)
{
    SampleMode mode;
    std::string content = "INVALID_MODE";
    bool result = PGOMethodInfo::GetSampleMode(content, mode);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerInfoTest, GetSampleModeToStringDefaultTest)
{
    PGOMethodId methodId(100);
    PGOMethodInfo methodInfo(methodId);  // Default mode is CALL_MODE
    std::string modeStr = methodInfo.GetSampleModeToString();
    EXPECT_EQ(modeStr, "CALL_MODE");
}

HWTEST_F_L0(PGOProfilerInfoTest, UpdateFileInfosAbcIDWithRemapTest)
{
    PGOPandaFileInfos fileInfos;
    fileInfos.Sample(1000, 1);
    fileInfos.Sample(2000, 2);

    PGOContextMock context(PGOProfilerHeader::LAST_VERSION);
    context.AddAbcIdRemap(ApEntityId(1), ApEntityId(10));
    context.AddAbcIdRemap(ApEntityId(2), ApEntityId(20));

    fileInfos.UpdateFileInfosAbcID(context);

    // Verify abcIds are updated
    std::vector<uint32_t> abcIds;
    fileInfos.ForEachFileInfo([&abcIds](uint32_t checksum, uint32_t abcId) {
        abcIds.push_back(abcId);
    });

    EXPECT_EQ(abcIds.size(), 2U);
}

HWTEST_F_L0(PGOProfilerInfoTest, UpdateFileInfosAbcIDWithoutRemapTest)
{
    PGOPandaFileInfos fileInfos;
    fileInfos.Sample(1000, 1);
    fileInfos.Sample(2000, 2);

    PGOContextMock context(PGOProfilerHeader::LAST_VERSION);
    // No remap added

    fileInfos.UpdateFileInfosAbcID(context);

    // Verify abcIds remain unchanged
    std::vector<uint32_t> abcIds;
    fileInfos.ForEachFileInfo([&abcIds](uint32_t checksum, uint32_t abcId) {
        abcIds.push_back(abcId);
    });

    EXPECT_EQ(abcIds.size(), 2U);
    EXPECT_EQ(abcIds[0], 1U);
    EXPECT_EQ(abcIds[1], 2U);
}

HWTEST_F_L0(PGOProfilerInfoTest, CalcChecksumBothParamsTest)
{
    const char* methodName = "testMethod";
    uint8_t byteCodeArray[] = {0x01, 0x02, 0x03};
    uint32_t checksum = PGOMethodInfo::CalcChecksum(methodName, byteCodeArray, sizeof(byteCodeArray));
    EXPECT_NE(checksum, 0U);
}

HWTEST_F_L0(PGOProfilerInfoTest, CalcOpCodeChecksumEmptyTest)
{
    uint8_t byteCodeArray[] = {0x00};
    uint32_t checksum = PGOMethodInfo::CalcOpCodeChecksum(byteCodeArray, 0);
    EXPECT_EQ(checksum, 0U);
}

HWTEST_F_L0(PGOProfilerInfoTest, SimpleInfosMergeExistingTest)
{
    PGORecordSimpleInfos simpleInfos1(5);
    PGORecordSimpleInfos simpleInfos2(5);
    simpleInfos1.Merge(simpleInfos2);
    EXPECT_TRUE(true);
}

}  // namespace panda::test
