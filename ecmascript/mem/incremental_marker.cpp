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

#include "ecmascript/mem/incremental_marker.h"

#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/parallel_marker.h"
#include "ecmascript/mem/old_gc_visitor-inl.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
IncrementalMarker::IncrementalMarker(Heap *heap)
    : heap_(heap),
    vm_(heap->GetEcmaVM()),
    workManager_(heap->GetWorkManager()) {}

void IncrementalMarker::TriggerIncrementalMark(int64_t idleMicroSec)
{
    ASSERT(!heap_->GetConcurrentMarker()->IsTriggeredConcurrentMark());
    double startTime = GetCurrentTimeInMs();
    switch (states_) {
        case IncrementalGCStates::ROOT_SCAN:
            Mark();
            ProcessIncrementalMark(idleMicroSec - (GetCurrentTimeInMs() - startTime));
            RecordIdleTime(idleMicroSec, startTime, true);
            break;
        case IncrementalGCStates::INCREMENTAL_MARK:
            ProcessIncrementalMark(idleMicroSec);
            RecordIdleTime(idleMicroSec, startTime);
            break;
        case IncrementalGCStates::REMARK:
            UpdateIncrementalMarkingSpeed(GetCurrentTimeInMs() - startTime_);
            heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::IDLE);
            RecordIdleTime(idleMicroSec, startTime);
            PrintGCIdleUsageStatistic();
            break;
        default: // LOCV_EXCL_BR_LINE
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void IncrementalMarker::MarkRoots()
{
    ASSERT(heap_->IsFullMark());
    OldGCMarkRootVisitor oldGCMarkRootVisitor(workManager_->GetWorkNodeHolder(MAIN_THREAD_INDEX));
    heap_->GetNonMovableMarker()->MarkRoots(oldGCMarkRootVisitor);
}

void IncrementalMarker::Mark()
{
    LOG_GC(DEBUG) << "IncrementalMarker: Incremental Marking Begin";
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "IncrementalMarker::Mark");
    MEM_ALLOCATE_AND_GC_TRACE(vm_, IncrementalMarking);
    Initialize();
    MarkRoots();
    workManager_->GetWorkNodeHolder(MAIN_THREAD_INDEX)->PushWorkNodeToGlobal(false);
    states_ = IncrementalGCStates::INCREMENTAL_MARK;
}

void IncrementalMarker::Initialize()
{
    MEM_ALLOCATE_AND_GC_TRACE(vm_, IncrementalMarkingInitialize);
    startTime_ = GetCurrentTimeInMs();
    startObjectSize_ = heap_->GetHeapObjectSize();
    isIncrementalMarking_ = true;
    markingFinished_ = false;
    heap_->SetMarkType(MarkType::MARK_FULL);
    heap_->GetJSThread()->SetMarkStatus(MarkStatus::MARKING);
    heap_->Prepare();
    heap_->GetOldSpace()->SelectCSet();
    heap_->GetAppSpawnSpace()->EnumerateRegions([](Region *current) {
        current->ClearMarkGCBitset();
        current->ClearCrossRegionRSet();
    });
    heap_->EnumerateNonNewSpaceRegions([](Region *current) {
        current->ResetAliveObject();
    });

    workManager_->Initialize(TriggerGCType::OLD_GC, ParallelGCTaskPhase::OLD_HANDLE_GLOBAL_POOL_TASK);
}

void IncrementalMarker::ProcessIncrementalMark(int64_t idleMicroSec)
{
    RecursionScope recurScope(this);
    if (idleMicroSec <= 0) {
        return;
    }
    LOG_GC(DEBUG) << "IncrementalMarker: Process Incremental Marking";
    uint32_t markStepSize = static_cast<uint32_t>(idleMicroSec) * markingSpeed_;
    heap_->GetNonMovableMarker()->ProcessIncrementalMarkStack(MAIN_THREAD_INDEX, markStepSize);
    if (markingFinished_) {
        states_ = IncrementalGCStates::REMARK;
        heap_->GetJSThread()->SetMarkStatus(MarkStatus::MARK_FINISHED);
        return;
    }
}

void IncrementalMarker::Finish()
{
    states_ = IncrementalGCStates::ROOT_SCAN;
    isIncrementalMarking_ = false;
    heap_->GetJSThread()->SetMarkStatus(MarkStatus::READY_TO_MARK);
    heap_->GetOldSpace()->RevertCSet();
    auto callback = [](Region *region) {
        region->ClearMarkGCBitset();
        region->ClearCrossRegionRSet();
        region->ResetAliveObject();
    };
    heap_->EnumerateRegions(callback);
}

double IncrementalMarker::GetCurrentTimeInMs()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<double>(tv.tv_sec) * MS_PER_SECOND + (tv.tv_usec / MS_PER_SECOND);
}

void IncrementalMarker::Reset()
{
    isIncrementalMarking_ = false;
    states_ = IncrementalGCStates::ROOT_SCAN;
    heap_->GetJSThread()->SetMarkStatus(MarkStatus::READY_TO_MARK);
    workManager_->Finish();
}

void IncrementalMarker::RecordIdleTime(int64_t idleMicroSec, double startTime, bool needInitialize)
{
    if (needInitialize) {
        receiveIdleTime_ = 0;
        totalUsedIdleTime_ = 0.0;
        exceedIdleTime_ = 0.0;
    }
    double usedIdleTime = GetCurrentTimeInMs() - startTime;
    receiveIdleTime_ += idleMicroSec;
    totalUsedIdleTime_ += std::min((double)idleMicroSec, usedIdleTime);
    exceedIdleTime_ += usedIdleTime > idleMicroSec ? usedIdleTime - idleMicroSec : 0;
}

void IncrementalMarker::PrintGCIdleUsageStatistic()
{
    if (!vm_->IsOptionalLogEnabled()) {
        return;
    }
    LOG_GC(INFO) << "/************* GCStats Idle usage statistic: *************/";
    LOG_GC(INFO) << "Receive idle time:   " << receiveIdleTime_ << "ms\n"
                 << "GC in idle time:     " << totalUsedIdleTime_ << "ms\n"
                 << "GC out of idle time: " << exceedIdleTime_ << "ms\n"
                 << "Total duration of gc:" << GetCurrentTimeInMs() - startTime_ << "ms";
}
}  // namespace panda::ecmascript
