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
            case TRIGGER_IDLE_GC_TYPE::OLD_GC:
                return "old gc";
            case TRIGGER_IDLE_GC_TYPE::FULL_GC:
                return "full gc";
            case TRIGGER_IDLE_GC_TYPE::SHARED_GC:
                return "shared gc";
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
    template<class T>
    bool ShouldCheckIdleOldGC(const T *baseHeap) const;
    template<class T>
    bool ShouldCheckIdleFullGC(const T *baseHeap) const;

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