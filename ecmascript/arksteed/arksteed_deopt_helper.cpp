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

#include "ecmascript/arksteed/arksteed_deopt_helper.h"

#include <algorithm>
#include <cstring>
#include <limits>

#include "ecmascript/arksteed/arksteed_assembler.h"
#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/base/hash_combine.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/machine_code.h"

namespace panda::ecmascript::arksteed {
namespace {
constexpr uint8_t LEB128_PAYLOAD_MASK = 0x7FU;
constexpr uint8_t LEB128_CONTINUATION_BIT = 0x80U;
constexpr uint8_t LEB128_SIGN_BIT = 0x40U;
constexpr uint32_t LEB128_PAYLOAD_BITS = 7U;
constexpr uint32_t LEB128_MAX_SHIFT = 63U;
constexpr size_t MIN_EXPLICIT_INPUT_SIZE = 3U;  // opcode, vreg and source
constexpr size_t DEOPT_TRANSLATION_HEADER_SIZE = sizeof(uint32_t) + sizeof(uint8_t);
constexpr uint8_t FIRST_SHORT_MATCH_OPCODE = static_cast<uint8_t>(DeoptTranslationOpcode::COUNT);
constexpr uint32_t MAX_SHORT_MATCH_COUNT = UINT8_MAX - FIRST_SHORT_MATCH_OPCODE + 1U;
constexpr int64_t ARKSTEED_SUPPORTED_INLINE_DEPTH = 0;

enum class DeoptTranslationSpecialKind : uint8_t {
    UNDEFINED = 0,
    NULL_VALUE,
    TRUE_VALUE,
    FALSE_VALUE,
    HOLE,
    EXCEPTION,
};

static_assert(FIRST_SHORT_MATCH_OPCODE <= UINT8_MAX);
static_assert(MAX_SHORT_MATCH_COUNT > 0);
static_assert(sizeof(kungfu::DeoptType) == sizeof(uint8_t));

void WriteUint32LE(uint32_t value, std::vector<uint8_t> &output)
{
    for (size_t index = 0; index < sizeof(value); ++index) {
        output.push_back(static_cast<uint8_t>(value >> (index * 8U)));
    }
}

void WriteUint64LE(uint64_t value, std::vector<uint8_t> &output)
{
    for (size_t index = 0; index < sizeof(value); ++index) {
        output.push_back(static_cast<uint8_t>(value >> (index * 8U)));
    }
}

bool ReadUint32LE(const uint8_t *&cursor, const uint8_t *end, uint32_t &value)
{
    if (cursor == nullptr || end == nullptr || cursor > end || static_cast<size_t>(end - cursor) < sizeof(uint32_t)) {
        return false;
    }

    uint8_t bytes[sizeof(uint32_t)] {};
    std::memcpy(bytes, cursor, sizeof(bytes));
    uint32_t result = 0;
    for (size_t index = 0; index < sizeof(bytes); ++index) {
        result |= static_cast<uint32_t>(bytes[index]) << (index * 8U);
    }
    cursor += sizeof(bytes);
    value = result;
    return true;
}

bool ReadUint64LE(const uint8_t *&cursor, const uint8_t *end, uint64_t &value)
{
    if (cursor == nullptr || end == nullptr || cursor > end || static_cast<size_t>(end - cursor) < sizeof(uint64_t)) {
        return false;
    }

    uint8_t bytes[sizeof(uint64_t)] {};
    std::memcpy(bytes, cursor, sizeof(bytes));
    uint64_t result = 0;
    for (size_t index = 0; index < sizeof(bytes); ++index) {
        result |= static_cast<uint64_t>(bytes[index]) << (index * 8U);
    }
    cursor += sizeof(bytes);
    value = result;
    return true;
}

void WriteULEB128(uint64_t value, std::vector<uint8_t> &output)
{
    do {
        uint8_t byte = static_cast<uint8_t>(value & LEB128_PAYLOAD_MASK);
        value >>= LEB128_PAYLOAD_BITS;
        if (value != 0) {
            byte |= LEB128_CONTINUATION_BIT;
        }
        output.push_back(byte);
    } while (value != 0);
}

void WriteSLEB128(int64_t value, std::vector<uint8_t> &output)
{
    uint64_t remaining = static_cast<uint64_t>(value);
    bool isNegative = value < 0;
    bool hasMore = true;
    while (hasMore) {
        uint8_t byte = static_cast<uint8_t>(remaining & LEB128_PAYLOAD_MASK);
        bool signBitSet = (byte & LEB128_SIGN_BIT) != 0;
        remaining >>= LEB128_PAYLOAD_BITS;
        if (isNegative) {
            remaining |= UINT64_MAX << (sizeof(remaining) * 8U - LEB128_PAYLOAD_BITS);
        }
        hasMore = !((remaining == 0 && !signBitSet) || (remaining == UINT64_MAX && signBitSet));
        if (hasMore) {
            byte |= LEB128_CONTINUATION_BIT;
        }
        output.push_back(byte);
    }
}

bool ReadULEB128(const uint8_t *&cursor, const uint8_t *end, uint64_t &value)
{
    if (cursor == nullptr || end == nullptr || cursor > end) {
        return false;
    }

    const uint8_t *current = cursor;
    uint64_t result = 0;
    uint32_t shift = 0;
    uint32_t byteCount = 0;
    while (current < end) {
        uint8_t byte = *current++;
        uint8_t payload = byte & LEB128_PAYLOAD_MASK;
        bool hasMore = (byte & LEB128_CONTINUATION_BIT) != 0;
        if (shift == LEB128_MAX_SHIFT && (payload > 1U || hasMore)) {
            return false;
        }

        result |= static_cast<uint64_t>(payload) << shift;
        ++byteCount;
        if (!hasMore) {
            if (byteCount > 1U && payload == 0) {
                return false;
            }
            cursor = current;
            value = result;
            return true;
        }
        shift += LEB128_PAYLOAD_BITS;
    }
    return false;
}

bool ReadSLEB128(const uint8_t *&cursor, const uint8_t *end, int64_t &value)
{
    if (cursor == nullptr || end == nullptr || cursor > end) {
        return false;
    }

    const uint8_t *current = cursor;
    uint64_t result = 0;
    uint32_t shift = 0;
    uint32_t byteCount = 0;
    uint8_t previousByte = 0;
    while (current < end) {
        uint8_t byte = *current++;
        uint8_t payload = byte & LEB128_PAYLOAD_MASK;
        bool hasMore = (byte & LEB128_CONTINUATION_BIT) != 0;
        if (shift == LEB128_MAX_SHIFT && (hasMore || (payload != 0 && payload != LEB128_PAYLOAD_MASK))) {
            return false;
        }

        result |= static_cast<uint64_t>(payload) << shift;
        ++byteCount;
        if (!hasMore) {
            bool previousSignBitSet = (previousByte & LEB128_SIGN_BIT) != 0;
            if (byteCount > 1U &&
                ((payload == 0 && !previousSignBitSet) || (payload == LEB128_PAYLOAD_MASK && previousSignBitSet))) {
                return false;
            }

            uint32_t nextShift = shift + LEB128_PAYLOAD_BITS;
            if ((byte & LEB128_SIGN_BIT) != 0 && nextShift < LEB128_MAX_SHIFT + 1U) {
                result |= UINT64_MAX << nextShift;
            }
            int64_t signedResult = 0;
            std::memcpy(&signedResult, &result, sizeof(signedResult));
            cursor = current;
            value = signedResult;
            return true;
        }
        previousByte = byte;
        shift += LEB128_PAYLOAD_BITS;
    }
    return false;
}

uint64_t HashTranslationBody(const DeoptTranslation &translation)
{
    uint64_t hash = base::HashCombiner::HashCombine(0, translation.bytecodeOffset);
    hash = base::HashCombiner::HashCombine(hash, translation.inputs.size());
    for (const auto &input : translation.inputs) {
        hash = base::HashCombiner::HashCombine(hash, static_cast<uint64_t>(input.vreg));
        hash = base::HashCombiner::HashCombine(hash, static_cast<uint64_t>(input.valueKind));
        hash = base::HashCombiner::HashCombine(hash, static_cast<uint64_t>(input.sourceKind));
        hash = base::HashCombiner::HashCombine(hash, static_cast<uint64_t>(input.source));
    }
    return hash;
}

uint64_t GetTranslationHeaderKey(uint32_t bodyId, kungfu::DeoptType type)
{
    return (static_cast<uint64_t>(bodyId) << 8U) | static_cast<uint8_t>(type);
}

bool IsValidDeoptType(uint8_t type)
{
    return type >= static_cast<uint8_t>(kungfu::DeoptType::LAZYDEOPT) &&
           type <= static_cast<uint8_t>(kungfu::DeoptType::HOTRELOAD_PATCHMAIN);
}

bool IsTaggedSpecial(const JSTaggedValue &value)
{
    return value.IsUndefined() || value.IsNull() || value.IsTrue() || value.IsFalse() || value.IsHole() ||
           value.IsException();
}

DeoptTranslationSpecialKind GetTaggedSpecialKind(const JSTaggedValue &value)
{
    if (value.IsUndefined()) {
        return DeoptTranslationSpecialKind::UNDEFINED;
    }
    if (value.IsNull()) {
        return DeoptTranslationSpecialKind::NULL_VALUE;
    }
    if (value.IsTrue()) {
        return DeoptTranslationSpecialKind::TRUE_VALUE;
    }
    if (value.IsFalse()) {
        return DeoptTranslationSpecialKind::FALSE_VALUE;
    }
    if (value.IsHole()) {
        return DeoptTranslationSpecialKind::HOLE;
    }
    CHECK(value.IsException());
    return DeoptTranslationSpecialKind::EXCEPTION;
}

void ValidateTaggedConstant(int64_t source)
{
    JSTaggedValue value(static_cast<JSTaggedType>(source));
    if (IsTaggedSpecial(value)) {
        return;
    }
    CHECK(!value.IsHeapObject());
    CHECK(value.IsInt() || value.IsDouble());
}

void ValidateTranslationInput(const DeoptTranslationInput &input)
{
    CHECK(static_cast<uint8_t>(input.valueKind) <= static_cast<uint8_t>(DeoptTranslationKind::RAW_INT32));
    CHECK(static_cast<uint8_t>(input.sourceKind) <= static_cast<uint8_t>(DeoptSourceKind::FP_REGISTER));
    if (input.sourceKind == DeoptSourceKind::GP_REGISTER) {
        CHECK(input.source >= 0);
        CHECK(static_cast<uint64_t>(input.source) <= std::numeric_limits<uint32_t>::max());
        CHECK(GetArkSteedDeoptGeneralSnapshotOffset(static_cast<uint32_t>(input.source)) >= 0);
    }
    if (input.sourceKind == DeoptSourceKind::FP_REGISTER) {
        CHECK(input.source >= 0);
        CHECK(static_cast<uint64_t>(input.source) <= std::numeric_limits<uint32_t>::max());
        CHECK(GetArkSteedDeoptFloatingSnapshotOffset(static_cast<uint32_t>(input.source)) >= 0);
    }
    if (input.sourceKind == DeoptSourceKind::STACK_SLOT) {
        CHECK(input.source >= std::numeric_limits<int32_t>::min());
        CHECK(input.source <= std::numeric_limits<int32_t>::max());
    }
    if (input.valueKind == DeoptTranslationKind::TAGGED) {
        CHECK(input.sourceKind != DeoptSourceKind::FP_REGISTER);
        if (input.sourceKind == DeoptSourceKind::CONSTANT) {
            ValidateTaggedConstant(input.source);
        }
        return;
    }
    if (input.valueKind == DeoptTranslationKind::FLOAT64_TO_TAGGED_DOUBLE) {
        CHECK(input.sourceKind != DeoptSourceKind::GP_REGISTER);
        return;
    }
    CHECK(input.valueKind == DeoptTranslationKind::INT32_TO_TAGGED ||
          input.valueKind == DeoptTranslationKind::RAW_INT32);
    CHECK(input.sourceKind != DeoptSourceKind::FP_REGISTER);
    if (input.sourceKind == DeoptSourceKind::CONSTANT) {
        CHECK(input.source >= std::numeric_limits<int32_t>::min());
        CHECK(input.source <= std::numeric_limits<int32_t>::max());
    }
}

void WriteOpcode(DeoptTranslationOpcode opcode, std::vector<uint8_t> &output)
{
    output.push_back(static_cast<uint8_t>(opcode));
}

void WriteSignedInput(DeoptTranslationOpcode opcode, const DeoptTranslationInput &input,
                      std::vector<uint8_t> &output)
{
    WriteOpcode(opcode, output);
    WriteSLEB128(input.vreg, output);
    WriteSLEB128(input.source, output);
}

void WriteRegisterInput(DeoptTranslationOpcode opcode, const DeoptTranslationInput &input,
                        std::vector<uint8_t> &output)
{
    CHECK(input.source >= 0);
    WriteOpcode(opcode, output);
    WriteSLEB128(input.vreg, output);
    WriteULEB128(static_cast<uint64_t>(input.source), output);
}

void EncodeTaggedConstant(const DeoptTranslationInput &input, std::vector<uint8_t> &output)
{
    JSTaggedValue value(static_cast<JSTaggedType>(input.source));
    if (IsTaggedSpecial(value)) {
        WriteOpcode(DeoptTranslationOpcode::TAGGED_SPECIAL, output);
        WriteSLEB128(input.vreg, output);
        WriteULEB128(static_cast<uint8_t>(GetTaggedSpecialKind(value)), output);
        return;
    }
    CHECK(!value.IsHeapObject());
    if (value.IsInt()) {
        WriteOpcode(DeoptTranslationOpcode::TAGGED_INT_CONSTANT, output);
        WriteSLEB128(input.vreg, output);
        WriteSLEB128(value.GetInt(), output);
        return;
    }
    CHECK(value.IsDouble());
    WriteOpcode(DeoptTranslationOpcode::TAGGED_DOUBLE_BITS, output);
    WriteSLEB128(input.vreg, output);
    WriteUint64LE(value.GetRawData(), output);
}

void EncodeExplicitInput(const DeoptTranslationInput &input, std::vector<uint8_t> &output)
{
    switch (input.valueKind) {
        case DeoptTranslationKind::TAGGED:
            if (input.sourceKind == DeoptSourceKind::CONSTANT) {
                EncodeTaggedConstant(input, output);
            } else if (input.sourceKind == DeoptSourceKind::STACK_SLOT) {
                WriteSignedInput(DeoptTranslationOpcode::TAGGED_STACK_SLOT, input, output);
            } else {
                CHECK(input.sourceKind == DeoptSourceKind::GP_REGISTER);
                WriteRegisterInput(DeoptTranslationOpcode::TAGGED_REGISTER, input, output);
            }
            return;
        case DeoptTranslationKind::INT32_TO_TAGGED:
            if (input.sourceKind == DeoptSourceKind::CONSTANT) {
                WriteSignedInput(DeoptTranslationOpcode::INT32_CONSTANT, input, output);
            } else if (input.sourceKind == DeoptSourceKind::STACK_SLOT) {
                WriteSignedInput(DeoptTranslationOpcode::INT32_STACK_SLOT, input, output);
            } else {
                CHECK(input.sourceKind == DeoptSourceKind::GP_REGISTER);
                WriteRegisterInput(DeoptTranslationOpcode::INT32_REGISTER, input, output);
            }
            return;
        case DeoptTranslationKind::FLOAT64_TO_TAGGED_DOUBLE:
            if (input.sourceKind == DeoptSourceKind::CONSTANT) {
                WriteOpcode(DeoptTranslationOpcode::FLOAT64_CONSTANT, output);
                WriteSLEB128(input.vreg, output);
                WriteUint64LE(static_cast<uint64_t>(input.source), output);
            } else if (input.sourceKind == DeoptSourceKind::STACK_SLOT) {
                WriteSignedInput(DeoptTranslationOpcode::FLOAT64_STACK_SLOT, input, output);
            } else {
                CHECK(input.sourceKind == DeoptSourceKind::FP_REGISTER);
                WriteRegisterInput(DeoptTranslationOpcode::FLOAT64_REGISTER, input, output);
            }
            return;
        case DeoptTranslationKind::RAW_INT32:
            if (input.sourceKind == DeoptSourceKind::CONSTANT) {
                WriteSignedInput(DeoptTranslationOpcode::RAW_INT32_CONSTANT, input, output);
            } else if (input.sourceKind == DeoptSourceKind::STACK_SLOT) {
                WriteSignedInput(DeoptTranslationOpcode::RAW_INT32_STACK_SLOT, input, output);
            } else {
                CHECK(input.sourceKind == DeoptSourceKind::GP_REGISTER);
                WriteRegisterInput(DeoptTranslationOpcode::RAW_INT32_REGISTER, input, output);
            }
            return;
    }
    UNREACHABLE();
}

void EncodeBasisMatch(size_t count, std::vector<uint8_t> &output)
{
    CHECK(count > 0);
    if (count <= MAX_SHORT_MATCH_COUNT) {
        output.push_back(static_cast<uint8_t>(FIRST_SHORT_MATCH_OPCODE + count - 1U));
        return;
    }
    WriteOpcode(DeoptTranslationOpcode::MATCH_BASIS, output);
    WriteULEB128(count, output);
}

void EncodeDeltaInputs(const DeoptTranslation &translation, const DeoptTranslation &basis,
                       std::vector<uint8_t> &output)
{
    CHECK(translation.inputs.size() == basis.inputs.size());
    size_t matchCount = 0;
    for (size_t index = 0; index < translation.inputs.size(); ++index) {
        CHECK(translation.inputs[index].vreg == basis.inputs[index].vreg);
        if (translation.inputs[index] == basis.inputs[index]) {
            ++matchCount;
            continue;
        }
        if (matchCount != 0) {
            EncodeBasisMatch(matchCount, output);
            matchCount = 0;
        }
        EncodeExplicitInput(translation.inputs[index], output);
    }
    if (matchCount != 0) {
        EncodeBasisMatch(matchCount, output);
    }
}

struct DecodedBodyHeader {
    uint32_t basisBodyIdDistance {0};
    uint32_t bytecodeOffset {0};
    uint32_t inputCount {0};
    const uint8_t *inputStart {nullptr};
};

int64_t Uint64ToInt64Bits(uint64_t bits)
{
    int64_t value = 0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

bool ReadUint32ULEB(const uint8_t *&cursor, const uint8_t *end, uint32_t *value)
{
    uint64_t decoded = 0;
    if (!ReadULEB128(cursor, end, decoded) || decoded > std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    *value = static_cast<uint32_t>(decoded);
    return true;
}

bool ReadInt32SLEB(const uint8_t *&cursor, const uint8_t *end, int32_t *value)
{
    int64_t decoded = 0;
    if (!ReadSLEB128(cursor, end, decoded) || decoded < std::numeric_limits<int32_t>::min() ||
        decoded > std::numeric_limits<int32_t>::max()) {
        return false;
    }
    *value = static_cast<int32_t>(decoded);
    return true;
}

bool DecodeBodyHeader(const uint8_t *begin, const uint8_t *end, DecodedBodyHeader *header)
{
    if (begin == nullptr || end == nullptr || header == nullptr || begin >= end ||
        *begin != static_cast<uint8_t>(DeoptTranslationOpcode::BEGIN)) {
        return false;
    }
    const uint8_t *cursor = begin + 1;
    if (!ReadUint32ULEB(cursor, end, &header->basisBodyIdDistance) ||
        !ReadUint32ULEB(cursor, end, &header->bytecodeOffset) ||
        !ReadUint32ULEB(cursor, end, &header->inputCount)) {
        return false;
    }
    header->inputStart = cursor;
    return true;
}

bool DecodeSpecialConstant(uint32_t specialKind, int64_t *source)
{
    JSTaggedValue value;
    switch (static_cast<DeoptTranslationSpecialKind>(specialKind)) {
        case DeoptTranslationSpecialKind::UNDEFINED:
            value = JSTaggedValue::Undefined();
            break;
        case DeoptTranslationSpecialKind::NULL_VALUE:
            value = JSTaggedValue::Null();
            break;
        case DeoptTranslationSpecialKind::TRUE_VALUE:
            value = JSTaggedValue::True();
            break;
        case DeoptTranslationSpecialKind::FALSE_VALUE:
            value = JSTaggedValue::False();
            break;
        case DeoptTranslationSpecialKind::HOLE:
            value = JSTaggedValue::Hole();
            break;
        case DeoptTranslationSpecialKind::EXCEPTION:
            value = JSTaggedValue::Exception();
            break;
        default:
            return false;
    }
    *source = Uint64ToInt64Bits(value.GetRawData());
    return true;
}

bool DecodeRegisterSource(const uint8_t *&cursor, const uint8_t *end, DeoptSourceKind sourceKind, int64_t *source)
{
    uint32_t registerCode = 0;
    if (!ReadUint32ULEB(cursor, end, &registerCode)) {
        return false;
    }
    if (sourceKind == DeoptSourceKind::GP_REGISTER && GetArkSteedDeoptGeneralSnapshotOffset(registerCode) < 0) {
        return false;
    }
    if (sourceKind == DeoptSourceKind::FP_REGISTER &&
        GetArkSteedDeoptFloatingSnapshotOffset(registerCode) < 0) {
        return false;
    }
    *source = registerCode;
    return true;
}

bool DecodeSignedSource(const uint8_t *&cursor, const uint8_t *end, int64_t *source)
{
    int32_t value = 0;
    if (!ReadInt32SLEB(cursor, end, &value)) {
        return false;
    }
    *source = value;
    return true;
}

bool DecodeTaggedConstant(DeoptTranslationOpcode opcode, const uint8_t *&cursor, const uint8_t *end, int64_t *source)
{
    if (opcode == DeoptTranslationOpcode::TAGGED_SPECIAL) {
        uint32_t specialKind = 0;
        return ReadUint32ULEB(cursor, end, &specialKind) && DecodeSpecialConstant(specialKind, source);
    }
    if (opcode == DeoptTranslationOpcode::TAGGED_INT_CONSTANT) {
        int32_t value = 0;
        if (!ReadInt32SLEB(cursor, end, &value)) {
            return false;
        }
        *source = Uint64ToInt64Bits(JSTaggedValue(value).GetRawData());
        return true;
    }
    uint64_t bits = 0;
    if (opcode != DeoptTranslationOpcode::TAGGED_DOUBLE_BITS || !ReadUint64LE(cursor, end, bits) ||
        !JSTaggedValue(bits).IsDouble()) {
        return false;
    }
    *source = Uint64ToInt64Bits(bits);
    return true;
}

bool DecodeExplicitInput(uint8_t encodedOpcode, const uint8_t *&cursor, const uint8_t *end,
                         DeoptTranslationInput *input)
{
    if (encodedOpcode == static_cast<uint8_t>(DeoptTranslationOpcode::BEGIN) ||
        encodedOpcode >= static_cast<uint8_t>(DeoptTranslationOpcode::COUNT) ||
        encodedOpcode == static_cast<uint8_t>(DeoptTranslationOpcode::MATCH_BASIS) || input == nullptr ||
        !ReadInt32SLEB(cursor, end, &input->vreg)) {
        return false;
    }
    auto opcode = static_cast<DeoptTranslationOpcode>(encodedOpcode);
    if (opcode >= DeoptTranslationOpcode::TAGGED_REGISTER &&
        opcode <= DeoptTranslationOpcode::TAGGED_STACK_SLOT) {
        input->valueKind = DeoptTranslationKind::TAGGED;
        input->sourceKind = opcode == DeoptTranslationOpcode::TAGGED_REGISTER ? DeoptSourceKind::GP_REGISTER
                                                                              : DeoptSourceKind::STACK_SLOT;
    } else if (opcode >= DeoptTranslationOpcode::TAGGED_SPECIAL &&
               opcode <= DeoptTranslationOpcode::TAGGED_DOUBLE_BITS) {
        input->valueKind = DeoptTranslationKind::TAGGED;
        input->sourceKind = DeoptSourceKind::CONSTANT;
        return DecodeTaggedConstant(opcode, cursor, end, &input->source);
    } else if (opcode >= DeoptTranslationOpcode::INT32_REGISTER &&
               opcode <= DeoptTranslationOpcode::INT32_CONSTANT) {
        input->valueKind = DeoptTranslationKind::INT32_TO_TAGGED;
        input->sourceKind = opcode == DeoptTranslationOpcode::INT32_REGISTER ? DeoptSourceKind::GP_REGISTER
                           : opcode == DeoptTranslationOpcode::INT32_STACK_SLOT ? DeoptSourceKind::STACK_SLOT
                                                                                : DeoptSourceKind::CONSTANT;
    } else if (opcode >= DeoptTranslationOpcode::FLOAT64_REGISTER &&
               opcode <= DeoptTranslationOpcode::FLOAT64_CONSTANT) {
        input->valueKind = DeoptTranslationKind::FLOAT64_TO_TAGGED_DOUBLE;
        input->sourceKind = opcode == DeoptTranslationOpcode::FLOAT64_REGISTER ? DeoptSourceKind::FP_REGISTER
                           : opcode == DeoptTranslationOpcode::FLOAT64_STACK_SLOT ? DeoptSourceKind::STACK_SLOT
                                                                                  : DeoptSourceKind::CONSTANT;
    } else if (opcode >= DeoptTranslationOpcode::RAW_INT32_REGISTER &&
               opcode <= DeoptTranslationOpcode::RAW_INT32_CONSTANT) {
        input->valueKind = DeoptTranslationKind::RAW_INT32;
        input->sourceKind = opcode == DeoptTranslationOpcode::RAW_INT32_REGISTER ? DeoptSourceKind::GP_REGISTER
                           : opcode == DeoptTranslationOpcode::RAW_INT32_STACK_SLOT ? DeoptSourceKind::STACK_SLOT
                                                                                    : DeoptSourceKind::CONSTANT;
    } else {
        return false;
    }

    if (input->sourceKind == DeoptSourceKind::GP_REGISTER || input->sourceKind == DeoptSourceKind::FP_REGISTER) {
        return DecodeRegisterSource(cursor, end, input->sourceKind, &input->source);
    }
    if (input->valueKind == DeoptTranslationKind::FLOAT64_TO_TAGGED_DOUBLE &&
        input->sourceKind == DeoptSourceKind::CONSTANT) {
        uint64_t bits = 0;
        if (!ReadUint64LE(cursor, end, bits)) {
            return false;
        }
        input->source = Uint64ToInt64Bits(bits);
        return true;
    }
    return DecodeSignedSource(cursor, end, &input->source);
}

bool DecodeBasisInputs(const DecodedBodyHeader &header, const uint8_t *end,
                       std::vector<DeoptTranslationInput> *inputs)
{
    if (header.inputStart > end || header.inputCount > inputs->max_size() ||
        header.inputCount > static_cast<size_t>(end - header.inputStart) / MIN_EXPLICIT_INPUT_SIZE) {
        return false;
    }
    const uint8_t *cursor = header.inputStart;
    inputs->clear();
    inputs->reserve(header.inputCount);
    for (uint32_t index = 0; index < header.inputCount; ++index) {
        if (cursor >= end) {
            return false;
        }
        uint8_t opcode = *cursor++;
        DeoptTranslationInput input {};
        if (!DecodeExplicitInput(opcode, cursor, end, &input) ||
            (!inputs->empty() && inputs->back().vreg >= input.vreg)) {
            return false;
        }
        inputs->push_back(input);
    }
    return cursor == end;
}

bool DecodeMatchCount(uint8_t opcode, const uint8_t *&cursor, const uint8_t *end, uint32_t *count)
{
    if (opcode >= static_cast<uint8_t>(DeoptTranslationOpcode::COUNT)) {
        *count = static_cast<uint32_t>(opcode - static_cast<uint8_t>(DeoptTranslationOpcode::COUNT)) + 1U;
        return true;
    }
    return opcode == static_cast<uint8_t>(DeoptTranslationOpcode::MATCH_BASIS) &&
           ReadUint32ULEB(cursor, end, count) && *count > 0;
}

bool ApplyDeltaInputs(const DecodedBodyHeader &header, const uint8_t *end,
                      std::vector<DeoptTranslationInput> *inputs)
{
    if (inputs == nullptr || header.inputCount != inputs->size()) {
        return false;
    }
    const uint8_t *cursor = header.inputStart;
    size_t position = 0;
    while (position < header.inputCount) {
        if (cursor >= end) {
            return false;
        }
        uint8_t opcode = *cursor++;
        uint32_t matchCount = 0;
        if (DecodeMatchCount(opcode, cursor, end, &matchCount)) {
            if (matchCount > inputs->size() - position) {
                return false;
            }
            position += matchCount;
            continue;
        }
        DeoptTranslationInput input {};
        if (!DecodeExplicitInput(opcode, cursor, end, &input) || input.vreg != (*inputs)[position].vreg) {
            return false;
        }
        (*inputs)[position++] = input;
    }
    return cursor == end;
}
}  // namespace

DeoptId DeoptTranslationBuilder::AddTranslation(
    uint32_t bytecodeOffset, kungfu::DeoptType type, std::vector<DeoptTranslationInput> inputs)
{
    CHECK(headers_.size() < std::numeric_limits<uint32_t>::max());
    CHECK(bodies_.size() < std::numeric_limits<uint32_t>::max());
    CHECK(inputs.size() <= std::numeric_limits<uint32_t>::max());
    CHECK(IsValidDeoptType(static_cast<uint8_t>(type)));
    for (size_t index = 0; index < inputs.size(); ++index) {
        ValidateTranslationInput(inputs[index]);
        CHECK(index == 0 || inputs[index - 1U].vreg < inputs[index].vreg);
    }

    auto basisIterator = basisBodyIds_.find(bytecodeOffset);
    if (basisIterator != basisBodyIds_.end()) {
        CHECK(basisIterator->second < bodies_.size());
        const auto &basisInputs = bodies_[basisIterator->second].inputs;
        CHECK(inputs.size() == basisInputs.size());
        for (size_t index = 0; index < inputs.size(); ++index) {
            CHECK(inputs[index].vreg == basisInputs[index].vreg);
        }
    }

    DeoptTranslation candidate {
        bytecodeOffset,
        std::move(inputs),
    };
    uint64_t bodyHash = HashTranslationBody(candidate);
    uint32_t bodyId = 0;
    bool foundBody = false;
    auto [begin, end] = bodyIndex_.equal_range(bodyHash);
    for (auto iterator = begin; iterator != end; ++iterator) {
        CHECK(iterator->second < bodies_.size());
        if (bodies_[iterator->second].BodyEquals(candidate)) {
            bodyId = iterator->second;
            foundBody = true;
            break;
        }
    }

    if (!foundBody) {
        bodyId = static_cast<uint32_t>(bodies_.size());
        bodies_.push_back(std::move(candidate));
        bodyIndex_.emplace(bodyHash, bodyId);
        basisBodyIds_.try_emplace(bytecodeOffset, bodyId);
    }

    uint64_t headerKey = GetTranslationHeaderKey(bodyId, type);
    auto headerIterator = headerIndex_.find(headerKey);
    if (headerIterator != headerIndex_.end()) {
        return headerIterator->second;
    }

    DeoptId deoptId {static_cast<uint32_t>(headers_.size())};
    headers_.push_back({bodyId, type});
    headerIndex_.emplace(headerKey, deoptId);
    return deoptId;
}

std::vector<uint8_t> DeoptTranslationBuilder::Encode() const
{
    if (headers_.empty()) {
        CHECK(bodies_.empty());
        return {};
    }

    CHECK(!bodies_.empty());
    CHECK(bodies_.size() <= headers_.size());
    CHECK(headers_.size() <= std::numeric_limits<uint32_t>::max());
    CHECK(bodies_.size() <= std::numeric_limits<uint32_t>::max());
    std::vector<uint32_t> offsets;
    std::vector<uint8_t> stream;
    offsets.reserve(bodies_.size());
    for (size_t index = 0; index < bodies_.size(); ++index) {
        const auto &translation = bodies_[index];
        CHECK(stream.size() <= std::numeric_limits<uint32_t>::max());
        offsets.push_back(static_cast<uint32_t>(stream.size()));

        auto basisIterator = basisBodyIds_.find(translation.bytecodeOffset);
        CHECK(basisIterator != basisBodyIds_.end());
        uint32_t basisBodyId = basisIterator->second;
        CHECK(basisBodyId <= index);
        CHECK(basisBodyId < offsets.size());
        uint32_t basisBodyIdDistance = static_cast<uint32_t>(index) - basisBodyId;

        WriteOpcode(DeoptTranslationOpcode::BEGIN, stream);
        WriteULEB128(basisBodyIdDistance, stream);
        WriteULEB128(translation.bytecodeOffset, stream);
        WriteULEB128(translation.inputs.size(), stream);
        if (basisBodyIdDistance == 0) {
            for (const auto &input : translation.inputs) {
                EncodeExplicitInput(input, stream);
            }
        } else {
            EncodeDeltaInputs(translation, bodies_[basisBodyId], stream);
        }
    }
    CHECK(stream.size() <= std::numeric_limits<uint32_t>::max());

    size_t fixedSize = sizeof(uint32_t) * 2U;
    CHECK(headers_.size() <=
          (std::numeric_limits<size_t>::max() - fixedSize) / DEOPT_TRANSLATION_HEADER_SIZE);
    fixedSize += headers_.size() * DEOPT_TRANSLATION_HEADER_SIZE;
    CHECK(offsets.size() <= (std::numeric_limits<size_t>::max() - fixedSize) / sizeof(uint32_t));
    fixedSize += offsets.size() * sizeof(uint32_t);
    CHECK(stream.size() <= std::numeric_limits<size_t>::max() - fixedSize);

    std::vector<uint8_t> output;
    output.reserve(fixedSize + stream.size());
    WriteUint32LE(static_cast<uint32_t>(headers_.size()), output);
    WriteUint32LE(static_cast<uint32_t>(bodies_.size()), output);
    for (const auto &header : headers_) {
        CHECK(header.bodyId < bodies_.size());
        CHECK(IsValidDeoptType(static_cast<uint8_t>(header.type)));
        WriteUint32LE(header.bodyId, output);
        output.push_back(static_cast<uint8_t>(header.type));
    }
    for (uint32_t offset : offsets) {
        WriteUint32LE(offset, output);
    }
    output.insert(output.end(), stream.begin(), stream.end());
    return output;
}

DeoptTranslationReader::DeoptTranslationReader(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(uint32_t) * 2U) {
        return;
    }
    uintptr_t dataAddress = reinterpret_cast<uintptr_t>(data);
    if (size > std::numeric_limits<uintptr_t>::max() - dataAddress) {
        return;
    }
    const uint8_t *cursor = data;
    const uint8_t *end = reinterpret_cast<const uint8_t *>(dataAddress + size);
    uint32_t deoptCount = 0;
    uint32_t bodyCount = 0;
    if (!ReadUint32LE(cursor, end, deoptCount) || !ReadUint32LE(cursor, end, bodyCount) ||
        deoptCount == 0 || bodyCount == 0 || bodyCount > deoptCount ||
        deoptCount > static_cast<size_t>(end - cursor) / DEOPT_TRANSLATION_HEADER_SIZE) {
        return;
    }
    const uint8_t *headerStart = cursor;
    size_t headerSize = static_cast<size_t>(deoptCount) * DEOPT_TRANSLATION_HEADER_SIZE;
    cursor += headerSize;
    if (bodyCount > static_cast<size_t>(end - cursor) / sizeof(uint32_t)) {
        return;
    }
    const uint8_t *offsetTable = cursor;
    size_t tableSize = static_cast<size_t>(bodyCount) * sizeof(uint32_t);
    const uint8_t *streamStart = offsetTable + tableSize;
    size_t streamSize = static_cast<size_t>(end - streamStart);
    if (streamSize == 0 || streamSize > std::numeric_limits<uint32_t>::max()) {
        return;
    }

    deoptCount_ = deoptCount;
    bodyCount_ = bodyCount;
    headerStart_ = headerStart;
    offsetTable_ = offsetTable;
    streamStart_ = streamStart;
    streamEnd_ = end;
}

bool DeoptTranslationReader::GetHeader(DeoptId deoptId, DeoptTranslationHeader *header) const
{
    if (!IsValid() || header == nullptr || deoptId.value >= deoptCount_) {
        return false;
    }
    const uint8_t *cursor = headerStart_ + static_cast<size_t>(deoptId.value) * DEOPT_TRANSLATION_HEADER_SIZE;
    const uint8_t *end = cursor + DEOPT_TRANSLATION_HEADER_SIZE;
    uint32_t bodyId = 0;
    if (!ReadUint32LE(cursor, end, bodyId) || cursor >= end) {
        return false;
    }
    uint8_t rawType = *cursor++;
    if (cursor != end || bodyId >= bodyCount_ || !IsValidDeoptType(rawType)) {
        return false;
    }
    *header = {bodyId, static_cast<kungfu::DeoptType>(rawType)};
    return true;
}

bool DeoptTranslationReader::ReadBodyOffset(uint32_t bodyId, uint32_t *offset) const
{
    if (!IsValid() || offset == nullptr || bodyId >= bodyCount_) {
        return false;
    }
    const uint8_t *cursor = offsetTable_ + static_cast<size_t>(bodyId) * sizeof(uint32_t);
    return ReadUint32LE(cursor, streamStart_, *offset);
}

bool DeoptTranslationReader::GetBodyRange(uint32_t bodyId, const uint8_t **begin, const uint8_t **end) const
{
    if (!IsValid() || begin == nullptr || end == nullptr || bodyId >= bodyCount_) {
        return false;
    }
    uint32_t startOffset = 0;
    size_t streamSize = static_cast<size_t>(streamEnd_ - streamStart_);
    if (!ReadBodyOffset(bodyId, &startOffset) || startOffset >= streamSize ||
        (bodyId == 0 && startOffset != 0) ||
        streamStart_[startOffset] != static_cast<uint8_t>(DeoptTranslationOpcode::BEGIN)) {
        return false;
    }
    if (bodyId != 0) {
        uint32_t previousOffset = 0;
        if (!ReadBodyOffset(bodyId - 1U, &previousOffset) || previousOffset >= startOffset) {
            return false;
        }
    }
    *begin = streamStart_ + startOffset;
    if (bodyId + 1U == bodyCount_) {
        *end = streamEnd_;
        return *begin < *end;
    }
    uint32_t endOffset = 0;
    if (!ReadBodyOffset(bodyId + 1U, &endOffset) || endOffset >= streamSize ||
        streamStart_[endOffset] != static_cast<uint8_t>(DeoptTranslationOpcode::BEGIN)) {
        return false;
    }
    *end = streamStart_ + endOffset;
    return *begin < *end;
}

bool DeoptTranslationReader::GetBody(uint32_t bodyId, DeoptTranslation *translation) const
{
    if (!IsValid() || translation == nullptr || bodyId >= bodyCount_) {
        return false;
    }
    const uint8_t *begin = nullptr;
    const uint8_t *end = nullptr;
    if (!GetBodyRange(bodyId, &begin, &end)) {
        return false;
    }
    DecodedBodyHeader header {};
    if (!DecodeBodyHeader(begin, end, &header)) {
        return false;
    }

    std::vector<DeoptTranslationInput> inputs;
    if (header.basisBodyIdDistance == 0) {
        if (!DecodeBasisInputs(header, end, &inputs)) {
            return false;
        }
    } else {
        if (header.basisBodyIdDistance > bodyId) {
            return false;
        }
        uint32_t basisBodyId = bodyId - header.basisBodyIdDistance;
        const uint8_t *basisBegin = nullptr;
        const uint8_t *basisEnd = nullptr;
        if (!GetBodyRange(basisBodyId, &basisBegin, &basisEnd)) {
            return false;
        }
        DecodedBodyHeader basisHeader {};
        if (!DecodeBodyHeader(basisBegin, basisEnd, &basisHeader) || basisHeader.basisBodyIdDistance != 0 ||
            basisHeader.bytecodeOffset != header.bytecodeOffset || basisHeader.inputCount != header.inputCount ||
            !DecodeBasisInputs(basisHeader, basisEnd, &inputs) || !ApplyDeltaInputs(header, end, &inputs)) {
            return false;
        }
    }

    DeoptTranslation decoded {
        header.bytecodeOffset,
        std::move(inputs),
    };
    *translation = std::move(decoded);
    return true;
}

int64_t GetFloat64RawBits(double value)
{
    int64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

int64_t GetConstantSourceForDeoptTranslation(const ValueVertex *value, DeoptTranslationKind valueKind)
{
    switch (value->GetOpcode()) {
        case VertexOpcode::TaggedConstant:
            ASSERT(valueKind == DeoptTranslationKind::TAGGED);
            return static_cast<int64_t>(value->Cast<TaggedConstantVertex>()->GetValue());
        case VertexOpcode::Int32Constant: {
            int32_t constant = value->Cast<Int32ConstantVertex>()->GetValue();
            if (valueKind == DeoptTranslationKind::RAW_INT32 || valueKind == DeoptTranslationKind::INT32_TO_TAGGED) {
                return constant;
            }
            ASSERT(valueKind == DeoptTranslationKind::TAGGED);
            return static_cast<int64_t>(JSTaggedValue(constant).GetRawData());
        }
        case VertexOpcode::Int64Constant: {
            int64_t constant = value->Cast<Int64ConstantVertex>()->GetValue();
            if (valueKind == DeoptTranslationKind::RAW_INT32 || valueKind == DeoptTranslationKind::INT32_TO_TAGGED) {
                return constant;
            }
            ASSERT(valueKind == DeoptTranslationKind::TAGGED);
            return static_cast<int64_t>(JSTaggedValue(static_cast<int>(constant)).GetRawData());
        }
        case VertexOpcode::Float64Constant:
            ASSERT(valueKind == DeoptTranslationKind::FLOAT64_TO_TAGGED_DOUBLE);
            return GetFloat64RawBits(value->Cast<Float64ConstantVertex>()->GetValue());
        default:
            UNREACHABLE();
    }
}

DeoptTranslationInput BuildDeoptTranslationInput(ArkSteedAssembler *assembler, const EagerDeoptimizableMixin *vertex,
                                                 uint32_t index)
{
    DeoptTranslationInput input {
        vertex->GetDeoptVReg(index),
        vertex->GetDeoptValueKind(index),
        DeoptSourceKind::CONSTANT,
        0,
    };
    const InstructionOperand &operand = vertex->GetDeoptSourceLocation(index)->GetOperand();
    if (operand.IsConstant()) {
        input.source = GetConstantSourceForDeoptTranslation(vertex->GetDeoptFrameValue(index), input.valueKind);
        return input;
    }

    ASSERT(operand.IsAllocated());
    auto location = AllocatedState::Cast(operand);
    if (location.IsRegister()) {
        input.sourceKind = DeoptSourceKind::GP_REGISTER;
        input.source = location.GetRegister().Code();
        return input;
    }
    if (location.IsDoubleRegister()) {
        input.sourceKind = DeoptSourceKind::FP_REGISTER;
        input.source = location.GetDoubleRegister().Code();
        return input;
    }

    ASSERT(location.IsAnyStackSlot());
    input.sourceKind = DeoptSourceKind::STACK_SLOT;
    input.source = assembler->GetFramePointerOffsetForStackSlot(location.GetIndex(), location.GetRepresentation());
    return input;
}

std::vector<DeoptTranslationInput> BuildDeoptTranslationInputs(ArkSteedAssembler *assembler,
                                                               const EagerDeoptimizableMixin *vertex)
{
    std::vector<DeoptTranslationInput> inputs;
    inputs.reserve(vertex->GetDeoptFrameValueCount() + 1);
    inputs.push_back({
        static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH),
        DeoptTranslationKind::RAW_INT32,
        DeoptSourceKind::CONSTANT,
        ARKSTEED_SUPPORTED_INLINE_DEPTH,
    });
    for (uint32_t index = 0; index < vertex->GetDeoptFrameValueCount(); ++index) {
        inputs.push_back(BuildDeoptTranslationInput(assembler, vertex, index));
    }
    std::sort(inputs.begin(), inputs.end(),
              [](const DeoptTranslationInput &lhs, const DeoptTranslationInput &rhs) { return lhs.vreg < rhs.vreg; });
    return inputs;
}

namespace {
using MaterializedVreg = std::pair<Deoptimizier::VRegId, JSTaggedType>;

struct MaterializedArkSteedDeoptFrame {
    size_t inlineDepth {0};
    std::vector<std::pair<int32_t, JSTaggedType>> values;
};

bool ReadTranslationFromMachineCode(const MachineCode *machineCode, DeoptId deoptId,
                                    DeoptTranslationHeader *header, DeoptTranslation *translation)
{
    if (machineCode == nullptr || header == nullptr || translation == nullptr) {
        return false;
    }
    const uint8_t *translationData = nullptr;
    size_t translationSize = 0;
    if (!machineCode->GetArkSteedTranslationData(&translationData, &translationSize)) {
        return false;
    }
    DeoptTranslationReader reader(translationData, translationSize);
    return reader.GetHeader(deoptId, header) && reader.GetBody(header->bodyId, translation);
}

uint64_t ReadDeoptInputRaw(const DeoptTranslationInput &input, uintptr_t callsiteFp, uintptr_t snapshot)
{
    switch (input.sourceKind) {
        case DeoptSourceKind::CONSTANT:
            return static_cast<uint64_t>(input.source);
        case DeoptSourceKind::STACK_SLOT: {
            uintptr_t addr = callsiteFp + static_cast<intptr_t>(input.source);
            if (input.valueKind == DeoptTranslationKind::RAW_INT32 ||
                input.valueKind == DeoptTranslationKind::INT32_TO_TAGGED) {
                return static_cast<uint64_t>(*reinterpret_cast<int32_t *>(addr));
            }
            if (input.valueKind == DeoptTranslationKind::FLOAT64_TO_TAGGED_DOUBLE) {
                return *reinterpret_cast<uint64_t *>(addr);
            }
            return *reinterpret_cast<JSTaggedType *>(addr);
        }
        case DeoptSourceKind::GP_REGISTER:
            return ReadArkSteedDeoptGeneralRegister(snapshot, static_cast<uint32_t>(input.source));
        case DeoptSourceKind::FP_REGISTER:
            return ReadArkSteedDeoptFloatingRegisterBits(snapshot, static_cast<uint32_t>(input.source));
    }
    UNREACHABLE();
}

JSTaggedType MaterializeDeoptInput(const DeoptTranslationInput &input, uintptr_t callsiteFp, uintptr_t snapshot)
{
    uint64_t raw = ReadDeoptInputRaw(input, callsiteFp, snapshot);
    switch (input.valueKind) {
        case DeoptTranslationKind::TAGGED:
            return static_cast<JSTaggedType>(raw);
        case DeoptTranslationKind::RAW_INT32:
            return static_cast<JSTaggedType>(static_cast<int32_t>(raw));
        case DeoptTranslationKind::INT32_TO_TAGGED:
            return JSTaggedValue(static_cast<int32_t>(raw)).GetRawData();
        case DeoptTranslationKind::FLOAT64_TO_TAGGED_DOUBLE: {
            if (raw >= static_cast<uint64_t>(JSTaggedValue::TAG_INT - JSTaggedValue::DOUBLE_ENCODE_OFFSET)) {
                return JSTaggedValue(base::NAN_VALUE).GetRawData();
            }
            return static_cast<JSTaggedType>(raw + JSTaggedValue::DOUBLE_ENCODE_OFFSET);
        }
    }
    UNREACHABLE();
}

bool MaterializeTranslation(const DeoptTranslation &translation, uintptr_t callsiteFp, uintptr_t snapshot,
                            MaterializedArkSteedDeoptFrame *frame)
{
    ASSERT(frame != nullptr);
    frame->inlineDepth = 0;
    frame->values.clear();
    frame->values.reserve(translation.inputs.size());
    bool hasInlineDepth = false;
    for (const auto &input : translation.inputs) {
        if (input.vreg == static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH)) {
            if (hasInlineDepth || input.valueKind != DeoptTranslationKind::RAW_INT32 ||
                input.sourceKind != DeoptSourceKind::CONSTANT) {
                return false;
            }
            JSTaggedType value = MaterializeDeoptInput(input, callsiteFp, snapshot);
            int32_t inlineDepth = static_cast<int32_t>(value);
            if (inlineDepth < 0) {
                return false;
            }
            frame->inlineDepth = static_cast<size_t>(inlineDepth);
            hasInlineDepth = true;
            continue;
        }
        JSTaggedType value = MaterializeDeoptInput(input, callsiteFp, snapshot);
        frame->values.emplace_back(input.vreg, value);
    }
    return hasInlineDepth;
}

bool DecodeArkSteedEagerDeoptExit(const MachineCode *machineCode, uintptr_t returnPc, DeoptId *deoptId)
{
    if (machineCode == nullptr || deoptId == nullptr || returnPc == 0) {
        return false;
    }

    uintptr_t textStart = machineCode->GetText();
    uint32_t textSize = machineCode->GetFuncSize();
    if (textStart == 0 || textSize == 0 ||
        textStart > std::numeric_limits<uintptr_t>::max() - textSize) {
        return false;
    }
    uintptr_t textEnd = textStart + textSize;
    // A return PC designates the byte immediately after a fixed call. Consequently the
    // last valid return PC is allowed to equal the exact (unaligned) function text end.
    if (returnPc <= textStart || returnPc > textEnd) {
        return false;
    }

    const uint8_t *translationData = nullptr;
    size_t translationSize = 0;
    if (!machineCode->GetArkSteedTranslationData(&translationData, &translationSize)) {
        return false;
    }
    DeoptTranslationReader reader(translationData, translationSize);
    uint32_t exitCount = reader.GetDeoptCount();
    if (exitCount == 0) {
        return false;
    }

    constexpr uint32_t exitSize = ARKSTEED_EAGER_DEOPT_EXIT_SIZE;
    uint64_t exitClusterSize = static_cast<uint64_t>(exitCount) * exitSize;
    if (exitClusterSize > textSize) {
        return false;
    }

    uint64_t firstReturnOffset = textSize - exitClusterSize + exitSize;
    uint64_t textOffset = returnPc - textStart;
    if (textOffset < firstReturnOffset) {
        return false;
    }
    uint64_t distance = textOffset - firstReturnOffset;
    if (distance % exitSize != 0) {
        return false;
    }
    uint64_t exitIndex = distance / exitSize;
    if (exitIndex >= exitCount) {
        return false;
    }
    deoptId->value = static_cast<uint32_t>(exitIndex);
    return true;
}

bool ResolveArkSteedEagerDeoptExit(JSThread *thread, uintptr_t returnPc,
                                   MachineCode **machineCode, DeoptId *deoptId)
{
    if (thread == nullptr || machineCode == nullptr || deoptId == nullptr || returnPc == 0) {
        return false;
    }

    // Ownership is queried with the last byte of the call so that a final exit whose
    // return PC equals text end is still resolved to the current MachineCode object.
    MachineCode *owner = thread->GetEcmaVM()->GetHeap()->GetMachineCodeObject(returnPc - 1U);
    DeoptId resolved {};
    if (owner == nullptr || !DecodeArkSteedEagerDeoptExit(owner, returnPc, &resolved)) {
        return false;
    }
    *machineCode = owner;
    *deoptId = resolved;
    return true;
}
}  // namespace

bool WouldStackOverflow(JSThread *thread, const JSTaggedType *sp)
{
    ASSERT(thread != nullptr);
    ASSERT(sp != nullptr);
    uintptr_t frameBaseAddress = thread->GetGlueAddr() + JSThread::GlueData::GetFrameBaseOffset(false);
    auto frameBase = *reinterpret_cast<JSTaggedType *const *>(frameBaseAddress);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return !thread->IsCrossThreadExecutionEnable() && sp <= frameBase + JSThread::RESERVE_STACK_SIZE;
}

bool HandleArkSteedDeoptNoGC(JSThread *thread, uintptr_t returnPc, uintptr_t inputFp,
                             uintptr_t snapshot, JSTaggedType *result)
{
    DISALLOW_GARBAGE_COLLECTION;
    DISALLOW_HEAP_ALLOC;

    if (thread == nullptr || result == nullptr || inputFp == 0 || snapshot == 0 ||
        inputFp % alignof(uintptr_t) != 0 ||
        snapshot % alignof(ArkSteedDeoptSnapshot) != 0) {
        return false;
    }
    // The materialized vreg map uses temporary JSHandles. The old generic
    // DeoptHandler acquired this scope through RUNTIME_STUBS_HEADER; the
    // dedicated NoGC entry must bound those handles explicitly so recovered
    // values do not remain permanent GC roots after the frame is published.
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    MachineCode *machineCode = nullptr;
    DeoptId deoptId {};
    if (!ResolveArkSteedEagerDeoptExit(thread, returnPc, &machineCode, &deoptId) ||
        machineCode->GetCalleeRegisterNum() != 0) {
        return false;
    }

    FrameIterator it(reinterpret_cast<JSTaggedType *>(inputFp), thread);
    if (it.GetFrameType() != FrameType::STEED_FUNCTION_FRAME) {
        return false;
    }
    auto *frame = it.GetFrame<SteedFunctionFrame>();
    if (!frame->GetFunction().IsJSFunction()) {
        return false;
    }

    DeoptTranslationHeader translationHeader {};
    DeoptTranslation translation {};
    if (!ReadTranslationFromMachineCode(machineCode, deoptId, &translationHeader, &translation)) {
        return false;
    }
    MaterializedArkSteedDeoptFrame materialized;
    if (!MaterializeTranslation(translation, inputFp, snapshot, &materialized) ||
        materialized.inlineDepth != static_cast<size_t>(ARKSTEED_SUPPORTED_INLINE_DEPTH)) {
        return false;
    }

    std::vector<MaterializedVreg> values;
    values.reserve(materialized.values.size());
    for (const auto &[vreg, value] : materialized.values) {
        if (vreg < std::numeric_limits<Deoptimizier::VRegId>::min() ||
            vreg > std::numeric_limits<Deoptimizier::VRegId>::max()) {
            return false;
        }
        values.emplace_back(static_cast<Deoptimizier::VRegId>(vreg), value);
    }

    kungfu::DeoptType deoptType = translationHeader.type;
    it.SetDeoptType(static_cast<uint32_t>(deoptType));
    Deoptimizier deopt(thread, materialized.inlineDepth, deoptType);
    if (!deopt.CollectSteedDeoptContextFromRuntime(it, frame, machineCode)) {
        return false;
    }
    deopt.CollectMaterializedVregs(values, Deoptimizier::ComputeShift(materialized.inlineDepth));
    deopt.UpdateAndDumpDeoptInfo(deoptType, false);
    JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
    *result = deopt.ConstructAsmInterpretFrame(undefined, true);
    if (*result == JSTaggedValue::Exception().GetRawData()) {
        ASSERT(!thread->HasPendingException());
        *result = static_cast<JSTaggedType>(ArkSteedEagerDeoptResult::STACK_OVERFLOW);
    }
    return true;
}

}  // namespace panda::ecmascript::arksteed
