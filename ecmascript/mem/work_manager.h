/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_WORK_MANAGER_H
#define ECMASCRIPT_MEM_WORK_MANAGER_H

#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/work_space_chunk.h"
#include "ecmascript/taskpool/taskpool.h"

namespace panda::ecmascript {
using SlotNeedUpdate = std::pair<TaggedObject *, ObjectSlot>;

static constexpr uint32_t MARKSTACK_MAX_SIZE = 100;
static constexpr uint32_t STACK_AREA_SIZE = sizeof(uintptr_t) * MARKSTACK_MAX_SIZE;

class Heap;
class SharedHeap;
class Stack;
class SemiSpaceCollector;
class TlabAllocator;
class SharedTlabAllocator;
class Region;
class WorkSpaceChunk;

enum ParallelGCTaskPhase {
    SEMI_HANDLE_THREAD_ROOTS_TASK,
    SEMI_HANDLE_SNAPSHOT_TASK,
    SEMI_HANDLE_GLOBAL_POOL_TASK,
    OLD_HANDLE_GLOBAL_POOL_TASK,
    COMPRESS_HANDLE_GLOBAL_POOL_TASK,
    CONCURRENT_HANDLE_GLOBAL_POOL_TASK,
    CONCURRENT_HANDLE_OLD_TO_NEW_TASK,
    UNDEFINED_TASK,
    TASK_LAST  // Count of different Task phase
};

enum SharedParallelMarkPhase {
    SHARED_MARK_TASK,
    SHARED_COMPRESS_TASK,
    SHARED_UNDEFINED_TASK,
    SHARED_TASK_LAST  // Count of different Task phase
};

class WorkNode {
public:
    explicit WorkNode(Stack *stack) : next_(nullptr), stack_(stack) {}
    ~WorkNode() = default;

    NO_COPY_SEMANTIC(WorkNode);
    NO_MOVE_SEMANTIC(WorkNode);

    bool PushObject(uintptr_t obj)
    {
        return stack_->PushBackChecked(obj);
    }

    bool PopObject(uintptr_t *obj)
    {
        if (IsEmpty()) {
            return false;
        }
        *obj = stack_->PopBackUnchecked();
        return true;
    }

    bool IsEmpty() const
    {
        return stack_->IsEmpty();
    }

    WorkNode *Next() const
    {
        return next_;
    }

    void SetNext(WorkNode *node)
    {
        next_ = node;
    }

private:
    WorkNode *next_;
    Stack *stack_;
};

class GlobalWorkStack {
public:
    GlobalWorkStack() : top_(nullptr) {}
    ~GlobalWorkStack() = default;

    NO_COPY_SEMANTIC(GlobalWorkStack);
    NO_MOVE_SEMANTIC(GlobalWorkStack);

    void Push(WorkNode *node)
    {
        if (node == nullptr) {
            return;
        }
        LockHolder lock(mtx_);
        node->SetNext(top_);
        top_ = node;
    }

    bool Pop(WorkNode **node)
    {
        LockHolder lock(mtx_);
        if (top_ == nullptr) {
            return false;
        }
        *node = top_;
        top_ = top_->Next();
        return true;
    }
private:
    WorkNode *top_ {nullptr};
    Mutex mtx_;
};

struct WorkNodeHolder {
    WorkNode *inNode_ {nullptr};
    WorkNode *outNode_ {nullptr};
    WorkNode *cachedInNode_ {nullptr};
    ProcessQueue *weakQueue_ {nullptr};
    std::vector<SlotNeedUpdate> pendingUpdateSlots_;
    TlabAllocator *allocator_ {nullptr};
    size_t aliveSize_ = 0;
    size_t promotedSize_ = 0;
};

class WorkManagerBase {
public:
    WorkManagerBase(NativeAreaAllocator *allocator);
    virtual ~WorkManagerBase();

    WorkSpaceChunk *GetSpaceChunk() const
    {
        return const_cast<WorkSpaceChunk *>(&spaceChunk_);
    }

    void InitializeBase()
    {
        spaceStart_ = workSpace_;
        spaceEnd_ = workSpace_ + WORKNODE_SPACE_SIZE;
    }

    void FinishBase()
    {
        while (!agedSpaces_.empty()) {
            GetSpaceChunk()->Free(reinterpret_cast<void *>(agedSpaces_.back()));
            agedSpaces_.pop_back();
        }
    }

    WorkNode *AllocateWorkNode();
    virtual size_t Finish()
    {
        LOG_ECMA(FATAL) << " WorkManagerBase Finish";
        return 0;
    }

    Mutex mtx_;
private:
    NO_COPY_SEMANTIC(WorkManagerBase);
    NO_MOVE_SEMANTIC(WorkManagerBase);
    
    WorkSpaceChunk spaceChunk_;
    uintptr_t workSpace_;
    uintptr_t spaceStart_;
    uintptr_t spaceEnd_;
    std::vector<uintptr_t> agedSpaces_;
};

class WorkManager : public WorkManagerBase {
public:
    WorkManager() = delete;
    WorkManager(Heap *heap, uint32_t threadNum);
    ~WorkManager() override;

    void Initialize(TriggerGCType gcType, ParallelGCTaskPhase taskPhase);
    size_t Finish() override;
    void Finish(size_t &aliveSize, size_t &promotedSize);

    bool Push(uint32_t threadId, TaggedObject *object);
    bool Pop(uint32_t threadId, TaggedObject **object);
    bool PopWorkNodeFromGlobal(uint32_t threadId);
    void PushWorkNodeToGlobal(uint32_t threadId, bool postTask = true);

    inline void PushWeakReference(uint32_t threadId, JSTaggedType *weak)
    {
        works_.at(threadId).weakQueue_->PushBack(weak);
    }

    inline void IncreaseAliveSize(uint32_t threadId, size_t size)
    {
        works_.at(threadId).aliveSize_ += size;
    }

    inline void IncreasePromotedSize(uint32_t threadId, size_t size)
    {
        works_.at(threadId).promotedSize_ += size;
    }

    inline ProcessQueue *GetWeakReferenceQueue(uint32_t threadId) const
    {
        return works_.at(threadId).weakQueue_;
    }

    inline TlabAllocator *GetTlabAllocator(uint32_t threadId) const
    {
        return works_.at(threadId).allocator_;
    }

    inline void PushSlotNeedUpdate(uint32_t threadId, SlotNeedUpdate slot)
    {
        works_.at(threadId).pendingUpdateSlots_.emplace_back(slot);
    }

    inline bool GetSlotNeedUpdate(uint32_t threadId, SlotNeedUpdate *slot)
    {
        std::vector<SlotNeedUpdate> &pendingUpdateSlots = works_.at(threadId).pendingUpdateSlots_;
        if (pendingUpdateSlots.empty()) {
            return false;
        }
        *slot = pendingUpdateSlots.back();
        pendingUpdateSlots.pop_back();
        return true;
    }

    inline uint32_t GetTotalThreadNum()
    {
        return threadNum_;
    }

    inline bool HasInitialized() const
    {
        return initialized_.load(std::memory_order_acquire);
    }

private:
    NO_COPY_SEMANTIC(WorkManager);
    NO_MOVE_SEMANTIC(WorkManager);

    Heap *heap_;
    uint32_t threadNum_;
    std::array<WorkNodeHolder, MAX_TASKPOOL_THREAD_NUM + 1> works_;
    std::array<ContinuousStack<JSTaggedType> *, MAX_TASKPOOL_THREAD_NUM + 1> continuousQueue_;
    GlobalWorkStack workStack_;
    ParallelGCTaskPhase parallelGCTaskPhase_;
    std::atomic<bool> initialized_ {false};
};

struct SharedGCWorkNodeHolder {
    WorkNode *inNode_ {nullptr};
    WorkNode *cachedInNode_ {nullptr};
    WorkNode *outNode_ {nullptr};
    ProcessQueue *weakQueue_ {nullptr};
    SharedTlabAllocator *allocator_ {nullptr};
    size_t aliveSize_ = 0;
};

class SharedGCWorkManager : public WorkManagerBase {
public:
    SharedGCWorkManager(SharedHeap *heap, uint32_t threadNum);
    ~SharedGCWorkManager() override;

    void Initialize(TriggerGCType gcType, SharedParallelMarkPhase taskPhase);
    size_t Finish() override;

    inline SharedTlabAllocator *GetTlabAllocator(uint32_t threadId) const
    {
        return works_.at(threadId).allocator_;
    }

    inline void IncreaseAliveSize(uint32_t threadId, size_t size)
    {
        works_.at(threadId).aliveSize_ += size;
    }

    bool Push(uint32_t threadId, TaggedObject *object);
    bool PushToLocalMarkingBuffer(WorkNode *&markingBuffer, TaggedObject *object);
    bool Pop(uint32_t threadId, TaggedObject **object);

    bool PopWorkNodeFromGlobal(uint32_t threadId);
    void PushWorkNodeToGlobal(uint32_t threadId, bool postTask = true);
    void PushLocalBufferToGlobal(WorkNode *&node, bool postTask = true);

    inline void PushWeakReference(uint32_t threadId, JSTaggedType *weak)
    {
        works_.at(threadId).weakQueue_->PushBack(weak);
    }

    inline ProcessQueue *GetWeakReferenceQueue(uint32_t threadId) const
    {
        return works_.at(threadId).weakQueue_;
    }

    inline uint32_t GetTotalThreadNum()
    {
        return threadNum_;
    }

    inline bool HasInitialized() const
    {
        return initialized_.load(std::memory_order_acquire);
    }

private:
    NO_COPY_SEMANTIC(SharedGCWorkManager);
    NO_MOVE_SEMANTIC(SharedGCWorkManager);

    SharedHeap *sHeap_;
    uint32_t threadNum_;
    std::array<SharedGCWorkNodeHolder, MAX_TASKPOOL_THREAD_NUM + 1> works_;
    std::array<ContinuousStack<JSTaggedType> *, MAX_TASKPOOL_THREAD_NUM + 1> continuousQueue_;
    GlobalWorkStack workStack_;
    std::atomic<bool> initialized_ {false};
    SharedParallelMarkPhase sharedTaskPhase_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_WORK_MANAGER_H
