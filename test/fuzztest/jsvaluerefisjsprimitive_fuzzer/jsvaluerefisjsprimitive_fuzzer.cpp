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

#include "jsvaluerefisjsprimitive_fuzzer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void IsJSPrimitiveSymbolFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<JSTaggedValue> jsTagValue;
        JSHandle<JSPrimitiveRef> jsPrimitive = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_SYMBOL, jsTagValue);
        JSHandle<JSTaggedValue> jsTagPrimitive = JSHandle<JSTaggedValue>::Cast(jsPrimitive);
        Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jsTagPrimitive);
        object->IsJSPrimitiveSymbol(vm);
    }
    JSNApi::DestroyJSVM(vm);
}

void IsJSPrimitiveStringFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<JSTaggedValue> jsTagValue;
        JSHandle<JSPrimitiveRef> jsPrimitive = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_SYMBOL, jsTagValue);
        JSHandle<JSTaggedValue> jsTagPrimitive = JSHandle<JSTaggedValue>::Cast(jsPrimitive);
        Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jsTagPrimitive);
        object->IsJSPrimitiveString(vm);
    }
    JSNApi::DestroyJSVM(vm);
}

void IsJSPrimitiveIntFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<JSTaggedValue> jsTagValue;
        JSHandle<JSPrimitiveRef> jsPrimitive = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BIGINT, jsTagValue);
        JSHandle<JSTaggedValue> jsTagPrimitive = JSHandle<JSTaggedValue>::Cast(jsPrimitive);
        Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jsTagPrimitive);
        object->IsJSPrimitiveInt(vm);
    }
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::IsJSPrimitiveSymbolFuzztest(data, size);
    OHOS::IsJSPrimitiveStringFuzztest(data, size);
    OHOS::IsJSPrimitiveIntFuzztest(data, size);
    return 0;
}