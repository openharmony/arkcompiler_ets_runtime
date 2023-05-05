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

#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/platform/map.h"
#include "ecmascript/platform/os.h"

namespace panda::ecmascript {
MemMapAllocator *MemMapAllocator::GetInstance()
{
    static MemMapAllocator *vmAllocator_ = new MemMapAllocator();
    return vmAllocator_;
}

MemMap MemMapAllocator::Allocate(size_t size, size_t alignment, bool regular, bool isMachineCode)
{
    if (UNLIKELY(memMapTotalSize_ + size > capacity_)) {
        LOG_GC(ERROR) << "memory map overflow";
        return MemMap();
    }

    MemMap mem;
    if (regular) {
        mem = memMapPool_.GetMemFromCache(size);
        if (mem.GetMem() != nullptr) {
            memMapTotalSize_ += size;
            int prot = isMachineCode ? PAGE_PROT_EXEC_READ : PAGE_PROT_READWRITE;
            PageTagType type = isMachineCode ? PageTagType::MACHINE_CODE : PageTagType::HEAP;
            PageProtect(mem.GetMem(), mem.GetSize(), prot);
            PageTag(mem.GetMem(), size, type);
            return mem;
        }
        mem = PageMap(REGULAR_REGION_MMAP_SIZE, PAGE_PROT_NONE, alignment);
        memMapPool_.InsertMemMap(mem);
        mem = memMapPool_.SplitMemFromCache(mem);
    } else {
        mem = memMapFreeList_.GetMemFromList(size);
    }
    if (mem.GetMem() != nullptr) {
        int prot = isMachineCode ? PAGE_PROT_EXEC_READ : PAGE_PROT_READWRITE;
        PageTagType type = isMachineCode ? PageTagType::MACHINE_CODE : PageTagType::HEAP;
        PageProtect(mem.GetMem(), mem.GetSize(), prot);
        PageTag(mem.GetMem(), mem.GetSize(), type);
        memMapTotalSize_ += mem.GetSize();
    }
    return mem;
}

void MemMapAllocator::Free(void *mem, size_t size, bool isRegular)
{
    memMapTotalSize_ -= size;
    PageTag(mem, size, PageTagType::MEMPOOL_CACHE);
    PageProtect(mem, size, PAGE_PROT_NONE);
    PageRelease(mem, size);
    if (isRegular) {
        memMapPool_.AddMemToCache(mem, size);
    } else {
        memMapFreeList_.AddMemToList(MemMap(mem, size));
    }
}

void MemMapAllocator::AdapterSuitablePoolCapacity()
{
    size_t physicalSize = PhysicalSize();
    capacity_ = std::max<size_t>(physicalSize / PHY_SIZE_MULTIPLE, MIN_MEM_POOL_CAPACITY);
    if (capacity_ > LARGE_POOL_SIZE) {
        capacity_ = std::max<size_t>(capacity_, STANDARD_POOL_SIZE);
    } else if (capacity_ >= MEDIUM_POOL_SIZE) {
        capacity_ = std::min<size_t>(capacity_, STANDARD_POOL_SIZE);
    } else if (capacity_ >= LOW_POOL_SIZE) {
        capacity_ = std::max<size_t>(capacity_, 128_MB);
    }
    LOG_GC(INFO) << "Ark Auto adapter memory pool capacity:" << capacity_;
}
}  // namespace panda::ecmascript
