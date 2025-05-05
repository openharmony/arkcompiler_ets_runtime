/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "builtinsarraybufferclonearraybuffer_fuzzer.h"

#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_global_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

#define MAXBYTELEN sizeof(uint32_t)

namespace OHOS {
    void BuiltinsArrayBufferCloneArrayBufferFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            JSThread *thread = vm->GetJSThread();
            JSHandle<GlobalEnv> env = vm->GetGlobalEnv();

            uint32_t input;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&input, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            const int32_t MaxMenory = 524288;
            if (input > MaxMenory) {
                input = MaxMenory;
            }

            JSHandle<JSTaggedValue> bufferConstructor = env->GetArrayBufferFunction();
            JSTaggedValue arrayBuffer = BuiltinsArrayBuffer::AllocateArrayBuffer(thread, bufferConstructor, input);
            BuiltinsArrayBuffer::CloneArrayBuffer(thread, JSHandle<JSTaggedValue>(thread, arrayBuffer),
                                                  0, bufferConstructor);
        }
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::BuiltinsArrayBufferCloneArrayBufferFuzzTest(data, size);
    return 0;
}