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

#include "ecmascript/js_set.h"
#include "ecmascript/linked_hash_table.h"
#include <cmath>

namespace panda::ecmascript {
// CanonicalizeKeyedCollectionKey operation
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

void JSSet::Add(JSThread *thread, const JSHandle<JSSet> &set, const JSHandle<JSTaggedValue> &value)
{
    if (!LinkedHashSet::IsKey(value.GetTaggedValue())) {
        //  throw error
        THROW_TYPE_ERROR(thread, "the value must be Key of JSSet");
    }
    // 3. Set value to CanonicalizeKeyedCollectionKey(value).
    JSTaggedValue canonicalValue = CanonicalizeKeyedCollectionKey(value.GetTaggedValue());
    JSHandle<LinkedHashSet> setHandle(thread, LinkedHashSet::Cast(set->GetLinkedSet(thread).GetTaggedObject()));
    JSHandle<LinkedHashSet> newSet = LinkedHashSet::Add(
        thread, setHandle, JSHandle<JSTaggedValue>(thread, canonicalValue));
    set->SetLinkedSet(thread, newSet);
}

bool JSSet::Delete(const JSThread *thread, const JSHandle<JSSet> &set, const JSHandle<JSTaggedValue> &value)
{
    // CanonicalizeKeyedCollectionKey(value)
    JSTaggedValue canonicalValue = CanonicalizeKeyedCollectionKey(value.GetTaggedValue());
    JSHandle<LinkedHashSet> setHandle(thread, LinkedHashSet::Cast(set->GetLinkedSet(thread).GetTaggedObject()));
    int entry = setHandle->FindElement(thread, canonicalValue);
    if (entry == -1) {
        return false;
    }
    setHandle->RemoveEntry(thread, entry);
    return true;
}

void JSSet::Clear(const JSThread *thread, const JSHandle<JSSet> &set)
{
    LinkedHashSet *linkedSet = LinkedHashSet::Cast(set->GetLinkedSet(thread).GetTaggedObject());
    JSHandle<LinkedHashSet> setHandle(thread, LinkedHashSet::Cast(set->GetLinkedSet(thread).GetTaggedObject()));
    JSHandle<LinkedHashSet> newSet = linkedSet->Clear(thread, setHandle);
    set->SetLinkedSet(thread, newSet);
}

bool JSSet::Has(const JSThread *thread, JSTaggedValue value) const
{
    // CanonicalizeKeyedCollectionKey(value)
    JSTaggedValue canonicalValue = CanonicalizeKeyedCollectionKey(value);
    return LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject())->Has(thread, canonicalValue);
}

uint32_t JSSet::GetSize(const JSThread *thread) const
{
    return LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject())->NumberOfElements();
}

JSTaggedValue JSSet::GetValue(const JSThread *thread, int entry) const
{
    ASSERT_PRINT(entry >= 0 && static_cast<uint32_t>(entry) < GetSize(thread),
        "entry must be non-negative integer less than capacity");
    return LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject())->GetValue(thread, entry);
}
}  // namespace panda::ecmascript
