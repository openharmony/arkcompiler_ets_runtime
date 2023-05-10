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

#include "ecmascript/js_stable_array.h"

#include "ecmascript/base/array_helper.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
using TypedArrayHelper = base::TypedArrayHelper;
using BuiltinsArrayBuffer = builtins::BuiltinsArrayBuffer;

JSTaggedValue JSStableArray::Push(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t argc = argv->GetArgsNumber();
    uint32_t oldLength = receiver->GetArrayLength();
    uint32_t newLength = argc + oldLength;

    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    if (newLength > elements->GetLength()) {
        elements = *JSObject::GrowElementsCapacity(thread, JSHandle<JSObject>::Cast(receiver), newLength);
    }

    for (uint32_t k = 0; k < argc; k++) {
        JSHandle<JSTaggedValue> value = argv->GetCallArg(k);
        elements->Set(thread, oldLength + k, value.GetTaggedValue());
    }
    receiver->SetArrayLength(thread, newLength);

    return JSTaggedValue(newLength);
}

JSTaggedValue JSStableArray::Pop(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t length = receiver->GetArrayLength();
    if (length == 0) {
        return JSTaggedValue::Undefined();
    }

    JSArray::CheckAndCopyArray(thread, receiver);
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t index = length - 1;
    auto result = elements->Get(index);
    if (TaggedArray::ShouldTrim(capacity, index)) {
        elements->Trim(thread, index);
    } else {
        elements->Set(thread, index, JSTaggedValue::Hole());
    }
    receiver->SetArrayLength(thread, index);
    return result;
}

JSTaggedValue JSStableArray::Splice(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv,
                                    uint32_t start, uint32_t insertCount, uint32_t actualDeleteCount)
{
    JSThread *thread = argv->GetThread();
    uint32_t len = receiver->GetArrayLength();
    uint32_t argc = argv->GetArgsNumber();

    JSHandle<JSObject> thisObjHandle(receiver);
    JSTaggedValue newArray = JSArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(actualDeleteCount));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);

    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    TaggedArray *srcElements = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    JSMutableHandle<TaggedArray> srcElementsHandle(thread, srcElements);
    if (newArray.IsStableJSArray(thread)) {
        TaggedArray *destElements = TaggedArray::Cast(newArrayHandle->GetElements().GetTaggedObject());
        if (actualDeleteCount > destElements->GetLength()) {
            destElements = *JSObject::GrowElementsCapacity(thread, newArrayHandle, actualDeleteCount);
        }

        for (uint32_t idx = 0; idx < actualDeleteCount; idx++) {
            if ((start + idx) >= srcElementsHandle->GetLength()) {
                destElements->Set(thread, idx, JSTaggedValue::Hole());
            } else {
                destElements->Set(thread, idx, srcElementsHandle->Get(start + idx));
            }
        }
        JSHandle<JSArray>::Cast(newArrayHandle)->SetArrayLength(thread, actualDeleteCount);
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
    uint32_t oldCapacity = srcElementsHandle->GetLength();
    uint32_t newCapacity = len - actualDeleteCount + insertCount;
    if (newCapacity > oldCapacity) {
        srcElementsHandle.Update(JSObject::GrowElementsCapacity(thread, thisObjHandle, newCapacity));
    }
    if (insertCount < actualDeleteCount) {
        JSArray::CheckAndCopyArray(thread, receiver);
        srcElementsHandle.Update(receiver->GetElements());
        for (uint32_t idx = start; idx < len - actualDeleteCount; idx++) {
            auto element = JSTaggedValue::Hole();
            if ((idx + actualDeleteCount) < srcElementsHandle->GetLength()) {
                element = srcElementsHandle->Get(idx + actualDeleteCount);
            }
            element = element.IsHole() ? JSTaggedValue::Undefined() : element;
            if ((idx + insertCount) < srcElementsHandle->GetLength()) {
                srcElementsHandle->Set(thread, idx + insertCount, element);
            }
        }

        if ((oldCapacity > newCapacity) && TaggedArray::ShouldTrim(oldCapacity, newCapacity)) {
            srcElementsHandle->Trim(thread, newCapacity);
        } else {
            for (uint32_t idx = newCapacity; idx < len; idx++) {
                if (idx < srcElementsHandle->GetLength()) {
                    srcElementsHandle->Set(thread, idx, JSTaggedValue::Hole());
                }
            }
        }
    } else {
        for (uint32_t idx = len - actualDeleteCount; idx > start; idx--) {
            auto element = srcElementsHandle->Get(idx + actualDeleteCount - 1);
            element = element.IsHole() ? JSTaggedValue::Undefined() : element;
            srcElementsHandle->Set(thread, idx + insertCount - 1, element);
        }
    }

    for (uint32_t i = 2, idx = start; i < argc; i++, idx++) {
        srcElementsHandle->Set(thread, idx, argv->GetCallArg(i));
    }

    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(newCapacity));
    JSTaggedValue::SetProperty(thread, thisObjVal, lengthKey, newLenHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newArrayHandle.GetTaggedValue();
}

JSTaggedValue JSStableArray::Shift(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t length = receiver->GetArrayLength();
    if (length == 0) {
        return JSTaggedValue::Undefined();
    }
    JSArray::CheckAndCopyArray(thread, receiver);
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    auto result = elements->Get(0);
    for (uint32_t k = 1; k < length; k++) {
        auto kValue = elements->Get(k);
        if (kValue.IsHole()) {
            elements->Set(thread, k - 1, JSTaggedValue::Undefined());
        } else {
            elements->Set(thread, k - 1, kValue);
        }
    }
    uint32_t capacity = elements->GetLength();
    uint32_t index = length - 1;
    if (TaggedArray::ShouldTrim(capacity, index)) {
        elements->Trim(thread, index);
    } else {
        elements->Set(thread, index, JSTaggedValue::Hole());
    }
    receiver->SetArrayLength(thread, index);
    return result;
}

JSTaggedValue JSStableArray::Join(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t length = receiver->GetArrayLength();
    JSHandle<JSTaggedValue> sepHandle = base::BuiltinsBase::GetCallArg(argv, 0);
    int sep = ',';
    uint32_t sepLength = 1;
    JSHandle<EcmaString> sepStringHandle;
    if (!sepHandle->IsUndefined()) {
        if (sepHandle->IsString()) {
            sepStringHandle = JSHandle<EcmaString>::Cast(sepHandle);
        } else {
            sepStringHandle = JSTaggedValue::ToString(thread, sepHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
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
    if (length == 0) {
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        return globalConst->GetEmptyString();
    }
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    size_t allocateLength = 0;
    bool isOneByte = (sep != JSStableArray::SeparatorFlag::MINUS_ONE) || EcmaStringAccessor(sepStringHandle).IsUtf8();
    CVector<JSHandle<EcmaString>> vec;
    JSMutableHandle<JSTaggedValue> elementHandle(thread, JSTaggedValue::Undefined());
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    uint32_t elementsLength = elements->GetLength();
    uint32_t len = elementsLength > length ? length : elementsLength;
    if (elementsLength == 0 && length != 0) {
        len = length;
    }
    for (uint32_t k = 0; k < len; k++) {
        JSTaggedValue element = JSTaggedValue::Undefined();
        if (k < elementsLength) {
            element = elements->Get(k);
        }
        if (!element.IsUndefinedOrNull() && !element.IsHole()) {
            if (!element.IsString()) {
                elementHandle.Update(element);
                JSHandle<EcmaString> strElement = JSTaggedValue::ToString(thread, elementHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                element = strElement.GetTaggedValue();
                elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
            }
            auto nextStr = EcmaString::Cast(element.GetTaggedObject());
            JSHandle<EcmaString> nextStrHandle(thread, nextStr);
            vec.push_back(nextStrHandle);
            isOneByte = EcmaStringAccessor(nextStr).IsUtf8() ? isOneByte : false;
            allocateLength += EcmaStringAccessor(nextStr).GetLength();
        } else {
            vec.push_back(JSHandle<EcmaString>(globalConst->GetHandledEmptyString()));
        }
    }
    if (len > 0) {
        allocateLength += sepLength * (len - 1);
    }
    auto newString = EcmaStringAccessor::AllocStringObject(thread->GetEcmaVM(), allocateLength, isOneByte);
    int current = 0;
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
        EcmaStringAccessor::ReadData(newString, *nextStr, current,
            allocateLength - static_cast<uint32_t>(current), nextLength);
        current += nextLength;
    }
    ASSERT_PRINT(
        isOneByte == EcmaStringAccessor::CanBeCompressed(newString), "isOneByte does not match the real value!");
    return JSTaggedValue(newString);
}

JSTaggedValue JSStableArray::HandleFindIndexOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                     JSHandle<JSTaggedValue> callbackFnHandle,
                                                     JSHandle<JSTaggedValue> thisArgHandle, uint32_t &k)
{
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint64_t len = static_cast<uint64_t>(base::ArrayHelper::GetArrayLength(thread, thisObjVal));
    JSTaggedValue callResult = base::BuiltinsBase::GetTaggedBoolean(false);
    const int32_t argsLength = 3; // 3: ?kValue, k, O?
    JSMutableHandle<TaggedArray> array(thread, thisObjHandle->GetElements());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    while (k < len) {
        // Elements of thisObjHandle may change.
        array.Update(thisObjHandle->GetElements());
        kValue.Update(array->Get(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), JSTaggedValue(k), thisObjVal.GetTaggedValue());
        callResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, callResult);
        if (callResult.ToBoolean()) {
            return callResult;
        }
        if (array->GetLength() < len) {
            len = array->GetLength();
        }
        if (base::ArrayHelper::GetArrayLength(thread, thisObjVal) > static_cast<int64_t>(len)) {
            array.Update(thisObjHandle->GetElements());
        }
        k++;
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
    JSMutableHandle<TaggedArray> array(thread, thisObjHandle->GetElements());
    uint64_t len = static_cast<uint64_t>(base::ArrayHelper::GetArrayLength(thread, thisObjVal));
    const int32_t argsLength = 3; // 3: ?kValue, k, O?
    JSTaggedValue callResult = base::BuiltinsBase::GetTaggedBoolean(true);
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    while (k < len) {
        // Elements of thisObjHandle may change.
        array.Update(thisObjHandle->GetElements());
        kValue.Update(array->Get(k));
        if (!kValue.GetTaggedValue().IsHole()) {
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            info->SetCallArg(kValue.GetTaggedValue(), JSTaggedValue(k), thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (array->GetLength() < len) {
                len = array->GetLength();
            }
        } else if (JSTaggedValue::HasProperty(thread, thisObjVal, k)) {
            JSHandle<JSTaggedValue> kValue1 = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
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

JSTaggedValue JSStableArray::HandleforEachOfStable(JSThread *thread, JSHandle<JSObject> thisObjHandle,
                                                   JSHandle<JSTaggedValue> callbackFnHandle,
                                                   JSHandle<JSTaggedValue> thisArgHandle, uint32_t &k)
{
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSMutableHandle<TaggedArray> array(thread, thisObjHandle->GetElements());
    uint64_t len = static_cast<uint64_t>(base::ArrayHelper::GetArrayLength(thread, thisObjVal));
    const int32_t argsLength = 3; // 3: ?kValue, k, O?
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    if (array->GetLength() <= k) {
        return base::BuiltinsBase::GetTaggedBoolean(false);
    }
    while (k < len) {
        // Elements of thisObjHandle may change.
        array.Update(thisObjHandle->GetElements());
        kValue.Update(array->Get(k));
        if (!kValue.GetTaggedValue().IsHole()) {
            key.Update(JSTaggedValue(k));
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            JSTaggedValue funcResult = JSFunction::Call(info);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
            if (array->GetLength() < len) {
                len = array->GetLength();
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

JSTaggedValue JSStableArray::IndexOf(JSThread *thread, JSHandle<JSTaggedValue> receiver,
                                     JSHandle<JSTaggedValue> searchElement, uint32_t from, uint32_t len)
{
    JSHandle<TaggedArray> elements(thread, JSHandle<JSObject>::Cast(receiver)->GetElements());
    while (from < len) {
        JSTaggedValue value = elements->Get(from);
        if (!value.IsUndefined() && !value.IsHole()) {
            if (JSTaggedValue::StrictEqual(searchElement.GetTaggedValue(), value)) {
                return JSTaggedValue(from);
            }
        } else {
            bool exist = JSTaggedValue::HasProperty(thread, receiver, from);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (exist) {
                JSHandle<JSTaggedValue> kValueHandle = JSArray::FastGetPropertyByValue(thread, receiver, from);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                if (JSTaggedValue::StrictEqual(thread, searchElement, kValueHandle)) {
                    return JSTaggedValue(from);
                }
            }
        }
        from++;
    }
    return JSTaggedValue(-1);
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
    JSMutableHandle<TaggedArray> array(thread, thisObjHandle->GetElements());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    while (k < len) {
        // Elements of thisObjHandle may change.
        array.Update(thisObjHandle->GetElements());
        kValue.Update(array->Get(k));
        if (!kValue.GetTaggedValue().IsHole()) {
            key.Update(JSTaggedValue(k));
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            JSTaggedValue callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (array->GetLength() < len) {
                len = array->GetLength();
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
    JSMutableHandle<TaggedArray> array(thread, thisObjHandle->GetElements());
    while (k < len) {
        // Elements of thisObjHandle may change.
        array.Update(thisObjHandle->GetElements());
        kValue.Update(array->Get(k));
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
            if (array->GetLength() < len) {
                len = array->GetLength();
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
                                     JSHandle<JSTaggedValue> thisHandle, int64_t &lower, uint32_t len)
{
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    if (thisObjHandle->IsJSArray()) {
        JSArray::CheckAndCopyArray(thread, JSHandle<JSArray>::Cast(thisObjHandle));
    }
    JSHandle<TaggedArray> array(thread, thisObjHandle->GetElements());
    JSMutableHandle<JSTaggedValue> lowerP(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> upperP(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> lowerValueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> upperValueHandle(thread, JSTaggedValue::Undefined());
    int64_t middle = std::floor(len / 2);
    while (lower != middle) {
        if (array->GetLength() != len) {
            break;
        }
        int64_t upper = static_cast<int64_t>(len) - lower - 1;
        lowerP.Update(JSTaggedValue(lower));
        upperP.Update(JSTaggedValue(upper));
        bool lowerExists = (thisHandle->IsTypedArray() || JSTaggedValue::HasProperty(thread, thisObjVal, lowerP));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (lowerExists) {
            lowerValueHandle.Update(array->Get(lower));
        }
        bool upperExists = (thisHandle->IsTypedArray() || JSTaggedValue::HasProperty(thread, thisObjVal, upperP));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (upperExists) {
            upperValueHandle.Update(array->Get(upper));
        }
        if (lowerExists && upperExists) {
            array->Set(thread, lower, upperValueHandle.GetTaggedValue());
            array->Set(thread, upper, lowerValueHandle.GetTaggedValue());
        } else if (upperExists) {
            array->Set(thread, lower, upperValueHandle.GetTaggedValue());
            JSTaggedValue::SetProperty(thread, thisObjVal, lowerP, upperValueHandle, true);
            JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, upperP);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else if (lowerExists) {
            array->Set(thread, upper, lowerValueHandle.GetTaggedValue());
            JSTaggedValue::SetProperty(thread, thisObjVal, upperP, lowerValueHandle, true);
            JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, lowerP);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        lower++;
    }
    return base::BuiltinsBase::GetTaggedDouble(true);
}

JSTaggedValue JSStableArray::Concat(JSThread *thread, JSHandle<JSObject> newArrayHandle,
                                    JSHandle<JSObject> thisObjHandle, int64_t &k, int64_t &n)
{
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    int64_t thisLen = base::ArrayHelper::GetArrayLength(thread, thisObjVal);
    JSHandle<TaggedArray> arrayFrom(thread, thisObjHandle->GetElements());
    JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
    while (k < thisLen) {
        if (arrayFrom->GetLength() != thisLen) {
            break;
        }
        toKey.Update(JSTaggedValue(n));
        JSTaggedValue kValue = arrayFrom->Get(k);
        if (!kValue.IsHole()) {
            JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, toKey, JSHandle<JSTaggedValue>(thread, kValue));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        n++;
        k++;
    }
    return base::BuiltinsBase::GetTaggedDouble(true);
}

JSTaggedValue JSStableArray::FastCopyFromArrayToTypedArray(JSThread *thread, JSHandle<JSTypedArray> &targetArray,
                                                           DataViewType targetType, uint32_t targetOffset,
                                                           uint32_t srcLength, JSHandle<TaggedArray> &elements)
{
    JSHandle<JSTaggedValue> targetBuffer(thread, targetArray->GetViewedArrayBuffer());
    // If IsDetachedBuffer(targetBuffer) is true, throw a TypeError exception.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(targetBuffer.GetTaggedValue())) {
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
    JSMutableHandle<JSTaggedValue> elem(thread, JSTaggedValue::Hole());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Hole());
    ContentType contentType = targetArray->GetContentType();
    for (uint32_t i = 0; i < srcLength; i++) {
        elem.Update(elements->Get(i));
        if (contentType == ContentType::BigInt) {
            kValue.Update(JSTaggedValue::ToBigInt(thread, elem));
        } else {
            kValue.Update(JSTaggedValue::ToNumber(thread, elem));
        }
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        BuiltinsArrayBuffer::SetValueInBuffer(thread, targetBuffer.GetTaggedValue(), targetByteIndex,
                                              targetType, kValue, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        targetByteIndex += targetElementSize;
    }
    return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript
