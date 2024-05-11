/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_THREAD_LOCAL_ALLOCATION_BUFFER_H
#define ECMASCRIPT_THREAD_LOCAL_ALLOCATION_BUFFER_H

#include "ecmascript/mem/allocator-inl.h"

namespace panda::ecmascript {
class ThreadLocalAllocationBuffer {
public:
    // perhaps need calculate tlab size dynamically.
    static constexpr size_t TLAB_SIZE = 8_KB;
    static constexpr size_t INIT_WASTE_LIMIT = 64;
    static constexpr size_t INCREASE_WASTE_SIZE = 64;
    ThreadLocalAllocationBuffer(Heap *heap) : heap_(heap) {}
    ~ThreadLocalAllocationBuffer() = default;
    NO_COPY_SEMANTIC(ThreadLocalAllocationBuffer);
    NO_MOVE_SEMANTIC(ThreadLocalAllocationBuffer);

    void Reset(uintptr_t begin = 0, uintptr_t end = 0, uintptr_t top = 0);

    void FillBumpPointer();

    uintptr_t Allocate(size_t size)
    {
        return bpAllocator_.Allocate(size);
    }

    size_t Available() const
    {
        return bpAllocator_.Available();
    }

    uintptr_t GetTop() const
    {
        return bpAllocator_.GetTop();
    }

    const uintptr_t *GetTopAddress()
    {
        return bpAllocator_.GetTopAddress();
    }

    const uintptr_t *GetEndAddress()
    {
        return bpAllocator_.GetEndAddress();
    }

    size_t GetTlabWasteLimit() const
    {
        return tlabWasteLimit_;
    }

    size_t ComputeSize() const
    {
        return TLAB_SIZE;
    }

    inline bool NeedNewTlab(size_t size)
    {
        if (size > TLAB_SIZE) {
            return false;
        }
        if (bpAllocator_.Available() >= tlabWasteLimit_) {
            tlabWasteLimit_ += INCREASE_WASTE_SIZE;
            return false;
        }
        if (disabled_) {
            return false;
        }
        return true;
    }

    void DisableNewTlab()
    {
        disabled_ = true;
    }

private:
    bool disabled_ {false};
    Heap *heap_ {nullptr};
    BumpPointerAllocator bpAllocator_;
    size_t tlabWasteLimit_ {INIT_WASTE_LIMIT};
};
}  //  namespace panda::ecmascript

#endif  // ECMASCRIPT_THREAD_LOCAL_ALLOCATION_BUFFER_H
