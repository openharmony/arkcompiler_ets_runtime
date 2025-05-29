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

    void SetUp() override {}

    void TearDown() override {}
};

HWTEST_F_L0(OSTest, CheckDiskSpaceTest)
{
    std::string validPath = "/tmp";
    ASSERT_TRUE(CheckDiskSpace(validPath, 1024));
    std::string invalidPath = "/invalid/path/that/does/not/exist";
    ASSERT_FALSE(CheckDiskSpace(invalidPath, 1024));
    size_t largeSize = SIZE_MAX;
    ASSERT_FALSE(CheckDiskSpace(validPath, largeSize));
    ASSERT_TRUE(CheckDiskSpace(validPath, 0));
}
}