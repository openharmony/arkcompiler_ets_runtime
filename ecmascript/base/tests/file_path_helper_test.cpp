/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class FilePathHelperTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FilePathHelperTest, RealPath)
{
    std::string filePath = "__FilePathHelperTest.test";
    EXPECT_TRUE(std::fopen(filePath.c_str(), "r") == nullptr);

    std::string realPath;
    bool result = RealPath(filePath, realPath, false);
    EXPECT_EQ(result, true);

    std::fstream stream {};
    stream.open(realPath, std::ios::out);
    EXPECT_EQ(stream.good(), true);
    EXPECT_TRUE(std::fopen(realPath.c_str(), "r") != nullptr);

    stream.close();
    stream.clear();
    std::remove(filePath.c_str());
    std::remove(realPath.c_str());
}
}  // namespace panda::test
