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

#ifndef ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_TYPES_H
#define ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_TYPES_H

#include "ecmascript/arksteed/arksteed_reglist.h"

#if defined(PANDA_TARGET_ARM64)
#include "ecmascript/compiler/assembler/aarch64/register_aarch64.h"
#elif defined(PANDA_TARGET_AMD64)
#include "ecmascript/compiler/assembler/x64/register_x64.h"
#endif
#include "ecmascript/arksteed/arksteed_opcode_list.h"
#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript::arksteed {

// Platform-specific register type aliases
#if defined(PANDA_TARGET_ARM64)
using ArkSteedRegister = aarch64::Register;
using ArkSteedDoubleRegister = aarch64::VRegister;
using ArkSteedRegList = RegListBase<aarch64::Register>;
using ArkDoubleRegList = RegListBase<aarch64::VRegister>;
#elif defined(PANDA_TARGET_AMD64)
using ArkSteedRegister = x64::Register;
using ArkSteedDoubleRegister = x64::XMMRegister;
using ArkSteedRegList = RegListBase<x64::Register>;
using ArkDoubleRegList = RegListBase<x64::XMMRegister>;
#endif

// Machine representation types
enum class MachineRepresentation : uint8_t {
    None = 0,
    Bit,
    Word8,
    Word16,
    Word32,
    Word64,
    Tagged,
    Float64,
};

const char *ToString(MachineRepresentation r);

constexpr bool IsFloatingPoint(MachineRepresentation rep)
{
    return rep == MachineRepresentation::Float64;
}

// Kind of instruction operand
enum class InstructionOperandKind : uint8_t {
    INVALID = 0,
    CONSTANT,
    UNALLOCATED,
    ALLOCATED,
};

const char *ToString(InstructionOperandKind kind);

// =============================================================================
// InstructionOperand - Base class for all instruction operands
// =============================================================================

class InstructionOperand {
public:
    using Kind = InstructionOperandKind;

    InstructionOperand() : value_(0) {}

    explicit InstructionOperand(Kind kind) : value_(KindField::Encode(kind)) {}

    InstructionOperand(const InstructionOperand &) = default;
    InstructionOperand &operator=(const InstructionOperand &) = default;
    InstructionOperand(InstructionOperand &&) = default;
    InstructionOperand &operator=(InstructionOperand &&) = default;

    Kind GetKind() const
    {
        return KindField::Decode(value_);
    }
    uint64_t GetPayload() const
    {
        return value_ & ~KindField::Mask();
    }
    uint64_t GetUnderlyingData() const
    {
        return value_;
    }
    void SetPayload(uint64_t payload)
    {
        value_ = (value_ & KindField::Mask()) | payload;
    }

    bool IsInvalid() const
    {
        return GetKind() == Kind::INVALID;
    }
    bool IsConstant() const
    {
        return GetKind() == Kind::CONSTANT;
    }
    bool IsUnallocated() const
    {
        return GetKind() == Kind::UNALLOCATED;
    }
    bool IsAllocated() const
    {
        return GetKind() == Kind::ALLOCATED;
    }

    bool IsAnyRegister() const;
    bool IsRegister() const;
    bool IsDoubleRegister() const;
    bool IsAnyStackSlot() const;
    bool IsStackSlot() const;
    bool IsFPStackSlot() const;

    bool operator==(const InstructionOperand &other) const
    {
        return value_ == other.value_;
    }
    bool operator!=(const InstructionOperand &other) const
    {
        return !(*this == other);
    }

    std::string Description() const;

protected:
    uint64_t value_ = 0;

    // Kind stored in low 3 bits (pointer alignment guarantees these bits are 0)
    using KindField = panda::BitField<Kind, 0, 3>;

    explicit InstructionOperand(Kind kind, uint64_t payload) : value_(KindField::Encode(kind) | payload) {}
};

// =============================================================================
// INSTRUCTION_OPERAND_CASTS macro - Generate Cast methods for operand subclasses
// =============================================================================
#define INSTRUCTION_OPERAND_CASTS(OperandType, OperandKindValue) \
                                                                 \
    static OperandType *Cast(InstructionOperand *op)             \
    {                                                            \
        ASSERT((op)->GetKind() == OperandKindValue);             \
        return static_cast<OperandType *>(op);                   \
    }                                                            \
                                                                 \
    static const OperandType *Cast(const InstructionOperand *op) \
    {                                                            \
        ASSERT((op)->GetKind() == OperandKindValue);             \
        return static_cast<const OperandType *>(op);             \
    }                                                            \
                                                                 \
    static OperandType Cast(const InstructionOperand &op)        \
    {                                                            \
        ASSERT(op.GetKind() == OperandKindValue);                \
        return *static_cast<const OperandType *>(&op);           \
    }

// =============================================================================
// UnallocatedState - Place-holder operand created before register allocation
// =============================================================================

class UnallocatedState : public InstructionOperand {
public:
    enum class BasicPolicy : uint8_t { FIXED_SLOT, EXTENDED_POLICY };
    enum class ExtendedPolicy : uint8_t {
        NONE = 0,
        FIXED_REGISTER,
        FIXED_FP_REGISTER,
        MUST_HAVE_REGISTER,
        MUST_HAVE_SLOT,
        REGISTER_OR_SLOT,
        REGISTER_OR_SLOT_OR_CONSTANT,
        SAME_AS_INPUT,
    };
    enum class LifetimeFlag : uint8_t { USED_AT_START, USED_AT_END };

    explicit UnallocatedState(int virtualRegister) : InstructionOperand(Kind::UNALLOCATED)
    {
        value_ |= VirtualRegisterField::Encode(static_cast<uint32_t>(virtualRegister));
    }

    explicit UnallocatedState(ExtendedPolicy policy, int virtualRegister) : UnallocatedState(virtualRegister)
    {
        value_ |= BasicPolicyField::Encode(BasicPolicy::EXTENDED_POLICY);
        value_ |= ExtendedPolicyField::Encode(policy);
        value_ |= LifetimeField::Encode(static_cast<unsigned>(LifetimeFlag::USED_AT_END));
    }

    UnallocatedState(BasicPolicy policy, int index) : UnallocatedState(0)
    {
        ASSERT(policy == BasicPolicy::FIXED_SLOT);
        value_ |= BasicPolicyField::Encode(policy);
        // FIXED_SLOT carries a signed stack-slot index, so encode it manually
        // instead of relying on the generic BitField helpers.
        value_ |= static_cast<uint64_t>(static_cast<int64_t>(index)) << FixedSlotIndexField::START_BIT;
        ASSERT(GetFixedSlotIndex() == index);
    }

    UnallocatedState(ExtendedPolicy policy, int index, int virtualRegister) : UnallocatedState(virtualRegister)
    {
        value_ |= BasicPolicyField::Encode(BasicPolicy::EXTENDED_POLICY);
        value_ |= ExtendedPolicyField::Encode(policy);
        value_ |= LifetimeField::Encode(static_cast<unsigned>(LifetimeFlag::USED_AT_END));
        value_ |= FixedRegisterField::Encode(static_cast<uint32_t>(index));
    }

    explicit UnallocatedState(int virtualRegister, int inputIndex) : UnallocatedState(virtualRegister)
    {
        value_ |= BasicPolicyField::Encode(BasicPolicy::EXTENDED_POLICY);
        value_ |= ExtendedPolicyField::Encode(ExtendedPolicy::SAME_AS_INPUT);
        value_ |= LifetimeField::Encode(static_cast<unsigned>(LifetimeFlag::USED_AT_END));
        value_ |= InputIndexField::Encode(static_cast<uint32_t>(inputIndex));
    }

    UnallocatedState(ExtendedPolicy policy, LifetimeFlag lifetime, int virtualRegister)
        : UnallocatedState(virtualRegister)
    {
        value_ |= BasicPolicyField::Encode(BasicPolicy::EXTENDED_POLICY);
        value_ |= ExtendedPolicyField::Encode(policy);
        value_ |= LifetimeField::Encode(static_cast<uint8_t>(lifetime));
    }

    ExtendedPolicy GetExtendedPolicy() const
    {
        ASSERT(BasicPolicyField::Decode(value_) == BasicPolicy::EXTENDED_POLICY);
        return ExtendedPolicyField::Decode(value_);
    }

    uint32_t GetFixedRegisterIndex() const
    {
        ASSERT(HasFixedRegisterPolicy() || HasFixedFPRegisterPolicy());
        return FixedRegisterField::Decode(value_);
    }

    uint32_t GetInputIndex() const
    {
        ASSERT(HasSameAsInputPolicy());
        return InputIndexField::Decode(value_);
    }

    uint32_t GetVirtualRegister() const
    {
        return VirtualRegisterField::Decode(value_);
    }

    bool HasSameAsInputPolicy() const
    {
        return BasicPolicyField::Decode(value_) == BasicPolicy::EXTENDED_POLICY &&
               ExtendedPolicyField::Decode(value_) == ExtendedPolicy::SAME_AS_INPUT;
    }

    bool HasFixedRegisterPolicy() const
    {
        return BasicPolicyField::Decode(value_) == BasicPolicy::EXTENDED_POLICY &&
               ExtendedPolicyField::Decode(value_) == ExtendedPolicy::FIXED_REGISTER;
    }

    bool HasFixedFPRegisterPolicy() const
    {
        return BasicPolicyField::Decode(value_) == BasicPolicy::EXTENDED_POLICY &&
               ExtendedPolicyField::Decode(value_) == ExtendedPolicy::FIXED_FP_REGISTER;
    }

    bool IsUsedAtStart() const
    {
        return LifetimeField::Decode(value_) == static_cast<unsigned>(LifetimeFlag::USED_AT_START);
    }

    bool IsFixedSlotPolicy() const
    {
        return BasicPolicyField::Decode(value_) == BasicPolicy::FIXED_SLOT;
    }

    BasicPolicy GetBasicPolicy() const
    {
        return BasicPolicyField::Decode(value_);
    }

    int32_t GetFixedSlotIndex() const
    {
        ASSERT(IsFixedSlotPolicy());
        return static_cast<int32_t>(static_cast<int64_t>(value_) >> FixedSlotIndexField::START_BIT);
    }

    std::string Description() const;

    INSTRUCTION_OPERAND_CASTS(UnallocatedState, Kind::UNALLOCATED)

private:
    using VirtualRegisterField = KindField::NextField<uint32_t, 32>;  // 32: uint32_t bit width
    using BasicPolicyField = VirtualRegisterField::NextField<BasicPolicy, 1>;
    using ExtendedPolicyField = BasicPolicyField::NextField<ExtendedPolicy, 3>;  // 3: ExtendedPolicy bit width
    using LifetimeField = ExtendedPolicyField::NextField<uint8_t, 1>;
    using FixedRegisterField = LifetimeField::NextField<uint32_t, 6>;  // 6: FixedRegister bit width
    using InputIndexField = FixedRegisterField::NextField<uint32_t, 3>;  // 3: InputIndex bit width
    using FixedSlotIndexField = BasicPolicyField::NextField<int32_t, 28>;  // 28: remaining bits for slot index
};

// =============================================================================
// ConstantOperand - Operand for constant values
// =============================================================================

class ConstantOperand : public InstructionOperand {
public:
    ConstantOperand() : InstructionOperand(Kind::CONSTANT) {}

    explicit ConstantOperand(VertexId vreg) : InstructionOperand(Kind::CONSTANT)
    {
        value_ |= VirtualRegisterField::Encode(vreg);
    }

    VertexId GetVirtualRegister() const
    {
        return VirtualRegisterField::Decode(value_);
    }

    std::string Description() const;

    INSTRUCTION_OPERAND_CASTS(ConstantOperand, Kind::CONSTANT)

private:
    using VirtualRegisterField = KindField::NextField<VertexId, 32>;  // 32: VertexId bit width
};

// =============================================================================
// LocationState - Base class for allocated resources (registers/stack slots)
// =============================================================================

class LocationState : public InstructionOperand {
public:
    enum LocationKind : uint8_t {
        REGISTER = 0,
        STACK_SLOT,
    };

    LocationState() : InstructionOperand(Kind::ALLOCATED) {}

    LocationState(Kind kind, LocationKind locationKind, MachineRepresentation rep, int32_t index)
    {
        ASSERT(kind == Kind::ALLOCATED);
        value_ |= KindField::Encode(kind);
        value_ |= LocationKindField::Encode(locationKind);
        value_ |= RepresentationField::Encode(rep);
        value_ |= static_cast<uint64_t>(static_cast<int64_t>(index)) << IndexField::START_BIT;
        ASSERT(GetIndex() == index);
    }

    LocationKind GetLocationKind() const
    {
        return LocationKindField::Decode(value_);
    }
    MachineRepresentation GetRepresentation() const
    {
        return RepresentationField::Decode(value_);
    }
    int32_t GetIndex() const
    {
        return static_cast<int32_t>(static_cast<int64_t>(value_) >> IndexField::START_BIT);
    }
    uint32_t GetRegisterCode() const
    {
        return static_cast<uint32_t>(GetIndex());
    }

    ArkSteedRegister GetRegister() const
    {
        ASSERT(IsRegister());
        return ArkSteedRegister::FromCode(GetRegisterCode());
    }

    ArkSteedDoubleRegister GetDoubleRegister() const
    {
        ASSERT(IsDoubleRegister());
        return ArkSteedDoubleRegister::FromCode(GetRegisterCode());
    }

    static bool IsSupportedRepresentation(MachineRepresentation rep)
    {
        return rep == MachineRepresentation::Word32 || rep == MachineRepresentation::Word64 ||
               rep == MachineRepresentation::Float64;
    }

    INSTRUCTION_OPERAND_CASTS(LocationState, InstructionOperand::Kind::ALLOCATED)

protected:
    using LocationKindField = KindField::NextField<LocationKind, 1>;
    // 8: MachineRepresentation bit width
    using RepresentationField = LocationKindField::NextField<MachineRepresentation, 8>;
    static_assert(RepresentationField::END_BIT <= 32);  // 32: total bits in a 32-bit word
    using IndexField = panda::BitField<int32_t, 32, 32>;  // 32: start bit; 32: int32_t bit width
};

// =============================================================================
// InstructionOperand inline method implementations
// =============================================================================

inline bool InstructionOperand::IsAnyRegister() const
{
    if (!IsAllocated()) {
        return false;
    }
    return LocationState::Cast(*this).GetLocationKind() == LocationState::LocationKind::REGISTER;
}

inline bool InstructionOperand::IsRegister() const
{
    if (!IsAnyRegister()) {
        return false;
    }
    return !IsFloatingPoint(LocationState::Cast(*this).GetRepresentation());
}

inline bool InstructionOperand::IsDoubleRegister() const
{
    if (!IsAnyRegister()) {
        return false;
    }
    return LocationState::Cast(*this).GetRepresentation() == MachineRepresentation::Float64;
}

inline bool InstructionOperand::IsAnyStackSlot() const
{
    if (!IsAllocated()) {
        return false;
    }
    return LocationState::Cast(*this).GetLocationKind() == LocationState::LocationKind::STACK_SLOT;
}

inline bool InstructionOperand::IsStackSlot() const
{
    if (!IsAnyStackSlot()) {
        return false;
    }
    return !IsFloatingPoint(LocationState::Cast(*this).GetRepresentation());
}

inline bool InstructionOperand::IsFPStackSlot() const
{
    if (!IsAnyStackSlot()) {
        return false;
    }
    return IsFloatingPoint(LocationState::Cast(*this).GetRepresentation());
}

// =============================================================================
// AllocatedState - Register or stack slot assigned by register allocator
// =============================================================================

class AllocatedState : public LocationState {
public:
    AllocatedState() : LocationState() {}

    AllocatedState(LocationKind kind, MachineRepresentation rep, int32_t index)
        : LocationState(Kind::ALLOCATED, kind, rep, index)
    {}

    // Constructor for register allocation
    explicit AllocatedState(LocationKind kind, MachineRepresentation rep, uint32_t regCode)
        : LocationState(Kind::ALLOCATED, kind, rep, static_cast<int32_t>(regCode))
    {}

    std::string Description() const;

    INSTRUCTION_OPERAND_CASTS(AllocatedState, InstructionOperand::Kind::ALLOCATED)
};

// =============================================================================
// ValueLocation - Stores location info for a value
// =============================================================================

class ValueLocation {
public:
    ValueLocation() = default;

    InstructionOperand &GetOperand()
    {
        return operand_;
    }
    const InstructionOperand &GetOperand() const
    {
        return operand_;
    }

    template <typename... Args>
    void SetUnallocated(Args &&...args)
    {
        operand_ = UnallocatedState(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void SetAllocated(Args &&...args)
    {
        operand_ = AllocatedState(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void SetConstant(Args &&...args)
    {
        operand_ = ConstantOperand(std::forward<Args>(args)...);
    }

    void InjectLocation(InstructionOperand location)
    {
        operand_ = location;
    }

    ArkSteedRegister GetAssignedGeneralRegister() const
    {
        ASSERT(!IsDoubleRegister());
        return AllocatedState::Cast(operand_).GetRegister();
    }

    ArkSteedDoubleRegister GetAssignedDoubleRegister() const
    {
        ASSERT(IsDoubleRegister());
        return AllocatedState::Cast(operand_).GetDoubleRegister();
    }

    bool IsAnyRegister() const
    {
        return operand_.IsAnyRegister();
    }
    bool IsRegister() const
    {
        return operand_.IsRegister();
    }
    bool IsDoubleRegister() const
    {
        return operand_.IsDoubleRegister();
    }
    bool IsStackSlot() const
    {
        return operand_.IsStackSlot();
    }
    bool IsFPStackSlot() const
    {
        return operand_.IsFPStackSlot();
    }

    bool IsConstant() const
    {
        return operand_.IsConstant();
    }
    bool IsUnallocated() const
    {
        return operand_.IsUnallocated();
    }
    bool IsInvalid() const
    {
        return operand_.IsInvalid();
    }
    bool IsAllocated() const
    {
        return operand_.IsAllocated();
    }

    bool operator==(const ValueLocation &other) const
    {
        return operand_ == other.operand_;
    }
    bool operator!=(const ValueLocation &other) const
    {
        return !(*this == other);
    }

    bool IsClobbered() const
    {
        ASSERT(IsUnallocated());
        return UnallocatedState::Cast(operand_).IsUsedAtStart();
    }

private:
    InstructionOperand operand_;
};

// =============================================================================
// InputLocation - Extended location info with next use tracking
// =============================================================================

class InputLocation : public ValueLocation {
public:
    InputLocation() = default;

    VertexId GetNextUseId() const
    {
        return nextUseId_;
    }
    void SetNextUseId(VertexId id)
    {
        nextUseId_ = id;
    }
    VertexId *GetNextUseIdAddress()
    {
        return &nextUseId_;
    }

private:
    VertexId nextUseId_ = INVALID_VERTEX_ID;
};

// =============================================================================
// LiveRange - Represents the liveness range of a value
// =============================================================================

struct LiveRange {
    VertexId start = INVALID_VERTEX_ID;
    VertexId end = INVALID_VERTEX_ID;
    bool IsValid() const
    {
        return start != INVALID_VERTEX_ID;
    }
};

// =============================================================================
// GetAllocatableList - Get platform-specific allocatable register lists
// =============================================================================

constexpr ArkSteedRegList GetAllocatableGeneralRegisters()
{
#if defined(PANDA_TARGET_ARM64)
    // Allocatable registers: x0-x15, x19-x21, x23-x28 (25 registers)
    // Excluded: x16-x17 (scratch), x18 (platform), x22 (currently kept live across the
    //           compiled code call by SteedCallAndPushArgv trampoline epilogue),
    //           x29-x31 (fp/lr/sp).
    // Keep this exclusion list in sync with kScratchRegister/kScratchRegister2 in
    // arch/arm64/arksteed_assembler_arm64-inl.h.
    // to do: Once ArkSteed generated functions save/restore their full callee-saved set, x22 can be reconsidered as
    //        allocatable.
    return ArkSteedRegList{aarch64::x0,  aarch64::x1,  aarch64::x2,  aarch64::x3,  aarch64::x4,
                           aarch64::x5,  aarch64::x6,  aarch64::x7,  aarch64::x8,  aarch64::x9,
                           aarch64::x10, aarch64::x11, aarch64::x12, aarch64::x13, aarch64::x14,
                           aarch64::x15, aarch64::x19, aarch64::x20, aarch64::x21, aarch64::x23,
                           aarch64::x24, aarch64::x25, aarch64::x26, aarch64::x27, aarch64::x28};
#elif defined(PANDA_TARGET_AMD64)
    // Allocatable registers: rax, rbx, rcx, rdx, rsi, rdi, r8, r9, r11, r12 (10 registers)
    // Excluded: rsp, rbp, r10 (scratch), r13 (argc), r14 (currently kept live across the compiled code call by
    // SteedCallAndPushArgv trampoline epilogue), r15 (callee-saved; the asm interpreter keeps it live across the
    // compiled call).
    // to do: Once ArkSteed generated functions save/restore their full callee-saved set, r14/r15 can be reconsidered as
    //        allocatable.
    return ArkSteedRegList{x64::rax,
                           x64::rbx,
                           x64::rcx,
                           x64::rdx,
                           x64::rsi,
                           x64::rdi,
                           x64::r8,
                           x64::r9,
                           x64::r11,
                           x64::r12};
#endif
}

constexpr ArkDoubleRegList GetAllocatableDoubleRegisters()
{
#if defined(PANDA_TARGET_ARM64)
    // Allocatable: d0-d29 (30 registers)
    // Reserved: d30-d31 (scratch)
    // Keep this exclusion list in sync with
    // kScratchDoubleRegister/kScratchDoubleRegister2 in
    // arch/arm64/arksteed_assembler_arm64-inl.h.
    return ArkDoubleRegList{
        aarch64::d0, aarch64::d1, aarch64::d2, aarch64::d3, aarch64::d4,
        aarch64::d5, aarch64::d6, aarch64::d7, aarch64::d8, aarch64::d9,
        aarch64::d10, aarch64::d11, aarch64::d12, aarch64::d13, aarch64::d14,
        aarch64::d15, aarch64::d16, aarch64::d17, aarch64::d18, aarch64::d19,
        aarch64::d20, aarch64::d21, aarch64::d22, aarch64::d23, aarch64::d24,
        aarch64::d25, aarch64::d26, aarch64::d27, aarch64::d28, aarch64::d29
    };
#elif defined(PANDA_TARGET_AMD64)
    // Allocatable: xmm0-xmm14 (15 registers)
    // Reserved: xmm15 (scratch)
    return ArkDoubleRegList{x64::xmm0,
                            x64::xmm1,
                            x64::xmm2,
                            x64::xmm3,
                            x64::xmm4,
                            x64::xmm5,
                            x64::xmm6,
                            x64::xmm7,
                            x64::xmm8,
                            x64::xmm9,
                            x64::xmm10,
                            x64::xmm11,
                            x64::xmm12,
                            x64::xmm13,
                            x64::xmm14};
#endif
}

template <typename RegisterT>
constexpr RegListBase<RegisterT> GetAllocatableList();

template <>
constexpr ArkSteedRegList GetAllocatableList<ArkSteedRegister>()
{
    return GetAllocatableGeneralRegisters();
}

template <>
constexpr ArkDoubleRegList GetAllocatableList<ArkSteedDoubleRegister>()
{
    return GetAllocatableDoubleRegisters();
}
}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_TYPES_H
