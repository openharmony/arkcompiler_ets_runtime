/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_INCREMENTAL_MARKER_H
#define ECMASCRIPT_MEM_INCREMENTAL_MARKER_H

#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/garbage_collector.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/work_manager.h"

namespace panda::ecmascript {
class EcmaVM;
class Heap;

enum class IncrementalGCStates {
    ROOT_SCAN,
    INCREMENTAL_MARK,
    REMARK,
};

// Incremental Mark only suport old gc
class IncrementalMarker {
public:
    IncrementalMarker(Heap *heap);
    ~IncrementalMarker() = default;

    void TriggerIncrementalMark(int64_t idleMicroSec = 0);
    void Reset();
    bool IsTriggeredIncrementalMark()
    {
        return isIncrementalMarking_;
    }

    void UpdateMarkingSpeed(uint32_t visitAddrNum, double costTime)
    {
        if (costTime > 1) {
            markingSpeed_ = static_cast<uint32_t>((double)visitAddrNum / costTime + markingSpeed_) >> 1;
        }
    }

    uint32_t GetMarkingSpeed()
    {
        return markingSpeed_;
    }

    void SetMarkingFinished(bool value)
    {
        markingFinished_ = value;
    }

    IncrementalGCStates GetIncrementalGCStates()
    {
        return states_;
    }

    uint32_t GetAverageIncrementalMarkingSpeed()
    {
        return incrementalMarkingSpeed_;
    }

    double GetCurrentTimeInMs();

private:
    void Mark();
    void Initialize();
    void Finish();
    void ProcessIncrementalMark(int64_t idleMicroSec);
    void RecordIdleTime(int64_t idleMicroSec, double startTime, bool needInitialize = false);
    void PrintGCIdleUsageStatistic();

    void UpdateIncrementalMarkingSpeed(double duration)
    {
        if (duration > 1) {
            incrementalMarkingSpeed_ = static_cast<uint32_t>(
                (double)startObjectSize_ / duration + incrementalMarkingSpeed_) >> 1;
        }
    }

    class RecursionScope {
    public:
        explicit RecursionScope(IncrementalMarker* marker) : marker_(marker)
        {
            if (marker_->recursionDepth_++ != 0) {
                LOG_GC(FATAL) << "Recursion in IncrementalMarker Constructor, depth:" << marker_->recursionDepth_;
            }
        }
        ~RecursionScope()
        {
            if (--marker_->recursionDepth_ != 0) {
                LOG_GC(FATAL) << "Recursion in IncrementalMarker Destructor, depth:" << marker_->recursionDepth_;
            }
        }
    private:
        IncrementalMarker* marker_ {nullptr};
    };

    Heap *heap_ {nullptr};
    EcmaVM *vm_ {nullptr};

    WorkManager *workManager_ {nullptr};
    double startTime_ {0.0};
    size_t startObjectSize_ {0};

    bool isIncrementalMarking_ {false};
    bool markingFinished_ {false};

    uint32_t markingSpeed_ {200_KB};
    uint32_t incrementalMarkingSpeed_ {100_KB};
    IncrementalGCStates states_ {IncrementalGCStates::ROOT_SCAN};

    int64_t receiveIdleTime_ {0};
    double totalUsedIdleTime_ {0.0};
    double exceedIdleTime_ {0.0};
    int32_t recursionDepth_ {0};

    friend class Heap;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_INCREMENTAL_MARKER_H
