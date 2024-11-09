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
#ifndef ECMASCRIPT_MEM_GC_TRIGGER_H
#define ECMASCRIPT_MEM_GC_TRIGGER_H

#include <atomic>
#include <ctime>
#include <chrono>

#include "libpandabase/macros.h"
#include "ecmascript/common.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/base/gc_ring_buffer.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/napi/include/jsnapi_expo.h"

namespace panda::ecmascript {
class Heap;
class SharedHeap;
class ConcurrentMarker;
class MemController;
class SharedMemController;
class EcmaVM;

class IdleGCTrigger {
using TRIGGER_IDLE_GC_TYPE = panda::JSNApi::TRIGGER_IDLE_GC_TYPE;
using Clock = std::chrono::high_resolution_clock;
public:
    explicit IdleGCTrigger(Heap *heap, SharedHeap *sHeap, JSThread *thread, bool logEnable = false)
        : heap_(heap),
        sHeap_(sHeap),
        thread_(thread),
        optionalLogEnabled_(logEnable) {};
    virtual ~IdleGCTrigger() = default;

    bool IsIdleState() const
    {
        return idleState_.load();
    }

    const char *GetGCTypeName(TRIGGER_IDLE_GC_TYPE gcType) const
    {
        switch (gcType) {
            case TRIGGER_IDLE_GC_TYPE::FULL_GC:
                return "full gc";
            case TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK:
                return "shared concurrent mark";
            case TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC:
                return "shared full gc";
            case TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_MARK:
                return "local concurrent mark";
            case TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK:
                return "local remark";
            default:
                return "UnknownType";
        }
    }

    bool IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE gcType) const
    {
        uint8_t bit = static_cast<uint8_t>(gcType);
        return (bit & gcTaskPostedState_) != bit;
    }

    void SetPostGCTask(TRIGGER_IDLE_GC_TYPE gcType)
    {
        uint8_t bit = static_cast<uint8_t>(gcType);
        gcTaskPostedState_ = (gcTaskPostedState_ | bit);
    }

    void ClearPostGCTask(TRIGGER_IDLE_GC_TYPE gcType)
    {
        uint8_t bit = static_cast<uint8_t>(gcType);
        gcTaskPostedState_ = (gcTaskPostedState_ & ~bit);
    }

    void SetTriggerGCTaskCallback(const TriggerGCTaskCallback& callback)
    {
        triggerGCTaskCallback_ = callback;
    }

    void NotifyVsyncIdleStart();
    void NotifyLooperIdleStart(int64_t timestamp, int idleTime);
    void NotifyLooperIdleEnd(int64_t timestamp);
    void TryTriggerHandleMarkFinished();
    void TryTriggerLocalConcurrentMark();
    bool TryTriggerIdleLocalOldGC();
    bool TryTriggerIdleSharedOldGC();
    bool ReachIdleLocalOldGCThresholds();
    bool ReachIdleSharedGCThresholds();
    void TryPostHandleMarkFinished();
    void TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE gcType);
    bool CheckIdleYoungGC() const;
    bool CheckLocalBindingNativeTriggerFullGC() const;
    template<class T>
    bool CheckIdleOrHintOldGC(const T *baseHeap) const
    {
        size_t heapAliveSizeAfterGC = baseHeap->GetHeapAliveSizeAfterGC();
        if (heapAliveSizeAfterGC == 0) {
            return false;
        }
        size_t expectHeapSize = std::max(static_cast<size_t>(heapAliveSizeAfterGC * IDLE_SPACE_SIZE_MIN_INC_RATIO),
            heapAliveSizeAfterGC + IDLE_SPACE_SIZE_MIN_INC_STEP);
        LOG_ECMA_IF(optionalLogEnabled_, DEBUG) << "IdleGCTrigger: check old GC heapAliveSizeAfterGC:"
            << heapAliveSizeAfterGC << ";expectHeapSize" << expectHeapSize
            << "heapObjectSize" << baseHeap->GetHeapObjectSize();
        return baseHeap->GetHeapObjectSize() >= expectHeapSize;
    }

    template<class T>
    bool CheckIdleOrHintFullGC(const T *baseHeap) const
    {
        size_t heapAliveSizeAfterGC = baseHeap->GetHeapAliveSizeAfterGC();
        size_t expectHeapSize = std::max(static_cast<size_t>(heapAliveSizeAfterGC * IDLE_SPACE_SIZE_MIN_INC_RATIO),
            heapAliveSizeAfterGC + IDLE_SPACE_SIZE_MIN_INC_STEP_FULL);
        LOG_GC(DEBUG) << "IdleGCTrigger: check full GC heapAliveSizeAfterGC:" << heapAliveSizeAfterGC
            << ";expectHeapSize:" << expectHeapSize << ";heapObjectSize:" << baseHeap->GetHeapObjectSize();
        if (baseHeap->GetHeapObjectSize() >= expectHeapSize) {
            return true;
        }
        size_t fragmentSizeAfterGC = baseHeap->GetFragmentSizeAfterGC();
        size_t heapBasicLoss = baseHeap->GetHeapBasicLoss();
        if (fragmentSizeAfterGC <= heapBasicLoss) {
            return false;
        }
        size_t fragmentSize = fragmentSizeAfterGC - heapBasicLoss;
        size_t expectFragmentSize = std::max(static_cast<size_t>((baseHeap->GetCommittedSize() - heapBasicLoss) *
            IDLE_FRAGMENT_SIZE_RATIO), IDLE_MIN_EXPECT_RECLAIM_SIZE);
        LOG_GC(DEBUG) << "IdleGCTrigger: check full GC fragmentSizeAfterGC:" << fragmentSizeAfterGC
            << ";heapBasicLoss:" << heapBasicLoss << ";expectFragmentSize" << expectFragmentSize;
        return fragmentSize >= expectFragmentSize;
    }

    template<class T>
    bool HintGCInLowDegree(const T *baseHeap) const
    {
        return CheckIdleOrHintOldGC<T>(baseHeap);
    }

    template<class T>
    bool HintGCInMiddleDegree(const T *baseHeap) const
    {
        return CheckIdleOrHintOldGC<T>(baseHeap);
    }

    template<class T>
    bool HintGCInHighDegree(const T *baseHeap) const
    {
        return CheckIdleOrHintFullGC<T>(baseHeap);
    }

private:
    void PostIdleGCTask(TRIGGER_IDLE_GC_TYPE gcType);

    Heap *heap_ {nullptr};
    SharedHeap *sHeap_ {nullptr};
    JSThread *thread_ {nullptr};
    bool optionalLogEnabled_ {false};

    std::atomic<bool> idleState_ {false};
    uint8_t gcTaskPostedState_ {0};
    TriggerGCTaskCallback triggerGCTaskCallback_ {nullptr};
};

}

#endif  // ECMASCRIPT_MEM_GC_TRIGGER_H