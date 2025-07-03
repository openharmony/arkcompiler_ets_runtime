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
#include "jsvaluerefiscontainer_fuzzer.h"
#include "common_components/base/utf_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/tagged_tree.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace common::utf_helper;

namespace OHOS {
void JSValueRefIsVectorFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    const int arkProp = fdp.ConsumeIntegral<int>();
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        JSThread *thread = vm->GetJSThread();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
        JSHandle<JSHClass> vectorClass = factory->NewEcmaHClass(JSAPIVector::SIZE, JSType::JS_API_VECTOR, proto);
        JSHandle<JSAPIVector> jsVector = JSHandle<JSAPIVector>::Cast(factory->NewJSObjectWithInit(vectorClass));
        jsVector->SetLength(0);
        JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(jsVector);
        Local<JSValueRef> isVector = JSNApiHelper::ToLocal<JSAPIVector>(argumentTag);
        isVector->IsVector(vm);
        option.SetArkProperties(arkProp);
    }
    JSNApi::DestroyJSVM(vm);
    return;
}

void JSValueRefIsMapFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    const int arkProp = fdp.ConsumeIntegral<int>();
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    option.SetArkProperties(arkProp);
    Local<MapRef> map = MapRef::New(vm);
    map->IsMap(vm);
    JSNApi::DestroyJSVM(vm);
    return;
}

void JSValueRefIsRegExpFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    const int arkProp = fdp.ConsumeIntegral<int>();
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
        JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
        JSHandle<JSHClass> jSRegExpClass = factory->NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
        JSHandle<JSRegExp> jSRegExp = JSHandle<JSRegExp>::Cast(factory->NewJSObject(jSRegExpClass));
        jSRegExp->SetByteCodeBuffer(thread, JSTaggedValue::Undefined());
        jSRegExp->SetOriginalSource(thread, JSTaggedValue::Undefined());
        jSRegExp->SetGroupName(thread, JSTaggedValue::Undefined());
        jSRegExp->SetOriginalFlags(thread, JSTaggedValue(0));
        jSRegExp->SetLength(0);
        option.SetArkProperties(arkProp);
        JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(jSRegExp);
        Local<JSValueRef> regexp = JSNApiHelper::ToLocal<JSRegExp>(argumentTag);
        regexp->IsRegExp(vm);
    }
    JSNApi::DestroyJSVM(vm);
    return;
}

void JSValueRefIsSetFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    const int arkProp = fdp.ConsumeIntegral<int>();
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        JSThread *thread = vm->GetJSThread();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
        JSHandle<JSSet> set =
            JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
        set->SetLinkedSet(thread, hashSet);
        JSHandle<JSTaggedValue> setTag = JSHandle<JSTaggedValue>::Cast(set);
        Local<SetRef> isSet = JSNApiHelper::ToLocal<SetRef>(setTag);
        isSet->IsSet(vm);
        option.SetArkProperties(arkProp);
    }
    JSNApi::DestroyJSVM(vm);
    return;
}

void JSValueRefIsTreeMapFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    const int arkProp = fdp.ConsumeIntegral<int>();
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        JSThread *thread = vm->GetJSThread();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
        JSHandle<JSHClass> mapClass = factory->NewEcmaHClass(JSAPITreeMap::SIZE, JSType::JS_API_TREE_MAP, proto);
        JSHandle<JSAPITreeMap> jsTreeMap = JSHandle<JSAPITreeMap>::Cast(factory->NewJSObjectWithInit(mapClass));
        JSHandle<TaggedTreeMap> treeMap(thread, TaggedTreeMap::Create(thread));
        jsTreeMap->SetTreeMap(thread, treeMap);
        JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(jsTreeMap);
        Local<JSValueRef> isTreeMap = JSNApiHelper::ToLocal<JSAPITreeMap>(argumentTag);
        isTreeMap->IsTreeMap(vm);
        option.SetArkProperties(arkProp);
    }
    JSNApi::DestroyJSVM(vm);
}

void JSValueRefIsTreeSetFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    const int arkProp = fdp.ConsumeIntegral<int>();
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        JSThread *thread = vm->GetJSThread();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
        JSHandle<JSHClass> setClass = factory->NewEcmaHClass(JSAPITreeSet::SIZE, JSType::JS_API_TREE_SET, proto);
        JSHandle<JSAPITreeSet> jsTreeSet = JSHandle<JSAPITreeSet>::Cast(factory->NewJSObjectWithInit(setClass));
        JSHandle<TaggedTreeSet> treeSet(thread, TaggedTreeSet::Create(thread));
        jsTreeSet->SetTreeSet(thread, treeSet);
        JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(jsTreeSet);
        Local<JSValueRef> isTreeSet = JSNApiHelper::ToLocal<JSAPITreeSet>(argumentTag);
        isTreeSet->IsTreeSet(vm);
        option.SetArkProperties(arkProp);
    }
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefIsVectorFuzzTest(data, size);
    OHOS::JSValueRefIsMapFuzzTest(data, size);
    OHOS::JSValueRefIsRegExpFuzzTest(data, size);
    OHOS::JSValueRefIsSetFuzzTest(data, size);
    OHOS::JSValueRefIsTreeMapFuzzTest(data, size);
    OHOS::JSValueRefIsTreeSetFuzzTest(data, size);
    return 0;
}