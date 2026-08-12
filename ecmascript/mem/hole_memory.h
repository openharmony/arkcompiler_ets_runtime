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

#ifndef ECMASCRIPT_MEM_HOLE_MEMORY_H
#define ECMASCRIPT_MEM_HOLE_MEMORY_H

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/js_tagged_value_internals.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
class Region;

// Large hole-initialised objects (ConstantPool, TaggedArray capacity, ...) are
// written full of JSTaggedValue::VALUE_HOLE at allocation and then, in many
// cases, never touched again. Every such page is a private dirty page, and
// there are usually a lot of identical ones.
//
// Instead of writing the value, map the pages onto a shared read-only template
// that already contains it (see CreateFillTemplate in platform/map.h). The
// pages cost no physical memory until something actually writes to them, and
// all processes forked from appspawn share the same template pages.
//
// Applicability is limited by mmap granularity: replacing a sub-range of a
// region splits its VMA, so this is only done for objects that own their whole
// region - i.e. huge-space objects. Doing it per object inside a shared region
// would create two extra VMAs per object and hit vm.max_map_count.
class HoleMemory {
public:
    // Smallest object worth mapping. The mmap plus its VMA is only worth it
    // when a useful number of whole pages sits strictly inside the payload.
    // Note this is currently below the huge-object threshold
    // (g_maxRegularHeapObjectSize, DEFAULT_REGION_SIZE * 2 / 3 ~= 170KB), so
    // every object reaching SkipRange already clears it; the gate
    // matters only if that threshold is lowered or this is extended to other
    // spaces.
    static constexpr size_t MIN_MAPPED_OBJECT_SIZE = 32_KB;
    // Size of the shared template. Ranges longer than this are covered by
    // repeating it, at one VMA per chunk, so a bigger template means fewer
    // VMAs for large objects at the cost of a few more shared resident pages.
    // Matching DEFAULT_REGION_SIZE keeps any object below one region down to a
    // single VMA; a 1MB object needs four.
    static constexpr size_t TEMPLATE_SIZE = 512_KB;

    // A template is one tagged slot repeated, so its width has to be the slot
    // width: a template of 8-byte holes read as 4-byte slots yields [0x5, 0x0],
    // and a zeroed slot is IsHeapObject() - a null pointer, not a hole.
    //
    // Two widths exist in one build: ConstantPool cache slots are compressed
    // (4 bytes under USE_COMPRESSED_POINTER), TaggedArray slots stay 8. So we
    // keep one template per width and SkipRange picks the one matching the
    // caller. Outside a compressed build the two are the same and only one
    // template exists.
    using TemplateWord = CompressedJSTaggedType;
    static constexpr size_t NARROW_WORD_SIZE = sizeof(CompressedJSTaggedType);
    static constexpr size_t WIDE_WORD_SIZE = sizeof(JSTaggedType);

    // Called once per process, before appspawn forks, from Runtime init. The
    // template is created unconditionally so that every forked application
    // process inherits the fd and shares its physical pages; whether a given
    // process uses it is decided afterwards by Disable().
    static bool PUBLIC_API Initialize();
    static void PUBLIC_API Destroy();

    // Stop mapping new ranges in this process. Called after fork when the
    // system parameter asks for it, and from Runtime init when the option or
    // the environment override says so.
    //
    // Ranges already mapped are unaffected and are still released normally:
    // release must never depend on this flag, or disabling mid-run would hand
    // template-backed memory back to the mem-map pool.
    static void PUBLIC_API Disable(const char *reason);

    // Whether new ranges may be mapped. Not a precondition for releasing.
    static bool IsEnabled()
    {
        return enabled_.load(std::memory_order_relaxed);
    }

    // The single place that decides whether a region's payload may be backed by
    // the template: it must own its whole region, so only the huge object
    // spaces, local and shared. Machine code regions are excluded because
    // remapping would drop PROT_EXEC.
    //
    // Used to keep the release path off the hot path for every other space;
    // getting it wrong only costs a lookup, since what actually gets released
    // is whatever SkipRange registered for this region.
    static bool PUBLIC_API IsMappableRegion(const Region *region);

    // Undo the mapping before the memory goes back to the mem-map pool, so a
    // recycled region does not carry a file mapping (or hole content) into its
    // next life. Releases exactly the range SkipRange registered for this
    // region, and does nothing at all if there is none.
    //
    // Must be called while the region is still valid (before Invalidate), and
    // regardless of IsEnabled().
    static void PUBLIC_API ReleaseHoleRange(Region *region);

    // The page-aligned interior of [begin, begin + size), i.e. the part that can
    // be backed by the template rather than written. False if there is none.
    static bool MappedInterior(uintptr_t begin, size_t size, uintptr_t &interiorBegin,
                               uintptr_t &interiorEnd);

    // Is [begin, +size) a range this mechanism may back? Region must be one
    // IsMappableRegion() accepts, and the range big enough to be worth an mmap.
    static bool PUBLIC_API IsHoleMapped(uintptr_t begin, size_t size);

    // Call this instead of writing VALUE_HOLE across [dataBegin, +bytes).
    //
    // It backs the page-aligned interior of that range with the hole template
    // for elemSize-wide slots and returns it. The caller writes everything
    // outside that sub-range and nothing inside it: writing inside would
    // copy-on-write the shared pages and throw away the entire saving. False
    // means "write it all yourself" and is always safe.
    //
    // Nothing outside [dataBegin, +bytes) is touched, so the object header, a
    // TaggedArray's extra-length area and a ConstantPool's reserved slots keep
    // reading as the zeroes a fresh region gives them - the template pattern
    // must never end up under a slot nobody initialises.
    //
    // Every place that bulk-initialises an object with Hole has to call this.
    // TaggedArray::InitializeWithSpecialValue is not the only one: ConstantPool
    // has its own, and it is the one that matters most in practice.
    //
    // Two preconditions, both met by initialising in allocation order:
    //  - the object's slots must not have been written yet, since mapping
    //    replaces those pages wholesale;
    //  - the object must own its region, which IsMappableRegion() enforces.
    //
    // The mapping happens here rather than at allocation because only the
    // caller knows how wide its slots are, and because a huge object that never
    // wanted holes (a string, machine code) should not be mapped at all.
    static bool PUBLIC_API SkipRange(uintptr_t dataBegin, size_t bytes, size_t elemSize,
                                     uintptr_t &skipBegin, uintptr_t &skipEnd);

    // Counters, reported by every Prepare/Release log line and summarised by
    // Destroy(). Logged at ERROR level on purpose: this is an experimental
    // feature and INFO is filtered out on device.
    static uint64_t PUBLIC_API MappedBytes()
    {
        return mappedBytes_.load(std::memory_order_relaxed);
    }

    static uint64_t PUBLIC_API ReleasedBytes()
    {
        return releasedBytes_.load(std::memory_order_relaxed);
    }

    // Bytes currently backed by the template, i.e. the memory this mechanism is
    // saving right now. The lifetime totals above say how much work was done,
    // which after a few GC cycles is a much larger and far less useful number.
    //
    // Exact, because a range is counted as released only if it was counted as
    // mapped first; a region freed without ever being mapped adds to neither.
    // The two loads are not atomic together, so a release landing in between
    // can make the difference underflow; saturate rather than report nonsense.
    static uint64_t PUBLIC_API LiveBytes()
    {
        uint64_t mapped = mappedBytes_.load(std::memory_order_relaxed);
        uint64_t released = releasedBytes_.load(std::memory_order_relaxed);
        return mapped > released ? mapped - released : 0;
    }

private:
    // Written after fork while other threads are already running.
    static std::atomic<bool> enabled_;
    static std::atomic<uint64_t> mappedRanges_;    // ranges backed by the template
    static std::atomic<uint64_t> mappedBytes_;     // bytes ever backed by the template
    static std::atomic<uint64_t> fallbackRanges_;  // ranges that had to be filled
    static std::atomic<uint64_t> releasedRanges_;
    static std::atomic<uint64_t> releasedBytes_;   // bytes handed back as anonymous
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_HOLE_MEMORY_H
