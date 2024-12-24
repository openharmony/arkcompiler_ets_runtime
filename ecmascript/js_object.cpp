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

#include "ecmascript/js_object.h"
#include "ecmascript/dfx/native_module_failure_info.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/property_accessor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

namespace panda::ecmascript {
using PGOProfiler = pgo::PGOProfiler;
PropertyAttributes::PropertyAttributes(const PropertyDescriptor &desc)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (desc.HasWritable()) {
        SetWritable(desc.IsWritable());
    }

    if (desc.HasEnumerable()) {
        SetEnumerable(desc.IsEnumerable());
    }

    if (desc.HasConfigurable()) {
        SetConfigurable(desc.IsConfigurable());
    }

    if (desc.IsAccessorDescriptor()) {
        SetIsAccessor(true);
    }
    // internal accessor
    if (desc.HasValue() && desc.GetValue()->IsAccessor()) {
        SetIsAccessor(true);
    }
}

void ThroughputJSObjectResizingStrategy::UpdateGrowStep(JSThread *thread, uint32_t step)
{
    // 2 : multiple double
    thread->SetPropertiesGrowStep(std::min(static_cast<uint32_t>(JSObjectResizingStrategy::PROPERTIES_GROW_SIZE * 2),
                                           step));
}

Method *ECMAObject::GetCallTarget() const
{
    const TaggedObject *obj = this;
    ASSERT(JSTaggedValue(obj).IsJSFunctionBase() || JSTaggedValue(obj).IsJSProxy());

    JSTaggedValue value;
    if (JSTaggedValue(obj).IsJSFunctionBase()) {
        value = JSFunctionBase::ConstCast(obj)->GetMethod();
    } else {
        value = JSProxy::ConstCast(obj)->GetMethod();
    }
    return Method::Cast(value.GetTaggedObject());
}

JSHandle<TaggedArray> JSObject::GrowElementsCapacity(const JSThread *thread, const JSHandle<JSObject> &obj,
                                                     uint32_t capacity, bool highGrowth, bool isNew)
{
    uint32_t newCapacity = 0;
    if (obj->IsJSArray()) {
        uint32_t hint = JSHandle<JSArray>(obj)->GetHintLength();
        newCapacity = ComputeElementCapacityWithHint(capacity, hint);
    }
    if (obj->IsJSSArray()) {
        uint32_t hint = JSHandle<JSSharedArray>(obj)->GetHintLength();
        newCapacity = ComputeElementCapacityWithHint(capacity, hint);
    }
    if (newCapacity == 0) {
        newCapacity = highGrowth ? ComputeElementCapacityHighGrowth(capacity) :
            ComputeElementCapacity(capacity, isNew);
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> oldElements(thread, obj->GetElements());
    uint32_t oldLength = oldElements->GetLength();

    ElementsKind kind = obj->GetClass()->GetElementsKind();
    JSHandle<TaggedArray> newElements =
        factory->CopyArray(oldElements, oldLength, newCapacity, JSTaggedValue::Hole(),
                           obj->IsJSShared() ? MemSpaceType::SHARED_OLD_SPACE : MemSpaceType::SEMI_SPACE, kind);
    obj->SetElements(thread, newElements);
    if (thread->IsPGOProfilerEnable() && obj->IsJSArray()) {
        auto trackInfo = JSHandle<JSArray>(obj)->GetTrackInfo();
        thread->GetEcmaVM()->GetPGOProfiler()->UpdateTrackArrayLength(trackInfo, newCapacity);
    }
    return newElements;
}

JSHandle<JSTaggedValue> JSObject::IterableToList(JSThread *thread, const JSHandle<JSTaggedValue> &items,
                                                 JSTaggedValue method)
{
    // 1. If method is present, then
    // a. Let iteratorRecord be ? GetIterator(items, sync, method).
    // 2. Else,
    // a. Let iteratorRecord be ? GetIterator(items, sync).
    JSHandle<JSTaggedValue> iteratorRecord;
    JSHandle<JSTaggedValue> methodHandle(thread, method);
    if (!methodHandle->IsUndefined()) {
        iteratorRecord = JSIterator::GetIterator(thread, items, methodHandle);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    } else {
        iteratorRecord = JSIterator::GetIterator(thread, items);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    }
    // 3. Let values be a new empty List.
    // 4. Let next be true.
    JSHandle<JSArray> array = JSHandle<JSArray>::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    JSHandle<JSTaggedValue> valuesList = JSHandle<JSTaggedValue>::Cast(array);
    JSMutableHandle<JSTaggedValue> next(thread, JSTaggedValue::True());
    // 5. Repeat, while next is not false,
    // a. Set next to ? IteratorStep(iteratorRecord).
    // b. If next is not false, then
    // i. Let nextValue be ? IteratorValue(next).
    // ii. Append nextValue to the end of the List values.
    uint32_t k = 0;
    while (!next->IsFalse()) {
        next.Update(JSIterator::IteratorStep(thread, iteratorRecord).GetTaggedValue());
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        if (!next->IsFalse()) {
            JSHandle<JSTaggedValue> nextValue(JSIterator::IteratorValue(thread, next));
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            JSArray::FastSetPropertyByValue(thread, valuesList, k, nextValue);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            k++;
        }
    }
    // 6. Return values.
    return valuesList;
}

bool JSObject::IsRegExp(JSThread *thread, const JSHandle<JSTaggedValue> &argument)
{
    if (!argument->IsECMAObject()) {
        return false;
    }
    JSHandle<JSTaggedValue> matchSymbol = thread->GetEcmaVM()->GetGlobalEnv()->GetMatchSymbol();
    JSTaggedValue isRegexp =  ObjectFastOperator::FastGetPropertyByValue(
        thread, argument.GetTaggedValue(), matchSymbol.GetTaggedValue());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (!isRegexp.IsUndefined()) {
        return isRegexp.ToBoolean();
    }
    JSHandle<JSObject> argumentObj = JSHandle<JSObject>::Cast(argument);
    return argumentObj->IsJSRegExp();
}

JSHandle<NameDictionary> JSObject::TransitionToDictionary(const JSThread *thread, const JSHandle<JSObject> &receiver)
{
    JSHandle<TaggedArray> array(thread, receiver->GetProperties());
    JSHandle<JSHClass> jshclass(thread, receiver->GetJSHClass());
    ASSERT(!jshclass->IsDictionaryMode());
    uint32_t propNumber = jshclass->NumberOfProps();

    ASSERT(!jshclass->GetLayout().IsNull());
    if (jshclass->IsJSShared()) {
        THROW_TYPE_ERROR_AND_RETURN(const_cast<JSThread *>(thread),
                                    "shared obj does not support changing or deleting attributes",
                                    JSHandle<NameDictionary>());
    }
    JSHandle<LayoutInfo> layoutInfoHandle(thread, jshclass->GetLayout());
    JSMutableHandle<NameDictionary> dict(
        thread, NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(propNumber)));
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    uint32_t numberInlinedProps = jshclass->GetInlinedProperties();
    for (uint32_t i = 0; i < propNumber; i++) {
        JSTaggedValue key = layoutInfoHandle->GetKey(i);
        PropertyAttributes attr = layoutInfoHandle->GetAttr(i);
        ASSERT(i == attr.GetOffset());
        JSTaggedValue value;

        if (i < numberInlinedProps) {
            value = receiver->GetPropertyInlinedPropsWithRep(i, attr);
            // If delete a property in hclass which has subtyping info and not prototype, only set value as hole and
            // not remove. When transition to dictionary, exclude it.
            if (value.IsHole()) {
                continue;
            }
        } else {
            value = array->Get(i - numberInlinedProps);
        }

        attr.SetBoxType(PropertyBoxType::UNDEFINED);
        valueHandle.Update(value);
        keyHandle.Update(key);
        JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread, dict, keyHandle, valueHandle, attr);
        dict.Update(newDict);
    }

    receiver->SetProperties(thread, dict);
    ElementsKind oldKind = receiver->GetJSHClass()->GetElementsKind();
    // change HClass
    JSHClass::TransitionToDictionary(thread, receiver);
    JSObject::TryMigrateToGenericKindForJSObject(thread, receiver, oldKind);

    // trim in-obj properties space
    TrimInlinePropsSpace(thread, receiver, numberInlinedProps);

    return dict;
}

void JSObject::ElementsToDictionary(const JSThread *thread, JSHandle<JSObject> obj)
{
    JSHandle<TaggedArray> elements(thread, obj->GetElements());
    ASSERT(!obj->GetJSHClass()->IsDictionaryElement());
    uint32_t length = elements->GetLength();
    if (obj->IsJSShared()) {
        THROW_TYPE_ERROR(const_cast<JSThread *>(thread),
                         "shared obj does not support changing or deleting attributes");
    }
    JSMutableHandle<NumberDictionary> dict(thread, NumberDictionary::Create(thread));
    auto attr = PropertyAttributes(PropertyAttributes::GetDefaultAttributes());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue ::Undefined());
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedValue value = ElementAccessor::Get(thread, obj, i);
        if (value.IsHole()) {
            continue;
        }
        key.Update(JSTaggedValue(i));
        valueHandle.Update(value);
        JSHandle<NumberDictionary> newDict = NumberDictionary::PutIfAbsent(thread, dict, key, valueHandle, attr);
        dict.Update(newDict);
    }
    obj->SetElements(thread, dict);

    ElementsKind oldKind = obj->GetJSHClass()->GetElementsKind();
    JSHClass::TransitionElementsToDictionary(thread, obj);
    TryMigrateToGenericKindForJSObject(thread, obj, oldKind);
}

bool JSObject::AttributesUnchanged(const JSThread *thread,
                                   const JSHandle<JSObject> &obj)
{
    JSHandle<NumberDictionary> elements(thread, obj->GetElements());
    uint32_t size = static_cast<uint32_t>(elements->Size());
    for (uint32_t hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = elements->GetKey(hashIndex);
        if (key.IsUndefined() || key.IsHole()) {
            continue;
        }
        PropertyAttributes attr = elements->GetAttributes(hashIndex);
        if (!attr.IsDefaultAttributes()) {
            return false;
        }
    }
    return true;
}

void JSObject::TryOptimizeAsFastElements(const JSThread *thread, JSHandle<JSObject> obj)
{
    ASSERT(obj->GetJSHClass()->IsDictionaryElement() && obj->IsJSArray());
    if (AttributesUnchanged(thread, obj)) {
        uint32_t length = JSArray::Cast(*obj)->GetLength();
        JSHandle<NumberDictionary> elements(thread, obj->GetElements());
        uint32_t size = static_cast<uint32_t>(elements->Size());
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> array = factory->NewTaggedArray(length);
        for (uint32_t hashIndex = 0; hashIndex < size; hashIndex++) {
            JSTaggedValue key = elements->GetKey(hashIndex);
            JSTaggedValue value = elements->GetValue(hashIndex);
            if (key.IsUndefined() || key.IsHole()) {
                continue;
            }
            ASSERT(key.IsInt());
            uint32_t uintKey = static_cast<uint32_t>(key.GetInt());
            if (uintKey < length) {
                array->Set(thread, uintKey, value);
            }
        }
        obj->SetElements(thread, array);
        JSHClass::OptimizeAsFastElements(thread, obj);
    }
}

void JSObject::OptimizeAsFastProperties(const JSThread *thread, JSHandle<JSObject> obj)
{
    ASSERT(obj->GetJSHClass()->IsDictionaryMode());
    // 1. Get NameDictionary properties
    JSHandle<NameDictionary> properties(thread, obj->GetProperties());

    int numberOfProperties = properties->EntriesCount();
    // Make sure we preserve enough capacity
    if (numberOfProperties > static_cast<int>(PropertyAttributes::MAX_FAST_PROPS_CAPACITY)) {
        return ;
    }

    // 2. iteration indices
    std::vector<int> indexOrder = properties->GetEnumerationOrder();
    ASSERT(static_cast<int>(indexOrder.size()) == numberOfProperties);

    // 3. Change Hclass
    int numberOfInlinedProps = static_cast<int>(obj->GetJSHClass()->GetInlinedProperties());
    JSHClass::OptimizeAsFastProperties(thread, obj, indexOrder, true);

    // 4. New out-properties
    int numberOfOutProperties = numberOfProperties - numberOfInlinedProps;
    ASSERT(numberOfOutProperties >= 0);
    JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(numberOfOutProperties);

    // 5. Fill properties
    for (int i = 0; i < numberOfProperties; i++) {
        JSTaggedValue value = properties->GetValue(indexOrder[i]);
        if (i < numberOfInlinedProps) {
            obj->SetPropertyInlinedPropsWithRep(thread, i, value);
        } else {
            array->Set(thread, i - numberOfInlinedProps, value);
        }
    }
    obj->SetProperties(thread, array);
}

void JSObject::SetSProperties(JSThread *thread, JSHandle<JSObject> obj, const std::vector<PropertyDescriptor> &descs)
{
    uint32_t length = descs.size();
    JSMutableHandle<JSTaggedValue> propKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> propValue(thread, JSTaggedValue::Undefined());

    if (LIKELY(!obj->GetJSHClass()->IsDictionaryMode())) {
        for (uint32_t i = 0; i < length; ++i) {
            propValue.Update(descs[i].GetValue());
            // note(lzl): IsSAccessor?
            if (!propValue->IsSharedType() && !propValue->IsAccessor()) {
                THROW_TYPE_ERROR(thread, "The properties of sendable class must be sendable too.");
            }
            obj->SetPropertyInlinedProps(thread, i, propValue.GetTaggedValue());
        }
    } else {
        JSMutableHandle<ecmascript::NameDictionary> dict(
            thread, ecmascript::NameDictionary::CreateInSharedHeap(
                thread, ecmascript::NameDictionary::ComputeHashTableSize(length)));
        for (uint32_t i = 0; i < length; i++) {
            propKey.Update(descs[i].GetKey());
            propValue.Update(descs[i].GetValue());
            PropertyAttributes attr =
                PropertyAttributes::Default(descs[i].IsWritable(), descs[i].IsEnumerable(), descs[i].IsConfigurable());
            JSHandle<ecmascript::NameDictionary> newDict =
                ecmascript::NameDictionary::PutIfAbsent(thread, dict, propKey, propValue, attr);
            dict.Update(newDict);
        }
        obj->SetProperties(thread, dict);
    }
}

bool JSObject::IsArrayLengthWritable(JSThread *thread, const JSHandle<JSObject> &receiver)
{
    auto *hclass = receiver->GetJSHClass();
    if (!hclass->IsDictionaryMode()) {
        LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
        PropertyAttributes attr(layoutInfo->GetAttr(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
        return attr.IsWritable();
    }
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    ObjectOperator op(thread, receiver, lengthKey, OperatorType::OWN);
    return op.GetAttr().IsWritable();
}

bool JSObject::CheckAndUpdateArrayLength(JSThread *thread, const JSHandle<JSObject> &receiver,
                                         uint32_t index, ElementsKind &kind)
{
    if (receiver->IsTypedArray() || receiver->IsSharedTypedArray()) {
        if (!JSTypedArray::IsValidIntegerIndex(JSHandle<JSTaggedValue>(receiver), index)) {
            return false;
        }
        return true;
    }
    if (receiver->IsJSArray()) {
        DISALLOW_GARBAGE_COLLECTION;
        JSArray *arr = JSArray::Cast(*receiver);
        uint32_t oldLength = arr->GetArrayLength();
        if (index >= oldLength) {
            if (!IsArrayLengthWritable(thread, receiver)) {
                return false;
            }
            arr->SetArrayLength(thread, index + 1);
            if (index > oldLength) {
                kind = ElementsKind::HOLE;
            }
        }
        return true;
    }
    if (receiver->IsJSSArray()) {
        uint32_t oldLength = JSSharedArray::Cast(*receiver)->GetArrayLength();
        if (index >= oldLength) {
            JSHandle<JSTaggedValue> newLength(thread, JSTaggedValue(static_cast<uint32_t>(index + 1)));
            JSSharedArray::LengthSetter(thread, receiver, newLength);
            if (index > oldLength) {
                kind = ElementsKind::HOLE;
            }
        }
        return true;
    }
    return true;
}

bool JSObject::AddElementInternal(JSThread *thread, const JSHandle<JSObject> &receiver,
                                  uint32_t index, const JSHandle<JSTaggedValue> &value,
                                  PropertyAttributes attr)
{
    ElementsKind kind = ElementsKind::NONE;
    if (!CheckAndUpdateArrayLength(thread, receiver, index, kind)) {
        return false;
    }

    thread->NotifyArrayPrototypeChangedGuardians(receiver);

    // check whether to convert to dictionary
    if (receiver->GetJSHClass()->IsDictionaryElement() && receiver->IsJSArray()) {
        JSArray *arr = JSArray::Cast(*receiver);
        uint32_t capacity = arr->GetArrayLength();
        TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
        ASSERT(elements->IsDictionaryMode());
        if (ShouldTransToFastElements(thread, elements, capacity, index)) {
            JSObject::TryOptimizeAsFastElements(thread, receiver);
        }
    }

    bool isDictionary = receiver->GetJSHClass()->IsDictionaryElement();
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    if (isDictionary) {
        ASSERT(elements->IsDictionaryMode());
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(static_cast<int32_t>(index)));
        JSHandle<NumberDictionary> newDict =
            NumberDictionary::Put(thread, JSHandle<NumberDictionary>(thread, elements), keyHandle, value, attr);
        receiver->SetElements(thread, newDict);
        return true;
    }

    uint32_t capacity = elements->GetLength();
    if (index >= capacity || !attr.IsDefaultAttributes()) {
        if (!receiver->IsJSSArray() && (ShouldTransToDict(capacity, index) || !attr.IsDefaultAttributes())) {
            JSObject::ElementsToDictionary(thread, receiver);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(static_cast<int32_t>(index)));
            JSHandle<NumberDictionary> dict(thread, receiver->GetElements());
            JSHandle<NumberDictionary> newKey = NumberDictionary::Put(thread, dict, keyHandle, value, attr);
            receiver->SetElements(thread, newKey);
            return true;
        }
        elements = *JSObject::GrowElementsCapacity(thread, receiver, index + 1);
    }
    bool needTransition = true;
    if (receiver->IsJSShared()) {
        needTransition = false;
    }
    ElementAccessor::Set(thread, receiver, index, value, needTransition, kind);
    return true;
}

void JSObject::DeletePropertyInternal(JSThread *thread, const JSHandle<JSObject> &obj,
                                      const JSHandle<JSTaggedValue> &key, uint32_t index)
{
    JSHandle<TaggedArray> array(thread, obj->GetProperties());

    if (obj->IsJSGlobalObject()) {
        JSHandle<GlobalDictionary> dictHandle(thread, obj->GetProperties());
        PropertyBox* box = dictHandle->GetBox(index);
        box->Clear(thread);
        JSHandle<GlobalDictionary> newDict = GlobalDictionary::Remove(thread, dictHandle, index);
        obj->SetProperties(thread, newDict);
        return;
    }

    if (!array->IsDictionaryMode()) {
        JSHandle<NameDictionary> dictHandle(TransitionToDictionary(thread, obj));
        RETURN_IF_ABRUPT_COMPLETION(thread);
        int entry = dictHandle->FindEntry(key.GetTaggedValue());
        ASSERT(entry != -1);
        JSHandle<NameDictionary> newDict = NameDictionary::Remove(thread, dictHandle, entry);
        obj->SetProperties(thread, newDict);
        return;
    }

    JSHandle<NameDictionary> dictHandle(array);
    JSHandle<NameDictionary> newDict = NameDictionary::Remove(thread, dictHandle, index);
    obj->SetProperties(thread, newDict);
}

void JSObject::GetAllKeys(const JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                          const JSHandle<TaggedArray> &keyArray)

{
    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (!array->IsDictionaryMode()) {
        int end = static_cast<int>(obj->GetJSHClass()->NumberOfProps());
        if (end > 0) {
            LayoutInfo::Cast(obj->GetJSHClass()->GetLayout().GetTaggedObject())
                ->GetAllKeys(thread, end, offset, *keyArray);
        }
        return;
    }

    if (obj->IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(array);
        return dict->GetAllKeys(thread, offset, *keyArray);
    }

    NameDictionary *dict = NameDictionary::Cast(obj->GetProperties().GetTaggedObject());
    dict->GetAllKeys(thread, offset, *keyArray);
}

void JSObject::GetAllKeysByFilter(const JSThread *thread, const JSHandle<JSObject> &obj,
                                  uint32_t &keyArrayEffectivelength,
                                  const JSHandle<TaggedArray> &keyArray,
                                  uint32_t filter)
{
    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (!array->IsDictionaryMode()) {
        uint32_t numberOfProps = obj->GetJSHClass()->NumberOfProps();
        if (numberOfProps > 0) {
            LayoutInfo::Cast(obj->GetJSHClass()->GetLayout().GetTaggedObject())->
                GetAllKeysByFilter(thread, numberOfProps, keyArrayEffectivelength, *keyArray, filter);
        }
        return;
    }

    if (obj->IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(array);
        return dict->GetAllKeysByFilter(thread, keyArrayEffectivelength, *keyArray, filter);
    }

    NameDictionary *dict = NameDictionary::Cast(obj->GetProperties().GetTaggedObject());
    dict->GetAllKeysByFilter(thread, keyArrayEffectivelength, *keyArray, filter);
}

// For Serialization use. Does not support JSGlobalObject
void JSObject::GetAllKeysForSerialization(const JSHandle<JSObject> &obj, std::vector<JSTaggedValue> &keyVector)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT_PRINT(!obj->IsJSGlobalObject(), "Do not support get key of JSGlobal Object");
    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (!array->IsDictionaryMode()) {
        int end = static_cast<int>(obj->GetJSHClass()->NumberOfProps());
        if (end > 0) {
            LayoutInfo::Cast(obj->GetJSHClass()->GetLayout().GetTaggedObject())->GetAllKeysForSerialization(end,
                keyVector);
        }
    } else {
        NameDictionary *dict = NameDictionary::Cast(obj->GetProperties().GetTaggedObject());
        dict->GetAllKeysIntoVector(keyVector);
    }
}

JSHandle<TaggedArray> JSObject::GetAllEnumKeys(JSThread *thread, const JSHandle<JSObject> &obj,
                                               uint32_t numOfKeys, uint32_t *keys)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (obj->IsJSGlobalObject()) {
        JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(numOfKeys);
        GlobalDictionary *dict = GlobalDictionary::Cast(obj->GetProperties().GetTaggedObject());
        dict->GetEnumAllKeys(thread, 0, *keyArray, keys);
        return keyArray;
    }

    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (!array->IsDictionaryMode()) {
        JSHClass *jsHclass = obj->GetJSHClass();
        JSTaggedValue enumCache = jsHclass->GetEnumCache();
        if (JSObject::GetEnumCacheKind(thread, enumCache) == EnumCacheKind::ONLY_OWN_KEYS) {
            JSHandle<TaggedArray> cacheArray = JSHandle<TaggedArray>(thread, enumCache);
            JSHandle<TaggedArray> keyArray = factory->CopyFromEnumCache(cacheArray);
            *keys = keyArray->GetLength();
            return keyArray;
        }

        if (numOfKeys > 0) {
            int end = static_cast<int>(jsHclass->NumberOfProps());
            JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(numOfKeys + EnumCache::ENUM_CACHE_HEADER_SIZE);
            LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject())
                ->GetAllEnumKeys(thread, end, EnumCache::ENUM_CACHE_HEADER_SIZE, keyArray, keys);
            JSObject::SetEnumCacheKind(thread, *keyArray, EnumCacheKind::ONLY_OWN_KEYS);
            if (!JSTaggedValue(jsHclass).IsInSharedHeap()) {
                jsHclass->SetEnumCache(thread, keyArray.GetTaggedValue());
            }
            JSHandle<TaggedArray> newkeyArray = factory->CopyFromEnumCache(keyArray);
            return newkeyArray;
        }
        return factory->EmptyArray();
    }

    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(numOfKeys);
    NameDictionary *dict = NameDictionary::Cast(obj->GetProperties().GetTaggedObject());
    dict->GetAllEnumKeys(thread, 0, keyArray, keys);
    return keyArray;
}

uint32_t JSObject::GetAllEnumKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                  const JSHandle<TaggedArray> &keyArray)
{
    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    uint32_t keys = 0;
    if (!array->IsDictionaryMode()) {
        JSHClass *jsHclass = obj->GetJSHClass();
        int end = static_cast<int>(jsHclass->NumberOfProps());
        if (end > 0) {
            LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject())
                ->GetAllEnumKeys(thread, end, offset, keyArray, &keys);
        }
        return keys;
    }
    if (obj->IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(obj->GetProperties().GetTaggedObject());
        dict->GetEnumAllKeys(thread, offset, *keyArray, &keys);
        return keys;
    }

    NameDictionary *dict = NameDictionary::Cast(obj->GetProperties().GetTaggedObject());
    dict->GetAllEnumKeys(thread, offset, keyArray, &keys);
    return keys;
}

void JSObject::GetAllElementKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                 const JSHandle<TaggedArray> &keyArray)
{
    uint32_t elementIndex = 0;
    if (obj->IsJSPrimitiveRef() && JSPrimitiveRef::Cast(*obj)->IsString()) {
        elementIndex = JSPrimitiveRef::Cast(*obj)->GetStringLength() + static_cast<uint32_t>(offset);
        for (uint32_t i = static_cast<uint32_t>(offset); i < elementIndex; ++i) {
            auto key = base::NumberHelper::IntToEcmaString(thread, i);
            keyArray->Set(thread, i, key);
        }
    }

    if (!ElementAccessor::IsDictionaryMode(obj)) {
        uint32_t elementsLen = ElementAccessor::GetElementsLength(obj);
        for (uint32_t i = 0, j = elementIndex; i < elementsLen; ++i) {
            if (!ElementAccessor::Get(thread, obj, i).IsHole()) {
                auto key = base::NumberHelper::IntToEcmaString(thread, i);
                keyArray->Set(thread, j++, key);
            }
        }
    } else {
        JSHandle<TaggedArray> elements(thread, obj->GetElements());
        NumberDictionary::GetAllKeys(thread, JSHandle<NumberDictionary>(elements), elementIndex, keyArray);
    }
}

void JSObject::GetAllElementKeysByFilter(JSThread *thread,
                                         const JSHandle<JSObject> &obj,
                                         const JSHandle<TaggedArray> &keyArray,
                                         uint32_t &keyArrayEffectiveLength,
                                         uint32_t filter)
{
    ASSERT_PRINT(obj->IsECMAObject(), "obj is not object");
    uint32_t elementIndex = 0;

    // For strings attributes, only enumerable is true
    if ((filter & NATIVE_ENUMERABLE) && obj->IsJSPrimitiveRef() && JSPrimitiveRef::Cast(*obj)->IsString()) {
        elementIndex = JSPrimitiveRef::Cast(*obj)->GetStringLength();
        for (uint32_t i = 0; i < elementIndex; ++i) {
            keyArray->Set(thread, keyArrayEffectiveLength, JSTaggedValue(i));
            keyArrayEffectiveLength++;
        }
    }

    JSHandle<JSTaggedValue> objValue(obj);

    if (!ElementAccessor::IsDictionaryMode(obj)) {
        uint32_t elementsLen = ElementAccessor::GetElementsLength(obj);
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!ElementAccessor::Get(thread, obj, i).IsHole()) {
                ObjectOperator op(thread, objValue, i, OperatorType::OWN);
                bool bIgnore = FilterHelper::IgnoreKeyByFilter<ObjectOperator>(op, filter);
                if (bIgnore) {
                    continue;
                }
                keyArray->Set(thread, keyArrayEffectiveLength, JSTaggedValue(i));
                keyArrayEffectiveLength++;
            }
        }
    } else {
        JSHandle<TaggedArray> elements(thread, obj->GetElements());
        NumberDictionary::GetAllKeysByFilter(thread, JSHandle<NumberDictionary>(elements),
            keyArrayEffectiveLength, keyArray, filter);
    }
}

void JSObject::GetALLElementKeysIntoVector(const JSThread *thread, const JSHandle<JSObject> &obj,
                                           std::vector<JSTaggedValue> &keyVector)
{
    if (!ElementAccessor::IsDictionaryMode(obj)) {
        uint32_t elementsLen = ElementAccessor::GetElementsLength(obj);
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!ElementAccessor::Get(thread, obj, i).IsHole()) {
                keyVector.emplace_back(JSTaggedValue(i));
            }
        }
    } else {
        JSHandle<TaggedArray> elements(thread, obj->GetElements());
        JSHandle<NumberDictionary> dict = JSHandle<NumberDictionary>::Cast(elements);
        dict->GetAllKeysIntoVector(keyVector);
    }
}

JSHandle<TaggedArray> JSObject::GetEnumElementKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                                   uint32_t numOfElements, uint32_t *keys)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> elementArray = factory->NewTaggedArray(numOfElements);
    CollectEnumElementsAlongProtoChain(thread, obj, offset, elementArray, keys);
    return elementArray;
}

void JSObject::CollectEnumElementsAlongProtoChain(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                                  JSHandle<TaggedArray> elementArray, uint32_t *keys,
                                                  int32_t lastLength)
{
    uint32_t elementIndex = static_cast<uint32_t>(offset);
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());

    if (obj->IsJSPrimitiveRef() && JSPrimitiveRef::Cast(*obj)->IsString()) {
        uint32_t strLen = JSPrimitiveRef::Cast(*obj)->GetStringLength();
        for (uint32_t i = 0; i < strLen; ++i) {
            keyHandle.Update(base::NumberHelper::IntToEcmaString(thread, i));
            elementArray->Set(thread, elementIndex, keyHandle);
            elementIndex++;
        }
        *keys += strLen;
    }

    if (!ElementAccessor::IsDictionaryMode(obj)) {
        JSHandle<TaggedQueue> emptyQueue = thread->GetEcmaVM()->GetFactory()->GetEmptyTaggedQueue();
        uint32_t elementsLen = ElementAccessor::GetElementsLength(obj);
        uint32_t preElementIndex = elementIndex;
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (ElementAccessor::Get(thread, obj, i).IsHole()) {
                continue;
            }
            keyHandle.Update(base::NumberHelper::IntToEcmaString(thread, i));
            bool isDuplicated = IsDepulicateKeys(thread, elementArray, lastLength, emptyQueue, keyHandle);
            if (isDuplicated) {
                continue;
            }
            elementArray->Set(thread, elementIndex, keyHandle);
            elementIndex++;
        }
        *keys += (elementIndex - preElementIndex);
    } else {
        JSHandle<TaggedArray> arr(thread, obj->GetElements());
        NumberDictionary::GetAllEnumKeys(
            thread, JSHandle<NumberDictionary>(arr), elementIndex, elementArray, keys, lastLength);
    }
}

void JSObject::GetEnumElementKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                  const JSHandle<TaggedArray> &keyArray)
{
    uint32_t elementIndex = 0;
    if (obj->IsJSPrimitiveRef() && JSPrimitiveRef::Cast(*obj)->IsString()) {
        elementIndex = JSPrimitiveRef::Cast(*obj)->GetStringLength() + static_cast<uint32_t>(offset);
        for (uint32_t i = static_cast<uint32_t>(offset); i < elementIndex; ++i) {
            auto key = base::NumberHelper::IntToEcmaString(thread, i);
            keyArray->Set(thread, i, key);
        }
    }

    if (!ElementAccessor::IsDictionaryMode(obj)) {
        uint32_t elementsLen = ElementAccessor::GetElementsLength(obj);
        for (uint32_t i = 0, j = elementIndex; i < elementsLen; ++i) {
            if (!ElementAccessor::Get(thread, obj, i).IsHole()) {
                auto key = base::NumberHelper::IntToEcmaString(thread, i);
                keyArray->Set(thread, j++, key);
            }
        }
    } else {
        JSHandle<TaggedArray> elements(thread, obj->GetElements());
        uint32_t keys = 0;
        NumberDictionary::GetAllEnumKeys(thread, JSHandle<NumberDictionary>(elements), elementIndex, keyArray, &keys);
    }
}

std::pair<uint32_t, uint32_t> JSObject::GetNumberOfEnumKeys() const
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray *array = TaggedArray::Cast(GetProperties().GetTaggedObject());
    if (!array->IsDictionaryMode()) {
        int end = static_cast<int>(GetJSHClass()->NumberOfProps());
        if (end > 0) {
            LayoutInfo *layout = LayoutInfo::Cast(GetJSHClass()->GetLayout().GetTaggedObject());
            return layout->GetNumOfEnumKeys(end);
        }
        return std::make_pair(0, 0);
    }
    if (IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(array);
        return dict->GetNumOfEnumKeys();
    }

    NameDictionary *dict = NameDictionary::Cast(GetProperties().GetTaggedObject());
    return dict->GetNumOfEnumKeys();
}

uint32_t JSObject::GetNumberOfKeys()
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray *array = TaggedArray::Cast(GetProperties().GetTaggedObject());

    if (!array->IsDictionaryMode()) {
        return GetJSHClass()->NumberOfProps();
    }

    return NameDictionary::Cast(array)->EntriesCount();
}

bool JSObject::GlobalSetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                 JSHandle<JSTaggedValue> value, bool mayThrow)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    ObjectOperator op(thread, key);
    if (!op.IsFound()) {
        PropertyAttributes attr = PropertyAttributes::Default(true, true, false);
        op.SetAttr(attr);
    }
    return SetProperty(&op, value, mayThrow);
}

uint32_t JSObject::GetNumberOfElements(JSThread *thread)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t numOfElements = 0;
    if (IsJSPrimitiveRef() && JSPrimitiveRef::Cast(this)->IsString()) {
        numOfElements = JSPrimitiveRef::Cast(this)->GetStringLength();
    }

    if (!ElementAccessor::IsDictionaryMode(this)) {
        uint32_t elementsLen = ElementAccessor::GetElementsLength(this);
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!ElementAccessor::Get(thread, this, i).IsHole()) {
                numOfElements++;
            }
        }
    } else {
        TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
        numOfElements += static_cast<uint32_t>(NumberDictionary::Cast(elements)->EntriesCount());
    }

    return numOfElements;
}

// 9.1.9 [[Set]] ( P, V, Receiver)
bool JSObject::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                           JSHandle<JSTaggedValue> value, const JSHandle<JSTaggedValue> &receiver, bool mayThrow)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    // 2 ~ 4 findProperty in Receiver, Obj and its parents
    ObjectOperator op(thread, obj, receiver, key);
    return SetProperty(&op, value, mayThrow);
}

bool JSObject::SetProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                           JSHandle<JSTaggedValue> value, bool mayThrow)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid JSObject");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    ObjectOperator op(thread, obj, key);
    return SetProperty(&op, value, mayThrow);
}

bool JSObject::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                           JSHandle<JSTaggedValue> value, bool mayThrow, SCheckMode sCheckMode)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    if (obj->IsJSSharedArray()) {
        return JSSharedArray::SetProperty(thread, obj, key, value, mayThrow, sCheckMode);
    }
    // 2 ~ 4 findProperty in Receiver, Obj and its parents
    ObjectOperator op(thread, obj, key);
    return SetProperty(&op, value, mayThrow);
}

bool JSObject::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index,
                           JSHandle<JSTaggedValue> value, bool mayThrow)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");

    ObjectOperator op(thread, obj, index);
    return SetProperty(&op, value, mayThrow);
}

bool JSObject::SetPropertyForDataDescriptorProxy(JSThread *thread, ObjectOperator *op,
                                                 const JSHandle<JSTaggedValue> &value,
                                                 JSHandle<JSTaggedValue> &receiver)
{
    ASSERT(receiver->IsJSProxy());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    if (op->IsElement()) {
        key.Update(JSTaggedValue(op->GetElementIndex()));
    } else {
        key.Update(op->GetKey().GetTaggedValue());
    }

    PropertyDescriptor existDesc(thread);
    JSProxy::GetOwnProperty(thread, JSHandle<JSProxy>::Cast(receiver), key, existDesc);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (!existDesc.IsEmpty()) {
        if (existDesc.IsAccessorDescriptor()) {
            return false;
        }

        if (!existDesc.IsWritable()) {
            return false;
        }

        PropertyDescriptor valueDesc(thread, value);
        return JSProxy::DefineOwnProperty(thread, JSHandle<JSProxy>::Cast(receiver), key, valueDesc);
    }
    return CreateDataProperty(thread, JSHandle<JSObject>(receiver), key, value);
}

bool JSObject::SetPropertyForDataDescriptor(ObjectOperator *op, JSHandle<JSTaggedValue> value,
                                            JSHandle<JSTaggedValue> &receiver, bool mayThrow, bool isInternalAccessor)
{
    JSThread *thread = op->GetThread();
    if (!op->IsWritable()) {
        if (mayThrow) {
            THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(SetReadOnlyProperty), false);
        }
        return false;
    }

    if (!receiver->IsECMAObject()) {
        if (mayThrow) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Receiver is not a JSObject", false);
        }
        return false;
    }
    if (op->IsFound() && receiver->IsJSShared()) {
        SharedFieldType type = op->GetSharedFieldType();
        if (!ClassHelper::MatchFieldType(type, value.GetTaggedValue())) {
            if (mayThrow) {
                THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(SetTypeMismatchedSharedProperty), false);
            }
            return false;
        }
        value = JSTaggedValue::PublishSharedValue(thread, value);
    }

    if (receiver->IsJSProxy()) {
        return SetPropertyForDataDescriptorProxy(thread, op, value, receiver);
    }

    // 5e. If existingDescriptor is not undefined, then
    bool hasReceiver = false;
    if (op->HasReceiver()) {
        op->ReLookupPropertyInReceiver();
        isInternalAccessor = false;
        if (op->IsAccessorDescriptor()) {
            JSTaggedValue ret = ShouldGetValueFromBox(op);
            isInternalAccessor = AccessorData::Cast(ret.GetTaggedObject())->IsInternal();
        }
        hasReceiver = true;
    }
    bool isSuccess = true;
    if (op->IsFound() && !op->IsOnPrototype()) {
        // i. If IsAccessorDescriptor(existingDescriptor) is true, return false.
        if (op->IsAccessorDescriptor() && !isInternalAccessor) {
            return false;
        }

        // ii. If existingDescriptor.[[Writable]] is false, return false.
        if (!op->IsWritable()) {
            if (mayThrow) {
                THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(SetReadOnlyProperty), false);
            }
            return false;
        }
        if (hasReceiver && receiver->IsJSShared()) {
            SharedFieldType type = op->GetSharedFieldType();
            if (!ClassHelper::MatchFieldType(type, value.GetTaggedValue())) {
                if (mayThrow) {
                    THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(SetTypeMismatchedSharedProperty), false);
                }
                return false;
            }
            value = JSTaggedValue::PublishSharedValue(thread, value);
        }
        isSuccess = op->UpdateDataValue(JSHandle<JSObject>(receiver), value, isInternalAccessor, mayThrow);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, isSuccess);
    } else {
        // 5f. Else if Receiver does not currently have a property P, Return CreateDataProperty(Receiver, P, V).
        // fixme(hzzhouzebin) this makes SharedArray's frozen no sense.
        if (!receiver->IsExtensible(thread) && !(receiver->IsJSSharedArray() && op->IsElement())) {
            if (mayThrow) {
                THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(SetPropertyWhenNotExtensible), false);
            }
            return false;
        }
        if (hasReceiver || isInternalAccessor) {
            return op->AddProperty(JSHandle<JSObject>(receiver), value, PropertyAttributes::Default());
        } else if (op->IsFound() && receiver.GetTaggedValue() != op->GetHolder().GetTaggedValue()) {
            return op->AddProperty(JSHandle<JSObject>(receiver), value, PropertyAttributes::Default());
        } else {
            return op->AddProperty(JSHandle<JSObject>(receiver), value, op->GetAttr());
        }
    }
    return isSuccess;
}

bool JSObject::SetProperty(ObjectOperator *op, JSHandle<JSTaggedValue> value, bool mayThrow)
{
    JSThread *thread = op->GetThread();
    op->UpdateDetector();

    JSHandle<JSTaggedValue> receiver = op->GetReceiver();
    JSHandle<JSTaggedValue> holder = op->GetHolder();
    if (holder->IsJSProxy()) {
        if (op->IsElement()) {
            JSHandle<JSTaggedValue> key(thread, JSTaggedValue(op->GetElementIndex()));
            return JSProxy::SetProperty(thread, JSHandle<JSProxy>::Cast(holder), key, value, receiver, mayThrow);
        }
        return JSProxy::SetProperty(thread, JSHandle<JSProxy>::Cast(holder), op->GetKey(), value, receiver, mayThrow);
    }

    // When op is not found and is not set extra attributes
    if (!op->IsFound() && op->IsPrimitiveAttr()) {
        op->SetAsDefaultAttr();
    }

    bool isInternalAccessor = false;
    if (op->IsAccessorDescriptor()) {
        JSTaggedValue ret = ShouldGetValueFromBox(op);
        isInternalAccessor = AccessorData::Cast(ret.GetTaggedObject())->IsInternal();
    }

    // 5. If IsDataDescriptor(ownDesc) is true, then
    if (!op->IsAccessorDescriptor() || isInternalAccessor) {
        return SetPropertyForDataDescriptor(op, value, receiver, mayThrow, isInternalAccessor);
    }
    // 6. Assert: IsAccessorDescriptor(ownDesc) is true.
    ASSERT(op->IsAccessorDescriptor());
    // 8. If setter is undefined, return false.
    JSTaggedValue ret = ShouldGetValueFromBox(op);
    AccessorData *accessor = AccessorData::Cast(ret.GetTaggedObject());
    return CallSetter(thread, *accessor, receiver, value, mayThrow);
}

bool JSObject::SetPropertyForData(ObjectOperator *op, const JSHandle<JSTaggedValue> &value, bool *isAccessor)
{
    JSThread *thread = op->GetThread();
    op->UpdateDetector();

    JSHandle<JSTaggedValue> receiver = op->GetReceiver();
    JSHandle<JSTaggedValue> holder = op->GetHolder();
    if (holder->IsJSProxy()) {
        if (op->IsElement()) {
            JSHandle<JSTaggedValue> key(thread, JSTaggedValue(op->GetElementIndex()));
            return JSProxy::SetProperty(thread, JSHandle<JSProxy>::Cast(holder), key, value, receiver, true);
        }
        return JSProxy::SetProperty(thread, JSHandle<JSProxy>::Cast(holder), op->GetKey(), value, receiver, true);
    }

    // When op is not found and is not set extra attributes
    if (!op->IsFound() && op->IsPrimitiveAttr()) {
        op->SetAsDefaultAttr();
    }

    bool isInternalAccessor = false;
    if (op->IsAccessorDescriptor()) {
        JSTaggedValue ret = ShouldGetValueFromBox(op);
        isInternalAccessor = AccessorData::Cast(ret.GetTaggedObject())->IsInternal();
    }

    // 5. If IsDataDescriptor(ownDesc) is true, then
    if (!op->IsAccessorDescriptor() || isInternalAccessor) {
        return SetPropertyForDataDescriptor(op, value, receiver, true, isInternalAccessor);
    }
    // 6. Assert: IsAccessorDescriptor(ownDesc) is true.
    ASSERT(op->IsAccessorDescriptor());
    *isAccessor = true;
    return true;
}

bool JSObject::SetPropertyForAccessor(ObjectOperator *op, const JSHandle<JSTaggedValue> &value)
{
    JSThread *thread = op->GetThread();
    JSHandle<JSTaggedValue> receiver = op->GetReceiver();
    JSTaggedValue ret = JSObject::ShouldGetValueFromBox(op);
    AccessorData *accessor = AccessorData::Cast(ret.GetTaggedObject());
    return JSObject::CallSetter(thread, *accessor, receiver, value, true);
}

bool JSObject::CallSetter(JSThread *thread, const AccessorData &accessor, const JSHandle<JSTaggedValue> &receiver,
                          const JSHandle<JSTaggedValue> &value, bool mayThrow)
{
    if (UNLIKELY(accessor.IsInternal())) {
        return accessor.CallInternalSet(thread, JSHandle<JSObject>::Cast(receiver), value, mayThrow);
    }
    JSTaggedValue setter = accessor.GetSetter();
    // 8. If setter is undefined, return false.
    if (setter.IsUndefined()) {
        if (mayThrow) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot set property when setter is undefined", false);
        }
        return false;
    }

    JSHandle<JSTaggedValue> func(thread, setter);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, receiver, undefined, 1);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    info->SetCallArg(value.GetTaggedValue());
    JSFunction::Call(info);

    // 10. ReturnIfAbrupt(setterResult).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    return true;
}

JSTaggedValue JSObject::CallGetter(JSThread *thread, const AccessorData *accessor,
                                   const JSHandle<JSTaggedValue> &receiver)
{
    JSTaggedValue getter = accessor->GetGetter();
    // 7. If getter is undefined, return undefined.
    if (getter.IsUndefined()) {
        return JSTaggedValue::Undefined();
    }

    JSHandle<JSTaggedValue> func(thread, getter);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, receiver, undefined, 0);
    JSTaggedValue res = JSFunction::Call(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

// 9.1.8 [[Get]] (P, Receiver)
OperationResult JSObject::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    ObjectOperator op(thread, obj, receiver, key);
    return OperationResult(thread, GetProperty(thread, &op), PropertyMetaData(op.IsFound()));
}

OperationResult JSObject::GetProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                      const JSHandle<JSTaggedValue> &key)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid JSObject");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    ObjectOperator op(thread, obj, key);
    return OperationResult(thread, GetProperty(thread, &op), PropertyMetaData(op.IsFound()));
}

OperationResult JSObject::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<JSTaggedValue> &key, SCheckMode sCheckMode)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    if (obj->IsJSSharedArray()) {
        return JSSharedArray::GetProperty(thread, obj, key, sCheckMode);
    }
    ObjectOperator op(thread, obj, key);
    return OperationResult(thread, GetProperty(thread, &op), PropertyMetaData(op.IsFound()));
}

OperationResult JSObject::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");

    ObjectOperator op(thread, obj, index);
    return OperationResult(thread, GetProperty(thread, &op), PropertyMetaData(op.IsFound()));
}

OperationResult JSObject::GetPropertyFromGlobal(JSThread *thread, const JSHandle<JSTaggedValue> &key)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    ObjectOperator op(thread, key);
    return OperationResult(thread, GetProperty(thread, &op), PropertyMetaData(op.IsFound()));
}

PropertyBox* JSObject::GetGlobalPropertyBox(JSTaggedValue key)
{
    ASSERT(IsJSGlobalObject());
    auto dict = GlobalDictionary::Cast(GetProperties().GetTaggedObject());
    auto entry = dict->FindEntry(key);
    if (entry == -1) {
        return nullptr;
    }
    return dict->GetBox(entry);
}

PropertyBox* JSObject::GetGlobalPropertyBox(JSThread *thread, const std::string& key)
{
    auto factory = thread->GetEcmaVM()->GetFactory();
    auto keyValue = factory->NewFromUtf8(key).GetTaggedValue();
    return GetGlobalPropertyBox(keyValue);
}

JSTaggedValue JSObject::GetProperty(JSThread *thread, ObjectOperator *op)
{
    JSHandle<JSTaggedValue> receiver = op->GetReceiver();
    JSHandle<JSTaggedValue> holder = op->GetHolder();
    if (receiver->IsNativeModuleFailureInfo()) {
        JSTaggedValue failureInfo = JSHandle<NativeModuleFailureInfo>::Cast(receiver)->GetArkNativeModuleFailureInfo();
        THROW_REFERENCE_ERROR_AND_RETURN(thread, ConvertToString(failureInfo).c_str(), JSTaggedValue::Undefined());
    }
    if (holder->IsJSProxy()) {
        if (op->IsElement()) {
            JSHandle<JSTaggedValue> key(thread, JSTaggedValue(op->GetElementIndex()));
            return JSProxy::GetProperty(thread, JSHandle<JSProxy>::Cast(holder), key, receiver)
                .GetValue()
                .GetTaggedValue();
        }
        return JSProxy::GetProperty(thread, JSHandle<JSProxy>::Cast(holder), op->GetKey(), receiver)
            .GetValue()
            .GetTaggedValue();
    }

    // 4. If desc is undefined, then
    if (!op->IsFound()) {
        // 4c. If obj and parent is null, return undefined.
        return JSTaggedValue::Undefined();
    }
    // 5. If IsDataDescriptor(desc) is true, return desc.[[Value]]
    JSTaggedValue ret = ShouldGetValueFromBox(op);
    if (!op->IsAccessorDescriptor()) {
        return ret;
    }

    // 6. Otherwise, IsAccessorDescriptor(desc) must be true so, let getter be desc.[[Get]].
    AccessorData *accessor = AccessorData::Cast(ret.GetTaggedObject());
    // 8. Return Call(getter, Receiver).
    if (UNLIKELY(accessor->IsInternal())) {
        return accessor->CallInternalGet(thread, JSHandle<JSObject>::Cast(holder));
    }
    return CallGetter(thread, accessor, receiver);
}

bool JSObject::DeleteProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                              SCheckMode sCheckMode)
{
    // 1. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 2. Let desc be O.[[GetOwnProperty]](P).
    ObjectOperator op(thread, JSHandle<JSTaggedValue>(obj), key, OperatorType::OWN);

    // 4. If desc is undefined, return true.
    if (!op.IsFound()) {
        return true;
    }
    // 5. If desc.[[Configurable]] is true, then
    // a. Remove the own property with name P from O.
    // b. Return true.
    // 6. Return false.
    if (op.IsConfigurable() || sCheckMode == SCheckMode::SKIP) {
        op.DeletePropertyInHolder();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        obj->GetClass()->SetHasDeleteProperty(true);
        return true;
    }
    return false;
}

bool JSObject::GetOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                              PropertyDescriptor &desc)
{
    return OrdinaryGetOwnProperty(thread, obj, key, desc);
}

bool JSObject::GlobalGetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    ObjectOperator op(thread, key, OperatorType::OWN);

    if (!op.IsFound()) {
        return false;
    }

    op.ToPropertyDescriptor(desc);

    if (desc.HasValue()) {
        PropertyBox *cell = PropertyBox::Cast(desc.GetValue().GetTaggedValue().GetTaggedObject());
        JSHandle<JSTaggedValue> valueHandle(thread, cell->GetValue());
        desc.SetValue(valueHandle);
    }
    ASSERT(!desc.GetValue()->IsInternalAccessor());
    return true;
}

bool JSObject::OrdinaryGetOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                      const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    ObjectOperator op(thread, JSHandle<JSTaggedValue>(obj), key, OperatorType::OWN);

    if (!op.IsFound()) {
        return false;
    }

    op.ToPropertyDescriptor(desc);

    if (desc.HasValue() && obj->IsJSGlobalObject()) {
        JSTaggedValue val = desc.GetValue().GetTaggedValue();
        if (val.IsPropertyBox()) {
            PropertyBox *cell = PropertyBox::Cast(val.GetTaggedObject());
            JSHandle<JSTaggedValue> valueHandle(thread, cell->GetValue());
            desc.SetValue(valueHandle);
        }
    }

    return true;
}

bool JSObject::DefineOwnProperty(JSThread *thread, ObjectOperator *op,
                                 const PropertyDescriptor &desc, SCheckMode sCheckMode)
{
    return OrdinaryDefineOwnProperty(thread, op, desc, sCheckMode);
}

bool JSObject::DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                 const PropertyDescriptor &desc, SCheckMode sCheckMode)
{
    return OrdinaryDefineOwnProperty(thread, obj, key, desc, sCheckMode);
}

bool JSObject::DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                 const PropertyDescriptor &desc, SCheckMode sCheckMode)
{
    return OrdinaryDefineOwnProperty(thread, obj, index, desc, sCheckMode);
}

// 9.1.6.1 OrdinaryDefineOwnProperty (O, P, Desc)
bool JSObject::OrdinaryDefineOwnProperty(JSThread *thread, ObjectOperator *op,
                                         const PropertyDescriptor &desc, SCheckMode sCheckMode)
{
    auto obj = JSHandle<JSObject>::Cast(op->GetHolder());
    bool extensible = obj->IsExtensible();
    // make extensible for shared array to add element.
    if (obj->IsJSSArray() && op->IsElement()) {
        extensible = true;
    }
    PropertyDescriptor current(thread);
    op->ToPropertyDescriptor(current);
    // 4. Return ValidateAndApplyPropertyDescriptor(O, P, extensible, Desc, current).
    return ValidateAndApplyPropertyDescriptor(op, extensible, desc, current, sCheckMode);
}

bool JSObject::OrdinaryDefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                         const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc,
                                         SCheckMode sCheckMode)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 1. Let current be O.[[GetOwnProperty]](P).
    JSHandle<JSTaggedValue> objValue(obj);
    ObjectOperator op(thread, objValue, key, OperatorType::OWN);
    return OrdinaryDefineOwnProperty(thread, &op, desc, sCheckMode);
}

bool JSObject::OrdinaryDefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                         const PropertyDescriptor &desc, SCheckMode sCheckMode)
{
    JSHandle<JSTaggedValue> objValue(obj);
    ObjectOperator op(thread, objValue, index, OperatorType::OWN);

    bool extensible = obj->IsExtensible();
    PropertyDescriptor current(thread);
    op.ToPropertyDescriptor(current);
    return ValidateAndApplyPropertyDescriptor(&op, extensible, desc, current, sCheckMode);
}

bool JSObject::ValidateDataDescriptorWhenConfigurable(ObjectOperator *op, const PropertyDescriptor &desc,
                                                      const PropertyDescriptor &current, SCheckMode sCheckMode)
{
    // 8a i. Return false, if the [[Writable]] field of current is false and the [[Writable]] field of Desc
    // is true.
    if (!current.IsWritable() && desc.HasWritable() && desc.IsWritable()) {
        return false;
    }
    // 8a ii. If the [[Writable]] field of current is false, then
    if (!current.IsWritable()) {
        if (desc.HasValue() && !JSTaggedValue::SameValue(current.GetValue(), desc.GetValue())) {
            return false;
        }
    }
    if (op->HasHolder() && op->GetHolder()->IsJSShared()) {
        SharedFieldType type = current.GetSharedFieldType();
        JSHandle<JSTaggedValue> value(desc.GetValue());
        if (sCheckMode == SCheckMode::CHECK) {
            if (!desc.HasValue()) {
                THROW_TYPE_ERROR_AND_RETURN(op->GetThread(), GET_MESSAGE_STRING(UpdateSendableAttributes), false);
            }
            if (!ClassHelper::MatchFieldType(type, value.GetTaggedValue())) {
                THROW_TYPE_ERROR_AND_RETURN(op->GetThread(),
                                            GET_MESSAGE_STRING(SetTypeMismatchedSharedProperty), false);
            }
        }
        value = JSTaggedValue::PublishSharedValue(op->GetThread(), value);
    }
    return true;
}

// 9.1.6.3 ValidateAndApplyPropertyDescriptor (O, P, extensible, Desc, current)
bool JSObject::ValidateAndApplyPropertyDescriptor(ObjectOperator *op, bool extensible, const PropertyDescriptor &desc,
                                                  const PropertyDescriptor &current, SCheckMode sCheckMode)
{
    // 2. If current is undefined, then
    if (current.IsEmpty()) {
        // 2a. If extensible is false, return false.
        if (!(extensible || (op->HasHolder() && op->GetHolder()->IsJSShared() && sCheckMode == SCheckMode::SKIP))) {
            return false;
        }
        if (!op->HasHolder()) {
            return true;
        }

        // 2c. If IsGenericDescriptor(Desc) or IsDataDescriptor(Desc) is true, then
        PropertyAttributes attr(desc);
        bool success = false;
        if (!desc.IsAccessorDescriptor()) {
            op->UpdateDetector();
            success = op->AddPropertyInHolder(desc.GetValue(), attr);
        } else {  // is AccessorDescriptor
            // may GC in NewAccessorData, so we need to handle getter and setter.
            JSThread *thread = op->GetThread();
            JSHandle<AccessorData> accessor = thread->GetEcmaVM()->GetFactory()->NewAccessorData();
            if (desc.HasGetter()) {
                accessor->SetGetter(thread, desc.GetGetter());
            }

            if (desc.HasSetter()) {
                accessor->SetSetter(thread, desc.GetSetter());
            }
            op->UpdateDetector();
            success = op->AddPropertyInHolder(JSHandle<JSTaggedValue>::Cast(accessor), attr);
        }

        return success;
    }

    // 3. Return true, if every field in Desc is absent
    // 4. Return true, if every field in Desc also occurs in current and the value of every field in Desc is the
    // same value as the corresponding field in current when compared using the SameValue algorithm.
    if ((!desc.HasEnumerable() || desc.IsEnumerable() == current.IsEnumerable()) &&
        (!desc.HasConfigurable() || desc.IsConfigurable() == current.IsConfigurable()) &&
        (!desc.HasValue() || JSTaggedValue::SameValue(current.GetValue(), desc.GetValue())) &&
        (!desc.HasWritable() || (current.IsWritable() == desc.IsWritable())) &&
        (!desc.HasGetter() ||
         (current.HasGetter() && JSTaggedValue::SameValue(current.GetGetter(), desc.GetGetter()))) &&
        (!desc.HasSetter() ||
         (current.HasSetter() && JSTaggedValue::SameValue(current.GetSetter(), desc.GetSetter())))) {
        return true;
    }

    // 5. If the [[Configurable]] field of current is false, then
    if (!current.IsConfigurable()) {
        // 5a. Return false, if the [[Configurable]] field of Desc is true.
        if (desc.HasConfigurable() && desc.IsConfigurable()) {
            return false;
        }
        // b. Return false, if the [[Enumerable]] field of Desc is present and the [[Enumerable]] fields of current
        // and Desc are the Boolean negation of each other.
        if (desc.HasEnumerable() && (desc.IsEnumerable() != current.IsEnumerable())) {
            return false;
        }
    }

    // 6. If IsGenericDescriptor(Desc) is true, no further validation is required.
    if (desc.IsGenericDescriptor()) {
        // 7. Else if IsDataDescriptor(current) and IsDataDescriptor(Desc) have different results, then
    } else if (current.IsDataDescriptor() != desc.IsDataDescriptor()) {
        // 7a. Return false, if the [[Configurable]] field of current is false.
        if (!current.IsConfigurable()) {
            return false;
        }
        // 7b. If IsDataDescriptor(current) is true, then
        if (current.IsDataDescriptor()) {
            // 7bi. If O is not undefined, convert the property named P of object O from a data property to an
            // accessor property. Preserve the existing values of the converted property’s [[Configurable]] and
            // [[Enumerable]] attributes and set the rest of the property’s attributes to their default values.
        } else {
            // 7ci.  If O is not undefined, convert the property named P of object O from an accessor property to a
            // data property. Preserve the existing values of the converted property’s [[Configurable]] and
            // [[Enumerable]] attributes and set the rest of the property’s attributes to their default values.
        }
        // 8. Else if IsDataDescriptor(current) and IsDataDescriptor(Desc) are both true, then
    } else if (current.IsDataDescriptor() && desc.IsDataDescriptor()) {
        // 8a. If the [[Configurable]] field of current is false, then
        if (!current.IsConfigurable() && !ValidateDataDescriptorWhenConfigurable(op, desc, current, sCheckMode)) {
            return false;
        }
        // 8b. Else the [[Configurable]] field of current is true, so any change is acceptable.
    } else {  // 9. Else IsAccessorDescriptor(current) and IsAccessorDescriptor(Desc) are both true,
        // 9a. If the [[Configurable]] field of current is false, then
        if (!current.IsConfigurable()) {
            // i. Return false, if the [[Set]] field of Desc is present and SameValue(Desc.[[Set]], current.[[Set]])
            // is false.
            if (desc.HasSetter() && !JSTaggedValue::SameValue(current.GetSetter(), desc.GetSetter())) {
                return false;
            }
            // ii. Return false, if the [[Get]] field of Desc is present and SameValue(Desc.[[Get]],
            // current.[[Get]]) is false.
            if (desc.HasGetter() && !JSTaggedValue::SameValue(current.GetGetter(), desc.GetGetter())) {
                return false;
            }
        }
    }

    if (op->HasHolder()) {
        // 10. If O is not undefined, then
        // a. For each field of Desc that is present, set the corresponding attribute of the property named P of object
        // O to the value of the field.
        if (!desc.HasValue() && desc.HasWritable() && current.HasValue()) {
            // [[Value]] and [[Writable]] attributes are set to the value of the corresponding field in Desc
            // if Desc has that field or to the attribute's default value otherwise.
            PropertyDescriptor newDesc = desc;
            JSHandle<JSTaggedValue> valueHandle = current.GetValue();
            if (valueHandle->IsPropertyBox()) {
                JSTaggedValue value = PropertyBox::Cast(valueHandle->GetTaggedObject())->GetValue();
                valueHandle = JSHandle<JSTaggedValue>(op->GetThread(), value);
            }
            newDesc.SetValue(valueHandle);
            op->UpdateDetector();
            return op->WriteDataPropertyInHolder(newDesc);
        }
        op->UpdateDetector();
        return op->WriteDataPropertyInHolder(desc);
    }
    return true;
}

// 9.1.6.2 IsCompatiblePropertyDescriptor (Extensible, Desc, Current)
bool JSObject::IsCompatiblePropertyDescriptor(bool extensible, const PropertyDescriptor &desc,
                                              const PropertyDescriptor &current)
{
    // 1. Return ValidateAndApplyPropertyDescriptor(undefined, undefined, Extensible, Desc, Current).
    ObjectOperator op;
    return ValidateAndApplyPropertyDescriptor(&op, extensible, desc, current);
}

JSTaggedValue JSObject::GetPrototype(const JSHandle<JSObject> &obj)
{
    JSHClass *hclass = obj->GetJSHClass();
    return hclass->GetPrototype();
}

bool JSObject::SetPrototype(JSThread *thread, const JSHandle<JSObject> &obj,
                            const JSHandle<JSTaggedValue> &proto,
                            bool isChangeProto)
{
    ASSERT_PRINT(proto->IsECMAObject() || proto->IsNull(), "proto must be object or null");
    JSTaggedValue current = JSObject::GetPrototype(obj);
    if (current == proto.GetTaggedValue()) {
        return true;
    }
    if (!obj->IsExtensible()) {
        return false;
    }
    bool done = false;
    JSMutableHandle<JSTaggedValue> tempProtoHandle(thread, proto.GetTaggedValue());
    while (!done) {
        if (tempProtoHandle->IsNull() || !tempProtoHandle->IsECMAObject()) {
            done = true;
        } else if (JSTaggedValue::SameValue(tempProtoHandle.GetTaggedValue(), obj.GetTaggedValue())) {
            return false;
        } else {
            if (tempProtoHandle->IsJSProxy()) {
                break;
            }
            tempProtoHandle.Update(JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(tempProtoHandle)));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        }
    }
    ElementsKind oldKind = obj->GetJSHClass()->GetElementsKind();
    // map transition
    JSHClass::SetPrototypeTransition(thread, obj, proto, isChangeProto);
    TryMigrateToGenericKindForJSObject(thread, obj, oldKind);
    return true;
}

bool JSObject::HasProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    JSHandle<JSTaggedValue> objValue(obj);
    ObjectOperator op(thread, objValue, key);

    JSHandle<JSTaggedValue> holder = op.GetHolder();
    if (holder->IsJSProxy()) {
        return JSProxy::HasProperty(thread, JSHandle<JSProxy>::Cast(holder), key);
    }

    return op.IsFound();
}

bool JSObject::HasProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index)
{
    JSHandle<JSTaggedValue> objValue(obj);
    ObjectOperator op(thread, objValue, index);

    JSHandle<JSTaggedValue> holder = op.GetHolder();
    if (holder->IsJSProxy()) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(index));
        return JSProxy::HasProperty(thread, JSHandle<JSProxy>::Cast(holder), key);
    }

    return op.IsFound();
}

bool JSObject::PreventExtensions(JSThread *thread, const JSHandle<JSObject> &obj)
{
    if (obj->IsExtensible()) {
        JSHandle<JSHClass> jshclass(thread, obj->GetJSHClass());
        JSHandle<JSHClass> newHclass = JSHClass::TransitionExtension(thread, jshclass);
#if ECMASCRIPT_ENABLE_IC
        JSHClass::NotifyHclassChanged(thread, jshclass, newHclass);
#endif
        ElementsKind oldKind = obj->GetJSHClass()->GetElementsKind();
        JSHClass::RestoreElementsKindToGeneric(*newHclass);
        obj->SynchronizedSetClass(thread, *newHclass);
        TryMigrateToGenericKindForJSObject(thread, obj, oldKind);
    }

    return true;
}

// 9.1.12 [[OwnPropertyKeys]] ( )
JSHandle<TaggedArray> JSObject::GetOwnPropertyKeys(JSThread *thread, const JSHandle<JSObject> &obj)
{
    uint32_t numOfElements = obj->GetNumberOfElements(thread);
    uint32_t keyLen = numOfElements + obj->GetNumberOfKeys();

    JSHandle<TaggedArray> keyArray = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(keyLen);

    if (numOfElements > 0) {
        GetAllElementKeys(thread, obj, 0, keyArray);
    }
    GetAllKeys(thread, obj, static_cast<int32_t>(numOfElements), keyArray);
    return keyArray;
}

JSHandle<TaggedArray> JSObject::GetAllPropertyKeys(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t filter)
{
    JSMutableHandle<JSObject> currentObj(thread, obj);
    JSMutableHandle<JSTaggedValue> currentObjValue(thread, currentObj);

    uint32_t curObjNumberOfElements = currentObj->GetNumberOfElements(thread);
    uint32_t curObjNumberOfKeys = currentObj->GetNumberOfKeys();
    uint32_t curObjectKeysLength = curObjNumberOfElements + curObjNumberOfKeys;
    uint32_t retArrayLength = curObjectKeysLength;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedArray> retArray(thread, factory->NewTaggedArray(retArrayLength));
    uint32_t retArrayEffectivelength = 0;

    do {
        curObjNumberOfElements = currentObj->GetNumberOfElements(thread);
        curObjNumberOfKeys = currentObj->GetNumberOfKeys();
        curObjectKeysLength = curObjNumberOfElements + curObjNumberOfKeys;
        uint32_t minRequireLength = curObjectKeysLength + retArrayEffectivelength;
        if (retArrayLength < minRequireLength) {
            // expand retArray
            if (retArrayLength != 0) {
                retArray.Update(factory->NewAndCopyTaggedArray(retArray, minRequireLength, retArrayLength));
            } else {
                retArray.Update(factory->NewTaggedArray(minRequireLength));
            }
            retArrayLength = minRequireLength;
        }

        GetAllElementKeysByFilter(thread, currentObj, retArray, retArrayEffectivelength, filter);

        GetAllKeysByFilter(thread, currentObj, retArrayEffectivelength, retArray, filter);
        bool isInculdePrototypes = (filter & NATIVE_KEY_INCLUDE_PROTOTYPES);
        if (!isInculdePrototypes) {
            break;
        }
        currentObj.Update(GetPrototype(currentObj));
        currentObjValue.Update(currentObj);
    } while (currentObjValue->IsHeapObject());

    if (retArrayEffectivelength == 0 && (filter & NATIVE_KEY_OWN_ONLY)) {
        return retArray;
    }
    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());
    if (filter & NATIVE_KEY_NUMBERS_TO_STRINGS) {
        for (uint32_t i = 0; i < retArrayEffectivelength; i++) {
            element.Update(retArray->Get(i));
            if (element->IsNumber()) {
                retArray->Set(thread, i, base::NumberHelper::NumberToString(thread,
                    JSTaggedValue(element->GetNumber())));
            }
        }
    }
    uint32_t elementIndex = 0;
    if (filter & NATIVE_KEY_SKIP_STRINGS) {
        while ((retArrayEffectivelength > 0) && (elementIndex < retArrayEffectivelength)) {
            if (retArray->Get(elementIndex).IsString()) {
                TaggedArray::RemoveElementByIndex(thread, retArray, elementIndex, retArrayEffectivelength);
                retArrayEffectivelength--;
            } else {
                elementIndex++;
            }
        }
    }
    if (retArray->GetLength() > retArrayEffectivelength) {
        retArray->Trim(thread, retArrayEffectivelength);
    }
    return retArray;
}

void JSObject::CollectEnumKeysAlongProtoChain(JSThread *thread, const JSHandle<JSObject> &obj,
                                              JSHandle<TaggedArray> keyArray, uint32_t *keys,
                                              JSHandle<TaggedQueue> shadowQueue, int32_t lastLength)
{
    ASSERT(!obj->IsJSGlobalObject());

    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (!array->IsDictionaryMode()) {
        JSHClass *jsHclass = obj->GetJSHClass();
        int end = static_cast<int>(jsHclass->NumberOfProps());
        if (end > 0) {
            LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject())
                ->GetAllEnumKeys(thread, end, *keys, keyArray, keys, shadowQueue, lastLength);
        }
        return;
    }
    NameDictionary *dict = NameDictionary::Cast(obj->GetProperties().GetTaggedObject());
    dict->GetAllEnumKeys(thread, *keys, keyArray, keys, shadowQueue, lastLength);
}

void JSObject::AppendOwnEnumPropertyKeys(JSThread *thread, const JSHandle<JSObject> &obj,
                                         JSHandle<TaggedArray> keyArray, uint32_t *keys,
                                         JSHandle<TaggedQueue> shadowQueue)
{
    int32_t lastLength = *keys;
    uint32_t numOfElements = obj->GetNumberOfElements(thread);
    if (numOfElements > 0) {
        CollectEnumElementsAlongProtoChain(thread, obj, *keys, keyArray, keys, lastLength);
    }
    CollectEnumKeysAlongProtoChain(thread, obj, keyArray, keys, shadowQueue, lastLength);
}

JSHandle<TaggedArray> JSObject::GetOwnEnumPropertyKeys(JSThread *thread, const JSHandle<JSObject> &obj)
{
    uint32_t numOfElements = obj->GetNumberOfElements(thread);
    uint32_t keyLen = numOfElements + obj->GetNumberOfKeys();

    JSHandle<TaggedArray> keyArray = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(keyLen);

    if (numOfElements > 0) {
        GetEnumElementKeys(thread, obj, 0, keyArray);
    }
    GetAllEnumKeys(thread, obj, static_cast<int32_t>(numOfElements), keyArray);
    return keyArray;
}

JSHandle<JSObject> JSObject::ObjectCreate(JSThread *thread, const JSHandle<JSObject> &proto)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> constructor(env->GetObjectFunction());
    JSHandle<JSObject> objHandle = thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(constructor);
    SetPrototype(thread, objHandle, JSHandle<JSTaggedValue>(proto));
    return objHandle;
}

// 7.3.4 CreateDataProperty (O, P, V)
bool JSObject::CreateDataProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                  const JSHandle<JSTaggedValue> &value, SCheckMode sCheckMode)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    if (!JSHandle<JSTaggedValue>::Cast(obj)->IsJSShared()) {
        sCheckMode = SCheckMode::CHECK;
    }
    auto result = ObjectFastOperator::SetPropertyByValue<ObjectFastOperator::Status::DefineSemantics>(
        thread, obj.GetTaggedValue(), key.GetTaggedValue(), value.GetTaggedValue(), sCheckMode);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (!result.IsHole()) {
        return !result.IsException();
    }
    PropertyDescriptor desc(thread, value, true, true, true);
    return JSTaggedValue::DefineOwnProperty(thread, JSHandle<JSTaggedValue>::Cast(obj), key, desc, sCheckMode);
}

bool JSObject::CreateDataProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                  const JSHandle<JSTaggedValue> &value, SCheckMode sCheckMode)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    auto result = ObjectFastOperator::SetPropertyByIndex<ObjectFastOperator::Status::DefineSemantics>
            (thread, obj.GetTaggedValue(), index, value.GetTaggedValue());
    if (!result.IsHole()) {
        return !result.IsException();
    }
    PropertyDescriptor desc(thread, value, true, true, true);
    return DefineOwnProperty(thread, obj, index, desc, sCheckMode);
}

// 7.3.5 CreateMethodProperty (O, P, V)
bool JSObject::CreateDataPropertyOrThrow(JSThread *thread, const JSHandle<JSObject> &obj,
                                         const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                         SCheckMode sCheckMode)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    bool success = CreateDataProperty(thread, obj, key, value, sCheckMode);
    if (!success) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "failed to create data property", success);
    }
    return success;
}

bool JSObject::CreateDataPropertyOrThrow(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                         const JSHandle<JSTaggedValue> &value,  SCheckMode sCheckMode)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");

    bool success = CreateDataProperty(thread, obj, index, value, sCheckMode);
    if (!success) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "failed to create data property", success);
    }
    return success;
}
// 7.3.6 CreateDataPropertyOrThrow (O, P, V)
bool JSObject::CreateMethodProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                    const JSHandle<JSTaggedValue> &value)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    PropertyDescriptor desc(thread, value, true, false, true);
    return DefineOwnProperty(thread, obj, key, desc);
}

JSHandle<JSTaggedValue> JSObject::CallFunction(JSThread *thread, const JSHandle<JSTaggedValue> &func)
{
    if (func->IsUndefined() || func->IsNull()) {
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined());
    }
    if (!func->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not Callable", func);
    }
    return func;
}

// 7.3.9 GetMethod (O, P)
JSHandle<JSTaggedValue> JSObject::GetMethod(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                            const JSHandle<JSTaggedValue> &key)
{
    JSHandle<JSTaggedValue> func = JSTaggedValue::GetProperty(thread, obj, key).GetValue();
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    func = CallFunction(thread, func);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return func;
}

JSHandle<JSTaggedValue> JSObject::FastGetMethod(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &key)
{
    JSHandle<JSTaggedValue> func(thread, ObjectFastOperator::FastGetPropertyByName(thread, obj.GetTaggedValue(),
                                                                                   key.GetTaggedValue()));
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    func = CallFunction(thread, func);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return func;
}

// 7.3.14 SetIntegrityLevel (O, level)
bool JSObject::SetIntegrityLevel(JSThread *thread, const JSHandle<JSObject> &obj, IntegrityLevel level)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT((level == IntegrityLevel::SEALED || level == IntegrityLevel::FROZEN),
                 "level is not a valid IntegrityLevel");

    bool status = JSTaggedValue::PreventExtensions(thread, JSHandle<JSTaggedValue>(obj));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (!status) {
        return false;
    }

    JSHandle<TaggedArray> jshandleKeys =
        JSTaggedValue::GetOwnPropertyKeys(thread, JSHandle<JSTaggedValue>(obj));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    PropertyDescriptor descNoConf(thread);
    descNoConf.SetConfigurable(false);
    PropertyDescriptor descNoConfWrite(thread);
    descNoConfWrite.SetWritable(false);
    descNoConfWrite.SetConfigurable(false);

    if (level == IntegrityLevel::SEALED) {
        uint32_t length = jshandleKeys->GetLength();
        if (length == 0) {
            return true;
        }
        auto key = jshandleKeys->Get(0);
        JSMutableHandle<JSTaggedValue> handleKey(thread, key);
        for (uint32_t i = 0; i < length; i++) {
            auto taggedKey = JSTaggedValue(jshandleKeys->Get(i));
            handleKey.Update(taggedKey);
            [[maybe_unused]] bool success =
                JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(obj), handleKey, descNoConf);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        }
    } else {
        uint32_t length = jshandleKeys->GetLength();
        if (length == 0) {
            return true;
        }
        auto key = jshandleKeys->Get(0);
        JSMutableHandle<JSTaggedValue> handleKey(thread, key);
        for (uint32_t i = 0; i < length; i++) {
            auto taggedKey = JSTaggedValue(jshandleKeys->Get(i));
            handleKey.Update(taggedKey);
            PropertyDescriptor currentDesc(thread);
            bool curDescStatus =
                JSTaggedValue::GetOwnProperty(thread, JSHandle<JSTaggedValue>(obj), handleKey, currentDesc);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            if (curDescStatus) {
                PropertyDescriptor desc = currentDesc.IsAccessorDescriptor() ? descNoConf : descNoConfWrite;
                [[maybe_unused]] bool success =
                    JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(obj), handleKey, desc);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            }
        }
    }
    return true;
}

bool JSObject::FreezeSharedObject(JSThread *thread, const JSHandle<JSObject> &obj)
{
    ASSERT_PRINT(JSHandle<JSTaggedValue>(obj)->IsJSSharedObject() ||
                 JSHandle<JSTaggedValue>(obj)->IsJSSharedFunction() ||
                 JSHandle<JSTaggedValue>(obj)->IsJSSharedAsyncFunction(),
                 "Obj is not a valid shared object");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // It is not extensible for shared object.
    if (obj->IsExtensible()) {
        return false;
    }
    JSHandle<JSHClass> hclass(thread, obj->GetClass());
    auto newClass = JSHClass::Clone(thread, hclass);
    if (!hclass->IsDictionaryMode()) {
        uint32_t propNumber = hclass->NumberOfProps();
        JSHandle<LayoutInfo> layoutInfo(thread, hclass->GetLayout());
        JSHandle<LayoutInfo> newLayoutInfo = factory->CreateSLayoutInfo(propNumber);
        for (uint32_t i = 0; i < propNumber; i++) {
            JSTaggedValue key = layoutInfo->GetKey(i);
            PropertyAttributes attr = layoutInfo->GetAttr(i);
            attr.SetWritable(false);
            newLayoutInfo->AddKey(thread, i, key, attr);
        }
        newClass->SetLayout(thread, newLayoutInfo);
        obj->SynchronizedSetClass(thread, *newClass);
    } else {
        auto dict = NameDictionary::Cast(obj->GetProperties().GetTaggedObject());
        dict->UpdateAllAttributesToNoWitable(thread);
    }
    return true;
}

// 7.3.15 TestIntegrityLevel (O, level)
bool JSObject::TestIntegrityLevel(JSThread *thread, const JSHandle<JSObject> &obj, IntegrityLevel level)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT((level == IntegrityLevel::SEALED || level == IntegrityLevel::FROZEN),
                 "level is not a valid IntegrityLevel");

    bool status = JSHandle<JSTaggedValue>(obj)->IsExtensible(thread);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (status) {
        return false;
    }

    JSHandle<TaggedArray> jshandleKeys =
        JSTaggedValue::GetOwnPropertyKeys(thread, JSHandle<JSTaggedValue>(obj));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    uint32_t length = jshandleKeys->GetLength();
    if (length == 0) {
        return true;
    }
    auto key = jshandleKeys->Get(0);
    JSMutableHandle<JSTaggedValue> handleKey(thread, key);
    for (uint32_t i = 0; i < length; i++) {
        auto taggedKey = JSTaggedValue(jshandleKeys->Get(i));
        handleKey.Update(taggedKey);
        PropertyDescriptor currentDesc(thread);
        bool curDescStatus =
            JSTaggedValue::GetOwnProperty(thread, JSHandle<JSTaggedValue>(obj), handleKey, currentDesc);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        if (curDescStatus) {
            if (currentDesc.IsConfigurable()) {
                return false;
            }
            if (level == IntegrityLevel::FROZEN &&
                currentDesc.IsDataDescriptor() && currentDesc.IsWritable()) {
                return false;
            }
        }
    }
    return true;
}

// 7.3.21 EnumerableOwnNames (O)
JSHandle<TaggedArray> JSObject::EnumerableOwnNames(JSThread *thread, const JSHandle<JSObject> &obj)
{
    ASSERT_PRINT(obj->IsECMAObject(), "obj is not object");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> tagObj(obj);
    // fast mode
    if (tagObj->IsJSObject() && !tagObj->IsTypedArray() && !tagObj->IsModuleNamespace()) {
        uint32_t copyLengthOfKeys = 0;
        uint32_t copyLengthOfElements = 0;
        auto keyElementPair = GetOwnEnumerableNamesInFastMode(thread, obj, &copyLengthOfKeys, &copyLengthOfElements);
        JSHandle<TaggedArray> keyArray = keyElementPair.first;
        JSHandle<TaggedArray> elementArray = keyElementPair.second;
        JSHandle<TaggedArray> keys;
        if (copyLengthOfKeys != 0 && copyLengthOfElements != 0) {
            keys = TaggedArray::AppendSkipHole(thread, elementArray, keyArray, copyLengthOfKeys + copyLengthOfElements);
        } else if (copyLengthOfKeys != 0) {
            if (copyLengthOfKeys < keyArray->GetLength()) {
                // keyArray will skip nonEnumerable properties, need re-set length.
                keyArray->Trim(thread, copyLengthOfKeys);
            }
            return keyArray;
        } else if (copyLengthOfElements != 0) {
            if (copyLengthOfElements < elementArray->GetLength()) {
                // elementArray will skip hole value, need re-set length.
                elementArray->Trim(thread, copyLengthOfElements);
            }
            return elementArray;
        } else {
            keys = factory->EmptyArray();
        }
        return keys;
    }

    uint32_t copyLength = 0;
    JSHandle<TaggedArray> keys = JSTaggedValue::GetOwnPropertyKeys(thread, tagObj);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
    uint32_t length = keys->GetLength();

    JSHandle<TaggedArray> names = factory->NewTaggedArray(length);
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < length; i++) {
        keyHandle.Update(keys->Get(i));
        if (keyHandle->IsString()) {
            PropertyDescriptor desc(thread);
            bool status = JSTaggedValue::GetOwnProperty(thread, JSHandle<JSTaggedValue>(obj),
                                                        keyHandle, desc);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);

            if (status && desc.IsEnumerable()) {
                names->Set(thread, copyLength, keyHandle);
                copyLength++;
            }
        }
    }

    return factory->CopyArray(names, length, copyLength);
}

void JSObject::EnumerableOwnPropertyNamesHelper(JSThread *thread, const JSHandle<JSObject> &obj,
    const JSHandle<TaggedArray> &arr, JSHandle<TaggedArray> &prop, uint32_t &index, bool &fastMode, PropertyKind kind)
{
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<JSHClass> objClass(thread, obj->GetJSHClass());
    uint32_t length = arr->GetLength();
    for (uint32_t i = 0; i < length; i++) {
        key.Update(arr->Get(thread, i));
        if (!JSTaggedValue::IsPropertyKey(key)) {
            break;
        }
        JSTaggedValue value = JSTaggedValue::Hole();
        if (fastMode) {
            value = ObjectFastOperator::GetPropertyByValue<ObjectFastOperator::Status::UseOwn>
                    (thread, obj.GetTaggedValue(), key.GetTaggedValue());
            RETURN_IF_ABRUPT_COMPLETION(thread);
        }
        if (value.IsHole()) {
            PropertyDescriptor desc(thread);
            bool status = JSTaggedValue::GetOwnProperty(thread, JSHandle<JSTaggedValue>(obj), key, desc);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            if (!status || !desc.IsEnumerable()) {
                continue;
            }
            if (desc.HasValue()) {
                value = desc.GetValue().GetTaggedValue();
            } else {
                OperationResult opResult = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj), key);
                RETURN_IF_ABRUPT_COMPLETION(thread);
                value = opResult.GetValue().GetTaggedValue();
            }
        }
        index = SetValuesOrEntries(thread, prop, index, key, JSHandle<JSTaggedValue>(thread, value), kind);
        fastMode = fastMode ? CheckHClassHit(obj, objClass) : fastMode;
    }
}

JSHandle<TaggedArray> JSObject::EnumerableOwnPropertyNames(JSThread *thread, const JSHandle<JSObject> &obj,
                                                           PropertyKind kind)
{
    // 1. Assert: Type(O) is Object.
    ASSERT_PRINT(obj->IsECMAObject(), "obj is not object");

    // 2. Let ownKeys be ? O.[[OwnPropertyKeys]]().
    JSHandle<JSTaggedValue> tagObj(obj);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (tagObj->IsJSObject() && !tagObj->IsJSProxy() && !tagObj->IsTypedArray() && !tagObj->IsModuleNamespace()) {
        uint32_t copyLengthOfKeys = 0;
        uint32_t copyLengthOfElements = 0;
        uint32_t index = 0;
        bool fastMode = true;
        auto keyElementPair = GetOwnEnumerableNamesInFastMode(thread, obj, &copyLengthOfKeys, &copyLengthOfElements);
        JSHandle<TaggedArray> keyArray = keyElementPair.first;
        JSHandle<TaggedArray> elementArray = keyElementPair.second;
        JSHandle<TaggedArray> properties = factory->NewTaggedArray(copyLengthOfKeys + copyLengthOfElements);
        if (copyLengthOfElements != 0) {
            EnumerableOwnPropertyNamesHelper(thread, obj, elementArray, properties, index, fastMode, kind);
        }
        if (copyLengthOfKeys != 0) {
            EnumerableOwnPropertyNamesHelper(thread, obj, keyArray, properties, index, fastMode, kind);
        }
        if (UNLIKELY(!fastMode && index < copyLengthOfKeys + copyLengthOfElements)) {
            properties->Trim(thread, index);
        }
        return properties;
    }

    JSHandle<TaggedArray> ownKeys = JSTaggedValue::GetOwnPropertyKeys(thread, tagObj);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);

    // 3. Let properties be a new empty List.
    uint32_t length = ownKeys->GetLength();
    JSHandle<TaggedArray> properties = factory->NewTaggedArray(length);

    // 4. For each element key of ownKeys, do
    // a. If Type(key) is String, then
    //     i. Let desc be ? O.[[GetOwnProperty]](key).
    //     ii. If desc is not undefined and desc.[[Enumerable]] is true, then
    //         1. If kind is key, append key to properties.
    //         2. Else,
    //            a. Let value be ? Get(O, key).
    //            b. If kind is value, append value to properties.
    //            c. Else,
    //               i. Assert: kind is key+value.
    //               ii. Let entry be ! CreateArrayFromList(« key, value »).
    //               iii. Append entry to properties.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    uint32_t index = 0;
    for (uint32_t i = 0; i < length; i++) {
        key.Update(ownKeys->Get(thread, i));
        if (key->IsString()) {
            PropertyDescriptor desc(thread);
            bool status = JSTaggedValue::GetOwnProperty(thread, JSHandle<JSTaggedValue>(obj),
                                                        key, desc);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
            if (status && desc.IsEnumerable()) {
                if (kind == PropertyKind::KEY) {
                    properties->Set(thread, index++, key);
                } else {
                    OperationResult result =
                        JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj), key);
                    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
                    JSHandle<JSTaggedValue> value = result.GetValue();
                    index = SetValuesOrEntries(thread, properties, index, key, value, kind);
                }
            }
        }
    }

    if (UNLIKELY(index < length)) {
        properties->Trim(thread, index);
    }
    // 5. Return properties.
    return properties;
}

JSHandle<GlobalEnv> JSObject::GetFunctionRealm(JSThread *thread, const JSHandle<JSTaggedValue> &object)
{
    // 1. Assert: obj is a callable object.
    ASSERT(object->IsCallable());
    // 2. If obj has a [[Realm]] internal slot, then return obj’s [[Realm]] internal slot.
    // 3. If obj is a Bound Function exotic object, then
    if (object->IsBoundFunction()) {
        // a. Let target be obj’s [[BoundTargetFunction]] internal slot.
        JSHandle<JSTaggedValue> target(thread, JSHandle<JSBoundFunction>(object)->GetBoundTarget());
        // b. Return GetFunctionRealm(target).
        return GetFunctionRealm(thread, target);
    }
    // 4. If obj is a Proxy exotic object, then
    if (object->IsJSProxy()) {
        // a. If the value of the [[ProxyHandler]] internal slot of obj is null, throw a TypeError exception.
        if (JSHandle<JSProxy>(object)->GetHandler().IsNull()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSObject::GetFunctionRealm: handler is null",
                                        JSHandle<GlobalEnv>(thread, JSTaggedValue::Exception()));
        }
        // b. Let proxyTarget be the value of obj’s [[ProxyTarget]] internal slot.
        JSHandle<JSTaggedValue> proxyTarget(thread, JSHandle<JSProxy>(object)->GetTarget());
        return GetFunctionRealm(thread, proxyTarget);
    }

    if (object->IsJSShared()) {
        // LexicalEnv in sharedConstructor is constructor itself. And Shared Constructors shares the same GlobalEnv.
        return thread->GetEcmaVM()->GetGlobalEnv();
    }

    JSTaggedValue maybeGlobalEnv = JSHandle<JSFunction>(object)->GetLexicalEnv();
    while (!maybeGlobalEnv.IsJSGlobalEnv()) {
        if (maybeGlobalEnv.IsUndefined()) {
            return thread->GetEcmaVM()->GetGlobalEnv();
        }
        maybeGlobalEnv = LexicalEnv::Cast(maybeGlobalEnv.GetTaggedObject())->GetParentEnv();
    }
    return JSHandle<GlobalEnv>(thread, maybeGlobalEnv);
}

bool JSObject::InstanceOf(JSThread *thread, const JSHandle<JSTaggedValue> &object,
                          const JSHandle<JSTaggedValue> &target)
{
    // 1. If Type(target) is not Object, throw a TypeError exception.
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "InstanceOf error when type of target is not Object", false);
    }

    EcmaVM *vm = thread->GetEcmaVM();
    // 2. Let instOfHandler be GetMethod(target, @@hasInstance).
    JSHandle<JSTaggedValue> instOfHandler = FastGetMethod(thread, target, vm->GetGlobalEnv()->GetHasInstanceSymbol());

    // 3. ReturnIfAbrupt(instOfHandler).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 4. If instOfHandler is not undefined, then
    if (!instOfHandler->IsUndefined()) {
        // a. Return ! ToBoolean(? Call(instOfHandler, target, «object»)).
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, instOfHandler, target, undefined, 1);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        info->SetCallArg(object.GetTaggedValue());
        JSTaggedValue tagged = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return tagged.ToBoolean();
    }

    // 5. If IsCallable(target) is false, throw a TypeError exception.
    if (!target->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "InstanceOf error when target is not Callable", false);
    }

    // 6. Return ? OrdinaryHasInstance(target, object).
    return JSFunction::OrdinaryHasInstance(thread, target, object);
}

// ecma6.0 6.2.4.4
JSHandle<JSTaggedValue> JSObject::FromPropertyDescriptor(JSThread *thread, const PropertyDescriptor &desc)
{
    // 1. If Desc is undefined, return undefined
    if (desc.IsEmpty()) {
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined());
    }

    // 2. Let obj be ObjectCreate(%ObjectPrototype%).
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> objFunc(env->GetObjectFunction());
    JSHandle<JSObject> objHandle = thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(objFunc);

    auto globalConst = thread->GlobalConstants();
    // 4. If Desc has a [[Value]] field, then Perform CreateDataProperty(obj, "value", Desc.[[Value]]).
    if (desc.HasValue()) {
        JSHandle<JSTaggedValue> valueStr = globalConst->GetHandledValueString();
        bool success = CreateDataProperty(thread, objHandle, valueStr, desc.GetValue());
        RASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    // 5. If Desc has a [[Writable]] field, then Perform CreateDataProperty(obj, "writable", Desc.[[Writable]]).
    if (desc.HasWritable()) {
        JSHandle<JSTaggedValue> writableStr = globalConst->GetHandledWritableString();
        JSHandle<JSTaggedValue> writable(thread, JSTaggedValue(desc.IsWritable()));
        [[maybe_unused]] bool success = CreateDataProperty(thread, objHandle, writableStr, writable);
        ASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    // 6. If Desc has a [[Get]] field, then Perform CreateDataProperty(obj, "get", Desc.[[Get]]).
    if (desc.HasGetter()) {
        JSHandle<JSTaggedValue> getStr = globalConst->GetHandledGetString();
        bool success = CreateDataProperty(thread, objHandle, getStr, desc.GetGetter());
        RASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    // 7. If Desc has a [[Set]] field, then Perform CreateDataProperty(obj, "set", Desc.[[Set]])
    if (desc.HasSetter()) {
        JSHandle<JSTaggedValue> setStr = globalConst->GetHandledSetString();
        bool success = CreateDataProperty(thread, objHandle, setStr, desc.GetSetter());
        RASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    // 8. If Desc has an [[Enumerable]] field, then Perform CreateDataProperty(obj, "enumerable",
    // Desc.[[Enumerable]]).
    if (desc.HasEnumerable()) {
        JSHandle<JSTaggedValue> enumerableStr = globalConst->GetHandledEnumerableString();
        JSHandle<JSTaggedValue> enumerable(thread, JSTaggedValue(desc.IsEnumerable()));
        [[maybe_unused]] bool success = CreateDataProperty(thread, objHandle, enumerableStr, enumerable);
        ASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    // 9. If Desc has a [[Configurable]] field, then Perform CreateDataProperty(obj , "configurable",
    // Desc.[[Configurable]]).
    if (desc.HasConfigurable()) {
        JSHandle<JSTaggedValue> configurableStr = globalConst->GetHandledConfigurableString();
        JSHandle<JSTaggedValue> configurable(thread, JSTaggedValue(desc.IsConfigurable()));
        [[maybe_unused]] bool success = CreateDataProperty(thread, objHandle, configurableStr, configurable);
        ASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    return JSHandle<JSTaggedValue>(objHandle);
}

bool JSObject::ToPropertyDescriptorFast(JSThread *thread, const JSHandle<JSTaggedValue> &obj, PropertyDescriptor &desc)
{
    auto *hclass = obj->GetTaggedObject()->GetClass();
    JSType jsType = hclass->GetObjectType();
    if (jsType != JSType::JS_OBJECT) {
        return false;
    }
    if (hclass->IsDictionaryMode()) {
        return false;
    }
    auto env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    if (hclass->GetPrototype() != env->GetObjectFunctionPrototype().GetTaggedValue()) {
        return false;
    }
    if (JSObject::Cast(hclass->GetPrototype().GetTaggedObject())->GetClass() !=
        env->GetObjectFunctionPrototypeClass().GetObject<JSHClass>()) {
        return false;
    }
    LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    uint32_t propsNumber = hclass->NumberOfProps();
    for (uint32_t i = 0; i < propsNumber; i++) {
        auto attr = layoutInfo->GetAttr(i);
        if (attr.IsAccessor()) {
            return false;
        }
        auto key = layoutInfo->GetKey(i);
        auto value = JSObject::Cast(obj->GetTaggedObject())->GetProperty(hclass, attr);
        if (key == globalConst->GetEnumerableString()) {
            bool enumerable = value.ToBoolean();
            desc.SetEnumerable(enumerable);
        } else if (key == globalConst->GetConfigurableString()) {
            bool configurable = value.ToBoolean();
            desc.SetConfigurable(configurable);
        } else if (key == globalConst->GetValueString()) {
            auto handleValue = JSHandle<JSTaggedValue>(thread, value);
            desc.SetValue(handleValue);
        } else if (key == globalConst->GetWritableString()) {
            bool writable = value.ToBoolean();
            desc.SetWritable(writable);
        } else if (key == globalConst->GetGetString()) {
            if (!value.IsCallable()) {
                return false;
            }
            auto getter = JSHandle<JSTaggedValue>(thread, value);
            desc.SetGetter(getter);
        } else if (key == globalConst->GetSetString()) {
            if (!value.IsCallable()) {
                return false;
            }
            auto setter = JSHandle<JSTaggedValue>(thread, value);
            desc.SetSetter(setter);
        }
    }

    if (desc.IsAccessorDescriptor()) {
        // 22a. If either desc.[[Value]] or desc.[[Writable]] is present, throw a TypeError exception.
        if (desc.HasValue() || desc.HasWritable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "either Value or Writable is present", true);
        }
    }
    return true;
}

// ecma6.0 6.2.4.5 ToPropertyDescriptor ( Obj )
void JSObject::ToPropertyDescriptor(JSThread *thread, const JSHandle<JSTaggedValue> &obj, PropertyDescriptor &desc)
{
    if (!obj->IsECMAObject()) {
        // 2. If Type(Obj) is not Object, throw a TypeError exception.
        THROW_TYPE_ERROR(thread, "ToPropertyDescriptor error obj is not Object");
    }

    if (ToPropertyDescriptorFast(thread, obj, desc)) {
        return;
    }
    auto globalConst = thread->GlobalConstants();
    // 3. Let desc be a new Property Descriptor that initially has no fields.
    // 4. Let hasEnumerable be HasProperty(Obj, "enumerable")
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), globalConst->GetEnumerableString());
        if (op.IsFound()) {
            auto value = op.FastGetValue();
            bool enumerable = value->IsException() ? false : value->ToBoolean();
            desc.SetEnumerable(enumerable);
        }
    }
    // 7. Let hasConfigurable be HasProperty(Obj, "configurable").
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), globalConst->GetConfigurableString());
        if (op.IsFound()) {
            auto value = op.FastGetValue();
            bool conf = value->IsException() ? false : value->ToBoolean();
            desc.SetConfigurable(conf);
        }
    }
    // 10. Let hasValue be HasProperty(Obj, "value").
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), globalConst->GetValueString());
        if (op.IsFound()) {
            JSHandle<JSTaggedValue> prop = op.FastGetValue();
            desc.SetValue(prop);
        }
    }
    // 13. Let hasWritable be HasProperty(Obj, "writable").
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), globalConst->GetWritableString());
        if (op.IsFound()) {
            auto value = op.FastGetValue();
            bool writable = value->IsException() ? false : value->ToBoolean();
            desc.SetWritable(writable);
        }
    }
    // 16. Let hasGet be HasProperty(Obj, "get").
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), globalConst->GetGetString());
        if (op.IsFound()) {
            JSHandle<JSTaggedValue> getter = op.FastGetValue();
            if (!getter->IsCallable() && !getter->IsUndefined()) {
                THROW_TYPE_ERROR(thread, "getter not callable or undefined");
            }
            desc.SetGetter(getter);
        }
    }

    // 19. Let hasSet be HasProperty(Obj, "set").
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), globalConst->GetSetString());
        if (op.IsFound()) {
            JSHandle<JSTaggedValue> setter = op.FastGetValue();
            if (!setter->IsCallable() && !setter->IsUndefined()) {
                THROW_TYPE_ERROR(thread, "setter not callable or undefined");
            }
            desc.SetSetter(setter);
        }
    }

    // 22. If either desc.[[Get]] or desc.[[Set]] is present, then
    if (desc.IsAccessorDescriptor()) {
        // 22a. If either desc.[[Value]] or desc.[[Writable]] is present, throw a TypeError exception.
        if (desc.HasValue() || desc.HasWritable()) {
            THROW_TYPE_ERROR(thread, "either desc.[[Value]] or desc.[[Writable]] is present");
        }
    }
    // 23. Return desc.
}

const CString JSObject::ExtractConstructorAndRecordName(JSThread *thread, TaggedObject *obj, bool noAllocate,
                                                        bool *isCallGetter)
{
    CString result = "";
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> contructorKey = globalConst->GetHandledConstructorString();
    JSTaggedValue objConstructor = ObjectFastOperator::GetPropertyByName(thread, JSTaggedValue(obj),
                                                                         contructorKey.GetTaggedValue(), noAllocate,
                                                                         isCallGetter);
    if (*isCallGetter) {
        return "JSObject";
    }

    if (!objConstructor.IsJSFunction()) {
        return "JSObject";
    }

    JSFunctionBase *func = JSFunctionBase::Cast(objConstructor.GetTaggedObject());
    Method *method = Method::Cast(func->GetMethod().GetTaggedObject());
    MethodLiteral *methodLiteral = method->GetMethodLiteral();
    if (methodLiteral == nullptr) {
        return "JSObject";
    }
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile();
    panda_file::File::EntityId methodId = methodLiteral->GetMethodId();
    const CString &nameStr = MethodLiteral::ParseFunctionNameToCString(jsPandaFile, methodId);
    const CString &moduleStr = method->GetRecordNameStr();

    if (!moduleStr.empty()) {
        result.append(moduleStr).append(" ");
    }
    if (nameStr.empty()) {
        result.append("JSObject");
    } else {
        result.append(nameStr);
    }
    DebugInfoExtractor *debugExtractor =
        JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
    if (debugExtractor == nullptr) {
        return result;
    }
    int32_t line = debugExtractor->GetFristLine(methodId);
    std::string fileName = debugExtractor->GetSourceFile(methodId);
    return result.append("(").append(fileName).append(std::to_string(line)).append(")");
}

JSHandle<JSTaggedValue> JSObject::SpeciesConstructor(JSThread *thread, const JSHandle<JSObject> &obj,
                                                     const JSHandle<JSTaggedValue> &defaultConstructor)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // Assert: Type(O) is Object.
    ASSERT_PRINT(obj->IsECMAObject(), "obj must be js object");

    // Let C be Get(O, "constructor").
    JSHandle<JSTaggedValue> contructorKey = globalConst->GetHandledConstructorString();
    JSHandle<JSTaggedValue> objConstructor(JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(obj),
        contructorKey).GetValue());
    // ReturnIfAbrupt(C).
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return SlowSpeciesConstructor(thread, objConstructor, defaultConstructor);
}

JSHandle<JSTaggedValue> JSObject::SlowSpeciesConstructor(JSThread *thread,
                                                         const JSHandle<JSTaggedValue> &objConstructor,
                                                         const JSHandle<JSTaggedValue> &defaultConstructor)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    if (objConstructor->IsUndefined()) {
        return defaultConstructor;
    }
    // If Type(C) is not Object, throw a TypeError exception.
    if (!objConstructor->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is not Object",
                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }
    // Let S be Get(C, @@species).
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesConstructor = GetProperty(thread, objConstructor, speciesSymbol).GetValue();
    // ReturnIfAbrupt(S).
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // If S is either undefined or null, return defaultConstructor.
    if (speciesConstructor->IsUndefined() || speciesConstructor->IsNull()) {
        return defaultConstructor;
    }
    // If IsConstructor(S) is true, return S.
    if (speciesConstructor->IsConstructor()) {
        return speciesConstructor;
    }
    // Throw a TypeError exception.
    THROW_TYPE_ERROR_AND_RETURN(thread, "Is not Constructor",
                                JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception());
}

// 6.2.4.6 CompletePropertyDescriptor ( Desc )
void PropertyDescriptor::CompletePropertyDescriptor(const JSThread *thread, PropertyDescriptor &desc)
{
    // 1. ReturnIfAbrupt(Desc).
    // 2. Assert: Desc is a Property Descriptor
    // 3. Let like be Record{[[Value]]: undefined, [[Writable]]: false, [[Get]]: undefined, [[Set]]: undefined,
    // [[Enumerable]]: false, [[Configurable]]: false}.
    // 4. If either IsGenericDescriptor(Desc) or IsDataDescriptor(Desc) is true, then
    if (!desc.IsAccessorDescriptor()) {
        // a. If Desc does not have a [[Value]] field, set Desc.[[Value]] to like.[[Value]].
        // b. If Desc does not have a [[Writable]] field, set Desc.[[Writable]] to like.[[Writable]].
        if (!desc.HasValue()) {
            desc.SetValue(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
        }
        if (!desc.HasWritable()) {
            desc.SetWritable(false);
        }
    } else {
        // a. If Desc does not have a [[Get]] field, set Desc.[[Get]] to like.[[Get]].
        // b. If Desc does not have a [[Set]] field, set Desc.[[Set]] to like.[[Set]].
        // Default value of Get and Set is undefined.
    }
    // 6. If Desc does not have an [[Enumerable]] field, set Desc.[[Enumerable]] to like.[[Enumerable]].
    // 7. If Desc does not have a [[Configurable]] field, set Desc.[[Configurable]] to like.[[Configurable]].
    if (!desc.HasEnumerable()) {
        desc.SetEnumerable(false);
    }
    if (!desc.HasConfigurable()) {
        desc.SetConfigurable(false);
    }
}

// static
// When receiver has no elements and there is no enum cache and elements on receiver's prototype chain,
// the enum cache is a simple enum cache.
// When receiver and receiver's prototype chain have no elements, and the prototype is not modified,
// the enum cache is a enum cache with protochain
bool JSObject::IsSimpleEnumCacheValid(JSThread *thread, JSTaggedValue receiver)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t numOfElements = JSObject::Cast(receiver.GetTaggedObject())->GetNumberOfElements(thread);
    if (numOfElements > 0) {
        return false;
    }

    JSTaggedValue current = JSObject::GetPrototype(receiver);
    while (current.IsHeapObject()) {
        JSObject *currentObj = JSObject::Cast(current.GetTaggedObject());
        uint32_t numOfCurrentElements = currentObj->GetNumberOfElements(thread);
        if (numOfCurrentElements > 0) {
            return false;
        }
        JSHClass *hclass = currentObj->GetJSHClass();
        JSTaggedValue protoEnumCache = hclass->GetEnumCache();
        if (!protoEnumCache.IsUndefined()) {
            return false;
        }
        current = JSObject::GetPrototype(current);
    }
    return true;
}

bool JSObject::IsEnumCacheWithProtoChainInfoValid(JSThread *thread, JSTaggedValue receiver)
{
    DISALLOW_GARBAGE_COLLECTION;
    // check elements of receiver
    uint32_t numOfElements = JSObject::Cast(receiver.GetTaggedObject())->GetNumberOfElements(thread);
    if (numOfElements > 0) {
        return false;
    }
    // check protochain keys
    JSTaggedValue proto = JSObject::GetPrototype(receiver);
    if (!proto.IsECMAObject()) {
        return false;
    }
    JSTaggedValue protoChangeMarker = proto.GetTaggedObject()->GetClass()->GetProtoChangeMarker();
    if (!protoChangeMarker.IsProtoChangeMarker()) {
        return false;
    }
    if (ProtoChangeMarker::Cast(protoChangeMarker.GetTaggedObject())->GetHasChanged()) {
        return false;
    }
    // check protochain elements
    JSTaggedValue current = proto;
    while (current.IsHeapObject()) {
        JSObject *currentObj = JSObject::Cast(current.GetTaggedObject());
        uint32_t numOfCurrentElements = currentObj->GetNumberOfElements(thread);
        if (numOfCurrentElements > 0) {
            return false;
        }
        current = JSObject::GetPrototype(current);
    }
    return true;
}

JSTaggedValue JSObject::TryGetEnumCache(JSThread *thread, JSTaggedValue obj)
{
    if (obj.IsSlowKeysObject() || obj.GetTaggedObject()->GetClass()->IsDictionaryMode()) {
        return JSTaggedValue::Undefined();
    }
    JSTaggedValue enumCache = obj.GetTaggedObject()->GetClass()->GetEnumCache();
    EnumCacheKind kind = JSObject::GetEnumCacheKind(thread, enumCache);
    bool isEnumCacheValid = false;
    switch (kind) {
        case EnumCacheKind::SIMPLE:
            isEnumCacheValid = IsSimpleEnumCacheValid(thread, obj);
            break;
        case EnumCacheKind::PROTOCHAIN:
            isEnumCacheValid = IsEnumCacheWithProtoChainInfoValid(thread, obj);
            break;
        default:
            break;
    }
    if (!isEnumCacheValid) {
        return JSTaggedValue::Undefined();
    }
    return enumCache;
}

// 13.7.5.15 EnumerateObjectProperties ( O )
JSHandle<JSForInIterator> JSObject::EnumerateObjectProperties(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    JSHandle<JSTaggedValue> object;
    if (obj->IsString()) {
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        object = JSHandle<JSTaggedValue>::Cast(JSPrimitiveRef::StringCreate(thread, obj, undefined));
    } else {
        object = JSTaggedValue::ToPrototypeOrObj(thread, obj);
    }

    JSMutableHandle<JSTaggedValue> keys(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> cachedHclass(thread, JSTaggedValue::Undefined());
    if (object->IsNull() || object->IsUndefined() || object->IsJSNativePointer()) {
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        keys.Update(factory->EmptyArray());
        return factory->NewJSForinIterator(undefined, keys, cachedHclass);
    }
    keys.Update(TryGetEnumCache(thread, object.GetTaggedValue()));
    if (!keys->IsUndefined()) {
        cachedHclass.Update(JSTaggedValue(JSHandle<JSObject>::Cast(object)->GetJSHClass()));
        return thread->GetEcmaVM()->GetFactory()->NewJSForinIterator(object, keys, cachedHclass);
    }
    return LoadEnumerateProperties(thread, object);
}

JSHandle<JSForInIterator> JSObject::LoadEnumerateProperties(JSThread *thread, const JSHandle<JSTaggedValue> &object)
{
    PropertyAccessor accessor(thread, object);
    JSHandle<JSTaggedValue> fastKeys = accessor.GetKeysFast();
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSForInIterator, thread);
    JSMutableHandle<JSTaggedValue> keys(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> cachedHclass(thread, JSTaggedValue::Undefined());
    if (fastKeys->IsUndefined()) {
        keys.Update(accessor.GetKeysSlow());
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSForInIterator, thread);
    } else {
        keys.Update(fastKeys);
        cachedHclass.Update(accessor.GetCachedHclass());
    }
    return thread->GetEcmaVM()->GetFactory()->NewJSForinIterator(object, keys, cachedHclass);
}

void JSObject::DefinePropertyByLiteral(JSThread *thread, const JSHandle<JSObject> &obj,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                       bool useForClass)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    PropertyAttributes attr = useForClass ? PropertyAttributes::Default(true, false, true)
                                          : PropertyAttributes::Default();

    if (value->IsAccessorData()) {
        attr.SetIsAccessor(true);
    }

    uint32_t index = 0;
    if (UNLIKELY(JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        AddElementInternal(thread, obj, index, value, attr);
        return;
    }
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

void JSObject::DefineSetter(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    ObjectOperator op(thread, obj, key, OperatorType::OWN);
    ASSERT(op.IsFound());
    op.DefineSetter(value);
}

void JSObject::DefineGetter(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    ObjectOperator op(thread, obj, key, OperatorType::OWN);
    ASSERT(op.IsFound());
    op.DefineGetter(value);
}

JSHandle<JSObject> JSObject::CreateObjectFromProperties(const JSThread *thread, const JSHandle<TaggedArray> &properties,
                                                        JSTaggedValue ihcVal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    size_t length = properties->GetLength();
    uint32_t propsLen = 0;
    for (size_t i = 0; i < length; i += 2) {  // 2: skip a pair of key and value
        if (properties->Get(i).IsHole()) {
            break;
        }
        propsLen++;
    }
    if (propsLen <= PropertyAttributes::MAX_FAST_PROPS_CAPACITY) {
        JSHandle<JSHClass> hclass;
        bool isLiteral = false;
        if (ihcVal.IsJSHClass()) {
            hclass = JSHandle<JSHClass>(thread, ihcVal);
        } else {
            hclass = factory->GetObjectLiteralHClass(properties, propsLen);
            isLiteral = true;
        }
        if (hclass->IsAOT()) {
            if (CheckPropertiesForRep(properties, propsLen, hclass)) {
                return CreateObjectFromPropertiesByIHClass(thread, properties, propsLen, hclass);
            } else if (!isLiteral) {
                hclass = factory->GetObjectLiteralHClass(properties, propsLen);
                // if check failed, get literal object again
                return CreateObjectFromPropertiesByIHClass(thread, properties, propsLen, hclass);
            }
        }
        return CreateObjectFromProperties(thread, hclass, properties, propsLen);
    } else {
        JSHandle<JSObject> obj = factory->NewEmptyJSObject(0); // 0: no inline field
        ElementsKind oldKind = obj->GetJSHClass()->GetElementsKind();
        JSHClass::TransitionToDictionary(thread, obj);
        JSObject::TryMigrateToGenericKindForJSObject(thread, obj, oldKind);

        JSMutableHandle<NameDictionary> dict(
            thread, NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(propsLen)));
        JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
        for (size_t i = 0; i < propsLen; i++) {
            PropertyAttributes attr = PropertyAttributes::Default();
            // 2: literal contains a pair of key-value
            valueHandle.Update(properties->Get(i * 2 + 1));
            // 2: literal contains a pair of key-value
            keyHandle.Update(properties->Get(i * 2));
            JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread, dict, keyHandle, valueHandle, attr);
            dict.Update(newDict);
        }
        obj->SetProperties(thread, dict);
        return obj;
    }
}

JSHandle<JSObject> JSObject::CreateObjectFromProperties(const JSThread *thread,
                                                        const JSHandle<JSHClass> &hclass,
                                                        const JSHandle<TaggedArray> &properties,
                                                        uint32_t propsLen)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewOldSpaceObjLiteralByHClass(hclass);
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");

    if (thread->IsPGOProfilerEnable()) {
        // PGO need to track TrackType
        JSHClass *oldHC = obj->GetJSHClass();
        LayoutInfo *layoutInfo = LayoutInfo::Cast(oldHC->GetLayout().GetTaggedObject());
        for (size_t i = 0; i < propsLen; i++) {
            auto value = properties->Get(i * 2 + 1);
            auto attr = layoutInfo->GetAttr(i);
            if (attr.UpdateTrackType(value) && !oldHC->IsJSShared()) {
                layoutInfo->SetNormalAttr(thread, i, attr);
            }
            obj->SetPropertyInlinedProps(thread, i, value);
        }
    } else {
        for (size_t i = 0; i < propsLen; i++) {
            // 2: literal contains a pair of key-value
            obj->SetPropertyInlinedProps(thread, i, properties->Get(i * 2 + 1));
        }
    }
    return obj;
}

JSHandle<JSObject> JSObject::CreateObjectFromPropertiesByIHClass(const JSThread *thread,
                                                                 const JSHandle<TaggedArray> &properties,
                                                                 uint32_t propsLen,
                                                                 const JSHandle<JSHClass> &ihc)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSObject> obj = factory->NewOldSpaceObjLiteralByHClass(ihc);
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    auto layout = LayoutInfo::Cast(ihc->GetLayout().GetTaggedObject());
    for (size_t i = 0; i < propsLen; i++) {
        auto attr = layout->GetAttr(i);
        auto value = JSObject::ConvertValueWithRep(attr, properties->Get(i * 2 + 1));
        ASSERT(value.first);
        obj->SetPropertyInlinedPropsWithRep(thread, i, value.second);
    }
    return obj;
}

bool JSObject::CheckPropertiesForRep(
    const JSHandle<TaggedArray> &properties, uint32_t propsLen, const JSHandle<JSHClass> &ihc)
{
    auto layout = LayoutInfo::Cast(ihc->GetLayout().GetTaggedObject());
    for (size_t i = 0; i < propsLen; i++) {
        auto attr = layout->GetAttr(i);
        auto value = JSObject::ConvertValueWithRep(attr, properties->Get(i * 2 + 1));
        // If value.first is false, indicating that value cannot be converted to the expected value of
        // representation. For example, the representation is INT, but the value type is string.
        if (!value.first) {
            return false;
        }
    }
    return true;
}

void JSObject::AddAccessor(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<AccessorData> &value, PropertyAttributes attr)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    ASSERT_PRINT(attr.IsAccessor(), "Attr is not AccessorData");
    ObjectOperator op(thread, obj, key, OperatorType::OWN);
    ASSERT(!op.IsFound());
    op.AddProperty(JSHandle<JSObject>::Cast(obj), JSHandle<JSTaggedValue>(value), attr);
}

bool JSObject::UpdatePropertyInDictionary(const JSThread *thread, JSTaggedValue key, JSTaggedValue value)
{
    DISALLOW_GARBAGE_COLLECTION;
    NameDictionary *dict = NameDictionary::Cast(GetProperties().GetTaggedObject());
    int entry = dict->FindEntry(key);
    if (entry == -1) {
        return false;
    }
    dict->UpdateValue(thread, entry, value);
    return true;
}

void JSObject::TrimInlinePropsSpace(const JSThread *thread, const JSHandle<JSObject> &object,
                                    uint32_t numberInlinedProps)
{
    if (numberInlinedProps > 0) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        uint32_t newSize = object->GetClass()->GetObjectSize();
        size_t trimBytes = numberInlinedProps * JSTaggedValue::TaggedTypeSize();
        factory->FillFreeObject(ToUintPtr(*object) + newSize, trimBytes, RemoveSlots::YES, ToUintPtr(*object));
    }
}

// The hash field may be a hash value, FunctionExtraInfo(JSNativePointer) or TaggedArray
void ECMAObject::SetHash(const JSThread *thread, int32_t hash, const JSHandle<ECMAObject> &obj)
{
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(*obj, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsHeapObject()) {
        // Hash position reserve in advance.
        if (value.IsTaggedArray()) {
            TaggedArray *array = TaggedArray::Cast(value.GetTaggedObject());
            array->Set(thread, array->GetExtraLength() + HASH_INDEX, JSTaggedValue(hash));
        } else if (value.IsNativePointer()) { // FunctionExtraInfo
            JSHandle<JSTaggedValue> nativePointer(thread, value);
            JSHandle<TaggedArray> newArray =
                thread->GetEcmaVM()->GetFactory()->NewTaggedArray(RESOLVED_MAX_SIZE);
            newArray->SetExtraLength(0);
            newArray->Set(thread, HASH_INDEX, JSTaggedValue(hash));
            newArray->Set(thread, FUNCTION_EXTRA_INDEX, nativePointer.GetTaggedValue());
            Barriers::SetObject<true>(thread, *obj, HASH_OFFSET, newArray.GetTaggedValue().GetRawData());
        } else {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    } else {
        Barriers::SetPrimitive<JSTaggedType>(*obj, HASH_OFFSET, JSTaggedValue(hash).GetRawData());
    }
}

int32_t ECMAObject::GetHash() const
{
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsHeapObject()) {
        if (value.IsTaggedArray()) {
            TaggedArray *array = TaggedArray::Cast(value.GetTaggedObject());
            return array->Get(array->GetExtraLength() + HASH_INDEX).GetInt();
        } else {
            // Default is 0
            return 0;
        }
    }
    return value.GetInt();
}

bool ECMAObject::HasHash() const
{
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsInt() && value.GetInt() == 0) {
        return false;
    }
    return true;
}

void *ECMAObject::GetNativePointerField(int32_t index) const
{
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsTaggedArray()) {
        auto array = TaggedArray::Cast(value);
        if (static_cast<int32_t>(array->GetExtraLength()) > index) {
            auto jsValue = array->Get(index);
            if (UNLIKELY(!jsValue.IsJSNativePointer())) {
                LOG_FULL(ERROR) << "jsValue is not js native pointer";
                return nullptr;
            }
            auto pointer = JSNativePointer::Cast(jsValue.GetTaggedObject());
            return pointer->GetExternalPointer();
        }
    }
    return nullptr;
}

// static
void ECMAObject::SetNativePointerField(const JSThread *thread, const JSHandle<JSObject> &obj, int32_t index,
                                       void *nativePointer, const NativePointerCallback &callBack, void *data,
                                       size_t nativeBindingsize, Concurrent isConcurrent)
{
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(*obj, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsTaggedArray()) {
        JSHandle<TaggedArray> array(thread, value);
        if (static_cast<int32_t>(array->GetExtraLength()) > index) {
            EcmaVM *vm = thread->GetEcmaVM();
            JSHandle<JSTaggedValue> current = JSHandle<JSTaggedValue>(thread, array->Get(thread, index));
            if (!current->IsHole() && nativePointer == nullptr) {
                // Try to remove native pointer if exists.
                vm->RemoveFromNativePointerList(*JSHandle<JSNativePointer>(current));
                array->Set(thread, index, JSTaggedValue::Hole());
            } else if (obj->IsJSShared()) {
                JSHandle<JSNativePointer> pointer =
                    vm->GetFactory()->NewSJSNativePointer(nativePointer, callBack, data, false, nativeBindingsize);
                array->Set(thread, index, pointer.GetTaggedValue());
            } else {
                JSHandle<JSNativePointer> pointer = vm->GetFactory()->NewJSNativePointer(
                    nativePointer, callBack, data, false, nativeBindingsize, isConcurrent);
                array->Set(thread, index, pointer.GetTaggedValue());
            }
        }
    }
}

int32_t ECMAObject::GetNativePointerFieldCount() const
{
    int32_t len = 0;
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsTaggedArray()) {
        TaggedArray *array = TaggedArray::Cast(value.GetTaggedObject());
        len = static_cast<int32_t>(array->GetExtraLength());
    }
    return len;
}

// static
void ECMAObject::SetNativePointerFieldCount(const JSThread *thread, const JSHandle<JSObject> &obj, int32_t count)
{
    if (count == 0) {
        return;
    }
    JSTaggedType hashField = Barriers::GetValue<JSTaggedType>(*obj, HASH_OFFSET);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(hashField));
    JSHandle<JSTaggedValue> object(obj);
    bool isShared = object->IsJSShared();
    if (value->IsHeapObject()) {
        if (value->IsTaggedArray()) {
            JSHandle<TaggedArray> array(value);
            // Native Pointer field count is fixed.
            if (array->GetExtraLength() == 0) {
                JSHandle<TaggedArray> newArray =
                    isShared ? thread->GetEcmaVM()->GetFactory()->NewSTaggedArray(count + RESOLVED_MAX_SIZE)
                             : thread->GetEcmaVM()->GetFactory()->NewTaggedArray(count + RESOLVED_MAX_SIZE);
                newArray->SetExtraLength(count);
                newArray->Set(thread, count + HASH_INDEX, array->Get(HASH_INDEX));
                newArray->Set(thread, count + FUNCTION_EXTRA_INDEX, array->Get(FUNCTION_EXTRA_INDEX));
                Barriers::SetObject<true>(thread, *obj, HASH_OFFSET, newArray.GetTaggedValue().GetRawData());
            }
        } else if (value->IsJSNativePointer()) {
            JSHandle<TaggedArray> newArray =
                isShared ? thread->GetEcmaVM()->GetFactory()->NewSTaggedArray(count + RESOLVED_MAX_SIZE)
                         : thread->GetEcmaVM()->GetFactory()->NewTaggedArray(count + RESOLVED_MAX_SIZE);
            newArray->SetExtraLength(count);
            newArray->Set(thread, count + HASH_INDEX, JSTaggedValue(0));
            newArray->Set(thread, count + FUNCTION_EXTRA_INDEX, value);
            Barriers::SetObject<true>(thread, *obj, HASH_OFFSET, newArray.GetTaggedValue().GetRawData());
        } else {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    } else {
        JSHandle<TaggedArray> newArray = isShared ? thread->GetEcmaVM()->GetFactory()->NewSTaggedArray(count + 1)
                                                  : thread->GetEcmaVM()->GetFactory()->NewTaggedArray(count + 1);
        newArray->SetExtraLength(count);
        newArray->Set(thread, count + HASH_INDEX, value);
        Barriers::SetObject<true>(thread, *obj, HASH_OFFSET, newArray.GetTaggedValue().GetRawData());
    }
}

bool JSObject::ElementsAndPropertiesIsEmpty() const
{
    if (TaggedArray::Cast(GetElements().GetTaggedObject())->GetLength() == 0 &&
        TaggedArray::Cast(GetProperties().GetTaggedObject())->GetLength() == 0) {
        return true;
    }
    return false;
}

void JSObject::TryMigrateToGenericKindForJSObject(const JSThread *thread, const JSHandle<JSObject> &obj,
                                                  const ElementsKind oldKind)
{
    if (obj->IsJSArray() && HasMutantTaggedArrayElements(obj)) {
        Elements::MigrateArrayWithKind(thread, obj, oldKind, ElementsKind::GENERIC);
    }
}
}  // namespace panda::ecmascript
