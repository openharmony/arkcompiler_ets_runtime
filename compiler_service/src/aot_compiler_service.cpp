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
#include "hilog/log.h"
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
    HiviewDFX::HiLog::Info(LABEL, "aot compiler service is onStart");
    if (state_ == ServiceRunningState::STATE_RUNNING) {
        HiviewDFX::HiLog::Info(LABEL, "aot compiler service has already started");
        return;
    }
    if (!Init()) {
        HiviewDFX::HiLog::Info(LABEL, "init aot compiler service failed");
        return;
    }
    bool ret = Publish(this);
    if (!ret) {
        HiviewDFX::HiLog::Error(LABEL, "publish service failed");
        return;
    }
    state_ = ServiceRunningState::STATE_RUNNING;
    RegisterPowerDisconnectedListener();
}

bool AotCompilerService::Init()
{
    auto runner = AppExecFwk::EventRunner::Create(TASK_ID);
    if (unLoadHandler_ == nullptr) {
        unLoadHandler_ = std::make_shared<AppExecFwk::EventHandler>(runner);
    }
    return true;
}

void AotCompilerService::DelayUnloadTask()
{
    auto task = []() {
        sptr<ISystemAbilityManager> samgr =
            SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgr == nullptr) {
            HiviewDFX::HiLog::Error(LABEL, "fail to get system ability manager");
            return;
        }
        int32_t ret = samgr->UnloadSystemAbility(AOT_COMPILER_SERVICE_ID);
        if (ret != ERR_OK) {
            HiviewDFX::HiLog::Error(LABEL, "remove system ability failed");
            return;
        }
    };
    unLoadHandler_->RemoveTask(TASK_ID);
    unLoadHandler_->PostTask(task, TASK_ID, DELAY_TIME);
}

void AotCompilerService::OnStop()
{
    HiviewDFX::HiLog::Info(LABEL, "aot compiler service has been onStop");
    state_ = ServiceRunningState::STATE_NOT_START;
    UnRegisterPowerDisconnectedListener();
}

int32_t AotCompilerService::AotCompiler(const std::unordered_map<std::string, std::string> &argsMap,
                                        std::vector<int16_t> &sigData)
{
    HiviewDFX::HiLog::Debug(LABEL, "begin to call aot compiler");
    unLoadHandler_->RemoveTask(TASK_ID);
    int32_t ret = AotCompilerImpl::GetInstance().EcmascriptAotCompiler(argsMap, sigData);
    HiviewDFX::HiLog::Debug(LABEL, "finish aot compiler");
    DelayUnloadTask();
    return ret;
}

int32_t AotCompilerService::GetAOTVersion(std::string& sigData)
{
    HiviewDFX::HiLog::Debug(LABEL, "begin to get AOT version");
    unLoadHandler_->RemoveTask(TASK_ID);
    int32_t ret = AotCompilerImpl::GetInstance().GetAOTVersion(sigData);
    HiviewDFX::HiLog::Debug(LABEL, "finish get AOT Version");
    DelayUnloadTask();
    return ret;
}

int32_t AotCompilerService::NeedReCompile(const std::string& args, bool& sigData)
{
    HiviewDFX::HiLog::Debug(LABEL, "begin to check need to re-compile version");
    unLoadHandler_->RemoveTask(TASK_ID);
    int32_t ret = AotCompilerImpl::GetInstance().NeedReCompile(args, sigData);
    HiviewDFX::HiLog::Debug(LABEL, "finish check need re-compile");
    DelayUnloadTask();
    return ret;
}

int32_t AotCompilerService::StopAotCompiler()
{
    HiviewDFX::HiLog::Debug(LABEL, "stop aot compiler service");
    unLoadHandler_->RemoveTask(TASK_ID);
    int32_t ret = AotCompilerImpl::GetInstance().StopAotCompiler();
    DelayUnloadTask();
    return ret;
}

void AotCompilerService::RegisterPowerDisconnectedListener()
{
    HiviewDFX::HiLog::Info(LABEL, "AotCompilerService::RegisterPowerDisconnectedListener");
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_POWER_DISCONNECTED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    powerDisconnectedListener_ = std::make_shared<PowerDisconnectedListener>(subscribeInfo);
    if (!EventFwk::CommonEventManager::SubscribeCommonEvent(powerDisconnectedListener_)) {
        HiviewDFX::HiLog::Info(LABEL, "AotCompilerService::RegisterPowerDisconnectedListener failed");
        powerDisconnectedListener_ = nullptr;
    } else {
        HiviewDFX::HiLog::Info(LABEL, "AotCompilerService::RegisterPowerDisconnectedListener success");
        isPowerEventSubscribered_ = true;
    }
}

void AotCompilerService::UnRegisterPowerDisconnectedListener()
{
    HiviewDFX::HiLog::Info(LABEL, "AotCompilerService::UnRegisterPowerDisconnectedListener");
    if (!isPowerEventSubscribered_) {
        return;
    }
    if (!EventFwk::CommonEventManager::UnSubscribeCommonEvent(powerDisconnectedListener_)) {
        HiviewDFX::HiLog::Info(LABEL, "AotCompilerService::UnRegisterPowerDisconnectedListener failed");
    }
    powerDisconnectedListener_ = nullptr;
    isPowerEventSubscribered_ = false;
    HiviewDFX::HiLog::Info(LABEL, "AotCompilerService::UnRegisterPowerDisconnectedListener done");
}
} // namespace OHOS::ArkCompiler