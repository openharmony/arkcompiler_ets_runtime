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

#ifndef ECMASCRIPT_IC_IC_HANDLER_H
#define ECMASCRIPT_IC_IC_HANDLER_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/mem/tagged_object.h"

namespace panda::ecmascript {
class HandlerBase {
public:
    static constexpr uint32_t KIND_BIT_LENGTH = 3;
    static constexpr uint32_t STORE_KIND_BIT_LENGTH = 2;
    enum HandlerKind {
        NONE = 0,
        FIELD,
        ELEMENT,
        DICTIONARY,
        STRING,
        STRING_LENGTH,
        TYPED_ARRAY,
        NON_EXIST,
        TOTAL_KINDS,
    };

    // Store Handler kind combined with KindBit called SWholeKindBit. Which used to quickly check S_FIELD kind
    enum StoreHandlerKind {
        S_NONE = HandlerKind::NONE,
        S_FIELD,
        S_ELEMENT,
        S_TOTAL_KINDS,
    };

    // For Load
    using KindBit = BitField<HandlerKind, 0, KIND_BIT_LENGTH>;
    using InlinedPropsBit = KindBit::NextFlag;
    using AccessorBit = InlinedPropsBit::NextFlag;
    using InternalAccessorBit = AccessorBit::NextFlag;
    using IsJSArrayBit = InternalAccessorBit::NextFlag;
    using OffsetBit = IsJSArrayBit::NextField<uint32_t, PropertyAttributes::OFFSET_BITFIELD_NUM>;
    using RepresentationBit = OffsetBit::NextField<Representation, PropertyAttributes::REPRESENTATION_NUM>;
    using AttrIndexBit = RepresentationBit::NextField<uint32_t, PropertyAttributes::OFFSET_BITFIELD_NUM>;
    using IsOnHeapBit = AttrIndexBit::NextFlag;
    using NeedSkipInPGODumpBit = IsOnHeapBit::NextFlag;
    static_assert(static_cast<size_t>(HandlerKind::TOTAL_KINDS) <= (1 << KIND_BIT_LENGTH));

    // For Store
    using SKindBit = BitField<StoreHandlerKind, 0, STORE_KIND_BIT_LENGTH>;
    using SSharedBit = SKindBit::NextFlag;
    using SWholeKindBit = KindBit;
    static_assert(SKindBit::START_BIT == SWholeKindBit::START_BIT);
    static_assert(SSharedBit::Mask() || SKindBit::Mask() == KindBit::Mask());
    using STrackTypeBit = AttrIndexBit::NextField<uint32_t, PropertyAttributes::TRACK_TYPE_NUM>;
    static_assert(static_cast<size_t>(StoreHandlerKind::S_TOTAL_KINDS) <= (1 << STORE_KIND_BIT_LENGTH));

    HandlerBase() = default;
    virtual ~HandlerBase() = default;

    static inline bool IsAccessor(uint32_t handler)
    {
        return AccessorBit::Get(handler);
    }

    static inline bool IsInternalAccessor(uint32_t handler)
    {
        return InternalAccessorBit::Get(handler);
    }

    static inline TrackType GetTrackType(uint32_t handler)
    {
        return static_cast<TrackType>(STrackTypeBit::Get(handler));
    }

    static inline bool IsNonExist(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::NON_EXIST;
    }

    static inline bool IsField(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::FIELD;
    }

    static inline bool IsNonSharedStoreField(uint32_t handler)
    {
        return static_cast<StoreHandlerKind>(GetKind(handler)) == StoreHandlerKind::S_FIELD;
    }

    static inline bool IsStoreShared(uint32_t handler)
    {
        return SSharedBit::Get(handler);
    }

    static inline void ClearSharedStoreKind(uint32_t &handler)
    {
        SSharedBit::Set<uint32_t>(false, &handler);
    }

    static inline bool IsString(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::STRING;
    }

    static inline bool IsStringLength(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::STRING_LENGTH;
    }

    static inline bool IsElement(uint32_t handler)
    {
        return IsNormalElement(handler) || IsStringElement(handler) || IsTypedArrayElement(handler);
    }

    static inline bool IsNormalElement(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::ELEMENT;
    }

    static inline bool IsStringElement(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::STRING;
    }

    static inline bool IsTypedArrayElement(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::TYPED_ARRAY;
    }

    static inline bool IsDictionary(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::DICTIONARY;
    }

    static inline bool IsInlinedProps(uint32_t handler)
    {
        return InlinedPropsBit::Get(handler);
    }

    static inline HandlerKind GetKind(uint32_t handler)
    {
        return KindBit::Get(handler);
    }

    static inline bool IsJSArray(uint32_t handler)
    {
        return IsJSArrayBit::Get(handler);
    }

    static inline bool NeedSkipInPGODump(uint32_t handler)
    {
        return NeedSkipInPGODumpBit::Get(handler);
    }

    static inline int GetOffset(uint32_t handler)
    {
        return OffsetBit::Get(handler);
    }

    static inline bool IsOnHeap(uint32_t handler)
    {
        return IsOnHeapBit::Get(handler);
    }
};

class LoadHandler final : public HandlerBase {
public:
    static inline JSHandle<JSTaggedValue> LoadProperty(const JSThread *thread, const ObjectOperator &op)
    {
        uint32_t handler = 0;
        ASSERT(!op.IsElement());
        if (!op.IsFound()) {
            KindBit::Set<uint32_t>(HandlerKind::NON_EXIST, &handler);
            return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
        }
        ASSERT(op.IsFastMode());

        JSTaggedValue val = op.GetValue();
        if (val.IsPropertyBox()) {
            return JSHandle<JSTaggedValue>(thread, val);
        }
        bool hasAccessor = op.IsAccessorDescriptor();
        AccessorBit::Set<uint32_t>(hasAccessor, &handler);

        if (!hasAccessor) {
            if (op.GetReceiver()->IsString()) {
                JSTaggedValue lenKey = thread->GlobalConstants()->GetLengthString();
                EcmaString *proKey = nullptr;
                if (op.GetKey()->IsString()) {
                    proKey = EcmaString::Cast(op.GetKey()->GetTaggedObject());
                }
                if (EcmaStringAccessor::StringsAreEqual(proKey, EcmaString::Cast(lenKey.GetTaggedObject()))) {
                    KindBit::Set<uint32_t>(HandlerKind::STRING_LENGTH, &handler);
                } else {
                    KindBit::Set<uint32_t>(HandlerKind::STRING, &handler);
                }
            } else {
                KindBit::Set<uint32_t>(HandlerKind::FIELD, &handler);
            }
        }

        if (op.IsInlinedProps()) {
            InlinedPropsBit::Set<uint32_t>(true, &handler);
            JSHandle<JSObject> holder = JSHandle<JSObject>::Cast(op.GetHolder());
            auto index = holder->GetJSHClass()->GetInlinedPropertiesIndex(op.GetIndex());
            OffsetBit::Set<uint32_t>(index, &handler);
            AttrIndexBit::Set<uint32_t>(op.GetIndex(), &handler);
            RepresentationBit::Set(op.GetRepresentation(), &handler);
            return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
        }
        if (op.IsFastMode()) {
            JSHandle<JSObject> holder = JSHandle<JSObject>::Cast(op.GetHolder());
            uint32_t inlinePropNum = holder->GetJSHClass()->GetInlinedProperties();
            AttrIndexBit::Set<uint32_t>(op.GetIndex() + inlinePropNum, &handler);
            OffsetBit::Set<uint32_t>(op.GetIndex(), &handler);
            RepresentationBit::Set(Representation::TAGGED, &handler);
            return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
        }
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }

    static inline JSHandle<JSTaggedValue> LoadElement(const JSThread *thread, const ObjectOperator &op)
    {
        uint32_t handler = 0;
        KindBit::Set<uint32_t>(HandlerKind::ELEMENT, &handler);

        // To avoid logical errors and Deopt, temporarily skipping PGO Profiling.
        // logical errors:
        //     When accessing an element of an object, AOT does not have a chain-climbing operation,
        //     so if the element is on a prototype, it will not be able to get the correct element.
        // deopt:
        //     Currently there is no way to save the type of the key in pgo file, even if the type of the key
        //     is string, it will be treated as a number type by the AOT, leading to deopt at runtime.
        if (op.GetReceiver() != op.GetHolder() ||
            op.KeyFromStringType()) {
            NeedSkipInPGODumpBit::Set<uint32_t>(true, &handler);
        }

        if (op.GetReceiver()->IsJSArray()) {
            IsJSArrayBit::Set<uint32_t>(true, &handler);
        }
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
    }

    static inline JSHandle<JSTaggedValue> LoadStringElement(const JSThread *thread)
    {
        uint32_t handler = 0;
        KindBit::Set<uint32_t>(HandlerKind::STRING, &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
    }

    static inline JSHandle<JSTaggedValue> LoadTypedArrayElement(const JSThread *thread,
                                                                JSHandle<JSTypedArray> typedArray)
    {
        uint32_t handler = 0;
        KindBit::Set<uint32_t>(HandlerKind::TYPED_ARRAY, &handler);
        IsOnHeapBit::Set<uint32_t>(JSHandle<TaggedObject>(typedArray)->GetClass()->IsOnHeapFromBitField(), &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
    }
};

class StoreHandler final : public HandlerBase {
public:
    static inline JSHandle<JSTaggedValue> StoreProperty(const JSThread *thread, const ObjectOperator &op)
    {
        uint32_t handler = 0;
        JSHandle<JSObject> receiver = JSHandle<JSObject>::Cast(op.GetReceiver());
        SSharedBit::Set<uint32_t>(op.GetReceiver()->IsJSShared(), &handler);
        TaggedArray *array = TaggedArray::Cast(receiver->GetProperties().GetTaggedObject());
        if (!array->IsDictionaryMode()) {
            STrackTypeBit::Set(static_cast<uint32_t>(op.GetAttr().GetTrackType()), &handler);
        } else {
            STrackTypeBit::Set(static_cast<uint32_t>(op.GetAttr().GetDictTrackType()), &handler);
        }
        if (op.IsElement()) {
            return StoreElement(thread, op.GetReceiver(), handler);
        }
        JSTaggedValue val = op.GetValue();
        if (val.IsPropertyBox()) {
            return JSHandle<JSTaggedValue>(thread, val);
        }
        bool hasSetter = op.IsAccessorDescriptor();
        AccessorBit::Set<uint32_t>(hasSetter, &handler);
        if (!hasSetter) {
            SKindBit::Set<uint32_t>(StoreHandlerKind::S_FIELD, &handler);
        }
        if (op.IsInlinedProps()) {
            InlinedPropsBit::Set<uint32_t>(true, &handler);
            uint32_t index = 0;
            if (!hasSetter) {
                index = receiver->GetJSHClass()->GetInlinedPropertiesIndex(op.GetIndex());
            } else {
                JSHandle<JSObject> holder = JSHandle<JSObject>::Cast(op.GetHolder());
                index = holder->GetJSHClass()->GetInlinedPropertiesIndex(op.GetIndex());
            }
            AttrIndexBit::Set<uint32_t>(op.GetIndex(), &handler);
            OffsetBit::Set<uint32_t>(index, &handler);
            RepresentationBit::Set(op.GetRepresentation(), &handler);
            return JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<int32_t>(handler)));
        }
        ASSERT(op.IsFastMode());
        uint32_t inlinePropNum = receiver->GetJSHClass()->GetInlinedProperties();
        AttrIndexBit::Set<uint32_t>(op.GetIndex() + inlinePropNum, &handler);
        OffsetBit::Set<uint32_t>(op.GetIndex(), &handler);
        RepresentationBit::Set(Representation::TAGGED, &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<int32_t>(handler)));
    }

    static inline JSHandle<JSTaggedValue> StoreElement(const JSThread *thread,
                                                       JSHandle<JSTaggedValue> receiver, uint32_t handler)
    {
        SKindBit::Set<uint32_t>(StoreHandlerKind::S_ELEMENT, &handler);

        if (receiver->IsJSArray()) {
            IsJSArrayBit::Set<uint32_t>(true, &handler);
        }
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<int32_t>(handler)));
    }
};

class TransitionHandler : public TaggedObject {
public:
    static TransitionHandler *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTransitionHandler());
        return static_cast<TransitionHandler *>(object);
    }

    static inline JSHandle<JSTaggedValue> StoreTransition(const JSThread *thread, const ObjectOperator &op)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<TransitionHandler> handler = factory->NewTransitionHandler();
        JSHandle<JSTaggedValue> handlerInfo = StoreHandler::StoreProperty(thread, op);
        handler->SetHandlerInfo(thread, handlerInfo);
        auto hclass = JSObject::Cast(op.GetReceiver()->GetTaggedObject())->GetJSHClass();
        handler->SetTransitionHClass(thread, JSTaggedValue(hclass));
        return JSHandle<JSTaggedValue>::Cast(handler);
    }

    static constexpr size_t HANDLER_INFO_OFFSET = TaggedObjectSize();

    ACCESSORS(HandlerInfo, HANDLER_INFO_OFFSET, TRANSITION_HCLASS_OFFSET)

    ACCESSORS(TransitionHClass, TRANSITION_HCLASS_OFFSET, SIZE)

    DECL_VISIT_OBJECT(HANDLER_INFO_OFFSET, SIZE)
    DECL_DUMP()
};

class PrototypeHandler : public TaggedObject {
public:
    static PrototypeHandler *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsPrototypeHandler());
        return static_cast<PrototypeHandler *>(object);
    }

    static inline JSHandle<JSTaggedValue> LoadPrototype(const JSThread *thread, const ObjectOperator &op,
                                                        const JSHandle<JSHClass> &hclass)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> handlerInfo = LoadHandler::LoadProperty(thread, op);
        JSHandle<PrototypeHandler> handler = factory->NewPrototypeHandler();
        handler->SetHandlerInfo(thread, handlerInfo);
        if (op.IsFound()) {
            handler->SetHolder(thread, op.GetHolder());
        }
        auto result = JSHClass::EnableProtoChangeMarker(thread, hclass);
        handler->SetProtoCell(thread, result);
        return JSHandle<JSTaggedValue>::Cast(handler);
    }
    static inline JSHandle<JSTaggedValue> StorePrototype(const JSThread *thread, const ObjectOperator &op,
                                                         const JSHandle<JSHClass> &hclass)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<PrototypeHandler> handler = factory->NewPrototypeHandler();
        JSHandle<JSTaggedValue> handlerInfo = StoreHandler::StoreProperty(thread, op);
        handler->SetHandlerInfo(thread, handlerInfo);
        handler->SetHolder(thread, op.GetHolder());
        auto result = JSHClass::EnableProtoChangeMarker(thread, hclass);
        handler->SetProtoCell(thread, result);
        return JSHandle<JSTaggedValue>::Cast(handler);
    }

    static constexpr size_t HANDLER_INFO_OFFSET = TaggedObjectSize();

    ACCESSORS(HandlerInfo, HANDLER_INFO_OFFSET, PROTO_CELL_OFFSET)

    ACCESSORS(ProtoCell, PROTO_CELL_OFFSET, HOLDER_OFFSET)

    ACCESSORS(Holder, HOLDER_OFFSET, SIZE)

    DECL_VISIT_OBJECT(HANDLER_INFO_OFFSET, SIZE)
    DECL_DUMP()
};

class TransWithProtoHandler : public TaggedObject {
public:
    static TransWithProtoHandler *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTransWithProtoHandler());
        return static_cast<TransWithProtoHandler *>(object);
    }

    static inline JSHandle<JSTaggedValue> StoreTransition(const JSThread *thread, const ObjectOperator &op,
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

    static constexpr size_t HANDLER_INFO_OFFSET = TaggedObjectSize();

    ACCESSORS(HandlerInfo, HANDLER_INFO_OFFSET, TRANSITION_HCLASS_OFFSET)

    ACCESSORS(TransitionHClass, TRANSITION_HCLASS_OFFSET, PROTO_CELL_OFFSET)

    ACCESSORS(ProtoCell, PROTO_CELL_OFFSET, SIZE)

    DECL_VISIT_OBJECT(HANDLER_INFO_OFFSET, SIZE)
    DECL_DUMP()
};

class StoreTSHandler : public TaggedObject {
public:
    static StoreTSHandler *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsStoreTSHandler());
        return static_cast<StoreTSHandler *>(object);
    }

    static inline JSHandle<JSTaggedValue> StoreAOT(const JSThread *thread, const ObjectOperator &op,
                                                   const JSHandle<JSHClass> &hclass)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<StoreTSHandler> handler = factory->NewStoreTSHandler();
        JSHandle<JSTaggedValue> handlerInfo = StoreHandler::StoreProperty(thread, op);
        handler->SetHandlerInfo(thread, handlerInfo);
        handler->SetHolder(thread, op.GetHolder());
        auto result = JSHClass::EnableProtoChangeMarker(thread, hclass);
        handler->SetProtoCell(thread, result);
        return JSHandle<JSTaggedValue>::Cast(handler);
    }

    static constexpr size_t HANDLER_INFO_OFFSET = TaggedObjectSize();

    ACCESSORS(HandlerInfo, HANDLER_INFO_OFFSET, PROTO_CELL_OFFSET)

    ACCESSORS(ProtoCell, PROTO_CELL_OFFSET, HOLDER_OFFSET)

    ACCESSORS(Holder, HOLDER_OFFSET, SIZE)

    DECL_VISIT_OBJECT(HANDLER_INFO_OFFSET, SIZE)
    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_IC_IC_HANDLER_H
