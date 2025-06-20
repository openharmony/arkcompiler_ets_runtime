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
#include "weaksetref_fuzzer.h"
#include "common_components/base/utf_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/linked_hash_table.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace common::utf_helper;

namespace OHOS {
void WeakSetRefGetSizeFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            return;
        }
        JSThread *thread = vm->GetJSThread();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> constructor = env->GetBuiltinsWeakSetFunction();
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor);
        JSHandle<JSWeakSet> weakSet = JSHandle<JSWeakSet>::Cast(obj);
        JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
        weakSet->SetLinkedSet(thread, hashSet);
        JSHandle<JSTaggedValue> weakSetTag = JSHandle<JSTaggedValue>::Cast(weakSet);
        Local<WeakSetRef> set = JSNApiHelper::ToLocal<WeakSetRef>(weakSetTag);
        FuzzedDataProvider fdp(data, size);
        std::string str = fdp.ConsumeRandomLengthString(1024);
        JSHandle<JSTaggedValue> value(factory->NewFromStdString(str));
        JSWeakSet::Add(thread, weakSet, value);
        set->GetSize(vm);
    }
    JSNApi::DestroyJSVM(vm);
    return;
}

void WeakSetRefGetTotalElementsFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            return;
        }
        JSThread *thread = vm->GetJSThread();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> constructor = env->GetBuiltinsWeakSetFunction();
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor);
        JSHandle<JSWeakSet> weakSet = JSHandle<JSWeakSet>::Cast(obj);
        JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
        weakSet->SetLinkedSet(thread, hashSet);
        JSHandle<JSTaggedValue> weakSetTag = JSHandle<JSTaggedValue>::Cast(weakSet);
        Local<WeakSetRef> set = JSNApiHelper::ToLocal<WeakSetRef>(weakSetTag);
        FuzzedDataProvider fdp(data, size);
        std::string str = fdp.ConsumeRandomLengthString(1024);
        JSHandle<JSTaggedValue> value(factory->NewFromStdString(str));
        JSWeakSet::Add(thread, weakSet, value);
        set->GetTotalElements(vm);
    }
    JSNApi::DestroyJSVM(vm);
    return;
}

void WeakSetRefGetValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        if (size <= 0) {
            return;
        }
        JSThread *thread = vm->GetJSThread();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> constructor = env->GetBuiltinsWeakSetFunction();
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor);
        JSHandle<JSWeakSet> weakSet = JSHandle<JSWeakSet>::Cast(obj);
        JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
        weakSet->SetLinkedSet(thread, hashSet);
        JSHandle<JSTaggedValue> weakSetTag = JSHandle<JSTaggedValue>::Cast(weakSet);
        Local<WeakSetRef> set = JSNApiHelper::ToLocal<WeakSetRef>(weakSetTag);
        FuzzedDataProvider fdp(data, size);
        std::string str = fdp.ConsumeRandomLengthString(1024);
        JSHandle<JSTaggedValue> value(factory->NewFromStdString(str));
        JSWeakSet::Add(thread, weakSet, value);
        set->GetValue(vm, 0);
    }
    JSNApi::DestroyJSVM(vm);
    return;
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::WeakSetRefGetSizeFuzzTest(data, size);
    OHOS::WeakSetRefGetTotalElementsFuzzTest(data, size);
    OHOS::WeakSetRefGetValueFuzzTest(data, size);
    return 0;
}