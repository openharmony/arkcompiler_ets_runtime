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

#include "ecmascript/compiler/aot_file/elf_reader.h"

namespace panda::ecmascript {
bool ElfReader::VerifyELFHeader(uint32_t version, bool strictMatch)
{
    llvm::ELF::Elf64_Ehdr header = *(reinterpret_cast<llvm::ELF::Elf64_Ehdr *>(fileMapMem_.GetOriginAddr()));
    if (header.e_ident[llvm::ELF::EI_MAG0] != llvm::ELF::ElfMagic[llvm::ELF::EI_MAG0]
        || header.e_ident[llvm::ELF::EI_MAG1] != llvm::ELF::ElfMagic[llvm::ELF::EI_MAG1]
        || header.e_ident[llvm::ELF::EI_MAG2] != llvm::ELF::ElfMagic[llvm::ELF::EI_MAG2]
        || header.e_ident[llvm::ELF::EI_MAG3] != llvm::ELF::ElfMagic[llvm::ELF::EI_MAG3]) {
        LOG_ECMA(ERROR) << "ELF format error, expected magic is " << llvm::ELF::ElfMagic
                        << ", but got " << header.e_ident[llvm::ELF::EI_MAG0] << header.e_ident[llvm::ELF::EI_MAG1]
                        << header.e_ident[llvm::ELF::EI_MAG2] << header.e_ident[llvm::ELF::EI_MAG3];
        return false;
    }
    if (!base::FileHeaderBase::VerifyVersion("Elf", header.e_version, version, strictMatch)) {
        return false;
    }
    if (ElfChecker(fileMapMem_).CheckValidElf() == false) {
        LOG_ECMA(ERROR) << "ELF file content is not valid";
        return false;
    }
    return true;
}

ModuleSectionDes::ModuleRegionInfo *ElfReader::GetCurModuleInfo(uint32_t i, llvm::ELF::Elf64_Off offset)
{
    uint64_t codeAddress = reinterpret_cast<uint64_t>(fileMapMem_.GetOriginAddr());
    uint64_t info = codeAddress + offset + i * sizeof(ModuleSectionDes::ModuleRegionInfo);
    return reinterpret_cast<ModuleSectionDes::ModuleRegionInfo *>(info);
}

void ElfReader::ParseELFSections(ModuleSectionDes &des, std::vector<ElfSecName> &secs)
{
    llvm::ELF::Elf64_Ehdr *ehdr = reinterpret_cast<llvm::ELF::Elf64_Ehdr *>(fileMapMem_.GetOriginAddr());
    char *addr = reinterpret_cast<char *>(ehdr);
    llvm::ELF::Elf64_Shdr *shdr = reinterpret_cast<llvm::ELF::Elf64_Shdr *>(addr + ehdr->e_shoff);
    ASSERT(ehdr->e_shstrndx != static_cast<llvm::ELF::Elf64_Half>(-1));
    llvm::ELF::Elf64_Shdr strdr = shdr[ehdr->e_shstrndx];
    for (size_t j = 0; j < secs.size(); ++j) {
        int secId = -1;
        ElfSecName sec = secs[j];
        std::string sectionName = ModuleSectionDes::GetSecName(sec);
        for (size_t i = 0; i < ehdr->e_shnum; ++i) {
            llvm::ELF::Elf64_Word shName = shdr[i].sh_name;
            char *curShName = reinterpret_cast<char *>(addr) + shName + strdr.sh_offset;
            if (sectionName.compare(curShName) == 0) {
                secId = static_cast<int>(i);
                break;
            }
        }
        if (secId == -1) {
            LOG_COMPILER(DEBUG) << "sectionName: " << sectionName << " not found in strtab";
            continue;
        }
        ASSERT(secId > 0 && secId < ehdr->e_shnum);
        llvm::ELF::Elf64_Shdr secShdr = shdr[secId];
        uintptr_t secAddr = reinterpret_cast<uintptr_t>(addr + secShdr.sh_offset);
        uint32_t secSize = secShdr.sh_size;
        if (sec == ElfSecName::ARK_FUNCENTRY) {
            ASSERT((secSize > 0) && (secSize % sizeof(AOTFileInfo::FuncEntryDes) == 0));
        }
        if (sec == ElfSecName::ARK_STACKMAP) {
            des.SetArkStackMapPtr(reinterpret_cast<uint8_t *>(secAddr));
            des.SetArkStackMapSize(secSize);
        } else {
            des.SetSecAddrAndSize(sec, secAddr, secSize);
        }
    }
}

void ElfReader::ParseELFSections(std::vector<ModuleSectionDes> &des, std::vector<ElfSecName> &secs)
{
    llvm::ELF::Elf64_Ehdr *ehdr = reinterpret_cast<llvm::ELF::Elf64_Ehdr *>(fileMapMem_.GetOriginAddr());
    char *addr = reinterpret_cast<char *>(ehdr);
    llvm::ELF::Elf64_Shdr *shdrs = reinterpret_cast<llvm::ELF::Elf64_Shdr *>(addr + ehdr->e_shoff);
    ASSERT(ehdr->e_shstrndx != static_cast<llvm::ELF::Elf64_Half>(-1));
    llvm::ELF::Elf64_Shdr strdr = shdrs[ehdr->e_shstrndx];
    ASSERT(ehdr->e_flags != static_cast<llvm::ELF::Elf64_Word>(-1));
    llvm::ELF::Elf64_Shdr moduledr = shdrs[ehdr->e_flags];
    size_t moduleInfoSize = moduledr.sh_size;
    uint32_t moduleNum = GetModuleNum(moduleInfoSize);
    des.resize(moduleNum);
    std::set<ElfSecName> secSet(secs.begin(), secs.end());
    for (ElfSecName sec : secSet) {
        int secId = -1;
        std::string sectionName = ModuleSectionDes::GetSecName(sec);
        for (size_t i = 0; i < ehdr->e_shnum; ++i) {
            llvm::ELF::Elf64_Word shName = shdrs[i].sh_name;
            char *curShName = reinterpret_cast<char *>(addr) + shName + strdr.sh_offset;
            if (sectionName.compare(curShName) == 0) {
                secId = static_cast<int>(i);
                break;
            }
        }
        if (secId == -1) {
            LOG_COMPILER(DEBUG) << "sectionName: " << sectionName << " not found in strtab";
            continue;
        }
        ASSERT(secId > 0 && secId < ehdr->e_shnum);
        llvm::ELF::Elf64_Shdr secShdr = shdrs[secId];
        uintptr_t secAddr = reinterpret_cast<uintptr_t>(addr + secShdr.sh_offset);
        uint32_t secSize = secShdr.sh_size;
        switch (sec) {
            case ElfSecName::TEXT: {
                llvm::ELF::Elf64_Off secOffset = 0;
                SeparateTextSections(des, secAddr, secOffset, moduledr.sh_offset);
                ASSERT(static_cast<uint32_t>(secOffset) == secSize);
                break;
            }
            case ElfSecName::ARK_STACKMAP: {
                llvm::ELF::Elf64_Off secOffset = 0;
                SeparateArkStackMapSections(des, secAddr, secOffset, moduledr.sh_offset);
                ASSERT(static_cast<uint32_t>(secOffset) == secSize);
                break;
            }
            case ElfSecName::STRTAB: {
                llvm::ELF::Elf64_Off secOffset = 0;
                SeparateStrtabSections(des, secAddr, secOffset, moduledr.sh_offset);
                ASSERT(static_cast<uint32_t>(secOffset) == secSize);
                break;
            }
            case ElfSecName::SYMTAB: {
                llvm::ELF::Elf64_Off secOffset = 0;
                SeparateSymtabSections(des, secAddr, secOffset, moduledr.sh_offset);
                ASSERT(static_cast<uint32_t>(secOffset) == secSize);
                break;
            }
            case ElfSecName::SHSTRTAB:
            case ElfSecName::ARK_FUNCENTRY:
            case ElfSecName::ARK_ASMSTUB:
            case ElfSecName::ARK_MODULEINFO: {
                if (sec == ElfSecName::ARK_FUNCENTRY) {
                    ASSERT((secSize > 0) && (secSize % sizeof(AOTFileInfo::FuncEntryDes) == 0));
                }
                des[0].SetSecAddrAndSize(sec, secAddr, secSize);
                break;
            }
            default: {
                LOG_ECMA(FATAL) << "this section should not dump to stub file";
                break;
            }
        }
    }
}

void ElfReader::ParseELFSections(BinaryBufferParser &parser,
                                 std::vector<ModuleSectionDes> &des,
                                 std::vector<ElfSecName> &secs)
{
    ASSERT(des.size() == ASMSTUB_MODULE_NUM);
    uint64_t codeAddress = reinterpret_cast<uint64_t>(stubsMem_.addr_);
    llvm::ELF::Elf64_Ehdr ehdr;
    parser.ParseBuffer(&ehdr, sizeof(ehdr), 0);
    std::vector<llvm::ELF::Elf64_Shdr> shdrs(ehdr.e_shnum);
    parser.ParseBuffer(shdrs.data(), sizeof(llvm::ELF::Elf64_Shdr) * ehdr.e_shnum, ehdr.e_shoff);

    ASSERT(ehdr.e_shstrndx != static_cast<llvm::ELF::Elf64_Half>(-1));
    llvm::ELF::Elf64_Shdr strdr = shdrs[ehdr.e_shstrndx];
    ASSERT(ehdr.e_flags != static_cast<llvm::ELF::Elf64_Word>(-1));
    llvm::ELF::Elf64_Shdr moduledr = shdrs[ehdr.e_flags];
    [[maybe_unused]] size_t moduleInfoSize = moduledr.sh_size;
    uint32_t moduleNum = GetModuleNum(moduleInfoSize);
    ASSERT(moduleNum == ASMSTUB_MODULE_NUM);
    moduleInfo_.resize(moduleNum);
    parser.ParseBuffer(moduleInfo_.data(), moduleInfoSize, moduledr.sh_offset);
    std::set<ElfSecName> secSet(secs.begin(), secs.end());
    for (ElfSecName sec : secSet) {
        int secId = -1;
        std::string sectionName = ModuleSectionDes::GetSecName(sec);
        for (size_t i = 0; i < ehdr.e_shnum; ++i) {
            llvm::ELF::Elf64_Word shName = shdrs[i].sh_name;
            char *curShName = reinterpret_cast<char *>(parser.GetAddr()) + shName + strdr.sh_offset;
            if (sectionName.compare(curShName) == 0) {
                secId = static_cast<int>(i);
                break;
            }
        }
        if (secId == -1) {
            LOG_COMPILER(DEBUG) << "sectionName: " << sectionName << " not found in strtab";
            continue;
        }
        ASSERT(secId > 0 && secId < ehdr.e_shnum);
        llvm::ELF::Elf64_Shdr secShdr = shdrs[secId];
        uint64_t secAddr = static_cast<uint64_t>(codeAddress + secShdr.sh_offset);
        uint32_t secSize = secShdr.sh_size;
        switch (sec) {
            case ElfSecName::TEXT: {
                llvm::ELF::Elf64_Off secOffset = 0;
                SeparateTextSections(parser, des, secAddr, secOffset, secShdr.sh_offset);
                ASSERT(static_cast<uint32_t>(secOffset) == secSize);
                break;
            }
            case ElfSecName::ARK_STACKMAP: {
                llvm::ELF::Elf64_Off secOffset = 0;
                SeparateArkStackMapSections(parser, des, secAddr, secOffset, secShdr.sh_offset);
                ASSERT(static_cast<uint32_t>(secOffset) == secSize);
                break;
            }
            case ElfSecName::STRTAB: {
                llvm::ELF::Elf64_Off secOffset = 0;
                SeparateStrtabSections(parser, des, secAddr, secOffset, secShdr.sh_offset);
                ASSERT(static_cast<uint32_t>(secOffset) == secSize);
                break;
            }
            case ElfSecName::SYMTAB: {
                llvm::ELF::Elf64_Off secOffset = 0;
                SeparateSymtabSections(parser, des, secAddr, secOffset, secShdr.sh_offset);
                ASSERT(static_cast<uint32_t>(secOffset) == secSize);
                break;
            }
            case ElfSecName::SHSTRTAB:
            case ElfSecName::ARK_FUNCENTRY:
            case ElfSecName::ARK_ASMSTUB:
            case ElfSecName::ARK_MODULEINFO: {
                if (sec == ElfSecName::ARK_FUNCENTRY) {
                    ASSERT((secSize > 0) && (secSize % sizeof(AOTFileInfo::FuncEntryDes) == 0));
                }
                parser.ParseBuffer(reinterpret_cast<void *>(secAddr), secSize, secShdr.sh_offset);
                des[0].SetSecAddrAndSize(sec, secAddr, secSize);
                break;
            }
            default: {
                LOG_ECMA(FATAL) << "this section should not dump to stub file";
                break;
            }
        }
    }
}

bool ElfReader::ParseELFSegment()
{
    if (fileMapMem_.GetOriginAddr() == nullptr) {
        return false;
    }
    char *addr = reinterpret_cast<char *>(fileMapMem_.GetOriginAddr());
    llvm::ELF::Elf64_Ehdr *ehdr = reinterpret_cast<llvm::ELF::Elf64_Ehdr *>(fileMapMem_.GetOriginAddr());
    llvm::ELF::Elf64_Phdr *phdr = reinterpret_cast<llvm::ELF::Elf64_Phdr *>(addr + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        if (phdr[i].p_type != llvm::ELF::PT_LOAD) {
            continue;
        }
        if (phdr[i].p_filesz > phdr[i].p_memsz) {
            LOG_COMPILER(ERROR) << " p_filesz:0x" << std::hex << phdr[i].p_filesz << " > p_memsz:0x"
                << phdr[i].p_memsz;
            return false;
        }
        if (!phdr[i].p_filesz) {
            continue;
        }
        unsigned char *virtualAddr = reinterpret_cast<unsigned char *>(addr + phdr[i].p_vaddr);
        ASSERT(phdr[i].p_offset % PageSize() == 0);
        if ((phdr[i].p_flags & llvm::ELF::PF_X) != 0) {
            ASSERT(reinterpret_cast<uintptr_t>(virtualAddr) % PageSize() == 0);
            if (!PageProtect(virtualAddr, phdr[i].p_memsz, PAGE_PROT_EXEC_READ)) {
                return false;
            }
        }
    }
    return true;
}

void ElfReader::SeparateTextSections(std::vector<ModuleSectionDes> &des,
                                     const uintptr_t &secAddr,
                                     llvm::ELF::Elf64_Off &secOffset,
                                     const llvm::ELF::Elf64_Off &moduleInfoOffset)
{
    for (size_t i = 0; i < des.size(); ++i) {
        auto moduleInfo = GetCurModuleInfo(i, moduleInfoOffset);
        secOffset = AlignUp(secOffset, AOTFileInfo::PAGE_ALIGN);
        uint32_t rodataSizeBeforeText = moduleInfo->rodataSizeBeforeText;
        uint32_t rodataSizeAfterText = moduleInfo->rodataSizeAfterText;
        if (rodataSizeBeforeText != 0) {
            secOffset += rodataSizeBeforeText;
            secOffset = AlignUp(secOffset, AOTFileInfo::TEXT_SEC_ALIGN);
        }
        uint32_t textSize = moduleInfo->textSize;
        des[i].SetSecAddrAndSize(ElfSecName::TEXT, secAddr + secOffset, textSize);
        secOffset += textSize;
        if (rodataSizeAfterText != 0) {
            secOffset = AlignUp(secOffset, AOTFileInfo::RODATA_SEC_ALIGN);
            secOffset += rodataSizeAfterText;
        }
    }
}

void ElfReader::SeparateArkStackMapSections(std::vector<ModuleSectionDes> &des,
                                            const uintptr_t &secAddr,
                                            llvm::ELF::Elf64_Off &secOffset,
                                            const llvm::ELF::Elf64_Off &moduleInfoOffset)
{
    for (size_t i = 0; i < des.size(); ++i) {
        auto moduleInfo = GetCurModuleInfo(i, moduleInfoOffset);
        uint32_t stackMapSize = moduleInfo->stackMapSize;
        des[i].SetArkStackMapPtr(reinterpret_cast<uint8_t *>(secAddr + secOffset));
        des[i].SetArkStackMapSize(stackMapSize);
        uint32_t index = moduleInfo->startIndex;
        uint32_t cnt = moduleInfo->funcCount;
        des[i].SetStartIndex(index);
        des[i].SetFuncCount(cnt);
        secOffset += stackMapSize;
    }
}

void ElfReader::SeparateStrtabSections(std::vector<ModuleSectionDes> &des,
                                       const uintptr_t &secAddr,
                                       llvm::ELF::Elf64_Off &secOffset,
                                       const llvm::ELF::Elf64_Off &moduleInfoOffset)
{
    for (size_t i = 0; i < des.size(); ++i) {
        auto moduleInfo = GetCurModuleInfo(i, moduleInfoOffset);
        uint32_t strtabSize = moduleInfo->strtabSize;
        des[i].SetSecAddrAndSize(ElfSecName::STRTAB, secAddr + secOffset, strtabSize);
        secOffset += strtabSize;
    }
}

void ElfReader::SeparateSymtabSections(std::vector<ModuleSectionDes> &des,
                                       const uintptr_t &secAddr,
                                       llvm::ELF::Elf64_Off &secOffset,
                                       const llvm::ELF::Elf64_Off &moduleInfoOffset)
{
    for (size_t i = 0; i < des.size(); ++i) {
        auto moduleInfo = GetCurModuleInfo(i, moduleInfoOffset);
        uint32_t symtabSize = moduleInfo->symtabSize;
        des[i].SetSecAddrAndSize(ElfSecName::SYMTAB, secAddr + secOffset, symtabSize);
        secOffset += symtabSize;
    }
}

void ElfReader::SeparateTextSections(BinaryBufferParser &parser,
                                     std::vector<ModuleSectionDes> &des,
                                     const uint64_t &secAddr,
                                     llvm::ELF::Elf64_Off &secOffset,
                                     const llvm::ELF::Elf64_Off &curShOffset)
{
    for (size_t i = 0; i < des.size(); ++i) {
        auto moduleInfo = moduleInfo_[i];
        secOffset = AlignUp(secOffset, AOTFileInfo::PAGE_ALIGN);
        uint32_t rodataSizeBeforeText = moduleInfo.rodataSizeBeforeText;
        uint32_t rodataSizeAfterText = moduleInfo.rodataSizeAfterText;
        if (rodataSizeBeforeText != 0) {
            parser.ParseBuffer(reinterpret_cast<void *>(secAddr + secOffset), rodataSizeBeforeText,
                               curShOffset + secOffset);
            secOffset += rodataSizeBeforeText;
            secOffset = AlignUp(secOffset, AOTFileInfo::TEXT_SEC_ALIGN);
        }
        uint32_t textSize = moduleInfo.textSize;
        parser.ParseBuffer(reinterpret_cast<void *>(secAddr + secOffset), textSize, curShOffset + secOffset);
        des[i].SetSecAddrAndSize(ElfSecName::TEXT, secAddr + secOffset, textSize);
        secOffset += textSize;
        if (rodataSizeAfterText != 0) {
            secOffset = AlignUp(secOffset, AOTFileInfo::RODATA_SEC_ALIGN);
            parser.ParseBuffer(reinterpret_cast<void *>(secAddr + secOffset), rodataSizeAfterText,
                               curShOffset + secOffset);
            secOffset += rodataSizeAfterText;
        }
    }
}

void ElfReader::SeparateArkStackMapSections(BinaryBufferParser &parser,
                                            std::vector<ModuleSectionDes> &des,
                                            const uint64_t &secAddr,
                                            llvm::ELF::Elf64_Off &secOffset,
                                            const llvm::ELF::Elf64_Off &curShOffset)
{
    for (size_t i = 0; i < des.size(); ++i) {
        auto moduleInfo = moduleInfo_[i];
        uint32_t stackMapSize = moduleInfo.stackMapSize;
        parser.ParseBuffer(reinterpret_cast<void *>(secAddr + secOffset), stackMapSize, curShOffset + secOffset);
        des[i].SetArkStackMapPtr(reinterpret_cast<uint8_t *>(secAddr + secOffset));
        des[i].SetArkStackMapSize(stackMapSize);
        uint32_t index = moduleInfo.startIndex;
        uint32_t cnt = moduleInfo.funcCount;
        des[i].SetStartIndex(index);
        des[i].SetFuncCount(cnt);
        secOffset += stackMapSize;
    }
}

void ElfReader::SeparateStrtabSections(BinaryBufferParser &parser,
                                       std::vector<ModuleSectionDes> &des,
                                       const uintptr_t &secAddr,
                                       llvm::ELF::Elf64_Off &secOffset,
                                       const llvm::ELF::Elf64_Off &curShOffset)
{
    for (size_t i = 0; i < des.size(); ++i) {
        auto moduleInfo = moduleInfo_[i];
        uint32_t strtabSize = moduleInfo.strtabSize;
        parser.ParseBuffer(reinterpret_cast<void *>(secAddr + secOffset), strtabSize, curShOffset + secOffset);
        des[i].SetSecAddrAndSize(ElfSecName::STRTAB, secAddr + secOffset, strtabSize);
        secOffset += strtabSize;
    }
}

void ElfReader::SeparateSymtabSections(BinaryBufferParser &parser,
                                       std::vector<ModuleSectionDes> &des,
                                       const uintptr_t &secAddr,
                                       llvm::ELF::Elf64_Off &secOffset,
                                       const llvm::ELF::Elf64_Off &curShOffset)
{
    for (size_t i = 0; i < des.size(); ++i) {
        auto moduleInfo = moduleInfo_[i];
        uint32_t symtabSize = moduleInfo.symtabSize;
        parser.ParseBuffer(reinterpret_cast<void *>(secAddr + secOffset), symtabSize, curShOffset + secOffset);
        des[i].SetSecAddrAndSize(ElfSecName::SYMTAB, secAddr + secOffset, symtabSize);
        secOffset += symtabSize;
    }
}
}  // namespace panda::ecmascript
