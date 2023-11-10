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

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

#include "gtest/gtest.h"

#include "ecmascript/ohos/white_list_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class OhosTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void TearDown() override {}
};

HWTEST_F_L0(OhosTest, AotWhiteListTest)
{
    const char *whiteListTestDir = "ohos-whiteList/";
    const char *whiteListName = "ohos-whiteList/app_aot_white_list.conf";
    std::string bundleScope = "com.bundle.scope.test";
    std::string moduleScope = "com.module.scope.test";
    mkdir(whiteListTestDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream file(whiteListName);
    file << bundleScope << std::endl;
    file << moduleScope << ":entry" << std::endl;

    file.close();
    auto helper = std::make_unique<WhiteListHelper>(whiteListName);
    ASSERT_TRUE(helper->IsEnable(bundleScope));
    ASSERT_TRUE(helper->IsEnable(bundleScope, "entry"));
    ASSERT_TRUE(helper->IsEnable(moduleScope, "entry"));
    ASSERT_FALSE(helper->IsEnable(moduleScope, "entry1"));
    unlink(whiteListName);
    rmdir(whiteListTestDir);
}
}  // namespace panda::test