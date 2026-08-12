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
#include <limits>
#include <numeric>
#include <sstream>

namespace panda::ecmascript::arksteed {
namespace {
constexpr size_t DEOPT_LOGICAL_PAIR_SIZE = 2;

void EncodeSignedValue(ChunkVector<uint8_t> *out, int64_t value)
{
    std::vector<uint8_t> bytes;
    size_t byteSize = 0;
    kungfu::LLVMStackMapType::EncodeData(bytes, byteSize, value);
    out->insert(out->end(), bytes.begin(), bytes.begin() + byteSize);
}

void EncodeDeoptValue(ChunkVector<uint8_t> *out, const ArkSteedDeoptValue &deopt)
{
    constexpr int64_t kindCount = 4;
    int64_t vregAndKind = static_cast<int64_t>(deopt.id) * kindCount + static_cast<uint8_t>(deopt.kind);
    EncodeSignedValue(out, vregAndKind);

    if (deopt.kind == ArkSteedDeoptValueKind::STACK_SLOT) {
        EncodeSignedValue(out, deopt.reg);
        EncodeSignedValue(out, deopt.value);
        return;
    }
    EncodeSignedValue(out, deopt.value);
}

ChunkVector<uint8_t> EncodeDeopts(Chunk *chunk, const std::vector<ArkSteedDeoptValue> &deopts)
{
    ChunkVector<uint8_t> out(chunk);
    for (const auto &deopt : deopts) {
        EncodeDeoptValue(&out, deopt);
    }
    return out;
}

void AddEncodedSize(size_t *size, size_t count, size_t itemSize)
{
    ASSERT(size != nullptr);
    ASSERT(itemSize == 0 || count <= (std::numeric_limits<size_t>::max() - *size) / itemSize);
    *size += count * itemSize;
}

ArkSteedSafepointEntry NewEntry(uint32_t pcOffset)
{
    ArkSteedSafepointEntry entry {};
    entry.pcOffset = pcOffset;
    return entry;
}

}  // namespace

// ============================================================================
// Builder
// ============================================================================

ArkSteedSafepointTableBuilder::Safepoint ArkSteedSafepointTableBuilder::DefineSafepoint(uint32_t pcOffset)
{
    if (entries_.empty()) {
        encodedDeoptData_.clear();
    }
    entries_.push_back(NewEntry(pcOffset));
    encodedDeoptData_.emplace_back(chunk_);
    return Safepoint(&entries_.back());
}

void ArkSteedSafepointTableBuilder::DefineDeoptSafepoint(
    uint32_t pcOffset, std::vector<ArkSteedDeoptValue> deopts, ExceptionHandlerKind exceptionHandlerKind)
{
    if (entries_.empty()) {
        encodedDeoptData_.clear();
    }
    std::sort(deopts.begin(), deopts.end(), [](const ArkSteedDeoptValue &lhs, const ArkSteedDeoptValue &rhs) {
        return lhs.id < rhs.id;
    });
    ASSERT(deopts.size() <= UINT16_MAX / DEOPT_LOGICAL_PAIR_SIZE);
    entries_.push_back(NewEntry(pcOffset));
    entries_.back().deoptNum = static_cast<uint16_t>(deopts.size() * DEOPT_LOGICAL_PAIR_SIZE);
    uint16_t kind = static_cast<uint16_t>(exceptionHandlerKind);
    ASSERT(kind <= static_cast<uint16_t>(ExceptionHandlerKind::LAZY_DEOPT));
    entries_.back().extraSpillSlotsAndFlags |=
        static_cast<uint16_t>(kind << ArkSteedSafepointEntry::EXCEPTION_HANDLER_KIND_SHIFT);
    encodedDeoptData_.push_back(EncodeDeopts(chunk_, deopts));
}

void ArkSteedSafepointTableBuilder::SetFrameSlots(uint32_t tagged, uint32_t untagged)
{
    numTaggedSlots_ = tagged;
    numUntaggedSlots_ = untagged;
}

size_t ArkSteedSafepointTableBuilder::GetTableSize() const
{
    ASSERT(entries_.size() <= std::numeric_limits<uint32_t>::max());
    size_t size = sizeof(ArkSteedSafepointHeader);
    AddEncodedSize(&size, entries_.size(), sizeof(ArkSteedSafepointEntry));
    ASSERT(entries_.size() == encodedDeoptData_.size());
    for (const auto &encoded : encodedDeoptData_) {
        AddEncodedSize(&size, encoded.size(), 1);
    }
    ASSERT(size <= std::numeric_limits<uint32_t>::max());
    return size;
}

void ArkSteedSafepointTableBuilder::Emit(uint8_t *buffer) const
{
    ASSERT(entries_.size() == encodedDeoptData_.size());
    auto *header = reinterpret_cast<ArkSteedSafepointHeader *>(buffer);
    header->numEntries = static_cast<uint32_t>(entries_.size());
    header->numTaggedSlots = numTaggedSlots_;
    header->numUntaggedSlots = numUntaggedSlots_;
    header->deoptLiteralCount = deoptLiteralCount_;

    auto *entryBuffer = reinterpret_cast<ArkSteedSafepointEntry *>(buffer + sizeof(ArkSteedSafepointHeader));

    ChunkVector<size_t> order(entries_.size(), chunk_);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [this](size_t lhs, size_t rhs) {
        return entries_[lhs].pcOffset < entries_[rhs].pcOffset;
    });

    size_t deoptOffset = sizeof(ArkSteedSafepointHeader) + entries_.size() * sizeof(ArkSteedSafepointEntry);
    for (size_t i = 0; i < order.size(); i++) {
        size_t index = order[i];
        entryBuffer[i] = entries_[index];
        const auto &encodedDeopts = encodedDeoptData_[index];
        if (!encodedDeopts.empty()) {
            ASSERT(deoptOffset <= std::numeric_limits<uint32_t>::max());
            entryBuffer[i].deoptOffset = static_cast<uint32_t>(deoptOffset);
            std::memcpy(buffer + deoptOffset, encodedDeopts.data(), encodedDeopts.size());
            deoptOffset += encodedDeopts.size();
        }
    }

    ASSERT(deoptOffset == GetTableSize());
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

std::string ArkSteedSafepointTableBuilder::DumpMemoryUsage() const
{
    ASSERT(entries_.size() == encodedDeoptData_.size());
    size_t headerSize = sizeof(ArkSteedSafepointHeader);
    size_t entriesSize = entries_.size() * sizeof(ArkSteedSafepointEntry);
    size_t deoptSize = 0;
    uint32_t deoptEntryCount = 0;
    for (const auto &encoded : encodedDeoptData_) {
        deoptSize += encoded.size();
        if (!encoded.empty()) {
            ++deoptEntryCount;
        }
    }
    size_t totalSize = headerSize + entriesSize + deoptSize;
    std::stringstream ss;
    ss << "Safepoint table: " << entries_.size() << " entries ("
       << deoptEntryCount << " with deopt), "
       << totalSize << " bytes total ("
       << headerSize << " header + "
       << entriesSize << " entries + "
       << deoptSize << " deopt data)";
    return ss.str();
}

// ============================================================================
// Table Reader
// ============================================================================

ArkSteedSafepointTable::ArkSteedSafepointTable(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(ArkSteedSafepointHeader)) {
        return;
    }
    auto *header = reinterpret_cast<const ArkSteedSafepointHeader *>(data);
    size_t entriesCapacity = (size - sizeof(ArkSteedSafepointHeader)) / sizeof(ArkSteedSafepointEntry);
    if (header->numEntries > entriesCapacity) {
        return;
    }
    data_ = data;
    size_ = size;
    header_ = header;
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

bool ArkSteedSafepointTable::GetDeoptInfo(uint32_t pcOffset, const uint64_t *deoptLiterals,
                                         uint32_t deoptLiteralCount, std::vector<kungfu::ARKDeopt> &deopts) const
{
    const ArkSteedSafepointEntry *entry = FindEntry(pcOffset);
    if (entry == nullptr || entry->pcOffset != pcOffset || entry->deoptNum == 0) {
        return true;
    }
    if (deoptLiteralCount != header_->deoptLiteralCount ||
        (deoptLiteralCount != 0 && deoptLiterals == nullptr)) {
        return false;
    }

    uint32_t offset = entry->deoptOffset;
    size_t entriesEnd = sizeof(ArkSteedSafepointHeader) + header_->numEntries * sizeof(ArkSteedSafepointEntry);
    ASSERT(offset >= entriesEnd && offset < size_);
    ASSERT(entry->deoptNum % DEOPT_LOGICAL_PAIR_SIZE == 0);
    std::vector<kungfu::ARKDeopt> decodedDeopts;
    decodedDeopts.reserve(entry->deoptNum / DEOPT_LOGICAL_PAIR_SIZE);
    for (uint32_t i = 0; i < entry->deoptNum; i += DEOPT_LOGICAL_PAIR_SIZE) {
        ASSERT(offset < size_);
        auto [vregsInfo, vregsInfoSize, infoIsFull] =
            panda::leb128::DecodeSigned<kungfu::LLVMStackMapType::SLeb128Type>(data_ + offset);
        (void)infoIsFull;
        ASSERT(vregsInfoSize > 0 && vregsInfoSize <= size_ - offset);
        constexpr int64_t kindCount = 4;
        auto kind = static_cast<ArkSteedDeoptValueKind>(static_cast<uint64_t>(vregsInfo) & (kindCount - 1));
        int64_t vreg = (vregsInfo - static_cast<uint8_t>(kind)) / kindCount;
        if (kind > ArkSteedDeoptValueKind::HEAP_LITERAL ||
            vreg < std::numeric_limits<kungfu::LLVMStackMapType::VRegId>::min() ||
            vreg > std::numeric_limits<kungfu::LLVMStackMapType::VRegId>::max()) {
            return false;
        }
        kungfu::ARKDeopt deopt;
        deopt.id = static_cast<kungfu::LLVMStackMapType::VRegId>(vreg);
        offset += vregsInfoSize;
        if (kind == ArkSteedDeoptValueKind::CONSTANT || kind == ArkSteedDeoptValueKind::HEAP_LITERAL) {
            ASSERT(offset < size_);
            auto [encodedValue, encodedSize, valueIsFull] =
                panda::leb128::DecodeSigned<kungfu::LLVMStackMapType::SLeb128Type>(data_ + offset);
            (void)valueIsFull;
            ASSERT(encodedSize > 0 && encodedSize <= size_ - offset);
            int64_t constant = encodedValue;
            if (kind == ArkSteedDeoptValueKind::HEAP_LITERAL) {
                if (encodedValue < 0 || static_cast<uint64_t>(encodedValue) >= deoptLiteralCount) {
                    return false;
                }
                constant = static_cast<int64_t>(deoptLiterals[static_cast<uint32_t>(encodedValue)]);
            }
            if (constant > INT32_MAX || constant < INT32_MIN) {
                deopt.kind = kungfu::LocationTy::Kind::CONSTANTNDEX;
                deopt.value = static_cast<kungfu::LLVMStackMapType::LargeInt>(constant);
            } else {
                deopt.kind = kungfu::LocationTy::Kind::CONSTANT;
                deopt.value = static_cast<kungfu::LLVMStackMapType::IntType>(constant);
            }
            offset += encodedSize;
        } else {
            ASSERT(offset < size_);
            auto [encodedReg, encodedRegSize, regIsFull] =
                panda::leb128::DecodeSigned<kungfu::LLVMStackMapType::SLeb128Type>(data_ + offset);
            (void)regIsFull;
            ASSERT(encodedRegSize > 0 && encodedRegSize <= size_ - offset);
            offset += encodedRegSize;
            if ((encodedReg != GCStackMapRegisters::FP && encodedReg != GCStackMapRegisters::SP) ||
                offset >= size_) {
                return false;
            }
            auto [encodedOffset, encodedOffsetSize, offsetIsFull] =
                panda::leb128::DecodeSigned<kungfu::LLVMStackMapType::SLeb128Type>(data_ + offset);
            (void)offsetIsFull;
            ASSERT(encodedOffsetSize > 0 && encodedOffsetSize <= size_ - offset);
            if (encodedOffset < std::numeric_limits<kungfu::LLVMStackMapType::OffsetType>::min() ||
                encodedOffset > std::numeric_limits<kungfu::LLVMStackMapType::OffsetType>::max()) {
                return false;
            }
            deopt.kind = kungfu::LocationTy::Kind::INDIRECT;
            deopt.value = std::make_pair(
                static_cast<kungfu::LLVMStackMapType::DwarfRegType>(encodedReg),
                static_cast<kungfu::LLVMStackMapType::OffsetType>(encodedOffset));
            offset += encodedOffsetSize;
        }
        decodedDeopts.emplace_back(deopt);
    }
    deopts.insert(deopts.end(), decodedDeopts.begin(), decodedDeopts.end());
    return true;
}

ExceptionHandlerKind ArkSteedSafepointTable::GetExceptionHandlerKind(uint32_t pcOffset) const
{
    const ArkSteedSafepointEntry *entry = FindEntry(pcOffset);
    if (entry == nullptr || entry->pcOffset != pcOffset) {
        return ExceptionHandlerKind::NONE;
    }
    return entry->GetExceptionHandlerKind();
}

}  // namespace panda::ecmascript::arksteed
