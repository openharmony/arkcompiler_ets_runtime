/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_CONCURRENT_MARKER_H
#define ECMASCRIPT_MEM_CONCURRENT_MARKER_H

#include <array>
#include <atomic>

#include "ecmascript/common.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/work_manager.h"
#include "ecmascript/taskpool/task.h"

#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
class EcmaVM;
class Heap;
// CONFIG_DISABLE means concurrent marker is disabled by options or macros and cannot be changed.
// REQUEST_DISABLE means we want to disable concurrent sweeper while it is marking.
// REQUEST_DISABLE can be ragarded as enable and will be changed into disable after this GC.
enum class EnableConcurrentMarkType : uint8_t {
    ENABLE,
    CONFIG_DISABLE,
    DISABLE,
    REQUEST_DISABLE
};

class ConcurrentMarker {
public:
    ConcurrentMarker(Heap *heap, EnableConcurrentMarkType type);
    ~ConcurrentMarker() = default;

    static bool TryIncreaseTaskCounts()
    {
        size_t taskPoolSize = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
        {
            LockHolder holder(taskCountMutex_);
            // total counts of running concurrent mark tasks should be less than taskPoolSize
            if (taskCounts_ + 1 < taskPoolSize) {
                taskCounts_++;
                return true;
            }
        }
        LOG_FULL(INFO) << "Concurrent mark tasks in taskPool are full";
        return false;
    }

    static void DecreaseTaskCounts()
    {
        LockHolder holder(taskCountMutex_);
        taskCounts_--;
    }

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
    void ProcessConcurrentMarkTask(uint32_t threadId);
    void Mark();
    void Finish();
    void ReMark();

    void HandleMarkingFinished(GCReason gcReason = GCReason::HANDLE_MARKING_FINISHED);  // call in vm thread.
    void WaitMarkingFinished();  // call in main thread
    void Reset(bool revertCSet = true);

    double GetDuration() const
    {
        return duration_;
    }

    double GetHeapObjectSize() const
    {
        return heapObjectSize_;
    }

private:
    NO_COPY_SEMANTIC(ConcurrentMarker);
    NO_MOVE_SEMANTIC(ConcurrentMarker);

    class RecursionScope {
    public:
        explicit RecursionScope(ConcurrentMarker* marker) : marker_(marker)
        {
            if (marker_->recursionDepth_++ != 0) {
                LOG_GC(FATAL) << "Recursion in ConcurrentMarker Constructor, depth: " << marker_->recursionDepth_;
            }
        }
        ~RecursionScope()
        {
            if (--marker_->recursionDepth_ != 0) {
                LOG_GC(FATAL) << "Recursion in ConcurrentMarker Destructor, depth: " << marker_->recursionDepth_;
            }
        }
    private:
        ConcurrentMarker* marker_ {nullptr};
    };

    void SetDuration(double duration)
    {
        duration_ = duration;
    }

    void MarkRoots();
    void InitializeMarking();
    bool ShouldNotifyMarkingFinished();  // call in GC thread.
    void FinishMarking();   // call in GC thread.
    bool VerifyAllRegionsNonFresh();

    static size_t taskCounts_;
    static Mutex taskCountMutex_;

    Heap *heap_ {nullptr};
    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};

    // obtained from the shared heap instance.
    WorkManager *workManager_ {nullptr};
    size_t heapObjectSize_ {0};
    double duration_ {0.0};
    EnableConcurrentMarkType enableMarkType_ {EnableConcurrentMarkType::CONFIG_DISABLE};

    std::atomic<int> runningTaskCount_ {0};
    bool notifyMarkingFinished_ {false};    // Use different value from markingFinished_ to prevent JSThread waking up
                                            // before FinishMarking finishes.
    bool markingFinished_ {false};
    bool isConcurrentMarking_ {false};
    Mutex waitMarkingFinishedMutex_;
    ConditionVariable waitMarkingFinishedCV_;
    int32_t recursionDepth_ {0};
    ClockScope clockScope_;

    friend class Heap;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_CONCURRENT_MARKER_H
