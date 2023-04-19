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

#include "ecmascript/platform/elf.h"

#include "ecmascript/log_wrapper.h"
#include "securec.h"

#define EI_MAG0		0
#define ELFMAG0		0x7f

#define EI_MAG1		1
#define ELFMAG1		'E'

#define EI_MAG2		2
#define ELFMAG2		'L'

#define EI_MAG3		3
#define ELFMAG3		'F'

#define EI_CLASS	4
#define ELFCLASS64	2

#define EI_DATA		5
#define ELFDATA2LSB	1

#define EI_VERSION	6

#define ET_DYN		3
#define EM_X86_64	62
#define EM_ARM		40
#define EM_AARCH64	183

namespace panda::ecmascript {
void PackELFHeader(Elf64_Ehdr &header, uint32_t version, kungfu::Triple triple)
{
    if (memset_s(reinterpret_cast<void *>(&header), sizeof(Elf64_Ehdr), 0, sizeof(Elf64_Ehdr)) != EOK) {
        UNREACHABLE();
    }
    header.e_ident[EI_MAG0] = ELFMAG0;
    header.e_ident[EI_MAG1] = ELFMAG1;
    header.e_ident[EI_MAG2] = ELFMAG2;
    header.e_ident[EI_MAG3] = ELFMAG3;
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA] = ELFDATA2LSB;
    header.e_ident[EI_VERSION] = 1;

    header.e_type = ET_DYN;
    switch (triple) {
        case kungfu::Triple::TRIPLE_AMD64:
            header.e_machine = EM_X86_64;
            break;
        case kungfu::Triple::TRIPLE_ARM32:
            header.e_machine = EM_ARM;
            break;
        case kungfu::Triple::TRIPLE_AARCH64:
            header.e_machine = EM_AARCH64;
            break;
        default:
            UNREACHABLE();
            break;
    }
    header.e_version = version;
}

bool VerifyELFHeader([[maybe_unused]] const Elf64_Ehdr &header, [[maybe_unused]] uint32_t version,
                     [[maybe_unused]] bool silent)
{
    LOG_ECMA(ERROR) << "Elf is not supported on windows platform";
    return false;
}
}  // namespace panda::ecmascript
