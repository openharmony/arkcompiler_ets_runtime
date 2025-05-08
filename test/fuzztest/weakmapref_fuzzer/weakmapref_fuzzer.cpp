/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "ecmascript/global_env.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "weakmapref_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void WeakMapRefFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            if (data == nullptr || size <= 0) {
                LOG_ECMA(ERROR) << "illegal input!";
                return;
            }
            uint8_t* ptr = nullptr;
            ptr = const_cast<uint8_t*>(data);
            JSThread *thread = vm->GetJSThread();
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            JSHandle<JSTaggedValue> constructor = env->GetBuiltinsWeakMapFunction();
            JSHandle<JSWeakMap> weakMap =
                JSHandle<JSWeakMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor),
                                                                            constructor));
            JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(thread);
            weakMap->SetLinkedMap(thread, hashMap);
            JSHandle<JSTaggedValue> weakMapTag = JSHandle<JSTaggedValue>::Cast(weakMap);
            Local<WeakMapRef> map = JSNApiHelper::ToLocal<WeakMapRef>(weakMapTag);
            JSHandle<JSTaggedValue> value(factory->NewFromASCII("value"));
            JSHandle<JSTaggedValue> key(factory->NewFromASCII("key"));
            JSWeakMap::Set(thread, weakMap, key, value);
            map->GetSize(vm);
            map->GetTotalElements(vm);
            map->GetKey(vm, 0);
            map->GetValue(vm, 0);
        }
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::WeakMapRefFuzzTest(data, size);
    return 0;
}