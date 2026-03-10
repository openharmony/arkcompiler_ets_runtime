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

#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_X64_REGISTER_X64_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_X64_REGISTER_X64_H

#include "ecmascript/compiler/assembler/register_base.h"

namespace panda::ecmascript::x64 {

// General purpose registers
#define GENERAL_REGISTERS(V)                                \
    V(rax) V(rcx) V(rdx) V(rbx) V(rsp) V(rbp) V(rsi) V(rdi) \
    V(r8)  V(r9)  V(r10) V(r11) V(r12) V(r13) V(r14) V(r15)

enum RegisterCode : int8_t {
#define DEFINE_REG_CODE(name) name##_code,
    GENERAL_REGISTERS(DEFINE_REG_CODE)
    REG_AFTER_LAST   // 16
#undef DEFINE_REG_CODE
};

// Register class
class Register : public RegisterBase<Register, REG_AFTER_LAST> {
public:
    constexpr int HighBit() const { return Code() >> 3; }
    constexpr int LowBits() const { return Code() & 0x7; }

    static constexpr Register FromCode(int8_t code) { return Register(code); }
    static constexpr Register Invalid() { return RegisterBase::InvalidReg(); }

    constexpr Register(RegisterCode code) : RegisterBase(static_cast<int8_t>(code)) {}

private:
    friend class RegisterBase<Register, REG_AFTER_LAST>;
    explicit constexpr Register(int8_t code) : RegisterBase(code) {}
};

// Predefined register constants
#define DEFINE_REGISTER(name) \
    constexpr Register name = Register(RegisterCode::name##_code);

GENERAL_REGISTERS(DEFINE_REGISTER)
#undef DEFINE_REGISTER
constexpr Register invalidReg = Register::Invalid();

// Floating-point/SIMD registers (XMM)
#define FP_SIMD_REGISTERS(V)                                               \
    V(xmm0) V(xmm1) V(xmm2)  V(xmm3)  V(xmm4)  V(xmm5)  V(xmm6)  V(xmm7)   \
    V(xmm8) V(xmm9) V(xmm10) V(xmm11) V(xmm12) V(xmm13) V(xmm14) V(xmm15)

enum XMMRegisterCode : int8_t {
#define DEFINE_XMM_REG_CODE(name) name##_code,
    FP_SIMD_REGISTERS(DEFINE_XMM_REG_CODE)
    kXMMRegAfterLast   // 16
#undef DEFINE_XMM_REG_CODE
};

// XMM Register class
class XMMRegister : public RegisterBase<XMMRegister, kXMMRegAfterLast> {
public:
    constexpr int HighBit() const { return Code() >> 3; }
    constexpr int LowBits() const { return Code() & 0x7; }

    static constexpr XMMRegister FromCode(int8_t code) { return XMMRegister(code); }
    static constexpr XMMRegister Invalid() { return RegisterBase::InvalidReg(); }

    constexpr XMMRegister(XMMRegisterCode code) : RegisterBase(static_cast<int8_t>(code)) {}

private:
    friend class RegisterBase<XMMRegister, kXMMRegAfterLast>;
    explicit constexpr XMMRegister(int8_t code) : RegisterBase(code) {}
};

// Type aliases
using FloatRegister = XMMRegister;
using DoubleRegister = XMMRegister;
using Simd128Register = XMMRegister;

// Predefined XMM register constants
#define DEFINE_FP_REG(name)                                                   \
    constexpr XMMRegister name = XMMRegister(XMMRegisterCode::name##_code);

FP_SIMD_REGISTERS(DEFINE_FP_REG)
#undef DEFINE_FP_REG

constexpr XMMRegister invalidFPReg = XMMRegister::Invalid();

}  // namespace panda::ecmascript::x64

#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_X64_REGISTER_X64_H
