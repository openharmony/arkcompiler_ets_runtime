/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "jsnapisethostpromiserejectiontracker_fuzzer.h"

#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void JSNApiSetHostPromiseRejectionTrackerFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        JSNApi::SetHostPromiseRejectionTracker(vm, (void *)(data + size), (void *)data);
        JSNApi::DestroyJSVM(vm);
    }

    void JSNApiSetNativePtrGetterFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        JSNApi::SetNativePtrGetter(vm, (void*)data);
        JSNApi::DestroyJSVM(vm);
    }

    void JSNApiSetHostResolveBufferTrackerFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        uint8_t* ptr = nullptr;
        ptr = const_cast<uint8_t*>(data);
        JSNApi::SetHostResolveBufferTracker(vm,
            [&](std::string, uint8_t **, size_t *, std::string &) -> bool { return true; });
        JSNApi::DestroyJSVM(vm);
    }

    void JSNApiSetUnloalNativeModuleCallbackFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        uint8_t* ptr = nullptr;
        ptr = const_cast<uint8_t*>(data);
        JSNApi::SetUnloadNativeModuleCallback(vm, [&](const std::string &) -> bool { return true; });
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::JSNApiSetHostPromiseRejectionTrackerFuzzTest(data, size);
    OHOS::JSNApiSetNativePtrGetterFuzzTest(data, size);
    OHOS::JSNApiSetHostResolveBufferTrackerFuzzTest(data, size);
    OHOS::JSNApiSetUnloalNativeModuleCallbackFuzzTest(data, size);
    return 0;
}