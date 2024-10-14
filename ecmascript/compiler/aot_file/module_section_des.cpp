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
#include <iomanip>

namespace panda::ecmascript {
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
        case ElfSecName::SHSTRTAB:
            return ".shstrtab";
        case ElfSecName::LLVM_STACKMAP:
            return ".llvm_stackmaps";
        case ElfSecName::ARK_STACKMAP:
            return ".ark_stackmaps";
        case ElfSecName::ARK_FUNCENTRY:
            return ".ark_funcentry";
        case ElfSecName::ARK_ASMSTUB:
            return ".ark_asmstub";
        case ElfSecName::ARK_MODULEINFO:
            return ".ark_moduleinfo";
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
}
}  // namespace panda::ecmascript
