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

#ifdef ENABLE_JITFORT
template <typename T>
FreeListAllocator<T>::FreeListAllocator(BaseHeap *heap) : heap_(heap)
#else
FreeListAllocator::FreeListAllocator(BaseHeap *heap) : heap_(heap)
#endif
{
#ifdef ENABLE_JITFORT
    freeList_ = std::make_unique<FreeObjectList<T>>();
#else
    freeList_ = std::make_unique<FreeObjectList>();
#endif
}

#ifdef ENABLE_JITFORT
template <typename T>
void FreeListAllocator<T>::Initialize(Region *region)
#else
void FreeListAllocator::Initialize(Region *region)
#endif
{
    bpAllocator_.Reset(region->GetBegin(), region->GetEnd());
}

#ifdef ENABLE_JITFORT
template <typename T>
void FreeListAllocator<T>::Reset(BaseHeap *heap)
#else
void FreeListAllocator::Reset(BaseHeap *heap)
#endif
{
    heap_ = heap;
#ifdef ENABLE_JITFORT
    freeList_ = std::make_unique<FreeObjectList<T>>();
#else
    freeList_ = std::make_unique<FreeObjectList>();
#endif
    FreeBumpPoint();
}

#ifdef ENABLE_JITFORT
template <typename T>
void FreeListAllocator<T>::AddFree(Region *region)
#else
void FreeListAllocator::AddFree(Region *region)
#endif
{
    auto begin = region->GetBegin();
    auto end = region->GetEnd();
    FreeBumpPoint();
    bpAllocator_.Reset(begin, end);
}

#ifdef ENABLE_JITFORT
template <typename T>
uintptr_t FreeListAllocator<T>::Allocate(size_t size)
#else
uintptr_t FreeListAllocator::Allocate(size_t size)
#endif
{
    auto ret = bpAllocator_.Allocate(size);
    if (LIKELY(ret != 0)) {
        allocationSizeAccumulator_ += size;
        return ret;
    }
#ifdef ENABLE_JITFORT
    T *object = freeList_->Allocate(size);
#else
    FreeObject *object = freeList_->Allocate(size);
#endif
    if (object != nullptr) {
        ret = Allocate(object, size);
    }
    return ret;
}


#ifdef ENABLE_JITFORT
template <typename T>
uintptr_t FreeListAllocator<T>::Allocate(T *object, size_t size)
#else
uintptr_t FreeListAllocator::Allocate(FreeObject *object, size_t size)
#endif
{
    uintptr_t begin = object->GetBegin();
    uintptr_t end = object->GetEnd();
    uintptr_t remainSize = end - begin - size;
    ASSERT(remainSize >= 0);
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

#ifdef ENABLE_JITFORT
template <typename T>
void FreeListAllocator<T>::FreeBumpPoint()
#else
void FreeListAllocator::FreeBumpPoint()
#endif
{
    auto begin = bpAllocator_.GetTop();
    auto size = bpAllocator_.Available();
    bpAllocator_.Reset();
    Free(begin, size, true);
}

#ifdef ENABLE_JITFORT
template <typename T>
void FreeListAllocator<T>::FillBumpPointer()
#else
void FreeListAllocator::FillBumpPointer()
#endif
{
    size_t size = bpAllocator_.Available();
    if (size != 0) {
        FreeObject::FillFreeObject(heap_, bpAllocator_.GetTop(), size);
    }
}

#ifdef ENABLE_JITFORT
template <typename T>
void FreeListAllocator<T>::ResetBumpPointer(uintptr_t begin, uintptr_t end, uintptr_t top)
#else
void FreeListAllocator::ResetBumpPointer(uintptr_t begin, uintptr_t end, uintptr_t top)
#endif
{
    bpAllocator_.Reset(begin, end, top);
}

#ifdef ENABLE_JITFORT
template <typename T>
void FreeListAllocator<T>::ResetTopPointer(uintptr_t top)
#else
void FreeListAllocator::ResetTopPointer(uintptr_t top)
#endif
{
    bpAllocator_.ResetTopPointer(top);
}

// The object will be marked with poison after being put into the freelist when is_asan is true.
#ifdef ENABLE_JITFORT
template <typename T>
void FreeListAllocator<T>::Free(uintptr_t begin, size_t size, bool isAdd)
#else
void FreeListAllocator::Free(uintptr_t begin, size_t size, bool isAdd)
#endif
{
    ASSERT(heap_ != nullptr);
    ASSERT(size >= 0);
    if (size != 0) {
#ifdef ENABLE_JITFORT
        T::FillFreeObject(heap_, begin, size);
#else
        FreeObject::FillFreeObject(heap_, begin, size);
#endif
        ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(begin), size);
        freeList_->Free(begin, size, isAdd);
#ifdef ARK_ASAN_ON
        ASAN_POISON_MEMORY_REGION(reinterpret_cast<void *>(begin), size);
#endif
    }
}
#ifdef ENABLE_JITFORT
template <>
void FreeListAllocator<MemDesc>::Free(uintptr_t begin, size_t size, bool isAdd);
#endif

#ifdef ENABLE_JITFORT
template <typename T>
uintptr_t FreeListAllocator<T>::LookupSuitableFreeObject(size_t size)
#else
uintptr_t FreeListAllocator::LookupSuitableFreeObject(size_t size)
#endif
{
    auto freeObject = freeList_->LookupSuitableFreeObject(size);
    if (freeObject != nullptr) {
        return freeObject->GetBegin();
    }
    return 0;
}

#ifdef ENABLE_JITFORT
template <typename T>
void FreeListAllocator<T>::RebuildFreeList()
#else
void FreeListAllocator::RebuildFreeList()
#endif
{
    bpAllocator_.Reset();
    freeList_->Rebuild();
}

#ifdef ENABLE_JITFORT
template <typename T>
inline void FreeListAllocator<T>::CollectFreeObjectSet(Region *region)
#else
inline void FreeListAllocator::CollectFreeObjectSet(Region *region)
#endif
{
#ifdef ENABLE_JITFORT
    region->EnumerateFreeObjectSets([&](FreeObjectSet<T> *set) {
#else
    region->EnumerateFreeObjectSets([&](FreeObjectSet *set) {
#endif
        if (set == nullptr || set->Empty()) {
            return;
        }
        freeList_->AddSet(set);
    });
    freeList_->IncreaseWastedSize(region->GetWastedSize());
}

#ifdef ENABLE_JITFORT
template <typename T>
inline bool FreeListAllocator<T>::MatchFreeObjectSet(Region *region, size_t size)
#else
inline bool FreeListAllocator::MatchFreeObjectSet(Region *region, size_t size)
#endif
{
    bool ret = false;
#ifdef ENABLE_JITFORT
    region->REnumerateFreeObjectSets([&](FreeObjectSet<T> *set) {
#else
    region->REnumerateFreeObjectSets([&](FreeObjectSet *set) {
#endif
        if (set == nullptr || set->Empty()) {
            return true;
        }
        ret = freeList_->MatchFreeObjectInSet(set, size);
        return false;
    });
    return ret;
}

#ifdef ENABLE_JITFORT
template <typename T>
inline void FreeListAllocator<T>::DetachFreeObjectSet(Region *region)
#else
inline void FreeListAllocator::DetachFreeObjectSet(Region *region)
#endif
{
#ifdef ENABLE_JITFORT
    region->EnumerateFreeObjectSets([&](FreeObjectSet<T> *set) {
#else
    region->EnumerateFreeObjectSets([&](FreeObjectSet *set) {
#endif
        if (set == nullptr || set->Empty()) {
            return;
        }
        freeList_->RemoveSet(set);
    });
    freeList_->DecreaseWastedSize(region->GetWastedSize());
}

#ifdef ENABLE_JITFORT
template <typename T>
size_t FreeListAllocator<T>::GetAvailableSize() const
#else
size_t FreeListAllocator::GetAvailableSize() const
#endif
{
    return freeList_->GetFreeObjectSize() + bpAllocator_.Available();
}

#ifdef ENABLE_JITFORT
template <typename T>
size_t FreeListAllocator<T>::GetWastedSize() const
#else
size_t FreeListAllocator::GetWastedSize() const
#endif
{
    return freeList_->GetWastedSize();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_ALLOCATOR_INL_H
