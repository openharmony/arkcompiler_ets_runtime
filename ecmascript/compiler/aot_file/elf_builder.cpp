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

#include "ecmascript/compiler/aot_file/elf_builder.h"

namespace panda::ecmascript {
void ElfBuilder::AddShStrTabSection()
{
    std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections =
        des_[ShStrTableModuleDesIndex].GetSectionsInfo();

    uint32_t size = 1;
    for (auto &s : sections_) {
        std::string str = ModuleSectionDes::GetSecName(s);
        size = size + str.size() + 1;
    }

    shStrTabPtr_ = std::make_unique<char []>(size);
    char *dst = shStrTabPtr_.get();
    dst[0] = 0x0;
    uint32_t i = 1;
    for (auto &s: sections_) {
        std::string str = ModuleSectionDes::GetSecName(s);
        uint32_t copySize = str.size();
        if (copySize == 0) {
            UNREACHABLE();
        }
        ASSERT(size >= i);
        if ((copySize != 0) && ((memcpy_s(dst + i, size - i + 1, str.data(), copySize)) != EOK)) {
            UNREACHABLE();
        }
        dst[i + copySize] = 0x0;
        i = i + copySize + 1;
    }
    if (sections.find(ElfSecName::SHSTRTAB) != sections.end()) {
        sections.erase(ElfSecName::SHSTRTAB);
    }
    sections[ElfSecName::SHSTRTAB] = std::make_pair(reinterpret_cast<uint64_t>(shStrTabPtr_.get()), size);
    if (enableSecDump_) {
        DumpSection();
    }
}

uint32_t ElfBuilder::AddAsmStubStrTab(std::ofstream &elfFile,
    const std::vector<std::pair<std::string, uint32_t>> &asmStubELFInfo)
{
    uint32_t size = 1;
    ASSERT(asmStubELFInfo.size() > 0);
    uint32_t asmStubSymTabNum = asmStubELFInfo.size() - 1;
    for (size_t idx = 0; idx < asmStubSymTabNum; ++idx) {
        const std::string &str = asmStubELFInfo[idx].first;
        size = size + str.size() + 1;
    }

    std::unique_ptr<char []> asmStubStrTabPtr = std::make_unique<char []>(size);
    char *dst = asmStubStrTabPtr.get();
    dst[0] = 0x0;
    uint32_t i = 1;
    for (size_t idx = 0; idx < asmStubSymTabNum; ++idx) {
        const std::string &str = asmStubELFInfo[idx].first;
        asmStubStrName_.emplace_back(i);
        uint32_t copySize = str.size();
        if (copySize == 0) {
            UNREACHABLE();
        }
        if ((copySize != 0) && ((memcpy_s(dst + i, size - i + 1, str.data(), copySize)) != EOK)) {
            UNREACHABLE();
        }
        dst[i + copySize] = 0x0;
        i = i + copySize + 1;
    }
    elfFile.write(reinterpret_cast<char *>(dst), size);
    return size;
}

void ElfBuilder::DumpSection() const
{
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetFullSecInfo();
    // dump
    for (auto &s : sections) {
        ElfSection section = ElfSection(s.first);
        if (!section.ShouldDumpToAOTFile()) {
            continue;
        }
        LOG_COMPILER(INFO) << "secname :" << std::dec << static_cast<int>(s.first)
            << " addr:0x" << std::hex << s.second.first << " size:0x" << s.second.second << std::endl;
    }
}

ElfBuilder::ElfBuilder(const std::vector<ModuleSectionDes> &des,
    const std::vector<ElfSecName> &sections): des_(des), sections_(sections)
{
    Initialize();
    AddShStrTabSection();
    RemoveNotNeedSection();
}

void ElfBuilder::Initialize()
{
    for (size_t i = 0; i < des_.size(); i++) {
        des_[i].AddArkStackMapSection();
    }
    sectionToAlign_ = {
        {ElfSecName::TEXT, AOTFileInfo::PAGE_ALIGN},
        {ElfSecName::STRTAB, 1},
        {ElfSecName::SYMTAB, AOTFileInfo::DATA_SEC_ALIGN},
        {ElfSecName::SHSTRTAB, AOTFileInfo::DATA_SEC_ALIGN},
        {ElfSecName::ARK_STACKMAP, AOTFileInfo::DATA_SEC_ALIGN},
        {ElfSecName::ARK_FUNCENTRY, AOTFileInfo::DATA_SEC_ALIGN},
        {ElfSecName::ARK_ASMSTUB, AOTFileInfo::DATA_SEC_ALIGN},
        {ElfSecName::ARK_MODULEINFO, AOTFileInfo::DATA_SEC_ALIGN},
    };

    sectionToSegment_ = {
        {ElfSecName::RODATA, ElfSecName::TEXT},
        {ElfSecName::RODATA_CST4, ElfSecName::TEXT},
        {ElfSecName::RODATA_CST8, ElfSecName::TEXT},
        {ElfSecName::RODATA_CST16, ElfSecName::TEXT},
        {ElfSecName::RODATA_CST32, ElfSecName::TEXT},
        {ElfSecName::TEXT, ElfSecName::TEXT},
        {ElfSecName::STRTAB, ElfSecName::DATA},
        {ElfSecName::SYMTAB, ElfSecName::DATA},
        {ElfSecName::SHSTRTAB, ElfSecName::DATA},
        {ElfSecName::ARK_STACKMAP, ElfSecName::DATA},
        {ElfSecName::ARK_FUNCENTRY, ElfSecName::DATA},
        {ElfSecName::ARK_ASMSTUB, ElfSecName::TEXT},
        {ElfSecName::ARK_MODULEINFO, ElfSecName::DATA},
    };

    segmentToFlag_ = {
        {ElfSecName::TEXT, llvm::ELF::PF_X | llvm::ELF::PF_R},
        {ElfSecName::DATA, llvm::ELF::PF_R},
    };

    SetLastSection();
}

void ElfBuilder::RemoveNotNeedSection()
{
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetFullSecInfo();
    for (size_t i = 0; i < sections_.size();) {
        if (sections.find(sections_[i]) == sections.end()) {
            auto it = sections_.begin() + i;
            sections_.erase(it);
            continue;
        }
        i++;
    }
}

ElfBuilder::~ElfBuilder()
{
    shStrTabPtr_ = nullptr;
}

uint32_t ElfBuilder::GetShIndex(ElfSecName section) const
{
    std::set<ElfSecName> secSet(sections_.begin(), sections_.end());
    uint32_t idx = 1;
    for (ElfSecName sec : secSet) {
        if (sec == section) {
            return idx;
        }
        idx++;
    }
    return 0;
}

int ElfBuilder::GetSecNum() const
{
    return sections_.size() + 1; // add first empty section.
}

/*
ELF Header as follow:
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              DYN (Shared object file)
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x4000001
  Entry point address:               0x0
  Start of program headers:          16384 (bytes into file)
  Start of section headers:          64 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         2
  Size of section headers:           64 (bytes)
  Number of section headers:         7
  Section header string table index: 3
*/
void ElfBuilder::PackELFHeader(llvm::ELF::Elf64_Ehdr &header, uint32_t version, Triple triple)
{
    if (memset_s(reinterpret_cast<void *>(&header), sizeof(llvm::ELF::Elf64_Ehdr), 0,
                 sizeof(llvm::ELF::Elf64_Ehdr)) != EOK) {
        UNREACHABLE();
    }
    header.e_ident[llvm::ELF::EI_MAG0] = llvm::ELF::ElfMagic[llvm::ELF::EI_MAG0];
    header.e_ident[llvm::ELF::EI_MAG1] = llvm::ELF::ElfMagic[llvm::ELF::EI_MAG1];
    header.e_ident[llvm::ELF::EI_MAG2] = llvm::ELF::ElfMagic[llvm::ELF::EI_MAG2];
    header.e_ident[llvm::ELF::EI_MAG3] = llvm::ELF::ElfMagic[llvm::ELF::EI_MAG3];
    header.e_ident[llvm::ELF::EI_CLASS] = llvm::ELF::ELFCLASS64;
    header.e_ident[llvm::ELF::EI_DATA] = llvm::ELF::ELFDATA2LSB;
    header.e_ident[llvm::ELF::EI_VERSION] = 1;

    header.e_type = llvm::ELF::ET_DYN;
    switch (triple) {
        case Triple::TRIPLE_AMD64:
            header.e_machine = llvm::ELF::EM_X86_64;
            break;
        case Triple::TRIPLE_ARM32:
            header.e_machine = llvm::ELF::EM_ARM;
            break;
        case Triple::TRIPLE_AARCH64:
            header.e_machine = llvm::ELF::EM_AARCH64;
            break;
        default:
            UNREACHABLE();
            break;
    }
    header.e_version = version;
    // start of section headers
    header.e_shoff = sizeof(llvm::ELF::Elf64_Ehdr);
    // size of ehdr
    header.e_ehsize = sizeof(llvm::ELF::Elf64_Ehdr);
    // size of section headers
    header.e_shentsize = sizeof(llvm::ELF::Elf64_Shdr);
    // number of section headers
    header.e_shnum = GetSecNum();
    // section header string table index
    header.e_shstrndx = static_cast<llvm::ELF::Elf64_Half>(GetShIndex(ElfSecName::SHSTRTAB));
    // section header stub sec info index
    header.e_flags = static_cast<llvm::ELF::Elf64_Word>(GetShIndex(ElfSecName::ARK_MODULEINFO));
    // phr
    header.e_phentsize = sizeof(llvm::ELF::Elf64_Phdr);
    header.e_phnum = GetSegmentNum();
}

int ElfBuilder::GetSegmentNum() const
{
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetFullSecInfo();
    std::set<ElfSecName> segments;
    for (auto &s: sections) {
        ElfSection section = ElfSection(s.first);
        if (!section.ShouldDumpToAOTFile()) {
            continue;
        }
        auto it = sectionToSegment_.find(s.first);
        ASSERT(it != sectionToSegment_.end());
        ElfSecName name = it->second;
        segments.insert(name);
    }
    return segments.size();
}

void ElfBuilder::SetLastSection()
{
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetFullSecInfo();
    for (auto &s: sections) {
        ElfSection section = ElfSection(s.first);
        if (!section.ShouldDumpToAOTFile()) {
            continue;
        }
        auto it = sectionToSegment_.find(s.first);
        ASSERT(it != sectionToSegment_.end());
        ElfSecName name = it->second;
        if (name == ElfSecName::TEXT) {
            lastCodeSection = std::max(lastCodeSection, s.first);
        } else {
            lastDataSection = std::max(lastDataSection, s.first);
        }
    }
}

llvm::ELF::Elf64_Word ElfBuilder::FindShName(std::string name, uintptr_t strTabPtr, int strTabSize)
{
    llvm::ELF::Elf64_Word ans = -1;
    int len = static_cast<int>(name.size());
    if (strTabSize < len + 1) {
        return ans;
    }
    LOG_ECMA(DEBUG) << "  FindShName name:" << name.c_str() << std::endl;
    for (int i = 0; i < strTabSize - len + 1; ++i) {
        char *dst = reinterpret_cast<char *>(strTabPtr) + i;
        if (name.compare(dst) == 0) {
            return i;
        }
    }
    return ans;
}

std::pair<uint64_t, uint32_t> ElfBuilder::FindShStrTab() const
{
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetFullSecInfo();
    uint64_t shStrTabAddr = 0;
    uint32_t shStrTabSize = 0;
    for (auto &s: sections) {
        uint32_t curSecSize = des_[ShStrTableModuleDesIndex].GetSecSize(s.first);
        uint64_t curSecAddr = des_[ShStrTableModuleDesIndex].GetSecAddr(s.first);
        if (s.first == ElfSecName::SHSTRTAB) {
            shStrTabSize = curSecSize;
            shStrTabAddr = curSecAddr;
            break;
        }
    }
    return std::make_pair(shStrTabAddr, shStrTabSize);
}

void ElfBuilder::AllocateShdr(std::unique_ptr<llvm::ELF::Elf64_Shdr []> &shdr, const uint32_t &secNum)
{
    shdr = std::make_unique<llvm::ELF::Elf64_Shdr []>(secNum);
    if (memset_s(reinterpret_cast<void *>(&shdr[0]),
                 sizeof(llvm::ELF::Elf64_Shdr),
                 0,
                 sizeof(llvm::ELF::Elf64_Shdr)) != EOK) {
        UNREACHABLE();
    }
}

llvm::ELF::Elf64_Off ElfBuilder::ComputeEndAddrOfShdr(const uint32_t &secNum) const
{
    llvm::ELF::Elf64_Off curSecOffset = sizeof(llvm::ELF::Elf64_Ehdr) + secNum * sizeof(llvm::ELF::Elf64_Shdr);
    curSecOffset = AlignUp(curSecOffset, PageSize()); // not pagesize align will cause performance degradation
    return curSecOffset;
}

ElfSecName ElfBuilder::GetSegmentName(const ElfSecName &secName) const
{
    auto it = sectionToSegment_.find(secName);
    ASSERT(it != sectionToSegment_.end());
    ElfSecName segName = it->second;
    return segName;
}

void ElfBuilder::MergeTextSections(std::ofstream &file,
                                   std::vector<ModuleSectionDes::ModuleRegionInfo> &moduleInfo,
                                   llvm::ELF::Elf64_Off &curSecOffset)
{
    for (size_t i = 0; i < des_.size(); ++i) {
        ModuleSectionDes &des = des_[i];
        ModuleSectionDes::ModuleRegionInfo &curInfo = moduleInfo[i];
        uint32_t curSecSize = des.GetSecSize(ElfSecName::TEXT);
        uint64_t curSecAddr = des.GetSecAddr(ElfSecName::TEXT);
        curSecOffset = AlignUp(curSecOffset, AOTFileInfo::PAGE_ALIGN);
        file.seekp(curSecOffset);
        auto curModuleSec = des.GetSectionsInfo();
        uint64_t rodataAddrBeforeText = 0;
        uint32_t rodataSizeBeforeText = 0;
        uint64_t rodataAddrAfterText = 0;
        uint32_t rodataSizeAfterText = 0;
        std::tie(rodataAddrBeforeText, rodataSizeBeforeText, rodataAddrAfterText, rodataSizeAfterText) =
            des.GetMergedRODataAddrAndSize(curSecAddr);
        if (rodataSizeBeforeText != 0) {
            file.write(reinterpret_cast<char *>(rodataAddrBeforeText), rodataSizeBeforeText);
            curInfo.rodataSizeBeforeText = rodataSizeBeforeText;
            curSecOffset += rodataSizeBeforeText;
            curSecOffset = AlignUp(curSecOffset, AOTFileInfo::TEXT_SEC_ALIGN);
            file.seekp(curSecOffset);
        }
        stubTextOffset_.emplace_back(curSecOffset);
        file.write(reinterpret_cast<char *>(curSecAddr), curSecSize);
        curInfo.textSize = curSecSize;
        curSecOffset += curSecSize;
        if (rodataSizeAfterText != 0) {
            curSecOffset = AlignUp(curSecOffset, AOTFileInfo::DATA_SEC_ALIGN);
            file.seekp(curSecOffset);
            file.write(reinterpret_cast<char *>(rodataAddrAfterText), rodataSizeAfterText);
            curInfo.rodataSizeAfterText = rodataSizeAfterText;
            curSecOffset += rodataSizeAfterText;
        }
    }
}

void ElfBuilder::MergeStrtabSections(std::ofstream &file,
                                     std::vector<ModuleSectionDes::ModuleRegionInfo> &moduleInfo,
                                     llvm::ELF::Elf64_Off &curSecOffset)
{
    for (size_t i = 0; i < des_.size(); ++i) {
        ModuleSectionDes &des = des_[i];
        ModuleSectionDes::ModuleRegionInfo &curInfo = moduleInfo[i];
        uint32_t curSecSize = des.GetSecSize(ElfSecName::STRTAB);
        uint64_t curSecAddr = des.GetSecAddr(ElfSecName::STRTAB);
        curInfo.strtabSize = curSecSize;
        file.write(reinterpret_cast<char *>(curSecAddr), curSecSize);
        curSecOffset += curSecSize;
        if (des.HasAsmStubStrTab()) {
            uint32_t asmStubStrTabSize = AddAsmStubStrTab(file, des.GetAsmStubELFInfo());
            curSecOffset += asmStubStrTabSize;
            curInfo.strtabSize += asmStubStrTabSize;
        }
    }
}

void ElfBuilder::MergeSymtabSections(std::ofstream &file,
                                     std::vector<ModuleSectionDes::ModuleRegionInfo> &moduleInfo,
                                     llvm::ELF::Elf64_Off &curSecOffset,
                                     llvm::ELF::Elf64_Off &asmStubOffset)
{
    using Elf64_Sym = llvm::ELF::Elf64_Sym;
    uint32_t strTabSize = 0;
    uint32_t textSecIndex = GetShIndex(ElfSecName::ARK_ASMSTUB);
    for (size_t i = 0; i < des_.size(); ++i) {
        ModuleSectionDes &des = des_[i];
        ModuleSectionDes::ModuleRegionInfo &curInfo = moduleInfo[i];
        uint32_t curSecSize = des.GetSecSize(ElfSecName::SYMTAB);
        uint64_t curSecAddr = des.GetSecAddr(ElfSecName::SYMTAB);
        curInfo.symtabSize = curSecSize;
        file.write(reinterpret_cast<char *>(curSecAddr), curSecSize);
        curSecOffset += curSecSize;
        strTabSize += des.GetSecSize(ElfSecName::STRTAB);
        if (des.HasAsmStubStrTab()) {
            const std::vector<std::pair<std::string, uint32_t>> &asmStubELFInfo = des.GetAsmStubELFInfo();
            ASSERT(asmStubELFInfo.size() > 0);
            uint32_t asmStubSymTabNum = asmStubELFInfo.size() - 1;
            std::unique_ptr<Elf64_Sym []> syms = std::make_unique<Elf64_Sym []>(asmStubSymTabNum);
            ASSERT(asmStubStrName_.size() == asmStubSymTabNum);
            for (size_t idx = 0; idx < asmStubSymTabNum; ++idx) {
                Elf64_Sym &sym = syms[idx];
                sym.setBindingAndType(llvm::ELF::STB_GLOBAL, llvm::ELF::STT_FUNC);
                sym.st_shndx = static_cast<uint16_t>(textSecIndex);
                sym.st_value = asmStubELFInfo[idx].second + asmStubOffset;
                sym.st_name = asmStubStrName_[idx];
                sym.st_name += strTabSize;
                sym.st_other = llvm::ELF::STV_DEFAULT;
                sym.st_size = asmStubELFInfo[idx + 1].second - asmStubELFInfo[idx].second;
            }
            uint32_t asmStubSymTabSize = asmStubSymTabNum * sizeof(llvm::ELF::Elf64_Sym);
            file.write(reinterpret_cast<char *>(syms.get()), asmStubSymTabSize);
            curInfo.symtabSize += asmStubSymTabSize;
            curSecOffset += asmStubSymTabSize;
        }
    }
}

void ElfBuilder::MergeArkStackMapSections(std::ofstream &file,
                                          std::vector<ModuleSectionDes::ModuleRegionInfo> &moduleInfo,
                                          llvm::ELF::Elf64_Off &curSecOffset)
{
    for (size_t i = 0; i < des_.size(); ++i) {
        ModuleSectionDes &des = des_[i];
        ModuleSectionDes::ModuleRegionInfo &curInfo = moduleInfo[i];
        uint32_t curSecSize = des.GetSecSize(ElfSecName::ARK_STACKMAP);
        uint64_t curSecAddr = des.GetSecAddr(ElfSecName::ARK_STACKMAP);
        uint32_t index = des.GetStartIndex();
        uint32_t cnt = des.GetFuncCount();
        curInfo.startIndex = index;
        curInfo.funcCount = cnt;
        curInfo.stackMapSize = curSecSize;
        file.write(reinterpret_cast<char *>(curSecAddr), curSecSize);
        curSecOffset += curSecSize;
    }
}

void ElfBuilder::FixSymtab(llvm::ELF::Elf64_Shdr* shdr)
{
    using Elf64_Sym = llvm::ELF::Elf64_Sym;
    ASSERT(stubTextOffset_.size() == des_.size());

    uint32_t secNum = static_cast<uint32_t>(GetSecNum());
    uint32_t shStrTabIndex = GetShIndex(ElfSecName::SHSTRTAB);
    uint32_t strTabIndex = GetShIndex(ElfSecName::STRTAB);
    uint32_t textSecIndex = GetShIndex(ElfSecName::TEXT);

    uint32_t strTabSize = 0;
    int firstGlobal = -1;
    uint32_t count = 0;

    for (size_t idx = 0; idx < des_.size(); ++idx) {
        uint32_t secSize = des_[idx].GetSecSize(ElfSecName::SYMTAB);
        uint64_t secAddr = des_[idx].GetSecAddr(ElfSecName::SYMTAB);
        Elf64_Sym *syms = reinterpret_cast<Elf64_Sym*>(secAddr);
        size_t n = secSize / sizeof(Elf64_Sym);
        for (size_t i = 0; i < n; ++i) {
            Elf64_Sym* sy = &syms[i];
            if (sy->getBinding() == llvm::ELF::STB_GLOBAL && firstGlobal == -1) {
                firstGlobal = static_cast<int>(count);
            }
            if (sy->getType() == llvm::ELF::STT_SECTION) {
                sy->st_shndx = static_cast<uint16_t>(shStrTabIndex);
            } else if (sy->getType() == llvm::ELF::STT_FUNC) {
                sy->st_shndx = static_cast<uint16_t>(textSecIndex);
                sy->st_value += stubTextOffset_[idx];
            }
            if (sy->st_shndx > secNum) {
                sy->st_shndx = 0;
            }
            sy->st_name += strTabSize;
            count++;
        }
        strTabSize += des_[idx].GetSecSize(ElfSecName::STRTAB);
    }
    shdr->sh_info = static_cast<uint32_t>(firstGlobal);
    shdr->sh_link = strTabIndex;
}

/*

section of aot.an layout as follows:
There are 7 section headers, starting at offset 0x40:

Section Headers:
  [Nr] Name              Type             Address           Offset    Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000  0000000000000000  0000000000000000           0     0     0
  [ 1] .text             PROGBITS         0000000000001000  00001000  0000000000000f61  0000000000000000  AX       0     0     16
  [ 2] .strtab           STRTAB           0000000000002000  00002000  0000000000000187  0000000000000000   A       0     0     1
  [ 3] .symtab           SYMTAB           0000000000002188  00002188  00000000000001c8  0000000000000018   A       1     0     8
  [ 4] .shstrtab         STRTAB           0000000000002350  00002350  000000000000003f  0000000000000000   A       0     0     8
  [ 5] .ark_funcentry    PROGBITS         0000000000002390  00002390  00000000000006c0  0000000000000000   A       0     0     8
  [ 6] .ark_stackmaps    PROGBITS         0000000000002a50  00002a50  000000000000070e  0000000000000000   A       0     0     8

section of stub.an layout as follows:
There are 7 section headers, starting at offset 0x40:

  [Nr] Name              Type             Address           Offset    Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000  0000000000000000  0000000000000000           0     0     0
  [ 1] .text             PROGBITS         0000000000001000  00001000  000000000008225e  0000000000000000  AX       0     0     16
  [ 2] .ark_asmstub      PROGBITS         0000000000083260  00083260  0000000000002dc0  0000000000000000  AX       0     0     8
  [ 3] .shstrtab         STRTAB           0000000000087000  00087000  000000000000004c  0000000000000000   A       0     0     8
  [ 4] .ark_funcentry    PROGBITS         0000000000087050  00087050  0000000000023ca0  0000000000000000   A       0     0     8
  [ 5] .ark_stackmaps    PROGBITS         00000000000aacf0  000aacf0  0000000000011e90  0000000000000000   A       0     0     8
  [ 6] .ark_moduleinfo   PROGBITS         00000000000bcb80  000bcb80  000000000000003c  0000000000000000   A       0     0     8

Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), l (large), p (processor specific)
*/
void ElfBuilder::PackELFSections(std::ofstream &file)
{
    uint32_t moduleNum = des_.size();
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetFullSecInfo();
    uint32_t secNum = sections.size() + 1; // 1 : section id = 0 is null section
    std::unique_ptr<llvm::ELF::Elf64_Shdr []> shdr;
    AllocateShdr(shdr, secNum);
    std::vector<ModuleSectionDes::ModuleRegionInfo> moduleInfo(moduleNum);
    llvm::ELF::Elf64_Off curSecOffset = ComputeEndAddrOfShdr(secNum);
    file.seekp(curSecOffset);

    int i = static_cast<int>(GetShIndex(ElfSecName::TEXT));
    auto shStrTab = FindShStrTab();

    for (auto const &[secName, secInfo] : sections) {
        auto &curShdr = shdr[i];
        ElfSection section = ElfSection(secName);
        if (!section.ShouldDumpToAOTFile()) {
            continue;
        }
        curShdr.sh_addralign = sectionToAlign_[secName];
        curSecOffset = AlignUp(curSecOffset, curShdr.sh_addralign);
        file.seekp(curSecOffset);
        ElfSecName segName = GetSegmentName(secName);
        segments_.insert(segName);
        std::string secNameStr = ModuleSectionDes::GetSecName(secName);
        // text section address needs 16 bytes alignment
        if (secName == ElfSecName::TEXT) {
            curSecOffset = AlignUp(curSecOffset, AOTFileInfo::PAGE_ALIGN);
            file.seekp(curSecOffset);
        }
        llvm::ELF::Elf64_Word shName = FindShName(secNameStr, shStrTab.first, shStrTab.second);
        ASSERT(shName != static_cast<llvm::ELF::Elf64_Word>(-1));
        curShdr.sh_name = shName;
        curShdr.sh_type = section.Type();
        curShdr.sh_flags = section.Flag();
        curShdr.sh_addr = curSecOffset;
        curShdr.sh_offset = static_cast<uint64_t>(curSecOffset);
        curShdr.sh_info = 0;
        curShdr.sh_link = static_cast<uint32_t>(section.Link());
        sectionToFileOffset_[secName] = static_cast<uintptr_t>(file.tellp());
        switch (secName) {
            case ElfSecName::ARK_MODULEINFO: {
                uint32_t curSecSize = sizeof(ModuleSectionDes::ModuleRegionInfo) * moduleInfo.size();
                file.write(reinterpret_cast<char *>(moduleInfo.data()), curSecSize);
                curSecOffset += curSecSize;
                curShdr.sh_size = curSecSize;
                break;
            }
            case ElfSecName::TEXT: {
                uint32_t curSize = curSecOffset;
                MergeTextSections(file, moduleInfo, curSecOffset);
                curShdr.sh_size = curSecOffset - curSize;
                break;
            }
            case ElfSecName::ARK_STACKMAP: {
                uint32_t curSize = curSecOffset;
                MergeArkStackMapSections(file, moduleInfo, curSecOffset);
                curShdr.sh_size = curSecOffset - curSize;
                break;
            }
            case ElfSecName::STRTAB: {
                uint32_t curSize = curSecOffset;
                MergeStrtabSections(file, moduleInfo, curSecOffset);
                curShdr.sh_size = curSecOffset - curSize;
                break;
            }
            case ElfSecName::SYMTAB: {
                FixSymtab(&curShdr);
                uint32_t curSize = curSecOffset;
                uint32_t asmSecIndex = GetShIndex(ElfSecName::ARK_ASMSTUB);
                MergeSymtabSections(file, moduleInfo, curSecOffset, shdr[asmSecIndex].sh_offset);
                curShdr.sh_size = curSecOffset - curSize;
                break;
            }
            case ElfSecName::SHSTRTAB:
            case ElfSecName::ARK_FUNCENTRY:
            case ElfSecName::ARK_ASMSTUB: {
                uint32_t curSecSize = des_[FullSecIndex].GetSecSize(secName);
                uint64_t curSecAddr = des_[FullSecIndex].GetSecAddr(secName);
                file.write(reinterpret_cast<char *>(curSecAddr), curSecSize);
                curSecOffset += curSecSize;
                curShdr.sh_size = curSecSize;
                break;
            }
            default: {
                LOG_ECMA(FATAL) << "this section should not dump to an file";
                break;
            }
        }
        if (secName == lastDataSection || secName == lastCodeSection) {
            curSecOffset = AlignUp(curSecOffset, PageSize());
            file.seekp(curSecOffset);
        }
        curShdr.sh_entsize = static_cast<uint64_t>(section.Entsize());
        sectionToShdr_[secName] = curShdr;
        LOG_COMPILER(DEBUG) << "  shdr[i].sh_entsize " << std::hex << curShdr.sh_entsize << std::endl;
        ++i;
    }
    uint32_t secEnd = static_cast<uint32_t>(file.tellp());
    file.seekp(sizeof(llvm::ELF::Elf64_Ehdr));
    file.write(reinterpret_cast<char *>(shdr.get()), secNum * sizeof(llvm::ELF::Elf64_Shdr));
    file.seekp(secEnd);
}

unsigned ElfBuilder::GetPFlag(ElfSecName segment) const
{
    return segmentToFlag_.at(segment);
}

/*
segment layout as follows:
An Elf file
Entry point 0x0
There are 2 program headers, starting at offset 16384

Program Headers:
  Type           Offset             VirtAddr           PhysAddr           FileSiz            MemSiz              Flags  Align
  LOAD           0x0000000000001000 0x0000000000001000 0x0000000000001000 0x0000000000000f61 0x0000000000001000  R E    0x1000
  LOAD           0x0000000000002000 0x0000000000002000 0x0000000000002000 0x000000000000115e 0x0000000000002000  R      0x1000

 Section to Segment mapping:
  Segment Sections...
   00     .text
   01     .strtab .symtab .shstrtab .ark_funcentry .ark_stackmaps
------------------------------------------------------------------------------------------------------------------------------
Stub Elf file
Entry point 0x0
There are 2 program headers, starting at offset 770048

Program Headers:
  Type           Offset             VirtAddr           PhysAddr           FileSiz            MemSiz              Flags  Align
  LOAD           0x0000000000001000 0x0000000000001000 0x0000000000001000 0x0000000000085020 0x0000000000086000  R E    0x1000
  LOAD           0x0000000000087000 0x0000000000087000 0x0000000000087000 0x0000000000035bbc 0x0000000000036000  R      0x1000

 Section to Segment mapping:
  Segment Sections...
   00     .text .ark_asmstub
   01     .shstrtab .ark_funcentry .ark_stackmaps .ark_moduleinfo
*/
void ElfBuilder::PackELFSegment(std::ofstream &file)
{
    llvm::ELF::Elf64_Off e_phoff = static_cast<uint64_t>(file.tellp());
    long phoff = (long)offsetof(struct llvm::ELF::Elf64_Ehdr, e_phoff);
    // write Elf32_Off e_phoff
    file.seekp(phoff);
    file.write(reinterpret_cast<char *>(&e_phoff), sizeof(e_phoff));
    file.seekp(static_cast<long>(e_phoff));

    int segNum = GetSegmentNum();
    auto phdrs = std::make_unique<llvm::ELF::Elf64_Phdr []>(segNum);
    std::map<ElfSecName, llvm::ELF::Elf64_Off> segmentToMaxOffset;
    std::map<ElfSecName, llvm::ELF::Elf64_Off> segmentToMaxAddress;
    std::set<ElfSecName> segments;
    // SecName -> addr & size
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetFullSecInfo();
    llvm::ELF::Elf64_Off offset = e_phoff;
    for (auto &s: sections) {
        ElfSection section = ElfSection(s.first);
        if (!section.ShouldDumpToAOTFile()) {
            continue;
        }
        auto it = sectionToSegment_.find(s.first);
        ASSERT(it != sectionToSegment_.end());
        ElfSecName segName = it->second;
        segments.insert(segName);
        if (segmentToMaxOffset.find(segName) == segmentToMaxOffset.end()) {
            segmentToMaxOffset[segName] = 0;
        }
        segmentToMaxOffset[segName] =
            std::max(segmentToMaxOffset[segName], sectionToShdr_[s.first].sh_offset + sectionToShdr_[s.first].sh_size);
        segmentToMaxAddress[segName] =
            std::max(segmentToMaxAddress[segName], sectionToShdr_[s.first].sh_addr + sectionToShdr_[s.first].sh_size);
        offset = std::min(offset, sectionToShdr_[s.first].sh_offset);
    }
    int phdrIndex = 0;
    llvm::ELF::Elf64_Addr addr = offset;
    for (auto &it: segments) {
        ElfSecName name = it;
        phdrs[phdrIndex].p_align = PageSize();
        phdrs[phdrIndex].p_type = llvm::ELF::PT_LOAD;
        phdrs[phdrIndex].p_flags = GetPFlag(name);
        offset = AlignUp(offset, PageSize());
        phdrs[phdrIndex].p_offset = offset;
        phdrs[phdrIndex].p_vaddr = addr % phdrs[phdrIndex].p_align == 0 ?
            addr : (addr / phdrs[phdrIndex].p_align + 1) * phdrs[phdrIndex].p_align;
        phdrs[phdrIndex].p_paddr = phdrs[phdrIndex].p_vaddr;

        phdrs[phdrIndex].p_filesz = segmentToMaxOffset[name] - phdrs[phdrIndex].p_offset;
        phdrs[phdrIndex].p_memsz = segmentToMaxAddress[name] - phdrs[phdrIndex].p_vaddr;
        phdrs[phdrIndex].p_memsz = AlignUp(phdrs[phdrIndex].p_memsz, PageSize());
        addr = phdrs[phdrIndex].p_vaddr + phdrs[phdrIndex].p_memsz;
        offset += phdrs[phdrIndex].p_filesz;
        ++phdrIndex;
    }
    file.write(reinterpret_cast<char *>(phdrs.get()), sizeof(llvm::ELF::Elf64_Phdr) * segNum);
}
}  // namespace panda::ecmascript
