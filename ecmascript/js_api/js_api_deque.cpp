/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/js_api/js_api_deque.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
class JSAPIDequelterator;

void JSAPIDeque::InsertFront(JSThread *thread, const JSHandle<JSAPIDeque> &deque, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedArray> elements(thread, deque->GetElements(thread));
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    uint32_t first = deque->GetFirst();
    uint32_t last = deque->GetLast();
    ASSERT(capacity != 0);
    if ((first + capacity - 1) % capacity == last) {
        elements = GrowCapacity(thread, deque, capacity, first, last);
        ASSERT(!elements->IsDictionaryMode());
        deque->SetLast(capacity - 1);
        first = 0;
    }
    capacity = elements->GetLength();
    ASSERT(capacity != 0);
    first = (first + capacity - 1) % capacity;
    elements->Set(thread, first, value);
    deque->SetFirst(first);
}

void JSAPIDeque::InsertEnd(JSThread *thread, const JSHandle<JSAPIDeque> &deque, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedArray> elements(thread, deque->GetElements(thread));
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    uint32_t first = deque->GetFirst();
    uint32_t last = deque->GetLast();
    ASSERT(capacity != 0);
    if (first == (last + 1) % capacity) {
        elements = GrowCapacity(thread, deque, capacity, first, last);
        ASSERT(!elements->IsDictionaryMode());
        deque->SetFirst(0);
        last = capacity - 1;
    }
    elements->Set(thread, last, value);
    capacity = elements->GetLength();
    ASSERT(capacity != 0);
    last = (last + 1) % capacity;
    deque->SetLast(last);
}

JSTaggedValue JSAPIDeque::GetFront(const JSThread *thread)
{
    if (JSAPIDeque::IsEmpty()) {
        return JSTaggedValue::Undefined();
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    return elements->Get(thread, GetFirst());
}

JSTaggedValue JSAPIDeque::GetTail(const JSThread *thread)
{
    if (JSAPIDeque::IsEmpty()) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    uint32_t last = GetLast();
    ASSERT(capacity != 0);
    return elements->Get(thread, (last + capacity - 1) % capacity);
}

JSHandle<TaggedArray> JSAPIDeque::GrowCapacity(JSThread *thread, const JSHandle<JSAPIDeque> &deque,
                                               uint32_t oldCapacity, uint32_t first, uint32_t last)
{
    JSHandle<TaggedArray> oldElements(thread, deque->GetElements(thread));
    ASSERT(!oldElements->IsDictionaryMode());
    uint32_t newCapacity = ComputeCapacity(oldCapacity);
    uint32_t size = deque->GetSize(thread);
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyDeque(oldElements, newCapacity, size, first, last);
    deque->SetElements(thread, newElements);
    return newElements;
}

JSTaggedValue JSAPIDeque::PopFirst(JSThread *thread)
{
    if (JSAPIDeque::IsEmpty()) {
        return JSTaggedValue::Undefined();
    }
    uint32_t first = GetFirst();
    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    JSTaggedValue firstElement = elements->Get(thread, first);
    ASSERT(capacity != 0);
    first = (first + 1) % capacity;
    SetFirst(first);
    return firstElement;
}

JSTaggedValue JSAPIDeque::PopLast(JSThread *thread)
{
    if (JSAPIDeque::IsEmpty()) {
        return JSTaggedValue::Undefined();
    }
    uint32_t last = GetLast();
    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t capacity = elements->GetLength();
    ASSERT(capacity != 0);
    last = (last + capacity - 1) % capacity;
    JSTaggedValue lastElement = elements->Get(thread, last);
    SetLast(last);
    return lastElement;
}

bool JSAPIDeque::IsEmpty()
{
    uint32_t first = GetFirst();
    uint32_t last = GetLast();
    return first == last;
}

uint32_t JSAPIDeque::GetSize(const JSThread *thread) const
{
    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t first = GetFirst();
    uint32_t last = GetLast();
    ASSERT(capacity != 0);
    return (last - first + capacity) % capacity;
}

JSTaggedValue JSAPIDeque::Get(const JSThread *thread, const uint32_t index)
{
    ASSERT(index < GetSize(thread));
    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t first = GetFirst();
    ASSERT(capacity != 0);
    uint32_t curIndex = (first + index) % capacity;
    return elements->Get(thread, curIndex);
}

JSTaggedValue JSAPIDeque::Set(JSThread *thread, const uint32_t index, JSTaggedValue value)
{
    uint32_t length = static_cast<uint32_t>(GetSize(thread));
    if (index < 0 || index >= length) {
        return JSTaggedValue::False();
    }
    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t first = GetFirst();
    ASSERT(capacity != 0);
    uint32_t curIndex = (first + index) % capacity;
    elements->Set(thread, curIndex, value);
    return JSTaggedValue::True();
}

bool JSAPIDeque::Has(const JSThread *thread, JSTaggedValue value) const
{
    uint32_t first = GetFirst();
    uint32_t last = GetLast();
    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t index = first;
    while (index != last) {
        if (JSTaggedValue::SameValue(thread, elements->Get(thread, index), value)) {
            return true;
        }
        ASSERT(capacity != 0);
        index = (index + 1) % capacity;
    }
    return false;
}

JSHandle<TaggedArray> JSAPIDeque::OwnKeys(JSThread *thread, const JSHandle<JSAPIDeque> &deque)
{
    uint32_t length = deque->GetSize(thread);

    JSHandle<TaggedArray> oldElements(thread, deque->GetElements(thread));
    uint32_t oldCapacity = oldElements->GetLength();
    uint32_t newCapacity = ComputeCapacity(oldCapacity);
    uint32_t firstIndex = deque->GetFirst();
    uint32_t lastIndex = deque->GetLast();
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyDeque(oldElements, newCapacity, length, firstIndex, lastIndex);
    deque->SetFirst(0);
    deque->SetLast(length);
    deque->SetElements(thread, newElements);

    return JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>::Cast(deque));
}

JSHandle<TaggedArray> JSAPIDeque::OwnEnumKeys(JSThread *thread, const JSHandle<JSAPIDeque> &deque)
{
    uint32_t length = deque->GetSize(thread);

    JSHandle<TaggedArray> oldElements(thread, deque->GetElements(thread));
    ASSERT(!oldElements->IsDictionaryMode());
    uint32_t oldCapacity = oldElements->GetLength();
    uint32_t newCapacity = ComputeCapacity(oldCapacity);
    uint32_t firstIndex = deque->GetFirst();
    uint32_t lastIndex = deque->GetLast();
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyDeque(oldElements, newCapacity, length, firstIndex, lastIndex);
    deque->SetFirst(0);
    deque->SetLast(length);
    deque->SetElements(thread, newElements);

    return JSObject::GetOwnEnumPropertyKeys(thread, JSHandle<JSObject>::Cast(deque));
}

bool JSAPIDeque::GetOwnProperty(JSThread *thread, const JSHandle<JSAPIDeque> &deque,
                                const JSHandle<JSTaggedValue> &key)
{
    uint32_t index = 0;
    if (UNLIKELY(!JSTaggedValue::ToElementIndex(thread, key.GetTaggedValue(), &index))) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, key.GetTaggedValue());
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        CString errorMsg =
            "The type of \"key\" can not obtain attributes of no-number type. Received value is: "
            + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }

    uint32_t length = deque->GetSize(thread);
    if (index >= length) {
        ASSERT(length > 0);
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be > " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }

    deque->Get(thread, index);
    return true;
}

JSTaggedValue JSAPIDeque::GetIteratorObj(JSThread *thread, const JSHandle<JSAPIDeque> &deque)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIDequeIterator> iter(factory->NewJSAPIDequeIterator(deque));

    return iter.GetTaggedValue();
}

OperationResult JSAPIDeque::GetProperty(JSThread *thread, const JSHandle<JSAPIDeque> &obj,
                                        const JSHandle<JSTaggedValue> &key)
{
    int length = static_cast<int>(obj->GetSize(thread));
    if (length == 0) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, "Container is empty");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, OperationResult(thread,
                                                                        JSTaggedValue::Exception(),
                                                                        PropertyMetaData(false)));
    }
    JSHandle<JSTaggedValue> indexKey = key;
    if (indexKey->IsDouble()) {
        // Math.floor(1) will produce TaggedDouble, we need to cast into TaggedInt
        // For integer which is greater than INT32_MAX, it will remain TaggedDouble
        indexKey = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(indexKey->GetDouble()));
    }
    if (!indexKey->IsInt()) {
        CString errorMsg = "The type of \"index\" must be small integer.";
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error,
                                         OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }

    int index = indexKey->GetInt();
    if (index < 0 || index >= length) {
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be >= 0 && <= " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, OperationResult(thread,
                                                                        JSTaggedValue::Exception(),
                                                                        PropertyMetaData(false)));
    }

    return OperationResult(thread, obj->Get(thread, index), PropertyMetaData(false));
}

bool JSAPIDeque::SetProperty(JSThread *thread, const JSHandle<JSAPIDeque> &obj,
                             const JSHandle<JSTaggedValue> &key,
                             const JSHandle<JSTaggedValue> &value)
{
    int length = static_cast<int>(obj->GetSize(thread));
    JSHandle<JSTaggedValue> indexKey = key;
    if (indexKey->IsDouble()) {
        // Math.floor(1) will produce TaggedDouble, we need to cast into TaggedInt
        // For integer which is greater than INT32_MAX, it will remain TaggedDouble
        indexKey = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(indexKey->GetDouble()));
    }
    if (!indexKey->IsInt()) {
        return false;
    }

    int index = indexKey->GetInt();
    if (index < 0 || index >= length) {
        return false;
    }

    obj->Set(thread, index, value.GetTaggedValue());
    return true;
}
} // namespace panda::ecmascript
