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

#include "common_components/taskpool/runner.h"
#include "ecmascript/cross_vm/work_manager_hybrid.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/work_space_chunk.h"

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
class WorkManager;

enum ParallelGCTaskPhase {
    OLD_HANDLE_GLOBAL_POOL_TASK,
    COMPRESS_HANDLE_GLOBAL_POOL_TASK,
    CONCURRENT_HANDLE_GLOBAL_POOL_TASK,
    UNIFIED_HANDLE_GLOBAL_POOL_TASK,
    UNDEFINED_TASK,
    TASK_LAST  // Count of different common::Task phase
};

enum SharedParallelMarkPhase {
    SHARED_MARK_TASK,
    SHARED_COMPRESS_TASK,
    SHARED_UNDEFINED_TASK,
    SHARED_TASK_LAST  // Count of different common::Task phase
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

    void Clear()
    {
        if (top_ != nullptr) {
            LOG_ECMA(ERROR) << "GlobalWorkStack is not nullptr in WorkManager::Finish.";
        }
        top_ = nullptr;
    }

private:
    WorkNode *top_ {nullptr};
    Mutex mtx_;
};

class WorkNodeHolder {
public:
    WorkNodeHolder() = default;
    ~WorkNodeHolder() = default;

    NO_COPY_SEMANTIC(WorkNodeHolder);
    NO_MOVE_SEMANTIC(WorkNodeHolder);

    inline void Setup(Heap *heap, WorkManager *workManager, GlobalWorkStack *workStack);
    inline void Destroy();
    inline void Initialize(TriggerGCType gcType, ParallelGCTaskPhase taskPhase);
    inline void Finish();

    inline bool Push(TaggedObject *object);
    inline bool Pop(TaggedObject **object);
    inline bool PopWorkNodeFromGlobal();
    inline void PushWorkNodeToGlobal(bool postTask = true);

    inline void PushWeakReference(JSTaggedType *weak);

    inline void PushJSWeakMap(TaggedObject *jsWeakMap);

    inline void IncreaseAliveSize(size_t size);

    inline void IncreasePromotedSize(size_t size);

    inline ProcessQueue *GetWeakReferenceQueue() const;

    inline JSWeakMapProcessQueue *GetJSWeakMapQueue() const;

    inline TlabAllocator *GetTlabAllocator() const;

    inline JSThread *GetJSThread() const;
private:
    Heap *heap_ {nullptr};
    WorkManager *workManager_ {nullptr};
    GlobalWorkStack *workStack_ {nullptr};
    ParallelGCTaskPhase parallelGCTaskPhase_ {ParallelGCTaskPhase::UNDEFINED_TASK};

    WorkNode *inNode_ {nullptr};
    WorkNode *outNode_ {nullptr};
    WorkNode *cachedInNode_ {nullptr};
    ProcessQueue *weakQueue_ {nullptr};
    JSWeakMapProcessQueue *jsWeakMapQueue_ {nullptr};
    ContinuousStack<JSTaggedType> *continuousQueue_ {nullptr};
    ContinuousStack<TaggedObject> *continuousJSWeakMapQueue_ {nullptr};
    TlabAllocator *allocator_ {nullptr};
    size_t aliveSize_ {0};
    size_t promotedSize_ {0};

    friend class WorkManager;
};

class WorkManagerBase {
public:
    inline WorkManagerBase(NativeAreaAllocator *allocator);
    inline virtual ~WorkManagerBase();

    WorkSpaceChunk *GetSpaceChunk() const
    {
        return const_cast<WorkSpaceChunk *>(&spaceChunk_);
    }

    void InitializeBase()
    {
        if (UNLIKELY(workSpace_ == 0)) {
            InitializeInPostFork();
        }
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

    void FinishInPreFork()
    {
        ASSERT(workSpace_ != 0);
        GetSpaceChunk()->Free(reinterpret_cast<void *>(workSpace_));
        workSpace_ = 0;
        spaceStart_ = 0;
        spaceEnd_ = 0;
    }

    void InitializeInPostFork()
    {
        ASSERT(workSpace_ == 0);
        auto allocatedSpace = GetSpaceChunk()->Allocate(WORKNODE_SPACE_SIZE);
        ASSERT(allocatedSpace != nullptr);
        workSpace_ = ToUintPtr(allocatedSpace);
    }

    inline WorkNode *AllocateWorkNode();
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
    inline WorkManager(Heap *heap, uint32_t threadNum);
    inline ~WorkManager() override;

    inline void Initialize(TriggerGCType gcType, ParallelGCTaskPhase taskPhase);
    inline size_t Finish() override;
    inline void Finish(size_t &aliveSize, size_t &promotedSize);

    inline uint32_t GetTotalThreadNum()
    {
        return threadNum_;
    }

    inline bool HasInitialized() const
    {
        return initialized_.load(std::memory_order_acquire);
    }

    inline WorkNodeHolder *GetWorkNodeHolder(uint32_t threadId)
    {
        return &works_.at(threadId);
    }
    WORKMANAGER_PUBLIC_HYBRID_EXTENSION();
private:
    NO_COPY_SEMANTIC(WorkManager);
    NO_MOVE_SEMANTIC(WorkManager);

    Heap *heap_;
    uint32_t threadNum_;
    std::array<WorkNodeHolder, common::MAX_TASKPOOL_THREAD_NUM + 1> works_;
    GlobalWorkStack workStack_ {};
    ParallelGCTaskPhase parallelGCTaskPhase_ {ParallelGCTaskPhase::UNDEFINED_TASK};
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
    inline SharedGCWorkManager(SharedHeap *heap, uint32_t threadNum);
    inline ~SharedGCWorkManager() override;

    inline void Initialize(TriggerGCType gcType, SharedParallelMarkPhase taskPhase);
    inline size_t Finish() override;

    inline SharedTlabAllocator *GetTlabAllocator(uint32_t threadId) const
    {
        return works_.at(threadId).allocator_;
    }

    inline void IncreaseAliveSize(uint32_t threadId, size_t size)
    {
        works_.at(threadId).aliveSize_ += size;
    }

    inline bool Push(uint32_t threadId, TaggedObject *object);
    inline bool PushToLocalMarkingBuffer(WorkNode *&markingBuffer, TaggedObject *object);
    inline bool Pop(uint32_t threadId, TaggedObject **object);

    inline bool PopWorkNodeFromGlobal(uint32_t threadId);
    inline void PushWorkNodeToGlobal(uint32_t threadId, bool postTask = true);
    inline void PushLocalBufferToGlobal(WorkNode *&node, bool postTask = true);

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
    std::array<SharedGCWorkNodeHolder, common::MAX_TASKPOOL_THREAD_NUM + 1> works_;
    std::array<ContinuousStack<JSTaggedType> *, common::MAX_TASKPOOL_THREAD_NUM + 1> continuousQueue_;
    GlobalWorkStack workStack_;
    std::atomic<bool> initialized_ {false};
    SharedParallelMarkPhase sharedTaskPhase_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_WORK_MANAGER_H
