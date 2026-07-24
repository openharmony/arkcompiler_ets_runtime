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
#include <type_traits>

#include "ecmascript/arksteed/arksteed_deopt_helper.h"
#include "ecmascript/base/hash_combine.h"

namespace panda::ecmascript::arksteed {
namespace {
constexpr size_t DEOPT_ENTRY_SIZE = 2;  // <id, value>
constexpr size_t DEOPT_TRANSLATION_COUNT_SIZE = sizeof(uint32_t);
constexpr size_t DEOPT_TRANSLATION_HEADER_SIZE = sizeof(uint32_t) * 4U;
constexpr size_t DEOPT_TRANSLATION_INPUT_SIZE = sizeof(int32_t) + sizeof(uint8_t) * 2U + sizeof(uint16_t) +
                                                sizeof(int64_t);

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

template <class T>
void WriteValue(uint8_t **cursor, T value)
{
    static_assert(std::is_trivially_copyable_v<T>);
    std::memcpy(*cursor, &value, sizeof(T));
    *cursor += sizeof(T);
}

template <class T>
bool ReadValue(const uint8_t **cursor, const uint8_t *end, T *value)
{
    static_assert(std::is_trivially_copyable_v<T>);
    if (*cursor > end || static_cast<size_t>(end - *cursor) < sizeof(T)) {
        return false;
    }
    std::memcpy(value, *cursor, sizeof(T));
    *cursor += sizeof(T);
    return true;
}

void AddEncodedSize(size_t *size, size_t count, size_t itemSize)
{
    ASSERT(size != nullptr);
    ASSERT(itemSize == 0 || count <= (std::numeric_limits<size_t>::max() - *size) / itemSize);
    *size += count * itemSize;
}

size_t GetArkSteedDeoptTranslationsSize(const std::vector<ArkSteedDeoptTranslation> &translations)
{
    ASSERT(translations.size() <= std::numeric_limits<uint32_t>::max());
    size_t size = DEOPT_TRANSLATION_COUNT_SIZE;
    for (const auto &translation : translations) {
        ASSERT(translation.inputs.size() <= std::numeric_limits<uint32_t>::max());
        AddEncodedSize(&size, 1, DEOPT_TRANSLATION_HEADER_SIZE);
        AddEncodedSize(&size, translation.inputs.size(), DEOPT_TRANSLATION_INPUT_SIZE);
    }
    return size;
}

void EncodeArkSteedDeoptTranslations(uint8_t *buffer,
                                     const std::vector<ArkSteedDeoptTranslation> &translations)
{
    uint8_t *cursor = buffer;
    WriteValue<uint32_t>(&cursor, static_cast<uint32_t>(translations.size()));
    for (const auto &translation : translations) {
        WriteValue<uint32_t>(&cursor, translation.id.value);
        WriteValue<uint32_t>(&cursor, translation.bytecodeOffset);
        WriteValue<uint32_t>(&cursor, static_cast<uint32_t>(translation.type));
        WriteValue<uint32_t>(&cursor, static_cast<uint32_t>(translation.inputs.size()));
        for (const auto &input : translation.inputs) {
            WriteValue<int32_t>(&cursor, input.vreg);
            WriteValue<uint8_t>(&cursor, static_cast<uint8_t>(input.valueKind));
            WriteValue<uint8_t>(&cursor, static_cast<uint8_t>(input.sourceKind));
            WriteValue<uint16_t>(&cursor, 0);
            WriteValue<int64_t>(&cursor, input.source);
        }
    }
    ASSERT(static_cast<size_t>(cursor - buffer) == GetArkSteedDeoptTranslationsSize(translations));
}

bool IsValidDeoptValueKind(uint8_t value)
{
    return value <= static_cast<uint8_t>(ArkSteedDeoptValueKind::RAW_INT32);
}

bool IsValidDeoptSourceKind(uint8_t value)
{
    return value <= static_cast<uint8_t>(ArkSteedDeoptSourceKind::FP_REGISTER);
}

bool IsValidDeoptType(uint32_t value)
{
    return value <= static_cast<uint32_t>(kungfu::DeoptType::HOTRELOAD_PATCHMAIN);
}

bool IsValidRegisterSource(ArkSteedDeoptSourceKind kind, int64_t source)
{
    if (kind == ArkSteedDeoptSourceKind::GP_REGISTER) {
        return source >= 0 && static_cast<uint64_t>(source) <= std::numeric_limits<uint32_t>::max() &&
               GetArkSteedDeoptGeneralSnapshotOffset(static_cast<uint32_t>(source)) >= 0;
    }
    if (kind == ArkSteedDeoptSourceKind::FP_REGISTER) {
        return source >= 0 && static_cast<uint64_t>(source) <= std::numeric_limits<uint32_t>::max() &&
               GetArkSteedDeoptFloatingSnapshotOffset(static_cast<uint32_t>(source)) >= 0;
    }
    return true;
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

ArkSteedDeoptId ArkSteedSafepointTableBuilder::DefineArkSteedDeoptTranslation(
    uint32_t bytecodeOffset, kungfu::DeoptType type, std::vector<ArkSteedDeoptTranslationInput> inputs)
{
    ASSERT(deoptTranslations_.size() <= static_cast<size_t>(std::numeric_limits<int32_t>::max()));
    ArkSteedDeoptTranslation candidate {
        ArkSteedDeoptId {static_cast<uint32_t>(deoptTranslations_.size())},
        bytecodeOffset,
        type,
        std::move(inputs),
    };
    uint64_t payloadHash = base::HashCombiner::HashCombine(0, candidate.bytecodeOffset);
    payloadHash = base::HashCombiner::HashCombine(payloadHash, candidate.inputs.size());
    for (const auto &input : candidate.inputs) {
        payloadHash = base::HashCombiner::HashCombine(payloadHash, static_cast<uint64_t>(input.vreg));
        payloadHash = base::HashCombiner::HashCombine(payloadHash, static_cast<uint64_t>(input.valueKind));
        payloadHash = base::HashCombiner::HashCombine(payloadHash, static_cast<uint64_t>(input.sourceKind));
        payloadHash = base::HashCombiner::HashCombine(payloadHash, static_cast<uint64_t>(input.source));
    }
    auto [begin, end] = deoptTranslationIndex_.equal_range(payloadHash);
    for (auto iterator = begin; iterator != end; ++iterator) {
        ASSERT(iterator->second.value < deoptTranslations_.size());
        const auto &translation = deoptTranslations_[iterator->second.value];
        if (translation.PayloadEquals(candidate)) {
            return translation.id;
        }
    }
    deoptTranslations_.push_back(std::move(candidate));
    deoptTranslationIndex_.emplace(payloadHash, deoptTranslations_.back().id);
    return deoptTranslations_.back().id;
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
    if (!deoptTranslations_.empty()) {
        AddEncodedSize(&size, 1, sizeof(ArkSteedSafepointExtensionHeader));
        AddEncodedSize(&size, GetArkSteedDeoptTranslationsSize(deoptTranslations_), 1);
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
    header->extensionOffset = 0;

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

    if (!deoptTranslations_.empty()) {
        size_t translationSize = GetArkSteedDeoptTranslationsSize(deoptTranslations_);
        size_t translationOffset = deoptOffset + sizeof(ArkSteedSafepointExtensionHeader);
        ASSERT(translationOffset <= std::numeric_limits<uint32_t>::max());
        ASSERT(translationSize <= std::numeric_limits<uint32_t>::max());
        header->extensionOffset = static_cast<uint32_t>(deoptOffset);
        ArkSteedSafepointExtensionHeader extension {
            ARKSTEED_SAFEPOINT_EXTENSION_MAGIC,
            ARKSTEED_SAFEPOINT_EXTENSION_VERSION,
            ARKSTEED_SAFEPOINT_EXTENSION_HAS_DEOPT_TRANSLATIONS,
            static_cast<uint32_t>(translationOffset),
            static_cast<uint32_t>(translationSize),
        };
        std::memcpy(buffer + deoptOffset, &extension, sizeof(extension));
        EncodeArkSteedDeoptTranslations(buffer + translationOffset, deoptTranslations_);
        deoptOffset = translationOffset + translationSize;
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
    if (header->numEntries > entriesCapacity) {
        return;
    }
    data_ = data;
    size_ = size;
    header_ = header;
    entries_ = reinterpret_cast<const ArkSteedSafepointEntry *>(data + sizeof(ArkSteedSafepointHeader));
}

bool ArkSteedSafepointTable::IsRangeValid(size_t offset, size_t length) const
{
    return offset <= size_ && length <= size_ - offset;
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

bool ArkSteedSafepointTable::GetArkSteedDeoptTranslation(ArkSteedDeoptId deoptId,
                                                         ArkSteedDeoptTranslation *translation) const
{
    if (header_ == nullptr || header_->extensionOffset == 0 || translation == nullptr) {
        return false;
    }

    size_t extensionOffset = header_->extensionOffset;
    size_t entriesEnd = sizeof(ArkSteedSafepointHeader) +
                        static_cast<size_t>(header_->numEntries) * sizeof(ArkSteedSafepointEntry);
    if (extensionOffset < entriesEnd || !IsRangeValid(extensionOffset, sizeof(ArkSteedSafepointExtensionHeader))) {
        return false;
    }

    ArkSteedSafepointExtensionHeader extension {};
    std::memcpy(&extension, data_ + extensionOffset, sizeof(extension));
    if (extension.magic != ARKSTEED_SAFEPOINT_EXTENSION_MAGIC ||
        extension.version != ARKSTEED_SAFEPOINT_EXTENSION_VERSION ||
        (extension.flags & static_cast<uint16_t>(~ARKSTEED_SAFEPOINT_EXTENSION_KNOWN_FLAGS)) != 0 ||
        (extension.flags & ARKSTEED_SAFEPOINT_EXTENSION_HAS_DEOPT_TRANSLATIONS) == 0) {
        return false;
    }

    size_t extensionEnd = extensionOffset + sizeof(ArkSteedSafepointExtensionHeader);
    size_t translationOffset = extension.deoptTranslationOffset;
    size_t translationSize = extension.deoptTranslationSize;
    if (translationOffset < extensionEnd || translationSize < DEOPT_TRANSLATION_COUNT_SIZE ||
        !IsRangeValid(translationOffset, translationSize)) {
        return false;
    }

    const uint8_t *cursor = data_ + translationOffset;
    const uint8_t *end = cursor + translationSize;
    uint32_t numTranslations = 0;
    if (!ReadValue<uint32_t>(&cursor, end, &numTranslations) ||
        numTranslations > static_cast<size_t>(end - cursor) / DEOPT_TRANSLATION_HEADER_SIZE) {
        return false;
    }

    for (uint32_t i = 0; i < numTranslations; ++i) {
        uint32_t currentId = 0;
        uint32_t bytecodeOffset = 0;
        uint32_t rawType = 0;
        uint32_t inputCount = 0;
        if (!ReadValue<uint32_t>(&cursor, end, &currentId) ||
            !ReadValue<uint32_t>(&cursor, end, &bytecodeOffset) ||
            !ReadValue<uint32_t>(&cursor, end, &rawType) ||
            !ReadValue<uint32_t>(&cursor, end, &inputCount) || currentId != i || !IsValidDeoptType(rawType) ||
            inputCount > static_cast<size_t>(end - cursor) / DEOPT_TRANSLATION_INPUT_SIZE) {
            return false;
        }

        bool isRequestedTranslation = currentId == deoptId.value;
        ArkSteedDeoptTranslation current {};
        if (isRequestedTranslation) {
            current.id = ArkSteedDeoptId {currentId};
            current.bytecodeOffset = bytecodeOffset;
            current.type = static_cast<kungfu::DeoptType>(rawType);
            if (inputCount > current.inputs.max_size()) {
                return false;
            }
            current.inputs.reserve(inputCount);
        }

        for (uint32_t inputIndex = 0; inputIndex < inputCount; ++inputIndex) {
            int32_t vreg = 0;
            uint8_t rawValueKind = 0;
            uint8_t rawSourceKind = 0;
            uint16_t reserved = 0;
            int64_t source = 0;
            if (!ReadValue<int32_t>(&cursor, end, &vreg) ||
                !ReadValue<uint8_t>(&cursor, end, &rawValueKind) ||
                !ReadValue<uint8_t>(&cursor, end, &rawSourceKind) ||
                !ReadValue<uint16_t>(&cursor, end, &reserved) ||
                !ReadValue<int64_t>(&cursor, end, &source) || reserved != 0 ||
                !IsValidDeoptValueKind(rawValueKind) || !IsValidDeoptSourceKind(rawSourceKind)) {
                return false;
            }
            auto sourceKind = static_cast<ArkSteedDeoptSourceKind>(rawSourceKind);
            if (!IsValidRegisterSource(sourceKind, source)) {
                return false;
            }
            if (isRequestedTranslation) {
                current.inputs.push_back({
                    vreg,
                    static_cast<ArkSteedDeoptValueKind>(rawValueKind),
                    sourceKind,
                    source,
                });
            }
        }
        if (isRequestedTranslation) {
            *translation = std::move(current);
            return true;
        }
    }
    return false;
}

}  // namespace panda::ecmascript::arksteed
