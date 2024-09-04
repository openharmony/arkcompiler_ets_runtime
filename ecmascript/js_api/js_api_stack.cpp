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

#include "ecmascript/js_api/js_api_stack.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/object_factory-inl.h"

namespace panda::ecmascript {
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
bool JSAPIStack::Empty()
{
    if (this->GetTop() == -1) {
        return true;
    }
    return false;
}

JSTaggedValue JSAPIStack::Push(JSThread *thread, const JSHandle<JSAPIStack> &stack,
                               const JSHandle<JSTaggedValue> &value)
{
    int top = static_cast<int>(stack->GetTop());
    JSHandle<TaggedArray> elements = GrowCapacity(thread, stack, top + 1);

    ASSERT(!elements->IsDictionaryMode());
    elements->Set(thread, top + 1, value);
    stack->SetTop(++top);
    return value.GetTaggedValue();
}

JSTaggedValue JSAPIStack::Peek()
{
    int top = this->GetTop();
    if (top == -1) {
        return JSTaggedValue::Undefined();
    }

    TaggedArray *elements = TaggedArray::Cast(this->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    return elements->Get(top);
}

JSTaggedValue JSAPIStack::Pop()
{
    int top = this->GetTop();
    if (top == -1) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *elements = TaggedArray::Cast(this->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    this->SetTop(--top);
    return elements->Get(top + 1);
}

int JSAPIStack::Search(const JSHandle<JSTaggedValue> &value)
{
    int top = this->GetTop();
    TaggedArray *elements = TaggedArray::Cast(this->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    for (int i = 0; i <= top; i++) {
        if (value.GetTaggedValue() == elements->Get(i)) {
            return i;
        }
    }
    return -1;
}

JSHandle<TaggedArray> JSAPIStack::GrowCapacity(const JSThread *thread, const JSHandle<JSAPIStack> &obj,
                                               uint32_t capacity)
{
    JSHandle<TaggedArray> oldElements(thread, obj->GetElements());
    uint32_t oldLength = oldElements->GetLength();
    if (capacity < oldLength) {
        return oldElements;
    }
    uint32_t newCapacity = ComputeCapacity(capacity);
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->CopyArray(oldElements, oldLength, newCapacity);

    obj->SetElements(thread, newElements);
    return newElements;
}


JSTaggedValue JSAPIStack::Get(const uint32_t index)
{
    ASSERT(static_cast<int>(index) <= GetTop());
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    return elements->Get(index);
}

JSTaggedValue JSAPIStack::Set(JSThread *thread, const uint32_t index, JSTaggedValue value)
{
    uint32_t length = GetSize() + 1;
    if (index >= length) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    elements->Set(thread, index, value);
    return JSTaggedValue::Undefined();
}

bool JSAPIStack::Has(JSTaggedValue value) const
{
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    int top = static_cast<int>(GetTop());
    if (top == -1) {
        return false;
    }

    for (int i = 0; i < top + 1; i++) {
        if (JSTaggedValue::SameValue(elements->Get(i), value)) {
            return true;
        }
    }
    return false;
}

JSHandle<TaggedArray> JSAPIStack::OwnKeys(JSThread *thread, const JSHandle<JSAPIStack> &obj)
{
    return JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>::Cast(obj));
}

JSHandle<TaggedArray> JSAPIStack::OwnEnumKeys(JSThread *thread, const JSHandle<JSAPIStack> &obj)
{
    return JSObject::GetOwnEnumPropertyKeys(thread, JSHandle<JSObject>::Cast(obj));
}

bool JSAPIStack::GetOwnProperty(JSThread *thread, const JSHandle<JSAPIStack> &obj,
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

    uint32_t length = static_cast<uint32_t>(obj->GetTop() + 1);
    if (index >= length) {
        std::ostringstream oss;
        ASSERT(length > 0);
        oss << "The value of \"index\" is out of range. It must be > " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }

    obj->Get(index);
    return true;
}

OperationResult JSAPIStack::GetProperty(JSThread *thread, const JSHandle<JSAPIStack> &obj,
                                        const JSHandle<JSTaggedValue> &key)
{
    int length = obj->GetTop() + 1;
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

    return OperationResult(thread, obj->Get(index), PropertyMetaData(false));
}

bool JSAPIStack::SetProperty(JSThread *thread, const JSHandle<JSAPIStack> &obj,
                             const JSHandle<JSTaggedValue> &key,
                             const JSHandle<JSTaggedValue> &value)
{
    int length = obj->GetTop() + 1;
    int index = key->GetInt();
    if (index < 0 || index >= length) {
        return false;
    }

    obj->Set(thread, index, value.GetTaggedValue());
    return true;
}
}  // namespace panda::ecmascript
