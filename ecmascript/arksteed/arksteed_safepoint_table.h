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

#ifndef ECMASCRIPT_ARKSTEED_SAFEPOINT_TABLE_H
#define ECMASCRIPT_ARKSTEED_SAFEPOINT_TABLE_H

#include <vector>

#include "ecmascript/stackmap/ark_stackmap.h"

namespace panda::ecmascript::arksteed {

enum class ExceptionHandlerKind : uint16_t {
    NONE = 0,
    COMPILED_CATCH = 1,
    LAZY_DEOPT = 2,
};

// ArkSteed-style safepoint table
//
// Binary layout:
//   Header (16 bytes):
//     uint32_t numEntries
//     uint32_t numTaggedSlots      (function-level, same for all safepoints)
//     uint32_t numUntaggedSlots
//     uint32_t reserved
//   Entry[] (16 bytes each, sorted by pcOffset ascending):
//     uint32_t pcOffset            (return address offset from code start)
//     uint16_t numExtraSpillSlots  (extra pushed slots at this safepoint)
//     uint16_t taggedRegisterIndexes (bitmap: which pushed regs are tagged)
//     uint32_t deoptOffset         (relative to the table start; 0 if absent)
//     uint16_t deoptNum            (encoded pairs: <id, value>)
//     uint16_t exceptionHandlerKind
//
// GC scanning:
//   1. All tagged stack slots (FP-relative) are roots at every safepoint
//   2. Per-safepoint: extra pushed registers marked as tagged in bitmap
//   3. Per-safepoint: outgoing stack arguments are roots until the call returns

#pragma pack(1)
struct ArkSteedSafepointHeader {
    uint32_t numEntries;
    uint32_t numTaggedSlots;
    uint32_t numUntaggedSlots;
    uint32_t reserved;
};

struct ArkSteedSafepointEntry {
    uint32_t pcOffset;
    uint16_t numExtraSpillSlots;
    uint16_t taggedRegisterIndexes;
    uint32_t deoptOffset;
    uint16_t deoptNum;
    uint16_t exceptionHandlerKind;
};
#pragma pack()

static_assert(sizeof(ArkSteedSafepointHeader) == 16, "Header must be 16 bytes");  // 16: header size in bytes
static_assert(sizeof(ArkSteedSafepointEntry) == 16, "Entry must be 16 bytes");  // 16: entry size in bytes

// ============================================================================
// Builder — used during compilation to collect safepoint entries
// ============================================================================

class PUBLIC_API ArkSteedSafepointTableBuilder {
public:
    ~ArkSteedSafepointTableBuilder();

    class Safepoint {
    public:
        void DefineTaggedRegister(int pushedRegIndex)
        {
            entry_->taggedRegisterIndexes |= static_cast<uint16_t>(1u << pushedRegIndex);
        }

        void SetNumExtraSpillSlots(int count)
        {
            entry_->numExtraSpillSlots = static_cast<uint16_t>(count);
        }

    private:
        friend class ArkSteedSafepointTableBuilder;
        explicit Safepoint(ArkSteedSafepointEntry *entry) : entry_(entry) {}
        ArkSteedSafepointEntry *entry_;
    };

    Safepoint DefineSafepoint(uint32_t pcOffset);
    void DefineDeoptSafepoint(uint32_t pcOffset, std::vector<kungfu::ARKDeopt> deopts,
                              ExceptionHandlerKind exceptionHandlerKind = ExceptionHandlerKind::NONE);
    void SetFrameSlots(uint32_t tagged, uint32_t untagged);

    size_t GetTableSize() const;
    void Emit(uint8_t *buffer) const;
    uint8_t *EmitToNewBuffer() const;

    uint32_t GetNumEntries() const
    {
        return static_cast<uint32_t>(entries_.size());
    }

private:
    uint32_t numTaggedSlots_ = 0;
    uint32_t numUntaggedSlots_ = 0;
    std::vector<ArkSteedSafepointEntry> entries_;
    // Per-safepoint deopt payloads, parallel to entries_ (entry i -> deopts for
    // safepoint i). Formerly a process-global std::unordered_map keyed by the
    // builder pointer; that shared, long-lived mutable state was the victim of
    // heap corruption during eager-deopt-heavy compiles (e.g. box2d), surfacing
    // as SIGSEGV inside the map's bucket walk. Making it a per-builder member
    // removes the shared heap entirely and ties the side table's lifetime to the
    // builder, so corruption can no longer cross compile boundaries.
    std::vector<std::vector<kungfu::ARKDeopt>> deoptSideTable_;
};

// ============================================================================
// Table Reader — used at runtime (GC) to look up safepoint entries
// ============================================================================

class ArkSteedSafepointTable {
public:
    ArkSteedSafepointTable(const uint8_t *data, size_t size);

    uint32_t GetNumTaggedSlots() const
    {
        return header_->numTaggedSlots;
    }
    uint32_t GetNumUntaggedSlots() const
    {
        return header_->numUntaggedSlots;
    }
    uint32_t GetNumEntries() const
    {
        return header_->numEntries;
    }

    const ArkSteedSafepointEntry *FindEntry(uint32_t pcOffset) const;
    void GetDeoptInfo(uint32_t pcOffset, std::vector<kungfu::ARKDeopt> &deopts) const;
    ExceptionHandlerKind GetExceptionHandlerKind(uint32_t pcOffset) const;

    bool IsValid() const
    {
        return header_ != nullptr;
    }

private:
    const ArkSteedSafepointHeader *header_ = nullptr;
    const ArkSteedSafepointEntry *entries_ = nullptr;
    const uint8_t *data_ = nullptr;
    size_t size_ = 0;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_SAFEPOINT_TABLE_H
