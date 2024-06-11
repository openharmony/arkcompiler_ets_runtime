/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/dump_code/jit_dump_elf.h"

namespace panda::ecmascript {

void JsJitDumpElf::Initx86ElfHeader()
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
    header.e_ehsize = sizeof(maplebe::FileHeader);
    header.e_phentsize = 0;
    header.e_phnum = 0;
    header.e_shentsize = sizeof(maplebe::SectionHeader);
    header.e_shnum = static_cast<uint16>(sections.size());
    header.e_shstrndx = strTabSection->GetIndex();
}

void JsJitDumpElf::InitArmElfHeader()
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
    header.e_machine = EM_AARCH64;
    header.e_flags = 0;
    header.e_entry = 0;
    header.e_ehsize = sizeof(maplebe::FileHeader);
    header.e_phentsize = sizeof(maplebe::SegmentHeader);
    header.e_shentsize = sizeof(maplebe::SectionHeader);
    header.e_shstrndx = strTabSection->GetIndex();
    header.e_shoff = 0;
    header.e_phoff = 0;
    header.e_shnum = sections.size();
    header.e_phnum = 0;
}

void JsJitDumpElf::UpdateSectionOffset(Section &section)
{
    if (section.GetType() != SHT_NOBITS) {
        section.SetOffset(globalOffset);
    } else {
        section.SetOffset(0);
    }
}

void JsJitDumpElf::UpdateGlobalOffset(Section &section)
{
    if (section.GetType() != SHT_NOBITS) {
        globalOffset += section.GetSectionSize();
    }
}

void JsJitDumpElf::LayoutSections()
{
    globalOffset = sizeof(maplebe::FileHeader);
    globalOffset = Alignment::Align<maplebe::Offset>(globalOffset, k8Bits);

    for (auto *section : sections) {
        section->SetSectionHeaderNameIndex(static_cast<maplebe::Word>(strTabSection->AddString(section->GetName())));
    }

    for (auto *section : sections) {
        globalOffset = Alignment::Align<maplebe::Offset>(globalOffset, section->GetAlign());
        /* lay out section */
        UpdateSectionOffset(*section);
        if (section->GetType() != SHT_NOBITS) {
            section->GenerateData();
        }
        UpdateGlobalOffset(*section);
    }

    globalOffset = Alignment::Align<maplebe::Offset>(globalOffset, 16U);
    header.e_shoff = globalOffset;
    header.e_shnum = static_cast<uint16>(sections.size());
}

void JsJitDumpElf::RegisterSection(Section &section)
{
    sections.push_back(&section);
    section.SetIndex(static_cast<maplebe::SectionIndex>(sections.size() - 1));
}

void JsJitDumpElf::Init()
{
    DataSection *nullDataSection = new DataSection(" ", SHT_NULL, 0, 0);
    RegisterSection(*nullDataSection);
    strTabSection = new StringSection(".strtab", SHT_STRTAB, 0, 1);
    RegisterSection(*strTabSection);
    textSection = new DataSection(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, k8Bits);
    RegisterSection(*textSection);
    symbolTabSection = new SymbolSection(".symtab", SHT_SYMTAB, 0, k8Bits, *strTabSection);
    RegisterSection(*symbolTabSection);
}

void JsJitDumpElf::AppendData(std::vector<uint8> &codeBuff)
{
    textSection->AppendData(codeBuff.data(), codeBuff.size());
}

void JsJitDumpElf::AddSymToSymTab(const maplebe::Symbol &symbol, int64 symIdx)
{
    localSymTab.push_back(std::make_pair(symbol, symIdx));
}

void JsJitDumpElf::AppendGlobalSymsToSymTabSec()
{
    for (auto elem : localSymTab) {
        const maplebe::Symbol &symbol = elem.first;
        int64 symIdx = elem.second;
        symbolTabSection->AppendSymbol(symbol);
        symbolTabSection->AppendIdxInSymbols(symIdx);
    }
}

void JsJitDumpElf::AppendSymbolToSymTab(int64 symIdx, uint64 funcSymValue, uint64 funcSymSize,
                                        const std::string &symbolName)
{
    uint8 funcSymType = STB_GLOBAL;
    auto nameIndex = strTabSection->AddString(symbolName);
    AddSymToSymTab({static_cast<maplebe::Word>(nameIndex),
        static_cast<uint8>((funcSymType << kLeftShift4Bits) + (STT_FUNC & 0xf)), 0, textSection->GetIndex(),
        funcSymValue, funcSymSize}, symIdx);
}

void JsJitDumpElf::SetFileOffset(int fd, uint64 offset)
{
    lseek(fd, offset, SEEK_SET);
}

void JsJitDumpElf::WriteJitElfFile(int fd)
{
    AppendGlobalSymsToSymTabSec();
    /* Init elf file header */
    InitArmElfHeader();
    LayoutSections();
    /* write header */
    (void)write(fd, reinterpret_cast<const char *>(&header), sizeof(header));
    /* write sections */
    for (auto *section : sections) {
        if (section->GetType() == SHT_NOBITS) {
            continue;
        }
        SetFileOffset(fd, section->GetOffset());
        section->WriteSection(fd);
    }
    SetFileOffset(fd, header.e_shoff);
    /* write section table */
    for (auto *section : sections) {
        (void)write(fd, &section->GetSectionHeader(), sizeof(section->GetSectionHeader()));
    }
}

void JsJitDumpElf::ClearData()
{
    localSymTab.clear();
    globalOffset = 0;
    textSection->ClearData();
    symbolTabSection->ClearData();
    strTabSection->ClearData();
    sections.clear();
}

}  // namespace panda::ecmascript
