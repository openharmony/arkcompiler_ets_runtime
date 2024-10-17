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

#include "ecmascript/dfx/hprof/heap_tracker.h"

#include "ecmascript/dfx/hprof/heap_snapshot.h"

namespace panda::ecmascript {
static constexpr int32_t MILLI_TO_MICRO = 1000;

void HeapTrackerSample::Run()
{
    while (!isInterrupt_) {
        snapshot_->RecordSampleTime();
        if (stream_ != nullptr) {
            snapshot_->PushHeapStat(stream_);
        }
        usleep(timeInterval_ * MILLI_TO_MICRO);
    }
}

void HeapTracker::AllocationEvent(TaggedObject *address, size_t size)
{
    if (snapshot_ == nullptr || address == nullptr || prevAllocation_ == address) {
        return;
    }
    prevAllocation_ = address;
    Node *node = snapshot_->AddNode(address, size);
    if (node != nullptr && snapshot_->trackAllocations()) {
        int selfSize = static_cast<int>(node->GetSelfSize());
        int sequenceId = static_cast<int>(node->GetId());
        int traceId = snapshot_->AddTraceNode(sequenceId, selfSize);
        if (traceId != -1) {
            node->SetTraceId(static_cast<uint64_t>(traceId));
        }
    }
}

void HeapTracker::MoveEvent(uintptr_t address, TaggedObject *forwardAddress, size_t size)
{
    if (snapshot_ != nullptr) {
        snapshot_->MoveNode(address, forwardAddress, size);
    }
}
}  // namespace panda::ecmascript
