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

#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "aot_compiler_client.h"
#include "aot_compiler_service.h"
#include "aot_compiler_error_utils.h"
#include "aot_compiler_load_callback.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

using namespace testing::ext;

namespace OHOS::ArkCompiler {
class AotCompilerErrorUtilsTest : public testing::Test {
public:
    AotCompilerErrorUtilsTest() {}
    virtual ~AotCompilerErrorUtilsTest() {}

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: AotCompilerErrorUtilsTest_001
 * @tc.desc: AotCompilerErrorUtil::GetErrorMessage(errCode)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerErrorUtilsTest, AotCompilerErrorUtilsTest_001, TestSize.Level0)
{
    AotCompilerErrorUtil aotErrorUtils;
    std::string errMsg1;
    std::string errMsg2;
    errMsg1 = aotErrorUtils.GetErrorMessage(ERR_OK);
    errMsg2 = aotErrorUtils.GetErrorMessage(INVALID_ERR_CODE);
    EXPECT_STREQ(errMsg1.c_str(), "success");
    EXPECT_STREQ(errMsg2.c_str(), "invalid errCode");
}
} // namespace OHOS::ArkCompiler