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

#include "ecmascript/common.h"

namespace panda::ecmascript::arksteed {

// ArkSteed-style safepoint table
//
// Binary layout:
//   Header (16 bytes):
//     uint32_t numEntries
//     uint32_t numTaggedSlots      (function-level, same for all safepoints)
//     uint32_t numUntaggedSlots
//     uint32_t reserved
//   Entry[] (8 bytes each, sorted by pcOffset ascending):
//     uint32_t pcOffset            (return address offset from code start)
//     uint16_t numExtraSpillSlots  (extra pushed slots at this safepoint)
//     uint16_t taggedRegisterIndexes (bitmap: which pushed regs are tagged)
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
};
#pragma pack()

static_assert(sizeof(ArkSteedSafepointHeader) == 16, "Header must be 16 bytes");  // 16: header size in bytes
static_assert(sizeof(ArkSteedSafepointEntry) == 8, "Entry must be 8 bytes");  // 8: entry size in bytes

// ============================================================================
// Builder — used during compilation to collect safepoint entries
// ============================================================================

class PUBLIC_API ArkSteedSafepointTableBuilder {
public:
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

    bool IsValid() const
    {
        return header_ != nullptr;
    }

private:
    const ArkSteedSafepointHeader *header_ = nullptr;
    const ArkSteedSafepointEntry *entries_ = nullptr;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_SAFEPOINT_TABLE_H
