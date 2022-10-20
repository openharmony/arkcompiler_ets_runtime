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

#include <map>
#include <cstring>

#include "ecmascript/common.h"

namespace panda::ecmascript {
enum class ElfSecName : uint8_t {
    NONE,
    RODATA,
    RODATA_CST4,
    RODATA_CST8,
    RODATA_CST16,
    RODATA_CST32,
    TEXT,
    DATA,
    GOT,
    RELATEXT,
    STRTAB,
    SYMTAB,
    LLVM_STACKMAP,
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
    }
    explicit ElfSection(size_t idx)
    {
        value_ = static_cast<ElfSecName>(idx);
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
        }  else if (str.compare(".llvm_stackmaps") == 0) {
            value_ = ElfSecName::LLVM_STACKMAP;
        }
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
        return ElfSecName::RODATA <= value_ && value_ <= ElfSecName::RODATA_CST8;
    }
private:
    ElfSecName value_ {ElfSecName::NONE};
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
        ElfSecFeature::VALID_NOT_SEQUENTIAL,
    };
};
}
#endif