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

#ifndef ECMASCRIPT_COMPILER_AOT_FILE_ELF_READER_H
#define ECMASCRIPT_COMPILER_AOT_FILE_ELF_READER_H

#include <utility>
#include <stdint.h>
#include <string>
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/compiler/binary_section.h"
#include "ecmascript/compiler/aot_file/elf_checker.h"

namespace panda::ecmascript {

class ModuleSectionDes;

class ElfReader {
public:
    ElfReader(const MemMap &fileMapMem) : fileMapMem_(fileMapMem) {};
    ElfReader(ExecutedMemoryAllocator::ExeMem &stubsMem) : stubsMem_(stubsMem) {};
    ~ElfReader() = default;
    bool VerifyELFHeader(uint32_t version, bool strictMatch);
    void ParseELFSections(ModuleSectionDes &des, std::vector<ElfSecName> &secs);
    void ParseELFSections(std::vector<ModuleSectionDes> &des, std::vector<ElfSecName> &secs);
    void ParseELFSections(BinaryBufferParser &parser, std::vector<ModuleSectionDes> &des,
        std::vector<ElfSecName> &secs);
    bool ParseELFSegment();
    ModuleSectionDes::ModuleRegionInfo *GetCurModuleInfo(uint32_t i, llvm::ELF::Elf64_Off offset);
    void SeparateTextSections(std::vector<ModuleSectionDes> &des, const uintptr_t &secAddr,
        llvm::ELF::Elf64_Off &secOffset, const llvm::ELF::Elf64_Off &moduleInfoOffset);
    void SeparateArkStackMapSections(std::vector<ModuleSectionDes> &des, const uintptr_t &secAddr,
        llvm::ELF::Elf64_Off &secOffset, const llvm::ELF::Elf64_Off &moduleInfoOffset);
    void SeparateStrtabSections(std::vector<ModuleSectionDes> &des, const uintptr_t &secAddr,
        llvm::ELF::Elf64_Off &secOffset, const llvm::ELF::Elf64_Off &moduleInfoOffset);
    void SeparateSymtabSections(std::vector<ModuleSectionDes> &des, const uintptr_t &secAddr,
        llvm::ELF::Elf64_Off &secOffset, const llvm::ELF::Elf64_Off &moduleInfoOffset);
    void SeparateTextSections(BinaryBufferParser &parser, std::vector<ModuleSectionDes> &des,
        const uint64_t &secAddr, llvm::ELF::Elf64_Off &secOffset, const llvm::ELF::Elf64_Off &curShOffset);
    void SeparateArkStackMapSections(BinaryBufferParser &parser, std::vector<ModuleSectionDes> &des,
        const uint64_t &secAddr, llvm::ELF::Elf64_Off &secOffset, const llvm::ELF::Elf64_Off &curShOffset);
    void SeparateStrtabSections(BinaryBufferParser &parser, std::vector<ModuleSectionDes> &des,
        const uintptr_t &secAddr, llvm::ELF::Elf64_Off &secOffset, const llvm::ELF::Elf64_Off &curShOffset);
    void SeparateSymtabSections(BinaryBufferParser &parser, std::vector<ModuleSectionDes> &des,
        const uintptr_t &secAddr, llvm::ELF::Elf64_Off &secOffset, const llvm::ELF::Elf64_Off &curShOffset);

private:
    static uint32_t GetModuleNum(size_t moduleInfoSize)
    {
        ASSERT(moduleInfoSize % sizeof(ModuleSectionDes::ModuleRegionInfo) == 0);
        return moduleInfoSize / sizeof(ModuleSectionDes::ModuleRegionInfo);
    }

    static constexpr uint32_t ASMSTUB_MODULE_NUM = 4;
    ExecutedMemoryAllocator::ExeMem stubsMem_ {};
    MemMap fileMapMem_ {};
    std::vector<ModuleSectionDes::ModuleRegionInfo> moduleInfo_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_AOT_FILE_ELF_READER_H
