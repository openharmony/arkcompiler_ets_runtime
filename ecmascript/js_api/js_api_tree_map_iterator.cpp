/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/js_api/js_api_tree_map_iterator.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tagged_tree.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
JSTaggedValue JSAPITreeMapIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // Let input be the this value
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPITreeMapIterator()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                            "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPITreeMapIterator> iter(input);
    // Let it be [[IteratedMap]].
    JSHandle<JSTaggedValue> iteratedMap(thread, iter->GetIteratedMap());

    // If it is undefined, return undefinedIteratorResult.
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    if (iteratedMap->IsUndefined()) {
        return globalConst->GetUndefinedIterResult();
    }
    JSHandle<TaggedTreeMap> map(thread, JSHandle<JSAPITreeMap>::Cast(iteratedMap)->GetTreeMap());
    uint32_t elements = static_cast<uint32_t>(map->NumberOfElements());

    JSMutableHandle<TaggedArray> entries(thread, iter->GetEntries());
    if ((iter->GetEntries().IsHole()) || (elements != entries->GetLength())) {
        entries.Update(TaggedTreeMap::GetArrayFromMap(thread, map).GetTaggedValue());
        iter->SetEntries(thread, entries);
    }

    // Let index be Map.[[NextIndex]].
    uint32_t index = static_cast<uint32_t>(iter->GetNextIndex());
    if (index < elements) {
        IterationKind itemKind = IterationKind(iter->GetIterationKind());

        int keyIndex = entries->Get(index).GetInt();
        iter->SetNextIndex(index + 1);

        JSHandle<JSTaggedValue> key(thread, map->GetKey(keyIndex));
        // If itemKind is key, let result be e.[[Key]]
        if (itemKind == IterationKind::KEY) {
            return JSIterator::CreateIterResultObject(thread, key, false).GetTaggedValue();
        }
        JSHandle<JSTaggedValue> value(thread, map->GetValue(keyIndex));
        // Else if itemKind is value, let result be e.[[Value]].
        if (itemKind == IterationKind::VALUE) {
            return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
        }
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> array = factory->NewTaggedArray(2);  // 2 means the length of array
        array->Set(thread, 0, key);
        array->Set(thread, 1, value);
        JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
        return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
    }

    // Set [[IteratedMap]] to undefined.
    iter->SetIteratedMap(thread, JSTaggedValue::Undefined());
    return globalConst->GetUndefinedIterResult();
}

JSHandle<JSTaggedValue> JSAPITreeMapIterator::CreateTreeMapIterator(JSThread *thread,
                                                                    JSHandle<JSTaggedValue> &obj,
                                                                    IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSAPITreeMap()) {
        if (obj->IsJSProxy() && JSHandle<JSProxy>::Cast(obj)->GetTarget().IsJSAPITreeMap()) {
            obj = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(obj)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The Symbol.iterator method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error,
                                             JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
        }
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSAPITreeMapIterator(JSHandle<JSAPITreeMap>(obj), kind));
    return iter;
}
}  // namespace panda::ecmascript
