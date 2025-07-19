/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_CROSS_VM_WORK_MANAGER_INL_H
#define ECMASCRIPT_CROSS_VM_WORK_MANAGER_INL_H

#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/tlab_allocator-inl.h"
#include "ecmascript/mem/work_manager.h"

namespace panda::ecmascript {
    void WorkManager::PushObjectToGlobal(TaggedObject *object)
    {
        WorkNode *tempNode = AllocateWorkNode();
        ASSERT(tempNode != nullptr);
        tempNode->PushObject(ToUintPtr(object));
        workStack_.Push(tempNode);
        if (heap_->IsParallelGCEnabled() && heap_->CheckCanDistributeTask() && !heap_->IsMarking()) {
            heap_->PostParallelGCTask(parallelGCTaskPhase_);
        }
    }
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CROSS_VM_WORK_MANAGER_HYBRID_H
