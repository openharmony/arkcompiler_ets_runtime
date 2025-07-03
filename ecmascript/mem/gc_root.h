/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_GC_ROOT_H
#define ECMASCRIPT_GC_ROOT_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/slots.h"

namespace panda::ecmascript {

/**
 * When CMCGC is enabled, this class ensures that root references can be safely
 * accessed during concurrent GC phases by:
 * - Adding write barriers on creation if needed
 * - Requiring read barriers when accessing values to handle object forwarding
 *
 */
class GCRoot {
public:
    /**
     * When CMCGC is enabled and needWriteBarrier is true, notifies the GC
     * that a new root reference has been created.
     * This is especially usefull when store GCRoot in a data structure
     * and act as a write-barrier for the root container.
     */
    template <typename T, bool needWriteBarrier = true>
    explicit GCRoot(T value) : objPtr_(value)
    {
        if constexpr (needWriteBarrier) {
            if (g_isEnableCMCGC) {
                common::BaseRuntime::WriteRoot(reinterpret_cast<void *>(objPtr_.GetRawData()));
            }
        }
    }

    /** Creates a GCRoot pointing to a hole value (empty/uninitialized) */
    explicit GCRoot() : objPtr_(JSTaggedValue::Hole()) {}

    // Copying is forbidden to prevent accidental root duplication
    GCRoot(const GCRoot &) = delete;
    GCRoot &operator=(const GCRoot &) = delete;

    // Move semantics are allowed for container storage
    GCRoot(GCRoot &&other) noexcept : objPtr_(other.objPtr_) {}
    GCRoot &operator=(GCRoot &&other) noexcept
    {
        objPtr_ = other.objPtr_;
        return *this;
    }

    ~GCRoot() = default;

    /**
     * Safely reads the wrapped value with GC synchronization.
     *
     * When CMCGC is enabled, applies a read barrier to handle
     * object forwarding if the object was moved during GC
     */
    JSTaggedValue Read()
    {
        if (g_isEnableCMCGC) {
            return JSTaggedValue(
                reinterpret_cast<JSTaggedType>(common::BaseRuntime::ReadBarrier(reinterpret_cast<void *>(&objPtr_))));
        } else {
            return objPtr_;
        }
    }

    /**
     * Allows GC to visit this root during root scanning phases.
     * Such as enumeration phase and fowarding phase
     */
    template <typename Func>
    void VisitRoot(Func &&fn) const
    {
        ObjectSlot slot(reinterpret_cast<uintptr_t>(&objPtr_));
        fn(slot);
    }

    /**
     * Hash functor for using GCRoot as hash table keys.
     *
     * WARNING: Only safe to use if the referenced objects are non-movable!
     *
     * For movable objects, the hash will become invalid after GC copy phases,
     * breaking hash table invariants. Safe alternatives:
     * 1. Only use with non-movable objects (e.g., HClass)
     * 2. Rehash/rebuild the table after each GC cycle
     * 3. Use object content hash instead of address-based hash
     */
    class GCRootHash {
    public:
        size_t operator()(const GCRoot &root) const
        {
            return std::hash<JSTaggedType>()(root.objPtr_.GetRawData());
        }

        template <typename T>
        size_t operator()(const T &key) const
        {
            return std::hash<JSTaggedType>()(key);
        }
    };

private:
    JSTaggedValue objPtr_;
};

}  // namespace panda::ecmascript
#endif
