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

#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/global_env_constants-inl.h"

namespace panda::ecmascript {

void UpdateHandlerKind(const JSThread *thread, const ObjectOperator &op, uint64_t &handler)
{
    JSHandle<JSTaggedValue> receiver = op.GetReceiver();
    if (receiver->IsString()) {
        JSTaggedValue lenKey = thread->GlobalConstants()->GetLengthString();
        JSHandle<JSTaggedValue> key = op.GetKey();
        EcmaString *proKey = key->IsString() ? EcmaString::Cast(key->GetTaggedObject()) : nullptr;
        if (proKey != nullptr &&
            EcmaStringAccessor::StringsAreEqual(proKey, EcmaString::Cast(lenKey.GetTaggedObject()))) {
            HandlerBase::KindBit::Set<uint64_t>(HandlerBase::HandlerKind::STRING_LENGTH, &handler);
        } else {
            HandlerBase::KindBit::Set<uint64_t>(HandlerBase::HandlerKind::STRING, &handler);
        }
    } else if (receiver->IsNumber()) {
        HandlerBase::KindBit::Set<uint64_t>(HandlerBase::HandlerKind::NUMBER, &handler);
    } else if (receiver->IsBoolean()) {
        HandlerBase::KindBit::Set<uint64_t>(HandlerBase::HandlerKind::BOOLEAN, &handler);
    } else {
        HandlerBase::KindBit::Set<uint64_t>(HandlerBase::HandlerKind::FIELD, &handler);
    }
}

JSHandle<JSTaggedValue> LoadHandler::LoadProperty(const JSThread *thread, const ObjectOperator &op)
{
    uint64_t handler = 0;
    ASSERT(!op.IsElement());
    if (!op.IsFound()) {
        KindBit::Set<uint64_t>(HandlerKind::NON_EXIST, &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::WrapUint64(handler));
    }
    ASSERT(op.IsFastMode());

    JSTaggedValue val = op.GetValue();
    if (val.IsPropertyBox()) {
        return JSHandle<JSTaggedValue>(thread, val);
    }
    bool hasAccessor = op.IsAccessorDescriptor();
    AccessorBit::Set<uint64_t>(hasAccessor, &handler);

    if (!hasAccessor) {
        UpdateHandlerKind(thread, op, handler);
    }

    if (op.IsInlinedProps()) {
        InlinedPropsBit::Set<uint64_t>(true, &handler);
        JSHandle<JSObject> holder = JSHandle<JSObject>::Cast(op.GetHolder());
        auto index = holder->GetJSHClass()->GetInlinedPropertiesIndex(op.GetIndex());
        OffsetBit::Set<uint64_t>(index, &handler);
        AttrIndexBit::Set<uint64_t>(op.GetIndex(), &handler);
        RepresentationBit::Set<uint64_t>(op.GetRepresentation(), &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::WrapUint64(handler));
    }
    if (op.IsFastMode()) {
        JSHandle<JSObject> holder = JSHandle<JSObject>::Cast(op.GetHolder());
        uint32_t inlinePropNum = holder->GetJSHClass()->GetInlinedProperties();
        AttrIndexBit::Set<uint64_t>(op.GetIndex() + inlinePropNum, &handler);
        OffsetBit::Set<uint64_t>(op.GetIndex(), &handler);
        RepresentationBit::Set<uint64_t>(Representation::TAGGED, &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::WrapUint64(handler));
    }
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

JSHandle<JSTaggedValue> LoadHandler::LoadElement(const JSThread *thread, const ObjectOperator &op)
{
    uint64_t handler = 0;
    KindBit::Set<uint64_t>(HandlerKind::ELEMENT, &handler);
    // To avoid logical errors and Deopt, temporarily skipping PGO Profiling.
    // logical errors:
    //     When accessing an element of an object, AOT does not have a chain-climbing operation,
    //     so if the element is on a prototype, it will not be able to get the correct element.
    // deopt:
    //     Currently there is no way to save the type of the key in pgo file, even if the type of the key
    //     is string, it will be treated as a number type by the AOT, leading to deopt at runtime.
    if (op.GetReceiver() != op.GetHolder() || op.KeyFromStringType()) {
        NeedSkipInPGODumpBit::Set<uint64_t>(true, &handler);
    }
    if (op.GetReceiver()->IsJSArray()) {
        IsJSArrayBit::Set<uint64_t>(true, &handler);
    }
    return JSHandle<JSTaggedValue>(thread, JSTaggedValue::WrapUint64(handler));
}

JSHandle<JSTaggedValue> StoreHandler::StoreProperty(const JSThread *thread, const ObjectOperator &op)
{
    uint64_t handler = 0;
    JSHandle<JSObject> receiver = JSHandle<JSObject>::Cast(op.GetReceiver());
    SFieldTypeBitSet(op, receiver, &handler);
    if (op.IsElement()) {
        SOutOfBoundsBit::Set<uint64_t>(op.GetElementOutOfBounds(), &handler);
        return StoreElement(thread, op.GetReceiver(), handler);
    }
    JSTaggedValue val = op.GetValue();
    if (val.IsPropertyBox()) {
        return JSHandle<JSTaggedValue>(thread, val);
    }
    bool hasSetter = op.IsAccessorDescriptor();
    AccessorBit::Set<uint64_t>(hasSetter, &handler);
    if (!hasSetter) {
        SKindBit::Set<uint64_t>(StoreHandlerKind::S_FIELD, &handler);
    }
    if (op.IsInlinedProps()) {
        InlinedPropsBit::Set<uint64_t>(true, &handler);
        uint32_t index = 0;
        if (!hasSetter) {
            index = receiver->GetJSHClass()->GetInlinedPropertiesIndex(op.GetIndex());
        } else {
            JSHandle<JSObject> holder = JSHandle<JSObject>::Cast(op.GetHolder());
            index = holder->GetJSHClass()->GetInlinedPropertiesIndex(op.GetIndex());
        }
        AttrIndexBit::Set<uint64_t>(op.GetIndex(), &handler);
        OffsetBit::Set<uint64_t>(index, &handler);
        RepresentationBit::Set(op.GetRepresentation(), &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::WrapUint64(handler));
    }
    ASSERT(op.IsFastMode());
    uint32_t inlinePropNum = receiver->GetJSHClass()->GetInlinedProperties();
    AttrIndexBit::Set<uint64_t>(op.GetIndex() + inlinePropNum, &handler);
    OffsetBit::Set<uint64_t>(op.GetIndex(), &handler);
    RepresentationBit::Set(Representation::TAGGED, &handler);
    return JSHandle<JSTaggedValue>(thread, JSTaggedValue::WrapUint64(handler));
}

JSHandle<JSTaggedValue> TransitionHandler::StoreTransition(const JSThread *thread, const ObjectOperator &op)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TransitionHandler> handler = factory->NewTransitionHandler();
    JSHandle<JSTaggedValue> handlerInfo = StoreHandler::StoreProperty(thread, op);
    handler->SetHandlerInfo(thread, handlerInfo);
    auto hclass = JSObject::Cast(op.GetReceiver()->GetTaggedObject())->GetJSHClass();
    handler->SetTransitionHClass(thread, JSTaggedValue(hclass));
    return JSHandle<JSTaggedValue>::Cast(handler);
}

JSHandle<JSTaggedValue> PrototypeHandler::LoadPrototype(const JSThread *thread, const ObjectOperator &op,
                                                        const JSHandle<JSHClass> &hclass)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handlerInfo = LoadHandler::LoadProperty(thread, op);
    JSHandle<PrototypeHandler> handler = factory->NewPrototypeHandler();
    handler->SetHandlerInfo(thread, handlerInfo);
    if (op.IsFound()) {
        handler->SetHolder(thread, op.GetHolder());
    } else {
        // In "Not Found" case we set holder to Undefined().
        handler->SetHolder(thread, JSTaggedValue::Undefined());
    }
    if (op.IsAccessorDescriptor()) {
        JSTaggedValue result = op.GetValue();
        if (result.IsPropertyBox()) {
            result = PropertyBox::Cast(result.GetTaggedObject())->GetValue();
        }
        AccessorData *accessor = AccessorData::Cast(result.GetTaggedObject());
        if (!accessor->IsInternal()) {
            JSTaggedValue getter = accessor->GetGetter();
            if (!getter.IsUndefined()) {
                JSHandle<JSFunction> func(thread, getter);
                uint32_t methodOffset = Method::Cast(func->GetMethod())->GetMethodId().GetOffset();
                handler->SetAccessorMethodId(methodOffset);
                handler->SetAccessorJSFunction(thread, getter);
            }
        }
    }
    auto result = JSHClass::EnableProtoChangeMarker(thread, hclass);
    handler->SetProtoCell(thread, result);
    return JSHandle<JSTaggedValue>::Cast(handler);
}

JSHandle<JSTaggedValue> PrototypeHandler::StorePrototype(const JSThread *thread, const ObjectOperator &op,
                                                         const JSHandle<JSHClass> &hclass)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<PrototypeHandler> handler = factory->NewPrototypeHandler();
    JSHandle<JSTaggedValue> handlerInfo = StoreHandler::StoreProperty(thread, op);
    handler->SetHandlerInfo(thread, handlerInfo);
    handler->SetHolder(thread, op.GetHolder());
    if (op.IsAccessorDescriptor()) {
        JSTaggedValue result = op.GetValue();
        if (result.IsPropertyBox()) {
            result = PropertyBox::Cast(result.GetTaggedObject())->GetValue();
        }
        AccessorData *accessor = AccessorData::Cast(result.GetTaggedObject());
        if (!accessor->IsInternal() && accessor->HasSetter()) {
            JSTaggedValue setter = accessor->GetSetter();
            JSHandle<JSFunction> func(thread, setter);
            handler->SetAccessorMethodId(
                Method::Cast(func->GetMethod())->GetMethodId().GetOffset());
            handler->SetAccessorJSFunction(thread, setter);
        }
    }
    // ShareToLocal is prohibited
    if (!hclass->IsJSShared()) {
        auto result = JSHClass::EnableProtoChangeMarker(thread, hclass);
        handler->SetProtoCell(thread, result);
    }
    return JSHandle<JSTaggedValue>::Cast(handler);
}

JSHandle<JSTaggedValue> TransWithProtoHandler::StoreTransition(const JSThread *thread, const ObjectOperator &op,
                                                               const JSHandle<JSHClass> &hclass)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TransWithProtoHandler> handler = factory->NewTransWithProtoHandler();
    JSHandle<JSTaggedValue> handlerInfo = StoreHandler::StoreProperty(thread, op);
    handler->SetHandlerInfo(thread, handlerInfo);
    auto result = JSHClass::EnableProtoChangeMarker(thread, hclass);
    handler->SetProtoCell(thread, result);
    handler->SetTransitionHClass(thread, hclass.GetTaggedValue());

    return JSHandle<JSTaggedValue>::Cast(handler);
}

void HandlerBase::PrintLoadHandler([[maybe_unused]] uint64_t handler, [[maybe_unused]] std::ostream& os)
{
#if ECMASCRIPT_ENABLE_TRACE_IC
    HandlerKind kind = GetKind(handler);
    os << "kind = ";
    switch (kind) {
        case HandlerKind::NONE:
            os << "NONE"
               << ", is InlinedPropsBit = "
               << InlinedPropsBit::Get(handler)
               << ", is AccessorBit = "
               << AccessorBit::Get(handler)
               << ", OffsetBit = "
               << OffsetBit::Get(handler)
               << ", AttrIndexBit = "
               << AttrIndexBit::Get(handler);
            break;
        case HandlerKind::FIELD:
            os << "FIELD"
               << ", is InlinedPropsBit = "
               << InlinedPropsBit::Get(handler)
               << ", is AccessorBit = "
               << AccessorBit::Get(handler)
               << ", OffsetBit = "
               << OffsetBit::Get(handler)
               << ", AttrIndexBit = "
               << AttrIndexBit::Get(handler);
            break;
        case HandlerKind::ELEMENT:
            os << "ELEMENT"
               << ", is JSArray = "
               << IsJSArrayBit::Get(handler);
            break;
        case HandlerKind::DICTIONARY:
            os << "DICTIONARY";
            break;
        case HandlerKind::STRING:
            os << "STRING";
            break;
        case HandlerKind::STRING_LENGTH:
            os << "STRING_LENGTH";
            break;
        case HandlerKind::TYPED_ARRAY:
            os << "TYPED_ARRAY,"
               << "is OnHeap = "
               << IsOnHeapBit::Get(handler);
            break;
        case HandlerKind::NUMBER:
            os << "NUMBER";
            break;
        case HandlerKind::NON_EXIST:
            os << "NON_EXIST";
            break;
        default:
            UNREACHABLE();
    }
    os << std::endl;
#endif
}

void HandlerBase::PrintStoreHandler([[maybe_unused]] uint64_t handler, [[maybe_unused]] std::ostream& os)
{
#if ECMASCRIPT_ENABLE_TRACE_IC
    StoreHandlerKind kind = static_cast<StoreHandlerKind>(GetKind(handler));
    os << "kind = ";
    switch (kind) {
        case StoreHandlerKind::S_NONE:
            os << "NONE"
               << ", is InlinedPropsBit = "
               << InlinedPropsBit::Get(handler)
               << ", is AccessorBit = "
               << AccessorBit::Get(handler)
               << ", OffsetBit = "
               << OffsetBit::Get(handler)
               << ", AttrIndexBit = "
               << AttrIndexBit::Get(handler)
               << ", SFieldTypeBit = "
               << static_cast<uint32_t>(SFieldTypeBit::Get(handler));
            break;
        case StoreHandlerKind::S_FIELD:
            os << "S_FIELD"
               << ", is InlinedPropsBit = "
               << InlinedPropsBit::Get(handler)
               << ", is AccessorBit = "
               << AccessorBit::Get(handler)
               << ", OffsetBit = "
               << OffsetBit::Get(handler)
               << ", AttrIndexBit = "
               << AttrIndexBit::Get(handler)
               << ", SFieldTypeBit = "
               << static_cast<uint32_t>(SFieldTypeBit::Get(handler));
            break;
        case StoreHandlerKind::S_ELEMENT:
            os << "S_ELEMENT"
               << ", is JSArray = "
               << IsJSArrayBit::Get(handler)
               << ", SOutOfBoundsBit = "
               << SOutOfBoundsBit::Get(handler)
               << ", SFieldTypeBit = "
               << static_cast<uint32_t>(SFieldTypeBit::Get(handler));
            break;
        default:
            UNREACHABLE();
    }
    os << std::endl;
#endif
}
} // namespace panda::ecmascript