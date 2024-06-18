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
#include "ecmascript/log_wrapper.h"
#include "hitrace_meter.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS::ArkCompiler {
namespace {
    const int LOAD_SA_TIMEOUT_MS = 6 * 1000;
} // namespace

AotCompilerClient::AotCompilerClient()
{
    aotCompilerDiedRecipient_ = new (std::nothrow) AotCompilerDiedRecipient();
    if (aotCompilerDiedRecipient_ == nullptr) {
        LOG_SA(ERROR) << "create aot compiler died recipient failed";
    }
}

AotCompilerClient &AotCompilerClient::GetInstance()
{
    static AotCompilerClient singleAotCompilerClient;
    return singleAotCompilerClient;
}

int32_t AotCompilerClient::AotCompiler(const std::unordered_map<std::string, std::string> &argsMap,
                                       std::vector<int16_t> &sigData)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    LOG_SA(DEBUG) << "aot compiler function called";
    auto aotCompilerProxy = GetAotCompilerProxy();
    if (aotCompilerProxy == nullptr) {
        LOG_SA(ERROR) << "get aot compiler service failed";
        return ERR_AOT_COMPILER_CONNECT_FAILED;
    }
    return aotCompilerProxy->AotCompiler(argsMap, sigData);
}

int32_t AotCompilerClient::StopAotCompiler()
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    LOG_SA(DEBUG) << "aot compiler function called";
    auto aotCompilerProxy = GetAotCompilerProxy();
    if (aotCompilerProxy == nullptr) {
        LOG_SA(ERROR) << "get aot compiler service failed";
        return ERR_AOT_COMPILER_CONNECT_FAILED;
    }
    return aotCompilerProxy->StopAotCompiler();
}

int32_t AotCompilerClient::GetAOTVersion(std::string& sigData)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    LOG_SA(DEBUG) << "aot compiler get AOT version called";
    auto aotCompilerProxy = GetAotCompilerProxy();
    if (aotCompilerProxy == nullptr) {
        LOG_SA(ERROR) << "get aot compiler service failed";
        return ERR_AOT_COMPILER_CONNECT_FAILED;
    }

    return aotCompilerProxy->GetAOTVersion(sigData);
}

int32_t AotCompilerClient::NeedReCompile(const std::string& oldVersion, bool& sigData)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    LOG_SA(DEBUG) << "aot compiler check need re-compile called";
    auto aotCompilerProxy = GetAotCompilerProxy();
    if (aotCompilerProxy == nullptr) {
        LOG_SA(ERROR) << "get aot compiler service failed";
        return ERR_AOT_COMPILER_CONNECT_FAILED;
    }
    return aotCompilerProxy->NeedReCompile(oldVersion, sigData);
}

sptr<IAotCompilerInterface> AotCompilerClient::GetAotCompilerProxy()
{
    LOG_SA(DEBUG) << "get aot compiler proxy function called";
    auto aotCompilerProxy = GetAotCompiler();
    if (aotCompilerProxy != nullptr) {
        LOG_SA(DEBUG) << "aot compiler service proxy has been started";
        return aotCompilerProxy;
    }
    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        LOG_SA(ERROR) << "failed to get system ability manager";
        return nullptr;
    }
    auto remoteObject = systemAbilityMgr->CheckSystemAbility(AOT_COMPILER_SERVICE_ID);
    if (remoteObject != nullptr) {
        aotCompilerProxy = iface_cast<IAotCompilerInterface>(remoteObject);
        return aotCompilerProxy;
    }
    if (!LoadAotCompilerService()) {
        LOG_SA(ERROR) << "load aot compiler service failed";
        return nullptr;
    }
    aotCompilerProxy = GetAotCompiler();
    if (aotCompilerProxy == nullptr || aotCompilerProxy->AsObject() == nullptr) {
        LOG_SA(ERROR) << "failed to get aot compiler service";
        return nullptr;
    }
    LOG_SA(DEBUG) << "get aot compiler proxy function finished";
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
        LOG_SA(ERROR) << "failed to get system ability manager";
        return false;
    }
    sptr<AotCompilerLoadCallback> loadCallback = new (std::nothrow) AotCompilerLoadCallback();
    if (loadCallback == nullptr) {
        LOG_SA(ERROR) << "failed to create load callback";
        return false;
    }
    auto ret = systemAbilityMgr->LoadSystemAbility(AOT_COMPILER_SERVICE_ID, loadCallback);
    if (ret != 0) {
        LOG_SA(ERROR) << "load system ability " << AOT_COMPILER_SERVICE_ID << " failed with " << ret;
        return false;
    }
    {
        std::unique_lock<std::mutex> lock(loadSaMutex_);
        auto waitStatus = loadSaCondition_.wait_for(lock, std::chrono::milliseconds(LOAD_SA_TIMEOUT_MS),
        [this]() {
            return loadSaFinished_;
        });
        if (!waitStatus) {
            LOG_SA(ERROR) << "wait for load SA timeout";
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
    if (aotCompilerDiedRecipient_ == nullptr) {
        LOG_SA(ERROR) << "register aot compiler died recipient failed";
        return;
    }
    if (!remoteObject->AddDeathRecipient(aotCompilerDiedRecipient_)) {
        LOG_SA(ERROR) << "add aot compiler died recipient failed";
        return;
    }
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

void AotCompilerClient::AotCompilerDiedRecipient::OnRemoteDied(const wptr<IRemoteObject> &remoteObject)
{
    if (remoteObject == nullptr) {
        LOG_SA(ERROR) << "remote object of aot compiler died recipient is nullptr";
        return;
    }
    AotCompilerClient::GetInstance().AotCompilerOnRemoteDied(remoteObject);
}

void AotCompilerClient::AotCompilerOnRemoteDied(const wptr<IRemoteObject> &remoteObject)
{
    LOG_SA(INFO) << "remote object of aot compiler died recipient is died";
    auto aotCompilerProxy = GetAotCompiler();
    if (aotCompilerProxy == nullptr) {
        LOG_SA(ERROR) << "aot compiler proxy is nullptr";
        return;
    }
    sptr<IRemoteObject> remotePromote = remoteObject.promote();
    if (remotePromote == nullptr) {
        LOG_SA(ERROR) << "remote object of aot compiler promote fail";
        return;
    }
    if (aotCompilerProxy->AsObject() != remotePromote) {
        LOG_SA(ERROR) << "aot compiler died recipient not find remote object";
        return;
    }
    remotePromote->RemoveDeathRecipient(aotCompilerDiedRecipient_);
    SetAotCompiler(nullptr);
}
} // namespace OHOS::ArkCompiler