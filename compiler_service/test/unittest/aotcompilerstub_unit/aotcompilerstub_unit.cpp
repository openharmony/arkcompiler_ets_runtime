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
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "aot_compiler_client.h"
#include "aot_compiler_service.h"
#include "aot_compiler_error_utils.h"
#include "aot_compiler_interface_proxy.h"
#include "aot_compiler_interface_stub.h"
#include "aot_compiler_load_callback.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

using namespace testing::ext;

namespace OHOS::ArkCompiler {
namespace {
constexpr int32_t TEST_COMMAND_AOT_COMPILER = MIN_TRANSACTION_ID + 0;
constexpr int32_t TEST_COMMAND_STOP_AOT_COMPILER = MIN_TRANSACTION_ID + 1;
constexpr int32_t TEST_COMMAND_GET_AOT_VERSION = MIN_TRANSACTION_ID + 2;
constexpr int32_t TEST_COMMAND_NEED_RE_COMPILE = MIN_TRANSACTION_ID + 3;
constexpr int32_t TEST_COMMAND_INVALID = MIN_TRANSACTION_ID + 1000;
const unsigned long MAP_MAX_SIZE = 102400;
}

class AotCompilerStubMock : public AotCompilerInterfaceStub {
public:
    AotCompilerStubMock() = default;
    virtual ~AotCompilerStubMock() = default;

    MOCK_METHOD(ErrCode, AotCompiler, ((const std::unordered_map<std::string, std::string> &argsMap),
        std::vector<int16_t> &sigData), (override));
    MOCK_METHOD(ErrCode, StopAotCompiler, (), (override));
    MOCK_METHOD(ErrCode, GetAOTVersion, (std::string& sigData), (override));
    MOCK_METHOD(ErrCode, NeedReCompile, (const std::string& args, bool& sigData), (override));
};

class AotCompilerStubTest : public testing::Test {
public:
    AotCompilerStubTest() {}
    virtual ~AotCompilerStubTest() {}

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override;
    void TearDown() override {}
    sptr<AotCompilerStubMock> aotCompilerStub_ = nullptr;
};

void AotCompilerStubTest::SetUp()
{
    aotCompilerStub_ = new (std::nothrow) AotCompilerStubMock();
}

/**
 * @tc.name: AotCompilerStubTest_001
 * @tc.desc: AotCompilerInterfaceStub::CommandAOTCompiler()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_001, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_AOT_COMPILER);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());
    AotCompilerService aotService(AOT_COMPILER_SERVICE_ID, false);
    int32_t ret = aotService.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
 * @tc.name: AotCompilerStubTest_002
 * @tc.desc: AotCompilerInterfaceStub::CommandStopAOTCompiler()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_002, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_STOP_AOT_COMPILER);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());
    AotCompilerService aotService(AOT_COMPILER_SERVICE_ID, false);
    int32_t ret = aotService.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
 * @tc.name: AotCompilerStubTest_003
 * @tc.desc: AotCompilerInterfaceStub::CommandGetAOTVersion()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_003, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_GET_AOT_VERSION);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());
    AotCompilerService aotService(AOT_COMPILER_SERVICE_ID, false);
    int32_t ret = aotService.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
 * @tc.name: AotCompilerStubTest_004
 * @tc.desc: AotCompilerInterfaceStub::CommandNeedReCompile()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_004, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_NEED_RE_COMPILE);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());
    AotCompilerService aotService(AOT_COMPILER_SERVICE_ID, false);
    int32_t ret = aotService.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
 * @tc.name: AotCompilerStubTest_005
 * @tc.desc: AotCompilerInterfaceStub::OnRemoteRequest() when code is invalid
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_005, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_INVALID);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());
    AotCompilerService aotService(AOT_COMPILER_SERVICE_ID, false);
    int32_t ret = aotService.OnRemoteRequest(code, data, reply, option);
    EXPECT_NE(ret, ERR_NONE);
}

/**
 * @tc.name: AotCompilerStubTest_006
 * @tc.desc: AotCompilerInterfaceStub::OnRemoteRequest() when descriptor mismatches
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_006, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_AOT_COMPILER);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(u"incorrect descriptor");

    AotCompilerService aotService(AOT_COMPILER_SERVICE_ID, false);
    int32_t ret = aotService.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(ret, ERR_TRANSACTION_FAILED);
}

/**
 * @tc.name: AotCompilerStubTest_007
 * @tc.desc: AotCompilerInterfaceStub::OnRemoteRequest() when MAP_MAX_SIZE invalid
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_007, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_AOT_COMPILER);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());
    data.WriteInt32(MAP_MAX_SIZE + 1);

    AotCompilerService aotService(AOT_COMPILER_SERVICE_ID, false);
    int32_t ret = aotService.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(ret, ERR_INVALID_DATA);
}

/**
 * @tc.name: AotCompilerStubTest_008
 * @tc.desc: AotCompilerInterfaceStub::OnRemoteRequest() when AotCompiler success
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_008, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_AOT_COMPILER);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());

    EXPECT_CALL(*aotCompilerStub_, AotCompiler(testing::_, testing::_)).
        Times(1).WillOnce(testing::Return(ERR_OK));
    int32_t result = aotCompilerStub_->OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(result, ERR_NONE);
}

/**
 * @tc.name: AotCompilerStubTest_009
 * @tc.desc: AotCompilerInterfaceStub::OnRemoteRequest() when AotCompiler fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_009, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_AOT_COMPILER);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());

    EXPECT_CALL(*aotCompilerStub_, AotCompiler(testing::_, testing::_)).
        Times(1).WillOnce(testing::Return(ERR_FAIL));
    int32_t result = aotCompilerStub_->OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(result, ERR_NONE);
}

/**
 * @tc.name: AotCompilerStubTest_010
 * @tc.desc: AotCompilerInterfaceStub::OnRemoteRequest() when GetAOTVersion success
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_010, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_GET_AOT_VERSION);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());

    EXPECT_CALL(*aotCompilerStub_, GetAOTVersion(testing::_)).
        Times(1).WillOnce(testing::Return(ERR_OK));
    int32_t result = aotCompilerStub_->OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(result, ERR_NONE);
}

/**
 * @tc.name: AotCompilerStubTest_011
 * @tc.desc: AotCompilerInterfaceStub::OnRemoteRequest() when GetAOTVersion fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_011, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_GET_AOT_VERSION);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());

    EXPECT_CALL(*aotCompilerStub_, GetAOTVersion(testing::_)).
        Times(1).WillOnce(testing::Return(ERR_FAIL));
    int32_t result = aotCompilerStub_->OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(result, ERR_NONE);
}

/**
 * @tc.name: AotCompilerStubTest_012
 * @tc.desc: AotCompilerInterfaceStub::OnRemoteRequest() when NeedReCompile success
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_012, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_NEED_RE_COMPILE);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());

    EXPECT_CALL(*aotCompilerStub_, NeedReCompile(testing::_, testing::_)).
        Times(1).WillOnce(testing::Return(ERR_OK));
    int32_t result = aotCompilerStub_->OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(result, ERR_NONE);
}

/**
 * @tc.name: AotCompilerStubTest_013
 * @tc.desc: AotCompilerInterfaceStub::OnRemoteRequest() when NeedReCompile fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerStubTest, AotCompilerStubTest_013, TestSize.Level0)
{
    uint32_t code = static_cast<uint32_t>(TEST_COMMAND_NEED_RE_COMPILE);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInterfaceToken(AotCompilerInterfaceStub::GetDescriptor());

    EXPECT_CALL(*aotCompilerStub_, NeedReCompile(testing::_, testing::_)).
        Times(1).WillOnce(testing::Return(ERR_FAIL));
    int32_t result = aotCompilerStub_->OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(result, ERR_NONE);
}
} // namespace OHOS::ArkCompiler