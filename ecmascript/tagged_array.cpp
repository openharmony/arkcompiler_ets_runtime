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

#define ECMASCRIPT_TAGGED_ARRAY_CPP

#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_array-inl.h"

#include "ecmascript/object_factory.h"

namespace panda::ecmascript {

JSTaggedValue TaggedArray::Get([[maybe_unused]] const JSThread *thread, uint32_t idx) const
{
    return Get(idx);
}

uint32_t TaggedArray::GetIdx(const JSTaggedValue &value) const
{
    uint32_t length = GetLength();

    for (uint32_t i = 0; i < length; i++) {
        if (JSTaggedValue::SameValue(Get(i), value)) {
            return i;
        }
    }
    return TaggedArray::MAX_ARRAY_INDEX;
}

JSTaggedValue TaggedArray::GetBit(uint32_t idx, uint32_t bitOffset) const
{
    ASSERT(idx < GetLength());
    JSTaggedType element = Get(idx).GetRawData();
    return JSTaggedValue(int((element >> bitOffset) & 1ULL));
}


void TaggedArray::SetBit(const JSThread *thread, uint32_t idx, uint32_t bitOffset, const JSTaggedValue &value)
{
    ASSERT(idx < GetLength());
    JSTaggedType element = Get(idx).GetRawData();
    if (value.IsZero()) {
        element &= ~(1ULL << bitOffset);
    } else {
        element |= (1ULL << bitOffset);
    }
    Set<false>(thread, idx, JSTaggedValue(element));
}

JSHandle<TaggedArray> TaggedArray::Append(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                          const JSHandle<TaggedArray> &second)
{
    uint32_t firstLength = first->GetLength();
    uint32_t secondLength = second->GetLength();
    uint32_t length = firstLength + secondLength;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> argument = factory->NewTaggedArray(length);
    uint32_t index = 0;
    for (; index < firstLength; ++index) {
        argument->Set(thread, index, first->Get(index));
    }
    for (; index < length; ++index) {
        argument->Set(thread, index, second->Get(index - firstLength));
    }
    return argument;
}

JSHandle<TaggedArray> TaggedArray::AppendSkipHole(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                                  const JSHandle<TaggedArray> &second, uint32_t copyLength)
{
    uint32_t firstLength = first->GetLength();
    uint32_t secondLength = second->GetLength();
    ASSERT(firstLength + secondLength >= copyLength);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> argument = factory->NewTaggedArray(copyLength);
    uint32_t index = 0;
    for (; index < firstLength; ++index) {
        JSTaggedValue val = first->Get(index);
        if (val.IsHole()) {
            break;
        }
        argument->Set(thread, index, val);
        ASSERT(copyLength >= index);
    }
    for (uint32_t i = 0; i < secondLength; ++i) {
        JSTaggedValue val = second->Get(i);
        if (val.IsHole()) {
            break;
        }
        argument->Set(thread, index++, val);
        ASSERT(copyLength >= index);
    }
    return argument;
}

bool TaggedArray::HasDuplicateEntry() const
{
    uint32_t length = GetLength();
    for (uint32_t i = 0; i < length; i++) {
        for (uint32_t j = i + 1; j < length; j++) {
            if (JSTaggedValue::SameValue(Get(i), Get(j))) {
                return true;
            }
        }
    }
    return false;
}

void TaggedArray::InitializeWithSpecialValue(JSTaggedValue initValue, uint32_t length, uint32_t extraLength)
{
    SetLength(length);
    SetExtraLength(extraLength);
    FillRangeWithSpecialValue(initValue, 0, length);
}

void TaggedArray::FillRangeWithSpecialValue(JSTaggedValue initValue, uint32_t start, uint32_t end)
{
    ASSERT(initValue.IsSpecial());
    for (uint32_t i = start; i < end; i++) {
        size_t offset = JSTaggedValue::TaggedTypeSize() * i;
        Barriers::SetPrimitive<JSTaggedType>(GetData(), offset, initValue.GetRawData());
    }
}

JSHandle<TaggedArray> TaggedArray::SetCapacity(const JSThread *thread, const JSHandle<TaggedArray> &array,
                                               uint32_t capa)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t oldLength = array->GetLength();
    JSHandle<TaggedArray> newArray = factory->CopyArray(array, oldLength, capa);
    return newArray;
}

JSHandle<TaggedArray> TaggedArray::SetCapacityInOldSpace(const JSThread *thread,
                                                         const JSHandle<TaggedArray> &array, uint32_t capa)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t oldLength = array->GetLength();
    JSHandle<TaggedArray> newArray =
        factory->CopyArray(array, oldLength, capa, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
    return newArray;
}

void TaggedArray::RemoveElementByIndex(const JSThread *thread, JSHandle<TaggedArray> &srcArray,
                                       uint32_t index, uint32_t effectiveLength, bool noNeedBarrier)
{
    ASSERT(0 <= index || index < effectiveLength);
    ASSERT(effectiveLength > 0);
    if (noNeedBarrier) {
        size_t taggedTypeSize = JSTaggedValue::TaggedTypeSize();
        size_t offset = taggedTypeSize * index;
        auto *addr = reinterpret_cast<JSTaggedType *>(ToUintPtr(srcArray->GetData()) + offset);
        while (index < effectiveLength - 1) {
            *addr = *(addr + 1);
            addr++;
            index++;
        }
    } else {
        while (index < effectiveLength - 1) {
            srcArray->Set(thread, index, srcArray->Get(index + 1));
            index++;
        }
    }
    srcArray->Set(thread, effectiveLength - 1, JSTaggedValue::Hole());
}

void TaggedArray::InsertElementByIndex(const JSThread *thread, JSHandle<TaggedArray> &srcArray,
    const JSHandle<JSTaggedValue> &value, uint32_t index, uint32_t effectiveLength)
{
    ASSERT(0 <= index || index <= effectiveLength);
    ASSERT(effectiveLength < srcArray->GetLength());
    while (effectiveLength != index && effectiveLength > 0) {
        JSTaggedValue oldValue = srcArray->Get(effectiveLength - 1);
        srcArray->Set(thread, effectiveLength, oldValue);
        effectiveLength--;
    }
    srcArray->Set(thread, index, value.GetTaggedValue());
}

void TaggedArray::CopyTaggedArrayElement(const JSThread *thread, JSHandle<TaggedArray> &srcElements,
                                         JSHandle<TaggedArray> &dstElements, uint32_t effectiveLength)
{
    ASSERT(effectiveLength <= srcElements->GetLength());
    ASSERT(effectiveLength <= dstElements->GetLength());
    dstElements->Copy(thread, 0, 0, srcElements.GetObject<TaggedArray>(), effectiveLength);
}

bool TaggedArray::IsDictionaryMode() const
{
    return GetClass()->IsDictionary();
}

bool TaggedArray::IsYoungAndNotMarking(const JSThread *thread)
{
    Region *region = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(this));
    return region->InYoungSpace() && !thread->IsConcurrentMarkingOrFinished();
}

void TaggedArray::Trim(const JSThread *thread, uint32_t newLength)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t oldLength = GetLength();
    ASSERT(oldLength > newLength);
    size_t trimBytes = (oldLength - newLength) * JSTaggedValue::TaggedTypeSize();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    factory->FillFreeObject<true>(ToUintPtr(this) + size, trimBytes, RemoveSlots::YES, ToUintPtr(this));
    SetLength(newLength);
}

void MutantTaggedArray::InitializeWithSpecialValue(JSTaggedType initValue, uint32_t length, uint32_t extraLength)
{
    SetLength(length);
    SetExtraLength(extraLength);
    for (uint32_t i = 0; i < length; i++) {
        size_t offset = JSTaggedValue::TaggedTypeSize() * i;
        // NOLINTNEXTLINE(readability-misleading-indentation)
        Barriers::SetPrimitive<JSTaggedType>(GetData(), offset, initValue);
    }
}
}  // namespace panda::ecmascript
