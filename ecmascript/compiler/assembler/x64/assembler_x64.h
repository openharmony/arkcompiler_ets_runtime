/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_X64_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_X64_H

#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/assembler/x64/register_x64.h"

namespace panda::ecmascript::x64 {

// Scale enum for memory addressing (kept from original)
enum Scale : uint8_t {
    Times1 = 0,
    Times2,
    Times4,
    Times8
};

class Immediate {
public:
    Immediate(int32_t value) : value_(value) {}
    ~Immediate() = default;

    int32_t Value() const
    {
        return value_;
    }
private:
    int32_t value_;
};

class Operand {
public:
    Operand(Register base, int32_t disp);
    Operand(Register base, Register index, Scale scale, int32_t disp);
    Operand(Register index, Scale scale, int32_t disp);
    ~Operand() = default;

    void BuildSIB(Scale scale, Register index, Register base);
    void BuildModerm(int32_t mode, Register rm);
    void BuildDisp8(int32_t disp);
    void BuildDisp32(int32_t disp);
    int32_t disp_ = 0;
    uint8_t rex_ = 0;
    uint8_t sib_ = 0;
    uint8_t moderm_ = 0;
    bool hasSIB_ = false;
    bool hasDisp8_ = false;
    bool hasDisp32_ = false;
};

// The Intel 64 instruction format is:
// | prefixs| opcode| modR/M| SIB| Displacement| Immediate|
class AssemblerX64 : public Assembler {
public:
    explicit AssemblerX64(Chunk *chunk)
        : Assembler(chunk) {}
    ~AssemblerX64() = default;

    void Pushq(Register x);
    void Pushq(Immediate x);
    void Push(Register x);
    void Popq(Register x);
    void Pop(Register x);
    void Movq(Register src, Register dst);
    void Movq(const Operand &src, Register dst);
    void Movq(Register src, const Operand &dst);
    void Movq(Immediate src, Operand dst);
    void Movq(Immediate src, Register dst);
    void Mov(const Operand &src, Register dst);
    void Mov(Register src, Register dst);
    void Addq(Immediate src, Register dst);
    void Addq(Register src, Register dst);
    void Addl(Immediate src, Register dst);
    void Subq(Immediate src, Register dst);
    void Subq(Register src, Register dst);
    void Subl(Immediate src, Register dst);
    void Cmpq(Immediate src, Register dst);
    void Cmpq(Register src, Register dst);
    void Cmpl(Immediate src, Register dst);
    void Cmpb(Immediate src, Register dst);
    void Cmp(Immediate src, Register dst);
    void Callq(Register addr);
    void Callq(Label *target);
    void Ret();
    void Jmp(Label *target, Distance distance = Distance::Far);
    void Jmp(Register dst);
    void Jmp(Immediate offset);
    void Bind(Label* target);
    void Align16();

    void Andq(Immediate src, Register dst);
    void Andl(Immediate src, Register dst);
    void And(Register src, Register dst);
    void Or(Immediate src, Register dst);
    void Orq(Register src, Register dst);
    void Btq(Immediate src, Register dst);
    void Btl(Immediate src, Register dst);
    void Cmpl(Register src, Register dst);
    void CMovbe(Register src, Register dst);
    void Ja(Label *target, Distance distance = Distance::Far);
    void Jb(Label *target, Distance distance = Distance::Far);
    void Jz(Label *target, Distance distance = Distance::Far);
    void Je(Label *target, Distance distance = Distance::Far);
    void Jg(Label *target, Distance distance = Distance::Far);
    void Jge(Label *target, Distance distance = Distance::Far);
    void Jne(Label *target, Distance distance = Distance::Far);
    void Jbe(Label *target, Distance distance = Distance::Far);
    void Jnz(Label *target, Distance distance = Distance::Far);
    void Jle(Label *target, Distance distance = Distance::Far);
    void Jae(Label *target, Distance distance = Distance::Far);
    void Jnb(Label *target, Distance distance = Distance::Far);
    void Leaq(const Operand &src, Register dst);
    void Leal(const Operand &src, Register dst);
    void Movl(Register src, Register dst);
    void Movl(const Operand &src, Register dst);
    void Movl(Register dst, const Operand& src);
    void Movzbq(const Operand &src, Register dst);
    void Movzbl(const Operand &src, Register dst);
    void Movzbl(Register src, Register dst);
    void Movabs(uint64_t src, Register dst);
    void Shrq(Immediate src, Register dst);
    void Shrl(Immediate src, Register dst);
    void Shr(Immediate src, Register dst);
    void Shll(Immediate src, Register dst);
    void Shlq(Immediate src, Register dst);
    void Btsl(Register src, Register dst);
    void Testq(Immediate src, Register dst);
    void Testb(Immediate src, Register dst);
    void Int3();
    void Movzwq(const Operand &src, Register dst);

private:
    void EmitRexPrefix(const Register &x)
    {
        if (x.HighBit() != 0) {
            EmitU8(REX_PREFIX_B);
        }
    }

    void EmitRexPrefixW(const Register &rm)
    {
        EmitU8(REX_PREFIX_W | rm.HighBit());
    }

    void EmitRexPrefixL(const Register &rm)
    {
        EmitU8(REX_PREFIX_FIXED_BITS | rm.HighBit());
    }

    void EmitRexPrefix(const Operand &rm)
    {
        // 0: Extension to the MODRM.rm field B
        EmitU8(REX_PREFIX_W | rm.rex_);
    }

    void EmitRexPrefix(Register reg, Register rm)
    {
        // 0: Extension to the MODRM.rm field B
        // 2: Extension to the MODRM.reg field R
        EmitU8(REX_PREFIX_W | (reg.HighBit() << 2) | rm.HighBit());
    }

    void EmitRexPrefixl(Register reg, Register rm)
    {
        // 0: Extension to the MODRM.rm field B
        if (reg.HighBit() != 0 || rm.HighBit() != 0) {
            // 2: Extension to the MODRM.reg field R
            EmitU8(REX_PREFIX_FIXED_BITS | (reg.HighBit() << 2) | rm.HighBit());
        }
    }

    void EmitRexPrefix(Register reg, Operand rm)
    {
        // 0: Extension to the MODRM.rm field B
        // 2: Extension to the MODRM.reg field R
        EmitU8(REX_PREFIX_W | (reg.HighBit() << 2) | rm.rex_);
    }

    void EmitRexPrefixl(Register reg, Operand rm)
    {
        // 0: Extension to the MODRM.rm field B
        if (reg.HighBit() != 0 || rm.rex_ != 0) {
            // 2: Extension to the MODRM.reg field R
            EmitU8(REX_PREFIX_FIXED_BITS | (reg.HighBit() << 2) | rm.rex_);
        }
    }

    // +---+---+---+---+---+---+---+---+
    // |  mod  |    reg    |     rm    |
    // +---+---+---+---+---+---+---+---+
    void EmitModrm(int32_t reg, Register rm)
    {
        EmitU8(MODE_RM | (static_cast<uint32_t>(reg) << LOW_BITS_SIZE) | rm.LowBits());
    }

    void EmitModrm(Register reg, Register rm)
    {
        EmitModrm(reg.LowBits(), rm);
    }

    void EmitOperand(Register reg, Operand rm)
    {
        EmitOperand(reg.LowBits(), rm);
    }
    void EmitOperand(int32_t reg, Operand rm);
    void EmitJmp(int32_t offset);
    void EmitJa(int32_t offset);
    void EmitJb(int32_t offset);
    void EmitJz(int32_t offset);
    void EmitJne(int32_t offset);
    void EmitJbe(int32_t offset);
    void EmitJnz(int32_t offset);
    void EmitJle(int32_t offset);
    void EmitJae(int32_t offset);
    void EmitJg(int32_t offset);
    void EmitJge(int32_t offset);
    void EmitJe(int32_t offset);
    void EmitCall(int32_t offset);
    void EmitJnb(int32_t offset);
    // +---+---+---+---+---+---+---+---+
    // | 0   1   0   0 | W | R | X | B |
    // +---+---+---+---+---+---+---+---+
    static constexpr uint8_t REX_PREFIX_FIXED_BITS = 0x40;
    static constexpr uint8_t REX_PREFIX_B = 0x41;
    static constexpr uint8_t REX_PREFIX_W = 0x48;
    // b11
    static constexpr uint8_t MODE_RM = 0xC0;
    // low bits: 3, high bit 1
    static constexpr size_t LOW_BITS_SIZE = 3;
    static constexpr size_t LOW_BITS_MASK = (1 << LOW_BITS_SIZE) - 1;

    static constexpr int32_t SIZE_OF_INT8 = static_cast<int32_t>(sizeof(int8_t));
    static constexpr int32_t SIZE_OF_INT32 = static_cast<int32_t>(sizeof(int32_t));

    static uint8_t GetModrm(int32_t mode, Register rm)
    {
        // [r/m]
        // [r/m + disp8]
        // [r/m + disp32]
        // 6: offset of mode
        return (static_cast<uint32_t>(mode) << 6) | rm.LowBits();
    }
    static uint8_t GetModrmRex(Register rm)
    {
        return rm.HighBit();
    }
    // +---+---+---+---+---+---+---+---+
    // | scale |   index   |    base   |
    // +---+---+---+---+---+---+---+---+
    static uint8_t GetSIB(Scale scale, Register index, Register base)
    {
        // 6: offset of scale
        return (static_cast<uint8_t>(scale) << 6) | (index.LowBits() << LOW_BITS_SIZE) |
            base.LowBits();
    }
    static uint8_t GetSIBRex(Register index, Register base)
    {
        return (index.HighBit() << 1) | base.HighBit();
    }
    friend class Operand;
};
}  // panda::ecmascript::x64
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_X64_H
