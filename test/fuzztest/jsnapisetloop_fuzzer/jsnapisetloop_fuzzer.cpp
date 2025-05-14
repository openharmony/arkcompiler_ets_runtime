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

#include "ecmascript/js_runtime_options.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "jsnapisetloop_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void JSNApiSetLoopFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        void *ptr = reinterpret_cast<void*>(const_cast<uint8_t*>(data));
        JSNApi::SetLoop(vm, ptr);
        JSNApi::DestroyJSVM(vm);
    }

    void JSNApiSynchronizVMInfoFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        uint8_t *ptr = nullptr;
        ptr = const_cast<uint8_t*>(data);
        JSRuntimeOptions option1;
        EcmaVM *hostVM = JSNApi::CreateEcmaVM(option1);
        JSNApi::SynchronizVMInfo(vm, hostVM);
        JSNApi::DestroyJSVM(vm);
        JSNApi::DestroyJSVM(hostVM);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::JSNApiSetLoopFuzzTest(data, size);
    OHOS::JSNApiSynchronizVMInfoFuzzTest(data, size);
    return 0;
}