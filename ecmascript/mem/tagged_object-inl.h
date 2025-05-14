/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TAGGED_OBJECT_HEADER_INL_H
#define ECMASCRIPT_TAGGED_OBJECT_HEADER_INL_H

#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/tagged_object.h"

#include <atomic>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"

namespace panda::ecmascript {
#ifdef USE_CMC_GC
inline void TaggedObject::SetClassWithoutBarrier(JSHClass *hclass)
{
    state_ = 0;
    TransitionClassWithoutBarrier(hclass);
}

inline void TaggedObject::TransitionClassWithoutBarrier(JSHClass *hclass)
{
    reinterpret_cast<TaggedStateWord *>(this)->SetClass(reinterpret_cast<uintptr_t>(hclass));
}

inline void TaggedObject::SetFreeObjectClass(JSHClass *hclass)
{
    ASSERT(hclass->IsFreeObject());
    StateWordType state = static_cast<StateWordType>(ToUintPtr(hclass));
    reinterpret_cast<TaggedStateWord *>(this)->SynchronizedSetGCStateWord(state);
}

inline void TaggedObject::SetClass(const JSThread *thread, JSHClass *hclass)
{
#ifndef ARK_USE_SATB_BARRIER
    SetClassWithoutBarrier(hclass);
    WriteBarrier(thread, this, HCLASS_OFFSET, JSTaggedValue(hclass).GetRawData());
#else
    WriteBarrier(thread, this, HCLASS_OFFSET, JSTaggedValue(hclass).GetRawData());
    SetClassWithoutBarrier(hclass);
#endif
}

inline void TaggedObject::SynchronizedTransitionClass(const JSThread *thread, JSHClass *hclass)
{
#ifndef ARK_USE_SATB_BARRIER
    reinterpret_cast<TaggedStateWord *>(this)->SynchronizedSetClass(reinterpret_cast<uintptr_t>(hclass));
    WriteBarrier(thread, this, HCLASS_OFFSET, JSTaggedValue(hclass).GetRawData());
#else
    WriteBarrier(thread, this, HCLASS_OFFSET, JSTaggedValue(hclass).GetRawData());
    reinterpret_cast<TaggedStateWord *>(this)->SynchronizedSetClass(reinterpret_cast<uintptr_t>(hclass));
#endif
}

inline JSHClass *TaggedObject::SynchronizedGetClass() const
{
    return reinterpret_cast<JSHClass *>(reinterpret_cast<const TaggedStateWord *>(this)->SynchronizedGetClass());
}

inline bool TaggedObject::IsInSharedHeap() const
{
    return GetClass()->IsJSShared();
}
#else
inline void TaggedObject::SetClassWithoutBarrier(JSHClass *hclass)
{
    class_ = reinterpret_cast<MarkWordType>(hclass);
}

inline void TaggedObject::TransitionClassWithoutBarrier(JSHClass *hclass)
{
    SetClassWithoutBarrier(hclass);
}

inline void TaggedObject::SetFreeObjectClass(JSHClass *hclass)
{
    ASSERT(hclass->IsFreeObject());
    SetClassWithoutBarrier(hclass);
}

inline void TaggedObject::SetClass(const JSThread *thread, JSHClass *hclass)
{
    Barriers::SetObject<true>(thread, this, HCLASS_OFFSET, JSTaggedValue(hclass).GetRawData());
}

inline void TaggedObject::SynchronizedTransitionClass(const JSThread *thread, JSHClass *hclass)
{
    reinterpret_cast<volatile std::atomic<MarkWordType> *>(this)->
        store(reinterpret_cast<MarkWordType>(hclass), std::memory_order_release);
    WriteBarrier(thread, this, HCLASS_OFFSET, reinterpret_cast<MarkWordType>(hclass));
}

inline JSHClass *TaggedObject::SynchronizedGetClass() const
{
    return reinterpret_cast<JSHClass *>(
        reinterpret_cast<volatile std::atomic<MarkWordType> *>(ToUintPtr(this))->load(std::memory_order_acquire));
}
#endif
}  //  namespace panda::ecmascript

#endif  // ECMASCRIPT_TAGGED_OBJECT_HEADER_INL_H
