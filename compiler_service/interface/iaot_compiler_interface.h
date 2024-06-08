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

#ifndef OHOS_ARKCOMPILER_IAOTCOMPILER_INTERFACE_H
#define OHOS_ARKCOMPILER_IAOTCOMPILER_INTERFACE_H

#include <vector>
#include <unordered_map>
#include <string_ex.h>
#include <cstdint>
#include <iremote_broker.h>
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

namespace OHOS::ArkCompiler {
class IAotCompilerInterface : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.ArkCompiler.IAotCompilerInterface");

    virtual ErrCode AotCompiler(
        const std::unordered_map<std::string, std::string>& argsMap,
        std::vector<int16_t>& sigData) = 0;

    virtual ErrCode StopAotCompiler() = 0;
    virtual ErrCode GetAOTVersion(std::string& sigData) = 0;
    virtual ErrCode NeedReCompile(const std::string& argsString, bool& sigData) = 0;
protected:
    const unsigned long vectorMaxSize = 102400;
    const unsigned long mapMaxSize = 102400;
};
} // namespace OHOS::ArkCompiler
#endif // OHOS_ARKCOMPILER_IAOTCOMPILER_INTERFACE_H

