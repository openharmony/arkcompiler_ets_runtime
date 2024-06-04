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

#include "ecmascript/elements.h"
#include "ecmascript/js_hclass-inl.h"

#include <algorithm>

#include "ecmascript/base/config.h"
#include "ecmascript/global_env.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/pgo_profiler/pgo_profiler_layout.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/vtable.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/weak_vector.h"

namespace panda::ecmascript {
using ProfileType = pgo::ProfileType;

JSHandle<TransitionsDictionary> TransitionsDictionary::PutIfAbsent(const JSThread *thread,
                                                                   const JSHandle<TransitionsDictionary> &dictionary,
                                                                   const JSHandle<JSTaggedValue> &key,
                                                                   const JSHandle<JSTaggedValue> &value,
                                                                   const JSHandle<JSTaggedValue> &metaData)
{
    int hash = TransitionsDictionary::Hash(key.GetTaggedValue(), metaData.GetTaggedValue());

    /* no need to add key if exist */
    int entry = dictionary->FindEntry(key.GetTaggedValue(), metaData.GetTaggedValue());
    if (entry != -1) {
        if (dictionary->GetValue(entry).IsUndefined()) {
            JSTaggedValue weakValue = JSTaggedValue(value->CreateAndGetWeakRef());
            dictionary->SetValue(thread, entry, weakValue);
        }
        return dictionary;
    }

    // Check whether the dictionary should be extended.
    JSHandle<TransitionsDictionary> newDictionary(HashTableT::GrowHashTable(thread, dictionary));
    // Compute the key object.
    entry = newDictionary->FindInsertIndex(hash);
    JSTaggedValue val = value.GetTaggedValue();
    newDictionary->SetEntry(thread, entry, key.GetTaggedValue(), val, metaData.GetTaggedValue());

    newDictionary->IncreaseEntries(thread);
    return newDictionary;
}

int TransitionsDictionary::FindEntry(const JSTaggedValue &key, const JSTaggedValue &metaData)
{
    size_t size = static_cast<size_t>(Size());
    uint32_t count = 1;
    int32_t hash = TransitionsDictionary::Hash(key, metaData);
    // GrowHashTable will guarantee the hash table is never full.
    for (uint32_t entry = GetFirstPosition(hash, size);; entry = GetNextPosition(entry, count++, size)) {
        JSTaggedValue element = GetKey(entry);
        if (element.IsHole()) {
            continue;
        }
        if (element.IsUndefined()) {
            return -1;
        }

        if (TransitionsDictionary::IsMatch(key, metaData, element, GetAttributes(entry).GetWeakRawValue())) {
            return static_cast<int>(entry);
        }
    }
    return -1;
}

JSHandle<TransitionsDictionary> TransitionsDictionary::Remove(const JSThread *thread,
                                                              const JSHandle<TransitionsDictionary> &table,
                                                              const JSHandle<JSTaggedValue> &key,
                                                              const JSTaggedValue &metaData)
{
    int entry = table->FindEntry(key.GetTaggedValue(), metaData);
    if (entry == -1) {
        return table;
    }

    table->RemoveElement(thread, entry);
    return TransitionsDictionary::Shrink(thread, table);
}

void TransitionsDictionary::Rehash(const JSThread *thread, TransitionsDictionary *newTable)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (newTable == nullptr) {
        return;
    }
    int size = this->Size();
    // Rehash elements to new table
    int entryCount = 0;
    for (int i = 0; i < size; i++) {
        int fromIndex = GetEntryIndex(i);
        JSTaggedValue k = this->GetKey(i);
        JSTaggedValue v = this->GetValue(i);
        if (IsKey(k) && TransitionsDictionary::CheckWeakExist(v)) {
            int hash = TransitionsDictionary::Hash(k, this->GetAttributes(i));
            int insertionIndex = GetEntryIndex(newTable->FindInsertIndex(hash));
            JSTaggedValue tv = Get(fromIndex);
            newTable->Set(thread, insertionIndex, tv);
            for (int j = 1; j < TransitionsDictionary::ENTRY_SIZE; j++) {
                tv = Get(fromIndex + j);
                newTable->Set(thread, insertionIndex + j, tv);
            }
            entryCount++;
        }
    }
    newTable->SetEntriesCount(thread, entryCount);
    newTable->SetHoleEntriesCount(thread, 0);
}

void JSHClass::InitializeWithDefaultValue(const JSThread *thread, uint32_t size, JSType type, uint32_t inlinedProps)
{
    DISALLOW_GARBAGE_COLLECTION;
    ClearBitField();
    if (JSType::JS_OBJECT_FIRST <= type && type <= JSType::JS_OBJECT_LAST) {
        SetObjectSize(size + inlinedProps * JSTaggedValue::TaggedTypeSize());
        SetInlinedPropsStart(size);
        SetLayout(thread, JSTaggedValue::Null());
    } else {
        SetObjectSize(size);
        SetLayout(thread, JSTaggedValue::Null());
    }
    if (type >= JSType::JS_FUNCTION_FIRST && type <= JSType::JS_FUNCTION_LAST) {
        SetIsJSFunction(true);
    }
    SetPrototype(thread, JSTaggedValue::Null());

    SetObjectType(type);
    SetExtensible(true);
    SetIsPrototype(false);
    SetHasDeleteProperty(false);
    SetIsAllTaggedProp(true);
    SetElementsKind(ElementsKind::GENERIC);
    SetTransitions(thread, JSTaggedValue::Undefined());
    SetParent(thread, JSTaggedValue::Undefined());
    SetProtoChangeMarker(thread, JSTaggedValue::Null());
    SetProtoChangeDetails(thread, JSTaggedValue::Null());
    SetEnumCache(thread, JSTaggedValue::Null());
    SetSupers(thread, JSTaggedValue::Undefined());
    SetLevel(0);
    SetVTable(thread, JSTaggedValue::Undefined());
}

bool JSHClass::IsJSTypeShared(JSType type)
{
    bool isShared = false;
    switch (type) {
        case JSType::JS_SHARED_OBJECT:
        case JSType::JS_SHARED_FUNCTION:
        case JSType::JS_SHARED_SET:
        case JSType::JS_SHARED_MAP:
        case JSType::JS_SHARED_ARRAY:
        case JSType::JS_SHARED_TYPED_ARRAY:
        case JSType::JS_SHARED_INT8_ARRAY:
        case JSType::JS_SHARED_UINT8_ARRAY:
        case JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
        case JSType::JS_SHARED_INT16_ARRAY:
        case JSType::JS_SHARED_UINT16_ARRAY:
        case JSType::JS_SHARED_INT32_ARRAY:
        case JSType::JS_SHARED_UINT32_ARRAY:
        case JSType::JS_SHARED_FLOAT32_ARRAY:
        case JSType::JS_SHARED_FLOAT64_ARRAY:
        case JSType::JS_SHARED_BIGINT64_ARRAY:
        case JSType::JS_SHARED_BIGUINT64_ARRAY:
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
        case JSType::BIGINT:
        case JSType::LINE_STRING:
        case JSType::CONSTANT_STRING:
        case JSType::SLICED_STRING:
        case JSType::TREE_STRING:
            isShared = true;
            break;
        default:
            break;
    }
    return isShared;
}

// class JSHClass
void JSHClass::Initialize(const JSThread *thread, uint32_t size, JSType type, uint32_t inlinedProps)
{
    InitializeWithDefaultValue(thread, size, type, inlinedProps);
    if (JSType::JS_OBJECT_FIRST <= type && type <= JSType::JS_OBJECT_LAST) {
        SetLayout(thread, thread->GlobalConstants()->GetEmptyLayoutInfo());
    }
    InitTSInheritInfo(thread);
}

// for sharedHeap
void JSHClass::Initialize(const JSThread *thread, uint32_t size, JSType type,
    uint32_t inlinedProps, const JSHandle<JSTaggedValue> &layout)
{
    InitializeWithDefaultValue(thread, size, type, inlinedProps);
    if (JSType::JS_OBJECT_FIRST <= type && type <= JSType::JS_OBJECT_LAST) {
        SetLayout(thread, layout);
    }
    if (IsJSTypeShared(type)) {
        SetIsJSShared(true);
    }
}

void JSHClass::InitTSInheritInfo(const JSThread *thread)
{
    // Supers and Level are used to record the relationship between TSHClass.
    if (ShouldSetDefaultSupers()) {
        ASSERT(thread->GlobalConstants()->GetDefaultSupers().IsTaggedArray());
        SetSupers(thread, thread->GlobalConstants()->GetDefaultSupers());
    } else {
        SetSupers(thread, JSTaggedValue::Undefined());
    }
    SetLevel(0);

    // VTable records the location information of properties and methods of TSHClass,
    // which is used to perform efficient IC at runtime
    SetVTable(thread, JSTaggedValue::Undefined());
}

JSHandle<JSHClass> JSHClass::Clone(const JSThread *thread, const JSHandle<JSHClass> &jshclass,
                                   bool withoutInlinedProperties, uint32_t incInlinedProperties)
{
    JSType type = jshclass->GetObjectType();
    uint32_t size = jshclass->GetInlinedPropsStartSize();
    uint32_t numInlinedProps = withoutInlinedProperties ? 0 : jshclass->GetInlinedProperties() + incInlinedProperties;
    JSHandle<JSHClass> newJsHClass;
    if (jshclass.GetTaggedValue().IsInSharedHeap()) {
        newJsHClass = thread->GetEcmaVM()->GetFactory()->NewSEcmaHClass(size, type, numInlinedProps);
    } else {
        newJsHClass = thread->GetEcmaVM()->GetFactory()->NewEcmaHClass(size, type, numInlinedProps);
    }
    // Copy all
    newJsHClass->Copy(thread, *jshclass);
    newJsHClass->SetTransitions(thread, JSTaggedValue::Undefined());
    newJsHClass->SetParent(thread, JSTaggedValue::Undefined());
    newJsHClass->SetProtoChangeDetails(thread, JSTaggedValue::Null());
    newJsHClass->SetEnumCache(thread, JSTaggedValue::Null());
    // reuse Attributes first.
    newJsHClass->SetLayout(thread, jshclass->GetLayout());

    if (jshclass->IsTS()) {
        newJsHClass->SetTS(false);
    }

    return newJsHClass;
}

// use for transition to dictionary
JSHandle<JSHClass> JSHClass::CloneWithoutInlinedProperties(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    return Clone(thread, jshclass, true);
}

void JSHClass::TransitionElementsToDictionary(const JSThread *thread, const JSHandle<JSObject> &obj)
{
    // property transition to slow first
    if (!obj->GetJSHClass()->IsDictionaryMode()) {
        JSObject::TransitionToDictionary(thread, obj);
    } else {
        TransitionToDictionary(thread, obj);
    }
    obj->GetJSHClass()->SetIsDictionaryElement(true);
    obj->GetJSHClass()->SetIsStableElements(false);
    obj->GetJSHClass()->SetElementsKind(ElementsKind::DICTIONARY);
}

void JSHClass::OptimizeAsFastElements(const JSThread *thread, JSHandle<JSObject> obj)
{
    if (obj->GetJSHClass()->IsDictionaryMode()) {
        JSObject::OptimizeAsFastProperties(thread, obj);
    } else {
        OptimizeAsFastProperties(thread, obj);
    }
    obj->GetJSHClass()->SetIsDictionaryElement(false);
    obj->GetJSHClass()->SetIsStableElements(true);
    obj->GetJSHClass()->SetElementsKind(ElementsKind::HOLE_TAGGED);
}

void JSHClass::AddProperty(const JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                           const PropertyAttributes &attr)
{
    JSHandle<JSHClass> jshclass(thread, obj->GetJSHClass());
    JSHClass *newClass = jshclass->FindTransitions(key.GetTaggedValue(), JSTaggedValue(attr.GetPropertyMetaData()));
    if (newClass != nullptr) {
        // The transition hclass from AOT, which does not have a prototype, needs to be reset here.
        if (newClass->IsTS()) {
            newClass->SetPrototype(thread, jshclass->GetPrototype());
        }
        obj->SynchronizedSetClass(thread, newClass);
        // Because we currently only supports Fast ElementsKind
        JSHandle<JSHClass> newHClass(thread, newClass);
        TryRestoreElementsKind(thread, newHClass, obj);
#if ECMASCRIPT_ENABLE_IC
        JSHClass::NotifyHclassChanged(thread, jshclass, JSHandle<JSHClass>(thread, newClass), key.GetTaggedValue());
#endif
        // The transition hclass from AOT, which does not have protochangemarker, needs to be reset here
        if (newClass->IsTS() && newClass->IsPrototype()) {
            JSHClass::RefreshUsers(thread, jshclass, JSHandle<JSHClass>(thread, newClass));
            JSHClass::EnableProtoChangeMarker(thread, JSHandle<JSHClass>(thread, newClass));
        }
        return;
    }
    JSHandle<JSHClass> newJsHClass = JSHClass::Clone(thread, jshclass);
    AddPropertyToNewHClass(thread, jshclass, newJsHClass, key, attr);
    // update hclass in object.
#if ECMASCRIPT_ENABLE_IC
    JSHClass::NotifyHclassChanged(thread, jshclass, newJsHClass, key.GetTaggedValue());
#endif
    obj->SynchronizedSetClass(thread, *newJsHClass);
    // Because we currently only supports Fast ElementsKind
    TryRestoreElementsKind(thread, newJsHClass, obj);
}

void JSHClass::TryRestoreElementsKind(const JSThread *thread, JSHandle<JSHClass> newJsHClass,
                                      const JSHandle<JSObject> &obj)
{
    ElementsKind newKind = ElementsKind::GENERIC;
    if (newJsHClass->GetObjectType() == JSType::JS_ARRAY &&
        obj->GetElements().IsMutantTaggedArray()) {
        ElementsKind oldKind = newJsHClass->GetElementsKind();
        Elements::MigrateArrayWithKind(thread, obj, oldKind, newKind);
    }
    newJsHClass->SetElementsKind(newKind);
}

JSHandle<JSHClass> JSHClass::TransitionExtension(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledPreventExtensionsString());
    {
        auto *newClass = jshclass->FindTransitions(key.GetTaggedValue(), JSTaggedValue(0));
        if (newClass != nullptr) {
            newClass->SetPrototype(thread, jshclass->GetPrototype());
            return JSHandle<JSHClass>(thread, newClass);
        }
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 2. new a hclass
    JSHandle<JSHClass> newJsHClass = JSHClass::Clone(thread, jshclass);
    newJsHClass->SetExtensible(false);

    JSTaggedValue attrs = newJsHClass->GetLayout();
    {
        JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, attrs);
        layoutInfoHandle.Update(factory->CopyLayoutInfo(layoutInfoHandle).GetTaggedValue());
        newJsHClass->SetLayout(thread, layoutInfoHandle);
    }

    // 3. Add newClass to old hclass's parent's transitions.
    AddExtensionTransitions(thread, jshclass, newJsHClass, key);
    // parent is the same as jshclass, already copy
    return newJsHClass;
}

JSHandle<JSHClass> JSHClass::TransitionProto(const JSThread *thread, const JSHandle<JSHClass> &jshclass,
                                             const JSHandle<JSTaggedValue> &proto)
{
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledPrototypeString());

    {
        auto *newClass = jshclass->FindProtoTransitions(key.GetTaggedValue(), proto.GetTaggedValue());
        if (newClass != nullptr) {
            return JSHandle<JSHClass>(thread, newClass);
        }
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 2. new a hclass
    JSHandle<JSHClass> newJsHClass = JSHClass::Clone(thread, jshclass);
    newJsHClass->SetPrototype(thread, proto.GetTaggedValue());

    JSTaggedValue layout = newJsHClass->GetLayout();
    {
        JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, layout);
        layoutInfoHandle.Update(factory->CopyLayoutInfo(layoutInfoHandle).GetTaggedValue());
        newJsHClass->SetLayout(thread, layoutInfoHandle);
    }

    // 3. Add newJsHClass to old jshclass's parent's transitions.
    AddProtoTransitions(thread, jshclass, newJsHClass, key, proto);

    // parent is the same as jshclass, already copy
    return newJsHClass;
}

JSHandle<JSHClass> JSHClass::CloneWithAddProto(const JSThread *thread, const JSHandle<JSHClass> &jshclass,
                                               const JSHandle<JSTaggedValue> &key,
                                               const JSHandle<JSTaggedValue> &proto)
{
    // 1. new a hclass
    JSHandle<JSHClass> newJsHClass = JSHClass::Clone(thread, jshclass);
    newJsHClass->SetPrototype(thread, proto.GetTaggedValue());

    // 2. Add newJsHClass to old jshclass's parent's transitions.
    AddProtoTransitions(thread, jshclass, newJsHClass, key, proto);
    // parent is the same as jshclass, already copy
    return newJsHClass;
}

JSHandle<JSHClass> JSHClass::TransProtoWithoutLayout(const JSThread *thread, const JSHandle<JSHClass> &jshclass,
                                                     const JSHandle<JSTaggedValue> &proto)
{
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledPrototypeString());

    {
        auto *newClass = jshclass->FindProtoTransitions(key.GetTaggedValue(), proto.GetTaggedValue());
        if (newClass != nullptr) {
            return JSHandle<JSHClass>(thread, newClass);
        }
    }

    return CloneWithAddProto(thread, jshclass, key, proto);
}

void JSHClass::SetPrototype(const JSThread *thread, JSTaggedValue proto)
{
    // Because the heap-space of hclass is non-movable, this function can be non-static.
    JSHandle<JSTaggedValue> protoHandle(thread, proto);
    SetPrototype(thread, protoHandle);
}

JSHandle<JSTaggedValue> JSHClass::SetPrototypeWithNotification(const JSThread *thread,
                                                               const JSHandle<JSTaggedValue> &hclass,
                                                               const JSHandle<JSTaggedValue> &proto)
{
    JSHandle<JSHClass> newClass = JSHClass::TransitionProto(thread, JSHandle<JSHClass>::Cast(hclass), proto);
    JSHClass::NotifyHclassChanged(thread, JSHandle<JSHClass>::Cast(hclass), newClass);
    return JSHandle<JSTaggedValue>(newClass);
}

void JSHClass::SetPrototypeTransition(JSThread *thread, const JSHandle<JSObject> &object,
                                      const JSHandle<JSTaggedValue> &proto)
{
    JSHandle<JSHClass> hclass(thread, object->GetJSHClass());
    JSHandle<JSHClass> newClass = JSHClass::TransitionProto(thread, hclass, proto);
    JSHClass::NotifyHclassChanged(thread, hclass, newClass);
    object->SynchronizedSetClass(thread, *newClass);
    JSHClass::TryRestoreElementsKind(thread, newClass, object);
    thread->NotifyStableArrayElementsGuardians(object, StableArrayChangeKind::PROTO);
    ObjectOperator::UpdateDetectorOnSetPrototype(thread, object.GetTaggedValue());
}

void JSHClass::SetPrototype(const JSThread *thread, const JSHandle<JSTaggedValue> &proto)
{
    // Because the heap-space of hclass is non-movable, this function can be non-static.
    if (proto->IsJSObject()) {
        OptimizePrototypeForIC(thread, proto);
    }
    SetProto(thread, proto);
}

void JSHClass::OptimizePrototypeForIC(const JSThread *thread, const JSHandle<JSTaggedValue> &proto)
{
    JSHandle<JSHClass> hclass(thread, proto->GetTaggedObject()->GetClass());
    ASSERT(!Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(*hclass))->InReadOnlySpace());
    if (!hclass->IsPrototype()) {
        if (!hclass->IsTS() && !hclass->IsJSShared()) {
            // The local IC and on-proto IC are different, because the former don't need to notify the whole
            // prototype-chain or listen the changes of prototype chain, but the latter do. Therefore, when
            // an object becomes a prototype object at the first time, we need to copy its hidden class in
            // order to maintain the previously generated local IC and support the on-proto IC in the future.
            // For example, a local IC adds a new property x for o1 and the o1.hclass1 -> o1.hclass2, when the
            // o1 becomes a prototype object of object o2 and an on-proto IC loading x from o2 will rely on the
            // stability of the prototype-chain o2 -> o1. If directly marking the o1.hclass1 as a prototype hclass,
            // the previous IC of adding property x won't trigger IC-miss and fails to notify the IC on o2.
            JSHandle<JSHClass> newProtoClass = JSHClass::Clone(thread, hclass);
            JSTaggedValue layout = newProtoClass->GetLayout();
            // If the type of object is JSObject, the layout info value is initialized to the default value,
            // if the value is not JSObject, the layout info value is initialized to null.
            if (!layout.IsNull()) {
                JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, layout);
                layoutInfoHandle.Update(
                    thread->GetEcmaVM()->GetFactory()->CopyLayoutInfo(layoutInfoHandle).GetTaggedValue());
                newProtoClass->SetLayout(thread, layoutInfoHandle);
            }

#if ECMASCRIPT_ENABLE_IC
            // After the hclass is updated, check whether the proto chain status of ic is updated.
            NotifyHclassChanged(thread, hclass, newProtoClass);
#endif
            JSObject::Cast(proto->GetTaggedObject())->SynchronizedSetClass(thread, *newProtoClass);
            newProtoClass->SetIsPrototype(true);
            thread->GetEcmaVM()->GetPGOProfiler()->UpdateRootProfileTypeSafe(*hclass, *newProtoClass);
        } else {
            // There is no sharing in AOT hclass. Therefore, it is not necessary or possible to clone here.
            hclass->SetIsPrototype(true);
        }
    }
}

void JSHClass::TransitionToDictionary(const JSThread *thread, const JSHandle<JSObject> &obj)
{
    // 1. new a hclass
    JSHandle<JSHClass> jshclass(thread, obj->GetJSHClass());
    JSHandle<JSHClass> newJsHClass = CloneWithoutInlinedProperties(thread, jshclass);
    UpdateRootHClass(thread, jshclass, newJsHClass);

    {
        DISALLOW_GARBAGE_COLLECTION;
        // 2. Copy
        newJsHClass->SetNumberOfProps(0);
        newJsHClass->SetIsDictionaryMode(true);
        ASSERT(newJsHClass->GetInlinedProperties() == 0);

        // 3. Add newJsHClass to ?
#if ECMASCRIPT_ENABLE_IC
        JSHClass::NotifyHclassChanged(thread, JSHandle<JSHClass>(thread, obj->GetJSHClass()), newJsHClass);
#endif
        obj->SynchronizedSetClass(thread, *newJsHClass);
        TryRestoreElementsKind(thread, newJsHClass, obj);
    }
}

void JSHClass::OptimizeAsFastProperties(const JSThread *thread, const JSHandle<JSObject> &obj,
                                        const std::vector<int> &indexOrder, bool isDictionary)
{
    // 1. new a hclass
    JSHandle<JSHClass> jshclass(thread, obj->GetJSHClass());
    JSHandle<JSHClass> newJsHClass = Clone(thread, jshclass, isDictionary);
    UpdateRootHClass(thread, jshclass, newJsHClass);

    // 2. If it is dictionary, migrate should change layout. otherwise, copy the hclass only.
    JSHandle<NameDictionary> properties(thread, obj->GetProperties());
    int numberOfProperties = properties->EntriesCount();
    if (isDictionary) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<LayoutInfo> layoutInfoHandle = factory->CreateLayoutInfo(numberOfProperties);
        int numberOfInlinedProps = static_cast<int>(newJsHClass->GetInlinedProperties());
        for (int i = 0; i < numberOfProperties; i++) {
            JSTaggedValue key = properties->GetKey(indexOrder[i]);
            PropertyAttributes attributes = properties->GetAttributes(indexOrder[i]);
            if (i < numberOfInlinedProps) {
                attributes.SetIsInlinedProps(true);
            } else {
                attributes.SetIsInlinedProps(false);
            }
            attributes.SetOffset(i);
            layoutInfoHandle->AddKey(thread, i, key, attributes);
        }

        {
            DISALLOW_GARBAGE_COLLECTION;
            newJsHClass->SetNumberOfProps(numberOfProperties);
            newJsHClass->SetLayout(thread, layoutInfoHandle);
        }
    }

    {
        DISALLOW_GARBAGE_COLLECTION;
        // 3. Copy
        newJsHClass->SetIsDictionaryMode(false);
        // 4. Add newJsHClass to ?
#if ECMASCRIPT_ENABLE_IC
        JSHClass::NotifyHclassChanged(thread, JSHandle<JSHClass>(thread, obj->GetJSHClass()), newJsHClass);
#endif
        obj->SynchronizedSetClass(thread, *newJsHClass);
    }
}

void JSHClass::TransitionForRepChange(const JSThread *thread, const JSHandle<JSObject> &receiver,
    const JSHandle<JSTaggedValue> &key, PropertyAttributes attr)
{
    JSHandle<JSHClass> oldHClass(thread, receiver->GetJSHClass());

    // 1. Create hclass and copy layout
    JSHandle<JSHClass> newHClass = JSHClass::Clone(thread, oldHClass);

    JSHandle<LayoutInfo> oldLayout(thread, newHClass->GetLayout());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LayoutInfo> newLayout(factory->CopyLayoutInfo(oldLayout));
    newHClass->SetLayout(thread, newLayout);

    // 2. update attr
    auto hclass = JSHClass::Cast(newHClass.GetTaggedValue().GetTaggedObject());
    int entry = JSHClass::FindPropertyEntry(thread, hclass, key.GetTaggedValue());
    ASSERT(entry != -1);
    newLayout->SetNormalAttr(thread, entry, attr);

    // 3. update hclass in object.
#if ECMASCRIPT_ENABLE_IC
    JSHClass::NotifyHclassChanged(thread, oldHClass, newHClass, key.GetTaggedValue());
#endif

    receiver->SynchronizedSetClass(thread, *newHClass);
    TryRestoreElementsKind(thread, newHClass, receiver);
    // 4. Maybe Transition And Maintain subtypeing check
}

JSHClass* JSHClass::GetInitialArrayHClassWithElementsKind(const JSThread *thread, const ElementsKind kind)
{
    const auto &arrayHClassIndexMap = thread->GetArrayHClassIndexMap();
    auto newKindIter = arrayHClassIndexMap.find(kind);
    if (newKindIter != arrayHClassIndexMap.end()) {
        auto index = static_cast<size_t>(newKindIter->second);
        auto hclassVal = thread->GlobalConstants()->GetGlobalConstantObject(index);
        JSHClass *hclass = JSHClass::Cast(hclassVal.GetTaggedObject());
        return hclass;
    }
    return nullptr;
}

bool JSHClass::TransitToElementsKindUncheck(const JSThread *thread, const JSHandle<JSObject> &obj,
                                            ElementsKind newKind)
{
    ElementsKind current = obj->GetJSHClass()->GetElementsKind();
    // currently we only support initial array hclass
    if (obj->GetClass() == GetInitialArrayHClassWithElementsKind(thread, current)) {
        const auto &arrayHClassIndexMap = thread->GetArrayHClassIndexMap();
        auto newKindIter = arrayHClassIndexMap.find(newKind);
        if (newKindIter != arrayHClassIndexMap.end()) {
            auto index = static_cast<size_t>(newKindIter->second);
            auto hclassVal = thread->GlobalConstants()->GetGlobalConstantObject(index);
            JSHClass *hclass = JSHClass::Cast(hclassVal.GetTaggedObject());
            obj->SynchronizedSetClass(thread, hclass);
            return true;
        }
    }
    return false;
}

void JSHClass::TransitToElementsKind(const JSThread *thread, const JSHandle<JSArray> &array,
                                     ElementsKind newKind)
{
    JSTaggedValue elements = array->GetElements();
    if (!elements.IsTaggedArray()) {
        return;
    }
    ElementsKind current = array->GetJSHClass()->GetElementsKind();
    newKind = Elements::MergeElementsKind(newKind, current);
    if (newKind == current) {
        return;
    }
    // Currently, we only support fast array elementsKind
    ASSERT(array->GetClass() == GetInitialArrayHClassWithElementsKind(thread, current));
    const auto &arrayHClassIndexMap = thread->GetArrayHClassIndexMap();
    auto newKindIter = arrayHClassIndexMap.find(newKind);
    if (newKindIter != arrayHClassIndexMap.end()) {
        auto index = static_cast<size_t>(newKindIter->second);
        auto hclassVal = thread->GlobalConstants()->GetGlobalConstantObject(index);
        JSHClass *hclass = JSHClass::Cast(hclassVal.GetTaggedObject());
        array->SynchronizedSetClass(thread, hclass);
    }
}

bool JSHClass::TransitToElementsKind(const JSThread *thread, const JSHandle<JSObject> &object,
                                     const JSHandle<JSTaggedValue> &value, ElementsKind kind)
{
    if (!object->IsJSArray()) {
        return false;
    }
    ElementsKind current = object->GetJSHClass()->GetElementsKind();
    if (Elements::IsGeneric(current)) {
        return false;
    }
    auto newKind = Elements::ToElementsKind(value.GetTaggedValue(), kind);
    // Merge current kind and new kind
    newKind = Elements::MergeElementsKind(current, newKind);
    if (newKind == current) {
        return false;
    }
    // Currently, we only support fast array elementsKind
    ASSERT(object->GetClass() == GetInitialArrayHClassWithElementsKind(thread, current));
    const auto &arrayHClassIndexMap = thread->GetArrayHClassIndexMap();
    auto newKindIter = arrayHClassIndexMap.find(newKind);
    if (newKindIter != arrayHClassIndexMap.end()) {
        auto index = static_cast<size_t>(newKindIter->second);
        auto hclassVal = thread->GlobalConstants()->GetGlobalConstantObject(index);
        JSHClass *hclass = JSHClass::Cast(hclassVal.GetTaggedObject());
        object->SynchronizedSetClass(thread, hclass);

        if (!thread->GetEcmaVM()->IsEnableElementsKind()) {
            // Update TrackInfo
            if (!thread->IsPGOProfilerEnable()) {
                return true;
            }
            auto trackInfoVal = JSHandle<JSArray>(object)->GetTrackInfo();
            thread->GetEcmaVM()->GetPGOProfiler()->UpdateTrackElementsKind(trackInfoVal, newKind);
            return true;
        }
        return true;
    }
    return false;
}

std::tuple<bool, bool, JSTaggedValue> JSHClass::ConvertOrTransitionWithRep(const JSThread *thread,
    const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
    PropertyAttributes &attr)
{
    auto hclass = receiver->GetJSHClass();
    auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    attr = layout->GetAttr(attr.GetOffset());
    if (thread->IsPGOProfilerEnable() && !hclass->IsJSShared() && attr.UpdateTrackType(value.GetTaggedValue())) {
        layout->SetNormalAttr(thread, attr.GetOffset(), attr);
    }

    Representation oldRep = attr.GetRepresentation();
    if (oldRep == Representation::DOUBLE) {
        if (value->IsInt()) {
            double doubleValue = value->GetInt();
            return std::tuple<bool, bool, JSTaggedValue>(false, false,
                JSTaggedValue(bit_cast<JSTaggedType>(doubleValue)));
        } else if (value->IsObject()) {
            // Is Object
            attr.SetRepresentation(Representation::TAGGED);
            // Transition
            JSHClass::TransitionForRepChange(thread, receiver, key, attr);
            return std::tuple<bool, bool, JSTaggedValue>(true, true, value.GetTaggedValue());
        } else {
            // Is TaggedDouble
            return std::tuple<bool, bool, JSTaggedValue>(false, false,
                JSTaggedValue(bit_cast<JSTaggedType>(value->GetDouble())));
        }
    } else if (oldRep == Representation::INT) {
        if (value->IsInt()) {
            int intValue = value->GetInt();
            return std::tuple<bool, bool, JSTaggedValue>(false, false, JSTaggedValue(static_cast<JSTaggedType>(intValue)));
        } else {
            attr.SetRepresentation(Representation::TAGGED);
            JSHClass::TransitionForRepChange(thread, receiver, key, attr);
            return std::tuple<bool, bool, JSTaggedValue>(true, true, value.GetTaggedValue());
        }
    }
    return std::tuple<bool, bool, JSTaggedValue>(true, false, value.GetTaggedValue());
}

JSHandle<JSTaggedValue> JSHClass::EnableProtoChangeMarker(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    JSTaggedValue proto = jshclass->GetPrototype();
    if (!proto.IsECMAObject()) {
        // Return JSTaggedValue directly. No proto check is needed.
        LOG_ECMA(INFO) << "proto is not ecmaobject: " << proto.GetRawData();
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null());
    }
    JSHandle<JSObject> protoHandle(thread, proto);
    JSHandle<JSHClass> protoClass(thread, protoHandle->GetJSHClass());
    // in AOT's IC mechanism (VTable), when the prototype chain changes, it needs to notify each subclass
    // PHC (prototype-HClass) and its IHC (instance-HClass) from the current PHC along the chain.
    // therefore, when registering, it is also necessary to register IHC into its
    // PHC's Listener to ensure that it can be notified.
    if (jshclass->IsTSIHCWithInheritInfo()) {
        RegisterOnProtoChain(thread, jshclass);
    } else {
        RegisterOnProtoChain(thread, protoClass);
    }

    JSTaggedValue protoChangeMarker = protoClass->GetProtoChangeMarker();
    if (protoChangeMarker.IsProtoChangeMarker()) {
        JSHandle<ProtoChangeMarker> markerHandle(thread, ProtoChangeMarker::Cast(protoChangeMarker.GetTaggedObject()));
        if (!markerHandle->GetHasChanged()) {
            return JSHandle<JSTaggedValue>(markerHandle);
        }
    }
    JSHandle<ProtoChangeMarker> markerHandle = thread->GetEcmaVM()->GetFactory()->NewProtoChangeMarker();
    markerHandle->SetHasChanged(false);
    // ShareToLocal is prohibited
    if (!protoClass->IsJSShared()) {
        protoClass->SetProtoChangeMarker(thread, markerHandle.GetTaggedValue());
    }
    return JSHandle<JSTaggedValue>(markerHandle);
}

JSHandle<JSTaggedValue> JSHClass::EnablePHCProtoChangeMarker(const JSThread *thread,
    const JSHandle<JSHClass> &protoClass)
{
    RegisterOnProtoChain(thread, protoClass);

    JSTaggedValue protoChangeMarker = protoClass->GetProtoChangeMarker();
    if (protoChangeMarker.IsProtoChangeMarker()) {
        JSHandle<ProtoChangeMarker> markerHandle(thread, ProtoChangeMarker::Cast(protoChangeMarker.GetTaggedObject()));
        if (!markerHandle->GetHasChanged()) {
            return JSHandle<JSTaggedValue>(markerHandle);
        }
    }
    JSHandle<ProtoChangeMarker> markerHandle = thread->GetEcmaVM()->GetFactory()->NewProtoChangeMarker();
    markerHandle->SetHasChanged(false);
    protoClass->SetProtoChangeMarker(thread, markerHandle.GetTaggedValue());
    return JSHandle<JSTaggedValue>(markerHandle);
}

void JSHClass::NotifyHclassChanged(const JSThread *thread, JSHandle<JSHClass> oldHclass, JSHandle<JSHClass> newHclass,
                                   JSTaggedValue addedKey)
{
    if (!oldHclass->IsPrototype()) {
        return;
    }
    // The old hclass is the same as new one
    if (oldHclass.GetTaggedValue() == newHclass.GetTaggedValue()) {
        return;
    }
    ASSERT(newHclass->IsPrototype());
    JSHClass::NoticeThroughChain(thread, oldHclass, addedKey);
    JSHClass::RefreshUsers(thread, oldHclass, newHclass);
}

void JSHClass::NotifyAccessorChanged(const JSThread *thread, JSHandle<JSHClass> hclass)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue markerValue = hclass->GetProtoChangeMarker();
    if (markerValue.IsProtoChangeMarker()) {
        ProtoChangeMarker *protoChangeMarker = ProtoChangeMarker::Cast(markerValue.GetTaggedObject());
        protoChangeMarker->SetAccessorHasChanged(true);
    }

    JSTaggedValue protoDetailsValue = hclass->GetProtoChangeDetails();
    if (!protoDetailsValue.IsProtoChangeDetails()) {
        return;
    }
    JSTaggedValue listenersValue = ProtoChangeDetails::Cast(protoDetailsValue.GetTaggedObject())->GetChangeListener();
    if (!listenersValue.IsTaggedArray()) {
        return;
    }
    ChangeListener *listeners = ChangeListener::Cast(listenersValue.GetTaggedObject());
    for (uint32_t i = 0; i < listeners->GetEnd(); i++) {
        JSTaggedValue temp = listeners->Get(i);
        if (temp.IsJSHClass()) {
            NotifyAccessorChanged(thread, JSHandle<JSHClass>(thread, listeners->Get(i).GetTaggedObject()));
        }
    }
}

void JSHClass::RegisterOnProtoChain(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    // ShareToLocal is prohibited
    if (jshclass->IsJSShared()) {
        return;
    }
    JSHandle<JSHClass> user = jshclass;
    JSHandle<ProtoChangeDetails> userDetails = GetProtoChangeDetails(thread, user);

    while (true) {
        // Find the prototype chain as far as the hclass has not been registered.
        if (userDetails->GetRegisterIndex() != static_cast<uint32_t>(ProtoChangeDetails::UNREGISTERED)) {
            return;
        }

        JSTaggedValue proto = user->GetPrototype();
        if (!proto.IsHeapObject()) {
            return;
        }
        if (proto.IsJSProxy()) {
            return;
        }
        // ShareToLocal is prohibited
        if (proto.IsJSShared()) {
            return;
        }
        ASSERT(proto.IsECMAObject());
        JSHandle<JSObject> protoHandle(thread, proto);
        JSHandle<ProtoChangeDetails> protoDetails =
            GetProtoChangeDetails(thread, JSHandle<JSHClass>(thread, protoHandle->GetJSHClass()));
        JSTaggedValue listeners = protoDetails->GetChangeListener();
        JSHandle<ChangeListener> listenersHandle;
        if (listeners.IsUndefined()) {
            listenersHandle = JSHandle<ChangeListener>(ChangeListener::Create(thread));
        } else {
            listenersHandle = JSHandle<ChangeListener>(thread, listeners);
        }
        uint32_t registerIndex = 0;
        JSHandle<ChangeListener> newListeners = ChangeListener::Add(thread, listenersHandle, user, &registerIndex);
        userDetails->SetRegisterIndex(registerIndex);
        protoDetails->SetChangeListener(thread, newListeners.GetTaggedValue());
        userDetails = protoDetails;
        user = JSHandle<JSHClass>(thread, protoHandle->GetJSHClass());
    }
}

bool JSHClass::UnregisterOnProtoChain(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    ASSERT(jshclass->IsPrototype());
    if (!jshclass->GetProtoChangeDetails().IsProtoChangeDetails()) {
        return false;
    }
    if (!jshclass->GetPrototype().IsECMAObject()) {
        JSTaggedValue listeners =
            ProtoChangeDetails::Cast(jshclass->GetProtoChangeDetails().GetTaggedObject())->GetChangeListener();
        return !listeners.IsUndefined();
    }
    JSHandle<ProtoChangeDetails> currentDetails = GetProtoChangeDetails(thread, jshclass);
    uint32_t index = currentDetails->GetRegisterIndex();
    if (index == static_cast<uint32_t>(ProtoChangeDetails::UNREGISTERED)) {
        return false;
    }
    JSTaggedValue proto = jshclass->GetPrototype();
    ASSERT(proto.IsECMAObject());
    JSTaggedValue protoDetailsValue = JSObject::Cast(proto.GetTaggedObject())->GetJSHClass()->GetProtoChangeDetails();
    if (protoDetailsValue.IsUndefined() || protoDetailsValue.IsNull()) {
        return false;
    }
    ASSERT(protoDetailsValue.IsProtoChangeDetails());
    JSTaggedValue listenersValue = ProtoChangeDetails::Cast(protoDetailsValue.GetTaggedObject())->GetChangeListener();
    ASSERT(!listenersValue.IsUndefined());
    JSHandle<ChangeListener> listeners(thread, listenersValue.GetTaggedObject());
    ASSERT(listeners->Get(index) == jshclass.GetTaggedValue());
    listeners->Delete(thread, index);
    return true;
}

JSHandle<ProtoChangeDetails> JSHClass::GetProtoChangeDetails(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    JSTaggedValue protoDetails = jshclass->GetProtoChangeDetails();
    if (protoDetails.IsProtoChangeDetails()) {
        return JSHandle<ProtoChangeDetails>(thread, protoDetails);
    }
    JSHandle<ProtoChangeDetails> protoDetailsHandle = thread->GetEcmaVM()->GetFactory()->NewProtoChangeDetails();
    jshclass->SetProtoChangeDetails(thread, protoDetailsHandle.GetTaggedValue());
    return protoDetailsHandle;
}

JSHandle<ProtoChangeDetails> JSHClass::GetProtoChangeDetails(const JSThread *thread, const JSHandle<JSObject> &obj)
{
    JSHandle<JSHClass> jshclass(thread, obj->GetJSHClass());
    return GetProtoChangeDetails(thread, jshclass);
}

void JSHClass::MarkProtoChanged(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(jshclass->IsPrototype() || jshclass->HasTSSubtyping());
    JSTaggedValue markerValue = jshclass->GetProtoChangeMarker();
    if (markerValue.IsProtoChangeMarker()) {
        ProtoChangeMarker *protoChangeMarker = ProtoChangeMarker::Cast(markerValue.GetTaggedObject());
        protoChangeMarker->SetHasChanged(true);
    }

    if (jshclass->HasTSSubtyping()) {
        jshclass->InitTSInheritInfo(thread);
    }
}

void JSHClass::NoticeThroughChain(const JSThread *thread, const JSHandle<JSHClass> &jshclass,
                                  JSTaggedValue addedKey)
{
    DISALLOW_GARBAGE_COLLECTION;
    MarkProtoChanged(thread, jshclass);
    JSTaggedValue protoDetailsValue = jshclass->GetProtoChangeDetails();
    if (!protoDetailsValue.IsProtoChangeDetails()) {
        return;
    }
    JSTaggedValue listenersValue = ProtoChangeDetails::Cast(protoDetailsValue.GetTaggedObject())->GetChangeListener();
    if (!listenersValue.IsTaggedArray()) {
        return;
    }
    ChangeListener *listeners = ChangeListener::Cast(listenersValue.GetTaggedObject());
    for (uint32_t i = 0; i < listeners->GetEnd(); i++) {
        JSTaggedValue temp = listeners->Get(i);
        if (temp.IsJSHClass()) {
            NoticeThroughChain(thread, JSHandle<JSHClass>(thread, listeners->Get(i).GetTaggedObject()), addedKey);
        }
    }
}

void JSHClass::RefreshUsers(const JSThread *thread, const JSHandle<JSHClass> &oldHclass,
                            const JSHandle<JSHClass> &newHclass)
{
    ASSERT(oldHclass->IsPrototype());
    ASSERT(newHclass->IsPrototype());
    bool onceRegistered = UnregisterOnProtoChain(thread, oldHclass);

    // oldHclass is already marked. Only update newHclass.protoChangeDetails if it doesn't exist for further use.
    if (!newHclass->GetProtoChangeDetails().IsProtoChangeDetails()) {
        newHclass->SetProtoChangeDetails(thread, oldHclass->GetProtoChangeDetails());
    }
    oldHclass->SetProtoChangeDetails(thread, JSTaggedValue::Undefined());
    if (onceRegistered) {
        if (newHclass->GetProtoChangeDetails().IsProtoChangeDetails()) {
            ProtoChangeDetails::Cast(newHclass->GetProtoChangeDetails().GetTaggedObject())
                ->SetRegisterIndex(ProtoChangeDetails::UNREGISTERED);
        }
        RegisterOnProtoChain(thread, newHclass);
    }
}

bool JSHClass::HasTSSubtyping() const
{
    // if fill TS inherit info, supers must not be empty
    if (!GetSupers().IsHeapObject()) {
        return false;
    }
    WeakVector *supers = WeakVector::Cast(GetSupers().GetTaggedObject());
    return !(supers->Empty());
}

bool JSHClass::IsTSIHCWithInheritInfo() const
{
    return IsTS() && !IsPrototype() && HasTSSubtyping();
}

PropertyLookupResult JSHClass::LookupPropertyInAotHClass(const JSThread *thread, JSHClass *hclass, JSTaggedValue key)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(hclass->IsTS());

    PropertyLookupResult result;
    if (hclass->IsDictionaryMode()) {
        // not fuond
        result.SetIsFound(false);
        return result;
    }

    int entry = JSHClass::FindPropertyEntry(thread, hclass, key);
    // found in local
    if (entry != -1) {
        result.SetIsFound(true);
        result.SetIsLocal(true);
        PropertyAttributes attr = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject())->GetAttr(entry);
        if (attr.IsInlinedProps()) {
            result.SetIsInlinedProps(true);
            result.SetOffset(hclass->GetInlinedPropertiesOffset(entry));
        } else {
            result.SetIsInlinedProps(false);
            result.SetOffset(attr.GetOffset() - hclass->GetInlinedProperties());
        }
        if (attr.IsNotHole()) {
            result.SetIsNotHole(true);
        }
        if (attr.IsAccessor()) {
            result.SetIsAccessor(true);
        }
        result.SetRepresentation(attr.GetRepresentation());
        result.SetIsWritable(attr.IsWritable());
        return result;
    }

    // found in vtable
    if (hclass->GetVTable().IsUndefined()) {
        result.SetIsFound(false);
        return result;
    }
    JSHandle<VTable> vtable(thread, hclass->GetVTable());
    entry = vtable->GetTupleIndexByName(key);
    if (entry != -1) {
        result.SetIsVtable();
        uint32_t offset = static_cast<uint32_t>(entry * VTable::TUPLE_SIZE);
        result.SetOffset(offset);
        if (vtable->IsAccessor(entry)) {
            result.SetIsAccessor(true);
        }
        return result;
    }

    // not found
    result.SetIsFound(false);
    return result;
}

PropertyLookupResult JSHClass::LookupPropertyInPGOHClass(const JSThread *thread, JSHClass *hclass, JSTaggedValue key)
{
    DISALLOW_GARBAGE_COLLECTION;

    PropertyLookupResult result;
    if (hclass->IsDictionaryMode()) {
        result.SetIsFound(false);
        return result;
    }

    int entry = JSHClass::FindPropertyEntry(thread, hclass, key);
    // found in local
    if (entry != -1) {
        result.SetIsFound(true);
        result.SetIsLocal(true);
        PropertyAttributes attr = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject())->GetAttr(entry);
        if (attr.IsInlinedProps()) {
            result.SetIsInlinedProps(true);
            result.SetOffset(hclass->GetInlinedPropertiesOffset(entry));
        } else {
            result.SetIsInlinedProps(false);
            result.SetOffset(attr.GetOffset() - hclass->GetInlinedProperties());
        }

        if (attr.IsNotHole()) {
            result.SetIsNotHole(true);
        }
        if (attr.IsAccessor()) {
            result.SetIsAccessor(true);
        }
        result.SetRepresentation(attr.GetRepresentation());
        result.SetIsWritable(attr.IsWritable());
        return result;
    }

    // not fuond
    result.SetIsFound(false);
    return result;
}

PropertyLookupResult JSHClass::LookupPropertyInBuiltinPrototypeHClass(const JSThread *thread, JSHClass *hclass,
                                                                      JSTaggedValue key)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(hclass->IsPrototype());

    PropertyLookupResult result;
    if (hclass->IsDictionaryMode()) {
        // not fuond
        result.SetIsFound(false);
        return result;
    }
    int entry = JSHClass::FindPropertyEntry(thread, hclass, key);
    // When the property is not found, the value of 'entry' is -1.
    // Currently, not all methods on the prototype of 'builtin' have been changed to inlined.
    // Therefore, when a non-inlined method is encountered, it is also considered not found.
    if (entry == -1 || static_cast<uint32_t>(entry) >= hclass->GetInlinedProperties()) {
        result.SetIsFound(false);
        return result;
    }

    result.SetIsFound(true);
    result.SetIsLocal(true);
    PropertyAttributes attr = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject())->GetAttr(entry);
    if (attr.IsInlinedProps()) {
        result.SetIsInlinedProps(true);
        result.SetOffset(hclass->GetInlinedPropertiesOffset(entry));
    } else {
        result.SetIsInlinedProps(false);
        result.SetOffset(attr.GetOffset() - hclass->GetInlinedProperties());
    }
    result.SetIsNotHole(true);
    if (attr.IsAccessor()) {
        result.SetIsAccessor(true);
    }
    result.SetRepresentation(attr.GetRepresentation());
    result.SetIsWritable(attr.IsWritable());
    return result;
}

void JSHClass::CopyTSInheritInfo(const JSThread *thread, const JSHandle<JSHClass> &oldHClass,
                                 JSHandle<JSHClass> &newHClass)
{
    JSHandle<WeakVector> supers(thread, oldHClass->GetSupers());
    JSHandle<WeakVector> copySupers = WeakVector::Copy(thread, supers);
    newHClass->SetSupers(thread, copySupers);

    uint8_t level = oldHClass->GetLevel();
    newHClass->SetLevel(level);

    JSHandle<VTable> vtable(thread, oldHClass->GetVTable());
    JSHandle<VTable> copyVtable = VTable::Copy(thread, vtable);
    newHClass->SetVTable(thread, copyVtable);
}

JSHandle<JSTaggedValue> JSHClass::ParseKeyFromPGOCString(ObjectFactory* factory,
                                                         const CString& cstring,
                                                         const PGOHandler& handler)
{
    if (handler.GetIsSymbol()) {
        JSHandle<JSSymbol> symbol;
        auto str = cstring.substr(0, 6); // `method` length is 6
        if (str == "method") { // cstring is `method_0ULL` after _ is private id of symbol
            symbol = factory->NewPublicSymbolWithChar("method");
            ASSERT(cstring.size() > 0);
            str = cstring.substr(7, cstring.size() - 1); // `method_` length is 7
            symbol->SetPrivateId(CStringToULL(str));
        } else { // cstring is private id of symbol
            symbol = factory->NewJSSymbol();
            symbol->SetPrivateId(CStringToULL(cstring));
        }
        return JSHandle<JSTaggedValue>(symbol);
    } else {
        return JSHandle<JSTaggedValue>(factory->NewFromStdString(std::string(cstring)));
    }
}

JSHandle<JSHClass> JSHClass::CreateRootHClassFromPGO(const JSThread* thread,
                                                     const HClassLayoutDesc* desc,
                                                     uint32_t maxNum)
{
    auto rootDesc = reinterpret_cast<const pgo::RootHClassLayoutDesc *>(desc);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t numOfProps = rootDesc->NumOfProps();
    uint32_t index = 0;
    JSType type = rootDesc->GetObjectType();
    size_t size = rootDesc->GetObjectSize();
    JSHandle<JSHClass> hclass = factory->NewEcmaHClass(size, type, maxNum);
    // Dictionary?
    JSHandle<LayoutInfo> layout = factory->CreateLayoutInfo(maxNum, MemSpaceType::SEMI_SPACE, GrowMode::KEEP);
    rootDesc->IterateProps([thread, factory, &index, hclass, layout](const pgo::PropertyDesc& propDesc) {
        auto& cstring = propDesc.first;
        auto& handler = propDesc.second;
        JSHandle<JSTaggedValue> key = ParseKeyFromPGOCString(factory, cstring, handler);
        PropertyAttributes attributes = PropertyAttributes::Default();
        if (handler.SetAttribute(thread, attributes)) {
            hclass->SetIsAllTaggedProp(false);
        }
        attributes.SetIsInlinedProps(true);
        attributes.SetOffset(index);
        layout->AddKey(thread, index, key.GetTaggedValue(), attributes);
        index++;
    });
    hclass->SetLayout(thread, layout);
    hclass->SetNumberOfProps(numOfProps);
    hclass->SetTS(true);
    return hclass;
}

JSHandle<JSHClass> JSHClass::CreateChildHClassFromPGO(const JSThread* thread,
                                                      const JSHandle<JSHClass>& parent,
                                                      const HClassLayoutDesc* desc)
{
    pgo::PropertyDesc propDesc = reinterpret_cast<const pgo::ChildHClassLayoutDesc *>(desc)->GetPropertyDesc();
    auto& cstring = propDesc.first;
    auto& handler = propDesc.second;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t numOfProps = parent->NumberOfProps();

    JSHandle<JSHClass> newJsHClass = JSHClass::Clone(thread, parent);
    newJsHClass->SetTS(true);
    ASSERT(newJsHClass->GetInlinedProperties() >= (numOfProps + 1));
    uint32_t offset = numOfProps;
    {
        JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, newJsHClass->GetLayout());
        if (layoutInfoHandle->NumberOfElements() != static_cast<int>(offset)) {
            layoutInfoHandle.Update(factory->CopyAndReSort(layoutInfoHandle, offset, offset + 1));
            newJsHClass->SetLayout(thread, layoutInfoHandle);
        } else if (layoutInfoHandle->GetPropertiesCapacity() <= static_cast<int>(offset)) {  // need to Grow
            layoutInfoHandle.Update(
                factory->ExtendLayoutInfo(layoutInfoHandle, offset));
            newJsHClass->SetLayout(thread, layoutInfoHandle);
        }
        JSHandle<JSTaggedValue> key = ParseKeyFromPGOCString(factory, cstring, handler);
        PropertyAttributes attributes = PropertyAttributes::Default();
        if (handler.SetAttribute(thread, attributes)) {
            newJsHClass->SetIsAllTaggedProp(false);
        }
        attributes.SetOffset(offset);
        attributes.SetIsInlinedProps(true);
        layoutInfoHandle->AddKey(thread, offset, key.GetTaggedValue(), attributes);
        newJsHClass->IncNumberOfProps();
        AddTransitions(thread, parent, newJsHClass, key, attributes);
        JSHClass::NotifyHclassChanged(thread, parent, newJsHClass, key.GetTaggedValue());
    }

    return newJsHClass;
}

bool JSHClass::DumpRootHClassByPGO(const JSHClass* hclass, HClassLayoutDesc* desc)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (hclass->IsDictionaryMode()) {
        return false;
    }

    LayoutInfo *layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    int element = static_cast<int>(hclass->NumberOfProps());
    for (int i = 0; i < element; i++) {
        layout->DumpFieldIndexByPGO(i, desc);
    }
    return true;
}

bool JSHClass::DumpChildHClassByPGO(const JSHClass* hclass, HClassLayoutDesc* desc)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (hclass->IsDictionaryMode()) {
        return false;
    }
    if (hclass->PropsIsEmpty()) {
        return false;
    }
    uint32_t last = hclass->LastPropIndex();
    LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    layoutInfo->DumpFieldIndexByPGO(last, desc);
    return true;
}

bool JSHClass::UpdateChildLayoutDescByPGO(const JSHClass* hclass, HClassLayoutDesc* childDesc)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (hclass->IsDictionaryMode()) {
        return false;
    }
    if (hclass->PropsIsEmpty()) {
        return false;
    }
    uint32_t last = hclass->LastPropIndex();
    LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    return layoutInfo->UpdateFieldIndexByPGO(last, childDesc);
}

bool JSHClass::UpdateRootLayoutDescByPGO(const JSHClass* hclass,
                                         const PGOHClassTreeDesc* treeDesc,
                                         HClassLayoutDesc* desc)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (hclass->IsDictionaryMode()) {
        return false;
    }

    auto rootDesc = reinterpret_cast<const pgo::RootHClassLayoutDesc *>(desc);
    int rootPropLen = static_cast<int>(rootDesc->NumOfProps());
    int element = static_cast<int>(hclass->NumberOfProps());
    ASSERT(element >= rootPropLen);
    LayoutInfo *layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    for (int i = 0; i < rootPropLen; i++) {
        layout->UpdateFieldIndexByPGO(i, desc);
    }
    auto lastDesc = desc;
    for (int i = rootPropLen; i < element - 1; i++) {
        if (lastDesc == nullptr || lastDesc->GetChildSize() == 0) {
            break;
        }
        lastDesc->IterateChilds([treeDesc, layout, i, &lastDesc] (const ProfileType &childType) -> bool {
            lastDesc = treeDesc->GetHClassLayoutDesc(childType);
            if (lastDesc == nullptr) {
                return true;
            }
            return !layout->UpdateFieldIndexByPGO(i, lastDesc);
        });
    }
    return true;
}

CString JSHClass::DumpToString(JSTaggedType hclassVal)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto hclass = JSHClass::Cast(JSTaggedValue(hclassVal).GetTaggedObject());
    if (hclass->IsDictionaryMode()) {
        return "";
    }

    CString result;
    LayoutInfo *layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    int element = static_cast<int>(hclass->NumberOfProps());
    for (int i = 0; i < element; i++) {
        auto key = layout->GetKey(i);
        if (key.IsString()) {
            result += EcmaStringAccessor(key).ToCString();
            auto attr = layout->GetAttr(i);
            result += static_cast<int32_t>(attr.GetTrackType());
            result += attr.GetPropertyMetaData();
        } else if (key.IsSymbol()) {
            result += JSSymbol::Cast(key)->GetPrivateId();
            auto attr = layout->GetAttr(i);
            result += static_cast<int32_t>(attr.GetTrackType());
            result += attr.GetPropertyMetaData();
        } else {
            LOG_ECMA(FATAL) << "JSHClass::DumpToString UNREACHABLE";
        }
    }
    return result;
}

PropertyLookupResult JSHClass::LookupPropertyInBuiltinHClass(const JSThread *thread, JSHClass *hclass,
                                                             JSTaggedValue key)
{
    DISALLOW_GARBAGE_COLLECTION;

    PropertyLookupResult result;
    if (hclass->IsDictionaryMode()) {
        result.SetIsFound(false);
        return result;
    }

    int entry = JSHClass::FindPropertyEntry(thread, hclass, key);
    // found in local
    if (entry != -1) {
        result.SetIsFound(true);
        result.SetIsLocal(true);
        PropertyAttributes attr = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject())->GetAttr(entry);
        if (attr.IsInlinedProps()) {
            result.SetIsInlinedProps(true);
            result.SetOffset(hclass->GetInlinedPropertiesOffset(entry));
        } else {
            result.SetIsInlinedProps(false);
            result.SetOffset(attr.GetOffset() - hclass->GetInlinedProperties());
        }

        if (attr.IsNotHole()) {
            result.SetIsNotHole(true);
        }
        if (attr.IsAccessor()) {
            result.SetIsAccessor(true);
        }
        result.SetRepresentation(attr.GetRepresentation());
        result.SetIsWritable(attr.IsWritable());
        return result;
    }

    // not fuond
    result.SetIsFound(false);
    return result;
}

JSHandle<JSHClass> JSHClass::CreateSHClass(JSThread *thread,
                                           const std::vector<PropertyDescriptor> &descs,
                                           const JSHClass *parentHClass,
                                           bool isFunction)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    uint32_t length = descs.size();
    uint32_t maxInline = isFunction ? JSSharedFunction::MAX_INLINE : JSSharedObject::MAX_INLINE;

    if (parentHClass) {
        if (parentHClass->IsDictionaryMode()) {
            auto dict = reinterpret_cast<NameDictionary *>(parentHClass->GetLayout().GetTaggedObject());
            length += static_cast<uint32_t>(dict->EntriesCount());
        } else {
            length += parentHClass->NumberOfProps();
        }
    }

    JSHandle<JSHClass> hclass =
        isFunction ? factory->NewSEcmaHClass(JSSharedFunction::SIZE, JSType::JS_SHARED_FUNCTION, length)
                   : factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, length);
    if (LIKELY(length <= maxInline)) {
        CreateSInlinedLayout(thread, descs, hclass, parentHClass);
    } else {
        CreateSDictLayout(thread, descs, hclass, parentHClass);
    }

    return hclass;
}

JSHandle<JSHClass> JSHClass::CreateSConstructorHClass(JSThread *thread, const std::vector<PropertyDescriptor> &descs)
{
    auto hclass = CreateSHClass(thread, descs, nullptr, true);
    hclass->SetClassConstructor(true);
    hclass->SetConstructor(true);
    return hclass;
}

JSHandle<JSHClass> JSHClass::CreateSPrototypeHClass(JSThread *thread, const std::vector<PropertyDescriptor> &descs)
{
    auto hclass = CreateSHClass(thread, descs);
    hclass->SetClassPrototype(true);
    hclass->SetIsPrototype(true);
    return hclass;
}

void JSHClass::CreateSInlinedLayout(JSThread *thread,
                                    const std::vector<PropertyDescriptor> &descs,
                                    const JSHandle<JSHClass> &hclass,
                                    const JSHClass *parentHClass)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    uint32_t parentLength{0};
    if (parentHClass) {
        parentLength = parentHClass->NumberOfProps();
    }
    auto length = descs.size();
    auto layout = factory->CreateSLayoutInfo(length + parentLength);

    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < length; ++i) {
        key.Update(descs[i].GetKey());
        PropertyAttributes attr =
            PropertyAttributes::Default(descs[i].IsWritable(), descs[i].IsEnumerable(), descs[i].IsConfigurable());
        if (UNLIKELY(descs[i].GetValue()->IsAccessor())) {
            attr.SetIsAccessor(true);
        }
        attr.SetIsInlinedProps(true);
        attr.SetRepresentation(Representation::TAGGED);
        attr.SetSharedFieldType(descs[i].GetSharedFieldType());
        attr.SetOffset(i);
        layout->AddKey(thread, i, key.GetTaggedValue(), attr);
    }

    auto index = length;
    if (parentHClass) {
        JSHandle<LayoutInfo> old(thread, parentHClass->GetLayout());
        for (uint32_t i = 0; i < parentLength; i++) {
            key.Update(old->GetKey(i));
            auto entry = layout->FindElementWithCache(thread, *hclass, key.GetTaggedValue(), index);
            if (entry != -1) {
                continue;
            }
            auto attr = PropertyAttributes(old->GetAttr(i));
            attr.SetOffset(index);
            layout->AddKey(thread, index, old->GetKey(i), attr);
            ++index;
        }
    }

    hclass->SetLayout(thread, layout);
    hclass->SetNumberOfProps(index);
    auto inlinedPropsLength = hclass->GetInlinedProperties();
    if (inlinedPropsLength > index) {
        uint32_t duplicatedSize = (inlinedPropsLength - index) * JSTaggedValue::TaggedTypeSize();
        hclass->SetObjectSize(hclass->GetObjectSize() - duplicatedSize);
    }
}

void JSHClass::CreateSDictLayout(JSThread *thread,
                                 const std::vector<PropertyDescriptor> &descs,
                                 const JSHandle<JSHClass> &hclass,
                                 const JSHClass *parentHClass)
{
    uint32_t parentLength{0};
    if (parentHClass) {
        if (parentHClass->IsDictionaryMode()) {
            parentLength = static_cast<uint32_t>(
                reinterpret_cast<NameDictionary *>(parentHClass->GetLayout().GetTaggedObject())->EntriesCount());
        } else {
            parentLength = parentHClass->NumberOfProps();
        }
    }
    auto length = descs.size();
    JSMutableHandle<NameDictionary> dict(
        thread,
        NameDictionary::CreateInSharedHeap(thread, NameDictionary::ComputeHashTableSize(length + parentLength)));
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    JSHandle<JSTaggedValue> value = globalConst->GetHandledUndefined();

    for (uint32_t i = 0; i < length; ++i) {
        key.Update(descs[i].GetKey());
        PropertyAttributes attr =
            PropertyAttributes::Default(descs[i].IsWritable(), descs[i].IsEnumerable(), descs[i].IsConfigurable());
        attr.SetSharedFieldType(descs[i].GetSharedFieldType());
        attr.SetBoxType(PropertyBoxType::UNDEFINED);
        JSHandle<NameDictionary> newDict = NameDictionary::Put(thread, dict, key, value, attr);
        dict.Update(newDict);
    }

    if (parentHClass) {
        if (parentHClass->IsDictionaryMode()) {
            JSHandle<NameDictionary> old(thread, parentHClass->GetLayout());
            std::vector<int> indexOrder = old->GetEnumerationOrder();
            for (uint32_t i = 0; i < parentLength; i++) {
                key.Update(old->GetKey(indexOrder[i]));
                JSHandle<NameDictionary> newDict = NameDictionary::Put(
                    thread, dict, key, value, PropertyAttributes(old->GetAttributes(indexOrder[i])));
                dict.Update(newDict);
            }
        } else {
            JSHandle<LayoutInfo> old(thread, parentHClass->GetLayout());
            for (uint32_t i = 0; i < parentLength; i++) {
                key.Update(old->GetKey(i));
                JSHandle<NameDictionary> newDict =
                    NameDictionary::Put(thread, dict, key, value, PropertyAttributes(old->GetAttr(i)));
                dict.Update(newDict);
            }
        }
    }

    hclass->SetLayout(thread, dict);
    hclass->SetNumberOfProps(0);
    hclass->SetIsDictionaryMode(true);
}

}  // namespace panda::ecmascript
