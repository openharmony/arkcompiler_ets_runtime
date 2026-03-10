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

#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_AARCH64_REGISTER_AARCH64_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_AARCH64_REGISTER_AARCH64_H

#include "ecmascript/compiler/assembler/register_base.h"
#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64_constants.h"

namespace panda::ecmascript::aarch64 {

// Register code numbers (0-31)
#define GENERAL_REGISTER_CODE_LIST(R)               \
    R(0)  R(1)  R(2)  R(3)  R(4)  R(5)  R(6)  R(7)  \
    R(8)  R(9)  R(10) R(11) R(12) R(13) R(14) R(15) \
    R(16) R(17) R(18) R(19) R(20) R(21) R(22) R(23) \
    R(24) R(25) R(26) R(27) R(28) R(29) R(30) R(31)

// Register code enum
enum RegisterCode : int8_t {
#define REGISTER_CODE(N) REG_CODE_##N,
    GENERAL_REGISTER_CODE_LIST(REGISTER_CODE)
#undef REGISTER_CODE
    REG_AFTER_LAST
};

// Register type
enum CPURegType : uint8_t {
    REGISTER = 0,
    V_REGISTER = 1,
    INVALID_REGISTER = 2,
};

// -----------------------------------------------------------------------------
// CPURegister - Base class for all registers
// -----------------------------------------------------------------------------
class CPURegister : public RegisterBase<CPURegister, REG_AFTER_LAST> {
public:
    static constexpr CPURegister Invalid()
    {
        return CPURegister(CODE_INVALID_REG, 0, INVALID_REGISTER);
    }

    static constexpr CPURegister Create(int code, int size, CPURegType type)
    {
        return CPURegister(code, size, type);
    }

    CPURegType GetType() const
    {
        return regType_;
    }

    int SizeInBits() const
    {
        return regSize_;
    }

    int SizeInBytes() const
    {
        return regSize_ / 8; // 8: bits per byte
    }

    bool Is8Bits() const
    {
        return regSize_ == B_REG_SIZE;
    }

    bool Is16Bits() const
    {
        return regSize_ == H_REG_SIZE;
    }

    bool Is32Bits() const
    {
        return regSize_ == S_REG_SIZE || regSize_ == W_REG_SIZE;
    }

    bool Is64Bits() const
    {
        return regSize_ == D_REG_SIZE || regSize_ == X_REG_SIZE;
    }

    bool Is128Bits() const
    {
        return regSize_ == Q_REG_SIZE;
    }

    bool IsNone() const
    {
        return regType_ == INVALID_REGISTER;
    }

    bool IsRegister() const
    {
        return regType_ == REGISTER;
    }

    bool IsVRegister() const
    {
        return regType_ == V_REGISTER;
    }

    bool IsFPRegister() const
    {
        return IsVRegister() && (regSize_ == S_REG_SIZE || regSize_ == D_REG_SIZE);
    }

    bool IsX() const
    {
        return IsRegister() && Is64Bits();
    }

    bool IsW() const
    {
        return IsRegister() && Is32Bits();
    }

    bool IsV() const
    {
        return IsVRegister();
    }

    bool IsB() const
    {
        return IsVRegister() && Is8Bits();
    }

    bool IsH() const
    {
        return IsVRegister() && Is16Bits();
    }

    bool IsS() const
    {
        return IsVRegister() && Is32Bits();
    }

    bool IsD() const
    {
        return IsVRegister() && Is64Bits();
    }

    bool IsQ() const
    {
        return IsVRegister() && Is128Bits();
    }

    bool IsSameSizeAndType(const CPURegister& other) const
    {
        return regSize_ == other.regSize_ && regType_ == other.regType_;
    }

    constexpr bool operator==(const CPURegister& other) const
    {
        return RegisterBase::operator==(other) && regSize_ == other.regSize_ && regType_ == other.regType_;
    }

    constexpr bool operator!=(const CPURegister& other) const
    {
        return !(*this == other);
    }

protected:
    uint8_t regSize_;
    CPURegType regType_;

    friend class RegisterBase<CPURegister, REG_AFTER_LAST>;
    constexpr CPURegister(int code, int size, CPURegType type)
        : RegisterBase(code), regSize_(size), regType_(type) {}
};

// -----------------------------------------------------------------------------
// Register - General purpose register (X/W)
// -----------------------------------------------------------------------------
class Register : public CPURegister {
public:
    static constexpr Register Invalid()
    {
        return Register(CPURegister::Invalid());
    }

    static constexpr Register Create(int code, int size)
    {
        return Register(CPURegister::Create(code, size, REGISTER));
    }

    static constexpr Register FromCode(int8_t code)
    {
        return Register(CPURegister::Create(code, X_REG_SIZE, REGISTER));
    }

    // Create register with W (32-bit) mode
    Register W() const
    {
        return Register(CPURegister::Create(Code(), W_REG_SIZE, REGISTER));
    }

    // Create register with X (64-bit) mode
    Register X() const
    {
        return Register(CPURegister::Create(Code(), X_REG_SIZE, REGISTER));
    }

    inline bool IsSp() const
    {
        return Code() == REG_AFTER_LAST - 1;  // SP is x31
    }

    inline uint8_t GetId() const
    {
        return static_cast<uint8_t>(Code());
    }

private:
    friend class CPURegister;
    explicit constexpr Register(const CPURegister& r) : CPURegister(r) {}
};

// -----------------------------------------------------------------------------
// VRegister - Vector register with lane count support
// -----------------------------------------------------------------------------
class VRegister : public CPURegister {
public:
    static constexpr VRegister Invalid()
    {
        return VRegister(CPURegister::Invalid(), 1);
    }

    static constexpr VRegister Create(int code, int size, int laneCount = 1)
    {
        return VRegister(CPURegister::Create(code, size, V_REGISTER), laneCount);
    }

    int GetLaneCount() const
    {
        return laneCount_;
    }

    inline uint8_t GetId() const
    {
        return static_cast<uint8_t>(Code());
    }

private:
    int laneCount_;

    friend class CPURegister;
    explicit constexpr VRegister(const CPURegister& r, int laneCount)
        : CPURegister(r), laneCount_(laneCount) {}
};

// Type aliases
using FloatRegister = VRegister;
using DoubleRegister = VRegister;
using Simd128Register = VRegister;
using CPURegisterBase = CPURegister;

// Predefined general purpose registers
#define DEFINE_REGISTER(N)                                         \
    constexpr Register w##N(Register::Create(N, W_REG_SIZE));      \
    constexpr Register x##N(Register::Create(N, X_REG_SIZE));

GENERAL_REGISTER_CODE_LIST(DEFINE_REGISTER)
#undef DEFINE_REGISTER

// Special registers
constexpr Register sp = x31;
constexpr Register wzr = w31;  // Zero register (32-bit)
constexpr Register xzr = x31; // Zero register (64-bit)
constexpr Register fp = x29;    // Frame pointer
constexpr Register lr = x30;    // Link register
constexpr Register invalidReg = Register::Invalid();

// Predefined vector registers (b0-b31, h0-h31, s0-s31, d0-d31, q0-q31, v0-v31)
#define DEFINE_VREGISTER(N)                                     \
    constexpr VRegister b##N(VRegister::Create(N, B_REG_SIZE)); \
    constexpr VRegister h##N(VRegister::Create(N, H_REG_SIZE)); \
    constexpr VRegister s##N(VRegister::Create(N, S_REG_SIZE)); \
    constexpr VRegister d##N(VRegister::Create(N, D_REG_SIZE)); \
    constexpr VRegister q##N(VRegister::Create(N, Q_REG_SIZE)); \
    constexpr VRegister v##N(q##N);

GENERAL_REGISTER_CODE_LIST(DEFINE_VREGISTER)
#undef DEFINE_VREGISTER

constexpr VRegister invalidVReg = VRegister::Invalid();

}  // namespace panda::ecmascript::aarch64

#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_AARCH64_REGISTER_AARCH64_H
