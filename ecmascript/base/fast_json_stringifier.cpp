/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/base/fast_json_stringifier.h"

#include "ecmascript/base/json_helper.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/object_fast_operator-inl.h"

namespace panda::ecmascript::base {
JSHandle<JSTaggedValue> FastJsonStringifier::Stringify(const JSHandle<JSTaggedValue> &value)
{
    factory_ = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> jsonCache = thread_->GetEcmaVM()->GetGlobalEnv()->GetJsonObjectHclassCache();
    if (jsonCache->IsHole()) {
        hclassCache_ = factory_->NewTaggedArray(JSON_CACHE_SIZE);
    } else {
        hclassCache_ = JSHandle<TaggedArray>::Cast(jsonCache);
    }
    JSTaggedValue tagValue = value.GetTaggedValue();
    handleValue_ = JSMutableHandle<JSTaggedValue>(thread_, tagValue);
    handleKey_ = JSMutableHandle<JSTaggedValue>(thread_, factory_->GetEmptyString());

    if (handleValue_->IsECMAObject() || handleValue_->IsBigInt()) {
        JSTaggedValue serializeValue = GetSerializeValue(handleKey_, handleValue_);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
        handleValue_.Update(serializeValue);
    }

    JSTaggedValue result = SerializeJSONProperty(handleValue_);

    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    if (!result.IsUndefined()) {
        return JSHandle<JSTaggedValue>(
            factory_->NewFromUtf8Literal(reinterpret_cast<const uint8_t *>(result_.c_str()), result_.size()));
    }
    return thread_->GlobalConstants()->GetHandledUndefined();
}

JSTaggedValue FastJsonStringifier::GetSerializeValue(const JSHandle<JSTaggedValue> &key,
                                                     const JSHandle<JSTaggedValue> &value)
{
    JSTaggedValue tagValue = value.GetTaggedValue();
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
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
    return tagValue;
}

JSTaggedValue FastJsonStringifier::SerializeJSONProperty(const JSHandle<JSTaggedValue> &value)
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
                SerializeJSArray(valHandle);
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
                THROW_TYPE_ERROR_AND_RETURN(thread_, "cannot serialize a BigInt", JSTaggedValue::Exception());
            }
            default: {
                if (!tagValue.IsCallable()) {
                    JSHClass *jsHclass = tagValue.GetTaggedObject()->GetClass();
                    if (UNLIKELY(jsHclass->IsJSProxy() &&
                        JSProxy::Cast(tagValue.GetTaggedObject())->IsArray(thread_))) {
                        SerializeJSProxy(valHandle);
                        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    } else {
                        SerializeJSONObject(valHandle);
                        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    }
                    return tagValue;
                }
            }
        }
    }
    return JSTaggedValue::Undefined();
}

CString FastJsonStringifier::SerializeObjectKey(const JSHandle<JSTaggedValue> &key, bool hasContent)
{
    if (hasContent) {
        result_ += ",";
    }

    CString str;
    if (key->IsString()) {
        str = ConvertToString(EcmaString::Cast(key->GetTaggedObject()), StringConvertedUsage::LOGICOPERATION);
    } else if (key->IsInt()) {
        str = NumberHelper::IntToString(static_cast<int32_t>(key->GetInt()));
    } else {
        str = ConvertToString(*JSTaggedValue::ToString(thread_, key), StringConvertedUsage::LOGICOPERATION);
    }
    JsonHelper::AppendValueToQuotedString(str, result_);
    result_ += ":";

    return str;
}

bool FastJsonStringifier::PushValue(const JSHandle<JSTaggedValue> &value)
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

void FastJsonStringifier::PopValue()
{
    stack_.pop_back();
}

bool FastJsonStringifier::SerializeJSONObject(const JSHandle<JSTaggedValue> &value)
{
    bool isContain = PushValue(value);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value, usually caused by circular structure", true);
    }

    result_ += "{";
    bool hasContent = false;

    ASSERT(!value->IsAccessor());
    JSHandle<JSObject> obj(value);
    if (UNLIKELY(value->IsJSProxy() || value->IsTypedArray())) {  // serialize proxy and typedArray
        JSHandle<TaggedArray> propertyArray = JSObject::EnumerableOwnNames(thread_, obj);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        uint32_t arrLength = propertyArray->GetLength();
        for (uint32_t i = 0; i < arrLength; i++) {
            handleKey_.Update(propertyArray->Get(i));
            JSHandle<JSTaggedValue> valueHandle = JSTaggedValue::GetProperty(thread_, value, handleKey_).GetValue();
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (UNLIKELY(valueHandle->IsECMAObject() || valueHandle->IsBigInt())) {
                JSTaggedValue serializeValue = GetSerializeValue(handleKey_, valueHandle);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
                    (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
                    continue;
                }
                handleValue_.Update(serializeValue);
            } else {
                handleValue_.Update(valueHandle);
            }
            SerializeObjectKey(handleKey_, hasContent);
            JSTaggedValue res = SerializeJSONProperty(handleValue_);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (!res.IsUndefined()) {
                hasContent = true;
            }
        }
    } else {
        uint32_t numOfKeys = obj->GetNumberOfKeys();
        uint32_t numOfElements = obj->GetNumberOfElements();
        if (numOfKeys + numOfElements < CACHE_MINIMUN_SIZIE || !cacheable_) {
            if (numOfElements > 0) {
                hasContent = DefaultSerializeElements(obj, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            if (numOfKeys > 0) {
                hasContent = DefaultSerializeKeys(obj, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
        } else {
            JSHClass *jsHclass = value->GetTaggedObject()->GetClass();
            int32_t index = FindCache(jsHclass, numOfKeys + numOfElements);
            if (index != INVALID_INDEX) {
                auto strCache = thread_->GetCurrentEcmaContext()->GetJsonStringifyCache(index);
                uint32_t cacheIndex = 0;
                if (numOfElements > 0) {
                    hasContent = SerializeElementsWithCache(obj, hasContent, strCache, cacheIndex, numOfElements);
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                }
                if (numOfKeys > 0) {
                    hasContent = SerializeKeysWithCache(obj, hasContent, strCache, cacheIndex);
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                }
            } else {
                CVector<std::pair<CString, int>> strCache;
                if (numOfElements > 0) {
                    hasContent = TryCacheSerializeElements(obj, hasContent, strCache);
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                }
                if (numOfKeys > 0) {
                    hasContent = TryCacheSerializeKeys(obj, hasContent, strCache);
                    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                }
                if (cacheable_) {
                    SetCache(value->GetTaggedObject()->GetClass(), numOfElements + numOfKeys, strCache);
                }
            }
        }
    }

    result_ += "}";
    PopValue();
    return true;
}

bool FastJsonStringifier::SerializeJSProxy(const JSHandle<JSTaggedValue> &object)
{
    bool isContain = PushValue(object);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value, usually caused by circular structure", true);
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
        if (UNLIKELY(valHandle->IsECMAObject() || valHandle->IsBigInt())) {
            JSTaggedValue serializeValue = GetSerializeValue(handleKey_, valHandle);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            handleValue_.Update(serializeValue);
        } else {
            handleValue_.Update(valHandle);
        }
        JSTaggedValue res = SerializeJSONProperty(handleValue_);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        if (res.IsUndefined()) {
            result_ += "null";
        }
    }

    result_ += "]";
    PopValue();
    return true;
}

bool FastJsonStringifier::SerializeJSArray(const JSHandle<JSTaggedValue> &value)
{
    // If state.[[Stack]] contains value, throw a TypeError exception because the structure is cyclical.
    bool isContain = PushValue(value);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value, usually caused by circular structure", true);
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
            if (handleValue_->IsECMAObject() || handleValue_->IsBigInt()) {
                JSTaggedValue serializeValue = GetSerializeValue(handleKey_, handleValue_);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                handleValue_.Update(serializeValue);
            }
            JSTaggedValue res = SerializeJSONProperty(handleValue_);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (res.IsUndefined()) {
                result_ += "null";
            }
        }
    }

    result_ += "]";
    PopValue();
    return true;
}

void FastJsonStringifier::SerializePrimitiveRef(const JSHandle<JSTaggedValue> &primitiveRef)
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
        THROW_TYPE_ERROR(thread_, "cannot serialize a BigInt");
    }
}

bool FastJsonStringifier::TryCacheSerializeElements(const JSHandle<JSObject> &obj, bool hasContent,
                                                    CVector<std::pair<CString, int>> &strCache)
{
    if (!ElementAccessor::IsDictionaryMode(obj)) {
        uint32_t elementsLen = ElementAccessor::GetElementsLength(obj);
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!ElementAccessor::Get(obj, i).IsHole()) {
                handleKey_.Update(JSTaggedValue(i));
                handleValue_.Update(ElementAccessor::Get(obj, i));
                hasContent = AppendJsonString(hasContent, strCache, i);
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
            JSTaggedValue value = numberDic->GetValue(index);
            if (UNLIKELY(value.IsAccessor())) {
                value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                             JSHandle<JSTaggedValue>(obj));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            handleValue_.Update(value);
            hasContent = AppendJsonString(hasContent, strCache, index);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
    }
    return hasContent;
}

bool FastJsonStringifier::SerializeElementsWithCache(const JSHandle<JSObject> &obj, bool hasContent,
    CVector<std::pair<CString, int>> &strCache, uint32_t &cacheIndex, uint32_t elementSize)
{
    if (!ElementAccessor::IsDictionaryMode(obj)) {
        uint32_t elementsLen = ElementAccessor::GetElementsLength(obj);
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!ElementAccessor::Get(obj, i).IsHole()) {
                CString key = strCache[cacheIndex++].first;
                handleValue_.Update(ElementAccessor::Get(obj, i));
                hasContent = FastAppendJsonString(hasContent, key);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
        }
    } else {
        JSHandle<TaggedArray> elementsArr(thread_, obj->GetElements());
        JSHandle<NumberDictionary> numberDic(elementsArr);
        for (; cacheIndex < elementSize; cacheIndex++) {
            CString key = strCache[cacheIndex].first;
            int index = strCache[cacheIndex].second;
            JSTaggedValue value = numberDic->GetValue(index);
            if (UNLIKELY(value.IsAccessor())) {
                value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                             JSHandle<JSTaggedValue>(obj));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            handleValue_.Update(value);
            hasContent = FastAppendJsonString(hasContent, key);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
    }
    return hasContent;
}

bool FastJsonStringifier::TryCacheSerializeKeys(const JSHandle<JSObject> &obj, bool hasContent,
                                                CVector<std::pair<CString, int>> &strCache)
{
    JSHandle<TaggedArray> propertiesArr(thread_, obj->GetProperties());
    if (!propertiesArr->IsDictionaryMode()) {
        return TryCacheSerializeKeysFromPropertiesArray(obj, hasContent, strCache);
    }

    if (obj->IsJSGlobalObject()) {
        return TryCacheSerializeKeysFromGlobalObject(obj, hasContent, strCache);
    }

    return TryCacheSerializeKeysFromNameDictionary(obj, hasContent, strCache);
}

bool FastJsonStringifier::TryCacheSerializeKeysFromPropertiesArray(const JSHandle<JSObject> &obj, bool hasContent,
                                                                   CVector<std::pair<CString, int>> &strCache)
{
    JSHandle<TaggedArray> propertiesArr(thread_, obj->GetProperties());
    JSHandle<JSHClass> jsHclass(thread_, obj->GetJSHClass());
    JSTaggedValue enumCache = jsHclass->GetEnumCache();
    if (JSObject::GetEnumCacheKind(thread_, enumCache) == EnumCacheKind::ONLY_OWN_KEYS) {
        return TryCacheSerializeKeysFromEnumCache(obj, hasContent, strCache);
    }

    int end = static_cast<int>(jsHclass->NumberOfProps());
    if (end <= 0) {
        return hasContent;
    }

    for (int i = 0; i < end; i++) {
        LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject());
        JSTaggedValue key = layoutInfo->GetKey(i);
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
            hasContent = AppendJsonString(hasContent, strCache, index);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
    }
    return hasContent;
}

bool FastJsonStringifier::TryCacheSerializeKeysFromEnumCache(const JSHandle<JSObject> &obj, bool hasContent,
                                                             CVector<std::pair<CString, int>> &strCache)
{
    JSHandle<TaggedArray> propertiesArr(thread_, obj->GetProperties());
    JSHandle<JSHClass> jsHclass(thread_, obj->GetJSHClass());
    JSHandle<TaggedArray> cache(thread_, jsHclass->GetEnumCache());
    uint32_t length = cache->GetLength();
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
        }
        handleValue_.Update(value);
        hasContent = AppendJsonString(hasContent, strCache, index);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return hasContent;
}

bool FastJsonStringifier::TryCacheSerializeKeysFromGlobalObject(const JSHandle<JSObject> &obj, bool hasContent,
                                                                CVector<std::pair<CString, int>> &strCache)
{
    JSHandle<TaggedArray> propertiesArr(thread_, obj->GetProperties());
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
        JSTaggedValue value = globalDic->GetValue(index);
        if (UNLIKELY(value.IsAccessor())) {
            value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                         JSHandle<JSTaggedValue>(obj));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        handleValue_.Update(value);
        hasContent = AppendJsonString(hasContent, strCache, index);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return hasContent;
}

bool FastJsonStringifier::TryCacheSerializeKeysFromNameDictionary(const JSHandle<JSObject> &obj, bool hasContent,
                                                                  CVector<std::pair<CString, int>> &strCache)
{
    JSHandle<TaggedArray> propertiesArr(thread_, obj->GetProperties());
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
        JSTaggedValue value = nameDic->GetValue(index);
        if (UNLIKELY(value.IsAccessor())) {
            value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                         JSHandle<JSTaggedValue>(obj));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        handleValue_.Update(value);
        hasContent = AppendJsonString(hasContent, strCache, index);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return hasContent;
}

bool FastJsonStringifier::SerializeKeysWithCache(const JSHandle<JSObject> &obj, bool hasContent,
                                                 CVector<std::pair<CString, int>> &strCache, uint32_t &cacheIndex)
{
    JSHandle<JSHClass> jsHclass(thread_, obj->GetJSHClass());
    JSHandle<TaggedArray> propertiesArr(thread_, obj->GetProperties());
    if (!propertiesArr->IsDictionaryMode()) {
        for (; cacheIndex < strCache.size(); cacheIndex++) {
            auto cacheValue = strCache[cacheIndex];
            CString str = cacheValue.first;
            int index = cacheValue.second;
            LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject());
            PropertyAttributes attr(layoutInfo->GetAttr(index));
            JSTaggedValue value = attr.IsInlinedProps()
                    ? obj->GetPropertyInlinedPropsWithRep(static_cast<uint32_t>(index), attr)
                    : propertiesArr->Get(static_cast<uint32_t>(index) - jsHclass->GetInlinedProperties());
            if (UNLIKELY(value.IsAccessor())) {
                value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                             JSHandle<JSTaggedValue>(obj));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            handleValue_.Update(value);
            hasContent = FastAppendJsonString(hasContent, str);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        return hasContent;
    }
    if (obj->IsJSGlobalObject()) {
        JSHandle<GlobalDictionary> globalDic(propertiesArr);
        for (; cacheIndex < strCache.size(); cacheIndex++) {
            auto cacheValue = strCache[cacheIndex];
            CString str = cacheValue.first;
            int index = cacheValue.second;
            JSTaggedValue value = globalDic->GetValue(index);
            if (UNLIKELY(value.IsAccessor())) {
                value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                             JSHandle<JSTaggedValue>(obj));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            handleValue_.Update(value);
            hasContent = FastAppendJsonString(hasContent, str);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        return hasContent;
    }
    JSHandle<NameDictionary> nameDic(propertiesArr);
    for (; cacheIndex < strCache.size(); cacheIndex++) {
        auto cacheValue = strCache[cacheIndex];
        CString str = cacheValue.first;
        int index = cacheValue.second;
        JSTaggedValue value = nameDic->GetValue(index);
        if (UNLIKELY(value.IsAccessor())) {
            value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                         JSHandle<JSTaggedValue>(obj));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        handleValue_.Update(value);
        hasContent = FastAppendJsonString(hasContent, str);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return hasContent;
}

bool FastJsonStringifier::AppendJsonString(bool hasContent, CVector<std::pair<CString, int>> &strCache, int index)
{
    if (handleValue_->IsECMAObject() || handleValue_->IsBigInt()) {
        JSTaggedValue serializeValue = GetSerializeValue(handleKey_, handleValue_);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
            (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
            return hasContent;
        }
        handleValue_.Update(serializeValue);
    }
    CString keyStr = SerializeObjectKey(handleKey_, hasContent);
    strCache.emplace_back(std::pair<CString, int>(keyStr, index));
    JSTaggedValue res = SerializeJSONProperty(handleValue_);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    if (!res.IsUndefined()) {
        return true;
    }
    EraseKeyString(keyStr, hasContent);
    return hasContent;
}

bool FastJsonStringifier::FastAppendJsonString(bool hasContent, CString &key)
{
    if (handleValue_->IsECMAObject() || handleValue_->IsBigInt()) {
        JSTaggedValue serializeValue = GetSerializeValue(handleKey_, handleValue_);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
            (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
            return hasContent;
        }
        handleValue_.Update(serializeValue);
    }
    FastSerializeObjectKey(key, hasContent);
    JSTaggedValue res = SerializeJSONProperty(handleValue_);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    if (!res.IsUndefined()) {
        return true;
    }
    EraseKeyString(key, hasContent);
    return hasContent;
}

bool FastJsonStringifier::DefaultSerializeElements(const JSHandle<JSObject> &obj, bool hasContent)
{
    if (!ElementAccessor::IsDictionaryMode(obj)) {
        uint32_t elementsLen = ElementAccessor::GetElementsLength(obj);
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!ElementAccessor::Get(obj, i).IsHole()) {
                handleKey_.Update(JSTaggedValue(i));
                handleValue_.Update(ElementAccessor::Get(obj, i));
                hasContent = AppendJsonString(hasContent);
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
            JSTaggedValue value = numberDic->GetValue(index);
            if (UNLIKELY(value.IsAccessor())) {
                value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                             JSHandle<JSTaggedValue>(obj));
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            handleValue_.Update(value);
            hasContent = AppendJsonString(hasContent);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
    }
    return hasContent;
}

bool FastJsonStringifier::DefaultSerializeKeys(const JSHandle<JSObject> &obj, bool hasContent)
{
    JSHandle<TaggedArray> propertiesArr(thread_, obj->GetProperties());
    if (!propertiesArr->IsDictionaryMode()) {
        JSHandle<JSHClass> jsHclass(thread_, obj->GetJSHClass());
        JSTaggedValue enumCache = jsHclass->GetEnumCache();
        if (JSObject::GetEnumCacheKind(thread_, enumCache) == EnumCacheKind::ONLY_OWN_KEYS) {
            return SerializeKeysFromCache(obj, enumCache, propertiesArr, hasContent);
        } else {
            return SerializeKeysFromLayout(obj, jsHclass, propertiesArr, hasContent);
        }
    } else if (obj->IsJSGlobalObject()) {
        return SerializeKeysFromGlobalDictionary(obj, propertiesArr, hasContent);
    } else {
        return SerializeKeysFromNameDictionary(obj, propertiesArr, hasContent);
    }
}

bool FastJsonStringifier::SerializeKeysFromCache(const JSHandle<JSObject> &obj, JSTaggedValue enumCache,
                                                 const JSHandle<TaggedArray> &propertiesArr, bool hasContent)
{
    JSHandle<TaggedArray> cache(thread_, enumCache);
    uint32_t length = cache->GetLength();
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedValue key = cache->Get(i);
        if (!key.IsString()) {
            continue;
        }
        hasContent = SerializeKeyValue(obj, key, propertiesArr, hasContent);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return hasContent;
}

bool FastJsonStringifier::SerializeKeysFromLayout(const JSHandle<JSObject> &obj, const JSHandle<JSHClass> &jsHclass,
                                                  const JSHandle<TaggedArray> &propertiesArr, bool hasContent)
{
    int end = static_cast<int>(jsHclass->NumberOfProps());
    if (end <= 0) {
        return hasContent;
    }
    for (int i = 0; i < end; i++) {
        LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject());
        JSTaggedValue key = layoutInfo->GetKey(i);
        if (key.IsString() && layoutInfo->GetAttr(i).IsEnumerable()) {
            hasContent = SerializeKeyValue(obj, key, propertiesArr, hasContent);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
    }
    return hasContent;
}

bool FastJsonStringifier::SerializeKeysFromGlobalDictionary(const JSHandle<JSObject> &obj,
                                                            const JSHandle<TaggedArray> &propertiesArr,
                                                            bool hasContent)
{
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
        JSTaggedValue value = globalDic->GetValue(index);
        if (UNLIKELY(value.IsAccessor())) {
            value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                         JSHandle<JSTaggedValue>(obj));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        handleValue_.Update(value);
        hasContent = AppendJsonString(hasContent);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return hasContent;
}

bool FastJsonStringifier::SerializeKeysFromNameDictionary(const JSHandle<JSObject> &obj,
                                                          const JSHandle<TaggedArray> &propertiesArr,
                                                          bool hasContent)
{
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
        JSTaggedValue value = nameDic->GetValue(index);
        if (UNLIKELY(value.IsAccessor())) {
            value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                         JSHandle<JSTaggedValue>(obj));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
        handleValue_.Update(value);
        hasContent = AppendJsonString(hasContent);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return hasContent;
}

bool FastJsonStringifier::SerializeKeyValue(const JSHandle<JSObject> &obj, JSTaggedValue key,
                                            const JSHandle<TaggedArray> &propertiesArr, bool hasContent)
{
    handleKey_.Update(key);
    JSHandle<JSHClass> jsHclass(thread_, obj->GetJSHClass());
    int index = JSHClass::FindPropertyEntry(thread_, *jsHclass, key);
    LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject());
    PropertyAttributes attr(layoutInfo->GetAttr(index));
    ASSERT(static_cast<int>(attr.GetOffset()) == index);
    JSTaggedValue value = attr.IsInlinedProps()
                          ? obj->GetPropertyInlinedPropsWithRep(static_cast<uint32_t>(index), attr)
                          : propertiesArr->Get(static_cast<uint32_t>(index) - jsHclass->GetInlinedProperties());
    if (attr.IsInlinedProps() && value.IsHole()) {
        return hasContent;
    }
    if (UNLIKELY(value.IsAccessor())) {
        value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                     JSHandle<JSTaggedValue>(obj));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    handleValue_.Update(value);
    hasContent = AppendJsonString(hasContent);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    return hasContent;
}

bool FastJsonStringifier::AppendJsonString(bool hasContent)
{
    if (handleValue_->IsECMAObject() || handleValue_->IsBigInt()) {
        JSTaggedValue serializeValue = GetSerializeValue(handleKey_, handleValue_);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
            (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
            return hasContent;
        }
        handleValue_.Update(serializeValue);
    }
    CString keyStr = SerializeObjectKey(handleKey_, hasContent);
    JSTaggedValue res = SerializeJSONProperty(handleValue_);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    if (!res.IsUndefined()) {
        return true;
    }
    EraseKeyString(keyStr, hasContent);
    return hasContent;
}

bool FastJsonStringifier::DefaultSerializeObject(const JSTaggedValue &object, uint32_t numOfKeys,
                                                 uint32_t numOfElements)
{
    JSHandle<JSTaggedValue> value(thread_, object);
    bool isContain = PushValue(value);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value, usually caused by circular structure", true);
    }

    result_ += "{";
    bool hasContent = false;

    JSHandle<JSObject> obj(value);
    if (numOfElements > 0) {
        hasContent = DefaultSerializeElements(obj, hasContent);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    if (numOfKeys > 0) {
        hasContent = DefaultSerializeKeys(obj, hasContent);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }

    result_ += "}";
    PopValue();
    return true;
}
}  // namespace panda::ecmascript::base
