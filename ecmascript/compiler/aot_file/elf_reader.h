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
#include "ecmascript/aot_file_manager.h"
#include "ecmascript/compiler/binary_section.h"
namespace panda::ecmascript {

struct ModuleSectionDes;

class ElfReader {
public:
    ElfReader(MemMap fileMapMem) : fileMapMem_(fileMapMem) {};
    ~ElfReader() = default;
    bool VerifyELFHeader(uint32_t version);
    void ParseELFSections(ModuleSectionDes &des, std::vector<ElfSecName> &secs);
    bool ParseELFSegment();

private:
    MemMap fileMapMem_ {};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_AOT_FILE_ELF_READER_H