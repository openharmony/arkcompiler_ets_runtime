/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jsnapiinformation_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void JSNApiCreateEcmaVMFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        return;
    }
    JSRuntimeOptions runtimeOptions;
    EcmaVM *workerVm = JSNApi::CreateEcmaVM(runtimeOptions);
    JSNApi::DestroyJSVM(workerVm);
    JSNApi::DestroyJSVM(vm);
}

void JSNApiEnableUserUncaughtErrorHandlerFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        return;
    }
    JSNApi::EnableUserUncaughtErrorHandler(vm);
    JSNApi::DestroyJSVM(vm);
}

void JSNApiFunctionFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        return;
    }
    std::function<bool(std::string dirPath, uint8_t * *buff, size_t * buffSize)> cb = [](const std::string &inputPath,
        uint8_t **buff, size_t *buffSize) -> bool {
        if (inputPath.empty() || buff == nullptr || buffSize == nullptr) {
            return false;
        }
        return false;
    };
    JSNApi::DestroyJSVM(vm);
}

void JSNApiIsMixedDebugEnabledFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        return;
    }
    JSNApi::IsMixedDebugEnabled(vm);
    JSNApi::DestroyJSVM(vm);
}

void JSNApiNotifyNativeCallingFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    JSNApi::NotifyNativeCalling(vm, (void *)data);
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSNApiCreateEcmaVMFuzzTest(data, size);
    OHOS::JSNApiEnableUserUncaughtErrorHandlerFuzzTest(data, size);
    OHOS::JSNApiFunctionFuzzTest(data, size);
    OHOS::JSNApiIsMixedDebugEnabledFuzzTest(data, size);
    OHOS::JSNApiNotifyNativeCallingFuzzTest(data, size);
    return 0;
}