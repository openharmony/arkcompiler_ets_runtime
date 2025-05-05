/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "jsvaluerefobject_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/js_function.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void JSValueRefIsSymbolFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<StringRef> description = StringRef::NewFromUtf8(vm, (char*)data, size);
        Local<SymbolRef> symbol = SymbolRef::New(vm, description);
        symbol->IsSymbol(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void JSValueRefIsBigIntFuzzTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        constexpr int input = 2147483646;
        Local<IntegerRef> intValue = IntegerRef::New(vm, input);
        [[maybe_unused]]bool res = intValue->IsBigInt(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void JSValueRefIsObjectFuzzTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<JSValueRef> res = IntegerRef::New(vm, (int)size);
        [[maybe_unused]]bool result = res->IsObject(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void IsArgumentsObjectFuzzTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            if (size <= 0) {
                LOG_ECMA(ERROR) << "illegal input!";
                return;
            }
            ObjectFactory *factory = vm->GetFactory();
            JSHandle<JSArguments> obj = factory->NewJSArguments();
            JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(obj);
            JSNApiHelper::ToLocal<ObjectRef>(argumentTag)->IsArgumentsObject(vm);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void IsJSPrimitiveBooleanFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        int length = size / sizeof(char16_t);
        Local<StringRef> obj =  StringRef::NewFromUtf16(vm, (char16_t*)data, length);
        obj->IsJSPrimitiveBoolean(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void IsGeneratorFunctionFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        int length = size / sizeof(char16_t);
        Local<StringRef> obj = StringRef::NewFromUtf16(vm, (char16_t*)data, length);
        obj->IsGeneratorFunction(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void IsMapIteratorFuzzTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        JSRuntimeOptions option;
        EcmaVM *vm = JSNApi::CreateEcmaVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<JSValueRef> object = IntegerRef::New(vm, (int)size);
        object->IsMapIterator(vm);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefIsSymbolFuzzTest(data, size);
    OHOS::JSValueRefIsBigIntFuzzTest(data, size);
    OHOS::JSValueRefIsObjectFuzzTest(data, size);
    OHOS::IsArgumentsObjectFuzzTest(data, size);
    OHOS::IsJSPrimitiveBooleanFuzzTest(data, size);
    OHOS::IsGeneratorFunctionFuzzTest(data, size);
    OHOS::IsMapIteratorFuzzTest(data, size);
    return 0;
}