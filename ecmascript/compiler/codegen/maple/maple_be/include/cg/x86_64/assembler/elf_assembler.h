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

#ifndef ELF_ASSEMBLER_H
#define ELF_ASSEMBLER_H

#include "assembler.h"
#include "elf_file.h"
#include "cg_option.h"

namespace assembler {
const uint8 kLeftShift6Bits = 6;
const uint8 kLeftShift3Bits = 3;
const uint8 kGetLow3Bits = 0b111;
const uint8 kLeftShift32Bits = 32;
const uint8 kLeftShift4Bits = 4;

class ElfAssembler : public Assembler {
public:
    ElfAssembler(const std::string &outputFileName) : Assembler()
    {
        const auto &emitMemoryManager = maplebe::CGOptions::GetInstance().GetEmitMemoryManager();
        if (emitMemoryManager.codeSpace == nullptr) {
            outFStream.open(outputFileName, std::ios::trunc | std::ios::binary);
        }
    }

    ~ElfAssembler() = default;

    /* override function in base class */
    void InitialFileInfo(const std::string &inputFileName) override;
    void EmitVariable(int64 symIdx, uint64 sizeInByte, uint8 alignInByte, SymbolAttr symAttr,
                      SectionKind sectionKind) override;
    void EmitFunctionHeader(int64 symIdx, SymbolAttr funcAttr, const std::string *secName) override;
    void EmitBBLabel(int64 labelSymIdx, bool genVerboseInfo, uint32 freq, const std::string *mirName) override;
    void EmitJmpTableElem(int64 jmpLabelIdx, const std::vector<int64> &labelSymIdxs) override;
    void EmitFunctionFoot(int64 symIdx, SymbolAttr funcAttr) override;
    void EmitDirectString(const std::string &ustr, bool belongsToDataSec, int64 strSymIdx, bool emitAscii) override;
    void EmitIndirectString(int64 strSymIdx, bool belongsToDataSec) override;
    void EmitIntValue(int64 value, size_t valueSize, bool belongsToDataSec) override;
    void EmitFloatValue(int64 symIdx, int64 value, size_t valueSize) override;
    void EmitAddrValue(int64 symIdx, int32 symAddrOfs, int32 structFieldOfs, bool belongsToDataSec) override;
    void EmitAddrOfFuncValue(int64 symIdx, bool belongsToDataSec) override;
    void EmitLabelValue(int64 symIdx, bool belongsToDataSec) override;
    void EmitBitFieldValue(uint64 combineBitFieldValue, bool belongsToDataSec) override;
    void EmitNull(uint64 sizeInByte) override;
    void PostEmitVariable(int64 symIdx, SymbolAttr symAttr, uint64 sizeInByte, bool belongsToTextSec) override;
    void FinalizeFileInfo() override;
    void EmitBssSectionVar(int64 symIdx, uint64 sizeInByte, uint8 alignInByte, SymbolAttr symAttr);
    void EmitDataSectionVar(int64 symIdx);

    /* emit debug info */
    void EmitDIHeader() override {};
    void EmitDIFooter() override {};
    void EmitDIHeaderFileInfo() override {};
    void EmitDIDebugInfoSectionHeader(uint64 debugInfoLength) override;
    void EmitDIDebugInfoSectionAbbrevId(bool verbose, uint32 abbrevId, const std::string &dieTagName, uint32 offset,
                                        uint32 size) override;
    /* nothing to do in ElfAssembler. */
    void EmitDIFormSpecification(unsigned int dwform) override {};
    void EmitDebugComment(const char* comment) override {};

    /* EmitDIAttrValue */
    void EmitDwFormString(const std::string &name) override;
    void EmitDwFormStrp(uint32 strLabelId, size_t strTableSize) override;
    void EmitDwFormData(int32 attrValue, uint8 sizeInByte) override;
    void EmitDwFormData8() override;
    void EmitDwFormData8(uint32 endLabelFuncPuIdx, uint32 startLabelFuncPuIdx, uint32 endLabelIdx,
                         uint32 startLabelIdx) override;
    void EmitLabel(uint32 funcPuIdx, uint32 labIdx) override;
    void EmitDwFormSecOffset() override;
    void EmitDwFormAddr(bool emitTextBegin) override;
    void EmitDwFormRef4(uint64 offsetOrValue, bool unknownType, bool emitOffset) override;
    void EmitDwFormExprlocCfa(uint32 dwOp) override;
    void EmitDwFormExprlocAddr(uint32 dwOp, const std::string &addrStr) override;
    void EmitDwFormExprlocFbreg(uint32 dwOp, int fboffset, size_t sleb128Size) override;
    void EmitDwFormExprlocBregn(uint32 dwOp, const std::string &dwOpName) override; /* n=0~7 */
    void EmitDwFormExprloc(uintptr elp) override;

    void EmitDIDwName(const std::string &dwAtName, const std::string &dwForName) override {};
    void EmitDIDWFormStr(const std::string &formStr) override {};
    void EmitDIDWDataMemberLocaltion(unsigned int lableIdx, uintptr_t attr) override {};
    void EmitDIDebugAbbrevSectionHeader() override {};
    void EmitDIDebugAbbrevDiae(bool verbose, uint32 abbrevId, uint32 tag, const std::string &dwTagName,
                               bool withChildren) override;
    void EmitDIDebugAbbrevDiaePairItem(bool verbose, uint32 aplAt, uint32 aplFrom, const std::string &dwAtName,
                                       const std::string &dwFromName) override;
    void EmitDIDebugSectionEnd(SectionKind secKind) override;
    void EmitDIDebugARangesSection() override
    {
        debugARangesSection = new DataSection(".debug_aranges", SHT_PROGBITS, 0, 1);
        RegisterSection(*debugARangesSection);
    }

    void EmitDIDebugRangesSection() override
    {
        debugRangesSection = new DataSection(".debug_ranges", SHT_PROGBITS, 0, 1);
        RegisterSection(*debugRangesSection);
    }

    void EmitDIDebugLineSection() override {};
    void EmitDIDebugStrSection(const std::vector<uint32> &strps, const std::vector<std::string> &debugStrs, uint64 size,
                               size_t strTableSize) override;
    void EmitLine() override {};
    void HandleDebugInfoSectionFixup();

    const std::vector<uint8> &EncodeULEB128(uint64 value)
    {
        static std::vector<uint8> uleb128Value;
        if (value == 0) {
            uleb128Value.push_back(0);
        }
        int rightShift7Bits = 7;
        while (value != 0) {
            uint8 byte = value & 0x7F;
            value >>= rightShift7Bits;
            byte = value != 0 ? byte |= 0x80 : byte;
            uleb128Value.push_back(byte);
        }
        return uleb128Value;
    }

    const std::vector<uint8> &EncodeSLEB128(int64 value)
    {
        static std::vector<uint8> sleb128Value;
        int more = 1;
        int rightShift7Bits = 7;
        while (more != 0) {
            uint8 byte = static_cast<uint8>(static_cast<uint64>(value) & 0x7F);
            uint8 sign = static_cast<uint8>(static_cast<uint64>(value) & 0x40);
            value = static_cast<uint64>(value) >> rightShift7Bits;
            if ((value == 0 && sign == 0) || (value == -1 && sign != 0)) {
                more = 0;
            } else {
                byte |= 0x80;
            }
            sleb128Value.push_back(byte);
        }
        return sleb128Value;
    }

    /* start of X64 instructions */
    /* mov */
    void Mov(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Mov(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) override;
    void Mov(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Mov(InsnSize insnSize, Reg reg, const Mem &mem) override;
    void Mov(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) override;
    /* floating point mov */
    void Mov(Reg srcReg, Reg destReg, bool isMovD) override;
    void MovF(const Mem &mem, Reg reg, bool isSingle) override;
    void MovF(Reg reg, const Mem &mem, bool isSingle) override;
    /* movabs */
    void Movabs(const ImmOpnd &immOpnd, Reg reg) override;
    void Movabs(int64 symIdx, Reg reg) override;
    /* push */
    void Push(InsnSize insnSize, Reg reg) override;
    /* pop */
    void Pop(InsnSize insnSize, Reg reg) override;
    /* lea */
    void Lea(InsnSize insnSize, const Mem &mem, Reg reg) override;
    /* movzx */
    void MovZx(InsnSize sSize, InsnSize dSize, Reg srcReg, Reg destReg) override;
    void MovZx(InsnSize sSize, InsnSize dSize, const Mem &mem, Reg reg) override;
    /* movsx */
    void MovSx(InsnSize sSize, InsnSize dSize, Reg srcReg, Reg destReg) override;
    void MovSx(InsnSize sSize, InsnSize dSize, const Mem &mem, Reg reg) override;
    /* add */
    void Add(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Add(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) override;
    void Add(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Add(InsnSize insnSize, Reg reg, const Mem &mem) override;
    void Add(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) override;
    /* add floating point */
    void Add(Reg srcReg, Reg destReg, bool isSingle) override;
    void Add(const Mem &mem, Reg reg, bool isSingle) override;
    /* sub */
    void Sub(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Sub(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) override;
    void Sub(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Sub(InsnSize insnSize, Reg reg, const Mem &mem) override;
    void Sub(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) override;
    /* sub floating point */
    void Sub(Reg srcReg, Reg destReg, bool isSingle) override;
    void Sub(const Mem &mem, Reg reg, bool isSingle) override;
    /* and */
    void And(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void And(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void And(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) override;
    void And(InsnSize insnSize, Reg reg, const Mem &mem) override;
    void And(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) override;
    /* or */
    void Or(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Or(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Or(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) override;
    void Or(InsnSize insnSize, Reg reg, const Mem &mem) override;
    void Or(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) override;
    /* xor */
    void Xor(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Xor(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) override;
    void Xor(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Xor(InsnSize insnSize, Reg reg, const Mem &mem) override;
    void Xor(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) override;
    /* bsr */
    void Bsr(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    /* not */
    void Not(InsnSize insnSize, Reg reg) override;
    void Not(InsnSize insnSize, const Mem &mem) override;
    /* neg */
    void Neg(InsnSize insnSize, Reg reg) override;
    void Neg(InsnSize insnSize, const Mem &mem) override;
    /* div & cwd, cdq, cqo */
    void Idiv(InsnSize insnSize, Reg reg) override;
    void Idiv(InsnSize insnSize, const Mem &mem) override;
    void Div(InsnSize insnSize, Reg reg) override;
    void Div(InsnSize insnSize, const Mem &mem) override;
    void Cwd() override;
    void Cdq() override;
    void Cqo() override;
    /* shl */
    void Shl(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Shl(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) override;
    void Shl(InsnSize insnSize, Reg reg, const Mem &mem) override;
    void Shl(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) override;
    /* sar */
    void Sar(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Sar(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) override;
    void Sar(InsnSize insnSize, Reg reg, const Mem &mem) override;
    void Sar(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) override;
    /* shr */
    void Shr(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Shr(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) override;
    void Shr(InsnSize insnSize, Reg reg, const Mem &mem) override;
    void Shr(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) override;
    /* jmp */
    void Jmp(Reg reg) override;
    void Jmp(const Mem &mem) override;
    void Jmp(int64 symIdx) override;
    /* jump condition */
    void Je(int64 symIdx) override;
    void Ja(int64 symIdx) override;
    void Jae(int64 symIdx) override;
    void Jne(int64 symIdx) override;
    void Jb(int64 symIdx) override;
    void Jbe(int64 symIdx) override;
    void Jg(int64 symIdx) override;
    void Jge(int64 symIdx) override;
    void Jl(int64 symIdx) override;
    void Jle(int64 symIdx) override;
    /* cmp */
    void Cmp(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cmp(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Cmp(InsnSize insnSize, Reg reg, const Mem &mem) override;
    void Cmp(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) override;
    void Cmp(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) override;
    /* test */
    void Test(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    /* set */
    void Setbe(Reg reg) override;
    void Setbe(const Mem &mem) override;
    void Setle(Reg reg) override;
    void Setle(const Mem &mem) override;
    void Setae(Reg reg) override;
    void Setae(const Mem &mem) override;
    void Setge(Reg reg) override;
    void Setge(const Mem &mem) override;
    void Setne(Reg reg) override;
    void Setne(const Mem &mem) override;
    void Setb(Reg reg) override;
    void Setb(const Mem &mem) override;
    void Setl(Reg reg) override;
    void Setl(const Mem &mem) override;
    void Seta(Reg reg) override;
    void Seta(const Mem &mem) override;
    void Setg(Reg reg) override;
    void Setg(const Mem &mem) override;
    void Sete(Reg reg) override;
    void Sete(const Mem &mem) override;
    void Seto(Reg reg) override;
    void Seto(const Mem &mem) override;
    /* cmov */
    void Cmova(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cmova(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Cmovae(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cmovae(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Cmovb(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cmovb(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Cmovbe(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cmovbe(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Cmove(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cmove(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Cmovg(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cmovg(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Cmovge(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cmovge(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Cmovl(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cmovl(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Cmovle(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cmovle(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Cmovne(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cmovne(InsnSize insnSize, const Mem &mem, Reg reg) override;
    void Cmovo(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    /* call */
    void Call(InsnSize insnSize, Reg reg) override;
    void Call(InsnSize insnSize, const Mem &mem) override;
    void Call(InsnSize insnSize, int64 symIdx) override;
    /* ret */
    void Ret() override;
    /* leave */
    void Leave() override;
    /* imul */
    void Imul(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    /* mul float */
    void Mul(Reg srcReg, Reg destReg, bool isSingle) override;
    void Mul(const Mem &mem, Reg reg, bool isSingle) override;
    /* nop */
    void Nop(InsnSize insnSize, const Mem &mem) override;
    void Nop() override;
    /* byte swap */
    void Bswap(InsnSize insnSize, Reg reg) override;
    void Xchg(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    /* pseudo insn */
    void DealWithPseudoInst(const std::string &insn) override {}
    /* floating point */
    void MovF(Reg srcReg, Reg destReg, bool isSingle) override;
    /* floating point and */
    void And(Reg srcReg, Reg destReg, bool isSingle) override;
    void And(const Mem &mem, Reg reg, bool isSingle) override;
    /* floating div */
    void Divsd(Reg srcReg, Reg destReg) override;
    void Divsd(const Mem &mem, Reg reg) override;
    /* convert int2float */
    void Cvtsi2ss(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cvtsi2sd(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    /*convert float2int */
    void Cvttsd2si(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    void Cvttss2si(InsnSize insnSize, Reg srcReg, Reg destReg) override;
    /* convert float2float */
    void Cvtss2sd(Reg srcReg, Reg destReg) override;
    void Cvtsd2ss(Reg srcReg, Reg destReg) override;
    /* unordered compare */
    void Ucomisd(Reg srcReg, Reg destReg) override;
    void Ucomiss(Reg srcReg, Reg destReg) override;
    void Cmpsd(Reg srcReg, Reg destReg, uint8 imm);
    void Cmpeqsd(Reg srcReg, Reg destReg) override;
    /* float sqrt*/
    void Sqrtss_r(Reg srcReg, Reg destReg) override;
    void Sqrtsd_r(Reg srcReg, Reg destReg) override;
    /* end of X64 instructions */
    /* process stackmap */
    void RecordStackmap(const std::vector<uint8> &referenceMap,
                        const std::vector<uint8> &deoptVreg2LocationInfo) override;
    uint32 GetCurModulePC() override;
    void SetLastModulePC(uint32 pc) override;

private:
    /* insn encode function */
    void Encodeb(uint64 code)
    {
        codeBuff.push_back(static_cast<uint8>(code));
    }

    void Encodeb(const uint8 *code, size_t codeSize)
    {
        for (size_t i = 0; i < codeSize; i++) {
            Encodeb(code[i]);
        }
    }

    void Encodeb(uint64 code, size_t codeSize)
    {
        for (size_t i = 0; i < codeSize; i++) {
            /* Use little endian for immediate value or displacement. */
            Encodeb(static_cast<uint8>(code >> (i * k8Bits)));
        }
    }

    void Encodew(uint64 code)
    {
        Encodeb(code, k16Bits / k8Bits); /* 2bytes, 16bits */
    }

    void Encoded(uint64 code)
    {
        Encodeb(code, k32Bits / k8Bits); /* 4bytes, 32bits */
    }

    void Encodeq(uint64 code)
    {
        Encodeb(code, k64Bits / k8Bits); /* 8bytes, 64bits */
    }

    void FixupEncode(size_t position, uint32 relOffset, size_t codeSize)
    {
        uint64 code = static_cast<uint64>(relOffset - position - codeSize);
        for (size_t i = 0; i < codeSize; i++) {
            codeBuff[position + i] = static_cast<uint8>(code >> (i * k8Bits));
        }
    }

    uint8 GetRegSize(Reg reg) const
    {
        const uint8 kRegSizeBits = 8; /* The high 8 bits is register's size. */
        return reg >> kRegSizeBits;
    }

    uint8 GetRegId(Reg reg) const
    {
        return static_cast<uint8_t>(reg & 0xF);
    }

    bool IsRegExt(Reg reg) const
    {
        /* 0xF is 4 bits, the lower 4 bits represent the id of the register. */
        uint8 regId = reg & 0xF;
        const uint8 kStartExtId = 8;
        return (regId & kStartExtId) == kStartExtId;
    }

    uint8 GetRegCodeId(Reg reg) const
    {
        /* The lower 3 bits are used to store the encoding id of the register. */
        return reg & kGetLow3Bits;
    }

    bool Need8BitRegPrefix(Reg reg) const
    {
        if (reg == ERR || GetRegSize(reg) != k8Bits) {
            return false;
        }
        uint8 regCodeId = GetRegCodeId(reg);
        /* From right to left, the eighth bit in reg equals 1 represents the lower 8-bit register. */
        const int kEighthBit = 8;
        bool isLow8BitReg = ((((reg & 0xFF) >> (kEighthBit - 1)) & 1) == 1);
        const uint8 kMinHigh8BitRegId = 4;
        const uint8 kMaxHigh8BitRegId = 7;
        return isLow8BitReg && (regCodeId >= kMinHigh8BitRegId && regCodeId <= kMaxHigh8BitRegId);
    }

    bool HasOpndSizePrefix(Reg reg) const
    {
        return GetRegSize(reg) == k16Bits ? true : false;
    }

    bool HasOpndSizePrefix(const Mem &mem) const
    {
        return mem.size == k16Bits ? true : false;
    }

    bool HasAddrSizePrefix(const Mem &mem) const
    {
        Reg reg = mem.base != ERR ? mem.base : (mem.index != ERR ? mem.index : ERR);
        uint8 regSize = reg == ERR ? k64Bits : GetRegSize(reg);
        return regSize == k32Bits ? true : false;
    }

    uint8 GetRex(Reg reg) const
    {
        uint8 rexW = GetRegSize(reg) == k64Bits ? 8 : 0;
        uint8 rexB = IsRegExt(reg) ? 1 : 0;
        uint8 rex = rexW | rexB;
        if (rex || Need8BitRegPrefix(reg)) {
            rex |= 0x40; /* 0x40 is rex fixed prefix. */
        }
        return rex;
    }

    uint8 GetRex(Reg reg1, Reg reg2) const
    {
        uint8 rexW = GetRegSize(reg1) == k64Bits || GetRegSize(reg2) == k64Bits ? 8 : 0;
        uint8 rexR = IsRegExt(reg1) ? 4 : 0;
        uint8 rexB = IsRegExt(reg2) ? 1 : 0;
        uint8 rex = rexW | rexR | rexB;
        if (rex || Need8BitRegPrefix(reg1) || Need8BitRegPrefix(reg2)) {
            rex |= 0x40; /* 0x40 is rex fixed prefix. */
        }
        return rex;
    }

    uint8 GetRex(const Mem &mem, Reg reg = ERR) const
    {
        uint8 rexW = ((mem.size == k64Bits) || (reg != ERR && GetRegSize(reg) == k64Bits)) ? 8 : 0;
        uint8 rexR = reg != ERR && IsRegExt(reg) ? 4 : 0;
        uint8 rexX = mem.index != ERR && IsRegExt(mem.index) ? 2 : 0;
        uint8 rexB = mem.base != ERR && IsRegExt(mem.base) ? 1 : 0;
        uint8 rex = rexW | rexR | rexX | rexB;
        if (rex || Need8BitRegPrefix(reg)) {
            rex |= 0x40; /* 0x40 is rex fixed prefix. */
        }
        return rex;
    }

    uint8 GetMod(Reg reg) const
    {
        return 0b11;
    } /* mod=b11, register direct addressing. */

    uint8 GetMod(const Mem &mem) const
    {
        int64 symIdx = mem.disp.first;
        int64 offset = mem.disp.second;
        if ((symIdx == 0 && offset != 0 && Is8Bits(offset) && mem.base != RIP) ||
            (mem.memType == kOnlyBase && (mem.base == RBP || mem.base == R13))) {
            return 0b01; /* mod=b01, ModRM=b01 Reg r/m */
        } else if (symIdx != 0 || (offset != 0 && Is32Bits(offset))) {
            if (mem.base == ERR || mem.base == RIP) {
                return 0b00; /* mod=b00, ModRM=b00 Reg r/m */
            } else {
                return 0b10; /* mod=b10, ModRM=b01 Reg r/m */
            }
        }
        return 0b00; /* mod=b00, ModRM=b00 Reg r/m */
    }

    uint8 GetModRM(Reg reg1, Reg reg2) const
    {
        uint8 modReg = GetRegCodeId(reg1);
        uint8 mod = GetMod(reg1);
        uint8 modrm = GetRegCodeId(reg2);
        return ((mod << kLeftShift6Bits) | (modReg << kLeftShift3Bits) | (modrm & kGetLow3Bits));
    }

    uint8 GetModRM(Reg reg, const Mem &mem) const
    {
        uint8 modReg = GetRegCodeId(reg);
        uint8 mod = GetMod(mem);
        uint8 modrm = !HasSIB(mem) ? GetRegCodeId(mem.base) : 0b100; /* r/m=b100, use SIB */
        return ((mod << kLeftShift6Bits) | (modReg << kLeftShift3Bits) | (modrm & kGetLow3Bits));
    }

    void SetModRM(uint8 mod, uint8 modReg, uint8 modrm)
    {
        uint8 modRM = (mod << kLeftShift6Bits) | (modReg << kLeftShift3Bits) | (modrm & kGetLow3Bits);
        Encodeb(modRM);
    }

    bool HasSIB(const Mem &mem) const
    {
        if (mem.memType == kBaseAndDisp || mem.memType == kOnlyBase) {
            return GetRegCodeId(mem.base) == 0b100;
        }
        return mem.memType != kBaseAndDisp && mem.memType != kOnlyBase && mem.memType != kNone;
    }

    uint8 GetSIB(const Mem &mem) const
    {
        std::unordered_map<uint8, uint8> log2Map = {{1, 0}, {2, 1}, {4, 2}, {8, 3}};
        uint8 scale = log2Map[mem.s];
        uint8 index = mem.index != ERR ? GetRegCodeId(mem.index) : 0b100;
        uint8 base = mem.base != ERR ? GetRegCodeId(mem.base) : 0b101;
        return ((scale << kLeftShift6Bits) | ((index & kGetLow3Bits) << kLeftShift3Bits) | (base & kGetLow3Bits));
    }

    void OpReg(Reg reg, uint8 opCode1, uint8 opCode2, uint8 modReg);
    void OpMem(const Mem &mem, uint8 opCode1, uint8 opCode2, uint8 modReg);
    void OpDisp(const Mem &mem);
    void OpRR(Reg reg1, Reg reg2, uint8 opCode1, uint8 opCode2 = 0, bool extInsn = false);
    void OpRM(Reg reg, const Mem &mem, uint8 opCode1, uint8 opCode2 = 0, bool extInsn = false);
    void OpImmAndReg(const ImmOpnd &immOpnd, Reg reg, uint8 opCode, uint8 modReg);
    void OpImmAndMem(const ImmOpnd &immOpnd, const Mem &mem, uint8 modReg);
    void MovRegAndDisp(Reg reg, const Mem &mem, uint8 opCode);
    void OpPushPop(Reg reg, uint8 opCode);
    void JmpToLabel(int64 labelIdx, uint8 opCode1, uint8 opCode2 = 0, size_t offsetSize = 4);
    void OpCmovcc(Reg srcReg, Reg dstReg, uint8 opCode1, uint8 opCode2);
    void UpdateLabel(int64 labelIdx, LabelType type = LabelType::kLNone, uint32 relOffset = 0xFFFFFFFFU);
    bool CanEncodeLabel(int64 labelIdx);
    uint32 GetLabelSize(int64 labelIdx) const;
    uint32 GetLabelRelOffset(int64 labelIdx) const;
    void AppendFixup(int64 labelIdx, FixupKind kind, const std::pair<uint32, size_t> &offsetPair,
                     std::vector<Fixup *> &tmpFixups, int64 disp = 0);
    /* elf file */
    void InitElfHeader();
    void RegisterSection(Section &section);
    void LayoutSections();
    void UpdateSectionOffset(Section &section);
    void UpdateGlobalOffset(Section &section);
    void SetFileOffset(uint64 offset);
    /* symIdx is the key used to get symbol's index in .symtab */
    void AddSymToSymTab(const Symbol &symbol, int64 symIdx);
    void AppendRela(const Label &label, const std::pair<uint32, size_t> &offsetPair, uint64 type, Sxword addend);
    uint64 GetRelaType(FixupKind kind) const;
    void HandleTextSectionFixup();
    void HandleDataSectionFixup();
    void HandleRodataSectionFixup();
    void WriteElfFile();
    void AppendSecSymsToSymTabSec();
    void AppendSymsToSymTabSec();
    void AppendGlobalSymsToSymTabSec();

private:
    std::vector<uint8> codeBuff;
    std::unordered_map<int64, Label *> labelManager;
    std::vector<std::pair<Symbol, int64>> localSymTab;
    std::vector<std::pair<Symbol, int64>> symTab;
    std::vector<Fixup *> fixups;
    std::vector<Fixup *> dataFixups;
    std::vector<Fixup *> rodataFixups;
    std::vector<Fixup *> debugInfoFixups;
    std::vector<Section *> sections;
    Offset globalOffset = 0; /* global offset of the elf file */
    FileHeader header {};
    DataSection *textSection = nullptr;
    DataSection *rodataSection = nullptr;
    DataSection *dataSection = nullptr;
    DataSection *bssSection = nullptr;
    RelaSection *relaSection = nullptr;
    RelaSection *relaDataSection = nullptr;
    RelaSection *relaRodataSection = nullptr;
    StringSection *strTabSection = nullptr;
    SymbolSection *symbolTabSection = nullptr;
    /* debug section */
    DataSection *debugInfoSection = nullptr;
    RelaSection *relaDebugInfoSection = nullptr;
    DataSection *debugAbbrevSection = nullptr;
    DataSection *debugStrSection = nullptr;
    DataSection *debugARangesSection = nullptr;
    DataSection *debugRangesSection = nullptr;
    DataSection *debugLineSection = nullptr;
    uint64_t lastModulePC;
}; /* class ElfAssembler */
} /* namespace assembler */

#endif /* ELF_ASSEMBLER_H */