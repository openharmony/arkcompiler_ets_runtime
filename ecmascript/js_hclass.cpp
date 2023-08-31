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
#include "ecmascript/tagged_array.h"
#include "ecmascript/vtable.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/subtyping_operator.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/weak_vector.h"

namespace panda::ecmascript {
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

// class JSHClass
void JSHClass::Initialize(const JSThread *thread, uint32_t size, JSType type, uint32_t inlinedProps,
                          bool isOptimized, bool canFastCall)
{
    DISALLOW_GARBAGE_COLLECTION;
    ClearBitField();
    if (JSType::JS_OBJECT_FIRST <= type && type <= JSType::JS_OBJECT_LAST) {
        SetObjectSize(size + inlinedProps * JSTaggedValue::TaggedTypeSize());
        SetInlinedPropsStart(size);
        SetLayout(thread, thread->GlobalConstants()->GetEmptyLayoutInfo());
    } else {
        SetObjectSize(size);
        SetLayout(thread, JSTaggedValue::Null());
    }
    if (type >= JSType::JS_FUNCTION_FIRST && type <= JSType::JS_FUNCTION_LAST) {
        SetIsJSFunction(true);
        SetIsOptimized(isOptimized);
        SetCanFastCall(canFastCall);
    }
    SetPrototype(thread, JSTaggedValue::Null());

    SetObjectType(type);
    SetExtensible(true);
    SetIsPrototype(false);
    SetHasDeleteProperty(false);
    SetIsAllTaggedProp(true);
    SetElementsKind(ElementsKind::GENERIC);
    SetTransitions(thread, JSTaggedValue::Undefined());
    SetProtoChangeMarker(thread, JSTaggedValue::Null());
    SetProtoChangeDetails(thread, JSTaggedValue::Null());
    SetEnumCache(thread, JSTaggedValue::Null());
    InitTSInheritInfo(thread);
}

void JSHClass::InitTSInheritInfo(const JSThread *thread)
{
    // Supers and Level are used to record the relationship between TSHClass.
    if (IsECMAObject()) {
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
                                   bool withoutInlinedProperties)
{
    JSType type = jshclass->GetObjectType();
    uint32_t size = jshclass->GetInlinedPropsStartSize();
    uint32_t numInlinedProps = withoutInlinedProperties ? 0 : jshclass->GetInlinedProperties();
    JSHandle<JSHClass> newJsHClass = thread->GetEcmaVM()->GetFactory()->NewEcmaHClass(size, type, numInlinedProps);
    // Copy all
    newJsHClass->Copy(thread, *jshclass);
    newJsHClass->SetTransitions(thread, JSTaggedValue::Undefined());
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
    }
    obj->GetJSHClass()->SetIsDictionaryElement(true);
    obj->GetJSHClass()->SetIsStableElements(false);
    obj->GetJSHClass()->SetElementsKind(ElementsKind::GENERIC);
}

JSHandle<JSHClass> JSHClass::SetPropertyOfObjHClass(const JSThread *thread, JSHandle<JSHClass> &jshclass,
                                                    const JSHandle<JSTaggedValue> &key,
                                                    const PropertyAttributes &attr)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHClass *newClass = jshclass->FindTransitions(key.GetTaggedValue(), JSTaggedValue(attr.GetPropertyMetaData()));
    if (newClass != nullptr) {
        return JSHandle<JSHClass>(thread, newClass);
    }

    JSHandle<JSHClass> newJsHClass = JSHClass::Clone(thread, jshclass);
    newJsHClass->IncNumberOfProps();
    uint32_t offset = attr.GetOffset();
    {
        JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, newJsHClass->GetLayout());
        if (layoutInfoHandle->NumberOfElements() != static_cast<int>(offset)) {
            layoutInfoHandle.Update(factory->CopyAndReSort(layoutInfoHandle, offset, offset + 1));
        } else if (layoutInfoHandle->GetPropertiesCapacity() <= static_cast<int>(offset)) { // need to Grow
            layoutInfoHandle.Update(
                factory->ExtendLayoutInfo(layoutInfoHandle, offset));
        }
        newJsHClass->SetLayout(thread, layoutInfoHandle);
        layoutInfoHandle->AddKey(thread, offset, key.GetTaggedValue(), attr);
    }

    AddTransitions(thread, jshclass, newJsHClass, key, attr);
    return newJsHClass;
}

void JSHClass::AddProperty(const JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                           const PropertyAttributes &attr)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> jshclass(thread, obj->GetJSHClass());
    JSHClass *newClass = jshclass->FindTransitions(key.GetTaggedValue(), JSTaggedValue(attr.GetPropertyMetaData()));
    if (newClass != nullptr) {
        obj->SynchronizedSetClass(newClass);
#if ECMASCRIPT_ENABLE_IC
        JSHClass::NotifyHclassChanged(thread, jshclass, JSHandle<JSHClass>(thread, newClass), key.GetTaggedValue());
#endif
        return;
    }

    // 2. Create hclass
    JSHandle<JSHClass> newJsHClass = JSHClass::Clone(thread, jshclass);

    // 3. Add Property and metaData
    uint32_t offset = attr.GetOffset();
    newJsHClass->IncNumberOfProps();

    {
        JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, newJsHClass->GetLayout());

        if (layoutInfoHandle->NumberOfElements() != static_cast<int>(offset)) {
            layoutInfoHandle.Update(factory->CopyAndReSort(layoutInfoHandle, offset, offset + 1));
        } else if (layoutInfoHandle->GetPropertiesCapacity() <= static_cast<int>(offset)) {  // need to Grow
            layoutInfoHandle.Update(
                factory->ExtendLayoutInfo(layoutInfoHandle, offset));
        }
        newJsHClass->SetLayout(thread, layoutInfoHandle);
        layoutInfoHandle->AddKey(thread, offset, key.GetTaggedValue(), attr);
    }

    // 4. Add newClass to old hclass's transitions.
    AddTransitions(thread, jshclass, newJsHClass, key, attr);

    // 5. update hclass in object.
#if ECMASCRIPT_ENABLE_IC
    JSHClass::NotifyHclassChanged(thread, jshclass, newJsHClass, key.GetTaggedValue());
#endif
    obj->SynchronizedSetClass(*newJsHClass);

    // Maintaining subtyping is no longer required when transition succeeds.
    if (jshclass->HasTSSubtyping()) {
        SubtypingOperator::TryMaintainTSSubtyping(thread, jshclass, newJsHClass, key);
    }
}

JSHandle<JSHClass> JSHClass::TransitionExtension(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledPreventExtensionsString());
    {
        auto *newClass = jshclass->FindTransitions(key.GetTaggedValue(), JSTaggedValue(0));
        if (newClass != nullptr) {
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

    // 2. new a hclass
    JSHandle<JSHClass> newJsHClass = JSHClass::Clone(thread, jshclass);
    newJsHClass->SetPrototype(thread, proto.GetTaggedValue());

    // 3. Add newJsHClass to old jshclass's parent's transitions.
    AddProtoTransitions(thread, jshclass, newJsHClass, key, proto);

    // parent is the same as jshclass, already copy
    return newJsHClass;
}

void JSHClass::SetPrototype(const JSThread *thread, JSTaggedValue proto)
{
    JSHandle<JSTaggedValue> protoHandle(thread, proto);
    SetPrototype(thread, protoHandle);
}

void JSHClass::SetPrototype(const JSThread *thread, const JSHandle<JSTaggedValue> &proto)
{
    // In the original version, whether the objcet is EcmaObject is determined,
    // but proxy is not allowd.
    if (proto->IsJSObject()) {
        ShouldUpdateProtoClass(thread, proto);
    }
    SetProto(thread, proto);
}

void JSHClass::ShouldUpdateProtoClass(const JSThread *thread, const JSHandle<JSTaggedValue> &proto)
{
    JSHandle<JSHClass> hclass(thread, proto->GetTaggedObject()->GetClass());
    ASSERT(!Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(*hclass))->InReadOnlySpace());
    if (!hclass->IsPrototype()) {
        // If the objcet should be changed to the proto of an object,
        // the original hclass cannot be shared.
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
        JSObject::Cast(proto->GetTaggedObject())->SynchronizedSetClass(*newProtoClass);
        newProtoClass->SetIsPrototype(true);
    }
}

void JSHClass::TransitionToDictionary(const JSThread *thread, const JSHandle<JSObject> &obj)
{
    // 1. new a hclass
    JSHandle<JSHClass> jshclass(thread, obj->GetJSHClass());
    JSHandle<JSHClass> newJsHClass = CloneWithoutInlinedProperties(thread, jshclass);

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
        obj->SynchronizedSetClass(*newJsHClass);
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

    receiver->SynchronizedSetClass(*newHClass);
    // 4. Maybe Transition And Maintain subtypeing check
}

void JSHClass::TransitToElementsKind(const JSThread *thread, const JSHandle<JSArray> &array)
{
    JSTaggedValue elements = array->GetElements();
    if (!elements.IsTaggedArray()) {
        return;
    }
    ElementsKind newKind = ElementsKind::NONE;
    auto elementArray = TaggedArray::Cast(elements);
    uint32_t length = elementArray->GetLength();
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedValue value = elementArray->Get(i);
        newKind = Elements::ToElementsKind(value, newKind);
    }
    ElementsKind current = array->GetJSHClass()->GetElementsKind();
    if (newKind == current) {
        return;
    }
    auto arrayHClassIndexMap = thread->GetArrayHClassIndexMap();
    if (arrayHClassIndexMap.find(newKind) != arrayHClassIndexMap.end()) {
        auto index = static_cast<size_t>(thread->GetArrayHClassIndexMap().at(newKind));
        auto hclassVal = thread->GlobalConstants()->GetGlobalConstantObject(index);
        JSHClass *hclass = JSHClass::Cast(hclassVal.GetTaggedObject());
        array->SetClass(hclass);
    }
}

void JSHClass::TransitToElementsKind(
    const JSThread *thread, const JSHandle<JSObject> &object, const JSHandle<JSTaggedValue> &value, ElementsKind kind)
{
    if (!object->IsJSArray()) {
        return;
    }
    ElementsKind current = object->GetJSHClass()->GetElementsKind();
    if (Elements::IsGeneric(current)) {
        return;
    }
    auto newKind = Elements::ToElementsKind(value.GetTaggedValue(), kind);
    // Merge current kind and new kind
    newKind = Elements::MergeElementsKind(current, newKind);
    if (newKind == current) {
        return;
    }
    auto arrayHClassIndexMap = thread->GetArrayHClassIndexMap();
    if (arrayHClassIndexMap.find(newKind) != arrayHClassIndexMap.end()) {
        auto index = static_cast<size_t>(thread->GetArrayHClassIndexMap().at(newKind));
        auto hclassVal = thread->GlobalConstants()->GetGlobalConstantObject(index);
        JSHClass *hclass = JSHClass::Cast(hclassVal.GetTaggedObject());
        object->SetClass(hclass);
    }
}

JSHandle<JSTaggedValue> JSHClass::EnableProtoChangeMarker(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    JSTaggedValue proto = jshclass->GetPrototype();
    if (!proto.IsECMAObject()) {
        // Return JSTaggedValue directly. No proto check is needed.
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
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
    newHclass->SetIsPrototype(true);
    JSHClass::NoticeThroughChain(thread, oldHclass, addedKey);
    JSHClass::RefreshUsers(thread, oldHclass, newHclass);
}

void JSHClass::RegisterOnProtoChain(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
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

void JSHClass::MarkProtoChanged(const JSThread *thread, const JSHandle<JSHClass> &jshclass,
                                JSTaggedValue addedKey)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(jshclass->IsPrototype() || jshclass->HasTSSubtyping());
    JSTaggedValue markerValue = jshclass->GetProtoChangeMarker();
    if (markerValue.IsProtoChangeMarker()) {
        ProtoChangeMarker *protoChangeMarker = ProtoChangeMarker::Cast(markerValue.GetTaggedObject());
        protoChangeMarker->SetHasChanged(true);
    }

    if (jshclass->HasTSSubtyping()) {
        if (addedKey.IsString()) {
            JSHandle<JSTaggedValue> key(thread, addedKey);
            if (SubtypingOperator::TryMaintainTSSubtypingOnPrototype(thread, jshclass, key)) {
                return;
            }
        }
        jshclass->InitTSInheritInfo(thread);
    }
}

void JSHClass::NoticeThroughChain(const JSThread *thread, const JSHandle<JSHClass> &jshclass,
                                  JSTaggedValue addedKey)
{
    DISALLOW_GARBAGE_COLLECTION;
    MarkProtoChanged(thread, jshclass, addedKey);
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
    int entry = JSHClass::FindPropertyEntry(thread, hclass, key);
    // found in local
    if (entry != -1) {
        result.SetIsFound(true);
        result.SetIsLocal(true);
        uint32_t offset = hclass->GetInlinedPropertiesOffset(entry);
        result.SetOffset(offset);
        PropertyAttributes attr = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject())->GetAttr(entry);
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
    uint32_t offset = hclass->GetInlinedPropertiesOffset(entry);
    result.SetOffset(offset);
    PropertyAttributes attr = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject())->GetAttr(entry);
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

bool JSHClass::DumpForProfile(const JSHClass *hclass, PGOHClassLayoutDesc &desc, PGOObjKind kind)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (hclass->IsDictionaryMode()) {
        return false;
    }
    if (kind == PGOObjKind::ELEMENT) {
        desc.UpdateElementKind(hclass->GetElementsKind());
    }

    LayoutInfo *layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    int element = static_cast<int>(hclass->NumberOfProps());
    for (int i = 0; i < element; i++) {
        layout->DumpFieldIndexForProfile(i, desc, kind);
    }
    return true;
}
}  // namespace panda::ecmascript
