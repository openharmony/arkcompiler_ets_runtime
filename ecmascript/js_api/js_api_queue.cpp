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

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
void JSAPIQueue::Add(JSThread *thread, const JSHandle<JSAPIQueue> &queue, const JSHandle<JSTaggedValue> &value)
{
    uint32_t length = queue->GetLength().GetArrayLength();
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
    JSHandle<TaggedArray> oldElements(thread, obj->GetElements());
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
    if (queue->GetLength().GetArrayLength() < 1) {
        return JSTaggedValue::Undefined();
    }

    uint32_t index = queue->GetFront();

    JSHandle<TaggedArray> elements(thread, queue->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    return elements->Get(index);
}

JSTaggedValue JSAPIQueue::Pop(JSThread *thread, const JSHandle<JSAPIQueue> &queue)
{
    uint32_t length = queue->GetLength().GetArrayLength();
    if (length < 1) {
        return JSTaggedValue::Undefined();
    }

    JSHandle<TaggedArray> elements(thread, queue->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t front = queue->GetFront();

    JSTaggedValue value = elements->Get(front);
    queue->SetLength(thread, JSTaggedValue(length - 1));
    uint32_t elementsSize = elements->GetLength();
    ASSERT(elementsSize != 0);
    queue->SetFront((front + 1) % elementsSize);

    return value;
}

JSTaggedValue JSAPIQueue::Get(JSThread *thread, const uint32_t index)
{
    uint32_t length = GetSize();
    if (index >= length) {
        std::ostringstream oss;
        oss << "The value of \"Get property index\" is out of range. It must be >= 0 && <= "
            << (length - 1) << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t front = GetCurrentFront();
    ASSERT(capacity != 0);
    uint32_t curIndex = (front + index) % capacity;
    return elements->Get(curIndex);
}

JSTaggedValue JSAPIQueue::Set(JSThread *thread, const uint32_t index, JSTaggedValue value)
{
    if (index < 0 || index >= GetLength().GetArrayLength()) {
        std::ostringstream oss;
        oss << "The value of \"Set property index\" is out of range. It must be >= 0 && <= "
            << (GetLength().GetArrayLength() - 1) << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    elements->Set(thread, index, value);
    return JSTaggedValue::Undefined();
}

bool JSAPIQueue::Has(JSTaggedValue value) const
{
    uint32_t begin = GetCurrentFront();
    uint32_t end = GetCurrentTail();
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    uint32_t capacity = elements->GetLength();

    uint32_t index = begin;
    while (index != end) {
        if (JSTaggedValue::SameValue(elements->Get(index), value)) {
            return true;
        }
        ASSERT(capacity != 0);
        index = (index + 1) % capacity;
    }
    return false;
}

JSHandle<TaggedArray> JSAPIQueue::OwnKeys(JSThread *thread, const JSHandle<JSAPIQueue> &obj)
{
    uint32_t length = obj->GetLength().GetArrayLength();

    JSHandle<TaggedArray> oldElements(thread, obj->GetElements());
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

    return JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>::Cast(obj));
}

JSHandle<TaggedArray> JSAPIQueue::OwnEnumKeys(JSThread *thread, const JSHandle<JSAPIQueue> &obj)
{
    uint32_t length = obj->GetLength().GetArrayLength();

    JSHandle<TaggedArray> oldElements(thread, obj->GetElements());
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
    if (UNLIKELY(!JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, key.GetTaggedValue());
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        CString errorMsg =
            "The type of \"index\" can not obtain attributes of no-number type. Received value is: "
            + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }

    uint32_t length = obj->GetLength().GetArrayLength();
    if (index >= length) {
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
    int length = static_cast<int>(obj->GetLength().GetArrayLength());
    int index = key->GetInt();
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

bool JSAPIQueue::SetProperty(JSThread *thread, const JSHandle<JSAPIQueue> &obj,
                             const JSHandle<JSTaggedValue> &key,
                             const JSHandle<JSTaggedValue> &value)
{
    int length = static_cast<int>(obj->GetLength().GetArrayLength());
    int index = key->GetInt();
    if (index < 0 || index >= length) {
        return false;
    }

    obj->Set(thread, index, value.GetTaggedValue());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    return true;
}

uint32_t JSAPIQueue::GetArrayLength(JSThread *thread, const JSHandle<JSAPIQueue> &queue)
{
    uint32_t begin = queue->GetCurrentFront();
    uint32_t end = queue->GetCurrentTail();
    JSHandle<TaggedArray> elements(thread, queue->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t elementsSize = elements->GetLength();
    ASSERT(elementsSize != 0);
    uint32_t length = (end - begin + elementsSize) % elementsSize;
    return length;
}

uint32_t JSAPIQueue::GetNextPosition(uint32_t current)
{
    uint32_t next = 0;
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    uint32_t elementsSize = elements->GetLength();
    ASSERT(elementsSize != 0);
    next = (current + 1) % elementsSize;
    return next;
}
} // namespace panda::ecmascript
