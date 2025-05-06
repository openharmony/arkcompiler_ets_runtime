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

#include "common_components/common_runtime/src/mutator/thread_local.h"

#include "common_components/common_runtime/src/heap/allocator/alloc_buffer.h"
#include "common_components/common_runtime/src/base/globals.h"

namespace panda {

PUBLIC_API thread_local ThreadLocalData threadLocalData;
ThreadLocalData* ThreadLocal::GetThreadLocalData()
{
    return &threadLocalData;
}

extern "C" void MCC_CheckThreadLocalDataOffset()
{
    static_assert(offsetof(ThreadLocalData, buffer) == 0,
                  "need to modify the offset of this value in llvm-project and thread at the same time");
    static_assert(offsetof(ThreadLocalData, mutator) == sizeof(void*),
                  "need to modify the offset of this value in llvm-project and thread at the same time");
    static_assert(offsetof(ThreadLocalData, thread) == sizeof(void*) * 2,
                  "need to modify the offset of this value in llvm-project and thread at the same time");
    static_assert(offsetof(ThreadLocalData, schedule) == sizeof(void*) * 3,
                  "need to modify the offset of this value in llvm-project and thread at the same time");
    static_assert(offsetof(ThreadLocalData, preemptFlag) == sizeof(void*) * 4,
                  "need to modify the offset of this value in llvm-project and thread at the same time");
    static_assert(offsetof(ThreadLocalData, protectAddr) == sizeof(void*) * 5,
                  "need to modify the offset of this value in llvm-project and thread at the same time");
    static_assert(offsetof(ThreadLocalData, safepointState) == sizeof(void*) * 6,
                  "need to modify the offset of this value in llvm-project and thread at the same time");
    static_assert(offsetof(ThreadLocalData, tid) == sizeof(void*) * 7,
                  "need to modify the offset of this value in llvm-project and thread at the same time");
    static_assert(offsetof(ThreadLocalData, foreignThread) == sizeof(void*) * 8,
                  "need to modify the offset of this value in llvm-project and thread at the same time");
    static_assert(sizeof(ThreadLocalData) == sizeof(void*) * 10,
                  "need to modify the offset of this value in llvm-project and thread at the same time");
}

} // namespace panda
