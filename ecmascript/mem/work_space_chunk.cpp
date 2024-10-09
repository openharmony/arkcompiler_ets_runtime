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

#include "ecmascript/mem/work_space_chunk.h"

#include "ecmascript/mem/native_area_allocator.h"

namespace panda::ecmascript {
WorkSpaceChunk::WorkSpaceChunk(NativeAreaAllocator *allocator) : allocator_(allocator) {}

uintptr_t WorkSpaceChunk::NewArea(size_t size)
{
    ASSERT(allocator_ != nullptr);
    auto area = reinterpret_cast<uintptr_t>(allocator_->AllocateBuffer(size));
    if (!area) { // LOCV_EXCL_BR_LINE
        LOG_ECMA_MEM(FATAL) << "OOM WorkSpaceChunk : NewArea area is nullptr";
        UNREACHABLE();
    }
    allocator_->IncreaseNativeSizeStats(size, NativeFlag::CHUNK_MEM);
    areaList_.emplace(area, area);
    return area;
}

void WorkSpaceChunk::Free([[maybe_unused]] void *mem)
{
    LockHolder lock(mtx_);
    auto iter = areaList_.find(reinterpret_cast<uintptr_t>(mem));
    if (iter != areaList_.end()) {
        areaList_.erase(iter);
    }
    allocator_->FreeBuffer(mem);
}

void WorkSpaceChunk::ReleaseMemory()
{
    LockHolder lock(mtx_);
    for (auto iter = areaList_.begin(); iter != areaList_.end(); ++iter) {
        allocator_->FreeBuffer(reinterpret_cast<void *>(iter->second));
    }
    areaList_.clear();
}
}  // namespace panda::ecmascript
