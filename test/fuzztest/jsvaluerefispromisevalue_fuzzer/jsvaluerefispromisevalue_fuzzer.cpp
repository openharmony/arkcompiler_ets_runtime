/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <fuzzer/FuzzedDataProvider.h>
#include "jsvaluerefispromisevalue_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
Local<JSValueRef> RejectCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}

void JSValueRefIsPromiseValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm);
    Local<PromiseRef> promise = capability->GetPromise(vm);
    FunctionCallback nativeFunc = RejectCallback;
    NativePointerCallback deleter = nullptr;
    FuzzedDataProvider fdp(data, size);
    std::string str = fdp.ConsumeRandomLengthString(1024);
    void *ptr = static_cast<void *>(const_cast<char *>(str.data()));
    Local<FunctionRef> reject = FunctionRef::New(vm, nativeFunc, deleter, ptr);
    Local<PromiseRef> catchPromise = promise->Catch(vm, reject);
    promise->IsPromise(vm);
    catchPromise->IsPromise(vm);
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefIsPromiseValueFuzzTest(data, size);
    return 0;
}