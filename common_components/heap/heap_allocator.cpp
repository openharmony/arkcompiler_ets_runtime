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

#include "common_interfaces/heap/heap_allocator.h"

#include "common_components/heap/heap_allocator-inl.h"
#include "common_components/common/type_def.h"
#include "common_components/heap/heap_manager.h"
#include "common_components/heap/allocator/region_manager.h"
#include "common_components/heap/allocator/region_space.h"

namespace common {
Address AllocateYoungInAllocBuffer(uintptr_t buffer, size_t size)
{
    ASSERT(buffer != 0);
    AllocationBuffer *allocBuffer = reinterpret_cast<AllocationBuffer *>(buffer);
    return allocBuffer->FastAllocateInTlab<AllocBufferType::YOUNG>(size);
}

Address AllocateOldInAllocBuffer(uintptr_t buffer, size_t size)
{
    ASSERT(buffer != 0);
    AllocationBuffer *allocBuffer = reinterpret_cast<AllocationBuffer *>(buffer);
    return allocBuffer->FastAllocateInTlab<common::AllocBufferType::OLD>(size);
}

Address HeapAllocator::AllocateInYoungOrHuge(size_t size, LanguageType language)
{
    auto address = HeapManager::Allocate(size);
    BaseObject::Cast(address)->SetLanguageType(language);
    return address;
}

Address HeapAllocator::AllocateInNonmoveOrHuge(size_t size, LanguageType language)
{
    auto address = HeapManager::Allocate(size, AllocType::PINNED_OBJECT);
    BaseObject::Cast(address)->SetLanguageType(language);
    return address;
}

Address HeapAllocator::AllocateInOldOrHuge(size_t size, LanguageType language)
{
    auto address = HeapManager::Allocate(size, AllocType::MOVEABLE_OLD_OBJECT);
    BaseObject::Cast(address)->SetLanguageType(language);
    return address;
}

Address HeapAllocator::AllocateInHuge(size_t size, LanguageType language)
{
    auto address = HeapManager::Allocate(size);
    BaseObject::Cast(address)->SetLanguageType(language);
    return address;
}

Address HeapAllocator::AllocateInReadOnly(size_t size, LanguageType language)
{
    auto address = HeapManager::Allocate(size, AllocType::READ_ONLY_OBJECT);
    BaseObject::Cast(address)->SetLanguageType(language);
    return address;
}

uintptr_t HeapAllocator::AllocateLargeJitFortRegion(size_t size, LanguageType language)
{
    RegionSpace& allocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    auto address =  allocator.AllocJitFortRegion(size);
    BaseObject::Cast(address)->SetLanguageType(language);
    return address;
}

// below are interfaces used for serialize
Address HeapAllocator::AllocateNoGC(size_t size)
{
    return HeapManager::Allocate(size, AllocType::MOVEABLE_OBJECT, false);
}

Address HeapAllocator::AllocateOldOrLargeNoGC(size_t size)
{
    if (size >= RegionDesc::LARGE_OBJECT_DEFAULT_THRESHOLD) {
        return AllocateLargeRegion(size);
    }
    return HeapManager::Allocate(size, AllocType::MOVEABLE_OLD_OBJECT, false);
}

Address HeapAllocator::AllocatePinNoGC(size_t size)
{
    return HeapManager::Allocate(size, AllocType::PINNED_OBJECT, false);
}

Address HeapAllocator::AllocateOldRegion()
{
    RegionSpace& allocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    return allocator.AllocOldRegion();
}

Address HeapAllocator::AllocatePinnedRegion()
{
    RegionSpace& allocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    return allocator.AllocPinnedRegion();
}

Address HeapAllocator::AllocateLargeRegion(size_t size)
{
    RegionSpace& allocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    return allocator.AllocLargeRegion(size);
}

}  // namespace common
