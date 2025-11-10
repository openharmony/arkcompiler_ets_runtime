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

#ifndef ECMASCRIPT_OBJECT_FACTORY_INL_H
#define ECMASCRIPT_OBJECT_FACTORY_INL_H

#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
EcmaString *ObjectFactory::AllocLineStringObjectNoGC(size_t size)
{
    TaggedObject *object = nullptr;
    if (thread_->IsEnableCMCGC()) {
        object = sHeap_->AllocateOldOrHugeObjectNoGC(thread_, size);
    } else {
        if (size > g_maxRegularHeapObjectSize) {
            object = reinterpret_cast<TaggedObject *>(sHeap_->GetHugeObjectSpace()->Allocate(thread_, size));
        } else {
            object = reinterpret_cast<TaggedObject *>(sHeap_->GetOldSpace()->TryAllocateAndExpand(thread_, size, true));
        }
    }
    if (object == nullptr) {
        LOG_ECMA(FATAL) << "Alloc size " << size << " bytes string fail";
        UNREACHABLE();
    }
    return EcmaString::Cast(object);
}

EcmaString *ObjectFactory::AllocNonMovableLineStringObject(size_t size)
{
    NewSObjectHook();
    EcmaString* str = reinterpret_cast<EcmaString *>(sHeap_->AllocateNonMovableOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetLineStringClass().GetTaggedObject()), size));
    return str;
}

EcmaString *ObjectFactory::AllocLineStringObject(size_t size)
{
    NewSObjectHook();
    EcmaString* str = reinterpret_cast<EcmaString *>(sHeap_->AllocateOldOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetLineStringClass().GetTaggedObject()), size));
    return str;
}

EcmaString *ObjectFactory::AllocOldSpaceLineStringObject(size_t size)
{
    NewSObjectHook();
    EcmaString* str = reinterpret_cast<EcmaString *>(sHeap_->AllocateOldOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetLineStringClass().GetTaggedObject()), size));
    return str;
}

EcmaString *ObjectFactory::AllocReadOnlyLineStringObject(size_t size)
{
    NewSObjectHook();
    EcmaString* str = reinterpret_cast<EcmaString *>(sHeap_->AllocateReadOnlyOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetLineStringClass().GetTaggedObject()), size));
    return str;
}

EcmaString *ObjectFactory::AllocSlicedStringObject(MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    NewSObjectHook();
    EcmaString* str = reinterpret_cast<EcmaString *>(AllocObjectWithSpaceType(SlicedString::SIZE,
        JSHClass::Cast(thread_->GlobalConstants()->GetSlicedStringClass().GetTaggedObject()), type));
    return str;
}

EcmaString *ObjectFactory::AllocTreeStringObject()
{
    NewSObjectHook();
    EcmaString* str = reinterpret_cast<EcmaString *>(sHeap_->AllocateOldOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetTreeStringClass().GetTaggedObject()),
        TreeString::SIZE));
    return str;
}

EcmaString *ObjectFactory::AllocCachedExternalStringObject()
{
    NewSObjectHook();
    EcmaString* str = reinterpret_cast<EcmaString *>(sHeap_->AllocateOldOrHugeObject(
        thread_, JSHClass::Cast(thread_->GlobalConstants()->GetCachedExternalStringClass().GetTaggedObject()),
        CachedExternalString::SIZE));
    return str;
}

JSHandle<JSNativePointer> ObjectFactory::NewJSNativePointer(void *externalPointer,
                                                            const NativePointerCallback &callBack,
                                                            void *data,
                                                            bool nonMovable,
                                                            size_t nativeBindingsize,
                                                            Concurrent isConcurrent,
                                                            NativeFlag flag)
{
    NewObjectHook();
    TaggedObject *header;
    auto jsNativePointerClass = JSHClass::Cast(thread_->GlobalConstants()->GetJSNativePointerClass().GetTaggedObject());
    if (nonMovable) {
        header = heap_->AllocateNonMovableOrHugeObject(jsNativePointerClass);
    } else {
        header = heap_->AllocateOldOrHugeObject(jsNativePointerClass);
    }
    JSHandle<JSNativePointer> obj(thread_, header);
    obj->SetExternalPointer(externalPointer);
    obj->SetDeleter(callBack);
    obj->SetData(data);
    uint32_t fixedNativeBindingsize = nativeBindingsize < UINT32_MAX ? nativeBindingsize
                                                                     : UINT32_MAX;
    obj->SetBindingSize(fixedNativeBindingsize);
    obj->SetNativeFlag(flag);

    heap_->IncreaseNativeBindingSize(fixedNativeBindingsize);
    if (callBack != nullptr) {
        vm_->PushToNativePointerList(static_cast<JSNativePointer *>(header), isConcurrent);
        // In some cases, the size of JS/TS object is too small and the native binding size is too large.
        // Check and try trigger concurrent mark here.
        if (!g_isEnableCMCGC) {
            heap_->TryTriggerFullMarkOrGCByNativeSize();
        }
    }
    return obj;
}

LexicalEnv *ObjectFactory::InlineNewLexicalEnv(int numSlots)
{
    NewObjectHook();
    size_t size = LexicalEnv::ComputeSize(numSlots);
    auto header = heap_->TryAllocateYoungGeneration(
        JSHClass::Cast(thread_->GlobalConstants()->GetLexicalEnvClass().GetTaggedObject()), size);
    if (UNLIKELY(header == nullptr)) {
        return nullptr;
    }
    LexicalEnv *array = LexicalEnv::Cast(header);
    array->InitializeWithSpecialValue(JSTaggedValue::Hole(), numSlots + LexicalEnv::RESERVED_ENV_LENGTH);
    return array;
}

template<typename T, typename S>
void ObjectFactory::NewJSIntlIcuData(const JSHandle<T> &obj, const S &icu, const NativePointerCallback &callback)
{
    S *icuPoint = vm_->GetNativeAreaAllocator()->New<S>(icu);
    ASSERT(icuPoint != nullptr);
    JSTaggedValue data = obj->GetIcuField(vm_->GetJSThread());
    if (data.IsHeapObject() && data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(thread_, icuPoint);
        return;
    }
    JSHandle<JSNativePointer> pointer = NewJSNativePointer(icuPoint, callback, vm_);
    obj->SetIcuField(thread_, pointer.GetTaggedValue());
}

TaggedObject *ObjectFactory::AllocObjectWithSpaceType(size_t size, JSHClass *cls, MemSpaceType type)
{
    switch (type) {
        case MemSpaceType::SEMI_SPACE:
            return heap_->AllocateYoungOrHugeObject(cls, size);
        case MemSpaceType::OLD_SPACE:
            return heap_->AllocateOldOrHugeObject(cls, size);
        case MemSpaceType::NON_MOVABLE:
            return heap_->AllocateNonMovableOrHugeObject(cls, size);
        case MemSpaceType::SHARED_OLD_SPACE:
            return sHeap_->AllocateOldOrHugeObject(thread_, cls, size);
        case MemSpaceType::SHARED_NON_MOVABLE:
            return sHeap_->AllocateNonMovableOrHugeObject(thread_, cls, size);
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

template <MemSpaceType type>
JSHandle<BigInt> ObjectFactory::NewBigIntWithoutInitData(uint32_t length)
{
    NewObjectHook();
    ASSERT(length > 0);
    size_t size = BigInt::ComputeSize(length);
    TaggedObject *header;
    if (type == MemSpaceType::SHARED_READ_ONLY_SPACE) {
        header = sHeap_->AllocateReadOnlyOrHugeObject(thread_,
            JSHClass::Cast(thread_->GlobalConstants()->GetBigIntClass().GetTaggedObject()), size);
    } else {
        header = sHeap_->AllocateOldOrHugeObject(thread_,
            JSHClass::Cast(thread_->GlobalConstants()->GetBigIntClass().GetTaggedObject()), size);
    }
    JSHandle<BigInt> bigint(thread_, header);
    bigint->SetLength(length);
    return bigint;
}

template <MemSpaceType type>
JSHandle<BigInt> ObjectFactory::NewBigInt(uint32_t length)
{
    JSHandle<BigInt> bigint = NewBigIntWithoutInitData<type>(length);
    bigint->InitializationZero();
    bigint->SetSign(false);
    return bigint;
}

template <MemSpaceType type>
JSHandle<BigInt> ObjectFactory::NewSubBigInt(const JSHandle<BigInt>& x, uint32_t length)
{
    JSHandle<BigInt> bigint = NewBigIntWithoutInitData<type>(length);
    bigint->SetSign(x->GetSign());
    ASSERT(x->GetLength() >= length);
    std::copy(x->GetData(), x->GetData() + length, bigint->GetData());
    return bigint;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_OBJECT_FACTORY_INL_H
