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

#include "ecmascript/arksteed/arksteed_deopt_helper.h"

namespace panda::ecmascript::arksteed {
namespace {
constexpr size_t DEOPT_ENTRY_SIZE = 2;  // <id, value>

#if defined(PANDA_TARGET_AMD64)
constexpr Triple TARGET_TRIPLE = Triple::TRIPLE_AMD64;
#elif defined(PANDA_TARGET_ARM64)
constexpr Triple TARGET_TRIPLE = Triple::TRIPLE_AARCH64;
#else
constexpr Triple TARGET_TRIPLE = Triple::TRIPLE_AMD64;
#endif

void EncodeDeoptValue(std::vector<uint8_t> *out, const kungfu::ARKDeopt &deopt)
{
    std::vector<uint8_t> bytes;
    size_t byteSize = 0;
    kungfu::LLVMStackMapType::EncodeVRegsInfo(bytes, byteSize, deopt.id, deopt.kind);
    out->insert(out->end(), bytes.begin(), bytes.begin() + byteSize);

    bytes.clear();
    byteSize = 0;
    if (std::holds_alternative<kungfu::LLVMStackMapType::DwarfRegAndOffsetType>(deopt.value)) {
        auto [reg, offset] = std::get<kungfu::LLVMStackMapType::DwarfRegAndOffsetType>(deopt.value);
        kungfu::LLVMStackMapType::EncodeRegAndOffset(bytes, byteSize, reg, offset, TARGET_TRIPLE);
    } else if (std::holds_alternative<kungfu::LLVMStackMapType::LargeInt>(deopt.value)) {
        kungfu::LLVMStackMapType::EncodeData(bytes, byteSize,
                                             static_cast<kungfu::LLVMStackMapType::LargeInt>(
                                                 std::get<kungfu::LLVMStackMapType::LargeInt>(deopt.value)));
    } else {
        kungfu::LLVMStackMapType::EncodeData(bytes, byteSize,
                                             static_cast<kungfu::LLVMStackMapType::IntType>(
                                                 std::get<kungfu::LLVMStackMapType::IntType>(deopt.value)));
    }
    out->insert(out->end(), bytes.begin(), bytes.begin() + byteSize);
}

std::vector<uint8_t> EncodeDeopts(const std::vector<kungfu::ARKDeopt> &deopts)
{
    std::vector<uint8_t> out;
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

ArkSteedSafepointTableBuilder::~ArkSteedSafepointTableBuilder() = default;

ArkSteedSafepointTableBuilder::Safepoint ArkSteedSafepointTableBuilder::DefineSafepoint(uint32_t pcOffset)
{
    if (entries_.empty()) {
        deoptSideTable_.clear();
    }
    entries_.push_back(NewEntry(pcOffset));
    deoptSideTable_.emplace_back();
    return Safepoint(&entries_.back());
}

void ArkSteedSafepointTableBuilder::DefineDeoptSafepoint(
    uint32_t pcOffset, std::vector<kungfu::ARKDeopt> deopts, ExceptionHandlerKind exceptionHandlerKind)
{
    if (entries_.empty()) {
        deoptSideTable_.clear();
    }
    std::sort(deopts.begin(), deopts.end(), [](const kungfu::ARKDeopt &lhs, const kungfu::ARKDeopt &rhs) {
        return lhs.id < rhs.id;
    });
    ASSERT(deopts.size() <= UINT16_MAX / DEOPT_ENTRY_SIZE);
    entries_.push_back(NewEntry(pcOffset));
    entries_.back().deoptNum = static_cast<uint16_t>(deopts.size() * DEOPT_ENTRY_SIZE);
    uint16_t kind = static_cast<uint16_t>(exceptionHandlerKind);
    ASSERT(kind <= static_cast<uint16_t>(ExceptionHandlerKind::LAZY_DEOPT));
    entries_.back().extraSpillSlotsAndFlags |=
        static_cast<uint16_t>(kind << ArkSteedSafepointEntry::EXCEPTION_HANDLER_KIND_SHIFT);
    deoptSideTable_.push_back(std::move(deopts));
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
    ASSERT(entries_.size() == deoptSideTable_.size());
    for (const auto &deopts : deoptSideTable_) {
        AddEncodedSize(&size, EncodeDeopts(deopts).size(), 1);
    }
    ASSERT(size <= std::numeric_limits<uint32_t>::max());
    return size;
}

void ArkSteedSafepointTableBuilder::Emit(uint8_t *buffer) const
{
    ASSERT(entries_.size() == deoptSideTable_.size());
    const auto &deoptEntries = deoptSideTable_;
    auto *header = reinterpret_cast<ArkSteedSafepointHeader *>(buffer);
    header->numEntries = static_cast<uint32_t>(entries_.size());
    header->numTaggedSlots = numTaggedSlots_;
    header->numUntaggedSlots = numUntaggedSlots_;
    header->reserved = 0;

    auto *entryBuffer = reinterpret_cast<ArkSteedSafepointEntry *>(buffer + sizeof(ArkSteedSafepointHeader));

    std::vector<size_t> order(entries_.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [this](size_t lhs, size_t rhs) {
        return entries_[lhs].pcOffset < entries_[rhs].pcOffset;
    });

    size_t deoptOffset = sizeof(ArkSteedSafepointHeader) + entries_.size() * sizeof(ArkSteedSafepointEntry);
    for (size_t i = 0; i < order.size(); i++) {
        size_t index = order[i];
        entryBuffer[i] = entries_[index];
        auto encodedDeopts = EncodeDeopts(deoptEntries[index]);
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
    if (header->reserved != 0 || header->numEntries > entriesCapacity) {
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

void ArkSteedSafepointTable::GetDeoptInfo(uint32_t pcOffset, std::vector<kungfu::ARKDeopt> &deopts) const
{
    const ArkSteedSafepointEntry *entry = FindEntry(pcOffset);
    if (entry == nullptr || entry->pcOffset != pcOffset || entry->deoptNum == 0) {
        return;
    }

    uint32_t offset = entry->deoptOffset;
    size_t entriesEnd = sizeof(ArkSteedSafepointHeader) + header_->numEntries * sizeof(ArkSteedSafepointEntry);
    ASSERT(offset >= entriesEnd && offset < size_);
    ASSERT(entry->deoptNum % DEOPT_ENTRY_SIZE == 0);
    for (uint32_t i = 0; i < entry->deoptNum; i += DEOPT_ENTRY_SIZE) {
        ASSERT(offset < size_);
        auto [vregsInfo, vregsInfoSize, infoIsFull] =
            panda::leb128::DecodeSigned<kungfu::LLVMStackMapType::SLeb128Type>(data_ + offset);
        (void)infoIsFull;
        ASSERT(vregsInfoSize > 0 && vregsInfoSize <= size_ - offset);
        kungfu::LLVMStackMapType::KindType kindType;
        kungfu::ARKDeopt deopt;
        kungfu::LLVMStackMapType::DecodeVRegsInfo(vregsInfo, deopt.id, kindType);
        offset += vregsInfoSize;
        ASSERT(kindType == kungfu::LLVMStackMapType::CONSTANT_TYPE ||
               kindType == kungfu::LLVMStackMapType::OFFSET_TYPE);
        if (kindType == kungfu::LLVMStackMapType::CONSTANT_TYPE) {
            ASSERT(offset < size_);
            auto [constant, constantSize, constIsFull] =
                panda::leb128::DecodeSigned<kungfu::LLVMStackMapType::SLeb128Type>(data_ + offset);
            (void)constIsFull;
            ASSERT(constantSize > 0 && constantSize <= size_ - offset);
            if (constant > INT32_MAX || constant < INT32_MIN) {
                deopt.kind = kungfu::LocationTy::Kind::CONSTANTNDEX;
                deopt.value = static_cast<kungfu::LLVMStackMapType::LargeInt>(constant);
            } else {
                deopt.kind = kungfu::LocationTy::Kind::CONSTANT;
                deopt.value = static_cast<kungfu::LLVMStackMapType::IntType>(constant);
            }
            offset += constantSize;
        } else {
            ASSERT(offset < size_);
            auto [regOffset, regOffsetSize, regOffIsFull] =
                panda::leb128::DecodeSigned<kungfu::LLVMStackMapType::SLeb128Type>(data_ + offset);
            (void)regOffIsFull;
            ASSERT(regOffsetSize > 0 && regOffsetSize <= size_ - offset);
            kungfu::LLVMStackMapType::DwarfRegType reg;
            kungfu::LLVMStackMapType::OffsetType stackOffset;
            kungfu::LLVMStackMapType::DecodeRegAndOffset(regOffset, reg, stackOffset);
            deopt.kind = kungfu::LocationTy::Kind::INDIRECT;
            deopt.value = std::make_pair(reg, stackOffset);
            offset += regOffsetSize;
        }
        deopts.emplace_back(deopt);
    }
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
