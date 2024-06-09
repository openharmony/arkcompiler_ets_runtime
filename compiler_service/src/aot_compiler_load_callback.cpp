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

#include "aot_compiler_load_callback.h"
#include "aot_compiler_client.h"
#include "ecmascript/log_wrapper.h"
#include "system_ability_definition.h"

namespace OHOS::ArkCompiler {
void AotCompilerLoadCallback::OnLoadSystemAbilitySuccess(int32_t systemAbilityId,
                                                         const sptr<IRemoteObject> &remoteObject)
{
    if (systemAbilityId != AOT_COMPILER_SERVICE_ID) {
        LOG_SA(ERROR) << "system ability id " << systemAbilityId << " mismatch";
        return;
    }

    if (remoteObject == nullptr) {
        LOG_SA(ERROR) << "remoteObject is nullptr";
        return;
    }

    LOG_SA(DEBUG) << "load system ability " << systemAbilityId << " succeed";
    AotCompilerClient::GetInstance().OnLoadSystemAbilitySuccess(remoteObject);
}

void AotCompilerLoadCallback::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    if (systemAbilityId != AOT_COMPILER_SERVICE_ID) {
        LOG_SA(ERROR) << "system ability id " << systemAbilityId << " mismatch";
        return;
    }

    LOG_SA(DEBUG) << "load system ability " << systemAbilityId << " failed";
    AotCompilerClient::GetInstance().OnLoadSystemAbilityFail();
}
} // namespace OHOS::ArkCompiler