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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARE_GC_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARE_GC_H

#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/garbage_collector.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/work_manager.h"

namespace panda::ecmascript {
class ShareGC : public GarbageCollector {
public:
    explicit ShareGC(SharedHeap *heap) : sHeap_(heap), sWorkManager_(heap->GetWorkManager()) {}
    ~ShareGC() override = default;
    NO_COPY_SEMANTIC(ShareGC);
    NO_MOVE_SEMANTIC(ShareGC);

    void RunPhases() override;
protected:
    void Initialize() override;
    void Mark() override;
    void Sweep() override;
    void Finish() override;

private:
    void UpdateRecordWeakReference();

    SharedHeap *sHeap_;
    ShareGCWorkManager *sWorkManager_ {nullptr};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARE_GC_H
