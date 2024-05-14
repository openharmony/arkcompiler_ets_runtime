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
#include "hilog/log.h"
#include "system_ability_definition.h"

namespace OHOS::ArkCompiler {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001800, "aot_compiler_service"};
} // namespace

void AotCompilerLoadCallback::OnLoadSystemAbilitySuccess(int32_t systemAbilityId,
                                                         const sptr<IRemoteObject> &remoteObject)
{
    if (systemAbilityId != AOT_COMPILER_SERVICE_ID) {
        HiviewDFX::HiLog::Error(LABEL, "system ability id %{public}d mismatch", systemAbilityId);
        return;
    }

    if (remoteObject == nullptr) {
        HiviewDFX::HiLog::Error(LABEL, "remoteObject is nullptr");
        return;
    }

    HiviewDFX::HiLog::Debug(LABEL, "load system ability %{public}d succeed", systemAbilityId);
    AotCompilerClient::GetInstance().OnLoadSystemAbilitySuccess(remoteObject);
}

void AotCompilerLoadCallback::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    if (systemAbilityId != AOT_COMPILER_SERVICE_ID) {
        HiviewDFX::HiLog::Error(LABEL, "system ability id %{public}d mismatch", systemAbilityId);
        return;
    }

    HiviewDFX::HiLog::Debug(LABEL, "load system ability %{public}d failed", systemAbilityId);
    AotCompilerClient::GetInstance().OnLoadSystemAbilityFail();
}
} // namespace OHOS::ArkCompiler