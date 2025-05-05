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

#include "ecmascript/global_env.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "promiseref_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void PromiseRefCatchFuzzTest(const uint8_t* data, size_t size)
    {
        using FunctionCallback = Local<JSValueRef>(*)(JsiRuntimeCallInfo*);
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        uint8_t* ptr = nullptr;
        ptr = const_cast<uint8_t*>(data);
        Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm);
        Local<PromiseRef> promise = capability->GetPromise(vm);
        FunctionCallback RejectCallback = nullptr;
        Local<FunctionRef> reject = FunctionRef::New(vm, RejectCallback);
        promise->Catch(vm, reject);
        JSNApi::DestroyJSVM(vm);
    }

    void PromiseRefThenFinallyFuzzTest(const uint8_t* data, size_t size)
    {
        using FunctionCallback = Local<JSValueRef>(*)(JsiRuntimeCallInfo*);
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        uint8_t* ptr = nullptr;
        ptr = const_cast<uint8_t*>(data);
        Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm);
        Local<PromiseRef> promise = capability->GetPromise(vm);
        FunctionCallback RejectCallback = nullptr;
        Local<FunctionRef> reject = FunctionRef::New(vm, RejectCallback);
        promise->Finally(vm, reject);
        promise->Then(vm, reject, reject);
        JSNApi::DestroyJSVM(vm);
    }
    
    void PromiseRefThenWithTwoParameterFuzzTest(const uint8_t* data, size_t size)
    {
        using FunctionCallback = Local<JSValueRef>(*)(JsiRuntimeCallInfo*);
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        uint8_t* ptr = nullptr;
        ptr = const_cast<uint8_t*>(data);
        Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm);
        Local<PromiseRef> promise = capability->GetPromise(vm);
        FunctionCallback RejectCallback = nullptr;
        Local<FunctionRef> reject = FunctionRef::New(vm, RejectCallback);
        promise->Then(vm, reject);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::PromiseRefCatchFuzzTest(data, size);
    OHOS::PromiseRefThenFinallyFuzzTest(data, size);
    OHOS::PromiseRefThenWithTwoParameterFuzzTest(data, size);
    return 0;
}