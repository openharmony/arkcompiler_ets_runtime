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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CONCURRENT_MARKER_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CONCURRENT_MARKER_H

#include "ecmascript/daemon/daemon_thread.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/shared_heap/shared_gc_marker.h"

namespace panda::ecmascript {
class EcmaVM;
class SharedHeap;

class SharedConcurrentMarker {
public:
    explicit SharedConcurrentMarker(EnableConcurrentMarkType type);
    ~SharedConcurrentMarker() = default;

    /*
     * Concurrent marking related configurations and utilities.
     */
    void EnableConcurrentMarking(EnableConcurrentMarkType type);

    bool IsEnabled() const
    {
        return !IsDisabled();
    }

    bool IsDisabled() const
    {
        return enableMarkType_ == EnableConcurrentMarkType::DISABLE ||
            enableMarkType_ == EnableConcurrentMarkType::CONFIG_DISABLE;
    }

    void ConfigConcurrentMark(bool enabled)
    {
        enableMarkType_ = enabled ? EnableConcurrentMarkType::ENABLE :
                          EnableConcurrentMarkType::CONFIG_DISABLE;
    }

    bool IsRequestDisabled() const
    {
        return enableMarkType_ == EnableConcurrentMarkType::REQUEST_DISABLE;
    }

    bool IsConfigDisabled() const
    {
        return enableMarkType_ == EnableConcurrentMarkType::CONFIG_DISABLE;
    }

    bool IsTriggeredConcurrentMark() const
    {
        return isConcurrentMarking_;
    }
    void Mark(TriggerGCType gcType);             // In daemon thread
    void ReMark();                          // In daemon thread

    void Reset(bool clearGCBits);    // In daemon thread

    void ResetWorkManager(SharedGCWorkManager *sWorkManager);       // In js thread

    double GetDuration() const
    {
        return duration_;
    }

    double GetHeapObjectSize() const
    {
        return sHeapObjectSize_;
    }

private:
    NO_COPY_SEMANTIC(SharedConcurrentMarker);
    NO_MOVE_SEMANTIC(SharedConcurrentMarker);

    class RecursionScope {
    public:
        explicit RecursionScope(SharedConcurrentMarker* sMarker) : sMarker_(sMarker)
        {
            if (sMarker_->recursionDepth_++ != 0) {
                LOG_GC(FATAL) << "Recursion in SharedConcurrentMarker Constructor, depth: "
                              << sMarker_->recursionDepth_;
            }
        }
        ~RecursionScope()
        {
            if (--sMarker_->recursionDepth_ != 0) {
                LOG_GC(FATAL) << "Recursion in SharedConcurrentMarker Destructor, depth: "
                              << sMarker_->recursionDepth_;
            }
        }
    private:
        SharedConcurrentMarker* sMarker_ {nullptr};
    };

    void SetDuration(double duration)
    {
        duration_ = duration;
    }

    void MarkRoots(SharedMarkType markType);    // In daemon thread
    void InitializeMarking();                   // In daemon thread
    void DoMarking();                           // In daemon thread
    void FinishMarking(float spendTime);        // In daemon thread
    void HandleMarkingFinished();               // In daemon thread

    void Finish();                              // In daemon thread

    SharedHeap *sHeap_ {nullptr};
    DaemonThread *dThread_ {nullptr};

    // obtained from the shared heap instance.
    SharedGCWorkManager *sWorkManager_ {nullptr};
    size_t sHeapObjectSize_ {0};
    double duration_ {0.0};
    EnableConcurrentMarkType enableMarkType_ {EnableConcurrentMarkType::CONFIG_DISABLE};

    bool isConcurrentMarking_ {false};
    int32_t recursionDepth_ {0};
    TriggerGCType gcType_ {TriggerGCType::SHARED_GC};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CONCURRENT_MARKER_H
