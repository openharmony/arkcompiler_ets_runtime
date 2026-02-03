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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SWEEP_GC_H
#define ECMASCRIPT_MEM_CMS_MEM_SWEEP_GC_H

#include "ecmascript/mem/garbage_collector.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/work_manager.h"

namespace panda {
namespace ecmascript {
class SweepGC : public GarbageCollector {
public:
    explicit SweepGC(Heap *heap);
    ~SweepGC() override = default;

    NO_COPY_SEMANTIC(SweepGC);
    NO_MOVE_SEMANTIC(SweepGC);

    void RunPhases() override;

    Heap *GetHeap() const
    {
        return heap_;
    }

protected:
    void Initialize() override;
    void Mark() override;
    void Sweep() override;
    void Finish() override;

private:
    void MarkRoots();
    void ProcessSharedGCRSetWorkList();
    void ProcessNativeDelete();
    void ClearDeadReferences();
    void UpdateRecordWeakReference(uint32_t threadId);
    void UpdateRecordWeakLinkedHashMap(uint32_t threadId);

    Heap *heap_ {nullptr};
    bool markingInProgress_ {false};
    // Obtained from the shared heap instance.
    WorkManager *workManager_ {nullptr};

    friend class WorkManager;
    friend class Heap;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_CMS_MEM_SWEEP_GC_H
