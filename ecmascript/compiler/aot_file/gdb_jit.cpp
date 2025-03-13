/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/aot_file/gdb_jit.h"
#include "llvm/BinaryFormat/ELF.h"
#include "ecmascript/log_wrapper.h"

#include <securec.h>
#include <mutex>

#ifndef PANDA_TARGET_MACOS
// Keep in sync with gdb/gdb/jit.h
extern "C" {
typedef enum {
    JIT_NOACTION = 0,
    JIT_REGISTER_FN,
    JIT_UNREGISTER_FN
} jit_actions_t;

struct jit_code_entry {
    struct jit_code_entry *next_entry;
    struct jit_code_entry *prev_entry;
    const char *symfile_addr;
    uint64_t symfile_size;
    const char *file_addr; // extend the standard
};

struct jit_descriptor {
    uint32_t version;
    // This should be jit_actions_t, but we want to be specific about the
    // bit-width.
    uint32_t action_flag;
    struct jit_code_entry *relevant_entry;
    struct jit_code_entry *first_entry;
};

// We put information about the JITed function in this global, which the
// debugger reads.  Make sure to specify the version statically, because the
// debugger checks the version before we can set it during runtime.
struct jit_descriptor __jit_debug_descriptor = {
    1, JIT_NOACTION, nullptr, nullptr
};

// Debuggers puts a breakpoint in this function.
void __attribute__((noinline)) __jit_debug_register_code()
{
    LOG_COMPILER(INFO) << "__jit_debug_register_code() is called.";
}
}

std::mutex g_descMutex;

namespace panda::ecmascript {
namespace jit_debug {
using namespace llvm::ELF;

static bool RegisterStubAnToDebuggerImpl(const char *fileAddr);

void RegisterStubAnToDebugger(const char *fileAddr)
{
    std::lock_guard<std::mutex> guard(g_descMutex);
    auto entry = __jit_debug_descriptor.first_entry;
    while (entry != nullptr && entry->file_addr != fileAddr) {
        entry = entry->next_entry;
    }
    if (entry != nullptr) {
        return;
    }
    if (RegisterStubAnToDebuggerImpl(fileAddr)) {
        LOG_COMPILER(INFO) << "success to register stub.an to debugger.";
    } else {
        LOG_COMPILER(ERROR) << "Can't register stub.an to debugger.";
    }
}

void UnregisterStubAnFromDebugger(const char *fileAddr)
{
    std::lock_guard<std::mutex> guard(g_descMutex);
    auto entry = __jit_debug_descriptor.first_entry;
    while (entry != nullptr && entry->file_addr != fileAddr) {
        entry = entry->next_entry;
    }
    if (entry == nullptr) {
        return;
    }
    if (entry->prev_entry != nullptr) {
        entry->prev_entry->next_entry = entry->next_entry;
    }
    if (entry->next_entry != nullptr) {
        entry->next_entry->prev_entry = entry->prev_entry;
    }
    __jit_debug_descriptor.action_flag = JIT_UNREGISTER_FN;
    __jit_debug_descriptor.relevant_entry = entry;
    __jit_debug_register_code();
    __jit_debug_descriptor.relevant_entry = nullptr;
    delete entry->symfile_addr;
    delete entry;
}

template<typename T, typename U>
inline T OffsetAlignUp(U *addr, uint64_t offset, uint32_t align)
{
    auto value = reinterpret_cast<uint64_t>(addr) + offset;
    auto result = value;
    if (align != 0 && (value % align != 0)) {
        result = value + (align - (value % align));
    }
    return reinterpret_cast<T>(result);
}

struct StubAnInfo {
    uintptr_t fileAddr;
    Elf64_Ehdr *ehdr;
    Elf64_Shdr *shdrTab;
    uint32_t shStrIdx;
    Elf64_Shdr *shStrHdr;
    Elf64_Shdr *textHdr;
    Elf64_Shdr *asmstubHdr;
    Elf64_Shdr *symtabHdr;
    Elf64_Shdr *strtabHdr;
    uint64_t bcStubBegin;
    uint64_t bcStubEnd;
    uint32_t symCnt;
};

/*
 * [0] file header
 * [1] program header
 * [2] shstrtab
 * [3] strtab
 * [4] symtab
 * [5] .eh_frame
 * [6] empty header
 * [7] shstrtab-header
 * [8] strtab-header
 * [9] symtab-header
 * [10] text-header
 * [11] .eh_frame header
 */
const char SHSTR[] = "\0.shstrtab\0.strtab\0.symtab\0.text\0.eh_frame";
const uint32_t SHSTRTAB_NAME = 1;
const uint32_t STRTAB_NAME = SHSTRTAB_NAME + strlen(".shstrtab") + 1;
const uint32_t SYMTAB_NAME = STRTAB_NAME + strlen(".strtab") + 1;
const uint32_t TEXT_NAME = SYMTAB_NAME + strlen(".symtab") + 1;
const uint32_t EH_FRAME_NAME = TEXT_NAME + strlen(".text") + 1;

const uint32_t SHSTRTAB_HDR_IDX = 1;
const uint32_t STRTAB_HDR_IDX = 2;
const uint32_t SYMTAB_HDR_IDX = 3;
const uint32_t TEXT_HDR_IDX = 4;

const uint32_t HEADER_CNT = 6;

inline int InfoGetBind(unsigned char info)
{
    const uint32_t shift = 4;
    return info >> shift;
}

StubAnInfo CollectStubAnInfo(uintptr_t fileAddr)
{
    auto *ehdr = reinterpret_cast<Elf64_Ehdr *>(fileAddr);
    auto *shdrTab = reinterpret_cast<Elf64_Shdr *>(fileAddr + ehdr->e_shoff);
    uint32_t shStrIdx = ehdr->e_shstrndx;
    Elf64_Shdr *shStrHdr = &shdrTab[shStrIdx];
    const char *shstrtab = reinterpret_cast<const char *>(fileAddr + shStrHdr->sh_offset);
    Elf64_Shdr *textHdr = nullptr;
    Elf64_Shdr *asmstubHdr = nullptr;
    Elf64_Shdr *symtabHdr = nullptr;
    Elf64_Shdr *strtabHdr = nullptr;
    for (uint32_t i = 0; i < ehdr->e_shnum; i++) {
        Elf64_Shdr *shdr = &shdrTab[i];
        const char *name = &shstrtab[shdr->sh_name];
        if (strcmp(name, ".text") == 0) {
            textHdr = shdr;
        } else if (strcmp(name, ".ark_asmstub") == 0) {
            asmstubHdr = shdr;
        } else if (strcmp(name, ".symtab") == 0) {
            symtabHdr = shdr;
        } else if (strcmp(name, ".strtab") == 0) {
            strtabHdr = shdr;
        }
    }
    ASSERT(symtabHdr != nullptr);
    Elf64_Sym *symtab = reinterpret_cast<Elf64_Sym *>(fileAddr + symtabHdr->sh_offset);
    const char *strtab = reinterpret_cast<const char *>(fileAddr + strtabHdr->sh_offset);
    uint32_t symCnt = 2;
    uint64_t bcStubBegin = UINT64_MAX;
    uint64_t bcStubEnd = 0;
    for (uint32_t symIdx = 0; symIdx < symtabHdr->sh_size / symtabHdr->sh_entsize; symIdx++) {
        Elf64_Sym *sym = symtab + symIdx;
        if (InfoGetBind(sym->st_info) != STB_GLOBAL) {
            continue;
        }
        if (strncmp(&strtab[sym->st_name], "BCStub", strlen("BCStub")) != 0) {
            symCnt++;
        } else {
            if (sym->st_value < bcStubBegin) {
                bcStubBegin = sym->st_value;
            }
            if (sym->st_value + sym->st_size > bcStubEnd) {
                bcStubEnd = sym->st_value + sym->st_size;
            }
        }
    }
    return StubAnInfo {
        fileAddr, ehdr, shdrTab, shStrIdx, shStrHdr, textHdr, asmstubHdr, symtabHdr, strtabHdr,
        bcStubBegin, bcStubEnd, symCnt
    };
}

bool CopyStrTab(uintptr_t baseAddr, const StubAnInfo &info)
{
    Elf64_Ehdr *newEhdr = reinterpret_cast<Elf64_Ehdr *>(baseAddr);
    Elf64_Phdr *newPhdr = reinterpret_cast<Elf64_Phdr *>(newEhdr + 1);
    char *shStrBuff = reinterpret_cast<char *>(newPhdr + 1);
    if (memcpy_s(shStrBuff, sizeof(SHSTR), SHSTR, sizeof(SHSTR)) != EOK) {
        return false;
    }
    char *newStrtab = shStrBuff + sizeof(SHSTR);
    if (memcpy_s(newStrtab, info.strtabHdr->sh_size,
        reinterpret_cast<void *>(info.fileAddr + info.strtabHdr->sh_offset), info.strtabHdr->sh_size) != EOK) {
        return false;
    }
    const char bcStubName[] = "BCStubInterpreterRoutine";
    if (memcpy_s((newStrtab + 1), sizeof(bcStubName), bcStubName, sizeof(bcStubName)) != EOK) {
        return false;
    }
    return true;
}

void ConstructSymTab(Elf64_Sym *newSymtab, const StubAnInfo &info)
{
    Elf64_Sym *symtab = reinterpret_cast<Elf64_Sym *>(info.fileAddr + info.symtabHdr->sh_offset);
    const char *strtab = reinterpret_cast<const char *>(info.fileAddr + info.strtabHdr->sh_offset);
    memset_s(newSymtab, sizeof(Elf64_Sym), 0, sizeof(Elf64_Sym));
    uint32_t newSymIdx = 1;
    for (uint32_t symIdx = 0; symIdx < info.symtabHdr->sh_size / info.symtabHdr->sh_entsize; symIdx++) {
        Elf64_Sym *src = symtab + symIdx;
        if (InfoGetBind(src->st_info) == STB_GLOBAL
            && strncmp(&strtab[src->st_name], "BCStub", strlen("BCStub")) != 0) {
            auto dst = newSymtab + newSymIdx;
            newSymIdx++;
            *dst = *src;
            dst->st_shndx = TEXT_HDR_IDX;
            dst->st_value -= info.textHdr->sh_offset;
        }
    }
    auto bcSym = newSymtab + newSymIdx;
    bcSym->st_name = 1;
    bcSym->st_info = newSymtab[1].st_info;
    bcSym->st_other = newSymtab[1].st_other;
    bcSym->st_shndx = TEXT_HDR_IDX;
    bcSym->st_value = info.bcStubBegin - info.textHdr->sh_offset;
    ASSERT(info.bcStubEnd >= info.bcStubBegin);
    bcSym->st_size = info.bcStubEnd - info.bcStubBegin;
}

void ConstructEhdrAndPhdr(Elf64_Ehdr *newEhdr, Elf64_Shdr *newShdrtab, uintptr_t baseAddr, const StubAnInfo &info)
{
    Elf64_Phdr *newPhdr = reinterpret_cast<Elf64_Phdr *>(newEhdr + 1);
    {
        *newEhdr = *info.ehdr;
        newEhdr->e_flags = info.ehdr->e_flags;
        newEhdr->e_machine = info.ehdr->e_machine;
        if (memcpy_s(newEhdr->e_ident, sizeof(info.ehdr->e_ident),
                     info.ehdr->e_ident, sizeof(info.ehdr->e_ident)) != EOK) {
            return;
        }
        newEhdr->e_version = 1;
        newEhdr->e_phoff = sizeof(Elf64_Ehdr);
        newEhdr->e_shoff = reinterpret_cast<uintptr_t>(newShdrtab) - baseAddr;
        newEhdr->e_ehsize = sizeof(Elf64_Ehdr);
        newEhdr->e_phentsize = sizeof(Elf64_Phdr);
        newEhdr->e_phnum = 1;
        newEhdr->e_shentsize = sizeof(Elf64_Shdr);
        newEhdr->e_shnum = HEADER_CNT;
        newEhdr->e_shstrndx = SHSTRTAB_HDR_IDX;
        newEhdr->e_type = ET_REL;
        newEhdr->e_entry = 0;
    }
    uintptr_t textAddr = info.textHdr->sh_offset + info.fileAddr;
    uint64_t textSize = info.asmstubHdr->sh_offset + info.asmstubHdr->sh_size - info.textHdr->sh_offset;
    {
        newPhdr->p_type = PT_LOAD;
        newPhdr->p_flags = PF_X | PF_R;
        newPhdr->p_offset = textAddr - info.fileAddr;
        newPhdr->p_vaddr = textAddr;
        newPhdr->p_paddr = textAddr;
        newPhdr->p_filesz = textSize;
        newPhdr->p_memsz = textSize;
        newPhdr->p_align = 0x1000;
    }
}

void ConstructShdrTab(Elf64_Shdr *newShdrTab, Elf64_Sym *newSymtab, uintptr_t baseAddr, void *ehFrame,
                      uint32_t ehFrameSize, const StubAnInfo &info)
{
    Elf64_Shdr hdr{};
    Elf64_Shdr *emptyShdr = newShdrTab;
    Elf64_Shdr *newShstrHdr = emptyShdr + 1;
    Elf64_Shdr *newStrtabHdr = newShstrHdr + 1;
    Elf64_Shdr *newSymHdr = newStrtabHdr + 1;
    Elf64_Shdr *newTextHdr = newSymHdr + 1;
    Elf64_Shdr *ehFrameHdr = newTextHdr + 1;
    *emptyShdr = hdr;

    newShstrHdr->sh_offset = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
    newShstrHdr->sh_size = sizeof(SHSTR);
    newShstrHdr->sh_name = SHSTRTAB_NAME;
    newShstrHdr->sh_addr = newStrtabHdr->sh_offset + baseAddr;
    newShstrHdr->sh_type = SHT_STRTAB;
    newShstrHdr->sh_flags = SHF_ALLOC;

    newStrtabHdr->sh_offset = newShstrHdr->sh_offset + newShstrHdr->sh_size;
    newStrtabHdr->sh_size = info.strtabHdr->sh_size;
    newStrtabHdr->sh_name = STRTAB_NAME;
    newStrtabHdr->sh_addr = newStrtabHdr->sh_offset + baseAddr;
    newStrtabHdr->sh_addralign = 1;
    newStrtabHdr->sh_type = SHT_STRTAB;
    newStrtabHdr->sh_flags = SHF_ALLOC;
    newStrtabHdr->sh_link = SHSTRTAB_HDR_IDX;

    *newSymHdr = *info.symtabHdr;
    newSymHdr->sh_offset = reinterpret_cast<uintptr_t>(newSymtab) - baseAddr;
    newSymHdr->sh_size = info.symCnt * info.symtabHdr->sh_entsize;
    newSymHdr->sh_entsize = info.symtabHdr->sh_entsize;
    newSymHdr->sh_addralign = info.symtabHdr->sh_addralign;
    newSymHdr->sh_name = SYMTAB_NAME;
    newSymHdr->sh_addr = reinterpret_cast<uintptr_t>(newSymtab);
    newSymHdr->sh_link = STRTAB_HDR_IDX;

    newTextHdr->sh_offset = 0;
    newTextHdr->sh_size = info.asmstubHdr->sh_offset + info.asmstubHdr->sh_size - info.textHdr->sh_offset;
    newTextHdr->sh_name = TEXT_NAME;
    newTextHdr->sh_addr = info.fileAddr + info.textHdr->sh_offset;
    newTextHdr->sh_addralign = sizeof(uint32_t);
    newTextHdr->sh_type = SHT_NOBITS;
    newTextHdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    newTextHdr->sh_link = SYMTAB_HDR_IDX;

    ehFrameHdr->sh_offset = reinterpret_cast<uintptr_t>(ehFrame) - baseAddr;
    ehFrameHdr->sh_size = ehFrameSize;
    ehFrameHdr->sh_name = EH_FRAME_NAME;
    ehFrameHdr->sh_addr = reinterpret_cast<uintptr_t>(ehFrame);
    ehFrameHdr->sh_addralign = sizeof(uintptr_t);
    ehFrameHdr->sh_type = SHT_PROGBITS;
    ehFrameHdr->sh_flags = SHF_ALLOC;
    ehFrameHdr->sh_link = TEXT_HDR_IDX;
}

bool CreateDebuggerElf(uintptr_t fileAddr, void **result, uint64_t *elfSize)
{
    auto info = CollectStubAnInfo(fileAddr);
    std::vector<uint8_t> ehFrame {
            0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x4, 0x78, 0x1e, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x4b, 0x0, 0x0, 0x0, 0x18, 0x0, 0x0, 0x0, 0x0, 0xe0, 0x73, 0xab,
            0xff, 0xff, 0x0, 0x0, 0x30, 0x2c, 0x12, 0x0, 0x0, 0x0, 0x0, 0x0, 0xc, 0x1d, 0x0, 0x10,
            0x1e, 0x17, 0x8d, 0x0, 0x12, 0x8, 0x18, 0x1c, 0x6, 0x8, 0x0, 0x29, 0x28, 0x7, 0x0, 0x8,
            0x10, 0x1c, 0x6, 0x2f, 0xee, 0xff, 0x8, 0x8, 0x22, 0x10, 0x1d, 0x17, 0x8d, 0x0, 0x12, 0x8,
            0x18, 0x1c, 0x6, 0x8, 0x0, 0x29, 0x28, 0x7, 0x0, 0x8, 0x10, 0x1c, 0x6, 0x2f, 0xee, 0xff,
            0x8, 0x0, 0x22,
    };
    const uint32_t addrOff = 28;
    const uint32_t lenOff = 36;
    auto writeU64 = [&ehFrame](uint32_t idx, uint64_t data) {
        for (uint32_t i = 0; i < sizeof(uint64_t); i++) {
            ehFrame[idx + i] = (data >> (8 * i)) & 0xff;
        }
    };
    writeU64(addrOff, info.bcStubBegin + fileAddr);
    ASSERT(info.bcStubEnd >= info.bcStubBegin);
    writeU64(lenOff, info.bcStubEnd - info.bcStubBegin);

    uint32_t totalSize = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + sizeof(SHSTR) + info.strtabHdr->sh_size;
    totalSize += info.symtabHdr->sh_entsize * info.symCnt + sizeof(Elf64_Sym); // for align
    totalSize += ehFrame.size() + sizeof(uintptr_t);
    totalSize += sizeof(Elf64_Shdr) * HEADER_CNT + sizeof(Elf64_Shdr); // for align

    char *buffer = new char[totalSize];
    Elf64_Ehdr *newEhdr = reinterpret_cast<Elf64_Ehdr *>(buffer);
    Elf64_Phdr *newPhdr = reinterpret_cast<Elf64_Phdr *>(newEhdr + 1);
    const char *shStrBuff = reinterpret_cast<const char *>(newPhdr + 1);
    const char *newStrtab = shStrBuff + sizeof(SHSTR);
    if (!CopyStrTab(reinterpret_cast<uintptr_t>(buffer), info)) {
        delete[] buffer;
        return false;
    }

    auto newSymtab = OffsetAlignUp<Elf64_Sym *>(newStrtab, info.strtabHdr->sh_size, info.symtabHdr->sh_addralign);
    ConstructSymTab(newSymtab, info);

    auto ehFrameBuffer = OffsetAlignUp<char *>(newSymtab, info.symtabHdr->sh_entsize * info.symCnt, sizeof(uintptr_t));
    if (memcpy_s(ehFrameBuffer, ehFrame.size(), ehFrame.data(), ehFrame.size()) != EOK) {
        delete[] buffer;
        return false;
    }

    auto newShdrtab = OffsetAlignUp<Elf64_Shdr *>(ehFrameBuffer, ehFrame.size(), sizeof(uintptr_t));
    ConstructEhdrAndPhdr(newEhdr, newShdrtab, reinterpret_cast<uintptr_t>(buffer), info);
    ConstructShdrTab(newShdrtab, newSymtab, reinterpret_cast<uintptr_t>(buffer), ehFrameBuffer, ehFrame.size(), info);

    *result = reinterpret_cast<void *>(buffer);
    *elfSize = totalSize;
    return true;
}

static bool RegisterStubAnToDebuggerImpl(const char *fileAddr)
{
    auto *entry = new jit_code_entry;
    if (!CreateDebuggerElf(reinterpret_cast<uintptr_t>(fileAddr),
        reinterpret_cast<void **>(const_cast<char **>(&entry->symfile_addr)), &entry->symfile_size)) {
        delete entry;
        return false;
    }
    entry->prev_entry = nullptr;
    entry->file_addr = fileAddr;

    // Insert this entry at the head of the list.
    jit_code_entry *nextEntry = __jit_debug_descriptor.first_entry;
    entry->next_entry = nextEntry;
    if (nextEntry) {
        nextEntry->prev_entry = entry;
    }

    __jit_debug_descriptor.first_entry = entry;
    __jit_debug_descriptor.relevant_entry = entry;
    __jit_debug_descriptor.action_flag = JIT_REGISTER_FN;
    __jit_debug_register_code();
    return true;
}

}
}
#else
namespace panda::ecmascript {
namespace jit_debug {
void RegisterStubAnToDebugger(const char *fileAddr)
{
    LOG_COMPILER(INFO) << "MACOS doesn't support RegisterStubAnToDebugger, fileAddr is" << fileAddr;
}

void UnregisterStubAnFromDebugger(const char *fileAddr)
{
    LOG_COMPILER(INFO) << "MACOS doesn't support RegisterStubAnToDebugger, fileAddr is" << fileAddr;
}
}
}
#endif
