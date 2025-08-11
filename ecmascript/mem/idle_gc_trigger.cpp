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

#include "ecmascript/mem/idle_gc_trigger.h"


#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/heap-inl.h"

namespace panda::ecmascript {
void IdleGCTrigger::NotifyVsyncIdleStart()
{
    TryTriggerIdleLocalOldGC();
}

bool IdleGCTrigger::NotifyLooperIdleStart([[maybe_unused]] int64_t timestamp, [[maybe_unused]] int idleTime)
{
    LOG_ECMA_IF(optionalLogEnabled_, DEBUG) << "IdleGCTrigger: recv once looper idle time";
    idleState_.store(true);
    if (heap_->GetJSThread()->IsMarkFinished() &&
        heap_->GetConcurrentMarker()->IsTriggeredConcurrentMark() &&
        thread_->IsReadyToSharedConcurrentMark()) {
        return PostIdleGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK);
    }
    if (!IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK) ||
        !IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK) ||
        !IsPossiblePostGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK)) {
        return true;
    }
    if (!heap_->CheckCanTriggerConcurrentMarking()) {
        return false;
    }
    return TryTriggerIdleLocalOldGC() || TryTriggerIdleYoungGC() || TryTriggerIdleSharedOldGC();
}

void IdleGCTrigger::NotifyLooperIdleEnd([[maybe_unused]] int64_t timestamp)
{
    idleState_.store(false);
}

void IdleGCTrigger::TryTriggerHandleMarkFinished()
{
    // wait sharedGC finish
    if (!thread_->IsReadyToSharedConcurrentMark()) {
        return ;
    }
    if (heap_->GetJSThread()->IsMarkFinished() && heap_->GetConcurrentMarker()->IsTriggeredConcurrentMark()
        && !heap_->GetOnSerializeEvent() && !heap_->InSensitiveStatus()) {
        heap_->SetCanThrowOOMError(false);
        heap_->GetConcurrentMarker()->HandleMarkingFinished(GCReason::IDLE);
        heap_->SetCanThrowOOMError(true);
    }
}

void IdleGCTrigger::TryTriggerLocalConcurrentMark(MarkType type)
{
    if (heap_->GetConcurrentMarker()->IsEnabled() && heap_->CheckCanTriggerConcurrentMarking()) {
        heap_->SetMarkType(type);
        heap_->TriggerConcurrentMarking(MarkReason::IDLE);
    }
}

bool IdleGCTrigger::TryTriggerIdleYoungGC()
{
    if (CheckIdleYoungGC() && !heap_->NeedStopCollection()) {
        return PostIdleGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK);
    }
    return false;
}

bool IdleGCTrigger::TryTriggerIdleLocalOldGC()
{
    if (heap_->GetJSThread()->FullMarkRequest() && !heap_->NeedStopCollection()) {
        heap_->GetJSThread()->ResetFullMarkRequest();
        return PostIdleGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    }
    if (CheckIdleLocalOldGC(heap_) && ReachIdleLocalOldGCThresholds() && !heap_->NeedStopCollection()) {
        return PostIdleGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK);
    }
    return false;
}

bool IdleGCTrigger::TryTriggerIdleSharedOldGC()
{
    if (!CheckIdleOrHintOldGC<SharedHeap>(sHeap_) || sHeap_->NeedStopCollection() ||
        !sHeap_->CheckCanTriggerConcurrentMarking(thread_)) {
        return false;
    }
    if (ReachIdleSharedGCThresholds()) {
        return PostIdleGCTask(TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK);
    }
    return false;
}

bool IdleGCTrigger::ReachIdleLocalOldGCThresholds()
{
    bool isFullMarking = heap_->IsConcurrentFullMark() && heap_->GetJSThread()->IsMarking();
    bool isNativeSizeLargeTrigger = isFullMarking ? false : heap_->GlobalNativeSizeLargerThanLimitForIdle();
    if (isNativeSizeLargeTrigger) {
        return true;
    }

    OldSpace *oldSpace = heap_->GetOldSpace();
    HugeObjectSpace *hugeObjectSpace = heap_->GetHugeObjectSpace();
    size_t idleSizeLimit = static_cast<size_t>(oldSpace->GetInitialCapacity() *
                                                IDLE_SPACE_SIZE_LIMIT_RATE);
    size_t currentSize = oldSpace->GetHeapObjectSize() + hugeObjectSpace->GetHeapObjectSize();
    if (currentSize >= idleSizeLimit) {
        return true;
    }

    size_t maxCapacity = oldSpace->GetMaximumCapacity() + oldSpace->GetOvershootSize() +
                        oldSpace->GetOutOfMemoryOvershootSize();
    size_t currentCapacity = oldSpace->GetCommittedSize() + hugeObjectSpace->GetCommittedSize();
    size_t idleCapacityLimit = static_cast<size_t>(maxCapacity * IDLE_SPACE_SIZE_LIMIT_RATE);
    if (currentCapacity >= idleCapacityLimit) {
        return true;
    }

    size_t oldSpaceAllocLimit = heap_->GetGlobalSpaceAllocLimit() + oldSpace->GetOvershootSize();
    size_t idleOldSpaceAllocLimit = static_cast<size_t>(oldSpaceAllocLimit * IDLE_SPACE_SIZE_LIMIT_RATE);
    if (heap_->GetHeapObjectSize() > idleOldSpaceAllocLimit) {
        return true;
    }
    return false;
}

bool IdleGCTrigger::ReachIdleSharedPartialGCThresholds()
{
    size_t expectGlobalSizeLimit = sHeap_->GetGlobalSpaceAllocLimit() * IDLE_PATIAL_GC_SPACE_SIZE_LIMIT_RATE;
    return sHeap_->GetHeapObjectSize() > expectGlobalSizeLimit;
}

bool IdleGCTrigger::ReachIdleSharedGCThresholds()
{
    size_t expectSizeLimit = sHeap_->GetOldSpace()->GetInitialCapacity() * IDLE_SPACE_SIZE_LIMIT_RATE;
    size_t currentOldSize = sHeap_->GetOldSpace()->GetHeapObjectSize();
    size_t expectGlobalSizeLimit = sHeap_->GetGlobalSpaceAllocLimit() * IDLE_SPACE_SIZE_LIMIT_RATE;
    return currentOldSize > expectSizeLimit || sHeap_->GetHeapObjectSize() > expectGlobalSizeLimit;
}

void IdleGCTrigger::TryPostHandleMarkFinished()
{
    if (IsIdleState()) {
        PostIdleGCTask(TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK);
    }
}

bool IdleGCTrigger::PostIdleGCTask(TRIGGER_IDLE_GC_TYPE gcType)
{
    if (triggerGCTaskCallback_ != nullptr && IsPossiblePostGCTask(gcType) && !heap_->NeedStopCollection()) {
        std::pair<void*, uint8_t> data(heap_->GetEcmaVM(), static_cast<uint8_t>(gcType));
        triggerGCTaskCallback_(data);
        SetPostGCTask(gcType);
        LOG_GC(INFO) << "IdleGCTrigger: post once " << GetGCTypeName(gcType) << " on idleTime";
        return true;
    }
    LOG_GC(DEBUG) << "IdleGCTrigger: failed to post once " << GetGCTypeName(gcType);
    return false;
}

bool IdleGCTrigger::CheckIdleYoungGC(bool isLongIdle) const
{
    auto newSpace = heap_->GetNewSpace();
    size_t allocatedSizeSinceGC = newSpace->GetAllocatedSizeSinceGC(newSpace->GetTop());
    LOG_GC(DEBUG) << "IdleGCTrigger: check young GC semi Space size since gc:" << allocatedSizeSinceGC;
    if (isLongIdle && allocatedSizeSinceGC > IDLE_MIN_EXPECT_RECLAIM_SIZE) {
        return true;
    }
    size_t expectIdleLimitSize = (newSpace->GetInitialCapacity() + newSpace->GetOvershootSize()) *
                                    IDLE_SPACE_SIZE_LIMIT_RATE;
    return allocatedSizeSinceGC > IDLE_MIN_EXPECT_RECLAIM_SIZE && newSpace->GetObjectSize() >= expectIdleLimitSize;
}

bool IdleGCTrigger::CheckIdleLocalOldGC(const Heap *heap) const
{
    size_t afterGCSize = heap->GetHeapAliveSizeExcludesYoungAfterGC();
    size_t currentSize = heap->GetHeapObjectSize() - heap->GetNewSpace()->GetHeapObjectSize();
    size_t expectSize = std::max(static_cast<size_t>(afterGCSize * IDLE_SPACE_SIZE_MIN_INC_RATIO),
        afterGCSize + IDLE_SPACE_SIZE_MIN_INC_STEP);
    return currentSize >= expectSize;
}

bool IdleGCTrigger::CheckLocalBindingNativeTriggerOldGC() const
{
    size_t nativeBindingSize = heap_->GetNativeBindingSize();
    size_t nativeBindingSizeLastGC = heap_->GetNativeBindingSizeAfterLastGC();
    if (nativeBindingSize <= nativeBindingSizeLastGC) {
        return false;
    }
    size_t expectIncrementalNative = std::max(IDLE_BINDING_NATIVE_MIN_INC_SIZE,
        static_cast<size_t>(nativeBindingSizeLastGC * IDLE_BINDING_NATIVE_MIN_INC_RATIO));
    size_t incrementalNative = nativeBindingSize - nativeBindingSizeLastGC;
    LOG_GC(DEBUG) << "IdleGCTrigger: check old GC expectIncrementalNative:" << expectIncrementalNative
        << ";incrementalNative:" << incrementalNative;
    return incrementalNative > expectIncrementalNative;
}

void IdleGCTrigger::TryTriggerIdleGC(TRIGGER_IDLE_GC_TYPE gcType)
{
    if (ecmascript::g_isEnableCMCGC) {
        if (gcType == TRIGGER_IDLE_GC_TYPE::FULL_GC) {
            common::Heap::GetHeap().TryIdleGC();
        }
        return;
    }
    LOG_GC(DEBUG) << "IdleGCTrigger: recv once notify of " << GetGCTypeName(gcType);
    switch (gcType) {
        case TRIGGER_IDLE_GC_TYPE::FULL_GC:
            if (CheckIdleOrHintFullGC<Heap>(heap_) && !heap_->NeedStopCollection()) {
                LOG_GC(INFO) << "IdleGCTrigger: trigger " << GetGCTypeName(gcType);
                heap_->CollectGarbage(TriggerGCType::FULL_GC, GCReason::IDLE);
            } else if (CheckLocalBindingNativeTriggerOldGC() && !heap_->NeedStopCollection()) {
                LOG_GC(INFO) << "IdleGCTrigger: trigger local old GC by native binding size.";
                heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::IDLE_NATIVE);
            } else if (CheckIdleYoungGC(true) && !heap_->NeedStopCollection()) {
                LOG_GC(INFO) << "IdleGCTrigger: trigger young gc";
                heap_->CollectGarbage(TriggerGCType::YOUNG_GC, GCReason::IDLE);
            }
            break;
        case TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_PARTIAL_MARK:
            if (CheckIdleOrHintOldGC<SharedHeap>(sHeap_) && sHeap_->CheckCanTriggerConcurrentMarking(thread_)
                && !sHeap_->NeedStopCollection()) {
                LOG_GC(INFO) << "IdleGCTrigger: trigger " << GetGCTypeName(gcType);
                sHeap_->TriggerConcurrentMarking<TriggerGCType::SHARED_PARTIAL_GC, MarkReason::IDLE>(thread_);
            }
            break;
        case TRIGGER_IDLE_GC_TYPE::SHARED_CONCURRENT_MARK:
            if (CheckIdleOrHintOldGC<SharedHeap>(sHeap_) && sHeap_->CheckCanTriggerConcurrentMarking(thread_)
                && !sHeap_->NeedStopCollection()) {
                LOG_GC(INFO) << "IdleGCTrigger: trigger " << GetGCTypeName(gcType);
                sHeap_->TriggerConcurrentMarking<TriggerGCType::SHARED_GC, MarkReason::IDLE>(thread_);
            }
            break;
        case TRIGGER_IDLE_GC_TYPE::SHARED_FULL_GC:
            if (CheckIdleOrHintFullGC<SharedHeap>(sHeap_) && !sHeap_->NeedStopCollection()) {
                LOG_GC(INFO) << "IdleGCTrigger: trigger " << GetGCTypeName(gcType);
                sHeap_->CompressCollectGarbageNotWaiting<GCReason::IDLE>(thread_);
            }
            break;
        case TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_YOUNG_MARK:
            if (CheckIdleYoungGC() && !heap_->NeedStopCollection()) {
                LOG_GC(INFO) << "IdleGCTrigger: trigger " << GetGCTypeName(gcType);
                TryTriggerLocalConcurrentMark(MarkType::MARK_YOUNG);
            }
            break;
        case TRIGGER_IDLE_GC_TYPE::LOCAL_CONCURRENT_FULL_MARK:
            if (CheckIdleLocalOldGC(heap_) && !heap_->NeedStopCollection()) {
                LOG_GC(INFO) << "IdleGCTrigger: trigger " << GetGCTypeName(gcType);
                TryTriggerLocalConcurrentMark(MarkType::MARK_FULL);
            }
            break;
        case TRIGGER_IDLE_GC_TYPE::LOCAL_REMARK:
            if (!heap_->NeedStopCollection()) {
                LOG_GC(INFO) << "IdleGCTrigger: trigger " << GetGCTypeName(gcType);
                TryTriggerHandleMarkFinished();
            }
            break;
        default: // LCOV_EXCL_BR_LINE
            LOG_GC(ERROR) << "IdleGCTrigger: this branch is unreachable";
            return;
    }
    ClearPostGCTask(gcType);
}
}
