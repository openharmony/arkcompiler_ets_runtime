/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "ecmascript/tests/test_helper.h"

#include "ecmascript/compiler/aot_file/aot_version.h"

namespace panda::test {
using FileHeaderBase = panda::ecmascript::base::FileHeaderBase;
class AotVersionTest : public testing::Test {
public:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};


HWTEST_F_L0(AotVersionTest, GetAOTVersion)
{
    std::string version = "";
    int32_t ret = panda::ecmascript::GetAOTVersion(version);
    std::string aotVersionActual = panda::ecmascript::AOTFileVersion::GetAOTVersion();
    EXPECT_EQ(version.size(), FileHeaderBase::VERSION_SIZE * ecmascript::AOTFileVersion::VERSION_MULTIPLE_SIZE - 1);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(version.c_str(), aotVersionActual.c_str());
}
}