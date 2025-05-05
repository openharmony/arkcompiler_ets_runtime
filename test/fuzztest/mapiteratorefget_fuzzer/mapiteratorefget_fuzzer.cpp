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

#include "mapiteratorefget_fuzzer.h"
#include "ecmascript/containers/containers_list.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_global_object.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/containers/containers_hashset.h"
#include "ecmascript/common.h"
#include "ecmascript/frames.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/tagged_hash_array.h"
#include "ecmascript/containers/containers_lightweightmap.h"
#include "ecmascript/containers/containers_lightweightset.h"

using namespace panda;
using namespace panda::test;
using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace OHOS {
void MapIteratorRefGetIndexFuzzTest([[maybe_unused]] const uint8_t *data, [[maybe_unused]] size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        auto thread = vm->GetAssociatedJSThread();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> builtinsMapFunc = env->GetBuiltinsMapFunction();
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(builtinsMapFunc), builtinsMapFunc);
        JSHandle<JSMap> jsMap(obj);
        JSHandle<JSTaggedValue> linkedHashMap(LinkedHashMap::Create(thread));
        jsMap->SetLinkedMap(thread, linkedHashMap);
        JSHandle<JSTaggedValue> mapValue(jsMap);
        JSHandle<JSTaggedValue> mapIteratorVal = JSMapIterator::CreateMapIterator(thread, mapValue, IterationKind::KEY);
        JSHandle<JSMapIterator> mapIterator = JSHandle<JSMapIterator>::Cast(mapIteratorVal);
        mapIterator->SetNextIndex(1);
        Local<MapIteratorRef> object = JSNApiHelper::ToLocal<MapIteratorRef>(mapIteratorVal);
        object->GetIndex();
    }
    JSNApi::DestroyJSVM(vm);
}

void MapIteratorRefGetKindFuzzTest([[maybe_unused]] const uint8_t *data, [[maybe_unused]] size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    {
        JsiFastNativeScope scope(vm);
        auto thread = vm->GetAssociatedJSThread();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> builtinsMapFunc = env->GetBuiltinsMapFunction();
        auto obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(builtinsMapFunc), builtinsMapFunc);
        JSHandle<JSMap> jsMap(obj);
        JSHandle<JSTaggedValue> linkedHashMap(LinkedHashMap::Create(thread));
        jsMap->SetLinkedMap(thread, linkedHashMap);
        JSHandle<JSTaggedValue> mapValue(jsMap);
        JSHandle<JSTaggedValue> mapIteratorVal =
            JSMapIterator::CreateMapIterator(thread, mapValue, IterationKind::KEY);
        JSHandle<JSMapIterator> mapIterator = JSHandle<JSMapIterator>::Cast(mapIteratorVal);
        mapIterator->SetIterationKind(IterationKind::VALUE);
        mapIterator->SetIterationKind(IterationKind::KEY_AND_VALUE);
        Local<MapIteratorRef> object = JSNApiHelper::ToLocal<MapIteratorRef>(mapIteratorVal);
        object->GetKind(vm);
    }
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::MapIteratorRefGetIndexFuzzTest(data, size);
    OHOS::MapIteratorRefGetKindFuzzTest(data, size);
    return 0;
}