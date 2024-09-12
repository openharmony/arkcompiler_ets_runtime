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
const unsigned long VECTOR_MAX_SIZE = 102400;
}

class MockAotCompilerStub : public IRemoteStub<IAotCompilerInterface> {
public:
    MockAotCompilerStub() = default;
    virtual ~MockAotCompilerStub() = default;

    MOCK_METHOD(int32_t, CommandAOTCompiler, (MessageParcel &data, MessageParcel &reply));
    MOCK_METHOD(int32_t, CommandStopAOTCompiler, (MessageParcel &reply));
    MOCK_METHOD(int32_t, CommandGetAOTVersion, (MessageParcel& reply));
    MOCK_METHOD(int32_t, CommandNeedReCompile, (MessageParcel& data, MessageParcel& reply));
    MOCK_METHOD(ErrCode, AotCompiler, ((const std::unordered_map<std::string, std::string> &argsMap),
        std::vector<int16_t> &sigData), (override));
    MOCK_METHOD(ErrCode, StopAotCompiler, (), (override));
    MOCK_METHOD(ErrCode, GetAOTVersion, (std::string& sigData), (override));
    MOCK_METHOD(ErrCode, NeedReCompile, (const std::string& args, bool& sigData), (override));

    int32_t OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
    {
        std::u16string localDescriptor = GetDescriptor();
        std::u16string remoteDescriptor = data.ReadInterfaceToken();
        if (localDescriptor != remoteDescriptor) {
            return TEST_COMMAND_INVALID;
        }
        switch (code) {
            case TEST_COMMAND_AOT_COMPILER:
                return CommandAOTCompiler(data, reply);
            case TEST_COMMAND_STOP_AOT_COMPILER:
                return CommandStopAOTCompiler(reply);
            case TEST_COMMAND_GET_AOT_VERSION:
                return CommandGetAOTVersion(reply);
            case TEST_COMMAND_NEED_RE_COMPILE:
                return CommandNeedReCompile(data, reply);
            default:
                return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
        return TEST_COMMAND_INVALID;
    }
};

class AotCompilerProxyMock : public AotCompilerInterfaceProxy {
public:
    explicit AotCompilerProxyMock(const sptr<IRemoteObject>& remote): AotCompilerInterfaceProxy(remote) {}
    virtual ~AotCompilerProxyMock() = default;

    IRemoteObject* Remote()
    {
        return nullptr;
    }
};

class AotCompilerProxyTest : public testing::Test {
public:
    AotCompilerProxyTest() {}
    virtual ~AotCompilerProxyTest() {}

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override;
    void TearDown() override {}
    sptr<MockAotCompilerStub> mockStub_ = nullptr;
    sptr<AotCompilerInterfaceProxy> proxy_ = nullptr;
};

void AotCompilerProxyTest::SetUp()
{
    mockStub_ = new (std::nothrow) MockAotCompilerStub();
    proxy_ = new (std::nothrow) AotCompilerInterfaceProxy(mockStub_->AsObject());
}

/**
 * @tc.name: AotCompilerProxyTest_001
 * @tc.desc: AotCompilerProxy::AotCompiler(argsMap, sigData) when SendRequest fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_001, TestSize.Level0)
{
    sptr<IRemoteObject> remote = nullptr;
    sptr<AotCompilerProxyMock> mockProxy_ = new AotCompilerProxyMock(remote);
    std::unordered_map<std::string, std::string> argsMap;
    std::vector<int16_t> sigData;
    int32_t result = mockProxy_->AotCompiler(argsMap, sigData);
    EXPECT_EQ(result, ERR_INVALID_DATA);
}

/**
 * @tc.name: AotCompilerProxyTest_002
 * @tc.desc: AotCompilerProxy::StopAotCompiler() when SendRequest fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_002, TestSize.Level0)
{
    sptr<IRemoteObject> remote = nullptr;
    sptr<AotCompilerProxyMock> mockProxy_ = new AotCompilerProxyMock(remote);
    int32_t result = mockProxy_->StopAotCompiler();
    EXPECT_EQ(result, ERR_INVALID_DATA);
}

/**
 * @tc.name: AotCompilerProxyTest_003
 * @tc.desc: AotCompilerProxy::GetAOTVersion(sigData) when SendRequest fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_003, TestSize.Level0)
{
    sptr<IRemoteObject> remote = nullptr;
    sptr<AotCompilerProxyMock> mockProxy_ = new AotCompilerProxyMock(remote);
    std::string sigData = "===";
    int32_t result = mockProxy_->GetAOTVersion(sigData);
    EXPECT_EQ(result, ERR_INVALID_DATA);
}

/**
 * @tc.name: AotCompilerProxyTest_004
 * @tc.desc: AotCompilerProxy::NeedReCompile(args, sigData) when SendRequest fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_004, TestSize.Level0)
{
    sptr<IRemoteObject> remote = nullptr;
    sptr<AotCompilerProxyMock> mockProxy_ = new AotCompilerProxyMock(remote);
    std::string args = "--aot";
    bool sigData = false;
    int32_t result = mockProxy_->NeedReCompile(args, sigData);
    EXPECT_EQ(result, ERR_INVALID_DATA);
}

/**
 * @tc.name: AotCompilerProxyTest_005
 * @tc.desc: AotCompilerProxy::AotCompiler(argsMap, sigData) when connect fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_005, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    std::vector<int16_t> sigData;

    EXPECT_CALL(*mockStub_, CommandAOTCompiler(testing::_, testing::_)).
        Times(1).WillOnce(testing::Return(ERR_AOT_COMPILER_CONNECT_FAILED));
    int32_t result = proxy_->AotCompiler(argsMap, sigData);
    EXPECT_EQ(result, ERR_AOT_COMPILER_CONNECT_FAILED);
}

/**
 * @tc.name: AotCompilerProxyTest_006
 * @tc.desc: AotCompilerProxy::AotCompiler(argsMap, sigData) when compile fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_006, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    std::vector<int16_t> sigData;

    auto func = [](MessageParcel &data, MessageParcel &reply) -> int32_t {
        reply.WriteInt32(ERR_AOT_COMPILER_CALL_FAILED);
        return ERR_OK;
    };
    EXPECT_CALL(*mockStub_, CommandAOTCompiler(testing::_, testing::_)).
        Times(1).WillOnce(testing::Invoke(func));
    int32_t result = proxy_->AotCompiler(argsMap, sigData);
    EXPECT_EQ(result, ERR_AOT_COMPILER_CALL_FAILED);
}

/**
 * @tc.name: AotCompilerProxyTest_007
 * @tc.desc: AotCompilerProxy::AotCompiler(argsMap, sigData) when VECTOR_MAX_SIZE invalid
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_007, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    std::vector<int16_t> sigData;

    auto func = [](MessageParcel &data, MessageParcel &reply) -> int32_t {
        reply.WriteInt32(ERR_OK);
        reply.WriteInt32(VECTOR_MAX_SIZE + 1);
        return ERR_OK;
    };
    EXPECT_CALL(*mockStub_, CommandAOTCompiler(testing::_, testing::_)).
        Times(1).WillOnce(testing::Invoke(func));
    int32_t result = proxy_->AotCompiler(argsMap, sigData);
    EXPECT_EQ(result, ERR_INVALID_DATA);
}

/**
 * @tc.name: AotCompilerProxyTest_008
 * @tc.desc: AotCompilerProxy::AotCompiler(argsMap, sigData) success
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_008, TestSize.Level0)
{
    std::unordered_map<std::string, std::string> argsMap;
    std::vector<int16_t> sigData;

    auto func = [](MessageParcel &data, MessageParcel &reply) -> int32_t {
        reply.WriteInt32(ERR_OK);
        reply.WriteInt32(0);
        return ERR_OK;
    };
    EXPECT_CALL(*mockStub_, CommandAOTCompiler(testing::_, testing::_)).
        Times(1).WillOnce(testing::Invoke(func));
    int32_t result = proxy_->AotCompiler(argsMap, sigData);
    EXPECT_EQ(result, ERR_OK);
}

/**
 * @tc.name: AotCompilerProxyTest_009
 * @tc.desc: AotCompilerProxy::StopAotCompiler() when connect fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_009, TestSize.Level0)
{
    EXPECT_CALL(*mockStub_, CommandStopAOTCompiler(testing::_)).
        Times(1).WillOnce(testing::Return(ERR_AOT_COMPILER_CONNECT_FAILED));
    int32_t result = proxy_->StopAotCompiler();
    EXPECT_EQ(result, ERR_AOT_COMPILER_CONNECT_FAILED);
}

/**
 * @tc.name: AotCompilerProxyTest_010
 * @tc.desc: AotCompilerProxy::StopAotCompiler() when stop fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_010, TestSize.Level0)
{
    auto func = [](MessageParcel &reply) -> int32_t {
        reply.WriteInt32(ERR_AOT_COMPILER_CALL_FAILED);
        return ERR_OK;
    };
    EXPECT_CALL(*mockStub_, CommandStopAOTCompiler(testing::_)).
        Times(1).WillOnce(testing::Invoke(func));
    int32_t result = proxy_->StopAotCompiler();
    EXPECT_EQ(result, ERR_AOT_COMPILER_CALL_FAILED);
}

/**
 * @tc.name: AotCompilerProxyTest_011
 * @tc.desc: AotCompilerProxy::StopAotCompiler() when stop success
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_011, TestSize.Level0)
{
    auto func = [](MessageParcel &reply) -> int32_t {
        reply.WriteInt32(ERR_OK);
        return ERR_OK;
    };
    EXPECT_CALL(*mockStub_, CommandStopAOTCompiler(testing::_)).
        Times(1).WillOnce(testing::Invoke(func));
    int32_t result = proxy_->StopAotCompiler();
    EXPECT_EQ(result, ERR_OK);
}

/**
 * @tc.name: AotCompilerProxyTest_012
 * @tc.desc: AotCompilerProxy::GetAOTVersion(sigData) when connect fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_012, TestSize.Level0)
{
    std::string sigData;
    EXPECT_CALL(*mockStub_, CommandGetAOTVersion(testing::_)).
        Times(1).WillOnce(testing::Return(ERR_AOT_COMPILER_CONNECT_FAILED));
    int32_t result = proxy_->GetAOTVersion(sigData);
    EXPECT_EQ(result, ERR_AOT_COMPILER_CONNECT_FAILED);
}

/**
 * @tc.name: AotCompilerProxyTest_013
 * @tc.desc: AotCompilerProxy::GetAOTVersion(sigData) when get error
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_013, TestSize.Level0)
{
    std::string sigData;
    auto func = [](MessageParcel &reply) -> int32_t {
        reply.WriteInt32(ERR_AOT_COMPILER_CALL_FAILED);
        return ERR_OK;
    };
    EXPECT_CALL(*mockStub_, CommandGetAOTVersion(testing::_)).
        Times(1).WillOnce(testing::Invoke(func));
    int32_t result = proxy_->GetAOTVersion(sigData);
    EXPECT_EQ(result, ERR_AOT_COMPILER_CALL_FAILED);
}

/**
 * @tc.name: AotCompilerProxyTest_014
 * @tc.desc: AotCompilerProxy::GetAOTVersion(sigData) when get success
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_014, TestSize.Level0)
{
    std::string sigData;
    std::string testData = "test";
    auto func = [testData](MessageParcel &reply) -> int32_t {
        reply.WriteInt32(ERR_OK);
        reply.WriteString16(Str8ToStr16(testData));
        return ERR_OK;
    };
    EXPECT_CALL(*mockStub_, CommandGetAOTVersion(testing::_)).
        Times(1).WillOnce(testing::Invoke(func));
    int32_t result = proxy_->GetAOTVersion(sigData);
    EXPECT_EQ(result, ERR_OK);
    EXPECT_STREQ(sigData.c_str(), testData.c_str());
}

/**
 * @tc.name: AotCompilerProxyTest_015
 * @tc.desc: AotCompilerProxy::NeedReCompile(args, sigData) when connect fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_015, TestSize.Level0)
{
    std::string args;
    bool sigData = false;

    EXPECT_CALL(*mockStub_, CommandNeedReCompile(testing::_, testing::_)).
        Times(1).WillOnce(testing::Return(ERR_AOT_COMPILER_CONNECT_FAILED));
    int32_t result = proxy_->NeedReCompile(args, sigData);
    EXPECT_EQ(result, ERR_AOT_COMPILER_CONNECT_FAILED);
}

/**
 * @tc.name: AotCompilerProxyTest_016
 * @tc.desc: AotCompilerProxy::NeedReCompile(args, sigData) when run fail
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_016, TestSize.Level0)
{
    std::string args;
    bool sigData = false;

    auto func = [](MessageParcel &data, MessageParcel &reply) -> int32_t {
        reply.WriteInt32(ERR_AOT_COMPILER_CALL_FAILED);
        return ERR_OK;
    };
    EXPECT_CALL(*mockStub_, CommandNeedReCompile(testing::_, testing::_)).
        Times(1).WillOnce(testing::Invoke(func));
    int32_t result = proxy_->NeedReCompile(args, sigData);
    EXPECT_EQ(result, ERR_AOT_COMPILER_CALL_FAILED);
}

/**
 * @tc.name: AotCompilerProxyTest_017
 * @tc.desc: AotCompilerProxy::NeedReCompile(args, sigData) when success
 * @tc.type: Func
*/
HWTEST_F(AotCompilerProxyTest, AotCompilerProxyTest_017, TestSize.Level0)
{
    std::string args;
    bool sigData = false;
    bool testData = true;
    auto func = [testData](MessageParcel &data, MessageParcel &reply) -> int32_t {
        reply.WriteInt32(ERR_OK);
        reply.WriteBool(testData);
        return ERR_OK;
    };
    EXPECT_CALL(*mockStub_, CommandNeedReCompile(testing::_, testing::_)).
        Times(1).WillOnce(testing::Invoke(func));
    int32_t result = proxy_->NeedReCompile(args, sigData);
    EXPECT_EQ(result, ERR_OK);
    EXPECT_TRUE(sigData);
}
} // namespace OHOS::ArkCompiler