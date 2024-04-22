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

#ifndef ECMASCRIPT_COMPILER_AOT_FILE_ELF_BUILDER_H
#define ECMASCRIPT_COMPILER_AOT_FILE_ELF_BUILDER_H

#include <map>
#include <set>
#include <utility>
#include <stdint.h>
#include <string>
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/compiler/binary_section.h"

namespace panda::ecmascript {

class ModuleSectionDes;

class ElfBuilder {
public:
    ElfBuilder(const std::vector<ModuleSectionDes> &des,
               const std::vector<ElfSecName> &sections);
    ~ElfBuilder();
    static constexpr uint32_t FuncEntryModuleDesIndex = 0;
    void PackELFHeader(llvm::ELF::Elf64_Ehdr &header, uint32_t version, Triple triple);
    void PackELFSections(std::ofstream &elfFile);
    void PackELFSegment(std::ofstream &elfFile);
    void MergeTextSections(std::ofstream &elfFile,
        std::vector<ModuleSectionDes::ModuleRegionInfo> &moduleInfo, llvm::ELF::Elf64_Off &curSecOffset);
    void MergeArkStackMapSections(std::ofstream &elfFile,
        std::vector<ModuleSectionDes::ModuleRegionInfo> &moduleInfo, llvm::ELF::Elf64_Off &curSecOffset);
    void MergeStrtabSections(std::ofstream &elfFile,
        std::vector<ModuleSectionDes::ModuleRegionInfo> &moduleInfo, llvm::ELF::Elf64_Off &curSecOffset);
    void MergeSymtabSections(std::ofstream &elfFile, std::vector<ModuleSectionDes::ModuleRegionInfo> &moduleInfo,
        llvm::ELF::Elf64_Off &curSecOffset, llvm::ELF::Elf64_Off &asmStubOffset);
    uint32_t AddAsmStubStrTab(std::ofstream &elfFile,
        const std::vector<std::pair<std::string, uint32_t>> &asmStubELFInfo);
    static llvm::ELF::Elf64_Word FindShName(std::string name, uintptr_t strTabPtr, int strTabSize);
    std::map<ElfSecName, std::pair<uint64_t, uint32_t>> GetFullSecInfo() const
    {
        return des_[FullSecIndex].GetSectionsInfo();
    }
    void SetEnableSecDump(bool flag)
    {
        enableSecDump_ = flag;
    }

private:
    uint32_t GetShIndex(ElfSecName section) const;
    int GetSegmentNum() const;
    int GetSecNum() const;
    unsigned GetPFlag(ElfSecName segment) const;
    ElfSecName GetSegmentName(const ElfSecName &secName) const;
    std::pair<uint64_t, uint32_t> FindShStrTab() const;
    void AllocateShdr(std::unique_ptr<llvm::ELF::Elf64_Shdr []> &shdr, const uint32_t &secNum);
    llvm::ELF::Elf64_Off ComputeEndAddrOfShdr(const uint32_t &secNum) const;
    bool SupportELF();
    void DumpSection() const;
    void AddShStrTabSection();
    void Initialize();
    void SetLastSection();
    void RemoveNotNeedSection();
    void FixSymtab(llvm::ELF::Elf64_Shdr* shdr);

    static constexpr uint32_t ASMSTUB_MODULE_NUM = 4;
    static constexpr uint32_t ShStrTableModuleDesIndex = 0;
    static constexpr uint32_t FullSecIndex = 0;

    std::vector<ModuleSectionDes> des_ {};
    std::unique_ptr<char []> shStrTabPtr_ {nullptr};
    std::map<ElfSecName, llvm::ELF::Elf64_Shdr> sectionToShdr_;
    std::map<ElfSecName, llvm::ELF::Elf64_Xword> sectionToAlign_;
    std::map<ElfSecName, ElfSecName> sectionToSegment_;
    std::map<ElfSecName, uintptr_t> sectionToFileOffset_;
    std::map<ElfSecName, unsigned> segmentToFlag_;
    std::vector<ElfSecName> sections_ {};
    std::set<ElfSecName> segments_;
    std::vector<uint32_t> stubTextOffset_ {};
    std::vector<uint32_t> asmStubStrName_ {};
    bool enableSecDump_ {false};
    ElfSecName lastDataSection {ElfSecName::NONE};
    ElfSecName lastCodeSection {ElfSecName::NONE};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_AOT_FILE_ELF_BUILDER_H
