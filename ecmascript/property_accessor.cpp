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

#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
PropertyAccessor::PropertyAccessor(JSThread *thread, JSHandle<JSTaggedValue> object)
    : thread_(thread),
      receiver_(thread, object.GetTaggedValue()),
      fastKeysArray_(thread, JSTaggedValue::Undefined()),
      cachedHClass_(thread, JSTaggedValue::Undefined()),
      enumCache_(thread, JSTaggedValue::Undefined()),
      keyLength_(0),
      shadowKeyLength_(0),
      onlyHasSimpleProperties_(true),
      hasSlowProperties_(false),
      hasPrototypeChainEnumCache_(false),
      slowKeysArray_(thread, JSTaggedValue::Undefined()),
      acutalKeyLength_(0)
{
    PreLoad();
}

void PropertyAccessor::PreLoad()
{
    JSHandle<JSObject> receiverObj(receiver_);
    JSHandle<JSHClass> jsHClass(thread_, receiverObj->GetJSHClass());
    if (receiver_->IsSlowKeysObject()) {
        hasSlowProperties_ = true;
        return;
    }
    enumCache_ = JSHandle<JSTaggedValue>::Cast(
        JSObject::GetOrCreateEnumCache(thread_, jsHClass));
    cachedHClass_.Update(jsHClass);
    if (jsHClass->IsDictionaryMode()) {
        onlyHasSimpleProperties_ = false;
    }
    uint32_t numOfElements = receiverObj->GetNumberOfElements(thread_);
    if (numOfElements > 0) {
        AccumulateKeyLength(numOfElements);
        onlyHasSimpleProperties_ = false;
        tryPrototypeChainEnumCache_ = false;
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
    CopyKeyLengthToSelf();
    hasPrototypeChainEnumCache_ = HasPrototypeChainEnumCache();
    CollectPrototypeInfo();
    if (hasSlowProperties_ || !onlyHasSimpleProperties_) {
        return;
    }
    ASSERT(!hasSlowProperties_ && onlyHasSimpleProperties_);
    // Fath path for simple properties.
    InitSimplePropertiesEnumCache();
}

bool PropertyAccessor::HasPrototypeChainEnumCache()
{
    JSTaggedValue current = JSTaggedValue::GetPrototype(thread_, receiver_);
    if (!current.IsHeapObject()) {
        tryPrototypeChainEnumCache_ = false;
        return false;
    }
    JSTaggedValue enumCache = current.GetTaggedObject()->GetClass()->GetEnumCache();
    return enumCache.IsEnumCacheAllValid();
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
            hasPrototypeChainEnumCache_ = false;
            tryPrototypeChainEnumCache_ = false;
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
            hasPrototypeChainEnumCache_ = false;
            tryPrototypeChainEnumCache_ = false;
        }
        current = JSObject::GetPrototype(current);
    }
}

// For simple enumCache, set the ProtoChainInfoEnumCache of
// the receiver's prototype to JSTaggedValue::UNDEFINED.
// Additionally, enable the proto change marker to ensure that if the receiver's prototype chain changes,
// the ProtoChainInfoEnumCache of the receiver's prototype becomes JSTaggedValue::NULL.
void PropertyAccessor::InitSimplePropertiesEnumCache()
{
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> receiverObj(receiver_);
    JSHClass *jsHClass = receiverObj->GetJSHClass();

    ASSERT(receiverObj->GetNumberOfElements(thread_) == 0);
    ASSERT(!receiver_->IsInSharedHeap());

    JSMutableHandle<TaggedArray> keyArray(thread_, JSTaggedValue::Undefined());
    if (keyLength_ == 0) {
        keyArray.Update(factory->EmptyArray());
        SetActualKeyLength(0);
    } else {
        uint32_t arraySize = keyLength_;
        JSHandle<TaggedArray> newArray = thread_->GetEcmaVM()->GetFactory()->NewTaggedArray(arraySize);
        uint32_t length = JSObject::GetAllEnumKeys(thread_, receiverObj, 0, newArray);
        SetActualKeyLength(length);
        keyArray.Update(newArray);
    }

    // Set proto's ProtoChainInfoEnumCache to JSTaggedValue::NULL and enable proto's ChangeMarker.
    JSTaggedValue proto = JSObject::GetPrototype(receiver_.GetTaggedValue());
    if (proto.IsHeapObject()) {
        JSHandle<EnumCache> enumCache = JSObject::GetOrCreateEnumCache(thread_,
            JSHandle<JSHClass>(thread_, proto.GetTaggedObject()->GetClass()));
        enumCache->SetProtoChainInfoEnumCache(thread_, JSTaggedValue::Undefined());
        JSHClass::EnableProtoChangeMarker(thread_, JSHandle<JSHClass>(thread_, jsHClass));
    }

    JSObject::SetEnumCacheKind(thread_, JSHandle<EnumCache>::Cast(enumCache_), EnumCacheKind::SIMPLE);
    JSObject::ClearHasDeleteProperty(receiver_);
    fastKeysArray_.Update(keyArray.GetTaggedValue());
}

inline void PropertyAccessor::CopyKeyLengthToSelf()
{
    keyLengthSelf_ = keyLength_;
    shadowKeyLengthSelf_ = shadowKeyLength_;
}

inline void PropertyAccessor::AccumulateKeyLength(uint32_t length)
{
    keyLength_ += length;
}

inline void PropertyAccessor::AccumulateShadowKeyLength(uint32_t length)
{
    shadowKeyLength_ += length;
}

JSHandle<JSTaggedValue> PropertyAccessor::GetCachedHClass() const
{
    return cachedHClass_;
}

JSHandle<JSTaggedValue> PropertyAccessor::GetEnumCache() const
{
    return enumCache_;
}

uint32_t PropertyAccessor::GetActualKeyLength() const
{
    return acutalKeyLength_;
}

inline void PropertyAccessor::SetActualKeyLength(uint32_t length)
{
    acutalKeyLength_ = length;
}

void PropertyAccessor::AddUndefinedEndIfNeeded(JSHandle<TaggedArray> keys)
{
    // when has duplicated keys
    if (acutalKeyLength_ < keyLength_) {
        keys->Set(thread_, acutalKeyLength_, JSTaggedValue::Undefined());
    }
}

void PropertyAccessor::AddUndefinedEndIfNeeded(JSHandle<TaggedArray> keys,
                                               const uint32_t keyLength, const uint32_t acutalKeyLength)
{
    if (acutalKeyLength < keyLength) {
        keys->Set(thread_, acutalKeyLength, JSTaggedValue::Undefined());
    }
}

JSHandle<TaggedArray> PropertyAccessor::GetChainKeys(const JSHandle<JSTaggedValue> &receiver,
                                                     const uint32_t keyLength,
                                                     const uint32_t shadowKeyLength)
{
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    uint32_t arraySize = keyLength;
    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(arraySize);
    JSHandle<TaggedQueue> shadowQueue = factory->NewTaggedQueue(shadowKeyLength + 1);
    uint32_t keysNum = 0;
    JSMutableHandle<JSTaggedValue> current(thread_, receiver);
    while (current->IsHeapObject()) {
        JSObject::AppendOwnEnumPropertyKeys(thread_, JSHandle<JSObject>(current), keyArray, &keysNum, shadowQueue);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread_);
        JSObject::ClearHasDeleteProperty(current);
        current.Update(JSObject::GetPrototype(current.GetTaggedValue()));
    }
    AddUndefinedEndIfNeeded(keyArray, keyLength, keysNum);
    return keyArray;
}

JSHandle<JSTaggedValue> PropertyAccessor::GetKeysFastWithoutCache()
{
    JSHandle<TaggedArray> keyArray = GetChainKeys(receiver_, keyLength_, shadowKeyLength_);
    return JSHandle<JSTaggedValue>::Cast(keyArray);
}

std::pair<JSHandle<TaggedArray>, JSHandle<TaggedQueue>> PropertyAccessor::GetOwnKeysWithoutCache()
{
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    uint32_t arraySize = keyLengthSelf_;
    JSHandle<TaggedArray> keyArrayOnReceiver = factory->NewTaggedArray(arraySize);
    JSHandle<TaggedQueue> shadowQueueOnReceiver = factory->NewTaggedQueue(shadowKeyLengthSelf_ + 1);
    uint32_t keysNum = 0;
    JSObject::AppendOwnEnumPropertyKeys(thread_, JSHandle<JSObject>(receiver_),
                                        keyArrayOnReceiver, &keysNum, shadowQueueOnReceiver);
    AddUndefinedEndIfNeeded(keyArrayOnReceiver, arraySize, keysNum);
    return std::make_pair(keyArrayOnReceiver, shadowQueueOnReceiver);
}

bool PropertyAccessor::IsObjectWithoutKey() const
{
    return keyLengthSelf_ == 0 && shadowKeyLengthSelf_ == 0;
}

std::pair<JSHandle<TaggedArray>, JSHandle<TaggedQueue>> PropertyAccessor::GetOwnKeys()
{
    if (IsObjectWithoutKey()) {
        ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> keyArrayOnReceiver = factory->NewTaggedArray(0);
        JSHandle<TaggedQueue> shadowQueueOnReceiver = factory->NewTaggedQueue(0);
        return std::make_pair(keyArrayOnReceiver, shadowQueueOnReceiver);
    }

    return GetOwnKeysWithoutCache();
}

JSHandle<TaggedArray> PropertyAccessor::GetAndSetChainKeys(const JSHandle<JSTaggedValue> &proto,
                                                           const uint32_t keyLength,
                                                           const uint32_t shadowKeyLength)
{
    JSHandle<TaggedArray> keyArrayOnPrototypeChain = GetChainKeys(proto, keyLength, shadowKeyLength);
    ASSERT(!receiver_->IsInSharedHeap());
    ASSERT(!onlyHasSimpleProperties_);
    JSHandle<JSObject> protoObj(proto);
    JSHandle<JSHClass> jsHClass(thread_, protoObj->GetJSHClass());
    JSHandle<EnumCache> enumCacheProto = JSObject::GetOrCreateEnumCache(thread_, jsHClass);
    
    // receiver's ProtoChainInfoEnumCache == proto's EnumCacheAll.
    JSHandle<EnumCache>::Cast(enumCache_)->
        SetProtoChainInfoEnumCache(thread_, keyArrayOnPrototypeChain.GetTaggedValue());
    enumCacheProto->SetEnumCacheAll(thread_, keyArrayOnPrototypeChain.GetTaggedValue());
    JSHClass::EnablePHCProtoChangeMarker(thread_, jsHClass);
    return keyArrayOnPrototypeChain;
}

void PropertyAccessor::AddKey(const JSHandle<JSTaggedValue> &value,
                              JSHandle<TaggedArray>& allKeys, uint32_t& allKeysLength,
                              const JSHandle<TaggedArray> &keyArrayOnReceiver,
                              const JSHandle<TaggedQueue> &shadowQueueOnReceiver)
{
    uint32_t receiverKeysLength = keyArrayOnReceiver->GetLength();
    JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < receiverKeysLength; i++) {
        key.Update(keyArrayOnReceiver->Get(i));
        if (JSTaggedValue::Equal(thread_, key, value)) {
            return;
        }
    }

    uint32_t shadowKeysSize = shadowQueueOnReceiver->Size();
    for (uint32_t i = 0; i < shadowKeysSize; i++) {
        key.Update(shadowQueueOnReceiver->Get(i));
        if (JSTaggedValue::Equal(thread_, key, value)) {
            return;
        }
    }
    allKeys->Set(thread_, allKeysLength++, value);
}

JSHandle<TaggedArray> PropertyAccessor::CombineKeys(const JSHandle<TaggedArray> &keyArrayOnReceiver,
                                                    const JSHandle<TaggedArray> &keyArrayOnPrototypeChain,
                                                    const JSHandle<TaggedQueue> &shadowQueueOnReceiver)
{
    uint32_t receiverKeysLength = keyArrayOnReceiver->GetLength();
    uint32_t PrototypeChainKeysLength = keyArrayOnPrototypeChain->GetLength();
    uint32_t allKeysLength = receiverKeysLength;
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    uint32_t arraySize = receiverKeysLength + PrototypeChainKeysLength;
    JSHandle<TaggedArray> allKeys = factory->NewTaggedArray(arraySize);
    allKeys->Copy(thread_, 0, 0, keyArrayOnReceiver.GetObject<TaggedArray>(), receiverKeysLength);
    JSMutableHandle<JSTaggedValue> value(thread_, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < PrototypeChainKeysLength; i++) {
        value.Update(keyArrayOnPrototypeChain->Get(i));
        AddKey(value, allKeys, allKeysLength, keyArrayOnReceiver, shadowQueueOnReceiver);
    }
    AddUndefinedEndIfNeeded(allKeys, arraySize, allKeysLength);
    return allKeys;
}

JSHandle<JSTaggedValue> PropertyAccessor::GetKeysFastWithPrototypeChainEnumCache()
{
    auto [keyArrayOnReceiver, shadowQueueOnReceiver] = GetOwnKeys();
    JSHandle<TaggedArray> keyArrayOnPrototypeChain;
    auto proto = JSTaggedValue::GetPrototype(thread_, receiver_);
    JSHandle<JSTaggedValue> protoHandle(thread_, proto);
    if (hasPrototypeChainEnumCache_) {
        EnumCache* enumCacheProto = EnumCache::Cast(proto.GetTaggedObject()->GetClass()->GetEnumCache());
        JSHandle<JSTaggedValue> keyValueOnPrototypeChain(thread_, enumCacheProto->GetEnumCacheAll());
        keyArrayOnPrototypeChain = JSHandle<TaggedArray>::Cast(keyValueOnPrototypeChain);
        JSHandle<EnumCache>::Cast(enumCache_)->
            SetProtoChainInfoEnumCache(thread_, keyArrayOnPrototypeChain.GetTaggedValue());
    } else {
        keyArrayOnPrototypeChain = GetAndSetChainKeys(protoHandle,
            keyLength_ - keyLengthSelf_, shadowKeyLength_ - shadowKeyLengthSelf_);
    }
    
    JSHandle<TaggedArray> allKeys = CombineKeys(keyArrayOnReceiver, keyArrayOnPrototypeChain, shadowQueueOnReceiver);
    return JSHandle<JSTaggedValue>::Cast(allKeys);
}

JSHandle<JSTaggedValue> PropertyAccessor::GetKeysFast()
{
    // Fath path for simple properties.
    if (!fastKeysArray_->IsUndefined()) {
        AddUndefinedEndIfNeeded(JSHandle<TaggedArray>(thread_, fastKeysArray_.GetTaggedValue()));
        JSHandle<EnumCache>::Cast(enumCache_)->SetEnumCacheAll(thread_, fastKeysArray_.GetTaggedValue());
        return fastKeysArray_;
    }

    if (hasSlowProperties_) {
        return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
    }

    ASSERT(!receiver_->IsInSharedHeap());
    ASSERT(!onlyHasSimpleProperties_);
    
    if (tryPrototypeChainEnumCache_) {
        // This handles the case where prototype chain and self have no elements or slow keys.
        fastKeysArray_.Update(GetKeysFastWithPrototypeChainEnumCache());
        JSHandle<EnumCache>::Cast(enumCache_)->SetEnumCacheAll(thread_, fastKeysArray_.GetTaggedValue());
    } else {
        fastKeysArray_.Update(GetKeysFastWithoutCache());
    }
    JSObject::SetEnumCacheKind(thread_, JSHandle<EnumCache>::Cast(enumCache_), EnumCacheKind::PROTOCHAIN);
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
    uint32_t arraySize = keyLength_;
    JSHandle<TaggedArray> keyArray = thread_->GetEcmaVM()->GetFactory()->NewTaggedArray(arraySize);

    JSMutableHandle<TaggedArray> remaining(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> keyHandle(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> objHandle(thread_, JSTaggedValue::Undefined());
    uint32_t index = 0;
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
    SetActualKeyLength(index);
    AddUndefinedEndIfNeeded(keyArray);
    slowKeysArray_.Update(keyArray.GetTaggedValue());
}
}  // namespace panda::ecmascript
