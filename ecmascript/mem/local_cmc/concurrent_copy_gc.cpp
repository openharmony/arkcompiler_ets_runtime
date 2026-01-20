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

#include "ecmascript/mem/local_cmc/concurrent_copy_gc.h"

#include "common_components/taskpool/taskpool.h"

#include "ecmascript/jit/jit.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/local_cmc/cc_evacuator-inl.h"
#include "ecmascript/mem/local_cmc/cc_gc_visitor-inl.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/mem/work_manager-inl.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
ConcurrentCopyGC::ConcurrentCopyGC(Heap *heap) : heap_(heap), thread_(heap->GetJSThread())
{
    for (uint32_t i = 0; i < common::MAX_TASKPOOL_THREAD_NUM + 1; i++) {
        tlabAllocators_.at(i).Setup(heap_);
    }
    auto mainEvacuator = new CCEvacuator(heap_, GetTlabAllocator(MAIN_THREAD_INDEX));
    thread_->InstallLocalCCEvacuator(mainEvacuator);
}

void ConcurrentCopyGC::RunPhase()
{
    ASSERT("ConcurrentCopyGC should be disabled" && !g_isEnableCMCGC);
    GCStats *gcStats = heap_->GetEcmaVM()->GetEcmaGCStats();
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        ("ConcurrentCopyGC::RunPhase;GCReason " + std::to_string(static_cast<int>(gcStats->GetGCReason()))
        + ";MarkReason" + std::to_string(static_cast<int>(gcStats->GetMarkReason()))
        + ";Sensitive" + std::to_string(static_cast<int>(heap_->GetSensitiveStatus()))
        + ";IsInBackground" + std::to_string(Runtime::GetInstance()->IsInBackground())
        + ";Startup" + std::to_string(static_cast<int>(heap_->GetStartupStatus()))
        + ";ConMark" + std::to_string(static_cast<int>(heap_->GetJSThread()->GetMarkStatus()))
        + ";Young" + std::to_string(heap_->GetNewSpace()->GetCommittedSize())
        + ";Old" + std::to_string(heap_->GetOldSpace()->GetCommittedSize())
        + ";TotalCommit" + std::to_string(heap_->GetCommittedSize())
        + ";NativeBindingSize" + std::to_string(heap_->GetNativeBindingSize())
        + ";NativeLimitSize" + std::to_string(heap_->GetGlobalSpaceNativeLimit())
        + ";ObjSizeBeforeSensitive" + std::to_string(heap_->GetRecordHeapObjectSizeBeforeSensitive())).c_str(), "");
    TRACE_GC(GCStats::Scope::ScopeId::TotalGC, gcStats);
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), PartialGC_RunPhases);
    Jit::JitGCLockHolder lock(thread_);
    PreGC();
    ProcessWeakReference();
    Sweep();
    UpdateRoot();
    ConcurrentCopy();
}

void ConcurrentCopyGC::PreGC()
{
    std::shared_ptr<pgo::PGOProfiler> pgoProfiler =  thread_->GetEcmaVM()->GetPGOProfiler();
    if (pgoProfiler != nullptr) {
        pgoProfiler->SuspendByGC();
    }
    heap_->SetGCState(true);
    {
        TRACE_GC(GCStats::Scope::ScopeId::ProcessSharedGCRSetWorkList, heap_->GetEcmaVM()->GetEcmaGCStats());
        heap_->ProcessSharedGCRSetWorkList();
    }
    if (UNLIKELY(heap_->ShouldVerifyHeap())) {
        Verification::VerifyMark(heap_);
    }
}

void ConcurrentCopyGC::ProcessWeakReference()
{
    TRACE_GC(GCStats::Scope::ScopeId::UpdateWeekRef, heap_->GetEcmaVM()->GetEcmaGCStats());
    ASSERT(heap_->IsParallelGCEnabled());
    uint32_t totalThreadCount =
        totalThreadCount = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1; // 1 : mainthread
    for (uint32_t i = 0; i < totalThreadCount; i++) {
        UpdateRecordWeakReference(i);
    }
    for (uint32_t i = 0; i < totalThreadCount; i++) {
        UpdateRecordJSWeakMap(i);
    }
    if (UNLIKELY(heap_->ShouldVerifyHeap())) {
        Verification(heap_, VerifyKind::VERIFY_WEAK_REF).VerifyAll();
    }
}

void ConcurrentCopyGC::Sweep()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ConcurrentCopyGC::Sweep", "");
    TRACE_GC(GCStats::Scope::ScopeId::Sweep, heap_->GetEcmaVM()->GetEcmaGCStats());
    heap_->GetSweeper()->Sweep(TriggerGCType::LOCAL_CC);
    heap_->PrepareRecordNonmovableRegions();
    heap_->GetConcurrentMarker()->Reset(false);
}

void ConcurrentCopyGC::UpdateRoot()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ConcurrentCopyGC::UpdateRoot", "");
    TRACE_GC(GCStats::Scope::ScopeId::UpdateRoot, heap_->GetEcmaVM()->GetEcmaGCStats());
    InitializeCopyPhase();
    CCEvacuator *mainEvacuator = thread_->GetLocalCCEvacuator();
    WeakRootVisitor weakVisitor = [mainEvacuator](TaggedObject *object) -> TaggedObject* {
        Region *objectRegion = Region::ObjectAddressToRange(object);
        ASSERT(objectRegion != nullptr);
        ASSERT(!objectRegion->IsToRegion());
        if (objectRegion->InSharedHeap()) {
            return object;
        } else if (!objectRegion->Test(object)) {
            return nullptr;
        } else if (objectRegion->IsFromRegion()) {
            MarkWord markWord(object, RELAXED_LOAD);
            if (markWord.IsForwardingAddress()) {
                return markWord.ToForwardingAddress();
            } else {
                TaggedObject *ref = mainEvacuator->Copy(object, markWord);
                return ref;
            }
        } else {
            return object;
        }
    };
    heap_->GetEcmaVM()->ProcessReferences(weakVisitor);
    heap_->GetSweeper()->PostTask(TriggerGCType::LOCAL_CC);
    heap_->GetEcmaVM()->GetJSThread()->IterateWeakEcmaGlobalStorage(weakVisitor);
    heap_->GetEcmaVM()->ProcessSnapShotEnv(weakVisitor);
    heap_->GetEcmaVM()->GetJSThread()->UpdateJitCodeMapReference(weakVisitor);
    heap_->GetEcmaVM()->IterateWeakGlobalEnvList(weakVisitor);
    CCUpdateRootVisitor updateRoot(mainEvacuator);
    ObjectXRay::VisitVMRoots(heap_->GetEcmaVM(), updateRoot);
    heap_->GetEcmaVM()->IterateGlobalEnvField(updateRoot);
}

void ConcurrentCopyGC::ConcurrentCopy()
{
    ASSERT(runningTaskCount_ == 0);
    runningTaskCount_ = CalculateCopyThreadNum();
    uint32_t totalTaskCount = runningTaskCount_;
    for (uint32_t i = 0; i < totalTaskCount; i++) {
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<ConcurrentCopyTask>(tasks_, taskIter_, runningTaskCount_, this));
    }
}

void ConcurrentCopyGC::InitializeCopyPhase()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ConcurrentCopyGC::InitializeCopyPhase", "");
    heap_->EnumerateNonNewSpaceRegions([](Region *current) {
        current->ClearOldToNewRSet();
    });
    thread_->SetReadBarrierState(true);
    thread_->SetCCStatus(CCStatus::COPYING);
    heap_->SelectFromSpace();
    ccUpdateFinished_ = false;
    GetTlabAllocator(MAIN_THREAD_INDEX)->Initialize();
}

bool ConcurrentCopyTask::Run(uint32_t threadIndex)
{
    auto allocator = cc_->GetTlabAllocator(threadIndex);
    allocator->Initialize();
    CCEvacuator evacuator(cc_->GetHeap(), allocator);
    Region *region = GetNextTask();
    while (region) {
        ASSERT(region->IsFromRegion());
        region->IterateAllMarkedBits([&](void *mem) {
            TaggedObject *object = reinterpret_cast<TaggedObject *>(mem);
            MarkWord markWord(object, RELAXED_LOAD);
            if (!markWord.IsForwardingAddress()) {
                TaggedObject *toObject = evacuator.Copy(object, markWord);
                Region::ObjectAddressToRange(toObject)->AtomicMark(toObject);
            } else {
                TaggedObject *toObject = markWord.ToForwardingAddress();
                Region::ObjectAddressToRange(toObject)->AtomicMark(toObject);
            }
        });
        region = GetNextTask();
    }
    allocator->Finalize();
    if (runningTaskCount_.fetch_sub(1, std::memory_order_seq_cst) == 1) {
        cc_->RunUpdatePhase();
    }
    return true;
}

Region* ConcurrentCopyTask::GetNextTask()
{
    uint32_t idx = static_cast<uint32_t>(taskIter_.fetch_add(1U, std::memory_order_relaxed));
    if (idx < totalSize_) {
        return tasks_[idx];
    }
    return nullptr;
}

int ConcurrentCopyGC::CalculateCopyThreadNum()
{
    tasks_.clear();
    taskIter_ = 0;
    auto collectTask = [this](Region *region) {
        tasks_.emplace_back(region);
    };
    heap_->GetFromSpaceDuringEvacuation()->EnumerateRegions(collectTask);
    heap_->GetCompressSpace()->EnumerateRegions(collectTask);
    uint32_t count = tasks_.size();
    constexpr uint32_t regionPerThread = 8;
    uint32_t maxThreadNum = std::min(heap_->GetMaxEvacuateTaskCount(),
        common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum());
    return static_cast<int>(std::min(std::max(1U, count / regionPerThread), maxThreadNum));
}

void ConcurrentCopyGC::UpdateRecordWeakReference(uint32_t threadIndex)
{
    auto workManager = heap_->GetWorkManager();
    ProcessQueue *queue = workManager->GetWorkNodeHolder(threadIndex)->GetWeakReferenceQueue();
    while (true) {
        auto obj = queue->PopBack();
        if (UNLIKELY(obj == nullptr)) {
            break;
        }
        ObjectSlot slot(ToUintPtr(obj));
        JSTaggedValue value(slot.GetTaggedType());
        if (!value.IsWeak()) {
            continue;
        }
        auto header = value.GetTaggedWeakRef();
        Region *objectRegion = Region::ObjectAddressToRange(header);
        if (objectRegion->IsToRegion() || objectRegion->InSharedHeap()) {
            continue;
        } else if (!objectRegion->Test(header)) {
            slot.Clear();
        }
    }
}

void ConcurrentCopyGC::UpdateRecordJSWeakMap(uint32_t threadIndex)
{
    std::function<bool(JSTaggedValue)> visitor = [this](JSTaggedValue key) {
        ASSERT(!key.IsHole());
        return key.IsUndefined();   // Dead key, and set to undefined by GC
    };
    auto workManager = heap_->GetWorkManager();
    JSWeakMapProcessQueue *queue = workManager->GetWorkNodeHolder(threadIndex)->GetJSWeakMapQueue();
    while (true) {
        TaggedObject *obj = queue->PopBack();
        if (UNLIKELY(obj == nullptr)) {
            break;
        }
        JSWeakMap *weakMap = JSWeakMap::Cast(obj);
        JSThread *thread = heap_->GetJSThread();
        JSTaggedValue maybeMap = weakMap->GetWeakLinkedMap(thread);
        if (maybeMap.IsUndefined()) {
            continue;
        }
        WeakLinkedHashMap *map = WeakLinkedHashMap::Cast(maybeMap.GetTaggedObject());
        map->ClearAllDeadEntries(thread, visitor);
    }
}

void ConcurrentCopyGC::RunUpdatePhase()
{
    ASSERT(thread_->NeedReadBarrier());
    CCUpdateVisitor<true> updatorWithBarrier(thread_);
    heap_->GetToSpace()->EnumerateRegions([&](Region *region) {
        region->IterateAllMarkedBits([&](void *mem) {
            TaggedObject *object = reinterpret_cast<TaggedObject *>(mem);
            JSHClass *jsHclass = object->SynchronizedGetClass();
            ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, jsHclass, updatorWithBarrier);
        });
        region->SetSwept();
    });
    CCUpdateVisitor<false> updatorWithoutBarrier(thread_);
    heap_->EnumerateNonmovableRegionsWithRecord([&](Region *region) {
        region->IterateAllMarkedBits([&](void *mem) {
            TaggedObject *object = reinterpret_cast<TaggedObject *>(mem);
            JSHClass *jsHclass = object->SynchronizedGetClass();
            ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, jsHclass, updatorWithoutBarrier);
        });
    });
    std::atomic_thread_fence(std::memory_order_seq_cst);
    LockHolder lock(waitMutex_);
    heap_->SetGCState(false);
    thread_->SetCCStatus(CCStatus::COPY_FINISHED);
    thread_->SetCheckSafePointStatus();
    ccUpdateFinished_ = true;
    waitCV_.SignalAll();
}

void ConcurrentCopyGC::WaitFinished()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ConcurrentCopyGC::WaitFinished", "");
    LockHolder lock(waitMutex_);
    while (!ccUpdateFinished_) {
        waitCV_.Wait(&waitMutex_);
    }
}

void ConcurrentCopyGC::HandleUpdateFinished()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ConcurrentCopyGC::HandleUpdateFinished", "");
    ASSERT(thread_->NeedReadBarrier());
    Finish();
    PostGC();
}

void ConcurrentCopyGC::Finish()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ConcurrentCopyGC::Finish", "");
    GetTlabAllocator(MAIN_THREAD_INDEX)->Finalize();
    heap_->GetOldSpace()->MergeToSpace(heap_->GetToSpace());
    if (UNLIKELY(heap_->ShouldVerifyHeap())) {
        Verification::VerifyCC(heap_);
    }
    thread_->SetReadBarrierState(false);
    thread_->SetCCStatus(CCStatus::READY);
    heap_->ResumeCC();
    heap_->SetFullMarkRequestedState(false);
    if (heap_->IsNearGCInSensitive()) {
        heap_->SetNearGCInSensitive(false);
    }
}

void ConcurrentCopyGC::PostGC()
{
    thread_->SwitchAllStub(true);
    std::shared_ptr<pgo::PGOProfiler> pgoProfiler = thread_->GetEcmaVM()->GetPGOProfiler();
    if (pgoProfiler != nullptr) {
        pgoProfiler->ResumeByGC();
    }
}
}  // namespace panda::ecmascript
