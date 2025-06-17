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

#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"

namespace panda::ecmascript {
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
void JSAPIQueue::Add(JSThread *thread, const JSHandle<JSAPIQueue> &queue, const JSHandle<JSTaggedValue> &value)
{
    uint32_t length = queue->GetLength(thread).GetArrayLength();
    JSHandle<TaggedArray> elements = GrowCapacity(thread, queue, length + 1);

    ASSERT(!elements->IsDictionaryMode());
    uint32_t tail = queue->GetTail();

    elements->Set(thread, tail, value);
    queue->SetLength(thread, JSTaggedValue(++length));

    uint32_t elementsSize = elements->GetLength();
    ASSERT(elementsSize != 0);
    queue->SetTail((tail + 1) % elementsSize);
}

JSHandle<TaggedArray> JSAPIQueue::GrowCapacity(const JSThread *thread, const JSHandle<JSAPIQueue> &obj,
                                               uint32_t capacity)
{
    JSHandle<TaggedArray> newElements;
    uint32_t front = obj->GetFront();
    uint32_t tail = obj->GetTail();
    JSHandle<TaggedArray> oldElements(thread, obj->GetElements(thread));
    ASSERT(!oldElements->IsDictionaryMode());
    uint32_t oldLength = oldElements->GetLength();
    uint32_t newCapacity = 0;
    if (oldLength == 0) {
        newCapacity = ComputeCapacity(capacity);
        newElements = thread->GetEcmaVM()->GetFactory()->CopyArray(oldElements, oldLength, newCapacity);
    } else if ((tail + 1) % oldLength == front) {
        newCapacity = ComputeCapacity(capacity);
        newElements = thread->GetEcmaVM()->GetFactory()->CopyQueue(oldElements, newCapacity, front, tail);
        front = 0;
        tail = oldLength - 1;
    } else {
        return oldElements;
    }

    obj->SetElements(thread, newElements);
    obj->SetFront(front);
    obj->SetTail(tail);
    return newElements;
}

JSTaggedValue JSAPIQueue::GetFirst(JSThread *thread, const JSHandle<JSAPIQueue> &queue)
{
    if (queue->GetLength(thread).GetArrayLength() < 1) {
        return JSTaggedValue::Undefined();
    }

    uint32_t index = queue->GetFront();

    JSHandle<TaggedArray> elements(thread, queue->GetElements(thread));
    ASSERT(!elements->IsDictionaryMode());
    return elements->Get(thread, index);
}

JSTaggedValue JSAPIQueue::Pop(JSThread *thread, const JSHandle<JSAPIQueue> &queue)
{
    uint32_t length = queue->GetLength(thread).GetArrayLength();
    if (length < 1) {
        return JSTaggedValue::Undefined();
    }

    JSHandle<TaggedArray> elements(thread, queue->GetElements(thread));
    ASSERT(!elements->IsDictionaryMode());
    uint32_t front = queue->GetFront();

    JSTaggedValue value = elements->Get(thread, front);
    queue->SetLength(thread, JSTaggedValue(length - 1));
    uint32_t elementsSize = elements->GetLength();
    ASSERT(elementsSize != 0);
    elements->Set(thread, front, JSTaggedValue::Hole());
    queue->SetFront((front + 1) % elementsSize);

    return value;
}

JSTaggedValue JSAPIQueue::Get(JSThread *thread, const uint32_t index)
{
    uint32_t length = GetSize(thread);
    if (index >= length) {
        ASSERT(length > 0);
        std::ostringstream oss;
        oss << "The value of \"Get property index\" is out of range. It must be >= 0 && <= "
            << (length - 1) << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t front = GetCurrentFront();
    ASSERT(capacity != 0);
    uint32_t curIndex = (front + index) % capacity;
    return elements->Get(thread, curIndex);
}

JSTaggedValue JSAPIQueue::Set(JSThread *thread, const uint32_t index, JSTaggedValue value)
{
    if (GetLength(thread).GetArrayLength() == 0) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, "Container is empty");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (index < 0 || index >= GetLength(thread).GetArrayLength()) {
        std::ostringstream oss;
        oss << "The value of \"Set property index\" is out of range. It must be >= 0 && <= "
            << (GetLength(thread).GetArrayLength() - 1) << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    elements->Set(thread, index, value);
    return JSTaggedValue::Undefined();
}

bool JSAPIQueue::Has(const JSThread *thread, JSTaggedValue value) const
{
    uint32_t begin = GetCurrentFront();
    uint32_t end = GetCurrentTail();
    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    uint32_t capacity = elements->GetLength();

    uint32_t index = begin;
    while (index != end) {
        if (JSTaggedValue::SameValue(thread, elements->Get(thread, index), value)) {
            return true;
        }
        ASSERT(capacity != 0);
        index = (index + 1) % capacity;
    }
    return false;
}

JSHandle<TaggedArray> JSAPIQueue::OwnKeys(JSThread *thread, const JSHandle<JSAPIQueue> &obj)
{
    uint32_t length = obj->GetLength(thread).GetArrayLength();

    JSHandle<TaggedArray> oldElements(thread, obj->GetElements(thread));
    uint32_t oldCapacity = oldElements->GetLength();
    uint32_t newCapacity = ComputeCapacity(oldCapacity);
    uint32_t front = obj->GetFront();
    uint32_t tail = obj->GetTail();
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyQueue(oldElements, newCapacity, front, tail);
    obj->SetFront(0);
    obj->SetTail(length);
    obj->SetElements(thread, newElements);

    return JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>::Cast(obj));
}

JSHandle<TaggedArray> JSAPIQueue::OwnEnumKeys(JSThread *thread, const JSHandle<JSAPIQueue> &obj)
{
    uint32_t length = obj->GetLength(thread).GetArrayLength();

    JSHandle<TaggedArray> oldElements(thread, obj->GetElements(thread));
    ASSERT(!oldElements->IsDictionaryMode());
    uint32_t oldCapacity = oldElements->GetLength();
    uint32_t newCapacity = ComputeCapacity(oldCapacity);
    uint32_t front = obj->GetFront();
    uint32_t tail = obj->GetTail();
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyQueue(oldElements, newCapacity, front, tail);
    obj->SetFront(0);
    obj->SetTail(length);
    obj->SetElements(thread, newElements);

    return JSObject::GetOwnEnumPropertyKeys(thread, JSHandle<JSObject>::Cast(obj));
}

bool JSAPIQueue::GetOwnProperty(JSThread *thread, const JSHandle<JSAPIQueue> &obj,
                                const JSHandle<JSTaggedValue> &key)
{
    uint32_t index = 0;
    if (UNLIKELY(!JSTaggedValue::ToElementIndex(thread, key.GetTaggedValue(), &index))) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, key.GetTaggedValue());
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        CString errorMsg =
            "The type of \"index\" can not obtain attributes of no-number type. Received value is: "
            + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }

    uint32_t length = obj->GetLength(thread).GetArrayLength();
    if (length == 0) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, "Container is empty");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }
    if (index >= length) {
        ASSERT(length > 0);
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be > " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }

    obj->Get(thread, index);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    return true;
}

OperationResult JSAPIQueue::GetProperty(JSThread *thread, const JSHandle<JSAPIQueue> &obj,
                                        const JSHandle<JSTaggedValue> &key)
{
    int32_t length = static_cast<int32_t>(obj->GetSize(thread));
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

    return OperationResult(thread, obj->Get(thread, static_cast<uint32_t>(index)), PropertyMetaData(false));
}

bool JSAPIQueue::SetProperty(JSThread *thread, const JSHandle<JSAPIQueue> &obj,
                             const JSHandle<JSTaggedValue> &key,
                             const JSHandle<JSTaggedValue> &value)
{
    uint32_t length = obj->GetSize(thread);
    double index = key->GetNumber();
    if (index < 0 || static_cast<uint32_t>(index) >= length) {
        return false;
    }

    obj->Set(thread, static_cast<uint32_t>(index), value.GetTaggedValue());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    return true;
}

uint32_t JSAPIQueue::GetArrayLength(JSThread *thread, const JSHandle<JSAPIQueue> &queue)
{
    uint32_t begin = queue->GetCurrentFront();
    uint32_t end = queue->GetCurrentTail();
    JSHandle<TaggedArray> elements(thread, queue->GetElements(thread));
    ASSERT(!elements->IsDictionaryMode());
    uint32_t elementsSize = elements->GetLength();
    ASSERT(elementsSize != 0);
    uint32_t length = (end - begin + elementsSize) % elementsSize;
    return length;
}

uint32_t JSAPIQueue::GetNextPosition(JSThread *thread, uint32_t current)
{
    uint32_t next = 0;
    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    uint32_t elementsSize = elements->GetLength();
    ASSERT(elementsSize != 0);
    next = (current + 1) % elementsSize;
    return next;
}
} // namespace panda::ecmascript
