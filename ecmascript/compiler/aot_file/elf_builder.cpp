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

#include "ecmascript/ecma_macros.h"
#include "securec.h"

namespace panda::ecmascript {
void ElfBuilder::ModifyStrTabSection()
{
    std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = des_[0].GetSectionsInfo();
    
    uint32_t size = 1;
    for (auto &s : sections_) {
        std::string str = ModuleSectionDes::GetSecName(s);
        size = size + str.size() + 1;
    }

    strTabPtr_ = std::make_unique<char []>(size);
    char *dst = strTabPtr_.get();
    dst[0] = 0x0;
    uint32_t i = 1;
    for (auto &s: sections_) {
        std::string str = ModuleSectionDes::GetSecName(s);
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
    if (sections.find(ElfSecName::STRTAB) != sections.end()) {
        sections.erase(ElfSecName::STRTAB);
    }
    sections[ElfSecName::STRTAB] = std::make_pair(reinterpret_cast<uint64_t>(strTabPtr_.get()), size);
    if (enableSecDump_) {
        DumpSection();
    }
}

void ElfBuilder::DumpSection() const
{
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetCurrentSecInfo();
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

void ElfBuilder::AddArkStackMapSection()
{
    for (auto &des : des_) {
        std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = des.GetSectionsInfo();
        std::shared_ptr<uint8_t> ptr = des.GetArkStackMapSharePtr();
        uint64_t arkStackMapAddr = reinterpret_cast<uint64_t>(ptr.get());
        uint32_t arkStackMapSize = des.GetArkStackMapSize();
        if (arkStackMapSize > 0) {
            sections[ElfSecName::ARK_STACKMAP] = std::pair(arkStackMapAddr, arkStackMapSize);
        }
    }
}

ElfBuilder::ElfBuilder(const std::vector<ModuleSectionDes> &des,
                       const std::vector<ElfSecName> &sections): des_(des), sections_(sections)
{
    ASSERT(des_.size() == AOT_MODULE_NUM || des_.size() == ASMSTUB_MODULE_NUM);
    AddArkStackMapSection();
    ModifyStrTabSection();
    sectionToAlign_ = {
        {ElfSecName::RODATA, TEXT_SEC_ALIGN},
        {ElfSecName::RODATA_CST4, TEXT_SEC_ALIGN},
        {ElfSecName::RODATA_CST8, TEXT_SEC_ALIGN},
        {ElfSecName::RODATA_CST16, TEXT_SEC_ALIGN},
        {ElfSecName::RODATA_CST32, TEXT_SEC_ALIGN},
        {ElfSecName::TEXT, TEXT_SEC_ALIGN},
        {ElfSecName::STRTAB, 1},
        {ElfSecName::ARK_STACKMAP, DATA_SEC_ALIGN},
        {ElfSecName::ARK_FUNCENTRY, DATA_SEC_ALIGN},
        {ElfSecName::ARK_ASMSTUB, DATA_SEC_ALIGN},
        {ElfSecName::ARK_MODULEINFO, DATA_SEC_ALIGN},
    };

    sectionToSegment_ = {
        {ElfSecName::RODATA, ElfSecName::TEXT},
        {ElfSecName::RODATA_CST4, ElfSecName::TEXT},
        {ElfSecName::RODATA_CST8, ElfSecName::TEXT},
        {ElfSecName::RODATA_CST16, ElfSecName::TEXT},
        {ElfSecName::RODATA_CST32, ElfSecName::TEXT},
        {ElfSecName::TEXT, ElfSecName::TEXT},
        {ElfSecName::STRTAB, ElfSecName::DATA},
        {ElfSecName::ARK_STACKMAP, ElfSecName::DATA},
        {ElfSecName::ARK_FUNCENTRY, ElfSecName::DATA},
        {ElfSecName::ARK_ASMSTUB, ElfSecName::TEXT},
        {ElfSecName::ARK_MODULEINFO, ElfSecName::DATA},
    };

    segmentToFlag_ = {
        {ElfSecName::TEXT, llvm::ELF::PF_X | llvm::ELF::PF_R},
        {ElfSecName::DATA, llvm::ELF::PF_R},
    };

    RemoveNotNeedSection();
}

void ElfBuilder::RemoveNotNeedSection()
{
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetCurrentSecInfo();
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
    strTabPtr_ = nullptr;
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
    return -1;
}

int ElfBuilder::GetSecNum() const
{
    return sections_.size();
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
    if (memset_s(reinterpret_cast<void *>(&header), sizeof(llvm::ELF::Elf64_Ehdr), 0, sizeof(llvm::ELF::Elf64_Ehdr)) != EOK) {
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
    header.e_shnum = GetSecNum() + 1; // 1: skip null section and ark stackmap
    // section header string table index
    header.e_shstrndx = static_cast<llvm::ELF::Elf64_Half>(GetShIndex(ElfSecName::STRTAB));
    // section header stub sec info index
    header.e_flags = static_cast<llvm::ELF::Elf64_Word>(GetShIndex(ElfSecName::ARK_MODULEINFO));
    // phr
    header.e_phentsize = sizeof(llvm::ELF::Elf64_Phdr);
    header.e_phnum = GetSegmentNum();
}

int ElfBuilder::GetSegmentNum() const
{
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetCurrentSecInfo();
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

ElfSecName ElfBuilder::FindLastSection(ElfSecName segment) const
{
    ElfSecName ans = ElfSecName::NONE;
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetCurrentSecInfo();
    for (auto &s: sections) {
        ElfSection section = ElfSection(s.first);
        if (!section.ShouldDumpToAOTFile()) {
            continue;
        }
        auto it = sectionToSegment_.find(s.first);
        ASSERT(it != sectionToSegment_.end());
        ElfSecName name = it->second;
        if (name != segment) {
            continue;
        }
        ans = std::max(ans, s.first);
    }
    return ans;
}

llvm::ELF::Elf64_Word ElfBuilder::FindShName(std::string name, uintptr_t strTabPtr, int strTabSize)
{
    llvm::ELF::Elf64_Word ans = -1;
    int len = name.size();
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

std::pair<uint64_t, uint32_t> ElfBuilder::FindStrTab() const
{
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetCurrentSecInfo();
    uint64_t strTabAddr = 0;
    uint32_t strTabSize = 0;
    for (auto &s: sections) {
        uint32_t curSecSize = des_[0].GetSecSize(s.first);
        uint64_t curSecAddr = des_[0].GetSecAddr(s.first);
        if (s.first == ElfSecName::STRTAB) {
            strTabSize = curSecSize;
            strTabAddr = curSecAddr;
            break;
        }
    }
    return std::make_pair(strTabAddr, strTabSize);
}

void ElfBuilder::AllocateShdr(std::unique_ptr<llvm::ELF::Elf64_Shdr []> &shdr, const uint32_t &secNum)
{
    shdr = std::make_unique<llvm::ELF::Elf64_Shdr []>(secNum);
    if (memset_s(reinterpret_cast<void *>(&shdr[0]), sizeof(llvm::ELF::Elf64_Shdr),
        0, sizeof(llvm::ELF::Elf64_Shdr)) != EOK) {
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
        curSecOffset = AlignUp(curSecOffset, TEXT_SEC_ALIGN);
        file.seekp(curSecOffset);
        auto curModuleSec = des.GetSectionsInfo();
        if (curModuleSec.find(ElfSecName::RODATA_CST8) != curModuleSec.end()) {
            uint32_t rodataSize = des.GetSecSize(ElfSecName::RODATA_CST8);
            uint64_t rodataAddr = des.GetSecAddr(ElfSecName::RODATA_CST8);
            file.write(reinterpret_cast<char *>(rodataAddr), rodataSize);
            curInfo.rodataSize = rodataSize;
            curSecOffset += rodataSize;
        }
        curInfo.textSize = curSecSize;
        file.write(reinterpret_cast<char *>(curSecAddr), curSecSize);
        curSecOffset += curSecSize;
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

/*
section layout as follows:
There are 6 section headers, starting at offset 0x40:

Section Headers:
  [Nr] Name              Type             Address           Offset    Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000  0000000000000000  0000000000000000           0     0     0
  [ 1] .rodata.cst8      PROGBITS         0000000000001000  00001000  0000000000000020  0000000000000000  AM       0     0     8
  [ 2] .text             PROGBITS         0000000000001020  00001020  000000000000112a  0000000000000000  AX       0     0     16
  [ 3] .strtab           STRTAB           0000000000003000  00003000  000000000000003a  0000000000000000   A       0     0     1
  [ 4] .ark_funcentry    PROGBITS         0000000000003040  00003040  00000000000006c0  0000000000000000   A       0     0     8
  [ 5] .ark_stackmaps    PROGBITS         0000000000003700  00003700  0000000000000754  0000000000000000   A       0     0     8
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), l (large), p (processor specific)
*/
void ElfBuilder::PackAnELFSections(std::ofstream &file)
{
    ASSERT(des_.size() == AOT_MODULE_NUM);
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetCurrentSecInfo();
    uint32_t secNum = sections.size() + 1; // 1 : section id = 0 is null section
    std::unique_ptr<llvm::ELF::Elf64_Shdr []> shdr;
    AllocateShdr(shdr, secNum);

    llvm::ELF::Elf64_Off curSecOffset = ComputeEndAddrOfShdr(secNum);
    file.seekp(curSecOffset);

    int i = 1; // 1: skip null section
    auto strTab = FindStrTab();

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
        uint32_t curSecSize = GetCurrentDes().GetSecSize(secName);
        uint64_t curSecAddr = GetCurrentDes().GetSecAddr(secName);
        std::string secNameStr = ModuleSectionDes::GetSecName(secName);
        sectionToFileOffset_[secName] = static_cast<uint8_t>(file.tellp());
        file.write(reinterpret_cast<char *>(curSecAddr), curSecSize);
        llvm::ELF::Elf64_Word shName = FindShName(secNameStr, strTab.first, strTab.second);
        ASSERT(shName != static_cast<llvm::ELF::Elf64_Word>(-1));
        curShdr.sh_name = shName;
        curShdr.sh_type = section.Type();
        curShdr.sh_flags = section.Flag();
        curShdr.sh_addr = curSecOffset;
        curShdr.sh_offset = curSecOffset;
        LOG_COMPILER(DEBUG) << "curSecOffset:   0x" << std::hex << curSecOffset << "  curSecSize:0x" << curSecSize;
        curSecOffset += curSecSize;
        ElfSecName lastSecName = FindLastSection(segName);
        if (secName == lastSecName) {
            curSecOffset = AlignUp(curSecOffset, PageSize());
            file.seekp(curSecOffset);
        }
        curShdr.sh_size = curSecSize;
        curShdr.sh_link = section.Link();
        curShdr.sh_info = 0;
        curShdr.sh_entsize = section.Entsize();
        sectionToShdr_[secName] = curShdr;
        LOG_COMPILER(DEBUG) << "  shdr[i].sh_entsize " << std::hex << curShdr.sh_entsize << std::endl;
        ++i;
    }
    uint32_t secEnd = static_cast<uint32_t>(file.tellp());
    file.seekp(sizeof(llvm::ELF::Elf64_Ehdr));
    file.write(reinterpret_cast<char *>(shdr.get()), secNum * sizeof(llvm::ELF::Elf64_Shdr));
    file.seekp(secEnd);
}

/*
section layout as follows:
There are 7 section headers, starting at offset 0x40:

Section Headers:
  [Nr] Name              Type             Address           Offset    Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000  0000000000000000  0000000000000000           0     0     0
  [ 1] .text             PROGBITS         0000000000001000  00001000  000000000008148e  0000000000000000  AX       0     0     16
  [ 2] .ark_asmstub      PROGBITS         0000000000082490  00082490  0000000000002dc0  0000000000000000  AX       0     0     8
  [ 3] .strtab           STRTAB           0000000000086000  00086000  0000000000000048  0000000000000000   A       0     0     1
  [ 4] .ark_funcentry    PROGBITS         0000000000086048  00086048  0000000000023ca0  0000000000000000   A       0     0     8
  [ 5] .ark_stackmaps    PROGBITS         00000000000a9ce8  000a9ce8  00000000000119cc  0000000000000000   A       0     0     8
  [ 6] .ark_moduleinfo   PROGBITS         00000000000bb6b8  000bb6b8  000000000000003c  0000000000000000   A       0     0     8
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), l (large), p (processor specific)
*/
void ElfBuilder::PackStubELFSections(std::ofstream &file)
{
    ASSERT(des_.size() == ASMSTUB_MODULE_NUM);
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetCurrentSecInfo();
    uint32_t secNum = sections.size() + 1; // 1 : section id = 0 is null section
    std::unique_ptr<llvm::ELF::Elf64_Shdr []> shdr;
    AllocateShdr(shdr, secNum);
    std::vector<ModuleSectionDes::ModuleRegionInfo> moduleInfo(ASMSTUB_MODULE_NUM);
    llvm::ELF::Elf64_Off curSecOffset = ComputeEndAddrOfShdr(secNum);
    file.seekp(curSecOffset);

    int i = 1; // 1: skip null section
    auto strTab = FindStrTab();

    for (auto const &[secName, secInfo] : sections) {
        auto &curShdr = shdr[i];
        ElfSection section = ElfSection(secName);
        if (!section.ShouldDumpToStubFile()) {
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
            curSecOffset = AlignUp(curSecOffset, TEXT_SEC_ALIGN);
            file.seekp(curSecOffset);
        }
        llvm::ELF::Elf64_Word shName = FindShName(secNameStr, strTab.first, strTab.second);
        ASSERT(shName != static_cast<llvm::ELF::Elf64_Word>(-1));
        curShdr.sh_name = shName;
        curShdr.sh_type = section.Type();
        curShdr.sh_flags = section.Flag();
        curShdr.sh_addr = curSecOffset;
        curShdr.sh_offset = curSecOffset;
        sectionToFileOffset_[secName] = file.tellp();
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
            case ElfSecName::STRTAB:
            case ElfSecName::ARK_FUNCENTRY:
            case ElfSecName::ARK_ASMSTUB: {
                uint32_t curSecSize = GetCurrentDes().GetSecSize(secName);
                uint64_t curSecAddr = GetCurrentDes().GetSecAddr(secName);
                file.write(reinterpret_cast<char *>(curSecAddr), curSecSize);
                curSecOffset += curSecSize;
                curShdr.sh_size = curSecSize;
                break;
            }
            default: {
                LOG_ECMA(FATAL) << "this section should not dump to stub file";
                break;
            }
        }
        ElfSecName lastSecName = FindLastSection(segName);
        if (secName == lastSecName) {
            curSecOffset = AlignUp(curSecOffset, PageSize());
            file.seekp(curSecOffset);
        }
        curShdr.sh_link = section.Link();
        curShdr.sh_info = 0;
        curShdr.sh_entsize = section.Entsize();
        sectionToShdr_[secName] = curShdr;
        LOG_COMPILER(DEBUG) << "  shdr[i].sh_entsize " << std::hex << curShdr.sh_entsize << std::endl;
        ++i;
    }
    uint32_t secEnd = file.tellp();
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
  LOAD           0x0000000000001000 0x0000000000001000 0x0000000000001000 0x000000000000114a 0x0000000000002000  R E    0x1000
  LOAD           0x0000000000003000 0x0000000000003000 0x0000000000003000 0x0000000000000e54 0x0000000000001000  R      0x1000

 Section to Segment mapping:
  Segment Sections...
   00     .rodata.cst8 .text
   01     .strtab .ark_funcentry .ark_stackmaps
------------------------------------------------------------------------------------------------------------------------------
Stub Elf file
Entry point 0x0
There are 2 program headers, starting at offset 770048

Program Headers:
  Type           Offset             VirtAddr           PhysAddr           FileSiz            MemSiz              Flags  Align
  LOAD           0x0000000000000000 0x0000000000000000 0x0000000000000000 0x0000000000085250 0x0000000000086000  R E    0x1000
  LOAD           0x0000000000086000 0x0000000000086000 0x0000000000086000 0x00000000000356f4 0x0000000000036000  R      0x1000

 Section to Segment mapping:
  Segment Sections...
   00     .text .ark_asmstub
   01     .strtab .ark_funcentry .ark_stackmaps .ark_moduleinfo
*/
void ElfBuilder::PackELFSegment(std::ofstream &file)
{
    llvm::ELF::Elf64_Off e_phoff = static_cast<uint64_t>(file.tellp());
    long phoff = (long)offsetof(struct llvm::ELF::Elf64_Ehdr, e_phoff);
    // write Elf32_Off e_phoff
    file.seekp(phoff);
    file.write(reinterpret_cast<char *>(&e_phoff), sizeof(e_phoff));
    file.seekp(e_phoff);

    int segNum = GetSegmentNum();
    auto phdrs = std::make_unique<llvm::ELF::Elf64_Phdr []>(segNum);
    std::map<ElfSecName, llvm::ELF::Elf64_Off> segmentToMaxOffset;
    std::map<ElfSecName, llvm::ELF::Elf64_Off> segmentToMaxAddress;
    std::set<ElfSecName> segments;
    // SecName -> addr & size
    const std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections = GetCurrentSecInfo();
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
