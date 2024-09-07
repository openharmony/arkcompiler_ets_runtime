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

#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/jit_fort.h"
#include "ecmascript/jit/jit.h"
#if defined(JIT_ENABLE_CODE_SIGN) && !defined(JIT_FORT_DISABLE)
#include <sys/ioctl.h>
#include <sys/prctl.h>

#define XPM_JITFORT_ENABLE_OPCODE 3
#define XPM_MAGIC 'x'
#define XPM_SET_JITFORT_ENABLE _IOW(XPM_MAGIC, XPM_JITFORT_ENABLE_OPCODE, unsigned long)
#endif

namespace panda::ecmascript {

template <>
FreeListAllocator<MemDesc>::FreeListAllocator(BaseHeap *heap, MemDescPool *pool, JitFort *fort)
    : memDescPool_(pool), heap_(heap)
{
    freeList_ = std::make_unique<FreeObjectList<MemDesc>>(fort);
}

JitFort::JitFort()
{
    jitFortMem_ = PageMap(JIT_FORT_REG_SPACE_MAX,
                          PageProtectProt(Jit::GetInstance()->IsDisableCodeSign() || !IsResourceAvailable()),
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
    constexpr size_t numRegions = JIT_FORT_REG_SPACE_MAX / DEFAULT_REGION_SIZE;
    for (size_t i = 0; i < numRegions; i++) {
        regions_[i]->DestroyFreeObjectSets();
        delete regions_[i];
    }
    delete allocator_;
    delete memDescPool_;
    PageUnmap(jitFortMem_);
}

void JitFort::InitRegions()
{
    constexpr size_t numRegions = JIT_FORT_REG_SPACE_MAX / DEFAULT_REGION_SIZE;
    for (size_t i = 0; i < numRegions; i++) {
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

uintptr_t JitFort::Allocate(MachineCodeDesc *desc)
{
    LockHolder lock(mutex_);

    size_t size = desc->instructionsSize;
    auto ret = allocator_->Allocate(size);
    if (ret == ToUintPtr(nullptr)) {
        if (AddRegion()) {
            LOG_JIT(DEBUG) << "JitFort: Allocate - AddRegion";
            ret = allocator_->Allocate(size);
        }
    }
    if (ret == ToUintPtr(nullptr)) {
        LOG_JIT(DEBUG) << "JitFort:: Allocate return nullptr for size " << size;
        return ret;
    }
    // Record allocation to keep it from being collected by the next
    // JitFort::UpdateFreeSpace in case corresponding Machine code object is not
    // marked for sweep yet by then.
    desc->memDesc = RecordLiveJitCodeNoLock(ret, size);
    LOG_JIT(DEBUG) << "JitFort:: Allocate " << (void *)ret << " - " << (void *)(ret+size-1) <<
        " size " << size << " MachineCodeGC " << IsMachineCodeGC();
    return ret;
}

MemDesc *JitFort::RecordLiveJitCodeNoLock(uintptr_t addr, size_t size, bool installed)
{
    // check duplicate
    for (size_t i = 0; i < liveJitCodeBlks_.size(); ++i) {
        if (liveJitCodeBlks_[i]->GetBegin() == addr && liveJitCodeBlks_[i]->Available() == size) {
            LOG_JIT(DEBUG) << "RecordLiveJitCode duplicate " << (void *)addr << " size " << size;
            return nullptr;
        }
        if (liveJitCodeBlks_[i]->GetBegin() == addr) { // LOCV_EXCL_BR_LINE
            LOG_JIT(FATAL) << "RecordLiveJitCode duplicate addr " << std::hex << addr << std::dec << " size " <<
                size << " existing entry size " << liveJitCodeBlks_[i]->Available() << std::endl;
            return nullptr;
        }
    }
    MemDesc *desc = memDescPool_->GetDescFromPool();
    ASSERT(desc != NULL);
    desc->SetMem(addr);
    desc->SetSize(size);
    desc->SetInstalled(installed);
    liveJitCodeBlks_.emplace_back(desc);
    return desc;
}

MemDesc *JitFort::RecordLiveJitCode(uintptr_t addr, size_t size, bool installed)
{
    LockHolder lock(mutex_);
    return RecordLiveJitCodeNoLock(addr, size, installed);
}

void JitFort::SortLiveMemDescList()
{
    if (liveJitCodeBlks_.size()) {
        std::sort(liveJitCodeBlks_.begin(), liveJitCodeBlks_.end(), [](MemDesc *first, MemDesc *second) {
            return first->GetBegin() < second->GetBegin();  // ascending order
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
    LOG_JIT(DEBUG) << "UpdateFreeSpace exit: allocator_->GetAvailableSize  "
        << allocator_->GetAvailableSize();

    SetMachineCodeGC(false);
}

void JitFort::CollectFreeRanges(JitFortRegion *region)
{
    LOG_JIT(DEBUG) << "region " << (void*)(region->GetBegin());
    uintptr_t freeStart = region->GetBegin();
    for (auto it = liveJitCodeBlks_.begin(); it !=  liveJitCodeBlks_.end();) {
        MemDesc* desc = *it;
        if (desc->GetBegin() < region->GetBegin()) {
            // Skip entries for fort mem awaiting installation that's already processed
            // by a previous call to CollectFreeRange. Do not use desc->IsInstalled()
            // to check here because an entry's IsInstalled flag can change from false to
            // true (if JSThread installs its corresponding MachineCode object) during
            // the time a GC thread runs UpdateFreeSpace
            it++;
            continue;
        }
        if (desc->GetBegin() >= region->GetBegin() && desc->GetBegin() < region->GetEnd()) {
            uintptr_t freeEnd = desc->GetBegin();
            LOG_JIT(DEBUG) << " freeStart = " << (void *)freeStart
                << " freeEnd = "<< (void*)freeEnd
                << " desc->GetBegin() = " << (void *)(desc->GetBegin())
                << " desc->GetEnd() = " << (void *)(desc->GetEnd());
            if (freeStart != freeEnd && freeEnd <= freeStart) { // LOCV_EXCL_BR_LINE
                    LOG_JIT(FATAL) << "CollectFreeRanges Abort: freeEnd smaller than freeStart";
                    return;
            }
            if (freeStart != freeEnd) {
                allocator_->Free(freeStart, freeEnd - freeStart, true);
            }
            freeStart = freeEnd + desc->Available();
            if (desc->IsInstalled()) {
                it = liveJitCodeBlks_.erase(it);
                memDescPool_->ReturnDescToPool(desc);
            } else {
                // retain liveJitCodeBlks entry for fort mem awaiting installation to keep from being freed
                it++;
            }
        } else {
            break;
        }
    }
    uintptr_t freeEnd = region->GetEnd();
    if (freeStart != freeEnd) {
        allocator_->Free(freeStart, freeEnd - freeStart, true);
        LOG_JIT(DEBUG) << " freeStart = " << (void *)freeStart << " freeEnd = " << (void*)freeEnd;
    }
}

bool JitFort::InRange(uintptr_t address) const
{
    return address >= jitFortBegin_ && (jitFortBegin_ + jitFortSize_) > 1 &&
        address <= (jitFortBegin_ + jitFortSize_ - 1);
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

bool JitFort::isResourceAvailable_ = true;
bool JitFort::IsResourceAvailable()
{
    return isResourceAvailable_;
}
void JitFort::InitJitFortResource()
{
#if defined(JIT_ENABLE_CODE_SIGN) && !defined(JIT_FORT_DISABLE)
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "JIT::InitJitFortResource");
    if (!Jit::GetInstance()->IsAppJit()) {
        int fd = open("/dev/xpm", O_RDWR);
        if (fd < 0) {
            isResourceAvailable_ = false;
            LOG_JIT(ERROR) << "Failed to init jitfort resource, open xpm failed: " << strerror(errno);
            return;
        }
        int rc = ioctl(fd, XPM_SET_JITFORT_ENABLE, 0);
        if (rc < 0) {
            isResourceAvailable_ = false;
            LOG_JIT(ERROR) << "Failed to init jitfort resource, enable xpm failed: " << strerror(errno);
            close(fd);
            return;
        }
        close(fd);
    }
    constexpr int prSetJitFort = 0x6a6974;
    constexpr int jitFortInit = 5;
    int res = prctl(prSetJitFort, jitFortInit, 0);
    if (res < 0) {
        isResourceAvailable_ = false;
        LOG_JIT(ERROR) << "Failed to init jitfort resource: " << strerror(errno);
        return;
    }
    res = prctl(prSetJitFort, jitFortInit, 0);
    if (res >= 0 || errno != EEXIST) {
        isResourceAvailable_ = false;
        LOG_JIT(ERROR) << "jitfort not support";
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
        for (size_t i = 0; i < MEMDESCS_PER_BLOCK; ++i) {
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
