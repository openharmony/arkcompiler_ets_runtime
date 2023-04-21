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

#ifndef ECMASCRIPT_PLATFORM_ELF_H
#define ECMASCRIPT_PLATFORM_ELF_H

#include <stdint.h>
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
#include <elf.h>
#else
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
#define EI_NIDENT (16)

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off  e_phoff;
    Elf64_Off  e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
}   Elf64_Ehdr;
#endif

namespace panda::ecmascript::kungfu {
enum class Triple {
    TRIPLE_AMD64,
    TRIPLE_AARCH64,
    TRIPLE_ARM32,
};
}  // namespace panda::ecmascript::kungfu

namespace panda::ecmascript {
void PackELFHeader(Elf64_Ehdr &header, uint32_t version, kungfu::Triple triple);
bool SupportELF();
bool VerifyELFHeader(const Elf64_Ehdr &header, uint32_t version, bool silent = false);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_ELF_H
