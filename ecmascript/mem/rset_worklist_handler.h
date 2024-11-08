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

#ifndef ECMASCRIPT_MEM_RSET_WORKLIST_HANDLER_H
#define ECMASCRIPT_MEM_RSET_WORKLIST_HANDLER_H

#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
class Heap;
class Region;
class RememberedSet;

class RSetItem {
public:
    explicit RSetItem(Region *region, RememberedSet *rSet) : region_(region), rSet_(rSet) {}
    ~RSetItem() = default;

    template<class Visitor>
    inline void Process(const Visitor &visitor);

    inline void MergeBack();

private:
    Region *region_ {nullptr};
    RememberedSet *rSet_ {nullptr};
};

class RSetWorkListHandler {
public:
    explicit RSetWorkListHandler(Heap *heap, JSThread *thread);
    ~RSetWorkListHandler() = default;

    inline void Initialize();

    template<class Visitor>
    inline void ProcessAll(const Visitor &visitor);

    // Only called from the bound js thread in RUNNING state.
    inline void WaitFinishedThenMergeBack();

    inline bool MergeBack();

    inline Heap *GetHeap();

    inline void EnumerateRegions(const Heap *heap);

    template<class Visitor>
    inline void ProcessAllVisitor(const Visitor &visitor, int done);

    inline void MergeBackForAllItem();

    JSThread *GetOwnerThreadUnsafe() const
    {
        return ownerThread_;
    }

private:
    inline void CollectRSetItemsInHeap(const Heap *heap);

    template<class Visitor>
    inline bool ProcessNext(const Visitor &visitor);

    inline bool TryMergeBack();

    Heap *heap_ {nullptr};
    // The thread is not guaranteed to be alive. The caller must ensure that the thread is alive.
    JSThread *ownerThread_ {nullptr};
    /**
     * This value represent whether there are some items to process, this is set to true in Initialize when collecting
     * the RSet in heap(call from daemon thread in SuspendAll), and use CAS to set to false when try to merge back and
     * clear(call maybe from daemon thread in SuspendAll, or from bound js thread in RUNNING state).
     * Only two ways to modify this value:
     * 1. Initialzie, from daemon thread in SuspendAll, and the modification must be visible to the bound js thread.
     * 2. MergeBackAndReset, maybe from daemon thread when SuspendAll, or from js thread which region
     * belonged to in RUNNING state.
     * So the bound js thread can always see the lastest value without using atomic.
     * And thus WaitFinishedThenMergeBack should ONLY be called from the bound js thread in RUNNING state.
    */
    bool initialized_ {false};
    std::vector<RSetItem> items_;
    std::atomic<int> nextItemIndex_ {-1};
    int remainItems_ {0};
    Mutex mutex_;
    ConditionVariable cv_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_RSET_WORKLIST_HANDLER_H
