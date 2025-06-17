/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/js_map_iterator.h"

#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/element_accessor-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/linked_hash_table.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
JSTaggedValue JSMapIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1.Let O be the this value
    JSHandle<JSTaggedValue> thisObj(BuiltinsBase::GetThis(argv));
    return NextInternal(thread, thisObj);
}

JSTaggedValue JSMapIterator::NextInternal(JSThread *thread, JSHandle<JSTaggedValue> thisObj)
{
    // 3.If O does not have all of the internal slots of a Map Iterator Instance (23.1.5.3), throw a TypeError
    // exception.
    if (!thisObj->IsJSMapIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not a map iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSMapIterator> iter(thisObj);
    iter->Update(thread);
    JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
    // 4.Let m be O.[[IteratedMap]].
    JSHandle<JSTaggedValue> iteratedMap(thread, iter->GetIteratedMap(thread));

    // 5.Let index be O.[[MapNextIndex]].
    int index = static_cast<int>(iter->GetNextIndex());
    IterationKind itemKind = iter->GetIterationKind();
    // 7.If m is undefined, return CreateIterResultObject(undefined, true).
    if (iteratedMap->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    };
    JSHandle<LinkedHashMap> map(iteratedMap);
    int totalElements = map->NumberOfElements() + map->NumberOfDeletedElements();

    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    while (index < totalElements) {
        JSTaggedValue key = map->GetKey(thread, index);
        if (!key.IsHole()) {
            iter->SetNextIndex(index + 1);
            keyHandle.Update(key);
            // If itemKind is key, let result be e.[[Key]]
            if (itemKind == IterationKind::KEY) {
                return JSIterator::CreateIterResultObject(thread, keyHandle, false).GetTaggedValue();
            }
            JSHandle<JSTaggedValue> value(thread, map->GetValue(thread, index));
            // Else if itemKind is value, let result be e.[[Value]].
            if (itemKind == IterationKind::VALUE) {
                return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
            }
            // Else
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<TaggedArray> array(factory->NewTaggedArray(2));  // 2 means the length of array
            array->Set(thread, 0, keyHandle);
            array->Set(thread, 1, value);
            JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
            return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
        }
        index++;
    }
    // 13.Set O.[[IteratedMap]] to undefined.
    iter->SetIteratedMap(thread, JSTaggedValue::Undefined());
    return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
}

void JSMapIterator::Update(const JSThread *thread)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue iteratedMap = GetIteratedMap(thread);
    if (iteratedMap.IsUndefined()) {
        return;
    }
    LinkedHashMap *map = LinkedHashMap::Cast(iteratedMap.GetTaggedObject());
    if (map->GetNextTable(thread).IsHole()) {
        return;
    }
    int index = static_cast<int>(GetNextIndex());
    JSTaggedValue nextTable = map->GetNextTable(thread);
    while (!nextTable.IsHole()) {
        index -= map->GetDeletedElementsAt(thread, index);
        map = LinkedHashMap::Cast(nextTable.GetTaggedObject());
        nextTable = map->GetNextTable(thread);
    }
    SetIteratedMap(thread, JSTaggedValue(map));
    SetNextIndex(index);
}

JSHandle<JSTaggedValue> JSMapIterator::CreateMapIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                         IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSMap()) {
        JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSMap", undefinedHandle);
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSMapIterator(JSHandle<JSMap>(obj), kind));
    return iter;
}

JSTaggedValue JSMapIterator::MapIteratorToList(JSThread *thread, JSHandle<JSTaggedValue> iterator)
{
    JSTaggedValue newArray = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);
    JSHandle<JSMapIterator> iter(iterator);
    JSHandle<JSTaggedValue> iteratedMap(thread, iter->GetIteratedMap(thread));
    if (iteratedMap->IsUndefined()) {
        return newArrayHandle.GetTaggedValue();
    }
    IterationKind itemKind = iter->GetIterationKind();
    JSHandle<LinkedHashMap> map(iteratedMap);
    int totalElements = map->NumberOfElements() + map->NumberOfDeletedElements();
    int index = static_cast<int>(iter->GetNextIndex());
    int k = 0;

    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> oldElements(thread, newArrayHandle->GetElements(thread));
    JSHandle<TaggedArray> elements = factory->ExtendArray(oldElements, totalElements);
    newArrayHandle->SetElements(thread, elements);
    while (index < totalElements) {
        JSTaggedValue key = map->GetKey(thread, index);
        if (!key.IsHole()) {
            keyHandle.Update(key);
            valueHandle.Update(map->GetValue(thread, index));
            if (itemKind == IterationKind::KEY) {
                ElementAccessor::Set(thread, newArrayHandle, k, keyHandle, true);
            } else if (itemKind == IterationKind::VALUE) {
                ElementAccessor::Set(thread, newArrayHandle, k, valueHandle, true);
            } else {
                JSHandle<TaggedArray> array(factory->NewTaggedArray(2));  // 2 means the length of array
                array->Set(thread, 0, keyHandle);
                array->Set(thread, 1, valueHandle);
                JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
                ElementAccessor::Set(thread, newArrayHandle, k, keyAndValue, true);
            }
            k++;
        }
        index++;
    }
    JSHandle<JSArray>(newArrayHandle)->SetArrayLength(thread, k);
    return newArrayHandle.GetTaggedValue();
}
}  // namespace panda::ecmascript
