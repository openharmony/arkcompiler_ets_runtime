/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/js_weak_container.h"

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/linked_hash_table.h"

namespace panda::ecmascript {
void JSWeakMap::Set(JSThread *thread, const JSHandle<JSWeakMap> &map, const JSHandle<JSTaggedValue> &key,
                    const JSHandle<JSTaggedValue> &value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    if (!LinkedHashMap::IsKey(JSTaggedValue(key.GetTaggedValue().CreateAndGetWeakRef()))) {
        THROW_TYPE_ERROR(thread, "the value must be Key of JSWeakMap");
    }
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap(thread).GetTaggedObject()));

    JSHandle<LinkedHashMap> newMap = LinkedHashMap::SetWeakRef(thread, mapHandle, key, value);
    map->SetLinkedMap(thread, newMap);
}

bool JSWeakMap::Delete(JSThread *thread, const JSHandle<JSWeakMap> &map, const JSHandle<JSTaggedValue> &key)
{
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap(thread).GetTaggedObject()));
    int entry = mapHandle->FindElement(thread, key.GetTaggedValue());
    if (entry == -1) {
        return false;
    }
    mapHandle->RemoveEntry(thread, entry);

    JSHandle<LinkedHashMap> newMap = LinkedHashMap::Shrink(thread, mapHandle);
    map->SetLinkedMap(thread, newMap);
    return true;
}

bool JSWeakMap::Has(JSThread *thread, JSTaggedValue key) const
{
    return LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject())->Has(thread, key);
}

JSTaggedValue JSWeakMap::Get(JSThread *thread, JSTaggedValue key) const
{
    return LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject())->Get(thread, key);
}

int JSWeakMap::GetSize(const JSThread *thread) const
{
    return LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject())->NumberOfElements();
}

JSTaggedValue JSWeakMap::GetKey(const JSThread *thread, int entry) const
{
    ASSERT_PRINT(entry >= 0 && entry < GetSize(thread), "entry must be non-negative integer less than capacity");
    return LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject())->GetKey(thread, entry);
}

JSTaggedValue JSWeakMap::GetValue(const JSThread *thread, int entry) const
{
    ASSERT_PRINT(entry >= 0 && entry < GetSize(thread), "entry must be non-negative integer less than capacity");
    return LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject())->GetValue(thread, entry);
}

void JSWeakSet::Add(JSThread *thread, const JSHandle<JSWeakSet> &weakSet, const JSHandle<JSTaggedValue> &value)
{
    if (!LinkedHashSet::IsKey(value.GetTaggedValue())) {
        THROW_TYPE_ERROR(thread, "the value must be Key of JSWeakSet");
    }
    JSHandle<LinkedHashSet> weakSetHandle(thread, LinkedHashSet::Cast(weakSet->GetLinkedSet(thread).GetTaggedObject()));

    JSHandle<LinkedHashSet> newSet = LinkedHashSet::AddWeakRef(thread, weakSetHandle, value);
    weakSet->SetLinkedSet(thread, newSet);
}

bool JSWeakSet::Delete(JSThread *thread, const JSHandle<JSWeakSet> &weakSet, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<LinkedHashSet> weakSetHandle(thread, LinkedHashSet::Cast(weakSet->GetLinkedSet(thread).GetTaggedObject()));
    int entry = weakSetHandle->FindElement(thread, value.GetTaggedValue());
    if (entry == -1) {
        return false;
    }
    weakSetHandle->RemoveEntry(thread, entry);
    JSHandle<LinkedHashSet> newSet = LinkedHashSet::Shrink(thread, weakSetHandle);
    weakSet->SetLinkedSet(thread, newSet);
    return true;
}

bool JSWeakSet::Has(JSThread *thread, JSTaggedValue value) const
{
    return LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject())->Has(thread, value);
}

int JSWeakSet::GetSize(const JSThread *thread) const
{
    return LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject())->NumberOfElements();
}

JSTaggedValue JSWeakSet::GetValue(const JSThread *thread, int entry) const
{
    ASSERT_PRINT(entry >= 0 && entry < GetSize(thread), "entry must be non-negative integer less than capacity");
    return LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject())->GetValue(thread, entry);
}
}  // namespace panda::ecmascript
