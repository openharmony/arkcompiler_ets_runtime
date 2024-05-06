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

#ifndef OHOS_ARKCOMPILER_AOTCOMPILER_SERVICE_H
#define OHOS_ARKCOMPILER_AOTCOMPILER_SERVICE_H

#include <string>
#include <unordered_map>

#include "aot_compiler_interface_stub.h"
#include "event_handler.h"
#include "event_runner.h"
#include "refbase.h"
#include "singleton.h"
#include "system_ability.h"

namespace OHOS::ArkCompiler {
enum class ServiceRunningState {
    STATE_NOT_START,
    STATE_RUNNING
};
class AotCompilerService : public SystemAbility, public AotCompilerInterfaceStub {
    DECLARE_SYSTEM_ABILITY(AotCompilerService);
    DECLARE_DELAYED_SINGLETON(AotCompilerService);
public:
    AotCompilerService(int32_t systemAbilityId, bool runOnCreate);
    int32_t AotCompiler(const std::unordered_map<std::string, std::string> &argsMap,
                        std::vector<int16_t> &sigData) override;
    int32_t StopAotCompiler() override;
    void DelayUnloadTask();
protected:
    void OnStart() override;
    void OnStop() override;
private:
    bool Init();
    std::shared_ptr<AppExecFwk::EventHandler> unLoadHandler_;
    ServiceRunningState state_;
};
} // namespace OHOS::ArkCompiler
#endif // OHOS_ARKCOMPILER_AOTCOMPILER_SERVICE_H