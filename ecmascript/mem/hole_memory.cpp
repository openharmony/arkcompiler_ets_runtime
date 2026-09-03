/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/hole_memory.h"

#include <cerrno>
#include <cstdlib>
#include <unordered_map>

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/platform/map.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
namespace {
// Every range handed to MapFillTemplate, keyed by the begin of the region that
// owns it. Release looks the range up instead of recomputing it, which is what
// lets the two sides disagree safely: a region that was never mapped costs
// nothing to free, and a range that was mapped is always released as exactly
// the same bytes even if the object's own view of its payload has changed.
struct MappedRange {
    uintptr_t begin {0};
    size_t size {0};
};
Mutex g_mappedRangesLock;
std::unordered_map<uintptr_t, MappedRange> g_mappedRanges;
}  // namespace

std::atomic<bool> HoleMemory::enabled_ {false};
std::atomic<uint64_t> HoleMemory::mappedRanges_ {0};
std::atomic<uint64_t> HoleMemory::mappedBytes_ {0};
std::atomic<uint64_t> HoleMemory::fallbackRanges_ {0};
std::atomic<uint64_t> HoleMemory::releasedRanges_ {0};
std::atomic<uint64_t> HoleMemory::releasedBytes_ {0};

bool HoleMemory::Initialize()
{
    // Always created, and always before appspawn forks: the fd is inherited by
    // every application process, and they all share its physical pages. A
    // process that ends up not wanting the mechanism calls Disable() instead,
    // which costs it nothing but the template's own pages.
    // VALUE_HOLE and COMPRESSED_VALUE_HOLE are both 0x5; only the slot width
    // differs. One template per width - outside a compressed build the two
    // calls are the same width and the second is a no-op.
    bool ready = CreateFillTemplate(TEMPLATE_SIZE, JSTaggedValue::VALUE_HOLE, NARROW_WORD_SIZE);
    if (ready && WIDE_WORD_SIZE != NARROW_WORD_SIZE &&
        !CreateFillTemplate(TEMPLATE_SIZE, JSTaggedValue::VALUE_HOLE, WIDE_WORD_SIZE)) {
        // Not fatal: pools still benefit, wide-slot arrays just fill as before.
        LOG_ECMA(DEBUG) << "HoleMemory: no " << WIDE_WORD_SIZE
                        << "B template, wide-slot arrays will not be shared (errno " << errno << ")";
    }
    enabled_.store(ready, std::memory_order_relaxed);
    if (!ready) {
        LOG_ECMA(DEBUG) << "HoleMemory: unavailable, could not create the fill template (errno "
                        << errno << ")";
        return false;
    }
    LOG_ECMA(DEBUG) << "HoleMemory: template ready, " << TEMPLATE_SIZE << "B filled with 0x"
                    << std::hex << JSTaggedValue::VALUE_HOLE << std::dec << " every "
                    << NARROW_WORD_SIZE << "B, min object size " << MIN_MAPPED_OBJECT_SIZE
                    << "B, page " << PageSize() << "B";
    return true;
}

void HoleMemory::Disable(const char *reason)
{
    if (!enabled_.exchange(false, std::memory_order_relaxed)) {
        return;
    }
    // Only stops new mappings. Ranges mapped before this point stay registered
    // and are still released by ReleaseHoleRange when their region is freed -
    // gating the release on enabled_ would hand template-backed memory back to
    // the mem-map pool, and its next user would read holes instead of zeroes.
    // The template fd stays open; this process just stops using it.
    LOG_ECMA(DEBUG) << "HoleMemory: disabled (" << reason << "), " << (LiveBytes() / 1_KB)
                    << "KB mapped from before, still tracked for release";
}

void HoleMemory::Destroy()
{
    size_t stillMapped = 0;
    {
        LockHolder lock(g_mappedRangesLock);
        stillMapped = g_mappedRanges.size();
        g_mappedRanges.clear();
    }
    // A non-zero live figure here means ranges went back to the mem-map pool
    // still backed by the template, which is a leak worth investigating. The
    // heap is already gone at this point, so there is nothing left to unmap
    // safely - report it and let the counters carry the evidence.
    LOG_ECMA(DEBUG) << "HoleMemory: shutting down, mapped " << mappedRanges_.load() << " ranges / "
                    << (mappedBytes_.load() / 1_KB) << "KB, released " << releasedRanges_.load()
                    << " ranges / " << (releasedBytes_.load() / 1_KB) << "KB, filled "
                    << fallbackRanges_.load() << " ranges, still live " << stillMapped
                    << " ranges / " << (LiveBytes() / 1_KB) << "KB";
    enabled_.store(false, std::memory_order_relaxed);
    DestroyFillTemplates();
}

bool HoleMemory::MappedInterior(uintptr_t begin, size_t size, uintptr_t &interiorBegin,
                                uintptr_t &interiorEnd)
{
    size_t pageSize = PageSize();
    interiorBegin = AlignUp(begin, pageSize);
    interiorEnd = AlignDown(begin + size, pageSize);
    return interiorEnd > interiorBegin;
}

bool HoleMemory::IsMappableRegion(const Region *region)
{
    return region != nullptr && (region->InHugeObjectSpace() || region->InSharedHugeObjectSpace());
}

bool HoleMemory::IsHoleMapped(uintptr_t begin, size_t size)
{
    if (!enabled_.load(std::memory_order_relaxed) || size < MIN_MAPPED_OBJECT_SIZE) {
        return false;
    }
    Region *region = Region::ObjectAddressToRange(begin);
    if (!IsMappableRegion(region)) {
        return false;
    }
    // The prepared range starts at the object, which is the region's begin.
    return begin >= region->GetBegin() && begin + size <= region->GetEnd();
}

bool HoleMemory::SkipRange(uintptr_t dataBegin, size_t bytes, size_t elemSize, uintptr_t &skipBegin,
                           uintptr_t &skipEnd)
{
    if ((elemSize != NARROW_WORD_SIZE && elemSize != WIDE_WORD_SIZE) ||
        !IsHoleMapped(dataBegin, bytes) ||
        !MappedInterior(dataBegin, bytes, skipBegin, skipEnd)) {
        return false;
    }
    Region *region = Region::ObjectAddressToRange(dataBegin);
    // Exactly the range the caller is told to skip, and nothing else. Mapping
    // the whole region payload instead would put the template under bytes no
    // caller asked about - the object header below dataBegin, a TaggedArray's
    // extra-length area, a ConstantPool's reserved slots - and those read as
    // 0x5 repeated rather than as the zeroes a fresh region used to give.
    size_t mappedSize = skipEnd - skipBegin;
#if defined(ENABLE_HUGE_PAGE_MEM) && ENABLE_HUGE_PAGE_MEM
    // Never map over a slot of a 2MB huge-page chunk: a file mapping covering
    // part of the chunk demotes its PMD, and the pool's madvise() cannot put
    // the huge page back. Huge-object regions do not come from that pool (they
    // allocate with regular=false), so this only catches a future change in
    // how those regions are routed.
    ASSERT(!MemMapAllocator::GetInstance()->OwnsHugePageChunk(skipBegin));
#endif
#ifndef NDEBUG
    {
        LockHolder lock(g_mappedRangesLock);
        // A huge object owns its whole region and is hole-initialised once, so
        // there is at most one range per region. The registry holds one entry per
        // region, so a second call could only ever leave one of the two mappings
        // releasable - which is why this is worth checking rather than assuming.
        ASSERT(g_mappedRanges.find(region->GetBegin()) == g_mappedRanges.end());
    }
#endif
    if (!MapFillTemplate(ToVoidPtr(skipBegin), mappedSize, elemSize)) {
        uint64_t ranges = fallbackRanges_.fetch_add(1, std::memory_order_relaxed) + 1;
        LOG_ECMA(DEBUG) << "HoleMemory: NOT mapped, caller will fill [0x" << std::hex << dataBegin
                        << ", +" << bytes << ") " << std::dec << "slot " << elemSize << "B, errno "
                        << errno << "; total fallbacks " << ranges;
        return false;
    }
    // No PageTag here on purpose: PR_SET_VMA_ANON_NAME only applies to
    // anonymous VMAs and returns EBADF on a file-backed one. While the range is
    // mapped it is attributed to the template's memfd name instead, which is
    // why that fd is called "ArkTS Hole". ReleaseHoleRange puts the heap tag
    // back once the range is anonymous again.
    // insert_or_assign, not emplace: emplace keeps the existing entry and
    // reports failure, so if the assert above ever does fire in a release
    // build the registry would track a stale range and release the wrong
    // bytes. Tracking what is mapped now is the recoverable outcome.
    {
        LockHolder lock(g_mappedRangesLock);
        g_mappedRanges.insert_or_assign(region->GetBegin(), MappedRange {skipBegin, mappedSize});
    }
    uint64_t ranges = mappedRanges_.fetch_add(1, std::memory_order_relaxed) + 1;
    mappedBytes_.fetch_add(mappedSize, std::memory_order_relaxed);
    LOG_ECMA(DEBUG) << "HoleMemory: mapped [0x" << std::hex << skipBegin << ", 0x" << skipEnd
                    << ") " << std::dec << (mappedSize / 1_KB) << "KB for a " << (bytes / 1_KB)
                    << "KB object, slot " << elemSize << "B; " << ranges << " ranges mapped, live "
                    << (LiveBytes() / 1_KB) << "KB";
    return true;
}

void HoleMemory::ReleaseHoleRange(Region *region)
{
    if (region == nullptr) {
        return;
    }
    // Deliberately not gated on enabled_: a range mapped before Disable() still
    // has to be handed back as anonymous memory.
    uintptr_t key = region->GetBegin();
    MappedRange range;
    {
        LockHolder lock(g_mappedRangesLock);
        auto it = g_mappedRanges.find(key);
        if (it == g_mappedRanges.end()) {
            // Never mapped. Nothing to undo, and no syscall to pay for - most
            // huge objects (strings, byte arrays) take this path.
            return;
        }
        range = it->second;
        g_mappedRanges.erase(it);
    }
    // Restores plain anonymous zeroed memory, so the range can be recycled by
    // the mem-map pool without carrying the file mapping with it.
    if (!UnmapFillTemplate(ToVoidPtr(range.begin), range.size)) {
        // The range is about to go back to the mem-map pool still backed by the
        // template; its next user would read holes instead of zeroes. Keep it
        // registered: it stays counted as live, it blocks a new region at this
        // address from mapping on top of it, and the next free of that address
        // retries the unmap.
        LockHolder lock(g_mappedRangesLock);
        g_mappedRanges.emplace(key, range);
        LOG_ECMA(DEBUG) << "HoleMemory: FAILED to release [0x" << std::hex << range.begin
                        << ", +" << range.size << ") " << std::dec << "errno " << errno;
        return;
    }
    // The replacement VMA is anonymous and unnamed, so it would not merge with
    // the named heap VMAs around it. Put the region's tag back - this one does
    // take effect, unlike a tag on the file-backed mapping.
    PageTag(ToVoidPtr(range.begin), range.size, PageTagType::HEAP, region->GetSpaceTypeName());
    uint64_t ranges = releasedRanges_.fetch_add(1, std::memory_order_relaxed) + 1;
    releasedBytes_.fetch_add(range.size, std::memory_order_relaxed);
    LOG_ECMA(DEBUG) << "HoleMemory: released [0x" << std::hex << range.begin << ", +"
                    << range.size << ") " << std::dec << "; " << ranges
                    << " ranges released, live " << (LiveBytes() / 1_KB) << "KB";
}
}  // namespace panda::ecmascript
