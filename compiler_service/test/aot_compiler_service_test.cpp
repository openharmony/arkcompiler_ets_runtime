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

#include "aot_compiler_client.h"
#include "aot_compiler_service.h"
#include "aot_compiler_error_utils.h"
#include "aot_compiler_load_callback.h"
#include "system_ability_definition.h"
#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include <vector>

using namespace testing::ext;

namespace OHOS {
namespace ArkCompiler {
class AotCompilerServiceTest : public testing::Test {
public:
    AotCompilerServiceTest() {}
    virtual ~AotCompilerServiceTest() {}

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: AotCompilerServiceTest_001
 * @tc.desc: invoke aot_compiler service in client program.
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_001, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    std::vector<int16_t> sigData;
    int32_t ret = AotCompilerClient::GetInstance().AotCompiler(argsMap, sigData);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
    EXPECT_EQ(argsMap.empty(), true);
    EXPECT_EQ(sigData.empty(), true);
}

/**
 * @tc.name: AotCompilerServiceTest_002
 * @tc.desc: Load SA fail.
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_002, TestSize.Level0)
{
    AotCompilerLoadCallback cb;
    cb.OnLoadSystemAbilityFail(AOT_COMPILER_SERVICE_ID);
}

/**
 * @tc.name: AotCompilerServiceTest_003
 * @tc.desc: Load SA success and returned SA_ID doesn't match.
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_003, TestSize.Level0)
{
    AotCompilerLoadCallback cb;
    cb.OnLoadSystemAbilitySuccess(AOT_COMPILER_SERVICE_ID - 1, nullptr);
}

/**
 * @tc.name: AotCompilerServiceTest_004
 * @tc.desc: Load SA success and returned remote object is null.
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_004, TestSize.Level0)
{
    AotCompilerLoadCallback cb;
    cb.OnLoadSystemAbilitySuccess(AOT_COMPILER_SERVICE_ID, nullptr);
}
} // namespace OHOS
} // namespace ArkCompiler