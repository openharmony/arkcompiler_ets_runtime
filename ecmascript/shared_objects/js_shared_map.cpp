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

#include "ecmascript/linked_hash_table.h"
#include "ecmascript/shared_objects/concurrent_api_scope.h"

namespace panda::ecmascript {
void JSSharedMap::Set(JSThread *thread, const JSHandle<JSSharedMap> &map,
                      const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    if (!key->IsSharedType() || !value->IsSharedType()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::TYPE_ERROR,
                                                               "Parameter error. Only accept sendable value.");
        THROW_NEW_ERROR_AND_RETURN(thread, error);
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(map));
    RETURN_IF_ABRUPT_COMPLETION(thread);

    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject()));
    JSHandle<LinkedHashMap> newMap = LinkedHashMap::Set(thread, mapHandle, key, value);
    map->SetLinkedMap(thread, newMap);
}

bool JSSharedMap::Delete(JSThread *thread, const JSHandle<JSSharedMap> &map, const JSHandle<JSTaggedValue> &key)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(map));
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
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(map));
    RETURN_IF_ABRUPT_COMPLETION(thread);
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject()));
    JSHandle<LinkedHashMap> newMap = LinkedHashMap::Clear(thread, mapHandle);
    map->SetLinkedMap(thread, newMap);
}

bool JSSharedMap::Has(JSThread *thread, const JSHandle<JSSharedMap> &map, JSTaggedValue key)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap> scope(thread, JSHandle<JSTaggedValue>::Cast(map));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    return LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject())->Has(thread, key);
}

JSTaggedValue JSSharedMap::Get(JSThread *thread, const JSHandle<JSSharedMap> &map, JSTaggedValue key)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap> scope(thread, JSHandle<JSTaggedValue>::Cast(map));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());
    return LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject())->Get(thread, key);
}

uint32_t JSSharedMap::GetSize(JSThread *thread, const JSHandle<JSSharedMap> &map)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap> scope(thread, JSHandle<JSTaggedValue>::Cast(map));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    return LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject())->NumberOfElements();
}

JSTaggedValue JSSharedMap::GetKey(JSThread *thread, const JSHandle<JSSharedMap> &map, uint32_t entry)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap> scope(thread, JSHandle<JSTaggedValue>::Cast(map));
    ASSERT_PRINT(entry >= 0 && entry < GetSize(thread, map), "entry must be non-negative integer less than capacity");
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());
    return LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject())->GetKey(entry);
}

JSTaggedValue JSSharedMap::GetValue(JSThread *thread, const JSHandle<JSSharedMap> &map, uint32_t entry)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap> scope(thread, JSHandle<JSTaggedValue>::Cast(map));
    ASSERT_PRINT(entry >= 0 && entry < GetSize(thread, map), "entry must be non-negative integer less than capacity");
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());
    return LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject())->GetValue(entry);
}
}  // namespace panda::ecmascript
