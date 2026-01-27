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

#include "ecmascript/mem/concurrent_sweeper.h"

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
ConcurrentSweeper::ConcurrentSweeper(Heap *heap, EnableConcurrentSweepType type)
    : heap_(heap),
      enableType_(type)
{
}

void ConcurrentSweeper::PostTask(TriggerGCType gcType)
{
    if (ConcurrentSweepEnabled()) {
        auto tid = heap_->GetJSThread()->GetThreadId();
        bool isFullGC = (gcType == TriggerGCType::FULL_GC);
        switch (gcType) {
            case TriggerGCType::YOUNG_GC:
            case TriggerGCType::OLD_GC:
                common::Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweeperTask>(
                    tid, this, MemSpaceType::OLD_SPACE, startSpaceType_, endSpaceType_, isFullGC));
                break;
            case TriggerGCType::CMS_GC:
                common::Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweeperTask>(
                    tid, this, MemSpaceType::SLOT_SPACE, startSpaceType_, endSpaceType_, isFullGC));
                break;
            case TriggerGCType::LOCAL_CC:
            case TriggerGCType::FULL_GC:
                break;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable, " << static_cast<int>(gcType);
                UNREACHABLE();
        }
        common::Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweeperTask>(
            tid, this, MemSpaceType::NON_MOVABLE, startSpaceType_, endSpaceType_, isFullGC));
        common::Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SweeperTask>(
            tid, this, MemSpaceType::MACHINE_CODE_SPACE, startSpaceType_, endSpaceType_, isFullGC));
    }
}

void ConcurrentSweeper::Sweep(TriggerGCType gcType)
{
    if (ConcurrentSweepEnabled()) {
        // Add all region to region list. Ensure all task finish
        switch (gcType) {
            case TriggerGCType::OLD_GC:
                startSpaceType_ = MemSpaceType::OLD_SPACE;
                endSpaceType_ = MemSpaceType::MACHINE_CODE_SPACE;
                heap_->GetOldSpace()->PrepareSweeping();
                break;
            case TriggerGCType::CMS_GC:
                startSpaceType_ = MemSpaceType::NON_MOVABLE;
                endSpaceType_ = MemSpaceType::SLOT_SPACE;
                heap_->GetSlotSpace()->PrepareSweeping();
                break;
            case TriggerGCType::LOCAL_CC:
            case TriggerGCType::FULL_GC:
                startSpaceType_ = MemSpaceType::NON_MOVABLE;
                endSpaceType_ = MemSpaceType::MACHINE_CODE_SPACE;
                break;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable, " << static_cast<int>(gcType);
                UNREACHABLE();
        }
        heap_->GetNonMovableSpace()->PrepareSweeping();
        heap_->GetMachineCodeSpace()->PrepareSweeping();
        // Prepare
        isSweeping_ = true;
        for (int type = startSpaceType_; type <= endSpaceType_; type++) {
            remainingTaskNum_[type] = endSpaceType_ - startSpaceType_ + 1;
        }
    } else {
        switch (gcType) {
            case TriggerGCType::OLD_GC:
                startSpaceType_ = MemSpaceType::OLD_SPACE;
                endSpaceType_ = MemSpaceType::MACHINE_CODE_SPACE;
                heap_->GetOldSpace()->Sweep();
                break;
            case TriggerGCType::CMS_GC:
                startSpaceType_ = MemSpaceType::NON_MOVABLE;
                endSpaceType_ = MemSpaceType::SLOT_SPACE;
                heap_->GetSlotSpace()->Sweep();
                break;
            case TriggerGCType::FULL_GC:
                startSpaceType_ = MemSpaceType::NON_MOVABLE;
                endSpaceType_ = MemSpaceType::MACHINE_CODE_SPACE;
                break;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable, " << static_cast<int>(gcType);
                UNREACHABLE();
        }
        heap_->GetNonMovableSpace()->Sweep();
        heap_->GetMachineCodeSpace()->Sweep();
    }
    heap_->GetHugeObjectSpace()->Sweep();
    heap_->GetHugeMachineCodeSpace()->Sweep();
}

void ConcurrentSweeper::SweepNewToOldRegions()
{
    if (ConcurrentSweepEnabled()) {
        heap_->GetOldSpace()->PrepareSweepNewToOldRegions();
        isSweeping_ = true;
        startSpaceType_ = OLD_SPACE;
        endSpaceType_ = MACHINE_CODE_SPACE;
        for (int type = startSpaceType_; type <= endSpaceType_; type++) {
            remainingTaskNum_[type] = endSpaceType_ - startSpaceType_ + 1;
        }
    } else {
        heap_->GetOldSpace()->SweepNewToOldRegions();
    }
}

void ConcurrentSweeper::AsyncSweepSpace(MemSpaceType type, bool isMain, bool releaseMemory)
{
    SweepableSpace *space = heap_->GetSweepableSpaceWithType(type);
    space->AsyncSweep(isMain, releaseMemory);

    LockHolder holder(mutexs_[type]);
    if (--remainingTaskNum_[type] == 0) {
        cvs_[type].SignalAll();
    }
}

void ConcurrentSweeper::WaitAllTaskFinished()
{
    if (!isSweeping_) {
        return;
    }
    for (int i = startSpaceType_; i <= endSpaceType_; i++) {
        if (remainingTaskNum_[i] > 0) {
            LockHolder holder(mutexs_[i]);
            while (remainingTaskNum_[i] > 0) {
                cvs_[i].Wait(&mutexs_[i]);
            }
        }
    }
}

void ConcurrentSweeper::EnsureAllTaskFinished()
{
    if (!isSweeping_) {
        return;
    }
    for (int i = startSpaceType_; i <= endSpaceType_; i++) {
        WaitingTaskFinish(static_cast<MemSpaceType>(i));
    }
    isSweeping_ = false;
    if (IsRequestDisabled()) {
        enableType_ = EnableConcurrentSweepType::DISABLE;
    }
}

void ConcurrentSweeper::EnsureTaskFinished(MemSpaceType type)
{
    CHECK_JS_THREAD(heap_->GetEcmaVM());
    EnsureTaskFinishedNoCheck(type);
}

void ConcurrentSweeper::EnsureTaskFinishedNoCheck(MemSpaceType type)
{
    if (!isSweeping_) {
        return;
    }
    WaitingTaskFinish(type);
}

void ConcurrentSweeper::WaitingTaskFinish(MemSpaceType type)
{
    if (remainingTaskNum_[type] > 0) {
        {
            LockHolder holder(mutexs_[type]);
            remainingTaskNum_[type]++;
        }
        AsyncSweepSpace(type, true, false);
        LockHolder holder(mutexs_[type]);
        while (remainingTaskNum_[type] > 0) {
            cvs_[type].Wait(&mutexs_[type]);
        }
    }
    SweepableSpace *space = heap_->GetSweepableSpaceWithType(type);
    space->FinishFillSweptRegion();
}

void ConcurrentSweeper::TryFillSweptRegion()
{
    for (int i = startSpaceType_; i <= endSpaceType_; i++) {
        SweepableSpace *space = heap_->GetSweepableSpaceWithType(static_cast<MemSpaceType>(i));
        space->TryFillSweptRegion();
    }
}

void ConcurrentSweeper::ClearRSetInRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd)
{
    if (ConcurrentSweepEnabled()) {
        // This clear may exist data race with array and jsobject trim, so use CAS
        current->AtomicClearSweepingOldToNewRSetInRange(freeStart, freeEnd);
        current->AtomicClearSweepingLocalToShareRSetInRange(freeStart, freeEnd);
    } else {
        current->ClearOldToNewRSetInRange(freeStart, freeEnd);
        current->ClearLocalToShareRSetInRange(freeStart, freeEnd);
    }
    current->ClearCrossRegionRSetInRange(freeStart, freeEnd);
}

bool ConcurrentSweeper::SweeperTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ConcurrentSweeper::Sweep", "");
    uint32_t sweepTypeNum = endSpaceType_ - startSpaceType_ + 1;
    for (size_t i = 0; i < sweepTypeNum; i++) {
        auto type = static_cast<MemSpaceType>(((i + type_) % sweepTypeNum) + startSpaceType_);
        bool releaseMemory = (type == MemSpaceType::NON_MOVABLE) && isFullGC_;
        sweeper_->AsyncSweepSpace(type, false, releaseMemory);
    }
    return true;
}

void ConcurrentSweeper::EnableConcurrentSweep(EnableConcurrentSweepType type)
{
    if (IsConfigDisabled()) {
        return;
    }
    if (ConcurrentSweepEnabled() && isSweeping_ && type == EnableConcurrentSweepType::DISABLE) {
        enableType_ = EnableConcurrentSweepType::REQUEST_DISABLE;
    } else {
        enableType_ = type;
    }
}
}  // namespace panda::ecmascript
