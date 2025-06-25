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

#include "ecmascript/js_stable_array.h"
#include "ecmascript/base/config.h"
#include "ecmascript/base/sort_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"

namespace panda::ecmascript {
using TypedArrayHelper = base::TypedArrayHelper;
using TypedArrayKind = base::TypedArrayKind;
using BuiltinsArrayBuffer = builtins::BuiltinsArrayBuffer;
using BuiltinsSendableArrayBuffer = builtins::BuiltinsSendableArrayBuffer;
template<TypedArrayKind typedArrayKind>
using BuiltinsArrayBufferType = base::BuiltinsArrayBufferType<typedArrayKind>;

JSTaggedValue JSStableArray::Push(JSHandle<JSSharedArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t argc = argv->GetArgsNumber();
    uint32_t oldLength = receiver->GetArrayLength();
    uint32_t newLength = argc + oldLength;
    JSHandle<JSObject> thisObjHandle(receiver);

    if (newLength > ElementAccessor::GetElementsLength(thisObjHandle)) {
        JSObject::GrowElementsCapacity(thread, JSHandle<JSObject>::Cast(receiver), newLength, true);
    }
    bool needTransition = true;
    for (uint32_t k = 0; k < argc; k++) {
        JSHandle<JSTaggedValue> value = argv->GetCallArg(k);
        ElementAccessor::Set(thread, thisObjHandle, oldLength + k, value, needTransition);
    }
    receiver->SetArrayLength(thread, newLength);

    return JSTaggedValue(newLength);
}

JSTaggedValue JSStableArray::Push(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t argc = argv->GetArgsNumber();
    uint32_t oldLength = receiver->GetArrayLength();
    uint32_t newLength = argc + oldLength;
    JSHandle<JSObject> thisObjHandle(receiver);

    if (newLength > ElementAccessor::GetElementsLength(thisObjHandle)) {
        JSObject::GrowElementsCapacity(thread, JSHandle<JSObject>::Cast(receiver), newLength, true);
    }
    bool needTransition = true;
    for (uint32_t k = 0; k < argc; k++) {
        JSHandle<JSTaggedValue> value = argv->GetCallArg(k);
        ElementAccessor::Set(thread, thisObjHandle, oldLength + k, value, needTransition);
    }
    receiver->SetArrayLength(thread, newLength);

    return JSTaggedValue(newLength);
}

JSTaggedValue JSStableArray::Pop(JSHandle<JSSharedArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t length = receiver->GetArrayLength();
    if (length == 0) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
    JSSharedArray::CheckAndCopyArray(thread, receiver);
    JSHandle<JSObject> obj(receiver);
    uint32_t capacity = ElementAccessor::GetElementsLength(obj);
    uint32_t index = length - 1;
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Hole());
    if (index < capacity) {
        result.Update(ElementAccessor::Get(thread, obj, index));
    }
    if (!result->IsHole()) {
        if (TaggedArray::ShouldTrim(capacity, index)) {
            TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
            elements->Trim(thread, index);
        } else {
            ElementAccessor::Set(thread, obj, index, holeHandle, false);
        }
    } else {
        JSHandle<JSTaggedValue> thisObjVal(receiver);
        result.Update(JSArray::FastGetPropertyByValue(thread, thisObjVal, index).GetTaggedValue());
    }
    receiver->SetArrayLength(thread, index);
    return result->IsHole() ? JSTaggedValue::Undefined() : result.GetTaggedValue();
}

JSTaggedValue JSStableArray::Pop(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t length = receiver->GetArrayLength();
    if (length == 0) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
    JSArray::CheckAndCopyArray(thread, receiver);
    JSHandle<JSObject> obj(receiver);
    uint32_t capacity = ElementAccessor::GetElementsLength(obj);
    uint32_t index = length - 1;
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Hole());
    if (index < capacity) {
        result.Update(ElementAccessor::Get(thread, obj, index));
    }
    if (!result->IsHole()) {
        if (TaggedArray::ShouldTrim(capacity, index)) {
            TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
            elements->Trim(thread, index);
        } else {
            ElementAccessor::Set(thread, obj, index, holeHandle, false);
        }
    } else {
        JSHandle<JSTaggedValue> thisObjVal(receiver);
        result.Update(JSArray::FastGetPropertyByValue(thread, thisObjVal, index).GetTaggedValue());
    }
    receiver->SetArrayLength(thread, index);
    return result->IsHole() ? JSTaggedValue::Undefined() : result.GetTaggedValue();
}

void JSStableArray::HandleArray(JSHandle<JSObject> &newArrayHandle, uint32_t &actualDeleteCount,
                                JSThread *thread, uint32_t &start, JSHandle<JSObject> &thisObjHandle,
                                JSHandle<JSTaggedValue> &holeHandle)
{
    TaggedArray *destElements = TaggedArray::Cast(newArrayHandle->GetElements().GetTaggedObject());
        if (actualDeleteCount > ElementAccessor::GetElementsLength(newArrayHandle)) {
            destElements = *JSObject::GrowElementsCapacity(thread, newArrayHandle, actualDeleteCount);
        }

        for (uint32_t idx = 0; idx < actualDeleteCount; idx++) {
            if ((start + idx) >= ElementAccessor::GetElementsLength(thisObjHandle)) {
                ElementAccessor::Set(thread, newArrayHandle, idx, holeHandle, true);
            } else {
                JSHandle<JSTaggedValue> valueHandle(thread, ElementAccessor::Get(thread, thisObjHandle, start + idx));
                ElementAccessor::Set(thread, newArrayHandle, idx, valueHandle, true);
            }
        }
        JSHandle<JSArray>::Cast(newArrayHandle)->SetArrayLength(thread, actualDeleteCount);
}

JSTaggedValue JSStableArray::UpdateArrayCapacity(JSHandle<JSObject> &thisObjHandle, uint32_t &len,
                                                 uint32_t &insertCount, uint32_t &actualDeleteCount,
                                                 JSHandle<JSArray> &receiver, uint32_t &start,
                                                 JSThread *thread, bool &needTransition,
                                                 JSHandle<JSTaggedValue> &holeHandle,
                                                 EcmaRuntimeCallInfo *argv, JSHandle<JSTaggedValue> &thisObjVal,
                                                 JSHandle<JSTaggedValue> &lengthKey)
{
    uint32_t oldCapacity = ElementAccessor::GetElementsLength(thisObjHandle);
    ASSERT(len + insertCount >= actualDeleteCount);
    uint32_t newCapacity = len - actualDeleteCount + insertCount;
    TaggedArray *srcElements = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    JSMutableHandle<TaggedArray> srcElementsHandle(thread, srcElements);
    uint32_t argc = argv->GetArgsNumber();
    if (newCapacity > oldCapacity) {
        srcElementsHandle.Update(JSObject::GrowElementsCapacity(thread, thisObjHandle, newCapacity));
    }
    if (insertCount < actualDeleteCount) {
        JSArray::CheckAndCopyArray(thread, receiver);
        srcElementsHandle.Update(receiver->GetElements());
        for (uint32_t idx = start; idx < len - actualDeleteCount; idx++) {
            JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Hole());
            if ((idx + actualDeleteCount) < ElementAccessor::GetElementsLength(thisObjHandle)) {
                element.Update(ElementAccessor::Get(thread, thisObjHandle, idx + actualDeleteCount));
            }
            if ((idx + insertCount) < ElementAccessor::GetElementsLength(thisObjHandle)) {
                ElementAccessor::Set(thread, thisObjHandle, idx + insertCount, element, needTransition);
            }
        }

        if ((oldCapacity > newCapacity) && TaggedArray::ShouldTrim(oldCapacity, newCapacity)) {
            srcElementsHandle->Trim(thread, newCapacity);
        } else {
            for (uint32_t idx = newCapacity; idx < len; idx++) {
                if (idx < ElementAccessor::GetElementsLength(thisObjHandle)) {
                    ElementAccessor::Set(thread, thisObjHandle, idx, holeHandle, needTransition);
                }
            }
        }
    } else {
        ASSERT(len >= actualDeleteCount);
        for (uint32_t idx = len - actualDeleteCount; idx > start; idx--) {
            JSHandle<JSTaggedValue> element(thread,
                ElementAccessor::Get(thread, thisObjHandle, idx + actualDeleteCount - 1));
            ElementAccessor::Set(thread, thisObjHandle, idx + insertCount - 1, element, needTransition);
        }
    }

    for (uint32_t i = 2, idx = start; i < argc; i++, idx++) {
        ElementAccessor::Set(thread, thisObjHandle, idx, argv->GetCallArg(i), needTransition);
    }

    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(newCapacity));
    JSTaggedValue::SetProperty(thread, thisObjVal, lengthKey, newLenHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSStableArray::Splice(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv,
                                    uint32_t start, uint32_t insertCount, uint32_t actualDeleteCount,
                                    JSHandle<JSObject> newArrayHandle, uint32_t len)
{
    JSThread *thread = argv->GetThread();

    JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
    JSHandle<JSObject> thisObjHandle(receiver);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSArray::CheckAndCopyArray(thread, receiver);
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    bool needTransition = true;
    if (newArrayHandle.GetTaggedValue().IsStableJSArray(thread)) {
        HandleArray(newArrayHandle, actualDeleteCount, thread, start, thisObjHandle, holeHandle);
    } else {
        JSMutableHandle<JSTaggedValue> fromKey(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
        uint32_t k = 0;
        while (k < actualDeleteCount) {
            uint32_t from = start + k;
            fromKey.Update(JSTaggedValue(from));
            bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, fromKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (exists) {
                JSHandle<JSTaggedValue> fromValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, fromKey);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                toKey.Update(JSTaggedValue(k));
                if (newArrayHandle->IsJSProxy()) {
                    toKey.Update(JSTaggedValue::ToString(thread, toKey).GetTaggedValue());
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                }
                JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, toKey, fromValue);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            k++;
        }

        JSHandle<JSTaggedValue> deleteCount(thread, JSTaggedValue(actualDeleteCount));
        JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(newArrayHandle), lengthKey, deleteCount,
                                   true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    UpdateArrayCapacity(thisObjHandle, len, insertCount, actualDeleteCount, receiver, start,
                        thread, needTransition, holeHandle, argv, thisObjVal, lengthKey);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newArrayHandle.GetTaggedValue();
}

JSTaggedValue JSStableArray::Splice(JSHandle<JSSharedArray> receiver, EcmaRuntimeCallInfo *argv,
                                    uint32_t start, uint32_t insertCount, uint32_t actualDeleteCount,
                                    JSHandle<JSObject> newArrayHandle, uint32_t len)
{
    JSThread *thread = argv->GetThread();
    uint32_t argc = argv->GetArgsNumber();

    JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
    JSHandle<JSObject> thisObjHandle(receiver);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSSharedArray::CheckAndCopyArray(thread, receiver);
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    TaggedArray *srcElements = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    JSMutableHandle<TaggedArray> srcElementsHandle(thread, srcElements);
    bool needTransition = true;
    if (newArrayHandle.GetTaggedValue().IsStableJSArray(thread)) {
        TaggedArray *destElements = TaggedArray::Cast(newArrayHandle->GetElements().GetTaggedObject());
        if (actualDeleteCount > ElementAccessor::GetElementsLength(newArrayHandle)) {
            destElements = *JSObject::GrowElementsCapacity(thread, newArrayHandle, actualDeleteCount);
        }

        for (uint32_t idx = 0; idx < actualDeleteCount; idx++) {
            if ((start + idx) >= ElementAccessor::GetElementsLength(thisObjHandle)) {
                ElementAccessor::Set(thread, newArrayHandle, idx, holeHandle, needTransition);
            } else {
                JSHandle<JSTaggedValue> valueHandle(thread, ElementAccessor::Get(thread, thisObjHandle, start + idx));
                ElementAccessor::Set(thread, newArrayHandle, idx, valueHandle, needTransition);
            }
        }
        JSHandle<JSSharedArray>::Cast(newArrayHandle)->SetArrayLength(thread, actualDeleteCount);
    } else {
        JSMutableHandle<JSTaggedValue> fromKey(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
        uint32_t k = 0;
        while (k < actualDeleteCount) {
            uint32_t from = start + k;
            fromKey.Update(JSTaggedValue(from));
            bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, fromKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (exists) {
                JSHandle<JSTaggedValue> fromValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, fromKey);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                toKey.Update(JSTaggedValue(k));
                if (newArrayHandle->IsJSProxy()) {
                    toKey.Update(JSTaggedValue::ToString(thread, toKey).GetTaggedValue());
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                }
                JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, toKey, fromValue);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            k++;
        }

        JSHandle<JSTaggedValue> deleteCount(thread, JSTaggedValue(actualDeleteCount));
        JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(newArrayHandle), lengthKey, deleteCount,
                                   true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    uint32_t oldCapacity = ElementAccessor::GetElementsLength(thisObjHandle);
    ASSERT(len + insertCount >= actualDeleteCount);
    uint32_t newCapacity = len - actualDeleteCount + insertCount;
    if (newCapacity > oldCapacity) {
        srcElementsHandle.Update(JSObject::GrowElementsCapacity(thread, thisObjHandle, newCapacity));
    }
    if (insertCount < actualDeleteCount) {
        JSSharedArray::CheckAndCopyArray(thread, receiver);
        srcElementsHandle.Update(receiver->GetElements());
        for (uint32_t idx = start; idx < len - actualDeleteCount; idx++) {
            JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Hole());
            if ((idx + actualDeleteCount) < ElementAccessor::GetElementsLength(thisObjHandle)) {
                element.Update(ElementAccessor::Get(thread, thisObjHandle, idx + actualDeleteCount));
            }
            if ((idx + insertCount) < ElementAccessor::GetElementsLength(thisObjHandle)) {
                ElementAccessor::Set(thread, thisObjHandle, idx + insertCount, element, needTransition);
            }
        }

        if ((oldCapacity > newCapacity) && TaggedArray::ShouldTrim(oldCapacity, newCapacity)) {
            srcElementsHandle->Trim(thread, newCapacity);
        } else {
            for (uint32_t idx = newCapacity; idx < len; idx++) {
                if (idx < ElementAccessor::GetElementsLength(thisObjHandle)) {
                    ElementAccessor::Set(thread, thisObjHandle, idx, holeHandle, needTransition);
                }
            }
        }
    } else {
        ASSERT(len >= actualDeleteCount);
        for (uint32_t idx = len - actualDeleteCount; idx > start; idx--) {
            JSHandle<JSTaggedValue> element(thread,
                ElementAccessor::Get(thread, thisObjHandle, idx + actualDeleteCount - 1));
            ElementAccessor::Set(thread, thisObjHandle, idx + insertCount - 1, element, needTransition);
        }
    }

    for (uint32_t i = 2, idx = start; i < argc; i++, idx++) {
        ElementAccessor::Set(thread, thisObjHandle, idx, argv->GetCallArg(i), needTransition);
    }

    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(newCapacity));
    JSTaggedValue::SetProperty(thread, thisObjVal, lengthKey, newLenHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newArrayHandle.GetTaggedValue();
}

JSTaggedValue JSStableArray::Shift(JSHandle<JSSharedArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    JSHandle<JSObject> thisObjHandle(receiver);
    JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
    uint32_t length = receiver->GetArrayLength();
    if (length == 0) {
        return JSTaggedValue::Undefined();
    }
    JSSharedArray::CheckAndCopyArray(thread, receiver);
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    JSHandle<JSTaggedValue> result(thread, ElementAccessor::Get(thread, thisObjHandle, 0));
    bool needTransition = false;
    for (uint32_t k = 1; k < length; k++) {
        JSHandle<JSTaggedValue> kValue(thread, ElementAccessor::Get(thread, thisObjHandle, k));
        ElementAccessor::Set(thread, thisObjHandle, k - 1, kValue, needTransition);
    }
    uint32_t capacity = ElementAccessor::GetElementsLength(thisObjHandle);
    uint32_t index = length - 1;
    if (TaggedArray::ShouldTrim(capacity, index)) {
        elements->Trim(thread, index);
    } else {
        ElementAccessor::Set(thread, thisObjHandle, index, holeHandle, needTransition);
    }
    receiver->SetArrayLength(thread, index);
    return result->IsHole() ? JSTaggedValue::Undefined() : result.GetTaggedValue();
}

JSTaggedValue JSStableArray::Shift(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    JSHandle<JSObject> thisObjHandle(receiver);
    JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
    uint32_t length = receiver->GetArrayLength();
    if (length == 0) {
        return JSTaggedValue::Undefined();
    }
    JSArray::CheckAndCopyArray(thread, receiver);
    JSHandle<TaggedArray> elements(thread, TaggedArray::Cast(receiver->GetElements().GetTaggedObject()));
    JSHandle<JSTaggedValue> result(thread, ElementAccessor::Get(thread, thisObjHandle, 0));
    bool needTransition = false;
    for (uint32_t k = 1; k < length; k++) {
        JSHandle<JSTaggedValue> kValue(thread, ElementAccessor::Get(thread, thisObjHandle, k));
        ElementAccessor::Set(thread, thisObjHandle, k - 1, kValue, needTransition);
    }
    uint32_t capacity = ElementAccessor::GetElementsLength(thisObjHandle);
    uint32_t index = length - 1;
    if (TaggedArray::ShouldTrim(capacity, index)) {
        elements->Trim(thread, index);
    } else {
        ElementAccessor::Set(thread, thisObjHandle, index, holeHandle, needTransition);
    }
    receiver->SetArrayLength(thread, index);
    return result->IsHole() ? JSTaggedValue::Undefined() : result.GetTaggedValue();
}

#if ENABLE_NEXT_OPTIMIZATION
bool JSStableArray::WorthUseTreeString(uint32_t sepLength, size_t allocateLength, uint32_t len)
{
    if (allocateLength >= TREE_STRING_THRESHOLD) {
        // if sepLength is 0, means all the elements in treeString is len -1;
        // otherwise, the num of elements is (len-1)(string in vector) + (len -1)(num of seps)
        size_t treeStringElementNum = (sepLength == 0) ? (len - 1) : (2 * (len - 1));

        if (treeStringElementNum * TreeString::SIZE <= allocateLength) {
            // heuristic: if tree string uses less memory than linestring, it is worth.
            // In other words, we hope tree string can work for the large strings join.
            return true;
        }
    }
    return false;
}

template <typename Container>
JSTaggedValue JSStableArray::JoinUseTreeString(const JSThread *thread, const JSHandle<JSTaggedValue> receiverValue,
                                               const JSHandle<EcmaString> sepStringHandle, uint32_t sepLength,
                                               Container &arrElements, uint32_t elemNum)
{
    // Do not concat the elements one by one, it will make the tree string unbalanced. Concat each element with its
    // right neighbor first level by level, then the tree string will be balanced as possible.
    if (sepLength != 0 && elemNum > 1) {
        for (uint32_t k = 0; k < elemNum - 1; k++) {
            arrElements[k] = JSHandle<EcmaString>(
                thread, EcmaStringAccessor::Concat(thread->GetEcmaVM(), arrElements[k], sepStringHandle));
            RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, receiverValue);
        }
    }

    while (elemNum > 1) {
        uint32_t newNum = (elemNum + 1) / NUM_2;
        for (uint32_t i = 0; i < elemNum / NUM_2; ++i) {
            arrElements[i] = JSHandle<EcmaString>(
                    thread,
                    EcmaStringAccessor::Concat(thread->GetEcmaVM(), arrElements[NUM_2 * i], arrElements[NUM_2 * i + 1])
                );
            RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, receiverValue);
        }
        if (elemNum % NUM_2 == 1) {
            arrElements[newNum - 1] = arrElements[elemNum - 1];
        }
        elemNum = newNum;
    }
    ArrayJoinStack::Pop(thread, receiverValue);
    return arrElements[0].GetTaggedValue();
}

template <typename Container>
void JSStableArray::ProcessElements(JSThread *thread, JSHandle<JSTaggedValue> receiverValue, uint32_t len,
                                    Container &arrElements, bool &isOneByte, uint64_t &allocateLength)
{
    JSMutableHandle<JSTaggedValue> elementHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSObject> obj(thread, receiverValue.GetTaggedValue());
    JSTaggedValue element = JSTaggedValue::Undefined();
    for (uint32_t k = 0; k < len; k++) {
        if (receiverValue->IsStableJSArray(thread) || receiverValue->IsJSSharedArray()) {
            element = k < ElementAccessor::GetElementsLength(obj) ?
                      ElementAccessor::Get(thread, obj, k) : JSTaggedValue::Hole();
        } else {
            element = JSArray::FastGetPropertyByValue(thread, receiverValue, k).GetTaggedValue();
            RETURN_IF_ABRUPT_COMPLETION(thread);
        }
        if (!element.IsUndefinedOrNull() && !element.IsHole()) {
            if (!element.IsString()) {
                elementHandle.Update(element);
                JSHandle<EcmaString> strElement = JSTaggedValue::ToString(thread, elementHandle);
                RETURN_IF_ABRUPT_COMPLETION(thread);
                element = strElement.GetTaggedValue();
            }
            auto nextStr = EcmaString::Cast(element.GetTaggedObject());
            arrElements[k] = JSHandle<EcmaString>(thread, nextStr);
            isOneByte = isOneByte & EcmaStringAccessor(nextStr).IsUtf8();
            allocateLength += EcmaStringAccessor(nextStr).GetLength();
        } else {
            arrElements[k] = JSHandle<EcmaString>(thread->GlobalConstants()->GetHandledEmptyString());
        }
    }
}

template <typename Container>
JSTaggedValue JSStableArray::DoStableArrayJoin(JSThread *thread, JSHandle<JSTaggedValue> receiverValue, uint32_t len,
                                               Container &arrElements, bool &isOneByte, uint32_t sep,
                                               uint32_t sepLength, JSHandle<EcmaString> sepStringHandle)
{
    uint64_t allocateLength = 0;
    ProcessElements(thread, receiverValue, len, arrElements, isOneByte, allocateLength);
    RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, receiverValue);

    if (len > 0) {
        allocateLength += static_cast<uint64_t>(sepLength) * (len - 1);
    }
    if (allocateLength > BaseString::MAX_STRING_LENGTH) {
        ArrayJoinStack::Pop(thread, receiverValue);
        THROW_RANGE_ERROR_AND_RETURN(thread, "Invalid string length", JSTaggedValue::Exception());
    }
    if (WorthUseTreeString(sepLength, allocateLength, len)) {
        return JoinUseTreeString(thread, receiverValue, sepStringHandle, sepLength, arrElements, len);
    }

    // 5. Let R be the empty String.
    auto newString =
        EcmaStringAccessor::CreateLineString(thread->GetEcmaVM(), static_cast<size_t>(allocateLength), isOneByte);
    int current = 0;
    {
        DISALLOW_GARBAGE_COLLECTION;
        // 6. Repeat, while k < len
        for (uint32_t k = 0; k < len; k++) {
            // a. If k > 0, set R to the string-concatenation of R and sep.
            if (k > 0) {
                if (sepLength == 1) {
                    EcmaStringAccessor(newString).Set(current, static_cast<uint16_t>(sep));
                } else if (sepLength > 1) {
                    EcmaStringAccessor::ReadData(newString, *sepStringHandle, current,
                                                 allocateLength - static_cast<uint32_t>(current), sepLength);
                }
                current += static_cast<int>(sepLength);
            }
            // b. Let element be ? Get(O, ToString(𝔽(k))).
            JSHandle<EcmaString> nextStr = arrElements[k];

            // c. Set R to the string-concatenation of R and S
            int nextLength = static_cast<int>(EcmaStringAccessor(nextStr).GetLength());
            EcmaStringAccessor::ReadData(newString, *nextStr, current, allocateLength - static_cast<uint32_t>(current),
                                         nextLength);
            current += nextLength;
        }
    }
    ASSERT_PRINT(isOneByte == EcmaStringAccessor::CanBeCompressed(newString),
                 "isOneByte does not match the real value!");
    JSHandle<JSTaggedValue> stringValue(thread, newString);
    ArrayJoinStack::Pop(thread, receiverValue);
    // return R
    return stringValue.GetTaggedValue();
}

JSTaggedValue JSStableArray::Join(JSHandle<JSTaggedValue> receiverValue, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // 1. Let O be ToObject(this.value)
    JSHandle<JSObject> obj(thread, receiverValue.GetTaggedValue());

    // 2. Let len be ToLength(Get(O, "length"))
    int64_t len = base::ArrayHelper::GetArrayLength(thread, receiverValue);

    int sep = ',';
    uint32_t sepLength = 1;
    JSHandle<JSTaggedValue> sepHandle = base::BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<EcmaString> sepStringHandle;
    if (sepHandle->IsUndefined()) {
        // 3. If separator is undefined, let sep be ",".
        sepHandle = globalConst->GetHandledCommaString();
        sepStringHandle = JSTaggedValue::ToString(thread, sepHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        // 4. Else, let sep be ? ToString(separator).
        sepStringHandle = JSTaggedValue::ToString(thread, sepHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        sepLength = EcmaStringAccessor(sepStringHandle).GetLength();
        if (sepLength == 1) {
            sep = EcmaStringAccessor(sepStringHandle).Get<false>(0);
        }
    }

    bool isOneByte = EcmaStringAccessor(sepStringHandle).IsUtf8();

    // Fastpath should put after parsing "sep". Error may occur if sep cannot be transformed to string,
    // which should be handled before fastpath return.
    if (len == 0 || !ArrayJoinStack::Push(thread, receiverValue)) {
        return globalConst->GetEmptyString();
    }

    if (len == 1) {
        // sep unused, set isOneByte to default(true)
        isOneByte = true;
    }

    // Use stack memory if the number of elements is less than USE_STACK_MEMORY_THRESHOLD.
    // arr can be faster then vector.
    if (len <= USE_STACK_MEMORY_THRESHOLD) {
        std::array<JSHandle<EcmaString>, USE_STACK_MEMORY_THRESHOLD> arr;
        return DoStableArrayJoin(thread, receiverValue, len, arr, isOneByte, sep, sepLength, sepStringHandle);
    } else {
        CVector<JSHandle<EcmaString>> vec(len);
        return DoStableArrayJoin(thread, receiverValue, len, vec, isOneByte, sep, sepLength, sepStringHandle);
    }
}
#endif

#if !ENABLE_NEXT_OPTIMIZATION
void JSStableArray::SetSepValue(JSHandle<EcmaString> sepStringHandle, int &sep, uint32_t &sepLength)
{
    if (EcmaStringAccessor(sepStringHandle).IsUtf8() && EcmaStringAccessor(sepStringHandle).GetLength() == 1) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        sep = EcmaStringAccessor(sepStringHandle).Get(0);
    } else if (EcmaStringAccessor(sepStringHandle).GetLength() == 0) {
        sep = JSStableArray::SeparatorFlag::MINUS_TWO;
        sepLength = 0;
    } else {
        sep = JSStableArray::SeparatorFlag::MINUS_ONE;
        sepLength = EcmaStringAccessor(sepStringHandle).GetLength();
    }
}

bool JSStableArray::WorthUseTreeString(int sep, size_t allocateLength, uint32_t len)
{
    if (allocateLength >= TREE_STRING_THRESHOLD) {
        size_t treeStringElementNum = (sep == MINUS_TWO) ? (len - 1) : (2 * (len - 1));
        // if sep is MINUS_TWO, means all the elements in treeString is len -1;
        // otherwise, the num of elements is (len-1)(string in vector) + (len -1)(num of seps)
        if (treeStringElementNum * TreeString::SIZE <= allocateLength) {
            // heuristic: if tree string uses less memory than linestring, it is worth.
            // In other words, we hope tree string can work for the large strings join.
            return true;
        }
    }
    return false;
}

JSTaggedValue JSStableArray::JoinUseTreeString(const JSThread *thread,
                                               const JSHandle<JSTaggedValue> receiverValue,
                                               const JSHandle<EcmaString> sepStringHandle, const int sep,
                                               CVector<JSHandle<EcmaString>> &vec)
{
    // Do not concat the elements one by one, it will make the tree string unbalanced. Concat each element with its
    // right neighbor first level by level, then the tree string will be balanced as possible.
    if (sep != JSStableArray::SeparatorFlag::MINUS_TWO) {
        auto last = std::prev(vec.end());
        for (auto iter = vec.begin(); iter != last; ++iter) {
            *iter =
                JSHandle<EcmaString>(thread, EcmaStringAccessor::Concat(thread->GetEcmaVM(), *iter, sepStringHandle));
            RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, receiverValue);
        }
    }
    size_t elemNum = vec.size();
    while (elemNum > 1) {
        size_t newNum = (elemNum + 1) / NUM_2;
        for (size_t i = 0; i < elemNum / NUM_2; ++i) {
            vec[i] = JSHandle<EcmaString>(
                thread, EcmaStringAccessor::Concat(thread->GetEcmaVM(), vec[NUM_2 * i], vec[NUM_2 * i + 1]));
            RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, receiverValue);
        }
        if (elemNum % NUM_2 == 1) {
            vec[newNum - 1] = vec[elemNum - 1];
        }
        elemNum = newNum;
    }
    ArrayJoinStack::Pop(thread, receiverValue);
    return vec.front().GetTaggedValue();
}

JSTaggedValue JSStableArray::Join(JSHandle<JSTaggedValue> receiverValue, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t length = base::ArrayHelper::GetArrayLength(thread, receiverValue);
    JSHandle<JSTaggedValue> sepHandle = base::BuiltinsBase::GetCallArg(argv, 0);
    int sep = ',';
    uint32_t sepLength = 1;
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<EcmaString> sepStringHandle = JSHandle<EcmaString>::Cast(globalConst->GetHandledCommaString());
    if (!sepHandle->IsUndefined()) {
        if (sepHandle->IsString()) {
            sepStringHandle = JSHandle<EcmaString>::Cast(sepHandle);
        } else {
            sepStringHandle = JSTaggedValue::ToString(thread, sepHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        SetSepValue(sepStringHandle, sep, sepLength);
    }
    if (length == 0 || !ArrayJoinStack::Push(thread, receiverValue)) {
        return globalConst->GetEmptyString();
    }
    JSHandle<JSObject> obj(thread, receiverValue.GetTaggedValue());
    uint64_t allocateLength = 0;
    bool isOneByte = (sep != JSStableArray::SeparatorFlag::MINUS_ONE) || EcmaStringAccessor(sepStringHandle).IsUtf8();
    JSMutableHandle<JSTaggedValue> elementHandle(thread, JSTaggedValue::Undefined());
    uint32_t elementsLength = ElementAccessor::GetElementsLength(obj);
    uint32_t len = elementsLength > length ? length : elementsLength;
    if (elementsLength == 0 && length != 0) {
        len = length;
    }
    if (len <= 1) {
        // sep unused, set isOneByte to default(true)
        isOneByte = true;
    }
    CVector<JSHandle<EcmaString>> vec;
    vec.reserve(len);
    JSTaggedValue element = JSTaggedValue::Hole();
    for (uint32_t k = 0; k < len; k++) {
        if (receiverValue->IsStableJSArray(thread)) {
            element = k < ElementAccessor::GetElementsLength(obj) ?
                      ElementAccessor::Get(thread, obj, k) : JSTaggedValue::Hole();
        } else {
            element = JSArray::FastGetPropertyByValue(thread, receiverValue, k).GetTaggedValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        if (!element.IsUndefinedOrNull() && !element.IsHole()) {
            if (!element.IsString()) {
                elementHandle.Update(element);
                JSHandle<EcmaString> strElement = JSTaggedValue::ToString(thread, elementHandle);
                RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, receiverValue);
                element = strElement.GetTaggedValue();
            }
            auto nextStr = EcmaString::Cast(element.GetTaggedObject());
            vec.emplace_back(thread, nextStr);
            isOneByte = EcmaStringAccessor(nextStr).IsUtf8() ? isOneByte : false;
            allocateLength += EcmaStringAccessor(nextStr).GetLength();
        } else {
            vec.emplace_back(globalConst->GetHandledEmptyString());
        }
    }
    if (len > 0) {
        allocateLength += static_cast<uint64_t>(sepLength) * (len - 1);
    }
    if (allocateLength > BaseString::MAX_STRING_LENGTH) {
        ArrayJoinStack::Pop(thread, receiverValue);
        THROW_RANGE_ERROR_AND_RETURN(thread, "Invalid string length", JSTaggedValue::Exception());
    }
    if (WorthUseTreeString(sep, allocateLength, len)) {
        return JoinUseTreeString(thread, receiverValue, sepStringHandle, sep, vec);
    }
    auto newString =
        EcmaStringAccessor::CreateLineString(thread->GetEcmaVM(), static_cast<size_t>(allocateLength), isOneByte);
    int current = 0;
    {
        DISALLOW_GARBAGE_COLLECTION;
        for (uint32_t k = 0; k < len; k++) {
            if (k > 0) {
                if (sep >= 0) {
                    EcmaStringAccessor(newString).Set(current, static_cast<uint16_t>(sep));
                } else if (sep != JSStableArray::SeparatorFlag::MINUS_TWO) {
                    EcmaStringAccessor::ReadData(newString, *sepStringHandle, current,
                                                 allocateLength - static_cast<uint32_t>(current), sepLength);
                }
                current += static_cast<int>(sepLength);
            }
            JSHandle<EcmaString> nextStr = vec[k];
            int nextLength = static_cast<int>(EcmaStringAccessor(nextStr).GetLength());
            EcmaStringAccessor::ReadData(newString, *nextStr, current, allocateLength - static_cast<uint32_t>(current),
                                         nextLength);
            current += nextLength;
        }
    }
    ASSERT_PRINT(isOneByte == EcmaStringAccessor::CanBeCompressed(newString),
                 "isOneByte does not match the real value!");
    JSHandle<JSTaggedValue> stringValue(thread, newString);
    ArrayJoinStack::Pop(thread, receiverValue);
    return stringValue.GetTaggedValue();
}
#endif

JSTaggedValue JSStableArray::HandleFindIndexOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                     JSHandle<JSTaggedValue> callbackFnHandle,
                                                     JSHandle<JSTaggedValue> thisArgHandle, uint32_t &k)
{
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint64_t len = static_cast<uint64_t>(base::ArrayHelper::GetArrayLength(thread, thisObjVal));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue callResult = base::BuiltinsBase::GetTaggedBoolean(false);
    const int32_t argsLength = 3; // 3: ?kValue, k, O?
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    while (k < len) {
        // Elements of thisObjHandle may change.
        JSTaggedValue val = ElementAccessor::Get(thread, thisObjHandle, k);
        if (val.IsHole()) {
            auto res = JSArray::FastGetPropertyByValue(thread, thisObjVal, k).GetTaggedValue();
            if (res.IsHole()) {
                kValue.Update(JSTaggedValue::Undefined());
            } else {
                kValue.Update(res);
            }
        } else {
            kValue.Update(val);
        }
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), JSTaggedValue(k), thisObjVal.GetTaggedValue());
        callResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, callResult);
        if (callResult.ToBoolean()) {
            return callResult;
        }
        if (ElementAccessor::GetElementsLength(thisObjHandle) < len) {
            len = ElementAccessor::GetElementsLength(thisObjHandle);
        }
        k++;
        if (!thisObjVal->IsStableJSArray(thread)) {
            return callResult;
        }
    }
    return callResult;
}

JSTaggedValue JSStableArray::HandleFindLastIndexOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                         JSHandle<JSTaggedValue> callbackFnHandle,
                                                         JSHandle<JSTaggedValue> thisArgHandle, int64_t &k)
{
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSTaggedValue callResult = base::BuiltinsBase::GetTaggedBoolean(false);
    const int32_t argsLength = 3; // 3: ?kValue, k, O?
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    while (k >= 0) {
        // Elements of thisObjHandle may change.
        JSTaggedValue val = ElementAccessor::Get(thread, thisObjHandle, k);
        if (val.IsHole()) {
            auto res = JSArray::FastGetPropertyByValue(thread, thisObjVal, k).GetTaggedValue();
            if (res.IsHole()) {
                kValue.Update(JSTaggedValue::Undefined());
            } else {
                kValue.Update(res);
            }
        } else {
            kValue.Update(val);
        }
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), JSTaggedValue(k), thisObjVal.GetTaggedValue());
        callResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, callResult);
        if (callResult.ToBoolean()) {
            return callResult;
        }
        k--;
        if (base::ArrayHelper::GetArrayLength(thread, thisObjVal) - 1 < k) {
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return callResult;
        }
        if (!thisObjVal->IsStableJSArray(thread)) {
            return callResult;
        }
    }
    return callResult;
}

JSTaggedValue JSStableArray::HandleEveryOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                 JSHandle<JSTaggedValue> callbackFnHandle,
                                                 JSHandle<JSTaggedValue> thisArgHandle, uint32_t &k)
{
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint64_t len = static_cast<uint64_t>(base::ArrayHelper::GetArrayLength(thread, thisObjVal));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    const int32_t argsLength = 3; // 3: ?kValue, k, O?
    JSTaggedValue callResult = base::BuiltinsBase::GetTaggedBoolean(true);
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    while (k < len) {
        // Elements of thisObjHandle may change.
        kValue.Update(ElementAccessor::Get(thread, thisObjHandle, k));
        if (!kValue.GetTaggedValue().IsHole()) {
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), JSTaggedValue(k), thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (ElementAccessor::GetElementsLength(thisObjHandle) < len) {
                len = ElementAccessor::GetElementsLength(thisObjHandle);
            }
        } else if (JSTaggedValue::HasProperty(thread, thisObjVal, k)) {
            JSHandle<JSTaggedValue> kValue1 = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue1.GetTaggedValue(), JSTaggedValue(k), thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        if (!callResult.ToBoolean()) {
            return base::BuiltinsBase::GetTaggedBoolean(false);
        }
        k++;
        if (!thisObjVal->IsStableJSArray(thread)) {
            return base::BuiltinsBase::GetTaggedBoolean(true);
        }
    }
    return base::BuiltinsBase::GetTaggedBoolean(true);
}

JSTaggedValue JSStableArray::HandleSomeOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                JSHandle<JSTaggedValue> callbackFnHandle,
                                                JSHandle<JSTaggedValue> thisArgHandle, uint32_t &k)
{
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint64_t len = static_cast<uint64_t>(base::ArrayHelper::GetArrayLength(thread, thisObjVal));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    const int32_t argsLength = 3; // 3: ?kValue, k, O?
    JSTaggedValue callResult = base::BuiltinsBase::GetTaggedBoolean(false);
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    while (k < len) {
        // Elements of thisObjHandle may change.
        kValue.Update(ElementAccessor::Get(thread, thisObjHandle, k));
        if (!kValue.GetTaggedValue().IsHole()) {
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), JSTaggedValue(k), thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else if (JSTaggedValue::HasProperty(thread, thisObjVal, k)) {
            JSHandle<JSTaggedValue> kValue1 = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue1.GetTaggedValue(), JSTaggedValue(k), thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        if (ElementAccessor::GetElementsLength(thisObjHandle) < len) {
            len = ElementAccessor::GetElementsLength(thisObjHandle);
        }
        if (callResult.ToBoolean()) {
            return base::BuiltinsBase::GetTaggedBoolean(true);
        }
        k++;
        if (!thisObjVal->IsStableJSArray(thread)) {
            return base::BuiltinsBase::GetTaggedBoolean(false);
        }
    }
    return base::BuiltinsBase::GetTaggedBoolean(false);
}

JSTaggedValue JSStableArray::HandleforEachOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                   JSHandle<JSTaggedValue> callbackFnHandle,
                                                   JSHandle<JSTaggedValue> thisArgHandle, uint32_t len, uint32_t &k)
{
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const int32_t argsLength = 3; // 3: ?kValue, k, O?
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    if (ElementAccessor::GetElementsLength(thisObjHandle) <= k) {
        return base::BuiltinsBase::GetTaggedBoolean(false);
    }
    while (k < len) {
        // Elements of thisObjHandle may change.
        kValue.Update(ElementAccessor::Get(thread, thisObjHandle, k));
        if (!kValue.GetTaggedValue().IsHole()) {
            key.Update(JSTaggedValue(k));
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            JSTaggedValue funcResult = JSFunction::Call(info);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
            if (ElementAccessor::GetElementsLength(thisObjHandle) < len) {
                len = ElementAccessor::GetElementsLength(thisObjHandle);
            }
        } else if (JSTaggedValue::HasProperty(thread, thisObjVal, k)) {
            key.Update(JSTaggedValue(k));
            JSHandle<JSTaggedValue> kValue1 = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue1.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            JSTaggedValue funcResult = JSFunction::Call(info);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        }
        k++;
        if (!thisObjVal->IsStableJSArray(thread)) {
            break;
        }
    }
    return base::BuiltinsBase::GetTaggedBoolean(true);
}

template <RBMode mode, class Predicate>
const JSTaggedType* JSStableArray::IndexOfElements(Span<const JSTaggedType> elements, IndexOfOptions options,
                                                   Predicate predicate)
{
    static_assert(std::is_invocable_r_v<bool, Predicate, JSTaggedType>, "Invalid call signature.");
    if (options.reversedOrder) {
        for (auto cur = elements.end() - 1; cur >= elements.begin(); --cur) {
            if (UNLIKELY(std::invoke(predicate, Barriers::GetTaggedValue<mode>(ToUintPtr(cur))))) {
                return cur;
            }
        }
    } else {
        for (auto cur = elements.begin(); cur < elements.end(); ++cur) {
            if (UNLIKELY(std::invoke(predicate, Barriers::GetTaggedValue<mode>(ToUintPtr(cur))))) {
                return cur;
            }
        }
    }
    return nullptr;
}

const JSTaggedType* JSStableArray::IndexOfUndefined(Span<const JSTaggedType> elements, IndexOfOptions options,
                                                    bool isMutant)
{
    // For mutant arrays, only raw int32, raw double and SPECIAL_HOLE may exist.
    if (isMutant) {
        if (!options.holeAsUndefined) {
            return nullptr;
        }
        return IndexOfElements(elements, options, [](JSTaggedType rawValue) {
            return rawValue == base::SPECIAL_HOLE;
        });
    }
    // For non-mutant arrays, taggedValue can never be SPECIAL_HOLE.
    if (!options.holeAsUndefined) {
        return IndexOfElements(elements, options, [](JSTaggedType taggedValue) {
            return JSTaggedValue(taggedValue).IsUndefined();
        });
    }
    return IndexOfElements(elements, options, [](JSTaggedType taggedValue) {
        return JSTaggedValue(taggedValue).IsHole() || JSTaggedValue(taggedValue).IsUndefined();
    });
}

const JSTaggedType* JSStableArray::IndexOfTaggedZero(Span<const JSTaggedType> elements, IndexOfOptions options)
{
    return IndexOfElements(elements, options, [](JSTaggedType taggedValue) {
        return JSTaggedValue(taggedValue).IsExactlyZero();
    });
}

// Raw int32 array (isMutant = true), or tagged array (isMutant = false)
const JSTaggedType* JSStableArray::IndexOfInt(Span<const JSTaggedType> elements, JSTaggedValue searchElement,
                                              IndexOfOptions options, bool isMutantInt32Array)
{
    ASSERT(!searchElement.IsUndefined());
    int32_t searchValue;
    if (searchElement.IsInt()) {
        searchValue = searchElement.GetInt();
    } else if (searchElement.WithinInt32(true)) {
        searchValue = static_cast<int32_t>(searchElement.GetDouble());
    } else {
        return nullptr;
    }
    if (isMutantInt32Array) {
        // For ElementsKind::INT: convertedValue = JSTaggedValue(static_cast<int>(rawValue))
        return IndexOfElements(elements, options, [searchValue](JSTaggedType rawValue) {
            return rawValue != base::SPECIAL_HOLE && searchValue == static_cast<int32_t>(rawValue);
        });
    }
    if (searchValue == 0) {
        return IndexOfTaggedZero(elements, options);
    }
    JSTaggedType taggedInt32 = JSTaggedValue(searchValue).GetRawData();
    JSTaggedType taggedDouble = JSTaggedValue(static_cast<double>(searchValue)).GetRawData();
    // Always false if taggedValue is not number
    return IndexOfElements(elements, options, [taggedInt32, taggedDouble](JSTaggedType taggedValue) {
        return taggedValue == taggedInt32 || taggedValue == taggedDouble;
    });
}

// Raw double array (isMutant = true), or tagged array (isMutant = false)
const JSTaggedType* JSStableArray::IndexOfDouble(Span<const JSTaggedType> elements, JSTaggedValue searchElement,
                                                 IndexOfOptions options, bool isMutantDoubleArray)
{
    ASSERT(!searchElement.IsUndefined());
    if (!searchElement.IsNumber()) {
        return nullptr;
    }
    double searchValue = searchElement.GetNumber();
    if (std::isnan(searchValue)) {
        if (options.compType != ComparisonType::SAME_VALUE_ZERO) {
            return nullptr;
        }
        if (isMutantDoubleArray) {
            // For ElementsKind::NUMBER: convertedValue = JSTaggedValue(base::bit_cast<double>(rawValue))
            return IndexOfElements(elements, options, [](JSTaggedType rawValue) {
                return rawValue != base::SPECIAL_HOLE && std::isnan(base::bit_cast<double>(rawValue));
            });
        }
        return IndexOfElements(elements, options, [](JSTaggedType taggedValue) {
            return JSTaggedValue(taggedValue).IsNaN();
        });
    }
    if (isMutantDoubleArray) {
        // Including the cases of +inf, -inf, +0.0 and -0.0
        // We assume that bit representation of searchValue can never be SPECIAL_HOLE (which is NaN)
        return IndexOfElements(elements, options, [searchValue](JSTaggedType rawValue) {
            return searchValue == base::bit_cast<double>(rawValue);
        });
    }
    if (searchValue == 0.0) {
        return IndexOfTaggedZero(elements, options);
    }
    JSTaggedType taggedDouble = JSTaggedValue(searchValue).GetRawData();
    if (JSTaggedValue(taggedDouble).WithinInt32()) {
        JSTaggedType taggedInt32 = JSTaggedValue(static_cast<int32_t>(searchValue)).GetRawData();
        return IndexOfElements(elements, options, [taggedDouble, taggedInt32](JSTaggedType taggedValue) {
            return taggedValue == taggedDouble || taggedValue == taggedInt32;
        });
    }
    return IndexOfElements(elements, options, [taggedDouble](JSTaggedType taggedValue) {
        return taggedValue == taggedDouble;
    });
}

const JSTaggedType* JSStableArray::IndexOfString(Span<const JSTaggedType> elements, JSTaggedValue searchElement,
                                                 IndexOfOptions options)
{
    ASSERT(!searchElement.IsUndefined());
    if (!searchElement.IsString()) {
        return nullptr;
    }
    if (g_isEnableCMCGC) {
        return IndexOfElements<RBMode::FAST_CMC_RB>(elements, options, [searchElement](JSTaggedType cur) {
            if (searchElement.GetRawData() == cur) {
                return true;
            }
            JSTaggedValue curValue(cur);
            if (!curValue.IsString()) {
                return false;
            }
            return JSTaggedValue::StringCompare<RBMode::FAST_CMC_RB>(
                EcmaString::Cast(curValue.GetTaggedObject()),
                EcmaString::Cast(searchElement.GetTaggedObject()));
        });
    } else {
        return IndexOfElements<RBMode::FAST_NO_RB>(elements, options, [searchElement](JSTaggedType cur) {
            if (searchElement.GetRawData() == cur) {
                return true;
            }
            JSTaggedValue curValue(cur);
            if (!curValue.IsString()) {
                return false;
            }
            return JSTaggedValue::StringCompare<RBMode::FAST_NO_RB>(
                EcmaString::Cast(curValue.GetTaggedObject()),
                EcmaString::Cast(searchElement.GetTaggedObject()));
        });
    }
}

const JSTaggedType* JSStableArray::IndexOfBigInt(Span<const JSTaggedType> elements, JSTaggedValue searchElement,
                                                 IndexOfOptions options)
{
    ASSERT(searchElement.IsBigInt());
    return IndexOfElements(elements, options, [searchElement](JSTaggedType cur) {
        if (searchElement.GetRawData() == cur) {
            return true;
        }
        JSTaggedValue curValue(cur);
        if (!curValue.IsBigInt()) {
            return false;
        }
        return BigInt::Equal(curValue, searchElement);
    });
}

const JSTaggedType* JSStableArray::IndexOfObjectAddress(Span<const JSTaggedType> elements, JSTaggedValue searchElement,
                                                        IndexOfOptions options)
{
    // Note: searchElement may be true, false or null
    ASSERT(searchElement.IsObject());
    JSTaggedType targetAddress = searchElement.GetRawData();
    return IndexOfElements(elements, options, [targetAddress](JSTaggedType cur) {
        return cur == targetAddress;
    });
}

JSTaggedValue JSStableArray::IndexOfDispatch(JSThread *thread, JSHandle<JSTaggedValue> receiver,
                                             JSHandle<JSTaggedValue> searchElementHandle, uint32_t from, uint32_t len,
                                             IndexOfOptions options)
{
    // Note: GC is guaranteed not to happen since no new object is created during the searching process.
    DISALLOW_GARBAGE_COLLECTION;
    const JSTaggedType *data = nullptr;
    JSTaggedValue elementsValue = JSHandle<JSObject>::Cast(receiver)->GetElements();
    bool isMutant = elementsValue.IsMutantTaggedArray();
    if (isMutant) {
        JSHandle<MutantTaggedArray> elements(thread, elementsValue);
        data = elements->GetData();
    } else {
        JSHandle<TaggedArray> elements(thread, elementsValue);
        data = elements->GetData();
    }
    Span<const JSTaggedType> range;
    if (options.reversedOrder) {
        range = {data, data + from + 1}; // lastIndexOf
    } else {
        range = {data + from, data + len}; // indexOf, includes
    }
    ElementsKind kind = JSHandle<JSObject>::Cast(receiver)->GetClass()->GetElementsKind();
    JSTaggedValue searchElement = searchElementHandle.GetTaggedValue();

    const JSTaggedType *foundPos = nullptr;
    if (searchElement.IsUndefined()) {
        foundPos = IndexOfUndefined(range, options, isMutant);
    } else if (isMutant) {
        LOG_DEBUGGER(DEBUG) << "IndexOfDispatch: isMutant";
        if (Elements::IsIntOrHoleInt(kind)) {
            foundPos = IndexOfInt(range, searchElement, options, true); // raw int32
        } else {
            ASSERT(Elements::IsInNumbers(kind));
            foundPos = IndexOfDouble(range, searchElement, options, true); // raw double
        }
    } else if (searchElement.IsInt() || Elements::IsIntOrHoleInt(kind)) {
        foundPos = IndexOfInt(range, searchElement, options, false);
    } else if (searchElement.IsDouble() || Elements::IsNumberOrHoleNumber(kind)) {
        foundPos = IndexOfDouble(range, searchElement, options, false);
    } else if (searchElement.IsString() || Elements::IsStringOrHoleString(kind)) {
        foundPos = IndexOfString(range, searchElement, options);
    } else if (searchElement.IsBigInt()) {
        foundPos = IndexOfBigInt(range, searchElement, options);
    } else {
        foundPos = IndexOfObjectAddress(range, searchElement, options);
    }
    if (options.returnType == IndexOfReturnType::TAGGED_FOUND_INDEX) {
        return foundPos == nullptr ? JSTaggedValue(-1) : JSTaggedValue(static_cast<int32_t>(foundPos - data));
    } else {
        ASSERT(options.returnType == IndexOfReturnType::TAGGED_FOUND_OR_NOT);
        return JSTaggedValue(foundPos != nullptr);
    }
}

JSTaggedValue JSStableArray::Includes(JSThread *thread, JSHandle<JSTaggedValue> receiver,
                                      JSHandle<JSTaggedValue> searchElement, uint32_t from, uint32_t len)
{
    IndexOfOptions options;
    options.compType = ComparisonType::SAME_VALUE_ZERO;
    options.returnType = IndexOfReturnType::TAGGED_FOUND_OR_NOT;
    options.holeAsUndefined = true;
    options.reversedOrder = false;
    return IndexOfDispatch(thread, receiver, searchElement, from, len, options);
}

JSTaggedValue JSStableArray::IndexOf(JSThread *thread, JSHandle<JSTaggedValue> receiver,
                                     JSHandle<JSTaggedValue> searchElement, uint32_t from, uint32_t len)
{
    IndexOfOptions options;
    options.compType = ComparisonType::STRICT_EQUAL;
    options.returnType = IndexOfReturnType::TAGGED_FOUND_INDEX;
    options.holeAsUndefined = false;
    options.reversedOrder = false;
    return IndexOfDispatch(thread, receiver, searchElement, from, len, options);
}

JSTaggedValue JSStableArray::LastIndexOf(JSThread *thread, JSHandle<JSTaggedValue> receiver,
                                         JSHandle<JSTaggedValue> searchElement, uint32_t from, uint32_t len)
{
    IndexOfOptions options;
    options.compType = ComparisonType::STRICT_EQUAL;
    options.returnType = IndexOfReturnType::TAGGED_FOUND_INDEX;
    options.holeAsUndefined = false;
    options.reversedOrder = true;
    return IndexOfDispatch(thread, receiver, searchElement, from, len, options);
}

JSTaggedValue JSStableArray::Filter(JSHandle<JSObject> newArrayHandle, JSHandle<JSObject> thisObjHandle,
                                    EcmaRuntimeCallInfo *argv, uint32_t &k, uint32_t &toIndex)
{
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> callbackFnHandle = base::BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> thisArgHandle = base::BuiltinsBase::GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> toIndexHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const int32_t argsLength = 3; // 3: ?kValue, k, O?
    uint64_t len = static_cast<uint64_t>(base::ArrayHelper::GetArrayLength(thread, thisObjVal));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    while (k < len) {
        // Elements of thisObjHandle may change.
        JSTaggedValue value = ElementAccessor::Get(thread, thisObjHandle, k);
        if (value.IsHole() && JSTaggedValue::HasProperty(thread, thisObjVal, k)) {
            value = JSArray::FastGetPropertyByValue(thread, thisObjVal, k).GetTaggedValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        kValue.Update(value);
        if (!kValue.GetTaggedValue().IsHole()) {
            key.Update(JSTaggedValue(k));
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            JSTaggedValue callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (ElementAccessor::GetElementsLength(thisObjHandle) < len) {
                len = ElementAccessor::GetElementsLength(thisObjHandle);
            }
            bool boolResult = callResult.ToBoolean();
            if (boolResult) {
                toIndexHandle.Update(JSTaggedValue(toIndex));
                JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, toIndexHandle, kValue);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                toIndex++;
            }
        }
        k++;
        if (!thisObjVal->IsStableJSArray(thread)) {
            break;
        }
    }
    return base::BuiltinsBase::GetTaggedDouble(true);
}

JSTaggedValue JSStableArray::Map(JSHandle<JSObject> newArrayHandle, JSHandle<JSObject> thisObjHandle,
                                 EcmaRuntimeCallInfo *argv, uint32_t &k, uint32_t len)
{
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> callbackFnHandle = base::BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> thisArgHandle = base::BuiltinsBase::GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> mapResultHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    const int32_t argsLength = 3; // 3: ?kValue, k, O?
    while (k < len) {
        // Elements of thisObjHandle may change.
        JSTaggedValue value = ElementAccessor::Get(thread, thisObjHandle, k);
        if (value.IsHole() && JSTaggedValue::HasProperty(thread, thisObjVal, k)) {
            value = JSArray::FastGetPropertyByValue(thread, thisObjVal, k).GetTaggedValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        kValue.Update(value);
        if (!kValue.GetTaggedValue().IsHole()) {
            key.Update(JSTaggedValue(k));
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            JSTaggedValue mapResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            mapResultHandle.Update(mapResult);
            JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, k, mapResultHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (ElementAccessor::GetElementsLength(thisObjHandle) < len) {
                len = ElementAccessor::GetElementsLength(thisObjHandle);
            }
        }
        k++;
        if (!thisObjVal->IsStableJSArray(thread)) {
            break;
        }
    }
    return base::BuiltinsBase::GetTaggedDouble(true);
}

JSTaggedValue JSStableArray::Reverse(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                     int64_t &lower, uint32_t len)
{
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    if (thisObjHandle->IsJSArray()) {
        JSArray::CheckAndCopyArray(thread, JSHandle<JSArray>::Cast(thisObjHandle));
    }
    ElementsKind kind = thisObjHandle->GetClass()->GetElementsKind();
    JSHandle<TaggedArray> elements(thread, thisObjHandle->GetElements());
    if (thread->IsEnableMutantArray()) {
        if (kind == ElementsKind::INT || kind == ElementsKind::HOLE_INT) {
            return FastReverse(thread, elements, lower, len, ElementsKind::INT);
        } else if (kind == ElementsKind::NUMBER || kind == ElementsKind::HOLE_NUMBER) {
            return FastReverse(thread, elements, lower, len, ElementsKind::NUMBER);
        }
    }
    return FastReverse(thread, elements, lower, len, ElementsKind::TAGGED);
}

JSTaggedValue JSStableArray::FastReverse(JSThread *thread, JSHandle<TaggedArray> elements,
                                         int64_t &lower, uint32_t len, ElementsKind kind)
{
    JSMutableHandle<JSTaggedValue> lowerValueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> upperValueHandle(thread, JSTaggedValue::Undefined());
    int64_t middle = std::floor(len / 2);
    while (lower != middle) {
        if (elements->GetLength() != len) {
            break;
        }
        int64_t upper = static_cast<int64_t>(len) - lower - 1;
        lowerValueHandle.Update(ElementAccessor::FastGet(elements, lower, kind));
        upperValueHandle.Update(ElementAccessor::FastGet(elements, upper, kind));
        ElementAccessor::FastSet(thread, elements, lower, upperValueHandle, kind);
        ElementAccessor::FastSet(thread, elements, upper, lowerValueHandle, kind);
        lower++;
    }
    return base::BuiltinsBase::GetTaggedDouble(true);
}

JSTaggedValue JSStableArray::Concat(JSThread *thread, JSHandle<JSObject> newArrayHandle,
                                    JSHandle<JSObject> thisObjHandle, int64_t &k, int64_t &n)
{
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    int64_t thisLen = base::ArrayHelper::GetArrayLength(thread, thisObjVal);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
    while (k < thisLen) {
        if (ElementAccessor::GetElementsLength(thisObjHandle) != thisLen) {
            break;
        }
        toKey.Update(JSTaggedValue(n));
        JSTaggedValue kValue = ElementAccessor::Get(thread, thisObjHandle, k);
        if (!kValue.IsHole()) {
            JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, toKey, JSHandle<JSTaggedValue>(thread, kValue));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        n++;
        k++;
    }
    return base::BuiltinsBase::GetTaggedDouble(true);
}

template JSTaggedValue JSStableArray::FastCopyFromArrayToTypedArray<TypedArrayKind::SHARED>(
    JSThread *thread, JSHandle<JSTypedArray> &targetArray, DataViewType targetType,
    uint64_t targetOffset, uint32_t srcLength, JSHandle<JSObject> &obj);
template JSTaggedValue JSStableArray::FastCopyFromArrayToTypedArray<TypedArrayKind::NON_SHARED>(
    JSThread *thread, JSHandle<JSTypedArray> &targetArray, DataViewType targetType,
    uint64_t targetOffset, uint32_t srcLength, JSHandle<JSObject> &obj);

template<TypedArrayKind typedArrayKind>
JSTaggedValue JSStableArray::FastCopyFromArrayToTypedArray(JSThread *thread, JSHandle<JSTypedArray> &targetArray,
                                                           DataViewType targetType, uint64_t targetOffset,
                                                           uint32_t srcLength, JSHandle<JSObject> &obj)
{
    JSHandle<JSTaggedValue> targetBuffer(thread, targetArray->GetViewedArrayBufferOrByteArray());
    // If IsDetachedBuffer(targetBuffer) is true, throw a TypeError exception.
    if (BuiltinsArrayBufferType<typedArrayKind>::Type::IsDetachedBuffer(targetBuffer.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The targetBuffer of This value is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    uint32_t targetLength = targetArray->GetArrayLength();
    uint32_t targetByteOffset = targetArray->GetByteOffset();
    uint32_t targetElementSize = TypedArrayHelper::GetSizeFromType(targetType);
    if (srcLength + targetOffset > targetLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "The sum of length and targetOffset is greater than targetLength.",
                                     JSTaggedValue::Exception());
    }
    uint32_t targetByteIndex = static_cast<uint32_t>(targetOffset * targetElementSize + targetByteOffset);
    ContentType contentType = targetArray->GetContentType();
    uint32_t elemLen = ElementAccessor::GetElementsLength(obj);
    if (contentType == ContentType::BigInt) {
        JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Hole());
        JSMutableHandle<JSTaggedValue> elem(thread, JSTaggedValue::Hole());
        for (uint32_t i = 0; i < srcLength; i++) {
            if (i < elemLen) {
                elem.Update(ElementAccessor::Get(thread, obj, i));
            } else {
                elem.Update(JSTaggedValue::Hole());
            }
            kValue.Update(JSTaggedValue::ToBigInt(thread, elem));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            BuiltinsArrayBufferType<typedArrayKind>::Type::SetValueInBuffer(
                thread, targetBuffer.GetTaggedValue(), targetByteIndex, targetType, kValue, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            targetByteIndex += targetElementSize;
        }
    } else {
        double val = 0.0;
        uint32_t copyLen = srcLength > elemLen ? elemLen : srcLength;
        for (uint32_t i = 0; i < copyLen; i++) {
            JSTaggedValue taggedVal = ElementAccessor::Get(thread, obj, i);
            if (!taggedVal.IsNumber()) {
                JSTaggedNumber taggedNumber = JSTaggedValue::ToNumber(thread, taggedVal);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                val = taggedNumber.GetNumber();
            } else {
                val = taggedVal.GetNumber();
            }
            BuiltinsArrayBufferType<typedArrayKind>::Type::FastSetValueInBuffer(
                thread, targetBuffer.GetTaggedValue(), targetByteIndex, targetType, val, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            targetByteIndex += targetElementSize;
        }

        for (uint32_t i = copyLen; i < srcLength; i++) {
            val = JSTaggedNumber(base::NAN_VALUE).GetNumber();
            BuiltinsArrayBufferType<typedArrayKind>::Type::FastSetValueInBuffer(
                thread, targetBuffer.GetTaggedValue(), targetByteIndex, targetType, val, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            targetByteIndex += targetElementSize;
        }
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSStableArray::At(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t thisLen = receiver->GetArrayLength();
    if (thisLen == 0) {
        return JSTaggedValue::Undefined();
    }
    JSTaggedNumber index = JSTaggedValue::ToInteger(thread, base::BuiltinsBase::GetCallArg(argv, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int64_t relativeIndex = index.GetNumber();
    int64_t k = 0;
    if (relativeIndex >= 0) {
        k = relativeIndex;
    } else {
        k = static_cast<int64_t>(thisLen) + relativeIndex;
    }
    if (k < 0 || k >= thisLen) {
        return JSTaggedValue::Undefined();
    }

    auto result = JSTaggedValue::Hole();
    result = ElementAccessor::Get(thread, JSHandle<JSObject>::Cast(receiver), k);
    return result.IsHole() ? JSTaggedValue::Undefined() : result;
}

JSTaggedValue JSStableArray::At(JSHandle<JSSharedArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t thisLen = receiver->GetArrayLength();
    if (thisLen == 0) {
        return JSTaggedValue::Undefined();
    }
    JSTaggedNumber index = JSTaggedValue::ToInteger(thread, base::BuiltinsBase::GetCallArg(argv, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int64_t relativeIndex = index.GetNumber();
    int64_t k = 0;
    if (relativeIndex >= 0) {
        k = relativeIndex;
    } else {
        k = static_cast<int64_t>(thisLen) + relativeIndex;
    }
    if (k < 0 || k >= thisLen) {
        return JSTaggedValue::Undefined();
    }

    auto result = JSTaggedValue::Hole();
    result = ElementAccessor::Get(thread, JSHandle<JSObject>::Cast(receiver), k);
    return result.IsHole() ? JSTaggedValue::Undefined() : result;
}

JSTaggedValue JSStableArray::With(JSThread *thread, JSHandle<JSArray> receiver,
                                  int64_t insertCount, int64_t index, JSHandle<JSTaggedValue> value)
{
    JSHandle<JSObject> thisObjHandle(receiver);
    JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> newArray = JSArray::ArrayCreate(thread, JSTaggedNumber(0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(newArray);

    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    TaggedArray *destElements = TaggedArray::Cast(newArrayHandle->GetElements().GetTaggedObject());

    if (insertCount > ElementAccessor::GetElementsLength(newArrayHandle)) {
        destElements = *JSObject::GrowElementsCapacity(thread, newArrayHandle, insertCount);
    }
    ASSERT(!newArrayHandle->GetJSHClass()->IsDictionaryMode());
    bool needTransition = true;
    for (uint32_t idx = 0; idx < insertCount; idx++) {
        if (idx == index) {
            ElementAccessor::Set(thread, newArrayHandle, idx, value, needTransition);
        } else {
            JSHandle<JSTaggedValue> kValue(thread, ElementAccessor::Get(thread, thisObjHandle, idx));
            if (kValue->IsHole()) {
                ElementAccessor::Set(thread, newArrayHandle, idx, undefinedHandle, needTransition);
            } else {
                ElementAccessor::Set(thread, newArrayHandle, idx, kValue, needTransition);
            }
        }
    }
    JSHandle<JSArray>::Cast(newArrayHandle)->SetArrayLength(thread, insertCount);
    return newArrayHandle.GetTaggedValue();
}

JSTaggedValue JSStableArray::ToSpliced(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv,
                                       int64_t argc, int64_t actualStart, int64_t actualSkipCount, int64_t insertCount)
{
    JSThread *thread = argv->GetThread();

    JSHandle<JSObject> thisObjHandle(receiver);
    JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> newArray = JSArray::ArrayCreate(thread, JSTaggedNumber(0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(newArray);

    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    TaggedArray *destElements = TaggedArray::Cast(newArrayHandle->GetElements().GetTaggedObject());

    if (insertCount > ElementAccessor::GetElementsLength(newArrayHandle)) {
        destElements = *JSObject::GrowElementsCapacity(thread, newArrayHandle, insertCount);
    }
    ASSERT(!newArrayHandle->GetJSHClass()->IsDictionaryMode());
    int64_t i = 0;
    int64_t r = actualStart + actualSkipCount;
    bool needTransition = true;
    for (int64_t idx = 0; idx < actualStart; idx++, i++) {
        JSHandle<JSTaggedValue> kValue(thread, ElementAccessor::Get(thread, thisObjHandle, idx));
        if (kValue->IsHole()) {
            ElementAccessor::Set(thread, newArrayHandle, i, undefinedHandle, needTransition);
        } else {
            ElementAccessor::Set(thread, newArrayHandle, i, kValue, needTransition);
        }
    }

    for (uint32_t pos = 2; pos < argc; ++pos) { // 2:2 means there two arguments before the insert items.
        auto element = base::BuiltinsBase::GetCallArg(argv, pos);
        ElementAccessor::Set(thread, newArrayHandle, i, element, needTransition);
        ++i;
    }

    while (i < insertCount) {
        JSHandle<JSTaggedValue> kValue(thread, ElementAccessor::Get(thread, thisObjHandle, r));
        if (kValue->IsHole()) {
            ElementAccessor::Set(thread, newArrayHandle, i, undefinedHandle, needTransition);
        } else {
            ElementAccessor::Set(thread, newArrayHandle, i, kValue, needTransition);
        }
        ++i;
        ++r;
    }

    JSHandle<JSArray>::Cast(newArrayHandle)->SetArrayLength(thread, insertCount);

    return newArrayHandle.GetTaggedValue();
}

JSTaggedValue JSStableArray::ToReversed(JSThread *thread, JSHandle<JSArray> receiver,
                                        int64_t insertCount)
{
    JSHandle<JSObject> thisObjHandle(receiver);
    JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> newArray = JSArray::ArrayCreate(thread, JSTaggedNumber(0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(newArray);

    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    TaggedArray *destElements = TaggedArray::Cast(newArrayHandle->GetElements().GetTaggedObject());

    if (insertCount > ElementAccessor::GetElementsLength(newArrayHandle)) {
        destElements = *JSObject::GrowElementsCapacity(thread, newArrayHandle, insertCount);
    }
    ASSERT(!newArrayHandle->GetJSHClass()->IsDictionaryMode());
    bool needTransition = true;
    for (uint32_t idx = 0; idx < insertCount; idx++) {
        JSHandle<JSTaggedValue> kValue(thread, ElementAccessor::Get(thread, thisObjHandle, idx));
        if (kValue->IsHole()) {
            ElementAccessor::Set(thread, newArrayHandle, insertCount - idx - 1,
                                 undefinedHandle, needTransition);
        } else {
            ElementAccessor::Set(thread, newArrayHandle, insertCount - idx - 1, kValue, needTransition);
        }
    }
    JSHandle<JSArray>::Cast(newArrayHandle)->SetArrayLength(thread, insertCount);

    return newArrayHandle.GetTaggedValue();
}

JSTaggedValue JSStableArray::Reduce(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                    JSHandle<JSTaggedValue> callbackFnHandle,
                                    JSMutableHandle<JSTaggedValue> accumulator, int64_t &k, int64_t &len)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSTaggedValue callResult = JSTaggedValue::Undefined();
    while (k < len) {
        JSTaggedValue kValue(ElementAccessor::Get(thread, thisObjHandle, k));
        if (!kValue.IsHole()) {
            JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
            const uint32_t argsLength = 4; // 4: «accumulator, kValue, k, O»
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, undefined, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(accumulator.GetTaggedValue(), kValue, JSTaggedValue(k),
                             thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (ElementAccessor::GetElementsLength(thisObjHandle) < len) {
                len = ElementAccessor::GetElementsLength(thisObjHandle);
            }
            accumulator.Update(callResult);
        }
        k++;
        if (!thisObjVal->IsStableJSArray(thread)) {
            break;
        }
    }
    return base::BuiltinsBase::GetTaggedDouble(true);
}

JSTaggedValue JSStableArray::Slice(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                   int64_t &k, int64_t &count)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    int64_t len = static_cast<int64_t>(ElementAccessor::GetElementsLength(thisObjHandle));
    int64_t oldLen;
    if (len > k + count) {
        oldLen = count;
    } else {
        oldLen = std::max<int64_t>(len - k, 0);
    }
    JSHandle<JSObject> arrayObj = factory->NewAndCopyJSArrayObject(thisObjHandle, count, oldLen, k);
    for (int i = 0; i < count; i++) {
        JSMutableHandle<JSTaggedValue> value(thread, ElementAccessor::Get(thread, arrayObj, i));
        if (value->IsHole() && JSTaggedValue::HasProperty(thread, thisObjVal, i + k)) {
            value.Update(JSArray::FastGetPropertyByValue(thread, thisObjVal, i + k).GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            ElementAccessor::Set(thread, arrayObj, i, value, true);
        }
    }
    return arrayObj.GetTaggedValue();
}

JSHandle<TaggedArray> JSStableArray::SortIndexedProperties(JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal,
                                                           int64_t len, const JSHandle<JSTaggedValue> &callbackFnHandle,
                                                           base::HolesType holes)
{
    JSHandle<JSObject> thisObj(thread, thisObjVal.GetTaggedValue());
    JSHandle<TaggedArray> elements(thread, thisObj->GetElements());
    ElementsKind kind = thisObj->GetClass()->GetElementsKind();
    if (!elements->GetClass()->IsMutantTaggedArray()) {
        kind = ElementsKind::GENERIC;
    }
    // 1. fill elements into items.
    JSHandle<TaggedArray> items(thread->GetEcmaVM()->GetFactory()->NewTaggedArray(len));
    bool kRead = false;
    int64_t tmp = 0;
    for (int k = 0; k < len; k++) {
        JSTaggedValue kValue = ElementAccessor::FastGet(elements, k, kind);
        if (holes == base::HolesType::SKIP_HOLES) {
            kRead = (kValue != JSTaggedValue::Hole());
        } else {
            ASSERT(holes == base::HolesType::READ_THROUGH_HOLES);
            kRead = true;
        }
        if (kRead) {
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, items);
            items->Set(thread, tmp++, kValue);
        }
    }
    // 2. trim
    if (len > tmp) {
        items->Trim(thread, tmp);
    }
    // 3. Sort items using an implementation-defined sequence of calls to SortCompare.
    // If any such call returns an abrupt completion,
    // stop before performing any further calls to SortCompare and return that Completion Record.
    base::TimSort::Sort(thread, items, callbackFnHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, items);
    // 4. Return items.
    return items;
}

JSTaggedValue JSStableArray::CopySortedListToReceiver(JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal,
                                                      JSHandle<TaggedArray> sortedList, uint32_t len)
{
    // 6. Let itemCount be the number of elements in sortedList.
    uint32_t itemCount = sortedList->GetLength();

    // grow elements if len > newLength.
    JSHandle<JSObject> thisObj(thisObjVal);
    uint32_t newLength = std::max(JSHandle<JSArray>::Cast(thisObjVal)->GetArrayLength(), itemCount);
    TaggedArray *elements = TaggedArray::Cast(thisObj->GetElements().GetTaggedObject());
    if (newLength > ElementAccessor::GetElementsLength(thisObj)) {
        elements = *JSObject::GrowElementsCapacity(thread, thisObj, newLength, true);
    }

    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    bool needTransition = true;
    // 7. Let j be 0.
    // 8. Repeat, while j < itemCount,
    //     a. Perform ! Set(obj, ! ToString((j)), sortedList[j], true).
    //     b. Set j to j + 1.
    for (uint32_t j = 0; j < itemCount; j++) {
        valueHandle.Update(sortedList->Get(j));
        ElementAccessor::Set(thread, thisObj, j, valueHandle, needTransition);
    }
    // 9. NOTE: The call to SortIndexedProperties in step 5 uses SKIP-HOLES.The remaining indices are deleted to
    // preserve the number of holes that were detected and excluded from the sort.
    // 10. Repeat, while j < len,
    //       a. Perform ? DeletePropertyOrThrow(obj, ! ToString((j))).
    //       b. Set j to j + 1.
    valueHandle.Update(JSTaggedValue::Hole());
    for (uint32_t j = itemCount; j < len; j++) {
        ElementAccessor::Set(thread, thisObj, j, valueHandle, needTransition);
    }
    JSHandle<JSArray>::Cast(thisObj)->SetArrayLength(thread, newLength);
    return thisObj.GetTaggedValue();
}

JSTaggedValue JSStableArray::Sort(JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal,
                                  const JSHandle<JSTaggedValue> &callbackFnHandle)
{
    // 3. Let len be ?LengthOfArrayLike(obj).
    int64_t len = base::ArrayHelper::GetArrayLength(thread, thisObjVal);
    // ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // If len is 0 or 1, no need to sort
    if (len == 0 || len == 1) {
        return thisObjVal.GetTaggedValue();
    }
    if (callbackFnHandle->IsUndefined()) {
        JSArray::SortElementsByObject(thread, JSHandle<JSObject>::Cast(thisObjVal), callbackFnHandle);
        return thisObjVal.GetTaggedValue();
    }
    JSHandle<TaggedArray> sortedList = JSStableArray::SortIndexedProperties(
        thread, thisObjVal, len, callbackFnHandle, base::HolesType::SKIP_HOLES);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (thisObjVal->IsStableJSArray(thread)) {
        CopySortedListToReceiver(thread, thisObjVal, sortedList, len);
    } else {
        JSArray::CopySortedListToReceiver(thread, thisObjVal, sortedList, len);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    return thisObjVal.GetTaggedValue();
}

JSTaggedValue JSStableArray::Fill(JSThread *thread, const JSHandle<JSObject> &thisObj,
                                  const JSHandle<JSTaggedValue> &value, int64_t start,
                                  int64_t end)
{
    JSArray::CheckAndCopyArray(thread, JSHandle<JSArray>::Cast(thisObj));
    uint32_t length = ElementAccessor::GetElementsLength(thisObj);
    ElementsKind oldKind = thisObj->GetClass()->GetElementsKind();
    if (start == 0 && end == length) {
        if (oldKind != ElementsKind::GENERIC) {
            JSHClass::TransitToElementsKindUncheck(thread, thisObj, Elements::ToElementsKind(value.GetTaggedValue()));
        }
    } else if (JSHClass::TransitToElementsKind(thread, thisObj, value)) {
        ElementsKind newKind = thisObj->GetClass()->GetElementsKind();
        Elements::MigrateArrayWithKind(thread, thisObj, oldKind, newKind);
    }
    if (length >= end) {
        if (thisObj->GetElements().IsMutantTaggedArray()) {
            ElementsKind kind = thisObj->GetClass()->GetElementsKind();
            TaggedArray *elements = TaggedArray::Cast(thisObj->GetElements());
            JSTaggedValue migratedValue = JSTaggedValue(ElementAccessor::ConvertTaggedValueWithElementsKind(
                value.GetTaggedValue(), kind));
            for (int64_t idx = start; idx < end; idx++) {
                elements->Set<false>(thread, idx, migratedValue);
            }
        } else {
            TaggedArray *elements = TaggedArray::Cast(thisObj->GetElements());
            for (int64_t idx = start; idx < end; idx++) {
                elements->Set(thread, idx, value);
            }
        }
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    if (JSHandle<JSArray>::Cast(thisObj)->GetArrayLength() < end) {
        JSHandle<JSArray>::Cast(thisObj)->SetArrayLength(thread, end);
    }
    return thisObj.GetTaggedValue();
}

JSTaggedValue JSStableArray::HandleFindLastOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                    JSHandle<JSTaggedValue> callbackFnHandle,
                                                    JSHandle<JSTaggedValue> thisArgHandle,
                                                    JSMutableHandle<JSTaggedValue> &kValue, int64_t &k)
{
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    JSTaggedValue callResult = base::BuiltinsBase::GetTaggedBoolean(false);
    while (k >= 0) {
        JSTaggedValue val = ElementAccessor::Get(thread, thisObjHandle, k);
        if (!val.IsHole()) {
            kValue.Update(val);
        } else if (JSTaggedValue::HasProperty(thread, thisObjVal, k)) {
            auto res = JSArray::FastGetPropertyByValue(thread, thisObjVal, k).GetTaggedValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            kValue.Update(res);
        } else {
            kValue.Update(JSTaggedValue::Undefined());
        }
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), JSTaggedValue(k), thisObjVal.GetTaggedValue());
        callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (callResult.ToBoolean()) {
            return callResult;
        }
        k--;
        ASSERT(ElementAccessor::GetElementsLength(thisObjHandle) > 0);
        if (ElementAccessor::GetElementsLength(thisObjHandle) - 1 < k) {
            break;
        }
        if (!thisObjVal->IsStableJSArray(thread)) {
            break;
        }
    }
    return callResult;
}

JSTaggedValue JSStableArray::HandleReduceRightOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                       JSHandle<JSTaggedValue> callbackFnHandle,
                                                       JSMutableHandle<JSTaggedValue> &accumulator,
                                                       JSHandle<JSTaggedValue> thisArgHandle, int64_t &k)
{
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Hole());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSTaggedValue callResult = JSTaggedValue::Undefined();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const int32_t argsLength = 4; // 4: «accumulator, kValue, k, O»
    int64_t len = static_cast<int64_t>(base::ArrayHelper::GetArrayLength(thread, thisObjVal));
    while (k >= 0) {
        key.Update(JSTaggedValue(k));
        kValue.Update(ElementAccessor::Get(thread, thisObjHandle, k));
        if (!kValue.GetTaggedValue().IsHole()) {
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(accumulator.GetTaggedValue(), kValue.GetTaggedValue(),
                key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            accumulator.Update(callResult);
        } else {
            bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, key);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (exists) {
                auto res = JSArray::FastGetPropertyByValue(thread, thisObjVal, key).GetTaggedValue();
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                kValue.Update(res);
                EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle,
                    thisArgHandle, undefined, argsLength);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                info->SetCallArg(accumulator.GetTaggedValue(), kValue.GetTaggedValue(),
                    key.GetTaggedValue(), thisObjVal.GetTaggedValue());
                callResult = JSFunction::Call(info);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                accumulator.Update(callResult);
            }
        }
        k--;
        int64_t newLen = static_cast<int64_t>(base::ArrayHelper::GetArrayLength(thread, thisObjVal));
        if (!thisObjVal->IsStableJSArray(thread) || newLen != len) {
            return base::BuiltinsBase::GetTaggedBoolean(false);
        }
    }
    return base::BuiltinsBase::GetTaggedBoolean(true);
}
}  // namespace panda::ecmascript
