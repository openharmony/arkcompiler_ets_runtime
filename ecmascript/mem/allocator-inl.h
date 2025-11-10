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

#ifndef ECMASCRIPT_MEM_ALLOCATOR_INL_H
#define ECMASCRIPT_MEM_ALLOCATOR_INL_H

#include <cstdlib>
#include <type_traits>

#include "ecmascript/free_object.h"
#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
BumpPointerAllocator::BumpPointerAllocator(uintptr_t begin, uintptr_t end) : begin_(begin), top_(begin), end_(end) {}

void BumpPointerAllocator::Reset()
{
    begin_ = 0;
    top_ = 0;
    end_ = 0;
}

void BumpPointerAllocator::Reset(uintptr_t begin, uintptr_t end)
{
    begin_ = begin;
    top_ = begin;
    end_ = end;
#ifdef ARK_ASAN_ON
    ASAN_POISON_MEMORY_REGION(reinterpret_cast<void *>(top_), (end_ - top_));
#endif
}

void BumpPointerAllocator::Reset(uintptr_t begin, uintptr_t end, uintptr_t top)
{
    begin_ = begin;
    top_ = top;
    end_ = end;
#ifdef ARK_ASAN_ON
    ASAN_POISON_MEMORY_REGION(reinterpret_cast<void *>(top_), (end_ - top_));
#endif
}

void BumpPointerAllocator::ResetTopPointer(uintptr_t top)
{
    top_ = top;
}

uintptr_t BumpPointerAllocator::Allocate(size_t size)
{
    ASSERT(size != 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (UNLIKELY(top_ + size > end_)) {
        return 0;
    }
    uintptr_t result = top_;
    // It need to mark unpoison when object being allocated.
    ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(result), size);
    top_ += size;
    return result;
}

template <typename T>
FreeListAllocator<T>::FreeListAllocator(BaseHeap *heap) : heap_(heap)
{
    freeList_ = std::make_unique<FreeObjectList<T>>();
}

template <typename T>
void FreeListAllocator<T>::Initialize(Region *region)
{
    bpAllocator_.Reset(region->GetBegin(), region->GetEnd());
}

template <typename T>
void FreeListAllocator<T>::Reset(BaseHeap *heap)
{
    heap_ = heap;
    freeList_ = std::make_unique<FreeObjectList<T>>();
    FreeBumpPoint();
}

template <typename T>
void FreeListAllocator<T>::AddFree(Region *region)
{
    auto begin = region->GetBegin();
    auto end = region->GetEnd();
    FreeBumpPoint();
    bpAllocator_.Reset(begin, end);
}

template <typename T>
uintptr_t FreeListAllocator<T>::Allocate(size_t size)
{
    auto ret = bpAllocator_.Allocate(size);
    if (LIKELY(ret != 0)) {
        allocationSizeAccumulator_ += size;
        return ret;
    }
    T *object = freeList_->Allocate(size);
    if (object != nullptr) {
        ret = Allocate(object, size);
    }
    return ret;
}


template <typename T>
uintptr_t FreeListAllocator<T>::Allocate(T *object, size_t size)
{
    uintptr_t begin = object->GetBegin();
    uintptr_t end = object->GetEnd();
    uintptr_t remainSize = end - begin - size;
    ASSERT(remainSize >= 0);
    if constexpr (std::is_same<T, MemDesc>::value) {
        memDescPool_->ReturnDescToPool(object);
    }
    // Keep a longest freeObject between bump-pointer and free object that just allocated
    allocationSizeAccumulator_ += size;
    if (remainSize <= bpAllocator_.Available()) {
        Free(begin + size, remainSize);
        return begin;
    } else {
        FreeBumpPoint();
        bpAllocator_.Reset(begin, end);
        auto ret = bpAllocator_.Allocate(size);
        return ret;
    }
}

template <typename T>
void FreeListAllocator<T>::FreeBumpPoint()
{
    auto begin = bpAllocator_.GetTop();
    auto size = bpAllocator_.Available();
    bpAllocator_.Reset();
    Free(begin, size, true);
}

template <typename T>
void FreeListAllocator<T>::FillBumpPointer()
{
    if constexpr (std::is_same<T, MemDesc>::value) {
        return;
    }
    size_t size = bpAllocator_.Available();
    if (size != 0) {
        FreeObject::FillFreeObject(heap_, bpAllocator_.GetTop(), size);
    }
}

template <typename T>
void FreeListAllocator<T>::ResetBumpPointer(uintptr_t begin, uintptr_t end, uintptr_t top)
{
    bpAllocator_.Reset(begin, end, top);
}

template <typename T>
void FreeListAllocator<T>::ResetTopPointer(uintptr_t top)
{
    bpAllocator_.ResetTopPointer(top);
}

// The object will be marked with poison after being put into the freelist when is_asan is true.
template <typename T>
void FreeListAllocator<T>::Free(uintptr_t begin, size_t size, bool isAdd)
{
    ASSERT(size >= 0);
    if (size != 0) {
        if constexpr (!std::is_same<T, MemDesc>::value) {
            ASSERT(heap_ != nullptr);
            T::FillFreeObject(heap_, begin, size);
        }

        ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(begin), size);
        freeList_->Free(begin, size, isAdd);
#ifdef ARK_ASAN_ON
        ASAN_POISON_MEMORY_REGION(reinterpret_cast<void *>(begin), size);
#endif
    }
}

template <typename T>
uintptr_t FreeListAllocator<T>::LookupSuitableFreeObject(size_t size)
{
    auto freeObject = freeList_->LookupSuitableFreeObject(size);
    if (freeObject != nullptr) {
        return freeObject->GetBegin();
    }
    return 0;
}

template <typename T>
void FreeListAllocator<T>::RebuildFreeList()
{
    bpAllocator_.Reset();
    freeList_->Rebuild();
}

template <typename T>
inline void FreeListAllocator<T>::CollectFreeObjectSet(Region *region)
{
    region->EnumerateFreeObjectSets([&](FreeObjectSet<T> *set) {
        if (set == nullptr || set->Empty()) {
            return;
        }
        freeList_->AddSet(set);
    });
    freeList_->IncreaseWastedSize(region->GetWastedSize());
}

template <typename T>
inline bool FreeListAllocator<T>::MatchFreeObjectSet(Region *region, size_t size)
{
    bool ret = false;
    region->REnumerateFreeObjectSets([&](FreeObjectSet<T> *set) {
        if (set == nullptr || set->Empty()) {
            return true;
        }
        ret = freeList_->MatchFreeObjectInSet(set, size);
        return false;
    });
    return ret;
}

template <typename T>
inline void FreeListAllocator<T>::DetachFreeObjectSet(Region *region)
{
    region->EnumerateFreeObjectSets([&](FreeObjectSet<T> *set) {
        if (set == nullptr || set->Empty()) {
            return;
        }
        freeList_->RemoveSet(set);
    });
    freeList_->DecreaseWastedSize(region->GetWastedSize());
}

template <typename T>
size_t FreeListAllocator<T>::GetAvailableSize() const
{
    return freeList_->GetFreeObjectSize() + bpAllocator_.Available();
}

template <typename T>
size_t FreeListAllocator<T>::GetWastedSize() const
{
    return freeList_->GetWastedSize();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_ALLOCATOR_INL_H
