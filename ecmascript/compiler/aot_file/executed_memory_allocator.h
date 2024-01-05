/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_COMPILER_AOT_FILE_EXECUTED_MEMORY_ALLOCATOR_H
#define ECMASCRIPT_COMPILER_AOT_FILE_EXECUTED_MEMORY_ALLOCATOR_H

#include "ecmascript/mem/mem.h"
#include "ecmascript/platform/map.h"

namespace panda::ecmascript {
class ExecutedMemoryAllocator {
public:
    struct ExeMem {
        void *addr_ {nullptr};
        size_t size_ {0};
    };

    static void AllocateBuf(uint32_t size, ExeMem &exeMem, int prot)
    {
        // For safety reason, Disallow prot have both write & exec capability except in JIT.
        ASSERT(prot != PAGE_PROT_EXEC_READWRITE);
        MemMap buf = MachineCodePageMap(AlignUp(size, PageSize()), prot);
        exeMem.addr_ = buf.GetMem();
        exeMem.size_ = buf.GetSize();
    }

    static void DestroyBuf(ExeMem &exeMem)
    {
        if (exeMem.addr_ != nullptr) {
            MachineCodePageUnmap(MemMap(exeMem.addr_, exeMem.size_));
            exeMem.addr_ = nullptr;
            exeMem.size_ = 0;
        }
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_AOT_FILE_EXECUTED_MEMORY_ALLOCATOR_H
