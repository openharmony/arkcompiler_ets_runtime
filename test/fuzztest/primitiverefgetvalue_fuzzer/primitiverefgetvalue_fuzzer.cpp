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

#include "primitiverefgetvalue_fuzzer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/object_factory.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void PrimitiveRefGetValueFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            return;
        }
        auto thread = vm->GetAssociatedJSThread();
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
        JSHandle<JSHClass> jsClassHandle =
            factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_PRIMITIVE_REF, nullHandle);
        TaggedObject *taggedObject = factory->NewObject(jsClassHandle);
        JSHandle<JSTaggedValue> jsTaggedValue(thread, JSTaggedValue(taggedObject));
        Local<PrimitiveRef> jsValueRef = JSNApiHelper::ToLocal<JSPrimitiveRef>(jsTaggedValue);
        jsValueRef->GetValue(vm);
    }
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::PrimitiveRefGetValueFuzzTest(data, size);
    return 0;
}