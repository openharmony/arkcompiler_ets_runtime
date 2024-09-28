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

#ifndef ECMASCRIPT_MEM_JIT_FORT_MEMDESC_H
#define ECMASCRIPT_MEM_JIT_FORT_MEMDESC_H

#include <deque>
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/base/asan_interface.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {

// Before Jit Fort, FreeList allocator uses FreeObject to link
// together free memory blocks in heap regions where each
// free memory block is a FreeObject with size of the block and
// a pointer to the next free block in the heap region. Its usage
// requires a mutable heap region and does not work with Jit Fort space
// which is immutbale execept for access by CodeSigner.
//
// When JIT Fort is enabled, JIT generated instructions are installed
// in memory allocated from JitFort space, and the corresponding mutable
// Code Cache object constains a pointer to the allocated blk.
// The information on free and used Jit Fort space is stored outside of
// JitFort using MemDesc which holds a pointer to a mem block in Jit Fort
// space, size of the block, and a pointer to the next MemDesc. It is
// used to maintain list of free mem blks in Jit Fort as well as list
// of live (in use) mem blocks.
//
// To reuse FreeList allocator code for JitFort, related classes
// (allocator/FreeObjectList/FreeObjectSet, etc) had to be changed into
// template classes to support both FreeObject and MemDesc targets,
// and MemDesc has to support same methods as FreeObject, and use the
// same null pointer value forlink pointer that FreeObject uses, i.e.
// NULL_POINTER with a value of 0x5 instead of 0.
//
#define INVALID_OBJPTR ((uintptr_t) JSTaggedValue::NULL_POINTER)

class MemDesc {
public:
    MemDesc() = default;
    ~MemDesc() = default;

    static MemDesc *Cast(uintptr_t object)
    {
        return reinterpret_cast<MemDesc *>(object);
    }

    inline uintptr_t GetBegin() const
    {
        return mem_;
    }

    inline uintptr_t GetEnd() const
    {
        return mem_ + size_;
    }

    inline void SetMem(uintptr_t mem)
    {
        mem_ = mem;
    }

    inline void SetSize(size_t size)
    {
        size_ = size;
    }

    inline void SetNext(MemDesc *desc)
    {
        next_ = desc;
    }

    inline MemDesc *GetNext()
    {
        return next_;
    }

    inline uint32_t Available() const
    {
        return size_;
    }

    inline bool IsFreeObject() const
    {
        return true; // for compatibility with FreeObject
    }

    inline void SetAvailable(uint32_t size)
    {
        size_ = size;
    }

    inline void AsanPoisonFreeObject() const
    {
        ASAN_POISON_MEMORY_REGION((const volatile void *)mem_, size_);
    }

    inline void AsanUnPoisonFreeObject() const
    {
        ASAN_UNPOISON_MEMORY_REGION((const volatile void *)mem_, size_);
    }

    inline void SetInstalled(bool installed)
    {
        installed_.store(installed, std::memory_order_release);
    }

    inline bool IsInstalled()
    {
        return installed_.load(std::memory_order_acquire);
    }

private:
    uintptr_t mem_ {0};
    size_t size_ {0};
    std::atomic<bool> installed_ {false};
    MemDesc *next_ {MemDesc::Cast(INVALID_OBJPTR)};
};

class MemDescPool {
public:
    MemDescPool(uintptr_t fortBegin, size_t fortSize);
    ~MemDescPool();

    static inline bool IsEmpty(MemDesc* list)
    {
        return (list == nullptr || list == MemDesc::Cast(INVALID_OBJPTR));
    }

    inline MemDesc *GetDescFromPool()
    {
        LockHolder lock(lock_);
        return GetDesc();
    }

    inline void ReturnDescToPool(MemDesc *desc)
    {
        LockHolder lock(lock_);
        Add(desc);
        returned_++;
    }

    inline uintptr_t JitFortBegin()
    {
        return fortBegin_;
    }

    inline size_t JitFortSize()
    {
        return fortSize_;
    }

private:
    MemDesc *GetDesc();
    void Add(MemDesc *);
    void Expand();

    static constexpr size_t MEMDESCS_PER_BLOCK = 100;
    MemDesc *freeList_ {nullptr};
    std::deque<void *> memDescBlocks_;
    size_t allocated_ {0};
    size_t returned_ {0};
    size_t highwater_ {0};
    Mutex lock_;

    uintptr_t fortBegin_;
    size_t fortSize_;
};

}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_JIT_FORT_MEMDESC_H
