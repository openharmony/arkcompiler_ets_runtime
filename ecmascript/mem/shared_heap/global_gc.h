/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_H
#define ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_H

#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/shared_heap/global_parallel_cleaner.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
class Heap;
class JSThread;
class SharedHeap;
class GlobalGCWorkManager;

static constexpr size_t GLOBAL_GC_OOM_THRESHOLD = 50_MB;

// GlobalGC resolves Shared Heap OOM caused by stale local-to-shared RSet entries.
// Dead local heap objects may still hold references to shared heap objects via RSet,
// preventing SharedGC from reclaiming them. GlobalGC marks all local heaps to identify
// dead objects, removes their RSet entries, then re-triggers SharedGC.
// Called from SharedHeap::AllocateHugeObject and SharedHeap::AllocateInSOldSpace
// when SharedGC fails to free enough space.
class GlobalGC {
public:
    explicit GlobalGC(SharedHeap *sHeap);
    ~GlobalGC();
    NO_COPY_SEMANTIC(GlobalGC);
    NO_MOVE_SEMANTIC(GlobalGC);

    void RunPhases();

    size_t GetFreedSize() const
    {
        return freedSize_;
    }

    void ResetWorkManager(GlobalGCWorkManager *workManager)
    {
        workManager_ = workManager;
    }

private:
    void Initialize();
    void Mark();
    void Clean();
    void RebuildFreeList();
    void Finish();
    void TriggerSharedGC();

    SharedHeap *sHeap_ {nullptr};
    GlobalGCWorkManager *workManager_ {nullptr};
    GlobalParallelCleaner cleaner_;
    size_t freedSize_ {0};
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_H
