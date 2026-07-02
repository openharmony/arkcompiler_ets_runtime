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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_PARTIAL_GC_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_PARTIAL_GC_H

#include "ecmascript/mem/shared_heap/shared_gc.h"

namespace panda::ecmascript {
class SharedPartialGC : public SharedGC {
public:
    explicit SharedPartialGC(SharedHeap *heap)
        : SharedGC(heap), dThread_(DaemonThread::GetInstance()) {}
    ~SharedPartialGC() override = default;
    NO_COPY_SEMANTIC(SharedPartialGC);
    NO_MOVE_SEMANTIC(SharedPartialGC);

    static TaggedObject* UpdateWeakRootVisitor(TaggedObject *object);
    void RunFlip(GCReason reason);
    void FlipUpdateWeakRoot(JSThread *thread);

    bool IsConcurrentUpdating()
    {
        return !concurrentUpdateTaskFinished_;
    }
    void WaitConcurrentUpdateFinished();
    void WaitConcurrentUpdateFinished(JSThread *thread);
    void SignalConcurrentUpdateFinished();
private:
    void RunPhases() override;
    void Sweep() override;
    void Finish() override;
    void PreSweep() override;
    void Evacuate();
    void PreProcessLocalThread();
    void PostProcessLocalThread();

    friend class SharedPartialGCSuspendCallback;
    DaemonThread *dThread_ {nullptr};
    Mutex waitConcurrentUpdateFinishedMutex_;
    ConditionVariable waitConcurrentUpdateFinishedCV_;
    bool concurrentUpdateTaskFinished_ {true};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_PARTIAL_GC_H
