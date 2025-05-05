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

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "publicapicollatorrefget_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void GetCompareFunctionFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
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
        JSThread *thread = vm->GetJSThread();
        JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<JSTaggedValue> ctor = env->GetCollatorFunction();
        JSHandle<JSCollator> collator =
            JSHandle<JSCollator>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
        JSHandle<JSTaggedValue> localeStr = thread->GlobalConstants()->GetHandledEnUsString();
        JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
        JSHandle<JSCollator> initCollator =
            JSCollator::InitializeCollator(thread, collator, localeStr, undefinedHandle);
        JSHandle<JSTaggedValue> collatorTagHandleVal = JSHandle<JSTaggedValue>::Cast(initCollator);
        Local<CollatorRef> object = JSNApiHelper::ToLocal<CollatorRef>(collatorTagHandleVal);
        object->GetCompareFunction(vm);
    }
    JSNApi::DestroyJSVM(vm);
    return;
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::GetCompareFunctionFuzzTest(data, size);
    return 0;
}