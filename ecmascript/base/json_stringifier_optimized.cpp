/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/base/dtoa_helper.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_set.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/tagged_hash_array.h"

namespace panda::ecmascript::base {
constexpr int GAP_MAX_LEN = 10;
using TransformType = base::JsonHelper::TransformType;

JSHandle<JSTaggedValue> JsonStringifier::Stringify(const JSHandle<JSTaggedValue> &value,
                                                   const JSHandle<JSTaggedValue> &replacer,
                                                   const JSHandle<JSTaggedValue> &gap)
{
    replacer_ = replacer;
    if (replacer->IsUndefined() && gap->IsUndefined()) {
        return StringifyInternal<true>(value, gap);
    }

    return StringifyInternal<false>(value, gap);
}

template <bool ReplacerAndGapUndefined>
JSHandle<JSTaggedValue> JsonStringifier::StringifyInternal(const JSHandle<JSTaggedValue> &value,
                                                           const JSHandle<JSTaggedValue> &gap)
{
    factory_ = thread_->GetEcmaVM()->GetFactory();
    handleValue_ = JSMutableHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
    handleKey_ = JSMutableHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());

    if constexpr (!ReplacerAndGapUndefined) {
        // Let isArray be IsArray(replacer).
        bool isArray = replacer_->IsArray(thread_);
        // ReturnIfAbrupt(isArray).
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
        // If isArray is true, then
        if (isArray) {
            uint32_t len = 0;
            if (replacer_->IsJSArray()) {
                // FastPath
                JSHandle<JSArray> arr(replacer_);
                len = arr->GetArrayLength();
            } else if (replacer_->IsJSSharedArray()) {
                JSHandle<JSSharedArray> arr(replacer_);
                len = arr->GetArrayLength();
            } else {
                // Let len be ToLength(Get(replacer, "length")).
                JSHandle<JSTaggedValue> lengthKey = thread_->GlobalConstants()->GetHandledLengthString();
                JSHandle<JSTaggedValue> lenResult =
                    JSTaggedValue::GetProperty(thread_, replacer_, lengthKey).GetValue();
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
                    JSTaggedValue prop = ObjectFastOperator::FastGetPropertyByIndex(thread_,
                        replacer_.GetTaggedValue(), i);
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
                        JSTaggedValue primitive = JSPrimitiveRef::Cast(prop.GetTaggedObject())->GetValue(thread_);
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
            JSTaggedValue primitive = JSPrimitiveRef::Cast(gap->GetTaggedObject())->GetValue(thread_);
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
    }

    JSHandle<JSTaggedValue> key(factory_->GetEmptyString());
    JSTaggedValue serializeValue = GetSerializeValue<ReplacerAndGapUndefined>(value, key, value);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    handleValue_.Update(serializeValue);
    JSTaggedValue result = SerializeJSONProperty<ReplacerAndGapUndefined>(handleValue_);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    if (!result.IsUndefined()) {
        if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
            return JSHandle<JSTaggedValue>(factory_->NewFromUtf8Literal(
                reinterpret_cast<const uint8_t *>(oneByteResult_.GetBuffer()), oneByteResult_.GetLength()));
        } else {
            return JSHandle<JSTaggedValue>(factory_->NewFromUtf16Literal(
                reinterpret_cast<const uint16_t *>(twoBytesResult_.GetBuffer()), twoBytesResult_.GetLength()));
        }
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
        if (JSTaggedValue::SameValue(thread_, propList_[i], addVal)) {
            return;
        }
    }
    propList_.emplace_back(addVal);
}

bool JsonStringifier::CalculateNumberGap(JSTaggedValue gap)
{
    double value = std::min(gap.GetNumber(), 10.0); // means GAP_MAX_LEN.
    if (value > 0) {
        int gapLength = static_cast<int>(value);
        gap_.append(gapLength, ' ');
    }
    return true;
}

bool JsonStringifier::CalculateStringGap(const JSHandle<EcmaString> &primString)
{
    if (EcmaStringAccessor(primString).IsUtf8()) {
        EcmaStringAccessor(primString).AppendToC16String(thread_, gap_);
    } else {
        ChangeEncoding();
        EcmaStringAccessor(primString).AppendToC16String(thread_, gap_);
    }
    if (gap_.size() > GAP_MAX_LEN) {
        gap_.resize(GAP_MAX_LEN);
    }
    return true;
}

template <bool ReplacerAndGapUndefined>
JSTaggedValue JsonStringifier::GetSerializeValue(const JSHandle<JSTaggedValue> &object,
                                                 const JSHandle<JSTaggedValue> &key,
                                                 const JSHandle<JSTaggedValue> &value)
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
            JSTaggedValue keyArg = key->IsNumber() ? JSTaggedValue::ToString(thread_, key).GetTaggedValue()
                : key.GetTaggedValue();
            info->SetCallArg(keyArg);
            tagValue = JSFunction::Call(info);
            // ii. ReturnIfAbrupt(value).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
        }
    }

    if constexpr (!ReplacerAndGapUndefined) {
        if (UNLIKELY(replacer_->IsCallable())) {
            handleValue_.Update(tagValue);
            // a. Let value be Call(ReplacerFunction, holder, «key, value»).
            const uint32_t argsLength = 2; // 2: «key, value»
            JSHandle<JSTaggedValue> holder = SerializeHolder(object, value);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread_, replacer_, holder, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
            JSTaggedValue keyArg = key->IsNumber() ? JSTaggedValue::ToString(thread_, key).GetTaggedValue() :
                key.GetTaggedValue();
            info->SetCallArg(keyArg, handleValue_.GetTaggedValue());
            tagValue = JSFunction::Call(info);
            // b. ReturnIfAbrupt(value).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
        }
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

template <bool ReplacerAndGapUndefined>
JSTaggedValue JsonStringifier::SerializeJSONProperty(const JSHandle<JSTaggedValue> &value)
{
    STACK_LIMIT_CHECK(thread_, JSTaggedValue::Exception());
    JSTaggedValue tagValue = value.GetTaggedValue();
    if (!tagValue.IsHeapObject()) {
        JSTaggedType type = tagValue.GetRawData();
        switch (type) {
            // If value is false, return "false".
            case JSTaggedValue::VALUE_FALSE:
                AppendLiteral("false");
                return tagValue;
            // If value is true, return "true".
            case JSTaggedValue::VALUE_TRUE:
                AppendLiteral("true");
                return tagValue;
            // If value is null, return "null".
            case JSTaggedValue::VALUE_NULL:
                AppendLiteral("null");
                return tagValue;
            default:
                // If Type(value) is Number, then
                if (tagValue.IsNumber()) {
                    // a. If value is finite, return ToString(value).
                    if (std::isfinite(tagValue.GetNumber())) {
                        AppendNumberToResult(value);
                    } else {
                        // b. Else, return "null".
                        AppendLiteral("null");
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
                SerializeJSArray<ReplacerAndGapUndefined>(valHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                return tagValue;
            }
            case JSType::JS_API_LINKED_LIST: {
                JSHandle listHandle = JSHandle<JSAPILinkedList>(thread_, tagValue);
                CheckStackPushSameValue(valHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                valHandle = JSHandle<JSTaggedValue>(thread_, JSAPILinkedList::ConvertToArray(thread_, listHandle));
                SerializeJSONObject<ReplacerAndGapUndefined>(valHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                return tagValue;
            }
            case JSType::JS_API_LIST: {
                JSHandle listHandle = JSHandle<JSAPIList>(thread_, tagValue);
                CheckStackPushSameValue(valHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                valHandle = JSHandle<JSTaggedValue>(thread_, JSAPIList::ConvertToArray(thread_, listHandle));
                SerializeJSONObject<ReplacerAndGapUndefined>(valHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                return tagValue;
            }
            case JSType::JS_API_FAST_BUFFER: {
                JSHandle bufferHandle = JSHandle<JSTaggedValue>(thread_, tagValue);
                CheckStackPushSameValue(valHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                valHandle = JSHandle<JSTaggedValue>(thread_, JSAPIFastBuffer::FromBufferToArray(thread_, bufferHandle));
                SerializeJSONObject<ReplacerAndGapUndefined>(valHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                return tagValue;
            }
            // If Type(value) is String, return QuoteJSONString(value).
            case JSType::LINE_STRING:
            case JSType::TREE_STRING:
            case JSType::SLICED_STRING:
            case JSType::CACHED_EXTERNAL_STRING: {
                JSHandle<EcmaString> strHandle = JSHandle<EcmaString>(valHandle);
                auto string = JSHandle<EcmaString>(thread_,
                    EcmaStringAccessor::Flatten(thread_->GetEcmaVM(), strHandle));
                AppendEcmaStringToResult(string);
                return tagValue;
            }
            case JSType::JS_PRIMITIVE_REF: {
                SerializePrimitiveRef<ReplacerAndGapUndefined>(valHandle);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, JSTaggedValue::Exception());
                return tagValue;
            }
            case JSType::SYMBOL:
                return JSTaggedValue::Undefined();
            case JSType::BIGINT: {
                if (transformType_ == TransformType::NORMAL) {
                    THROW_TYPE_ERROR_AND_RETURN(thread_, "cannot serialize a BigInt", JSTaggedValue::Exception());
                } else {
                    AppendBigIntToResult(valHandle);
                    return tagValue;
                }
            }
            case JSType::JS_NATIVE_POINTER: {
                AppendLiteral("{}");
                return tagValue;
            }
            case JSType::JS_SHARED_MAP: {
                if (transformType_ == TransformType::SENDABLE) {
                    CheckStackPushSameValue(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SerializeJSONSharedMap<ReplacerAndGapUndefined>(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    return tagValue;
                }
                [[fallthrough]];
            }
            case JSType::JS_MAP: {
                if (transformType_ == TransformType::SENDABLE) {
                    CheckStackPushSameValue(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SerializeJSONMap<ReplacerAndGapUndefined>(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    return tagValue;
                }
                [[fallthrough]];
            }
            case JSType::JS_SET: {
                if (transformType_ == TransformType::SENDABLE) {
                    CheckStackPushSameValue(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SerializeJSONSet<ReplacerAndGapUndefined>(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    return tagValue;
                }
                [[fallthrough]];
            }
            case JSType::JS_SHARED_SET: {
                if (transformType_ == TransformType::SENDABLE) {
                    CheckStackPushSameValue(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SerializeJSONSharedSet<ReplacerAndGapUndefined>(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    return tagValue;
                }
                [[fallthrough]];
            }
            case JSType::JS_API_HASH_MAP: {
                if (transformType_ == TransformType::SENDABLE) {
                    CheckStackPushSameValue(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SerializeJSONHashMap<ReplacerAndGapUndefined>(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    return tagValue;
                }
                [[fallthrough]];
            }
            case JSType::JS_API_HASH_SET: {
                if (transformType_ == TransformType::SENDABLE) {
                    CheckStackPushSameValue(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SerializeJSONHashSet<ReplacerAndGapUndefined>(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    return tagValue;
                }
                [[fallthrough]];
            }
            default: {
                if (!tagValue.IsCallable()) {
                    JSHClass *jsHclass = tagValue.GetTaggedObject()->GetClass();
                    if (UNLIKELY(jsHclass->IsJSProxy() &&
                        JSProxy::Cast(tagValue.GetTaggedObject())->IsArray(thread_))) {
                        SerializeJSProxy<ReplacerAndGapUndefined>(valHandle);
                        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    } else {
                        CheckStackPushSameValue(valHandle);
                        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                        SerializeJSONObject<ReplacerAndGapUndefined>(valHandle);
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
    if (hasContent) {
        AppendChar(',');
    }
    if (!gap_.empty()) {
        NewLine();
    }

    if (key->IsString()) {
        JSTaggedValue keyValue = key.GetTaggedValue();
        auto string = JSHandle<EcmaString>(thread_, key->GetTaggedObject());
        if (GetKeyCache()->TryGetCachedKey(keyValue)) {
            const EcmaString* strPtr = string.GetObject<EcmaString>();
            if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
                AppendFastPathKeyString(strPtr, oneByteResult_);
            } else {
                AppendFastPathKeyString(strPtr, twoBytesResult_);
            }
        } else {
            if (AppendEcmaStringToResult(string)) {
                GetKeyCache()->CacheKey(keyValue);
            }
        }
    } else if (key->IsInt()) {
        AppendChar('\"');
        AppendIntToResult(static_cast<int32_t>(key->GetInt()));
        AppendChar('\"');
    } else {
        auto str = JSTaggedValue::ToString(thread_, key);
        AppendEcmaStringToResult(str);
    }

    AppendChar(':');
    if (!gap_.empty()) {
        AppendChar(' ');
    }
}

bool JsonStringifier::PushValue(const JSHandle<JSTaggedValue> &value)
{
    uint32_t thisLen = stack_.size();

    for (uint32_t i = 0; i < thisLen; i++) {
        bool equal = JSTaggedValue::SameValue(thread_, stack_[i].GetTaggedValue(), value.GetTaggedValue());
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

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeJSONObject(const JSHandle<JSTaggedValue> &value)
{
    Indent();
    AppendChar('{');

    bool hasContent = false;
    ASSERT(!value->IsAccessor());
    JSHandle<JSObject> obj(value);

    if constexpr (!ReplacerAndGapUndefined) {
        if (!replacer_->IsArray(thread_)) {
            hasContent = SerializeObjectProperties<ReplacerAndGapUndefined>(obj, hasContent);
        } else {
            hasContent = SerializeObjectWithReplacerArray<ReplacerAndGapUndefined>(obj, hasContent);
        }
    } else {
        hasContent = SerializeObjectProperties<ReplacerAndGapUndefined>(obj, hasContent);
    }
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);

    Unindent();
    if (hasContent && gap_.length() != 0) {
        NewLine();
    }
    AppendChar('}');
    PopValue();
    return true;
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeObjectProperties(const JSHandle<JSObject> &obj,
                                                bool hasContent)
{
    if (UNLIKELY(obj->IsJSProxy() || obj->IsTypedArray())) {
        // serialize proxy and typedArray
        JSHandle<TaggedArray> propertyArray = JSObject::EnumerableOwnNames(thread_, obj);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        uint32_t arrLength = propertyArray->GetLength();
        for (uint32_t i = 0; i < arrLength; i++) {
            handleKey_.Update(propertyArray->Get(thread_, i));
            JSHandle<JSTaggedValue> valueHandle =
                JSTaggedValue::GetProperty(thread_, JSHandle<JSTaggedValue>(obj), handleKey_).GetValue();
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            hasContent = ProcessSerializedProperty<ReplacerAndGapUndefined>(JSHandle<JSTaggedValue>(obj),
                handleKey_, valueHandle, hasContent);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
    } else {
        uint32_t numOfKeys = obj->GetNumberOfKeys(thread_);
        uint32_t numOfElements = obj->GetNumberOfElements(thread_);
        if (numOfElements > 0) {
            hasContent = JsonStringifier::SerializeElements<ReplacerAndGapUndefined>(obj, hasContent);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        if (numOfKeys > 0) {
            hasContent = JsonStringifier::SerializeKeys<ReplacerAndGapUndefined>(obj, hasContent);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
    }
    return hasContent;
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeObjectWithReplacerArray(const JSHandle<JSObject> &obj,
                                                       bool hasContent)
{
    uint32_t propLen = propList_.size();
    for (uint32_t i = 0; i < propLen; i++) {
        JSTaggedValue tagVal =
            ObjectFastOperator::FastGetPropertyByValue(thread_, obj.GetTaggedValue(),
                                                       propList_[i].GetTaggedValue());
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        handleValue_.Update(tagVal);
        hasContent = ProcessSerializedProperty<ReplacerAndGapUndefined>(JSHandle<JSTaggedValue>(obj),
            propList_[i], handleValue_, hasContent);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return hasContent;
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::ProcessSerializedProperty(const JSHandle<JSTaggedValue> &object,
                                                const JSHandle<JSTaggedValue> &key,
                                                const JSHandle<JSTaggedValue> &value,
                                                bool hasContent)
{
    JSTaggedValue serializeValue = GetSerializeValue<ReplacerAndGapUndefined>(object, key, value);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
        (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
        return hasContent;
    }
    handleValue_.Update(serializeValue);
    SerializeObjectKey(key, hasContent);
    JSTaggedValue res = SerializeJSONProperty<ReplacerAndGapUndefined>(handleValue_);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    if (!res.IsUndefined()) {
        return true;
    }
    return hasContent;
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeJSONSharedMap(const JSHandle<JSTaggedValue> &value)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap> scope(thread_, value);
    JSHandle<JSSharedMap> sharedMap(value);
    JSHandle<LinkedHashMap> hashMap(thread_, sharedMap->GetLinkedMap(thread_));
    return SerializeLinkedHashMap<ReplacerAndGapUndefined>(hashMap);
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeJSONSharedSet(const JSHandle<JSTaggedValue> &value)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedSet> scope(thread_, value);
    JSHandle<JSSharedSet> sharedSet(value);
    JSHandle<LinkedHashSet> hashSet(thread_, sharedSet->GetLinkedSet(thread_));
    return SerializeLinkedHashSet<ReplacerAndGapUndefined>(hashSet);
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeJSONMap(const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSMap> jsMap(value);
    JSHandle<LinkedHashMap> hashMap(thread_, jsMap->GetLinkedMap(thread_));
    return SerializeLinkedHashMap<ReplacerAndGapUndefined>(hashMap);
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeJSONSet(const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSSet> jsSet(value);
    JSHandle<LinkedHashSet> hashSet(thread_, jsSet->GetLinkedSet(thread_));
    return SerializeLinkedHashSet<ReplacerAndGapUndefined>(hashSet);
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeJSONHashMap(const JSHandle<JSTaggedValue> &value)
{
    AppendChar('{');
    JSHandle<JSAPIHashMap> hashMap(value);
    JSHandle<TaggedHashArray> table(thread_, hashMap->GetTable(thread_));
    uint32_t len = table->GetLength();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedQueue> queue(thread_, factory->NewTaggedQueue(0));
    JSMutableHandle<TaggedNode> node(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> keyHandle(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueHandle(thread_, JSTaggedValue::Undefined());
    uint32_t index = 0;
    bool needRemove = false;
    while (index < len) {
        node.Update(TaggedHashArray::GetCurrentNode(thread_, queue, table, index));
        if (node.GetTaggedValue().IsHole()) {
            continue;
        }
        keyHandle.Update(node->GetKey(thread_));
        valueHandle.Update(node->GetValue(thread_));
        if (valueHandle->IsUndefined()) {
            continue;
        }
        if (UNLIKELY(!keyHandle->IsString())) {
            AppendChar('"');
            SerializeJSONProperty<ReplacerAndGapUndefined>(keyHandle);
            AppendChar('"');
        } else {
            SerializeJSONProperty<ReplacerAndGapUndefined>(keyHandle);
        }
        AppendChar(':');
        SerializeJSONProperty<ReplacerAndGapUndefined>(valueHandle);
        AppendChar(',');
        needRemove = true;
    }
    if (needRemove) {
        PopBack();
    }
    AppendChar('}');
    PopValue();
    return true;
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeJSONHashSet(const JSHandle<JSTaggedValue> &value)
{
    AppendChar('[');
    JSHandle<JSAPIHashSet> hashSet(value);
    JSHandle<TaggedHashArray> table(thread_, hashSet->GetTable(thread_));
    uint32_t len = table->GetLength();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedQueue> queue(thread_, factory->NewTaggedQueue(0));
    JSMutableHandle<TaggedNode> node(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> currentKey(thread_, JSTaggedValue::Undefined());
    uint32_t index = 0;
    bool needRemove = false;
    while (index < len) {
        node.Update(TaggedHashArray::GetCurrentNode(thread_, queue, table, index));
        if (node.GetTaggedValue().IsHole()) {
            continue;
        }
        currentKey.Update(node->GetKey(thread_));
        JSTaggedValue res = SerializeJSONProperty<ReplacerAndGapUndefined>(currentKey);
        if (res.IsUndefined()) {
            AppendLiteral("null");
        }
        AppendChar(',');
        needRemove = true;
    }
    if (needRemove) {
        PopBack();
    }
    AppendChar(']');
    PopValue();
    return true;
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeLinkedHashMap(const JSHandle<LinkedHashMap> &hashMap)
{
    AppendChar('{');
    int index = 0;
    int totalElements = hashMap->NumberOfElements() + hashMap->NumberOfDeletedElements();
    JSMutableHandle<JSTaggedValue> keyHandle(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valHandle(thread_, JSTaggedValue::Undefined());
    bool needRemove = false;
    while (index < totalElements) {
        keyHandle.Update(hashMap->GetKey(thread_, index++));
        valHandle.Update(hashMap->GetValue(thread_, index - 1));
        if (keyHandle->IsHole() || valHandle->IsUndefined()) {
            continue;
        }
        if (UNLIKELY(keyHandle->IsUndefined())) {
            AppendLiteral("\"undefined\"");
        } else if (UNLIKELY(!keyHandle->IsString())) {
            AppendLiteral("\"");
            SerializeJSONProperty<ReplacerAndGapUndefined>(keyHandle);
            AppendLiteral("\"");
        } else {
            SerializeJSONProperty<ReplacerAndGapUndefined>(keyHandle);
        }
        AppendChar(':');
        SerializeJSONProperty<ReplacerAndGapUndefined>(valHandle);
        AppendChar(',');
        needRemove = true;
    }
    if (needRemove) {
        PopBack();
    }
    AppendChar('}');
    PopValue();
    return true;
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeLinkedHashSet(const JSHandle<LinkedHashSet> &hashSet)
{
    AppendChar('[');
    JSMutableHandle<JSTaggedValue> keyHandle(thread_, JSTaggedValue::Undefined());
    bool needRemove = false;

    int index = 0;
    int totalElements = hashSet->NumberOfElements() + hashSet->NumberOfDeletedElements();
    while (index < totalElements) {
        keyHandle.Update(hashSet->GetKey(thread_, index++));
        if (keyHandle->IsHole()) {
            continue;
        }
        JSTaggedValue res = SerializeJSONProperty<ReplacerAndGapUndefined>(keyHandle);
        if (res.IsUndefined()) {
            AppendLiteral("null");
        }
        AppendChar(',');
        needRemove = true;
    }
    if (needRemove) {
        PopBack();
    }
    AppendChar(']');
    PopValue();
    return true;
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeJSProxy(const JSHandle<JSTaggedValue> &object)
{
    bool isContain = PushValue(object);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value, usually caused by circular structure", true);
    }

    Indent();

    AppendChar('[');
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
            AppendChar(',');
        }
        if (!gap_.empty()) {
            NewLine();
        }
        JSTaggedValue serializeValue = GetSerializeValue<ReplacerAndGapUndefined>(object, handleKey_, valHandle);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        handleValue_.Update(serializeValue);
        JSTaggedValue res = SerializeJSONProperty<ReplacerAndGapUndefined>(handleValue_);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        if (res.IsUndefined()) {
            AppendLiteral("null");
        }
    }

    if (length > 0 && !gap_.empty()) {
        NewLine();
    }
    AppendChar(']');
    PopValue();
    Unindent();
    return true;
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeJSArray(const JSHandle<JSTaggedValue> &value)
{
    // If state.[[Stack]] contains value, throw a TypeError exception because the structure is cyclical.
    bool isContain = PushValue(value);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value, usually caused by circular structure", true);
    }

    AppendChar('[');
    uint32_t len = 0;
    if (value->IsJSArray()) {
        JSHandle<JSArray> jsArr(value);
        len = jsArr->GetArrayLength();
    } else if (value->IsJSSharedArray()) {
        JSHandle<JSSharedArray> jsArr(value);
        len = jsArr->GetArrayLength();
    }
    if (len > 0) {
        Indent();
        JSMutableHandle<JSTaggedValue> tagValHandle(thread_, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < len; i++) {
            JSTaggedValue tagVal = ObjectFastOperator::FastGetPropertyByIndex(thread_, value.GetTaggedValue(), i);
            tagValHandle.Update(tagVal);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (UNLIKELY(tagVal.IsAccessor())) {
                JSTaggedValue getterResult = JSObject::CallGetter(
                    thread_, AccessorData::Cast(tagVal.GetTaggedObject()), value);
                tagValHandle.Update(getterResult);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            handleKey_.Update(JSTaggedValue(i));
            handleValue_.Update(tagValHandle.GetTaggedValue());

            if (i > 0) {
                AppendChar(',');
            }
            if (!gap_.empty()) {
                NewLine();
            }
            JSTaggedValue serializeValue = GetSerializeValue<ReplacerAndGapUndefined>(value, handleKey_, handleValue_);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            handleValue_.Update(serializeValue);
            JSTaggedValue res = SerializeJSONProperty<ReplacerAndGapUndefined>(handleValue_);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (res.IsUndefined()) {
                AppendLiteral("null");
            }
        }

        if (!gap_.empty()) {
            Unindent();
            NewLine();
        }
    }

    AppendChar(']');
    PopValue();
    return true;
}

template <bool ReplacerAndGapUndefined>
void JsonStringifier::SerializePrimitiveRef(const JSHandle<JSTaggedValue> &primitiveRef)
{
    JSTaggedValue primitive = JSPrimitiveRef::Cast(primitiveRef.GetTaggedValue().GetTaggedObject())->GetValue(thread_);
    if (primitive.IsString()) {
        auto priStr = JSTaggedValue::ToString(thread_, primitiveRef);
        RETURN_IF_ABRUPT_COMPLETION(thread_);
        AppendEcmaStringToResult(priStr);
    } else if (primitive.IsNumber()) {
        auto priNum = JSTaggedValue::ToNumber(thread_, primitiveRef);
        RETURN_IF_ABRUPT_COMPLETION(thread_);
        if (std::isfinite(priNum.GetNumber())) {
            AppendNumberToResult(JSHandle<JSTaggedValue>(thread_, priNum));
        } else {
            AppendLiteral("null");
        }
    } else if (primitive.IsBoolean()) {
        if (primitive.IsTrue()) {
            AppendLiteral("true");
        } else {
            AppendLiteral("false");
        }
    } else if (primitive.IsBigInt()) {
        if (transformType_ == TransformType::NORMAL) {
            THROW_TYPE_ERROR(thread_, "cannot serialize a BigInt");
        } else {
            auto value = JSHandle<JSTaggedValue>(thread_, primitive);
            AppendBigIntToResult(value);
        }
    } else {
        ASSERT(primitive.IsSymbol());
        CheckStackPushSameValue(primitiveRef);
        RETURN_IF_ABRUPT_COMPLETION(thread_);
        SerializeJSONObject<ReplacerAndGapUndefined>(primitiveRef);
    }
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeElements(const JSHandle<JSObject> &obj, bool hasContent)
{
    if (!ElementAccessor::IsDictionaryMode(thread_, obj)) {
        uint32_t elementsLen = ElementAccessor::GetElementsLength(thread_, obj);
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!ElementAccessor::Get(thread_, obj, i).IsHole()) {
                handleKey_.Update(JSTaggedValue(i));
                handleValue_.Update(ElementAccessor::Get(thread_, obj, i));
                hasContent = JsonStringifier::AppendJsonString<ReplacerAndGapUndefined>(obj, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
        }
    } else {
        JSHandle<TaggedArray> elementsArr(thread_, obj->GetElements(thread_));
        JSHandle<NumberDictionary> numberDic(elementsArr);
        CVector<JSHandle<JSTaggedValue>> sortArr;
        int size = numberDic->Size();
        for (int hashIndex = 0; hashIndex < size; hashIndex++) {
            JSTaggedValue key = numberDic->GetKey(thread_, hashIndex);
            if (!key.IsUndefined() && !key.IsHole()) {
                PropertyAttributes attr = numberDic->GetAttributes(thread_, hashIndex);
                if (attr.IsEnumerable()) {
                    JSTaggedValue numberKey = key.IsInt() ? JSTaggedValue(static_cast<uint32_t>(key.GetInt())) :
                                                            JSTaggedValue(key.GetDouble());
                    sortArr.emplace_back(JSHandle<JSTaggedValue>(thread_, numberKey));
                }
            }
        }
        std::sort(sortArr.begin(), sortArr.end(), JsonHelper::CompareNumber);
        for (const auto &entry : sortArr) {
            JSTaggedValue entryKey = entry.GetTaggedValue();
            handleKey_.Update(entryKey);
            int index = numberDic->FindEntry(thread_, entryKey);
            if (index < 0) {
                continue;
            }
            JSTaggedValue value = numberDic->GetValue(thread_, index);
            if (UNLIKELY(value.IsAccessor())) {
                value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                             JSHandle<JSTaggedValue>(obj));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            handleValue_.Update(value);
            hasContent = JsonStringifier::AppendJsonString<ReplacerAndGapUndefined>(obj, hasContent);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
    }
    return hasContent;
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::SerializeKeys(const JSHandle<JSObject> &obj, bool hasContent)
{
    JSHandle<TaggedArray> propertiesArr(thread_, obj->GetProperties(thread_));
    if (!propertiesArr->IsDictionaryMode()) {
        bool hasChangedToDictionaryMode = false;
        JSHandle<JSHClass> jsHClass(thread_, obj->GetJSHClass());
        if (jsHClass->GetEnumCache(thread_).IsEnumCacheOwnValid(thread_)) {
            auto cache = JSHClass::GetEnumCacheOwnWithOutCheck(thread_, jsHClass);
            uint32_t length = cache->GetLength();
            uint32_t dictStart = length;
            for (uint32_t i = 0; i < length; i++) {
                JSTaggedValue key = cache->Get(thread_, i);
                if (!key.IsString()) {
                    continue;
                }
                handleKey_.Update(key);
                JSTaggedValue value;
                LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHClass->GetLayout(thread_).GetTaggedObject());
                int index = JSHClass::FindPropertyEntry(thread_, *jsHClass, key);
                PropertyAttributes attr(layoutInfo->GetAttr(thread_, index));
                ASSERT(static_cast<int>(attr.GetOffset()) == index);
                value = attr.IsInlinedProps()
                        ? obj->GetPropertyInlinedPropsWithRep(thread_, static_cast<uint32_t>(index), attr)
                        : propertiesArr->Get(thread_, static_cast<uint32_t>(index) - jsHClass->GetInlinedProperties());
                if (attr.IsInlinedProps() && value.IsHole()) {
                    continue;
                }
                if (UNLIKELY(value.IsAccessor())) {
                    value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                                 JSHandle<JSTaggedValue>(obj));
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                    if (obj->GetProperties(thread_).IsDictionary()) {
                        dictStart = i;
                        handleValue_.Update(value);
                        hasContent = JsonStringifier::AppendJsonString<ReplacerAndGapUndefined>(obj, hasContent);
                        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                        break;
                    }
                }
                handleValue_.Update(value);
                hasContent = JsonStringifier::AppendJsonString<ReplacerAndGapUndefined>(obj, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            if (dictStart < length) {
                propertiesArr = JSHandle<TaggedArray>(thread_, obj->GetProperties(thread_));
                JSHandle<NameDictionary> nameDic(propertiesArr);
                for (uint32_t i = dictStart + 1;i < length; i++) {
                    JSTaggedValue key = cache->Get(thread_, i);
                    int hashIndex = nameDic->FindEntry(thread_, key);
                    PropertyAttributes attr = nameDic->GetAttributes(thread_, hashIndex);
                    if (!key.IsString() || hashIndex < 0 || !attr.IsEnumerable()) {
                        continue;
                    }
                    handleKey_.Update(key);
                    JSTaggedValue value = nameDic->GetValue(thread_, hashIndex);
                    if (UNLIKELY(value.IsAccessor())) {
                        value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                            JSHandle<JSTaggedValue>(obj));
                        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                    }
                    handleValue_.Update(value);
                    hasContent = JsonStringifier::AppendJsonString<ReplacerAndGapUndefined>(obj, hasContent);
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                }
            }
            return hasContent;
        }
        int end = static_cast<int>(jsHClass->NumberOfProps());
        if (end <= 0) {
            return hasContent;
        }
        for (int i = 0; i < end; i++) {
            LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHClass->GetLayout(thread_).GetTaggedObject());
            JSTaggedValue key = layoutInfo->GetKey(thread_, i);
            if (!hasChangedToDictionaryMode) {
                PropertyAttributes attr(layoutInfo->GetAttr(thread_, i));
                ASSERT(static_cast<int>(attr.GetOffset()) == i);
                if (key.IsString() && attr.IsEnumerable()) {
                    handleKey_.Update(key);
                    JSTaggedValue value = attr.IsInlinedProps()
                        ? obj->GetPropertyInlinedPropsWithRep(thread_, static_cast<uint32_t>(i), attr)
                        : propertiesArr->Get(thread_, static_cast<uint32_t>(i) - jsHClass->GetInlinedProperties());
                    if (attr.IsInlinedProps() && value.IsHole()) {
                        continue;
                    }
                    if (UNLIKELY(value.IsAccessor())) {
                        value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                            JSHandle<JSTaggedValue>(obj));
                        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                    }
                    handleValue_.Update(value);
                    hasContent = JsonStringifier::AppendJsonString<ReplacerAndGapUndefined>(obj, hasContent);
                    if (obj->GetProperties(thread_).IsDictionary()) {
                        hasChangedToDictionaryMode = true;
                        propertiesArr = JSHandle<TaggedArray>(thread_, obj->GetProperties(thread_));
                    }
                    jsHClass = JSHandle<JSHClass>(thread_, obj->GetJSHClass());
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                }
            } else {
                    JSHandle<NameDictionary> nameDic(propertiesArr);
                    int index = nameDic->FindEntry(thread_, key);
                    if (!key.IsString()) {
                        continue;
                    }
                    PropertyAttributes attr = nameDic->GetAttributes(thread_, index);
                    if (!attr.IsEnumerable() || index < 0) {
                        continue;
                    }
                    JSTaggedValue value = nameDic->GetValue(thread_, index);
                    handleKey_.Update(key);
                    if (UNLIKELY(value.IsAccessor())) {
                        value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                            JSHandle<JSTaggedValue>(obj));
                        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                        jsHClass = JSHandle<JSHClass>(thread_, obj->GetJSHClass());
                    }
                    handleValue_.Update(value);
                    hasContent = JsonStringifier::AppendJsonString<ReplacerAndGapUndefined>(obj, hasContent);
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
            JSTaggedValue key = globalDic->GetKey(thread_, hashIndex);
            if (!key.IsString()) {
                continue;
            }
            PropertyAttributes attr = globalDic->GetAttributes(thread_, hashIndex);
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
            int index = globalDic->FindEntry(thread_, entryKey);
            if (index == -1) {
                continue;
            }
            JSTaggedValue value = globalDic->GetValue(thread_, index);
            if (UNLIKELY(value.IsAccessor())) {
                value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                             JSHandle<JSTaggedValue>(obj));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            handleValue_.Update(value);
            hasContent = JsonStringifier::AppendJsonString<ReplacerAndGapUndefined>(obj, hasContent);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        return hasContent;
    }
    JSHandle<NameDictionary> nameDic(propertiesArr);
    int size = nameDic->Size();
    CVector<std::pair<JSHandle<JSTaggedValue>, PropertyAttributes>> sortArr;
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = nameDic->GetKey(thread_, hashIndex);
        if (!key.IsString()) {
            continue;
        }
        PropertyAttributes attr = nameDic->GetAttributes(thread_, hashIndex);
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
        int index = nameDic->FindEntry(thread_, entryKey);
        if (index < 0) {
            continue;
        }
        JSTaggedValue value = nameDic->GetValue(thread_, index);
        if (UNLIKELY(value.IsAccessor())) {
            value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                         JSHandle<JSTaggedValue>(obj));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        handleValue_.Update(value);
        hasContent = JsonStringifier::AppendJsonString<ReplacerAndGapUndefined>(obj, hasContent);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return hasContent;
}

template <bool ReplacerAndGapUndefined>
bool JsonStringifier::AppendJsonString(const JSHandle<JSObject> &obj, bool hasContent)
{
    JSTaggedValue serializeValue =
        GetSerializeValue<ReplacerAndGapUndefined>(JSHandle<JSTaggedValue>(obj), handleKey_, handleValue_);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
        (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
        return hasContent;
    }
    handleValue_.Update(serializeValue);
    SerializeObjectKey(handleKey_, hasContent);
    JSTaggedValue res = SerializeJSONProperty<ReplacerAndGapUndefined>(handleValue_);
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

inline void JsonStringifier::PopBack()
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        oneByteResult_.PopBack();
    } else {
        twoBytesResult_.PopBack();
    }
}

inline void JsonStringifier::EnsureCapacityFor(size_t size)
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        oneByteResult_.EnsureCapacity(size);
    } else {
        twoBytesResult_.EnsureCapacity(size);
    }
}


// Perform length validation
template <size_t N>
inline void JsonStringifier::AppendLiteral(const char(&src)[N])
{
    EnsureCapacityFor(N - 1);
    AppendStringInternal(src, N - 1);
}

// Perform length validation
inline void JsonStringifier::AppendChar(const char src)
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        oneByteResult_.EnsureCapacity(1);
        oneByteResult_.Append(src);
    } else {
        twoBytesResult_.EnsureCapacity(1);
        twoBytesResult_.Append(static_cast<char16_t>(src));
    }
}

// Perform length validation
void JsonStringifier::ChangeEncoding()
{
    encoding_ = Encoding::TWO_BYTE_ENCODING;
    size_t length = oneByteResult_.GetLength();
    twoBytesResult_.EnsureCapacity(length);

    const uint8_t* src = oneByteResult_.GetBuffer();
    uint16_t* dst = twoBytesResult_.GetBuffer();
    for (size_t i = 0; i < length; ++i) {
        dst[i] = static_cast<char16_t>(src[i]);
    }

    twoBytesResult_.SetLength(length);
}

// Perform length validation
void JsonStringifier::NewLine()
{
    size_t needed = 1 + indent_ * gap_.size();
    EnsureCapacityFor(needed);
    
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        oneByteResult_.Append('\n');
        for (int i = 0; i < indent_; ++i) {
            for (auto ch : gap_) {
                oneByteResult_.Append(static_cast<char>(ch));
            }
        }
    } else {
        twoBytesResult_.Append('\n');
        for (int i = 0; i < indent_; ++i) {
            for (auto ch : gap_) {
                twoBytesResult_.Append(ch);
            }
        }
    }
}

// Perform length validation
inline void JsonStringifier::AppendNumberToResult(const JSHandle<JSTaggedValue> &value)
{
    if (value->IsInt()) {
        EnsureCapacityFor(INT_MAX_LENGTH);
        if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
            AppendIntToFastStringBuilder(oneByteResult_, value->GetInt());
        } else {
            AppendIntToFastStringBuilder(twoBytesResult_, value->GetInt());
        }
    } else {
        EnsureCapacityFor(DOUBLE_MAX_LENGTH);
        if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
            AppendDoubleToFastStringBuilder(oneByteResult_, value->GetDouble());
        } else {
            AppendDoubleToFastStringBuilder(twoBytesResult_, value->GetDouble());
        }
    }
}

// Perform length validation
inline void JsonStringifier::AppendIntToResult(int32_t key)
{
    EnsureCapacityFor(INT_MAX_LENGTH);
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        AppendIntToFastStringBuilder(oneByteResult_, key);
    } else {
        AppendIntToFastStringBuilder(twoBytesResult_, key);
    }
}

// Skip length validation
template <typename DestChar>
void JsonStringifier::AppendIntToFastStringBuilder(FastStringBuilder<DestChar> &str, int number)
{
    size_t start = str.GetLength();
    int64_t n = static_cast<int64_t>(number);
    bool isNeg = true;
    if (n >= 0) {
        n = -n;
        isNeg = false;
    }
    do {
        str.Append(static_cast<DestChar>('0' - (n % DEC_BASE)));
        n /= DEC_BASE;
    } while (n);
    if (isNeg) {
        str.Append(static_cast<DestChar>('-'));
    }
    size_t end = str.GetLength();
    str.Reverse(start, end);
}

// Skip length validation
template <typename DestChar>
bool JsonStringifier::AppendSpecialDoubleToFastStringBuilder(FastStringBuilder<DestChar> &str, double d)
{
    if (d == 0.0) {
        str.AppendCString(base::NumberHelper::ZERO_STR.c_str());
        return true;
    }
    if (std::isnan(d)) {
        str.AppendCString(base::NumberHelper::NAN_STR.c_str());
        return true;
    }
    if (std::isinf(d)) {
        str.AppendCString((d < 0 ? base::NumberHelper::MINUS_INFINITY_STR : base::NumberHelper::INFINITY_STR).c_str());
        return true;
    }
    return false;
}

// Skip length validation
template <typename DestChar>
void JsonStringifier::AppendDoubleToFastStringBuilder(FastStringBuilder<DestChar> &str, double d)
{
    if (AppendSpecialDoubleToFastStringBuilder(str, d)) {
        return;
    }

    bool isNeg = d < 0;
    if (isNeg) {
        d = -d;
    }

    ASSERT(d > 0);
    constexpr int startIdx = 8; // when (isNeg and n = -5), result moves left for 8times
    char buff[base::JS_DTOA_BUF_SIZE] = {0};
    char *result = buff + startIdx;
    ASSERT(startIdx < base::JS_DTOA_BUF_SIZE);
    int n = 0;
    int k = 0;
    int len = 0;
    DtoaHelper::Dtoa(d, result, &n, &k);
    if (n > 0 && n <= base::MAX_DIGITS) {
        if (k <= n) {
            std::fill(result + k, result + n, '0');
            len = n;
        } else {
            --result;
            std::copy(result + 1, result + 1 + n, result);
            result[n] = '.';
            len = k + 1;
        }
    } else if (base::MIN_DIGITS < n && n <= 0) {
        constexpr int prefixLen = 2;
        result -= (-n + prefixLen);
        ASSERT(result >= buff);
        ASSERT(result + prefixLen - n <= buff + base::JS_DTOA_BUF_SIZE);
        result[0] = '0';
        result[1] = '.';
        std::fill(result + prefixLen, result + prefixLen - n, '0');
        len = -n + prefixLen + k;
    } else {
        int pos = k;
        if (k != 1) {
            --result;
            result[0] = result[1];
            result[1] = '.';
            ++pos;
        }
        result[pos++] = 'e';
        if (n >= 1) {
            result[pos++] = '+';
        }
        auto expo = std::to_string(n - 1);
        for (size_t i = 0; i < expo.length(); ++i) {
            result[pos++] = expo[i];
        }
        len = pos;
    }
    if (isNeg) {
        --result;
        result[0] = '-';
        len += 1;
    }

    str.AppendString(result, len);
}

// Perform length validation
inline void JsonStringifier::AppendBigIntToResult(JSHandle<JSTaggedValue> &valHandle)
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        AppendBigIntToString(oneByteResult_, valHandle.GetObject<BigInt>());
    } else {
        AppendBigIntToString(twoBytesResult_, valHandle.GetObject<BigInt>());
    }
}

// Perform length validation
template <typename DestType>
void JsonStringifier::AppendBigIntToString(DestType &str, BigInt *bigint)
{
    DISALLOW_GARBAGE_COLLECTION;
    constexpr uint32_t conversionToRadix = BigInt::DECIMAL;
    CString res = BigIntHelper::Conversion(BigIntHelper::GetBinary(bigint), conversionToRadix, BINARY);
    size_t needed = res.size() + 1;
    str.EnsureCapacity(needed);
    if (bigint->GetSign()) {
        str.Append('-');
    }
    str.AppendString(res.c_str(), res.size());
}

// Perform length validation
inline bool JsonStringifier::AppendEcmaStringToResult(JSHandle<EcmaString> &string)
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        if (EcmaStringAccessor(*string).IsUtf8()) {
            return AppendQuotedStringToFastStringBuilder<uint8_t>(thread_,
                oneByteResult_, string.GetObject<EcmaString>());
        } else {
            ChangeEncoding();
            return AppendQuotedStringToFastStringBuilder<uint16_t>(thread_,
                twoBytesResult_, string.GetObject<EcmaString>());
        }
    } else {
        return AppendQuotedStringToFastStringBuilder<uint16_t>(thread_,
            twoBytesResult_, string.GetObject<EcmaString>());
    }
}

// Perform length validation
template <typename BuilderType>
inline void JsonStringifier::AppendFastPathKeyString(const EcmaString* strPtr, BuilderType &output)
{
    EcmaStringAccessor accessor(const_cast<EcmaString*>(strPtr));
    uint32_t strLen = accessor.GetLength();

    size_t maxAdditionalLength = strLen + 2;
    output.EnsureCapacity(maxAdditionalLength);
    output.Append('"');
    CVector<uint8_t> buf;
    const uint8_t* data = accessor.GetUtf8DataFlat(thread_, strPtr, buf);
    output.AppendString(reinterpret_cast<const char*>(data), strLen);
    output.Append('"');
}

// Perform length validation
template <typename DestChar>
bool JsonStringifier::AppendQuotedStringToFastStringBuilder(const JSThread *thread,
                                                            FastStringBuilder<DestChar> &builder,
                                                            const EcmaString *str)
{
    uint32_t strLen = EcmaStringAccessor(const_cast<EcmaString *>(str)).GetLength();

    // Pre-allocate space (worst case: each char becomes 6 chars with escapes, +2 for quotes)
    // For safety and speed, we use (strLen << 3)
    size_t maxAdditionalLength = strLen << 3;
    builder.EnsureCapacity(maxAdditionalLength);

    if (EcmaStringAccessor(const_cast<EcmaString *>(str)).IsUtf8()) {
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaStringAccessor::GetUtf8DataFlat(thread, str, buf);
        const common::Span<const uint8_t> dataSpan(data, strLen);
        return base::JsonHelper::AppendValueToQuotedString(dataSpan, builder);
    } else {
        CVector<uint16_t> buf;
        const uint16_t *data = EcmaStringAccessor::GetUtf16DataFlat(thread, str, buf);
        const common::Span<const uint16_t> dataSpan(data, strLen);
        return base::JsonHelper::AppendValueToQuotedString(dataSpan, builder);
    }
}
}  // namespace panda::ecmascript::base
