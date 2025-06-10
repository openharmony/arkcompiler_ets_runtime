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

#include "setrefgetvalue_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void SetRefGetValueFuzztest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
        JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
        JSHandle<JSSet> jsSet =
            JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(vm->GetJSThread());
        jsSet->SetLinkedSet(vm->GetJSThread(), hashSet);
        JSHandle<JSTaggedValue> setTag = JSHandle<JSTaggedValue>::Cast(jsSet);
        Local<SetRef> set = JSNApiHelper::ToLocal<SetRef>(setTag);
        JSHandle<JSTaggedValue> value(factory->NewFromASCII("value"));
        JSHandle<JSTaggedValue> fristValue(factory->NewFromASCII("vlue1"));
        JSSet::Add(vm->GetJSThread(), jsSet, fristValue);
        JSHandle<JSTaggedValue> secondValue(factory->NewFromASCII("vlue2"));
        JSSet::Add(vm->GetJSThread(), jsSet, secondValue);
        int index = size % 2; // 2:Take the remainder of 2
        set->GetValue(vm, index);
    }
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::SetRefGetValueFuzztest(data, size);
    return 0;
}