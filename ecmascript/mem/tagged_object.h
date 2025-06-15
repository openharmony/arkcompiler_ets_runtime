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

#ifndef ECMASCRIPT_TAGGED_OBJECT_HEADER_H
#define ECMASCRIPT_TAGGED_OBJECT_HEADER_H

#include "common_interfaces/base/mem.h"
#include "common_interfaces/objects/base_object.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/shared_heap/shared_value_helper.h"
#include "ecmascript/mem/tagged_state_word.h"

namespace panda::ecmascript {
class JSHClass;
template<typename T>
class JSHandle;
class JSThread;

using ::common::BaseObject;
using ::common::ToUintPtr;
using ::common::ToVoidPtr;

class TaggedObject : public BaseObject {
public:
    static TaggedObject *Cast(const BaseObject *header)
    {
        return static_cast<TaggedObject *>(const_cast<BaseObject *>(header));
    }

    static TaggedObject *Cast(TaggedObject *header)
    {
        return static_cast<TaggedObject *>(header);
    }
    TaggedObject() = default;

    void SynchronizedTransitionClass(const JSThread *thread, JSHClass *hclass);
    void SetClassWithoutBarrier(JSHClass *hclass);
    void SetFreeObjectClass(JSHClass *hclass);
    void TransitionClassWithoutBarrier(JSHClass *hclass);

    JSHClass *SynchronizedGetClass() const;
    void SetForwardingPointerAfterExclusive(BaseObject *fwdPtr)
    {
        reinterpret_cast<TaggedStateWord *>(this)->SetForwardingAddress(reinterpret_cast<uintptr_t>(fwdPtr));
    }

    BaseObject *GetForwardingPointer() const
    {
        return reinterpret_cast<BaseObject *>(reinterpret_cast<const TaggedStateWord *>(this)->GetForwardingAddress());
    }

    JSHClass *GetClass() const
    {
        return reinterpret_cast<JSHClass *>(reinterpret_cast<const TaggedStateWord *>(this)->GetClass());
    }

    size_t GetSize();

    bool IsInSharedHeap() const;

    // Size of object header
    static constexpr size_t TaggedObjectSize()
    {
        return sizeof(TaggedObject);
    }

    static constexpr uint64_t GC_STATE_MASK = 0x0FFFFFFFFFFFFFFF;
    static constexpr int HCLASS_OFFSET = 0;
    static constexpr int SIZE = sizeof(TaggedStateWord);

private:
    void SetClass(const JSThread *thread, JSHClass *hclass);

    friend class BaseHeap;
    friend class Heap;
    friend class SharedHeap;
    friend class ObjectFactory;
    friend class EcmaString;
};
static_assert(TaggedObject::TaggedObjectSize() == sizeof(TaggedStateWord));
}  //  namespace panda::ecmascript

#endif  // ECMASCRIPT_TAGGED_OBJECT_HEADER_H
