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

#ifndef ECMASCRIPT_ARKSTEED_DEOPT_ABI_H
#define ECMASCRIPT_ARKSTEED_DEOPT_ABI_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::arksteed {

constexpr size_t ARKSTEED_EAGER_DEOPT_STACK_ALIGNMENT = 16U;  // 16: stack alignment of both supported ABIs.
constexpr size_t ARKSTEED_EAGER_DEOPT_SLOT_SIZE = sizeof(uint64_t);

enum class ArkSteedEagerDeoptReturnPcSource : uint8_t {
    STACK_LINK,
    LINK_REGISTER,
};

// A valid AsmStackContext pointer is the READY result. The two small values are
// reserved for results that do not publish the staging area.
enum class ArkSteedEagerDeoptResult : uintptr_t {
    INVALID = 0,
    STACK_OVERFLOW = 1,
};

constexpr size_t AlignArkSteedEagerDeoptSnapshot(size_t size)
{
    return (size + ARKSTEED_EAGER_DEOPT_STACK_ALIGNMENT - 1U) & ~(ARKSTEED_EAGER_DEOPT_STACK_ALIGNMENT - 1U);
}

// The host ark_stub_compiler can emit code for an architecture different from
// its own. Keep both layouts available without consulting host target macros.
namespace x64_eager_deopt_abi {
constexpr uint16_t EXIT_SIZE = 5U;  // 5: byte width of CALL rel32.
constexpr ArkSteedEagerDeoptReturnPcSource RETURN_PC_SOURCE = ArkSteedEagerDeoptReturnPcSource::STACK_LINK;
constexpr size_t FIXED_EXIT_LINK_SIZE = ARKSTEED_EAGER_DEOPT_SLOT_SIZE;  // CALL pushes one return-address slot.

constexpr std::array<uint8_t, 10U> GENERAL_REGISTER_CODES = {
    // 10: all allocatable x64 GPRs.
    0U,   // rax
    3U,   // rbx
    1U,   // rcx
    2U,   // rdx
    6U,   // rsi
    7U,   // rdi
    8U,   // r8
    9U,   // r9
    11U,  // r11
    12U,  // r12
};
constexpr uint32_t FLOATING_REGISTER_COUNT = 15U;       // xmm0-xmm14
constexpr uint32_t GENERAL_REGISTER_CODE_COUNT = 16U;   // 16: architectural GPR codes rax-r15.
constexpr uint32_t FLOATING_REGISTER_CODE_COUNT = 16U;  // 16: architectural FP codes xmm0-xmm15.
constexpr uint32_t FLOATING_SNAPSHOT_OFFSET = GENERAL_REGISTER_CODES.size() * ARKSTEED_EAGER_DEOPT_SLOT_SIZE;
constexpr uint32_t SNAPSHOT_SIZE = static_cast<uint32_t>(AlignArkSteedEagerDeoptSnapshot(
    FLOATING_SNAPSHOT_OFFSET + FLOATING_REGISTER_COUNT * ARKSTEED_EAGER_DEOPT_SLOT_SIZE));
constexpr size_t ENTRY_METADATA_SIZE = 2U * ARKSTEED_EAGER_DEOPT_SLOT_SIZE;  // Glue plus SysV call alignment.
constexpr size_t ENTRY_FRAME_SIZE = SNAPSHOT_SIZE + ENTRY_METADATA_SIZE;
constexpr size_t VENEER_FRAME_SIZE = ARKSTEED_EAGER_DEOPT_SLOT_SIZE;  // CALL continuation.
}  // namespace x64_eager_deopt_abi

namespace aarch64_eager_deopt_abi {
constexpr uint16_t EXIT_SIZE = sizeof(uint32_t);  // 4 bytes: one BL instruction.
constexpr ArkSteedEagerDeoptReturnPcSource RETURN_PC_SOURCE = ArkSteedEagerDeoptReturnPcSource::LINK_REGISTER;
constexpr size_t FIXED_EXIT_LINK_SIZE = 0U;  // 0: BL keeps the fixed-exit return PC in LR.

constexpr std::array<uint8_t, 26U> GENERAL_REGISTER_CODES = {
    // 26: all allocatable AArch64 GPRs.
    0U,   // x0
    1U,   // x1
    2U,   // x2
    3U,   // x3
    4U,   // x4
    5U,   // x5
    6U,   // x6
    7U,   // x7
    8U,   // x8
    9U,   // x9
    10U,  // x10
    11U,  // x11
    12U,  // x12
    13U,  // x13
    14U,  // x14
    15U,  // x15
    19U,  // x19
    20U,  // x20
    21U,  // x21
    22U,  // x22
    23U,  // x23
    24U,  // x24
    25U,  // x25
    26U,  // x26
    27U,  // x27
    28U,  // x28
};
constexpr uint32_t FLOATING_REGISTER_COUNT = 30U;       // d0-d29
constexpr uint32_t GENERAL_REGISTER_CODE_COUNT = 32U;   // 32: architectural GPR codes x0-x31.
constexpr uint32_t FLOATING_REGISTER_CODE_COUNT = 32U;  // 32: architectural FP codes d0-d31.
constexpr uint32_t FLOATING_SNAPSHOT_OFFSET = GENERAL_REGISTER_CODES.size() * ARKSTEED_EAGER_DEOPT_SLOT_SIZE;
constexpr uint32_t SNAPSHOT_SIZE = static_cast<uint32_t>(AlignArkSteedEagerDeoptSnapshot(
    FLOATING_SNAPSHOT_OFFSET + FLOATING_REGISTER_COUNT * ARKSTEED_EAGER_DEOPT_SLOT_SIZE));
constexpr size_t ENTRY_METADATA_SIZE = 2U * ARKSTEED_EAGER_DEOPT_SLOT_SIZE;  // Veneer continuation and glue.
constexpr size_t ENTRY_FRAME_SIZE = SNAPSHOT_SIZE + ENTRY_METADATA_SIZE;
constexpr size_t VENEER_FRAME_SIZE = 2U * ARKSTEED_EAGER_DEOPT_SLOT_SIZE;  // Saved fixed-exit LR plus alignment.
}  // namespace aarch64_eager_deopt_abi

#if defined(PANDA_TARGET_AMD64)
constexpr uint16_t ARKSTEED_EAGER_DEOPT_EXIT_SIZE = x64_eager_deopt_abi::EXIT_SIZE;
constexpr ArkSteedEagerDeoptReturnPcSource ARKSTEED_EAGER_DEOPT_RETURN_PC_SOURCE =
    x64_eager_deopt_abi::RETURN_PC_SOURCE;
constexpr size_t ARKSTEED_EAGER_DEOPT_FIXED_EXIT_LINK_SIZE = x64_eager_deopt_abi::FIXED_EXIT_LINK_SIZE;
constexpr auto ARKSTEED_DEOPT_GENERAL_REGISTER_CODES = x64_eager_deopt_abi::GENERAL_REGISTER_CODES;
constexpr uint32_t ARKSTEED_DEOPT_FLOATING_REGISTER_COUNT = x64_eager_deopt_abi::FLOATING_REGISTER_COUNT;
constexpr uint32_t ARKSTEED_DEOPT_GENERAL_REGISTER_CODE_COUNT = x64_eager_deopt_abi::GENERAL_REGISTER_CODE_COUNT;
constexpr uint32_t ARKSTEED_DEOPT_FLOATING_REGISTER_CODE_COUNT = x64_eager_deopt_abi::FLOATING_REGISTER_CODE_COUNT;

// r10 carries the global entry target and r13 carries glue before the snapshot is complete.
constexpr ArkSteedRegister ARKSTEED_EAGER_DEOPT_ENTRY_TARGET_REGISTER = x64::r10;
constexpr ArkSteedRegister ARKSTEED_EAGER_DEOPT_ENTRY_GLUE_REGISTER = x64::r13;
#elif defined(PANDA_TARGET_ARM64)
constexpr uint16_t ARKSTEED_EAGER_DEOPT_EXIT_SIZE = aarch64_eager_deopt_abi::EXIT_SIZE;
constexpr ArkSteedEagerDeoptReturnPcSource ARKSTEED_EAGER_DEOPT_RETURN_PC_SOURCE =
    aarch64_eager_deopt_abi::RETURN_PC_SOURCE;
constexpr size_t ARKSTEED_EAGER_DEOPT_FIXED_EXIT_LINK_SIZE = aarch64_eager_deopt_abi::FIXED_EXIT_LINK_SIZE;
constexpr auto ARKSTEED_DEOPT_GENERAL_REGISTER_CODES = aarch64_eager_deopt_abi::GENERAL_REGISTER_CODES;
constexpr uint32_t ARKSTEED_DEOPT_FLOATING_REGISTER_COUNT = aarch64_eager_deopt_abi::FLOATING_REGISTER_COUNT;
constexpr uint32_t ARKSTEED_DEOPT_GENERAL_REGISTER_CODE_COUNT = aarch64_eager_deopt_abi::GENERAL_REGISTER_CODE_COUNT;
constexpr uint32_t ARKSTEED_DEOPT_FLOATING_REGISTER_CODE_COUNT = aarch64_eager_deopt_abi::FLOATING_REGISTER_CODE_COUNT;

// x16 carries the global entry target and x17 carries glue before the snapshot is complete.
constexpr ArkSteedRegister ARKSTEED_EAGER_DEOPT_ENTRY_TARGET_REGISTER = aarch64::x16;
constexpr ArkSteedRegister ARKSTEED_EAGER_DEOPT_ENTRY_GLUE_REGISTER = aarch64::x17;
#endif

struct alignas(ARKSTEED_EAGER_DEOPT_STACK_ALIGNMENT) ArkSteedDeoptSnapshot {
    static constexpr uint32_t GENERAL_SLOT_COUNT = ARKSTEED_DEOPT_GENERAL_REGISTER_CODES.size();
    static constexpr uint32_t FLOATING_SLOT_COUNT = ARKSTEED_DEOPT_FLOATING_REGISTER_COUNT;

    uint64_t general[GENERAL_SLOT_COUNT];
    uint64_t floating[FLOATING_SLOT_COUNT];
};

constexpr uint32_t ARKSTEED_DEOPT_SNAPSHOT_SIZE = sizeof(ArkSteedDeoptSnapshot);
constexpr uint32_t ARKSTEED_DEOPT_FLOATING_SNAPSHOT_OFFSET =
    static_cast<uint32_t>(sizeof(uint64_t) * ArkSteedDeoptSnapshot::GENERAL_SLOT_COUNT);

constexpr int32_t GetArkSteedDeoptGeneralSnapshotOffset(uint32_t code)
{
    for (uint32_t slot = 0; slot < ARKSTEED_DEOPT_GENERAL_REGISTER_CODES.size(); ++slot) {
        if (ARKSTEED_DEOPT_GENERAL_REGISTER_CODES[slot] == code) {
            return static_cast<int32_t>(slot * sizeof(uint64_t));
        }
    }
    return -1;  // -1: the register has no snapshot slot.
}

constexpr int32_t GetArkSteedDeoptFloatingSnapshotOffset(uint32_t code)
{
    if (code >= ArkSteedDeoptSnapshot::FLOATING_SLOT_COUNT) {
        return -1;  // -1: the register has no snapshot slot.
    }
    return static_cast<int32_t>(ARKSTEED_DEOPT_FLOATING_SNAPSHOT_OFFSET + code * sizeof(uint64_t));
}

constexpr uint64_t GetArkSteedDeoptGeneralRegisterMask()
{
    uint64_t mask = 0;
    for (uint32_t code : ARKSTEED_DEOPT_GENERAL_REGISTER_CODES) {
        mask |= 1ULL << code;
    }
    return mask;
}

constexpr uint64_t GetArkSteedDeoptFloatingRegisterMask()
{
    return (1ULL << ARKSTEED_DEOPT_FLOATING_REGISTER_COUNT) - 1ULL;
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
    constexpr uint32_t slotCount =
        ArkSteedDeoptSnapshot::GENERAL_SLOT_COUNT + ArkSteedDeoptSnapshot::FLOATING_SLOT_COUNT;
    static_assert(slotCount < std::numeric_limits<uint64_t>::digits);
    return occupiedSlots == ((1ULL << slotCount) - 1ULL);
}

// The global entry reserves the snapshot plus the native values that cannot live in allocatable registers.
#if defined(PANDA_TARGET_AMD64)
constexpr size_t ARKSTEED_EAGER_DEOPT_ENTRY_METADATA_SIZE = x64_eager_deopt_abi::ENTRY_METADATA_SIZE;
#elif defined(PANDA_TARGET_ARM64)
constexpr size_t ARKSTEED_EAGER_DEOPT_ENTRY_METADATA_SIZE = aarch64_eager_deopt_abi::ENTRY_METADATA_SIZE;
#endif
constexpr size_t ARKSTEED_EAGER_DEOPT_ENTRY_FRAME_SIZE =
    ARKSTEED_DEOPT_SNAPSHOT_SIZE + ARKSTEED_EAGER_DEOPT_ENTRY_METADATA_SIZE;
#if defined(PANDA_TARGET_AMD64)
constexpr size_t ARKSTEED_EAGER_DEOPT_VENEER_FRAME_SIZE = x64_eager_deopt_abi::VENEER_FRAME_SIZE;
#elif defined(PANDA_TARGET_ARM64)
constexpr size_t ARKSTEED_EAGER_DEOPT_VENEER_FRAME_SIZE = aarch64_eager_deopt_abi::VENEER_FRAME_SIZE;
#endif

static_assert(GetAllocatableGeneralRegisters().Count() == ArkSteedDeoptSnapshot::GENERAL_SLOT_COUNT);
static_assert(GetAllocatableDoubleRegisters().Count() == ArkSteedDeoptSnapshot::FLOATING_SLOT_COUNT);
static_assert(GetAllocatableGeneralRegisters().Bits() == GetArkSteedDeoptGeneralRegisterMask());
static_assert(GetAllocatableDoubleRegisters().Bits() == GetArkSteedDeoptFloatingRegisterMask());
static_assert(AllocatableRegistersHaveDeoptSnapshotOffsets());
static_assert(!GetAllocatableGeneralRegisters().Has(ARKSTEED_EAGER_DEOPT_ENTRY_TARGET_REGISTER));
static_assert(!GetAllocatableGeneralRegisters().Has(ARKSTEED_EAGER_DEOPT_ENTRY_GLUE_REGISTER));
static_assert(ARKSTEED_EAGER_DEOPT_ENTRY_TARGET_REGISTER != ARKSTEED_EAGER_DEOPT_ENTRY_GLUE_REGISTER);
static_assert(alignof(ArkSteedDeoptSnapshot) == ARKSTEED_EAGER_DEOPT_STACK_ALIGNMENT);
static_assert(ARKSTEED_DEOPT_FLOATING_SNAPSHOT_OFFSET % alignof(uint64_t) == 0U);
static_assert(ARKSTEED_EAGER_DEOPT_ENTRY_FRAME_SIZE % ARKSTEED_EAGER_DEOPT_STACK_ALIGNMENT == 0U);
static_assert(x64_eager_deopt_abi::SNAPSHOT_SIZE == 208U);         // 208: align16((10 GP + 15 FP) * 8 bytes).
static_assert(x64_eager_deopt_abi::ENTRY_FRAME_SIZE == 224U);      // 224: 208-byte snapshot + 16-byte metadata.
static_assert(aarch64_eager_deopt_abi::SNAPSHOT_SIZE == 448U);     // 448: align16((26 GP + 30 FP) * 8 bytes).
static_assert(aarch64_eager_deopt_abi::ENTRY_FRAME_SIZE == 464U);  // 464: 448-byte snapshot + 16-byte metadata.

#if defined(PANDA_TARGET_AMD64)
static_assert(ARKSTEED_EAGER_DEOPT_EXIT_SIZE == 5U);  // 5: byte width of CALL rel32.
static_assert(ARKSTEED_EAGER_DEOPT_RETURN_PC_SOURCE == ArkSteedEagerDeoptReturnPcSource::STACK_LINK);
static_assert(ARKSTEED_DEOPT_SNAPSHOT_SIZE == 208U);  // 208: x64 snapshot layout pinned above.
static_assert(ARKSTEED_DEOPT_SNAPSHOT_SIZE == x64_eager_deopt_abi::SNAPSHOT_SIZE);
static_assert(ARKSTEED_EAGER_DEOPT_ENTRY_FRAME_SIZE == x64_eager_deopt_abi::ENTRY_FRAME_SIZE);
static_assert(ARKSTEED_EAGER_DEOPT_VENEER_FRAME_SIZE == x64_eager_deopt_abi::VENEER_FRAME_SIZE);
#elif defined(PANDA_TARGET_ARM64)
static_assert(ARKSTEED_EAGER_DEOPT_EXIT_SIZE == sizeof(uint32_t));
static_assert(ARKSTEED_EAGER_DEOPT_RETURN_PC_SOURCE == ArkSteedEagerDeoptReturnPcSource::LINK_REGISTER);
static_assert(ARKSTEED_DEOPT_SNAPSHOT_SIZE == 448U);  // 448: AArch64 snapshot layout pinned above.
static_assert(ARKSTEED_DEOPT_SNAPSHOT_SIZE == aarch64_eager_deopt_abi::SNAPSHOT_SIZE);
static_assert(ARKSTEED_EAGER_DEOPT_ENTRY_FRAME_SIZE == aarch64_eager_deopt_abi::ENTRY_FRAME_SIZE);
static_assert(ARKSTEED_EAGER_DEOPT_VENEER_FRAME_SIZE == aarch64_eager_deopt_abi::VENEER_FRAME_SIZE);
#endif

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

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_DEOPT_ABI_H
