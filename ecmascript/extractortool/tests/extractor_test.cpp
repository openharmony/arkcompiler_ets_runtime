/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distribZuted under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdio>
#include <iostream>

#include "ecmascript/extractortool/src/extractor.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::ecmascript {
class ExtractorFriend {
public:
    explicit ExtractorFriend(const std::string &source) : extractor_(source)
    {
    }

    bool GetInitial() const
    {
        return extractor_.initial_;
    }

    void SetInitial(bool initialValue)
    {
        extractor_.initial_ = initialValue;
    }

    const std::string &GetHapPath() const
    {
        return extractor_.hapPath_;
    }

    void SetHapPath(const std::string &newHapPath)
    {
        extractor_.hapPath_ = newHapPath;
    }

    std::optional<bool> GetIsStageModel() const
    {
        return extractor_.isStageModel_;
    }

    void SetIsStageModel(bool model)
    {
        extractor_.isStageModel_ = model;
    }

    Extractor extractor_;
};

class ExtractorUtilFriend {
public:
    void DeleteExtractor(const std::string &hapPath)
    {
        return extractorUtil_.DeleteExtractor(hapPath);
    }

    std::unordered_map<std::string, std::shared_ptr<Extractor>> GetExtractorMap()
    {
        return extractorUtil_.extractorMap_;
    }

private:
    ExtractorUtil extractorUtil_;
};
}

namespace panda::test {
class ExtractorTest : public testing::Test {
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

HWTEST_F_L0(ExtractorTest, InitTest)
{
    ExtractorFriend extractorFriend("TestFile");
    EXPECT_FALSE(extractorFriend.extractor_.Init());
}

HWTEST_F_L0(ExtractorTest, GetFileBufferTest)
{
    ExtractorFriend extractorFriend("TestFile");
    std::ostringstream dest;
    EXPECT_FALSE(extractorFriend.extractor_.GetFileBuffer("Test", dest));
    extractorFriend.SetInitial(true);
    EXPECT_FALSE(extractorFriend.extractor_.GetFileBuffer("", dest));
    EXPECT_FALSE(extractorFriend.extractor_.GetFileBuffer("Test", dest));
}

HWTEST_F_L0(ExtractorTest, GetFileListTest)
{
    ExtractorFriend extractorFriend("TestFile");
    std::vector<std::string> assetList;
    EXPECT_FALSE(extractorFriend.extractor_.GetFileList("TestSrcPath", assetList));
    extractorFriend.SetInitial(true);
    EXPECT_FALSE(extractorFriend.extractor_.GetFileList("", assetList));
    EXPECT_TRUE(extractorFriend.extractor_.GetFileList("/", assetList));
    std::set<std::string> fileSet;
    EXPECT_TRUE(extractorFriend.extractor_.GetFileList("TestSrcPath", fileSet));
}

HWTEST_F_L0(ExtractorTest, HasEntryTest)
{
    ExtractorFriend extractorFriend("TestFile");
    EXPECT_FALSE(extractorFriend.extractor_.HasEntry("TestFileName"));
    extractorFriend.SetInitial(true);
    EXPECT_FALSE(extractorFriend.extractor_.HasEntry("TestFileName"));
}

HWTEST_F_L0(ExtractorTest, IsDirExistTest)
{
    ExtractorFriend extractorFriend("TestFile");
    EXPECT_FALSE(extractorFriend.extractor_.IsDirExist("TestFileName"));
    extractorFriend.SetInitial(true);
    EXPECT_FALSE(extractorFriend.extractor_.IsDirExist(""));
    EXPECT_TRUE(extractorFriend.extractor_.IsDirExist("/"));
}

HWTEST_F_L0(ExtractorTest, ExtractByNameTest)
{
    ExtractorFriend extractorFriend("TestFile");
    std::ostringstream dest;
    EXPECT_FALSE(extractorFriend.extractor_.ExtractByName("TestFileName", dest));
    extractorFriend.SetInitial(true);
}

HWTEST_F_L0(ExtractorTest, ExtractFileTest)
{
    ExtractorFriend extractorFriend("TestFile");
    EXPECT_FALSE(extractorFriend.extractor_.ExtractFile("TestFileName", "TestPath"));
}

HWTEST_F_L0(ExtractorTest, GetSafeDataTest)
{
    ExtractorFriend extractorFriend("TestFile");
    auto result = extractorFriend.extractor_.GetSafeData("TestFileName.abc");
    EXPECT_EQ(result, nullptr);

    result = extractorFriend.extractor_.GetSafeData("TestFileName");
    EXPECT_EQ(result, nullptr);
}

HWTEST_F_L0(ExtractorTest, IsStageModelTest)
{
    ExtractorFriend extractorFriend("TestFile");
    EXPECT_TRUE(extractorFriend.extractor_.IsStageModel());
    EXPECT_TRUE(extractorFriend.extractor_.IsStageModel());
}

HWTEST_F_L0(ExtractorTest, ExtractToBufByNameTest)
{
    ExtractorFriend extractorFriend("TestFile");
    auto byteArray = std::make_unique<uint8_t[]>(1024);
    size_t len;
    EXPECT_FALSE(extractorFriend.extractor_.ExtractToBufByName("TestFileName", byteArray, len));
}

HWTEST_F_L0(ExtractorTest, GetFileInfoTest)
{
    ExtractorFriend extractorFriend("TestFile");
    FileInfo fileInfo;
    EXPECT_FALSE(extractorFriend.extractor_.GetFileInfo("TestFileName", fileInfo));
}

HWTEST_F_L0(ExtractorTest, GetFileListTest2)
{
    ExtractorFriend extractorFriend("TestFile");
    std::set<std::string> fileSet;
    EXPECT_FALSE(extractorFriend.extractor_.GetFileList("TestSrcPath", fileSet));

    extractorFriend.SetInitial(true);
    EXPECT_FALSE(extractorFriend.extractor_.GetFileList("", fileSet));
}

HWTEST_F_L0(ExtractorTest, IsHapCompressTest)
{
    ExtractorFriend extractorFriend("TestFile");
    EXPECT_FALSE(extractorFriend.extractor_.IsHapCompress("TestFileName"));
}

HWTEST_F_L0(ExtractorTest, GetLoadFilePathTest)
{
    ExtractorUtil util;
    EXPECT_EQ(util.GetLoadFilePath("/data/app/el1/bundle/public/Test.hap"), "/data/storage/el1/bundle/Test.hap");
    EXPECT_EQ(util.GetLoadFilePath("Test.hap"), "Test.hap");
}

HWTEST_F_L0(ExtractorTest, GetExtractorTest)
{
    ExtractorUtil util;
    bool newCreate = false;
    bool cache = false;
    auto result = util.GetExtractor("", newCreate, cache);
    EXPECT_EQ(result, nullptr);

    result = util.GetExtractor("TestHapPath.hap", newCreate, cache);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F_L0(ExtractorTest, DeleteExtractorTest)
{
    ExtractorUtilFriend util;
    util.DeleteExtractor("");
    util.DeleteExtractor("TestHapPath.hap");
    std::unordered_map<std::string, std::shared_ptr<Extractor>> emptyMap;
    auto map = util.GetExtractorMap();
    EXPECT_EQ(map, emptyMap);
}
}