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

#include <fstream>

#include "ecmascript/platform/filesystem.h"
#include "ecmascript/platform/os.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class OSTest : public ::testing::Test {
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
        tempDir_ = filesystem::TempDirectoryPath() + "/os_test_temp";
        filesystem::CreateDirectory(tempDir_);
    }

    void TearDown() override
    {
        filesystem::RemoveAll(tempDir_);
    }

protected:
    std::string tempDir_;
};

HWTEST_F_L0(OSTest, CheckDiskSpaceTest)
{
    ASSERT_TRUE(CheckDiskSpace(tempDir_, 1024));
    std::string invalidPath = "/invalid/path/that/does/not/exist";
    ASSERT_FALSE(CheckDiskSpace(invalidPath, 1024));
    size_t largeSize = SIZE_MAX;
    ASSERT_FALSE(CheckDiskSpace(tempDir_, largeSize));
    ASSERT_TRUE(CheckDiskSpace(tempDir_, 0));
}

HWTEST_F_L0(OSTest, FileSizeTest)
{
    // Test case 1: file does not exist, stat fails, should return 0
    std::string nonExistentPath = tempDir_ + "/non_existent_file.txt";
    ASSERT_EQ(filesystem::FileSize(nonExistentPath), 0);

    // Test case 2: create an empty file, should return 0
    std::string emptyFilePath = tempDir_ + "/empty_file.txt";
    std::ofstream emptyFile(emptyFilePath);
    emptyFile.close();
    ASSERT_EQ(filesystem::FileSize(emptyFilePath), 0);

    // Test case 3: create a file with content, should return correct size
    std::string contentFilePath = tempDir_ + "/content_file.txt";
    std::string testContent = "Hello, World!";
    std::ofstream contentFile(contentFilePath);
    contentFile << testContent;
    contentFile.close();
    ASSERT_EQ(filesystem::FileSize(contentFilePath), testContent.size());
}
}