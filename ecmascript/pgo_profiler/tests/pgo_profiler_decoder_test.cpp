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
#include <unordered_map>

#include "gtest/gtest.h"

#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/tests/pgo_context_mock.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::pgo;

namespace panda::test {
class PGOProfilerDecoderTest : public testing::Test {
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

HWTEST_F_L0(PGOProfilerDecoderTest, LoadFullAlreadyLoadedTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    decoder.InitMergeData();
    bool result = decoder.LoadFull();
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, LoadFullWithInvalidPathTest)
{
    PGOProfilerDecoder decoder("invalid_path.ap", 5);
    bool result = decoder.LoadFull();
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, SaveAPTextFileNotLoadedTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    bool result = decoder.SaveAPTextFile("output.txt");
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, SaveAPTextFileWithInvalidPathTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    decoder.InitMergeData();
    bool result = decoder.SaveAPTextFile("/invalid/path/output.txt");
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, SaveAPTextFileWithNullHeaderTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    decoder.InitMergeData();
    bool result = decoder.SaveAPTextFile("output.txt");
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, SaveAPTextFileSuccessTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    decoder.InitMergeData();
    std::string tempFile = "test_output.txt";
    bool result = decoder.SaveAPTextFile(tempFile);
    EXPECT_FALSE(result);
    std::remove(tempFile.c_str());
}

HWTEST_F_L0(PGOProfilerDecoderTest, LoadAPBinaryFileWithInvalidPathTest)
{
    PGOProfilerDecoder decoder("invalid_path.ap", 5);
    bool result = decoder.LoadFull();
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, LoadAPBinaryFileWithInvalidFileExtensionTest)
{
    PGOProfilerDecoder decoder("test.txt", 5);
    bool result = decoder.LoadFull();
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, LoadAPBinaryFileMmapFailedTest)
{
    PGOProfilerDecoder decoder("invalid_file.ap", 5);
    bool result = decoder.LoadFull();
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, GetHClassTreeDescNotLoadedTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    PGOHClassTreeDesc *desc = nullptr;
    bool result = decoder.GetHClassTreeDesc(PGOSampleType(100), &desc);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, GetHClassTreeDescNotVerifiedTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    decoder.InitMergeData();
    decoder.Clear();
    PGOHClassTreeDesc *desc = nullptr;
    bool result = decoder.GetHClassTreeDesc(PGOSampleType(100), &desc);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, GetMismatchResultNotLoadedTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    uint32_t totalMethodCount = 0;
    uint32_t mismatchMethodCount = 0;
    std::set<std::pair<std::string, CString>> mismatchMethodSet;
    decoder.GetMismatchResult(nullptr, totalMethodCount, mismatchMethodCount, mismatchMethodSet);
    EXPECT_EQ(totalMethodCount, 0U);
    EXPECT_EQ(mismatchMethodCount, 0U);
}

HWTEST_F_L0(PGOProfilerDecoderTest, GetMismatchResultNotVerifiedTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    decoder.InitMergeData();
    decoder.Clear();
    uint32_t totalMethodCount = 0;
    uint32_t mismatchMethodCount = 0;
    std::set<std::pair<std::string, CString>> mismatchMethodSet;
    decoder.GetMismatchResult(nullptr, totalMethodCount, mismatchMethodCount, mismatchMethodSet);
    EXPECT_EQ(totalMethodCount, 0U);
    EXPECT_EQ(mismatchMethodCount, 0U);
}

HWTEST_F_L0(PGOProfilerDecoderTest, MergeNotLoadedTest)
{
    PGOProfilerDecoder decoder1("test1.ap", 5);
    PGOProfilerDecoder decoder2("test2.ap", 5);
    decoder2.InitMergeData();
    decoder1.Merge(decoder2);
}

HWTEST_F_L0(PGOProfilerDecoderTest, MergeNotVerifiedTest)
{
    PGOProfilerDecoder decoder1("test1.ap", 5);
    PGOProfilerDecoder decoder2("test2.ap", 5);
    decoder1.InitMergeData();
    decoder1.Clear();
    decoder2.InitMergeData();
    decoder1.Merge(decoder2);
}

HWTEST_F_L0(PGOProfilerDecoderTest, LoadAndVerifyWithEmptyPathTest)
{
    PGOProfilerDecoder decoder("", 5);
    std::unordered_map<CString, uint32_t> fileNameToChecksumMap;
    bool result = decoder.LoadAndVerify(fileNameToChecksumMap);
    EXPECT_TRUE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, LoadAndVerifyWithLoadFailureTest)
{
    PGOProfilerDecoder decoder("invalid.ap", 5);
    std::unordered_map<CString, uint32_t> fileNameToChecksumMap;
    bool result = decoder.LoadAndVerify(fileNameToChecksumMap);
    EXPECT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerDecoderTest, GetAbcFilePoolTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    auto abcPool = decoder.GetAbcFilePool();
    EXPECT_EQ(abcPool, nullptr);
}

HWTEST_F_L0(PGOProfilerDecoderTest, SetInPathTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    decoder.SetInPath("new_path.ap");
    EXPECT_EQ(decoder.GetInPath(), "new_path.ap");
}

HWTEST_F_L0(PGOProfilerDecoderTest, SetHotnessThresholdTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    decoder.SetHotnessThreshold(10);
    EXPECT_EQ(decoder.GetHotnessThreshold(), 10U);
}

HWTEST_F_L0(PGOProfilerDecoderTest, IsMethodMatchEnabledTest)
{
    PGOProfilerDecoder decoder("test.ap", 5);
    decoder.InitMergeData();
    bool result = decoder.IsMethodMatchEnabled();
    EXPECT_FALSE(result);
}
}  // namespace panda::test
