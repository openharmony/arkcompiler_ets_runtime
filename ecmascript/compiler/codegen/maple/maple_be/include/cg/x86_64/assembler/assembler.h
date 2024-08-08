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

#ifndef X64_ASSEMBLER_H
#define X64_ASSEMBLER_H

#ifndef NDEBUG
#define NDEBUG
#endif

#include <cassert>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include "cg_option.h"
#include "operand.h"
#include "stackmap.h"

namespace assembler {
enum SymbolAttr {
    kSAGlobal,
    kSALocal,
    kSAWeak,
    kSAHidden,
    kSAStatic,
};

enum SectionKind {
    kSBss,
    kSComm,
    kSData,
    kSRodata,
    kSTbss,
    kSTdata,
    kSText,
    kSDebugInfo,
    kSDebugAbbrev,
    kSDebugStr,
};

constexpr uint32 kDwarfVersion = 4;
constexpr uint32 kSizeOfPTR = 8;
constexpr uint32 k9ByteSize = 9;

class Assembler {
public:
    Assembler() = default;
    virtual ~Assembler() = default;

    virtual void CloseOutput()
    {
        if (outFStream.is_open()) {
            outFStream << outStream.str();
            outFStream.close();
        }
    }

    void Emit(int64 val)
    {
        outStream << val;
    }

    void Emit(const std::string &str)
    {
        outStream << str;
    }

    void Emit(const void *data, uint64 sizeInByte)
    {
        outStream.write(reinterpret_cast<const char *>(data), sizeInByte);
    }

    void StoreNameIntoSymMap(int64 symIdx, const std::string &name, bool isLocal = false)
    {
        if (isLocal) {
            (void)localSymMap.emplace(symIdx, name);
        } else {
            (void)globalSymMap.emplace(symIdx, name);
        }
    }

    const std::string &GetNameFromSymMap(int64 symIdx, bool isLocal = false)
    {
        if (isLocal) {
            return localSymMap.at(symIdx);
        } else {
            return globalSymMap.at(symIdx);
        }
    }

    void ClearLocalSymMap()
    {
        localSymMap.clear();
    }

    virtual void InitialFileInfo(const std::string &inputFileName) = 0;
    virtual void EmitVariable(int64 symIdx, uint64 sizeInByte, uint8 alignInByte, SymbolAttr symAttr,
                              SectionKind sectionKind) = 0;
    virtual void EmitFunctionFoot(int64 symIdx, SymbolAttr funcAttr) = 0;
    virtual void EmitFunctionHeader(int64 symIdx, SymbolAttr funcAttr, const std::string *secName = nullptr) = 0;
    virtual void EmitBBLabel(int64 labelSymIdx, bool genVerboseInfo = false, uint32 freq = 0,
                             const std::string *mirName = nullptr) = 0;
    virtual void EmitJmpTableElem(int64 jmpLabelIdx, const std::vector<int64> &labelIdxs) = 0;
    virtual void EmitDirectString(const std::string &ustr, bool belongsToDataSec = true, int64 strSymIdx = 0,
                                  bool emitAscii = false) = 0;
    /* Indirect strings refer to string pointers, such as "char *buf". */
    virtual void EmitIndirectString(int64 strSymIdx, bool belongsToDataSec = true) = 0;
    virtual void EmitIntValue(int64 value, size_t valueSize, bool belongsToDataSec = true) = 0;
    virtual void EmitFloatValue(int64 symIdx, int64 value, size_t valueSize) = 0;
    virtual void EmitAddrValue(int64 symIdx, int32 symAddrOfs, int32 structFieldOfs, bool belongsToDataSec = true) = 0;
    virtual void EmitAddrOfFuncValue(int64 symIdx, bool belongsToDataSec = true) = 0;
    virtual void EmitLabelValue(int64 symIdx, bool belongsToDataSec = true) = 0;
    virtual void EmitBitFieldValue(uint64 combineBitFieldValue, bool belongsToDataSec = true) = 0;
    virtual void EmitNull(uint64 sizeInByte) = 0;
    virtual void PostEmitVariable(int64 symIdx, SymbolAttr symAttr, uint64 sizeInByte,
                                  bool belongsToTextSec = false) = 0;
    virtual void FinalizeFileInfo() = 0;

    /* emit debug info */
    virtual void EmitDIHeader() = 0;
    virtual void EmitDIFooter() = 0;
    virtual void EmitDIHeaderFileInfo() = 0;
    virtual void EmitDIDebugInfoSectionHeader(uint64 debugInfoLength) = 0;
    virtual void EmitDIDebugInfoSectionAbbrevId(bool verbose, uint32 abbrevId, const std::string &dieTagName,
                                                uint32 offset, uint32 size) = 0;
    virtual void EmitDIFormSpecification(unsigned int dwform) = 0;
    virtual void EmitDebugComment(const char* comment) = 0;
    /* EmitDIAttrValue */
    virtual void EmitDwFormString(const std::string &name) = 0;
    /* strTableSize is used to calculate unique id for the debug string */
    virtual void EmitDwFormStrp(uint32 strLabelId, size_t strTableSize) = 0;
    virtual void EmitDwFormData(int32 attrValue, uint8 sizeInByte) = 0;
    virtual void EmitDwFormData8() = 0;
    virtual void EmitDwFormData8(uint32 endLabelFuncPuIdx, uint32 startLabelFuncPuIdx, uint32 endLabelIdx,
                                 uint32 startLabelIdx) = 0;
    virtual void EmitLabel(uint32 funcPuIdx, uint32 labIdx) = 0;
    virtual void EmitDwFormSecOffset() = 0;
    virtual void EmitDwFormAddr(bool emitTextBegin = false) = 0;
    virtual void EmitDwFormRef4(uint64 offsetOrValue, bool unknownType = false, bool emitOffset = false) = 0;
    virtual void EmitDwFormExprlocCfa(uint32 dwOp) = 0;
    virtual void EmitDwFormExprlocAddr(uint32 dwOp, const std::string &addrStr) = 0;
    virtual void EmitDwFormExprlocFbreg(uint32 dwOp, int fboffset, size_t sleb128Size) = 0;
    virtual void EmitDwFormExprlocBregn(uint32 dwOp, const std::string &dwOpName) = 0; /* n=0~7 */
    virtual void EmitDwFormExprloc(uintptr elp) = 0;

    virtual void EmitDIDwName(const std::string &dwAtName, const std::string &dwForName) = 0;
    virtual void EmitDIDWFormStr(const std::string &formStr) = 0;
    virtual void EmitDIDWDataMemberLocaltion(unsigned int lableIdx, uintptr_t attr) = 0;
    virtual void EmitDIDebugAbbrevSectionHeader() = 0;
    virtual void EmitDIDebugAbbrevDiae(bool verbose, uint32 abbrevId, uint32 tag, const std::string &dwTagName,
                                       bool withChildren) = 0;
    virtual void EmitDIDebugAbbrevDiaePairItem(bool verbose, uint32 aplAt, uint32 aplFrom, const std::string &dwAtName,
                                               const std::string &dwFromName) = 0;
    virtual void EmitDIDebugSectionEnd(SectionKind secKind) = 0;
    virtual void EmitDIDebugARangesSection() = 0;
    virtual void EmitDIDebugRangesSection() = 0;
    virtual void EmitDIDebugLineSection() = 0;
    virtual void EmitDIDebugStrSection(const std::vector<uint32> &strps, const std::vector<std::string> &debugStrs,
                                       uint64 size, size_t strTableSize) = 0;
    virtual void EmitLine() = 0;

    /* start of X64 instructions */
    /* mov */
    virtual void Mov(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Mov(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) = 0;
    virtual void Mov(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Mov(InsnSize insnSize, Reg reg, const Mem &mem) = 0;
    virtual void Mov(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) = 0;
    /* floating point mov */
    virtual void Mov(Reg srcReg, Reg destReg, bool isMovD = true) = 0;
    virtual void MovF(const Mem &mem, Reg reg, bool isSingle = true) = 0;
    virtual void MovF(Reg reg, const Mem &mem, bool isSingle = true) = 0;
    /* movabs */
    virtual void Movabs(const ImmOpnd &immOpnd, Reg reg) = 0;
    virtual void Movabs(int64 symIdx, Reg reg) = 0;
    /* push */
    virtual void Push(InsnSize insnSize, Reg reg) = 0;
    /* pop */
    virtual void Pop(InsnSize insnSize, Reg reg) = 0;
    /* lea */
    virtual void Lea(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    /* movzx */
    virtual void MovZx(InsnSize sSize, InsnSize dSize, Reg srcReg, Reg destReg) = 0;
    virtual void MovZx(InsnSize sSize, InsnSize dSize, const Mem &mem, Reg reg) = 0;
    /* movsx */
    virtual void MovSx(InsnSize sSize, InsnSize dSize, Reg srcReg, Reg destReg) = 0;
    virtual void MovSx(InsnSize sSize, InsnSize dSize, const Mem &mem, Reg reg) = 0;
    /* add */
    virtual void Add(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Add(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) = 0;
    virtual void Add(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Add(InsnSize insnSize, Reg reg, const Mem &mem) = 0;
    virtual void Add(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) = 0;
    /* add floating point */
    virtual void Add(Reg srcReg, Reg destReg, bool isSingle = true) = 0;
    virtual void Add(const Mem &mem, Reg reg, bool isSingle = true) = 0;
    /* sub */
    virtual void Sub(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Sub(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) = 0;
    virtual void Sub(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Sub(InsnSize insnSize, Reg reg, const Mem &mem) = 0;
    virtual void Sub(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) = 0;
    /* sub floating point */
    virtual void Sub(Reg srcReg, Reg destReg, bool isSingle = true) = 0;
    virtual void Sub(const Mem &mem, Reg reg, bool isSingle = true) = 0;
    /* and */
    virtual void And(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void And(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void And(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) = 0;
    virtual void And(InsnSize insnSize, Reg reg, const Mem &mem) = 0;
    virtual void And(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) = 0;
    /* or */
    virtual void Or(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Or(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Or(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) = 0;
    virtual void Or(InsnSize insnSize, Reg reg, const Mem &mem) = 0;
    virtual void Or(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) = 0;
    /* xor */
    virtual void Xor(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Xor(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) = 0;
    virtual void Xor(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Xor(InsnSize insnSize, Reg reg, const Mem &mem) = 0;
    virtual void Xor(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) = 0;
    /* bsr */
    virtual void Bsr(InsnSize insnSize, Reg srcReg, Reg destReg) =0;
    /* not */
    virtual void Not(InsnSize insnSize, Reg reg) = 0;
    virtual void Not(InsnSize insnSize, const Mem &mem) = 0;
    /* neg */
    virtual void Neg(InsnSize insnSize, Reg reg) = 0;
    virtual void Neg(InsnSize insnSize, const Mem &mem) = 0;
    /* div & cwd, cdq, cqo */
    virtual void Idiv(InsnSize insnSize, Reg reg) = 0;
    virtual void Idiv(InsnSize insnSize, const Mem &mem) = 0;
    virtual void Div(InsnSize insnSize, Reg reg) = 0;
    virtual void Div(InsnSize insnSize, const Mem &mem) = 0;
    virtual void Cwd() = 0;
    virtual void Cdq() = 0;
    virtual void Cqo() = 0;
    /* shl */
    virtual void Shl(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Shl(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) = 0;
    virtual void Shl(InsnSize insnSize, Reg reg, const Mem &mem) = 0;
    virtual void Shl(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) = 0;
    /* sar */
    virtual void Sar(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Sar(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) = 0;
    virtual void Sar(InsnSize insnSize, Reg reg, const Mem &mem) = 0;
    virtual void Sar(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) = 0;
    /* shr */
    virtual void Shr(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Shr(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) = 0;
    virtual void Shr(InsnSize insnSize, Reg reg, const Mem &mem) = 0;
    virtual void Shr(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) = 0;
    /* jmp */
    virtual void Jmp(Reg reg) = 0;
    virtual void Jmp(const Mem &mem) = 0;
    virtual void Jmp(int64 symIdx) = 0;
    /* jump condition */
    virtual void Je(int64 symIdx) = 0;
    virtual void Ja(int64 symIdx) = 0;
    virtual void Jae(int64 symIdx) = 0;
    virtual void Jne(int64 symIdx) = 0;
    virtual void Jb(int64 symIdx) = 0;
    virtual void Jbe(int64 symIdx) = 0;
    virtual void Jg(int64 symIdx) = 0;
    virtual void Jge(int64 symIdx) = 0;
    virtual void Jl(int64 symIdx) = 0;
    virtual void Jle(int64 symIdx) = 0;
    /* cmp */
    virtual void Cmp(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cmp(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Cmp(InsnSize insnSize, Reg reg, const Mem &mem) = 0;
    virtual void Cmp(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg) = 0;
    virtual void Cmp(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem) = 0;
    /* test */
    virtual void Test(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    /* set */
    virtual void Setbe(Reg reg) = 0;
    virtual void Setbe(const Mem &mem) = 0;
    virtual void Setle(Reg reg) = 0;
    virtual void Setle(const Mem &mem) = 0;
    virtual void Setae(Reg reg) = 0;
    virtual void Setae(const Mem &mem) = 0;
    virtual void Setge(Reg reg) = 0;
    virtual void Setge(const Mem &mem) = 0;
    virtual void Setne(Reg reg) = 0;
    virtual void Setne(const Mem &mem) = 0;
    virtual void Setb(Reg reg) = 0;
    virtual void Setb(const Mem &mem) = 0;
    virtual void Setl(Reg reg) = 0;
    virtual void Setl(const Mem &mem) = 0;
    virtual void Seta(Reg reg) = 0;
    virtual void Seta(const Mem &mem) = 0;
    virtual void Setg(Reg reg) = 0;
    virtual void Setg(const Mem &mem) = 0;
    virtual void Sete(Reg reg) = 0;
    virtual void Sete(const Mem &mem) = 0;
    virtual void Seto(Reg reg) = 0;
    virtual void Seto(const Mem &mem) = 0;
    /* cmov */
    virtual void Cmova(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cmova(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Cmovae(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cmovae(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Cmovb(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cmovb(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Cmovbe(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cmovbe(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Cmove(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cmove(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Cmovg(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cmovg(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Cmovge(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cmovge(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Cmovl(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cmovl(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Cmovle(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cmovle(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Cmovne(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cmovne(InsnSize insnSize, const Mem &mem, Reg reg) = 0;
    virtual void Cmovo(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    /* call */
    virtual void Call(InsnSize insnSize, Reg reg) = 0;
    virtual void Call(InsnSize insnSize, const Mem &mem) = 0;
    virtual void Call(InsnSize insnSize, int64 symIdx) = 0;
    /* ret */
    virtual void Ret() = 0;
    /* leave */
    virtual void Leave() = 0;
    /* imul */
    virtual void Imul(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    /* mul float */
    virtual void Mul(Reg srcReg, Reg destReg, bool isSingle = true) = 0;
    virtual void Mul(const Mem &mem, Reg reg, bool isSingle = true) = 0;
    /* nop */
    virtual void Nop(InsnSize insnSize, const Mem &mem) = 0;
    virtual void Nop() = 0;
    /* byte swap */
    virtual void Bswap(InsnSize insnSize, Reg reg) = 0;
    virtual void Xchg(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    /* pseudo insn */
    virtual void DealWithPseudoInst(const std::string &insn) = 0;
    /* floating point */
    virtual void MovF(Reg srcReg, Reg destReg, bool isSingle = true) = 0;
    /* floating point and */
    virtual void And(Reg srcReg, Reg destReg, bool isSingle = true) = 0;
    virtual void And(const Mem &mem, Reg reg, bool isSingle = true) = 0;
    /* floating div */
    virtual void Divsd(Reg srcReg, Reg destReg) = 0;
    virtual void Divsd(const Mem &mem, Reg reg) = 0;
    /* convert int2float */
    virtual void Cvtsi2ss(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cvtsi2sd(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    /*convert float2int */
    virtual void Cvttsd2si(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    virtual void Cvttss2si(InsnSize insnSize, Reg srcReg, Reg destReg) = 0;
    /* convert float2float */
    virtual void Cvtss2sd(Reg srcReg, Reg destReg) = 0;
    virtual void Cvtsd2ss(Reg srcReg, Reg destReg) = 0;
    /* unordered compare */
    virtual void Ucomisd(Reg srcReg, Reg destReg) = 0;
    virtual void Ucomiss(Reg srcReg, Reg destReg) = 0;
    virtual void Cmpeqsd(Reg srcReg, Reg destReg) = 0;
    /* float sqrt*/
    virtual void Sqrtss_r(Reg srcReg, Reg destReg) = 0;
    virtual void Sqrtsd_r(Reg srcReg, Reg destReg) = 0;
    /* end of X64 instructions */
    /* process stackmap */
    virtual void RecordStackmap(const std::vector<uint8> &referenceMap, const std::vector<uint8> &deoptInfo) = 0;
    virtual uint32 GetCurModulePC() = 0;
    virtual void SetLastModulePC(uint32 pc) = 0;

protected:
    std::ostringstream outStream;
    std::ofstream outFStream;
    std::string fileName;
    std::unordered_map<int64, const std::string> globalSymMap; /* store global variable symbols */
    std::unordered_map<int64, const std::string> localSymMap;  /* store local variable symbols for each function */
    std::string currDbgComment;
};
} /* namespace assembler */

#endif /* X64_ASSEMBLER_H */