/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/shared_objects/js_shared_map_iterator.h"

#include "ecmascript/js_array.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/shared_objects/concurrent_api_scope.h"
#include "ecmascript/shared_objects/js_shared_map.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
JSTaggedValue JSSharedMapIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj(BuiltinsBase::GetThis(argv));
    return NextInternal(thread, thisObj);
}

JSTaggedValue JSSharedMapIterator::NextInternal(JSThread *thread, JSHandle<JSTaggedValue> thisObj)
{
    if (!thisObj->IsJSSharedMapIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not a map iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSSharedMapIterator> iter(thisObj);
    JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
    if (iter->GetIteratedMap().IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    JSHandle<JSSharedMap> iteratedMap(thread, iter->GetIteratedMap());
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap> scope(thread, JSHandle<JSTaggedValue>::Cast(iteratedMap));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    JSHandle<LinkedHashMap> map(thread, iteratedMap->GetLinkedMap());

    int index = static_cast<int>(iter->GetNextIndex());
    IterationKind itemKind = iter->GetIterationKind();
    int totalElements = map->NumberOfElements() + map->NumberOfDeletedElements();

    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    while (index < totalElements) {
        JSTaggedValue key = map->GetKey(index);
        if (!key.IsHole()) {
            iter->SetNextIndex(index + 1);
            keyHandle.Update(key);
            if (itemKind == IterationKind::KEY) {
                return JSIterator::CreateIterResultObject(thread, keyHandle, false).GetTaggedValue();
            }
            JSHandle<JSTaggedValue> value(thread, map->GetValue(index));
            if (itemKind == IterationKind::VALUE) {
                return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
            }
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<TaggedArray> array(factory->NewTaggedArray(2));  // 2 means the length of array
            array->Set(thread, 0, keyHandle);
            array->Set(thread, 1, value);
            JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
            return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
        }
        index++;
    }
    iter->SetIteratedMap(thread, JSTaggedValue::Undefined());
    return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
}

JSHandle<JSTaggedValue> JSSharedMapIterator::CreateMapIterator(JSThread *thread,
    const JSHandle<JSTaggedValue> &obj, IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSSharedMap()) {
        JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not SharedMap", undefinedHandle);
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSMapIterator(JSHandle<JSSharedMap>(obj), kind));
    return iter;
}
}  // namespace panda::ecmascript
