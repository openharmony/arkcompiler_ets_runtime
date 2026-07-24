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

#include <cstdint>
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

// These values are part of the versioned safepoint translation encoding.
enum class ArkSteedDeoptValueKind : uint8_t {
    TAGGED = 0,
    INT32_TO_TAGGED = 1,
    FLOAT64_TO_TAGGED_DOUBLE = 2,
    RAW_INT32 = 3,
};

enum class ArkSteedDeoptSourceKind : uint8_t {
    CONSTANT = 0,
    STACK_SLOT = 1,
    GP_REGISTER = 2,
    FP_REGISTER = 3,
};

struct ArkSteedDeoptId {
    uint32_t value {0};

    bool operator==(const ArkSteedDeoptId &other) const
    {
        return value == other.value;
    }

    bool operator!=(const ArkSteedDeoptId &other) const
    {
        return !(*this == other);
    }
};

struct ArkSteedDeoptTranslationInput {
    int32_t vreg {0};
    ArkSteedDeoptValueKind valueKind {ArkSteedDeoptValueKind::TAGGED};
    ArkSteedDeoptSourceKind sourceKind {ArkSteedDeoptSourceKind::CONSTANT};
    int64_t source {0};

    bool operator==(const ArkSteedDeoptTranslationInput &other) const
    {
        return vreg == other.vreg && valueKind == other.valueKind && sourceKind == other.sourceKind &&
               source == other.source;
    }
};

struct ArkSteedDeoptTranslation {
    ArkSteedDeoptId id;
    uint32_t bytecodeOffset {0};
    kungfu::DeoptType type {kungfu::DeoptType::NONE};
    std::vector<ArkSteedDeoptTranslationInput> inputs;

    bool PayloadEquals(const ArkSteedDeoptTranslation &other) const
    {
        // The reason remains representative diagnostic data; recovery identity is the bytecode state and sources.
        return bytecodeOffset == other.bytecodeOffset && inputs == other.inputs;
    }
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
#elif defined(PANDA_TARGET_ARM64)
constexpr uint32_t ARKSTEED_DEOPT_GENERAL_REGISTER_CODE_COUNT = 32;
#endif

constexpr uint32_t ARKSTEED_DEOPT_SNAPSHOT_SIZE = sizeof(ArkSteedDeoptSnapshot);
constexpr uint32_t ARKSTEED_DEOPT_FLOATING_SNAPSHOT_OFFSET =
    static_cast<uint32_t>(sizeof(uint64_t) * ArkSteedDeoptSnapshot::GENERAL_SLOT_COUNT);

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
int64_t GetConstantSourceForArkSteedDeoptTranslation(const ValueVertex *value, ArkSteedDeoptValueKind valueKind);
ArkSteedDeoptTranslationInput BuildArkSteedDeoptTranslationInput(ArkSteedAssembler *assembler,
                                                                 const EagerDeoptimizableMixin *vertex, uint32_t index);
std::vector<ArkSteedDeoptTranslationInput> BuildArkSteedDeoptTranslationInputs(ArkSteedAssembler *assembler,
                                                                               const EagerDeoptimizableMixin *vertex);
uint32_t GetTaggedDeoptSnapshotGeneralRegisters(const std::vector<ArkSteedDeoptTranslationInput> &inputs);
void CollectUsedDeoptSnapshotRegisters(const std::vector<ArkSteedDeoptTranslationInput> &inputs,
                                       ArkSteedRegList *generalRegisters, ArkDoubleRegList *floatingRegisters);

bool HandleArkSteedDeopt(JSThread *thread, ArkSteedDeoptId deoptId, JSTaggedType *result);

}  // namespace arksteed
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_ARKSTEED_DEOPT_HELPER_H
