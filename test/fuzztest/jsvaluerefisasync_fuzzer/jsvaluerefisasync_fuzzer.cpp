/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include <fuzzer/FuzzedDataProvider.h>
#include "ecmascript/base/string_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "jsvaluerefisasync_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void JSValueRefIsAsyncGeneratorFunctionTrueFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        FuzzedDataProvider fdp(data, size);
        const int arkProp = fdp.ConsumeIntegral<int>();
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            option.SetArkProperties(arkProp);
            ObjectFactory *factory = vm->GetFactory();
            MethodLiteral *methodLiteral = nullptr;
            JSHandle<Method> method = factory->NewSMethod(methodLiteral);
            JSHandle<JSFunction> asyncGeneratorFunction = factory->NewJSAsyncGeneratorFunction(method);
            JSHandle<JSTaggedValue> asyncgefu = JSHandle<JSTaggedValue>::Cast(asyncGeneratorFunction);
            Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(asyncgefu);
            [[maybe_unused]] auto it = object->IsAsyncGeneratorFunction(vm);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void JSValueRefIsAsyncGeneratorFunctionFalseFuzzTest(const uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        const int arkProp = fdp.ConsumeIntegral<int>();
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        Local<JSValueRef> object = IntegerRef::New(vm, arkProp);
        object->IsAsyncGeneratorFunction(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void JSValueRefIsAsyncGeneratorObjectTrueFuzzTest(const uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        const int arkProp = fdp.ConsumeIntegral<int>();
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            option.SetArkProperties(arkProp);
            ObjectFactory *factory = vm->GetFactory();
            MethodLiteral *methodLiteral = nullptr;
            JSHandle<Method> method = factory->NewSMethod(methodLiteral);
            JSHandle<JSFunction> asyncGeneratorFunction = factory->NewJSAsyncGeneratorFunction(method);
            JSHandle<JSTaggedValue> asyncgefu = JSHandle<JSTaggedValue>::Cast(asyncGeneratorFunction);
            Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(asyncgefu);
            [[maybe_unused]] auto it = object->IsAsyncGeneratorObject(vm);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void JSValueRefIsAsyncGeneratorObjectFalseFuzzTest(const uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        const int arkProp = fdp.ConsumeIntegral<int>();
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        Local<JSValueRef> object = IntegerRef::New(vm, arkProp);
        object->IsAsyncGeneratorObject(vm);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefIsAsyncGeneratorFunctionTrueFuzzTest(data, size);
    OHOS::JSValueRefIsAsyncGeneratorFunctionFalseFuzzTest(data, size);
    OHOS::JSValueRefIsAsyncGeneratorObjectTrueFuzzTest(data, size);
    OHOS::JSValueRefIsAsyncGeneratorObjectFalseFuzzTest(data, size);
    return 0;
}