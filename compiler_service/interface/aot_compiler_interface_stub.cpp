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

#include "aot_compiler_interface_stub.h"
#include "hilog/log.h"
#include "hitrace_meter.h"

using OHOS::HiviewDFX::HiLog;

namespace OHOS {
namespace ArkCompiler {
int32_t AotCompilerInterfaceStub::OnRemoteRequest(
    uint32_t code,
    MessageParcel& data,
    MessageParcel& reply,
    MessageOption& option)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    std::u16string localDescriptor = GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (localDescriptor != remoteDescriptor) {
        return ERR_TRANSACTION_FAILED;
    }
    switch (code) {
        case COMMAND_AOT_COMPILER: {
            return CommandAOTCompiler(data, reply);
        }
        case COMMAND_STOP_AOT_COMPILER: {
            return CommandStopAOTCompiler(reply);
        }
        default:
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return ERR_TRANSACTION_FAILED;
}

int32_t AotCompilerInterfaceStub::CommandAOTCompiler(MessageParcel &data,
                                                     MessageParcel &reply)
{
    std::unordered_map<std::string, std::string> argsMap;
    int32_t argsMapSize = data.ReadInt32();
    for (int32_t i = 0; i < argsMapSize; ++i) {
        std::string key = Str16ToStr8(data.ReadString16());
        std::string value = Str16ToStr8(data.ReadString16());
        argsMap[key] = value;
    }
    std::vector<int16_t> sigData;
    ErrCode errCode = AotCompiler(argsMap, sigData);
    if (!reply.WriteInt32(errCode)) {
        HiLog::Error(LABEL, "Write Int32 failed!");
        return ERR_INVALID_VALUE;
    }
    if (SUCCEEDED(errCode)) {
        if (sigData.size() > vectorMaxSize) {
            HiLog::Error(LABEL, "The vector size exceeds the security limit!");
            return ERR_INVALID_DATA;
        }
        reply.WriteInt32(sigData.size());
        for (auto it = sigData.begin(); it != sigData.end(); ++it) {
            if (!reply.WriteInt32(*it)) {
                HiLog::Error(LABEL, "Write sigData array failed!");
                return ERR_INVALID_DATA;
            }
        }
    }
    return ERR_NONE;
}

int32_t AotCompilerInterfaceStub::CommandStopAOTCompiler(MessageParcel& reply)
{
    ErrCode errCode = StopAotCompiler();
    if (!reply.WriteInt32(errCode)) {
        HiLog::Error(LABEL, "Write Int32 failed!");
        return ERR_INVALID_VALUE;
    }
    return ERR_NONE;
}

} // namespace ArkCompiler
} // namespace OHOS
