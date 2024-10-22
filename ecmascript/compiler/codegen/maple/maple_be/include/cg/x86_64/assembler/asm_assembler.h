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

#ifndef ASM_ASSEMBLER_H
#define ASM_ASSEMBLER_H

#include "assembler.h"

namespace assembler {

#if TARGRISCV64
#define CMNT "\t# "
#else
#define CMNT "\t// "
#endif
#define TEXT_BEGIN text0
#define TEXT_END etext0
#define DEBUG_INFO_0 debug_info0
#define DEBUG_ABBREV_0 debug_abbrev0
#define DEBUG_LINE_0 debug_line0
#define DEBUG_STR_LABEL ASF

#define XSTR(s) #s

enum Directive : uint8 { kAlign, kComm, kFile, kFuncType, kHidden, kName, kObjType, kSection, kSize, kZero };

class AsmAssembler : public Assembler {
public:
    explicit AsmAssembler(const std::string &outputFileName) : Assembler()
    {
        outFStream.open(outputFileName, std::ios::trunc);
    }

    ~AsmAssembler() = default;

    void CloseOutput() override
    {
        Assembler::CloseOutput();

        auto options = maplebe::CGOptions::GetInstance();
        options.GetLogStream() << "~~~~~~~~~~~~~~ LiteCG x64 disasm begin ~~~~~~~~~~~~~~\n";
        options.GetLogStream() << outStream.str();
        options.GetLogStream() << "~~~~~~~~~~~~~~ LiteCG x64 disasm end  ~~~~~~~~~~~~~~~\n";
    }

    void InitialFileInfo(const std::string &inputFileName) override;
    void EmitFunctionHeader(int64 symIdx, SymbolAttr funcAttr, const std::string *secName) override;
    void EmitBBLabel(int64 labelSymIdx, bool genVerboseInfo, uint32 freq, const std::string *mirName) override;
    void EmitJmpTableElem(int64 jmpLabelIdx, const std::vector<int64> &labelIdxs) override;
    void EmitFunctionFoot(int64 symIdx, SymbolAttr funcAttr) override;
    /* emit variable's value */
    void EmitVariable(int64 symIdx, uint64 sizeInByte, uint8 alignInByte, SymbolAttr symAttr,
                      SectionKind sectionKind) override;
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
    void FinalizeFileInfo() override {}

    /* emit debug info */
    void EmitDIHeader() override;
    void EmitDIFooter() override;
    void EmitDIHeaderFileInfo() override;
    void EmitDIDebugInfoSectionHeader(uint64 debugInfoLength) override;
    void EmitDIDebugInfoSectionAbbrevId(bool verbose, uint32 abbrevId, const std::string &dieTagName, uint32 offset,
                                        uint32 size) override;
    void EmitDIFormSpecification(unsigned int dwform) override;
    void EmitDebugComment(const char* comment) override;

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

    void EmitDIDwName(const std::string &dwAtName, const std::string &dwForName) override;
    void EmitDIDWFormStr(const std::string &formStr) override
    {
        Emit(" : ");
        Emit(formStr);
    }

    void EmitDIDWDataMemberLocaltion(unsigned int lableIdx, uintptr_t attr) override
    {
        Emit(" : ");
        Emit(lableIdx);
        Emit("  attr= ");
        EmitHexUnsigned(attr);
    }

    void EmitDIDebugAbbrevSectionHeader() override
    {
        Emit("\t.section\t.debug_abbrev,\"\",@progbits\n");
        Emit(".L" XSTR(DEBUG_ABBREV_0) ":\n");
    }

    void EmitDIDebugAbbrevDiae(bool verbose, uint32 abbrevId, uint32 tag, const std::string &dwTagName,
                               bool withChildren) override;
    void EmitDIDebugAbbrevDiaePairItem(bool verbose, uint32 aplAt, uint32 aplFrom, const std::string &dwAtName,
                                       const std::string &dwFromName) override;

    void EmitDIDebugSectionEnd(SectionKind secKind) override
    {
        Emit("\t.byte    0x0\n");
    }

    void EmitDIDebugARangesSection() override
    {
        Emit("\t.section\t.debug_aranges,\"\",@progbits\n");
    }

    void EmitDIDebugRangesSection() override
    {
        Emit("\t.section\t.debug_ranges,\"\",@progbits\n");
    }

    void EmitDIDebugLineSection() override
    {
        Emit("\t.section\t.debug_line,\"\",@progbits\n");
        Emit(".L" XSTR(DEBUG_LINE_0) ":\n");
    }

    void EmitDIDebugStrSection(const std::vector<uint32> &strps, const std::vector<std::string> &debugStrs, uint64 size,
                               size_t strTableSize) override;
    void EmitHexUnsigned(uint64 num);
    void EmitDecUnsigned(uint64 num);
    void EmitDecSigned(int64 num);

    void EmitLine() override
    {
        Emit("\n");
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
    /* not */
    void Not(InsnSize insnSize, Reg reg) override;
    void Not(InsnSize insnSize, const Mem &mem) override;
    /* bsr */
    void Bsr(InsnSize insnSize, Reg srcReg, Reg destReg) override;
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
    void DealWithPseudoInst(const std::string &insn) override;
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
    void Cmpeqsd(Reg srcReg, Reg destReg) override;
    /* float sqrt*/
    void Sqrtss_r(Reg srcReg, Reg destReg) override;
    void Sqrtsd_r(Reg srcReg, Reg destReg) override;
    /* end of X64 instructions */
    /* process stackmap */
    void RecordStackmap(const std::vector<uint8> &referenceMap, const std::vector<uint8> &deoptInfo) override {}
    uint32 GetCurModulePC() override { return 0; }
    void SetLastModulePC(uint32 pc) override {}

private:
    void EmitComment(std::string comment)
    {
        EmitDebugComment(comment.c_str());
    }

    void EmitSizeDirective(uint8 elemSize, int64 value, bool isSymbol, bool isLocal = false)
    {
        std::unordered_map<uint8, std::string> symSizeDirMap = {
            {k1Byte, ".byte"}, {k2Bytes, ".value"}, {k4Bytes, ".long"}, {k8Bytes, ".quad"}};
        Emit("\t");
        Emit(symSizeDirMap.at(elemSize));
        Emit("\t");
        if (isSymbol) {
            std::string name = GetNameFromSymMap(value, isLocal);
            Emit(name);
        } else {
            Emit(value);
        }
        Emit("\n");
    }

    void EmitSectionDirective(SectionKind sectionKind)
    {
        std::unordered_map<SectionKind, std::string> secDirMap = {{kSBss, ".bss\t"},       {kSData, ".data\n"},
                                                                  {kSRodata, ".rodata\n"}, {kSTbss, ".tbss\t"},
                                                                  {kSTdata, ".tdata\n"},   {kSText, ".text\n"}};
        Emit("\t.section\t");
        Emit(secDirMap.at(sectionKind));
    }

    void EmitSymbolAttrDirective(SymbolAttr symAttr, int64 symIdx, bool isLocal = false)
    {
        std::unordered_map<SymbolAttr, std::string> symAttrDirMap = {{kSAGlobal, ".global"},
                                                                     {kSALocal, ".local"},
                                                                     {kSAHidden, ".hidden"},
                                                                     {kSAStatic, ".local"},
                                                                     {kSAWeak, ".weak"}};
        std::string name = GetNameFromSymMap(symIdx, isLocal);
        Emit("\t");
        Emit(symAttrDirMap.at(symAttr));
        Emit("\t");
        Emit(name);
        Emit("\n");
    }

    void EmitDirective(Directive directive, int64 symIdx = 0, bool isLocal = false, uint8 alignInByte = 0)
    {
        std::string name = "";
        if (symIdx != 0) {
            name = GetNameFromSymMap(symIdx, isLocal);
        }
        switch (directive) {
            case kAlign: {
                if (alignInByte > 0) {
                    Emit("\t.align\t");
                    Emit(alignInByte);
                    Emit("\n");
                }
                break;
            }
            case kFile:
                Emit("\t.file\t");
                break;
            case kFuncType:
                Emit("\t.type\t");
                Emit(name);
                Emit(", @function\n");
                break;
            case kHidden:

                Emit(name);
                Emit("\n");
                break;
            case kName:
                Emit(name);
                Emit(":\n");
                break;
            case kObjType:
                Emit("\t.type\t");
                Emit(name);
                Emit(", @object\n");
                break;
            case kSection:
                Emit("\t.section\t");
                break;
            case kSize:
                Emit("\t.size\t");
                Emit(name);
                Emit(", .-");
                Emit(name);
                Emit("\n");
                break;
            case kZero:
                Emit("\t.zero\t");
                break;
            default:
                CHECK_FATAL(false, "EmitDirective: unsupport directive");
                break;
        }
    }

    void EmitInsnSuffix(InsnSize insnSize)
    {
        std::unordered_map<InsnSize, std::string> insnSuffixDirMap = {{kB, "b"}, {kW, "w"}, {kL, "l"}, {kQ, "q"}};
        Emit(insnSuffixDirMap.at(insnSize));
    }

    void EmitReg(Reg reg)
    {
        std::string regStr = kRegStrMap.at(reg);
        Emit("%");
        Emit(regStr);
    }

    void EmitSymbol(int64 symIdx)
    {
        std::string symbolName = GetNameFromSymMap(symIdx);
        Emit("$");
        Emit(symbolName);
    }

    void EmitMem(const Mem &mem)
    {
        /* emit displacement */
        if (mem.disp.first != 0) {
            std::string dispSymName = GetNameFromSymMap(mem.disp.first);
            Emit(dispSymName);
            if (mem.disp.second != 0) {
                Emit("+");
                Emit(mem.disp.second);
            }
        } else {
            Emit(mem.disp.second);
        }
        /* emit base & index registers */
        Emit("(");
        if (mem.base != ERR) {
            EmitReg(mem.base);
        }
        if (mem.index != ERR) {
            Emit(", ");
            EmitReg(mem.index);
            Emit(", ");
            Emit(mem.s);
        }
        Emit(")");
    }

    void EmitImm(int64 imm)
    {
        Emit("$");
        Emit(imm);
    }

    void EmitLabel(int64 symIdx)
    {
        std::string labelName = GetNameFromSymMap(symIdx);
        if (symIdx < 0 && labelName[0] != '.') {
            Emit("$");
        }
        Emit(labelName);
    }

    void EmitRegReg(Reg srcReg, Reg destReg)
    {
        EmitReg(srcReg);
        Emit(",\t");
        EmitReg(destReg);
    }

    void EmitImmOrSymbolReg(int64 val, bool isSymbol, Reg reg)
    {
        if (isSymbol) {
            EmitSymbol(val);
        } else {
            EmitImm(val);
        }
        Emit(",\t");
        EmitReg(reg);
    }

    void EmitLabelReg(int64 labelIdx, Reg reg)
    {
        EmitLabel(labelIdx);
        Emit(",\t");
        EmitReg(reg);
    }

    void EmitMemReg(const Mem &mem, Reg reg)
    {
        EmitMem(mem);
        Emit(",\t");
        EmitReg(reg);
    }

    void EmitRegMem(Reg reg, const Mem &mem)
    {
        EmitReg(reg);
        Emit(",\t");
        EmitMem(mem);
    }

    void EmitImmOrSymbolMem(int64 val, bool isSymbol, Mem mem)
    {
        if (isSymbol) {
            EmitSymbol(val);
        } else {
            EmitImm(val);
        }
        Emit(",\t");
        EmitMem(mem);
    }

    void EmitLabelMem(int64 labelIdx, const Mem &mem)
    {
        EmitLabel(labelIdx);
        Emit(",\t");
        EmitMem(mem);
    }
}; /* class AsmAssembler */
} /* namespace assembler */

#endif /* ASM_ASSEMBLER_H */