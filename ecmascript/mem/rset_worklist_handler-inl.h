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

#ifndef ECMASCRIPT_MEM_RSET_WORKLIST_HANDLER_INL_H
#define ECMASCRIPT_MEM_RSET_WORKLIST_HANDLER_INL_H

#include "ecmascript/mem/rset_worklist_handler.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
inline RSetWorkListHandler::RSetWorkListHandler(Heap *heap, JSThread *thread) : heap_(heap), ownerThread_(thread)
{
    CollectRSetItemsInHeap(heap);
}

template<class Visitor>
inline void RSetItem::Process([[maybe_unused]] const Visitor &visitor)
{
    visitor(region_, rSet_);
}

inline void RSetItem::MergeBack()
{
    region_->MergeLocalToShareRSetForCM(rSet_);
}

inline void RSetWorkListHandler::EnumerateRegions(const Heap *heap)
{
    heap->EnumerateRegions([this](Region *region) {
        RememberedSet *rset = region->ExtractLocalToShareRSet();
        if (rset != nullptr) {
            items_.emplace_back(region, rset);
        }
    });
}

inline void RSetWorkListHandler::CollectRSetItemsInHeap(const Heap *heap)
{
    ASSERT(items_.empty());
    ASSERT(items_.capacity() == 0);
    items_.reserve(heap->GetRegionCount());
    EnumerateRegions(heap);
    ASSERT(items_.size() <= heap->GetRegionCount());
    Initialize();
}

ARK_INLINE void RSetWorkListHandler::Initialize()
{
    ASSERT(!initialized_);
    // Maybe less than -1, when js thread called MergeBackAndReset first, and then daemon thread called
    // ProcessAll, which will make it less than -1.
    ASSERT(nextItemIndex_ < 0);
    ASSERT(remainItems_ == 0);
    initialized_ = true;
    int num = static_cast<int>(items_.size());
    nextItemIndex_.store(num - 1, std::memory_order_relaxed);
    remainItems_ = num;
}

template<class Visitor>
ARK_INLINE bool RSetWorkListHandler::ProcessNext(const Visitor &visitor)
{
    // At this time, the items may be already merged back and cleared by other threads,
    // but the idx will get a negative value, so it's ok.
    int idx = nextItemIndex_.fetch_sub(1, std::memory_order_relaxed);
    if (idx < 0) {
        return false;
    }
    items_[idx].Process(visitor);
    return true;
}

template<class Visitor>
inline void RSetWorkListHandler::ProcessAllVisitor(const Visitor &visitor, int done)
{
    while (ProcessNext(visitor)) {
        ++done;
    }
    if (done > 0) {
        LockHolder lock(mutex_);
        remainItems_ -= done;
        if (remainItems_ == 0) {
            cv_.SignalAll();
        }
    }
}

template<class Visitor>
inline void RSetWorkListHandler::ProcessAll(const Visitor &visitor)
{
    ASSERT(JSThread::GetCurrent()->IsDaemonThread() ||
          (JSThread::GetCurrent() == heap_->GetEcmaVM()->GetJSThread() && JSThread::GetCurrent()->IsInRunningState()));
    // At this time, the items may be already cleared, but the ProcessNext will do nothing and return false,
    // it just means that there is nothing to work.
    int done = 0;
    ProcessAllVisitor(visitor, done);
}

// This should only be called from js thread in RUNNING state, see comments for initialized_.
inline void RSetWorkListHandler::WaitFinishedThenMergeBack()
{
    ASSERT(JSThread::GetCurrent() == heap_->GetEcmaVM()->GetJSThread());
    ASSERT(JSThread::GetCurrent()->IsInRunningState());
    ASSERT(initialized_);
    LockHolder lock(mutex_);
    while (remainItems_ != 0) {
        cv_.Wait(&mutex_);
    }
    ASSERT(remainItems_ == 0);
    [[maybe_unused]] bool res = MergeBack();
    ASSERT(res);
}

inline bool RSetWorkListHandler::TryMergeBack()
{
    return reinterpret_cast<std::atomic<bool>*>(&initialized_)->exchange(false, std::memory_order_relaxed) == true;
}

inline void RSetWorkListHandler::MergeBackForAllItem()
{
    ASSERT(nextItemIndex_ < 0);
    ASSERT(remainItems_ == 0);
    for (RSetItem &item : items_) {
        item.MergeBack();
    }
}

inline bool RSetWorkListHandler::MergeBack()
{
    ASSERT((JSThread::GetCurrent()->IsJSThread() && JSThread::GetCurrent()->IsInRunningState()) ||
           (JSThread::GetCurrent()->IsDaemonThread() && JSThread::GetCurrent()->IsInSuspendedState()));
    if (!TryMergeBack()) {
        // Is merged back by bound js thread.
        ASSERT(JSThread::GetCurrent()->IsDaemonThread());
        return false;
    }
    MergeBackForAllItem();
    // This in only called in the bound js thread, or daemon thread when SuspendAll, so this set without atomic
    // is safety.
    heap_->SetRSetWorkListHandler(nullptr);
    return true;
}

inline Heap *RSetWorkListHandler::GetHeap()
{
    return heap_;
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_RSET_WORKLIST_HANDLER_INL_H
