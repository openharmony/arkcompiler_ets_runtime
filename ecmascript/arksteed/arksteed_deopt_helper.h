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

#ifndef ECMASCRIPT_ARKSTEED_DEOPT_HELPER_H
#define ECMASCRIPT_ARKSTEED_DEOPT_HELPER_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/compiler/deopt_type.h"
#include "ecmascript/js_tagged_value_internals.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class JSThread;

namespace arksteed {
class ArkSteedAssembler;
class EagerDeoptimizableMixin;
class ValueVertex;

// Reuse DeoptHandlerAsm's stable bridge ABI while keeping ArkSteed dispatch explicit.
constexpr int32_t ARKSTEED_DEOPT_DISPATCH_MARKER = -1;
static_assert(ARKSTEED_DEOPT_DISPATCH_MARKER < 0);

// These values describe the recovery semantics used by codegen and runtime materialization.
enum class DeoptTranslationKind : uint8_t {
    TAGGED = 0,
    INT32_TO_TAGGED = 1,
    FLOAT64_TO_TAGGED_DOUBLE = 2,
    RAW_INT32 = 3,
};

enum class DeoptSourceKind : uint8_t {
    CONSTANT = 0,
    STACK_SLOT = 1,
    GP_REGISTER = 2,
    FP_REGISTER = 3,
};

/*
 * Persisted deopt translation format:
 *
 *   uint32_t translationCount
 *   uint32_t translationOffsets[translationCount]
 *   uint8_t opcodeStream[]
 *
 * Fixed-width integers use little-endian byte order and are read with memcpy to permit unaligned data. The offset
 * table is indexed by DeoptId. Each offset is relative to the beginning of opcodeStream and points to a BEGIN opcode.
 * A translation does not encode its DeoptId again.
 *
 * Unsigned operands use ULEB128:
 *   basisIdDistance, bytecodeOffset, deoptType, inputCount, registerCode, specialKind and long matchCount.
 * Signed operands use SLEB128:
 *   vreg, frame-pointer-relative stackOffset and signed integer constants.
 * LEB128 operands use their shortest canonical byte sequence. Readers reject overflow and unterminated sequences.
 * specialKind values are 0:undefined, 1:null, 2:true, 3:false, 4:hole and 5:exception.
 * Float64 constants and tagged-double bit patterns store their raw 64-bit value in little-endian byte order.
 *
 * Opcode operands:
 *   BEGIN                    basisIdDistance:ULEB, bytecodeOffset:ULEB, deoptType:ULEB, inputCount:ULEB
 *   TAGGED_REGISTER          vreg:SLEB, registerCode:ULEB
 *   TAGGED_STACK_SLOT        vreg:SLEB, stackOffset:SLEB
 *   TAGGED_SPECIAL           vreg:SLEB, specialKind:ULEB
 *   TAGGED_INT_CONSTANT      vreg:SLEB, value:SLEB
 *   TAGGED_DOUBLE_BITS       vreg:SLEB, rawBits:uint64
 *   INT32_REGISTER           vreg:SLEB, registerCode:ULEB
 *   INT32_STACK_SLOT         vreg:SLEB, stackOffset:SLEB
 *   INT32_CONSTANT           vreg:SLEB, value:SLEB
 *   FLOAT64_REGISTER         vreg:SLEB, registerCode:ULEB
 *   FLOAT64_STACK_SLOT       vreg:SLEB, stackOffset:SLEB
 *   FLOAT64_CONSTANT         vreg:SLEB, rawBits:uint64
 *   RAW_INT32_REGISTER       vreg:SLEB, registerCode:ULEB
 *   RAW_INT32_STACK_SLOT     vreg:SLEB, stackOffset:SLEB
 *   RAW_INT32_CONSTANT       vreg:SLEB, value:SLEB
 *   MATCH_BASIS              matchCount:ULEB
 *
 * basisIdDistance is the difference between the current DeoptId and the Basis DeoptId. Zero identifies a full Basis
 * translation. A delta translation directly references an earlier full Basis with the same bytecodeOffset, inputCount
 * and logical vreg order. MATCH_BASIS copies input descriptions from the same logical positions in that Basis.
 * Values below COUNT are regular opcodes. Values from COUNT through UINT8_MAX encode a short Basis match whose count
 * is encodedValue - COUNT + 1. The decoder finishes after producing exactly inputCount logical inputs; no END opcode
 * is stored. Raw translation data may contain immediate tagged values, signed integers and floating-point bits, but
 * never an address managed by the moving GC.
 */
enum class DeoptTranslationOpcode : uint8_t {
    BEGIN = 0,
    TAGGED_REGISTER,
    TAGGED_STACK_SLOT,
    TAGGED_SPECIAL,
    TAGGED_INT_CONSTANT,
    TAGGED_DOUBLE_BITS,
    INT32_REGISTER,
    INT32_STACK_SLOT,
    INT32_CONSTANT,
    FLOAT64_REGISTER,
    FLOAT64_STACK_SLOT,
    FLOAT64_CONSTANT,
    RAW_INT32_REGISTER,
    RAW_INT32_STACK_SLOT,
    RAW_INT32_CONSTANT,
    MATCH_BASIS,
    COUNT,
};

struct DeoptId {
    uint32_t value {0};

    bool operator==(const DeoptId &other) const
    {
        return value == other.value;
    }

    bool operator!=(const DeoptId &other) const
    {
        return !(*this == other);
    }
};

struct DeoptTranslationInput {
    int32_t vreg {0};
    DeoptTranslationKind valueKind {DeoptTranslationKind::TAGGED};
    DeoptSourceKind sourceKind {DeoptSourceKind::CONSTANT};
    int64_t source {0};

    bool operator==(const DeoptTranslationInput &other) const
    {
        return vreg == other.vreg && valueKind == other.valueKind && sourceKind == other.sourceKind &&
               source == other.source;
    }
};

struct DeoptTranslation {
    uint32_t bytecodeOffset {0};
    kungfu::DeoptType type {kungfu::DeoptType::NONE};
    std::vector<DeoptTranslationInput> inputs;

    bool PayloadEquals(const DeoptTranslation &other) const
    {
        // The reason remains representative diagnostic data; recovery identity is the bytecode state and sources.
        return bytecodeOffset == other.bytecodeOffset && inputs == other.inputs;
    }
};

class DeoptTranslationBuilder {
public:
    DeoptId AddTranslation(uint32_t bytecodeOffset, kungfu::DeoptType type, std::vector<DeoptTranslationInput> inputs);
    std::vector<uint8_t> Encode() const;

private:
    std::vector<DeoptTranslation> translations_;
    // Maps a payload hash to candidate ids for deduplication. A hash match is followed by a full PayloadEquals
    // comparison against translations_[id.value]; only an equal payload reuses the existing DeoptId.
    std::unordered_multimap<uint64_t, DeoptId> translationIndex_;
    // The first unique translation at each bytecode offset is the only Basis referenced by later translations.
    std::unordered_map<uint32_t, DeoptId> basisTranslationIds_;
};

class DeoptTranslationReader {
public:
    DeoptTranslationReader(const uint8_t *data, size_t size);

    bool IsValid() const
    {
        return streamStart_ != nullptr;
    }

    bool GetTranslation(DeoptId deoptId, DeoptTranslation *translation) const;

private:
    bool ReadOffset(uint32_t index, uint32_t *offset) const;
    bool GetTranslationRange(uint32_t index, const uint8_t **begin, const uint8_t **end) const;

    uint32_t translationCount_ {0};
    const uint8_t *offsetTable_ {nullptr};
    const uint8_t *streamStart_ {nullptr};
    const uint8_t *streamEnd_ {nullptr};
};

struct alignas(16) ArkSteedDeoptSnapshot {
#if defined(PANDA_TARGET_AMD64)
    static constexpr uint32_t GENERAL_SLOT_COUNT = 10;
    static constexpr uint32_t FLOATING_SLOT_COUNT = 15;
#elif defined(PANDA_TARGET_ARM64)
    static constexpr uint32_t GENERAL_SLOT_COUNT = 26;
    static constexpr uint32_t FLOATING_SLOT_COUNT = 30;
#endif

    uint64_t general[GENERAL_SLOT_COUNT];
    uint64_t floating[FLOATING_SLOT_COUNT];
};

#if defined(PANDA_TARGET_AMD64)
constexpr uint32_t ARKSTEED_DEOPT_GENERAL_REGISTER_CODE_COUNT = 16;
constexpr uint32_t ARKSTEED_DEOPT_FLOATING_REGISTER_CODE_COUNT = 16;
#elif defined(PANDA_TARGET_ARM64)
constexpr uint32_t ARKSTEED_DEOPT_GENERAL_REGISTER_CODE_COUNT = 32;
constexpr uint32_t ARKSTEED_DEOPT_FLOATING_REGISTER_CODE_COUNT = 32;
#endif

constexpr uint32_t ARKSTEED_DEOPT_SNAPSHOT_SIZE = sizeof(ArkSteedDeoptSnapshot);
constexpr uint32_t ARKSTEED_DEOPT_FLOATING_SNAPSHOT_OFFSET =
    static_cast<uint32_t>(sizeof(uint64_t) * ArkSteedDeoptSnapshot::GENERAL_SLOT_COUNT);

// The shared eager-deopt entry saves the complete ArkSteed allocation domain. Keep the persisted snapshot layout
// tied to the allocator lists so a newly allocatable register cannot silently escape deopt materialization or GC.
static_assert(GetAllocatableGeneralRegisters().Count() == ArkSteedDeoptSnapshot::GENERAL_SLOT_COUNT);
static_assert(GetAllocatableDoubleRegisters().Count() == ArkSteedDeoptSnapshot::FLOATING_SLOT_COUNT);

constexpr int32_t GetArkSteedDeoptGeneralSnapshotOffset(uint32_t code)
{
#if defined(PANDA_TARGET_AMD64)
    switch (code) {
        case 0:  // rax
            return 0;
        case 3:  // rbx
            return static_cast<int32_t>(sizeof(uint64_t));
        case 1:  // rcx
            return static_cast<int32_t>(2U * sizeof(uint64_t));
        case 2:  // rdx
            return static_cast<int32_t>(3U * sizeof(uint64_t));
        case 6:  // rsi
            return static_cast<int32_t>(4U * sizeof(uint64_t));
        case 7:  // rdi
            return static_cast<int32_t>(5U * sizeof(uint64_t));
        case 8:  // r8
            return static_cast<int32_t>(6U * sizeof(uint64_t));
        case 9:  // r9
            return static_cast<int32_t>(7U * sizeof(uint64_t));
        case 11:  // r11
            return static_cast<int32_t>(8U * sizeof(uint64_t));
        case 12:  // r12
            return static_cast<int32_t>(9U * sizeof(uint64_t));
        default:
            return -1;
    }
#elif defined(PANDA_TARGET_ARM64)
    if (code <= 15U) {
        return static_cast<int32_t>(code * sizeof(uint64_t));
    }
    if (code >= 19U && code <= 28U) {
        return static_cast<int32_t>((code - 3U) * sizeof(uint64_t));
    }
    return -1;
#endif
}

constexpr int32_t GetArkSteedDeoptFloatingSnapshotOffset(uint32_t code)
{
    if (code >= ArkSteedDeoptSnapshot::FLOATING_SLOT_COUNT) {
        return -1;
    }
    return static_cast<int32_t>(ARKSTEED_DEOPT_FLOATING_SNAPSHOT_OFFSET + code * sizeof(uint64_t));
}

constexpr bool AllocatableRegistersHaveDeoptSnapshotOffsets()
{
    uint64_t occupiedSlots = 0;
    uint64_t generalRegisters = GetAllocatableGeneralRegisters().Bits();
    for (uint32_t code = 0; code < ARKSTEED_DEOPT_GENERAL_REGISTER_CODE_COUNT; ++code) {
        if ((generalRegisters & (1ULL << code)) != 0) {
            int32_t offset = GetArkSteedDeoptGeneralSnapshotOffset(code);
            if (offset < 0 || offset % static_cast<int32_t>(sizeof(uint64_t)) != 0 ||
                static_cast<uint32_t>(offset) >= ARKSTEED_DEOPT_SNAPSHOT_SIZE) {
                return false;
            }
            uint32_t slot = static_cast<uint32_t>(offset) / sizeof(uint64_t);
            if ((occupiedSlots & (1ULL << slot)) != 0) {
                return false;
            }
            occupiedSlots |= 1ULL << slot;
        }
    }
    uint64_t floatingRegisters = GetAllocatableDoubleRegisters().Bits();
    for (uint32_t code = 0; code < ARKSTEED_DEOPT_FLOATING_REGISTER_CODE_COUNT; ++code) {
        if ((floatingRegisters & (1ULL << code)) != 0) {
            int32_t offset = GetArkSteedDeoptFloatingSnapshotOffset(code);
            if (offset < 0 || offset % static_cast<int32_t>(sizeof(uint64_t)) != 0 ||
                static_cast<uint32_t>(offset) >= ARKSTEED_DEOPT_SNAPSHOT_SIZE) {
                return false;
            }
            uint32_t slot = static_cast<uint32_t>(offset) / sizeof(uint64_t);
            if ((occupiedSlots & (1ULL << slot)) != 0) {
                return false;
            }
            occupiedSlots |= 1ULL << slot;
        }
    }
    return true;
}

static_assert(AllocatableRegistersHaveDeoptSnapshotOffsets());

#if defined(PANDA_TARGET_AMD64)
static_assert(ARKSTEED_DEOPT_SNAPSHOT_SIZE == 208U);
#elif defined(PANDA_TARGET_ARM64)
static_assert(ARKSTEED_DEOPT_SNAPSHOT_SIZE == 448U);
#endif
static_assert(alignof(ArkSteedDeoptSnapshot) == 16U);

inline uintptr_t GetArkSteedDeoptSnapshotFromCallsiteSp(uintptr_t callsiteSp)
{
    // DeoptHandlerAsm reports the optimized frame callsite SP after the call return address is skipped.
    return callsiteSp;
}

inline uint64_t ReadArkSteedDeoptGeneralRegister(uintptr_t snapshot, uint32_t code)
{
    int32_t offset = GetArkSteedDeoptGeneralSnapshotOffset(code);
    ASSERT(offset >= 0);
    return *reinterpret_cast<const uint64_t *>(snapshot + static_cast<uintptr_t>(offset));
}

inline uint64_t ReadArkSteedDeoptFloatingRegisterBits(uintptr_t snapshot, uint32_t code)
{
    int32_t offset = GetArkSteedDeoptFloatingSnapshotOffset(code);
    ASSERT(offset >= 0);
    return *reinterpret_cast<const uint64_t *>(snapshot + static_cast<uintptr_t>(offset));
}

int64_t GetFloat64RawBits(double value);
int64_t GetConstantSourceForDeoptTranslation(const ValueVertex *value, DeoptTranslationKind valueKind);
DeoptTranslationInput BuildDeoptTranslationInput(ArkSteedAssembler *assembler, const EagerDeoptimizableMixin *vertex,
                                                 uint32_t index);
std::vector<DeoptTranslationInput> BuildDeoptTranslationInputs(ArkSteedAssembler *assembler,
                                                               const EagerDeoptimizableMixin *vertex);
uint32_t GetTaggedDeoptSnapshotGeneralRegisters(const std::vector<DeoptTranslationInput> &inputs);

bool HandleArkSteedDeopt(JSThread *thread, DeoptId deoptId, JSTaggedType *result);

}  // namespace arksteed
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_ARKSTEED_DEOPT_HELPER_H
