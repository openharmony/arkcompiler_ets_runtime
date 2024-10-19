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

// Fort buf allocation is in multiples of FORT_BUF_ALIGN
size_t JitFort::FortAllocSize(size_t instrSize)
{
    return(AlignUp(instrSize, FORT_BUF_ALIGN));
}

uintptr_t JitFort::Allocate(MachineCodeDesc *desc)
{
    LockHolder lock(mutex_);

    size_t size = FortAllocSize(desc->instructionsSize);
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
    ASSERT((ret & FORT_BUF_ADDR_MASK) == 0);
    MarkJitFortMemAwaitInstall(ret, size);
    LOG_JIT(DEBUG) << "JitFort:: Allocate " << (void *)ret << " - " << (void *)(ret+size-1) <<
        " size " << size << " instructionsSize " << desc->instructionsSize;
    return ret;
}

// Called by GC thread during Mark to mark Fort buf alive.
// Encoding details in GCBitset for marking live Fort buffers:
//   Begin of a live Fort buf:
//     - 10: a live Jit Fort buf that has been installed
//     - 11: a live Jit Fort buf that has not been installed yet
//   End of a live Fort buf:
//     - 01
// This encoding requires 4 GCBitset bits to mark a live JitFort
// buffer, which makes the minimum Fort buffer allocation size 32 bytes.
void JitFort::MarkJitFortMemAlive(MachineCode *obj)
{
    size_t size = FortAllocSize(obj->GetInstructionsSize());
    uintptr_t addr = obj->GetText();
    uintptr_t endAddr = addr + size - 1;
    uint32_t regionIdx = AddrToFortRegionIdx(addr);
    regions_[regionIdx]->AtomicMark(reinterpret_cast<void *>(addr));
    regions_[regionIdx]->AtomicMark(reinterpret_cast<void *>(endAddr));
    LOG_JIT(DEBUG) << "MarkFortMemAlive: addr " << (void *)addr << " size " << size
        << " regionIdx " << regionIdx << " instructionsSize " << obj->GetInstructionsSize();
}

// Called by Jit Compile thread during JitFort Allocate to mark Fort buf
// awaiting install. Need mutex (JitGCLockHolder) for region gcBitset access.
// See JitFort::MarkJitFortMemAlive comments for mark bit encoding in GC bitset.
void JitFort::MarkJitFortMemAwaitInstall(uintptr_t addr, size_t size)
{
    uintptr_t endAddr = addr + size - 1;
    uint32_t regionIdx = AddrToFortRegionIdx(addr);
    regions_[regionIdx]->AtomicMark(reinterpret_cast<void *>(addr));
    regions_[regionIdx]->AtomicMark(reinterpret_cast<void *>(addr + sizeof(uint64_t))); // mark next bit
    regions_[regionIdx]->AtomicMark(reinterpret_cast<void *>(endAddr));
    LOG_JIT(DEBUG) << "MarkFortMemAwaitInstall: addr " << (void *)addr << " size " << size
        << " regionIdx " << regionIdx;
}

// Called by JS/Main thread during SafePoint to clear Fort buf AwaitInstall bit
// See JitFort::MarkJitFortMemAlive comments for mark bit encoding in GC bitset.
void JitFort::MarkJitFortMemInstalled(MachineCode *obj)
{
    size_t size = FortAllocSize(obj->GetInstructionsSize());
    uintptr_t addr = obj->GetText();
    uint32_t regionIdx = AddrToFortRegionIdx(addr);
    regions_[regionIdx]->GetGCBitset()->ClearMark(addr + sizeof(uint64_t)); // clear next bit
    LOG_JIT(DEBUG) << "MarkFortMemInstalled: addr " << (void *)addr << " size " << size
        << " regionIdx " << regionIdx << " instructionsSize " << obj->GetInstructionsSize();
}

uint32_t JitFort::AddrToFortRegionIdx(uint64_t addr)
{
    ASSERT(InRange(addr));
    uint32_t regionIdx = ((addr - JitFortBegin()) & ~(DEFAULT_REGION_MASK)) >> REGION_SIZE_LOG2;
    ASSERT(regionIdx < MAX_JIT_FORT_REGIONS);
    return regionIdx;
}

/*
 * Called from GC worker thread duing Old/Full GC Sweep (AsyncSweep). Mutex is needed
 * to ensure exclusive access to JitFort memory by GC thread when it frees JitFort mem
 * blocks, and by Jit compiled thread when it allocates Fort mem.
 */
void JitFort::UpdateFreeSpace()
{
    if (!Jit::GetInstance()->IsEnableJitFort()) {
        return;
    }

    LockHolder lock(mutex_);

    if (!regionList_.GetLength()) { // LCOV_EXCL_BR_LINE
        return;
    }
    LOG_JIT(DEBUG) << "UpdateFreeSpace enter: " << "Fort space allocated: "
        << allocator_->GetAllocatedSize()
        << " available: " << allocator_->GetAvailableSize();
    allocator_->RebuildFreeList();
    JitFortRegion *region = regionList_.GetFirst();
    while (region) {
        FreeRegion(region);
        region = region->GetNext();
    }
    LOG_JIT(DEBUG) << "UpdateFreeSpace exit: allocator_->GetAvailableSize  "
        << allocator_->GetAvailableSize();
}

void JitFort::FreeRegion(JitFortRegion *region)
{
    LOG_JIT(DEBUG) << "JitFort FreeRegion " << (void*)(region->GetBegin());

    uintptr_t freeStart = region->GetBegin();
    region->GetGCBitset()->IterateMarkedBitsConst(
        region->GetBegin(), region->GetGCBitsetSize(),
        [this, &region, &freeStart](void *mem, size_t size) {
            ASSERT(region->InRange(ToUintPtr(mem)));
            (void) region;
            uintptr_t freeEnd = ToUintPtr(mem);
            if (freeStart != freeEnd) {
                allocator_->Free(freeStart, freeEnd - freeStart, true);
            }
            freeStart = freeEnd + size;
        });
    uintptr_t freeEnd = region->GetEnd();
    if (freeStart != freeEnd) {
        allocator_->Free(freeStart, freeEnd - freeStart, true);
    }
}

bool JitFort::InRange(uintptr_t address) const
{
    return address >= jitFortBegin_ && (jitFortBegin_ + jitFortSize_) > 1 &&
        address <= (jitFortBegin_ + jitFortSize_ - 1);
}

void JitFort::PrepareSweeping()
{
    isSweeping_.store(false, std::memory_order_release);
}

// concurrent sweep - only one of the AsyncSweep task will do JitFort sweep
void JitFort::AsyncSweep()
{
    bool expect = false;
    if (isSweeping_.compare_exchange_strong(expect, true, std::memory_order_seq_cst)) {
        LOG_JIT(DEBUG) << "JitFort::AsyncSweep";
        UpdateFreeSpace();
    }
}

// non-concurrent sweep
void JitFort::Sweep()
{
    LOG_JIT(DEBUG) << "JitFort::Sweep";
    UpdateFreeSpace();
}

// Used by JitFort::UpdateFreeSpace call path to find corresponding
// JitFortRegion for a free block in JitFort space, in order to put the blk into
// the corresponding free set of the JitFortRegion the free block belongs.
JitFortRegion *JitFort::ObjectAddressToRange(uintptr_t addr)
{
    return regions_[AddrToFortRegionIdx(addr)];
}

void JitFortGCBitset::MarkStartAddr(bool awaitInstall, uintptr_t startAddr, uint32_t index, uint32_t &word)
{
    if (!awaitInstall) {
        ClearMark(startAddr);
        word &= ~(1u << index);
    } else {
        word &= ~(1u << index);
        word &= ~(1u << (index+1));
    }
}

void JitFortGCBitset::MarkEndAddr(bool awaitInstall, uintptr_t endAddr, uint32_t index, uint32_t &word)
{
    if (!awaitInstall) {
        ClearMark(endAddr - 1);
    }
    word &= ~(1u << index);
}

// See JitFort::MarkJitFortMemAlive comments for mark bit encoding in JitFort GC bitset.
template <typename Visitor>
void JitFortGCBitset::IterateMarkedBitsConst(uintptr_t regionAddr, size_t bitsetSize, Visitor visitor)
{
    bool awaitInstall = false;
    uintptr_t startAddr = 0;
    uintptr_t endAddr = 0;

    auto words = Words();
    uint32_t wordCount = WordCount(bitsetSize);
    uint32_t index = BIT_PER_WORD;
    for (uint32_t i = 0; i < wordCount; i++) {
        uint32_t word = words[i];
        while (word != 0) {
            index = static_cast<uint32_t>(__builtin_ctz(word));
            ASSERT(index < BIT_PER_WORD);
            if (!startAddr) {
                startAddr = regionAddr + (index << TAGGED_TYPE_SIZE_LOG);
                awaitInstall = Test(regionAddr + ((index+1) << TAGGED_TYPE_SIZE_LOG));
                MarkStartAddr(awaitInstall, startAddr, index, word);
            } else {
                endAddr = regionAddr + ((index+1) << TAGGED_TYPE_SIZE_LOG);
                LOG_JIT(DEBUG) << "Live Jit Mem " << (void *)startAddr << " size " << endAddr-startAddr;
                visitor(reinterpret_cast<void *>(startAddr), endAddr - startAddr);
                MarkEndAddr(awaitInstall, endAddr, index, word);
                awaitInstall = false;
                startAddr = 0;
            }
        }
        regionAddr += TAGGED_TYPE_SIZE * BIT_PER_WORD;
    }
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
    if (!IsEmpty(freeList_)) { // LCOV_EXCL_BR_LINE
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
