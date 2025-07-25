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

#ifndef ECMASCRIPT_JS_HCLASS_INL_H
#define ECMASCRIPT_JS_HCLASS_INL_H

#include "ecmascript/js_hclass.h"

#include "ecmascript/byte_array.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/layout_info-inl.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/transitions_dictionary.h"

namespace panda::ecmascript {

void JSHClass::AddTransitions(const JSThread *thread, const JSHandle<JSHClass> &parent, const JSHandle<JSHClass> &child,
                              const JSHandle<JSTaggedValue> &key, PropertyAttributes attributes)
{
    UpdateRootHClass(thread, parent, child);
    JSTaggedValue transitions = parent->GetTransitions(thread);
    if (transitions.IsUndefined()) {
        NotifyLeafHClassChanged(const_cast<JSThread *>(thread), parent);
        JSTaggedValue weakChild = JSTaggedValue(child.GetTaggedValue().CreateAndGetWeakRef());
        parent->SetTransitions(thread, weakChild);
        return;
    }
    ASSERT(!parent->IsStable());
    JSMutableHandle<TransitionsDictionary> dict(thread, JSTaggedValue::Undefined());
    if (transitions.IsWeak()) {
        auto cachedHClass = JSHClass::Cast(transitions.GetTaggedWeakRef());
        if (cachedHClass->HasProps()) {
            uint32_t last = cachedHClass->LastPropIndex();
            LayoutInfo* layoutInfo = LayoutInfo::Cast(cachedHClass->GetLayout(thread).GetTaggedObject());
            auto metaData = JSHandle<JSTaggedValue>(thread,
                JSTaggedValue(layoutInfo->GetAttr(thread, last).GetPropertyMetaData()));
            auto lastKey = JSHandle<JSTaggedValue>(thread, layoutInfo->GetKey(thread, last));
            auto lastHClass = JSHandle<JSTaggedValue>(thread, cachedHClass);
            dict.Update(TransitionsDictionary::Create(thread));
            transitions = TransitionsDictionary::PutIfAbsent(thread, dict, lastKey, lastHClass,
                metaData).GetTaggedValue();
        }
    }
    auto metaData = JSHandle<JSTaggedValue>(thread, JSTaggedValue(attributes.GetPropertyMetaData()));
    dict.Update(transitions);
    transitions = TransitionsDictionary::PutIfAbsent(thread, dict, key, JSHandle<JSTaggedValue>(child),
        metaData).GetTaggedValue();
    parent->SetTransitions(thread, transitions);
}

void JSHClass::AddExtensionTransitions(const JSThread *thread, const JSHandle<JSHClass> &parent,
                                       const JSHandle<JSHClass> &child, const JSHandle<JSTaggedValue> &key)
{
    auto attr = JSHandle<JSTaggedValue>(thread, PropertyAttributes(0).GetTaggedValue());
    AddProtoTransitions(thread, parent, child, key, attr);
}

void JSHClass::AddProtoTransitions(const JSThread *thread, const JSHandle<JSHClass> &parent,
                                   const JSHandle<JSHClass> &child, const JSHandle<JSTaggedValue> &key,
                                   const JSHandle<JSTaggedValue> &proto)
{
    ALLOW_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
    UpdateRootHClass(thread, parent, child);
    JSTaggedValue transitions = parent->GetTransitions(thread);
    JSMutableHandle<TransitionsDictionary> dict(thread, JSTaggedValue::Undefined());
    if (transitions.IsUndefined()) {
        NotifyLeafHClassChanged(const_cast<JSThread *>(thread), parent);
        transitions = TransitionsDictionary::Create(thread).GetTaggedValue();
    } else if (transitions.IsWeak()) {
        auto cachedHClass = JSHClass::Cast(transitions.GetTaggedWeakRef());
        if (cachedHClass->HasProps()) {
            uint32_t last = cachedHClass->LastPropIndex();
            LayoutInfo* layoutInfo = LayoutInfo::Cast(cachedHClass->GetLayout(thread).GetTaggedObject());
            auto metaData = JSHandle<JSTaggedValue>(thread,
                JSTaggedValue(layoutInfo->GetAttr(thread, last).GetPropertyMetaData()));
            auto lastKey = JSHandle<JSTaggedValue>(thread, layoutInfo->GetKey(thread, last));
            auto lastHClass = JSHandle<JSTaggedValue>(thread, cachedHClass);
            dict.Update(TransitionsDictionary::Create(thread));
            transitions = TransitionsDictionary::PutIfAbsent(thread, dict, lastKey, lastHClass,
                metaData).GetTaggedValue();
        }
    }
    ASSERT(!parent->IsStable());
    dict.Update(transitions);
    transitions =
        TransitionsDictionary::PutIfAbsent(thread, dict, key, JSHandle<JSTaggedValue>(child), proto).GetTaggedValue();
    parent->SetTransitions(thread, transitions);
}

inline JSHClass *JSHClass::FindTransitions(const JSThread *thread, const JSTaggedValue &key,
                                           const JSTaggedValue &metaData, const Representation &rep)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue transitions = GetTransitions(thread);
    if (transitions.IsUndefined()) {
        return nullptr;
    }
    if (transitions.IsWeak()) {
        auto cachedHClass = JSHClass::Cast(transitions.GetTaggedWeakRef());
        if (cachedHClass->PropsIsEmpty()) {
            return nullptr;
        }
        int last = static_cast<int>(cachedHClass->LastPropIndex());
        LayoutInfo *layoutInfo = LayoutInfo::Cast(cachedHClass->GetLayout(thread).GetTaggedObject());
        auto lastMetaData = layoutInfo->GetAttr(thread, last).GetPropertyMetaData();
        auto lastKey = layoutInfo->GetKey(thread, last);
        if (lastMetaData == metaData.GetInt() && key == lastKey) {
            return CheckHClassForRep(thread, cachedHClass, rep);
        }
        return nullptr;
    }

    ASSERT(transitions.IsTaggedArray());
    TransitionsDictionary *dict = TransitionsDictionary::Cast(transitions.GetTaggedObject());
    auto entry = dict->FindEntry(thread, key, metaData);
    if (entry == -1) {
        return nullptr;
    }

    JSTaggedValue ret = dict->GetValue(thread, entry);
    if (ret.IsUndefined()) {
        return nullptr;
    }

    return CheckHClassForRep(thread, JSHClass::Cast(ret.GetTaggedWeakRef()), rep);
}

inline JSHClass *JSHClass::FindProtoTransitions(const JSThread *thread, const JSTaggedValue &key,
                                                const JSTaggedValue &proto)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue transitions = GetTransitions(thread);
    if (transitions.IsWeak() || !transitions.IsTaggedArray()) {
        ASSERT(transitions.IsUndefined() || transitions.IsWeak());
        return nullptr;
    }
    ASSERT(transitions.IsTaggedArray());
    TransitionsDictionary *dict = TransitionsDictionary::Cast(transitions.GetTaggedObject());
    auto entry = dict->FindEntry(thread, key, proto);
    if (entry == -1) {
        return nullptr;
    }

    JSTaggedValue ret = dict->GetValue(thread, entry);
    if (ret.IsUndefined()) {
        return nullptr;
    }

    return JSHClass::Cast(ret.GetTaggedWeakRef());
}

inline void JSHClass::RestoreElementsKindToGeneric(JSHClass *newJsHClass)
{
    newJsHClass->SetElementsKind(ElementsKind::GENERIC);
}

inline JSHClass *JSHClass::CheckHClassForRep(const JSThread *thread, JSHClass *hclass, const Representation &rep)
{
    if (!hclass->IsAOT()) {
        return hclass;
    }
    if (rep == Representation::NONE) {
        return hclass;
    }

    int last = static_cast<int>(hclass->LastPropIndex());
    LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout(thread).GetTaggedObject());
    auto lastRep = layoutInfo->GetAttr(thread, last).GetRepresentation();
    auto result = hclass;
    if (lastRep == Representation::INT) {
        if (rep != Representation::INT) {
            result = nullptr;
        }
    } else if (lastRep == Representation::DOUBLE) {
        if (rep != Representation::INT && rep != Representation::DOUBLE) {
            result = nullptr;
        }
    }
    return result;
}

inline void JSHClass::UpdatePropertyMetaData(const JSThread *thread, [[maybe_unused]] const JSTaggedValue &key,
                                             const PropertyAttributes &metaData)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(!GetLayout(thread).IsNull());
    LayoutInfo *layoutInfo = LayoutInfo::Cast(GetLayout(thread).GetTaggedObject());
    ASSERT(layoutInfo->GetLength() != 0);
    uint32_t entry = metaData.GetOffset();

    layoutInfo->SetNormalAttr(thread, entry, metaData);
}

inline bool JSHClass::HasReferenceField()
{
    auto type = GetObjectType();
    switch (type) {
        case JSType::LINE_STRING:
        case JSType::JS_NATIVE_POINTER:
            return false;
        default:
            return true;
    }
}

inline size_t JSHClass::SizeFromJSHClass(TaggedObject *header)
{
    // CAUTION! Never use T::Cast(header) in this function
    // it would cause issue during GC because hclass may forward to a new addres
    // and the casting method would still use the old address.
    auto type = GetObjectType();
    size_t size = 0;
    switch (type) {
        case JSType::TAGGED_ARRAY:
        case JSType::TAGGED_DICTIONARY:
        case JSType::LEXICAL_ENV:
        case JSType::SFUNCTION_ENV:
        case JSType::SENDABLE_ENV:
        case JSType::CONSTANT_POOL:
        case JSType::AOT_LITERAL_INFO:
        case JSType::VTABLE:
        case JSType::COW_TAGGED_ARRAY:
        case JSType::MUTANT_TAGGED_ARRAY:
        case JSType::COW_MUTANT_TAGGED_ARRAY:
        case JSType::PROFILE_TYPE_INFO:
            size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(),
                reinterpret_cast<TaggedArray *>(header)->GetLength());
            break;
        case JSType::BYTE_ARRAY:
            size = ByteArray::ComputeSize(reinterpret_cast<ByteArray *>(header)->GetByteLength(),
                                          reinterpret_cast<ByteArray *>(header)->GetArrayLength());
            size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            break;
        case JSType::LINE_STRING:
            size = LineString::ObjectSize(reinterpret_cast<BaseString* >(header));
            size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            break;
        case JSType::TREE_STRING:
            size = TreeString::SIZE;
            size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            break;
        case JSType::SLICED_STRING:
            size = SlicedString::SIZE;
            size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            break;
        case JSType::MACHINE_CODE_OBJECT:
            size = reinterpret_cast<MachineCode *>(header)->GetMachineCodeObjectSize();
            size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            break;
        case JSType::BIGINT:
            size = BigInt::ComputeSize(reinterpret_cast<BigInt *>(header)->GetLength());
            size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            break;
        default:
            ASSERT(GetObjectSize() != 0);
            size = GetObjectSize();
            break;
    }
    ASSERT(AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)) == size);
    return size;
}

inline void JSHClass::Copy(const JSThread *thread, const JSHClass *jshclass)
{
    DISALLOW_GARBAGE_COLLECTION;

    // copy jshclass
    SetPrototype(thread, jshclass->GetPrototype(thread));
    SetBitField(jshclass->GetBitField());
    SetIsAllTaggedProp(jshclass->IsAllTaggedProp());
    SetNumberOfProps(jshclass->NumberOfProps());
}

inline JSHClass *JSHClass::FindRootHClass(const JSThread *thread, JSHClass *hclass)
{
    auto root = hclass;
    while (!ProfileType(root->GetProfileType()).IsRootType()) {
        auto parent = root->GetParent(thread);
        if (!parent.IsJSHClass()) {
            break;
        }
        root = JSHClass::Cast(parent.GetTaggedObject());
    }
    return root;
}

inline JSTaggedValue JSHClass::FindProtoHClass(const JSThread *thread, JSHClass *hclass)
{
    auto proto = hclass->GetProto(thread);
    if (proto.IsJSObject()) {
        auto prototypeObj = JSObject::Cast(proto);
        return JSTaggedValue(prototypeObj->GetClass());
    }
    return JSTaggedValue::Undefined();
}

inline JSTaggedValue JSHClass::FindProtoRootHClass(const JSThread *thread, JSHClass *hclass)
{
    auto proto = hclass->GetProto(thread);
    if (proto.IsJSObject()) {
        auto prototypeObj = JSObject::Cast(proto);
        auto prototypeHClass = prototypeObj->GetClass();
        return JSTaggedValue(JSHClass::FindRootHClass(thread, prototypeHClass));
    }
    return JSTaggedValue::Undefined();
}

inline void JSHClass::UpdateRootHClass(const JSThread *thread, const JSHandle<JSHClass> &parent,
                                       const JSHandle<JSHClass> &child)
{
    if (thread->GetEcmaVM()->IsEnablePGOProfiler()) {
        child->SetParent(thread, parent);
    }
}

inline int JSHClass::FindPropertyEntry(const JSThread *thread, JSHClass *hclass, JSTaggedValue key)
{
    DISALLOW_GARBAGE_COLLECTION;
    LayoutInfo *layout = LayoutInfo::Cast(hclass->GetLayout(thread).GetTaggedObject());
    uint32_t propsNumber = hclass->NumberOfProps();
    int entry = layout->FindElementWithCache(thread, hclass, key, propsNumber);
    return entry;
}

inline void JSHClass::CompleteObjSizeTracking(const JSThread *thread)
{
    if (!IsObjSizeTrackingInProgress()) {
        return;
    }
    uint32_t finalInObjPropsNum = JSHClass::VisitTransitionAndFindMaxNumOfProps(thread, this);
    if (finalInObjPropsNum < GetInlinedProperties()) {
        // UpdateObjSize with finalInObjPropsNum
        JSHClass::VisitTransitionAndUpdateObjSize(thread, this, finalInObjPropsNum);
    }
    SetConstructionCounter(0); // fini ObjSizeTracking
}

inline void JSHClass::ObjSizeTrackingStep(const JSThread *thread)
{
    if (!IsObjSizeTrackingInProgress()) {
        return;
    }
    uint32_t constructionCounter = GetConstructionCounter();
    ASSERT(constructionCounter != 0);
    SetConstructionCounter(--constructionCounter);
    if (constructionCounter == 0) {
        uint32_t finalInObjPropsNum = JSHClass::VisitTransitionAndFindMaxNumOfProps(thread, this);
        if (finalInObjPropsNum < GetInlinedProperties()) {
            // UpdateObjSize with finalInObjPropsNum
            JSHClass::VisitTransitionAndUpdateObjSize(thread, this, finalInObjPropsNum);
        }
    }
}

template<bool isForAot>
void JSHClass::MarkProtoChanged(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(jshclass->IsPrototype());
    JSTaggedValue enumCache = jshclass->GetEnumCache(thread);
    JSTaggedValue markerValue = jshclass->GetProtoChangeMarker(thread);
    // Used in for-in.
    if (enumCache.IsEnumCache()) {
        EnumCache::Cast(enumCache)->SetInvalidState(thread);
    }
    if (markerValue.IsProtoChangeMarker()) {
        ProtoChangeMarker *protoChangeMarker = ProtoChangeMarker::Cast(markerValue.GetTaggedObject());
        if constexpr (isForAot) {
            protoChangeMarker->SetNotFoundHasChanged(true);
        } else {
            protoChangeMarker->SetHasChanged(true);
        }
    }
}

template<bool isForAot /* = false*/>
void JSHClass::NoticeThroughChain(const JSThread *thread, const JSHandle<JSHClass> &jshclass,
                                  JSTaggedValue addedKey)
{
    DISALLOW_GARBAGE_COLLECTION;
    MarkProtoChanged<isForAot>(thread, jshclass);
    JSTaggedValue protoDetailsValue = jshclass->GetProtoChangeDetails(thread);
    if (!protoDetailsValue.IsProtoChangeDetails()) {
        return;
    }
    JSTaggedValue listenersValue =
        ProtoChangeDetails::Cast(protoDetailsValue.GetTaggedObject())->GetChangeListener(thread);
    if (!listenersValue.IsTaggedArray()) {
        return;
    }
    ChangeListener *listeners = ChangeListener::Cast(listenersValue.GetTaggedObject());
    for (uint32_t i = 0; i < listeners->GetEnd(); i++) {
        JSTaggedValue temp = listeners->Get(thread, i);
        if (temp.IsJSHClass()) {
            NoticeThroughChain<isForAot>(thread,
                JSHandle<JSHClass>(thread, listeners->Get(thread, i).GetTaggedObject()), addedKey);
        }
    }
}

template<bool checkDuplicateKeys /* = false*/>
void JSHClass::AddPropertyToNewHClass(const JSThread *thread, JSHandle<JSHClass> &jshclass,
                                      JSHandle<JSHClass> &newJsHClass,
                                      const JSHandle<JSTaggedValue> &key,
                                      const PropertyAttributes &attr)
{
    ASSERT(!jshclass->IsDictionaryMode());
    ASSERT(!newJsHClass->IsDictionaryMode());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Add Property and metaData
    uint32_t offset = attr.GetOffset();
    newJsHClass->IncNumberOfProps();

    {
        JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, newJsHClass->GetLayout(thread));

        if (layoutInfoHandle->NumberOfElements() != static_cast<int>(offset)) {
            layoutInfoHandle.Update(factory->CopyAndReSort(layoutInfoHandle, offset, offset + 1));
            newJsHClass->SetLayout(thread, layoutInfoHandle);
        } else if (layoutInfoHandle->GetPropertiesCapacity() <= static_cast<int>(offset)) {  // need to Grow
            layoutInfoHandle.Update(
                factory->ExtendLayoutInfo(layoutInfoHandle, offset));
            newJsHClass->SetLayout(thread, layoutInfoHandle);
        }
        layoutInfoHandle->AddKey<checkDuplicateKeys>(thread, offset, key.GetTaggedValue(), attr);
    }

    // Add newClass to old hclass's transitions.
    AddTransitions(thread, jshclass, newJsHClass, key, attr);

    if UNLIKELY(key.GetTaggedValue() == thread->GlobalConstants()->GetConstructorString()
        && (jshclass->IsJSArray() || jshclass->IsTypedArray())) {
        newJsHClass->SetHasConstructor(true);
    }
}

void JSHClass::AddInlinedPropToHClass(const JSThread *thread, const PropertyDescriptor &desc, size_t attrOffset,
                                      const JSHandle<JSTaggedValue> &key, JSHandle<JSHClass> &hClass)
{
    ecmascript::PropertyAttributes attr(desc);
    attr.SetIsInlinedProps(true);
    attr.SetOffset(attrOffset);
    attr.SetRepresentation(ecmascript::Representation::TAGGED);
    // Classes defined by napi_define_class are different from each other, there's no need to consider about transition.
    JSHClass::AddPropertyToNewHClassWithoutTransition(thread, hClass, key, attr);
}

template<bool checkDuplicateKeys /* = false*/>
JSHandle<JSHClass> JSHClass::SetPropertyOfObjHClass(const JSThread *thread, JSHandle<JSHClass> &jshclass,
                                                    const JSHandle<JSTaggedValue> &key,
                                                    const PropertyAttributes &attr, const Representation &rep,
                                                    bool specificInlinedProps, uint32_t specificNumInlinedProps)
{
    JSHClass *newClass = jshclass->FindTransitions(thread,
        key.GetTaggedValue(), JSTaggedValue(attr.GetPropertyMetaData()), rep);
    if (newClass != nullptr) {
        newClass->SetPrototype(thread, jshclass->GetPrototype(thread));
        return JSHandle<JSHClass>(thread, newClass);
    }

    JSHandle<JSHClass> newJsHClass = JSHClass::Clone(thread, jshclass, specificInlinedProps, specificNumInlinedProps);
    AddPropertyToNewHClass<checkDuplicateKeys>(thread, jshclass, newJsHClass, key, attr);
    return newJsHClass;
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_HCLASS_INL_H
