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

#include "ecmascript/mem/free_object_set.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/jit_fort.h"
#include <sys/mman.h>

namespace panda::ecmascript {

template <>
uintptr_t FreeListAllocator<MemDesc>::Allocate(MemDesc *object, size_t size)
{
    uintptr_t begin = object->GetBegin();
    uintptr_t end = object->GetEnd();
    uintptr_t remainSize = end - begin - size;
    ASSERT(remainSize >= 0);
    JitFort::GetInstance()->ReturnMemDescToPool(object);

    // Keep a longest freeObject between bump-pointer and free object that just allocated
    allocationSizeAccumulator_ += size;
    if (remainSize <= bpAllocator_.Available()) {
        Free(begin + size, remainSize, true);
        return begin;
    } else {
        FreeBumpPoint();
        bpAllocator_.Reset(begin, end);
        auto ret = bpAllocator_.Allocate(size);
        return ret;
    }
}

template <>
void FreeListAllocator<MemDesc>::FillBumpPointer()
{
    return;
}

template <>
void FreeListAllocator<MemDesc>::Free(uintptr_t begin, size_t size, bool isAdd)
{
    ASSERT(size >= 0);
    if (size != 0) {
        ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(begin), size);
        freeList_->Free(begin, size, isAdd);
        ASAN_POISON_MEMORY_REGION(reinterpret_cast<void *>(begin), size);
    }
}

JitFort::JitFort()
{
    jitFortMem_ = PageMap(JIT_FORT_REG_SPACE_MAX, PAGE_PROT_EXEC_READWRITE, DEFAULT_REGION_SIZE, nullptr, MAP_JITFORT);
    jitFortBegin_ = reinterpret_cast<uintptr_t>(jitFortMem_.GetMem());
    jitFortSize_ = JIT_FORT_REG_SPACE_MAX;
    memDescPool_ = new MemDescPool();
    allocator_ = new FreeListAllocator<MemDesc>(nullptr);
    InitRegions();
    LOG_JIT(DEBUG) << "JitFort Begin " << (void *)JitFortBegin() << " end " << (void *)(JitFortBegin() + JitFortSize());
}

JitFort::~JitFort()
{
    delete allocator_;
    delete memDescPool_;
    PageUnmap(jitFortMem_);
}

void JitFort::InitRegions()
{
    auto numRegions = JIT_FORT_REG_SPACE_MAX/DEFAULT_REGION_SIZE;
    for (auto i = 0; i < numRegions; i++) {
        uintptr_t mem = reinterpret_cast<uintptr_t>(jitFortMem_.GetMem()) + i*DEFAULT_REGION_SIZE;
        uintptr_t end = mem + DEFAULT_REGION_SIZE;
        JitFortRegion *region = new JitFortRegion(nullptr, mem, end, RegionSpaceFlag::IN_MACHINE_CODE_SPACE);
        region->InitializeFreeObjectSets();
        regions_[i] = region;
    }
    AddRegion();
}

bool JitFort::AddRegion()
{
    if (nextFreeRegionIdx_ < MAX_JIT_FORT_REGIONS) {
        allocator_->AddFree(regions_[nextFreeRegionIdx_]);
        regionList_.AddNode(regions_[nextFreeRegionIdx_]);
        nextFreeRegionIdx_++;
        return true;
    }
    return false;
}

uintptr_t JitFort::Allocate(size_t size)
{
    auto ret = allocator_->Allocate(size);
    if (ret == ToUintPtr(nullptr)) {
        if (AddRegion()) {
            LOG_JIT(DEBUG) << "JitFort: Allocate - AddRegion";
            ret = allocator_->Allocate(size);
        }
    }
    LOG_JIT(DEBUG) << "JitFort::Allocate " << size << " bytes "
        << "ret " << (void*)ret << " remainder " << allocator_->GetAvailableSize();
    return ret;
}

void JitFort::RecordLiveJitCode(uintptr_t addr, size_t size)
{
    LockHolder lock(liveJitCodeBlksLock_);
    MemDesc *desc = GetMemDescFromPool();
    desc->SetMem(addr);
    desc->SetSize(size);
    liveJitCodeBlks_.emplace_back(desc);
}

void JitFort::SortLiveMemDescList()
{
    if (liveJitCodeBlks_.size()) {
        std::sort(liveJitCodeBlks_.begin(), liveJitCodeBlks_.end(), [](MemDesc *first, MemDesc *second) {
            return second->GetBegin() < first->GetBegin();  // descending order
        });
    }
}

// Called by concurrent sweep in PrepareSweep() and non-concurent sweep in Sweep()
void JitFort::RebuildFreeList()
{
    // disable Fort collection for now - temporary until collection bug fixed
    return;

    freeListUpdated_ = false;
    allocator_->RebuildFreeList();
}

/*
 * UpdateFreeSpace updates JitFort allocator free object list by go through mem blocks
 * in use (liveJitCodeBlks_) in Jit fort space and putting free space in between into
 * allocator free list .
 *
 * This is to be done once when an old or full GC Sweep finishes, and needs to be mutext
 * protected because if concurrent sweep is enabled, this func may be called simulatneously
 * from a GC worker thread when Old/Full GC Seep finishes (AsyncClearTask), or from main/JS
 * thread AllocateMachineCode if an Old/Full GC is in progress.
 *
 * The following must be done before calling UpdateFreeSpace:
 * - JitFort::RebuildFreeList() called to clear JitFort allocator freeList
 * - MachineCodeSpace::FreeRegion completed (whether sync or aync) on all regions
*/
void JitFort::UpdateFreeSpace()
{
    if (!regionList_.GetLength()) {
        return;
    }

    // disable Fort collection for now - temporary until collection bug fixed
    return;

    LockHolder lock(mutex_);
    if (freeListUpdated_) {
        LOG_JIT(DEBUG) << "FreeFortSpace already done for current GC Sweep";
        return;
    } else {
        freeListUpdated_ = true;
    }
    SortLiveMemDescList();
    LOG_JIT(DEBUG) << "UpdateFreeSpace enter: " << "Fort space allocated: "
        << JitFort::GetInstance()->allocator_->GetAllocatedSize()
        << " available: " << JitFort::GetInstance()->allocator_->GetAvailableSize()
        << " liveJitCodeBlks: " << liveJitCodeBlks_.size();
    auto region = regionList_.GetFirst();
    while (region) {
        CollectFreeRanges(region);
        region = region->GetNext();
    }
    liveJitCodeBlks_.clear();
    LOG_JIT(DEBUG) << "UpdateFreeSpace exit: allocator_->GetAvailableSize  "
        << JitFort::GetInstance()->allocator_->GetAvailableSize();
}

void JitFort::CollectFreeRanges(JitFortRegion *region)
{
    LOG_JIT(DEBUG) << "region " << (void*)(region->GetBegin());
    uintptr_t freeStart = region->GetBegin();
    while (!liveJitCodeBlks_.empty()) {
        MemDesc *desc = liveJitCodeBlks_.back();
        if (desc->GetBegin() >= region->GetBegin() && desc->GetBegin() < region->GetEnd()) {
            uintptr_t freeEnd = desc->GetBegin();
            LOG_JIT(DEBUG) << " freeStart = " << (void *)freeStart
                << " freeEnd = "<< (void*)freeEnd
                << " desc->GetBegin() = " << (void *)(desc->GetBegin())
                << " desc->GetEnd() = " << (void *)(desc->GetEnd());
            if (freeStart != freeEnd) {
                allocator_->Free(freeStart, freeEnd - freeStart, true);
            }
            freeStart = freeEnd + desc->Available();
            ReturnMemDescToPool(desc);
            liveJitCodeBlks_.pop_back();
        } else {
            break;
        }
    }
    uintptr_t freeEnd = region->GetEnd();
    if (freeStart != freeEnd) {
        allocator_->Free(freeStart, freeEnd - freeStart, true);
    }
}

// Used by JitFort::UpdateFreeSpace call path to find corresponding
// JitFortRegion for a free block in JitFort space, in order to put the blk into
// the corresponding free set of the JitFortRegion the free block belongs.
JitFortRegion *JitFortRegion::ObjectAddressToRange(uintptr_t objAddress)
{
    JitFortRegion *region = JitFort::GetInstance()->GetRegionList();
    while (region != nullptr) {
        if (objAddress >= region->GetBegin() && objAddress < region->GetEnd()) {
            return region;
        }
        region = region->GetNext();
    }
    return region;
}

MemDescPool::MemDescPool()
{
    Expand();
}

MemDescPool::~MemDescPool()
{
    for (const auto& block : memDescBlocks_) {
        if (block) {
            free(block);
        }
    }
}

MemDesc *MemDescPool::GetDesc()
{
    if (IsEmpty(freeList_)) {
        Expand();
    }
    if (!IsEmpty(freeList_)) {
        MemDesc *res = freeList_;
        freeList_ = freeList_->GetNext();
        allocated_++;
        if (allocated_-returned_ > highwater_) {
            highwater_ = allocated_ - returned_;
        }
        return res;
    }
    return nullptr;
}

void MemDescPool::Expand()
{
    void *block = malloc(sizeof(MemDesc) * MEMDESCS_PER_BLOCK);
    if (block) {
        memDescBlocks_.push_back(block);
        for (auto i = 0; i < MEMDESCS_PER_BLOCK; ++i) {
            Add(new (ToVoidPtr(reinterpret_cast<uintptr_t>(block) + i*sizeof(MemDesc))) MemDesc());
        }
    }
}

void MemDescPool::Add(MemDesc *desc)
{
    desc->SetNext(freeList_);
    freeList_ = desc;
}

}  // namespace panda::ecmascript
