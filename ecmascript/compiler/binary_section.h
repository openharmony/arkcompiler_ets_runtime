/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BINARY_SECTION_H
#define ECMASCRIPT_COMPILER_BINARY_SECTION_H

#include <string>
#include <map>
#include "ecmascript/common.h"
#include "llvm/BinaryFormat/ELF.h"

namespace panda::ecmascript {
enum class ElfSecName : uint8_t {
    NONE,
    RODATA,
    RODATA_CST4,
    RODATA_CST8,
    RODATA_CST16,
    RODATA_CST32,
    TEXT,
    ARK_ASMSTUB,
    DATA,
    GOT,
    RELATEXT,
    STRTAB,
    SYMTAB,
    SHSTRTAB,
    LLVM_STACKMAP,
    ARK_FUNCENTRY,
    ARK_STACKMAP,
    ARK_MODULEINFO,
    SIZE
};

enum ElfSecFeature : uint8_t {
    NOT_VALID,
    VALID_NOT_SEQUENTIAL = 1,
    VALID_AND_SEQUENTIAL = (1 << 1) | 1,

    VALID_MASK = 0x1,
    SEQUENTIAL_MASK = 0x2
};

class PUBLIC_API ElfSection {
public:
    ElfSection() = delete;

    explicit ElfSection(ElfSecName idx)
    {
        value_ = idx;
        InitShTypeAndFlag();
    }
    explicit ElfSection(size_t idx)
    {
        value_ = static_cast<ElfSecName>(idx);
        InitShTypeAndFlag();
    }
    explicit ElfSection(std::string str)
    {
        if (str.compare(".rodata") == 0) {
            value_ = ElfSecName::RODATA;
        } else if (str.compare(".rodata.cst4") == 0) {
            value_ = ElfSecName::RODATA_CST4;
        } else if (str.compare(".rodata.cst8") == 0) {
            value_ = ElfSecName::RODATA_CST8;
        } else if (str.compare(".rodata.cst16") == 0) {
            value_ = ElfSecName::RODATA_CST16;
        } else if (str.compare(".rodata.cst32") == 0) {
            value_ = ElfSecName::RODATA_CST32;
        } else if (str.compare(".text") == 0) {
            value_ = ElfSecName::TEXT;
        } else if (str.compare(".data") == 0) {
            value_ = ElfSecName::DATA;
        } else if (str.compare(".got") == 0) {
            value_ = ElfSecName::GOT;
        } else if (str.compare(".rela.text") == 0) {
            value_ = ElfSecName::RELATEXT;
        } else if (str.compare(".strtab") == 0) {
            value_ = ElfSecName::STRTAB;
        } else if (str.compare(".symtab") == 0) {
            value_ = ElfSecName::SYMTAB;
        } else if (str.compare(".shstrtab") == 0) {
            value_ = ElfSecName::SHSTRTAB;
        }  else if (str.compare(".llvm_stackmaps") == 0) {
            value_ = ElfSecName::LLVM_STACKMAP;
        } else if (str.compare(".ark_stackmaps") == 0) {
            value_ = ElfSecName::ARK_STACKMAP;
        } else if (str.compare(".ark_funcentry") == 0) {
            value_ = ElfSecName::ARK_FUNCENTRY;
        } else if (str.compare(".ark_asmstub") == 0) {
            value_ = ElfSecName::ARK_ASMSTUB;
        } else if (str.compare(".ark_moduleinfo") == 0) {
            value_ = ElfSecName::ARK_MODULEINFO;
        }
        InitShTypeAndFlag();
    }

    bool ShouldDumpToAOTFile() const
    {
        bool saveForAot = false;
        switch (value_) {
            case ElfSecName::TEXT:
            case ElfSecName::STRTAB:
            case ElfSecName::SYMTAB:
            case ElfSecName::SHSTRTAB:
            case ElfSecName::ARK_FUNCENTRY:
            case ElfSecName::ARK_ASMSTUB:
            case ElfSecName::ARK_STACKMAP:
            case ElfSecName::ARK_MODULEINFO: {
                saveForAot = true;
                break;
            }
            default: {
                break;
            }
        }
        return saveForAot;
    }

    ElfSecName Value() const
    {
        return value_;
    }

    int Entsize() const
    {
        if (value_ == ElfSecName::RELATEXT || value_ == ElfSecName::SYMTAB) {
            return FIX_SIZE;
        }
        return 0;
    }

    int Link() const
    {
        // The strtab index is 2 inside An file.
        return value_ == ElfSecName::SYMTAB ? 2 : 0;
    }

    void InitShTypeAndFlag()
    {
        std::map<ElfSecName, std::pair<unsigned, unsigned>> nameToTypeAndFlag = {
            {ElfSecName::RODATA, {llvm::ELF::SHT_PROGBITS, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_MERGE}},
            {ElfSecName::RODATA_CST4, {llvm::ELF::SHT_PROGBITS, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_MERGE}},
            {ElfSecName::RODATA_CST8, {llvm::ELF::SHT_PROGBITS, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_MERGE}},
            {ElfSecName::RODATA_CST16, {llvm::ELF::SHT_PROGBITS, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_MERGE}},
            {ElfSecName::RODATA_CST32, {llvm::ELF::SHT_PROGBITS, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_MERGE}},
            {ElfSecName::TEXT, {llvm::ELF::SHT_PROGBITS, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_EXECINSTR}},
            {ElfSecName::ARK_ASMSTUB, {llvm::ELF::SHT_PROGBITS, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_EXECINSTR}},
            {ElfSecName::DATA, {llvm::ELF::SHT_PROGBITS, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_WRITE}},
            {ElfSecName::GOT, {llvm::ELF::SHT_PROGBITS, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_WRITE}},
            {ElfSecName::RELATEXT, {llvm::ELF::SHT_RELA, llvm::ELF::SHF_ALLOC | llvm::ELF::SHF_WRITE}},
            {ElfSecName::STRTAB, {llvm::ELF::SHT_STRTAB, llvm::ELF::SHF_ALLOC}},
            {ElfSecName::SYMTAB, {llvm::ELF::SHT_SYMTAB, llvm::ELF::SHF_ALLOC}},
            {ElfSecName::SHSTRTAB, {llvm::ELF::SHT_STRTAB, llvm::ELF::SHF_ALLOC}},
            {ElfSecName::LLVM_STACKMAP, {llvm::ELF::SHT_RELA, llvm::ELF::SHF_ALLOC}},
            {ElfSecName::ARK_FUNCENTRY, {llvm::ELF::SHF_WRITE, llvm::ELF::SHF_ALLOC}},
            {ElfSecName::ARK_STACKMAP, {llvm::ELF::SHF_WRITE, llvm::ELF::SHF_ALLOC}},
            {ElfSecName::ARK_MODULEINFO, {llvm::ELF::SHF_WRITE, llvm::ELF::SHF_ALLOC}},
        };
        auto it = nameToTypeAndFlag.find(value_);
        if (it == nameToTypeAndFlag.end()) {
            return;
        }
        ASSERT(it != nameToTypeAndFlag.end());
        type_ = it->second.first;
        flag_ = it->second.second;
    }

    unsigned Type() const
    {
        return type_;
    }

    unsigned Flag() const
    {
        return flag_;
    }

    bool isValidAOTSec() const
    {
        auto idx = static_cast<size_t>(value_);
        return static_cast<uint8_t>(AOTSecFeatureTable_[idx]) & ElfSecFeature::VALID_MASK;
    }

    bool isSequentialAOTSec() const
    {
        auto idx = static_cast<size_t>(value_);
        return static_cast<uint8_t>(AOTSecFeatureTable_[idx]) & ElfSecFeature::SEQUENTIAL_MASK;
    }

    ElfSecName GetElfEnumValue() const
    {
        return value_;
    }

    int GetIntIndex() const
    {
        return static_cast<int>(value_);
    }

    // RO data section needs 16 bytes alignment
    bool InRodataSection() const
    {
        return ElfSecName::RODATA <= value_ && value_ <= ElfSecName::RODATA_CST32;
    }
private:
    static int const FIX_SIZE = 24; // 24:Elf_Rel
    ElfSecName value_ {ElfSecName::NONE};
    unsigned type_ {0};
    unsigned flag_ {0};

    static constexpr size_t AOTSecFeatureTable_[static_cast<size_t>(ElfSecName::SIZE)] = {
        ElfSecFeature::NOT_VALID,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_AND_SEQUENTIAL,
        ElfSecFeature::VALID_NOT_SEQUENTIAL,
        ElfSecFeature::VALID_NOT_SEQUENTIAL,
    };
};
}
#endif