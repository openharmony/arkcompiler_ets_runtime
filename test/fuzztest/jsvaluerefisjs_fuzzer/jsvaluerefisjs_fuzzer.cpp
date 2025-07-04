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
#include "jsvaluerefisjs_fuzzer.h"
#include "common_components/base/utf_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace common::utf_helper;

namespace OHOS {
void JSValueRefIsJSArrayFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            return;
        }
        FuzzedDataProvider fdp(data, size);
        const int len = fdp.ConsumeIntegralInRange<int>(0, 1024);
        JSHandle<JSTaggedValue> jsArrayTag = JSArray::ArrayCreate(vm->GetJSThread(), JSTaggedNumber(len));
        Local<JSValueRef> jsArray = JSNApiHelper::ToLocal<JSTypedArray>(jsArrayTag);
        jsArray->IsJSArray(vm);
    }
    JSNApi::DestroyJSVM(vm);
    return;
}

void JSValueRefIsJSPrimitiveNumberFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            return;
        }
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<JSTaggedValue> jstagvalue;
        FuzzedDataProvider fdp(data, size);
        auto type = fdp.PickValueInArray({
            PrimitiveType::PRIMITIVE_BOOLEAN,
            PrimitiveType::PRIMITIVE_NUMBER,
            PrimitiveType::PRIMITIVE_SYMBOL,
            PrimitiveType::PRIMITIVE_BIGINT,
        });
        JSHandle<JSPrimitiveRef> jsprimitive = factory->NewJSPrimitiveRef(type, jstagvalue);
        JSHandle<JSTaggedValue> jspri = JSHandle<JSTaggedValue>::Cast(jsprimitive);
        Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jspri);
        object->IsJSPrimitiveNumber(vm);
    }
    JSNApi::DestroyJSVM(vm);
    return;
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefIsJSArrayFuzzTest(data, size);
    OHOS::JSValueRefIsJSPrimitiveNumberFuzzTest(data, size);
    return 0;
}