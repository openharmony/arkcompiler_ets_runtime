/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CC_EVACUATOR_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CC_EVACUATOR_H

#include "ecmascript/common.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript {
class SharedHeap;
class SharedTlabAllocator;

class SharedCCEvacuator {
public:
    explicit SharedCCEvacuator(SharedHeap *heap);
    SharedCCEvacuator(SharedHeap *heap, SharedTlabAllocator *tlab)
        : sHeap_(heap), tlab_(tlab), ownsAllocator_(false) {}
    ~SharedCCEvacuator()
    {
        if (ownsAllocator_) {
            delete tlab_;
        }
    }

    NO_COPY_SEMANTIC(SharedCCEvacuator);
    NO_MOVE_SEMANTIC(SharedCCEvacuator);

    TaggedObject* Copy(TaggedObject *fromObj, const MarkWord &markWord);

    SharedTlabAllocator *GetTlabAllocator() const
    {
        return tlab_;
    }

private:
    void RawCopyObject(uintptr_t from, uintptr_t to, size_t size, const MarkWord &markWord);
    bool SetForwardingAddress(TaggedObject *object, const MarkWord &markWord, uintptr_t forwardAddr);

    SharedHeap *sHeap_{nullptr};
    SharedTlabAllocator *tlab_{nullptr};
    bool ownsAllocator_{false};
};

class SharedCCUpdateVisitor final : public BaseObjectVisitor<SharedCCUpdateVisitor> {
public:
    SharedCCUpdateVisitor() = default;
    ~SharedCCUpdateVisitor() = default;

    void VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end,
        VisitObjectArea area) override;

    void HandleSlot(ObjectSlot slot);

private:
    void HandleInObjectArea(TaggedObject *rootObject, ObjectSlot startSlot, ObjectSlot endSlot);
};


}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CC_EVACUATOR_H
