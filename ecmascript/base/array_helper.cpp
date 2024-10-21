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

#include "ecmascript/base/array_helper.h"

#include "ecmascript/base/sort_helper.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/object_fast_operator-inl.h"

namespace panda::ecmascript::base {
int64_t ArrayHelper::GetStartIndex(JSThread *thread, const JSHandle<JSTaggedValue> &startIndexHandle,
                                   int64_t length)
{
    // Common procedure to clamp fromIndexValue to the range [0, length].
    // For integer case, conditional selection instructions (csel in ARM, cmov in x86, etc.)
    // may be utilized by the compiler to minimize branching.
    auto doClamp = [length](auto fromIndexValue) -> int64_t {
        if (LIKELY(fromIndexValue >= 0)) {
            // Including the case where fromIndexValue == Infinity
            return (fromIndexValue >= length) ? length : static_cast<int64_t>(fromIndexValue);
        }
        auto plusLength = fromIndexValue + length;
        if (plusLength >= 0) {
            return static_cast<int64_t>(plusLength);
        }
        return 0; // Including the case where fromIndexValue == -Infinity
    };
    if (LIKELY(startIndexHandle->IsInt())) {
        // Fast path: startIndexHandle is tagged int32.
        return doClamp(startIndexHandle->GetInt());
    }
    // Slow path: startIndexHandle is targged double, or type conversion is involved.
    JSTaggedNumber fromIndexTemp = JSTaggedValue::ToNumber(thread, startIndexHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, length);

    double fromIndexValue = base::NumberHelper::TruncateDouble(fromIndexTemp.GetNumber()); // NaN -> 0
    return doClamp(fromIndexValue);
}

int64_t ArrayHelper::GetStartIndexFromArgs(JSThread *thread, EcmaRuntimeCallInfo *argv,
                                           uint32_t argIndex, int64_t length)
{
    uint32_t argc = argv->GetArgsNumber();
    if (argc <= argIndex) {
        return 0;
    }
    JSHandle<JSTaggedValue> arg = base::BuiltinsBase::GetCallArg(argv, argIndex);
    return GetStartIndex(thread, arg, length);
}

int64_t ArrayHelper::GetLastStartIndex(JSThread *thread, const JSHandle<JSTaggedValue> &startIndexHandle,
                                       int64_t length)
{
    // Common procedure to clamp fromIndexValue to the range [-1, length-1].
    auto doClamp = [length](auto fromIndexValue) -> int64_t {
        if (LIKELY(fromIndexValue >= 0)) {
            // Including the case where fromIndexValue == Infinity
            return (length - 1 < fromIndexValue) ? (length - 1) : static_cast<int64_t>(fromIndexValue);
        }
        auto plusLength = fromIndexValue + length;
        if (plusLength >= 0) {
            return static_cast<int64_t>(plusLength);
        }
        return -1; // Including the case where fromIndexValue == -Infinity
    };
    if (LIKELY(startIndexHandle->IsInt())) {
        // Fast path: startIndexHandle is tagged int32.
        return doClamp(startIndexHandle->GetInt());
    }
    // Slow path: startIndexHandle is targged double, or type conversion is involved.
    JSTaggedNumber fromIndexTemp = JSTaggedValue::ToNumber(thread, startIndexHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, -1);

    double fromIndexValue = base::NumberHelper::TruncateDouble(fromIndexTemp.GetNumber()); // NaN -> 0
    return doClamp(fromIndexValue);
}

int64_t ArrayHelper::GetLastStartIndexFromArgs(JSThread *thread, EcmaRuntimeCallInfo *argv,
                                               uint32_t argIndex, int64_t length)
{
    uint32_t argc = argv->GetArgsNumber();
    if (argc <= argIndex) {
        return length - 1;
    }
    JSHandle<JSTaggedValue> arg = base::BuiltinsBase::GetCallArg(argv, argIndex);
    return GetLastStartIndex(thread, arg, length);
}

bool ArrayHelper::ElementIsStrictEqualTo(JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal,
                                         const JSHandle<JSTaggedValue> &keyHandle,
                                         const JSHandle<JSTaggedValue> &target)
{
    bool exists = thisObjVal->IsTypedArray() || thisObjVal->IsSharedTypedArray() ||
        JSTaggedValue::HasProperty(thread, thisObjVal, keyHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (thread->HasPendingException() || !exists) {
        return false;
    }
    JSHandle<JSTaggedValue> valueHandle = JSArray::FastGetPropertyByValue(thread, thisObjVal, keyHandle);
    if (thread->HasPendingException()) {
        return false;
    }
    return JSTaggedValue::StrictEqual(thread, target, valueHandle);
}

bool ArrayHelper::IsConcatSpreadable(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    // 1. If Type(O) is not Object, return false.
    if (!obj->IsECMAObject()) {
        return false;
    }

    // 2. Let spreadable be Get(O, @@isConcatSpreadable).
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSTaggedValue> isConcatsprKey = env->GetIsConcatSpreadableSymbol();
    JSTaggedValue spreadable = ObjectFastOperator::FastGetPropertyByValue(thread, obj.GetTaggedValue(),
                                                                          isConcatsprKey.GetTaggedValue());
    // 3. ReturnIfAbrupt(spreadable).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 4. If spreadable is not undefined, return ToBoolean(spreadable).
    if (!spreadable.IsUndefined()) {
        return spreadable.ToBoolean();
    }

    // 5. Return IsArray(O).
    return obj->IsArray(thread) || obj->IsJSSharedArray();
}

// must use 'double' as return type, for sort result may double.
// let arr = [1,2,3,4,5,6]; arr.sort(() => Math.random() - 0.5);
double ArrayHelper::SortCompare(JSThread *thread, const JSHandle<JSTaggedValue> &callbackfnHandle,
                                const JSHandle<JSTaggedValue> &valueX, const JSHandle<JSTaggedValue> &valueY)
{
    // 1. If x and y are both undefined, return +0.
    if (valueX->IsHole()) {
        if (valueY->IsHole()) {
            return 0;
        }
        return 1;
    }
    if (valueY->IsHole()) {
        return -1;
    }
    if (valueX->IsUndefined()) {
        if (valueY->IsUndefined()) {
            return 0;
        }
        // 2. If x is undefined, return 1.
        return 1;
    }
    // 3. If y is undefined, return -1.
    if (valueY->IsUndefined()) {
        return -1;
    }
    // 4. If the argument comparefn is not undefined, then
    // a. Let v be ToNumber(Call(comparefn, undefined, «x, y»)).
    // b. ReturnIfAbrupt(v).
    // c. If v is NaN, return +0.
    // d. Return v.
    if (!callbackfnHandle->IsUndefined() && !callbackfnHandle->IsNull()) {
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackfnHandle, undefined, undefined, 2); // 2: «x, y»
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
        info->SetCallArg(valueX.GetTaggedValue(), valueY.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
        if (callResult.IsInt()) {
            return callResult.GetInt();
        }
        JSHandle<JSTaggedValue> testResult(thread, callResult);
        JSTaggedNumber v = JSTaggedValue::ToNumber(thread, testResult);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
        double value = v.GetNumber();
        if (std::isnan(value)) {
            return +0;
        }
        return value;
    }
    // 5. Let xString be ToString(x).
    // 6. ReturnIfAbrupt(xString).
    // 7. Let yString be ToString(y).
    // 8. ReturnIfAbrupt(yString).
    // 9. If xString < yString, return -1.
    // 10. If xString > yString, return 1.
    // 11. Return +0.
    if (valueX->IsInt() && valueY->IsInt()) {
        return JSTaggedValue::IntLexicographicCompare(valueX.GetTaggedValue(), valueY.GetTaggedValue());
    }
    if (valueX->IsString() && valueY->IsString()) {
        return EcmaStringAccessor::Compare(thread->GetEcmaVM(),
            JSHandle<EcmaString>(valueX), JSHandle<EcmaString>(valueY));
    }
    JSHandle<JSTaggedValue> xValueHandle(JSTaggedValue::ToString(thread, valueX));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    JSHandle<JSTaggedValue> yValueHandle(JSTaggedValue::ToString(thread, valueY));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    ComparisonResult compareResult = JSTaggedValue::Compare(thread, xValueHandle, yValueHandle);
    if (compareResult == ComparisonResult::GREAT) {
        return 1;
    }
    if (compareResult == ComparisonResult::LESS) {
        return -1;
    }
    return 0;
}

double ArrayHelper::StringSortCompare(JSThread *thread, const JSHandle<JSTaggedValue> &valueX,
                                      const JSHandle<JSTaggedValue> &valueY)
{
    ASSERT(valueX->IsString());
    ASSERT(valueY->IsString());
    // 9. If xString < yString, return -1.
    // 10. If xString > yString, return 1.
    // 11. Return +0.
    auto xHandle = JSHandle<EcmaString>(valueX);
    auto yHandle = JSHandle<EcmaString>(valueY);
    int result = EcmaStringAccessor::Compare(thread->GetEcmaVM(), xHandle, yHandle);
    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }
    return 0;
}

int64_t ArrayHelper::GetLength(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle)
{
    if (thisHandle->IsJSArray()) {
        return JSArray::Cast(thisHandle->GetTaggedObject())->GetArrayLength();
    }
    if (thisHandle->IsJSSharedArray()) {
        return JSSharedArray::Cast(thisHandle->GetTaggedObject())->GetArrayLength();
    }
    if (thisHandle->IsTypedArray() || thisHandle->IsSharedTypedArray()) {
        return JSHandle<JSTypedArray>::Cast(thisHandle)->GetArrayLength();
    }
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> lenResult = JSTaggedValue::GetProperty(thread, thisHandle, lengthKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    JSTaggedNumber len = JSTaggedValue::ToLength(thread, lenResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    return len.GetNumber();
}

int64_t ArrayHelper::GetArrayLength(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle)
{
    if (thisHandle->IsJSArray()) {
        return JSArray::Cast(thisHandle->GetTaggedObject())->GetArrayLength();
    }
    if (thisHandle->IsJSSharedArray()) {
        return JSSharedArray::Cast(thisHandle->GetTaggedObject())->GetArrayLength();
    }
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> lenResult = JSTaggedValue::GetProperty(thread, thisHandle, lengthKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    JSTaggedNumber len = JSTaggedValue::ToLength(thread, lenResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    return len.GetNumber();
}

JSTaggedValue ArrayHelper::FlattenIntoArray(JSThread *thread, const JSHandle<JSObject> &newArrayHandle,
                                            const JSHandle<JSTaggedValue> &thisObjVal, const FlattenArgs &args,
                                            const JSHandle<JSTaggedValue> &mapperFunctionHandle,
                                            const JSHandle<JSTaggedValue> &thisArg)
{
    if (thread->DoStackLimitCheck()) {
        return JSTaggedValue::Exception();
    }
    // 1. Assert: Type(target) is Object.
    // 2. Assert: Type(source) is Object.
    // 3. Assert: If mapperFunction is present, then ! IsCallable(mapperFunction) is true,
    //    thisArg is present, and depth is 1.
    ASSERT(mapperFunctionHandle->IsUndefined() || mapperFunctionHandle->IsCallable() ||
           (!thisArg->IsUndefined() && args.depth == 1));
    // 4. Let targetIndex be start.
    // 5. Let sourceIndex be +0!.
    FlattenArgs tempArgs;
    tempArgs.start = args.start;
    int64_t sourceIndex = 0;
    JSMutableHandle<JSTaggedValue> p(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> targetIndexHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> sourceIndexHandle(thread, JSTaggedValue::Undefined());
    JSHandle<EcmaString> sourceIndexStr;
    // 6. Repeat, while (sourceIndex) < sourceLen,
    //     a. Let P be ! ToString(sourceIndex).
    //     b. Let exists be ? HasProperty(source, P).
    //     c. If exists is true, then
    //         i. Let element be ? Get(source, P).
    //     ii. If mapperFunction is present, then
    //             1. Set element to ? Call(mapperFunction, thisArg, « element, sourceIndex, source »).
    //     iii. Let shouldFlatten be false.
    //     iv. If depth > 0, then
    //             1. Set shouldFlatten to ? IsArray(element).
    //         v. If shouldFlatten is true, then
    //             1. If depth is +∞, let newDepth be +∞.
    //             2. Else, let newDepth be depth - 1.
    //             3. Let elementLen be ? LengthOfArrayLike(element).
    //             4. Set targetIndex to ? FlattenIntoArray(target, element, elementLen, targetIndex, newDepth).
    //     vi. Else,
    //             1. If targetIndex ≥ 2^53 - 1, throw a TypeError exception.
    //             2. Perform ? CreateDataPropertyOrThrow(target, ! ToString(!(targetIndex)), element).
    //             3. Set targetIndex to targetIndex + 1.
    //     d. Set sourceIndex to sourceIndex + 1!.
    while (sourceIndex < args.sourceLen) {
        sourceIndexHandle.Update(JSTaggedValue(sourceIndex));
        sourceIndexStr = JSTaggedValue::ToString(thread, sourceIndexHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        p.Update(sourceIndexStr.GetTaggedValue());
        bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, p);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            element.Update(JSArray::FastGetPropertyByValue(thread, thisObjVal, p).GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (!mapperFunctionHandle->IsUndefined()) {
                const int32_t argsLength = 3; // 3: « element, sourceIndex, source »
                JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
                EcmaRuntimeCallInfo *info =
                    EcmaInterpreter::NewRuntimeCallInfo(thread, mapperFunctionHandle, thisArg, undefined, argsLength);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                info->SetCallArg(element.GetTaggedValue(), p.GetTaggedValue(), thisObjVal.GetTaggedValue());
                JSTaggedValue obj = JSFunction::Call(info);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                element.Update(obj);
            }
            bool shouldFlatten = false;
            if (args.depth > 0) {
                shouldFlatten = element->IsArray(thread);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            if (shouldFlatten) {
                tempArgs.depth = args.depth > POSITIVE_INFINITY ? POSITIVE_INFINITY : args.depth - 1;
                tempArgs.sourceLen = ArrayHelper::GetLength(thread, element);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                JSTaggedValue TargetIndexObj = FlattenIntoArray(thread, newArrayHandle, element, tempArgs,
                                                                thread->GlobalConstants()->GetHandledUndefined(),
                                                                thread->GlobalConstants()->GetHandledUndefined());
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                targetIndexHandle.Update(TargetIndexObj);
                JSTaggedNumber targetIndexTemp = JSTaggedValue::ToNumber(thread, targetIndexHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                tempArgs.start = base::NumberHelper::TruncateDouble(targetIndexTemp.GetNumber());
            } else {
                if (tempArgs.start > base::MAX_SAFE_INTEGER) {
                    THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
                }
                sourceIndexHandle.Update(JSTaggedValue(tempArgs.start));
                sourceIndexStr = JSTaggedValue::ToString(thread, sourceIndexHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                targetIndexHandle.Update(sourceIndexStr.GetTaggedValue());
                JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, targetIndexHandle, element);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                tempArgs.start++;
            }
        }
        sourceIndex++;
    }
    // 7. Return targetIndex.
    return BuiltinsBase::GetTaggedDouble(tempArgs.start);
}

JSHandle<TaggedArray> ArrayHelper::SortIndexedProperties(JSThread *thread, const JSHandle<JSTaggedValue> &thisObj,
                                                         int64_t len, const JSHandle<JSTaggedValue> &callbackFnHandle,
                                                         HolesType holes)
{
    // 1. Let items be a new empty List.
    JSHandle<TaggedArray> items(thread->GetEcmaVM()->GetFactory()->EmptyArray());
    CVector<JSHandle<JSTaggedValue>> itemsVector;
    // 2. Let k be 0.
    int64_t k = 0;
    // 3. Repeat, while k < len,
    //     a. Let Pk be ! ToString(𝔽(k)).
    //     b. If holes is skip-holes, then
    //         i. Let kRead be ? HasProperty(obj, Pk).
    //     c. Else,
    //         i. Assert: holes is read-through-holes.
    //         ii. Let kRead be true.
    //     d. If kRead is true, then
    //         i. Let kValue be ? Get(obj, Pk).
    //         ii. Append kValue to items.
    //     e. Set k to k + 1.
    bool kRead = false;
    JSMutableHandle<JSTaggedValue> pk(thread, JSTaggedValue::Undefined());

    while (k < len) {
        if (holes == HolesType::SKIP_HOLES) {
            pk.Update(JSTaggedValue(k));
            kRead = JSTaggedValue::HasProperty(thread, thisObj, pk);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, items);
        } else {
            ASSERT(holes == HolesType::READ_THROUGH_HOLES);
            kRead = true;
        }
        if (kRead) {
            JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObj, k);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, items);
            itemsVector.push_back(kValue);
        }
        ++k;
    }
    items = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(itemsVector.size());
    for (size_t i = 0; i < itemsVector.size(); ++i) {
        items->Set(thread, i, itemsVector[i].GetTaggedValue());
    }
    // 4. Sort items using an implementation-defined sequence of calls to SortCompare.
    // If any such call returns an abrupt completion,
    // stop before performing any further calls to SortCompare and return that Completion Record.
    TimSort::Sort(thread, items, callbackFnHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, items);
    // 5. Return items.
    return items;
}
}  // namespace panda::ecmascript::base
