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

#include "obj_emit.h"
#include "namemangler.h"
#include "cg.h"

namespace maplebe {
using namespace maple;
using namespace namemangler;

void ObjEmitter::Run(FuncEmitInfo &funcEmitInfo)
{
    InsertNopInsn(static_cast<ObjFuncEmitInfo &>(funcEmitInfo));
    EmitFuncBinaryCode(static_cast<ObjFuncEmitInfo &>(funcEmitInfo));
}

/* traverse insns, get the binary code and saved in buffer */
void ObjEmitter::EmitFuncBinaryCode(ObjFuncEmitInfo &objFuncEmitInfo)
{
    CGFunc &cgFunc = objFuncEmitInfo.GetCGFunc();
    objFuncEmitInfo.SetFuncName(cgFunc.GetName());

    int labelSize = static_cast<int>(cgFunc.GetLab2BBMap().size()) +
                    static_cast<int>(cgFunc.GetLabelAndValueMap().size()) + 1;
    std::vector<uint32> label2Offset(labelSize, 0xFFFFFFFFULL);
    EmitInstructions(objFuncEmitInfo, label2Offset);
    objFuncEmitInfo.UpdateMethodCodeSize();

    int symbolSize = static_cast<int>(cgFunc.GetLabelIdx() + 1);
    std::vector<uint32> symbol2Offset(symbolSize, 0xFFFFFFFFULL);
    EmitFunctionSymbolTable(objFuncEmitInfo, symbol2Offset);
    EmitSwitchTable(objFuncEmitInfo, symbol2Offset);

    /* local float variable */
    for (const auto &mpPair : cgFunc.GetLabelAndValueMap()) {
        CHECK_FATAL(mpPair.first < label2Offset.size(), "label2Offset");
        label2Offset[mpPair.first] = objFuncEmitInfo.GetTextDataSize();
        objFuncEmitInfo.AppendTextData(&(mpPair.second), k8ByteSize);
    }

    /* handle branch fixup here */
    objFuncEmitInfo.HandleLocalBranchFixup(label2Offset, symbol2Offset);
}

void ObjEmitter::EmitInstructions(ObjFuncEmitInfo &objFuncEmitInfo, std::vector<uint32> &label2Offset)
{
    CGFunc &cgFunc = objFuncEmitInfo.GetCGFunc();
    FOR_ALL_BB(bb, &cgFunc) {
        if (bb->GetLabIdx() != 0) {
            CHECK_FATAL(bb->GetLabIdx() < label2Offset.size(), "label2Offset");
            label2Offset[bb->GetLabIdx()] = objFuncEmitInfo.GetTextDataSize();
            objFuncEmitInfo.AppendLabel2Order(bb->GetLabIdx());
        }

        FOR_BB_INSNS(insn, bb) {
            if (!insn->IsMachineInstruction() || insn->IsAsmInsn() || insn->IsPseudo()) {
                continue;
            }

            /* get binary code and save in buffer */
            if (insn->GetDesc()->IsIntrinsic()) {
                EmitIntrinsicInsn(*insn, objFuncEmitInfo);
            } else if (insn->GetDesc()->IsSpecialIntrinsic()) {
                EmitSpinIntrinsicInsn(*insn, objFuncEmitInfo);
            } else {
                EncodeInstruction(*insn, label2Offset, objFuncEmitInfo);
            }
        }
    }
}

void ObjEmitter::EmitSwitchTable(ObjFuncEmitInfo &objFuncEmitInfo, const std::vector<uint32> &symbol2Offset)
{
    CGFunc &cgFunc = objFuncEmitInfo.GetCGFunc();
    if (cgFunc.GetEmitStVec().size() == 0) {
        return;
    }
    uint32 tmpOffset = GetBeforeTextDataSize(objFuncEmitInfo);
    /* align is 8 push padding to objFuncEmitInfo.data */
    uint32 startOffset = Alignment::Align<uint32>(tmpOffset, k8ByteSize);
    uint32 padding = startOffset - tmpOffset;
    objFuncEmitInfo.FillTextDataNop(padding);

    uint32 curOffset = objFuncEmitInfo.GetTextDataSize();
    for (std::pair<uint32, MIRSymbol *> st : cgFunc.GetEmitStVec()) {
        objFuncEmitInfo.SetSwitchTableOffset(st.second->GetName(), curOffset);
        MIRAggConst *arrayConst = safe_cast<MIRAggConst>(st.second->GetKonst());
        DEBUG_ASSERT(arrayConst != nullptr, "null ptr check");
        for (size_t i = 0; i < arrayConst->GetConstVec().size(); ++i) {
            MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));
            DEBUG_ASSERT(lblConst != nullptr, "null ptr check");
            CHECK_FATAL(lblConst->GetValue() <= symbol2Offset.size(), "symbol2Offset");
            uint64 offset = static_cast<uint64>(symbol2Offset[lblConst->GetValue()]) - static_cast<uint64>(curOffset);
            objFuncEmitInfo.AppendTextData(offset, k8ByteSize);
        }

        curOffset += arrayConst->GetConstVec().size() * k8ByteSize;
    }
}

void ObjEmitter::WriteObjFile()
{
    const auto &emitMemorymanager = CGOptions::GetInstance().GetEmitMemoryManager();
    if (emitMemorymanager.codeSpace != nullptr) {
        DEBUG_ASSERT(textSection != nullptr, "textSection has not been initialized");
        uint8 *codeSpace = emitMemorymanager.allocateDataSection(emitMemorymanager.codeSpace,
            textSection->GetDataSize(), textSection->GetAlign(), textSection->GetName().c_str());
        memcpy_s(codeSpace, textSection->GetDataSize(), textSection->GetData().data(), textSection->GetDataSize());
        if (CGOptions::addFuncSymbol()) {
            uint8 *symtabSpace = emitMemorymanager.allocateDataSection(emitMemorymanager.codeSpace,
                symbolTabSection->GetDataSize(), symbolTabSection->GetAlign(), symbolTabSection->GetName().c_str());
            memcpy_s(symtabSpace, symbolTabSection->GetDataSize(),
                symbolTabSection->GetAddr(), symbolTabSection->GetDataSize());
            uint8 *stringTabSpace = emitMemorymanager.allocateDataSection(emitMemorymanager.codeSpace,
                strTabSection->GetDataSize(), strTabSection->GetAlign(), strTabSection->GetName().c_str());
            memcpy_s(stringTabSpace, strTabSection->GetDataSize(),
                strTabSection->GetData().data(), strTabSection->GetDataSize());
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
        section->WriteSection(outStream);
    }

    /* write section table */
    SetFileOffset(header.e_shoff);
    for (auto section : sections) {
        Emit(&section->GetSectionHeader(), sizeof(section->GetSectionHeader()));
    }
}

void ObjEmitter::AddSymbol(const std::string &name, Word size, const Section &section, Address value)
{
    auto nameIndex = strTabSection->AddString(name);
    symbolTabSection->AppendSymbol({static_cast<Word>(nameIndex),
                                    static_cast<uint8_t>((STB_GLOBAL << 4) + (STT_SECTION & 0xf)), 0,
                                    section.GetIndex(), value, size});
}

void ObjEmitter::AddFuncSymbol(const MapleString &name, Word size, Address value)
{
    auto symbolStrIndex = strTabSection->AddString(name);
    symbolTabSection->AppendSymbol({static_cast<Word>(symbolStrIndex),
                                    static_cast<uint8>((STB_GLOBAL << k4BitSize) + (STT_FUNC & 0xf)), 0,
                                    textSection->GetIndex(), value, size});
}

void ObjEmitter::ClearData()
{
    globalLabel2Offset.clear();
    for (auto *section : sections) {
        if (section != nullptr) {
            section->ClearData();
        }
    }
}

void ObjEmitter::InitELFHeader()
{
    header.e_ident[EI_MAG0] = ELFMAG0;
    header.e_ident[EI_MAG1] = ELFMAG1;
    header.e_ident[EI_MAG2] = ELFMAG2;
    header.e_ident[EI_MAG3] = ELFMAG3;
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA] = ELFDATA2LSB;
    header.e_ident[EI_VERSION] = EV_CURRENT;
    header.e_ident[EI_OSABI] = ELFOSABI_LINUX;
    header.e_ident[EI_ABIVERSION] = 0;
    std::fill_n(&header.e_ident[EI_PAD], EI_NIDENT - EI_PAD, 0);
    header.e_type = ET_REL;
    header.e_version = 1;
    UpdateMachineAndFlags(header);
    header.e_entry = 0;
    header.e_ehsize = sizeof(FileHeader);
    header.e_phentsize = sizeof(SegmentHeader);
    header.e_shentsize = sizeof(SectionHeader);
    header.e_shstrndx = shStrSection->GetIndex();
    header.e_shoff = 0;
    header.e_phoff = 0;
    header.e_shnum = sections.size();
    header.e_phnum = 0;
}

void ObjEmitter::EmitMIRIntConst(EmitInfo &emitInfo)
{
    DEBUG_ASSERT(IsPrimitiveScalar(emitInfo.elemConst.GetType().GetPrimType()), "must be primitive type!");
    MIRIntConst &intConst = static_cast<MIRIntConst &>(emitInfo.elemConst);
    size_t size = GetPrimTypeSize(emitInfo.elemConst.GetType().GetPrimType());
    const IntVal &value = intConst.GetValue();
    int64 val = value.GetExtValue();
    dataSection->AppendData(&val, size);
    emitInfo.offset += size;
#ifdef OBJ_DEBUG
    LogInfo::MapleLogger() << val << " size: " << size << "\n";
#endif
}

void ObjEmitter::EmitMIRAddrofConstCommon(EmitInfo &emitInfo, uint64 specialOffset)
{
    MIRAddrofConst &symAddr = static_cast<MIRAddrofConst &>(emitInfo.elemConst);
    MIRSymbol *symAddrSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(symAddr.GetSymbolIndex().Idx());
    DEBUG_ASSERT(symAddrSym != nullptr, "null ptr check");
    const std::string &symAddrName = symAddrSym->GetName();
    LabelFixup labelFixup(symAddrName, emitInfo.offset, kLabelFixupDirect64);
    if (specialOffset != 0) {
        DataSection::AddLabelFixup(emitInfo.labelFixups, labelFixup);
    }
    uint64 value = specialOffset - emitInfo.offset;
    size_t size = GetPrimTypeSize(emitInfo.elemConst.GetType().GetPrimType());
    dataSection->AppendData(&value, size);
    emitInfo.offset += size;

#ifdef OBJ_DEBUG
    LogInfo::MapleLogger() << symAddrName << " size: " << size << "\n";
#endif
}

void ObjEmitter::EmitMIRAddrofConst(EmitInfo &emitInfo)
{
    EmitMIRAddrofConstCommon(emitInfo, 0);
}

void ObjEmitter::EmitMIRAddrofConstOffset(EmitInfo &emitInfo)
{
    /* 2 is fixed offset in runtime */
    EmitMIRAddrofConstCommon(emitInfo, 2);
}

void ObjEmitter::EmitFunctionSymbolTable(ObjFuncEmitInfo &objFuncEmitInfo, std::vector<uint32> &symbol2Offset)
{
    CGFunc &cgFunc = objFuncEmitInfo.GetCGFunc();
    MIRFunction *func = &cgFunc.GetFunction();

    size_t size =
        (func == nullptr) ? GlobalTables::GetGsymTable().GetTable().size() : func->GetSymTab()->GetTable().size();
    for (size_t i = 0; i < size; ++i) {
        const MIRSymbol *st = nullptr;
        if (func == nullptr) {
            auto &symTab = GlobalTables::GetGsymTable();
            st = symTab.GetSymbol(i);
        } else {
            auto &symTab = *func->GetSymTab();
            st = symTab.GetSymbolAt(i);
        }
        if (st == nullptr) {
            continue;
        }
        MIRStorageClass storageClass = st->GetStorageClass();
        MIRSymKind symKind = st->GetSKind();
        if (storageClass == kScPstatic && symKind == kStConst) {
            // align
            size_t tmpOffset = GetBeforeTextDataSize(objFuncEmitInfo);
            uint32 offset = Alignment::Align<uint32>(tmpOffset, k8ByteSize);
            uint32 padding = offset - tmpOffset;
            objFuncEmitInfo.FillTextDataNop(padding);
            CHECK_FATAL(cgFunc.GetLocalSymLabelIndex(*st) <= symbol2Offset.size(), "symbol2Offset");
            symbol2Offset[cgFunc.GetLocalSymLabelIndex(*st)] = static_cast<uint32>(objFuncEmitInfo.GetTextDataSize());
            if (st->GetKonst()->GetKind() == kConstStr16Const) {
                EmitStr16Const(objFuncEmitInfo, *st);
                continue;
            }

            if (st->GetKonst()->GetKind() == kConstStrConst) {
                EmitStrConst(objFuncEmitInfo, *st);
                continue;
            }

            switch (st->GetKonst()->GetType().GetPrimType()) {
                case PTY_u32: {
                    MIRIntConst *intConst = safe_cast<MIRIntConst>(st->GetKonst());
                    uint32 value = static_cast<uint32>(intConst->GetValue().GetExtValue());
                    objFuncEmitInfo.AppendTextData(&value, sizeof(value));
                    break;
                }
                case PTY_f32: {
                    MIRFloatConst *floatConst = safe_cast<MIRFloatConst>(st->GetKonst());
                    uint32 value = static_cast<uint32>(floatConst->GetIntValue());
                    objFuncEmitInfo.AppendTextData(&value, sizeof(value));
                    break;
                }
                case PTY_f64: {
                    MIRDoubleConst *doubleConst = safe_cast<MIRDoubleConst>(st->GetKonst());
                    uint32 value = doubleConst->GetIntLow32();
                    objFuncEmitInfo.AppendTextData(&value, sizeof(value));
                    value = doubleConst->GetIntHigh32();
                    objFuncEmitInfo.AppendTextData(&value, sizeof(value));
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void ObjEmitter::EmitStr16Const(ObjFuncEmitInfo &objFuncEmitInfo, const MIRSymbol &str16Symbol)
{
    MIRStr16Const *mirStr16Const = safe_cast<MIRStr16Const>(str16Symbol.GetKonst());
    DEBUG_ASSERT(mirStr16Const != nullptr, "nullptr check");
    const std::u16string &str16 = GlobalTables::GetU16StrTable().GetStringFromStrIdx(mirStr16Const->GetValue());

    uint32 len = str16.length();
    for (uint32 i = 0; i < len; ++i) {
        char16_t c = str16[i];
        objFuncEmitInfo.AppendTextData(&c, sizeof(c));
    }
    if ((str16.length() & 0x1) == 1) {
        uint16 value = 0;
        objFuncEmitInfo.AppendTextData(&value, sizeof(value));
    }
}

void ObjEmitter::EmitStrConst(ObjFuncEmitInfo &objFuncEmitInfo, const MIRSymbol &strSymbol)
{
    MIRStrConst *mirStrConst = safe_cast<MIRStrConst>(strSymbol.GetKonst());

    auto str = GlobalTables::GetUStrTable().GetStringFromStrIdx(mirStrConst->GetValue());
    size_t size = str.length();
    /* 1 is tail 0 of the str string */
    objFuncEmitInfo.AppendTextData(str.c_str(), size + 1);
}
} /* namespace maplebe */
