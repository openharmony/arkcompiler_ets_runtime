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
#include "ecmascript/compiler/aot_file/module_section_des.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/platform/file.h"
#include <iomanip>

namespace panda::ecmascript {
void ModuleSectionDes::SaveSectionsInfo(std::ofstream &file)
{
    uint32_t secInfoSize = GetSecInfosSize();
    uint32_t secSize = 0;
    file.write(reinterpret_cast<char *>(&secInfoSize), sizeof(secInfoSize));
    std::multimap<std::string, double> SecMap;
    for (auto &s : sectionsInfo_) {
        uint8_t secName = static_cast<uint8_t>(s.first);
        uint32_t curSecSize = GetSecSize(s.first);
        uint64_t curSecAddr = GetSecAddr(s.first);
        secSize += curSecSize;
        SecMap.insert(make_pair(GetSecName(s.first), static_cast<double>(curSecSize)));
        file.write(reinterpret_cast<char *>(&secName), sizeof(secName));
        file.write(reinterpret_cast<char *>(&curSecSize), sizeof(curSecSize));
        file.write(reinterpret_cast<char *>(curSecAddr), curSecSize);
    }
    std::shared_ptr<uint8_t> ptr = GetArkStackMapSharePtr();
    uint32_t size = GetArkStackMapSize();
    file.write(reinterpret_cast<char *>(&size), sizeof(size));
    file.write(reinterpret_cast<char *>(ptr.get()), size);

    uint32_t index = GetStartIndex();
    uint32_t cnt = GetFuncCount();
    file.write(reinterpret_cast<char *>(&index), sizeof(index));
    file.write(reinterpret_cast<char *>(&cnt), sizeof(cnt));
    for (auto [key, val] : SecMap) {
        LOG_COMPILER(DEBUG) << key << " size is " << std::fixed << std::setprecision(DECIMAL_LENS) << (val / 1_KB)
                            << "KB "
                            << "percentage:" << std::fixed << std::setprecision(PERCENT_LENS)
                            << (val / secSize * HUNDRED_TIME) << "% ";
    }
    LOG_COMPILER(DEBUG) << "elf sections size = " << (secSize / 1_KB) << "KB"
                        << ", ark stack map size = " << (size / 1_KB) << "KB";
}

void ModuleSectionDes::LoadStackMapSection(BinaryBufferParser &parser, uintptr_t secBegin, uint32_t &curUnitOffset)
{
    uint32_t size = 0;
    parser.ParseBuffer(&size, sizeof(size));
    parser.ParseBuffer(reinterpret_cast<void *>(secBegin), size);
    SetArkStackMapSize(size);
    SetArkStackMapPtr(reinterpret_cast<uint8_t *>(secBegin));
    curUnitOffset += size;
    uint32_t index = 0;
    uint32_t cnt = 0;
    parser.ParseBuffer(&index, sizeof(index));
    parser.ParseBuffer(&cnt, sizeof(cnt));
    SetStartIndex(index);
    SetFuncCount(cnt);
}

void ModuleSectionDes::LoadSectionsInfo(BinaryBufferParser &parser, uint32_t &curUnitOffset, uint64_t codeAddress)
{
    uint32_t secInfoSize = 0;
    parser.ParseBuffer(&secInfoSize, sizeof(secInfoSize));
    auto secBegin = codeAddress + static_cast<uintptr_t>(curUnitOffset);
    for (uint8_t i = 0; i < secInfoSize; i++) {
        uint8_t secName = 0;
        parser.ParseBuffer(&secName, sizeof(secName));
        auto secEnumName = static_cast<ElfSecName>(secName);
        uint32_t secSize = 0;
        parser.ParseBuffer(&secSize, sizeof(secSize));
        SetSecSize(secSize, secEnumName);
        parser.ParseBuffer(reinterpret_cast<void *>(secBegin), secSize);
        curUnitOffset += secSize;
        SetSecAddr(secBegin, secEnumName);
        secBegin += secSize;
    }
    LoadStackMapSection(parser, secBegin, curUnitOffset);
}

void ModuleSectionDes::LoadStackMapSection(std::ifstream &file, uintptr_t secBegin, uint32_t &curUnitOffset)
{
    uint32_t size = 0;
    file.read(reinterpret_cast<char *>(&size), sizeof(size));
    file.read(reinterpret_cast<char *>(secBegin), size);
    SetArkStackMapSize(size);
    SetArkStackMapPtr(reinterpret_cast<uint8_t *>(secBegin));
    curUnitOffset += size;
    uint32_t index = 0;
    uint32_t cnt = 0;
    file.read(reinterpret_cast<char *>(&index), sizeof(index));
    file.read(reinterpret_cast<char *>(&cnt), sizeof(cnt));
    SetStartIndex(index);
    SetFuncCount(cnt);
}

void ModuleSectionDes::LoadSectionsInfo(std::ifstream &file, uint32_t &curUnitOffset, uint64_t codeAddress)
{
    uint32_t secInfoSize = 0;
    file.read(reinterpret_cast<char *>(&secInfoSize), sizeof(secInfoSize));
    auto secBegin = codeAddress + static_cast<uintptr_t>(curUnitOffset);
    for (uint8_t i = 0; i < secInfoSize; i++) {
        uint8_t secName = 0;
        file.read(reinterpret_cast<char *>(&secName), sizeof(secName));
        auto secEnumName = static_cast<ElfSecName>(secName);
        uint32_t secSize;
        file.read(reinterpret_cast<char *>(&secSize), sizeof(secSize));
        SetSecSize(secSize, secEnumName);
        file.read(reinterpret_cast<char *>(secBegin), secSize);
        curUnitOffset += secSize;
        SetSecAddr(secBegin, secEnumName);
        secBegin += secSize;
    }
    LoadStackMapSection(file, secBegin, curUnitOffset);
}

std::string ModuleSectionDes::GetSecName(const ElfSecName idx)
{
    switch (idx) {
        case ElfSecName::RODATA:
            return ".rodata";
        case ElfSecName::RODATA_CST4:
            return ".rodata.cst4";
        case ElfSecName::RODATA_CST8:
            return ".rodata.cst8";
        case ElfSecName::RODATA_CST16:
            return ".rodata.cst16";
        case ElfSecName::RODATA_CST32:
            return ".rodata.cst32";
        case ElfSecName::TEXT:
            return ".text";
        case ElfSecName::DATA:
            return ".data";
        case ElfSecName::GOT:
            return ".got";
        case ElfSecName::RELATEXT:
            return ".rela.text";
        case ElfSecName::STRTAB:
            return ".strtab";
        case ElfSecName::SYMTAB:
            return ".symtab";
        case ElfSecName::LLVM_STACKMAP:
            return ".llvm_stackmaps";
        case ElfSecName::ARK_STACKMAP:
            return ".ark_stackmaps";
        case ElfSecName::ARK_FUNCENTRY:
            return ".ark_funcentry";
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
}
}  // namespace panda::ecmascript
