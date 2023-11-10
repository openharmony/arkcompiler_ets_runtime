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

#ifndef MAPLEBE_INCLUDE_CG_IFILE_TYPE_H
#define MAPLEBE_INCLUDE_CG_IFILE_TYPE_H
#include <cstdint>

namespace maplebe {
// The following definitions are excerpted from the elf.h
#define EI_NIDENT (16)
#define EI_MAG0 0                      /* File identification byte 0 index */
#define EI_MAG1 1                      /* File identification byte 1 index */
#define EI_MAG2 2                      /* File identification byte 2 index */
#define EI_MAG3 3                      /* File identification byte 3 index */
#define EI_CLASS 4                     /* File class byte index */
#define EI_DATA 5                      /* Data encoding byte index */
#define EI_VERSION 6                   /* File version byte index */
#define EI_OSABI 7                     /* OS ABI identification */
#define EI_ABIVERSION 8                /* ABI version */
#define EI_PAD 9                       /* Byte index of padding bytes */
#define ELFCLASS64 2                   /* 64-bit objects */
#define ELFDATA2LSB 1                  /* 2's complement, little endian */
#define ELFMAG0 0x7f                   /* Magic number byte 0 */
#define ELFOSABI_LINUX 3               /* Compatibility alias.  */
#define ELFOSABI_NONE 0                /* UNIX System V ABI */
#define ELFMAG1 'E'                    /* Magic number byte 1 */
#define ELFMAG2 'L'                    /* Magic number byte 2 */
#define ELFMAG3 'F'                    /* Magic number byte 3 */
#define EM_AARCH64 183                 /* ARM AARCH64 */
#define EM_X86_64 62                   /* AMD x86-64 architecture */
#define ET_REL 1                       /* Relocatable file */
#define EV_CURRENT 1                   /* Current version */
#define R_AARCH64_ADR_PREL_PG_HI21 275 /* Page-rel. ADRP imm. from 32:12.  */
#define R_AARCH64_ADD_ABS_LO12_NC 277  /* Dir. ADD imm. from bits 11:0.  */
#define R_AARCH64_CALL26 283           /* Likewise for CALL.  */
#define R_X86_64_32 10                 /* Direct 32 bit zero extended */
#define R_X86_64_64 1                  /* Direct 64 bit  */
#define R_X86_64_NONE 0                /* No reloc */
#define R_X86_64_PC32 2                /* PC relative 32 bit signed */
#define R_X86_64_PC64 24               /* PC relative 64 bit */
#define R_X86_64_PLT32 4               /* 32 bit PLT address */
#define SHF_ALLOC (1 << 1)             /* Occupies memory during execution */
#define SHF_EXECINSTR (1 << 2)         /* Executable */
#define SHF_INFO_LINK (1 << 6)         /* `sh_info' contains SHT index */
#define SHF_WRITE (1 << 0)             /* Writable */
#define SHF_MASKPROC 0xf0000000        /* Processor-specific */
#define SHN_COMMON 0xfff2              /* Associated symbol is common */
#define SHT_NULL 0                     /* Section header table entry unused */
#define SHT_PROGBITS 1                 /* Program data */
#define SHT_SYMTAB 2                   /* Symbol table */
#define SHT_STRTAB 3                   /* String table */
#define SHT_RELA 4                     /* Relocation entries with addends */
#define SHT_NOBITS 8                   /* Program space with no data (bss) */
#define STB_LOCAL 0                    /* Local symbol */
#define STB_GLOBAL 1                   /* Global symbol */
#define STB_WEAK 2                     /* Weak symbol */
#define STT_NOTYPE 0                   /* Symbol type is unspecified */
#define STT_OBJECT 1                   /* Symbol is a data object */
#define STT_FUNC 2                     /* Symbol is a code object */
#define STT_SECTION 3                  /* Symbol associated with a section */

using Address = uint64_t;
using Offset = uint64_t;
using Word = uint32_t;
using Xword = uint64_t;
using SectionIndex = uint16_t;
using Sxword = int64_t;

typedef struct {
    unsigned char e_ident[EI_NIDENT]; /* ELF "magic number" */
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry; /* Entry point virtual address */
    uint64_t e_phoff; /* Program header table file offset */
    uint64_t e_shoff; /* Section header table file offset */
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} FileHeader;

typedef struct {
    uint32_t sh_name;      /* Section name, index in string tbl */
    uint32_t sh_type;      /* Type of section */
    uint64_t sh_flags;     /* Miscellaneous section attributes */
    uint64_t sh_addr;      /* Section virtual addr at execution */
    uint64_t sh_offset;    /* Section file offset */
    uint64_t sh_size;      /* Size of section in bytes */
    uint32_t sh_link;      /* Index of another section */
    uint32_t sh_info;      /* Additional section information */
    uint64_t sh_addralign; /* Section alignment */
    uint64_t sh_entsize;   /* Entry size if section holds table */
} SectionHeader;

typedef struct {
    uint32_t st_name;       /* Symbol name, index in string tbl */
    unsigned char st_info;  /* Type and binding attributes */
    unsigned char st_other; /* No defined meaning, 0 */
    uint16_t st_shndx;      /* Associated section index */
    uint64_t st_value;      /* Value of the symbol */
    uint64_t st_size;       /* Associated symbol size */
} Symbol;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} SegmentHeader;

typedef struct {
    int64_t d_tag; /* entry tag value */
    union {
        uint64_t d_val;
        uint64_t d_ptr;
    } d_un;
} DynSectionEntry;

typedef struct {
    uint64_t r_offset; /* Location at which to apply the action */
    uint64_t r_info;   /* index and type of relocation */
    int64_t r_addend;  /* Constant addend used to compute value */
} Rela;
}  // namespace maplebe
#endif /* MAPLEBE_INCLUDE_CG_IFILE_TYPE_H */