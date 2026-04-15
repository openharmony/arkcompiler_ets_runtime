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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_PARALLEL_CLEANER_H
#define ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_PARALLEL_CLEANER_H

#include <atomic>
#include <vector>

#include "common_components/taskpool/task.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
class Region;
class SharedHeap;

class GlobalParallelCleaner {
public:
    explicit GlobalParallelCleaner(SharedHeap *sHeap) : sHeap_(sHeap) {}
    ~GlobalParallelCleaner() = default;

    NO_COPY_SEMANTIC(GlobalParallelCleaner);
    NO_MOVE_SEMANTIC(GlobalParallelCleaner);

    void Clean();

private:
    class CleanerTask : public common::Task {
    public:
        CleanerTask(int32_t id, GlobalParallelCleaner *cleaner, common::TaskPackMonitor *monitor)
            : common::Task(id), cleaner_(cleaner), monitor_(monitor) {}

        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(CleanerTask);
        NO_MOVE_SEMANTIC(CleanerTask);

    private:
        GlobalParallelCleaner *cleaner_ {nullptr};
        common::TaskPackMonitor *monitor_ {nullptr};
    };

    void CollectRegions();
    void ParallelCleanRegions();
    void ProcessRegions(common::TaskPackMonitor *monitor);
    void SequentialCleanRegions();
    void ClearDeadRSetForRegion(Region *region, uintptr_t endAddr);

    SharedHeap *sHeap_ {nullptr};
    std::vector<Region *> oldRegions_;
    std::vector<std::pair<Region *, uintptr_t>> youngRegions_;
    std::atomic<size_t> regionIdx_ {0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_PARALLEL_CLEANER_H
