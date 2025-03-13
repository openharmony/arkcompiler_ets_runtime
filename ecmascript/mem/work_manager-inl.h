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

#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/tlab_allocator-inl.h"

namespace panda::ecmascript {
void WorkNodeHolder::Setup(Heap *heap, WorkManager *workManager, GlobalWorkStack *workStack)
{
    heap_ = heap;
    workManager_ = workManager;
    workStack_ = workStack;
    continuousQueue_ = new ProcessQueue();
}

void WorkNodeHolder::Destroy()
{
    continuousQueue_->Destroy();
    delete continuousQueue_;
    continuousQueue_ = nullptr;
}

void WorkNodeHolder::Initialize(TriggerGCType gcType, ParallelGCTaskPhase taskPhase)
{
    inNode_ = workManager_->AllocateWorkNode();
    cachedInNode_ = workManager_->AllocateWorkNode();
    outNode_ = workManager_->AllocateWorkNode();
    weakQueue_ = new ProcessQueue();
    weakQueue_->BeginMarking(continuousQueue_);
    aliveSize_ = 0;
    promotedSize_ = 0;
    parallelGCTaskPhase_ = taskPhase;
    if (gcType != TriggerGCType::OLD_GC) {
        allocator_ = new TlabAllocator(heap_);
    }
}

void WorkNodeHolder::Finish()
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
        if (postTask && heap_->IsParallelGCEnabled() && heap_->CheckCanDistributeTask() &&
            !(heap_->IsMarking() && heap_->GetIncrementalMarker()->IsTriggeredIncrementalMark())) {
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
            cachedInNode_->SetNext(outNode_);
            outNode_->SetNext(nullptr);
            if (!PopWorkNodeFromGlobal()) {
                return false;
            }
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
TlabAllocator *WorkNodeHolder::GetTlabAllocator() const
{
    return allocator_;
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

SharedGCWorkManager::SharedGCWorkManager(SharedHeap *heap, uint32_t threadNum)
    : WorkManagerBase(heap->GetNativeAreaAllocator()), sHeap_(heap), threadNum_(threadNum),
      continuousQueue_ { nullptr }, sharedTaskPhase_(SHARED_UNDEFINED_TASK)
{
    for (uint32_t i = 0; i < threadNum_; i++) {
        continuousQueue_.at(i) = new ProcessQueue();
    }
}

SharedGCWorkManager::~SharedGCWorkManager()
{
    Finish();
    for (uint32_t i = 0; i < threadNum_; i++) {
        continuousQueue_.at(i)->Destroy();
        delete continuousQueue_.at(i);
        continuousQueue_.at(i) = nullptr;
    }
}

void SharedGCWorkManager::Initialize(TriggerGCType gcType, SharedParallelMarkPhase taskPhase)
{
    sharedTaskPhase_ = taskPhase;
    InitializeBase();
    for (uint32_t i = 0; i < threadNum_; i++) {
        SharedGCWorkNodeHolder &holder = works_.at(i);
        holder.inNode_ = AllocateWorkNode();
        holder.cachedInNode_ = AllocateWorkNode();
        holder.outNode_ = AllocateWorkNode();
        holder.weakQueue_ = new ProcessQueue();
        holder.weakQueue_->BeginMarking(continuousQueue_.at(i));
        if (gcType == TriggerGCType::SHARED_FULL_GC) {
            holder.allocator_ = new SharedTlabAllocator(sHeap_);
        }
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
        if (holder.weakQueue_ != nullptr) {
            holder.weakQueue_->FinishMarking(continuousQueue_.at(i));
            delete holder.weakQueue_;
            holder.weakQueue_ = nullptr;
        }
        aliveSize += holder.aliveSize_;
        if (holder.allocator_ != nullptr) {
            holder.allocator_->Finalize();
            delete holder.allocator_;
            holder.allocator_ = nullptr;
        }
    }
    workStack_.Clear();
    FinishBase();
    initialized_.store(false, std::memory_order_release);
    return aliveSize;
}

bool SharedGCWorkManager::Push(uint32_t threadId, TaggedObject *object)
{
    WorkNode *&inNode = works_.at(threadId).inNode_;
    if (!inNode->PushObject(ToUintPtr(object))) {
        PushWorkNodeToGlobal(threadId);
        return inNode->PushObject(ToUintPtr(object));
    }
    return true;
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

void SharedGCWorkManager::PushWorkNodeToGlobal(uint32_t threadId, bool postTask)
{
    WorkNode *&inNode = works_.at(threadId).inNode_;
    if (!inNode->IsEmpty()) {
        workStack_.Push(inNode);
        inNode = works_.at(threadId).cachedInNode_;
        ASSERT(inNode != nullptr);
        works_.at(threadId).cachedInNode_ = AllocateWorkNode();
        if (postTask && sHeap_->IsParallelGCEnabled() && sHeap_->CheckCanDistributeTask()) {
            sHeap_->PostGCMarkingTask(sharedTaskPhase_);
        }
    }
}

void SharedGCWorkManager::PushLocalBufferToGlobal(WorkNode *&node, bool postTask)
{
    ASSERT(node != nullptr);
    ASSERT(!node->IsEmpty());
    workStack_.Push(node);
    if (postTask && sHeap_->IsParallelGCEnabled() && sHeap_->CheckCanDistributeTask()) {
        sHeap_->PostGCMarkingTask(sharedTaskPhase_);
    }
    node = nullptr;
}

bool SharedGCWorkManager::Pop(uint32_t threadId, TaggedObject **object)
{
    WorkNode *&outNode = works_.at(threadId).outNode_;
    WorkNode *&inNode = works_.at(threadId).inNode_;
    if (!outNode->PopObject(reinterpret_cast<uintptr_t *>(object))) {
        if (!inNode->IsEmpty()) {
            WorkNode *tmp = outNode;
            outNode = inNode;
            inNode = tmp;
        } else if (!PopWorkNodeFromGlobal(threadId)) {
            return false;
        }
        return outNode->PopObject(reinterpret_cast<uintptr_t *>(object));
    }
    return true;
}

bool SharedGCWorkManager::PopWorkNodeFromGlobal(uint32_t threadId)
{
    return workStack_.Pop(&works_.at(threadId).outNode_);
}
}  // namespace panda::ecmascript
#endif  //  ECMASCRIPT_MEM_WORK_MANAGER_INL_H
