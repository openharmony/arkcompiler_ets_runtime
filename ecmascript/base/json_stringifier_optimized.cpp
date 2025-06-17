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

#include "ecmascript/ecma_string.h"
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

    JSHandle<JSTaggedValue> key(factory_->GetEmptyString());
    JSTaggedValue serializeValue = GetSerializeValue(value, key, value, replacer);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    handleValue_.Update(serializeValue);
    JSTaggedValue result = SerializeJSONProperty(handleValue_, replacer);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    if (!result.IsUndefined()) {
        if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
            return JSHandle<JSTaggedValue>(factory_->NewFromUtf8Literal(
                reinterpret_cast<const uint8_t *>(oneByteResult_.c_str()), oneByteResult_.size()));
        } else {
            return JSHandle<JSTaggedValue>(factory_->NewFromUtf16Literal(
                reinterpret_cast<const uint16_t *>(twoBytesResult_.c_str()), twoBytesResult_.size()));
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
            case JSType::JS_API_FAST_BUFFER: {
                JSHandle bufferHandle = JSHandle<JSTaggedValue>(thread_, tagValue);
                CheckStackPushSameValue(valHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                valHandle = JSHandle<JSTaggedValue>(thread_, JSAPIFastBuffer::FromBufferToArray(thread_, bufferHandle));
                SerializeJSONObject(valHandle, replacer);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                return tagValue;
            }
            // If Type(value) is String, return QuoteJSONString(value).
            case JSType::LINE_STRING:
            case JSType::TREE_STRING:
            case JSType::SLICED_STRING: {
                JSHandle<EcmaString> strHandle = JSHandle<EcmaString>(valHandle);
                auto string = JSHandle<EcmaString>(thread_,
                    EcmaStringAccessor::Flatten(thread_->GetEcmaVM(), strHandle));
                AppendEcmaStringToResult(string);
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
                    SerializeJSONSharedMap(valHandle, replacer);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    return tagValue;
                }
                [[fallthrough]];
            }
            case JSType::JS_MAP: {
                if (transformType_ == TransformType::SENDABLE) {
                    CheckStackPushSameValue(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SerializeJSONMap(valHandle, replacer);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    return tagValue;
                }
                [[fallthrough]];
            }
            case JSType::JS_SET: {
                if (transformType_ == TransformType::SENDABLE) {
                    CheckStackPushSameValue(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SerializeJSONSet(valHandle, replacer);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    return tagValue;
                }
                [[fallthrough]];
            }
            case JSType::JS_SHARED_SET: {
                if (transformType_ == TransformType::SENDABLE) {
                    CheckStackPushSameValue(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SerializeJSONSharedSet(valHandle, replacer);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    return tagValue;
                }
                [[fallthrough]];
            }
            case JSType::JS_API_HASH_MAP: {
                if (transformType_ == TransformType::SENDABLE) {
                    CheckStackPushSameValue(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SerializeJSONHashMap(valHandle, replacer);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    return tagValue;
                }
                [[fallthrough]];
            }
            case JSType::JS_API_HASH_SET: {
                if (transformType_ == TransformType::SENDABLE) {
                    CheckStackPushSameValue(valHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    SerializeJSONHashSet(valHandle, replacer);
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
    if (hasContent) {
        AppendChar(',');
    }
    if (!gap_.empty()) {
        NewLine();
    }
    if (key->IsString()) {
        auto string = JSHandle<EcmaString>(thread_, key->GetTaggedObject());
        AppendEcmaStringToResult(string);
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

bool JsonStringifier::SerializeJSONObject(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer)
{
    Indent();
    AppendChar('{');

    bool hasContent = false;

    ASSERT(!value->IsAccessor());
    JSHandle<JSObject> obj(value);
    if (!replacer->IsArray(thread_)) {
        if (UNLIKELY(value->IsJSProxy() || value->IsTypedArray())) {  // serialize proxy and typedArray
            JSHandle<TaggedArray> propertyArray = JSObject::EnumerableOwnNames(thread_, obj);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            uint32_t arrLength = propertyArray->GetLength();
            for (uint32_t i = 0; i < arrLength; i++) {
                handleKey_.Update(propertyArray->Get(thread_, i));
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
            uint32_t numOfKeys = obj->GetNumberOfKeys(thread_);
            uint32_t numOfElements = obj->GetNumberOfElements(thread_);
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
    Unindent();
    if (hasContent && gap_.length() != 0) {
        NewLine();
    }
    AppendChar('}');
    PopValue();
    return true;
}

bool JsonStringifier::SerializeJSONSharedMap(const JSHandle<JSTaggedValue> &value,
                                             const JSHandle<JSTaggedValue> &replacer)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap> scope(thread_, value);
    JSHandle<JSSharedMap> sharedMap(value);
    JSHandle<LinkedHashMap> hashMap(thread_, sharedMap->GetLinkedMap(thread_));
    return SerializeLinkedHashMap(hashMap, replacer);
}

bool JsonStringifier::SerializeJSONSharedSet(const JSHandle<JSTaggedValue> &value,
                                             const JSHandle<JSTaggedValue> &replacer)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedSet> scope(thread_, value);
    JSHandle<JSSharedSet> sharedSet(value);
    JSHandle<LinkedHashSet> hashSet(thread_, sharedSet->GetLinkedSet(thread_));
    return SerializeLinkedHashSet(hashSet, replacer);
}

bool JsonStringifier::SerializeJSONMap(const JSHandle<JSTaggedValue> &value,
                                       const JSHandle<JSTaggedValue> &replacer)
{
    JSHandle<JSMap> jsMap(value);
    JSHandle<LinkedHashMap> hashMap(thread_, jsMap->GetLinkedMap(thread_));
    return SerializeLinkedHashMap(hashMap, replacer);
}

bool JsonStringifier::SerializeJSONSet(const JSHandle<JSTaggedValue> &value,
                                       const JSHandle<JSTaggedValue> &replacer)
{
    JSHandle<JSSet> jsSet(value);
    JSHandle<LinkedHashSet> hashSet(thread_, jsSet->GetLinkedSet(thread_));
    return SerializeLinkedHashSet(hashSet, replacer);
}

bool JsonStringifier::SerializeJSONHashMap(const JSHandle<JSTaggedValue> &value,
                                           const JSHandle<JSTaggedValue> &replacer)
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
            AppendChar('\"');
            SerializeJSONProperty(keyHandle, replacer);
            AppendChar('\"');
        } else {
            SerializeJSONProperty(keyHandle, replacer);
        }
        AppendChar(':');
        SerializeJSONProperty(valueHandle, replacer);
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

bool JsonStringifier::SerializeJSONHashSet(const JSHandle<JSTaggedValue> &value,
                                           const JSHandle<JSTaggedValue> &replacer)
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
        JSTaggedValue res = SerializeJSONProperty(currentKey, replacer);
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

bool JsonStringifier::SerializeLinkedHashMap(const JSHandle<LinkedHashMap> &hashMap,
                                             const JSHandle<JSTaggedValue> &replacer)
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
            SerializeJSONProperty(keyHandle, replacer);
            AppendLiteral("\"");
        } else {
            SerializeJSONProperty(keyHandle, replacer);
        }
        AppendChar(':');
        SerializeJSONProperty(valHandle, replacer);
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

bool JsonStringifier::SerializeLinkedHashSet(const JSHandle<LinkedHashSet> &hashSet,
                                             const JSHandle<JSTaggedValue> &replacer)
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
        JSTaggedValue res = SerializeJSONProperty(keyHandle, replacer);
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

bool JsonStringifier::SerializeJSProxy(const JSHandle<JSTaggedValue> &object, const JSHandle<JSTaggedValue> &replacer)
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
        JSTaggedValue serializeValue = GetSerializeValue(object, handleKey_, valHandle, replacer);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        handleValue_.Update(serializeValue);
        JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
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

bool JsonStringifier::SerializeJSArray(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer)
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
                AppendChar(',');
            }
            if (!gap_.empty()) {
                NewLine();
            }
            JSTaggedValue serializeValue = GetSerializeValue(value, handleKey_, handleValue_, replacer);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            handleValue_.Update(serializeValue);
            JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
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
    }
}

bool JsonStringifier::SerializeElements(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer,
                                        bool hasContent)
{
    if (!ElementAccessor::IsDictionaryMode(thread_, obj)) {
        uint32_t elementsLen = ElementAccessor::GetElementsLength(thread_, obj);
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!ElementAccessor::Get(thread_, obj, i).IsHole()) {
                handleKey_.Update(JSTaggedValue(i));
                handleValue_.Update(ElementAccessor::Get(thread_, obj, i));
                hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
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
                    JSTaggedValue numberKey = JSTaggedValue(static_cast<uint32_t>(key.GetInt()));
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
            hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
    }
    return hasContent;
}

bool JsonStringifier::SerializeKeys(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer,
                                    bool hasContent)
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
                    hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
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
                    hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
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
            hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
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

template <size_t N>
inline void JsonStringifier::AppendLiteral(const char(&src)[N])
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        AppendString(oneByteResult_, src, N - 1);
    } else {
        AppendString(twoBytesResult_, src, N - 1);
    }
}

inline void JsonStringifier::AppendChar(const char src)
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        oneByteResult_ += src;
    } else {
        twoBytesResult_ += static_cast<char16_t>(src);
    }
}

void JsonStringifier::ChangeEncoding()
{
    encoding_ = Encoding::TWO_BYTE_ENCODING;
    size_t length = oneByteResult_.length();
    twoBytesResult_.resize(length);
    for (size_t i = 0; i < length; ++i) {
        twoBytesResult_[i] = oneByteResult_[i];
    }
}

void JsonStringifier::NewLine()
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        oneByteResult_ += "\n";
        for (int i = 0; i < indent_; ++i) {
            for (auto ch : gap_) {
                oneByteResult_ += static_cast<char>(ch);
            }
        }
    } else {
        twoBytesResult_ += u"\n";
        for (int i = 0; i < indent_; ++i) {
            AppendCString(twoBytesResult_, gap_);
        }
    }
}

inline void JsonStringifier::AppendNumberToResult(const JSHandle<JSTaggedValue> &value)
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        ConvertNumberToCStringAndAppend(oneByteResult_, value.GetTaggedValue());
    } else {
        ConvertNumberToCStringAndAppend(twoBytesResult_, value.GetTaggedValue());
    }
}

inline void JsonStringifier::AppendIntToResult(int32_t key)
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        NumberHelper::AppendIntToString(oneByteResult_, key);
    } else {
        NumberHelper::AppendIntToString(twoBytesResult_, key);
    }
}

inline void JsonStringifier::AppendEcmaStringToResult(JSHandle<EcmaString> &string)
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        if (EcmaStringAccessor(*string).IsUtf8()) {
            ConvertQuotedAndAppendToCString(thread_, oneByteResult_, *string);
        } else {
            ChangeEncoding();
            ConvertQuotedAndAppendToC16String(thread_, twoBytesResult_, *string);
        }
    } else {
        ConvertQuotedAndAppendToC16String(thread_, twoBytesResult_, *string);
    }
}

inline void JsonStringifier::AppendBigIntToResult(JSHandle<JSTaggedValue> &valHandle)
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        BigInt::AppendToCString(oneByteResult_, BigInt::Cast(valHandle->GetTaggedObject()));
    } else {
        BigInt::AppendToCString(twoBytesResult_, BigInt::Cast(valHandle->GetTaggedObject()));
    }
}

inline void JsonStringifier::PopBack()
{
    if (encoding_ == Encoding::ONE_BYTE_ENCODING) {
        oneByteResult_.pop_back();
    } else {
        twoBytesResult_.pop_back();
    }
}

}  // namespace panda::ecmascript::base
