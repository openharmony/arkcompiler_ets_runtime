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

#include "ecmascript/compiler/aot_file/gdb_jit.h"
#include "llvm/BinaryFormat/ELF.h"
#include "ecmascript/log_wrapper.h"

#include <vector>
#include <cstring>

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
void __attribute__((noinline)) __jit_debug_register_code() {
    // The noinline and the asm prevent calls to this function from being
    // optimized out.
#if !defined(_MSC_VER)
    asm volatile("" ::: "memory");
#endif
}
}

namespace panda::ecmascript {
namespace jit_debug {
using namespace llvm::ELF;

static bool RegisterStubAnToDebuggerImpl(const char *fileAddr);

void RegisterStubAnToDebugger(const char *fileAddr)
{
    if (RegisterStubAnToDebuggerImpl(fileAddr)) {
        LOG_COMPILER(INFO) << "success to register stub.an to debugger.";
    } else {
        LOG_COMPILER(ERROR) << "Can't register stub.an to debugger.";
    }
}

#define ALIGN_UP(addr, align) ((void *)(((uintptr_t)(addr)) % (align) == 0 ? \
    (uintptr_t)(addr) : (uintptr_t)(addr) - ((uintptr_t)(addr)) % (align) + (align)))

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

const char g_shstr[] = "\0.shstrtab\0.strtab\0.symtab\0.text\0.eh_frame";

StubAnInfo CollectStubAnInfo(uintptr_t fileAddr)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)fileAddr;
    Elf64_Shdr *shdrTab = (Elf64_Shdr *)(fileAddr + ehdr->e_shoff);
    uint32_t shStrIdx = ehdr->e_shstrndx;
    Elf64_Shdr *shStrHdr = &shdrTab[shStrIdx];
    const char *shstrtab = (const char *)(fileAddr + shStrHdr->sh_offset);
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
    Elf64_Sym *symtab = (Elf64_Sym *)(fileAddr + symtabHdr->sh_offset);
    const char *strtab = (const char *)(fileAddr + strtabHdr->sh_offset);
    uint32_t symCnt = 2;
    uint64_t bcStubBegin = UINT64_MAX;
    uint64_t bcStubEnd = 0;
    for (uint32_t symIdx = 0; symIdx < symtabHdr->sh_size / symtabHdr->sh_entsize; symIdx++) {
        Elf64_Sym *sym = symtab + symIdx;
        if ((sym->st_info >> 4) != STB_GLOBAL) {
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

bool CopyStrTab(void *buffer, const StubAnInfo &info)
{
    Elf64_Ehdr *newEhdr = (Elf64_Ehdr *)(buffer);
    Elf64_Phdr *newPhdr = (Elf64_Phdr *)(newEhdr + 1);
    const char *shStrBuff = (const char *)(newPhdr + 1);
    if (memcpy((void *) shStrBuff, g_shstr, sizeof(g_shstr)) == 0) {
        return false;
    }
    const char *newStrtab = shStrBuff + sizeof(g_shstr);
    if (memcpy((void *) newStrtab, (void *)(info.fileAddr + info.strtabHdr->sh_offset), info.strtabHdr->sh_size) == 0) {
        return false;
    }
    const char bcStubName[] = "BCStubInterpreterRoutine";
    if (memcpy((void *)(newStrtab + 1), (void *)bcStubName, sizeof(bcStubName)) == 0) {
        return false;
    }
    return true;
}

void ConstructSymTab(Elf64_Sym *newSymtab, const StubAnInfo &info)
{
    Elf64_Sym *symtab = (Elf64_Sym *)(info.fileAddr + info.symtabHdr->sh_offset);
    const char *strtab = (const char *)(info.fileAddr + info.strtabHdr->sh_offset);
    memset((void *)newSymtab, 0, sizeof(Elf64_Sym));
    uint32_t newSymIdx = 1;
    for (uint32_t symIdx = 0; symIdx < info.symtabHdr->sh_size / info.symtabHdr->sh_entsize; symIdx++) {
        Elf64_Sym *src = symtab + symIdx;
        if ((src->st_info >> 4) == STB_GLOBAL
            && strncmp(&strtab[src->st_name], "BCStub", strlen("BCStub")) != 0) {
            auto dst = newSymtab + newSymIdx;
            newSymIdx++;
            *dst = *src;
            dst->st_shndx = 4;
            dst->st_value -= info.textHdr->sh_offset;
        }
    }
    auto bcSym = newSymtab + newSymIdx;
    bcSym->st_name = 1;
    bcSym->st_info = newSymtab[1].st_info;
    bcSym->st_other = newSymtab[1].st_other;
    bcSym->st_shndx = 4;
    bcSym->st_value = info.bcStubBegin - info.textHdr->sh_offset;
    bcSym->st_size = info.bcStubEnd - info.bcStubBegin;
}

void ConstructEhdrAndPhdr(Elf64_Ehdr *newEhdr, Elf64_Shdr *newShdrtab, char *buffer, const StubAnInfo &info)
{
    Elf64_Phdr *newPhdr = (Elf64_Phdr *)(newEhdr + 1);
    {
        *newEhdr = *info.ehdr;
        newEhdr->e_flags = info.ehdr->e_flags;
        newEhdr->e_machine = info.ehdr->e_machine;
        memcpy(newEhdr->e_ident, info.ehdr->e_ident, sizeof(info.ehdr->e_ident));
        newEhdr->e_version = 1;
        newEhdr->e_phoff = sizeof(Elf64_Ehdr);
        newEhdr->e_shoff = (uintptr_t)newShdrtab - (uintptr_t)buffer;
        newEhdr->e_ehsize = sizeof(Elf64_Ehdr);
        newEhdr->e_phentsize = sizeof(Elf64_Phdr);
        newEhdr->e_phnum = 1;
        newEhdr->e_shentsize = sizeof(Elf64_Shdr);
        newEhdr->e_shnum = 6;
        newEhdr->e_shstrndx = 1;
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

void ConstructShdrTab(Elf64_Shdr *newShdrTab, Elf64_Sym *newSymtab, char *buffer, void *ehFrame, uint32_t ehFrameSize,
                      const StubAnInfo &info)
{
    Elf64_Shdr hdr{};
    Elf64_Shdr *emptyShdr = newShdrTab;
    Elf64_Shdr *newShstrHdr = emptyShdr + 1;
    Elf64_Shdr *newStrtabHdr = newShstrHdr + 1;
    Elf64_Shdr *newSymHdr = newStrtabHdr + 1;
    Elf64_Shdr *newTextHdr = newSymHdr + 1;
    Elf64_Shdr *ehFrameHdr = newTextHdr + 1;
    *emptyShdr = hdr;
    {
        *newShstrHdr = hdr;
        newShstrHdr->sh_offset = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
        newShstrHdr->sh_size = sizeof(g_shstr);
        newShstrHdr->sh_name = 1;
        newShstrHdr->sh_addr = newStrtabHdr->sh_offset + (uintptr_t)buffer;
        newShstrHdr->sh_type = SHT_STRTAB;
        newShstrHdr->sh_flags = SHF_ALLOC;
    }

    {
        *newStrtabHdr = hdr;
        newStrtabHdr->sh_offset = newShstrHdr->sh_offset + newShstrHdr->sh_size;
        newStrtabHdr->sh_size = info.strtabHdr->sh_size;
        newStrtabHdr->sh_name = 11;
        newStrtabHdr->sh_addr = newStrtabHdr->sh_offset + (uintptr_t)buffer;
        newStrtabHdr->sh_addralign = 1;
        newStrtabHdr->sh_type = SHT_STRTAB;
        newStrtabHdr->sh_flags = SHF_ALLOC;
        newStrtabHdr->sh_link = 0;
    }

    {
        *newSymHdr = *info.symtabHdr;
        newSymHdr->sh_offset = (uintptr_t)newSymtab - (uintptr_t)buffer;
        newSymHdr->sh_size = info.symCnt * info.symtabHdr->sh_entsize;
        newSymHdr->sh_entsize = info.symtabHdr->sh_entsize;
        newSymHdr->sh_addralign = info.symtabHdr->sh_addralign;
        newSymHdr->sh_name = 19;
        newSymHdr->sh_addr = (uintptr_t)newSymtab;
        newSymHdr->sh_link = 2;
    }

    {
        *newTextHdr = hdr;
        newTextHdr->sh_offset = 0;
        newTextHdr->sh_size = info.asmstubHdr->sh_offset + info.asmstubHdr->sh_size - info.textHdr->sh_offset;
        newTextHdr->sh_name = 27;
        newTextHdr->sh_addr = info.fileAddr + info.textHdr->sh_offset;
        newTextHdr->sh_addralign = 0x10;
        newTextHdr->sh_type = SHT_NOBITS;
        newTextHdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
        newTextHdr->sh_link = 3;
    }

    {
        ehFrameHdr->sh_offset = (uintptr_t)ehFrame - (uintptr_t )buffer;
        ehFrameHdr->sh_size = ehFrameSize;
        ehFrameHdr->sh_name = 33;
        ehFrameHdr->sh_addr = (uintptr_t)ehFrame;
        ehFrameHdr->sh_addralign = 8;
        ehFrameHdr->sh_type = SHT_PROGBITS;
        ehFrameHdr->sh_flags = SHF_ALLOC;
        ehFrameHdr->sh_link = 4;
    }
}

bool CreateDebuggerElf(uintptr_t fileAddr, void **result, uint64_t *elfSize)
{
    auto info = CollectStubAnInfo(fileAddr);
    std::vector<uint8_t> ehFrame {
            0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            0x1, 0x0, 0x4, 0x78, 0x1e, 0x0, 0x0, 0x0,
            0x0, 0x0, 0x0, 0x0, 0x4b, 0x0, 0x0, 0x0,
            0x18, 0x0, 0x0, 0x0, 0x0, 0xe0, 0x73, 0xab,
            0xff, 0xff, 0x0, 0x0, 0x30, 0x2c, 0x12, 0x0,
            0x0, 0x0, 0x0, 0x0, 0xc, 0x1d, 0x0, 0x10,
            0x1e, 0x17, 0x8d, 0x0, 0x12, 0x8, 0x18, 0x1c,
            0x6, 0x8, 0x0, 0x29, 0x28, 0x7, 0x0, 0x8,
            0x10, 0x1c, 0x6, 0x2f, 0xee, 0xff, 0x8, 0x8,
            0x22, 0x10, 0x1d, 0x17, 0x8d, 0x0, 0x12, 0x8,
            0x18, 0x1c, 0x6, 0x8, 0x0, 0x29, 0x28, 0x7,
            0x0, 0x8, 0x10, 0x1c, 0x6, 0x2f, 0xee, 0xff,
            0x8, 0x0, 0x22,
    };
    const uint32_t addrOff = 28;
    const uint32_t lenOff = 36;
    auto writeU64 = [&](uint32_t idx, uint64_t data) {
        for (uint32_t i = 0; i < sizeof(uint64_t); i++) {
            ehFrame[idx + i] = (data >> (8 * i)) & 0xff;
        }
    };
    writeU64(addrOff, info.bcStubBegin + fileAddr);
    writeU64(lenOff, info.bcStubEnd - info.bcStubBegin);
    /*
     * [0] file header
     * [1] program header
     * [2] shstrtab
     * [3] strtab
     * [4] symtab
     * [5] .eh_frame
     * [6] shstrtab-header
     * [7] strtab-header
     * [8] symtab-header
     * [9] text-header
     * [10] .eh_frame header
     */
    uint32_t totalSize = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
    totalSize += sizeof(g_shstr);
    totalSize += info.strtabHdr->sh_size;
    totalSize += info.symtabHdr->sh_entsize * info.symCnt + sizeof(Elf64_Sym); // for align
    totalSize += ehFrame.size() + sizeof(uintptr_t);
    totalSize += sizeof(Elf64_Shdr) * 6 + sizeof(Elf64_Shdr); // for align

    char *buffer = new char[totalSize];
    Elf64_Ehdr *newEhdr = (Elf64_Ehdr *)(buffer);
    Elf64_Phdr *newPhdr = (Elf64_Phdr *)(newEhdr + 1);
    const char *shStrBuff = (const char *)(newPhdr + 1);
    const char *newStrtab = shStrBuff + sizeof(g_shstr);
    if (!CopyStrTab(buffer, info)) {
        delete[] buffer;
        return false;
    }

    Elf64_Sym *newSymtab = (Elf64_Sym *) ALIGN_UP((uintptr_t)newStrtab + info.strtabHdr->sh_size, info.symtabHdr->sh_addralign);
    ConstructSymTab(newSymtab, info);

    void *ehFrameBuffer = (void *)((uintptr_t)newSymtab + info.symtabHdr->sh_entsize * info.symCnt);
    ehFrameBuffer = (void *) ALIGN_UP(ehFrameBuffer, 8);
    if (memcpy(ehFrameBuffer, ehFrame.data(), ehFrame.size()) == nullptr) {
        delete[] buffer;
        return false;
    }

    Elf64_Shdr *newShdrtab = (Elf64_Shdr *)((uintptr_t)ehFrameBuffer + ehFrame.size());
    newShdrtab = (Elf64_Shdr *)ALIGN_UP(newShdrtab, 8);

    ConstructEhdrAndPhdr(newEhdr, newShdrtab, buffer, info);
    ConstructShdrTab(newShdrtab, newSymtab, buffer, ehFrameBuffer, ehFrame.size(), info);

    *result = (void *)buffer;
    *elfSize = totalSize;
    return true;
}

static bool RegisterStubAnToDebuggerImpl(const char *fileAddr)
{
    auto *entry = new jit_code_entry;
    if (!CreateDebuggerElf((uintptr_t)fileAddr, (void **)&entry->symfile_addr, &entry->symfile_size)) {
        return false;
    }
    entry->prev_entry = nullptr;

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