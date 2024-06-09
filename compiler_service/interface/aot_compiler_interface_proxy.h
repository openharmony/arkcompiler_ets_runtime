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

#ifndef OHOS_ARKCOMPILER_AOTCOMPILER_INTERFACE_PROXY_H
#define OHOS_ARKCOMPILER_AOTCOMPILER_INTERFACE_PROXY_H

#include <iremote_proxy.h>
#include "iaot_compiler_interface.h"

namespace OHOS::ArkCompiler {
class AotCompilerInterfaceProxy : public IRemoteProxy<IAotCompilerInterface> {
public:
    explicit AotCompilerInterfaceProxy(
        const sptr<IRemoteObject>& remote)
        : IRemoteProxy<IAotCompilerInterface>(remote)
    {}

    virtual ~AotCompilerInterfaceProxy()
    {}

    ErrCode AotCompiler(
        const std::unordered_map<std::string, std::string>& argsMap,
        std::vector<int16_t>& sigData) override;

    ErrCode StopAotCompiler() override;

    ErrCode GetAOTVersion(std::string& sigData) override;

    ErrCode NeedReCompile(const std::string& args, bool& sigData) override;

private:
    static constexpr int32_t COMMAND_AOT_COMPILER = MIN_TRANSACTION_ID + 0;
    static constexpr int32_t COMMAND_STOP_AOT_COMPILER = MIN_TRANSACTION_ID + 1;
    static constexpr int32_t COMMAND_GET_AOT_VERSION = MIN_TRANSACTION_ID + 2;
    static constexpr int32_t COMMAND_NEED_RE_COMPILE = MIN_TRANSACTION_ID + 3;

    static inline BrokerDelegator<AotCompilerInterfaceProxy> delegator_;
};
} // namespace OHOS::ArkCompiler
#endif // OHOS_ARKCOMPILER_AOTCOMPILER_INTERFACE_PROXY_H

