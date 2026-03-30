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

#include "ecmascript/platform/file.h"
#include "ecmascript/tests/test_helper.h"
#include "macros.h"

#include "gtest/gtest.h"
#include <cstdio>
#include <system_error>

namespace panda::test {
using namespace panda::ecmascript;

class ChmodTest : public BaseTestWithOutScope {
public:
    constexpr static const std::string_view EXISTING_FILE_NAME = "./test_chmod.txt";
    constexpr static const std::string_view NOT_EXISTING_FILE_NAME = "./test_chmod_not_exist.txt";
    void SetUp() override
    {
        file_ = std::fopen(EXISTING_FILE_NAME.data(), "w");
        ASSERT_NE(file_, nullptr);
        std::fflush(file_);
    }

    void TearDown() override
    {
        std::fclose(file_);
        std::remove(EXISTING_FILE_NAME.data());
        file_ = nullptr;
    }

    std::FILE *file_{};
};

HWTEST_F_L0(ChmodTest, Chmod_ExistFile_NoErrorCode)
{
    ASSERT_TRUE(Chmod(EXISTING_FILE_NAME, "r"));
}

HWTEST_F_L0(ChmodTest, Chmod_NotExistFile_NoErrorCode)
{
    ASSERT_FALSE(Chmod(NOT_EXISTING_FILE_NAME, "r"));
}

HWTEST_F_L0(ChmodTest, Chmod_ExistFile_WithErrorCode)
{
    std::error_code ec{};
    ASSERT_TRUE(Chmod(EXISTING_FILE_NAME, "r", ec));
    ASSERT_EQ(ec.value(), 0);
}

HWTEST_F_L0(ChmodTest, Chmod_NotExistFile_WithErrorCode)
{
    std::error_code ec{};
    ASSERT_FALSE(Chmod(NOT_EXISTING_FILE_NAME, "r", ec));
    ASSERT_NE(ec.value(), 0);
}
} // namespace panda::test
