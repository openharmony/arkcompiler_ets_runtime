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

#ifndef ECMASCRIPT_IC_IC_HANDLER_H
#define ECMASCRIPT_IC_IC_HANDLER_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript {
class HandlerBase {
public:
    static constexpr uint32_t KIND_BIT_LENGTH = 4;
    static constexpr uint32_t STORE_KIND_BIT_LENGTH = 2;
    static constexpr uint32_t MAX_BIT_SIZE = 48;
    enum HandlerKind {
        NONE = 0,
        FIELD,
        ELEMENT,
        DICTIONARY,
        STRING,
        STRING_LENGTH,
        TYPED_ARRAY,
        NUMBER,
        NON_EXIST,
        TOTAL_KINDS,
    };
    static_assert(static_cast<size_t>(HandlerKind::TOTAL_KINDS) <= (1 << KIND_BIT_LENGTH));

    // Store Handler kind combined with KindBit called SWholeKindBit. Which used to quickly check S_FIELD kind
    enum StoreHandlerKind {
        S_NONE = HandlerKind::NONE,
        S_FIELD,
        S_ELEMENT,
        S_TOTAL_KINDS,
    };
    static_assert(static_cast<size_t>(StoreHandlerKind::S_TOTAL_KINDS) <= (1 << STORE_KIND_BIT_LENGTH));

    // For Load
    using KindBit = BitField<HandlerKind, 0, KIND_BIT_LENGTH>;                                              // [0, 4)
    using InlinedPropsBit = KindBit::NextFlag;                                                              // [4, 5)
    using AccessorBit = InlinedPropsBit::NextFlag;                                                          // [5, 6)
    using IsJSArrayBit = AccessorBit::NextFlag;                                                             // [6, 7)
    using OffsetBit = IsJSArrayBit::NextField<uint32_t, PropertyAttributes::OFFSET_BITFIELD_NUM>;           // [7, 17)
    using RepresentationBit = OffsetBit::NextField<Representation, PropertyAttributes::REPRESENTATION_NUM>; // [17, 19)
    using AttrIndexBit = RepresentationBit::NextField<uint32_t, PropertyAttributes::OFFSET_BITFIELD_NUM>;   // [19, 29)
    using IsOnHeapBit = AttrIndexBit::NextFlag;                                                             // [29, 30)
    using NeedSkipInPGODumpBit  = IsOnHeapBit::NextFlag;                                                    // [30, 31)
    static_assert(NeedSkipInPGODumpBit::END_BIT <= MAX_BIT_SIZE, "load handler overflow");

    // For Store
    using SWholeKindBit = KindBit;
    using SKindBit = BitField<StoreHandlerKind, 0, STORE_KIND_BIT_LENGTH>;                                  // [0, 2)
    static_assert(SKindBit::START_BIT == KindBit::START_BIT);
    using SSharedBit = SKindBit::NextFlag;                                                                  // [2, 4)
    static_assert((SKindBit::SIZE + SSharedBit::SIZE) <= KindBit::SIZE);              // reuse: [0, 4) bits
                                                                                      // shared with Load bits: [4, 30)
    using SOutOfBoundsBit = IsOnHeapBit::NextFlag;                                    // reuse: [30, 31) bit
    using SFieldTypeBit = SOutOfBoundsBit::NextField<SharedFieldType, PropertyAttributes::FIELD_TYPE_NUM>;  // [31, 39)
    static_assert(SFieldTypeBit::END_BIT <= MAX_BIT_SIZE, "store handler overflow");
    using Type = uint64_t;
    static_assert(sizeof(Type) <= JSTaggedValue::TaggedTypeSize());

    HandlerBase() = default;
    virtual ~HandlerBase() = default;

    static inline bool IsAccessor(Type handler)
    {
        return AccessorBit::Get(handler);
    }

    static inline SharedFieldType GetFieldType(Type handler)
    {
        return static_cast<SharedFieldType>(SFieldTypeBit::Get(handler));
    }

    static inline bool IsNonExist(Type handler)
    {
        return GetKind(handler) == HandlerKind::NON_EXIST;
    }

    static inline bool IsField(Type handler)
    {
        return GetKind(handler) == HandlerKind::FIELD;
    }

    static inline bool IsNonSharedStoreField(Type handler)
    {
        return static_cast<StoreHandlerKind>(GetKind(handler)) == StoreHandlerKind::S_FIELD;
    }

    static inline bool IsStoreShared(Type handler)
    {
        return SSharedBit::Get(handler);
    }

    static inline void ClearSharedStoreKind(Type &handler)
    {
        SSharedBit::Set<Type>(false, &handler);
    }

    static inline bool IsStoreOutOfBounds(Type handler)
    {
        return SOutOfBoundsBit::Get(handler);
    }

    static inline void ClearStoreOutOfBounds(Type &handler)
    {
        SOutOfBoundsBit::Set<Type>(false, &handler);
    }

    static inline bool IsString(Type handler)
    {
        return GetKind(handler) == HandlerKind::STRING;
    }

    static inline bool IsNumber(Type handler)
    {
        return GetKind(handler) == HandlerKind::NUMBER;
    }

    static inline bool IsStringLength(Type handler)
    {
        return GetKind(handler) == HandlerKind::STRING_LENGTH;
    }

    static inline bool IsElement(Type handler)
    {
        return IsNormalElement(handler) || IsStringElement(handler) || IsTypedArrayElement(handler);
    }

    static inline bool IsNormalElement(Type handler)
    {
        return GetKind(handler) == HandlerKind::ELEMENT;
    }

    static inline bool IsStringElement(Type handler)
    {
        return GetKind(handler) == HandlerKind::STRING;
    }

    static inline bool IsTypedArrayElement(Type handler)
    {
        return GetKind(handler) == HandlerKind::TYPED_ARRAY;
    }

    static inline bool IsDictionary(Type handler)
    {
        return GetKind(handler) == HandlerKind::DICTIONARY;
    }

    static inline bool IsInlinedProps(Type handler)
    {
        return InlinedPropsBit::Get(handler);
    }

    static inline HandlerKind GetKind(Type handler)
    {
        return KindBit::Get(handler);
    }

    static inline bool IsJSArray(Type handler)
    {
        return IsJSArrayBit::Get(handler);
    }

    static inline bool NeedSkipInPGODump(Type handler)
    {
        return NeedSkipInPGODumpBit::Get(handler);
    }

    static inline int GetOffset(Type handler)
    {
        return OffsetBit::Get(handler);
    }

    static inline bool IsOnHeap(Type handler)
    {
        return IsOnHeapBit::Get(handler);
    }
};

class LoadHandler final : public HandlerBase {
public:
    static inline JSHandle<JSTaggedValue> LoadProperty(const JSThread *thread, const ObjectOperator &op)
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
            if (op.GetReceiver()->IsString()) {
                JSTaggedValue lenKey = thread->GlobalConstants()->GetLengthString();
                EcmaString *proKey = nullptr;
                if (op.GetKey()->IsString()) {
                    proKey = EcmaString::Cast(op.GetKey()->GetTaggedObject());
                }
                if (EcmaStringAccessor::StringsAreEqual(proKey, EcmaString::Cast(lenKey.GetTaggedObject()))) {
                    KindBit::Set<uint64_t>(HandlerKind::STRING_LENGTH, &handler);
                } else {
                    KindBit::Set<uint64_t>(HandlerKind::STRING, &handler);
                }
            } else if (op.GetReceiver()->IsNumber()) {
                KindBit::Set<uint64_t>(HandlerKind::NUMBER, &handler);
            } else {
                KindBit::Set<uint64_t>(HandlerKind::FIELD, &handler);
            }
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

    static inline JSHandle<JSTaggedValue> LoadElement(const JSThread *thread, const ObjectOperator &op)
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
        if (op.GetReceiver() != op.GetHolder() ||
            op.KeyFromStringType()) {
            NeedSkipInPGODumpBit::Set<uint64_t>(true, &handler);
        }

        if (op.GetReceiver()->IsJSArray()) {
            IsJSArrayBit::Set<uint64_t>(true, &handler);
        }
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::WrapUint64(handler));
    }

    static inline JSHandle<JSTaggedValue> LoadStringElement(const JSThread *thread)
    {
        uint64_t handler = 0;
        KindBit::Set<uint64_t>(HandlerKind::STRING, &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::WrapUint64(handler));
    }

    static inline JSHandle<JSTaggedValue> LoadTypedArrayElement(const JSThread *thread,
                                                                JSHandle<JSTypedArray> typedArray)
    {
        uint64_t handler = 0;
        KindBit::Set<uint64_t>(HandlerKind::TYPED_ARRAY, &handler);
        IsOnHeapBit::Set<uint64_t>(JSHandle<TaggedObject>(typedArray)->GetClass()->IsOnHeapFromBitField(), &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::WrapUint64(handler));
    }
};

class StoreHandler final : public HandlerBase {
public:
    static inline JSHandle<JSTaggedValue> StoreProperty(const JSThread *thread, const ObjectOperator &op)
    {
        uint64_t handler = 0;
        JSHandle<JSObject> receiver = JSHandle<JSObject>::Cast(op.GetReceiver());
        SSharedBit::Set<uint64_t>(op.GetReceiver()->IsJSShared(), &handler);
        TaggedArray *array = TaggedArray::Cast(receiver->GetProperties().GetTaggedObject());
        if (!array->IsDictionaryMode()) {
            SFieldTypeBit::Set<uint64_t>(op.GetAttr().GetSharedFieldType(), &handler);
        } else {
            SFieldTypeBit::Set<uint64_t>(op.GetAttr().GetDictSharedFieldType(), &handler);
        }
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

    static inline JSHandle<JSTaggedValue> StoreElement(const JSThread *thread,
                                                       JSHandle<JSTaggedValue> receiver, uint64_t handler)
    {
        SKindBit::Set<uint64_t>(StoreHandlerKind::S_ELEMENT, &handler);

        if (receiver->IsJSArray()) {
            IsJSArrayBit::Set<uint64_t>(true, &handler);
        }
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::WrapUint64(handler));
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
        // ShareToLocal is prohibited
        if (!hclass->IsJSShared()) {
            auto result = JSHClass::EnableProtoChangeMarker(thread, hclass);
            handler->SetProtoCell(thread, result);
        }
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

    static constexpr size_t HANDLER_INFO_OFFSET = TaggedObjectSize();

    ACCESSORS(HandlerInfo, HANDLER_INFO_OFFSET, PROTO_CELL_OFFSET)

    ACCESSORS(ProtoCell, PROTO_CELL_OFFSET, HOLDER_OFFSET)

    ACCESSORS(Holder, HOLDER_OFFSET, ACCESSOR_JSFUNCTION_OFFSET)
    ACCESSORS(AccessorJSFunction, ACCESSOR_JSFUNCTION_OFFSET, ACCESSOR_METHOD_ID_OFFSET)

    ACCESSORS_PRIMITIVE_FIELD(AccessorMethodId, uint32_t, ACCESSOR_METHOD_ID_OFFSET, LAST_OFFSET)

    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    DECL_VISIT_OBJECT(HANDLER_INFO_OFFSET, ACCESSOR_METHOD_ID_OFFSET)
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
