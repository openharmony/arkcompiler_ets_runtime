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

#include "ecmascript/mem/shared_heap/shared_gc_evacuator.h"

#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/tlab_allocator-inl.h"

namespace panda::ecmascript {
void SharedGCEvacuator::Evacuate()
{
    EvacuateRegions();
    UpdateReference();
}

void SharedGCEvacuator::EvacuateRegions()
{
    TRACE_GC(GCStats::Scope::ScopeId::Evacuate,  sHeap_->GetEcmaGCStats());
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCEvacuator::EvacuateRegions;cset count: "
        + std::to_string(sHeap_->GetOldSpace()->GetCollectSetRegionCount()));
    auto sTlabAllocator = new SharedTlabAllocator(sHeap_);
    auto inHeapProfiler = sHeap_->InHeapProfiler();
    sHeap_->GetOldSpace()->EnumerateCollectRegionSet([this, inHeapProfiler, sTlabAllocator](Region *region) {
        region->IterateAllMarkedBits([this, inHeapProfiler, sTlabAllocator](void *mem) {
            auto header = reinterpret_cast<TaggedObject *>(mem);
            JSHClass *klass = header->GetClass();
            size_t size = klass->SizeFromJSHClass(header);
            uintptr_t address = sTlabAllocator->Allocate(size, SHARED_COMPRESS_SPACE);
            ASSERT(address != 0);
            if (memcpy_s(ToVoidPtr(address), size, ToVoidPtr(ToUintPtr(mem)), size) != EOK) { // LOCV_EXCL_BR_LINE
                LOG_ECMA_MEM(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            if (UNLIKELY(inHeapProfiler)) {
                sHeap_->OnMoveEvent(reinterpret_cast<intptr_t>(mem), reinterpret_cast<TaggedObject *>(address), size);
            }
            Barriers::SetPrimitive(header, 0, MarkWord::FromForwardingAddress(address));
            ProcessObjectField(reinterpret_cast<TaggedObject *>(address), klass);
        });
        sHeap_->GetOldSpace()->DecreaseCommitted(region->GetCapacity());
        sHeap_->GetOldSpace()->DecreaseObjectSize(region->GetSize());
    });
    sTlabAllocator->Finalize();
    delete sTlabAllocator;
}

void SharedGCEvacuator::UpdateReference()
{
    TRACE_GC(GCStats::Scope::ScopeId::UpdateReference,  sHeap_->GetEcmaGCStats());
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCEvacuator::UpdateReference");
    Runtime *runtime = Runtime::GetInstance();
    runtime->GCIterateThreadList([this](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        auto heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
        heap->GetSweeper()->EnsureAllTaskFinished();
        heap->EnumerateRegions([this](Region *region) {
            AddWorkload(std::make_unique<UpdateLocalReferenceWorkload>(this, region));
        });
    });
    sHeap_->EnumerateOldSpaceRegions([this](Region *region) {
        AddWorkload(std::make_unique<UpdateSharedReferenceWorkload>(this, region));
    });
    if (sHeap_->IsParallelGCEnabled()) {
        LockHolder holder(lock_);
        parallel_ = CalculateUpdateThreadNum();
        int32_t dTid = DaemonThread::GetInstance()->GetThreadId();
        for (int i = 0; i < parallel_; i++) {
            Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<UpdateReferenceTask>(dTid, this));
        }
    }
    runtime->IterateSharedRoot(rootVisitor_);
    runtime->GCIterateThreadList([this](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        auto vm = thread->GetEcmaVM();
        ObjectXRay::VisitVMRoots(vm, rootVisitor_, VMRootVisitType::UPDATE_ROOT);
    });
    ProcessWorkloads(true);
    WaitFinished();
}

void SharedGCEvacuator::UpdateLocalReferenceWorkload::Process([[maybe_unused]]bool isMain)
{
    region_->IterateAllLocalToShareBits([this](void *mem) {
        ObjectSlot slot(ToUintPtr(mem));
        return evacuator_->UpdateObjectSlot(slot);
    });
}

void SharedGCEvacuator::UpdateSharedReferenceWorkload::Process([[maybe_unused]]bool isMain)
{
    region_->IterateAllCrossRegionBits([this](void *mem) {
        ObjectSlot slot(ToUintPtr(mem));
        evacuator_->UpdateObjectSlot(slot);
    });
}

bool SharedGCEvacuator::UpdateReferenceTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCEvacuator::UpdateReferenceTask");
    evacuator_->ProcessWorkloads(false);
    return true;
}

void SharedGCEvacuator::WaitFinished()
{
    if (parallel_ > 0) {
        LockHolder holder(lock_);
        while (parallel_ > 0) {
            condition_.Wait(&lock_);
        }
    }
}

int SharedGCEvacuator::CalculateUpdateThreadNum()
{
    uint32_t count = workloads_.size();
    constexpr double RATIO = 1.0 / 4;
    count = std::pow(count, RATIO);
    return static_cast<int>(std::min(std::max(1U, count), Taskpool::GetCurrentTaskpool()->GetTotalThreadNum()));
}

bool SharedGCEvacuator::UpdateObjectSlot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return false;
    }
    Region *region = Region::ObjectAddressToRange(slot.GetTaggedType());
    if (!region->InSCollectSet()) {
        return true;
    }
    ASSERT(region->InSharedHeap());
    bool isWeak = value.IsWeakForHeapObject();
    TaggedObject *object = value.GetHeapObject();
    MarkWord markWord(object);
    if (markWord.IsForwardingAddress()) {
        TaggedObject *dst = markWord.ToForwardingAddress();
        if (isWeak) {
            dst = JSTaggedValue(dst).CreateAndGetWeakRef().GetRawTaggedObject();
        }
        slot.Update(dst);
        return true;
    } else {
        slot.Clear();
        return false;
    }
}

void SharedGCEvacuator::ObjectFieldCSetVisitor::VisitObjectRangeImpl(TaggedObject *root, ObjectSlot start,
    ObjectSlot end, VisitObjectArea area)
{
    Region *rootRegion = Region::ObjectAddressToRange(root);
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        JSHClass *hclass = root->GetClass();
        ASSERT(!hclass->IsAllTaggedProp());
        int index = 0;
        TaggedObject *dst = hclass->GetLayout().GetTaggedObject();
        LayoutInfo *layout = LayoutInfo::UncheckCast(dst);
        ObjectSlot realEnd = start;
        realEnd += layout->GetPropertiesCapacity();
        end = end > realEnd ? realEnd : end;
        for (ObjectSlot slot = start; slot < end; slot++) {
            auto attr = layout->GetAttr(index++);
            if (attr.IsTaggedRep()) {
                evacuator_->UpdateCrossRegionRSet(slot, rootRegion);
            }
        }
        return;
    }
    for (ObjectSlot slot = start; slot < end; slot++) {
        evacuator_->UpdateCrossRegionRSet(slot, rootRegion);
    }
}

void SharedGCEvacuator::ProcessObjectField(TaggedObject *object, JSHClass *hclass)
{
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, hclass, objectFieldCSetVisitor_);
}

void SharedGCEvacuator::UpdateCrossRegionRSet(ObjectSlot slot, Region *objectRegion)
{
    JSTaggedType value = slot.GetTaggedType();
    if (!JSTaggedValue(value).IsHeapObject()) {
        return;
    }
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (valueRegion->InSCollectSet()) {
        objectRegion->InsertCrossRegionRSet(slot.SlotAddress());
    }
}

void SharedGCEvacuator::UpdateRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    UpdateObjectSlotRoot(slot);
}

void SharedGCEvacuator::UpdateRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        UpdateObjectSlotRoot(slot);
    }
}

void SharedGCEvacuator::UpdateRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base,
                                                                   ObjectSlot derived, uintptr_t baseOldObject)
{
    if (JSTaggedValue(base.GetTaggedType()).IsHeapObject()) {
        derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
    }
}

void SharedGCEvacuator::UpdateRootVisitor::UpdateObjectSlotRoot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        Region *region = Region::ObjectAddressToRange(slot.GetTaggedType());
        if (region->InSCollectSet()) {
            ASSERT(region->InSharedHeap());
            ASSERT(!value.IsWeakForHeapObject());
            TaggedObject *object = value.GetHeapObject();
            MarkWord markWord(object);
            if (markWord.IsForwardingAddress()) {
                TaggedObject *dst = markWord.ToForwardingAddress();
                slot.Update(dst);
            } else {
                slot.Clear();
            }
        }
    }
}
}  // namespace panda::ecmascript