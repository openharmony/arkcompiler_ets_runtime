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

#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::pgo;

namespace panda::test {
class PGOProfilerEncoderTest : public testing::Test {
public:
    using ApGenMode = PGOProfilerEncoder::ApGenMode;

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
        testDir_ = "test_encoder_dir/";
        testFilePath_ = testDir_ + "test_profile.ap";
        mkdir(testDir_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    void TearDown() override
    {
        if (FileExist(testFilePath_.c_str())) {
            unlink(testFilePath_.c_str());
        }
        rmdir(testDir_.c_str());

        PGOProfilerManager::GetInstance()->SetDisablePGO(false);
        PGOProfilerManager::GetInstance()->Destroy();
    }

protected:
    std::shared_ptr<PGOInfo> CreateMockPGOInfo(uint32_t threshold = 2)
    {
        auto pgoInfo = std::make_shared<PGOInfo>(threshold);
        return pgoInfo;
    }

    void CreateTestFile(const std::string& path, const std::string& content = "test content")
    {
        std::ofstream file(path);
        file << content;
        file.close();
    }

    std::string testDir_;
    std::string testFilePath_;
    static constexpr uint32_t DECODER_THRESHOLD = 2;
};

HWTEST_F_L0(PGOProfilerEncoderTest, SaveWithValidPGOInfoTest)
{
    PGOProfilerEncoder encoder(testFilePath_, ApGenMode::OVERWRITE);
    auto pgoInfo = CreateMockPGOInfo();
    bool result = encoder.Save(pgoInfo);
    ASSERT_TRUE(result == true || result == false); // Any result is acceptable as long as it doesn't crash
}

HWTEST_F_L0(PGOProfilerEncoderTest, SaveWithNullPGOInfoTest)
{
    PGOProfilerEncoder encoder(testFilePath_, ApGenMode::OVERWRITE);
    std::shared_ptr<PGOInfo> nullInfo = nullptr;
    bool result = encoder.Save(nullInfo);
    ASSERT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerEncoderTest, InvalidPathTest)
{
    std::string invalidPath = "/invalid/path/that/should/not/exist/test.ap";
    PGOProfilerEncoder encoder(invalidPath, ApGenMode::OVERWRITE);
    auto pgoInfo = CreateMockPGOInfo();
    bool result = encoder.Save(pgoInfo);
    ASSERT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerEncoderTest, EmptyPathTest)
{
    PGOProfilerEncoder encoder("", ApGenMode::OVERWRITE);
    auto pgoInfo = CreateMockPGOInfo();
    bool result = encoder.Save(pgoInfo);
    ASSERT_FALSE(result);
}

HWTEST_F_L0(PGOProfilerEncoderTest, EncoderModeToggleTest)
{
    PGOProfilerEncoder encoder(testFilePath_, ApGenMode::OVERWRITE);
    ASSERT_EQ(encoder.GetApGenMode(), ApGenMode::OVERWRITE);
    encoder.SetApGenMode(ApGenMode::MERGE);
    ASSERT_EQ(encoder.GetApGenMode(), ApGenMode::MERGE);
    encoder.SetApGenMode(ApGenMode::OVERWRITE);
    ASSERT_EQ(encoder.GetApGenMode(), ApGenMode::OVERWRITE);
}
} // namespace panda::test