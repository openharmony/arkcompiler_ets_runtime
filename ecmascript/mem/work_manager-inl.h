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

#ifndef ECMASCRIPT_MEM_WORK_MANAGER_INL_H
#define ECMASCRIPT_MEM_WORK_MANAGER_INL_H

#include "ecmascript/mem/work_manager.h"

#include "ecmascript/mem/cms_mem/slot_gc_allocator-inl.h"
#include "ecmascript/mem/tlab_allocator-inl.h"

namespace panda::ecmascript {

template <typename T>
void WorkNodeWrapper<T>::Setup(T *globalStack)
{
    ASSERT(globalStack != nullptr);
    globalStack_ = globalStack;
}

template <typename T>
void WorkNodeWrapper<T>::Initialize(WorkManager *workManager)
{
    inNode_ = workManager->AllocateWorkNode<WorkNode>();
    cachedInNode_ = workManager->AllocateWorkNode<WorkNode>();
    outNode_ = workManager->AllocateWorkNode<WorkNode>();
}

template <typename T>
void WorkNodeWrapper<T>::Push(ElementType e, WorkManager *workManager)
{
    if (UNLIKELY(inNode_->IsFull())) {
        PushWorkNodeToGlobal(workManager);
        if constexpr (std::is_same_v<GlobalMarkStack, T>) {
            workManager->CheckAndPostTask();
        }
    }
    inNode_->Push(e);
}

template <typename T>
void WorkNodeWrapper<T>::PushWorkNodeToGlobal(WorkManager *workManager)
{
    if (!inNode_->IsEmpty()) {
        globalStack_->Push(inNode_);
        inNode_ = cachedInNode_;
        ASSERT(inNode_ != nullptr);
        cachedInNode_ = cachedInNode_->GetNext();
        if (cachedInNode_ == nullptr) {
            cachedInNode_ = workManager->AllocateWorkNode<WorkNode>();
        }
    }
}

template <typename T>
bool WorkNodeWrapper<T>::Pop(ElementType *e)
{
    if (UNLIKELY(outNode_->IsEmpty())) {
        if (!inNode_->IsEmpty()) {
            WorkNode *tmp = outNode_;
            outNode_ = inNode_;
            inNode_ = tmp;
        } else {
            WorkNode *tmp = outNode_;
            if (!PopWorkNodeFromGlobal()) {
                return false;
            }
            tmp->SetNext(cachedInNode_);
            cachedInNode_ = tmp;
        }
    }
    outNode_->Pop(e);
    return true;
}

template <typename T>
bool WorkNodeWrapper<T>::PopWorkNodeFromGlobal()
{
    return globalStack_->Pop(&outNode_);
}

template <typename T>
bool WorkNodeWrapper<T>::IsLocalEmpty() const
{
    ASSERT(inNode_ != nullptr);
    ASSERT(outNode_ != nullptr);
    ASSERT(cachedInNode_ != nullptr);
    return inNode_->IsEmpty() && outNode_->IsEmpty() && cachedInNode_->IsEmpty();
}

template <typename T>
bool WorkNodeWrapper<T>::IsGlobalEmpty() const
{
    ASSERT(globalStack_ != nullptr);
    return globalStack_->IsEmpty();
}

template <typename T>
bool WorkNodeWrapper<T>::IsLocalAndGlobalEmpty() const
{
    return IsLocalEmpty() && IsGlobalEmpty();
}

#define PARAM_WEAK_AGGREGATE_GLOBAL_STACK(type, name, _)            Global##type##Stack *name##Stack,
void WorkNodeHolder::Setup(Heap *heap, WorkManager *workManager,
                           WEAK_AGGREGATE_LIST(PARAM_WEAK_AGGREGATE_GLOBAL_STACK)
                           GlobalMarkStack *markStack)
#undef PARAM_WEAK_AGGREGATE_GLOBAL_STACK
{
    heap_ = heap;
    workManager_ = workManager;
    markWorkNodeWrapper_.Setup(markStack);
#define SETUP_WEAK_AGGREGATE(_, name, __)      name##WorkNodeWrapper_.Setup(name##Stack);
    WEAK_AGGREGATE_LIST(SETUP_WEAK_AGGREGATE)
#undef SETUP_WEAK_AGGREGATE
    continuousQueue_ = new ProcessQueue();
    continuousWeakLinkedHashMapQueue_ = new WeakLinkedHashMapProcessQueue();
    // fixme: refactor?
    if constexpr (G_USE_CMS_GC) {
        slotGCAllocator_.Setup(heap->GetSlotSpace());
    }
}

void WorkNodeHolder::Destroy()
{
    continuousQueue_->Destroy();
    delete continuousQueue_;
    continuousQueue_ = nullptr;

    continuousWeakLinkedHashMapQueue_->Destroy();
    delete continuousWeakLinkedHashMapQueue_;
    continuousWeakLinkedHashMapQueue_ = nullptr;
}

void WorkNodeHolder::Initialize(TriggerGCType gcType, ParallelGCTaskPhase taskPhase)
{
    markWorkNodeWrapper_.Initialize(workManager_);
#define INITIALIZE_WEAK_AGGREGATE(_, name, __)          name##WorkNodeWrapper_.Initialize(workManager_);
    WEAK_AGGREGATE_LIST(INITIALIZE_WEAK_AGGREGATE)
#undef INITIALIZE_WEAK_AGGREGATE
    weakQueue_ = new ProcessQueue();
    weakQueue_->BeginMarking(continuousQueue_);
    weakLinkedHashMapQueue_ = new WeakLinkedHashMapProcessQueue();
    weakLinkedHashMapQueue_->BeginMarking(continuousWeakLinkedHashMapQueue_);
    aliveSize_ = 0;
    promotedSize_ = 0;
    if (gcType == TriggerGCType::FULL_GC) {
        // fixme: refactor?
        if constexpr (G_USE_CMS_GC) {
            slotGCAllocator_.Initialize();
        } else {
            allocator_ = new TlabAllocator(heap_);
        }
    }
}

void WorkNodeHolder::Finish()
{
    if (weakQueue_ != nullptr) {
        weakQueue_->FinishMarking(continuousQueue_);
        delete weakQueue_;
        weakQueue_ = nullptr;
    }
    if (weakLinkedHashMapQueue_ != nullptr) {
        weakLinkedHashMapQueue_->FinishMarking(continuousWeakLinkedHashMapQueue_);
        delete weakLinkedHashMapQueue_;
        weakLinkedHashMapQueue_ = nullptr;
    }
    if (allocator_ != nullptr) {
        allocator_->Finalize();
        delete allocator_;
        allocator_ = nullptr;
    }
    // fixme: refactor?
    if constexpr (G_USE_CMS_GC) {
        slotGCAllocator_.Finalize(heap_);
    }
}

void WorkNodeHolder::PushWeakReference(JSTaggedType *weak)
{
    weakQueue_->PushBack(weak);
}

void WorkNodeHolder::PushWeakLinkedHashMap(TaggedObject *weakLinkedHashMap)
{
    weakLinkedHashMapQueue_->PushBack(weakLinkedHashMap);
}

void WorkNodeHolder::IncreaseAliveSize(size_t size)
{
    aliveSize_ += size;
}

void WorkNodeHolder::IncreasePromotedSize(size_t size)
{
    promotedSize_ += size;
}

ProcessQueue *WorkNodeHolder::GetWeakReferenceQueue() const
{
    return weakQueue_;
}

WeakLinkedHashMapProcessQueue *WorkNodeHolder::GetWeakLinkedHashMapQueue() const
{
    return weakLinkedHashMapQueue_;
}

TlabAllocator *WorkNodeHolder::GetTlabAllocator() const
{
    return allocator_;
}

SlotGCAllocator *WorkNodeHolder::GetSlotGCAllocator()
{
    return &slotGCAllocator_;
}

WorkManagerBase::WorkManagerBase(NativeAreaAllocator *allocator)
    : spaceChunk_(allocator), workSpace_(0), spaceStart_(0), spaceEnd_(0)
{
    auto allocatedSpace = GetSpaceChunk()->Allocate(WORKNODE_SPACE_SIZE);
    ASSERT(allocatedSpace != nullptr);
    workSpace_ = ToUintPtr(allocatedSpace);
}

template <typename T>
T *WorkManagerBase::AllocateWorkNode()
{
    LockHolder lock(mtx_);
    constexpr size_t ALLOC_SIZE = T::GetAllocateSize();
    ASSERT(ALLOC_SIZE < WORKNODE_SPACE_SIZE);

    uintptr_t begin = spaceStart_;
    if (begin + ALLOC_SIZE >= spaceEnd_) {
        agedSpaces_.emplace_back(workSpace_);
        workSpace_ = ToUintPtr(GetSpaceChunk()->Allocate(WORKNODE_SPACE_SIZE));
        spaceStart_ = workSpace_;
        spaceEnd_ = workSpace_ + WORKNODE_SPACE_SIZE;
        begin = spaceStart_;
    }
    spaceStart_ = begin + ALLOC_SIZE;
    return new (ToVoidPtr(begin)) T();
}

WorkManagerBase::~WorkManagerBase()
{
    if (workSpace_ != 0) {
        GetSpaceChunk()->Free(reinterpret_cast<void *>(workSpace_));
    }
}

WorkManager::WorkManager(Heap *heap, uint32_t threadNum)
    : WorkManagerBase(heap->GetNativeAreaAllocator()), heap_(heap), threadNum_(threadNum),
      parallelGCTaskPhase_(UNDEFINED_TASK)
{
    for (uint32_t i = 0; i < threadNum_; i++) {
#define PARAM_WEAK_AGGREGATE_GLOBAL_STACK(type, name, _)            &name##Stack_,
        works_.at(i).Setup(heap_, this,
                           WEAK_AGGREGATE_LIST(PARAM_WEAK_AGGREGATE_GLOBAL_STACK)
                           &markStack_);
#undef PARAM_WEAK_AGGREGATE_GLOBAL_STACK
    }
}

WorkManager::~WorkManager()
{
    Finish();
    for (uint32_t i = 0; i < threadNum_; i++) {
        works_.at(i).Destroy();
    }
}

void WorkManager::CheckAndPostTask()
{
    if (heap_->IsParallelGCEnabled() && heap_->CheckCanDistributeTask()) {
        heap_->TryPostParallelGCTask(parallelGCTaskPhase_);
    }
}

size_t WorkManager::Finish()
{
    size_t aliveSize = 0;
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = works_.at(i);
        holder.Finish();
        aliveSize += holder.aliveSize_;
    }
    markStack_.Clear();
#define CLEAR_WEAK_AGGREGATE_GLOBAL_STACK(type, name, _)          name##Stack_.Clear();
    WEAK_AGGREGATE_LIST(CLEAR_WEAK_AGGREGATE_GLOBAL_STACK)
#undef CLEAR_WEAK_AGGREGATE_GLOBAL_STACK
    FinishBase();
    initialized_.store(false, std::memory_order_release);
    return aliveSize;
}

void WorkManager::Finish(size_t &aliveSize, size_t &promotedSize)
{
    aliveSize = Finish();
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = works_.at(i);
        promotedSize += holder.promotedSize_;
        if (holder.allocator_ != nullptr) {
            holder.allocator_->Finalize();
            delete holder.allocator_;
            holder.allocator_ = nullptr;
        }
    }
    initialized_.store(false, std::memory_order_release);
}

void WorkManager::Initialize(TriggerGCType gcType, ParallelGCTaskPhase taskPhase)
{
    parallelGCTaskPhase_ = taskPhase;
    InitializeBase();
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = works_.at(i);
        holder.Initialize(gcType, taskPhase);
    }
    if (initialized_.load(std::memory_order_acquire)) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    initialized_.store(true, std::memory_order_release);
}

void SharedGCWorkNodeHolder::Setup(SharedHeap *sHeap, SharedGCWorkManager *sWorkManager, GlobalMarkStack *markStack)
{
    sHeap_ = sHeap;
    sWorkManager_ = sWorkManager;
    markStack_ = markStack;
    continuousQueue_ = new ProcessQueue();
}

void SharedGCWorkNodeHolder::Destroy()
{
    continuousQueue_->Destroy();
    delete continuousQueue_;
    continuousQueue_ = nullptr;
}

void SharedGCWorkNodeHolder::Initialize(TriggerGCType gcType, SharedParallelMarkPhase sTaskPhase)
{
    inNode_ = sWorkManager_->AllocateWorkNode<MarkWorkNode>();
    cachedInNode_ = sWorkManager_->AllocateWorkNode<MarkWorkNode>();
    outNode_ = sWorkManager_->AllocateWorkNode<MarkWorkNode>();
    weakQueue_ = new ProcessQueue();
    weakQueue_->BeginMarking(continuousQueue_);
    aliveSize_ = 0;
    sTaskPhase_ = sTaskPhase;
    if (gcType == TriggerGCType::SHARED_FULL_GC) {
        allocator_ = new SharedTlabAllocator(sHeap_);
    }
}

void SharedGCWorkNodeHolder::Finish()
{
    if (weakQueue_ != nullptr) {
        weakQueue_->FinishMarking(continuousQueue_);
        delete weakQueue_;
        weakQueue_ = nullptr;
    }
    if (allocator_ != nullptr) {
        allocator_->Finalize();
        delete allocator_;
        allocator_ = nullptr;
    }
    sTaskPhase_ = SharedParallelMarkPhase::SHARED_UNDEFINED_TASK;
}

bool SharedGCWorkNodeHolder::Push(TaggedObject *object)
{
    if (UNLIKELY(inNode_->IsFull())) {
        PushWorkNodeToGlobal();
    }
    inNode_->Push(object);
    return true;
}

void SharedGCWorkNodeHolder::PushWorkNodeToGlobal(bool postTask)
{
    if (!inNode_->IsEmpty()) {
        markStack_->Push(inNode_);
        inNode_ = cachedInNode_;
        ASSERT(inNode_ != nullptr);
        cachedInNode_ = sWorkManager_->AllocateWorkNode<MarkWorkNode>();
        if (postTask && sHeap_->IsParallelGCEnabled() && sHeap_->CheckCanDistributeTask()) {
            sHeap_->TryPostGCMarkingTask(sTaskPhase_);
        }
    }
}

bool SharedGCWorkNodeHolder::Pop(TaggedObject **object)
{
    if (UNLIKELY(outNode_->IsEmpty())) {
        if (!inNode_->IsEmpty()) {
            MarkWorkNode *tmp = outNode_;
            outNode_ = inNode_;
            inNode_ = tmp;
        } else if (!PopWorkNodeFromGlobal()) {
            return false;
        }
    }
    outNode_->Pop(object);
    return true;
}

bool SharedGCWorkNodeHolder::PopWorkNodeFromGlobal()
{
    return markStack_->Pop(&outNode_);
}

void SharedGCWorkNodeHolder::PushWeakReference(JSTaggedType *weak)
{
    weakQueue_->PushBack(weak);
}

void SharedGCWorkNodeHolder::IncreaseAliveSize(size_t size)
{
    aliveSize_ += size;
}

ProcessQueue *SharedGCWorkNodeHolder::GetWeakReferenceQueue() const
{
    return weakQueue_;
}

SharedTlabAllocator *SharedGCWorkNodeHolder::GetTlabAllocator() const
{
    return allocator_;
}

SharedGCWorkManager::SharedGCWorkManager(SharedHeap *heap, uint32_t threadNum)
    : WorkManagerBase(heap->GetNativeAreaAllocator()), sHeap_(heap), threadNum_(threadNum)
{
    for (uint32_t i = 0; i < threadNum_; i++) {
        works_.at(i).Setup(sHeap_, this, &markStack_);
    }
}

SharedGCWorkManager::~SharedGCWorkManager()
{
    Finish();
    for (uint32_t i = 0; i < threadNum_; i++) {
        works_.at(i).Destroy();
    }
}

void SharedGCWorkManager::Initialize(TriggerGCType gcType, SharedParallelMarkPhase sTaskPhase,
                                     size_t numExtraTemporaryHolders)
{
    sTaskPhase_ = sTaskPhase;
    InitializeBase();
    for (uint32_t i = 0; i < threadNum_; i++) {
        SharedGCWorkNodeHolder &holder = works_.at(i);
        holder.Initialize(gcType, sTaskPhase);
    }
    CreateExtraTemporaryHolders(numExtraTemporaryHolders);
    for (size_t i = 0; i < numExtraTemporaryHolders; ++i) {
        extraTemporaryHolders_.holders_[i]->Initialize(gcType, sTaskPhase);
    }
    if (initialized_.load(std::memory_order_acquire)) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    initialized_.store(true, std::memory_order_release);
}

size_t SharedGCWorkManager::Finish()
{
    size_t aliveSize = 0;
    for (uint32_t i = 0; i < threadNum_; i++) {
        SharedGCWorkNodeHolder &holder = works_.at(i);
        holder.Finish();
        aliveSize += holder.aliveSize_;
    }
    FinishAndDestroyExtraTemporaryHolders();
    markStack_.Clear();
    FinishBase();
    initialized_.store(false, std::memory_order_release);
    return aliveSize;
}

void SharedGCWorkManager::PushToLocalMarkingBuffer(MarkWorkNode *&markingBuffer, TaggedObject *object)
{
    if (UNLIKELY(markingBuffer == nullptr)) {
        markingBuffer = AllocateWorkNode<MarkWorkNode>();
    }
    ASSERT(markingBuffer != nullptr);
    if (UNLIKELY(markingBuffer->IsFull())) {
        PushLocalBufferToGlobal(markingBuffer);
        ASSERT(markingBuffer == nullptr);
        markingBuffer = AllocateWorkNode<MarkWorkNode>();
    }
    markingBuffer->Push(object);
}

void SharedGCWorkManager::PushLocalBufferToGlobal(MarkWorkNode *&node, bool postTask)
{
    ASSERT(node != nullptr);
    ASSERT(!node->IsEmpty());
    markStack_.Push(node);
    if (postTask && sHeap_->IsParallelGCEnabled() && sHeap_->CheckCanDistributeTask()) {
        sHeap_->TryPostGCMarkingTask(sTaskPhase_);
    }
    node = nullptr;
}

void SharedGCWorkManager::CreateExtraTemporaryHolders(size_t numExtraTemporaryHolders)
{
    ASSERT(extraTemporaryHolders_.holders_.empty());
    ASSERT(extraTemporaryHolders_.nextUsableHolderIdx_ == 0);
    for (size_t i = 0; i < numExtraTemporaryHolders; ++i) {
        SharedGCWorkNodeHolder *holder = new SharedGCWorkNodeHolder();
        holder->Setup(sHeap_, this, &markStack_);
        extraTemporaryHolders_.holders_.emplace_back(holder);
    }
}

void SharedGCWorkManager::FinishAndDestroyExtraTemporaryHolders()
{
    for (SharedGCWorkNodeHolder *holder : extraTemporaryHolders_.holders_) {
        holder->Finish();
        holder->Destroy();
        // Shared full gc do not support flip
        ASSERT(holder->aliveSize_ == 0);
        delete holder;
    }
    extraTemporaryHolders_.holders_.clear();
    extraTemporaryHolders_.nextUsableHolderIdx_ = 0;
}

SharedGCWorkNodeHolder *SharedGCWorkManager::GetTemporaryWorkNodeHolder()
{
    size_t idx = extraTemporaryHolders_.nextUsableHolderIdx_.fetch_add(1, std::memory_order_relaxed);
    ASSERT(idx < extraTemporaryHolders_.holders_.size());
    return extraTemporaryHolders_.holders_[idx];
}

void SharedGCWorkManager::FlushTemporaryWorkNodeHolder(SharedGCWorkNodeHolder *holder)
{
    ASSERT(std::find(extraTemporaryHolders_.holders_.begin(), extraTemporaryHolders_.holders_.end(), holder) !=
        extraTemporaryHolders_.holders_.end());
    holder->PushWorkNodeToGlobal(true);
}

template <typename Visitor>
void SharedGCWorkManager::ForEachExtraTemporaryWorkNodeHolder(Visitor &&visitor)
{
    for (SharedGCWorkNodeHolder *holder : extraTemporaryHolders_.holders_) {
        visitor(holder);
    }
}
}  // namespace panda::ecmascript
#endif  //  ECMASCRIPT_MEM_WORK_MANAGER_INL_H
