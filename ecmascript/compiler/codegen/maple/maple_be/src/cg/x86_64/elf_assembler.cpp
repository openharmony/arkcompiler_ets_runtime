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

#include <climits>
#include "assembler/elf_assembler.h"

namespace assembler {
/* These used in ModRM mode when an instruction is encoded. */
const uint8 kSubModReg = 5;
const uint8 kAndModReg = 4;
const uint8 kOrModReg = 1;
const uint8 kXorModReg = 6;
const uint8 kNotModReg = 2;
const uint8 kNegModReg = 3;
const uint8 kIdivModReg = 7;
const uint8 kDivModReg = 6;
const uint8 kShlModReg = 4;
const uint8 kSarModReg = 7;
const uint8 kShrModReg = 5;
const uint8 kJmpModReg = 4;
const uint8 kCmpModReg = 7;
const uint8 kCallModReg = 2;

/* override function in base class */
void ElfAssembler::InitialFileInfo(const std::string &inputFileName)
{
    /* Initialize some sections that must be used. */
    DataSection *nullDataSection = new DataSection(" ", SHT_NULL, 0, 0);
    RegisterSection(*nullDataSection);
    strTabSection = new StringSection(".strtab", SHT_STRTAB, 0, 1);
    RegisterSection(*strTabSection);
    textSection = new DataSection(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, k8Bits);
    RegisterSection(*textSection);
    symbolTabSection = new SymbolSection(".symtab", SHT_SYMTAB, 0, k8Bits, *strTabSection);
    RegisterSection(*symbolTabSection);
}

void ElfAssembler::EmitVariable(int64 symIdx, uint64 sizeInByte, uint8 alignInByte, SymbolAttr symAttr,
                                SectionKind sectionKind)
{
    switch (sectionKind) {
        case kSBss:
        case kSComm:
        case kSTbss:
            EmitBssSectionVar(symIdx, sizeInByte, alignInByte, symAttr);
            break;
        case kSTdata:
        case kSData:
            EmitDataSectionVar(symIdx);
            break;
        case kSRodata:
            if (rodataSection == nullptr) {
                rodataSection = new DataSection(".rodata", SHT_PROGBITS, SHF_ALLOC, k8Bits);
                RegisterSection(*rodataSection);
            }
            UpdateLabel(symIdx, LabelType::kConst, rodataSection->GetDataSize());
            break;
        case kSText:
            UpdateLabel(symIdx);
            break;
        case kSDebugInfo:
        case kSDebugAbbrev:
        case kSDebugStr:
        default:
            assert(false && "unprocessed Section in EmitVariable");
            break;
    }
}

void ElfAssembler::EmitFloatValue(int64 symIdx, int64 value, size_t valueSize)
{
    auto reloffset = codeBuff.size();
    Encodeb(value, valueSize);

    if (valueSize == maplebe::k4ByteSize) {
        UpdateLabel(symIdx, LabelType::kFloatLabel, reloffset);
    } else if (valueSize == maplebe::k8ByteSize) {
        UpdateLabel(symIdx, LabelType::kDoubleLabel, reloffset);
    } else {
        CHECK_FATAL(false, "--Err: EmitFloatValue only handle float and double value");
    }
}

void ElfAssembler::EmitBssSectionVar(int64 symIdx, uint64 sizeInByte, uint8 alignInByte, SymbolAttr symAttr)
{
    if (bssSection == nullptr) {
        bssSection = new DataSection(".bss", SHT_NOBITS, SHF_WRITE | SHF_ALLOC, k8Bits);
        RegisterSection(*bssSection);
    }
    uint64 bssCurSize = bssSection->GetSectionSize();
    bssSection->SetSectionSize(static_cast<Xword>(bssCurSize + sizeInByte));
    if (symAttr == kSALocal) {
        const std::string &symbolName = GetNameFromSymMap(symIdx, true);
        auto nameIndex = strTabSection->AddString(symbolName);
        AddSymToSymTab({static_cast<Word>(nameIndex), static_cast<uint8>((STB_LOCAL << 4) + (STT_OBJECT & 0xf)), 0,
                        bssSection->GetIndex(), bssCurSize, sizeInByte},
                       symIdx);
        UpdateLabel(symIdx, LabelType::kLocalUninitialized, static_cast<uint32>(bssCurSize));
    } else {
        const std::string &symbolName = GetNameFromSymMap(symIdx);
        auto nameIndex = strTabSection->AddString(symbolName);
        AddSymToSymTab({static_cast<Word>(nameIndex), static_cast<uint8>((STB_GLOBAL << 4) + (STT_OBJECT & 0xf)), 0,
                        SHN_COMMON, static_cast<Address>(alignInByte), sizeInByte},
                       symIdx);
        UpdateLabel(symIdx, LabelType::kGlobalUninitialized, static_cast<uint32>(bssCurSize));
    }
}

void ElfAssembler::EmitDataSectionVar(int64 symIdx)
{
    if (dataSection == nullptr) {
        dataSection = new DataSection(".data", SHT_PROGBITS, SHF_WRITE | SHF_ALLOC, k8Bits);
        RegisterSection(*dataSection);
    }
    uint32 pos = dataSection->GetDataSize();
    UpdateLabel(symIdx, LabelType::kStatic, pos);
}

void ElfAssembler::EmitFunctionHeader(int64 symIdx, SymbolAttr funcAttr, const std::string *secName)
{
    const auto &emitMemoryManager = maplebe::CGOptions::GetInstance().GetEmitMemoryManager();
    if (emitMemoryManager.funcAddressSaver != nullptr) {
        const std::string &funcName = GetNameFromSymMap(symIdx);
        emitMemoryManager.funcAddressSaver(emitMemoryManager.codeSpace, funcName,
            static_cast<uint32>(lastModulePC + codeBuff.size()));
    }
    UpdateLabel(symIdx, LabelType::kFunc, static_cast<uint32>(codeBuff.size()));
}

void ElfAssembler::EmitBBLabel(int64 labelSymIdx, bool genVerboseInfo, uint32 freq, const std::string *mirName)
{
    UpdateLabel(labelSymIdx, LabelType::kBBLabel, static_cast<uint32>(codeBuff.size()));
}

void ElfAssembler::EmitJmpTableElem(int64 jmpLabelIdx, const std::vector<int64> &labelSymIdxs)
{
    UpdateLabel(jmpLabelIdx, LabelType::kJmpLabel, static_cast<uint32>(codeBuff.size()));
    const size_t kLabelSize = 8;
    for (auto labelSymIdx : labelSymIdxs) {
        AppendFixup(labelSymIdx, kAbsolute64, {static_cast<uint32>(codeBuff.size()), kLabelSize}, fixups);
        uint8 imm = 0;
        Encodeb(imm, kLabelSize);
    }
}

void ElfAssembler::EmitFunctionFoot(int64 symIdx, SymbolAttr funcAttr)
{
    uint64 funcSymValue = static_cast<uint64>(GetLabelRelOffset(symIdx));
    uint64 funcSymSize = static_cast<uint64>(GetLabelSize(symIdx));
    uint8 funcSymType = STB_GLOBAL;
    switch (funcAttr) {
        case kSALocal:
            funcSymType = STB_LOCAL;
            break;
        case kSAGlobal:
            funcSymType = STB_GLOBAL;
            break;
        case kSAWeak:
            funcSymType = STB_WEAK;
            break;
        case kSAStatic:
        case kSAHidden:
        default:
            assert(false && "unkonwn/unsupport SymbolAttr in EmitFunctionFoot");
            break;
    }
    const std::string &symbolName = GetNameFromSymMap(symIdx);
    auto nameIndex = strTabSection->AddString(symbolName);
    AddSymToSymTab({static_cast<Word>(nameIndex),
                    static_cast<uint8>((funcSymType << kLeftShift4Bits) + (STT_FUNC & 0xf)), 0, textSection->GetIndex(),
                    funcSymValue, funcSymSize},
                   symIdx);
}

void ElfAssembler::EmitDirectString(const std::string &str, bool belongsToDataSec, int64 strSymIdx, bool emitAscii)
{
    /* Add a terminator to a string. */
    std::string ustr = str;
    ustr += '\0';
    if (strSymIdx != 0) {
        if (dataSection == nullptr) {
            dataSection = new DataSection(".data", SHT_PROGBITS, SHF_WRITE | SHF_ALLOC, k8Bits);
            RegisterSection(*dataSection);
        }
        uint32 pos = dataSection->GetDataSize();
        UpdateLabel(strSymIdx, LabelType::kStrLabel, pos);
        dataSection->AppendData(ustr.data(), ustr.size());
        const size_t kStrAlignSize = 8;
        /* append size, append 0 when align need. */
        size_t appendSize = kStrAlignSize - ustr.size() % kStrAlignSize;
        int64 appendData = 0;
        dataSection->AppendData(appendData, appendSize);
    } else {
        if (belongsToDataSec) {
            dataSection->AppendData(ustr.data(), ustr.size());
        } else {
            rodataSection->AppendData(ustr.data(), ustr.size());
        }
    }
}

void ElfAssembler::EmitIndirectString(int64 strSymIdx, bool belongsToDataSec)
{
    const size_t kStrAddrSize = 8;
    uint32 pos = 0;
    int64 addr = 0;
    if (belongsToDataSec) {
        pos = dataSection->GetDataSize();
        dataSection->AppendData(addr, kStrAddrSize);
        AppendFixup(strSymIdx, kAbsolute64, {pos, kStrAddrSize}, dataFixups);
    } else {
        pos = rodataSection->GetDataSize();
        rodataSection->AppendData(addr, kStrAddrSize);
        AppendFixup(strSymIdx, kAbsolute64, {pos, kStrAddrSize}, rodataFixups);
    }
}

void ElfAssembler::EmitIntValue(int64 value, size_t valueSize, bool belongsToDataSec)
{
    if (belongsToDataSec) {
        dataSection->AppendData(value, valueSize);
    } else {
        rodataSection->AppendData(value, valueSize);
    }
}

void ElfAssembler::EmitAddrValue(int64 symIdx, int32 symAddrOfs, int32 structFieldOfs, bool belongsToDataSec)
{
    const size_t kAddrSize = 8;
    uint32 pos = 0;
    int64 addr = 0;
    if (belongsToDataSec) {
        pos = dataSection->GetDataSize();
        dataSection->AppendData(addr, kAddrSize);
        AppendFixup(symIdx, kAbsolute64, {pos, kAddrSize}, dataFixups, symAddrOfs);
    } else {
        pos = rodataSection->GetDataSize();
        rodataSection->AppendData(addr, kAddrSize);
        AppendFixup(symIdx, kAbsolute64, {pos, kAddrSize}, rodataFixups, symAddrOfs);
    }
}

void ElfAssembler::EmitAddrOfFuncValue(int64 symIdx, bool belongsToDataSec)
{
    EmitLabelValue(symIdx, belongsToDataSec);
}

void ElfAssembler::EmitLabelValue(int64 symIdx, bool belongsToDataSec)
{
    const size_t kAddrSize = 8;
    uint32 pos = 0;
    int64 addr = 0;
    if (belongsToDataSec) {
        pos = dataSection->GetDataSize();
        dataSection->AppendData(addr, kAddrSize);
        AppendFixup(symIdx, kAbsolute64, {pos, kAddrSize}, dataFixups);
    } else {
        pos = rodataSection->GetDataSize();
        rodataSection->AppendData(addr, kAddrSize);
        AppendFixup(symIdx, kAbsolute64, {pos, kAddrSize}, rodataFixups);
    }
}

void ElfAssembler::EmitBitFieldValue(uint64 combineBitFieldValue, bool belongsToDataSec)
{
    if (belongsToDataSec) {
        dataSection->AppendData(static_cast<int64>(combineBitFieldValue), 1);
    } else {
        rodataSection->AppendData(static_cast<int64>(combineBitFieldValue), 1);
    }
}

void ElfAssembler::EmitNull(uint64 sizeInByte)
{
    int64 data = 0;
    dataSection->AppendData(data, static_cast<size_t>(sizeInByte));
}

void ElfAssembler::PostEmitVariable(int64 symIdx, SymbolAttr symAttr, uint64 sizeInByte, bool belongsToTextSec)
{
    Label *label = labelManager.at(symIdx);
    uint64 pos = static_cast<uint64>(label->GetRelOffset());
    auto secIdx = belongsToTextSec ? textSection->GetIndex() : dataSection->GetIndex();
    if (symAttr == kSALocal) {
        const std::string &symbolName = GetNameFromSymMap(symIdx, true);
        auto index = strTabSection->AddString(symbolName);
        AddSymToSymTab({static_cast<Word>(index), static_cast<uint8>((STB_LOCAL << 4) + (STT_OBJECT & 0xf)), 0, secIdx,
            pos, sizeInByte}, symIdx);
    } else {
        const std::string &symbolName = GetNameFromSymMap(symIdx);
        auto index = strTabSection->AddString(symbolName);
        uint8 symInfo = symAttr == kSAGlobal ? STB_GLOBAL : STB_WEAK;
        AddSymToSymTab({static_cast<Word>(index), static_cast<uint8>((symInfo << 4) + (STT_OBJECT & 0xf)), 0, secIdx,
            pos, sizeInByte}, symIdx);
    }
}

void ElfAssembler::FinalizeFileInfo()
{
    AppendGlobalSymsToSymTabSec();
    HandleTextSectionFixup();
    HandleDataSectionFixup();
    HandleRodataSectionFixup();
    HandleDebugInfoSectionFixup();
    WriteElfFile();
}

/* encode function */
void ElfAssembler::OpReg(Reg reg, uint8 opCode1, uint8 opCode2, uint8 modReg)
{
    if (HasOpndSizePrefix(reg)) {
        Encodeb(0x66);
    }
    uint8 rex = GetRex(reg);
    if (rex != 0) {
        Encodeb(rex);
    }
    Encodeb(opCode1 | (GetRegSize(reg) == k8Bits ? 0 : 1));
    if (opCode2 != 0) {
        Encodeb(opCode2);
    }
    uint8 modrm = GetRegCodeId(reg);
    SetModRM(GetMod(reg), modReg, modrm);
}

void ElfAssembler::OpMem(const Mem &mem, uint8 opCode1, uint8 opCode2, uint8 modReg)
{
    if (HasOpndSizePrefix(mem)) {
        Encodeb(0x66);
    }

    if (HasAddrSizePrefix(mem)) {
        Encodeb(0x67);
    }

    uint8 rex = GetRex(mem);
    if (rex != 0) {
        Encodeb(rex);
    }
    Encodeb(opCode1 | (mem.size == k8Bits ? 0 : 1));
    if (opCode2 != 0) {
        Encodeb(opCode2);
    }
    uint8 modrm = 0;
    if (!HasSIB(mem)) {
        modrm = GetRegCodeId(mem.base);
    } else {
        modrm = 0b100; /* r/m=b100, use SIB */
    }
    SetModRM(GetMod(mem), modReg, modrm);
    if (HasSIB(mem)) {
        Encodeb(GetSIB(mem));
    }
    OpDisp(mem);
}

void ElfAssembler::OpDisp(const Mem &mem)
{
    int64 symIdx = mem.disp.first;
    uint64 offset = static_cast<uint64>(mem.disp.second);
    if (symIdx != 0) {
        if (!CanEncodeLabel(symIdx)) {
            size_t offsetSize = 4;
            UpdateLabel(symIdx);
            AppendFixup(symIdx, kRelative, {static_cast<uint32>(codeBuff.size()), offsetSize}, fixups, mem.disp.second);
            uint8 imm = 0;
            Encodeb(imm, offsetSize);
        }
    } else if (offset == 0) {
        if (mem.memType == kOnlyBase && (mem.base == RBP || mem.base == R13)) {
            Encodeb(offset);
        } else if (mem.base == RIP) {
            Encoded(offset);
        } else {
            return;
        }
    } else {
        if (mem.base != RIP && Is8Bits(offset)) {
            Encodeb(offset); /* 8-bit displacement */
        } else {
            Encoded(offset); /* 32-bit displacement */
        }
    }
}

void ElfAssembler::OpRR(Reg reg1, Reg reg2, uint8 opCode1, uint8 opCode2, bool extInsn)
{
    if (!extInsn && (HasOpndSizePrefix(reg1) || HasOpndSizePrefix(reg2))) {
        Encodeb(0x66);
    }
    uint8 rex = extInsn ? GetRex(reg2, reg1) : GetRex(reg1, reg2);
    if (rex != 0) {
        Encodeb(rex);
    }
    Encodeb(opCode1 | (GetRegSize(reg1) == k8Bits ? 0 : 1));
    if (opCode2 != 0) {
        Encodeb(opCode2);
    }
    uint8 modrm = extInsn ? GetModRM(reg2, reg1) : GetModRM(reg1, reg2);
    if (modrm != 0) {
        Encodeb(modrm);
    }
}

void ElfAssembler::OpRM(Reg reg, const Mem &mem, uint8 opCode1, uint8 opCode2, bool extInsn)
{
    if (!extInsn && HasOpndSizePrefix(reg)) {
        Encodeb(0x66);
    }
    if (!extInsn && HasAddrSizePrefix(mem)) {
        Encodeb(0x67);
    }
    uint8 rex = GetRex(mem, reg);
    if (rex != 0) {
        Encodeb(rex);
    }
    Encodeb(opCode1 | (GetRegSize(reg) == k8Bits ? 0 : 1));
    if (opCode2 != 0) {
        Encodeb(opCode2);
    }
    uint8 modrm = GetModRM(reg, mem);
    Encodeb(modrm);
    if (HasSIB(mem)) {
        Encodeb(GetSIB(mem));
    }
    OpDisp(mem);
}

void ElfAssembler::OpImmAndReg(const ImmOpnd &immOpnd, Reg reg, uint8 opCode, uint8 modReg)
{
    bool isSymbol = immOpnd.second;
    uint32 imm = static_cast<uint32>(immOpnd.first); /* When isSymbol is true, this is index. */
    uint8 immBit = Is8Bits(imm) ? k8Bits : (Is16Bits(imm) ? k16Bits : k32Bits);
    uint8 regSize = GetRegSize(reg);
    if (regSize == k8Bits) {
        immBit = k8Bits;
    }
    if (immBit == k16Bits && (regSize == k64Bits || regSize == k32Bits)) {
        immBit = k32Bits; /* if 32/64bit mode, imm val can not use 16-bit. */
    }
    immBit = isSymbol ? k32Bits : immBit;
    if (GetRegId(reg) == 0 && (regSize == immBit || (regSize == k64Bits && immBit == k32Bits))) {
        if (HasOpndSizePrefix(reg)) {
            Encodeb(0x66);
        }
        if (GetRex(reg)) {
            Encodeb(GetRex(reg));
        }
        Encodeb(opCode | 0x4 | (immBit == k8Bits ? 0 : 1));
    } else {
        uint8 tmp = immBit < std::min(static_cast<uint32>(regSize), 32U) ? 2 : 0;
        OpReg(reg, 0x80 | tmp, 0, modReg);
    }
    if (isSymbol) {
        if (!CanEncodeLabel(immOpnd.first)) {
            UpdateLabel(immOpnd.first);
            AppendFixup(immOpnd.first, kRelative, {static_cast<uint32>(codeBuff.size()), immBit / k8Bits}, fixups);
            imm = 0;
            Encodeb(imm, immBit / k8Bits);
        }
    } else {
        Encodeb(imm, immBit / k8Bits);
    }
}

void ElfAssembler::OpImmAndMem(const ImmOpnd &immOpnd, const Mem &mem, uint8 modReg)
{
    bool isSymbol = immOpnd.second;
    uint32 imm = static_cast<uint32>(immOpnd.first); /* When isSymbol is true, this is index. */
    if (isSymbol) {
        if (!CanEncodeLabel(immOpnd.first)) {
            size_t offsetSize = 4;
            UpdateLabel(immOpnd.first);
            AppendFixup(immOpnd.first, kRelative, {static_cast<uint32>(codeBuff.size()), offsetSize}, fixups);
            imm = 0;
            OpMem(mem, 0x80, 0, modReg);
            Encodeb(imm, offsetSize);
        }
    } else {
        uint8 immBit = Is8Bits(imm) ? k8Bits : (Is16Bits(imm) ? k16Bits : k32Bits);
        if (mem.size == k8Bits) {
            immBit = k8Bits;
        }
        if (immBit == k16Bits && (mem.size == k64Bits || mem.size == k32Bits)) {
            immBit = k32Bits; /* if 32/64bit mode, imm val can not use 16-bit. */
        }
        uint8 tmp = immBit < std::min(static_cast<uint32>(mem.size), 32U) ? 2 : 0;
        OpMem(mem, 0x80 | tmp, 0, modReg);
        Encodeb(imm, immBit / k8Bits);
    }
}

void ElfAssembler::MovRegAndDisp(Reg reg, const Mem &mem, uint8 opCode)
{
    if (HasOpndSizePrefix(reg)) {
        Encodeb(0x66);
    }
    if (HasAddrSizePrefix(mem)) {
        Encodeb(0x67);
    }
    uint8 rex = GetRex(mem, reg);
    if (rex != 0) {
        Encodeb(rex);
    }
    Encodeb(opCode | (GetRegSize(reg) == k8Bits ? 0 : 1));
    int64 symIdx = mem.disp.first;
    uint64 offset = static_cast<uint64>(mem.disp.second);
    if (symIdx != 0) {
        size_t offsetSize = k8Bits;
        Encodeb(static_cast<uint8>(0), offsetSize);
        UpdateLabel(symIdx);
        AppendFixup(symIdx, kAbsolute64, {static_cast<uint32>(codeBuff.size()), offsetSize}, fixups);
    }
    if (Is64Bits(offset)) {
        Encodeq(offset);
    } else {
        Encoded(offset);
    }
}

void ElfAssembler::OpPushPop(Reg reg, uint8 code)
{
    if (HasOpndSizePrefix(reg)) {
        Encodeb(0x66);
    }
    if (IsRegExt(reg)) {
        Encodeb(0x41); /* Rex prefix */
    }
    Encodeb(code | GetRegCodeId(reg));
}

void ElfAssembler::JmpToLabel(int64 labelIdx, uint8 opCode1, uint8 opCode2, size_t offsetSize)
{
    Encodeb(opCode1);
    if (opCode2 != 0) {
        Encodeb(opCode2);
    }
    if (!CanEncodeLabel(labelIdx)) {
        UpdateLabel(labelIdx);
        AppendFixup(labelIdx, kRelative, {static_cast<uint32>(codeBuff.size()), offsetSize}, fixups);
        uint8 imm = 0;
        Encodeb(imm, offsetSize);
    }
}

void ElfAssembler::OpCmovcc(Reg srcReg, Reg dstReg, uint8 opCode1, uint8 opCode2)
{
    if (HasOpndSizePrefix(srcReg) || HasOpndSizePrefix(dstReg)) {
        Encodeb(0x66);
    }
    uint8 rex = GetRex(dstReg, srcReg);
    if (rex != 0) {
        Encodeb(rex);
    }
    Encodeb(opCode1);
    Encodeb(opCode2);
    uint8 modrm = GetModRM(dstReg, srcReg);
    if (modrm != 0) {
        Encodeb(modrm);
    }
}

void ElfAssembler::UpdateLabel(int64 labelIdx, LabelType type, uint32 relOffset)
{
    if (labelManager.count(labelIdx) == 0) {
        Label *label = new Label(labelIdx, relOffset, type);
        (void)labelManager.emplace(labelIdx, label);
    } else {
        Label *label = labelManager.at(labelIdx);
        if (type != LabelType::kLNone) {
            label->SetLabelType(type);
        }
        if (relOffset != 0xFFFFFFFFU) {
            label->SetRelOffset(relOffset);
        }
    }
}

bool ElfAssembler::CanEncodeLabel(int64 labelIdx)
{
    if (labelManager.count(labelIdx) != 0) {
        Label *label = labelManager.at(labelIdx);
        uint32 relOffset = label->GetRelOffset();
        LabelType labelType = label->GetLabelType();
        bool canEncode = (labelType == LabelType::kBBLabel || labelType == LabelType::kFunc ||
                          labelType == LabelType::kDoubleLabel || labelType == LabelType::kFloatLabel);
        if (canEncode && relOffset != 0xFFFFFFFFU) {
            size_t offsetSize = 4;
            uint64 offset = static_cast<uint64>((relOffset - codeBuff.size()) - offsetSize);
            Encodeb(offset, offsetSize);
            return true;
        }
    }
    return false;
}

uint32 ElfAssembler::GetLabelSize(int64 labelIdx) const
{
    return static_cast<uint32>(codeBuff.size()) - GetLabelRelOffset(labelIdx);
}

uint32 ElfAssembler::GetLabelRelOffset(int64 labelIdx) const
{
    if (labelManager.count(labelIdx) != 0) {
        Label *label = labelManager.at(labelIdx);
        assert(label->GetRelOffset() != 0xFFFFFFFFU && "label's relOffset doesn't exist");
        return label->GetRelOffset();
    }
    return 0;
}

void ElfAssembler::AppendFixup(int64 labelIdx, FixupKind kind, const std::pair<uint32, size_t> &offsetPair,
                               std::vector<Fixup *> &tmpFixups, int64 disp)
{
    tmpFixups.push_back(new Fixup(labelIdx, kind, offsetPair, disp));
}

/* elf file */
void ElfAssembler::InitElfHeader()
{
    header.e_ident[EI_MAG0] = ELFMAG0;
    header.e_ident[EI_MAG1] = ELFMAG1;
    header.e_ident[EI_MAG2] = ELFMAG2;
    header.e_ident[EI_MAG3] = ELFMAG3;
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA] = ELFDATA2LSB;
    header.e_ident[EI_VERSION] = EV_CURRENT;
    header.e_ident[EI_OSABI] = ELFOSABI_NONE; /* ELFOSABI_NONE represents UNIX System V */
    header.e_ident[EI_ABIVERSION] = 0;
    (void)std::fill_n(&header.e_ident[EI_PAD], EI_NIDENT - EI_PAD, 0);
    header.e_type = ET_REL;
    header.e_machine = EM_X86_64;
    header.e_version = EV_CURRENT;
    header.e_entry = 0;
    header.e_phoff = 0;
    header.e_shoff = 0; /* later get */
    header.e_flags = 0; /* The Intel architecture defines no flags; so this member contains zero. */
    header.e_ehsize = sizeof(FileHeader);
    header.e_phentsize = 0;
    header.e_phnum = 0;
    header.e_shentsize = sizeof(SectionHeader);
    header.e_shnum = static_cast<uint16>(sections.size());
    header.e_shstrndx = strTabSection->GetIndex();
}

void ElfAssembler::RegisterSection(Section &section)
{
    sections.push_back(&section);
    DEBUG_ASSERT(sections.size() > 0, "must not be zero");
    section.SetIndex(static_cast<SectionIndex>(sections.size() - 1));
}

void ElfAssembler::LayoutSections()
{
    globalOffset = sizeof(FileHeader);
    globalOffset = Alignment::Align<Offset>(globalOffset, k8Bits);

    for (auto *section : sections) {
        section->SetSectionHeaderNameIndex(static_cast<Word>(strTabSection->AddString(section->GetName())));
    }

    for (auto *section : sections) {
        globalOffset = Alignment::Align<Offset>(globalOffset, section->GetAlign());
        /* lay out section */
        UpdateSectionOffset(*section);
        if (section->GetType() != SHT_NOBITS) {
            section->GenerateData();
        }
        UpdateGlobalOffset(*section);
    }

    globalOffset = Alignment::Align<Offset>(globalOffset, 16U);
    header.e_shoff = globalOffset;
    header.e_shnum = static_cast<uint16>(sections.size());
}

void ElfAssembler::UpdateSectionOffset(Section &section)
{
    if (section.GetType() != SHT_NOBITS) {
        section.SetOffset(globalOffset);
    } else {
        section.SetOffset(0);
    }
}

void ElfAssembler::UpdateGlobalOffset(Section &section)
{
    if (section.GetType() != SHT_NOBITS) {
        globalOffset += section.GetSectionSize();
    }
}

void ElfAssembler::SetFileOffset(uint64 offset)
{
    (void)outStream.seekp(offset);
}

/* symIdx is the key used to get symbol's index in .symtab */
void ElfAssembler::AddSymToSymTab(const Symbol &symbol, int64 symIdx)
{
    const int kGetHigh4Bits = 4;
    if ((symbol.st_info >> kGetHigh4Bits) == STB_LOCAL) {
        localSymTab.push_back(std::make_pair(symbol, symIdx));
    } else {
        symTab.push_back(std::make_pair(symbol, symIdx));
    }
}

void ElfAssembler::AppendRela(const Label &label, const std::pair<uint32, size_t> &offsetPair, uint64 type,
                              Sxword addend)
{
    LabelType labelType = label.GetLabelType();
    int64 relOffset = static_cast<int64>(label.GetRelOffset());
    uint64 offset = static_cast<uint64>(offsetPair.first);
    int64 offsetSize = static_cast<int64>(offsetPair.second);
    if (labelType == LabelType::kConst) {
        int64 rodataSecSymIdx = ~rodataSection->GetIndex() + 1;
        relaSection->AppendRela(
            {offset,
             static_cast<Xword>((symbolTabSection->GetIdxInSymbols(rodataSecSymIdx) << kLeftShift32Bits) +
                                (type & 0xffffffff)),
             relOffset});
    } else if (labelType == LabelType::kGlobal) {
        addend -= offsetSize;
        relaSection->AppendRela(
            {offset,
             static_cast<Xword>((symbolTabSection->GetIdxInSymbols(label.GetlabelIdx()) << kLeftShift32Bits) +
                                (type & 0xffffffff)),
             addend});
    } else if (labelType == LabelType::kStatic) {
        addend += relOffset - offsetSize;
        int64 dataSecSymIdx = ~dataSection->GetIndex() + 1;
        relaSection->AppendRela(
            {offset,
             static_cast<Xword>((symbolTabSection->GetIdxInSymbols(dataSecSymIdx) << kLeftShift32Bits) +
                                (type & 0xffffffff)),
             addend});
    } else if (labelType == LabelType::kLocalUninitialized) {
        addend = addend + relOffset - offsetSize;
        int64 bssSecSymIdx = ~bssSection->GetIndex() + 1;
        relaSection->AppendRela(
            {offset,
             static_cast<Xword>((symbolTabSection->GetIdxInSymbols(bssSecSymIdx) << kLeftShift32Bits) +
                                (type & 0xffffffff)),
             addend});
    } else if (labelType == LabelType::kGlobalUninitialized) {
        addend = addend - offsetSize;
        relaSection->AppendRela(
            {offset,
             static_cast<Xword>((symbolTabSection->GetIdxInSymbols(label.GetlabelIdx()) << kLeftShift32Bits) +
                                (type & 0xffffffff)),
             addend});
    } else if (labelType == LabelType::kJmpLabel) {
        type = R_X86_64_32;
        addend = relOffset;
        int64 textSecSymIdx = ~textSection->GetIndex() + 1;
        relaSection->AppendRela(
            {offset,
             static_cast<Xword>((symbolTabSection->GetIdxInSymbols(textSecSymIdx) << kLeftShift32Bits) +
                                (type & 0xffffffff)),
             addend});
    } else if (labelType == LabelType::kBBLabel) {
        addend = relOffset;
        int64 textSecSymIdx = ~textSection->GetIndex() + 1;
        relaSection->AppendRela(
            {offset,
             static_cast<Xword>((symbolTabSection->GetIdxInSymbols(textSecSymIdx) << kLeftShift32Bits) +
                                (type & 0xffffffff)),
             addend});
    } else if (labelType == LabelType::kFunc ||
               (label.GetRelOffset() == 0xFFFFFFFFU && labelType == LabelType::kLNone)) {
        int64 labelIdx = label.GetlabelIdx();
        if (!symbolTabSection->ExistSymInSymbols(labelIdx)) {
            symbolTabSection->AppendIdxInSymbols(labelIdx);
        }
        relaSection->AppendRela({offsetPair.first,
                                 static_cast<Xword>((symbolTabSection->GetIdxInSymbols(labelIdx) << kLeftShift32Bits) +
                                                    (type & 0xffffffff)),
                                 addend});
    } else {
        assert(false && "unsupported label type in func AddRela");
    }
}

uint64 ElfAssembler::GetRelaType(FixupKind kind) const
{
    switch (kind) {
        case kRelative:
            return R_X86_64_PC32;
        case kRelative64:
            return R_X86_64_PC64;
        case kAbsolute:
            return R_X86_64_32;
        case kAbsolute64:
            return R_X86_64_64;
        case kPLT:
            return R_X86_64_PLT32;
        case kFNone:
            return R_X86_64_NONE;
    }
}

void ElfAssembler::HandleTextSectionFixup()
{
    if (!fixups.empty()) {
        relaSection =
            new RelaSection(".rela.text", SHT_RELA, SHF_INFO_LINK, textSection->GetIndex(), k8Bits, *symbolTabSection);
        RegisterSection(*relaSection);
    }

    for (auto fixup : fixups) {
        int64 labelIdx = fixup->GetlabelIdx();
        if (labelManager.count(labelIdx) == 0) {
            continue;
        }

        const std::pair<uint32, size_t> &offsetPair = fixup->GetOffset();
        Label *label = labelManager.at(labelIdx);
        uint32 relOffset = label->GetRelOffset();
        LabelType labelType = label->GetLabelType();

        FixupKind fixupKind = fixup->GetFixupKind();
        if ((fixupKind == kRelative || fixupKind == kRelative64) &&
            (labelType == LabelType::kBBLabel || labelType == LabelType::kFunc)) {
            FixupEncode(offsetPair.first, relOffset, offsetPair.second);
            fixup->SetFixupKind(kFNone);
        }

        if (relOffset != 0xFFFFFFFFU && fixupKind == kPLT) {
            FixupEncode(offsetPair.first, relOffset, offsetPair.second);
            fixup->SetFixupKind(kFNone);
        }

        fixupKind = fixup->GetFixupKind();
        uint64 type = GetRelaType(fixupKind);
        int64 addend = (fixupKind == kAbsolute || fixupKind == kAbsolute64) ? 0 : -0x4;
        if (fixupKind != kFNone) {
            addend = labelType == LabelType::kGlobalUninitialized || labelType == LabelType::kLocalUninitialized ||
                             labelType == LabelType::kGlobal || labelType == LabelType::kStatic
                         ? fixup->GetDisp()
                         : addend;
            AppendRela(*label, offsetPair, type, addend);
        }
    }
    textSection->AppendData(codeBuff.data(), codeBuff.size());
}

void ElfAssembler::HandleDataSectionFixup()
{
    if (!dataFixups.empty()) {
        relaDataSection =
            new RelaSection(".rela.data", SHT_RELA, SHF_INFO_LINK, dataSection->GetIndex(), k8Bits, *symbolTabSection);
        RegisterSection(*relaDataSection);
    }
    for (auto fixup : dataFixups) {
        int64 labelIdx = fixup->GetlabelIdx();
        std::pair<uint32, size_t> offset = fixup->GetOffset();
        const uint32 relocType = R_X86_64_64;
        if (labelManager.count(labelIdx) == 0) {
            continue;
        }
        Label *label = labelManager.at(labelIdx);
        LabelType labelType = label->GetLabelType();
        int64 addend = 0;
        int64 relOffset = static_cast<int64>(label->GetRelOffset());
        if (labelType == LabelType::kGlobalUninitialized) {
            addend = fixup->GetDisp();
            uint64 pos = symbolTabSection->GetIdxInSymbols(labelIdx);
            relaDataSection->AppendRela(
                {offset.first, static_cast<Xword>((pos << kLeftShift32Bits) + (relocType & 0xffffffff)), addend});
        } else if (labelType == LabelType::kLocalUninitialized) {
            addend = fixup->GetDisp();
            int64 bssSecSymIdx = ~bssSection->GetIndex() + 1;
            relaDataSection->AppendRela(
                {offset.first,
                 static_cast<Xword>((symbolTabSection->GetIdxInSymbols(bssSecSymIdx) << kLeftShift32Bits) +
                                    (relocType & 0xffffffff)),
                 addend});
        } else if (labelType == LabelType::kFunc) {
            uint64 pos = symbolTabSection->GetIdxInSymbols(labelIdx);
            relaDataSection->AppendRela(
                {offset.first, static_cast<Xword>((pos << kLeftShift32Bits) + (relocType & 0xffffffff)), addend});
        } else if (labelType == LabelType::kStrLabel || labelType == LabelType::kGlobal ||
                   labelType == LabelType::kStatic) {
            uint64 pos = symbolTabSection->GetIdxInSymbols(~dataSection->GetIndex() + 1);
            addend = (labelType == LabelType::kGlobal || labelType == LabelType::kStatic) ? fixup->GetDisp() + relOffset
                                                                                          : relOffset;
            relaDataSection->AppendRela(
                {offset.first, static_cast<Xword>((pos << kLeftShift32Bits) + (relocType & 0xffffffff)), addend});
        } else {
            addend = relOffset;
            int64 textSecSymIdx = ~textSection->GetIndex() + 1;
            relaDataSection->AppendRela(
                {offset.first,
                 static_cast<Xword>((symbolTabSection->GetIdxInSymbols(textSecSymIdx) << kLeftShift32Bits) +
                                    (relocType & 0xffffffff)),
                 addend});
        }
    }
}

void ElfAssembler::HandleRodataSectionFixup()
{
    if (!rodataFixups.empty()) {
        relaRodataSection = new RelaSection(".rela.rodata", SHT_RELA, SHF_INFO_LINK, rodataSection->GetIndex(), k8Bits,
                                            *symbolTabSection);
        RegisterSection(*relaRodataSection);
    }
    for (auto fixup : rodataFixups) {
        int64 labelIdx = fixup->GetlabelIdx();
        std::pair<uint32, size_t> offset = fixup->GetOffset();
        const uint32 relocType = R_X86_64_64;
        if (labelManager.count(labelIdx) == 0) {
            continue;
        }
        Label *label = labelManager.at(labelIdx);
        LabelType labelType = label->GetLabelType();
        int64 addend = 0;
        int64 relOffset = static_cast<int64>(label->GetRelOffset());
        if (labelType == LabelType::kGlobalUninitialized || labelType == LabelType::kLocalUninitialized) {
            addend = relOffset;
            uint64 pos = symbolTabSection->GetIdxInSymbols(~textSection->GetIndex() + 1);
            relaRodataSection->AppendRela(
                {offset.first, static_cast<Xword>((pos << kLeftShift32Bits) + (relocType & 0xffffffff)), addend});
        }
    }
}

void ElfAssembler::WriteElfFile()
{
    /* Init elf file header */
    InitElfHeader();

    LayoutSections();

    const auto &emitMemoryManager = maplebe::CGOptions::GetInstance().GetEmitMemoryManager();
    if (emitMemoryManager.codeSpace != nullptr) {
        DEBUG_ASSERT(textSection != nullptr, "textSection has not been initialized");
        uint8 *codeSpace = emitMemoryManager.allocateDataSection(emitMemoryManager.codeSpace,
            textSection->GetSectionSize(), textSection->GetAlign(), textSection->GetName());
        auto res = memcpy_s(codeSpace, textSection->GetSectionSize(), textSection->GetData().data(),
                            textSection->GetDataSize());
        CHECK_FATAL(res == EOK, "memcpy failed");
        if (CGOptions::addFuncSymbol()) {
            uint8 *symtabSpace = emitMemoryManager.allocateDataSection(
                emitMemoryManager.codeSpace, symbolTabSection->GetDataSize(), symbolTabSection->GetAlign(),
                symbolTabSection->GetName().c_str());
            res = memcpy_s(symtabSpace, symbolTabSection->GetDataSize(), symbolTabSection->GetAddr(),
                           symbolTabSection->GetDataSize());
            CHECK_FATAL(res == EOK, "memcpy failed");
            uint8 *stringTabSpace =
                emitMemoryManager.allocateDataSection(emitMemoryManager.codeSpace, strTabSection->GetDataSize(),
                                                      strTabSection->GetAlign(), strTabSection->GetName().c_str());
            res = memcpy_s(stringTabSpace, strTabSection->GetDataSize(), strTabSection->GetData().data(),
                           strTabSection->GetDataSize());
            CHECK_FATAL(res == EOK, "memcpy failed");
        }
        return;
    }

    /* write header */
    Emit(&header, sizeof(header));

    /* write sections */
    for (auto *section : sections) {
        if (section->GetType() == SHT_NOBITS) {
            continue;
        }
        SetFileOffset(section->GetOffset());
        section->WriteSection(outFStream);
    }

    /* write section table */
    SetFileOffset(header.e_shoff);
    for (auto *section : sections) {
        Emit(&section->GetSectionHeader(), sizeof(section->GetSectionHeader()));
    }
}

/* Append the symbol of non-empty and necessary section to symbol table section. */
void ElfAssembler::AppendSecSymsToSymTabSec()
{
    for (Section *section : sections) {
        if (section->GetType() != SHT_PROGBITS && section->GetType() != SHT_NOBITS) {
            continue;
        }
        DataSection *dataSec = static_cast<DataSection *>(section);
        if (section->GetFlags() == (SHF_ALLOC | SHF_EXECINSTR) || section->GetSectionSize() != 0 ||
            (dataSec != nullptr && dataSec->GetDataSize() != 0)) {
            auto nameIndex = strTabSection->AddString(section->GetName());
            symbolTabSection->AppendSymbol({static_cast<Word>(nameIndex),
                                            static_cast<uint8>((STB_LOCAL << kLeftShift4Bits) + (STT_SECTION & 0xf)), 0,
                                            section->GetIndex(), 0, 0});
            /* Indexed by the inverse of the section index. */
            int64 secSymIdx = ~section->GetIndex() + 1;
            symbolTabSection->AppendIdxInSymbols(secSymIdx);
        }
    }
}

void ElfAssembler::AppendGlobalSymsToSymTabSec()
{
    for (auto elem : symTab) {
        const Symbol &symbol = elem.first;
        int64 symIdx = elem.second;
        symbolTabSection->AppendSymbol(symbol);
        symbolTabSection->AppendIdxInSymbols(symIdx);
    }
}

void ElfAssembler::AppendSymsToSymTabSec()
{
    /* emit local symbol */
    for (auto elem : localSymTab) {
        Symbol symbol = elem.first;
        int64 symIdx = elem.second;
        symbolTabSection->AppendSymbol(symbol);
        symbolTabSection->AppendIdxInSymbols(symIdx);
    }

    /* Append section symbol that may be used in relocation item, section is local. */
    AppendSecSymsToSymTabSec();

    /* set .symtab's info : index of the first non-local symbol */
    symbolTabSection->SetInfo(symbolTabSection->GetSymbolsSize());

    /* emit global and other symbol */
    for (auto elem : symTab) {
        const Symbol &symbol = elem.first;
        int64 symIdx = elem.second;
        symbolTabSection->AppendSymbol(symbol);
        symbolTabSection->AppendIdxInSymbols(symIdx);
    }
}

/* emit debug info */
void ElfAssembler::EmitDIDebugInfoSectionHeader(uint64 debugInfoLength)
{
    debugInfoSection = new DataSection(".debug_info", SHT_PROGBITS, 0, 1);
    RegisterSection(*debugInfoSection);
    /* length of .debug_info section, 4 bytes */
    size_t debugInfoLenSize = 4;
    debugInfoSection->AppendData(static_cast<int64>(debugInfoLength), debugInfoLenSize);
    size_t dwarfVersionSize = 2;
    /* DWARF version, 2 bytes */
    debugInfoSection->AppendData(static_cast<int64>(kDwarfVersion), dwarfVersionSize);
    /* debug_abbrev_offset. 4 bytes for dwarf32, 8 bytes for dwarf64 */
    int64 debugAbbrevOffset = 0;
    size_t debugAbbrevOffsetSize = 4;
    /* If labelSymIdx equals LLONG_MAX, there is not a real label bound to the fixup. */
    AppendFixup(LLONG_MAX, kAbsolute, {debugInfoSection->GetDataSize(), debugAbbrevOffsetSize}, debugInfoFixups);
    debugInfoSection->AppendData(debugAbbrevOffset, debugAbbrevOffsetSize);
    /* address size. 1 byte */
    size_t byteOfkSizeOfPTR = 1;
    debugInfoSection->AppendData(kSizeOfPTR, byteOfkSizeOfPTR);
}

void ElfAssembler::EmitDIDebugInfoSectionAbbrevId(bool verbose, uint32 abbrevId, const std::string &dieTagName,
                                                  uint32 offset, uint32 size)
{
    auto abbrevIdUleb128 = EncodeULEB128(abbrevId);
    debugInfoSection->AppendData(&abbrevIdUleb128, abbrevIdUleb128.size());
}

/* EmitDIAttrValue */
void ElfAssembler::EmitDwFormString(const std::string &name)
{
    debugInfoSection->AppendData(&name, name.size());
}

void ElfAssembler::EmitDwFormStrp(uint32 strLabelId, size_t strTableSize)
{
    int64 labelSymIdx = CalculateStrLabelSymIdx(static_cast<int64>(strLabelId), static_cast<int64>(strTableSize));
    UpdateLabel(labelSymIdx, LabelType::kDebugStrLabel);
    int64 strLabelOffset = 0;
    size_t strLabelOffsetSize = 4;
    AppendFixup(labelSymIdx, kAbsolute, {debugInfoSection->GetDataSize(), strLabelOffsetSize}, debugInfoFixups);
    debugInfoSection->AppendData(strLabelOffset, strLabelOffsetSize);
}

void ElfAssembler::EmitDwFormData(int32 attrValue, uint8 sizeInByte)
{
    debugInfoSection->AppendData(attrValue, sizeInByte);
}

void ElfAssembler::EmitDwFormData8()
{
    int64 addr = 0;
    size_t addrSizeInByte = 8;
    debugInfoSection->AppendData(addr, addrSizeInByte);
}

void ElfAssembler::EmitDwFormData8(uint32 endLabelFuncPuIdx, uint32 startLabelFuncPuIdx, uint32 endLabelIdx,
                                   uint32 startLabelIdx)
{
    int64 addr = 0;
    size_t addrSizeInByte = 8;
    debugInfoSection->AppendData(addr, addrSizeInByte);
}

void ElfAssembler::EmitLabel(uint32 funcPuIdx, uint32 labIdx)
{
    int64 labSymIdx = CalculateLabelSymIdx(funcPuIdx, labIdx);
    UpdateLabel(labIdx);
    int64 addr = 0;
    size_t addrSizeInByte = 8;
    AppendFixup(labSymIdx, kAbsolute64, {debugInfoSection->GetDataSize(), addrSizeInByte}, debugInfoFixups);
    debugInfoSection->AppendData(addr, addrSizeInByte);
}

void ElfAssembler::EmitDwFormSecOffset()
{
    int64 lineLabelOffset = 0;
    size_t lineLabelOffsetSize = 4;
    /* If labelSymIdx equals  - 2, there is not a real label bound to the fixup. */
    AppendFixup(LLONG_MAX - 2, kAbsolute, {debugInfoSection->GetDataSize(), lineLabelOffsetSize}, debugInfoFixups);
    debugInfoSection->AppendData(lineLabelOffset, lineLabelOffsetSize);
}

void ElfAssembler::EmitDwFormAddr(bool emitTextBegin)
{
    if (emitTextBegin) {
        int64 addr = 0;
        size_t addrSizeInByte = 8;
        /* If labelSymIdx equals LLONG_MAX - 1, there is not a real label bound to the fixup. */
        AppendFixup(LLONG_MAX - 1, kAbsolute64, {debugInfoSection->GetDataSize(), addrSizeInByte}, debugInfoFixups);
        debugInfoSection->AppendData(addr, addrSizeInByte);
    }
}

void ElfAssembler::EmitDwFormRef4(uint64 offsetOrValue, bool unknownType, bool emitOffset)
{
    size_t offsetOrValueSize = 4;
    debugInfoSection->AppendData(static_cast<int64>(offsetOrValue), offsetOrValueSize);
}

void ElfAssembler::EmitDwFormExprlocCfa(uint32 dwOp)
{
    debugInfoSection->AppendData(1, 1);
    debugInfoSection->AppendData(static_cast<int64>(dwOp), 1);
}

void ElfAssembler::EmitDwFormExprlocAddr(uint32 dwOp, const std::string &addrStr)
{
    debugInfoSection->AppendData(static_cast<int64>(k9ByteSize), 1);
    debugInfoSection->AppendData(static_cast<int64>(dwOp), 1);
    size_t addStrSize = 8;
    debugInfoSection->AppendData(&addrStr, addStrSize);
}

void ElfAssembler::EmitDwFormExprlocFbreg(uint32 dwOp, int fboffset, size_t sleb128Size)
{
    auto sleb128SizeEncode = EncodeSLEB128(1 + static_cast<int64>(sleb128Size));
    debugInfoSection->AppendData(&sleb128SizeEncode, sleb128SizeEncode.size());
    debugInfoSection->AppendData(static_cast<int64>(dwOp), 1);
    auto fboffsetSleb128 = EncodeSLEB128(fboffset);
    debugInfoSection->AppendData(&fboffsetSleb128, fboffsetSleb128.size());
}

void ElfAssembler::EmitDwFormExprlocBregn(uint32 dwOp, const std::string &dwOpName)
{
    debugInfoSection->AppendData(static_cast<int64>(k2Bytes), 1);
    debugInfoSection->AppendData(static_cast<int64>(dwOp), 1);
    debugInfoSection->AppendData(&dwOpName, dwOpName.size());
    int64 offset = 0;
    debugInfoSection->AppendData(offset, 1);
}

void ElfAssembler::EmitDwFormExprloc(uintptr elp)
{
    auto elpUleb128 = EncodeULEB128(elp);
    debugInfoSection->AppendData(&elpUleb128, elpUleb128.size());
}

void ElfAssembler::EmitDIDebugAbbrevDiae(bool verbose, uint32 abbrevId, uint32 tag, const std::string &dwTagName,
                                         bool withChildren)
{
    debugAbbrevSection = new DataSection(".debug_abbrev", SHT_PROGBITS, 0, 1);
    RegisterSection(*debugAbbrevSection);
    /* Abbrev Entry ID */
    auto abbrevIdUleb128 = EncodeULEB128(abbrevId);
    debugAbbrevSection->AppendData(&abbrevIdUleb128, abbrevIdUleb128.size());
    /* TAG */
    auto tagUleb128 = EncodeULEB128(tag);
    debugAbbrevSection->AppendData(&tagUleb128, tagUleb128.size());
    /* children */
    auto childrenValue = withChildren ? 1 : 0;
    debugAbbrevSection->AppendData(childrenValue, 1);
}

void ElfAssembler::EmitDIDebugAbbrevDiaePairItem(bool verbose, uint32 aplAt, uint32 aplFrom,
                                                 const std::string &dwAtName, const std::string &dwFromName)
{
    /* odd entry -- DW_AT_*, even entry -- DW_FORM_* */
    auto aplAtUleb128 = EncodeULEB128(aplAt);
    debugAbbrevSection->AppendData(&aplAtUleb128, aplAtUleb128.size());
    auto aplFromUleb128 = EncodeULEB128(aplFrom);
    debugAbbrevSection->AppendData(&aplFromUleb128, aplFromUleb128.size());
}

void ElfAssembler::EmitDIDebugSectionEnd(SectionKind secKind)
{
    int64 value = 0;
    size_t valueSizeInByte = 1;
    switch (secKind) {
        case kSDebugInfo:
            debugInfoSection->AppendData(value, valueSizeInByte);
            break;
        case kSDebugAbbrev:
            debugAbbrevSection->AppendData(value, valueSizeInByte);
            break;
        case kSBss:
        case kSComm:
        case kSData:
        case kSRodata:
        case kSTbss:
        case kSTdata:
        case kSText:
        case kSDebugStr:
        default:
            assert(false && "unsupport SectionKind in EmitDIDebugSectionEnd");
            break;
    }
}

void ElfAssembler::EmitDIDebugStrSection(const std::vector<uint32> &strps, const std::vector<std::string> &debugStrs,
                                         uint64 size, size_t strTableSize)
{
    debugStrSection = new DataSection(".debug_str", SHT_PROGBITS, SHF_MASKPROC, 1);
    RegisterSection(*debugStrSection);
    for (int i = 0; i < static_cast<int>(debugStrs.size()); i++) {
        int64 strLabSymIdx = CalculateStrLabelSymIdx(size, strps[i], strTableSize);
        UpdateLabel(strLabSymIdx, LabelType::kDebugStrLabel, debugStrSection->GetDataSize());
        debugStrSection->AppendData(&debugStrs[i], debugStrs[i].size());
        EmitDIDebugSectionEnd(kSDebugStr);
    }
}

void ElfAssembler::HandleDebugInfoSectionFixup()
{
    if (!debugInfoFixups.empty()) {
        relaDebugInfoSection = new RelaSection(".rela.debug_info", SHT_RELA, SHF_INFO_LINK,
                                               debugInfoSection->GetIndex(), k8Bits, *symbolTabSection);
        RegisterSection(*relaDebugInfoSection);
    }
    for (auto fixup : debugInfoFixups) {
        int64 labelIdx = fixup->GetlabelIdx();
        const std::pair<uint32, size_t> &offsetPair = fixup->GetOffset();
        FixupKind fixupKind = fixup->GetFixupKind();
        uint64 relocType = GetRelaType(fixupKind);
        int64 addend = fixup->GetDisp();
        int64 textSecSymIdx = ~textSection->GetIndex() + 1;
        int64 debugLineSecSymIdx = ~debugLineSection->GetIndex() + 1;
        int64 abbrevSecSymIdx = ~debugAbbrevSection->GetIndex() + 1;
        uint64 pos = labelIdx == LLONG_MAX
                         ? symbolTabSection->GetIdxInSymbols(debugLineSecSymIdx)
                         : (labelIdx == LLONG_MAX - 1 ? symbolTabSection->GetIdxInSymbols(textSecSymIdx)
                                                      : symbolTabSection->GetIdxInSymbols(abbrevSecSymIdx));
        if (!labelManager.count(labelIdx)) {
            relaDebugInfoSection->AppendRela(
                {offsetPair.first, static_cast<Xword>((pos << kLeftShift32Bits) + (relocType & 0xffffffff)), addend});
            continue;
        }
        Label *label = labelManager.at(labelIdx);
        LabelType labelType = label->GetLabelType();
        addend = label->GetRelOffset();
        if (labelType == LabelType::kBBLabel) {
            pos = symbolTabSection->GetIdxInSymbols(textSecSymIdx);
        } else if (labelType == LabelType::kDebugStrLabel) {
            pos = symbolTabSection->GetIdxInSymbols(~debugStrSection->GetIndex() + 1);
        } else {
            assert(false && "unsupport label type in HandleDebugInfoSectionFixup!");
        }
        relaDebugInfoSection->AppendRela(
            {offsetPair.first, static_cast<Xword>((pos << kLeftShift32Bits) + (relocType & 0xffffffff)), addend});
    }
}

/* start of X64 instructions */
/* mov */
void ElfAssembler::Mov(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpRR(srcReg, destReg, 0x88, 0);
}

void ElfAssembler::Mov(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    bool isSymbol = immOpnd.second;
    uint64 imm = static_cast<uint64>(immOpnd.first); /* When isSymbol is true, this is index. */
    uint8 regSize = GetRegSize(reg);
    uint8 regId = GetRegCodeId(reg);
    uint8 code = 0xB0 | ((regSize == k8Bits ? 0 : 1) << kLeftShift3Bits);
    if (HasOpndSizePrefix(reg)) {
        Encodeb(0x66);
    }
    if (GetRex(reg) != 0) {
        Encodeb(GetRex(reg));
    }
    if (regSize == k64Bits && (isSymbol || Is32Bits(imm))) {
        Encodeb(0xC7);
        code = 0xC0;
        regSize = k32Bits;
    }
    size_t offsetSize = isSymbol ? k64Bits : regSize / k8Bits;
    Encodeb(code | regId);
    if (isSymbol) {
        UpdateLabel(immOpnd.first);
        AppendFixup(immOpnd.first, kAbsolute64, {static_cast<uint32>(codeBuff.size()), offsetSize}, fixups);
        imm = 0;
    }
    Encodeb(imm, offsetSize);
}

void ElfAssembler::Mov(InsnSize insnSize, const Mem &mem, Reg reg)
{
    if (GetRegId(reg) == 0 && mem.memType == kOnlyDisp) {
        MovRegAndDisp(reg, mem, 0xA0);
    } else {
        OpRM(reg, mem, 0x8A, 0);
    }
}

void ElfAssembler::Mov(InsnSize insnSize, Reg reg, const Mem &mem)
{
    if (GetRegId(reg) == 0 && mem.memType == kOnlyDisp) {
        MovRegAndDisp(reg, mem, 0xA2);
    } else {
        OpRM(reg, mem, 0x88, 0);
    }
}

void ElfAssembler::Mov(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    bool isSymbol = immOpnd.second;
    uint32 imm = static_cast<uint32>(immOpnd.first); /* When isSymbol is true, this is index. */
    uint8 immBit = Is8Bits(imm) ? k8Bits : (Is16Bits(imm) ? k16Bits : k32Bits);
    if (mem.size == k8Bits) {
        immBit = k8Bits;
    }
    if (immBit == k16Bits && (mem.size == k64Bits || mem.size == k32Bits)) {
        immBit = k32Bits; /* if 32/64bit mode, imm val can not use 16-bit. */
    }
    immBit = isSymbol ? k64Bits : immBit;
    size_t immSize = immBit / k8Bits;
    OpMem(mem, 0xC6, 0, 0);
    if (isSymbol) {
        UpdateLabel(immOpnd.first);
        AppendFixup(immOpnd.first, kAbsolute64, {static_cast<uint32>(codeBuff.size()), immSize}, fixups);
        imm = 0;
    }
    Encodeb(imm, immSize);
}

/* floating point mov */
void ElfAssembler::Mov(Reg srcReg, Reg destReg, bool isMovD)
{
    uint8 srcRegSize = GetRegSize(srcReg);
    uint8 destRegSize = GetRegSize(destReg);
    if (srcRegSize == k128Bits || destRegSize == k128Bits) {
        Encodeb(0x66);
    }
    if (srcRegSize == k128Bits) {
        OpRR(srcReg, destReg, 0x0F, 0x7E);
    } else if (destRegSize == k128Bits) {
        OpRR(destReg, srcReg, 0x0F, 0x6E);
    }
}

void ElfAssembler::MovF(const Mem &mem, Reg reg, bool isSingle)
{
    if (isSingle) {
        Encodeb(0xF3);
    } else {
        Encodeb(0xF2);
    }
    OpRM(reg, mem, 0x0F, 0x10);
}

void ElfAssembler::MovF(Reg reg, const Mem &mem, bool isSingle)
{
    if (isSingle) {
        Encodeb(0xF3);
    } else {
        Encodeb(0xF2);
    }
    OpRM(reg, mem, 0x0F, 0x11);
}

/* movabs */
void ElfAssembler::Movabs(const ImmOpnd &immOpnd, Reg reg)
{
    bool isSymbol = immOpnd.second;
    uint64 imm = static_cast<uint64>(immOpnd.first); /* When isSymbol is true, this is index. */
    if (GetRex(reg) != 0) {
        Encodeb(GetRex(reg));
    }
    Encodeb(0xB8 | GetRegCodeId(reg));
    size_t offsetSize = 8;
    if (isSymbol) {
        UpdateLabel(immOpnd.first);
        AppendFixup(immOpnd.first, kAbsolute64, {static_cast<uint32>(codeBuff.size()), offsetSize}, fixups);
        imm = 0;
    }
    Encodeb(imm, offsetSize);
}

void ElfAssembler::Movabs(int64 symIdx, Reg reg)
{
    if (GetRex(reg) != 0) {
        Encodeb(GetRex(reg));
    }
    Encodeb(0xB8 | GetRegCodeId(reg));
    size_t offsetSize = 8;
    size_t offset = codeBuff.size() - offsetSize;
    UpdateLabel(symIdx);
    AppendFixup(symIdx, kAbsolute64, {offset, offsetSize}, fixups);
    uint8 imm = 0;
    Encodeb(imm, offsetSize);
}

/* push */
void ElfAssembler::Push(InsnSize insnSize, Reg reg)
{
    OpPushPop(reg, 0x50);
}

/* pop */
void ElfAssembler::Pop(InsnSize insnSize, Reg reg)
{
    OpPushPop(reg, 0x58);
}

/* lea */
void ElfAssembler::Lea(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x8C);
}

/* movzx */
void ElfAssembler::MovZx(InsnSize sSize, InsnSize dSize, Reg srcReg, Reg destReg)
{
    OpRR(srcReg, destReg, 0x0F, 0xB6 | (GetRegSize(srcReg) == k8Bits ? 0 : 1), true);
}

void ElfAssembler::MovZx(InsnSize sSize, InsnSize dSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0F, 0xB6 | (mem.size == k8Bits ? 0 : 1), true);
}

/* movsx */
void ElfAssembler::MovSx(InsnSize sSize, InsnSize dSize, Reg srcReg, Reg destReg)
{
    uint8 code1 = 0x0F;
    uint8 code2 = 0xBE | (GetRegSize(srcReg) == k8Bits ? 0 : 1);
    if (GetRegSize(srcReg) == k32Bits && GetRegSize(destReg) == k64Bits) {
        code1 = 0x63;
        code2 = 0;
    }
    OpRR(srcReg, destReg, code1, code2, true);
}

void ElfAssembler::MovSx(InsnSize sSize, InsnSize dSize, const Mem &mem, Reg reg)
{
    uint8 code1 = 0x0F;
    uint8 code2 = 0xBE | (mem.size == k8Bits ? 0 : 1);
    if (mem.size == k32Bits && GetRegSize(reg) == k64Bits) {
        code1 = 0x63;
        code2 = 0;
    }
    OpRM(reg, mem, code1, code2, true);
}

/* add */
void ElfAssembler::Add(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpRR(srcReg, destReg, 0x00);
}

void ElfAssembler::Add(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    OpImmAndReg(immOpnd, reg, 0x00, 0);
}

void ElfAssembler::Add(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x02);
}

void ElfAssembler::Add(InsnSize insnSize, Reg reg, const Mem &mem)
{
    OpRM(reg, mem, 0x00);
}

void ElfAssembler::Add(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    OpImmAndMem(immOpnd, mem, 0);
}

/* add floating point */
void ElfAssembler::Add(Reg srcReg, Reg destReg, bool isSingle)
{
    if (isSingle) {
        Encodeb(0xF3);
    } else {
        Encodeb(0xF2);
    }
    OpRR(destReg, srcReg, 0x0F, 0x58);
}

void ElfAssembler::Add(const Mem &mem, Reg reg, bool isSingle)
{
    if (isSingle) {
        Encodeb(0xF3);
    } else {
        Encodeb(0xF2);
    }
    OpRM(reg, mem, 0x0F, 0x58);
}

/* sub */
void ElfAssembler::Sub(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpRR(srcReg, destReg, 0x28);
}

void ElfAssembler::Sub(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    OpImmAndReg(immOpnd, reg, 0x28, kSubModReg);
}

void ElfAssembler::Sub(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x2A);
}

void ElfAssembler::Sub(InsnSize insnSize, Reg reg, const Mem &mem)
{
    OpRM(reg, mem, 0x28);
}

void ElfAssembler::Sub(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    OpImmAndMem(immOpnd, mem, kSubModReg);
}

/* sub floating point */
void ElfAssembler::Sub(Reg srcReg, Reg destReg, bool isSingle)
{
    if (isSingle) {
        Encodeb(0xF3);
    } else {
        Encodeb(0xF2);
    }
    OpRR(destReg, srcReg, 0x0F, 0x5c);
}

void ElfAssembler::Sub(const Mem &mem, Reg reg, bool isSingle)
{
    if (isSingle) {
        Encodeb(0xF3);
    } else {
        Encodeb(0xF2);
    }
    OpRM(reg, mem, 0x0F, 0x5c);
}

/* and */
void ElfAssembler::And(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpRR(srcReg, destReg, 0x20);
}

void ElfAssembler::And(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x21);
}

void ElfAssembler::And(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    OpImmAndReg(immOpnd, reg, 0x20, kAndModReg);
}

void ElfAssembler::And(InsnSize insnSize, Reg reg, const Mem &mem)
{
    OpRM(reg, mem, 0x20);
}

void ElfAssembler::And(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    OpImmAndMem(immOpnd, mem, kAndModReg);
}

/* or */
void ElfAssembler::Or(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpRR(srcReg, destReg, 0x08);
}

void ElfAssembler::Or(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0A);
}

void ElfAssembler::Or(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    OpImmAndReg(immOpnd, reg, 0x08, kOrModReg);
}

void ElfAssembler::Or(InsnSize insnSize, Reg reg, const Mem &mem)
{
    OpRM(reg, mem, 0x08);
}

void ElfAssembler::Or(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    OpImmAndMem(immOpnd, mem, kOrModReg);
}

/* xor */
void ElfAssembler::Xor(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpRR(srcReg, destReg, 0x30);
}

void ElfAssembler::Xor(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    OpImmAndReg(immOpnd, reg, 0x30, kXorModReg);
}

void ElfAssembler::Xor(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x32);
}

void ElfAssembler::Xor(InsnSize insnSize, Reg reg, const Mem &mem)
{
    OpRM(reg, mem, 0x30);
}

void ElfAssembler::Xor(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    OpImmAndMem(immOpnd, mem, kXorModReg);
}

/* bsr */
void ElfAssembler::Bsr(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpRR(srcReg, destReg, 0x0f, 0xbd);
}

/* not */
void ElfAssembler::Not(InsnSize insnSize, Reg reg)
{
    OpReg(reg, 0xF6, 0, kNotModReg);
}

void ElfAssembler::Not(InsnSize insnSize, const Mem &mem)
{
    OpMem(mem, 0xF6, 0, kNotModReg);
}

/* neg */
void ElfAssembler::Neg(InsnSize insnSize, Reg reg)
{
    OpReg(reg, 0xF6, 0, kNegModReg);
}

void ElfAssembler::Neg(InsnSize insnSize, const Mem &mem)
{
    OpMem(mem, 0xF6, 0, kNegModReg);
}

/* div & cwd, cdq, cqo */
void ElfAssembler::Idiv(InsnSize insnSize, Reg reg)
{
    OpReg(reg, 0xF6, 0, kIdivModReg);
}

void ElfAssembler::Idiv(InsnSize insnSize, const Mem &mem)
{
    OpMem(mem, 0xF6, 0, kIdivModReg);
}

void ElfAssembler::Div(InsnSize insnSize, Reg reg)
{
    OpReg(reg, 0xF6, 0, kDivModReg);
}

void ElfAssembler::Div(InsnSize insnSize, const Mem &mem)
{
    OpMem(mem, 0xF6, 0, kDivModReg);
}

void ElfAssembler::Cwd()
{
    Encodeb(0x66);
    Encodeb(0x99);
}

void ElfAssembler::Cdq()
{
    Encodeb(0x99);
}

void ElfAssembler::Cqo()
{
    Encodeb(0x48);
    Encodeb(0x99);
}

/* shl */
void ElfAssembler::Shl(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpReg(destReg, 0xD2, 0, kShlModReg);
}

void ElfAssembler::Shl(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    OpReg(reg, 0xC0, 0, kShlModReg);
    Encodeb(static_cast<uint64>(immOpnd.first));
}

void ElfAssembler::Shl(InsnSize insnSize, Reg reg, const Mem &mem)
{
    OpMem(mem, 0xD2, 0, kShlModReg);
}

void ElfAssembler::Shl(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    OpMem(mem, 0xC0, 0, kShlModReg);
    Encodeb(static_cast<uint64>(immOpnd.first));
}

/* sar */
void ElfAssembler::Sar(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpReg(destReg, 0xD2, 0, kSarModReg);
}

void ElfAssembler::Sar(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    OpReg(reg, 0xC0, 0, kSarModReg);
    Encodeb(static_cast<uint64>(immOpnd.first));
}

void ElfAssembler::Sar(InsnSize insnSize, Reg reg, const Mem &mem)
{
    OpMem(mem, 0xD2, 0, kSarModReg);
}

void ElfAssembler::Sar(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    OpMem(mem, 0xC0, 0, kSarModReg);
    Encodeb(static_cast<uint64>(immOpnd.first));
}

/* shr */
void ElfAssembler::Shr(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpReg(destReg, 0xD2, 0, kShrModReg);
}

void ElfAssembler::Shr(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    OpReg(reg, 0xC0, 0, kShrModReg);
    Encodeb(static_cast<uint64>(immOpnd.first));
}

void ElfAssembler::Shr(InsnSize insnSize, Reg reg, const Mem &mem)
{
    OpMem(mem, 0xD2, 0, kShrModReg);
}

void ElfAssembler::Shr(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    OpMem(mem, 0xC0, 0, kShrModReg);
    Encodeb(static_cast<uint64>(immOpnd.first));
}

/* jmp */
void ElfAssembler::Jmp(Reg reg)
{
    OpReg(reg, 0xFF, 0, kJmpModReg);
}

void ElfAssembler::Jmp(const Mem &mem)
{
    OpMem(mem, 0xFF, 0, kJmpModReg);
}

void ElfAssembler::Jmp(int64 symIdx)
{
    JmpToLabel(symIdx, 0xE9);
}

/* jump condition */
void ElfAssembler::Je(int64 symIdx)
{
    JmpToLabel(symIdx, 0x0F, 0x84);
}

void ElfAssembler::Ja(int64 symIdx)
{
    JmpToLabel(symIdx, 0x0F, 0x87);
}

void ElfAssembler::Jae(int64 symIdx)
{
    JmpToLabel(symIdx, 0x0F, 0x83);
}

void ElfAssembler::Jne(int64 symIdx)
{
    JmpToLabel(symIdx, 0x0F, 0x85);
}

void ElfAssembler::Jb(int64 symIdx)
{
    JmpToLabel(symIdx, 0x0F, 0x82);
}

void ElfAssembler::Jbe(int64 symIdx)
{
    JmpToLabel(symIdx, 0x0F, 0x86);
}

void ElfAssembler::Jg(int64 symIdx)
{
    JmpToLabel(symIdx, 0x0F, 0x8F);
}

void ElfAssembler::Jge(int64 symIdx)
{
    JmpToLabel(symIdx, 0x0F, 0x8D);
}

void ElfAssembler::Jl(int64 symIdx)
{
    JmpToLabel(symIdx, 0x0F, 0x8C);
}

void ElfAssembler::Jle(int64 symIdx)
{
    JmpToLabel(symIdx, 0x0F, 0x8E);
}

/* cmp */
void ElfAssembler::Cmp(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpRR(srcReg, destReg, 0x38);
}

void ElfAssembler::Cmp(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x3A);
}

void ElfAssembler::Cmp(InsnSize insnSize, Reg reg, const Mem &mem)
{
    OpRM(reg, mem, 0x38);
}

void ElfAssembler::Cmp(InsnSize insnSize, const ImmOpnd &immOpnd, Reg reg)
{
    OpImmAndReg(immOpnd, reg, 0x38, kCmpModReg);
}

void ElfAssembler::Cmp(InsnSize insnSize, const ImmOpnd &immOpnd, const Mem &mem)
{
    OpImmAndMem(immOpnd, mem, kCmpModReg);
}

/* test */
void ElfAssembler::Test(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpRR(srcReg, destReg, 0x84);
}

/* setcc */
void ElfAssembler::Setbe(Reg reg)
{
    OpReg(reg, 0x0F, 0x96, 0);
}

void ElfAssembler::Setbe(const Mem &mem)
{
    OpMem(mem, 0x0F, 0x96, 0);
}

void ElfAssembler::Setle(Reg reg)
{
    OpReg(reg, 0x0F, 0x9E, 0);
}

void ElfAssembler::Setle(const Mem &mem)
{
    OpMem(mem, 0x0F, 0x9E, 0);
}

void ElfAssembler::Setae(Reg reg)
{
    OpReg(reg, 0x0F, 0x93, 0);
}

void ElfAssembler::Setae(const Mem &mem)
{
    OpMem(mem, 0x0F, 0x93, 0);
}

void ElfAssembler::Setge(Reg reg)
{
    OpReg(reg, 0x0F, 0x9D, 0);
}
void ElfAssembler::Setge(const Mem &mem)
{
    OpMem(mem, 0x0F, 0x9D, 0);
}

void ElfAssembler::Setne(Reg reg)
{
    OpReg(reg, 0x0F, 0x95, 0);
}

void ElfAssembler::Setne(const Mem &mem)
{
    OpMem(mem, 0x0F, 0x95, 0);
}

void ElfAssembler::Setb(Reg reg)
{
    OpReg(reg, 0x0F, 0x92, 0);
}

void ElfAssembler::Setb(const Mem &mem)
{
    OpMem(mem, 0x0F, 0x92, 0);
}

void ElfAssembler::Setl(Reg reg)
{
    OpReg(reg, 0x0F, 0x9C, 0);
}

void ElfAssembler::Setl(const Mem &mem)
{
    OpMem(mem, 0x0F, 0x9C, 0);
}

void ElfAssembler::Seta(Reg reg)
{
    OpReg(reg, 0x0F, 0x97, 0);
}

void ElfAssembler::Seta(const Mem &mem)
{
    OpMem(mem, 0x0F, 0x97, 0);
}

void ElfAssembler::Setg(Reg reg)
{
    OpReg(reg, 0x0F, 0x9F, 0);
}

void ElfAssembler::Setg(const Mem &mem)
{
    OpMem(mem, 0x0F, 0x9F, 0);
}

void ElfAssembler::Sete(Reg reg)
{
    OpReg(reg, 0x0F, 0x94, 0);
}

void ElfAssembler::Sete(const Mem &mem)
{
    OpMem(mem, 0x0F, 0x94, 0);
}

void ElfAssembler::Seto(Reg reg)
{
    OpReg(reg, 0x0F, 0x90, 0);
}

void ElfAssembler::Seto(const Mem &mem)
{
    OpMem(mem, 0x0F, 0x90, 0);
}

/* cmov */
void ElfAssembler::Cmova(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpCmovcc(srcReg, destReg, 0x0F, 0x47);
}

void ElfAssembler::Cmova(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0E, 0x47);
}
void ElfAssembler::Cmovae(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpCmovcc(srcReg, destReg, 0x0F, 0x43);
}

void ElfAssembler::Cmovae(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0E, 0x43);
}

void ElfAssembler::Cmovb(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpCmovcc(srcReg, destReg, 0x0F, 0x42);
}

void ElfAssembler::Cmovb(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0E, 0x42);
}

void ElfAssembler::Cmovbe(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpCmovcc(srcReg, destReg, 0x0F, 0x46);
}

void ElfAssembler::Cmovbe(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0E, 0x46);
}

void ElfAssembler::Cmove(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpCmovcc(srcReg, destReg, 0x0F, 0x44);
}

void ElfAssembler::Cmove(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0E, 0x44);
}

void ElfAssembler::Cmovg(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpCmovcc(srcReg, destReg, 0x0F, 0x4F);
}

void ElfAssembler::Cmovg(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0E, 0x4F);
}

void ElfAssembler::Cmovge(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpCmovcc(srcReg, destReg, 0x0F, 0x4D);
}

void ElfAssembler::Cmovge(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0E, 0x4D);
}

void ElfAssembler::Cmovl(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpCmovcc(srcReg, destReg, 0x0F, 0x4C);
}

void ElfAssembler::Cmovl(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0E, 0x4C);
}

void ElfAssembler::Cmovle(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpCmovcc(srcReg, destReg, 0x0F, 0x4E);
}

void ElfAssembler::Cmovle(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0E, 0x4E);
}

void ElfAssembler::Cmovo(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpCmovcc(srcReg, destReg, 0x0F, 0x40);
}

void ElfAssembler::Cmovne(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpCmovcc(srcReg, destReg, 0x0F, 0x45);
}

void ElfAssembler::Cmovne(InsnSize insnSize, const Mem &mem, Reg reg)
{
    OpRM(reg, mem, 0x0E, 0x45);
}

/* call */
void ElfAssembler::Call(InsnSize insnSize, Reg reg)
{
    // Save to disignate memory
    OpReg(reg, 0xFF, 0, kCallModReg);
}

void ElfAssembler::Call(InsnSize insnSize, const Mem &mem)
{
    OpMem(mem, 0xFF, 0, kCallModReg);
}

void ElfAssembler::Call(InsnSize insnSize, int64 symIdx)
{
    Encodeb(0xE8);
    if (!CanEncodeLabel(symIdx)) {
        size_t offsetSize = 4;
        UpdateLabel(symIdx, LabelType::kFunc);
        AppendFixup(symIdx, kPLT, {static_cast<uint32>(codeBuff.size()), offsetSize}, fixups);
        uint8 imm = 0;
        Encodeb(imm, offsetSize);
    }
}

/* ret */
void ElfAssembler::Ret()
{
    Encodeb(0xC3);
}

/* leave */
void ElfAssembler::Leave()
{
    Encodeb(0xC9);
}

/* imul */
void ElfAssembler::Imul(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    OpRR(destReg, srcReg, 0x0F, 0xAF);
}

/* mul float */
void ElfAssembler::Mul(Reg srcReg, Reg destReg, bool isSingle)
{
    if (isSingle) {
        Encodeb(0xF3);
    } else {
        Encodeb(0xF2);
    }
    OpRR(destReg, srcReg, 0x0F, 0x59);
}

void ElfAssembler::Mul(const Mem &mem, Reg reg, bool isSingle)
{
    if (isSingle) {
        Encodeb(0xF3);
    } else {
        Encodeb(0xF2);
    }
    OpRM(reg, mem, 0x0F, 0x59);
}

/* nop */
void ElfAssembler::Nop(InsnSize insnSize, const Mem &mem)
{
    OpMem(mem, 0x0E, 0x1F, 0);
}

void ElfAssembler::Nop()
{
    Encodeb(0x90);
}

/* byte swap */
void ElfAssembler::Bswap(InsnSize insnSize, Reg reg)
{
    uint8 rex = GetRex(reg);
    if (rex != 0) {
        Encodeb(rex);
    }
    Encodeb(0x0F);
    Encodeb(0xC8 | GetRegCodeId(reg));
}

void ElfAssembler::Xchg(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    /* if the reg is ax, eax or rax */
    if ((GetRegId(srcReg) == 0 || GetRegId(destReg) == 0) && GetRegSize(srcReg) != k8Bits) {
        uint8 rex = GetRex(srcReg, destReg);
        if (rex != 0) {
            Encodeb(rex);
        } else if (GetRegSize(srcReg) == k16Bits) {
            Encodeb(0x66);
        }
        uint8 regCodeId = GetRegId(srcReg) == 0 ? GetRegCodeId(destReg) : GetRegCodeId(srcReg);
        Encodeb(0x90 | regCodeId);
    } else {
        OpRR(srcReg, destReg, 0x86);
    }
}

/* floating point */
void ElfAssembler::MovF(Reg srcReg, Reg destReg, bool isSingle)
{
    bool isXMM = GetRegSize(srcReg) == k128Bits || GetRegSize(destReg) == k128Bits;
    if (isSingle) {
        if (isXMM) {
            Encodeb(0xF3);
        }
        OpRR(destReg, srcReg, 0x0F, 0x10);
    } else {
        if (isXMM) {
            Encodeb(0xF2);
        }
        OpRR(destReg, srcReg, 0x0F, 0x10);
    }
}

/* floating point and */
void ElfAssembler::And(Reg srcReg, Reg destReg, bool isSingle)
{
    if (isSingle) {
        Encodeb(0x100);
    } else {
        Encodeb(0x66);
    }
    OpRR(destReg, srcReg, 0x0F, 0x54);
}

void ElfAssembler::And(const Mem &mem, Reg reg, bool isSingle)
{
    if (isSingle) {
        Encodeb(0x100);
    } else {
        Encodeb(0x66);
    }
    OpRM(reg, mem, 0x0F, 0x54);
}

/* floating div */
void ElfAssembler::Divsd(Reg srcReg, Reg destReg)
{
    Encodeb(0xF2);
    OpRR(destReg, srcReg, 0x0F, 0x5E);
}

void ElfAssembler::Divsd(const Mem &mem, Reg reg)
{
    Encodeb(0xF2);
    OpRM(reg, mem, 0x0F, 0x5E);
}

/* convert int2float */
void ElfAssembler::Cvtsi2ss(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Encodeb(0xF3);
    OpRR(destReg, srcReg, 0x0F, 0x2A);
}

void ElfAssembler::Cvtsi2sd(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Encodeb(0xF2);
    OpRR(destReg, srcReg, 0x0F, 0x2A);
}

/*convert float2int */
void ElfAssembler::Cvttsd2si(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Encodeb(0xF2);
    OpRR(destReg, srcReg, 0x0F, 0x2C);
}

void ElfAssembler::Cvttss2si(InsnSize insnSize, Reg srcReg, Reg destReg)
{
    Encodeb(0xF3);
    OpRR(destReg, srcReg, 0x0F, 0x2C);
}

/* convert float2float */
void ElfAssembler::Cvtss2sd(Reg srcReg, Reg destReg)
{
    Encodeb(0xF3);
    OpRR(destReg, srcReg, 0x0F, 0x5A);
}

void ElfAssembler::Cvtsd2ss(Reg srcReg, Reg destReg)
{
    Encodeb(0xF2);
    OpRR(destReg, srcReg, 0x0F, 0x5A);
}

/* unordered compare */
void ElfAssembler::Ucomisd(Reg srcReg, Reg destReg)
{
    Encodeb(0x66);
    OpRR(destReg, srcReg, 0x0F, 0x2E);
}

void ElfAssembler::Ucomiss(Reg srcReg, Reg destReg)
{
    Encodeb(0x100);
    OpRR(destReg, srcReg, 0x0F, 0x2E);
}

void ElfAssembler::Cmpsd(Reg srcReg, Reg destReg, uint8 imm)
{
    Encodeb(0xF2);
    OpRR(destReg, srcReg, 0x0F, 0xC2);
    Encodeb(imm);
}

void ElfAssembler::Cmpeqsd(Reg srcReg, Reg destReg)
{
    Cmpsd(srcReg, destReg, 0);
}

/* float sqrt*/
void ElfAssembler::Sqrtss_r(Reg srcReg, Reg destReg)
{
    Encodeb(0xF3);
    OpRR(destReg, srcReg, 0x0F, 0x51);
}

void ElfAssembler::Sqrtsd_r(Reg srcReg, Reg destReg)
{
    Encodeb(0xF2);
    OpRR(destReg, srcReg, 0x0F, 0x51);
}
/* end of X64 instructions */

/* process stackmap */
void ElfAssembler::RecordStackmap(const std::vector<uint8> &referenceMap,
                                  const std::vector<uint8> &deoptVreg2LocationInfo)
{
    const auto &emitMemoryManager = maplebe::CGOptions::GetInstance().GetEmitMemoryManager();
    if (emitMemoryManager.codeSpace == nullptr) {
        return;
    }
    emitMemoryManager.pc2CallSiteInfoSaver(emitMemoryManager.codeSpace, lastModulePC + codeBuff.size(), referenceMap);
    emitMemoryManager.pc2DeoptInfoSaver(emitMemoryManager.codeSpace, lastModulePC + codeBuff.size(),
                                        deoptVreg2LocationInfo);
}

uint32 ElfAssembler::GetCurModulePC()
{
    return static_cast<uint32>(lastModulePC + codeBuff.size());
}

void ElfAssembler::SetLastModulePC(uint32 pc)
{
    lastModulePC = pc;
}
} /* namespace assembler */