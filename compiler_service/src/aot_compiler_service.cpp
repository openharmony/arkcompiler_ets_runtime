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

#include "aot_compiler_service.h"
#include "aot_compiler_error_utils.h"
#include "aot_compiler_impl.h"
#include "ecmascript/log_wrapper.h"
#include "iservice_registry.h"
#include "ipc_skeleton.h"
#include "iremote_object.h"
#include "system_ability_definition.h"

#include "power_disconnected_listener.h"

#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "common_event_subscriber.h"

namespace OHOS::ArkCompiler {
namespace {
const std::string TASK_ID = "UnLoadSA";
constexpr int32_t DELAY_TIME = 180000;
}

REGISTER_SYSTEM_ABILITY_BY_ID(AotCompilerService, AOT_COMPILER_SERVICE_ID, false);

AotCompilerService::AotCompilerService()
    : SystemAbility(AOT_COMPILER_SERVICE_ID, false), state_(ServiceRunningState::STATE_NOT_START)
{
}

AotCompilerService::AotCompilerService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate), state_(ServiceRunningState::STATE_NOT_START)
{
}

AotCompilerService::~AotCompilerService()
{
}

void AotCompilerService::OnStart()
{
    LOG_SA(INFO) << "aot compiler service is onStart";
    if (state_ == ServiceRunningState::STATE_RUNNING) {
        LOG_SA(INFO) << "aot compiler service has already started";
        return;
    }
    if (!Init()) {
        LOG_SA(INFO) << "init aot compiler service failed";
        return;
    }
    bool ret = Publish(this);
    if (!ret) {
        LOG_SA(ERROR) << "publish service failed";
        return;
    }
    state_ = ServiceRunningState::STATE_RUNNING;
    RegisterPowerDisconnectedListener();
    RegisterScreenStatusSubscriber();
    RegisterThermalMgrListener();
}

bool AotCompilerService::Init()
{
    auto runner = AppExecFwk::EventRunner::Create(TASK_ID);
    if (unLoadHandler_ == nullptr) {
        unLoadHandler_ = std::make_shared<AppExecFwk::EventHandler>(runner);
    }
    return true;
}

void AotCompilerService::RemoveUnloadTask(const std::string &taskId)
{
    if (unLoadHandler_ == nullptr) {
        LOG_SA(ERROR) << "NULL pointer of unLoadHandler_ error";
        return;
    }
    unLoadHandler_->RemoveTask(taskId);
}

void AotCompilerService::DelayUnloadTask(const std::string &taskId, const int32_t delayTime)
{
    if (unLoadHandler_ == nullptr) {
        LOG_SA(ERROR) << "NULL pointer of unLoadHandler_ error";
        return;
    }
    auto task = []() {
        sptr<ISystemAbilityManager> samgr =
            SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr == nullptr) {
            LOG_SA(ERROR) << "fail to get system ability manager";
            return;
        }
        int32_t ret = samgr->UnloadSystemAbility(AOT_COMPILER_SERVICE_ID);
        if (ret != ERR_OK) {
            LOG_SA(ERROR) << "remove system ability failed";
            return;
        }
    };
    unLoadHandler_->PostTask(task, taskId, delayTime);
}

void AotCompilerService::OnStop()
{
    LOG_SA(INFO) << "aot compiler service has been onStop";
    state_ = ServiceRunningState::STATE_NOT_START;
    UnRegisterPowerDisconnectedListener();
    UnRegisterScreenStatusSubscriber();
    UnRegisterThermalMgrListener();
}

int32_t AotCompilerService::AotCompiler(const std::unordered_map<std::string, std::string> &argsMap,
                                        std::vector<int16_t> &sigData)
{
    LOG_SA(DEBUG) << "begin to call aot compiler";
    RemoveUnloadTask(TASK_ID);
    int32_t ret = ERR_FAIL;
    {
        std::lock_guard<std::mutex> lock(aotCompilerMutex_);
        ret = AotCompilerImpl::GetInstance().EcmascriptAotCompiler(argsMap, sigData);
    }
    LOG_SA(DEBUG) << "finish aot compiler";
    DelayUnloadTask(TASK_ID, DELAY_TIME);
    return ret;
}

int32_t AotCompilerService::GetAOTVersion(std::string& sigData)
{
    LOG_SA(DEBUG) << "begin to get AOT version";
    RemoveUnloadTask(TASK_ID);
    int32_t ret = AotCompilerImpl::GetInstance().GetAOTVersion(sigData);
    LOG_SA(DEBUG) << "finish get AOT Version";
    DelayUnloadTask(TASK_ID, DELAY_TIME);
    return ret;
}

int32_t AotCompilerService::NeedReCompile(const std::string& args, bool& sigData)
{
    LOG_SA(DEBUG) << "begin to check need to re-compile version";
    RemoveUnloadTask(TASK_ID);
    int32_t ret = AotCompilerImpl::GetInstance().NeedReCompile(args, sigData);
    LOG_SA(DEBUG) << "finish check need re-compile";
    DelayUnloadTask(TASK_ID, DELAY_TIME);
    return ret;
}

int32_t AotCompilerService::StopAotCompiler()
{
    LOG_SA(DEBUG) << "stop aot compiler service";
    RemoveUnloadTask(TASK_ID);
    int32_t ret = AotCompilerImpl::GetInstance().StopAotCompiler();
    DelayUnloadTask(TASK_ID, DELAY_TIME);
    return ret;
}

void AotCompilerService::RegisterPowerDisconnectedListener()
{
    LOG_SA(DEBUG) << "AotCompilerService::RegisterPowerDisconnectedListener";
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_POWER_DISCONNECTED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    powerDisconnectedListener_ = std::make_shared<PowerDisconnectedListener>(subscribeInfo);
    if (!EventFwk::CommonEventManager::SubscribeCommonEvent(powerDisconnectedListener_)) {
        LOG_SA(INFO) << "AotCompilerService::RegisterPowerDisconnectedListener failed";
        powerDisconnectedListener_ = nullptr;
    } else {
        LOG_SA(INFO) << "AotCompilerService::RegisterPowerDisconnectedListener success";
        isPowerEventSubscribered_ = true;
    }
}

void AotCompilerService::RegisterScreenStatusSubscriber()
{
    LOG_SA(DEBUG) << "AotCompilerService::RegisterScreenStatusSubscriber";
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    screenStatusSubscriber_ = std::make_shared<ScreenStatusSubscriber>(subscribeInfo);
    if (!EventFwk::CommonEventManager::SubscribeCommonEvent(screenStatusSubscriber_)) {
        LOG_SA(WARN) << "AotCompilerService::RegisterScreenStatusSubscriber failed";
        screenStatusSubscriber_ = nullptr;
    } else {
        LOG_SA(INFO) << "AotCompilerService::RegisterScreenStatusSubscriber success";
        isScreenStatusSubscribered_ = true;
    }
}

void AotCompilerService::RegisterThermalMgrListener()
{
    LOG_SA(DEBUG) << "AotCompilerService::RegisterThermalMgrListener";
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_THERMAL_LEVEL_CHANGED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    thermalMgrListener_ = std::make_shared<ThermalMgrListener>(subscribeInfo);
    if (!EventFwk::CommonEventManager::SubscribeCommonEvent(thermalMgrListener_)) {
        LOG_SA(WARN) << "AotCompilerService::RegisterThermalMgrListener failed";
        thermalMgrListener_ = nullptr;
    } else {
        LOG_SA(INFO) << "AotCompilerService::RegisterThermalMgrListener success";
        isThermalLevelEventSubscribered_ = true;
    }
}

void AotCompilerService::UnRegisterPowerDisconnectedListener()
{
    LOG_SA(DEBUG) << "AotCompilerService::UnRegisterPowerDisconnectedListener";
    if (!isPowerEventSubscribered_) {
        return;
    }
    if (!EventFwk::CommonEventManager::UnSubscribeCommonEvent(powerDisconnectedListener_)) {
        LOG_SA(INFO) << "AotCompilerService::UnRegisterPowerDisconnectedListener failed";
    }
    powerDisconnectedListener_ = nullptr;
    isPowerEventSubscribered_ = false;
    LOG_SA(INFO) << "AotCompilerService::UnRegisterPowerDisconnectedListener done";
}

void AotCompilerService::UnRegisterScreenStatusSubscriber()
{
    LOG_SA(DEBUG) << "AotCompilerService::UnRegisterScreenStatusSubscriber";
    if (!isScreenStatusSubscribered_) {
        return;
    }
    if (!EventFwk::CommonEventManager::UnSubscribeCommonEvent(screenStatusSubscriber_)) {
        LOG_SA(WARN) << "AotCompilerService::UnRegisterScreenStatusSubscriber failed";
    }
    screenStatusSubscriber_ = nullptr;
    isScreenStatusSubscribered_ = false;
    LOG_SA(INFO) << "AotCompilerService::UnRegisterScreenStatusSubscriber done";
}

void AotCompilerService::UnRegisterThermalMgrListener()
{
    LOG_SA(DEBUG) << "AotCompilerService::UnRegisterThermalMgrListener";
    if (!isThermalLevelEventSubscribered_) {
        return;
    }
    if (!EventFwk::CommonEventManager::UnSubscribeCommonEvent(thermalMgrListener_)) {
        LOG_SA(WARN) << "AotCompilerService::UnRegisterThermalMgrListener failed";
    }
    thermalMgrListener_ = nullptr;
    isThermalLevelEventSubscribered_ = false;
    LOG_SA(INFO) << "AotCompilerService::UnRegisterThermalMgrListener done";
}
} // namespace OHOS::ArkCompiler