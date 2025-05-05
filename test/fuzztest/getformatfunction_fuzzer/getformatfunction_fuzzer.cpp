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

#include "ecmascript/base/string_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory-inl.h"
#include "getformatfunction_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void NumberGetFormatFunctionFuzzerTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        auto thread = vm->GetAssociatedJSThread();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> ctor = env->GetNumberFormatFunction();
        JSHandle<JSNumberFormat> numberFormat =
            JSHandle<JSNumberFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
        JSHandle<JSTaggedValue> locales(factory->NewFromASCII("zh-Hans-CN"));
        JSHandle<JSTaggedValue> undefinedOptions(thread, JSTaggedValue::Undefined());
        JSNumberFormat::InitializeNumberFormat(thread, numberFormat, locales, undefinedOptions);
        JSHandle<JSTaggedValue> numberformatTagHandleVal = JSHandle<JSTaggedValue>::Cast(numberFormat);
        Local<NumberFormatRef> object = JSNApiHelper::ToLocal<NumberFormatRef>(numberformatTagHandleVal);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        object->GetFormatFunction(vm);
    }
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::NumberGetFormatFunctionFuzzerTest(data, size);
    return 0;
}