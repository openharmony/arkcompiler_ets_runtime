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

#include "ecmascript/weak_vector.h"

#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
JSHandle<WeakVector> WeakVector::Create(const JSThread *thread, uint32_t capacity, MemSpaceType type)
{
    ASSERT(capacity < MAX_VECTOR_INDEX);

    uint32_t length = VectorToArrayIndex(capacity);
    JSHandle<WeakVector> vector;
    if (type == MemSpaceType::NON_MOVABLE) {
        vector = JSHandle<WeakVector>(thread->GetEcmaVM()->GetFactory()
                                      ->NewTaggedArray(length, JSTaggedValue::Hole(), MemSpaceType::NON_MOVABLE));
    } else {
        vector = JSHandle<WeakVector>(thread->GetEcmaVM()->GetFactory()->NewTaggedArray(length));
    }

    vector->SetEnd(thread, 0);
    return vector;
}

bool WeakVector::Delete(const JSThread *thread, uint32_t index)
{
    uint32_t end = GetEnd();
    if (index < end) {
        Set(thread, index, JSTaggedValue::Hole());
        return true;
    }
    return false;
}

JSHandle<WeakVector> WeakVector::Grow(const JSThread *thread, const JSHandle<WeakVector> &old, uint32_t newCapacity)
{
    uint32_t oldCapacity = old->GetCapacity();
    ASSERT(newCapacity > oldCapacity);
    if (oldCapacity == MAX_VECTOR_INDEX) {
        return old;
    }

    if (newCapacity > MAX_VECTOR_INDEX) {
        newCapacity = MAX_VECTOR_INDEX;
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newVec = factory->CopyArray(JSHandle<TaggedArray>(old), VectorToArrayIndex(oldCapacity),
                                                      VectorToArrayIndex(newCapacity));

    return JSHandle<WeakVector>(newVec);
}

JSHandle<WeakVector> WeakVector::Append(const JSThread *thread, const JSHandle<WeakVector> &vec,
                                        const JSHandle<JSTaggedValue> &value, ElementType type)
{
    if (!vec->Full()) {
        JSTaggedValue storeVal = GetStoreVal(value, type);
        vec->PushBack(thread, storeVal);
        return vec;
    }

    return AppendToFullVec(thread, vec, value, type);
}

JSHandle<WeakVector> WeakVector::FillOrAppend(const JSThread *thread, const JSHandle<WeakVector> &vec,
                                              const JSHandle<JSTaggedValue> &value, ElementType type)
{
    if (!vec->Full()) {
        JSTaggedValue storeVal = GetStoreVal(value, type);
        vec->PushBack(thread, storeVal);
        return vec;
    }

    // if exist hole, use it.
    uint32_t holeIndex = CheckHole(vec);
    if (holeIndex != TaggedArray::MAX_ARRAY_INDEX) {
        JSTaggedValue storeVal = GetStoreVal(value, type);
        vec->Set(thread, holeIndex, storeVal);
        return vec;
    }

    return AppendToFullVec(thread, vec, value, type);
}

JSHandle<WeakVector> WeakVector::AppendToFullVec(const JSThread *thread, const JSHandle<WeakVector> &vec,
                                                 const JSHandle<JSTaggedValue> &value, ElementType type)
{
    uint32_t newCapacity = vec->GetCapacity() + DEFAULT_GROW_SIZE;
    JSHandle<WeakVector> newVec = WeakVector::Grow(thread, JSHandle<WeakVector>(vec), newCapacity);
    JSTaggedValue storeVal = GetStoreVal(value, type);
    [[maybe_unused]] uint32_t index = newVec->PushBack(thread, storeVal);
    ASSERT(index != TaggedArray::MAX_ARRAY_INDEX);
    return newVec;
}

JSTaggedValue WeakVector::GetStoreVal(const JSHandle<JSTaggedValue> &value, ElementType type)
{
    if (type == ElementType::NORMAL) {
        return value.GetTaggedValue();
    }

    if (value->IsHeapObject()) {
        return value->CreateAndGetWeakRef();
    }
    return value.GetTaggedValue();
}

JSHandle<WeakVector> WeakVector::Copy(const JSThread *thread, const JSHandle<WeakVector> &vec,
                                      bool needExtend)
{
    uint32_t capacity = vec->GetCapacity();
    uint32_t oldLength = VectorToArrayIndex(capacity);
    if (needExtend) {
        capacity += DEFAULT_GROW_SIZE;
    }
    uint32_t newLength = VectorToArrayIndex(capacity);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newVec = factory->CopyArray(JSHandle<TaggedArray>(vec), oldLength, newLength);
    return JSHandle<WeakVector>(newVec);
}

uint32_t WeakVector::CheckHole(const JSHandle<WeakVector> &vec)
{
    for (uint32_t i = 0; i < vec->GetEnd(); i++) {
        JSTaggedValue value = vec->Get(i);
        if (value.IsHole()) {
            return i;
        }
    }
    return TaggedArray::MAX_ARRAY_INDEX;
}

uint32_t WeakVector::PushBack(const JSThread *thread, JSTaggedValue value)
{
    uint32_t end = GetEnd();
    if (end == GetCapacity()) {
        return TaggedArray::MAX_ARRAY_INDEX;
    }

    Set(thread, end, value);
    SetEnd(thread, end + 1);
    return end;
}
}  // namespace panda::ecmascript
