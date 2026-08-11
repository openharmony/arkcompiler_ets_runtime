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

#include "ecmascript/arksteed/arksteed_deopt_abi.h"
#include "ecmascript/compiler/deopt_type.h"
#include "ecmascript/js_tagged_value_internals.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class JSThread;
namespace arksteed {
class ArkSteedAssembler;
class EagerDeoptimizableMixin;
class ValueVertex;

// These values describe the recovery semantics used by codegen and runtime materialization.
enum class DeoptTranslationKind : uint8_t {
    TAGGED = 0,
    INT32_TO_TAGGED = 1,
    FLOAT64_TO_TAGGED_DOUBLE = 2,
    RAW_INT32 = 3,
};

// Lazy-deopt safepoint offsets are FP-relative and 8-byte aligned, so the low 3 bits are
// free.  We encode DeoptTranslationKind there so the trampoline can reconstruct the correct
// JSTaggedValue without requiring upfront normalization nodes on the hot path.
constexpr int32_t LAZY_DEOPT_OFFSET_TAG_MASK = 0x7;
static_assert(static_cast<int32_t>(DeoptTranslationKind::RAW_INT32) <= LAZY_DEOPT_OFFSET_TAG_MASK);

inline int32_t EncodeLazyDeoptOffset(int32_t offset, DeoptTranslationKind valueKind)
{
    ASSERT((offset & LAZY_DEOPT_OFFSET_TAG_MASK) == 0);
    int32_t tag = static_cast<int32_t>(valueKind);
    return offset | tag;
}

inline DeoptTranslationKind DecodeLazyDeoptOffsetTag(int32_t offset)
{
    return static_cast<DeoptTranslationKind>(static_cast<uint8_t>(offset & LAZY_DEOPT_OFFSET_TAG_MASK));
}

inline int32_t StripLazyDeoptOffsetTag(int32_t offset)
{
    return offset & ~LAZY_DEOPT_OFFSET_TAG_MASK;
}

enum class DeoptSourceKind : uint8_t {
    CONSTANT = 0,
    STACK_SLOT = 1,
    GP_REGISTER = 2,
    FP_REGISTER = 3,
};

/*
 * Persisted deopt translation format:
 *
 *   uint32_t deoptCount
 *   uint32_t bodyCount
 *   Header headers[deoptCount]
 *     uint32_t bodyId
 *     uint8_t deoptType
 *   uint32_t bodyOffsets[bodyCount]
 *   uint8_t bodyOpcodeStream[]
 *
 * Fixed-width integers use little-endian byte order and are read with memcpy to permit unaligned data. A DeoptId
 * indexes a five-byte Header, whose bodyId indexes bodyOffsets. Each body offset is relative to the beginning of
 * bodyOpcodeStream and points to a BEGIN opcode.
 *
 * Unsigned operands use ULEB128:
 *   bytecodeOffset, inputCount, registerCode, specialKind.
 * Signed operands use SLEB128:
 *   vreg, frame-pointer-relative stackOffset and signed integer constants.
 * LEB128 operands use their shortest canonical byte sequence. Readers reject overflow and unterminated sequences.
 * specialKind values are 0:undefined, 1:null, 2:true, 3:false, 4:hole and 5:exception.
 * Float64 constants and tagged-double bit patterns store their raw 64-bit value in little-endian byte order.
 *
 * Opcode operands:
 *   BEGIN                    bytecodeOffset:ULEB, inputCount:ULEB
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
 *
 * Each body is self-contained: BEGIN is followed by exactly inputCount explicit input opcodes, one per logical vreg.
 * The decoder finishes after producing exactly inputCount logical inputs; no END opcode is stored. Raw translation
 * data may contain immediate tagged values, signed integers and floating-point bits, but never an address managed
 * by the moving GC.
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

struct DeoptTranslationHeader {
    uint32_t bodyId {0};
    kungfu::DeoptType type {kungfu::DeoptType::NONE};
};

struct DeoptTranslation {
    uint32_t bytecodeOffset {0};
    std::vector<DeoptTranslationInput> inputs;

    bool BodyEquals(const DeoptTranslation &other) const
    {
        return bytecodeOffset == other.bytecodeOffset && inputs == other.inputs;
    }
};

class DeoptTranslationBuilder {
public:
    DeoptId AddTranslation(uint32_t bytecodeOffset, kungfu::DeoptType type,
                           std::vector<DeoptTranslationInput> inputs);
    std::vector<uint8_t> Encode() const;

private:
    std::vector<DeoptTranslationHeader> headers_;
    std::vector<DeoptTranslation> bodies_;
    // A body hash selects candidates only. Full BodyEquals comparison decides whether a bodyId can be reused.
    std::unordered_multimap<uint64_t, uint32_t> bodyIndex_;
    std::unordered_map<uint64_t, DeoptId> headerIndex_;
};

class DeoptTranslationReader {
public:
    DeoptTranslationReader(const uint8_t *data, size_t size);

    bool IsValid() const
    {
        return streamStart_ != nullptr;
    }

    bool GetHeader(DeoptId deoptId, DeoptTranslationHeader *header) const;
    bool GetBody(uint32_t bodyId, DeoptTranslation *translation) const;

    uint32_t GetDeoptCount() const
    {
        return IsValid() ? deoptCount_ : 0U;
    }

private:
    bool ReadBodyOffset(uint32_t bodyId, uint32_t *offset) const;
    bool GetBodyRange(uint32_t bodyId, const uint8_t **begin, const uint8_t **end) const;

    uint32_t deoptCount_ {0};
    uint32_t bodyCount_ {0};
    const uint8_t *headerStart_ {nullptr};
    const uint8_t *offsetTable_ {nullptr};
    const uint8_t *streamStart_ {nullptr};
    const uint8_t *streamEnd_ {nullptr};
};

int64_t GetFloat64RawBits(double value);
int64_t GetConstantSourceForDeoptTranslation(const ValueVertex *value, DeoptTranslationKind valueKind);
DeoptTranslationInput BuildDeoptTranslationInput(ArkSteedAssembler *assembler, const EagerDeoptimizableMixin *vertex,
                                                 uint32_t index);
std::vector<DeoptTranslationInput> BuildDeoptTranslationInputs(ArkSteedAssembler *assembler,
                                                               const EagerDeoptimizableMixin *vertex);

bool WouldStackOverflow(JSThread *thread, const JSTaggedType *sp);
bool HandleArkSteedDeoptNoGC(JSThread *thread, uintptr_t returnPc, uintptr_t inputFp,
                             uintptr_t snapshot, JSTaggedType *result);
}  // namespace arksteed
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_ARKSTEED_DEOPT_HELPER_H
