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

#include "aot_compiler_interface_proxy.h"
#include "ecmascript/log_wrapper.h"
#include "hitrace_meter.h"

namespace OHOS::ArkCompiler {
ErrCode AotCompilerInterfaceProxy::AotCompiler(
    const std::unordered_map<std::string, std::string>& argsMap,
    std::vector<int16_t>& sigData)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOG_SA(ERROR) << "Write interface token failed!";
        return ERR_INVALID_VALUE;
    }

    if (argsMap.size() > mapMaxSize) {
        LOG_SA(ERROR) << "The map size exceeds the security limit!";
        return ERR_INVALID_DATA;
    }

    data.WriteInt32(argsMap.size());
    for (auto it = argsMap.begin(); it != argsMap.end(); ++it) {
        if (!data.WriteString16(Str8ToStr16((it->first)))) {
            LOG_SA(ERROR) << "Write [(it->first)] failed!";
            return ERR_INVALID_DATA;
        }
        if (!data.WriteString16(Str8ToStr16((it->second)))) {
            LOG_SA(ERROR) << "Write [(it->second)] failed!";
            return ERR_INVALID_DATA;
        }
    }

    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        LOG_SA(ERROR) << "Remote is nullptr!";
        return ERR_INVALID_DATA;
    }
    int32_t result = remote->SendRequest(COMMAND_AOT_COMPILER, data, reply, option);
    if (FAILED(result)) {
        LOG_SA(ERROR) << "Send request failed!";
        return result;
    }

    ErrCode errCode = reply.ReadInt32();
    if (FAILED(errCode)) {
        LOG_SA(ERROR) << "Read Int32 failed!";
        return errCode;
    }

    int32_t sigDataSize = reply.ReadInt32();
    if (static_cast<unsigned long>(sigDataSize) > vectorMaxSize) {
        LOG_SA(ERROR) << "The vector/array size exceeds the security limit!";
        return ERR_INVALID_DATA;
    }
    for (int32_t i = 0; i < sigDataSize; ++i) {
        int16_t value = reply.ReadInt16();
        sigData.push_back(value);
    }
    return ERR_OK;
}

ErrCode AotCompilerInterfaceProxy::StopAotCompiler()
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOG_SA(ERROR) << "Write interface token failed!";
        return ERR_INVALID_VALUE;
    }

    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        LOG_SA(ERROR) << "Remote is nullptr!";
        return ERR_INVALID_DATA;
    }
    int32_t result = remote->SendRequest(COMMAND_STOP_AOT_COMPILER, data, reply, option);
    if (FAILED(result)) {
        LOG_SA(ERROR) << "Send request failed!";
        return result;
    }

    ErrCode errCode = reply.ReadInt32();
    if (FAILED(errCode)) {
        LOG_SA(ERROR) << "Read Int32 failed!";
        return errCode;
    }

    return ERR_OK;
}

ErrCode AotCompilerInterfaceProxy::GetAOTVersion(std::string& sigData)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOG_SA(ERROR) << "Write interface token failed!";
        return ERR_INVALID_VALUE;
    }

    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        LOG_SA(ERROR) << "Remote is nullptr!";
        return ERR_INVALID_DATA;
    }
    int32_t result = remote->SendRequest(COMMAND_GET_AOT_VERSION, data, reply, option);
    if (FAILED(result)) {
        LOG_SA(ERROR) << "Send request failed!";
        return result;
    }

    ErrCode errCode = reply.ReadInt32();
    if (FAILED(errCode)) {
        LOG_SA(ERROR) << "Read Int32 failed!";
        return errCode;
    }

    sigData = Str16ToStr8(reply.ReadString16());

    return ERR_OK;
}

ErrCode AotCompilerInterfaceProxy::NeedReCompile(const std::string& args, bool& sigData)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LOG_SA(ERROR) << "Write interface token failed!";
        return ERR_INVALID_VALUE;
    }

    data.WriteString16(Str8ToStr16(args));

    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        LOG_SA(ERROR) << "Remote is nullptr!";
        return ERR_INVALID_DATA;
    }
    int32_t result = remote->SendRequest(COMMAND_NEED_RE_COMPILE, data, reply, option);
    if (FAILED(result)) {
        LOG_SA(ERROR) << "Send request failed!";
        return result;
    }

    ErrCode errCode = reply.ReadInt32();
    if (FAILED(errCode)) {
        LOG_SA(ERROR) << "Read Int32 failed!";
        return errCode;
    }

    sigData = reply.ReadBool();

    return ERR_OK;
}
} // namespace OHOS::ArkCompiler
