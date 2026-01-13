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
void WorkNodeHolder::Setup(Heap *heap, WorkManager *workManager, GlobalWorkStack *workStack)
{
    heap_ = heap;
    workManager_ = workManager;
    workStack_ = workStack;
    continuousQueue_ = new ProcessQueue();
    continuousJSWeakMapQueue_ = new JSWeakMapProcessQueue();
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

    continuousJSWeakMapQueue_->Destroy();
    delete continuousJSWeakMapQueue_;
    continuousJSWeakMapQueue_ = nullptr;
}

void WorkNodeHolder::Initialize(TriggerGCType gcType, ParallelGCTaskPhase taskPhase)
{
    inNode_ = workManager_->AllocateWorkNode();
    cachedInNode_ = workManager_->AllocateWorkNode();
    outNode_ = workManager_->AllocateWorkNode();
    weakQueue_ = new ProcessQueue();
    weakQueue_->BeginMarking(continuousQueue_);
    jsWeakMapQueue_ = new JSWeakMapProcessQueue();
    jsWeakMapQueue_->BeginMarking(continuousJSWeakMapQueue_);
    aliveSize_ = 0;
    promotedSize_ = 0;
    parallelGCTaskPhase_ = taskPhase;
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
    if (jsWeakMapQueue_ != nullptr) {
        jsWeakMapQueue_->FinishMarking(continuousJSWeakMapQueue_);
        delete jsWeakMapQueue_;
        jsWeakMapQueue_ = nullptr;
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
    parallelGCTaskPhase_ = ParallelGCTaskPhase::UNDEFINED_TASK;
}

bool WorkNodeHolder::Push(TaggedObject *object)
{
    if (!inNode_->PushObject(ToUintPtr(object))) {
        PushWorkNodeToGlobal();
        return inNode_->PushObject(ToUintPtr(object));
    }
    return true;
}

void WorkNodeHolder::PushWorkNodeToGlobal(bool postTask)
{
    if (!inNode_->IsEmpty()) {
        workStack_->Push(inNode_);
        inNode_ = cachedInNode_;
        ASSERT(inNode_ != nullptr);
        cachedInNode_ = cachedInNode_->Next();
        if (cachedInNode_ == nullptr) {
            cachedInNode_ = workManager_->AllocateWorkNode();
        }
        if (postTask && heap_->IsParallelGCEnabled() && heap_->CheckCanDistributeTask()) {
            heap_->PostParallelGCTask(parallelGCTaskPhase_);
        }
    }
}

bool WorkNodeHolder::Pop(TaggedObject **object)
{
    if (!outNode_->PopObject(reinterpret_cast<uintptr_t *>(object))) {
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
        return outNode_->PopObject(reinterpret_cast<uintptr_t *>(object));
    }
    return true;
}

bool WorkNodeHolder::PopWorkNodeFromGlobal()
{
    return workStack_->Pop(&outNode_);
}

void WorkNodeHolder::PushWeakReference(JSTaggedType *weak)
{
    weakQueue_->PushBack(weak);
}

void WorkNodeHolder::PushJSWeakMap(TaggedObject *jsWeakMap)
{
    jsWeakMapQueue_->PushBack(jsWeakMap);
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

JSWeakMapProcessQueue *WorkNodeHolder::GetJSWeakMapQueue() const
{
    return jsWeakMapQueue_;
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

WorkNode *WorkManagerBase::AllocateWorkNode()
{
    LockHolder lock(mtx_);
    size_t allocatedSize = sizeof(WorkNode) + sizeof(Stack) + STACK_AREA_SIZE;
    ASSERT(allocatedSize < WORKNODE_SPACE_SIZE);

    uintptr_t begin = spaceStart_;
    if (begin + allocatedSize >= spaceEnd_) {
        agedSpaces_.emplace_back(workSpace_);
        workSpace_ = ToUintPtr(GetSpaceChunk()->Allocate(WORKNODE_SPACE_SIZE));
        spaceStart_ = workSpace_;
        spaceEnd_ = workSpace_ + WORKNODE_SPACE_SIZE;
        begin = spaceStart_;
    }
    spaceStart_ = begin + allocatedSize;
    Stack *stack = reinterpret_cast<Stack *>(begin + sizeof(WorkNode));
    stack->ResetBegin(begin + sizeof(WorkNode) + sizeof(Stack), begin + allocatedSize);
    WorkNode *work = reinterpret_cast<WorkNode *>(begin);
    return new (work) WorkNode(stack);
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
        works_.at(i).Setup(heap_, this, &workStack_);
    }
}

WorkManager::~WorkManager()
{
    Finish();
    for (uint32_t i = 0; i < threadNum_; i++) {
        works_.at(i).Destroy();
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
    workStack_.Clear();
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

void SharedGCWorkNodeHolder::Setup(SharedHeap *sHeap, SharedGCWorkManager *sWorkManager, GlobalWorkStack *workStack)
{
    sHeap_ = sHeap;
    sWorkManager_ = sWorkManager;
    workStack_ = workStack;
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
    inNode_ = sWorkManager_->AllocateWorkNode();
    cachedInNode_ = sWorkManager_->AllocateWorkNode();
    outNode_ = sWorkManager_->AllocateWorkNode();
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
    if (!inNode_->PushObject(ToUintPtr(object))) {
        PushWorkNodeToGlobal();
        return inNode_->PushObject(ToUintPtr(object));
    }
    return true;
}

void SharedGCWorkNodeHolder::PushWorkNodeToGlobal(bool postTask)
{
    if (!inNode_->IsEmpty()) {
        workStack_->Push(inNode_);
        inNode_ = cachedInNode_;
        ASSERT(inNode_ != nullptr);
        cachedInNode_ = sWorkManager_->AllocateWorkNode();
        if (postTask && sHeap_->IsParallelGCEnabled() && sHeap_->CheckCanDistributeTask()) {
            sHeap_->PostGCMarkingTask(sTaskPhase_);
        }
    }
}

bool SharedGCWorkNodeHolder::Pop(TaggedObject **object)
{
    if (!outNode_->PopObject(reinterpret_cast<uintptr_t *>(object))) {
        if (!inNode_->IsEmpty()) {
            WorkNode *tmp = outNode_;
            outNode_ = inNode_;
            inNode_ = tmp;
        } else if (!PopWorkNodeFromGlobal()) {
            return false;
        }
        return outNode_->PopObject(reinterpret_cast<uintptr_t *>(object));
    }
    return true;
}

bool SharedGCWorkNodeHolder::PopWorkNodeFromGlobal()
{
    return workStack_->Pop(&outNode_);
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
        works_.at(i).Setup(sHeap_, this, &workStack_);
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
    workStack_.Clear();
    FinishBase();
    initialized_.store(false, std::memory_order_release);
    return aliveSize;
}

bool SharedGCWorkManager::PushToLocalMarkingBuffer(WorkNode *&markingBuffer, TaggedObject *object)
{
    if (UNLIKELY(markingBuffer == nullptr)) {
        markingBuffer = AllocateWorkNode();
    }
    ASSERT(markingBuffer != nullptr);
    if (UNLIKELY(!markingBuffer->PushObject(ToUintPtr(object)))) {
        PushLocalBufferToGlobal(markingBuffer);
        ASSERT(markingBuffer == nullptr);
        markingBuffer = AllocateWorkNode();
        return markingBuffer->PushObject(ToUintPtr(object));
    }
    return true;
}

void SharedGCWorkManager::PushLocalBufferToGlobal(WorkNode *&node, bool postTask)
{
    ASSERT(node != nullptr);
    ASSERT(!node->IsEmpty());
    workStack_.Push(node);
    if (postTask && sHeap_->IsParallelGCEnabled() && sHeap_->CheckCanDistributeTask()) {
        sHeap_->PostGCMarkingTask(sTaskPhase_);
    }
    node = nullptr;
}

void SharedGCWorkManager::CreateExtraTemporaryHolders(size_t numExtraTemporaryHolders)
{
    ASSERT(extraTemporaryHolders_.holders_.empty());
    ASSERT(extraTemporaryHolders_.nextUsableHolderIdx_ == 0);
    for (size_t i = 0; i < numExtraTemporaryHolders; ++i) {
        SharedGCWorkNodeHolder *holder = new SharedGCWorkNodeHolder();
        holder->Setup(sHeap_, this, &workStack_);
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
