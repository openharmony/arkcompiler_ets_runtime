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

#ifndef ECMASCRIPT_MEM_STICKY_MEM_SWEEP_GC_H
#define ECMASCRIPT_MEM_STICKY_MEM_SWEEP_GC_H

#include "ecmascript/mem/garbage_collector.h"
#include "ecmascript/mem/cms_mem/sweep_gc.h"

namespace panda {
namespace ecmascript {
class StickySweepGC : public SweepGC {
public:
    explicit StickySweepGC(Heap *heap);
    ~StickySweepGC() override = default;

    NO_COPY_SEMANTIC(StickySweepGC);
    NO_MOVE_SEMANTIC(StickySweepGC);

    void RunPhases() override;

    Heap *GetHeap() const
    {
        return heap_;
    }

protected:
    void Initialize() override;
    void Mark() override;
    void ProcessNativeDelete() override;
    void Sweep() override;

private:
    void MarkRoots();

    friend class WorkManager;
    friend class Heap;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_STICKY_MEM_SWEEP_GC_H
