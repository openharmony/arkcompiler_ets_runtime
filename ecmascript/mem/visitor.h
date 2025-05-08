/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_VISITOR_H
#define ECMASCRIPT_MEM_VISITOR_H

#include <functional>

#include "common_interfaces/heap/heap_visitor.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/tagged_object.h"
#include "libpandabase/mem/mem.h"

namespace panda::ecmascript {
enum class Root {
    ROOT_FRAME,
    ROOT_HANDLE,
    ROOT_VM,
    ROOT_STRING,
    ROOT_INTERNAL_CALL_PARAMS,
};

enum class VisitObjectArea {
    NORMAL,
    NATIVE_POINTER,
    IN_OBJECT,
    RAW_DATA
};

enum class VisitType : size_t { SEMI_GC_VISIT, OLD_GC_VISIT, SNAPSHOT_VISIT, ALL_VISIT };
enum class VMRootVisitType : uint8_t { MARK, UPDATE_ROOT, VERIFY, HEAP_SNAPSHOT };

template <class DerivedVisitor>
class BaseObjectVisitor {
public:
    BaseObjectVisitor() = default;
    virtual ~BaseObjectVisitor() = default;

    NO_MOVE_SEMANTIC(BaseObjectVisitor);
    NO_COPY_SEMANTIC(BaseObjectVisitor);

    void operator()(BaseObject *rootObject, uintptr_t startAddr, uintptr_t endAddr, VisitObjectArea area)
    {
        static_cast<DerivedVisitor*>(this)->VisitObjectRangeImpl(rootObject, startAddr, endAddr, area);
    }

    void VisitHClass(BaseObject *hclass)
    {
        static_cast<DerivedVisitor*>(this)->VisitObjectHClassImpl(hclass);
    }

    virtual void VisitObjectRangeImpl([[maybe_unused]] BaseObject *rootObject, [[maybe_unused]] uintptr_t startAddr,
                                      [[maybe_unused]] uintptr_t endAddr, [[maybe_unused]] VisitObjectArea area)
    {}

    virtual void VisitObjectHClassImpl([[maybe_unused]] BaseObject *hclass)
    {}
};

class RootVisitor {
public:
    RootVisitor() = default;
    virtual ~RootVisitor() = default;

    NO_MOVE_SEMANTIC(RootVisitor);
    NO_COPY_SEMANTIC(RootVisitor);

    virtual void VisitRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot slot) = 0;

    virtual void VisitRangeRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot start,
        [[maybe_unused]] ObjectSlot end) = 0;

    virtual void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
        [[maybe_unused]] ObjectSlot derived, [[maybe_unused]] uintptr_t baseOldObject) = 0;
};

class WeakVisitor {
public:
    WeakVisitor() = default;
    virtual ~WeakVisitor() = default;

    NO_MOVE_SEMANTIC(WeakVisitor);
    NO_COPY_SEMANTIC(WeakVisitor);

    virtual bool VisitRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot slot) = 0;
};

using WeakRootVisitor = std::function<TaggedObject *(TaggedObject *p)>;

template <VisitType visitType, size_t size>
class PrimitiveObjectBodyIterator {
public:
    template <class DerivedVisitor>
    static inline void IterateBody(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        if constexpr (visitType == VisitType::ALL_VISIT) {
            constexpr size_t hclassEnd = sizeof(JSTaggedType);
            visitor(root, ToUintPtr(root),
                ToUintPtr(root) + hclassEnd, VisitObjectArea::NORMAL);
            if constexpr (size > hclassEnd) {
                visitor(root, ToUintPtr(root) + hclassEnd,
                    ToUintPtr(root) + size, VisitObjectArea::RAW_DATA);
            }
        }
    }
};

template <VisitType visitType, size_t startOffset, size_t endOffset,
          size_t size, size_t startSize = sizeof(JSTaggedType)>
class ObjectBodyIterator {
public:
    template <VisitObjectArea area, bool visitHClass, class DerivedVisitor>
    static inline void IterateBody(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        if constexpr (visitType == VisitType::ALL_VISIT) {
            if constexpr (visitHClass) {
                IterateHClass(root, visitor);
            }
            IterateBefore(root, visitor);
        }
        if constexpr (startOffset < endOffset) {
            visitor(root, ToUintPtr(root) + startOffset,
                ToUintPtr(root) + endOffset, area);
        }

        if constexpr (visitType == VisitType::ALL_VISIT) {
            IterateAfter(root, visitor);
        }
    }

    template <class DerivedVisitor>
    static inline void IterateRefBody(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        IterateBody<VisitObjectArea::NORMAL, true>(root, visitor);
    }

    template <class DerivedVisitor>
    static inline void IterateNativeBody(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        IterateBody<VisitObjectArea::NATIVE_POINTER, true>(root, visitor);
    }

    template <class DerivedVisitor>
    static inline void IterateDerivedRefBody(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        IterateBody<VisitObjectArea::NORMAL, false>(root, visitor);
    }

    template <class DerivedVisitor>
    static inline void IterateHClass(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        visitor.VisitHClass(root->GetClass());
    }

    template <class DerivedVisitor>
    static inline void IterateBefore(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        if constexpr (startOffset > startSize) {
            ASSERT(startOffset != endOffset);
            IteratorRange(root, visitor, startSize, startOffset);
        }
    }

    template <class DerivedVisitor>
    static inline void IterateAfter(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        if constexpr (size > endOffset) {
            IteratorRange(root, visitor, endOffset, size);
        }
    }

    template <class DerivedVisitor>
    static inline void IteratorRange(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor,
        size_t start, size_t end)
    {
        visitor(root, ToUintPtr(root) + start,
                ToUintPtr(root) + end, VisitObjectArea::RAW_DATA);
    }
};

template <VisitType visitType, size_t startOffset>
class ArrayBodyIterator {
public:
    template <class DerivedVisitor>
    static inline void IterateBody(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor,
        size_t refLength, size_t length)
    {
        if constexpr (visitType == VisitType::ALL_VISIT) {
            IterateBefore(root, visitor);
        }
        if (LIKELY(refLength != 0)) {
            size_t endOffset = startOffset + refLength * JSTaggedValue::TaggedTypeSize();
            visitor(root, ToUintPtr(root) + startOffset,
                ToUintPtr(root) + endOffset, VisitObjectArea::NORMAL);
        }
        if constexpr (visitType == VisitType::ALL_VISIT) {
            IterateAfter(root, visitor, refLength, length);
        }
    }

    template <class DerivedVisitor>
    static inline void IterateBefore(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        size_t hclassEnd = sizeof(JSTaggedType);
        ASSERT(startOffset > hclassEnd);
        visitor.VisitHClass(root->GetClass());
        IteratorRange(root, visitor, hclassEnd, startOffset);
    }

    template <class DerivedVisitor>
    static inline void IterateAfter(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor,
        size_t refLength, size_t length)
    {
        if (length > refLength) {
            size_t endOffset = startOffset + refLength * JSTaggedValue::TaggedTypeSize();
            size_t size = startOffset + length * JSTaggedValue::TaggedTypeSize();
            IteratorRange(root, visitor, endOffset, size);
        }
    }

    template <class DerivedVisitor>
    static inline void IteratorRange(TaggedObject *root, BaseObjectVisitor<DerivedVisitor> &visitor,
        size_t start, size_t end)
    {
        visitor(root, ToUintPtr(root) + start,
                ToUintPtr(root) + end, VisitObjectArea::RAW_DATA);
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_VISITOR_H
