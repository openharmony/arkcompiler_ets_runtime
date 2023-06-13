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

#ifndef ECMASCRIPT_LAYOUT_INFO_INL_H
#define ECMASCRIPT_LAYOUT_INFO_INL_H

#include "ecmascript/layout_info.h"

#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
inline int LayoutInfo::GetPropertiesCapacity() const
{
    return static_cast<int>((GetLength()) >> ELEMENTS_INDEX_LOG2);
}

inline int LayoutInfo::NumberOfElements() const
{
    return GetExtraLength();
}

inline void LayoutInfo::SetNumberOfElements([[maybe_unused]] const JSThread *thread, int properties)
{
    SetExtraLength(properties);
}

inline uint32_t LayoutInfo::GetKeyIndex(int index) const
{
    return static_cast<uint32_t>(index) << ELEMENTS_INDEX_LOG2;
}

inline uint32_t LayoutInfo::GetAttrIndex(int index) const
{
    return (static_cast<uint32_t>(index) << ELEMENTS_INDEX_LOG2) + ATTR_INDEX_OFFSET;
}

inline void LayoutInfo::SetPropertyInit(const JSThread *thread, int index, const JSTaggedValue &key,
                                        const PropertyAttributes &attr)
{
    uint32_t fixedIdx = GetKeyIndex(index);
    TaggedArray::Set(thread, fixedIdx, key);
    TaggedArray::Set(thread, fixedIdx + ATTR_INDEX_OFFSET, attr.GetNormalTagged());
}

inline void LayoutInfo::SetNormalAttr(const JSThread *thread, int index, const PropertyAttributes &attr)
{
    uint32_t fixedIdx = GetAttrIndex(index);
    PropertyAttributes oldAttr(TaggedArray::Get(fixedIdx));
    oldAttr.SetNormalAttr(attr.GetNormalAttr());
    TaggedArray::Set(thread, fixedIdx, oldAttr.GetTaggedValue());
}

inline JSTaggedValue LayoutInfo::GetKey(int index) const
{
    uint32_t fixedIdx = GetKeyIndex(index);
    return TaggedArray::Get(fixedIdx);
}

inline PropertyAttributes LayoutInfo::GetAttr(int index) const
{
    uint32_t fixedIdx = GetAttrIndex(index);
    return PropertyAttributes(TaggedArray::Get(fixedIdx));
}

inline JSTaggedValue LayoutInfo::GetSortedKey(int index) const
{
    uint32_t fixedIdx = GetSortedIndex(index);
    return GetKey(fixedIdx);
}

inline uint32_t LayoutInfo::GetSortedIndex(int index) const
{
    return GetAttr(index).GetSortedIndex();
}

inline void LayoutInfo::SetSortedIndex(const JSThread *thread, int index, int sortedIndex)
{
    uint32_t fixedIdx = GetAttrIndex(index);
    PropertyAttributes attr(TaggedArray::Get(fixedIdx));
    attr.SetSortedIndex(sortedIndex);
    TaggedArray::Set(thread, fixedIdx, attr.GetTaggedValue());
}

inline int LayoutInfo::FindElementWithCache(const JSThread *thread, JSHClass *cls, JSTaggedValue key,
                                            int propertiesNumber)
{
    ASSERT(NumberOfElements() >= propertiesNumber);
    const int MAX_ELEMENTS_LINER_SEARCH = 9; // 9: Builtins Object properties number is nine;
    if (propertiesNumber <= MAX_ELEMENTS_LINER_SEARCH) {
        Span<struct Properties> sp(GetProperties(), propertiesNumber);
        for (int i = 0; i < propertiesNumber; i++) {
            if (sp[i].key_ == key) {
                return i;
            }
        }
        return -1;
    }

    PropertiesCache *cache = thread->GetPropertiesCache();
    int index = cache->Get(cls, key);
    if (index == PropertiesCache::NOT_FOUND) {
        index = BinarySearch(key, propertiesNumber);
        cache->Set(cls, key, index);
    }
    return index;
}

inline int LayoutInfo::BinarySearch(JSTaggedValue key, int propertiesNumber)
{
    ASSERT(NumberOfElements() >= propertiesNumber);
    int low = 0;
    int elements = NumberOfElements();
    int high = elements - 1;
    uint32_t keyHash = key.GetKeyHashCode();

    ASSERT(low <= high);

    while (low <= high) {
        int mid = low + (high - low) / 2;  // 2: half
        JSTaggedValue midKey = GetSortedKey(mid);
        uint32_t midHash = midKey.GetKeyHashCode();
        if (midHash > keyHash) {
            high = mid - 1;
        } else if (midHash < keyHash) {
            low = mid + 1;
        } else {
            int sortIndex = static_cast<int>(GetSortedIndex(mid));
            JSTaggedValue currentKey = GetKey(sortIndex);
            if (currentKey == key) {
                return sortIndex < propertiesNumber ? sortIndex : -1;
            }
            int midLeft = mid;
            int midRight = mid;
            while (midLeft - 1 >= 0) {
                sortIndex = static_cast<int>(GetSortedIndex(--midLeft));
                currentKey = GetKey(sortIndex);
                if (currentKey.GetKeyHashCode() == keyHash) {
                    if (currentKey == key) {
                        return sortIndex < propertiesNumber ? sortIndex : -1;
                    }
                } else {
                    break;
                }
            }
            while (midRight + 1 < elements) {
                sortIndex = static_cast<int>(GetSortedIndex(++midRight));
                currentKey = GetKey(sortIndex);
                if (currentKey.GetKeyHashCode() == keyHash) {
                    if (currentKey == key) {
                        return sortIndex < propertiesNumber ? sortIndex : -1;
                    }
                } else {
                    break;
                }
            }
            return -1;
        }
    }
    return -1;
}

inline void LayoutInfo::SetIsNotHole(const JSThread *thread, int index)
{
    uint32_t fixedIdx = GetAttrIndex(index);
    PropertyAttributes attr(TaggedArray::Get(fixedIdx));
    attr.SetIsNotHole(true);
    TaggedArray::Set(thread, fixedIdx, attr.GetTaggedValue());
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_LAYOUT_INFO_INL_H
