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

#include "functionrefcall_fuzzer.h"

#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    Local<JSValueRef> FuncRefCallCallbackForTest(JsiRuntimeCallInfo* info)
    {
        EscapeLocalScope scope(info->GetVM());
        return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
    }

    void FunctionRefCallFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        FunctionCallback nativeFunc = FuncRefCallCallbackForTest;
        NativePointerCallback deleter = nullptr;
        Local<FunctionRef> func = FunctionRef::New(vm, nativeFunc, deleter, (void *)(data + size));
        const int32_t argvLen = 3;
        Local<JSValueRef> thisObj(JSValueRef::Undefined(vm));
        Local<JSValueRef> argv[argvLen];
        for (int32_t i = 0; i < argvLen; i++) {
            argv[i] = JSValueRef::Undefined(vm);
        }
        func->Call(vm, thisObj, argv, argvLen);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::FunctionRefCallFuzzTest(data, size);
    return 0;
}