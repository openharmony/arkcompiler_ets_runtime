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

#include "assembler/asm_assembler.h"
#include <unistd.h>
#include <securec.h>
#include "dwarf.h"

namespace assembler {
void AsmAssembler::InitialFileInfo(const std::string &inputFileName)
{
    std::string curDirName = ".";
    std::string path(curDirName);
    std::string cgFile(path.append("/mplcg"));
    EmitComment(cgFile);
    EmitComment("Compiling");
    EmitComment("Be options");

    path = curDirName;
    (void)path.append("/").append(inputFileName);
    std::string irFile("\"");
    (void)irFile.append(path).append("\"");
    EmitDirective(kFile);
    Emit(irFile);
    Emit("\n");
}

void AsmAssembler::EmitFunctionHeader(int64 symIdx, SymbolAttr funcAttr, const std::string *secName)
{
    if (secName != nullptr) {
        EmitDirective(kSection);
        Emit(*secName);
        Emit(",\"ax\",@progbits\n");
    } else {
        EmitSectionDirective(kSText);
    }
    EmitDirective(kAlign, 0, false, k8Bits);

    EmitSymbolAttrDirective(funcAttr, symIdx);

    EmitDirective(kFuncType, symIdx);
    EmitDirective(kName, symIdx);
    Emit("\t.cfi_startproc\n");
}

void AsmAssembler::EmitBBLabel(int64 labelSymIdx, bool genVerboseInfo, uint32 freq, const std::string *mirName)
{
    std::string bbLabel = GetNameFromSymMap(labelSymIdx);
    if (genVerboseInfo) {
        Emit("//  freq:");
        Emit(freq);
        Emit("\n");

        Emit(bbLabel);
        Emit(":");
        if (mirName != nullptr) {
            Emit("\t//  MIR: @");
            Emit(*mirName);
        }
        Emit("\n");
    } else {
        EmitDirective(kName, labelSymIdx);
    }
}

void AsmAssembler::EmitFunctionFoot(int64 symIdx, SymbolAttr funcAttr)
{
    (void)funcAttr;
    Emit("\t.cfi_endproc\n");
    EmitDirective(kSize, symIdx);
    Emit("\n");
}

void AsmAssembler::PostEmitVariable(int64 symIdx, SymbolAttr symAttr, uint64 sizeInByte, bool belongsToTextSec)
{
    (void)sizeInByte;
    bool isLocal = false;
    if (symAttr == kSALocal) {
        isLocal = true;
    }
    EmitDirective(kSize, symIdx, isLocal);
    Emit("\n");
}

void AsmAssembler::EmitFloatValue(int64 symIdx, int64 value, size_t valueSize)
{
    (void)symIdx;
    EmitSizeDirective(valueSize, value, false, true);
}

void AsmAssembler::EmitJmpTableElem(int64 jmpLabelIdx, const std::vector<int64> &labelIdxs)
{
    EmitDirective(kAlign, 0, false, k8Bits);
    EmitDirective(kName, jmpLabelIdx);
    for (int64 labelIdx : labelIdxs) {
        EmitSizeDirective(k8Bytes, labelIdx, true);
    }
}

void AsmAssembler::EmitVariable(int64 symIdx, uint64 sizeInByte, uint8 alignInByte, SymbolAttr symAttr,
                                SectionKind sectionKind)
{
    bool isLocal = false;
    if (symAttr == kSALocal) {
        isLocal = true;
    }

    if (sectionKind == kSComm || sectionKind == kSBss) {
        EmitSectionDirective(kSData);
        EmitSymbolAttrDirective(symAttr, symIdx, isLocal);
        EmitDirective(kAlign, 0, isLocal, alignInByte);
        Emit("\t.comm\t");
        std::string name = GetNameFromSymMap(symIdx, isLocal);
        Emit(name);
        Emit(", ");
        Emit(sizeInByte);
        Emit(", ");
        Emit(alignInByte);
        Emit("\n");
    } else {
        EmitDirective(kObjType, symIdx, isLocal);
        EmitSectionDirective(sectionKind);
        EmitSymbolAttrDirective(symAttr, symIdx, isLocal);
        EmitDirective(kAlign, 0, isLocal, alignInByte);
        EmitDirective(kName, symIdx, isLocal);
    }
}

void AsmAssembler::EmitDirectString(const std::string &ustr, bool belongsToDataSec, int64 strSymIdx, bool emitAscii)
{
    (void)belongsToDataSec;
    if (strSymIdx != 0) {
        EmitSectionDirective(kSData);
        EmitDirective(kAlign, 0, false, k8Bits);
        EmitDirective(kName, strSymIdx);
    }

    if (emitAscii) {
        Emit("\t.ascii\t\"");
    } else {
        Emit("\t.string\t\"");
    }

    const char *str = ustr.c_str();
    size_t len = ustr.size();
    /* Rewrite special char with \\ */
    for (size_t i = 0; i < len; i++) {
        /* Referred to GNU AS: 3.6.1.1 Strings */
        constexpr int kBufSize = 5;
        constexpr int kFirstChar = 0;
        constexpr int kSecondChar = 1;
        constexpr int kThirdChar = 2;
        constexpr int kLastChar = 4;
        char buf[kBufSize];
        if (isprint(*str)) {
            buf[kFirstChar] = *str;
            buf[kSecondChar] = 0;
            if (*str == '\\' || *str == '\"') {
                buf[kFirstChar] = '\\';
                buf[kSecondChar] = *str;
                buf[kThirdChar] = 0;
            }
            Emit(buf);
        } else if (*str == '\b') {
            Emit("\\b");
        } else if (*str == '\n') {
            Emit("\\n");
        } else if (*str == '\r') {
            Emit("\\r");
        } else if (*str == '\t') {
            Emit("\\t");
        } else if (*str == '\0') {
            buf[kFirstChar] = '\\';
            buf[kSecondChar] = '0';
            buf[kThirdChar] = 0;
            Emit(buf);
        } else {
            /* all others, print as number */
            /* 4: max store chars */
            (void)snprintf_s(buf, sizeof(buf), 4, "\\%03o", static_cast<unsigned char>(*str) & 0xFF);
            buf[kLastChar] = '\0';
            Emit(buf);
        }
        str++;
    }
    Emit("\"\n");
}

void AsmAssembler::EmitIndirectString(int64 strSymIdx, bool belongsToDataSec)
{
    (void)belongsToDataSec;
    EmitSizeDirective(k8Bytes, strSymIdx, true);
}

void AsmAssembler::EmitIntValue(int64 value, size_t elemSize, bool belongsToDataSec)
{
    (void)belongsToDataSec;
    EmitSizeDirective(elemSize, value, false);
}

void AsmAssembler::EmitAddrValue(int64 symIdx, int32 symAddrOfs, int32 structFieldOfs, bool belongsToDataSec)
{
    (void)belongsToDataSec;
    Emit("\t.quad\t");
    std::string name = GetNameFromSymMap(symIdx);
    Emit(name);
    if (symAddrOfs != 0) {
        Emit(" + ");
        Emit(symAddrOfs);
    }
    if (structFieldOfs != 0) {
        Emit(" + ");
        Emit(structFieldOfs);
    }
    Emit("\n");
}

void AsmAssembler::EmitAddrOfFuncValue(int64 symIdx, bool belongsToDataSec)
{
    (void)belongsToDataSec;
    EmitSizeDirective(k8Bytes, symIdx, true);
}

void AsmAssembler::EmitLabelValue(int64 symIdx, bool belongsToDataSec)
{
    (void)belongsToDataSec;
    EmitSizeDirective(k8Bytes, symIdx, true);
}

void AsmAssembler::EmitBitFieldValue(uint64 combineBitFieldValue, bool belongsToDataSec)
{
    (void)belongsToDataSec;
    EmitSizeDirective(k1Byte, combineBitFieldValue, false);
}

/* emit debug info */
void AsmAssembler::EmitHexUnsigned(uint64 num)
{
    std::ios::fmtflags flag(this->outStream.flags());
    this->outStream << "0x" << std::hex << num;
    (void)this->outStream.flags(flag);
}

void AsmAssembler::EmitDecUnsigned(uint64 num)
{
    std::ios::fmtflags flag(outStream.flags());
    outStream << std::dec << num;
    (void)outStream.flags(flag);
}

void AsmAssembler::EmitDecSigned(int64 num)
{
    std::ios::fmtflags flag(outStream.flags());
    outStream << std::dec << num;
    (void)outStream.flags(flag);
}

void AsmAssembler::EmitDIHeader()
{
    Emit("\t.section ." + std::string("c_text") + ",\"ax\"\n");
    Emit(".L" XSTR(TEXT_BEGIN) ":\n");
}

void AsmAssembler::EmitDIFooter()
{
    Emit("\t.section ." + std::string("c_text") + ",\"ax\"\n");
    Emit(".L" XSTR(TEXT_END) ":\n");
}

void AsmAssembler::EmitDIHeaderFileInfo()
{
    Emit("// dummy header file 1\n");
    Emit("// dummy header file 2\n");
    Emit("// dummy header file 3\n");
}

void AsmAssembler::EmitDIDebugInfoSectionHeader(uint64 debugInfoLength)
{
    /* From DWARF Standard Specification V4. 7.5.1
       collect section size */
    Emit("\t.section\t.debug_info,\"\",@progbits\n");
    /* label to mark start of the .debug_info section */
    Emit(".L" XSTR(DEBUG_INFO_0) ":\n");
    /* $ 7.5.1.1 */
    Emit("\t.4byte\t");
    EmitHexUnsigned(debugInfoLength);
    Emit(CMNT "section length\n");
    /* DWARF version. uhalf. */
    Emit("\t.2byte\t");
    /* 4 for version 4. */
    EmitHexUnsigned(kDwarfVersion);
    Emit("\n");
    /* debug_abbrev_offset. 4byte for 32-bit, 8byte for 64-bit */
    Emit("\t.4byte\t.L" XSTR(DEBUG_ABBREV_0) "\n");
    /* address size. ubyte */
    Emit("\t.byte\t");
    EmitHexUnsigned(kSizeOfPTR);
    Emit("\n");
}

void AsmAssembler::EmitDIDebugInfoSectionAbbrevId(bool verbose, uint32 abbrevId, const std::string &dieTagName,
                                                  uint32 offset, uint32 size)
{
    if (verbose) {
        Emit("\n");
    }
    Emit("\t.uleb128 ");
    EmitHexUnsigned(abbrevId);
    if (verbose) {
        Emit(CMNT);
        Emit(dieTagName);
        Emit(" Offset= ");
        EmitHexUnsigned(offset);
        Emit(" (");
        EmitDecUnsigned(offset);
        Emit(" ),  Size= ");
        EmitHexUnsigned(size);
        Emit(" (");
        EmitDecUnsigned(size);
        Emit(" )\n");
    } else {
        Emit("\n");
    }
}

void AsmAssembler::EmitDIFormSpecification(unsigned int dwform)
{
    Emit("\t");
    switch (dwform) {
        case DW_FORM_string:
            Emit(".string");
            break;
        case DW_FORM_strp:
        case DW_FORM_data4:
        case DW_FORM_ref4:
            Emit(".4byte   ");
            break;
        case DW_FORM_data1:
            Emit(".byte    ");
            break;
        case DW_FORM_data2:
            Emit(".2byte   ");
            break;
        case DW_FORM_data8:
            Emit(".8byte   ");
            break;
        case DW_FORM_sec_offset:
            Emit(".4byte   ");
            break;
            /* if DWARF64, should be .8byte? */
        case DW_FORM_addr: /* Should we use DWARF64? for now, we generate .8byte as gcc does for DW_FORM_addr */
            Emit(".8byte   ");
            break;
        case DW_FORM_exprloc:
            Emit(".uleb128 ");
            break;
        default:
            assert(0 && "NYI");
            break;
    }
}

void AsmAssembler::EmitDebugComment(const char* comment)
{
    Emit("\t// ");
    Emit(comment);
    Emit("\n");
}

void AsmAssembler::EmitDwFormString(const std::string &name)
{
    Emit("\"");
    Emit(name);
    Emit("\"");
    Emit(CMNT "len = ");
    EmitDecUnsigned(name.length() + 1);
}

void AsmAssembler::EmitDwFormStrp(uint32 strLabelId, size_t strTableSize)
{
    Emit(".L" XSTR(DEBUG_STR_LABEL));
    outStream << strLabelId;
}

void AsmAssembler::EmitDwFormData(int32 attrValue, uint8 sizeInByte)
{
    EmitHexUnsigned(attrValue);
}

void AsmAssembler::EmitDwFormData8()
{
    Emit(".L" XSTR(TEXT_END) "-.L" XSTR(TEXT_BEGIN));
}

void AsmAssembler::EmitDwFormData8(uint32 endLabelFuncPuIdx, uint32 startLabelFuncPuIdx, uint32 endLabelIdx,
                                   uint32 startLabelIdx)
{
    outStream << ".L." << endLabelFuncPuIdx << "__" << endLabelIdx;
    Emit("-");
    outStream << ".L." << startLabelFuncPuIdx << "__" << startLabelIdx;
}

void AsmAssembler::EmitLabel(uint32 funcPuIdx, uint32 labIdx)
{
    outStream << ".L." << funcPuIdx << "__" << labIdx;
}

void AsmAssembler::EmitDwFormSecOffset()
{
    Emit(".L");
    Emit(XSTR(DEBUG_LINE_0));
}

void AsmAssembler::EmitDwFormAddr(bool emitTextBegin)
{
    if (emitTextBegin) {
        Emit(".L" XSTR(TEXT_BEGIN));
    } else {
        Emit("XXX--ADDR--XXX");
    }
}

void AsmAssembler::EmitDwFormRef4(uint64 offsetOrValue, bool unknownType, bool emitOffset)
{
    if (emitOffset) {
        Emit(" OFFSET ");
    }
    EmitHexUnsigned(offsetOrValue);
    if (unknownType) {
        Emit(CMNT "Warning: dummy type used");
    }
}

void AsmAssembler::EmitDwFormExprlocCfa(uint32 dwOp)
{
    EmitHexUnsigned(1);
    Emit("\n\t.byte    ");
    EmitHexUnsigned(dwOp);
}

void AsmAssembler::EmitDwFormExprlocAddr(uint32 dwOp, const std::string &addrStr)
{
    EmitHexUnsigned(k9ByteSize);
    Emit("\n\t.byte    ");
    EmitHexUnsigned(dwOp);
    Emit("\n\t.8byte   ");
    Emit(addrStr);
}

void AsmAssembler::EmitDwFormExprlocFbreg(uint32 dwOp, int fboffset, size_t sleb128Size)
{
    EmitHexUnsigned(1 + sleb128Size);
    Emit(CMNT "uleb128 size");
    Emit("\n\t.byte    ");
    EmitHexUnsigned(dwOp);
    Emit("\n\t.sleb128 ");
    EmitDecSigned(fboffset);
}

void AsmAssembler::EmitDwFormExprlocBregn(uint32 dwOp, const std::string &dwOpName)
{
    EmitHexUnsigned(k2Bytes);
    Emit(CMNT "size");
    Emit("\n\t.byte    ");
    EmitHexUnsigned(dwOp);
    Emit(CMNT);
    Emit(dwOpName);
    Emit("\n\t.sleb128 ");
    EmitDecSigned(0);
    Emit(CMNT "offset");
}

void AsmAssembler::EmitDwFormExprloc(uintptr elp)
{
    EmitHexUnsigned(elp);
}

void AsmAssembler::EmitDIDwName(const std::string &dwAtName, const std::string &dwForName)
{
    Emit(CMNT);
    Emit(dwAtName);
    Emit(" : ");
    Emit(dwForName);
}

void AsmAssembler::EmitDIDebugAbbrevDiae(bool verbose, uint32 abbrevId, uint32 tag, const std::string &dwTagName,
                                         bool withChildren)
{
    if (verbose) {
        Emit("\n");
    }
    Emit("\t.uleb128 ");
    EmitHexUnsigned(abbrevId);
    if (verbose) {
        Emit(CMNT "Abbrev Entry ID");
    }
    Emit("\n");
    /* TAG */
    Emit("\t.uleb128 ");
    EmitHexUnsigned(tag);
    if (verbose) {
        Emit(CMNT);
        Emit(dwTagName);
    }
    Emit("\n");
    /* children? */
    Emit("\t.byte    ");
    EmitHexUnsigned(static_cast<uint64>(withChildren));
    if (verbose) {
        Emit(withChildren ? CMNT "DW_CHILDREN_yes" : CMNT "DW_CHILDREN_no");
    }
    Emit("\n");
}

void AsmAssembler::EmitDIDebugAbbrevDiaePairItem(bool verbose, uint32 aplAt, uint32 aplFrom,
                                                 const std::string &dwAtName, const std::string &dwFromName)
{
    /* odd entry -- DW_AT_*, even entry -- DW_FORM_* */
    Emit("\t.uleb128 ");
    EmitHexUnsigned(aplAt);
    if (verbose) {
        Emit(CMNT);
        Emit(dwAtName);
    }
    Emit("\n");
    Emit("\t.uleb128 ");
    EmitHexUnsigned(aplFrom);
    if (verbose) {
        Emit(CMNT);
        Emit(dwFromName);
    }
    Emit("\n");
}

void AsmAssembler::EmitDIDebugStrSection(const std::vector<uint32> &strps, const std::vector<std::string> &debugStrs,
                                         uint64 size, size_t strTableSize)
{
    Emit("\t.section\t.debug_str,\"MS\",@progbits,1\n");
    for (int i = 0; i < static_cast<int>(debugStrs.size()); i++) {
        Emit(".L" XSTR(DEBUG_STR_LABEL));
        this->outStream << strps[i];
        Emit(":\n");
        Emit("\t.string \"");
        Emit(debugStrs[i]);
        Emit("\"\n");
    }
}

void AsmAssembler::EmitNull(uint64 sizeInByte)
{
    EmitDirective(kZero);
    Emit(sizeInByte);
    Emit("\n");
}

/* start of X64 instructions */
/* mov */
void AsmAssembler::Mov(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tmov");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Mov(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    Emit("\tmov");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolReg(immOpnd.first, immOpnd.second, reg);
    Emit("\n");
}

void AsmAssembler::Mov(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tmov");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Mov(InsnSize insnSize, Reg reg, const Mem &mem)
{
    Emit("\tmov");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegMem(reg, mem);
    Emit("\n");
}

void AsmAssembler::Mov(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    Emit("\tmov");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolMem(immOpnd.first, immOpnd.second, mem);
    Emit("\n");
}

/* floating point mov */
void AsmAssembler::Mov(Reg srcReg, Reg destReg, bool isMovD)
{
    if (isMovD) {
        Emit("\tmovd\t");
    } else {
        Emit("\tmovq\t");
    }
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::MovF(const Mem &mem, Reg reg, bool isSingle)
{
    if (isSingle) {
        Emit("\tmovss\t");
    } else {
        Emit("\tmovsd\t");
    }
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::MovF(Reg reg, const Mem &mem, bool isSingle)
{
    if (isSingle) {
        Emit("\tmovss\t");
    } else {
        Emit("\tmovsd\t");
    }
    EmitRegMem(reg, mem);
    Emit("\n");
}

/* movabs */
void AsmAssembler::Movabs(const ImmOpnd &immOpnd, Reg reg)
{
    Emit("\tmovabs");
    Emit("\t");
    EmitImmOrSymbolReg(immOpnd.first, immOpnd.second, reg);
    Emit("\n");
}

void AsmAssembler::Movabs(int64 symIdx, Reg reg)
{
    Emit("\tmovabs");
    Emit("\t");
    EmitLabelReg(symIdx, reg);
    Emit("\n");
}

/* push */
void AsmAssembler::Push(InsnSize insnSize, Reg reg)
{
    Emit("\tpush");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitReg(reg);
    Emit("\n");
}

/* pop */
void AsmAssembler::Pop(InsnSize insnSize, Reg reg)
{
    Emit("\tpop");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitReg(reg);
    Emit("\n");
}

/* lea */
void AsmAssembler::Lea(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tlea");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

/* movzx */
void AsmAssembler::MovZx(InsnSize sSize, InsnSize dSize, Reg srcReg, Reg destReg)
{
    Emit("\tmovz");
    EmitInsnSuffix(sSize);
    EmitInsnSuffix(dSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::MovZx(InsnSize sSize, InsnSize dSize, const Mem &mem, Reg reg)
{
    Emit("\tmovz");
    EmitInsnSuffix(sSize);
    EmitInsnSuffix(dSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

/* movsx */
void AsmAssembler::MovSx(InsnSize sSize, InsnSize dSize, Reg srcReg, Reg destReg)
{
    Emit("\tmovs");
    EmitInsnSuffix(sSize);
    EmitInsnSuffix(dSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::MovSx(InsnSize sSize, InsnSize dSize, const Mem &mem, Reg reg)
{
    Emit("\tmovs");
    EmitInsnSuffix(sSize);
    EmitInsnSuffix(dSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

/* add */
void AsmAssembler::Add(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tadd");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Add(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    Emit("\tadd");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolReg(immOpnd.first, immOpnd.second, reg);
    Emit("\n");
}

void AsmAssembler::Add(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tadd");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Add(InsnSize insnSize, Reg reg, const Mem &mem)
{
    Emit("\tadd");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegMem(reg, mem);
    Emit("\n");
}

void AsmAssembler::Add(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    Emit("\tadd");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolMem(immOpnd.first, immOpnd.second, mem);
    Emit("\n");
}

/* add floating point */
void AsmAssembler::Add(Reg srcReg, Reg destReg, bool isSingle)
{
    if (isSingle) {
        Emit("\taddss\t");
    } else {
        Emit("\taddsd\t");
    }
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Add(const Mem &mem, Reg reg, bool isSingle)
{
    if (isSingle) {
        Emit("\taddss\t");
    } else {
        Emit("\taddsd\t");
    }
    EmitMemReg(mem, reg);
    Emit("\n");
}

/* sub */
void AsmAssembler::Sub(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tsub");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Sub(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    Emit("\tsub");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolReg(immOpnd.first, immOpnd.second, reg);
    Emit("\n");
}

void AsmAssembler::Sub(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tsub");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Sub(InsnSize insnSize, Reg reg, const Mem &mem)
{
    Emit("\tsub");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegMem(reg, mem);
    Emit("\n");
}

void AsmAssembler::Sub(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    Emit("\tsub");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolMem(immOpnd.first, immOpnd.second, mem);
    Emit("\n");
}

/* sub floating point */
void AsmAssembler::Sub(Reg srcReg, Reg destReg, bool isSingle)
{
    if (isSingle) {
        Emit("\tsubss\t");
    } else {
        Emit("\tsubsd\t");
    }
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Sub(const Mem &mem, Reg reg, bool isSingle)
{
    if (isSingle) {
        Emit("\tsubss\t");
    } else {
        Emit("\tsubsd\t");
    }
    EmitMemReg(mem, reg);
    Emit("\n");
}

/* and */
void AsmAssembler::And(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tand");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::And(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tand");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::And(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    Emit("\tand");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolReg(immOpnd.first, immOpnd.second, reg);
    Emit("\n");
}

void AsmAssembler::And(InsnSize insnSize, Reg reg, const Mem &mem)
{
    Emit("\tand");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegMem(reg, mem);
    Emit("\n");
}

void AsmAssembler::And(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    Emit("\tand");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolMem(immOpnd.first, immOpnd.second, mem);
    Emit("\n");
}

/* or */
void AsmAssembler::Or(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tor");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Or(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tor");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Or(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    Emit("\tor");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolReg(immOpnd.first, immOpnd.second, reg);
    Emit("\n");
}

void AsmAssembler::Or(InsnSize insnSize, Reg reg, const Mem &mem)
{
    Emit("\tor");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegMem(reg, mem);
    Emit("\n");
}

void AsmAssembler::Or(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    Emit("\tor");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolMem(immOpnd.first, immOpnd.second, mem);
    Emit("\n");
}

/* xor */
void AsmAssembler::Xor(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\txor");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Xor(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    Emit("\txor");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolReg(immOpnd.first, immOpnd.second, reg);
    Emit("\n");
}

void AsmAssembler::Xor(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\txor");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Xor(InsnSize insnSize, Reg reg, const Mem &mem)
{
    Emit("\txor");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegMem(reg, mem);
    Emit("\n");
}

void AsmAssembler::Xor(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    Emit("\txor");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolMem(immOpnd.first, immOpnd.second, mem);
    Emit("\n");
}

/* bsr */
void AsmAssembler::Bsr(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\bbsr");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

/* not */
void AsmAssembler::Not(InsnSize insnSize, Reg reg)
{
    Emit("\tnot");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Not(InsnSize insnSize, const Mem &mem)
{
    Emit("\tnot");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMem(mem);
    Emit("\n");
}

/* neg */
void AsmAssembler::Neg(InsnSize insnSize, Reg reg)
{
    Emit("\tneg");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Neg(InsnSize insnSize, const Mem &mem)
{
    Emit("\tneg");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMem(mem);
    Emit("\n");
}

/* div & cwd, cdq, cqo */
void AsmAssembler::Idiv(InsnSize insnSize, Reg reg)
{
    Emit("\tidiv");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Idiv(InsnSize insnSize, const Mem &mem)
{
    Emit("\tidiv");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Div(InsnSize insnSize, Reg reg)
{
    Emit("\tdiv");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Div(InsnSize insnSize, const Mem &mem)
{
    Emit("\tdiv");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Cwd()
{
    Emit("\tcwd\n");
}

void AsmAssembler::Cdq()
{
    Emit("\tcdq\n");
}

void AsmAssembler::Cqo()
{
    Emit("\tcqo\n");
}

/* shl */
void AsmAssembler::Shl(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tshl");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Shl(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    Emit("\tshl");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolReg(immOpnd.first, immOpnd.second, reg);
    Emit("\n");
}

void AsmAssembler::Shl(InsnSize insnSize, Reg reg, const Mem &mem)
{
    Emit("\tshl");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegMem(reg, mem);
    Emit("\n");
}

void AsmAssembler::Shl(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    Emit("\tshl");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolMem(immOpnd.first, immOpnd.second, mem);
    Emit("\n");
}

/* sar */
void AsmAssembler::Sar(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tsar");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Sar(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    Emit("\tsar");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolReg(immOpnd.first, immOpnd.second, reg);
    Emit("\n");
}

void AsmAssembler::Sar(InsnSize insnSize, Reg reg, const Mem &mem)
{
    Emit("\tsar");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegMem(reg, mem);
    Emit("\n");
}

void AsmAssembler::Sar(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    Emit("\tsar");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolMem(immOpnd.first, immOpnd.second, mem);
    Emit("\n");
}

/* shr */
void AsmAssembler::Shr(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tshr");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Shr(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    Emit("\tshr");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolReg(immOpnd.first, immOpnd.second, reg);
    Emit("\n");
}

void AsmAssembler::Shr(InsnSize insnSize, Reg reg, const Mem &mem)
{
    Emit("\tshr");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegMem(reg, mem);
    Emit("\n");
}

void AsmAssembler::Shr(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    Emit("\tshr");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolMem(immOpnd.first, immOpnd.second, mem);
    Emit("\n");
}

/* jmp */
void AsmAssembler::Jmp(Reg reg)
{
    Emit("\tjmp\t");
    Emit("*");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Jmp(const Mem &mem)
{
    Emit("\tjmp\t");
    Emit("*");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Jmp(int64 symIdx)
{
    Emit("\tjmp\t");
    EmitLabel(symIdx);
    Emit("\n");
}

/* jump condition */
void AsmAssembler::Je(int64 symIdx)
{
    Emit("\tje\t");
    EmitLabel(symIdx);
    Emit("\n");
}

void AsmAssembler::Ja(int64 symIdx)
{
    Emit("\tja\t");
    EmitLabel(symIdx);
    Emit("\n");
}

void AsmAssembler::Jae(int64 symIdx)
{
    Emit("\tjae\t");
    EmitLabel(symIdx);
    Emit("\n");
}

void AsmAssembler::Jne(int64 symIdx)
{
    Emit("\tjne\t");
    EmitLabel(symIdx);
    Emit("\n");
}

void AsmAssembler::Jb(int64 symIdx)
{
    Emit("\tjb\t");
    EmitLabel(symIdx);
    Emit("\n");
}

void AsmAssembler::Jbe(int64 symIdx)
{
    Emit("\tjbe\t");
    EmitLabel(symIdx);
    Emit("\n");
}

void AsmAssembler::Jg(int64 symIdx)
{
    Emit("\tjg\t");
    EmitLabel(symIdx);
    Emit("\n");
}

void AsmAssembler::Jge(int64 symIdx)
{
    Emit("\tjge\t");
    EmitLabel(symIdx);
    Emit("\n");
}

void AsmAssembler::Jl(int64 symIdx)
{
    Emit("\tjl\t");
    EmitLabel(symIdx);
    Emit("\n");
}

void AsmAssembler::Jle(int64 symIdx)
{
    Emit("\tjle\t");
    EmitLabel(symIdx);
    Emit("\n");
}

/* cmp */
void AsmAssembler::Cmp(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmp");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmp(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tcmp");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Cmp(InsnSize insnSize, Reg reg, const Mem &mem)
{
    Emit("\tcmp");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegMem(reg, mem);
    Emit("\n");
}

void AsmAssembler::Cmp(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    Emit("\tcmp");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolReg(immOpnd.first, immOpnd.second, reg);
    Emit("\n");
}

void AsmAssembler::Cmp(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    Emit("\tcmp");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitImmOrSymbolMem(immOpnd.first, immOpnd.second, mem);
    Emit("\n");
}

/* test */
void AsmAssembler::Test(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\ttest");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

/* set */
void AsmAssembler::Setbe(Reg reg)
{
    Emit("\tsetbe\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Setbe(const Mem &mem)
{
    Emit("\tsetbe\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Setle(Reg reg)
{
    Emit("\tsetle\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Setle(const Mem &mem)
{
    Emit("\tsetle\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Setae(Reg reg)
{
    Emit("\tsetae\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Setae(const Mem &mem)
{
    Emit("\tsetae\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Setge(Reg reg)
{
    Emit("\tsetge\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Setge(const Mem &mem)
{
    Emit("\tsetge\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Setne(Reg reg)
{
    Emit("\tsetne\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Setne(const Mem &mem)
{
    Emit("\tsetne\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Setb(Reg reg)
{
    Emit("\tsetb\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Setb(const Mem &mem)
{
    Emit("\tsetb\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Setl(Reg reg)
{
    Emit("\tsetl\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Setl(const Mem &mem)
{
    Emit("\tsetl\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Seta(Reg reg)
{
    Emit("\tseta\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Seta(const Mem &mem)
{
    Emit("\tseta\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Setg(Reg reg)
{
    Emit("\tsetg\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Setg(const Mem &mem)
{
    Emit("\tsetg\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Sete(Reg reg)
{
    Emit("\tsete\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Sete(const Mem &mem)
{
    Emit("\tsete\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Seto(Reg reg)
{
    Emit("\tseto\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Seto(const Mem &mem)
{
    Emit("\tseto\t");
    EmitMem(mem);
    Emit("\n");
}

/* cmov */
void AsmAssembler::Cmova(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmova");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmova(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tcmova");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Cmovae(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmovae");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmovae(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tcmovae");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Cmovb(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmovb");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmovb(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tcmovb");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Cmovbe(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmovbe");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmovbe(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tcmovbe");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}
void AsmAssembler::Cmove(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmove");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmove(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tcmove");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Cmovg(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmovg");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmovg(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tcmovg");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Cmovge(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmovge");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmovge(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tcmovge");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Cmovl(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmovl");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmovl(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tcmovl");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Cmovle(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmovle");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmovle(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tcmovle");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Cmovo(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmovo");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmovne(InsnSize insnSize, const Mem &mem, Reg reg)
{
    Emit("\tcmovne");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

void AsmAssembler::Cmovne(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcmovne");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

/* call */
void AsmAssembler::Call(InsnSize insnSize, Reg reg)
{
    Emit("\tcall");
    EmitInsnSuffix(insnSize);
    Emit("\t*");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Call(InsnSize insnSize, const Mem &mem)
{
    Emit("\tcall");
    EmitInsnSuffix(insnSize);
    Emit("\t*");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Call(InsnSize insnSize, int64 symIdx)
{
    Emit("\tcall");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitLabel(symIdx);
    Emit("\n");
}

/* ret */
void AsmAssembler::Ret()
{
    Emit("\tret\n");
}

/* leave */
void AsmAssembler::Leave()
{
    Emit("\tleave\n");
}

/* imul */
void AsmAssembler::Imul(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\timul");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

/* mul float */
void AsmAssembler::Mul(Reg srcReg, Reg destReg, bool isSingle)
{
    if (isSingle) {
        Emit("\tmulss\t");
    } else {
        Emit("\tmulsd\t");
    }
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Mul(const Mem &mem, Reg reg, bool isSingle)
{
    if (isSingle) {
        Emit("\tmulss\t");
    } else {
        Emit("\tmulsd\t");
    }
    EmitMemReg(mem, reg);
    Emit("\n");
}

/* nop */
void AsmAssembler::Nop(InsnSize insnSize, const Mem &mem)
{
    Emit("\tnop");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitMem(mem);
    Emit("\n");
}

void AsmAssembler::Nop()
{
    Emit("\tnop\n");
}

/* byte swap */
void AsmAssembler::Bswap(InsnSize insnSize, Reg reg)
{
    Emit("\tbswap");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitReg(reg);
    Emit("\n");
}

void AsmAssembler::Xchg(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\txchg");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

/* pseudo insn */
void AsmAssembler::DealWithPseudoInst(const std::string &insn)
{
    Emit("\t");
    Emit(insn);
    Emit("\n");
}

/* floating point */
void AsmAssembler::MovF(Reg srcReg, Reg destReg, bool isSingle)
{
    if (isSingle) {
        Emit("\tmovss\t");
    } else {
        Emit("\tmovsd\t");
    }
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

/* floating point and */
void AsmAssembler::And(Reg srcReg, Reg destReg, bool isSingle)
{
    if (isSingle) {
        Emit("\tandps\t");
    } else {
        Emit("\tandpd\t");
    }
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::And(const Mem &mem, Reg reg, bool isSingle)
{
    if (isSingle) {
        Emit("\tandps\t");
    } else {
        Emit("\tandpd\t");
    }
    EmitMemReg(mem, reg);
    Emit("\n");
}

/* floating div */
void AsmAssembler::Divsd(Reg srcReg, Reg destReg)
{
    Emit("\tdivsd\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Divsd(const Mem &mem, Reg reg)
{
    Emit("\tdivsd\t");
    EmitMemReg(mem, reg);
    Emit("\n");
}

/* convert int2float */
void AsmAssembler::Cvtsi2ss(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcvtsi2ss");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cvtsi2sd(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcvtsi2sd");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

/*convert float2int */
void AsmAssembler::Cvttsd2si(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcvttsd2si");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cvttss2si(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Emit("\tcvttss2si");
    EmitInsnSuffix(insnSize);
    Emit("\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

/* convert float2float */
void AsmAssembler::Cvtss2sd(Reg srcReg, Reg destReg)
{
    Emit("\tcvtss2sd\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cvtsd2ss(Reg srcReg, Reg destReg)
{
    Emit("\tcvtsd2ss\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

/* unordered compare */
void AsmAssembler::Ucomisd(Reg srcReg, Reg destReg)
{
    Emit("\tucomisd\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Ucomiss(Reg srcReg, Reg destReg)
{
    Emit("\tucomiss\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Cmpeqsd(Reg srcReg, Reg destReg)
{
    Emit("\tcmpeqsd\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

/* float sqrt*/
void AsmAssembler::Sqrtss_r(Reg srcReg, Reg destReg)
{
    Emit("\tsqrtss\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}

void AsmAssembler::Sqrtsd_r(Reg srcReg, Reg destReg)
{
    Emit("\tsqrtsd\t");
    EmitRegReg(srcReg, destReg);
    Emit("\n");
}
/* end of X64 instructions */
} /* namespace assembler */
