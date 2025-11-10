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

void ConcurrentSweeper::PostTask(bool fullGC)
{
    if (ConcurrentSweepEnabled()) {
        auto tid = heap_->GetJSThread()->GetThreadId();
        if (!fullGC) {
            common::Taskpool::GetCurrentTaskpool()->PostTask(
                std::make_unique<SweeperTask>(tid, this, OLD_SPACE, startSpaceType_));
        }
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<SweeperTask>(tid, this, NON_MOVABLE, startSpaceType_));
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<SweeperTask>(tid, this, MACHINE_CODE_SPACE, startSpaceType_));
    }
}

void ConcurrentSweeper::Sweep(bool fullGC)
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ConcurrentSweepingInitialize);
    if (ConcurrentSweepEnabled()) {
        // Add all region to region list. Ensure all task finish
        if (!fullGC) {
            heap_->GetOldSpace()->PrepareSweeping();
        }
        heap_->GetNonMovableSpace()->PrepareSweeping();
        heap_->GetMachineCodeSpace()->PrepareSweeping();
        // Prepare
        isSweeping_ = true;
        startSpaceType_ = fullGC ? NON_MOVABLE : OLD_SPACE;
        for (int type = startSpaceType_; type < FREE_LIST_NUM; type++) {
            remainingTaskNum_[type] = FREE_LIST_NUM - startSpaceType_;
        }
    } else {
        if (!fullGC) {
            heap_->GetOldSpace()->Sweep();
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
        for (int type = startSpaceType_; type < FREE_LIST_NUM; type++) {
            remainingTaskNum_[type] = FREE_LIST_NUM - startSpaceType_;
        }
    } else {
        heap_->GetOldSpace()->SweepNewToOldRegions();
    }
}

void ConcurrentSweeper::AsyncSweepSpace(MemSpaceType type, bool isMain)
{
    auto space = heap_->GetSpaceWithType(type);
    space->AsyncSweep(isMain);

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
    for (int i = startSpaceType_; i < FREE_LIST_NUM; i++) {
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
    for (int i = startSpaceType_; i < FREE_LIST_NUM; i++) {
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
        AsyncSweepSpace(type, true);
        LockHolder holder(mutexs_[type]);
        while (remainingTaskNum_[type] > 0) {
            cvs_[type].Wait(&mutexs_[type]);
        }
    }
    SparseSpace *space = heap_->GetSpaceWithType(type);
    space->FinishFillSweptRegion();
}

void ConcurrentSweeper::TryFillSweptRegion()
{
    for (int i = startSpaceType_; i < FREE_LIST_NUM; i++) {
        SparseSpace *space = heap_->GetSpaceWithType(static_cast<MemSpaceType>(i));
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
    uint32_t sweepTypeNum = FREE_LIST_NUM - startSpaceType_;
    for (size_t i = startSpaceType_; i < FREE_LIST_NUM; i++) {
        auto type = static_cast<MemSpaceType>(((i + type_) % sweepTypeNum) + startSpaceType_);
        sweeper_->AsyncSweepSpace(type, false);
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
