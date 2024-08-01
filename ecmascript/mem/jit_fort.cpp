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
#include "ecmascript/jit/jit.h"
#if defined(CODE_SIGN_ENABLE) && !defined(JIT_FORT_DISABLE)
#include <sys/prctl.h>
#endif

namespace panda::ecmascript {

template <>
FreeListAllocator<MemDesc>::FreeListAllocator(BaseHeap *heap, MemDescPool *pool, JitFort *fort)
    : memDescPool_(pool), heap_(heap)
{
    freeList_ = std::make_unique<FreeObjectList<MemDesc>>(fort);
}

template <>
uintptr_t FreeListAllocator<MemDesc>::Allocate(MemDesc *object, size_t size)
{
    uintptr_t begin = object->GetBegin();
    uintptr_t end = object->GetEnd();
    uintptr_t remainSize = end - begin - size;
    ASSERT(remainSize >= 0);
    memDescPool_->ReturnDescToPool(object);

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
    jitFortMem_ = PageMap(JIT_FORT_REG_SPACE_MAX, PageProtectProt(Jit::GetInstance()->IsDisableCodeSign()),
        DEFAULT_REGION_SIZE, nullptr, MAP_JITFORT);
    jitFortBegin_ = reinterpret_cast<uintptr_t>(jitFortMem_.GetMem());
    jitFortSize_ = JIT_FORT_REG_SPACE_MAX;
    memDescPool_ = new MemDescPool(jitFortBegin_, jitFortSize_);
    allocator_ = new FreeListAllocator<MemDesc>(nullptr, memDescPool_, this);
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
        JitFortRegion *region = new JitFortRegion(nullptr, mem, end, RegionSpaceFlag::IN_MACHINE_CODE_SPACE,
            memDescPool_);
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
    LockHolder lock(mutex_);

    auto ret = allocator_->Allocate(size);
    if (ret == ToUintPtr(nullptr)) {
        if (AddRegion()) {
            LOG_JIT(DEBUG) << "JitFort: Allocate - AddRegion";
            ret = allocator_->Allocate(size);
        }
    }
    return ret;
}

void JitFort::RecordLiveJitCode(uintptr_t addr, size_t size)
{
    LockHolder lock(liveJitCodeBlksLock_);
    // check duplicate
    for (size_t i = 0; i < liveJitCodeBlks_.size(); ++i) {
        if (liveJitCodeBlks_[i]->GetBegin() == addr && liveJitCodeBlks_[i]->Available() == size) {
            LOG_JIT(DEBUG) << "RecordLiveJitCode duplicate " << (void *)addr << " size " << size;
            return;
        }
    }
    MemDesc *desc = memDescPool_->GetDescFromPool();
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
    LockHolder lock(mutex_);

    if (!regionList_.GetLength()) {
        return;
    }

    if (!IsMachineCodeGC()) {
        return;
    }
    SetMachineCodeGC(false);

    LOG_JIT(DEBUG) << "UpdateFreeSpace enter: " << "Fort space allocated: "
        << allocator_->GetAllocatedSize()
        << " available: " << allocator_->GetAvailableSize()
        << " liveJitCodeBlks: " << liveJitCodeBlks_.size();
    allocator_->RebuildFreeList();
    SortLiveMemDescList();
    auto region = regionList_.GetFirst();
    while (region) {
        CollectFreeRanges(region);
        region = region->GetNext();
    }
    liveJitCodeBlks_.clear();
    LOG_JIT(DEBUG) << "UpdateFreeSpace exit: allocator_->GetAvailableSize  "
        << allocator_->GetAvailableSize();
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
            memDescPool_->ReturnDescToPool(desc);
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

bool JitFort::InRange(uintptr_t address) const
{
    return address >= jitFortBegin_ && address <= (jitFortBegin_ + jitFortSize_ - 1);
}

// Used by JitFort::UpdateFreeSpace call path to find corresponding
// JitFortRegion for a free block in JitFort space, in order to put the blk into
// the corresponding free set of the JitFortRegion the free block belongs.
JitFortRegion *JitFort::ObjectAddressToRange(uintptr_t objAddress)
{
    JitFortRegion *region = GetRegionList();
    while (region != nullptr) {
        if (objAddress >= region->GetBegin() && objAddress < region->GetEnd()) {
            return region;
        }
        region = region->GetNext();
    }
    return region;
}

void JitFort::InitJitFortResource()
{
#if defined(CODE_SIGN_ENABLE) && !defined(JIT_FORT_DISABLE)
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "JIT::InitJitFortResource");
    constexpr int prSetJitFort = 0x6a6974;
    constexpr int jitFortInit = 5;
    int res = prctl(prSetJitFort, jitFortInit, 0);
    if (res < 0) {
        LOG_JIT(ERROR) << "Failed to init jitfort resource";
    }
#endif
}

MemDescPool::MemDescPool(uintptr_t fortBegin, size_t fortSize)
    : fortBegin_(fortBegin), fortSize_(fortSize)
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
