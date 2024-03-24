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

#include "ecmascript/shared_objects/js_shared_map.h"

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/shared_objects/concurrent_modification_scope.h"

namespace panda::ecmascript {
void JSSharedMap::Set(JSThread *thread, const JSHandle<JSSharedMap> &map,
                      const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    if (!key->IsSharedType()) {
        THROW_TYPE_ERROR(thread, "the key of shared map must be shared too");
    }
    if (!value->IsSharedType()) {
        THROW_TYPE_ERROR(thread, "the value of shared map must be shared too");
    }
    [[maybe_unused]] ConcurrentModScope<JSSharedMap, ModType::WRITE> scope(thread,
        map.GetTaggedValue().GetTaggedObject());
    RETURN_IF_ABRUPT_COMPLETION(thread);

    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject()));
    JSHandle<LinkedHashMap> newMap = LinkedHashMap::Set(thread, mapHandle, key, value);
    map->SetLinkedMap(thread, newMap);
}

bool JSSharedMap::Delete(JSThread *thread, const JSHandle<JSSharedMap> &map, const JSHandle<JSTaggedValue> &key)
{
    [[maybe_unused]] ConcurrentModScope<JSSharedMap, ModType::WRITE> scope(thread,
        map.GetTaggedValue().GetTaggedObject());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject()));
    int entry = mapHandle->FindElement(thread, key.GetTaggedValue());
    if (entry == -1) {
        return false;
    }
    mapHandle->RemoveEntry(thread, entry);
    return true;
}

void JSSharedMap::Clear(JSThread *thread, const JSHandle<JSSharedMap> &map)
{
    [[maybe_unused]] ConcurrentModScope<JSSharedMap, ModType::WRITE> scope(thread,
        map.GetTaggedValue().GetTaggedObject());
    RETURN_IF_ABRUPT_COMPLETION(thread);
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject()));
    JSHandle<LinkedHashMap> newMap = LinkedHashMap::Clear(thread, mapHandle);
    map->SetLinkedMap(thread, newMap);
}

bool JSSharedMap::Has(JSThread *thread, JSTaggedValue key) const
{
    [[maybe_unused]] ConcurrentModScope<JSSharedMap> scope(thread, reinterpret_cast<const TaggedObject*>(this));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->Has(thread, key);
}

JSTaggedValue JSSharedMap::Get(JSThread *thread, JSTaggedValue key) const
{
    [[maybe_unused]] ConcurrentModScope<JSSharedMap> scope(thread, reinterpret_cast<const TaggedObject*>(this));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->Get(thread, key);
}

uint32_t JSSharedMap::GetSize(JSThread *thread) const
{
    [[maybe_unused]] ConcurrentModScope<JSSharedMap> scope(thread, reinterpret_cast<const TaggedObject*>(this));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->NumberOfElements();
}

JSTaggedValue JSSharedMap::GetKey(JSThread *thread, uint32_t entry) const
{
    [[maybe_unused]] ConcurrentModScope<JSSharedMap> scope(thread, reinterpret_cast<const TaggedObject*>(this));
    ASSERT_PRINT(entry >= 0 && entry < GetSize(thread), "entry must be non-negative integer less than capacity");
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->GetKey(entry);
}

JSTaggedValue JSSharedMap::GetValue(JSThread *thread, uint32_t entry) const
{
    [[maybe_unused]] ConcurrentModScope<JSSharedMap> scope(thread, reinterpret_cast<const TaggedObject*>(this));
    ASSERT_PRINT(entry >= 0 && entry < GetSize(thread), "entry must be non-negative integer less than capacity");
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());
    return LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject())->GetValue(entry);
}
}  // namespace panda::ecmascript
