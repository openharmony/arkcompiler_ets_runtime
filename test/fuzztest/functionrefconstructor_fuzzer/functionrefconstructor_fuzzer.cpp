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

#include "functionrefconstructor_fuzzer.h"

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    Local<JSValueRef> FuncRefConCallbackForTest(JsiRuntimeCallInfo* info)
    {
        EscapeLocalScope scope(info->GetVM());
        return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
    }

    void FunctionRefConstructorFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        FunctionCallback nativeFunc = FuncRefConCallbackForTest;
        NativePointerCallback deleter = nullptr;
        Local<FunctionRef> func = FunctionRef::New(vm, nativeFunc, deleter, (void *)(data + size));
        Local<JSValueRef> argv[1];
        argv[0] = NumberRef::New(vm, 1.3); // 1.3 : test case of input
        func->Constructor(vm, argv, 1);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::FunctionRefConstructorFuzzTest(data, size);
    return 0;
}