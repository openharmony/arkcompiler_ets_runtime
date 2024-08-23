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

#include "ecmascript/base/json_stringifier.h"

#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/object_fast_operator-inl.h"

namespace panda::ecmascript::base {
constexpr int GAP_MAX_LEN = 10;
using TransformType = base::JsonHelper::TransformType;

JSHandle<JSTaggedValue> JsonStringifier::Stringify(const JSHandle<JSTaggedValue> &value,
                                                   const JSHandle<JSTaggedValue> &replacer,
                                                   const JSHandle<JSTaggedValue> &gap)
{
    factory_ = thread_->GetEcmaVM()->GetFactory();
    handleValue_ = JSMutableHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
    handleKey_ = JSMutableHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
    // Let isArray be IsArray(replacer).
    bool isArray = replacer->IsArray(thread_);
    // ReturnIfAbrupt(isArray).
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    // If isArray is true, then
    if (isArray) {
        uint32_t len = 0;
        if (replacer->IsJSArray()) {
            // FastPath
            JSHandle<JSArray> arr(replacer);
            len = arr->GetArrayLength();
        } else if (replacer->IsJSSharedArray()) {
            JSHandle<JSSharedArray> arr(replacer);
            len = arr->GetArrayLength();
        } else {
            // Let len be ToLength(Get(replacer, "length")).
            JSHandle<JSTaggedValue> lengthKey = thread_->GlobalConstants()->GetHandledLengthString();
            JSHandle<JSTaggedValue> lenResult = JSTaggedValue::GetProperty(thread_, replacer, lengthKey).GetValue();
            // ReturnIfAbrupt(len).
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
            JSTaggedNumber lenNumber = JSTaggedValue::ToLength(thread_, lenResult);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
            len = lenNumber.ToUint32();
        }
        if (len > 0) {
            JSMutableHandle<JSTaggedValue> propHandle(thread_, JSTaggedValue::Undefined());
            // Repeat while k<len.
            for (uint32_t i = 0; i < len; i++) {
                // a. Let v be Get(replacer, ToString(k)).
                JSTaggedValue prop = ObjectFastOperator::FastGetPropertyByIndex(thread_, replacer.GetTaggedValue(), i);
                // b. ReturnIfAbrupt(v).
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
                /*
                 * c. Let item be undefined.
                 * d. If Type(v) is String, let item be v.
                 * e. Else if Type(v) is Number, let item be ToString(v).
                 * f. Else if Type(v) is Object, then
                 * i. If v has a [[StringData]] or [[NumberData]] internal slot, let item be ToString(v).
                 * ii. ReturnIfAbrupt(item).
                 * g. If item is not undefined and item is not currently an element of PropertyList, then
                 * i. Append item to the end of PropertyList.
                 * h. Let k be k+1.
                 */
                propHandle.Update(prop);
                if (prop.IsNumber() || prop.IsString()) {
                    AddDeduplicateProp(propHandle);
                    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
                } else if (prop.IsJSPrimitiveRef()) {
                    JSTaggedValue primitive = JSPrimitiveRef::Cast(prop.GetTaggedObject())->GetValue();
                    if (primitive.IsNumber() || primitive.IsString()) {
                        AddDeduplicateProp(propHandle);
                        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
                    }
                }
            }
        }
    }

    // If Type(space) is Object, then
    if (gap->IsJSPrimitiveRef()) {
        JSTaggedValue primitive = JSPrimitiveRef::Cast(gap->GetTaggedObject())->GetValue();
        // a. If space has a [[NumberData]] internal slot, then
        if (primitive.IsNumber()) {
            // i. Let space be ToNumber(space).
            JSTaggedNumber num = JSTaggedValue::ToNumber(thread_, gap);
            // ii. ReturnIfAbrupt(space).
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
            CalculateNumberGap(num);
        } else if (primitive.IsString()) {
            // b. Else if space has a [[StringData]] internal slot, then
            // i. Let space be ToString(space).
            auto str = JSTaggedValue::ToString(thread_, gap);
            // ii. ReturnIfAbrupt(space).
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
            CalculateStringGap(JSHandle<EcmaString>(thread_, str.GetTaggedValue()));
        }
    } else if (gap->IsNumber()) {
        // If Type(space) is Number
        CalculateNumberGap(gap.GetTaggedValue());
    } else if (gap->IsString()) {
        // Else if Type(space) is String
        CalculateStringGap(JSHandle<EcmaString>::Cast(gap));
    }

    JSHandle<JSTaggedValue> key(factory_->GetEmptyString());
    JSTaggedValue serializeValue = GetSerializeValue(value, key, value, replacer);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    handleValue_.Update(serializeValue);
    JSTaggedValue result = SerializeJSONProperty(handleValue_, replacer);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    if (!result.IsUndefined()) {
        return JSHandle<JSTaggedValue>(
            factory_->NewFromUtf8Literal(reinterpret_cast<const uint8_t *>(result_.c_str()), result_.size()));
    }
    return thread_->GlobalConstants()->GetHandledUndefined();
}

void JsonStringifier::AddDeduplicateProp(const JSHandle<JSTaggedValue> &property)
{
    JSHandle<EcmaString> primString = JSTaggedValue::ToString(thread_, property);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    JSHandle<JSTaggedValue> addVal(thread_, *primString);

    uint32_t propLen = propList_.size();
    for (uint32_t i = 0; i < propLen; i++) {
        if (JSTaggedValue::SameValue(propList_[i], addVal)) {
            return;
        }
    }
    propList_.emplace_back(addVal);
}

bool JsonStringifier::CalculateNumberGap(JSTaggedValue gap)
{
    double numValue = gap.GetNumber();
    int num = static_cast<int>(numValue);
    if (num > 0) {
        int gapLength = std::min(num, GAP_MAX_LEN);
        gap_.append(gapLength, ' ');
        gap_.append("\0");
    }
    return true;
}

bool JsonStringifier::CalculateStringGap(const JSHandle<EcmaString> &primString)
{
    CString gapString = ConvertToString(*primString, StringConvertedUsage::LOGICOPERATION);
    uint32_t gapLen = gapString.length();
    if (gapLen > 0) {
        uint32_t gapLength = gapLen;
        if (gapLen > GAP_MAX_LEN) {
            if (gapString.at(GAP_MAX_LEN - 1) == static_cast<char>(utf_helper::UTF8_2B_FIRST)) {
                gapLength = GAP_MAX_LEN + 1;
            } else {
                gapLength = GAP_MAX_LEN;
            }
        }
        gap_.append(gapString.c_str(), gapLength);
        gap_.append("\0");
    }
    return true;
}

JSTaggedValue JsonStringifier::GetSerializeValue(const JSHandle<JSTaggedValue> &object,
                                                 const JSHandle<JSTaggedValue> &key,
                                                 const JSHandle<JSTaggedValue> &value,
                                                 const JSHandle<JSTaggedValue> &replacer)
{
    JSTaggedValue tagValue = value.GetTaggedValue();
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    // If Type(value) is Object, then
    if (value->IsECMAObject() || value->IsBigInt()) {
        // a. Let toJSON be Get(value, "toJSON").
        JSHandle<JSTaggedValue> toJson = thread_->GlobalConstants()->GetHandledToJsonString();
        JSHandle<JSTaggedValue> toJsonFun(
            thread_, ObjectFastOperator::FastGetPropertyByValue(thread_, tagValue, toJson.GetTaggedValue()));
        // b. ReturnIfAbrupt(toJSON).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
        tagValue = value.GetTaggedValue();
        // c. If IsCallable(toJSON) is true
        if (UNLIKELY(toJsonFun->IsCallable())) {
            // Let value be Call(toJSON, value, «key»).
            EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread_, toJsonFun, value, undefined, 1);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
            info->SetCallArg(key.GetTaggedValue());
            tagValue = JSFunction::Call(info);
            // ii. ReturnIfAbrupt(value).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
        }
    }

    if (UNLIKELY(replacer->IsCallable())) {
        handleValue_.Update(tagValue);
        // a. Let value be Call(ReplacerFunction, holder, «key, value»).
        const uint32_t argsLength = 2; // 2: «key, value»
        JSHandle<JSTaggedValue> holder = SerializeHolder(object, value);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread_, replacer, holder, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
        info->SetCallArg(key.GetTaggedValue(), handleValue_.GetTaggedValue());
        tagValue = JSFunction::Call(info);
        // b. ReturnIfAbrupt(value).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
    }
    return tagValue;
}

JSHandle<JSTaggedValue> JsonStringifier::SerializeHolder(const JSHandle<JSTaggedValue> &object,
                                                         const JSHandle<JSTaggedValue> &value)
{
    if (stack_.size() <= 0) {
        JSHandle<JSObject> holder = factory_->CreateNullJSObject();
        JSHandle<JSTaggedValue> holderKey(factory_->GetEmptyString());
        JSObject::CreateDataPropertyOrThrow(thread_, holder, holderKey, value);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
        return JSHandle<JSTaggedValue>(holder);
    }
    return object;
}

JSTaggedValue JsonStringifier::SerializeJSONProperty(const JSHandle<JSTaggedValue> &value,
                                                     const JSHandle<JSTaggedValue> &replacer)
{
    JSTaggedValue tagValue = value.GetTaggedValue();
    if (!tagValue.IsHeapObject()) {
        JSTaggedType type = tagValue.GetRawData();
        switch (type) {
            // If value is false, return "false".
            case JSTaggedValue::VALUE_FALSE:
                result_ += "false";
                return tagValue;
            // If value is true, return "true".
            case JSTaggedValue::VALUE_TRUE:
                result_ += "true";
                return tagValue;
            // If value is null, return "null".
            case JSTaggedValue::VALUE_NULL:
                result_ += "null";
                return tagValue;
            default:
                // If Type(value) is Number, then
                if (tagValue.IsNumber()) {
                    // a. If value is finite, return ToString(value).
                    if (std::isfinite(tagValue.GetNumber())) {
                        result_ += ConvertToString(*base::NumberHelper::NumberToString(thread_, tagValue));
                    } else {
                        // b. Else, return "null".
                        result_ += "null";
                    }
                    return tagValue;
                }
        }
    } else {
        JSType jsType = tagValue.GetTaggedObject()->GetClass()->GetObjectType();
        JSHandle<JSTaggedValue> valHandle(thread_, tagValue);
        switch (jsType) {
            case JSType::JS_ARRAY:
            case JSType::JS_SHARED_ARRAY: {
                SerializeJSArray(valHandle, replacer);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                return tagValue;
            }
            case JSType::JS_API_LINKED_LIST: {
                JSHandle listHandle = JSHandle<JSAPILinkedList>(thread_, tagValue);
                CheckStackPushSameValue(valHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                valHandle = JSHandle<JSTaggedValue>(thread_, JSAPILinkedList::ConvertToArray(thread_, listHandle));
                SerializeJSONObject(valHandle, replacer);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                return tagValue;
            }
            case JSType::JS_API_LIST: {
                JSHandle listHandle = JSHandle<JSAPIList>(thread_, tagValue);
                CheckStackPushSameValue(valHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                valHandle = JSHandle<JSTaggedValue>(thread_, JSAPIList::ConvertToArray(thread_, listHandle));
                SerializeJSONObject(valHandle, replacer);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                return tagValue;
            }
            // If Type(value) is String, return QuoteJSONString(value).
            case JSType::LINE_STRING:
            case JSType::CONSTANT_STRING:
            case JSType::TREE_STRING:
            case JSType::SLICED_STRING: {
                JSHandle<EcmaString> strHandle = JSHandle<EcmaString>(valHandle);
                auto string = JSHandle<EcmaString>(thread_,
                    EcmaStringAccessor::Flatten(thread_->GetEcmaVM(), strHandle));
                CString str = ConvertToString(*string, StringConvertedUsage::LOGICOPERATION);
                JsonHelper::AppendValueToQuotedString(str, result_);
                return tagValue;
            }
            case JSType::JS_PRIMITIVE_REF: {
                SerializePrimitiveRef(valHandle);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, JSTaggedValue::Exception());
                return tagValue;
            }
            case JSType::SYMBOL:
                return JSTaggedValue::Undefined();
            case JSType::BIGINT: {
                if (transformType_ == TransformType::NORMAL) {
                    THROW_TYPE_ERROR_AND_RETURN(thread_, "cannot serialize a BigInt", JSTaggedValue::Exception());
                } else {
                    JSHandle<BigInt> thisBigint(thread_, valHandle.GetTaggedValue());
                    auto bigIntStr = BigInt::ToString(thread_, thisBigint);
                    result_ += ConvertToString(*bigIntStr);
                    return tagValue;
                }
            }
            case JSType::JS_NATIVE_POINTER: {
                result_ += "{}";
                return tagValue;
            }
            default: {
                if (!tagValue.IsCallable()) {
                    JSHClass *jsHclass = tagValue.GetTaggedObject()->GetClass();
                    if (UNLIKELY(jsHclass->IsJSProxy() &&
                        JSProxy::Cast(tagValue.GetTaggedObject())->IsArray(thread_))) {
                        SerializeJSProxy(valHandle, replacer);
                        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    } else {
                        CheckStackPushSameValue(valHandle);
                        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                        SerializeJSONObject(valHandle, replacer);
                        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    }
                    return tagValue;
                }
            }
        }
    }
    return JSTaggedValue::Undefined();
}

void JsonStringifier::SerializeObjectKey(const JSHandle<JSTaggedValue> &key, bool hasContent)
{
    CString stepBegin;
    CString stepEnd;
    if (hasContent) {
        result_ += ",";
    }
    if (!gap_.empty()) {
        stepBegin += "\n";
        stepBegin += indent_;
        stepEnd += " ";
    }
    CString str;
    if (key->IsString()) {
        str = ConvertToString(EcmaString::Cast(key->GetTaggedObject()), StringConvertedUsage::LOGICOPERATION);
    } else if (key->IsInt()) {
        str = NumberHelper::IntToString(static_cast<int32_t>(key->GetInt()));
    } else {
        str = ConvertToString(*JSTaggedValue::ToString(thread_, key), StringConvertedUsage::LOGICOPERATION);
    }
    result_ += stepBegin;
    JsonHelper::AppendValueToQuotedString(str, result_);
    result_ += ":";
    result_ += stepEnd;
}

bool JsonStringifier::PushValue(const JSHandle<JSTaggedValue> &value)
{
    uint32_t thisLen = stack_.size();

    for (uint32_t i = 0; i < thisLen; i++) {
        bool equal = JSTaggedValue::SameValue(stack_[i].GetTaggedValue(), value.GetTaggedValue());
        if (equal) {
            return true;
        }
    }

    stack_.emplace_back(value);
    return false;
}

void JsonStringifier::PopValue()
{
    stack_.pop_back();
}

bool JsonStringifier::SerializeJSONObject(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer)
{
    CString stepback = indent_;
    indent_ += gap_;

    result_ += "{";
    bool hasContent = false;

    ASSERT(!value->IsAccessor());
    JSHandle<JSObject> obj(value);
    if (!replacer->IsArray(thread_)) {
        if (UNLIKELY(value->IsJSProxy() || value->IsTypedArray())) {  // serialize proxy and typedArray
            JSHandle<TaggedArray> propertyArray = JSObject::EnumerableOwnNames(thread_, obj);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            uint32_t arrLength = propertyArray->GetLength();
            for (uint32_t i = 0; i < arrLength; i++) {
                handleKey_.Update(propertyArray->Get(i));
                JSHandle<JSTaggedValue> valueHandle = JSTaggedValue::GetProperty(thread_, value, handleKey_).GetValue();
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                JSTaggedValue serializeValue = GetSerializeValue(value, handleKey_, valueHandle, replacer);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
                    (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
                    continue;
                }
                handleValue_.Update(serializeValue);
                SerializeObjectKey(handleKey_, hasContent);
                JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                if (!res.IsUndefined()) {
                    hasContent = true;
                }
            }
        } else {
            uint32_t numOfKeys = obj->GetNumberOfKeys();
            uint32_t numOfElements = obj->GetNumberOfElements();
            if (numOfElements > 0) {
                hasContent = JsonStringifier::SerializeElements(obj, replacer, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            if (numOfKeys > 0) {
                hasContent = JsonStringifier::SerializeKeys(obj, replacer, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
        }
    } else {
        uint32_t propLen = propList_.size();
        for (uint32_t i = 0; i < propLen; i++) {
            JSTaggedValue tagVal =
                ObjectFastOperator::FastGetPropertyByValue(thread_, obj.GetTaggedValue(),
                                                           propList_[i].GetTaggedValue());
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            handleValue_.Update(tagVal);
            JSTaggedValue serializeValue = GetSerializeValue(value, propList_[i], handleValue_, replacer);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
                (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
                continue;
            }
            handleValue_.Update(serializeValue);
            SerializeObjectKey(propList_[i], hasContent);
            JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (!res.IsUndefined()) {
                hasContent = true;
            }
        }
    }
    if (hasContent && gap_.length() != 0) {
        result_ += "\n";
        result_ += stepback;
    }
    result_ += "}";
    PopValue();
    indent_ = stepback;
    return true;
}

bool JsonStringifier::SerializeJSProxy(const JSHandle<JSTaggedValue> &object, const JSHandle<JSTaggedValue> &replacer)
{
    bool isContain = PushValue(object);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value, usually caused by circular structure", true);
    }

    CString stepback = indent_;
    CString stepBegin;
    indent_ += gap_;

    if (!gap_.empty()) {
        stepBegin += "\n";
        stepBegin += indent_;
    }
    result_ += "[";
    JSHandle<JSProxy> proxy(object);
    JSHandle<JSTaggedValue> lengthKey = thread_->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> lenghHandle = JSProxy::GetProperty(thread_, proxy, lengthKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    JSTaggedNumber lenNumber = JSTaggedValue::ToLength(thread_, lenghHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    uint32_t length = lenNumber.ToUint32();
    for (uint32_t i = 0; i < length; i++) {
        handleKey_.Update(JSTaggedValue(i));
        JSHandle<JSTaggedValue> valHandle = JSProxy::GetProperty(thread_, proxy, handleKey_).GetValue();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        if (i > 0) {
            result_ += ",";
        }
        result_ += stepBegin;
        JSTaggedValue serializeValue = GetSerializeValue(object, handleKey_, valHandle, replacer);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        handleValue_.Update(serializeValue);
        JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        if (res.IsUndefined()) {
            result_ += "null";
        }
    }

    if (length > 0 && !gap_.empty()) {
        result_ += "\n";
        result_ += stepback;
    }
    result_ += "]";
    PopValue();
    indent_ = stepback;
    return true;
}

bool JsonStringifier::SerializeJSArray(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer)
{
    // If state.[[Stack]] contains value, throw a TypeError exception because the structure is cyclical.
    bool isContain = PushValue(value);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value, usually caused by circular structure", true);
    }

    CString stepback = indent_;
    CString stepBegin;
    indent_ += gap_;

    if (!gap_.empty()) {
        stepBegin += "\n";
        stepBegin += indent_;
    }
    result_ += "[";
    uint32_t len = 0;
    if (value->IsJSArray()) {
        JSHandle<JSArray> jsArr(value);
        len = jsArr->GetArrayLength();
    } else if (value->IsJSSharedArray()) {
        JSHandle<JSSharedArray> jsArr(value);
        len = jsArr->GetArrayLength();
    }
    if (len > 0) {
        for (uint32_t i = 0; i < len; i++) {
            JSTaggedValue tagVal = ObjectFastOperator::FastGetPropertyByIndex(thread_, value.GetTaggedValue(), i);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (UNLIKELY(tagVal.IsAccessor())) {
                tagVal = JSObject::CallGetter(thread_, AccessorData::Cast(tagVal.GetTaggedObject()), value);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            handleKey_.Update(JSTaggedValue(i));
            handleValue_.Update(tagVal);

            if (i > 0) {
                result_ += ",";
            }
            result_ += stepBegin;
            JSTaggedValue serializeValue = GetSerializeValue(value, handleKey_, handleValue_, replacer);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            handleValue_.Update(serializeValue);
            JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (res.IsUndefined()) {
                result_ += "null";
            }
        }

        if (!gap_.empty()) {
            result_ += "\n";
            result_ += stepback;
        }
    }

    result_ += "]";
    PopValue();
    indent_ = stepback;
    return true;
}

void JsonStringifier::SerializePrimitiveRef(const JSHandle<JSTaggedValue> &primitiveRef)
{
    JSTaggedValue primitive = JSPrimitiveRef::Cast(primitiveRef.GetTaggedValue().GetTaggedObject())->GetValue();
    if (primitive.IsString()) {
        auto priStr = JSTaggedValue::ToString(thread_, primitiveRef);
        RETURN_IF_ABRUPT_COMPLETION(thread_);
        CString str = ConvertToString(*priStr, StringConvertedUsage::LOGICOPERATION);
        JsonHelper::AppendValueToQuotedString(str, result_);
    } else if (primitive.IsNumber()) {
        auto priNum = JSTaggedValue::ToNumber(thread_, primitiveRef);
        RETURN_IF_ABRUPT_COMPLETION(thread_);
        if (std::isfinite(priNum.GetNumber())) {
            result_ += ConvertToString(*base::NumberHelper::NumberToString(thread_, priNum));
        } else {
            result_ += "null";
        }
    } else if (primitive.IsBoolean()) {
        result_ += primitive.IsTrue() ? "true" : "false";
    } else if (primitive.IsBigInt()) {
        if (transformType_ == TransformType::NORMAL) {
            THROW_TYPE_ERROR(thread_, "cannot serialize a BigInt");
        } else {
            JSHandle<BigInt> thisBigint(thread_, primitive);
            auto bigIntStr = BigInt::ToString(thread_, thisBigint);
            result_ += ConvertToString(*bigIntStr);
        }
    }
}

bool JsonStringifier::SerializeElements(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer,
                                        bool hasContent)
{
    if (!ElementAccessor::IsDictionaryMode(obj)) {
        uint32_t elementsLen = ElementAccessor::GetElementsLength(obj);
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!ElementAccessor::Get(obj, i).IsHole()) {
                handleKey_.Update(JSTaggedValue(i));
                handleValue_.Update(ElementAccessor::Get(obj, i));
                hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
        }
    } else {
        JSHandle<TaggedArray> elementsArr(thread_, obj->GetElements());
        JSHandle<NumberDictionary> numberDic(elementsArr);
        CVector<JSHandle<JSTaggedValue>> sortArr;
        int size = numberDic->Size();
        for (int hashIndex = 0; hashIndex < size; hashIndex++) {
            JSTaggedValue key = numberDic->GetKey(hashIndex);
            if (!key.IsUndefined() && !key.IsHole()) {
                PropertyAttributes attr = numberDic->GetAttributes(hashIndex);
                if (attr.IsEnumerable()) {
                    JSTaggedValue numberKey = JSTaggedValue(static_cast<uint32_t>(key.GetInt()));
                    sortArr.emplace_back(JSHandle<JSTaggedValue>(thread_, numberKey));
                }
            }
        }
        std::sort(sortArr.begin(), sortArr.end(), JsonHelper::CompareNumber);
        for (const auto &entry : sortArr) {
            JSTaggedValue entryKey = entry.GetTaggedValue();
            handleKey_.Update(entryKey);
            int index = numberDic->FindEntry(entryKey);
            if (index < 0) {
                continue;
            }
            JSTaggedValue value = numberDic->GetValue(index);
            if (UNLIKELY(value.IsAccessor())) {
                value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                             JSHandle<JSTaggedValue>(obj));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            handleValue_.Update(value);
            hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
    }
    return hasContent;
}

bool JsonStringifier::SerializeKeys(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer,
                                    bool hasContent)
{
    JSHandle<TaggedArray> propertiesArr(thread_, obj->GetProperties());
    if (!propertiesArr->IsDictionaryMode()) {
        bool hasChangedToDictionaryMode = false;
        JSHandle<JSHClass> jsHclass(thread_, obj->GetJSHClass());
        JSTaggedValue enumCache = jsHclass->GetEnumCache();
        if (JSObject::GetEnumCacheKind(thread_, enumCache) == EnumCacheKind::ONLY_OWN_KEYS) {
            JSHandle<TaggedArray> cache(thread_, enumCache);
            uint32_t length = cache->GetLength();
            uint32_t dictStart = length;
            for (uint32_t i = 0; i < length; i++) {
                JSTaggedValue key = cache->Get(i);
                if (!key.IsString()) {
                    continue;
                }
                handleKey_.Update(key);
                JSTaggedValue value;
                LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject());
                int index = JSHClass::FindPropertyEntry(thread_, *jsHclass, key);
                PropertyAttributes attr(layoutInfo->GetAttr(index));
                ASSERT(static_cast<int>(attr.GetOffset()) == index);
                value = attr.IsInlinedProps()
                        ? obj->GetPropertyInlinedPropsWithRep(static_cast<uint32_t>(index), attr)
                        : propertiesArr->Get(static_cast<uint32_t>(index) - jsHclass->GetInlinedProperties());
                if (attr.IsInlinedProps() && value.IsHole()) {
                    continue;
                }
                if (UNLIKELY(value.IsAccessor())) {
                    value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                                 JSHandle<JSTaggedValue>(obj));
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                    if (obj->GetProperties().IsDictionary()) {
                        dictStart = i;
                        handleValue_.Update(value);
                        hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
                        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                        break;
                    }
                }
                handleValue_.Update(value);
                hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            if (dictStart < length) {
                propertiesArr = JSHandle<TaggedArray>(thread_, obj->GetProperties());
                JSHandle<NameDictionary> nameDic(propertiesArr);
                for (uint32_t i = dictStart + 1;i < length; i++) {
                    JSTaggedValue key = cache->Get(i);
                    int hashIndex = nameDic->FindEntry(key);
                    PropertyAttributes attr = nameDic->GetAttributes(hashIndex);
                    if (!key.IsString() || hashIndex < 0 || !attr.IsEnumerable()) {
                        continue;
                    }
                    handleKey_.Update(key);
                    JSTaggedValue value = nameDic->GetValue(hashIndex);
                    if (UNLIKELY(value.IsAccessor())) {
                        value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                            JSHandle<JSTaggedValue>(obj));
                        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                    }
                    handleValue_.Update(value);
                    hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                }
            }
            return hasContent;
        }
        int end = static_cast<int>(jsHclass->NumberOfProps());
        if (end <= 0) {
            return hasContent;
        }
        for (int i = 0; i < end; i++) {
            LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject());
            JSTaggedValue key = layoutInfo->GetKey(i);
            if (!hasChangedToDictionaryMode) {
                if (key.IsString() && layoutInfo->GetAttr(i).IsEnumerable()) {
                    handleKey_.Update(key);
                    JSTaggedValue value;
                    int index = JSHClass::FindPropertyEntry(thread_, *jsHclass, key);
                    PropertyAttributes attr(layoutInfo->GetAttr(index));
                    ASSERT(static_cast<int>(attr.GetOffset()) == index);
                    value = attr.IsInlinedProps()
                            ? obj->GetPropertyInlinedPropsWithRep(static_cast<uint32_t>(index), attr)
                            : propertiesArr->Get(static_cast<uint32_t>(index) - jsHclass->GetInlinedProperties());
                    if (attr.IsInlinedProps() && value.IsHole()) {
                        continue;
                    }
                    if (UNLIKELY(value.IsAccessor())) {
                        value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                            JSHandle<JSTaggedValue>(obj));
                        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                    }
                    handleValue_.Update(value);
                    hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
                    if (obj->GetProperties().IsDictionary()) {
                        hasChangedToDictionaryMode = true;
                        propertiesArr = JSHandle<TaggedArray>(thread_, obj->GetProperties());
                    }
                    jsHclass = JSHandle<JSHClass>(thread_, obj->GetJSHClass());
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                }
            } else {
                    JSHandle<NameDictionary> nameDic(propertiesArr);
                    int index = nameDic->FindEntry(key);
                    if (!key.IsString()) {
                        continue;
                    }
                    PropertyAttributes attr = nameDic->GetAttributes(index);
                    if (!attr.IsEnumerable() || index < 0) {
                        continue;
                    }
                    JSTaggedValue value = nameDic->GetValue(index);
                    handleKey_.Update(key);
                    if (UNLIKELY(value.IsAccessor())) {
                        value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                            JSHandle<JSTaggedValue>(obj));
                        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                        jsHclass = JSHandle<JSHClass>(thread_, obj->GetJSHClass());
                    }
                    handleValue_.Update(value);
                    hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
        }
        return hasContent;
    }
    if (obj->IsJSGlobalObject()) {
        JSHandle<GlobalDictionary> globalDic(propertiesArr);
        int size = globalDic->Size();
        CVector<std::pair<JSHandle<JSTaggedValue>, PropertyAttributes>> sortArr;
        for (int hashIndex = 0; hashIndex < size; hashIndex++) {
            JSTaggedValue key = globalDic->GetKey(hashIndex);
            if (!key.IsString()) {
                continue;
            }
            PropertyAttributes attr = globalDic->GetAttributes(hashIndex);
            if (!attr.IsEnumerable()) {
                continue;
            }
            std::pair<JSHandle<JSTaggedValue>, PropertyAttributes> pair(JSHandle<JSTaggedValue>(thread_, key), attr);
            sortArr.emplace_back(pair);
        }
        std::sort(sortArr.begin(), sortArr.end(), JsonHelper::CompareKey);
        for (const auto &entry : sortArr) {
            JSTaggedValue entryKey = entry.first.GetTaggedValue();
            handleKey_.Update(entryKey);
            int index = globalDic->FindEntry(entryKey);
            if (index == -1) {
                continue;
            }
            JSTaggedValue value = globalDic->GetValue(index);
            if (UNLIKELY(value.IsAccessor())) {
                value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                             JSHandle<JSTaggedValue>(obj));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            handleValue_.Update(value);
            hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        return hasContent;
    }
    JSHandle<NameDictionary> nameDic(propertiesArr);
    int size = nameDic->Size();
    CVector<std::pair<JSHandle<JSTaggedValue>, PropertyAttributes>> sortArr;
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = nameDic->GetKey(hashIndex);
        if (!key.IsString()) {
            continue;
        }
        PropertyAttributes attr = nameDic->GetAttributes(hashIndex);
        if (!attr.IsEnumerable()) {
            continue;
        }
        std::pair<JSHandle<JSTaggedValue>, PropertyAttributes> pair(JSHandle<JSTaggedValue>(thread_, key), attr);
        sortArr.emplace_back(pair);
    }
    std::sort(sortArr.begin(), sortArr.end(), JsonHelper::CompareKey);
    for (const auto &entry : sortArr) {
        JSTaggedValue entryKey = entry.first.GetTaggedValue();
        handleKey_.Update(entryKey);
        int index = nameDic->FindEntry(entryKey);
        if (index < 0) {
            continue;
        }
        JSTaggedValue value = nameDic->GetValue(index);
        if (UNLIKELY(value.IsAccessor())) {
            value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                         JSHandle<JSTaggedValue>(obj));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        handleValue_.Update(value);
        hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return hasContent;
}

bool JsonStringifier::AppendJsonString(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer,
                                       bool hasContent)
{
    JSTaggedValue serializeValue = GetSerializeValue(JSHandle<JSTaggedValue>(obj), handleKey_, handleValue_, replacer);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
        (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
        return hasContent;
    }
    handleValue_.Update(serializeValue);
    SerializeObjectKey(handleKey_, hasContent);
    JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    if (!res.IsUndefined()) {
        return true;
    }
    return hasContent;
}

bool JsonStringifier::CheckStackPushSameValue(JSHandle<JSTaggedValue> value)
{
    bool isContain = PushValue(value);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value, usually caused by circular structure", true);
    }
    return false;
}

}  // namespace panda::ecmascript::base
