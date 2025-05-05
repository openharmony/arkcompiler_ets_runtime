/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "builtinsarraybufferisview_fuzzer.h"

#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_global_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

#define MAXBYTELEN sizeof(uint16_t)

namespace OHOS {
    EcmaRuntimeCallInfo *CreateEcmaRuntimeCallInfo(JSThread *thread, JSTaggedValue newTgt, uint32_t numArgs)
    {
        auto factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> hclass(thread, newTgt);
        JSHandle<JSTaggedValue> callee(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *objCallInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, callee, undefined, numArgs);
        return objCallInfo;
    }

    void BuiltinsArrayBufferIsViewFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            JSThread *thread = vm->GetJSThread();
            JSHandle<GlobalEnv> env = vm->GetGlobalEnv();

            int32_t input;
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

            JSHandle<JSFunction> arrayBuffer(thread, env->GetArrayBufferFunction().GetTaggedValue());
            JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

            auto ecmaRuntimeCallInfo = CreateEcmaRuntimeCallInfo(thread, arrayBuffer.GetTaggedValue(), 6);
            ecmaRuntimeCallInfo->SetFunction(arrayBuffer.GetTaggedValue());
            ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
            ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(input));

            builtins::BuiltinsArrayBuffer::IsView(ecmaRuntimeCallInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::BuiltinsArrayBufferIsViewFuzzTest(data, size);
    return 0;
}