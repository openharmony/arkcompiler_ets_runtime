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

#include <thread>
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_vm.h"
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
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
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
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        uint8_t *ptr = nullptr;
        ptr = const_cast<uint8_t*>(data);
        JSThread *thread = vm->GetJSThread();
        {
            LocalScope scope(vm);
            std::thread t1([vm]() {
                JSRuntimeOptions option1;
                EcmaVM *hostVM = JSNApi::CreateEcmaVM(option1);
                {
                    LocalScope scope2(hostVM);
                    JSNApi::SynchronizVMInfo(vm, hostVM);
                }
                JSNApi::DestroyJSVM(hostVM);
            });
            {
                ecmascript::ThreadSuspensionScope suspensionScope(thread);
                t1.join();
            }
        }
        JSNApi::DestroyJSVM(vm);
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