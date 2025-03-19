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
#include "ecmascript/compiler/aot_file/elf_checker.h"

namespace panda::ecmascript {
/* Check Aligned */
template <typename AlignType> static bool CheckAlign(void* alignPtr, size_t offset = 0)
{
    if (ALLOW_UNALIGNED != 0) {
        return true;
    }
    size_t ptrPos = reinterpret_cast<size_t>(alignPtr) + offset;
    if ((ptrPos & (alignof(AlignType) - 1)) == 0) {
        return true;
    } else {
        return false;
    }
}

/* Swap LSB and MSB */
template <typename IntType> static void ByteSwap(IntType& x)
{
    static constexpr unsigned int iByteMuskOff0 = 0xffu;
    static constexpr unsigned int iByteMuskOff1 = iByteMuskOff0 << 8;
    static constexpr unsigned int iByteMuskOff2 = iByteMuskOff1 << 8;
    static constexpr unsigned int iByteMuskOff3 = iByteMuskOff2 << 8;
    static constexpr unsigned long long llByteMuskOff0 = 0xffuLL;
    static constexpr unsigned long long llByteMuskOff1 = llByteMuskOff0 << 8;
    static constexpr unsigned long long llByteMuskOff2 = llByteMuskOff1 << 8;
    static constexpr unsigned long long llByteMuskOff3 = llByteMuskOff2 << 8;
    static constexpr unsigned long long llByteMuskOff4 = llByteMuskOff3 << 8;
    static constexpr unsigned long long llByteMuskOff5 = llByteMuskOff4 << 8;
    static constexpr unsigned long long llByteMuskOff6 = llByteMuskOff5 << 8;
    static constexpr unsigned long long llByteMuskOff7 = llByteMuskOff6 << 8;
    static constexpr unsigned int move1 = 8; // 8: move 8 bits
    static constexpr unsigned int move2 = move1 + 8 * 2; // 8: skip one pair
    static constexpr unsigned int move3 = move2 + 8 * 2; // 8: skip another pair
    static constexpr unsigned int move4 = move3 + 8 * 2; // 8: skip another pair
    switch (sizeof(IntType)) {
        case sizeof(int8_t):
            return;
        case sizeof(int16_t):
            x = IntType((((x) >> move1) & iByteMuskOff0) |
                (((x) & iByteMuskOff0) << move1)); // 8: position to change between LSB and MSB
            return;
        case sizeof(int32_t):
            x = ((((x) & iByteMuskOff3) >> move2) | (((x) & iByteMuskOff2) >> move1) |
                (((x) & iByteMuskOff1) << move1) | (((x) & iByteMuskOff0) << move2));
            return;
        case sizeof(int64_t):
            x = ((((x) & llByteMuskOff7) >> move4) | (((x) & llByteMuskOff6) >> move3) |
                (((x) & llByteMuskOff5) >> move2) | (((x) & llByteMuskOff4) >> move1) |
                (((x) & llByteMuskOff3) << move1) | (((x) & llByteMuskOff2) << move2) |
                (((x) & llByteMuskOff1) << move3) | (((x) & llByteMuskOff0) << move4));
            return;
        default:
            return;
    }
}

/* This function requires copying memory. Avoid using it if MemMap is avalible. */
ElfChecker::ElfChecker(const void* data, int len) : elfLen_(len), elfErrorCode_(0), fromMmap_(false)
{
    if (len <= 0 || data == nullptr) {
        elfData_ = nullptr;
        return;
    }
    elfData_ = new char[len];
    if (memcpy_s(elfData_, len, data, len) != 0) {
        delete[] elfData_;
        elfData_ = nullptr;
        return;
    }
}

ElfChecker::ElfChecker(const std::string& path) : elfErrorCode_(0), fromMmap_(false)
{
    if (!panda::ecmascript::FileExist(path.c_str())) {
        elfData_ = nullptr;
        elfLen_ = 0;
        return;
    }
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (file.is_open() == false) {
        elfData_ = nullptr;
        elfLen_ = 0;
        return;
    }
    elfLen_ = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    if (elfLen_ <= 0) {
        elfData_ = nullptr;
        return;
    }
    elfData_ = new char[elfLen_];
    file.read(elfData_, elfLen_);
    return;
}

ElfChecker::ElfChecker(const MemMap& mem) : elfErrorCode_(0), fromMmap_(true)
{
    elfData_ = reinterpret_cast<char*>(mem.GetMem());
    elfLen_ = mem.GetSize();
    if (elfLen_ == 0) {
        elfData_ = nullptr;
    }
}

bool ElfChecker::CheckValidElf()
{
    if (elfData_ == nullptr || elfLen_ <= 0) {
        return false;
    }
    Elf* elf = ElfMemory(elfData_, elfLen_);
    if (elf == nullptr || elf->kind == ELF_KIND_NONE || CheckIfError() == false) {
        ElfRelease(elf);
        return false;
    }
    int release_num = ElfRelease(elf);
    if (release_num != 0 || CheckIfError() == false) {
        return false;
    }
    return true;
}

int ElfChecker::CheckIfError() const
{
    if (elfErrorCode_ != ELF_ERR_NOERROR) {
        return false;
    }
    return true;
}

void ElfChecker::SetErrorCode(int value)
{
    if (value >= 0 && value < ELF_ERR_NUM) {
        elfErrorCode_ = value;
    } else {
        elfErrorCode_ = ELF_ERR_UNKNOWN_ERROR;
    }
}

ElfChecker::Elf* ElfChecker::ElfAllocate(
    void* mapAddress, int64_t offset, size_t maxsize, ElfCommand cmd, Elf* parent, ElfKind kind, size_t extra)
{
    size_t alloc_size = sizeof(Elf) + extra;
    Elf* result = reinterpret_cast<Elf*>(new char[alloc_size]());
    if (result == nullptr) {
        SetErrorCode(ELF_ERR_NOMEM);
    } else {
        result->kind = kind;
        result->refCount = 1;
        result->cmd = cmd;
        result->startOffset = offset;
        result->maxSize = maxsize;
        result->mapAddress = mapAddress;
        result->parent = parent;
    }

    return result;
}

ElfChecker::ElfKind ElfChecker::GetElfKind(void* buf) const
{
    /* Could be an ELF file.  */
    int eclass = (int)(reinterpret_cast<unsigned char*>(buf))[llvm::ELF::EI_CLASS];
    int data = (int)(reinterpret_cast<unsigned char*>(buf))[llvm::ELF::EI_DATA];
    int version = (int)(reinterpret_cast<unsigned char*>(buf))[llvm::ELF::EI_VERSION];

    if (eclass > llvm::ELF::ELFCLASSNONE && eclass <= llvm::ELF::ELFCLASS64 && data > llvm::ELF::ELFDATANONE &&
        data <= llvm::ELF::ELFDATA2MSB && version == llvm::ELF::EV_CURRENT) {
        return ELF_KIND_ELF;
    }

    /* We do not know this file type.  */
    return ELF_KIND_NONE;
}

/* Get the number of sections from the ELF header.  */
template <typename FromElfHeader, typename FromElfSectionHeader, typename SizeType>
size_t ElfChecker::GetSectionHeaderNum(
    FromElfHeader* ehdr, size_t maxsize, unsigned char* eIdent, void* mapAddress, int64_t offset)
{
    size_t result = ehdr->e_shnum;
    if (SECUREC_UNLIKELY(result == 0) && ehdr->e_shoff != 0) {
        if (SECUREC_UNLIKELY(ehdr->e_shoff >= maxsize) ||
            SECUREC_UNLIKELY(maxsize - ehdr->e_shoff < sizeof(FromElfSectionHeader))) {
            return 0;
        }
        SizeType size;
        FromElfSectionHeader* dest =
            reinterpret_cast<FromElfSectionHeader*>(reinterpret_cast<char*>(mapAddress) + ehdr->e_shoff + offset);
        if (eIdent[llvm::ELF::EI_DATA] == ELF_BYTEORDER && CheckAlign<FromElfSectionHeader>(dest) == true) {
            /* We can directly access the memory.  */
            size = dest->sh_size;
        } else {
            char* memSrc =
                reinterpret_cast<char*>(mapAddress) + ehdr->e_shoff + offset + offsetof(FromElfSectionHeader, sh_size);
            if (memcpy_s(&size, sizeof(size), memSrc, sizeof(SizeType)) != 0) {
                SetErrorCode(ELF_ERR_NOMEM);
                return 0;
            }

            if (eIdent[llvm::ELF::EI_DATA] != ELF_BYTEORDER) {
                ByteSwap(size);
            }
            if (size > ~((llvm::ELF::Elf64_Word)0)) {
                /* Invalid value, it is too large.  */
                SetErrorCode(ELF_ERR_INVALID_ELF);
                return 0;
            }
        }
        result = size;
    }

    /* If the section headers were truncated, pretend none were there.
     */
    if (ehdr->e_shoff > maxsize || maxsize - ehdr->e_shoff < sizeof(FromElfSectionHeader) * result) {
        result = 0;
    }
    return result;
}

/* Get Section Header Number */
size_t ElfChecker::GetShnum(void* mapAddress, unsigned char* eIdent, int64_t offset, size_t maxsize)
{
    union {
        llvm::ELF::Elf32_Ehdr* e32;
        llvm::ELF::Elf64_Ehdr* e64;
        void* p;
    } ehdrPtr;
    union {
        llvm::ELF::Elf32_Ehdr e32;
        llvm::ELF::Elf64_Ehdr e64;
    } ehdrMem;
    bool is32 = eIdent[llvm::ELF::EI_CLASS] == llvm::ELF::ELFCLASS32;

    if ((is32 && maxsize < sizeof(llvm::ELF::Elf32_Ehdr)) || (!is32 && maxsize < sizeof(llvm::ELF::Elf64_Ehdr))) {
        /* Not long enough */
        SetErrorCode(ELF_ERR_INVALID_ELF);
        return 0;
    }
    /* Make the ELF header available.  */
    bool isAligned = (is32 ? CheckAlign<llvm::ELF::Elf32_Ehdr>(eIdent) : CheckAlign<llvm::ELF::Elf64_Ehdr>(eIdent));
    if (eIdent[llvm::ELF::EI_DATA] == ELF_BYTEORDER && isAligned) {
        ehdrPtr.p = eIdent;
    } else {
        ehdrPtr.p = &ehdrMem;
        if (is32) {
            if (ALLOW_UNALIGNED) {
                ehdrMem.e32.e_shnum = reinterpret_cast<llvm::ELF::Elf32_Ehdr*>(eIdent)->e_shnum;
                ehdrMem.e32.e_shoff = reinterpret_cast<llvm::ELF::Elf32_Ehdr*>(eIdent)->e_shoff;
            } else {
                if (memcpy_s(&ehdrMem, sizeof(ehdrMem), eIdent, sizeof(llvm::ELF::Elf32_Ehdr)) != 0) {
                    SetErrorCode(ELF_ERR_NOMEM);
                    return 0;
                };
            }

            if (eIdent[llvm::ELF::EI_DATA] != ELF_BYTEORDER) {
                ByteSwap(ehdrMem.e32.e_shnum);
                ByteSwap(ehdrMem.e32.e_shoff);
            }
        } else {
            if (ALLOW_UNALIGNED) {
                ehdrMem.e64.e_shnum = reinterpret_cast<llvm::ELF::Elf64_Ehdr*>(eIdent)->e_shnum;
                ehdrMem.e64.e_shoff = reinterpret_cast<llvm::ELF::Elf64_Ehdr*>(eIdent)->e_shoff;
            } else {
                if (memcpy_s(&ehdrMem, sizeof(ehdrMem), eIdent, sizeof(llvm::ELF::Elf64_Ehdr)) != 0) {
                    SetErrorCode(ELF_ERR_NOMEM);
                    return 0;
                }
            };

            if (eIdent[llvm::ELF::EI_DATA] != ELF_BYTEORDER) {
                ByteSwap(ehdrMem.e64.e_shnum);
                ByteSwap(ehdrMem.e64.e_shoff);
            }
        }
    }
    if (is32) {
        return GetSectionHeaderNum<llvm::ELF::Elf32_Ehdr, llvm::ELF::Elf32_Shdr, llvm::ELF::Elf32_Word>(
            ehdrPtr.e32, maxsize, eIdent, mapAddress, offset);
    } else {
        return GetSectionHeaderNum<llvm::ELF::Elf64_Ehdr, llvm::ELF::Elf64_Shdr, llvm::ELF::Elf64_Xword>(
            ehdrPtr.e64, maxsize, eIdent, mapAddress, offset);
    }
}

/* Get Details from mapAddress,  */
template <typename ElfEhdr, typename ElfShdr, typename ElfItemField>
ElfChecker::Elf* ElfChecker::GetElfItem(ElfItemField& elfItemField, void* mapAddress, unsigned char* eIdent,
    int64_t offset, size_t maxSize, ElfCommand cmd, size_t scnCnt, Elf* elf)
{
    /* This pointer might not be directly usable if the alignment is
       not sufficient for the architecture.  */
    ElfEhdr* ehdr = reinterpret_cast<ElfEhdr*>(reinterpret_cast<char*>(mapAddress) + offset);
    if (eIdent[llvm::ELF::EI_DATA] == ELF_BYTEORDER && CheckAlign<ElfEhdr>(ehdr)) {
        /* We can use the mmapped memory.  */
        elfItemField.ehdr = ehdr;
    } else {
        if (memcpy_s(&elfItemField.elfHeaderMem, sizeof(ElfEhdr), eIdent, sizeof(ElfEhdr)) != 0) {
            SetErrorCode(ELF_ERR_NOMEM);
            return nullptr;
        }
        elfItemField.ehdr = reinterpret_cast<ElfEhdr*>(&elfItemField.elfHeaderMem);
        if (eIdent[llvm::ELF::EI_DATA] != ELF_BYTEORDER) {
            ByteSwap(elfItemField.elfHeaderMem.e_type);
            ByteSwap(elfItemField.elfHeaderMem.e_machine);
            ByteSwap(elfItemField.elfHeaderMem.e_version);
            ByteSwap(elfItemField.elfHeaderMem.e_entry);
            ByteSwap(elfItemField.elfHeaderMem.e_phoff);
            ByteSwap(elfItemField.elfHeaderMem.e_shoff);
            ByteSwap(elfItemField.elfHeaderMem.e_flags);
            ByteSwap(elfItemField.elfHeaderMem.e_ehsize);
            ByteSwap(elfItemField.elfHeaderMem.e_phentsize);
            ByteSwap(elfItemField.elfHeaderMem.e_phnum);
            ByteSwap(elfItemField.elfHeaderMem.e_shentsize);
            ByteSwap(elfItemField.elfHeaderMem.e_shnum);
            ByteSwap(elfItemField.elfHeaderMem.e_shstrndx);
        }
    }
    auto elfSHOff = elfItemField.ehdr->e_shoff;
    if (eIdent[llvm::ELF::EI_DATA] == ELF_BYTEORDER && cmd != ELF_CMD_READ_MMAP &&
        CheckAlign<ElfShdr>(ehdr, elfSHOff)) {
        if (SECUREC_UNLIKELY(scnCnt > 0 && elfSHOff >= maxSize) ||
            SECUREC_UNLIKELY(maxSize - elfSHOff < scnCnt * sizeof(ElfShdr))) {
            delete[] reinterpret_cast<char*>(elf);
            elf = nullptr;
            SetErrorCode(ELF_ERR_INVALID_ELF);
            return nullptr;
        }
        if (scnCnt > 0) {
            elfItemField.shdr = reinterpret_cast<ElfShdr*>(reinterpret_cast<char*>(ehdr) + elfSHOff);
        }
        for (size_t cnt = 0; cnt < scnCnt; ++cnt) {
            elfItemField.sections.data()[cnt].index = cnt;
            elfItemField.sections.data()[cnt].elf = elf;
            if (std::is_same<ElfShdr, llvm::ELF::Elf32_Shdr>::value) {
                elfItemField.sections.data()[cnt].shdr.e32 =
                    reinterpret_cast<llvm::ELF::Elf32_Shdr*>(&elfItemField.shdr[cnt]);
            } else {
                elfItemField.sections.data()[cnt].shdr.e64 =
                    reinterpret_cast<llvm::ELF::Elf64_Shdr*>(&elfItemField.shdr[cnt]);
            }

            if (SECUREC_LIKELY(elfItemField.shdr[cnt].sh_offset < maxSize) &&
                SECUREC_LIKELY(elfItemField.shdr[cnt].sh_size <= maxSize - elfItemField.shdr[cnt].sh_offset)) {
                elfItemField.sections.data()[cnt].rawDataBase = elfItemField.sections.data()[cnt].dataBase =
                    (reinterpret_cast<char*>(mapAddress) + offset + elfItemField.shdr[cnt].sh_offset);
            }
            elfItemField.sections.data()[cnt].list = &elfItemField.sections;

            /* If this is a section with an extended index add a
               reference in the section which uses the extended
               index.  */
            if (elfItemField.shdr[cnt].sh_type == llvm::ELF::SHT_SYMTAB_SHNDX &&
                elfItemField.shdr[cnt].sh_link < scnCnt) {
                elfItemField.sections.data()[elfItemField.shdr[cnt].sh_link]
                    .extendSectionHeaderIndex = static_cast<int>(cnt);
            }
            /* Set the own extendSectionHeaderIndex field in case it has not yet
               been set.  */
            if (elfItemField.sections.data()[cnt].extendSectionHeaderIndex == 0) {
                elfItemField.sections.data()[cnt].extendSectionHeaderIndex = -1;
            }
        }
    } else {
        for (size_t cnt = 0; cnt < scnCnt; ++cnt) {
            elfItemField.sections.data()[cnt].index = cnt;
            elfItemField.sections.data()[cnt].elf = elf;
            elfItemField.sections.data()[cnt].list = &elfItemField.sections;
        }
    }

    /* So far only one block with sections.  */
    elfItemField.sectionLast = &elfItemField.sections;
    return elf;
}

/* Create descriptor for ELF file in memory.  */
ElfChecker::Elf* ElfChecker::FileReadElf(
    void* mapAddress, unsigned char* eIdent, int64_t offset, size_t maxSize, ElfCommand cmd, Elf* parent)
{
    /* Verify the binary is of the class we can handle.  */
    if (SECUREC_UNLIKELY((eIdent[llvm::ELF::EI_CLASS] != llvm::ELF::ELFCLASS32 &&
                          eIdent[llvm::ELF::EI_CLASS] != llvm::ELF::ELFCLASS64) ||
                         (eIdent[llvm::ELF::EI_DATA] != llvm::ELF::ELFDATA2LSB &&
                          eIdent[llvm::ELF::EI_DATA] != llvm::ELF::ELFDATA2MSB))) {
        /* Cannot handle this.  */
        SetErrorCode(ELF_ERR_INVALID_ELF);
        return nullptr;
    }

    size_t scncnt = GetShnum(mapAddress, eIdent, offset, maxSize);
    if (CheckIfError() == false) {
        return nullptr;
    }

    /* Check for too many sections.  */
    if (eIdent[llvm::ELF::EI_CLASS] == llvm::ELF::ELFCLASS32) {
        if (scncnt > SIZE_MAX / (sizeof(ElfSectionBase) + sizeof(llvm::ELF::Elf32_Shdr))) {
            SetErrorCode(ELF_ERR_INVALID_ELF);
            return nullptr;
        }
    } else if (scncnt > SIZE_MAX / (sizeof(ElfSectionBase) + sizeof(llvm::ELF::Elf64_Shdr))) {
        SetErrorCode(ELF_ERR_INVALID_ELF);
        return nullptr;
    }

    /* We can now allocate the memory.  Even if there are no section
       headers, we allocate space for a zeroth section in case we need it
       later.  */
    const size_t scnMax = (scncnt ? scncnt : (cmd == ELF_CMD_RDWR || cmd == ELF_CMD_RDWR_MMAP) ? 1 : 0);
    Elf* elf = ElfAllocate(mapAddress, offset, maxSize, cmd, parent, ELF_KIND_ELF, scnMax * sizeof(ElfSectionBase));
    if (elf == nullptr) {
        return nullptr;
    }

    ASSERT((unsigned int)scncnt == scncnt);
    ASSERT(offsetof(Elf, state.elf32.sections) == offsetof(Elf, state.elf64.sections));
    elf->state.elf32.sections.cnt = scncnt;
    elf->state.elf32.sections.max = scnMax;

    /* Make the class easily available.  */
    elf->binaryClass = eIdent[llvm::ELF::EI_CLASS];

    if (eIdent[llvm::ELF::EI_CLASS] == llvm::ELF::ELFCLASS32) {
        return GetElfItem<llvm::ELF::Elf32_Ehdr, llvm::ELF::Elf32_Shdr, decltype(elf->state.elf32)>(
            elf->state.elf32, mapAddress, eIdent, offset, maxSize, cmd, scncnt, elf);
    } else {
        return GetElfItem<llvm::ELF::Elf64_Ehdr, llvm::ELF::Elf64_Shdr, decltype(elf->state.elf64)>(
            elf->state.elf64, mapAddress, eIdent, offset, maxSize, cmd, scncnt, elf);
    }
}

ElfChecker::Elf* ElfChecker::ReadMmapedFile(
    void* mapAddress, int64_t offset, size_t maxsize, ElfCommand cmd, Elf* parent)
{
    unsigned char* eIdent = reinterpret_cast<unsigned char*>(mapAddress) + offset;
    /* See what kind of object we have here.  */
    ElfKind kind = GetElfKind(eIdent);
    if (kind == ELF_KIND_ELF) {
        return FileReadElf(mapAddress, eIdent, offset, maxsize, cmd, parent);
    } else {
        return ElfAllocate(mapAddress, offset, maxsize, cmd, parent, ELF_KIND_NONE, 0);
    }
}

ElfChecker::Elf* ElfChecker::ElfMemory(char* image, size_t size)
{
    if (image == nullptr) {
        SetErrorCode(ELF_ERR_INVALID_OPERAND);
        return nullptr;
    }
    return ReadMmapedFile(image, 0, size, ELF_CMD_READ, nullptr);
}

int ElfChecker::ElfRelease(Elf* elf) const
{
    if (elf == nullptr) {
        return 0;
    }
    if (elf->refCount != 0) {
        --elf->refCount;
        if (elf->refCount != 0) {
            int result = elf->refCount;
            return result;
        }
    }
    Elf* parent = elf->parent;
    delete[] reinterpret_cast<char*>(elf);
    elf = nullptr;
    return (parent != nullptr && parent->refCount == 0 ? ElfRelease(parent) : 0);
}

ElfChecker::~ElfChecker()
{
    if (fromMmap_ == false && elfData_ != nullptr) {
        delete[] elfData_;
        elfData_ = nullptr;
    }
}
} // namespace panda::ecmascript
