/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DEOPTIMIZER_RELCATOR_H
#define ECMASCRIPT_DEOPTIMIZER_RELCATOR_H

#include "ecmascript/common.h"
#if !WIN_OR_MAC_OR_IOS_PLATFORM

#include <elf.h>
#include <optional>
#include "ecmascript/ecma_macros.h"

namespace panda::ecmascript {
struct RelocateTextInfo {
    uintptr_t textAddr_ {0};
    uintptr_t relaTextAddr_ {0};
    uintptr_t relaTextSize_ {0};
};

struct SymAndStrTabInfo {
    uintptr_t symAddr_ {0};
    uintptr_t symSize_ {0};
    uintptr_t strAddr_ {0};
    uintptr_t strSize_ {0};
};

class Relocator {
public:
    PUBLIC_API Relocator(RelocateTextInfo relaText, SymAndStrTabInfo symAndStrTabInfo)
        :relocateTextInfo_(relaText), symAndStrTabInfo_(symAndStrTabInfo) {};

    PUBLIC_API bool RelocateBySymbolId(Elf64_Word symbolId, uintptr_t patchAddr);
    PUBLIC_API bool RelocateBySymbol(const char* symbol, uintptr_t patchAddr);
    PUBLIC_API void DumpRelocateText();
    ~Relocator() = default;
private:
    bool HasSymStrTable() const;
    bool HasRelocateText() const;
    std::optional<Elf64_Word> GetSymbol(const char* symbol) const;
    bool Relocate(Elf64_Rela *sec, uintptr_t symbolAddr, uintptr_t patchAddr);
    static bool BlInRange(intptr_t imm);

    Elf64_Word GetSymbol(Elf64_Rela *cur) const
    {
        Elf64_Word id = (cur->r_info >> 32); // 32: get high 32 bits
        return id;
    }

    Elf64_Word GetType(Elf64_Rela *cur) const
    {
        return (cur->r_info & 0xffffffffL); // 0xffffffff :get lower 32 bits
    }

    // These accessors and mutators are identical to those defined for ELF32
    // symbol table entries.
    unsigned char GetBinding(Elf64_Sym *cur) const
    {
        return cur->st_info >> 4; // 4: offset
    }

    unsigned char GetType(Elf64_Sym *cur) const
    {
        return cur->st_info & 0x0f; // f: get lowest 4 bits
    }

    RelocateTextInfo relocateTextInfo_ {0};
    SymAndStrTabInfo symAndStrTabInfo_ {0};
};
}  // panda::ecmascript
#endif
#endif  // ECMASCRIPT_DEOPTIMIZER_RELCATOR_H

