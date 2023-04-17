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

#include "ecmascript/deoptimizer/relocator.h"
#include <climits>
#include <iomanip>
#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64_constants.h"
#include "ecmascript/message_string.h"
#if !WIN_OR_MAC_OR_IOS_PLATFORM

namespace panda::ecmascript {
std::optional<Elf64_Word> Relocator::GetSymbol(const char* symbol) const
{
    ASSERT(symAndStrTabInfo_.symSize_ % sizeof(Elf64_Sym) == 0);
    int n = symAndStrTabInfo_.symSize_ / sizeof(Elf64_Sym);
    ASSERT(symAndStrTabInfo_.symAddr_ > 0 && symAndStrTabInfo_.symSize_ > 0);
    Elf64_Sym *ptr = reinterpret_cast<Elf64_Sym *>(symAndStrTabInfo_.symAddr_);
    for (int i = 0; i < n; i++) {
        Elf64_Sym *cur = ptr + i;
        const char *name = reinterpret_cast<char *>(symAndStrTabInfo_.strAddr_) + cur->st_name;
        if (std::strcmp(symbol, name) == 0) {
            return static_cast<Elf64_Word>(i);
        }
    }
    return std::nullopt;
}

bool Relocator::Relocate(Elf64_Rela *sec, uintptr_t symbolAddr, uintptr_t patchAddr)
{
    bool ret = false;
    ASSERT(reinterpret_cast<intptr_t>(sec) > 0);
    Elf64_Word type = GetType(sec);
    Elf64_Sxword addend = sec->r_addend;

    switch (type) {
        case R_AARCH64_CALL26: {
            /* S + A - P
            S: (when used on its own) is the address of the symbol
            A: is the addend for the relocation
            P: is the address of the place beging relocated(derived from r_offset)
            */
            intptr_t imm = patchAddr + addend - symbolAddr;
            if (BlInRange(imm)) {
                imm = (imm & 0x0FFFFFFC) >> 2; // 0x0FFFFFFC: get immediate file to bits [27:2]
                *(reinterpret_cast<uint32_t *>(symbolAddr)) = imm | panda::ecmascript::aarch64::CallOpCode::BL;
                ret = true;
            }
            break;
        }
        case R_X86_64_PLT32: {
            /* S + A - P
            S: (when used on its own) is the address of the symbol
            A: is the addend for the relocation
            P: is the address of the place beging relocated(derived from r_offset)
            */
            intptr_t v = patchAddr + addend - symbolAddr;
            ASSERT((v >= INT_MIN) && (v <= INT_MAX));
            *(reinterpret_cast<uint32_t *>(symbolAddr)) = v;
            ret = true;
            break;
        }
        default: {
            LOG_COMPILER(FATAL) << " unsupported type:" << type;
            return false;
        }
    }
    return ret;
}

bool Relocator::HasSymStrTable() const
{
    return (symAndStrTabInfo_.symAddr_ >= 0) && (symAndStrTabInfo_.symSize_ >= 0)
        && (symAndStrTabInfo_.strAddr_ >= 0) && (symAndStrTabInfo_.strSize_ >= 0);
}

bool Relocator::HasRelocateText() const
{
    return (relocateTextInfo_.relaTextAddr_ > 0) && (relocateTextInfo_.relaTextSize_ > 0);
}

bool Relocator::RelocateBySymbolId(Elf64_Word symbolId, uintptr_t patchAddr)
{
    bool ret = false;
    ASSERT(relocateTextInfo_.relaTextSize_ % sizeof(Elf64_Rela) == 0);
    ASSERT(relocateTextInfo_.relaTextAddr_ > 0 && relocateTextInfo_.relaTextSize_ > 0);
    size_t n  = relocateTextInfo_.relaTextSize_ / sizeof(Elf64_Rela);
    Elf64_Rela *ptr = reinterpret_cast<Elf64_Rela *>(relocateTextInfo_.relaTextAddr_);
    for (size_t i = 0; i < n; i++) {
        Elf64_Rela *cur = ptr + i;
        Elf64_Word id = GetSymbol(cur);
        intptr_t symbolAddr = relocateTextInfo_.textAddr_ + static_cast<uintptr_t>(cur->r_offset);
        if (id == symbolId) {
            ret = Relocate(cur, symbolAddr, patchAddr);
        }
    }
    return ret;
}

bool Relocator::RelocateBySymbol(const char* symbol, uintptr_t patchAddr)
{
    if (!HasSymStrTable()) {
        return false;
    }
    auto symId = GetSymbol(symbol);
    if (!symId.has_value()) {
        LOG_COMPILER(DEBUG) << " don't find symbol:" << symbol << " in symbol table.";
        return false;
    }
    bool ret = RelocateBySymbolId(symId.value(), patchAddr);
    return ret;
}

void Relocator::DumpRelocateText()
{
    if (!HasRelocateText()) {
        LOG_COMPILER(ERROR) << " input valid relocateText addr & size:";
        return;
    }
    ASSERT(relocateTextInfo_.relaTextSize_ % sizeof(Elf64_Rela) == 0);
    ASSERT(relocateTextInfo_.relaTextAddr_ > 0 && relocateTextInfo_.relaTextSize_ > 0);
    size_t n  = relocateTextInfo_.relaTextSize_ / sizeof(Elf64_Rela);
    Elf64_Rela *ptr = reinterpret_cast<Elf64_Rela *>(relocateTextInfo_.relaTextAddr_);
    static constexpr int leftAdjustment = 12;
    LOG_COMPILER(DEBUG) << std::left << std::setw(leftAdjustment) << "symbolId "
                   << std::left << std::setw(leftAdjustment) << "Info(0x): "
                   << std::left << std::setw(leftAdjustment) << "Type: "
                   << std::left << std::setw(leftAdjustment) << "r_offset(0x): "
                   << std::left << std::setw(leftAdjustment) << "addend: ";
    for (size_t i = 0; i < n; i++) {
        Elf64_Rela *cur = ptr + i;
        Elf64_Word id = GetSymbol(cur);
        Elf64_Word type = GetType(cur);
        Elf64_Sxword addend = ptr->r_addend;
        LOG_COMPILER(DEBUG) << std::left << std::setw(leftAdjustment) << id
                << std::left << std::setw(leftAdjustment) << std::hex << cur->r_info
                << std::left << std::setw(leftAdjustment) << std::dec << type
                << std::left << std::setw(leftAdjustment) << std::hex << static_cast<intptr_t>(cur->r_offset)
                << std::left << std::setw(leftAdjustment) << std::dec << static_cast<intptr_t>(addend);
    }
    if (!HasSymStrTable()) {
        return;
    }
    ASSERT(symAndStrTabInfo_.symSize_ % sizeof(Elf64_Sym) == 0);
    n = symAndStrTabInfo_.symSize_ / sizeof(Elf64_Sym);
    ASSERT(symAndStrTabInfo_.symAddr_ > 0 && symAndStrTabInfo_.symSize_ > 0);
    Elf64_Sym *symPtr = reinterpret_cast<Elf64_Sym *>(symAndStrTabInfo_.symAddr_);
    LOG_COMPILER(DEBUG) << std::left << std::setw(leftAdjustment) << "symbolId "
                << std::left << std::setw(leftAdjustment) << "binding: "
                << std::left << std::setw(leftAdjustment) << "Type: "
                << std::left << std::setw(leftAdjustment) << "st_name: "
                << std::left << std::setw(leftAdjustment) << "name: ";
    for (size_t i = 0; i < n; i++) {
        Elf64_Sym *cur = symPtr + i;
        const char *name = reinterpret_cast<char *>(symAndStrTabInfo_.strAddr_) + cur->st_name;
        unsigned char binding = GetBinding(cur);
        unsigned char type = GetType(cur);
        LOG_COMPILER(DEBUG) << std::left << std::setw(leftAdjustment) << i
            << std::left << std::setw(leftAdjustment) << std::dec << static_cast<int>(binding)
            << std::left << std::setw(leftAdjustment) << std::dec << static_cast<int>(type)
            << std::left << std::setw(leftAdjustment) << std::dec << cur->st_name
            << std::left << std::setw(leftAdjustment) << name;
    }
}

bool Relocator::BlInRange(intptr_t imm)
{
    return -(1 << 27) <= imm && imm < (1 << 27);
}
} // namespace panda::ecmascript
#endif
