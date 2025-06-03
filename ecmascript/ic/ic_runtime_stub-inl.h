/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_IC_IC_RUNTIME_STUB_INL_H
#define ECMASCRIPT_IC_IC_RUNTIME_STUB_INL_H

#include "ecmascript/base/config.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/ic/ic_runtime_stub.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/ic_runtime.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/js_primitive_ref.h"

namespace panda::ecmascript {
JSTaggedValue ICRuntimeStub::LoadGlobalICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                JSTaggedValue globalValue, JSTaggedValue key, uint32_t slotId,
                                                bool tryLoad)
{
    INTERPRETER_TRACE(thread, LoadGlobalICByName);
    JSTaggedValue handler = profileTypeInfo->Get(slotId);
    if (handler.IsHeapObject()) {
        auto result = LoadGlobal(handler);
        if (!result.IsHole()) {
            return result;
        }
    }
    ICKind kind = tryLoad ? ICKind::NamedGlobalTryLoadIC : ICKind::NamedGlobalLoadIC;
    return LoadMiss(thread, profileTypeInfo, globalValue, key, slotId, kind);
}

JSTaggedValue ICRuntimeStub::StoreGlobalICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                 JSTaggedValue globalValue, JSTaggedValue key,
                                                 JSTaggedValue value, uint32_t slotId, bool tryStore)
{
    INTERPRETER_TRACE(thread, StoreGlobalICByName);
    JSTaggedValue handler = profileTypeInfo->Get(slotId);
    if (handler.IsHeapObject()) {
        auto result = StoreGlobal(thread, value, handler);
        if (!result.IsHole()) {
            return result;
        }
    }
    ICKind kind = tryStore ? ICKind::NamedGlobalTryStoreIC : ICKind::NamedGlobalStoreIC;
    return StoreMiss(thread, profileTypeInfo, globalValue, key, value, slotId, kind);
}

JSTaggedValue ICRuntimeStub::CheckPolyHClass(JSTaggedValue cachedValue, JSHClass* hclass)
{
    if (!cachedValue.IsWeak()) {
        ASSERT(cachedValue.IsTaggedArray());
        TaggedArray *array = TaggedArray::Cast(cachedValue.GetTaggedObject());
        uint32_t length = array->GetLength();
        for (uint32_t i = 0; i < length; i += 2) {  // 2 means one ic, two slot
            auto result = array->Get(i);
            if (!result.IsUndefined() && result.GetWeakReferent() == hclass) {
                return array->Get(i + 1);
            }
        }
    }
    return JSTaggedValue::Hole();
}

ARK_INLINE JSTaggedValue ICRuntimeStub::TryLoadICByName(JSThread *thread, JSTaggedValue receiver,
                                                        JSTaggedValue firstValue, JSTaggedValue secondValue)
{
    INTERPRETER_TRACE(thread, TryLoadICByName);
    if (LIKELY(receiver.IsHeapObject())) {
        auto hclass = receiver.GetTaggedObject()->GetClass();
        if (LIKELY(firstValue.GetWeakReferentUnChecked() == hclass)) {
            return LoadICWithHandler(thread, receiver, receiver, secondValue);
        }
        JSTaggedValue cachedHandler = CheckPolyHClass(firstValue, hclass);
        if (!cachedHandler.IsHole()) {
            return LoadICWithHandler(thread, receiver, receiver, cachedHandler);
        }
    } else if (receiver.IsNumber()) {
        JSHandle<JSFunction> function(thread->GetEcmaVM()->GetGlobalEnv()->GetNumberFunction());
        auto hclass = reinterpret_cast<JSHClass *>(function->GetProtoOrHClass().GetTaggedObject());
        if (firstValue.GetWeakReferentUnChecked() == hclass) {
            return LoadICWithHandler(thread, receiver, receiver, secondValue);
        }
    }
    return JSTaggedValue::Hole();
}

ARK_NOINLINE JSTaggedValue ICRuntimeStub::LoadICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                       JSTaggedValue receiver, JSTaggedValue key, uint32_t slotId)
{
    INTERPRETER_TRACE(thread, LoadICByName);
    return LoadMiss(thread, profileTypeInfo, receiver, key, slotId, ICKind::NamedLoadIC);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::TryLoadICByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                         JSTaggedValue firstValue, JSTaggedValue secondValue)
{
    INTERPRETER_TRACE(thread, TryLoadICByValue);
    if (receiver.IsHeapObject()) {
        auto hclass = receiver.GetTaggedObject()->GetClass();
        if (firstValue.GetWeakReferentUnChecked() == hclass) {
            if (HandlerBase::IsNormalElement(secondValue.GetNumber())) {
                return LoadElement(thread, JSObject::Cast(receiver.GetTaggedObject()), key);
            } else if (HandlerBase::IsTypedArrayElement(secondValue.GetNumber())) {
                return LoadTypedArrayElement(thread, receiver, key);
            }
            ASSERT(HandlerBase::IsStringElement(secondValue.GetNumber()));
            return LoadStringElement(thread, receiver, key);
        }
        // check ploy
        if (secondValue.IsHole() && !firstValue.IsHole()) {
            JSTaggedValue cachedHandler = CheckPolyHClass(firstValue, hclass);
            return LoadICWithElementHandler(thread, receiver, cachedHandler, key);
        }
        // Check key
        if (firstValue == key) {
            JSTaggedValue cachedHandler = CheckPolyHClass(secondValue, hclass);
            if (!cachedHandler.IsHole()) {
                return LoadICWithHandler(thread, receiver, receiver, cachedHandler);
            }
        }
    }
    return JSTaggedValue::Hole();
}

ARK_NOINLINE JSTaggedValue ICRuntimeStub::LoadICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                        JSTaggedValue receiver, JSTaggedValue key, uint32_t slotId)
{
    INTERPRETER_TRACE(thread, LoadICByValue);
    return LoadValueMiss(thread, profileTypeInfo, receiver, key, slotId, ICKind::LoadIC);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::TryStoreICByValue(JSThread *thread, JSTaggedValue receiver,
                                                          JSTaggedValue key, JSTaggedValue firstValue,
                                                          JSTaggedValue secondValue, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, TryStoreICByValue);
    if (receiver.IsHeapObject()) {
        auto hclass = receiver.GetTaggedObject()->GetClass();
        if (firstValue.GetWeakReferentUnChecked() == hclass) {
            return StoreElement(thread, JSObject::Cast(receiver.GetTaggedObject()), key, value, secondValue);
        }
        // Check key
        if (firstValue == key) {
            JSTaggedValue cachedHandler = CheckPolyHClass(secondValue, hclass);
            if (!cachedHandler.IsHole()) {
                return StoreICWithHandler(thread, receiver, receiver, value, cachedHandler);
            }
        }
    }

    return JSTaggedValue::Hole();
}

ARK_NOINLINE JSTaggedValue ICRuntimeStub::StoreICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                         JSTaggedValue receiver, JSTaggedValue key,
                                                         JSTaggedValue value, uint32_t slotId)
{
    INTERPRETER_TRACE(thread, StoreICByValue);
    return StoreMiss(thread, profileTypeInfo, receiver, key, value, slotId, ICKind::StoreIC);
}

ARK_NOINLINE JSTaggedValue ICRuntimeStub::StoreOwnICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                            JSTaggedValue receiver, JSTaggedValue key,
                                                            JSTaggedValue value, uint32_t slotId)
{
    INTERPRETER_TRACE(thread, StoreOwnICByValue);
    return StoreMiss(thread, profileTypeInfo, receiver, key, value, slotId, ICKind::StoreIC, true);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::TryStoreICByName(JSThread *thread, JSTaggedValue receiver,
                                                         JSTaggedValue firstValue, JSTaggedValue secondValue,
                                                         JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, TryStoreICByName);
    if (receiver.IsHeapObject()) {
        auto hclass = receiver.GetTaggedObject()->GetClass();
        if (firstValue.GetWeakReferentUnChecked() == hclass) {
            return StoreICWithHandler(thread, receiver, receiver, value, secondValue);
        }
        JSTaggedValue cachedHandler = CheckPolyHClass(firstValue, hclass);
        if (!cachedHandler.IsHole()) {
            return StoreICWithHandler(thread, receiver, receiver, value, cachedHandler);
        }
    }
    return JSTaggedValue::Hole();
}

ARK_NOINLINE JSTaggedValue ICRuntimeStub::StoreICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                        JSTaggedValue receiver, JSTaggedValue key,
                                                        JSTaggedValue value, uint32_t slotId)
{
    INTERPRETER_TRACE(thread, StoreICByName);
    return StoreMiss(thread, profileTypeInfo, receiver, key, value, slotId, ICKind::NamedStoreIC);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::StoreICWithHandler(JSThread *thread, JSTaggedValue receiver,
                                                           JSTaggedValue holder,
                                                           JSTaggedValue value, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, StoreICWithHandler);
    if (handler.IsInt()) {
        auto handlerInfo = JSTaggedValue::UnwrapToUint64(handler);
        HandlerBase::PrintStoreHandler(handlerInfo, std::cout);
        if (HandlerBase::IsNonSharedStoreField(handlerInfo)) {
            StoreField(thread, JSObject::Cast(receiver.GetTaggedObject()), value, handlerInfo);
            return JSTaggedValue::Undefined();
        }
        bool isShared = HandlerBase::IsStoreShared(handlerInfo);
        if (isShared) {
            SharedFieldType fieldType { HandlerBase::GetFieldType(handlerInfo) };
            bool hasAccessor = HandlerBase::IsAccessor(handlerInfo);
            if (!hasAccessor) {
                if (!ClassHelper::MatchFieldType(fieldType, value)) {
                    [[maybe_unused]] EcmaHandleScope handleScope(thread);
                    THROW_TYPE_ERROR_AND_RETURN((thread), GET_MESSAGE_STRING(SetTypeMismatchedSharedProperty),
                                                JSTaggedValue::Exception());
                }
                if (value.IsTreeString()) {
                    [[maybe_unused]] EcmaHandleScope handleScope(thread);
                    JSHandle<JSTaggedValue> objHandle(thread, receiver);
                    JSHandle<JSTaggedValue> holderHandle(thread, holder);
                    JSHandle<JSTaggedValue> valueHandle(thread, value);
                    value = JSTaggedValue::PublishSharedValue(thread, valueHandle).GetTaggedValue();
                    receiver = objHandle.GetTaggedValue();
                    holder = holderHandle.GetTaggedValue();
                }
            }
            HandlerBase::ClearSharedStoreKind(handlerInfo);
            return StoreICWithHandler(thread, receiver, holder, value,
                                      JSTaggedValue::WrapUint64(handlerInfo));
        }
        ASSERT(HandlerBase::IsAccessor(handlerInfo));
        auto accessor = LoadFromField(JSObject::Cast(holder.GetTaggedObject()), handlerInfo);
        return FastRuntimeStub::CallSetter(thread, JSTaggedValue(receiver), value, accessor);
    }
    if (handler.IsTransitionHandler()) {
        StoreWithTransition(thread, JSObject::Cast(receiver.GetTaggedObject()), value, handler);
        return JSTaggedValue::Undefined();
    }
    if (handler.IsTransWithProtoHandler()) {
        return StoreTransWithProto(thread, JSObject::Cast(receiver.GetTaggedObject()), value, handler);
    }
    if (handler.IsPrototypeHandler()) {
        return StorePrototype(thread, receiver, value, handler);
    }
    if (handler.IsPropertyBox()) {
        return StoreGlobal(thread, value, handler);
    }
    if (handler.IsStoreAOTHandler()) {
        return StoreWithTS(thread, receiver, value, handler);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ICRuntimeStub::StorePrototype(JSThread *thread, JSTaggedValue receiver,
                                            JSTaggedValue value, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, StorePrototype);
    ASSERT(handler.IsPrototypeHandler());
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(handler.GetTaggedObject());
    if (!receiver.IsJSShared()) {
        auto cellValue = prototypeHandler->GetProtoCell();
        if (cellValue.IsNull()) {
            return JSTaggedValue::Hole();
        }
        ASSERT(cellValue.IsProtoChangeMarker());
        ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
        if (cell->GetHasChanged()) {
            return JSTaggedValue::Hole();
        }
    }
    auto holder = prototypeHandler->GetHolder();
    JSTaggedValue handlerInfo = prototypeHandler->GetHandlerInfo();
    return StoreICWithHandler(thread, receiver, holder, value, handlerInfo);
}

JSTaggedValue ICRuntimeStub::StoreWithTS(JSThread *thread, JSTaggedValue receiver,
                                         JSTaggedValue value, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, StoreWithAOT);
    ASSERT(handler.IsStoreAOTHandler());
    StoreAOTHandler *storeAOTHandler = StoreAOTHandler::Cast(handler.GetTaggedObject());
    auto cellValue = storeAOTHandler->GetProtoCell();
    if (cellValue == JSTaggedValue::Undefined()) {
        return JSTaggedValue::Hole();
    }
    ASSERT(cellValue.IsProtoChangeMarker());
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return JSTaggedValue::Hole();
    }
    auto holder = storeAOTHandler->GetHolder();
    JSTaggedValue handlerInfo = storeAOTHandler->GetHandlerInfo();
    auto handlerInfoInt = JSTaggedValue::UnwrapToUint64(handlerInfo);
    if (HandlerBase::IsField(handlerInfoInt)) {
        StoreField(thread, JSObject::Cast(receiver.GetTaggedObject()), value, handlerInfoInt);
        return JSTaggedValue::Undefined();
    }
    ASSERT(HandlerBase::IsAccessor(handlerInfoInt));
    auto accessor = LoadFromField(JSObject::Cast(holder.GetTaggedObject()), handlerInfoInt);
    return FastRuntimeStub::CallSetter(thread, JSTaggedValue(receiver), value, accessor);
}

void ICRuntimeStub::StoreWithTransition(JSThread *thread, JSObject *receiver, JSTaggedValue value,
                                        JSTaggedValue handler, bool withPrototype)
{
    INTERPRETER_TRACE(thread, StoreWithTransition);

    JSHClass *newHClass = nullptr;
    uint64_t handlerInfo = 0;

    if (withPrototype) {
        TransWithProtoHandler *transWithProtoHandler = TransWithProtoHandler::Cast(handler.GetTaggedObject());
        newHClass = JSHClass::Cast(transWithProtoHandler->GetTransitionHClass().GetTaggedObject());
        handlerInfo = JSTaggedValue::UnwrapToUint64(transWithProtoHandler->GetHandlerInfo());
    } else {
        TransitionHandler *transitionHandler = TransitionHandler::Cast(handler.GetTaggedObject());
        newHClass = JSHClass::Cast(transitionHandler->GetTransitionHClass().GetTaggedObject());
        handlerInfo = JSTaggedValue::UnwrapToUint64(transitionHandler->GetHandlerInfo());
    }
    JSHandle<JSHClass> newHClassHandle(thread, newHClass);
    JSHandle<JSHClass> oldHClassHandle(thread, receiver->GetJSHClass());
    if (newHClassHandle->IsPrototype()) {
        newHClassHandle->SetProtoChangeDetails(thread, oldHClassHandle->GetProtoChangeDetails());
    }
    JSHandle<JSObject> objHandle(thread, receiver);
    ElementsKind oldKind = receiver->GetJSHClass()->GetElementsKind();
    JSHClass::RestoreElementsKindToGeneric(newHClass);
    objHandle->SynchronizedTransitionClass(thread, newHClass);
    JSObject::TryMigrateToGenericKindForJSObject(thread, objHandle, oldKind);

    ASSERT(HandlerBase::IsField(handlerInfo));

    if (!HandlerBase::IsInlinedProps(handlerInfo)) {
        TaggedArray *array = TaggedArray::Cast(objHandle->GetProperties().GetTaggedObject());
        int capacity = static_cast<int>(array->GetLength());
        int index = HandlerBase::GetOffset(handlerInfo);
        if (index >= capacity) {
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            [[maybe_unused]] EcmaHandleScope handleScope(thread);
            JSHandle<TaggedArray> properties;
            JSHandle<JSTaggedValue> valueHandle(thread, value);
            if (capacity == 0) {
                capacity = JSObject::MIN_PROPERTIES_LENGTH;
                properties = factory->NewTaggedArray(capacity);
            } else {
                auto arrayHandle = JSHandle<TaggedArray>(thread, array);
                uint32_t maxNonInlinedFastPropsCapacity = objHandle->GetNonInlinedFastPropsCapacity();
                uint32_t newLen = JSObject::ComputeNonInlinedFastPropsCapacity(thread, capacity,
                                                                               maxNonInlinedFastPropsCapacity);
                properties = factory->CopyArray(arrayHandle, capacity, newLen);
            }
            properties->Set(thread, index, valueHandle);
            objHandle->SetProperties(thread, properties);
            return;
        }
        array->Set(thread, index, value);
        return;
    }
    StoreField(thread, *objHandle, value, handlerInfo);
}

JSTaggedValue ICRuntimeStub::StoreTransWithProto(JSThread *thread, JSObject *receiver, JSTaggedValue value,
                                                 JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, StoreTransWithProto);
    ASSERT(handler.IsTransWithProtoHandler());
    ASSERT(!receiver->GetClass()->IsJSShared());
    TransWithProtoHandler *transWithProtoHandler = TransWithProtoHandler::Cast(handler.GetTaggedObject());
    auto cellValue = transWithProtoHandler->GetProtoCell();
    if (cellValue == JSTaggedValue::Undefined()) {
        return JSTaggedValue::Hole();
    }
    ASSERT(cellValue.IsProtoChangeMarker());
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return JSTaggedValue::Hole();
    }

    StoreWithTransition(thread, receiver, value, handler, true);
    return JSTaggedValue::Undefined();
}

ARK_INLINE void ICRuntimeStub::StoreField(JSThread *thread, JSObject *receiver, JSTaggedValue value, uint64_t handler)
{
    INTERPRETER_TRACE(thread, StoreField);
    int index = HandlerBase::GetOffset(handler);
    if (HandlerBase::IsInlinedProps(handler)) {
        SET_VALUE_WITH_BARRIER(thread, receiver, static_cast<uint32_t>(index) * JSTaggedValue::TaggedTypeSize(), value);
        return;
    }
    TaggedArray *array = TaggedArray::Cast(receiver->GetProperties().GetTaggedObject());
    ASSERT(index < static_cast<int>(array->GetLength()));
    array->Set(thread, index, value);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::LoadFromField(JSObject *receiver, uint64_t handlerInfo)
{
    int index = HandlerBase::GetOffset(handlerInfo);
    if (HandlerBase::IsInlinedProps(handlerInfo)) {
        return JSTaggedValue(GET_VALUE(receiver, static_cast<size_t>(index) * JSTaggedValue::TaggedTypeSize()));
    }
    return TaggedArray::Cast(receiver->GetProperties().GetTaggedObject())->Get(index);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::LoadGlobal(JSTaggedValue handler)
{
    ASSERT(handler.IsPropertyBox());
    PropertyBox *cell = PropertyBox::Cast(handler.GetTaggedObject());
    if (cell->IsInvalid() || cell->GetValue().IsAccessorData()) {
        return JSTaggedValue::Hole();
    }
    JSTaggedValue ret = cell->GetValue();
    ASSERT(!ret.IsAccessorData());
    return ret;
}

ARK_INLINE JSTaggedValue ICRuntimeStub::StoreGlobal(JSThread *thread, JSTaggedValue value, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, StoreGlobal);
    ASSERT(handler.IsPropertyBox());
    PropertyBox *cell = PropertyBox::Cast(handler.GetTaggedObject());
    if (cell->IsInvalid() || cell->GetValue().IsAccessorData()) {
        return JSTaggedValue::Hole();
    }
    ASSERT(!cell->GetValue().IsAccessorData());
    cell->SetValue(thread, value);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ICRuntimeStub::LoadPrototype(JSThread *thread, JSTaggedValue receiver, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, LoadPrototype);
    ASSERT(handler.IsPrototypeHandler());
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(handler.GetTaggedObject());
    auto holder = prototypeHandler->GetHolder();
    if (!receiver.IsJSShared()) {
        auto cellValue = prototypeHandler->GetProtoCell();
        if (cellValue == JSTaggedValue::Undefined()) {
            return JSTaggedValue::Hole();
        }
        ASSERT(cellValue.IsProtoChangeMarker());
        ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
        if (cell->GetHasChanged()) {
            return JSTaggedValue::Hole();
        }

        // For "Not Found" case (holder equals Undefined()),
        // we should ensure that both GetNotFoundHasChanged() and GetHasChanged() return false.
        if (holder == JSTaggedValue::Undefined()) {
            if (cell->GetNotFoundHasChanged()) {
                return JSTaggedValue::Hole();
            }
            return JSTaggedValue::Undefined();
        }
    }

    JSTaggedValue handlerInfo = prototypeHandler->GetHandlerInfo();
    return LoadICWithHandler(thread, receiver, holder, handlerInfo);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::LoadICWithHandler(JSThread *thread, JSTaggedValue receiver,
                                                          JSTaggedValue holder, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, LoadICWithHandler);
    if (LIKELY(handler.IsInt())) {
        auto handlerInfo = JSTaggedValue::UnwrapToUint64(handler);
        HandlerBase::PrintLoadHandler(handlerInfo, std::cout);
        if (LIKELY(HandlerBase::IsField(handlerInfo))) {
            return LoadFromField(JSObject::Cast(holder.GetTaggedObject()), handlerInfo);
        }
        if (HandlerBase::IsSupportedPrimitiveTypeICHandler(handlerInfo)) {
            return LoadFromField(JSObject::Cast(holder.GetTaggedObject()), handlerInfo);
        }

        // For the special "Not Found" case we may generate ic by "LoadHandler::LoadProperty".
        // In this situation, you can trust ic without ChangeMarker.
        if (HandlerBase::IsNonExist(handlerInfo)) {
            return JSTaggedValue::Undefined();
        }

        if (HandlerBase::IsStringLength(handlerInfo)) {
            return JSTaggedNumber((EcmaStringAccessor(EcmaString::Cast(holder)).GetLength()));
        }
        ASSERT(HandlerBase::IsAccessor(handlerInfo));
        auto accessor = LoadFromField(JSObject::Cast(holder.GetTaggedObject()), handlerInfo);
        return FastRuntimeStub::CallGetter(thread, receiver, holder, accessor);
    }

    if (handler.IsPrototypeHandler()) {
        return LoadPrototype(thread, receiver, handler);
    }

    return LoadGlobal(handler);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::LoadICWithElementHandler(JSThread *thread, JSTaggedValue receiver,
    JSTaggedValue handler, JSTaggedValue key)
{
    if (LIKELY(handler.IsInt())) {
        auto handlerInfo = JSTaggedValue::UnwrapToUint64(handler);
        HandlerBase::PrintLoadHandler(handlerInfo, std::cout);
        if (HandlerBase::IsNormalElement(handlerInfo)) {
            return LoadElement(thread, JSObject::Cast(receiver.GetTaggedObject()), key);
        } else if (HandlerBase::IsTypedArrayElement(handlerInfo)) {
            return LoadTypedArrayElement(thread, receiver, key);
        }
        ASSERT(HandlerBase::IsStringElement(handlerInfo));
        return LoadStringElement(thread, receiver, key);
    }
    return JSTaggedValue::Hole();
}

ARK_INLINE JSTaggedValue ICRuntimeStub::LoadElement(JSThread *thread, JSObject *receiver, JSTaggedValue key)
{
    auto index = TryToElementsIndex(key);
    if (index < 0) {
        return JSTaggedValue::Hole();
    }
    uint32_t elementIndex = static_cast<uint32_t>(index);
    if (ElementAccessor::GetElementsLength(receiver) <= elementIndex) {
        return JSTaggedValue::Hole();
    }

    JSTaggedValue value = ElementAccessor::Get(thread, receiver, elementIndex);
    // TaggedArray elements
    return value;
}

ARK_INLINE JSTaggedValue ICRuntimeStub::LoadStringElement(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key)
{
    auto index = TryToElementsIndex(key);
    if (index < 0) {
        return JSTaggedValue::Hole();
    }
    uint32_t elementIndex = static_cast<uint32_t>(index);
    uint16_t tmpChar = 0;
    {
        JSHandle<EcmaString> strHandle(thread, receiver);
        JSHandle<EcmaString> strFlat(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), strHandle));
        if (EcmaStringAccessor(strFlat).GetLength() <= elementIndex) {
            return JSTaggedValue::Hole();
        }
        tmpChar = EcmaStringAccessor(strFlat).Get(elementIndex);
    }
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> value(factory->NewFromUtf16(&tmpChar, 1));
    return value.GetTaggedValue();
}

ARK_INLINE JSTaggedValue ICRuntimeStub::LoadTypedArrayElement(JSThread *thread, JSTaggedValue receiver,
                                                              JSTaggedValue key)
{
    auto index = TryToElementsIndex(key);
    if (index < 0) {
        return JSTaggedValue::Hole();
    }
    auto typedarrayObj = JSTypedArray::Cast(receiver.GetTaggedObject());
    uint32_t arrLen = typedarrayObj->GetArrayLength();
    if (index >= arrLen) {
        return JSTaggedValue::Hole();
    }
    JSType type = typedarrayObj->GetJSHClass()->GetObjectType();
    return JSTypedArray::FastGetPropertyByIndex(thread, receiver, index, type);
}

template <typename T>
ARK_INLINE JSTaggedValue ICRuntimeStub::StoreElementWithProtoHandler(JSThread *thread,
                                                                     JSObject *receiver,
                                                                     JSTaggedValue key,
                                                                     JSTaggedValue value,
                                                                     JSTaggedValue handler)
{
    if (receiver->GetClass()->IsJSShared()) {
        THROW_TYPE_ERROR_AND_RETURN(thread,
            GET_MESSAGE_STRING(SetTypeMismatchedSharedProperty), JSTaggedValue::Exception());
    }
    T *protoHandler = T::Cast(handler.GetTaggedObject());
    auto cellValue = protoHandler->GetProtoCell();
    if (cellValue == JSTaggedValue::Undefined() || cellValue == JSTaggedValue::Null()) {
        return JSTaggedValue::Hole();
    }
    ASSERT(cellValue.IsProtoChangeMarker());
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetTaggedObject());
    if (cell->GetHasChanged()) {
        return JSTaggedValue::Hole();
    }
    JSTaggedValue handlerInfo = protoHandler->GetHandlerInfo();
    return StoreElement(thread, receiver, key, value, handlerInfo);
}

JSTaggedValue ICRuntimeStub::StoreElement(JSThread *thread, JSObject *receiver, JSTaggedValue key,
                                          JSTaggedValue value, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, StoreElement);
    auto index = TryToElementsIndex(key);
    if (index < 0) {
        return JSTaggedValue::Hole();
    }
    uint32_t elementIndex = static_cast<uint32_t>(index);
    if (handler.IsInt()) {
        auto handlerInfo = JSTaggedValue::UnwrapToUint64(handler);
        HandlerBase::PrintStoreHandler(handlerInfo, std::cout);
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        JSHandle<JSObject> receiverHandle(thread, receiver);
        JSHandle<JSTaggedValue> valueHandle(thread, value);
        if (HandlerBase::IsTypedArrayElement(handlerInfo)) {
            return StoreTypedArrayElement(thread, JSTaggedValue::Cast(receiver), key, value);
        } else if (HandlerBase::IsJSArray(handlerInfo)) {
            JSTaggedValue receiveValue = receiverHandle.GetTaggedValue();
            if (receiveValue.IsJSCOWArray()) {
                // Copy on write array.
                JSArray::CheckAndCopyArray(thread, JSHandle<JSArray>::Cast(receiverHandle));
            }
            JSArray *arr = JSArray::Cast(*receiverHandle);
            uint32_t oldLength = arr->GetArrayLength();
            if (elementIndex >= oldLength) {
                arr->SetArrayLength(thread, elementIndex + 1);
            }
        }
        TaggedArray *elements = TaggedArray::Cast(receiverHandle->GetElements().GetTaggedObject());
        uint32_t capacity = elements->GetLength();
        if (elementIndex >= capacity) {
            if (JSObject::ShouldTransToDict(capacity, elementIndex)) {
                return JSTaggedValue::Hole();
            }
            elements = *JSObject::GrowElementsCapacity(thread, receiverHandle, elementIndex + 1);
            receiverHandle->SetElements(thread, JSTaggedValue(elements));
            elements->Set(thread, elementIndex, valueHandle);
            return JSTaggedValue::Undefined();
        }
        elements->Set(thread, elementIndex, valueHandle);
    } else {
        if (handler.IsPrototypeHandler()) {
            return StoreElementWithProtoHandler<PrototypeHandler>(thread, receiver, key, value, handler);
        } else if (handler.IsTransWithProtoHandler()) {
            return StoreElementWithProtoHandler<TransWithProtoHandler>(thread, receiver, key, value, handler);
        } else {
            ASSERT(handler.IsTransitionHandler());
            if (receiver->GetClass()->IsJSShared()) {
                THROW_TYPE_ERROR_AND_RETURN(thread,
                    GET_MESSAGE_STRING(SetTypeMismatchedSharedProperty), JSTaggedValue::Exception());
            }
            TransitionHandler *transitionHandler = TransitionHandler::Cast(handler.GetTaggedObject());
            JSTaggedValue handlerInfo = transitionHandler->GetHandlerInfo();
            return StoreElement(thread, receiver, key, value, handlerInfo);
        }
    }
    return JSTaggedValue::Undefined();
}

ARK_INLINE JSTaggedValue ICRuntimeStub::StoreTypedArrayElement(JSThread *thread, JSTaggedValue receiver,
                                                               JSTaggedValue key, JSTaggedValue value)
{
    auto index = TryToElementsIndex(key);
    if (index < 0) {
        return JSTaggedValue::Hole();
    }
    auto typedarrayObj = JSTypedArray::Cast(receiver.GetTaggedObject());
    uint32_t arrLen = typedarrayObj->GetArrayLength();
    if (index >= arrLen) {
        return JSTaggedValue::Hole();
    }
    JSType type = typedarrayObj->GetJSHClass()->GetObjectType();
    return JSTypedArray::FastSetPropertyByIndex(thread, receiver, index, value, type);
}

ARK_INLINE int64_t ICRuntimeStub::TryToElementsIndex(JSTaggedValue key)
{
    if (LIKELY(key.IsInt())) {
        return key.GetInt();
    }

    if (key.IsString()) {
        uint32_t index = 0;
        if (JSTaggedValue::StringToElementIndex(key, &index)) {
            return static_cast<int64_t>(index);
        }
    }

    if (key.IsDouble()) {
        double number = key.GetDouble();
        auto integer = static_cast<int32_t>(number);
        if (number == integer) {
            return integer;
        }
    }

    return -1;
}

JSTaggedValue ICRuntimeStub::LoadMiss(JSThread *thread, ProfileTypeInfo *profileTypeInfo, JSTaggedValue receiver,
                                      JSTaggedValue key, uint32_t slotId, ICKind kind)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, receiver);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId, kind);
    return icRuntime.LoadMiss(receiverHandle, keyHandle);
}

JSTaggedValue ICRuntimeStub::LoadValueMiss(JSThread *thread, ProfileTypeInfo *profileTypeInfo, JSTaggedValue receiver,
                                           JSTaggedValue key, uint32_t slotId, ICKind kind)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, receiver);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId, kind);
    return icRuntime.LoadValueMiss(receiverHandle, keyHandle);
}

JSTaggedValue ICRuntimeStub::StoreMiss(JSThread *thread, ProfileTypeInfo *profileTypeInfo, JSTaggedValue receiver,
                                       JSTaggedValue key, JSTaggedValue value, uint32_t slotId, ICKind kind, bool isOwn)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, receiver);
    auto valueHandle = JSHandle<JSTaggedValue>(thread, value);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId, kind);
    return icRuntime.StoreMiss(receiverHandle, keyHandle, valueHandle, isOwn);
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_IC_IC_RUNTIME_STUB_INL_H
