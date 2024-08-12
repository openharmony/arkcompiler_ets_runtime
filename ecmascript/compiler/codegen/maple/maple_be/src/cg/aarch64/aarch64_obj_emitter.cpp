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

#include "aarch64_obj_emitter.h"
#include "aarch64_isa.h"

namespace {
enum ShiftNumber : maple::uint8 {
    kShiftFour = 4,
    kShiftFive = 5,
    kShiftSix = 6,
    kShiftEight = 8,
    kShiftTen = 10,
    kShiftTwelve = 12,
    kShiftThirteen = 13,
    kShiftFifteen = 15,
    kShiftSixteen = 16,
    kShiftNineteen = 19,
    kShiftTwenty = 20,
    kShiftTwentyOne = 21,
    kShiftTwentyTwo = 22,
    kShiftTwentyFour = 24,
    kShiftTwentyNine = 29,
};

enum ShiftTypeValue : maple::uint32 {
    kShiftLSL = 0,
    kShiftLSR = 1,
    kShiftASR = 2,
};

/* from armv8 manual C1.2.3 */
maple::uint8 ccEncode[maplebe::kCcLast] = {
#define CONDCODE(a, encode) (encode),
#include "aarch64_cc.def"
#undef CONDCODE
};
};  // namespace

namespace maplebe {
/* fixup b .label, b(cond) .label, ldr label insn */
void AArch64ObjFuncEmitInfo::HandleLocalBranchFixup(const std::vector<uint32> &label2Offset,
                                                    const std::vector<uint32> &symbol2Offset)
{
    for (auto *fixup : localFixups) {
        uint32 useOffset = fixup->GetOffset();
        uint32 useLabelIndex = fixup->GetLabelIndex();
        uint32 defOffset = label2Offset[useLabelIndex];

        FixupKind fixupKind = fixup->GetFixupKind();
        CHECK_FATAL((defOffset != 0xFFFFFFFFULL || static_cast<AArch64FixupKind>(fixupKind) == kAArch64LoadPCRelImm19),
                    "fixup is not local");
        if (static_cast<AArch64FixupKind>(fixupKind) == kAArch64CondBranchPCRelImm19 ||
            static_cast<AArch64FixupKind>(fixupKind) == kAArch64CompareBranchPCRelImm19) {
            uint32 pcRelImm = (defOffset - useOffset) >> k2BitSize;
            uint32 mask = 0x7FFFF;
#ifdef EMIT_DEBUG
            LogInfo::MapleLogger() << "contents: " << std::hex << GetTextDataElem32(useOffset) << "\n";
#endif
            CHECK_FATAL(useOffset < textData.size(), "out of range");
            uint32 newValue = GetTextDataElem32(useOffset) | ((pcRelImm & mask) << kShiftFive);
            SwapTextData(&newValue, useOffset, sizeof(uint32));
#ifdef EMIT_DEBUG
            LogInfo::MapleLogger() << "after contents: " << std::hex << GetTextDataElem32(useOffset) << "\n";
#endif
        } else if (static_cast<AArch64FixupKind>(fixupKind) == kAArch64UnCondBranchPCRelImm26) {
            uint32 pcRelImm = (defOffset - useOffset) >> k2BitSize;
            uint32 mask = 0x3FFFFFF;
#ifdef EMIT_DEBUG
            LogInfo::MapleLogger() << "contents: " << std::hex << GetTextDataElem32(useOffset) << "\n";
#endif
            CHECK_FATAL(useOffset < textData.size(), "out of vector size!");
            uint32 newValue = GetTextDataElem32(useOffset) | (pcRelImm & mask);
            SwapTextData(&newValue, useOffset, sizeof(uint32));
#ifdef EMIT_DEBUG
            LogInfo::MapleLogger() << "after contents: " << std::hex << GetTextDataElem32(useOffset) << "\n";
#endif
        } else if (static_cast<AArch64FixupKind>(fixupKind) == kAArch64TestBranchPCRelImm14) {
            uint32 pcRelImm = (defOffset - useOffset) >> k2BitSize;
            uint32 mask = 0x3FFF;
            CHECK_FATAL(useOffset < textData.size(), "out of vector size");
            uint32 newValue = GetTextDataElem32(useOffset) | ((pcRelImm & mask) << kShiftFive);
            SwapTextData(&newValue, useOffset, sizeof(uint32));
        } else if (static_cast<AArch64FixupKind>(fixupKind) == kAArch64LoadPCRelImm19) {
            defOffset = symbol2Offset[useLabelIndex];
            uint32 pcRelImm = (defOffset - useOffset) >> k2BitSize;
            uint32 mask = 0x7FFFF;
            uint32 newValue = GetTextDataElem32(useOffset) | ((pcRelImm & mask) << kShiftFive);
            SwapTextData(&newValue, useOffset, sizeof(uint32));
        }
    }
    localFixups.clear();
}

void AArch64ObjEmitter::HandleTextSectionGlobalFixup()
{
    for (auto *content : contents) {
        if (content == nullptr) {
            continue;
        }
        for (auto *fixup : content->GetGlobalFixups()) {
            switch (static_cast<AArch64FixupKind>(fixup->GetFixupKind())) {
                case kAArch64CallPCRelImm26: {
                    HandleCallFixup(*content, *fixup);
                    break;
                }
                case kAArch64PCRelAdrImm21: {
                    HandleAdrFixup(*content, *fixup);
                    break;
                }
                default:
                    DEBUG_ASSERT(false, "unsupported FixupKind");
                    break;
            }
        }
    }
}

void AArch64ObjEmitter::HandleCallFixup(ObjFuncEmitInfo &funcEmitInfo, const Fixup &fixup)
{
    AArch64ObjFuncEmitInfo &objFuncEmitInfo = static_cast<AArch64ObjFuncEmitInfo &>(funcEmitInfo);
    uint32 useOffset = objFuncEmitInfo.GetStartOffset() + fixup.GetOffset();
    const std::string &funcName = fixup.GetLabel();
    auto str2objSymbolItr = globalLabel2Offset.find(funcName);
    if (str2objSymbolItr != globalLabel2Offset.end()) {
        uint32 defOffset = str2objSymbolItr->second.offset;
        uint32 pcRelImm = (defOffset - useOffset) >> k2BitSize;
        uint32 newValue = objFuncEmitInfo.GetTextDataElem32(fixup.GetOffset()) | (pcRelImm & 0x3FFFFFF);
        objFuncEmitInfo.SwapTextData(&newValue, fixup.GetOffset(), sizeof(uint32));
    }
}

void AArch64ObjEmitter::HandleAdrFixup(ObjFuncEmitInfo &funcEmitInfo, const Fixup &fixup)
{
    AArch64ObjFuncEmitInfo &objFuncEmitInfo = static_cast<AArch64ObjFuncEmitInfo &>(funcEmitInfo);
    uint32 useOffset = objFuncEmitInfo.GetStartOffset() + fixup.GetOffset();
    const std::string &label = fixup.GetLabel();
    auto str2objSymbolItr = globalLabel2Offset.find(label);
    if (str2objSymbolItr != globalLabel2Offset.end()) {
        uint32 defOffset = str2objSymbolItr->second.offset + fixup.GetRelOffset();
        uint32 pcRelImm = defOffset - useOffset;
        uint32 immLow = (pcRelImm & 0x3) << kShiftTwentyNine;
        uint32 immHigh = ((pcRelImm >> k2BitSize) & 0x7FFFF) << kShiftFive;
        uint32 newValue = objFuncEmitInfo.GetTextDataElem32(fixup.GetOffset()) | immLow | immHigh;
        objFuncEmitInfo.SwapTextData(&newValue, fixup.GetOffset(), sizeof(uint32));
    }
}

void AArch64ObjEmitter::AppendTextSectionData()
{
    auto &contents = GetContents();
    for (auto *content : contents) {
        if (content == nullptr) {
            continue;
        }
        MapleVector<uint8> funcTextData = content->GetTextData();
        textSection->AppendData(funcTextData);
    }
}

void AArch64ObjEmitter::AppendGlobalLabel()
{
    uint32 lastModulePc = cg->GetMIRModule()->GetLastModulePC();
    auto &contents = GetContents();
    uint32 offset = lastModulePc;
    for (size_t i = 0; i < contents.size(); i++) {
        auto *content = contents[i];
        if (content == nullptr) {
            continue;
        }
        content->SetStartOffset(offset);
        ObjLabel objLabel = {offset, content->GetTextDataSize()};
        std::string funcName(content->GetFuncName().c_str());
        const auto &emitMemoryManager = CGOptions::GetInstance().GetEmitMemoryManager();
        if (emitMemoryManager.funcAddressSaver != nullptr) {
            emitMemoryManager.funcAddressSaver(emitMemoryManager.codeSpace, funcName, offset);
        }
        if (emitMemoryManager.codeSpace != nullptr) {
            auto &offset2StackMapInfo = content->GetOffset2StackMapInfo();
            for (const auto &elem : offset2StackMapInfo) {
                const auto &stackMapInfo = elem.second;
                emitMemoryManager.pc2CallSiteInfoSaver(
                    emitMemoryManager.codeSpace, content->GetStartOffset() + elem.first, stackMapInfo.referenceMap);
                emitMemoryManager.pc2DeoptInfoSaver(emitMemoryManager.codeSpace, content->GetStartOffset() + elem.first,
                                                    stackMapInfo.deoptInfo);
            }
            offset2StackMapInfo.clear();
        }

        offset += content->GetTextDataSize();
        cg->GetMIRModule()->SetCurModulePC(offset);
        RegisterGlobalLabel(funcName, objLabel);
        /* register all the start of switch table */
        const MapleMap<MapleString, uint32> &switchTableOffset = content->GetSwitchTableOffset();
        for (auto &elem : switchTableOffset) {
            ObjLabel switchTableLabel = {elem.second + content->GetStartOffset(), 0};
            RegisterGlobalLabel(elem.first.c_str(), switchTableLabel);
        }
    }
}

void AArch64ObjEmitter::AppendSymsToSymTabSec()
{
    Address offset = 0;
    auto &contents = GetContents();
    for (auto *content : contents) {
        if (content == nullptr) {
            continue;
        }
        // func symbol
        AddFuncSymbol(content->GetFuncName(), content->GetTextData().size(), offset);
        offset += content->GetTextData().size();
    }
}

void AArch64ObjEmitter::InitSections()
{
    (void)memPool->New<DataSection>(" ", SHT_NULL, 0, 0, *this, *memPool);
    textSection =
        memPool->New<DataSection>(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, k4ByteSize, *this, *memPool);
    dataSection = memPool->New<DataSection>(".data", SHT_PROGBITS, SHF_WRITE | SHF_ALLOC, k8ByteSize, *this, *memPool);
    strTabSection = memPool->New<StringSection>(".strtab", SHT_STRTAB, 0, 1, *this, *memPool);
    symbolTabSection =
        memPool->New<SymbolSection>(".symtab", SHT_SYMTAB, 0, sizeof(Symbol), *this, *memPool, *strTabSection);
    shStrSection = memPool->New<StringSection>(".shstrtab", SHT_STRTAB, 0, 1, *this, *memPool);
}

void AArch64ObjEmitter::LayoutSections()
{
    /* Init elf file header */
    InitELFHeader();
    globalOffset = sizeof(FileHeader);
    globalOffset = Alignment::Align<Offset>(globalOffset, k8ByteSize);

    globalAddr = globalOffset;

    for (auto *section : sections) {
        section->SetSectionHeaderNameIndex(static_cast<Word>(shStrSection->AddString(section->GetName())));
    }

    for (auto *section : sections) {
        globalOffset = Alignment::Align<Offset>(globalOffset, section->GetAlign());
        globalAddr = Alignment::Align<Address>(globalAddr, section->GetAlign());
        section->Layout();
    }

    globalOffset = Alignment::Align<Offset>(globalOffset, k8ByteSize);
    header.e_shoff = globalOffset;
    header.e_phnum = 0;
    header.e_shnum = sections.size();
}

void AArch64ObjEmitter::UpdateMachineAndFlags(FileHeader &header)
{
    header.e_machine = EM_AARCH64;
    header.e_flags = 0;
}

/* input insn, ang get the binary code of insn */
uint32 AArch64ObjEmitter::GetBinaryCodeForInsn(const Insn &insn, const std::vector<uint32> &label2Offset,
                                               ObjFuncEmitInfo &objFuncEmitInfo) const
{
    const InsnDesc &md = AArch64CG::kMd[insn.GetMachineOpcode()];
    uint32 binInsn = md.GetMopEncode();
    switch (md.GetEncodeType()) {
        case kMovReg:
            return GenMovReg(insn);

        case kMovImm:
            return GenMovImm(insn);

        case kAddSubExtendReg:
            return binInsn | GenAddSubExtendRegInsn(insn);

        case kAddSubImm:
            return binInsn | GenAddSubImmInsn(insn);

        case kAddSubShiftImm:
            return binInsn | GenAddSubShiftImmInsn(insn);

        case kAddSubReg:
            return binInsn | GenAddSubRegInsn(insn);

        case kAddSubShiftReg:
            return binInsn | GenAddSubShiftRegInsn(insn);

        case kBitfield: {
            if (insn.GetMachineOpcode() == MOP_xuxtw64) {
                uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
                opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftSixteen;
                opnd |= (0b11111 << kShiftFive) | AArch64CG::kMd[MOP_wiorrrr].GetMopEncode();
                return opnd;
            }
            return binInsn | GenBitfieldInsn(insn);
        }

        case kExtract:
            return binInsn | GenExtractInsn(insn);

        case kBranchImm:
            return binInsn | GenBranchImmInsn(insn, label2Offset, objFuncEmitInfo);

        case kBranchReg:
            return binInsn | GenBranchRegInsn(insn);

        case kCompareBranch:
            return binInsn | GenCompareBranchInsn(insn, objFuncEmitInfo);

        case kCondCompareImm:
            return binInsn | GenCondCompareImmInsn(insn);

        case kCondCompareReg:
            return binInsn | GenCondCompareRegInsn(insn);

        case kConditionalSelect:
            return binInsn | GenConditionalSelectInsn(insn);

        case kDataProcess1Src:
            return binInsn | GenDataProcess1SrcInsn(insn);

        case kDataProcess2Src:
            return binInsn | GenDataProcess2SrcInsn(insn);

        case kDataProcess3Src:
            return binInsn | GenDataProcess3SrcInsn(insn);

        case kFloatIntConversions:
            return binInsn | GenFloatIntConversionsInsn(insn);

        case kFloatCompare:
            return binInsn | GenFloatCompareInsn(insn);

        case kFloatDataProcessing1:
            return binInsn | GenFloatDataProcessing1Insn(insn);

        case kFloatDataProcessing2:
            return binInsn | GenFloatDataProcessing2Insn(insn);

        case kFloatImm:
            return binInsn | GenFloatImmInsn(insn);

        case kFloatCondSelect:
            return binInsn | GenFloatCondSelectInsn(insn);

        case kLoadStoreReg:
            return GenLoadStoreRegInsn(insn, objFuncEmitInfo);

        case kLoadStoreAR:
            return binInsn | GenLoadStoreARInsn(insn);

        case kLoadExclusive:
            return binInsn | GenLoadExclusiveInsn(insn);

        case kLoadExclusivePair:
            return binInsn | GenLoadExclusivePairInsn(insn);

        case kStoreExclusive:
            return binInsn | GenStoreExclusiveInsn(insn);

        case kStoreExclusivePair:
            return binInsn | GenStoreExclusivePairInsn(insn);

        case kLoadPair:
            return binInsn | GenLoadPairInsn(insn);

        case kStorePair:
            return binInsn | GenStorePairInsn(insn);

        case kLoadStoreFloat:
            return GenLoadStoreFloatInsn(insn, objFuncEmitInfo);

        case kLoadPairFloat:
            return binInsn | GenLoadPairFloatInsn(insn);

        case kStorePairFloat:
            return binInsn | GenStorePairFloatInsn(insn);

        case kLoadLiteralReg:
            return binInsn | GenLoadLiteralRegInsn(insn, objFuncEmitInfo);

        case kLogicalReg:
            return binInsn | GenLogicalRegInsn(insn);

        case kLogicalImm:
            return binInsn | GenLogicalImmInsn(insn);

        case kMoveWide:
            return binInsn | GenMoveWideInsn(insn);

        case kPCRelAddr:
            return binInsn | GenPCRelAddrInsn(insn, objFuncEmitInfo);

        case kAddPCRelAddr:
            return binInsn | GenAddPCRelAddrInsn(insn, objFuncEmitInfo);

        case kSystemInsn:
            return binInsn | GenSystemInsn(insn);

        case kTestBranch:
            return binInsn | GenTestBranchInsn(insn, objFuncEmitInfo);

        case kCondBranch:
            return binInsn | GenCondBranchInsn(insn, objFuncEmitInfo);

        case kUnknownEncodeType:
            break;
        case kBrkInsn:
            return binInsn | ((GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd)) & 0xFFFF) << kShiftFive);
        default:
            break;
    }
    return binInsn;
}

/* get binary code of operand */
uint32 AArch64ObjEmitter::GetOpndMachineValue(const Operand &opnd) const
{
    if (opnd.IsRegister()) {
        const RegOperand &regOpnd = static_cast<const RegOperand &>(opnd);
        uint32 regNO = regOpnd.GetRegisterNumber();
        if (regNO == kRFLAG) {
            return 0;
        }
        if (regOpnd.IsOfIntClass()) {
            if (regOpnd.GetRegisterNumber() == RZR) {
                return regNO - R0 - kRegNum2;
            }
            if (regOpnd.GetRegisterNumber() == RSP) {
                return regNO - R0 - 1;
            }
            DEBUG_ASSERT(regNO >= R0, "value overflow");
            return regNO - R0;
        }
        return regNO - V0;
    } else if (opnd.IsImmediate()) {
        return static_cast<uint32>(static_cast<const ImmOperand &>(opnd).GetValue());
    } else if (opnd.IsConditionCode()) {
        const CondOperand &condOpnd = static_cast<const CondOperand &>(opnd);
        return static_cast<uint32>(ccEncode[condOpnd.GetCode()]);
    } else if (opnd.IsOpdExtend()) {
        const ExtendShiftOperand &exendOpnd = static_cast<const ExtendShiftOperand &>(opnd);
        uint32 shift = exendOpnd.GetShiftAmount();
        DEBUG_ASSERT(exendOpnd.GetExtendOp() == ExtendShiftOperand::kSXTW, "support kSXTW only!");
        uint32 option = 0x30;
        return option | shift;
    } else {
        CHECK_FATAL(false, "not supported operand type currently");
    }
}

uint32 AArch64ObjEmitter::GetAdrLabelOpndValue(const Insn &insn, const Operand &opnd,
                                               ObjFuncEmitInfo &objFuncEmitInfo) const
{
    FixupKind fixupKind =
        (insn.GetMachineOpcode() == MOP_xadrp) ? FixupKind(kAArch64PCRelAdrpImm21) : FixupKind(kAArch64PCRelAdrImm21);
    if (opnd.IsMemoryAccessOperand()) {
        const MemOperand &memOpnd = static_cast<const MemOperand &>(opnd);
        Fixup *fixup = memPool->New<Fixup>(memOpnd.GetSymbolName(), 0, objFuncEmitInfo.GetTextDataSize(), fixupKind);
        objFuncEmitInfo.AppendGlobalFixups(*fixup);
    } else if (opnd.IsStImmediate()) {
        const StImmOperand &stOpnd = static_cast<const StImmOperand &>(opnd);
        Fixup *fixup =
            memPool->New<Fixup>(stOpnd.GetName(), stOpnd.GetOffset(), objFuncEmitInfo.GetTextDataSize(), fixupKind);
        objFuncEmitInfo.AppendGlobalFixups(*fixup);
    } else {
        CHECK_FATAL(opnd.IsImmediate(), "check kind failed");
    }
    return 0;
}

uint32 AArch64ObjEmitter::GetLoadLiteralOpndValue(const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    FixupKind fixupKind = FixupKind(kAArch64LoadPCRelImm19);
    CHECK_FATAL(opnd.IsLabelOpnd(), "check literal kind failed");
    const LabelOperand &label = static_cast<const LabelOperand &>(opnd);
    LocalFixup *fixup = memPool->New<LocalFixup>(label.GetLabelIndex(), objFuncEmitInfo.GetTextDataSize(), fixupKind);
    objFuncEmitInfo.AppendLocalFixups(*fixup);
    return 0;
}

uint32 AArch64ObjEmitter::GetCondBranchOpndValue(const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    FixupKind fixupKind = FixupKind(kAArch64CondBranchPCRelImm19);
    uint32 labelIndex = static_cast<const LabelOperand &>(opnd).GetLabelIndex();
    LocalFixup *fixup = memPool->New<LocalFixup>(labelIndex, objFuncEmitInfo.GetTextDataSize(), fixupKind);
    objFuncEmitInfo.AppendLocalFixups(*fixup);
    return 0;
}

uint32 AArch64ObjEmitter::GetUnCondBranchOpndValue(const Operand &opnd, const std::vector<uint32> &label2Offset,
                                                   ObjFuncEmitInfo &objFuncEmitInfo) const
{
    auto labelIndex = static_cast<const LabelOperand &>(opnd).GetLabelIndex();
    CHECK_FATAL(labelIndex < label2Offset.size(), "labelIndex is out of range");
    uint32 defOffset = label2Offset[labelIndex];
    if (defOffset != 0xFFFFFFFFULL) {
        uint32 useOffset = objFuncEmitInfo.GetTextDataSize();
        uint32 pcRelImm = (defOffset - useOffset) >> k2BitSize;
        return (pcRelImm & 0x3FFFFFF);
    }

    FixupKind fixupKind = FixupKind(kAArch64UnCondBranchPCRelImm26);
    LocalFixup *fixup = memPool->New<LocalFixup>(labelIndex, objFuncEmitInfo.GetTextDataSize(), fixupKind);
    objFuncEmitInfo.AppendLocalFixups(*fixup);
    return 0;
}

uint32 AArch64ObjEmitter::GetCallFuncOpndValue(const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    const FuncNameOperand &funcNameOpnd = static_cast<const FuncNameOperand &>(opnd);
    const MIRSymbol *funcSymbol = funcNameOpnd.GetFunctionSymbol();
    FixupKind fixupKind = FixupKind(kAArch64CallPCRelImm26);

    Fixup *fixup = memPool->New<Fixup>(funcNameOpnd.GetName(), 0, objFuncEmitInfo.GetTextDataSize(), fixupKind);
    if (funcSymbol->IsGlobal()) {
        objFuncEmitInfo.AppendGlobalFixups(*fixup);
    }
    return 0;
}

uint32 AArch64ObjEmitter::GetCompareBranchOpndValue(const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    FixupKind fixupKind = FixupKind(kAArch64CompareBranchPCRelImm19);
    uint32 labelIndex = static_cast<const LabelOperand &>(opnd).GetLabelIndex();
    LocalFixup *fixup = memPool->New<LocalFixup>(labelIndex, objFuncEmitInfo.GetTextDataSize(), fixupKind);
    objFuncEmitInfo.AppendLocalFixups(*fixup);
    return 0;
}

uint32 AArch64ObjEmitter::GetTestBranchOpndValue(const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    FixupKind fixupKind = FixupKind(kAArch64TestBranchPCRelImm14);
    uint32 labelIndex = static_cast<const LabelOperand &>(opnd).GetLabelIndex();
    LocalFixup *fixup = memPool->New<LocalFixup>(labelIndex, objFuncEmitInfo.GetTextDataSize(), fixupKind);
    objFuncEmitInfo.AppendLocalFixups(*fixup);
    return 0;
}

uint32 AArch64ObjEmitter::GetLo12LitrealOpndValue(MOperator mOp, const Operand &opnd,
                                                  ObjFuncEmitInfo &objFuncEmitInfo) const
{
    FixupKind fixupKind = (mOp == MOP_xadrpl12) ? FixupKind(kAArch64AddPCRelLo12) : FixupKind(kAArch64LdrPCRelLo12);
    if (opnd.IsMemoryAccessOperand()) {
        const MemOperand &memOpnd = static_cast<const MemOperand &>(opnd);
        uint32 offset = 0;
        if (memOpnd.GetOffsetImmediate() != nullptr) {
            offset = static_cast<uint32>(memOpnd.GetOffsetImmediate()->GetOffsetValue());
        }
        Fixup *fixup =
            memPool->New<Fixup>(memOpnd.GetSymbolName(), offset, objFuncEmitInfo.GetTextDataSize(), fixupKind);
        objFuncEmitInfo.AppendGlobalFixups(*fixup);
    } else {
        CHECK_FATAL(opnd.IsStImmediate(), "check opnd kind");
        const StImmOperand &stOpnd = static_cast<const StImmOperand &>(opnd);
        Fixup *fixup =
            memPool->New<Fixup>(stOpnd.GetName(), stOpnd.GetOffset(), objFuncEmitInfo.GetTextDataSize(), fixupKind);
        objFuncEmitInfo.AppendGlobalFixups(*fixup);
    }
    return 0;
}

uint32 AArch64ObjEmitter::GenMovReg(const Insn &insn) const
{
    Operand &opnd1 = insn.GetOperand(kInsnFirstOpnd);
    Operand &opnd2 = insn.GetOperand(kInsnSecondOpnd);
    DEBUG_ASSERT(opnd1.IsRegister(), "opnd1 must be a register");
    DEBUG_ASSERT(opnd2.IsRegister(), "opnd2 must be a register");
    uint32 opCode = 0;
    if (static_cast<RegOperand &>(opnd1).GetRegisterNumber() == RSP ||
        static_cast<RegOperand &>(opnd2).GetRegisterNumber() == RSP) {
        if (insn.GetMachineOpcode() == MOP_xmovrr) {
            const InsnDesc &md = AArch64CG::kMd[MOP_xaddrri12];
            opCode = md.GetMopEncode();
        } else {
            DEBUG_ASSERT(insn.GetMachineOpcode() == MOP_wmovrr, "support MOP_wmovrr Currently!");
            const InsnDesc &md = AArch64CG::kMd[MOP_waddrri12];
            opCode = md.GetMopEncode();
        }
        /* Rd */
        uint32 opnd = opCode | GetOpndMachineValue(opnd1);
        /* Rn */
        opnd |= GetOpndMachineValue(opnd2) << kShiftFive;
        return opnd;
    } else {
        if (insn.GetMachineOpcode() == MOP_xmovrr) {
            const InsnDesc &md = AArch64CG::kMd[MOP_xiorrrr];
            opCode = md.GetMopEncode();
        } else {
            DEBUG_ASSERT(insn.GetMachineOpcode() == MOP_wmovrr, "support MOP_wmovrr Currently!");
            const InsnDesc &md = AArch64CG::kMd[MOP_wiorrrr];
            opCode = md.GetMopEncode();
        }
        /* Rd */
        uint32 opnd = opCode | GetOpndMachineValue(opnd1);
        /* Rn */
        opnd |= GetOpndMachineValue(opnd2) << kShiftSixteen;
        /* Rm */
        uint32 zr = 0x1f; /* xzr / wzr */
        opnd |= zr << kShiftFive;
        return opnd;
    }
}

uint32 AArch64ObjEmitter::GenMovImm(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    ImmOperand &immOpnd = static_cast<ImmOperand &>(insn.GetOperand(kInsnSecondOpnd));
    uint32 immSize = insn.GetDesc()->GetOpndDes(kInsnSecondOpnd)->GetSize();
    uint64 immValue = static_cast<uint64>(immOpnd.GetValue());
    bool isMovz = IsMoveWidableImmediate(immValue, immSize);
    bool isMovn = IsMoveWidableImmediate(~immValue, immSize);
    if (isMovz || isMovn) {
        if (!isMovz) {
            immValue = ~immValue;
        }
        uint32 hwFlag = 0;
        if (immSize == k32BitSize) {
            auto &md = isMovz ? AArch64CG::kMd[MOP_wmovzri16] : AArch64CG::kMd[MOP_wmovnri16];
            opnd |= md.GetMopEncode();
            immValue = static_cast<uint32>(immValue);
            uint32 bitFieldValue = 0xFFFF;
            if (((static_cast<uint32>(immValue)) & (bitFieldValue << k16BitSize)) != 0) {
                hwFlag = 1;
            }
        } else {
            DEBUG_ASSERT(immSize == k64BitSize, "support 64 bit only!");
            auto &md = isMovz ? AArch64CG::kMd[MOP_xmovzri16] : AArch64CG::kMd[MOP_xmovnri16];
            opnd |= md.GetMopEncode();
            uint64 bitFieldValue = 0xFFFF;
            for (hwFlag = 0; hwFlag <= 3; ++hwFlag) {  // hwFlag is just from 0(00b) to 3(11b)
                if (immValue & (bitFieldValue << (k16BitSize * hwFlag))) {
                    break;
                }
            }
        }
        opnd |= ((static_cast<uint32>(immValue >> (hwFlag * k16BitSize))) << kShiftFive);
        opnd |= (hwFlag << kShiftTwentyOne);
    } else {
        if (immSize == k32BitSize) {
            auto &md = AArch64CG::kMd[MOP_wiorrri12];
            opnd |= md.GetMopEncode();
        } else {
            DEBUG_ASSERT(immSize == k64BitSize, "support 64 bit only!");
            auto &md = AArch64CG::kMd[MOP_xiorrri13];
            opnd |= md.GetMopEncode();
        }
        uint64 value = static_cast<uint64>(immOpnd.GetValue());
        uint32 size = (immSize == k32BitSize) ? k32BitSize : k64BitSize;
        opnd |= EncodeLogicaImm(value, size) << kShiftTen;
        opnd |= (0x1FU << kShiftFive);
    }

    return opnd;
}

uint32 AArch64ObjEmitter::GenAddSubExtendRegInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    /* Rm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;
    /* Extend */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnFourthOpnd)) << kShiftTen;
    return opnd;
}

uint32 AArch64ObjEmitter::GenAddPCRelAddrInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    /* Imm */
    opnd |= GetLo12LitrealOpndValue(insn.GetMachineOpcode(), insn.GetOperand(kInsnThirdOpnd), objFuncEmitInfo)
            << kShiftTen;
    return opnd;
}

uint32 AArch64ObjEmitter::GenAddSubImmInsn(const Insn &insn) const
{
    uint32 operandSize = 4;  // subs insn
    int32 index = insn.GetOperandSize() == operandSize ? 1 : 0;
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd + index));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd + index)) << kShiftFive;
    /* Imm */
    uint32 immValue = GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd + index));
    if ((immValue & (0xFFFU)) == 0 && ((immValue & (0xFFFU << kShiftTwelve))) != 0) {
        opnd |= (1U << kShiftTwentyTwo);
        immValue >>= kShiftTwelve;
    }
    opnd |= (immValue << kShiftTen);
    return opnd;
}

uint32 AArch64ObjEmitter::GenAddSubShiftImmInsn(const Insn &insn) const
{
    uint32 operandSize = 5;  // subs insn
    int32 index = insn.GetOperandSize() == operandSize ? 1 : 0;
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd + index));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd + index)) << kShiftFive;
    /* Imm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd + index)) << kShiftTen;
    /* Shift */
    BitShiftOperand &lslOpnd = static_cast<BitShiftOperand &>(insn.GetOperand(kInsnFourthOpnd + index));
    if (lslOpnd.GetShiftAmount() > 0) {
        uint32 shift = 0x1;
        opnd |= shift << kShiftTwentyTwo;
    }
    return opnd;
}

uint32 AArch64ObjEmitter::GenAddSubRegInsn(const Insn &insn) const
{
    int32 index = insn.GetOperandSize() == k4ByteSize ? 1 : 0;  // subs insn
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd + index));
    if (insn.GetOperandSize() == k2ByteSize) {  // neg, cmp or cmn insn
        /* Rm */
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftSixteen;
        return opnd;
    }
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd + index)) << kShiftFive;
    /* Rm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd + index)) << kShiftSixteen;

    RegOperand &rd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd + index));
    RegOperand &rn = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd + index));
    // SP register can only be used with LSL or Extend
    if (rd.GetRegisterNumber() == RSP || rn.GetRegisterNumber() == RSP) {
        uint32 regSize = insn.GetDesc()->GetOpndDes(kInsnSecondOpnd)->GetSize();
        opnd |= 1 << kShiftTwentyOne;
        opnd |= ((regSize == k64BitSize ? 0b11 : 0b10) << kShiftThirteen);  // option
    }
    return opnd;
}

uint32 AArch64ObjEmitter::GenAddSubShiftRegInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));

    BitShiftOperand *bitShiftOpnd = nullptr;

    uint32 operandSize = 3;
    if (insn.GetOperandSize() == operandSize) {
        /* Rm */
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftSixteen;
        bitShiftOpnd = static_cast<BitShiftOperand *>(&insn.GetOperand(kInsnThirdOpnd));
    } else {
        /* Rn */
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
        /* Rm */
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;
        bitShiftOpnd = static_cast<BitShiftOperand *>(&insn.GetOperand(kInsnFourthOpnd));
    }
    uint32 shift = 0;
    switch (bitShiftOpnd->GetShiftOp()) {
        case BitShiftOperand::kLSL:
            shift = kShiftLSL;
            break;
        case BitShiftOperand::kLSR:
            shift = kShiftLSR;
            break;
        case BitShiftOperand::kASR:
            shift = kShiftASR;
            break;
        default:
            break;
    }
    /* Shift */
    opnd |= shift << kShiftTwentyTwo;
    /* Imm */
    opnd |= bitShiftOpnd->GetShiftAmount() << kShiftTen;
    return opnd;
}

uint32 AArch64ObjEmitter::GenBitfieldInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;

    uint32 operandSize = 4;
    if (insn.GetMachineOpcode() == MOP_wubfizrri5i5 || insn.GetMachineOpcode() == MOP_xubfizrri6i6 ||
        insn.GetMachineOpcode() == MOP_wbfirri5i5 || insn.GetMachineOpcode() == MOP_xbfirri6i6) {
        uint32 mod = insn.GetDesc()->GetOpndDes(kInsnFirstOpnd)->GetSize(); /* 64 & 32 from ARMv8 manual C5.6.114 */
        CHECK_FATAL(mod == 64 || mod == 32, "mod must be 64/32");
        uint32 shift = GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd));
        uint32 immr = -shift % mod;
        opnd |= immr << kShiftSixteen;
        uint32 width = GetOpndMachineValue(insn.GetOperand(kInsnFourthOpnd));
        CHECK_FATAL(width >= 1, "value overflow");
        uint32 imms = width - 1;
        opnd |= imms << kShiftTen;
    } else if (insn.GetOperandSize() == operandSize) {
        uint32 lab = GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd));
        opnd |= lab << kShiftSixteen;
        uint32 width = GetOpndMachineValue(insn.GetOperand(kInsnFourthOpnd));
        CHECK_FATAL(lab < UINT64_MAX - width, "value overflow");
        CHECK_FATAL(lab + width >= 1, "value overflow");
        opnd |= (lab + width - 1) << kShiftTen;
    } else if (insn.GetMachineOpcode() == MOP_xlslrri6 || insn.GetMachineOpcode() == MOP_wlslrri5) {
        uint32 mod = insn.GetDesc()->GetOpndDes(kInsnFirstOpnd)->GetSize(); /* 64 & 32 from ARMv8 manual C5.6.114 */
        CHECK_FATAL(mod == 64 || mod == 32, "mod must be 64/32");
        uint32 shift = GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd));
        uint32 immr = -shift % mod;
        opnd |= immr << kShiftSixteen;
        uint32 imms = mod - 1 - shift;
        opnd |= imms << kShiftTen;
    } else if (insn.GetMachineOpcode() == MOP_xlsrrri6 || insn.GetMachineOpcode() == MOP_wlsrrri5 ||
               insn.GetMachineOpcode() == MOP_xasrrri6 || insn.GetMachineOpcode() == MOP_wasrrri5) {
        uint32 mod = insn.GetDesc()->GetOpndDes(kInsnFirstOpnd)->GetSize(); /* 64 & 32 from ARMv8 manual C5.6.114 */
        CHECK_FATAL(mod == 64 || mod == 32, "mod must be 64/32");
        uint32 immr = GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd));
        opnd |= immr << kShiftSixteen;
        uint32 imms = mod - 1;
        opnd |= imms << kShiftTen;
    }
    return opnd;
}

uint32 AArch64ObjEmitter::GenExtractInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    /* Imm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftTen;
    /* Rm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;

    return opnd;
}

uint32 AArch64ObjEmitter::GenBranchImmInsn(const Insn &insn, const std::vector<uint32> &label2Offset,
                                           ObjFuncEmitInfo &objFuncEmitInfo) const
{
    /* Imm */
    if (insn.IsCall()) {
        return GetCallFuncOpndValue(insn.GetOperand(kInsnFirstOpnd), objFuncEmitInfo);
    } else {
        return GetUnCondBranchOpndValue(insn.GetOperand(kInsnFirstOpnd), label2Offset, objFuncEmitInfo);
    }
}

uint32 AArch64ObjEmitter::GenBranchRegInsn(const Insn &insn) const
{
    if (insn.GetMachineOpcode() == MOP_xret || insn.GetMachineOpcode() == MOP_clrex) {
        return 0;
    }
    /* Rn */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd)) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenCompareBranchInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    /* Rt */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Imm */
    opnd |= GetCompareBranchOpndValue(insn.GetOperand(kInsnSecondOpnd), objFuncEmitInfo) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenCondCompareImmInsn(const Insn &insn) const
{
    /* Rn */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    /* Imm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;
    /* Nzcv */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnFourthOpnd));
    /* Cond */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnFifthOpnd)) << kShiftTwelve;
    return opnd;
}

uint32 AArch64ObjEmitter::GenCondCompareRegInsn(const Insn &insn) const
{
    /* Rn */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    /* Rm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;
    /* Nzcv */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnFourthOpnd));
    /* Cond */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnFifthOpnd)) << kShiftTwelve;
    return opnd;
}

uint32 AArch64ObjEmitter::GenConditionalSelectInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    uint32 operandSize = 5;
    if (insn.GetOperandSize() == operandSize) {
        /* Rn */
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
        /* Rm */
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;
        /* Cond */
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnFourthOpnd)) << kShiftTwelve;
    } else if (insn.GetMachineOpcode() == MOP_wcnegrrrc || insn.GetMachineOpcode() == MOP_xcnegrrrc) {
        /* Rn */
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
        /* Rm Rn==Rm */
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftSixteen;
        /* Cond */
        uint8 cond = GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd));
        /* invert cond */
        opnd |= ((cond ^ 1u) & 0xfu) << kShiftTwelve;
    } else {
        /* Cond */
        uint8 cond = GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd));
        /* invert cond */
        opnd |= ((cond ^ 1u) & 0xfu) << kShiftTwelve;
    }
    return opnd;
}

uint32 AArch64ObjEmitter::GenDataProcess1SrcInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenDataProcess2SrcInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    /* Rm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;
    return opnd;
}

uint32 AArch64ObjEmitter::GenDataProcess3SrcInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    /* Rm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;
    /* Ra */
    uint32 operandSize = 4;
    if (insn.GetOperandSize() == operandSize) {
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnFourthOpnd)) << kShiftTen;
    }
    return opnd;
}

uint32 AArch64ObjEmitter::GenFloatIntConversionsInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenFloatCompareInsn(const Insn &insn) const
{
    /* Rn */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    if (insn.GetOperand(kInsnThirdOpnd).IsRegister()) {
        /* Rm */
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;
    }
    return opnd;
}

uint32 AArch64ObjEmitter::GenFloatDataProcessing1Insn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenFloatDataProcessing2Insn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    /* Rm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;
    return opnd;
}

uint32 AArch64ObjEmitter::GenFloatImmInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Imm */
    opnd |= (GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) & 0xff) << kShiftThirteen;
    return opnd;
}

uint32 AArch64ObjEmitter::GenFloatCondSelectInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    /* Rm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;
    /* Cond */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnFourthOpnd)) << kShiftTwelve;
    return opnd;
}

uint32 AArch64ObjEmitter::GenLoadStoreModeLiteral(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    /* Rt */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Mem */
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnSecondOpnd));
    FixupKind fixupKind = FixupKind(kAArch64LoadPCRelImm19);
    LocalFixup *fixup =
        memPool->New<LocalFixup>(objFuncEmitInfo.GetCGFunc().GetLocalSymLabelIndex(*memOpnd.GetSymbol()),
                                 objFuncEmitInfo.GetTextDataSize(), fixupKind);
    objFuncEmitInfo.AppendLocalFixups(*fixup);
    MOperator mOp = insn.GetMachineOpcode();
    if (mOp == MOP_sldr) {
        mOp = MOP_sldli;
    } else if (mOp == MOP_dldr) {
        mOp = MOP_dldli;
    } else if (mOp == MOP_xldr) {
        mOp = MOP_xldli;
    } else if (mOp == MOP_wldr) {
        mOp = MOP_wldli;
    } else {
        CHECK_FATAL(false, "unsupported mOp");
    }
    auto &md = AArch64CG::kMd[mOp];
    return md.GetMopEncode() | opnd;
}

uint32 AArch64ObjEmitter::GenLoadStoreModeBOi(const Insn &insn) const
{
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnSecondOpnd));
    OfstOperand *ofstOpnd = static_cast<OfstOperand *>(memOpnd.GetOffsetImmediate());
    /* Imm */
    int32 offsetValue = ofstOpnd->GetOffsetValue();
    uint32 imm9Mask = 0x1ff;
    uint32 opnd = 0U;
    if (memOpnd.IsPostIndexed()) {
        opnd |= (static_cast<uint32>(offsetValue) & imm9Mask) << kShiftTwelve;
        uint32 specialOpCode = 0x1;
        opnd |= specialOpCode << kShiftTen;
    } else if (memOpnd.IsPreIndexed()) {
        opnd |= (static_cast<uint32>(offsetValue) & imm9Mask) << kShiftTwelve;
        uint32 specialOpCode = 0x3;
        opnd |= specialOpCode << kShiftTen;
    } else {
        DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "must be kIntact!");
        uint32 specialOpCode = 0x1;
        opnd |= specialOpCode << kShiftTwentyFour;
        uint32 divisor = 1;
        MOperator mOp = insn.GetMachineOpcode();
        if ((mOp == MOP_xldr) || (mOp == MOP_xstr) || (mOp == MOP_dldr) || (mOp == MOP_dstr)) {
            divisor = k8BitSize;
        } else if ((mOp == MOP_wldr) || (mOp == MOP_wstr) || (mOp == MOP_sstr) || (mOp == MOP_sldr)) {
            divisor = k4BitSize;
        } else if (mOp == MOP_hldr) {
            divisor = k2BitSize;
        }
        uint32 shiftRightNum = 0;
        if ((mOp == MOP_wldrsh) || (mOp == MOP_wldrh) || (mOp == MOP_wstrh)) {
            shiftRightNum = 1;
        }
        opnd |= ((static_cast<uint32>(offsetValue) >> shiftRightNum) / divisor) << kShiftTen;
    }
    return opnd;
}

uint32 AArch64ObjEmitter::GenLoadStoreModeBOrX(const Insn &insn) const
{
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnSecondOpnd));
    uint32 opnd = 0;
    opnd |= 0x1 << kShiftTwentyOne;
    opnd |= 0x2 << kShiftTen;
    RegOperand *offsetReg = memOpnd.GetIndexRegister();
    opnd |= GetOpndMachineValue(*offsetReg) << kShiftSixteen;
    std::string extend = memOpnd.GetExtendAsString();
    uint32 shift = memOpnd.ShiftAmount();
    uint32 option = 0;
    if (extend == "UXTW") {
        option = 0x2;
    } else if (extend == "LSL") {
        option = 0x3;
        uint32 regSize = insn.GetDesc()->GetOpndDes(kInsnFirstOpnd)->GetSize();
        // lsl extend insn shift amount can only be 0 or 1(16-bit def opnd) or 2(32-bit def opnd) or
        // 3(64-bit def opnd) or 4(128-bit def opnd) in ldr/str insn
        CHECK_FATAL((shift == k0BitSize) || (regSize == k16BitSize && shift == k1BitSize) ||
                        (regSize == k32BitSize && shift == k2BitSize) ||
                        (regSize == k64BitSize && shift == k3BitSize) || (regSize == k128BitSize && shift == k4BitSize),
                    "unsupport LSL amount");
    } else if (extend == "SXTW") {
        option = 0x6;
    } else {
        DEBUG_ASSERT(extend == "SXTX", "must be SXTX!");
        option = 0x7;
    }
    opnd |= option << kShiftThirteen;
    uint32 s = (shift > 0) ? 1 : 0;
    opnd |= s << kShiftTwelve;
    return opnd;
}

uint32 AArch64ObjEmitter::GenLoadStoreRegInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    /* Mem */
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnSecondOpnd));
    if (memOpnd.GetAddrMode() == MemOperand::kAddrModeLiteral) {
        return GenLoadStoreModeLiteral(insn, objFuncEmitInfo);
    }

    MOperator mOp = insn.GetMachineOpcode();
#ifdef USE_32BIT_REF
    if (((mOp == MOP_xstr) || (mOp == MOP_xldr)) &&
        static_cast<AArch64RegOperand &>(insn.GetOperand(kInsnFirstOpnd)).IsRefField()) {
        mOp = (mOp == MOP_xstr) ? MOP_wstr : MOP_wldr;
    }
#endif
    auto &md = AArch64CG::kMd[mOp];
    uint32 binInsn = md.GetMopEncode();
    // invalid insn generated by the eval node
    if (static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber() == RZR) {
        if (mOp == MOP_sldr) {
            binInsn = AArch64CG::kMd[MOP_wldr].GetMopEncode();
        } else if (mOp == MOP_dldr) {
            binInsn = AArch64CG::kMd[MOP_xldr].GetMopEncode();
        } else if (mOp == MOP_sstr) {
            binInsn = AArch64CG::kMd[MOP_wstr].GetMopEncode();
        } else if (mOp == MOP_dstr) {
            binInsn = AArch64CG::kMd[MOP_xstr].GetMopEncode();
        }
    }
    /* Rt */
    binInsn |= GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rn */
    Operand *baseReg = memOpnd.GetBaseRegister();
    binInsn |= GetOpndMachineValue(*baseReg) << kShiftFive;

    if (memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi) {
        uint32 size = insn.GetDesc()->GetOpndDes(kInsnSecondOpnd)->GetSize();
        OfstOperand *ofstOpnd = static_cast<OfstOperand *>(memOpnd.GetOffsetImmediate());
        /* Imm */
        int32 offsetValue = ofstOpnd != nullptr ? ofstOpnd->GetOffsetValue() : 0;
        if ((((size == k16BitSize) && (offsetValue % k2BitSize) != 0) ||
             ((size == k32BitSize) && (offsetValue % k4BitSize) != 0) ||
             ((size == k64BitSize) && (offsetValue % k8BitSize) != 0)) &&
            ((offsetValue < k256BitSizeInt) && (offsetValue >= kNegative256BitSize))) {
            uint32 mopEncode = 0;
            // ldur, ldurh, ldurb
            if (insn.IsLoad()) {
                if (insn.GetDesc()->GetEncodeType() == kLoadStoreFloat) {
                    mopEncode = size == k16BitSize ? 0x7c400000 : (size == k32BitSize ? 0xbc400000 : 0xfc400000);
                } else {
                    mopEncode = size == k16BitSize ? 0x78400000 : (size == k32BitSize ? 0xb8400000 : 0xf8400000);
                }
            } else {  // stur, sturh, sturb
                if (insn.GetDesc()->GetEncodeType() == kLoadStoreFloat) {
                    mopEncode = size == k16BitSize ? 0x7c000000 : (size == k32BitSize ? 0xbc000000 : 0xfc000000);
                } else {
                    mopEncode = size == k16BitSize ? 0x78000000 : (size == k32BitSize ? 0xb8000000 : 0xf8000000);
                }
            }
            binInsn =
                GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd)) | (GetOpndMachineValue(*baseReg) << kShiftFive);
            return binInsn | mopEncode | (offsetValue << kShiftTwelve);
        }
        return binInsn | GenLoadStoreModeBOi(insn);
    } else if (memOpnd.GetAddrMode() == MemOperand::kAddrModeBOrX) {
        return binInsn | GenLoadStoreModeBOrX(insn);
    }
    DEBUG_ASSERT(memOpnd.GetAddrMode() == MemOperand::kAddrModeLo12Li, "support kAddrModeLo12Li only!");
    DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "must be kIntact!");
    binInsn |= GetLo12LitrealOpndValue(insn.GetMachineOpcode(), memOpnd, objFuncEmitInfo) << kShiftTen;
    uint32 specialOpCode = 0x1;
    binInsn |= specialOpCode << kShiftTwentyFour;

    return binInsn;
}

uint32 AArch64ObjEmitter::GenLoadStoreARInsn(const Insn &insn) const
{
    /* Rt */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Mem */
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnSecondOpnd));
    DEBUG_ASSERT(memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi, "support kAddrModeBOi only!");
    DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "must be kIntact!");
    Operand *baseReg = memOpnd.GetBaseRegister();
    /* Rn */
    opnd |= GetOpndMachineValue(*baseReg) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenLoadExclusiveInsn(const Insn &insn) const
{
    /* Rt */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Mem */
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnSecondOpnd));
    DEBUG_ASSERT(memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi, "support kAddrModeBOi only!");
    DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "must be kIntact!");
    Operand *baseReg = memOpnd.GetBaseRegister();
    /* Rn */
    opnd |= GetOpndMachineValue(*baseReg) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenLoadExclusivePairInsn(const Insn &insn) const
{
    /* Rt */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rt2 */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftTen;
    /* Mem */
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnThirdOpnd));
    DEBUG_ASSERT(memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi, "support kAddrModeBOi only!");
    DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "must be kIntact!");
    Operand *baseReg = memOpnd.GetBaseRegister();
    /* Rn */
    opnd |= GetOpndMachineValue(*baseReg) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenStoreExclusiveInsn(const Insn &insn) const
{
    /* Rs */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd)) << kShiftSixteen;
    /* Rt */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd));
    /* Mem */
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnThirdOpnd));
    DEBUG_ASSERT(memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi, "support kAddrModeBOi only!");
    DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "must be kIntact!");
    Operand *baseReg = memOpnd.GetBaseRegister();
    /* Rn */
    opnd |= GetOpndMachineValue(*baseReg) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenStoreExclusivePairInsn(const Insn &insn) const
{
    /* Rs */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd)) << kShiftSixteen;
    /* Rt */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd));
    /* Rt2 */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftTen;
    /* Mem */
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnFourthOpnd));
    DEBUG_ASSERT(memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi, "support kAddrModeBOi only!");
    DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "must be kIntact!");
    Operand *baseReg = memOpnd.GetBaseRegister();
    /* Rn */
    opnd |= GetOpndMachineValue(*baseReg) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenLoadPairInsn(const Insn &insn) const
{
    /* Rt */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rt2 */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftTen;
    /* Mem */
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnThirdOpnd));
    DEBUG_ASSERT(memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi, "support kAddrModeBOi only!");
    /* Rn */
    Operand *baseReg = memOpnd.GetBaseRegister();
    opnd |= GetOpndMachineValue(*baseReg) << kShiftFive;
    /* Imm */
    OfstOperand *ofstOpnd = static_cast<OfstOperand *>(memOpnd.GetOffsetImmediate());
    int32 offsetValue = ofstOpnd->GetOffsetValue();
    uint32 divisor = 0;
    if (insn.GetDesc()->GetOpndDes(kInsnThirdOpnd)->GetSize() == k64BitSize) {
        divisor = k8ByteSize;
    } else {
        divisor = k4ByteSize;
    }
    uint32 imm7Mask = 0x7f;
    opnd |= (((static_cast<uint32>(offsetValue / divisor) & imm7Mask)) << kShiftFifteen);

    uint32 specialOpCode = 0;
    if (memOpnd.IsPostIndexed()) {
        specialOpCode = 0x3;
    } else if (memOpnd.IsPreIndexed()) {
        specialOpCode = 0x7;
    } else {
        DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "must be kIntact!");
        specialOpCode = 0x5;
    }
    opnd |= specialOpCode << kShiftTwentyTwo;
    return opnd;
}

uint32 AArch64ObjEmitter::GenStorePairInsn(const Insn &insn) const
{
    /* Rt */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rt2 */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftTen;
    /* Mem */
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnThirdOpnd));
    DEBUG_ASSERT(memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi, "support kAddrModeBOi only!");
    /* Rn */
    Operand *baseReg = memOpnd.GetBaseRegister();
    opnd |= GetOpndMachineValue(*baseReg) << kShiftFive;
    /* Imm */
    OfstOperand *ofstOpnd = static_cast<OfstOperand *>(memOpnd.GetOffsetImmediate());
    int32 offsetValue = ofstOpnd->GetOffsetValue();
    uint32 divisor = 0;
    if (insn.GetDesc()->GetOpndDes(kInsnThirdOpnd)->GetSize() == k64BitSize) {
        divisor = k8ByteSize;
    } else {
        divisor = k4ByteSize;
    }
    uint32 imm7Mask = 0x7f;
    opnd |= ((static_cast<uint32>(offsetValue / divisor) & imm7Mask) << kShiftFifteen);

    uint32 specialOpCode = 0;
    if (memOpnd.IsPostIndexed()) {
        specialOpCode = 0x2;
    } else if (memOpnd.IsPreIndexed()) {
        specialOpCode = 0x6;
    } else {
        DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "must be kIntact!");
        specialOpCode = 0x4;
    }
    opnd |= specialOpCode << kShiftTwentyTwo;
    return opnd;
}

uint32 AArch64ObjEmitter::GenLoadStoreFloatInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    return GenLoadStoreRegInsn(insn, objFuncEmitInfo);
}

uint32 AArch64ObjEmitter::GenLoadPairFloatInsn(const Insn &insn) const
{
    /* Rt */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rt2 */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftTen;
    /* Mem */
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnThirdOpnd));
    DEBUG_ASSERT(memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi, "support kAddrModeBOi only!");
    /* Rn */
    Operand *baseReg = memOpnd.GetBaseRegister();
    opnd |= GetOpndMachineValue(*baseReg) << kShiftFive;
    /* Imm */
    OfstOperand *ofstOpnd = static_cast<OfstOperand *>(memOpnd.GetOffsetImmediate());
    int32 offsetValue = ofstOpnd->GetOffsetValue();
    uint32 divisor = 0;
    if (insn.GetDesc()->GetOpndDes(kInsnThirdOpnd)->GetSize() == k64BitSize) {
        divisor = k8ByteSize;
    } else {
        divisor = k4ByteSize;
    }
    uint32 imm7Mask = 0x7f;
    opnd |= (static_cast<uint32>(static_cast<uint32>(offsetValue) / divisor) & imm7Mask) << kShiftFifteen;

    uint32 specialOpCode = 0;
    if (memOpnd.IsPostIndexed()) {
        specialOpCode = 0x3;
    } else if (memOpnd.IsPreIndexed()) {
        specialOpCode = 0x7;
    } else {
        DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "must be kIntact!");
        specialOpCode = 0x5;
    }
    opnd |= specialOpCode << kShiftTwentyTwo;
    return opnd;
}

uint32 AArch64ObjEmitter::GenStorePairFloatInsn(const Insn &insn) const
{
    /* Rt */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Rt2 */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftTen;
    /* Mem */
    MemOperand &memOpnd = static_cast<MemOperand &>(insn.GetOperand(kInsnThirdOpnd));
    DEBUG_ASSERT(memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi, "support kAddrModeBOi only!");
    /* Rn */
    Operand *baseReg = memOpnd.GetBaseRegister();
    opnd |= GetOpndMachineValue(*baseReg) << kShiftFive;
    /* Imm */
    OfstOperand *ofstOpnd = static_cast<OfstOperand *>(memOpnd.GetOffsetImmediate());
    int32 offsetValue = ofstOpnd->GetOffsetValue();
    uint32 divisor = 0;
    if (insn.GetDesc()->GetOpndDes(kInsnThirdOpnd)->GetSize() == k64BitSize) {
        divisor = k8ByteSize;
    } else {
        divisor = k4ByteSize;
    }
    uint32 imm7Mask = 0x7f;
    opnd |= (static_cast<uint32>(static_cast<uint32>(offsetValue) / divisor) & imm7Mask) << kShiftFifteen;

    uint32 specialOpCode = 0;
    if (memOpnd.IsPostIndexed()) {
        specialOpCode = 0x2;
    } else if (memOpnd.IsPreIndexed()) {
        specialOpCode = 0x6;
    } else {
        DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "must be kIntact!");
        specialOpCode = 0x4;
    }
    opnd |= specialOpCode << kShiftTwentyTwo;
    return opnd;
}

uint32 AArch64ObjEmitter::GenLoadLiteralRegInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    /* Rt */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Imm */
    opnd |= GetLoadLiteralOpndValue(insn.GetOperand(kInsnSecondOpnd), objFuncEmitInfo) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenLogicalRegInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));

    uint32 operandSize = k2ByteSize;  // mvn insn
    if (insn.GetOperandSize() == operandSize) {
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd)) << kShiftFive;
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftSixteen;
        return opnd;
    }

    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    /* Rm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnThirdOpnd)) << kShiftSixteen;

    operandSize = k4ByteSize;
    if (insn.GetOperandSize() == operandSize) {
        BitShiftOperand &bitShiftOpnd = static_cast<BitShiftOperand &>(insn.GetOperand(kInsnFourthOpnd));
        uint32 shift = 0;
        switch (bitShiftOpnd.GetShiftOp()) {
            case BitShiftOperand::kLSL:
                shift = kShiftLSL;
                break;
            case BitShiftOperand::kLSR:
                shift = kShiftLSR;
                break;
            case BitShiftOperand::kASR:
                shift = kShiftASR;
                break;
            default:
                break;
        }
        /* Shift */
        opnd |= shift << kShiftTwentyTwo;
        /* Imm */
        opnd |= bitShiftOpnd.GetShiftAmount() << kShiftTen;
    }
    return opnd;
}

uint32 AArch64ObjEmitter::GenLogicalImmInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    if (insn.GetMachineOpcode() == MOP_wmovri32 || insn.GetMachineOpcode() == MOP_xmovri64) {
        opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftTen;
        return opnd;
    }

    // tst insn
    if (insn.GetMachineOpcode() == MOP_wtstri32 || insn.GetMachineOpcode() == MOP_xtstri64) {
        // Rn
        uint32 opndValue = GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
        // Imm
        ImmOperand &immOpnd = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
        uint64 value = static_cast<uint64>(immOpnd.GetValue());
        uint32 size = insn.GetDesc()->GetOpndDes(kInsnThirdOpnd)->GetSize();
        opndValue |= EncodeLogicaImm(value, size) << kShiftTen;
        return opndValue;
    }

    /* Rn */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;
    /* Imm */
    ImmOperand &immOpnd = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
    uint64 value = static_cast<uint64>(immOpnd.GetValue());
    uint32 size = insn.GetDesc()->GetOpndDes(kInsnSecondOpnd)->GetSize();
    opnd |= EncodeLogicaImm(value, size) << kShiftTen;
    return opnd;
}

uint32 AArch64ObjEmitter::GenMoveWideInsn(const Insn &insn) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Imm */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftFive;

    BitShiftOperand &lslOpnd = static_cast<BitShiftOperand &>(insn.GetOperand(kInsnThirdOpnd));
    uint32 mod = 16; /* 16 from Armv8 Manual C5.6.128 */
    uint32 shift = lslOpnd.GetShiftAmount() / mod;
    /* Shift */
    opnd |= shift << kShiftTwentyOne;
    return opnd;
}

uint32 AArch64ObjEmitter::GenPCRelAddrInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    /* Rd */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* Imm */
    opnd |= GetAdrLabelOpndValue(insn, insn.GetOperand(kInsnSecondOpnd), objFuncEmitInfo) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenSystemInsn(const Insn &insn) const
{
    (void)insn;
    return 0;
}

uint32 AArch64ObjEmitter::GenTestBranchInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    /* Rt */
    uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
    /* b40 */
    opnd |= GetOpndMachineValue(insn.GetOperand(kInsnSecondOpnd)) << kShiftNineteen;
    /* Imm */
    opnd |= GetTestBranchOpndValue(insn.GetOperand(kInsnThirdOpnd), objFuncEmitInfo) << kShiftFive;
    return opnd;
}

uint32 AArch64ObjEmitter::GenCondBranchInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const
{
    /* Imm */
    uint32 opnd = GetCondBranchOpndValue(insn.GetOperand(kInsnSecondOpnd), objFuncEmitInfo) << kShiftFive;
    return opnd;
}

void AArch64ObjEmitter::InsertNopInsn(ObjFuncEmitInfo &objFuncEmitInfo) const
{
    AArch64CGFunc &cgFunc = static_cast<AArch64CGFunc &>(objFuncEmitInfo.GetCGFunc());
    bool found = false;
    FOR_ALL_BB_REV(bb, &cgFunc)
    {
        FOR_BB_INSNS_REV(insn, bb)
        {
            if (insn->IsMachineInstruction()) {
                if (insn->IsCall()) {
                    Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_nop);
                    bb->InsertInsnAfter(*insn, newInsn);
                }
                found = true;
                break;
            }
        }
        if (found) {
            break;
        }
    }
}

uint32 AArch64ObjEmitter::EncodeLogicaImm(uint64 imm, uint32 size) const
{
    /* the element size */
    uint32 elementSize = size;
    while (elementSize > k2ByteSize) {
        elementSize >>= 1;
        uint64 mask = (1ULL << elementSize) - 1;
        if ((imm & mask) != ((imm >> elementSize) & mask)) {
            elementSize <<= 1;
            break;
        }
    }

    if (elementSize != k64BitSize) {
        imm &= ((1ULL << elementSize) - 1);
    }
    std::bitset<k64BitSize> bitValue(imm);
    uint32 trailCount = 0;
    for (uint32 i = 1; i < elementSize; ++i) {
        if (bitValue[i] ^ bitValue[0]) {
            trailCount = i;
            break;
        }
    }

    uint32 immr = 0;
    uint32 oneNum = bitValue.count();
    if (bitValue.test(0)) { /* for 1+0+1+ pattern */
        DEBUG_ASSERT(oneNum >= trailCount, "value overflow");
        immr = oneNum - trailCount;
    } else { /* for 0+1+0+ pattern */
        immr = elementSize - trailCount;
    }
    CHECK_FATAL(elementSize >= 1, "value overflow");
    DEBUG_ASSERT(oneNum >= 1, "value overflow");
    uint32 imms = ~(elementSize - 1) << 1;
    imms |= oneNum - 1u;
    uint32 n = (elementSize == k64BitSize) ? 1 : 0;
    return (n << kShiftTwelve) | (immr << kShiftSix) | (imms & 0x3f);
}

void AArch64ObjEmitter::EmitIntrinsicInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo)
{
    switch (insn.GetMachineOpcode()) {
        // adrp    xd, label
        // add     xd, xd, #:lo12:label
        case MOP_adrp_label: {
            uint32 opnd = GetOpndMachineValue(insn.GetOperand(kInsnFirstOpnd));
            uint32 binInsn = AArch64CG::kMd[MOP_xadrp].GetMopEncode();
            binInsn |= opnd;
            objFuncEmitInfo.AppendTextData(binInsn, k4ByteSize);
            binInsn = AArch64CG::kMd[MOP_xaddrri12].GetMopEncode();
            binInsn |= opnd | (opnd << kShiftFive);
            objFuncEmitInfo.AppendTextData(binInsn, k4ByteSize);
            break;
        }
        default:
            CHECK_FATAL(false, "unsupport mop in EmitIntrinsicInsn!\n");
    }
}

void AArch64ObjEmitter::EmitSpinIntrinsicInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo)
{
    switch (insn.GetMachineOpcode()) {
        case MOP_tls_desc_rel: {
            objFuncEmitInfo.AppendTextData(0x91400000, k4ByteSize);
            objFuncEmitInfo.AppendTextData(0x91000000, k4ByteSize);
            break;
        }
        default:
            CHECK_FATAL(false, "unsupport mop in EmitSpinIntrinsicInsn!\n");
    }
}
} /* namespace maplebe */
