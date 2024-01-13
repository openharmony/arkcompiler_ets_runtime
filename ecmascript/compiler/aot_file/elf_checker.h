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

#ifndef ECMASCRIPT_COMPILER_AOT_FILE_ELF_CHECKER_H
#define ECMASCRIPT_COMPILER_AOT_FILE_ELF_CHECKER_H

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/compiler/binary_section.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <unistd.h>

#if BYTE_ORDER == LITTLE_ENDIAN
#define ELF_BYTEORDER llvm::ELF::ELFDATA2LSB
#else
#define ELF_BYTEORDER llvm::ELF::ELFDATA2MSB
#endif
#if (defined __i386__ || defined __x86_64__) && !CHECK_UNDEFINED
#define ALLOW_UNALIGNED 1
#else
#define ALLOW_UNALIGNED 0
#endif

namespace panda::ecmascript {
class ElfChecker {
public:
    enum { ELF_F_MMAPPED = 0x40, ELF_F_MALLOCED = 0x80, ELF_F_FILEDATA = 0x100 };

    /* Identification values for recognized object files.  */
    enum ElfKind {
        ELF_KIND_NONE, /* Unknown.  */
        ELF_KIND_COFF, /* Stupid old COFF.  */
        ELF_KIND_ELF, /* ELF file.  */
        /* Keep this the last entry.  */
        ELF_KIND_NUM
    };

    /* Commands for `...'.  */
    enum ElfCommand {
        ELF_CMD_NULL, /* Nothing, terminate, or compute only.  */
        ELF_CMD_READ, /* Read .. */
        ELF_CMD_RDWR, /* Read and write .. */
        ELF_CMD_WRITE, /* Write .. */
        ELF_CMD_CLR, /* Clear flag.  */
        ELF_CMD_SET, /* Set flag.  */
        ELF_CMD_FDDONE, /* Signal that file descriptor will not be
                 used anymore.  */
        ELF_CMD_FDREAD, /* Read rest of data so that file descriptor
                 is not used anymore.  */
        /* The following are extensions.  */
        ELF_CMD_READ_MMAP, /* Read, but mmap the file if possible.  */
        ELF_CMD_RDWR_MMAP, /* Read and write, with mmap.  */
        ELF_CMD_WRITE_MMAP, /* Write, with mmap.  */
        ELF_CMD_READ_MMAP_PRIVATE, /* Read, but memory is writable, results are
                        not written to the file.  */
        ELF_CMD_EMPTY, /* Copy basic file data but not the content. */
        /* Keep this the last entry.  */
        ELF_CMD_NUM
    };

    /* Known translation types.  */
    enum ElfType {
        ELF_TYPE_BYTE, /* unsigned char */
        ELF_TYPE_ADDR, /* Elf32_Addr, Elf64_Addr, ... */
        ELF_TYPE_DYN, /* Dynamic section record.  */
        ELF_TYPE_EHDR, /* ELF header.  */
        ELF_TYPE_HALF, /* Elf32_Half, Elf64_Half, ... */
        ELF_TYPE_OFF, /* Elf32_Off, Elf64_Off, ... */
        ELF_TYPE_PHDR, /* Program header.  */
        ELF_TYPE_RELA, /* Relocation entry with addend.  */
        ELF_TYPE_REL, /* Relocation entry.  */
        ELF_TYPE_SHDR, /* Section header.  */
        ELF_TYPE_SWORD, /* Elf32_Sword, Elf64_Sword, ... */
        ELF_TYPE_SYM, /* Symbol record.  */
        ELF_TYPE_WORD, /* Elf32_Word, Elf64_Word, ... */
        ELF_TYPE_XWORD, /* Elf32_Xword, Elf64_Xword, ... */
        ELF_TYPE_SXWORD, /* Elf32_Sxword, Elf64_Sxword, ... */
        ELF_TYPE_VDEF, /* Elf32_Verdef, Elf64_Verdef, ... */
        ELF_TYPE_VDAUX, /* Elf32_Verdaux, Elf64_Verdaux, ... */
        ELF_TYPE_VNEED, /* Elf32_Verneed, Elf64_Verneed, ... */
        ELF_TYPE_VNAUX, /* Elf32_Vernaux, Elf64_Vernaux, ... */
        ELF_TYPE_NHDR, /* Elf32_Nhdr, Elf64_Nhdr, ... */
        ELF_TYPE_SYMINFO, /* Elf32_Syminfo, Elf64_Syminfo, ... */
        ELF_TYPE_MOVE, /* Elf32_Move, Elf64_Move, ... */
        ELF_TYPE_LIB, /* Elf32_Lib, Elf64_Lib, ... */
        ELF_TYPE_GNUHASH, /* GNU-style hash section.  */
        ELF_TYPE_AUXV, /* Elf32_auxv_t, Elf64_auxv_t, ... */
        ELF_TYPE_CHDR, /* Compressed, Elf32_Chdr, Elf64_Chdr, ... */
        ELF_TYPE_NHDR8, /* Special GNU Properties note.  Same as Nhdr,
                except padding.  */
        /* Keep this the last entry.  */
        ELF_TYPE_NUM
    };

    /* Error values.  */
    enum {
        ELF_ERR_NOERROR = 0,
        ELF_ERR_UNKNOWN_ERROR,
        ELF_ERR_UNKNOWN_VERSION,
        ELF_ERR_UNKNOWN_TYPE,
        ELF_ERR_INVALID_HANDLE,
        ELF_ERR_SOURCE_SIZE,
        ELF_ERR_DEST_SIZE,
        ELF_ERR_INVALID_ENCODING,
        ELF_ERR_NOMEM,
        ELF_ERR_INVALID_FILE,
        ELF_ERR_INVALID_ELF,
        ELF_ERR_INVALID_OP,
        ELF_ERR_NO_VERSION,
        ELF_ERR_INVALID_CMD,
        ELF_ERR_RANGE,
        ELF_ERR_ARCHIVE_FMAG,
        ELF_ERR_INVALID_ARCHIVE,
        ELF_ERR_NO_ARCHIVE,
        ELF_ERR_NO_INDEX,
        ELF_ERR_READ_ERROR,
        ELF_ERR_WRITE_ERROR,
        ELF_ERR_INVALID_CLASS,
        ELF_ERR_INVALID_INDEX,
        ELF_ERR_INVALID_OPERAND,
        ELF_ERR_INVALID_SECTION,
        ELF_ERR_INVALID_COMMAND,
        ELF_ERR_WRONG_ORDER_EHDR,
        ELF_ERR_FD_DISABLED,
        ELF_ERR_FD_MISMATCH,
        ELF_ERR_OFFSET_RANGE,
        ELF_ERR_NOT_NUL_SECTION,
        ELF_ERR_DATA_MISMATCH,
        ELF_ERR_INVALID_SECTION_HEADER,
        ELF_ERR_INVALID_DATA,
        ELF_ERR_DATA_ENCODING,
        ELF_ERR_SECTION_TOO_SMALL,
        ELF_ERR_INVALID_ALIGN,
        ELF_ERR_INVALID_SHENTSIZE,
        ELF_ERR_UPDATE_RO,
        ELF_ERR_NOFILE,
        ELF_ERR_GROUP_NOT_REL,
        ELF_ERR_INVALID_PHDR,
        ELF_ERR_NO_PHDR,
        ELF_ERR_INVALID_OFFSET,
        ELF_ERR_INVALID_SECTION_TYPE,
        ELF_ERR_INVALID_SECTION_FLAGS,
        ELF_ERR_NOT_COMPRESSED,
        ELF_ERR_ALREADY_COMPRESSED,
        ELF_ERR_UNKNOWN_COMPRESSION_TYPE,
        ELF_ERR_COMPRESS_ERROR,
        ELF_ERR_DECOMPRESS_ERROR,
        /* Keep this as the last entry.  */
        ELF_ERR_NUM
    };

    /* Descriptor for data to be converted to or from memory format.  */
    struct ElfData {
        void* dataBuffer; /* Pointer to the actual data.  */
        ElfType dataType; /* Type of this piece of data.  */
        unsigned int dataVersion; /* ELF version.  */
        size_t dataSize; /* Size in bytes.  */
        int64_t sectionOffset; /* Offset into section.  */
        size_t sectionAlignment; /* Alignment in section.  */
    };

    struct ElfSectionBase;
    struct ElfSectionList;
    struct Elf;

    /* The visible `ElfData' type is not sufficient for some operations due
       to a misdesigned interface.  Extend it for internal purposes.  */
    struct ElfSectionData {
        ElfData data;
        ElfSectionBase* sectionPtr;
    };

    /* List of `ElfData' descriptors.  This is what makes up the section
       contents.  */
    struct ElfDataList {
        /* `data' *must* be the first element in the struct.  */
        ElfSectionData data;
        struct ElfDataList* next;
        int flags;
    };

    /* Descriptor for ELF section.  */
    struct ElfSectionBase {
        /* Describe for elf sections */
        ElfDataList dataList; /* List of data buffers.  */
        ElfDataList* dataListRear; /* Pointer to the rear of the data list. */

        ElfSectionData rawData; /* Uninterpreted data of the section.  */

        int dataRead; /* Nonzero if the section was created by the
                      user or if the data from the file/memory
                      is read.  */
        int extendSectionHeaderIndex; /* Index of the extended section index
                  table for this symbol table (if this
                  section is a symbol table).  */

        size_t index; /* Index of this section.  */
        struct Elf* elf; /* The underlying ELF file.  */

        union {
            llvm::ELF::Elf32_Shdr* e32; /* Pointer to 32bit section header.  */
            llvm::ELF::Elf64_Shdr* e64; /* Pointer to 64bit section header.  */
        } shdr;

        unsigned int sectionHeaderFlags; /* Section header modified?  */
        unsigned int flags; /* Section changed in size? */

        char* rawDataBase; /* The unmodified data of the section.  */
        char* dataBase; /* The converted data of the section.  */

        char* zipDataBase; /* The uncompressed data of the section.  */
        size_t zipDataSize; /* If zipDataBase != NULL, the size of data.  */
        size_t zipDataAlign; /* If zipDataBase != NULL, the addralign.  */

        ElfSectionList* list; /* Pointer to the section list element the
                         data is in.  */
    };

    /* List of section.  */
    struct ElfSectionList {
        unsigned int cnt; /* Number of elements of 'data' used.  */
        unsigned int max; /* Number of elements of 'data' allocated.  */
        struct ElfSectionList* next; /* Next block of sections.  */
        ElfSectionBase* data()
        {
            return reinterpret_cast<ElfSectionBase*>(reinterpret_cast<char*>(this) + sizeof(ElfSectionList));
        }
    };

    /* elf_getdata_rawchunk result.  */
    struct ElfDataChunk {
        ElfSectionData data;
        union {
            ElfSectionBase dummySection;
            struct ElfDataChunk* next;
        };
        int64_t offset; /* The original raw offset in the Elf image.  */
    };

    struct Elf {
        /* Address to which the file was mapped.  NULL if not mapped.  */
        void* mapAddress;

        /* When created for an archive member this points to the descriptor
           for the archive. */
        Elf* parent;
        Elf* next; /* Used in list of archive descriptors.  */

        /* What kind of file is underneath (ELF file, archive...).  */
        ElfKind kind;

        /* Command used to create this descriptor.  */
        ElfCommand cmd;

        /* The binary class.  */
        unsigned int binaryClass;

        /* The used file descriptor.  -1 if not available anymore.  */
        int fildes;

        /* Offset in the archive this file starts or zero.  */
        int64_t startOffset;

        /* Size of the file in the archive or the entire file size, or ~0
           for an (yet) unknown size.  */
        size_t maxSize;

        /* Describes the way the memory was allocated and if the dirty bit is
           signalled it means that the whole file has to be rewritten since
           the layout changed.  */
        int flags;

        /* Reference counting for the descriptor.  */
        int refCount;

        union {
            struct {
                void* elfHeaderPtr;
                void* secHeaderPtr;
                void* proHeaderPtr;

                ElfSectionList* sectionLast; /* Last element in the section list.
                             If NULL the data has not yet been
                             read from the file.  */
                ElfDataChunk* rawChunks; /* List of elf_getdata_rawchunk results.  */
                unsigned int scnincr; /* Number of sections allocate the last
                             time.  */
                int elfHeaderFlags; /* Flags (dirty) for ELF header.  */
                int progHeaderFlags; /* Flags (dirty|malloc) for program header.  */
                int secHeaderMalloced; /* Nonzero if shdr array was allocated.  */
                off_t sizeStrOffset; /* Offset of the size string in the parent
                             if this is an archive member.  */
            } elf;

            struct {
                llvm::ELF::Elf32_Ehdr* ehdr; /* Pointer to the ELF header.  This is
                       never malloced.  */
                llvm::ELF::Elf32_Shdr* shdr; /* Used when reading from a file.  */
                llvm::ELF::Elf32_Phdr* phdr; /* Pointer to the program header array.  */
                ElfSectionList* sectionLast; /* Last element in the section list.
                             If NULL the data has not yet been
                             read from the file.  */
                ElfDataChunk* rawChunks; /* List of elf_getdata_rawchunk results.  */
                unsigned int scnincr; /* Number of sections allocate the last
                             time.  */
                int elfHeaderFlags; /* Flags (dirty) for ELF header.  */
                int progHeaderFlags; /* Flags (dirty|malloc) for program header.  */
                int secHeaderMalloced; /* Nonzero if shdr array was allocated.  */
                int64_t sizeStrOffset; /* Offset of the size string in the parent
                             if this is an archive member.  */
                llvm::ELF::Elf32_Ehdr elfHeaderMem; /* Memory used for ELF header when
                            not mmaped.  */
                char fillToAlign[sizeof(llvm::ELF::Elf64_Ehdr) - sizeof(llvm::ELF::Elf32_Ehdr)];

                /* The section array.  */
                ElfSectionList sections;
            } elf32;

            struct {
                llvm::ELF::Elf64_Ehdr* ehdr; /* Pointer to the ELF header.  This is
                       never malloced.  */
                llvm::ELF::Elf64_Shdr* shdr; /* Used when reading from a file.  */
                llvm::ELF::Elf64_Phdr* phdr; /* Pointer to the program header array.  */
                ElfSectionList* sectionLast; /* Last element in the section list.
                             If NULL the data has not yet been
                             read from the file.  */
                ElfDataChunk* rawChunks; /* List of elf_getdata_rawchunk results.  */
                unsigned int scnincr; /* Number of sections allocate the last
                             time.  */
                int elfHeaderFlags; /* Flags (dirty) for ELF header.  */
                int progHeaderFlags; /* Flags (dirty|malloc) for program header.  */
                int secHeaderMalloced; /* Nonzero if shdr array was allocated.  */
                int64_t sizeStrOffset; /* Offset of the size string in the parent
                             if this is an archive member.  */
                llvm::ELF::Elf64_Ehdr elfHeaderMem; /* Memory used for ELF header when
                            not mmaped.  */

                /* The section array.  */
                ElfSectionList sections;
            } elf64;
        } state;

        /* There absolutely never must be anything following the union.  */
    };

    ElfChecker() = delete;
    ElfChecker(const ElfChecker&) = delete;
    explicit ElfChecker(const void* data, int len);
    explicit ElfChecker(const std::string&);
    explicit ElfChecker(const MemMap&);
    ~ElfChecker();
    bool CheckValidElf();

private:
    int CheckIfError(void) const;
    void SetErrorCode(int value);

    Elf* ElfAllocate(
        void* map_address, int64_t offset, size_t maxsize, ElfCommand cmd, Elf* parent, ElfKind kind, size_t extra);

    ElfKind GetElfKind(void* buf) const;

    template <typename FromElfHeader, typename FromElfSectionHeader, typename SizeType>
    size_t GetSectionHeaderNum(
        FromElfHeader* ehdr, size_t maxsize, unsigned char* eIdent, void* mapAddress, int64_t offset);
    size_t GetShnum(void* map_address, unsigned char* e_ident, int64_t offset, size_t maxsize);
    /* Create descriptor for ELF file in memory.  */
    template <typename ElfEhdr, typename ElfShdr, typename ElfItemField>
    Elf* GetElfItem(ElfItemField& elfItemField, void* mapAddress, unsigned char* eIdent, int64_t offset, size_t maxSize,
        ElfCommand cmd, size_t scnCnt, Elf* elf);
    Elf* FileReadElf(
        void* map_address, unsigned char* e_ident, int64_t offset, size_t maxsize, ElfCommand cmd, Elf* parent);
    Elf* ReadMmapedFile(void* map_address, int64_t offset, size_t maxsize, ElfCommand cmd, Elf* parent);
    Elf* ElfMemory(char* image, size_t size);
    int ElfRelease(Elf* elf) const;
    char* elfData_;
    size_t elfLen_;
    int elfErrorCode_ { 0 };
    const bool fromMmap_ { false };
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_COMPILER_AOT_FILE_ELF_CHECKER_H
