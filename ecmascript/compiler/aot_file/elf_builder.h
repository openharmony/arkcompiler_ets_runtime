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
    ElfBuilder(ModuleSectionDes sectionDes);
    ~ElfBuilder();
    void PackELFHeader(llvm::ELF::Elf64_Ehdr &header, uint32_t version, Triple triple);
    void PackELFSections(std::ofstream &elfFile);
    void PackELFSegment(std::ofstream &elfFile);
    static llvm::ELF::Elf64_Word FindShName(std::string name, uintptr_t strTabPtr, int strTabSize);
    void SetEnableSecDump(bool flag)
    {
        enableSecDump_ = flag;
    }

private:
    llvm::ELF::Elf64_Half GetShStrNdx(std::map<ElfSecName, std::pair<uint64_t, uint32_t>> &sections) const;
    llvm::ELF::Elf64_Half GetSecSize() const;
    ElfSecName FindLastSection(ElfSecName segment) const;
    int GetSegmentNum() const;
    int GetSecNum() const;
    unsigned GetPFlag(ElfSecName segment) const;
    std::pair<uint64_t, uint32_t> FindStrTab() const;
    bool SupportELF();
    void AddArkStackMapSection();
    void DumpSection() const;
    void ModifyStrTabSection();

    static constexpr uint32_t TEXT_SEC_ALIGN = 16;
    ModuleSectionDes sectionDes_;
    std::unique_ptr<char []> strTabPtr_ {nullptr};
    std::map<ElfSecName, llvm::ELF::Elf64_Shdr> sectionToShdr_;
    std::map<ElfSecName, llvm::ELF::Elf64_Xword> sectionToAlign_;
    std::map<ElfSecName, ElfSecName> sectionToSegment_;
    std::map<ElfSecName, uintptr_t> sectionToFileOffset_;
    std::map<ElfSecName, unsigned> segmentToFlag_;
    std::set<ElfSecName> segments_;
    bool enableSecDump_ {false};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_AOT_FILE_ELF_BUILDER_H
