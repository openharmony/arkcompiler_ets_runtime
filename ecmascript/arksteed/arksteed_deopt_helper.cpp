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
#include "ecmascript/frames.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/machine_code.h"

namespace panda::ecmascript::arksteed {
namespace {
constexpr uint8_t LEB128_PAYLOAD_MASK = 0x7FU;
constexpr uint8_t LEB128_CONTINUATION_BIT = 0x80U;
constexpr uint8_t LEB128_SIGN_BIT = 0x40U;
constexpr uint32_t LEB128_PAYLOAD_BITS = 7U;
constexpr uint32_t LEB128_MAX_SHIFT = 63U;
constexpr size_t MIN_EXPLICIT_INPUT_SIZE = 3U;  // opcode, vreg and source
constexpr uint8_t FIRST_SHORT_MATCH_OPCODE = static_cast<uint8_t>(DeoptTranslationOpcode::COUNT);
constexpr uint32_t MAX_SHORT_MATCH_COUNT = UINT8_MAX - FIRST_SHORT_MATCH_OPCODE + 1U;

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

uint64_t HashTranslationPayload(const DeoptTranslation &translation)
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

struct DecodedTranslationHeader {
    uint32_t basisIdDistance {0};
    uint32_t bytecodeOffset {0};
    kungfu::DeoptType type {kungfu::DeoptType::NONE};
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

bool DecodeTranslationHeader(const uint8_t *begin, const uint8_t *end, DecodedTranslationHeader *header)
{
    if (begin == nullptr || end == nullptr || header == nullptr || begin >= end ||
        *begin != static_cast<uint8_t>(DeoptTranslationOpcode::BEGIN)) {
        return false;
    }
    const uint8_t *cursor = begin + 1;
    uint32_t rawType = 0;
    if (!ReadUint32ULEB(cursor, end, &header->basisIdDistance) ||
        !ReadUint32ULEB(cursor, end, &header->bytecodeOffset) || !ReadUint32ULEB(cursor, end, &rawType) ||
        !ReadUint32ULEB(cursor, end, &header->inputCount) ||
        rawType > static_cast<uint32_t>(kungfu::DeoptType::HOTRELOAD_PATCHMAIN)) {
        return false;
    }
    header->type = static_cast<kungfu::DeoptType>(rawType);
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

bool DecodeBasisInputs(const DecodedTranslationHeader &header, const uint8_t *end,
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
        if (!DecodeExplicitInput(opcode, cursor, end, &input)) {
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

bool ApplyDeltaInputs(const DecodedTranslationHeader &header, const uint8_t *end,
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
    CHECK(translations_.size() < std::numeric_limits<uint32_t>::max());
    CHECK(inputs.size() <= std::numeric_limits<uint32_t>::max());
    CHECK(static_cast<uint32_t>(type) <= static_cast<uint32_t>(kungfu::DeoptType::HOTRELOAD_PATCHMAIN));
    for (size_t index = 0; index < inputs.size(); ++index) {
        ValidateTranslationInput(inputs[index]);
        CHECK(index == 0 || inputs[index - 1U].vreg < inputs[index].vreg);
    }

    auto basisIterator = basisTranslationIds_.find(bytecodeOffset);
    if (basisIterator != basisTranslationIds_.end()) {
        CHECK(basisIterator->second.value < translations_.size());
        const auto &basisInputs = translations_[basisIterator->second.value].inputs;
        CHECK(inputs.size() == basisInputs.size());
        for (size_t index = 0; index < inputs.size(); ++index) {
            CHECK(inputs[index].vreg == basisInputs[index].vreg);
        }
    }

    DeoptTranslation candidate {
        bytecodeOffset,
        type,
        std::move(inputs),
    };
    uint64_t payloadHash = HashTranslationPayload(candidate);
    auto [begin, end] = translationIndex_.equal_range(payloadHash);
    for (auto iterator = begin; iterator != end; ++iterator) {
        CHECK(iterator->second.value < translations_.size());
        const auto &translation = translations_[iterator->second.value];
        if (translation.PayloadEquals(candidate)) {
            return iterator->second;
        }
    }

    DeoptId id {static_cast<uint32_t>(translations_.size())};
    translations_.push_back(std::move(candidate));
    translationIndex_.emplace(payloadHash, id);
    basisTranslationIds_.try_emplace(bytecodeOffset, id);
    return id;
}

std::vector<uint8_t> DeoptTranslationBuilder::Encode() const
{
    if (translations_.empty()) {
        return {};
    }

    CHECK(translations_.size() <= std::numeric_limits<uint32_t>::max());
    std::vector<uint32_t> offsets;
    std::vector<uint8_t> stream;
    offsets.reserve(translations_.size());
    for (size_t index = 0; index < translations_.size(); ++index) {
        const auto &translation = translations_[index];
        CHECK(stream.size() <= std::numeric_limits<uint32_t>::max());
        offsets.push_back(static_cast<uint32_t>(stream.size()));

        auto basisIterator = basisTranslationIds_.find(translation.bytecodeOffset);
        CHECK(basisIterator != basisTranslationIds_.end());
        DeoptId basisId = basisIterator->second;
        CHECK(basisId.value <= index);
        CHECK(basisId.value < offsets.size());
        uint32_t basisIdDistance = static_cast<uint32_t>(index) - basisId.value;

        WriteOpcode(DeoptTranslationOpcode::BEGIN, stream);
        WriteULEB128(basisIdDistance, stream);
        WriteULEB128(translation.bytecodeOffset, stream);
        WriteULEB128(static_cast<uint8_t>(translation.type), stream);
        WriteULEB128(translation.inputs.size(), stream);
        if (basisIdDistance == 0) {
            for (const auto &input : translation.inputs) {
                EncodeExplicitInput(input, stream);
            }
        } else {
            EncodeDeltaInputs(translation, translations_[basisId.value], stream);
        }
    }
    CHECK(stream.size() <= std::numeric_limits<uint32_t>::max());

    CHECK(offsets.size() <= (std::numeric_limits<size_t>::max() - sizeof(uint32_t)) / sizeof(uint32_t));
    size_t headerSize = sizeof(uint32_t) + offsets.size() * sizeof(uint32_t);
    CHECK(stream.size() <= std::numeric_limits<size_t>::max() - headerSize);
    std::vector<uint8_t> output;
    output.reserve(headerSize + stream.size());
    WriteUint32LE(static_cast<uint32_t>(translations_.size()), output);
    for (uint32_t offset : offsets) {
        WriteUint32LE(offset, output);
    }
    output.insert(output.end(), stream.begin(), stream.end());
    return output;
}

DeoptTranslationReader::DeoptTranslationReader(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(uint32_t)) {
        return;
    }
    uintptr_t dataAddress = reinterpret_cast<uintptr_t>(data);
    if (size > std::numeric_limits<uintptr_t>::max() - dataAddress) {
        return;
    }
    const uint8_t *cursor = data;
    const uint8_t *end = reinterpret_cast<const uint8_t *>(dataAddress + size);
    uint32_t translationCount = 0;
    if (!ReadUint32LE(cursor, end, translationCount) || translationCount == 0 ||
        translationCount > static_cast<size_t>(end - cursor) / sizeof(uint32_t)) {
        return;
    }
    size_t tableSize = static_cast<size_t>(translationCount) * sizeof(uint32_t);
    const uint8_t *streamStart = cursor + tableSize;
    size_t streamSize = static_cast<size_t>(end - streamStart);
    if (streamSize == 0 || streamSize > std::numeric_limits<uint32_t>::max()) {
        return;
    }

    uint32_t previousOffset = 0;
    const uint8_t *offsetCursor = cursor;
    for (uint32_t index = 0; index < translationCount; ++index) {
        uint32_t offset = 0;
        if (!ReadUint32LE(offsetCursor, streamStart, offset) || offset >= streamSize ||
            (index == 0 && offset != 0) || (index != 0 && offset <= previousOffset) ||
            streamStart[offset] != static_cast<uint8_t>(DeoptTranslationOpcode::BEGIN)) {
            return;
        }
        previousOffset = offset;
    }

    translationCount_ = translationCount;
    offsetTable_ = cursor;
    streamStart_ = streamStart;
    streamEnd_ = end;
}

bool DeoptTranslationReader::ReadOffset(uint32_t index, uint32_t *offset) const
{
    if (!IsValid() || offset == nullptr || index >= translationCount_) {
        return false;
    }
    const uint8_t *cursor = offsetTable_ + static_cast<size_t>(index) * sizeof(uint32_t);
    return ReadUint32LE(cursor, streamStart_, *offset);
}

bool DeoptTranslationReader::GetTranslationRange(uint32_t index, const uint8_t **begin, const uint8_t **end) const
{
    if (begin == nullptr || end == nullptr) {
        return false;
    }
    uint32_t startOffset = 0;
    if (!ReadOffset(index, &startOffset)) {
        return false;
    }
    *begin = streamStart_ + startOffset;
    if (index + 1U == translationCount_) {
        *end = streamEnd_;
        return true;
    }
    uint32_t endOffset = 0;
    if (!ReadOffset(index + 1U, &endOffset)) {
        return false;
    }
    *end = streamStart_ + endOffset;
    return *begin < *end;
}

bool DeoptTranslationReader::GetTranslation(DeoptId deoptId, DeoptTranslation *translation) const
{
    if (!IsValid() || translation == nullptr || deoptId.value >= translationCount_) {
        return false;
    }
    const uint8_t *begin = nullptr;
    const uint8_t *end = nullptr;
    if (!GetTranslationRange(deoptId.value, &begin, &end)) {
        return false;
    }
    DecodedTranslationHeader header {};
    if (!DecodeTranslationHeader(begin, end, &header)) {
        return false;
    }

    std::vector<DeoptTranslationInput> inputs;
    if (header.basisIdDistance == 0) {
        if (!DecodeBasisInputs(header, end, &inputs)) {
            return false;
        }
    } else {
        if (header.basisIdDistance > deoptId.value) {
            return false;
        }
        uint32_t basisIndex = deoptId.value - header.basisIdDistance;
        const uint8_t *basisBegin = nullptr;
        const uint8_t *basisEnd = nullptr;
        if (!GetTranslationRange(basisIndex, &basisBegin, &basisEnd)) {
            return false;
        }
        DecodedTranslationHeader basisHeader {};
        if (!DecodeTranslationHeader(basisBegin, basisEnd, &basisHeader) || basisHeader.basisIdDistance != 0 ||
            basisHeader.bytecodeOffset != header.bytecodeOffset || basisHeader.inputCount != header.inputCount ||
            !DecodeBasisInputs(basisHeader, basisEnd, &inputs) || !ApplyDeltaInputs(header, end, &inputs)) {
            return false;
        }
    }

    DeoptTranslation decoded {
        header.bytecodeOffset,
        header.type,
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
        0,
    });
    for (uint32_t index = 0; index < vertex->GetDeoptFrameValueCount(); ++index) {
        inputs.push_back(BuildDeoptTranslationInput(assembler, vertex, index));
    }
    std::sort(inputs.begin(), inputs.end(),
              [](const DeoptTranslationInput &lhs, const DeoptTranslationInput &rhs) { return lhs.vreg < rhs.vreg; });
    return inputs;
}

uint32_t GetTaggedDeoptSnapshotGeneralRegisters(const std::vector<DeoptTranslationInput> &inputs)
{
    uint32_t taggedRegisters = 0;
    for (const auto &input : inputs) {
        if (input.valueKind != DeoptTranslationKind::TAGGED || input.sourceKind != DeoptSourceKind::GP_REGISTER) {
            continue;
        }
        ASSERT(input.source >= 0);
        uint32_t registerCode = static_cast<uint32_t>(input.source);
        ASSERT(GetArkSteedDeoptGeneralSnapshotOffset(registerCode) >= 0);
        taggedRegisters |= 1U << registerCode;
    }
    return taggedRegisters;
}

namespace {
using MaterializedVreg = std::pair<Deoptimizier::VRegId, JSTaggedType>;

struct MaterializedArkSteedDeoptFrame {
    size_t inlineDepth {0};
    bool hasInlineDepth {false};
    std::vector<MaterializedVreg> values;
};

bool IsNonSteedOptimizedFrame(FrameType type)
{
    return type == FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME || type == FrameType::OPTIMIZED_JS_FUNCTION_FRAME ||
           type == FrameType::FASTJIT_FUNCTION_FRAME || type == FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME;
}

bool ReadTranslationFromSteedFrame(const FrameIterator &it, DeoptId deoptId, DeoptTranslation *translation)
{
#if ECMASCRIPT_ENABLE_ARK_STEED
    JSTaggedValue machineCodeValue(*it.GetMachineCodeSlot());
    if (!machineCodeValue.IsMachineCodeObject()) {
        return false;
    }

    const MachineCode *machineCode = MachineCode::Cast(machineCodeValue.GetTaggedObject());
    const uint8_t *translationData = nullptr;
    size_t translationSize = 0;
    if (!machineCode->GetArkSteedTranslationData(&translationData, &translationSize)) {
        return false;
    }
    DeoptTranslationReader reader(translationData, translationSize);
    if (!reader.IsValid()) {
        return false;
    }
    return reader.GetTranslation(deoptId, translation);
#else
    (void)it;
    (void)deoptId;
    (void)translation;
    return false;
#endif
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
    frame->hasInlineDepth = false;
    frame->values.clear();
    frame->values.reserve(translation.inputs.size());
    for (const auto &input : translation.inputs) {
        if (input.vreg == static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH)) {
            if (frame->hasInlineDepth || input.valueKind != DeoptTranslationKind::RAW_INT32) {
                return false;
            }
            JSTaggedType value = MaterializeDeoptInput(input, callsiteFp, snapshot);
            int32_t inlineDepth = static_cast<int32_t>(value);
            if (inlineDepth < 0) {
                return false;
            }
            frame->inlineDepth = static_cast<size_t>(inlineDepth);
            frame->hasInlineDepth = true;
            continue;
        }
        JSTaggedType value = MaterializeDeoptInput(input, callsiteFp, snapshot);
        frame->values.emplace_back(static_cast<Deoptimizier::VRegId>(input.vreg), value);
    }
    return frame->hasInlineDepth;
}
}  // namespace

bool HandleArkSteedDeopt(JSThread *thread, DeoptId deoptId, JSTaggedType *result)
{
    if (thread == nullptr || result == nullptr) {
        return false;
    }

    JSTaggedType *asmBridgeSp = nullptr;
    JSTaggedType *lastLeave = const_cast<JSTaggedType *>(thread->GetLastLeaveFrame());
    FrameIterator it(lastLeave, thread);
    for (; !it.Done(); it.Advance<GCVisitedFlag::DEOPT>()) {
        FrameType frameType = it.GetFrameType();
        if (IsNonSteedOptimizedFrame(frameType)) {
            return false;
        }
        switch (frameType) {
            case FrameType::ASM_BRIDGE_FRAME:
                asmBridgeSp = it.GetSp();
                break;
            case FrameType::OPTIMIZED_FRAME:
            case FrameType::LEAVE_FRAME:
                break;
            case FrameType::STEED_FUNCTION_FRAME: {
                if (asmBridgeSp == nullptr) {
                    return false;
                }
                DeoptTranslation translation {};
                if (!ReadTranslationFromSteedFrame(it, deoptId, &translation)) {
                    return false;
                }

                uintptr_t callsiteFp = reinterpret_cast<uintptr_t>(it.GetSp());
                uintptr_t snapshot = GetArkSteedDeoptSnapshotFromCallsiteSp(it.GetCallSiteSp());
                MaterializedArkSteedDeoptFrame materialized;
                if (!MaterializeTranslation(translation, callsiteFp, snapshot, &materialized)) {
                    return false;
                }

                it.SetDeoptType(static_cast<uint32_t>(translation.type));
                Deoptimizier deopt(thread, materialized.inlineDepth, translation.type);
                auto frame = it.GetFrame<SteedFunctionFrame>();
                deopt.CollectSteedDeoptContext(it, frame, asmBridgeSp);
                deopt.CollectMaterializedVregs(materialized.values,
                                               Deoptimizier::ComputeShift(materialized.inlineDepth));
                deopt.UpdateAndDumpDeoptInfo(translation.type, false);
                JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
                *result = deopt.ConstructAsmInterpretFrame(undefined);
                return true;
            }
            default:
                return false;
        }
    }
    return false;
}

}  // namespace panda::ecmascript::arksteed
