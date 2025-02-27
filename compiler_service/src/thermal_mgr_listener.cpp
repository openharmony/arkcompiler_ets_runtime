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

#include "thermal_mgr_listener.h"

#include "aot_compiler_impl.h"
#include "ecmascript/log_wrapper.h"

namespace OHOS::ArkCompiler {
ThermalMgrListener::ThermalMgrListener(const EventFwk::CommonEventSubscribeInfo &subscribeInfo)
    : EventFwk::CommonEventSubscriber(subscribeInfo)
{
}

void ThermalMgrListener::OnReceiveEvent(const EventFwk::CommonEventData &data)
{
    LOG_SA(INFO) << "ThermalMgrListener::OnReceiveEvent";
    std::string key = "0";
    int32_t invalidLevel = -1;
    int32_t level = data.GetWant().GetIntParam(key, invalidLevel);
    AotCompilerImpl::GetInstance().HandleThermalLevelChanged(level);
}
}  // namespace OHOS::ArkCompiler