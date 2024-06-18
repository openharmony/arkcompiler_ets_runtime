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

#ifndef ECMASCRIPT_MEM_HEAP_VERIFICATION_H
#define ECMASCRIPT_MEM_HEAP_VERIFICATION_H

#include <cstdint>

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/slots.h"

namespace panda::ecmascript {
static constexpr uint32_t INVALID_THRESHOLD = 0x40000;

class VerifyScope {
public:
    VerifyScope(BaseHeap *heap) : heap_(heap)
    {
        heap_->SetVerifying(true);
    }

    ~VerifyScope()
    {
        heap_->SetVerifying(false);
    }
private:
    BaseHeap *heap_ {nullptr};
};

// Verify the object body
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
class VerifyObjectVisitor {
public:
    // Only used for verify InactiveSemiSpace
    static void VerifyInactiveSemiSpaceMarkedObject(const BaseHeap *heap, void *addr);

    VerifyObjectVisitor(const BaseHeap *heap, size_t *failCount,
                        VerifyKind verifyKind = VerifyKind::VERIFY_PRE_GC)
        : heap_(heap), failCount_(failCount), verifyKind_(verifyKind)
    {
    }
    ~VerifyObjectVisitor() = default;

    void operator()(TaggedObject *obj)
    {
        VisitAllObjects(obj);
    }

    void operator()(TaggedObject *obj, JSTaggedValue value);

    size_t GetFailedCount() const
    {
        return *failCount_;
    }

private:
    void VisitAllObjects(TaggedObject *obj);
    void VerifyObjectSlotLegal(ObjectSlot slot, TaggedObject *obj) const;
    void VerifyHeapObjectSlotLegal(ObjectSlot slot, JSTaggedValue value, TaggedObject *obj) const;
    void VerifyMarkEden(TaggedObject *obj, ObjectSlot slot, TaggedObject *value) const;
    void VerifyEvacuateEden(TaggedObject *obj, ObjectSlot slot, TaggedObject *value) const;
    void VerifyMarkYoung(TaggedObject *obj, ObjectSlot slot, TaggedObject *value) const;
    void VerifyEvacuateYoung(TaggedObject *obj, ObjectSlot slot, TaggedObject *value) const;
    void VerifyMarkFull(TaggedObject *obj, ObjectSlot slot, TaggedObject *value) const;
    void VerifyEvacuateOld(TaggedObject *obj, ObjectSlot slot, TaggedObject *value) const;
    void VerifyEvacuateFull(TaggedObject *obj, ObjectSlot slot, TaggedObject *value) const;
    void VerifySharedRSetPostFullGC(TaggedObject *obj, ObjectSlot slot, TaggedObject *value) const;
    void VerifySharedObjectReference(TaggedObject *obj, ObjectSlot slot, TaggedObject *value) const;

    const BaseHeap* const heap_ {nullptr};
    size_t* const failCount_ {nullptr};
    VerifyKind verifyKind_;
};

class Verification {
public:
    explicit Verification(Heap *heap, VerifyKind verifyKind = VerifyKind::VERIFY_PRE_GC)
        : heap_(heap), verifyKind_(verifyKind) {}
    ~Verification() = default;

    static void VerifyMark(Heap *heap);
    static void VerifyEvacuate(Heap *heap);
    void VerifyAll() const;

    size_t VerifyRoot() const;
    size_t VerifyHeap() const;
    size_t VerifyOldToNewRSet() const;
private:
    void VerifyObjectSlot(const ObjectSlot &slot, size_t *failCount) const;

    NO_COPY_SEMANTIC(Verification);
    NO_MOVE_SEMANTIC(Verification);

    Heap *heap_ {nullptr};
    VerifyKind verifyKind_;
};

class SharedHeapVerification {
public:
    explicit SharedHeapVerification(SharedHeap *heap, VerifyKind verifyKind)
        : sHeap_(heap), verifyKind_(verifyKind) {}
    ~SharedHeapVerification() = default;

    void VerifyAll() const;
    void VerifyMark(bool cm) const;
    void VerifySweep(bool cm) const;

    size_t VerifyRoot() const;
    size_t VerifyHeap() const;
private:
    void VerifyObjectSlot(const ObjectSlot &slot, size_t *failCount) const;

    NO_COPY_SEMANTIC(SharedHeapVerification);
    NO_MOVE_SEMANTIC(SharedHeapVerification);

    SharedHeap *sHeap_ {nullptr};
    VerifyKind verifyKind_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_VERIFICATION_H
