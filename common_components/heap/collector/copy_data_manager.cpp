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
#ifdef _WIN64
#include <errhandlingapi.h>
#include <handleapi.h>
#include <memoryapi.h>
#endif

#include "common_components/base/immortal_wrapper.h"
#include "common_components/heap/allocator/region_desc.h"
#include "common_components/heap/collector/region_bitmap.h"
#include "common_components/heap/collector/copy_data_manager.h"
#include "common_components/platform/os.h"

namespace common {

static ImmortalWrapper<HeapBitmapManager> forwardDataManager;
HeapBitmapManager& HeapBitmapManager::GetHeapBitmapManager() { return *forwardDataManager; }

void HeapBitmapManager::InitializeHeapBitmap()
{
    size_t maxHeapBytes = Heap::GetHeap().GetMaxCapacity();
    size_t heapBitmapSize = RoundUp(GetHeapBitmapSize(maxHeapBytes), COMMON_PAGE_SIZE);
    allHeapBitmapSize_ = heapBitmapSize;

#ifdef _WIN64
    void* startAddress = VirtualAlloc(NULL, allHeapBitmapSize_, MEM_RESERVE, PAGE_READWRITE);
    if (startAddress == NULL) {
        LOG_COMMON(FATAL) << "failed to initialize HeapBitmapManager";
        UNREACHABLE_CC();
    }
#else
    void* startAddress = mmap(nullptr, allHeapBitmapSize_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (startAddress == MAP_FAILED) {
        LOG_COMMON(FATAL) << "failed to initialize HeapBitmapManager";
        UNREACHABLE_CC();
    } else {
#ifndef __APPLE__
        (void)madvise(startAddress, allHeapBitmapSize_, MADV_NOHUGEPAGE);
        COMMON_PRCTL(startAddress, allHeapBitmapSize_, "forward_data");
#endif
    }
#endif

    heapBitmapStart_ = reinterpret_cast<uintptr_t>(startAddress);
    heapBitmap_[0].InitializeMemory(heapBitmapStart_, heapBitmapSize, regionUnitCount_);

    os::PrctlSetVMA(startAddress, allHeapBitmapSize_, "ARKTS_CMC_GC_HEAP_BIT_MAP");
}
} // namespace common
