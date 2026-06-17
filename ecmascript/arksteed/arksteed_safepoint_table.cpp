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
#include <numeric>
#include <unordered_map>

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

ArkSteedSafepointEntry NewEntry(uint32_t pcOffset)
{
    ArkSteedSafepointEntry entry {};
    entry.pcOffset = pcOffset;
    return entry;
}

using DeoptSideTable = std::vector<std::vector<kungfu::ARKDeopt>>;

std::unordered_map<const ArkSteedSafepointTableBuilder *, DeoptSideTable> &GetDeoptSideTables()
{
    static std::unordered_map<const ArkSteedSafepointTableBuilder *, DeoptSideTable> tables;
    return tables;
}

DeoptSideTable &GetDeoptSideTable(const ArkSteedSafepointTableBuilder *builder)
{
    return GetDeoptSideTables()[builder];
}

DeoptSideTable &GetSyncedDeoptSideTable(const ArkSteedSafepointTableBuilder *builder, size_t entryCount)
{
    DeoptSideTable &deoptEntries = GetDeoptSideTable(builder);
    deoptEntries.resize(entryCount);
    return deoptEntries;
}
}  // namespace

// ============================================================================
// Builder
// ============================================================================

ArkSteedSafepointTableBuilder::~ArkSteedSafepointTableBuilder()
{
    GetDeoptSideTables().erase(this);
}

ArkSteedSafepointTableBuilder::Safepoint ArkSteedSafepointTableBuilder::DefineSafepoint(uint32_t pcOffset)
{
    DeoptSideTable &deoptEntries = GetDeoptSideTable(this);
    if (entries_.empty()) {
        deoptEntries.clear();
    }
    entries_.push_back(NewEntry(pcOffset));
    deoptEntries.emplace_back();
    return Safepoint(&entries_.back());
}

void ArkSteedSafepointTableBuilder::DefineDeoptSafepoint(uint32_t pcOffset, std::vector<kungfu::ARKDeopt> deopts)
{
    DeoptSideTable &deoptEntries = GetDeoptSideTable(this);
    if (entries_.empty()) {
        deoptEntries.clear();
    }
    std::sort(deopts.begin(), deopts.end(), [](const kungfu::ARKDeopt &lhs, const kungfu::ARKDeopt &rhs) {
        return lhs.id < rhs.id;
    });
    entries_.push_back(NewEntry(pcOffset));
    entries_.back().deoptNum = static_cast<uint16_t>(deopts.size() * DEOPT_ENTRY_SIZE);
    deoptEntries.push_back(std::move(deopts));
}

void ArkSteedSafepointTableBuilder::SetFrameSlots(uint32_t tagged, uint32_t untagged)
{
    numTaggedSlots_ = tagged;
    numUntaggedSlots_ = untagged;
}

size_t ArkSteedSafepointTableBuilder::GetTableSize() const
{
    size_t size = sizeof(ArkSteedSafepointHeader) + entries_.size() * sizeof(ArkSteedSafepointEntry);
    const auto &deoptEntries = GetSyncedDeoptSideTable(this, entries_.size());
    for (const auto &deopts : deoptEntries) {
        size += EncodeDeopts(deopts).size();
    }
    return size;
}

void ArkSteedSafepointTableBuilder::Emit(uint8_t *buffer) const
{
    const auto &deoptEntries = GetSyncedDeoptSideTable(this, entries_.size());
    ASSERT(entries_.size() == deoptEntries.size());
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

    uint32_t deoptOffset =
        static_cast<uint32_t>(sizeof(ArkSteedSafepointHeader) + entries_.size() * sizeof(ArkSteedSafepointEntry));
    for (size_t i = 0; i < order.size(); i++) {
        size_t index = order[i];
        entryBuffer[i] = entries_[index];
        auto encodedDeopts = EncodeDeopts(deoptEntries[index]);
        if (!encodedDeopts.empty()) {
            entryBuffer[i].deoptOffset = deoptOffset;
            std::memcpy(buffer + deoptOffset, encodedDeopts.data(), encodedDeopts.size());
            deoptOffset += static_cast<uint32_t>(encodedDeopts.size());
        }
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
    data_ = data;
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

void ArkSteedSafepointTable::GetDeoptInfo(uint32_t pcOffset, std::vector<kungfu::ARKDeopt> &deopts) const
{
    const ArkSteedSafepointEntry *entry = FindEntry(pcOffset);
    if (entry == nullptr || entry->pcOffset != pcOffset || entry->deoptNum == 0) {
        return;
    }

    uint32_t offset = entry->deoptOffset;
    ASSERT(entry->deoptNum % DEOPT_ENTRY_SIZE == 0);
    for (uint32_t i = 0; i < entry->deoptNum; i += DEOPT_ENTRY_SIZE) {
        auto [vregsInfo, vregsInfoSize, infoIsFull] =
            panda::leb128::DecodeSigned<kungfu::LLVMStackMapType::SLeb128Type>(data_ + offset);
        (void)infoIsFull;
        kungfu::LLVMStackMapType::KindType kindType;
        kungfu::ARKDeopt deopt;
        kungfu::LLVMStackMapType::DecodeVRegsInfo(vregsInfo, deopt.id, kindType);
        offset += vregsInfoSize;
        ASSERT(kindType == kungfu::LLVMStackMapType::CONSTANT_TYPE ||
               kindType == kungfu::LLVMStackMapType::OFFSET_TYPE);
        if (kindType == kungfu::LLVMStackMapType::CONSTANT_TYPE) {
            auto [constant, constantSize, constIsFull] =
                panda::leb128::DecodeSigned<kungfu::LLVMStackMapType::SLeb128Type>(data_ + offset);
            (void)constIsFull;
            if (constant > INT32_MAX || constant < INT32_MIN) {
                deopt.kind = kungfu::LocationTy::Kind::CONSTANTNDEX;
                deopt.value = static_cast<kungfu::LLVMStackMapType::LargeInt>(constant);
            } else {
                deopt.kind = kungfu::LocationTy::Kind::CONSTANT;
                deopt.value = static_cast<kungfu::LLVMStackMapType::IntType>(constant);
            }
            offset += constantSize;
        } else {
            auto [regOffset, regOffsetSize, regOffIsFull] =
                panda::leb128::DecodeSigned<kungfu::LLVMStackMapType::SLeb128Type>(data_ + offset);
            (void)regOffIsFull;
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

}  // namespace panda::ecmascript::arksteed
