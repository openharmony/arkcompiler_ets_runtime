/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "securec.h"
#include "ecmascript/compiler/aot_file/elf_reader.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class ElfReaderTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
    }

    void SetUp() override {}

    void TearDown() override {}

protected:
    // Allocate page-aligned buffer to satisfy ElfReader alignment requirements
    static uint8_t *AllocBuffer(size_t size)
    {
        void *ptr = nullptr;
        if (posix_memalign(&ptr, static_cast<size_t>(AOTFileInfo::PAGE_ALIGN), size) != 0) {
            return nullptr;
        }
        if (memset_s(ptr, size, 0, size) != EOK) {
            free(ptr);
            return nullptr;
        }
        return static_cast<uint8_t *>(ptr);
    }

    static void FreeBuffer(uint8_t *buf)
    {
        free(buf);
    }

    // Write ModuleRegionInfo array into buffer at given offset
    static void WriteModuleInfos(uint8_t *buf, uint64_t offset,
                                 const std::vector<ModuleSectionDes::ModuleRegionInfo> &infos)
    {
        size_t copySize = infos.size() * sizeof(ModuleSectionDes::ModuleRegionInfo);
        if (memcpy_s(buf + offset, copySize, infos.data(), copySize) != EOK) {
            return;
        }
    }

    // Initialize a minimal ELF64 header in buffer for ParseELFSegment tests
    static void InitElfHeader(uint8_t *buf, uint64_t phoff, uint16_t phnum)
    {
        llvm::ELF::Elf64_Ehdr *ehdr = reinterpret_cast<llvm::ELF::Elf64_Ehdr *>(buf);
        ehdr->e_ident[llvm::ELF::EI_MAG0] = '\x7f';
        ehdr->e_ident[llvm::ELF::EI_MAG1] = 'E';
        ehdr->e_ident[llvm::ELF::EI_MAG2] = 'L';
        ehdr->e_ident[llvm::ELF::EI_MAG3] = 'F';
        ehdr->e_ident[llvm::ELF::EI_CLASS] = llvm::ELF::ELFCLASS64;
        ehdr->e_ident[llvm::ELF::EI_DATA] = llvm::ELF::ELFDATA2LSB;
        ehdr->e_ident[llvm::ELF::EI_VERSION] = llvm::ELF::EV_CURRENT;
        ehdr->e_type = llvm::ELF::ET_EXEC;
        ehdr->e_machine = llvm::ELF::EM_X86_64;
        ehdr->e_version = llvm::ELF::EV_CURRENT;
        ehdr->e_entry = 0;
        ehdr->e_phoff = phoff;
        ehdr->e_shoff = 0;
        ehdr->e_flags = 0;
        ehdr->e_ehsize = sizeof(llvm::ELF::Elf64_Ehdr);
        ehdr->e_phentsize = sizeof(llvm::ELF::Elf64_Phdr);
        ehdr->e_phnum = phnum;
        ehdr->e_shentsize = sizeof(llvm::ELF::Elf64_Shdr);
        ehdr->e_shnum = 0;
        ehdr->e_shstrndx = static_cast<uint16_t>(-1);
    }
};

// ==================== GetCurModuleInfo Tests ====================

// Test GetCurModuleInfo returns correct data for a single module at non-zero offset
HWTEST_F_L0(ElfReaderTest, GetCurModuleInfo_SingleModule)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.startIndex = 0;
    info.funcCount = 5;
    info.textSize = 128;
    info.stackMapSize = 64;
    info.strtabSize = 32;
    info.symtabSize = 48;
    info.rodataSizeBeforeText = 0;
    info.rodataSizeAfterText = 0;

    size_t bufferSize = 4096;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t offset = 64;
    WriteModuleInfos(buffer, offset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    auto *result = reader.GetCurModuleInfo(0, offset);
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->startIndex, 0u);
    ASSERT_EQ(result->funcCount, 5u);
    ASSERT_EQ(result->textSize, 128u);
    ASSERT_EQ(result->stackMapSize, 64u);
    ASSERT_EQ(result->strtabSize, 32u);
    ASSERT_EQ(result->symtabSize, 48u);

    FreeBuffer(buffer);
}

// Test GetCurModuleInfo with multiple modules at sequential indices
HWTEST_F_L0(ElfReaderTest, GetCurModuleInfo_MultipleModules)
{
    std::vector<ModuleSectionDes::ModuleRegionInfo> infos(3);
    infos[0].startIndex = 0;
    infos[0].funcCount = 10;
    infos[0].textSize = 100;
    infos[1].startIndex = 10;
    infos[1].funcCount = 20;
    infos[1].textSize = 200;
    infos[2].startIndex = 30;
    infos[2].funcCount = 15;
    infos[2].textSize = 300;

    size_t bufferSize = 4096;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t offset = 128;
    WriteModuleInfos(buffer, offset, infos);

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    for (size_t i = 0; i < 3; ++i) {
        auto *result = reader.GetCurModuleInfo(i, offset);
        ASSERT_NE(result, nullptr);
        ASSERT_EQ(result->startIndex, infos[i].startIndex);
        ASSERT_EQ(result->funcCount, infos[i].funcCount);
        ASSERT_EQ(result->textSize, infos[i].textSize);
    }

    FreeBuffer(buffer);
}

// Test GetCurModuleInfo with offset = 0 (module info at buffer start)
HWTEST_F_L0(ElfReaderTest, GetCurModuleInfo_ZeroOffset)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.textSize = 256;
    info.funcCount = 1;

    size_t bufferSize = 4096;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    WriteModuleInfos(buffer, 0, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    auto *result = reader.GetCurModuleInfo(0, 0);
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->textSize, 256u);
    ASSERT_EQ(result->funcCount, 1u);

    FreeBuffer(buffer);
}

// ==================== VerifyELFHeader Tests ====================

// Test VerifyELFHeader with completely invalid magic bytes
HWTEST_F_L0(ElfReaderTest, VerifyELFHeader_InvalidMagic)
{
    size_t bufferSize = 4096;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);

    llvm::ELF::Elf64_Ehdr *ehdr = reinterpret_cast<llvm::ELF::Elf64_Ehdr *>(buffer);
    ehdr->e_ident[llvm::ELF::EI_MAG0] = 0x00;
    ehdr->e_ident[llvm::ELF::EI_MAG1] = 'X';
    ehdr->e_ident[llvm::ELF::EI_MAG2] = 'X';
    ehdr->e_ident[llvm::ELF::EI_MAG3] = 'X';

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    ASSERT_FALSE(reader.VerifyELFHeader(1, true));

    FreeBuffer(buffer);
}

// Test VerifyELFHeader with partially correct magic (first 2 bytes only)
HWTEST_F_L0(ElfReaderTest, VerifyELFHeader_PartialMagic)
{
    size_t bufferSize = 4096;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);

    llvm::ELF::Elf64_Ehdr *ehdr = reinterpret_cast<llvm::ELF::Elf64_Ehdr *>(buffer);
    ehdr->e_ident[llvm::ELF::EI_MAG0] = 0x7f;
    ehdr->e_ident[llvm::ELF::EI_MAG1] = 'E';
    ehdr->e_ident[llvm::ELF::EI_MAG2] = 'X'; // Wrong: should be 'L'
    ehdr->e_ident[llvm::ELF::EI_MAG3] = 'X';

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    ASSERT_FALSE(reader.VerifyELFHeader(1, true));

    FreeBuffer(buffer);
}

// Test VerifyELFHeader with all-zero header (no magic)
HWTEST_F_L0(ElfReaderTest, VerifyELFHeader_AllZeros)
{
    size_t bufferSize = 4096;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    // Buffer already zeroed by AllocBuffer

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    ASSERT_FALSE(reader.VerifyELFHeader(1, true));

    FreeBuffer(buffer);
}

// Test VerifyELFHeader with only first magic byte correct
HWTEST_F_L0(ElfReaderTest, VerifyELFHeader_OnlyFirstByte)
{
    size_t bufferSize = 4096;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);

    llvm::ELF::Elf64_Ehdr *ehdr = reinterpret_cast<llvm::ELF::Elf64_Ehdr *>(buffer);
    ehdr->e_ident[llvm::ELF::EI_MAG0] = 0x7f;
    // EI_MAG1,2,3 remain zero

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    ASSERT_FALSE(reader.VerifyELFHeader(1, true));

    FreeBuffer(buffer);
}

// ==================== ParseELFSegment Tests ====================

// Test ParseELFSegment returns false when file mapping is null
HWTEST_F_L0(ElfReaderTest, ParseELFSegment_NullMapping)
{
    MemMap nullMap; // Default constructor: originAddr_ = nullptr
    ASSERT_EQ(nullMap.GetOriginAddr(), nullptr);

    ElfReader reader(nullMap);
    ASSERT_FALSE(reader.ParseELFSegment());
}

// Test ParseELFSegment returns false when p_filesz > p_memsz
HWTEST_F_L0(ElfReaderTest, ParseELFSegment_FileszGreaterThanMemsz)
{
    size_t bufferSize = 4096;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);

    InitElfHeader(buffer, sizeof(llvm::ELF::Elf64_Ehdr), 1);

    llvm::ELF::Elf64_Phdr *phdr = reinterpret_cast<llvm::ELF::Elf64_Phdr *>(
        buffer + sizeof(llvm::ELF::Elf64_Ehdr));
    phdr[0].p_type = llvm::ELF::PT_LOAD;
    phdr[0].p_filesz = 200;
    phdr[0].p_memsz = 100; // filesz > memsz -> error
    phdr[0].p_vaddr = 0;
    phdr[0].p_offset = 0;
    phdr[0].p_flags = 0;

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    ASSERT_FALSE(reader.ParseELFSegment());

    FreeBuffer(buffer);
}

// Test ParseELFSegment skips segment when p_filesz is 0
HWTEST_F_L0(ElfReaderTest, ParseELFSegment_ZeroFilesz)
{
    size_t bufferSize = 4096;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);

    InitElfHeader(buffer, sizeof(llvm::ELF::Elf64_Ehdr), 1);

    llvm::ELF::Elf64_Phdr *phdr = reinterpret_cast<llvm::ELF::Elf64_Phdr *>(
        buffer + sizeof(llvm::ELF::Elf64_Ehdr));
    phdr[0].p_type = llvm::ELF::PT_LOAD;
    phdr[0].p_filesz = 0; // Zero filesz -> continue (skip)
    phdr[0].p_memsz = 100;

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    ASSERT_TRUE(reader.ParseELFSegment());

    FreeBuffer(buffer);
}

// Test ParseELFSegment skips non-PT_LOAD segments
HWTEST_F_L0(ElfReaderTest, ParseELFSegment_NonLoadSegment)
{
    size_t bufferSize = 4096;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);

    InitElfHeader(buffer, sizeof(llvm::ELF::Elf64_Ehdr), 1);

    llvm::ELF::Elf64_Phdr *phdr = reinterpret_cast<llvm::ELF::Elf64_Phdr *>(
        buffer + sizeof(llvm::ELF::Elf64_Ehdr));
    phdr[0].p_type = llvm::ELF::PT_NULL; // Not PT_LOAD -> skip

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    ASSERT_TRUE(reader.ParseELFSegment());

    FreeBuffer(buffer);
}

// Test ParseELFSegment with multiple segments of mixed types
HWTEST_F_L0(ElfReaderTest, ParseELFSegment_MultipleSegments)
{
    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);

    InitElfHeader(buffer, sizeof(llvm::ELF::Elf64_Ehdr), 3);

    llvm::ELF::Elf64_Phdr *phdr = reinterpret_cast<llvm::ELF::Elf64_Phdr *>(
        buffer + sizeof(llvm::ELF::Elf64_Ehdr));

    // Segment 0: Not PT_LOAD -> skip
    phdr[0].p_type = llvm::ELF::PT_NULL;

    // Segment 1: PT_LOAD with zero filesz -> skip
    phdr[1].p_type = llvm::ELF::PT_LOAD;
    phdr[1].p_filesz = 0;
    phdr[1].p_memsz = 100;

    // Segment 2: PT_LOAD with valid non-executable data
    phdr[2].p_type = llvm::ELF::PT_LOAD;
    phdr[2].p_filesz = 256;
    phdr[2].p_memsz = 512;
    phdr[2].p_vaddr = 4096; // Page-aligned
    phdr[2].p_offset = 4096; // Page-aligned
    phdr[2].p_flags = llvm::ELF::PF_R; // Non-executable -> no PageProtect call

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    ASSERT_TRUE(reader.ParseELFSegment());

    FreeBuffer(buffer);
}

// Test ParseELFSegment with equal filesz and memsz (valid case)
HWTEST_F_L0(ElfReaderTest, ParseELFSegment_EqualFileszMemsz)
{
    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);

    InitElfHeader(buffer, sizeof(llvm::ELF::Elf64_Ehdr), 1);

    llvm::ELF::Elf64_Phdr *phdr = reinterpret_cast<llvm::ELF::Elf64_Phdr *>(
        buffer + sizeof(llvm::ELF::Elf64_Ehdr));
    phdr[0].p_type = llvm::ELF::PT_LOAD;
    phdr[0].p_filesz = 512;
    phdr[0].p_memsz = 512;
    phdr[0].p_vaddr = 4096;
    phdr[0].p_offset = 4096;
    phdr[0].p_flags = llvm::ELF::PF_R; // Read-only, no exec

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    ASSERT_TRUE(reader.ParseELFSegment());

    FreeBuffer(buffer);
}

// ==================== SeparateTextSections Tests ====================

// Test SeparateTextSections with single module, no rodata
HWTEST_F_L0(ElfReaderTest, SeparateTextSections_SingleModuleNoRodata)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.textSize = 64;
    info.rodataSizeBeforeText = 0;
    info.rodataSizeAfterText = 0;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateTextSections(des, secAddr, secOffset, moduleInfoOffset);

    llvm::ELF::Elf64_Off alignedOffset = AlignUp(0ULL, static_cast<size_t>(AOTFileInfo::PAGE_ALIGN));
    ASSERT_EQ(des[0].GetSecAddr(ElfSecName::TEXT), secAddr + alignedOffset);
    ASSERT_EQ(des[0].GetSecSize(ElfSecName::TEXT), 64u);
    ASSERT_EQ(secOffset, alignedOffset + 64u);

    FreeBuffer(buffer);
}

// Test SeparateTextSections with multiple modules
HWTEST_F_L0(ElfReaderTest, SeparateTextSections_MultipleModules)
{
    std::vector<ModuleSectionDes::ModuleRegionInfo> infos(2);
    infos[0].textSize = 64;
    infos[0].rodataSizeBeforeText = 0;
    infos[0].rodataSizeAfterText = 0;
    infos[1].textSize = 128;
    infos[1].rodataSizeBeforeText = 0;
    infos[1].rodataSizeAfterText = 0;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, infos);

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(2);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateTextSections(des, secAddr, secOffset, moduleInfoOffset);

    // Module 0: aligned to PAGE_ALIGN, textSize = 64
    llvm::ELF::Elf64_Off offset0 = AlignUp(0ULL, static_cast<size_t>(AOTFileInfo::PAGE_ALIGN));
    ASSERT_EQ(des[0].GetSecAddr(ElfSecName::TEXT), secAddr + offset0);
    ASSERT_EQ(des[0].GetSecSize(ElfSecName::TEXT), 64u);

    // Module 1: after module 0, aligned again
    llvm::ELF::Elf64_Off offset1 = AlignUp(offset0 + 64ULL, static_cast<size_t>(AOTFileInfo::PAGE_ALIGN));
    ASSERT_EQ(des[1].GetSecAddr(ElfSecName::TEXT), secAddr + offset1);
    ASSERT_EQ(des[1].GetSecSize(ElfSecName::TEXT), 128u);

    FreeBuffer(buffer);
}

// Test SeparateTextSections with rodataSizeBeforeText affecting text offset
HWTEST_F_L0(ElfReaderTest, SeparateTextSections_WithRodataBeforeText)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.textSize = 64;
    info.rodataSizeBeforeText = 32;
    info.rodataSizeAfterText = 0;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateTextSections(des, secAddr, secOffset, moduleInfoOffset);

    // Page align -> add rodataSizeBeforeText(32) -> align to TEXT_SEC_ALIGN(16) -> text
    llvm::ELF::Elf64_Off alignedOff = AlignUp(0ULL, static_cast<size_t>(AOTFileInfo::PAGE_ALIGN));
    alignedOff += 32;
    alignedOff = AlignUp(alignedOff, static_cast<size_t>(AOTFileInfo::TEXT_SEC_ALIGN));

    ASSERT_EQ(des[0].GetSecAddr(ElfSecName::TEXT), secAddr + alignedOff);
    ASSERT_EQ(des[0].GetSecSize(ElfSecName::TEXT), 64u);

    FreeBuffer(buffer);
}

// Test SeparateTextSections with rodataSizeAfterText affecting final offset
HWTEST_F_L0(ElfReaderTest, SeparateTextSections_WithRodataAfterText)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.textSize = 64;
    info.rodataSizeBeforeText = 0;
    info.rodataSizeAfterText = 48;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateTextSections(des, secAddr, secOffset, moduleInfoOffset);

    llvm::ELF::Elf64_Off alignedOff = AlignUp(0ULL, static_cast<size_t>(AOTFileInfo::PAGE_ALIGN));
    ASSERT_EQ(des[0].GetSecAddr(ElfSecName::TEXT), secAddr + alignedOff);
    ASSERT_EQ(des[0].GetSecSize(ElfSecName::TEXT), 64u);

    // After text: secOffset += 64, align up to RODATA_SEC_ALIGN(16), += 48
    llvm::ELF::Elf64_Off afterText = alignedOff + 64;
    afterText = AlignUp(afterText, static_cast<size_t>(AOTFileInfo::RODATA_SEC_ALIGN));
    afterText += 48;
    ASSERT_EQ(secOffset, afterText);

    FreeBuffer(buffer);
}

// Test SeparateTextSections with both rodata before and after text
HWTEST_F_L0(ElfReaderTest, SeparateTextSections_WithBothRodata)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.textSize = 64;
    info.rodataSizeBeforeText = 32;
    info.rodataSizeAfterText = 48;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateTextSections(des, secAddr, secOffset, moduleInfoOffset);

    // Page align -> +rodataBefore(32) -> align TEXT_SEC_ALIGN -> text(64) -> align RODATA_SEC_ALIGN -> +rodataAfter(48)
    llvm::ELF::Elf64_Off off = AlignUp(0ULL, static_cast<size_t>(AOTFileInfo::PAGE_ALIGN));
    off += 32;
    off = AlignUp(off, static_cast<size_t>(AOTFileInfo::TEXT_SEC_ALIGN));

    ASSERT_EQ(des[0].GetSecAddr(ElfSecName::TEXT), secAddr + off);
    ASSERT_EQ(des[0].GetSecSize(ElfSecName::TEXT), 64u);

    off += 64;
    off = AlignUp(off, static_cast<size_t>(AOTFileInfo::RODATA_SEC_ALIGN));
    off += 48;
    ASSERT_EQ(secOffset, off);

    FreeBuffer(buffer);
}

// Test SeparateTextSections with zero textSize
HWTEST_F_L0(ElfReaderTest, SeparateTextSections_ZeroTextSize)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.textSize = 0;
    info.rodataSizeBeforeText = 0;
    info.rodataSizeAfterText = 0;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateTextSections(des, secAddr, secOffset, moduleInfoOffset);

    llvm::ELF::Elf64_Off alignedOffset = AlignUp(0ULL, static_cast<size_t>(AOTFileInfo::PAGE_ALIGN));
    ASSERT_EQ(des[0].GetSecAddr(ElfSecName::TEXT), secAddr + alignedOffset);
    ASSERT_EQ(des[0].GetSecSize(ElfSecName::TEXT), 0u);
    ASSERT_EQ(secOffset, alignedOffset);

    FreeBuffer(buffer);
}

// ==================== SeparateArkStackMapSections Tests ====================

// Test SeparateArkStackMapSections with a single module
HWTEST_F_L0(ElfReaderTest, SeparateArkStackMapSections_SingleModule)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.stackMapSize = 128;
    info.startIndex = 5;
    info.funcCount = 10;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateArkStackMapSections(des, secAddr, secOffset, moduleInfoOffset);

    ASSERT_EQ(des[0].GetArkStackMapSize(), 128u);
    ASSERT_EQ(des[0].GetStartIndex(), 5u);
    ASSERT_EQ(des[0].GetFuncCount(), 10u);
    ASSERT_NE(des[0].GetArkStackMapRawPtr(), nullptr);
    ASSERT_EQ(secOffset, 128u);

    FreeBuffer(buffer);
}

// Test SeparateArkStackMapSections with multiple modules accumulating offsets
HWTEST_F_L0(ElfReaderTest, SeparateArkStackMapSections_MultipleModules)
{
    std::vector<ModuleSectionDes::ModuleRegionInfo> infos(2);
    infos[0].stackMapSize = 64;
    infos[0].startIndex = 0;
    infos[0].funcCount = 8;
    infos[1].stackMapSize = 96;
    infos[1].startIndex = 8;
    infos[1].funcCount = 12;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, infos);

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(2);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateArkStackMapSections(des, secAddr, secOffset, moduleInfoOffset);

    ASSERT_EQ(des[0].GetArkStackMapSize(), 64u);
    ASSERT_EQ(des[0].GetStartIndex(), 0u);
    ASSERT_EQ(des[0].GetFuncCount(), 8u);
    ASSERT_EQ(des[1].GetArkStackMapSize(), 96u);
    ASSERT_EQ(des[1].GetStartIndex(), 8u);
    ASSERT_EQ(des[1].GetFuncCount(), 12u);
    ASSERT_EQ(secOffset, 160u); // 64 + 96

    FreeBuffer(buffer);
}

// Test SeparateArkStackMapSections with zero stackMapSize
HWTEST_F_L0(ElfReaderTest, SeparateArkStackMapSections_ZeroStackMapSize)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.stackMapSize = 0;
    info.startIndex = 0;
    info.funcCount = 0;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateArkStackMapSections(des, secAddr, secOffset, moduleInfoOffset);

    ASSERT_EQ(des[0].GetArkStackMapSize(), 0u);
    ASSERT_EQ(des[0].GetStartIndex(), 0u);
    ASSERT_EQ(des[0].GetFuncCount(), 0u);
    ASSERT_EQ(secOffset, 0u);

    FreeBuffer(buffer);
}

// Test SeparateArkStackMapSections with non-zero starting offset
HWTEST_F_L0(ElfReaderTest, SeparateArkStackMapSections_NonZeroStartingOffset)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.stackMapSize = 64;
    info.startIndex = 2;
    info.funcCount = 4;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 200; // Start from non-zero offset

    reader.SeparateArkStackMapSections(des, secAddr, secOffset, moduleInfoOffset);

    ASSERT_EQ(des[0].GetArkStackMapSize(), 64u);
    ASSERT_EQ(des[0].GetStartIndex(), 2u);
    ASSERT_EQ(des[0].GetFuncCount(), 4u);
    ASSERT_EQ(secOffset, 264u); // 200 + 64

    FreeBuffer(buffer);
}

// ==================== SeparateStrtabSections Tests ====================

// Test SeparateStrtabSections with a single module
HWTEST_F_L0(ElfReaderTest, SeparateStrtabSections_SingleModule)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.strtabSize = 100;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateStrtabSections(des, secAddr, secOffset, moduleInfoOffset);

    ASSERT_EQ(des[0].GetSecAddr(ElfSecName::STRTAB), secAddr);
    ASSERT_EQ(des[0].GetSecSize(ElfSecName::STRTAB), 100u);
    ASSERT_EQ(secOffset, 100u);

    FreeBuffer(buffer);
}

// Test SeparateStrtabSections with multiple modules accumulating offsets
HWTEST_F_L0(ElfReaderTest, SeparateStrtabSections_MultipleModules)
{
    std::vector<ModuleSectionDes::ModuleRegionInfo> infos(3);
    infos[0].strtabSize = 50;
    infos[1].strtabSize = 80;
    infos[2].strtabSize = 120;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, infos);

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(3);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateStrtabSections(des, secAddr, secOffset, moduleInfoOffset);

    ASSERT_EQ(des[0].GetSecAddr(ElfSecName::STRTAB), secAddr);
    ASSERT_EQ(des[0].GetSecSize(ElfSecName::STRTAB), 50u);

    ASSERT_EQ(des[1].GetSecAddr(ElfSecName::STRTAB), secAddr + 50);
    ASSERT_EQ(des[1].GetSecSize(ElfSecName::STRTAB), 80u);

    ASSERT_EQ(des[2].GetSecAddr(ElfSecName::STRTAB), secAddr + 130); // 50 + 80
    ASSERT_EQ(des[2].GetSecSize(ElfSecName::STRTAB), 120u);

    ASSERT_EQ(secOffset, 250u); // 50 + 80 + 120

    FreeBuffer(buffer);
}

// Test SeparateStrtabSections with zero size
HWTEST_F_L0(ElfReaderTest, SeparateStrtabSections_ZeroSize)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.strtabSize = 0;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateStrtabSections(des, secAddr, secOffset, moduleInfoOffset);

    ASSERT_EQ(des[0].GetSecSize(ElfSecName::STRTAB), 0u);
    ASSERT_EQ(secOffset, 0u);

    FreeBuffer(buffer);
}

// ==================== SeparateSymtabSections Tests ====================

// Test SeparateSymtabSections with a single module
HWTEST_F_L0(ElfReaderTest, SeparateSymtabSections_SingleModule)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.symtabSize = 200;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateSymtabSections(des, secAddr, secOffset, moduleInfoOffset);

    ASSERT_EQ(des[0].GetSecAddr(ElfSecName::SYMTAB), secAddr);
    ASSERT_EQ(des[0].GetSecSize(ElfSecName::SYMTAB), 200u);
    ASSERT_EQ(secOffset, 200u);

    FreeBuffer(buffer);
}

// Test SeparateSymtabSections with multiple modules accumulating offsets
HWTEST_F_L0(ElfReaderTest, SeparateSymtabSections_MultipleModules)
{
    std::vector<ModuleSectionDes::ModuleRegionInfo> infos(2);
    infos[0].symtabSize = 60;
    infos[1].symtabSize = 90;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, infos);

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(2);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateSymtabSections(des, secAddr, secOffset, moduleInfoOffset);

    ASSERT_EQ(des[0].GetSecAddr(ElfSecName::SYMTAB), secAddr);
    ASSERT_EQ(des[0].GetSecSize(ElfSecName::SYMTAB), 60u);

    ASSERT_EQ(des[1].GetSecAddr(ElfSecName::SYMTAB), secAddr + 60);
    ASSERT_EQ(des[1].GetSecSize(ElfSecName::SYMTAB), 90u);

    ASSERT_EQ(secOffset, 150u); // 60 + 90

    FreeBuffer(buffer);
}

// Test SeparateSymtabSections with non-zero starting offset
HWTEST_F_L0(ElfReaderTest, SeparateSymtabSections_NonZeroStartingOffset)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.symtabSize = 64;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 256;

    reader.SeparateSymtabSections(des, secAddr, secOffset, moduleInfoOffset);

    ASSERT_EQ(des[0].GetSecAddr(ElfSecName::SYMTAB), secAddr + 256);
    ASSERT_EQ(des[0].GetSecSize(ElfSecName::SYMTAB), 64u);
    ASSERT_EQ(secOffset, 320u); // 256 + 64

    FreeBuffer(buffer);
}

// Test SeparateSymtabSections with zero size
HWTEST_F_L0(ElfReaderTest, SeparateSymtabSections_ZeroSize)
{
    ModuleSectionDes::ModuleRegionInfo info {};
    info.symtabSize = 0;

    size_t bufferSize = 8192;
    uint8_t *buffer = AllocBuffer(bufferSize);
    ASSERT_TRUE(buffer != nullptr);
    uint64_t moduleInfoOffset = 0;
    WriteModuleInfos(buffer, moduleInfoOffset, {info});

    MemMap memMap(buffer, bufferSize);
    ElfReader reader(memMap);

    std::vector<ModuleSectionDes> des(1);
    uintptr_t secAddr = reinterpret_cast<uintptr_t>(buffer + 4096);
    llvm::ELF::Elf64_Off secOffset = 0;

    reader.SeparateSymtabSections(des, secAddr, secOffset, moduleInfoOffset);

    ASSERT_EQ(des[0].GetSecSize(ElfSecName::SYMTAB), 0u);
    ASSERT_EQ(secOffset, 0u);

    FreeBuffer(buffer);
}
}  // namespace panda::test
