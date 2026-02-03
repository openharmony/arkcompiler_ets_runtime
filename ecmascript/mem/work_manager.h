/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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
#include "ecmascript/mem/cms_mem/slot_gc_allocator.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/work_space_chunk.h"
#include "ecmascript/mem/work_stack-inl.h"

namespace panda::ecmascript {
static constexpr size_t WORK_STACK_MAX_SIZE = 100;

using GlobalMarkStack = StackList<TaggedObject *, WORK_STACK_MAX_SIZE>;
using MarkWorkNode = GlobalMarkStack::InternalStack;

class CompressGCMarker;
class FullGC;
class Heap;
class Region;
class SharedHeap;
class Stack;
class SemiSpaceCollector;
class SharedGCWorkManager;
class SharedTlabAllocator;
class TlabAllocator;
class WorkManager;
class WorkSpaceChunk;

enum ParallelGCTaskPhase {
    HANDLE_GLOBAL_POOL_TASK,
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

struct WeakAggregate {
    ObjectSlot keySlot;
    ObjectSlot valueSlot;
};

using GlobalWeakAggregateStack = StackList<WeakAggregate, WORK_STACK_MAX_SIZE>;
using WeakAggregateWorkNode = GlobalWeakAggregateStack::InternalStack;

#define WEAK_AGGREGATE_LIST(V)                                                          \
    V(WeakAggregate, pendingWeakAggregate, PendingWeakAggregate)                        \
    V(WeakAggregate, freshWeakAggregate, FreshWeakAggregate)

template <typename T>
class WorkNodeWrapper {
public:
    using WorkNode = typename T::InternalStack;
    using ElementType = typename WorkNode::ElementType;

    WorkNodeWrapper() = default;
    ~WorkNodeWrapper() = default;

    NO_COPY_SEMANTIC(WorkNodeWrapper);
    NO_MOVE_SEMANTIC(WorkNodeWrapper);

    void Setup(T *globalStack);
    void Initialize(WorkManager *workManager);

    void Push(ElementType e, WorkManager *workManager);
    void PushWorkNodeToGlobal(WorkManager *workManager);
    bool Pop(ElementType *e);
    bool PopWorkNodeFromGlobal();

    bool IsLocalEmpty() const;
    bool IsGlobalEmpty() const;
    bool IsLocalAndGlobalEmpty() const;
private:
    WorkNode *inNode_ {nullptr};
    WorkNode *outNode_ {nullptr};
    WorkNode *cachedInNode_ {nullptr};
    T *globalStack_ {nullptr};
};

class WorkNodeHolder {
public:
    WorkNodeHolder() = default;
    ~WorkNodeHolder() = default;

    NO_COPY_SEMANTIC(WorkNodeHolder);
    NO_MOVE_SEMANTIC(WorkNodeHolder);

#define PARAM_WEAK_AGGREGATE_GLOBAL_STACK(type, name, _)            Global##type##Stack *name##Stack,
    inline void Setup(Heap *heap, WorkManager *workManager,
                      WEAK_AGGREGATE_LIST(PARAM_WEAK_AGGREGATE_GLOBAL_STACK)
                      GlobalMarkStack *markStack);
#undef PARAM_WEAK_AGGREGATE_GLOBAL_STACK
    inline void Destroy();
    inline void Initialize(TriggerGCType gcType, ParallelGCTaskPhase taskPhase);
    inline void Finish();

    void Push(TaggedObject *object)
    {
        markWorkNodeWrapper_.Push(object, workManager_);
    }

    bool Pop(TaggedObject **object)
    {
        return markWorkNodeWrapper_.Pop(object);
    }

    bool IsAllLocalEmpty() const
    {
        return markWorkNodeWrapper_.IsLocalEmpty()
#define WEAK_AGGREGATE_IS_LOCAL_EMPTY(_, name, __)                          \
            && name##WorkNodeWrapper_.IsLocalEmpty()
        WEAK_AGGREGATE_LIST(WEAK_AGGREGATE_IS_LOCAL_EMPTY);
#undef WEAK_AGGREGATE_IS_LOCAL_EMPTY
    }

    void FlushAll()
    {
        markWorkNodeWrapper_.PushWorkNodeToGlobal(workManager_);
#define FLUSH_WEAK_AGGREGATE(_, name, __)                                   \
            name##WorkNodeWrapper_.PushWorkNodeToGlobal(workManager_);
        WEAK_AGGREGATE_LIST(FLUSH_WEAK_AGGREGATE)
#undef FLUSH_WEAK_AGGREGATE
        ASSERT(IsAllLocalEmpty());
    }

#define DEFINE_WEAK_AGGREGATE_FUNC(type, name, Name)                        \
    void Push##Name(type e)                                                 \
    {                                                                       \
        name##WorkNodeWrapper_.Push(e, workManager_);                       \
    }                                                                       \
    bool Pop##Name(type *e)                                                 \
    {                                                                       \
        return name##WorkNodeWrapper_.Pop(e);                               \
    }
    WEAK_AGGREGATE_LIST(DEFINE_WEAK_AGGREGATE_FUNC)
#undef DEFINE_WEAK_AGGREGATE_FUNC

    inline void PushWeakReference(JSTaggedType *weak);

    inline void PushWeakLinkedHashMap(TaggedObject *weakLinkedHasMap);

    inline void IncreaseAliveSize(size_t size);

    inline void IncreasePromotedSize(size_t size);

    inline ProcessQueue *GetWeakReferenceQueue() const;

    inline WeakLinkedHashMapProcessQueue *GetWeakLinkedHashMapQueue() const;

    inline TlabAllocator *GetTlabAllocator() const;

    inline SlotGCAllocator *GetSlotGCAllocator();
private:
    Heap *heap_ {nullptr};
    WorkManager *workManager_ {nullptr};
    WorkNodeWrapper<GlobalMarkStack> markWorkNodeWrapper_ {};

#define DECLARE_WEAK_AGGREGATE(type, name, _)                \
    WorkNodeWrapper<Global##type##Stack> name##WorkNodeWrapper_ {};
    WEAK_AGGREGATE_LIST(DECLARE_WEAK_AGGREGATE)
#undef DECLARE_WEAK_AGGREGATE

    ProcessQueue *weakQueue_ {nullptr};
    WeakLinkedHashMapProcessQueue *weakLinkedHashMapQueue_ {nullptr};
    ContinuousStack<JSTaggedType> *continuousQueue_ {nullptr};
    ContinuousStack<TaggedObject> *continuousWeakLinkedHashMapQueue_ {nullptr};
    TlabAllocator *allocator_ {nullptr};
    SlotGCAllocator slotGCAllocator_ {};
    size_t aliveSize_ {0};
    size_t promotedSize_ {0};

    friend class CompressGCMarker;
    friend class FullGC;
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
        auto allocatedSpace = GetSpaceChunk()->Allocate(WORKNODE_SPACE_SIZE);
        ASSERT(allocatedSpace != nullptr);
        workSpace_ = ToUintPtr(allocatedSpace);
    }

    template <typename T>
    T *AllocateWorkNode();

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

    inline void CheckAndPostTask();

    WORKMANAGER_PUBLIC_HYBRID_EXTENSION();
private:
    NO_COPY_SEMANTIC(WorkManager);
    NO_MOVE_SEMANTIC(WorkManager);

    Heap *heap_;
    uint32_t threadNum_;
    std::array<WorkNodeHolder, common::MAX_TASKPOOL_THREAD_NUM + 1> works_;
    GlobalMarkStack markStack_ {};
#define DECLARE_WEAK_AGGREGATE_GLOBAL_STACK(type, name, _)          Global##type##Stack name##Stack_ {};
    WEAK_AGGREGATE_LIST(DECLARE_WEAK_AGGREGATE_GLOBAL_STACK)
#undef DECLARE_WEAK_AGGREGATE_GLOBAL_STACK
    ParallelGCTaskPhase parallelGCTaskPhase_ {ParallelGCTaskPhase::UNDEFINED_TASK};
    std::atomic<bool> initialized_ {false};
};

class SharedGCWorkNodeHolder {
public:
    SharedGCWorkNodeHolder() = default;
    ~SharedGCWorkNodeHolder() = default;

    NO_COPY_SEMANTIC(SharedGCWorkNodeHolder);
    NO_MOVE_SEMANTIC(SharedGCWorkNodeHolder);

    inline void Setup(SharedHeap *sHeap, SharedGCWorkManager *sWorkManager, GlobalMarkStack *markStack);
    inline void Destroy();
    inline void Initialize(TriggerGCType gcType, SharedParallelMarkPhase sTaskPhase);
    inline void Finish();
    
    inline bool Push(TaggedObject *object);
    inline bool Pop(TaggedObject **object);
    inline void PushWorkNodeToGlobal(bool postTask = true);
    inline bool PopWorkNodeFromGlobal();

    inline void PushWeakReference(JSTaggedType *weak);

    inline void IncreaseAliveSize(size_t size);

    inline ProcessQueue *GetWeakReferenceQueue() const;

    inline SharedTlabAllocator *GetTlabAllocator() const;

private:
    SharedHeap *sHeap_ {nullptr};
    SharedGCWorkManager *sWorkManager_ {nullptr};
    GlobalMarkStack *markStack_ {nullptr};
    SharedParallelMarkPhase sTaskPhase_ {SharedParallelMarkPhase::SHARED_UNDEFINED_TASK};
    
    MarkWorkNode *inNode_ {nullptr};
    MarkWorkNode *outNode_ {nullptr};
    MarkWorkNode *cachedInNode_ {nullptr};
    ProcessQueue *weakQueue_ {nullptr};
    ContinuousStack<JSTaggedType> *continuousQueue_ {nullptr};
    SharedTlabAllocator *allocator_ {nullptr};
    size_t aliveSize_ = 0;

    friend class SharedGCWorkManager;
};

class SharedGCWorkManager : public WorkManagerBase {
public:
    inline SharedGCWorkManager(SharedHeap *heap, uint32_t threadNum);
    inline ~SharedGCWorkManager() override;

    NO_COPY_SEMANTIC(SharedGCWorkManager);
    NO_MOVE_SEMANTIC(SharedGCWorkManager);

    inline void Initialize(TriggerGCType gcType, SharedParallelMarkPhase sTaskPhase, size_t numExtraTemporaryHolders);
    inline size_t Finish() override;

    inline void PushToLocalMarkingBuffer(MarkWorkNode *&markingBuffer, TaggedObject *object);

    inline void PushLocalBufferToGlobal(MarkWorkNode *&node, bool postTask = true);

    inline uint32_t GetTotalThreadNum()
    {
        return threadNum_;
    }

    inline bool HasInitialized() const
    {
        return initialized_.load(std::memory_order_acquire);
    }

    inline SharedGCWorkNodeHolder *GetSharedGCWorkNodeHolder(uint32_t threadId)
    {
        return &works_.at(threadId);
    }

    // Use for JSThread to process mark task in flip function
    inline SharedGCWorkNodeHolder *GetTemporaryWorkNodeHolder();
    
    // Use for JSThread to process mark task in flip function
    inline void FlushTemporaryWorkNodeHolder(SharedGCWorkNodeHolder *holder);

    template <typename Visitor>
    void ForEachExtraTemporaryWorkNodeHolder(Visitor &&visitor);

private:
    // Use for JSThread to process mark task in flip function
    struct ExtraTemporarySharedGCWorkNodeHolderPack {
        std::vector<SharedGCWorkNodeHolder *> holders_ {};
        std::atomic<size_t> nextUsableHolderIdx_ {0};
    };

    inline void CreateExtraTemporaryHolders(size_t numExtraTemporaryHolders);

    inline void FinishAndDestroyExtraTemporaryHolders();

    SharedHeap *sHeap_;
    uint32_t threadNum_;
    std::array<SharedGCWorkNodeHolder, common::MAX_TASKPOOL_THREAD_NUM + 1> works_;
    GlobalMarkStack markStack_ {};
    SharedParallelMarkPhase sTaskPhase_ {SharedParallelMarkPhase::SHARED_UNDEFINED_TASK};
    std::atomic<bool> initialized_ {false};
    ExtraTemporarySharedGCWorkNodeHolderPack extraTemporaryHolders_ {};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_WORK_MANAGER_H
