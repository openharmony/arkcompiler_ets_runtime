/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/js_map.h"

#include "ecmascript/linked_hash_table.h"
#include <cmath>

namespace panda::ecmascript {
// This operation canonicalizes -0 to +0 for keyed collection keys
// NaN is returned as-is (SameValueZero treats NaN as equal to NaN)
static inline JSTaggedValue CanonicalizeKeyedCollectionKey(JSTaggedValue key)
{
    if (!key.IsDouble()) {
        return key;
    }
    double d = key.GetDouble();
    // 3.a. If key is -0, return +0
    // Only canonicalize -0, not all negative numbers like -Infinity
    if (d == 0.0 && std::signbit(d) != 0) {
        return JSTaggedValue(0.0);  // NOLINT(readability-magic-numbers)
    }
    // 3.b. If key is NaN, return key as-is (SameValueZero handles NaN equality)
    return key;
}

void JSMap::Set(JSThread *thread, const JSHandle<JSMap> &map, const JSHandle<JSTaggedValue> &key,
                const JSHandle<JSTaggedValue> &value)
{
    // 3. Set key to CanonicalizeKeyedCollectionKey(key).
    JSTaggedValue canonicalKey = CanonicalizeKeyedCollectionKey(key.GetTaggedValue());
    if (!LinkedHashMap::IsKey(canonicalKey)) {
        THROW_TYPE_ERROR(thread, "the value must be Key of JSSet");
    }
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap(thread).GetTaggedObject()));
    JSHandle<JSTaggedValue> canonicalKeyHandle(thread, canonicalKey);

    JSHandle<LinkedHashMap> newMap = LinkedHashMap::Set(thread, mapHandle, canonicalKeyHandle, value);
    map->SetLinkedMap(thread, newMap);
}

bool JSMap::Delete(const JSThread *thread, const JSHandle<JSMap> &map, const JSHandle<JSTaggedValue> &key)
{
    JSTaggedValue canonicalKey = CanonicalizeKeyedCollectionKey(key.GetTaggedValue());
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap(thread).GetTaggedObject()));
    int entry = mapHandle->FindElement(thread, canonicalKey);
    if (entry == -1) {
        return false;
    }
    mapHandle->RemoveEntry(thread, entry);
    return true;
}

void JSMap::Clear(const JSThread *thread, const JSHandle<JSMap> &map)
{
    LinkedHashMap *linkedMap = LinkedHashMap::Cast(map->GetLinkedMap(thread).GetTaggedObject());
    JSHandle<LinkedHashMap> mapHandle(thread, LinkedHashMap::Cast(map->GetLinkedMap(thread).GetTaggedObject()));
    JSHandle<LinkedHashMap> newMap = linkedMap->Clear(thread, mapHandle);
    map->SetLinkedMap(thread, newMap);
}

bool JSMap::Has(JSThread *thread, JSTaggedValue key) const
{
    JSTaggedValue canonicalKey = CanonicalizeKeyedCollectionKey(key);
    return LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject())->Has(thread, canonicalKey);
}

JSTaggedValue JSMap::Get(JSThread *thread, JSTaggedValue key) const
{
    JSTaggedValue canonicalKey = CanonicalizeKeyedCollectionKey(key);
    return LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject())->Get(thread, canonicalKey);
}

uint32_t JSMap::GetSize(JSThread *thread) const
{
    return LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject())->NumberOfElements();
}

JSTaggedValue JSMap::GetKey(JSThread *thread, uint32_t entry) const
{
    ASSERT_PRINT(entry >= 0 && entry < GetSize(thread), "entry must be non-negative integer less than capacity");
    return LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject())->GetKey(thread, entry);
}

JSTaggedValue JSMap::GetValue(JSThread *thread, uint32_t entry) const
{
    ASSERT_PRINT(entry >= 0 && entry < GetSize(thread), "entry must be non-negative integer less than capacity");
    return LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject())->GetValue(thread, entry);
}
}  // namespace panda::ecmascript
