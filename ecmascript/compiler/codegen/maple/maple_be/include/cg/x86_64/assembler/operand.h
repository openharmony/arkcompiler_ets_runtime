/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef X64_ASSEMBLER_OPERAND_H
#define X64_ASSEMBLER_OPERAND_H

#include <unordered_map>
#include <string>
#include <array>
#include "util.h"

namespace assembler {
using ImmOpnd = std::pair<int64, bool>;

/* Use 16 bits to represent a register:
   The high 8 bits is register's size.
   The low 4 bits is register's id.
   The fifth bit from right to left is used to identity register rip, the bit equals 1 represents the RIP register.
   The sixth bit from right to left is used to identity float register, the bit equals 1 represents the float register.
   The eighth bit from right to left is used to determine whether
       it is the high 8-bit register or the lower 8-bit register, the bit equals 1 represents the lower 8-bit register.
  The specific distribution of the 16 bits is shown below:
  +-----------------------------------+-----------+-------+---------+-------+-------------------+
  |  15  14  13  12  11  10   9   8   |     7     |   6   |    5    |   4   |   3   2   1   0   |
  +-----------------------------------+-----------+-------+---------+-------+-------------------+
  |         Reg's size in bits        | H/L8-reg  | unuse | IsFloat | IsRIP |     Reg's id      |
  +-----------------------------------+-----------+-------+---------+-------+-------------------+
  |   0   0   0   0   0   0   0   0   |     0     |   0   |    0    |   0   |   0   0   0   0   |
  +-----------------------------------+-----------+-------+---------+------+--------------------+
*/
enum Reg : uint16 {
    XMM0 = 0x8000,
    XMM1,
    XMM2,
    XMM3,
    XMM4,
    XMM5,
    XMM6,
    XMM7,
    XMM8,
    XMM9,
    XMM10,
    XMM11,
    XMM12,
    XMM13,
    XMM14,
    XMM15,
    MMX0 = 0x4020, MMX1, MMX2, MMX3, MMX4, MMX5, MMX6, MMX7,
    RAX = 0x4000,
    RCX,
    RDX,
    RBX,
    RSP,
    RBP,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
    RIP = 0x4015,
    EAX = 0x2000,
    ECX,
    EDX,
    EBX,
    ESP,
    EBP,
    ESI,
    EDI,
    R8D,
    R9D,
    R10D,
    R11D,
    R12D,
    R13D,
    R14D,
    R15D,
    AX = 0x1000,
    CX,
    DX,
    BX,
    SP,
    BP,
    SI,
    DI,
    R8W,
    R9W,
    R10W,
    R11W,
    R12W,
    R13W,
    R14W,
    R15W,
    AL = 0x0880,
    CL,
    DL,
    BL,
    SPL,
    BPL,
    SIL,
    DIL,
    R8B,
    R9B,
    R10B,
    R11B,
    R12B,
    R13B,
    R14B,
    R15B,
    AH = 0x0804,
    CH,
    DH,
    BH,
    ES = 0,
    CS,
    SS,
    DS,
    FS,
    GS,
    ERR = 0xFFFF
};

static const uint8 kMaxRegNum = 35;
static const uint8 kRegSizeType = 6;
// To implement operating 64/32 bit float with 128bit asm insn,
// 32bit/64bit/128bit regs in kRegArray has done some processes same with openArkCompiler.
static const std::array<std::array<Reg, kMaxRegNum>, kRegSizeType> kRegArray = {
    {{ERR, AL, CL, DL, BL, SPL, BPL, SIL, DIL, R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B},
     {ERR, AH, CH, DH, BH},
     {ERR, AX, CX, DX, BX, SP, BP, SI, DI, R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W},
     {ERR, EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI, R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D, ERR,
      XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15},
     {ERR, RAX,  RCX,  RDX,  RBX,  RSP,  RBP,  RSI,  RDI,  R8,   R9,   R10,   R11,   R12,   R13,   R14,   R15,
      RIP, XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15},
     {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR,
      XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15}}
      };

static const std::unordered_map<Reg, std::string> kRegStrMap = {
    {XMM0, "xmm0"},   {XMM1, "xmm1"},   {XMM2, "xmm2"},   {XMM3, "xmm3"},   {XMM4, "xmm4"},   {XMM5, "xmm5"},
    {XMM6, "xmm6"},   {XMM7, "xmm7"},   {XMM8, "xmm8"},   {XMM9, "xmm9"},   {XMM10, "xmm10"}, {XMM11, "xmm11"},
    {XMM12, "xmm12"}, {XMM13, "xmm13"}, {XMM14, "xmm14"}, {XMM15, "xmm15"},
    {MMX0, "mmx0"}, {MMX1, "mmx1"}, {MMX2, "mmx2"}, {MMX3, "mmx3"}, {MMX4, "mmx4"}, {MMX5, "mmx5"}, {MMX6, "mmx6"},
    {MMX7, "mmx7"},
    {RAX, "rax"},     {RDX, "rdx"},
    {RCX, "rcx"},     {RBX, "rbx"},     {RSP, "rsp"},     {RBP, "rbp"},     {RSI, "rsi"},     {RDI, "rdi"},
    {R8, "r8"},       {R9, "r9"},       {R10, "r10"},     {R11, "r11"},     {R12, "r12"},     {R13, "r13"},
    {R14, "r14"},     {R15, "r15"},     {RIP, "rip"},     {EAX, "eax"},     {ECX, "ecx"},     {EDX, "edx"},
    {EBX, "ebx"},     {ESP, "esp"},     {EBP, "ebp"},     {ESI, "esi"},     {EDI, "edi"},     {R8D, "r8d"},
    {R9D, "r9d"},     {R10D, "r10d"},   {R11D, "r11d"},   {R12D, "r12d"},   {R13D, "r13d"},   {R14D, "r14d"},
    {R15D, "r15d"},   {AX, "ax"},       {CX, "cx"},       {DX, "dx"},       {BX, "bx"},       {SP, "sp"},
    {BP, "bp"},       {SI, "si"},       {DI, "di"},       {R8W, "r8w"},     {R9W, "r9w"},     {R10W, "r10w"},
    {R11W, "r11w"},   {R12W, "r12w"},   {R13W, "r13w"},   {R14W, "r14w"},   {R15W, "r15w"},   {AL, "al"},
    {CL, "cl"},       {DL, "dl"},       {BL, "bl"},       {SPL, "spl"},     {BPL, "bpl"},     {SIL, "sil"},
    {DIL, "dil"},     {R8B, "r8b"},     {R9B, "r9b"},     {R10B, "r10b"},   {R11B, "r11b"},   {R12B, "r12b"},
    {R13B, "r13b"},   {R14B, "r14b"},   {R15B, "r15b"},   {AH, "ah"},       {CH, "ch"},       {DH, "dh"},
    {BH, "bh"},       {ES, "es"},       {CS, "cs"},       {SS, "ss"},       {DS, "ds"},       {FS, "fs"},
    {GS, "gs"},       {ERR, "err"}};

enum MemType {
    kNone,
    kOnlyBase,
    kOnlyDisp,
    kBaseAndDisp,
    kIndexAndDisp, /* If there is a index register in the mem, there must be a scasle. */
    kSIB,          /* SIB is abbreviation of Scale, Index, Base. */
    kSIBAndDisp
};

struct Mem {
    Reg base = ERR;
    Reg index = ERR;
    uint8 s = 0;                           /* scale = log2(s) */
    std::pair<int64, int64> disp = {0, 0}; /* first: symbol id, second: offset */
    uint32 size = 32;
    MemType memType = kNone;

    Mem() {}

    void SetMemType()
    {
        if (base != ERR && index != ERR && (disp.second != 0 || disp.first != 0)) {
            memType = kSIBAndDisp;
        } else if (base != ERR && index != ERR && disp.second == 0 && disp.first == 0) {
            memType = kSIB;
        } else if (base == ERR && index != ERR && (disp.second != 0 || disp.first != 0)) {
            memType = kIndexAndDisp;
        } else if (base != ERR && index == ERR && (disp.second != 0 || disp.first != 0)) {
            memType = kBaseAndDisp;
        } else if (base == ERR && index == ERR && (disp.second != 0 || disp.first != 0)) {
            memType = kOnlyDisp;
        } else if (base != ERR && index == ERR && disp.second == 0 && disp.first == 0) {
            memType = kOnlyBase;
        } else {
            memType = kNone;
        }
    }
}; /* struct Mem */

enum FixupKind {
    kFNone,
    kRelative,
    kRelative64,
    kAbsolute,
    kAbsolute64,
    kPLT,
};

enum class LabelType {
    kLNone,
    kBBLabel,
    kFunc,
    kConst,
    kGlobal,
    kStatic,
    kGlobalUninitialized,
    kLocalUninitialized,
    kStrLabel,
    kJmpLabel,
    kFloatLabel,
    kDoubleLabel,
    /* for debug */
    kDebugStrLabel,
};

class Label {
public:
    Label(int64 index, uint32 relOffsetVal, LabelType type)
        : labelIndex(index), relOffset(relOffsetVal), labelType(type)
    {
    }

    ~Label() = default;

    int64 GetlabelIdx() const
    {
        return labelIndex;
    }

    uint32 GetRelOffset() const
    {
        return relOffset;
    }

    LabelType GetLabelType() const
    {
        return labelType;
    }

    void SetRelOffset(uint32 value)
    {
        relOffset = value;
    }

    void SetLabelType(LabelType type)
    {
        labelType = type;
    }

private:
    int64 labelIndex;    /* target label index */
    uint32 relOffset;    /* record the label's position */
    LabelType labelType; /* target label type */
};                       /* class Label */

class Fixup {
public:
    Fixup(int64 index, FixupKind kind, const std::pair<uint32, size_t> &offset, int64 dispVal)
        : labelIndex(index), fixupKind(kind), offset(offset), disp(dispVal)
    {
    }

    int64 GetlabelIdx() const
    {
        return labelIndex;
    }

    void SetLabelIdx(int64 index)
    {
        labelIndex = index;
    }

    FixupKind GetFixupKind() const
    {
        return fixupKind;
    }

    void SetFixupKind(FixupKind kind)
    {
        fixupKind = kind;
    }

    const std::pair<uint32, size_t> &GetOffset() const
    {
        return offset;
    }

    void SetOffset(const std::pair<uint32, size_t> &fixupOffset)
    {
        offset = fixupOffset;
    }

    int64 GetDisp() const
    {
        return disp;
    }

    void SetDisp(int64 value)
    {
        disp = value;
    }

private:
    int64 labelIndex;                 /* record the label needs to fix up */
    FixupKind fixupKind;              /* record how to fix up */
    std::pair<uint32, uint64> offset; /* record the location and size to fixup,
                                         the first is offset, the second is offset's size */
    int64 disp;                       /* record the symbol's addend for relocation */
};                                    /* class Fixup */
} /* namespace assembler */

#endif /* X64_ASSEMBLER_OPERAND_H */