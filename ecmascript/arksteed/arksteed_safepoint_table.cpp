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

#include "ecmascript/arksteed/arksteed_safepoint_table.h"

#include "libpandabase/macros.h"

#include <algorithm>
#include <cstring>

namespace panda::ecmascript::arksteed {

// ============================================================================
// Builder
// ============================================================================

ArkSteedSafepointTableBuilder::Safepoint ArkSteedSafepointTableBuilder::DefineSafepoint(uint32_t pcOffset)
{
    ArkSteedSafepointEntry entry;
    entry.pcOffset = pcOffset;
    entry.numExtraSpillSlots = 0;
    entry.taggedRegisterIndexes = 0;
    entries_.push_back(entry);
    return Safepoint(&entries_.back());
}

void ArkSteedSafepointTableBuilder::SetFrameSlots(uint32_t tagged, uint32_t untagged)
{
    numTaggedSlots_ = tagged;
    numUntaggedSlots_ = untagged;
}

size_t ArkSteedSafepointTableBuilder::GetTableSize() const
{
    return sizeof(ArkSteedSafepointHeader) + entries_.size() * sizeof(ArkSteedSafepointEntry);
}

void ArkSteedSafepointTableBuilder::Emit(uint8_t *buffer) const
{
    auto *header = reinterpret_cast<ArkSteedSafepointHeader *>(buffer);
    header->numEntries = static_cast<uint32_t>(entries_.size());
    header->numTaggedSlots = numTaggedSlots_;
    header->numUntaggedSlots = numUntaggedSlots_;
    header->reserved = 0;

    auto *entryBuffer = reinterpret_cast<ArkSteedSafepointEntry *>(buffer + sizeof(ArkSteedSafepointHeader));

    // Copy entries sorted by pcOffset
    std::vector<ArkSteedSafepointEntry> sorted = entries_;
    std::sort(sorted.begin(), sorted.end(), [](const ArkSteedSafepointEntry &a, const ArkSteedSafepointEntry &b) {
        return a.pcOffset < b.pcOffset;
    });

    for (size_t i = 0; i < sorted.size(); i++) {
        entryBuffer[i] = sorted[i];
    }
}

uint8_t *ArkSteedSafepointTableBuilder::EmitToNewBuffer() const
{
    size_t size = GetTableSize();
    if (size == 0) {
        return nullptr;
    }
    ASSERT(size > 0);
    uint8_t *buffer = new uint8_t[size];
    Emit(buffer);
    return buffer;
}

// ============================================================================
// Table Reader
// ============================================================================

ArkSteedSafepointTable::ArkSteedSafepointTable(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(ArkSteedSafepointHeader)) {
        return;
    }
    header_ = reinterpret_cast<const ArkSteedSafepointHeader *>(data);
    size_t expectedSize = sizeof(ArkSteedSafepointHeader) + header_->numEntries * sizeof(ArkSteedSafepointEntry);
    if (size < expectedSize) {
        header_ = nullptr;
        return;
    }
    entries_ = reinterpret_cast<const ArkSteedSafepointEntry *>(data + sizeof(ArkSteedSafepointHeader));
}

const ArkSteedSafepointEntry *ArkSteedSafepointTable::FindEntry(uint32_t pcOffset) const
{
    if (header_ == nullptr || header_->numEntries == 0) {
        return nullptr;
    }

    // Binary search: find the last entry with pcOffset <= target
    uint32_t lo = 0;
    uint32_t hi = header_->numEntries;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;  // 2: binary search mid-point calculation
        if (entries_[mid].pcOffset <= pcOffset) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    if (lo == 0) {
        return nullptr;
    }
    return &entries_[lo - 1];
}

}  // namespace panda::ecmascript::arksteed
