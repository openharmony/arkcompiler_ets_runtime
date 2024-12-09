/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/property_accessor.h"

#include "ecmascript/js_object-inl.h"

namespace panda::ecmascript {
PropertyAccessor::PropertyAccessor(JSThread *thread, JSHandle<JSTaggedValue> object)
    : thread_(thread),
      receiver_(thread, object.GetTaggedValue()),
      fastKeysArray_(thread, JSTaggedValue::Undefined()),
      cachedHclass_(thread, JSTaggedValue::Undefined()),
      keyLength_(0),
      shadowKeyLength_(0),
      onlyHasSimpleProperties_(true),
      canUseEnumCache_(true),
      hasSlowProperties_(false),
      slowKeysArray_(thread, JSTaggedValue::Undefined()),
      acutalKeyLength_(0)
{
    PreLoad();
}

void PropertyAccessor::PreLoad()
{
    if (receiver_->IsSlowKeysObject()) {
        hasSlowProperties_ = true;
        return;
    }
    JSHandle<JSObject> receiverObj(receiver_);
    JSHClass *jshclass = receiverObj->GetJSHClass();
    if (jshclass->IsDictionaryMode()) {
        onlyHasSimpleProperties_ = false;
        canUseEnumCache_ = false;
    }
    uint32_t numOfElements = receiverObj->GetNumberOfElements(thread_);
    if (numOfElements > 0) {
        AccumulateKeyLength(numOfElements);
        onlyHasSimpleProperties_ = false;
        canUseEnumCache_ = false;
    }
    std::pair<uint32_t, uint32_t> numOfKeys = receiverObj->GetNumberOfEnumKeys();
    uint32_t numOfEnumKeys = numOfKeys.first;
    if (numOfEnumKeys > 0) {
        AccumulateKeyLength(numOfEnumKeys);
    }
    uint32_t numOfShadowKeys = numOfKeys.second;
    if (numOfShadowKeys > 0) {
        AccumulateShadowKeyLength(numOfShadowKeys);
    }

    CollectPrototypeInfo();
    if (hasSlowProperties_ || !onlyHasSimpleProperties_) {
        return;
    }
    ASSERT(canUseEnumCache_);
    // fast path
    InitSimplePropertiesEnumCache();
}

void PropertyAccessor::CollectPrototypeInfo()
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue current = JSTaggedValue::GetPrototype(thread_, receiver_);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    while (current.IsHeapObject()) {
        if (current.IsSlowKeysObject()) {
            hasSlowProperties_ = true;
            break;
        }
        JSObject *currentObj = JSObject::Cast(current.GetTaggedObject());
        uint32_t numOfCurrentElements = currentObj->GetNumberOfElements(thread_);
        if (numOfCurrentElements > 0) {
            AccumulateKeyLength(numOfCurrentElements);
            onlyHasSimpleProperties_ = false;
            canUseEnumCache_ = false;
        }
        std::pair<uint32_t, uint32_t> numOfKeys = currentObj->GetNumberOfEnumKeys();
        uint32_t numOfEnumKeys = numOfKeys.first;
        if (numOfEnumKeys > 0) {
            AccumulateKeyLength(numOfEnumKeys);
            onlyHasSimpleProperties_ = false;
        }
        uint32_t numOfShadowKeys = numOfKeys.second;
        if (numOfShadowKeys > 0) {
            AccumulateShadowKeyLength(numOfShadowKeys);
        }
        JSHClass *jshclass = currentObj->GetJSHClass();
        if (jshclass->IsDictionaryMode()) {
            onlyHasSimpleProperties_ = false;
            canUseEnumCache_ = false;
        }
        if (onlyHasSimpleProperties_) {
            // a fast path to check simple enum cache
            jshclass->SetEnumCache(thread_, JSTaggedValue::Undefined());
        }
        current = JSObject::GetPrototype(current);
    }
}

void PropertyAccessor::InitSimplePropertiesEnumCache()
{
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> receiverObj(receiver_);
    ASSERT(receiverObj->GetNumberOfElements(thread_) == 0);
    ASSERT(!receiver_->IsInSharedHeap());
    JSMutableHandle<TaggedArray> keyArray(thread_, JSTaggedValue::Undefined());
    if (keyLength_ == 0) {
        keyArray.Update(factory->EmptyArray());
        SetActualKeyLength(0);
    } else {
        uint32_t arraySize = keyLength_ + EnumCache::ENUM_CACHE_HEADER_SIZE;
        JSHandle<TaggedArray> newArray = thread_->GetEcmaVM()->GetFactory()->NewTaggedArray(arraySize);
        uint32_t length = JSObject::GetAllEnumKeys(thread_, receiverObj, EnumCache::ENUM_CACHE_HEADER_SIZE, newArray);
        SetActualKeyLength(length);
        JSObject::SetEnumCacheKind(thread_, *newArray, EnumCacheKind::SIMPLE);
        keyArray.Update(newArray);
    }
    JSObject::ClearHasDeleteProperty(receiver_);
    JSHClass *jsHclass = receiverObj->GetJSHClass();
    jsHclass->SetEnumCache(thread_, keyArray.GetTaggedValue());
    fastKeysArray_.Update(keyArray.GetTaggedValue());
    cachedHclass_.Update(JSTaggedValue(jsHclass));
}

inline void PropertyAccessor::AccumulateKeyLength(uint32_t length)
{
    keyLength_ += length;
}

inline void PropertyAccessor::AccumulateShadowKeyLength(uint32_t length)
{
    shadowKeyLength_ += length;
}

JSHandle<JSTaggedValue> PropertyAccessor::GetCachedHclass()
{
    return cachedHclass_;
}

uint32_t PropertyAccessor::GetActualKeyLength() const
{
    return acutalKeyLength_;
}

inline void PropertyAccessor::SetActualKeyLength(uint32_t length)
{
    acutalKeyLength_ = length;
}

void PropertyAccessor::AddKeysEndIfNeeded(JSHandle<TaggedArray> keys)
{
    // when has duplicated keys
    if (acutalKeyLength_ < keyLength_) {
        keys->Set(thread_, acutalKeyLength_ + EnumCache::ENUM_CACHE_HEADER_SIZE, JSTaggedValue::Undefined());
    }
}

void PropertyAccessor::TryInitEnumCacheWithProtoChainInfo()
{
#if ECMASCRIPT_ENABLE_IC
    if (!canUseEnumCache_) {
        JSObject::SetEnumCacheKind(thread_, TaggedArray::Cast(fastKeysArray_->GetTaggedObject()), EnumCacheKind::NONE);
        return;
    }
    ASSERT(!receiver_->IsInSharedHeap());
    ASSERT(!onlyHasSimpleProperties_);
    JSHandle<JSObject> receiverObj(receiver_);
    JSHandle<JSHClass> jsHclass(thread_, receiverObj->GetJSHClass());
    jsHclass->SetEnumCache(thread_, fastKeysArray_.GetTaggedValue());
    JSObject::SetEnumCacheKind(
        thread_, TaggedArray::Cast(fastKeysArray_->GetTaggedObject()), EnumCacheKind::PROTOCHAIN);
    cachedHclass_.Update(jsHclass);
    JSHClass::EnableProtoChangeMarker(thread_, jsHclass);
#endif
}

JSHandle<JSTaggedValue> PropertyAccessor::GetKeysFast()
{
    if (!fastKeysArray_->IsUndefined()) {
        AddKeysEndIfNeeded(JSHandle<TaggedArray>(thread_, fastKeysArray_.GetTaggedValue()));
        return fastKeysArray_;
    }
    if (hasSlowProperties_) {
        return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
    }
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    uint32_t arraySize = keyLength_ + EnumCache::ENUM_CACHE_HEADER_SIZE;
    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(arraySize);
    JSHandle<TaggedQueue> shadowQueue = factory->NewTaggedQueue(shadowKeyLength_ + 1);
    uint32_t keysNum = EnumCache::ENUM_CACHE_HEADER_SIZE;
    JSMutableHandle<JSTaggedValue> current(thread_, receiver_);
    while (current->IsHeapObject()) {
        JSObject::AppendOwnEnumPropertyKeys(thread_, JSHandle<JSObject>(current), keyArray, &keysNum, shadowQueue);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
        JSObject::ClearHasDeleteProperty(current);
        current.Update(JSObject::GetPrototype(current.GetTaggedValue()));
    }
    SetActualKeyLength(keysNum - EnumCache::ENUM_CACHE_HEADER_SIZE);
    AddKeysEndIfNeeded(keyArray);
    fastKeysArray_.Update(keyArray.GetTaggedValue());
    TryInitEnumCacheWithProtoChainInfo();
    return fastKeysArray_;
}

JSHandle<JSTaggedValue> PropertyAccessor::GetKeysSlow()
{
    std::vector<JSHandle<TaggedArray>> remainings;
    std::vector<JSHandle<JSTaggedValue>> visited;
    JSMutableHandle<JSTaggedValue> current(thread_, receiver_);
    while (current->IsHeapObject()) {
        PushRemainingKeys(JSHandle<JSObject>(current), remainings);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
        JSObject::ClearHasDeleteProperty(current);
        visited.emplace_back(thread_, current.GetTaggedValue());
        current.Update(JSTaggedValue::GetPrototype(thread_, current));
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    }
    MergeRemainings(remainings, visited);
    return JSHandle<JSTaggedValue>(thread_, slowKeysArray_.GetTaggedValue());
}

void PropertyAccessor::PushRemainingKeys(JSHandle<JSObject> object, std::vector<JSHandle<TaggedArray>> &remainings)
{
    JSMutableHandle<JSTaggedValue> value(thread_, JSTaggedValue::Undefined());
    uint32_t remainingIndex = 0;
    if (object->IsJSProxy()) {
        JSHandle<TaggedArray> proxyArr = JSProxy::OwnPropertyKeys(thread_, JSHandle<JSProxy>(object));
        RETURN_IF_ABRUPT_COMPLETION(thread_);
        uint32_t length = proxyArr->GetLength();
        for (uint32_t i = 0; i < length; i++) {
            value.Update(proxyArr->Get(i));
            PropertyDescriptor desc(thread_);
            JSProxy::GetOwnProperty(thread_, JSHandle<JSProxy>(object), value, desc);
            RETURN_IF_ABRUPT_COMPLETION(thread_);
            if (!desc.IsEnumerable()) {
                proxyArr->Set(thread_, i, JSTaggedValue::Hole());
            } else {
                remainingIndex++;
            }
        }
        remainings.push_back(proxyArr);
        AccumulateKeyLength(remainingIndex);
    } else {
        JSHandle<TaggedArray> array = JSTaggedValue::GetOwnEnumPropertyKeys(thread_, JSHandle<JSTaggedValue>(object));
        uint32_t length = array->GetLength();
        for (uint32_t i = 0; i < length; i++) {
            value.Update(array->Get(i));
            if (!value->IsString()) {
                array->Set(thread_, i, JSTaggedValue::Hole());
            } else {
                remainingIndex++;
            }
        }
        remainings.push_back(array);
        AccumulateKeyLength(remainingIndex);
    }
}

void PropertyAccessor::MergeRemainings(const std::vector<JSHandle<TaggedArray>> &remainings,
                                       const std::vector<JSHandle<JSTaggedValue>> &visited)
{
    uint32_t arraySize = keyLength_ + EnumCache::ENUM_CACHE_HEADER_SIZE;
    JSHandle<TaggedArray> keyArray = thread_->GetEcmaVM()->GetFactory()->NewTaggedArray(arraySize);

    JSMutableHandle<TaggedArray> remaining(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> keyHandle(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> objHandle(thread_, JSTaggedValue::Undefined());
    uint32_t index = EnumCache::ENUM_CACHE_HEADER_SIZE;
    uint32_t numberOfRemaining = remainings.size();
    for (uint32_t i = 0; i < numberOfRemaining; i++) {
        remaining.Update(remainings[i]);
        uint32_t remainingSize = remaining->GetLength();
        for (uint32_t j = 0; j < remainingSize; j++) {
            keyHandle.Update(remaining->Get(thread_, j));
            if (keyHandle->IsHole()) {
                continue;
            }
            bool has = false;
            for (uint32_t k = 0; k < i; k++) {
                objHandle.Update(visited[k]);
                PropertyDescriptor desc(thread_);
                has = JSTaggedValue::GetOwnProperty(thread_, objHandle, keyHandle, desc);
                RETURN_IF_ABRUPT_COMPLETION(thread_);
                if (has) {
                    break;
                }
            }
            if (!has) {
                keyArray->Set(thread_, index, keyHandle);
                index++;
            }
        }
    }
    SetActualKeyLength(index - EnumCache::ENUM_CACHE_HEADER_SIZE);
    AddKeysEndIfNeeded(keyArray);
    slowKeysArray_.Update(keyArray.GetTaggedValue());
    JSObject::SetEnumCacheKind(thread_, *keyArray, EnumCacheKind::NONE);
}
}  // namespace panda::ecmascript
