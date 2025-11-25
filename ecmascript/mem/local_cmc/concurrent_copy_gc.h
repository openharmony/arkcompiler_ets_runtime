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

#ifndef ECMASCRIPT_MEM_LOCAL_CMC_CONCURRENT_COPY_GC_H
#define ECMASCRIPT_MEM_LOCAL_CMC_CONCURRENT_COPY_GC_H

#include "ecmascript/mem/heap.h"

namespace panda {
namespace ecmascript {
class CCTlabAllocator;
class ConcurrentCopyGC {
public:
    explicit ConcurrentCopyGC(Heap *heap);
    ~ConcurrentCopyGC() = default;
    NO_COPY_SEMANTIC(ConcurrentCopyGC);
    NO_MOVE_SEMANTIC(ConcurrentCopyGC);

    void RunPhase();
    Heap *GetHeap() const
    {
        return heap_;
    }
    void HandleUpdateFinished();
    PUBLIC_API void WaitFinished();

private:
    void ProcessWeakReference();
    void Sweep();
    void InitializeCopyPhase();
    void UpdateRoot();
    void ConcurrentCopy();
    void RunUpdatePhase();
    void ReclaimHuge();
    void Finish();

    void UpdateRecordWeakReference(uint32_t threadIndex);
    void UpdateRecordJSWeakMap(uint32_t threadIndex);
    void PreGC();
    void PostGC();

    int CalculateCopyThreadNum();

    Heap *heap_{nullptr};
    JSThread *thread_ {nullptr};
    bool ccUpdateFinished_{false};
    CCTlabAllocator *allocator_ {nullptr};
    std::atomic<size_t> runningTaskCount_ {0};
    Mutex waitMutex_;
    ConditionVariable waitCV_;

    std::vector<Region*> tasks_;
    std::atomic<size_t> taskIter_;
    friend class Heap;
    friend class ConcurrentCopyTask;
};

class ConcurrentCopyTask : public common::Task {
public:
    ConcurrentCopyTask(std::vector<Region*> &tasks, std::atomic<size_t> &taskIter,
                       std::atomic<size_t> &runningTaskCount, ConcurrentCopyGC *cc)
        : Task(0), tasks_(tasks), taskIter_(taskIter), runningTaskCount_(runningTaskCount),
          cc_(cc), totalSize_(tasks_.size()) {}
    ~ConcurrentCopyTask() override = default;
    bool Run(uint32_t threadIndex) override;

    NO_COPY_SEMANTIC(ConcurrentCopyTask);
    NO_MOVE_SEMANTIC(ConcurrentCopyTask);

private:
    Region *GetNextTask();
    std::vector<Region*> &tasks_;
    std::atomic<size_t> &taskIter_;
    std::atomic<size_t> &runningTaskCount_;
    ConcurrentCopyGC *cc_ {nullptr};
    size_t totalSize_ {0};
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_LOCAL_CMC_CONCURRENT_COPY_GC_H
