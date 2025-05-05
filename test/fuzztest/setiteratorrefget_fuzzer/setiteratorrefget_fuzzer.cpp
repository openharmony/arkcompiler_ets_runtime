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

#include "setiteratorrefget_fuzzer.h"

#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
JSSet *CreateJSSet(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
    set->SetLinkedSet(thread, hashSet);
    return JSSet::Cast(set.GetTaggedValue().GetTaggedObject());
}

void SetIteratorRefGetFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
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
        auto thread = vm->GetAssociatedJSThread();
        JSHandle<JSSet> jsSet(thread, CreateJSSet(thread));
        JSHandle<JSTaggedValue> jsTagSetIterator =
            JSSetIterator::CreateSetIterator(thread, JSHandle<JSTaggedValue>(jsSet), IterationKind::KEY);
        JSHandle<JSSetIterator> jsSetIterator1(jsTagSetIterator);
        JSTaggedValue::SameValue(jsSetIterator1->GetIteratedSet(), jsSet->GetLinkedSet());
        Local<SetIteratorRef> setIterator = JSNApiHelper::ToLocal<SetIteratorRef>(jsTagSetIterator);
        Local<JSValueRef> setIterator1 = setIterator;
        setIterator1->IsSetIterator(vm);
        setIterator->GetIndex();
        setIterator->GetKind(vm);
    }
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::SetIteratorRefGetFuzzTest(data, size);
    return 0;
}