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

#ifndef ECMASCRIPT_WORK_SPACE_CHUNK_H
#define ECMASCRIPT_WORK_SPACE_CHUNK_H

#include "ecmascript/common.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/ecma_list.h"
#include "ecmascript/mem/area.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
class NativeAreaAllocator;

class PUBLIC_API WorkSpaceChunk {
public:
    static constexpr size_t MEM_ALIGN = 8U;

    explicit WorkSpaceChunk(NativeAreaAllocator *allocator);
    WorkSpaceChunk() = default;
    ~WorkSpaceChunk()
    {
        ReleaseMemory();
    }

    NO_COPY_SEMANTIC(WorkSpaceChunk);
    NO_MOVE_SEMANTIC(WorkSpaceChunk);

    void *Allocate(size_t size)
    {
        LockHolder lock(mtx_);
        if (size != WORKNODE_SPACE_SIZE) {
            LOG_ECMA_MEM(FATAL) << "WorkNode size is illegal, size:" << size;
            UNREACHABLE();
        }
        size = AlignUp(size, MEM_ALIGN);
        uintptr_t result = NewArea(size);

        return reinterpret_cast<void *>(result);
    }

    void Free([[maybe_unused]] void *mem);

private:
    uintptr_t NewArea(size_t size);
    void ReleaseMemory();
    NativeAreaAllocator *allocator_ {nullptr};
    Mutex mtx_;
    CUnorderedMap<uintptr_t, uintptr_t> areaList_ {};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_WORK_SPACE_CHUNK_H
