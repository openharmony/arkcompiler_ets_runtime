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

#include "ecmascript/compiler/aot_compiler_preprocessor.h"
#include "ecmascript/platform/filesystem.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::kungfu;
class CreateEmptyFileTest : public testing::Test {
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
        temp_dir = filesystem::TempDirectoryPath() + "/create_empty_file_test";
        filesystem::CreateDirectory(temp_dir);
    }

    void TearDown() override
    {
        filesystem::RemoveAll(temp_dir);
    }

protected:
    std::string temp_dir;
};

HWTEST_F_L0(CreateEmptyFileTest, FileDoesNotExist)
{
    std::string file_path = temp_dir + "/dir1/dir2/entry.an";
    filesystem::CreateEmptyFile(file_path);

    EXPECT_TRUE(filesystem::Exists(file_path));
}

HWTEST_F_L0(CreateEmptyFileTest, FileExists)
{
    std::string file_path = temp_dir + "/entry.an";

    std::ofstream ofs(file_path);
    ofs << "test data";
    ofs.close();

    auto old_size = filesystem::FileSize(file_path);
    EXPECT_GT(old_size, 0);

    filesystem::CreateEmptyFile(file_path);

    auto new_size = filesystem::FileSize(file_path);

    EXPECT_TRUE(filesystem::Exists(file_path));
    EXPECT_GT(old_size, 0);
    EXPECT_GT(new_size, 0);
}

HWTEST_F_L0(CreateEmptyFileTest, DirectoryDoesNotExist)
{
    std::string dir_path = temp_dir + "/dir4/com.hm.app";
    std::string file_path = dir_path + "/entry.an";
    filesystem::CreateEmptyFile(file_path);

    EXPECT_TRUE(filesystem::Exists(dir_path));
    EXPECT_TRUE(filesystem::Exists(file_path));
}
} // namespace panda::test