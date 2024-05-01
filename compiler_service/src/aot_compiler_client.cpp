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
#include "aot_compiler_error_utils.h"
#include "aot_compiler_load_callback.h"
#include "hilog/log.h"
#include "hitrace_meter.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS::ArkCompiler {
namespace {
    const int LOAD_SA_TIMEOUT_MS = 6 * 1000;
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001800, "aot_compiler_service"};
} // namespace
    
AotCompilerClient &AotCompilerClient::GetInstance()
{
    static AotCompilerClient singleAotCompilerClient;
    return singleAotCompilerClient;
}

int32_t AotCompilerClient::AotCompiler(const std::unordered_map<std::string, std::string> &argsMap,
                                       std::vector<int16_t> &sigData)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HiviewDFX::HiLog::Debug(LABEL, "aot compiler function called");
    auto aotCompilerProxy = GetAotCompilerProxy();
    if (aotCompilerProxy == nullptr) {
        HiviewDFX::HiLog::Error(LABEL, "get aot compiler service failed");
        return ERR_AOT_COMPILER_CONNECT_FAILED;
    }
    return aotCompilerProxy->AotCompiler(argsMap, sigData);
}

int32_t AotCompilerClient::StopAotCompiler()
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HiviewDFX::HiLog::Debug(LABEL, "aot compiler function called");
    auto aotCompilerProxy = GetAotCompilerProxy();
    if (aotCompilerProxy == nullptr) {
        HiviewDFX::HiLog::Error(LABEL, "get aot compiler service failed");
        return ERR_AOT_COMPILER_CONNECT_FAILED;
    }
    return aotCompilerProxy->StopAotCompiler();
}

sptr<IAotCompilerInterface> AotCompilerClient::GetAotCompilerProxy()
{
    HiviewDFX::HiLog::Debug(LABEL, "get aot compiler proxy function called");
    auto aotCompilerProxy = GetAotCompiler();
    if (aotCompilerProxy != nullptr) {
        HiviewDFX::HiLog::Debug(LABEL, "aot compiler service proxy has been started");
        return aotCompilerProxy;
    }
    if (!LoadAotCompilerService()) {
        HiviewDFX::HiLog::Error(LABEL, "load aot compiler service failed");
        return nullptr;
    }
    aotCompilerProxy = GetAotCompiler();
    if (aotCompilerProxy == nullptr || aotCompilerProxy->AsObject() == nullptr) {
        HiviewDFX::HiLog::Error(LABEL, "failed to get aot compiler service");
        return nullptr;
    }
    HiviewDFX::HiLog::Debug(LABEL, "get aot compiler proxy function finished");
    return aotCompilerProxy;
}

bool AotCompilerClient::LoadAotCompilerService()
{
    {
        std::unique_lock<std::mutex> lock(loadSaMutex_);
        loadSaFinished_ = false;
    }
    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        HiviewDFX::HiLog::Error(LABEL, "failed to get system ability manager");
        return false;
    }
    sptr<AotCompilerLoadCallback> loadCallback = new (std::nothrow) AotCompilerLoadCallback();
    if (loadCallback == nullptr) {
        HiviewDFX::HiLog::Error(LABEL, "failed to create load callback.");
        return false;
    }
    auto ret = systemAbilityMgr->LoadSystemAbility(AOT_COMPILER_SERVICE_ID, loadCallback);
    if (ret != 0) {
        HiviewDFX::HiLog::Error(LABEL, "load system ability %{public}d failed with %{public}d",
                                AOT_COMPILER_SERVICE_ID, ret);
        return false;
    }
    {
        std::unique_lock<std::mutex> lock(loadSaMutex_);
        auto waitStatus = loadSaCondition_.wait_for(lock, std::chrono::milliseconds(LOAD_SA_TIMEOUT_MS),
        [this]() {
            return loadSaFinished_;
        });
        if (!waitStatus) {
            HiviewDFX::HiLog::Error(LABEL, "wait for load SA timeout");
            return false;
        }
    }
    return true;
}

void AotCompilerClient::SetAotCompiler(const sptr<IRemoteObject> &remoteObject)
{
    std::lock_guard<std::mutex> lock(mutex_);
    aotCompilerProxy_ = iface_cast<IAotCompilerInterface>(remoteObject);
}

sptr<IAotCompilerInterface> AotCompilerClient::GetAotCompiler()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return aotCompilerProxy_;
}

void AotCompilerClient::OnLoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject)
{
    SetAotCompiler(remoteObject);
    std::unique_lock<std::mutex> lock(loadSaMutex_);
    loadSaFinished_ = true;
    loadSaCondition_.notify_one();
}

void AotCompilerClient::OnLoadSystemAbilityFail()
{
    SetAotCompiler(nullptr);
    std::unique_lock<std::mutex> lock(loadSaMutex_);
    loadSaFinished_ = true;
    loadSaCondition_.notify_one();
}
} // namespace OHOS::ArkCompiler