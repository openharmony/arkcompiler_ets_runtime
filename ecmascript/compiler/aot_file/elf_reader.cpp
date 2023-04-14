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

#include "ecmascript/ecma_macros.h"
#include "securec.h"

namespace panda::ecmascript {
bool ElfReader::VerifyELFHeader(uint32_t version)
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
    if (header.e_version > version) {
        LOG_ECMA(ERROR) << "Elf format error, expected version should be less or equal than "
                        << version << ", but got " << header.e_version;
        return false;
    }
    return true;
}

void ElfReader::ParseELFSections(ModuleSectionDes &des, std::vector<ElfSecName> &secs)
{
    llvm::ELF::Elf64_Ehdr *ehdr = reinterpret_cast<llvm::ELF::Elf64_Ehdr *>(fileMapMem_.GetOriginAddr());
    char *addr = reinterpret_cast<char *>(ehdr);
    llvm::ELF::Elf64_Shdr *shdr = reinterpret_cast<llvm::ELF::Elf64_Shdr *>(addr + ehdr->e_shoff);
    llvm::ELF::Elf64_Shdr strdr = shdr[ehdr->e_shstrndx];
    for (size_t j = 0; j < secs.size(); ++j) {
        int secId = -1;
        ElfSecName sec = secs[j];
        std::string sectionName = ModuleSectionDes::GetSecName(sec);
        for (size_t i = 0; i < ehdr->e_shnum; ++i) {
            llvm::ELF::Elf64_Word shName = shdr[i].sh_name;
            char *curShName = reinterpret_cast<char *>(addr) + shName + strdr.sh_offset;
            if (sectionName.compare(curShName) == 0) {
                secId = i;
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
            des.SetSecAddr(secAddr, sec);
            des.SetSecSize(secSize, sec);
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
            PageProtect(virtualAddr, phdr[i].p_memsz, PAGE_PROT_EXEC_READ);
        }
    }
    return true;
}
}  // namespace panda::ecmascript