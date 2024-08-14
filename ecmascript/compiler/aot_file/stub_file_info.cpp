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

#include "ecmascript/compiler/aot_file/stub_file_info.h"
#include "ecmascript/compiler/aot_file/elf_builder.h"
#include "ecmascript/compiler/aot_file/elf_reader.h"

#ifndef PANDA_TARGET_OHOS
extern const uint8_t _binary_stub_an_start[];
extern const uint32_t _binary_stub_an_length;
#endif

namespace panda::ecmascript {
void StubFileInfo::Save(const std::string &filename, Triple triple)
{
    std::string realPath;
    if (!RealPath(filename, realPath, false)) {
        return;
    }

    std::ofstream file(realPath.c_str(), std::ofstream::binary);
    ASSERT(GetCodeUnitsNum() == ASMSTUB_MODULE_NUM);
    SetStubNum(entries_.size());
    ModuleSectionDes &des = des_[0];
    size_t lastModuleSectionIdx = ASMSTUB_MODULE_NUM - 1;
    // add section
    uint64_t funcEntryAddr = reinterpret_cast<uint64_t>(entries_.data());
    uint32_t funcEntrySize = sizeof(FuncEntryDes) * entryNum_;
    des.SetSecAddrAndSize(ElfSecName::ARK_FUNCENTRY, funcEntryAddr, funcEntrySize);
    uint64_t asmStubAddr = GetAsmStubAddr();
    uint32_t asmStubSize = GetAsmStubSize();
    des.SetSecAddrAndSize(ElfSecName::ARK_ASMSTUB, asmStubAddr, asmStubSize);
    std::vector<uint32_t> moduleInfo = {1};
    uint64_t secSizeInfoAddr = reinterpret_cast<uint64_t>(moduleInfo.data());
    des.SetSecAddrAndSize(ElfSecName::ARK_MODULEINFO, secSizeInfoAddr, sizeof(uint32_t));
    des_[lastModuleSectionIdx].AddAsmStubELFInfo(asmStubELFInfo_);

    ElfBuilder builder(des_, GetDumpSectionNames());
    llvm::ELF::Elf64_Ehdr header;
    builder.PackELFHeader(header, base::FileHeaderBase::ToVersionNumber(AOTFileVersion::AN_VERSION), triple);
    file.write(reinterpret_cast<char *>(&header), sizeof(llvm::ELF::Elf64_Ehdr));
    builder.PackELFSections(file);
    builder.PackELFSegment(file);
    file.close();
}

bool StubFileInfo::MmapLoad(const std::string &fileName)
{
    std::string filename = fileName.empty() ? STUB_AN_FILE : fileName;
    std::string realPath;
    if (!RealPath(filename, realPath, false)) {
        LOG_COMPILER(ERROR) << "Can not load stub file from path [ " << filename << " ], "
                            << "please execute ark_stub_compiler with options --stub-file.";
        return false;
    }

    if (!FileExist(realPath.c_str())) {
        LOG_ECMA(WARN) << "File not exist. file: " << realPath;
        return false;
    }

    fileMapMem_ = FileMap(realPath.c_str(), FILE_RDONLY, PAGE_PROT_READ);
    if (fileMapMem_.GetOriginAddr() == nullptr) {
        LOG_ECMA(ERROR) << "File mmap failed";
        return false;
    }

    ElfReader reader(fileMapMem_);
    std::vector<ElfSecName> secs = GetDumpSectionNames();
    reader.ParseELFSections(des_, secs);
    moduleNum_ = des_.size();
    ASSERT(moduleNum_ == ASMSTUB_MODULE_NUM);

    if (!reader.ParseELFSegment()) {
        LOG_ECMA(ERROR) << "modify mmap area permission failed";
        return false;
    }

    ModuleSectionDes &des = des_[0];
    uint64_t funcEntryAddr = des.GetSecAddr(ElfSecName::ARK_FUNCENTRY);
    uint32_t funcEntrySize = des.GetSecSize(ElfSecName::ARK_FUNCENTRY);
    FuncEntryDes *entryDes = reinterpret_cast<FuncEntryDes *>(funcEntryAddr);
    entryNum_ = funcEntrySize / sizeof(FuncEntryDes);
    entries_.assign(entryDes, entryDes + entryNum_);
    uint64_t asmStubAddr = des.GetSecAddr(ElfSecName::ARK_ASMSTUB);
    uint32_t asmStubSize = des.GetSecSize(ElfSecName::ARK_ASMSTUB);
    SetAsmStubAddr(asmStubAddr);
    SetAsmStubSize(asmStubSize);

    for (auto &entry : entries_) {
        if (entry.IsGeneralRTStub()) {
            uint64_t begin = GetAsmStubAddr();
            entry.codeAddr_ += begin;
        } else {
            auto moduleDes = des_[entry.moduleIndex_];
            entry.codeAddr_ += moduleDes.GetSecAddr(ElfSecName::TEXT);
        }
    }
    LOG_COMPILER(INFO) << "loaded stub file successfully";
    return true;
}
#ifndef PANDA_TARGET_OHOS
bool StubFileInfo::Load()
{
    if (_binary_stub_an_length <= 1) {
        LOG_FULL(FATAL) << "stub.an length <= 1, is default and invalid.";
        return false;
    }

    BinaryBufferParser binBufparser(const_cast<uint8_t *>(_binary_stub_an_start), _binary_stub_an_length);
    moduleNum_ = ASMSTUB_MODULE_NUM;
    des_.resize(moduleNum_);

    ExecutedMemoryAllocator::AllocateBuf(_binary_stub_an_length, stubsMem_, PAGE_PROT_READWRITE);

    ElfReader reader(stubsMem_);
    std::vector<ElfSecName> secs = GetDumpSectionNames();
    reader.ParseELFSections(binBufparser, des_, secs);

    ModuleSectionDes &des = des_[0];
    uint64_t funcEntryAddr = des.GetSecAddr(ElfSecName::ARK_FUNCENTRY);
    uint32_t funcEntrySize = des.GetSecSize(ElfSecName::ARK_FUNCENTRY);
    FuncEntryDes *entryDes = reinterpret_cast<FuncEntryDes *>(funcEntryAddr);
    entryNum_ = funcEntrySize / sizeof(FuncEntryDes);
    entries_.assign(entryDes, entryDes + entryNum_);
    uint64_t asmStubAddr = des.GetSecAddr(ElfSecName::ARK_ASMSTUB);
    uint32_t asmStubSize = des.GetSecSize(ElfSecName::ARK_ASMSTUB);
    SetAsmStubAddr(asmStubAddr);
    SetAsmStubSize(asmStubSize);

    for (auto &entry : entries_) {
        if (entry.IsGeneralRTStub()) {
            uint64_t begin = GetAsmStubAddr();
            entry.codeAddr_ += begin;
        } else {
            auto moduleDes = des_[entry.moduleIndex_];
            entry.codeAddr_ += moduleDes.GetSecAddr(ElfSecName::TEXT);
        }
    }
    LOG_COMPILER(INFO) << "loaded stub file successfully";
    if (!PageProtect(stubsMem_.addr_, stubsMem_.size_, PAGE_PROT_EXEC_READ)) {
        return false;
    }
    return true;
}
#endif
const std::vector<ElfSecName> &StubFileInfo::GetDumpSectionNames()
{
    static const std::vector<ElfSecName> secNames = {
        ElfSecName::TEXT,
        ElfSecName::STRTAB,
        ElfSecName::SYMTAB,
        ElfSecName::SHSTRTAB,
        ElfSecName::ARK_STACKMAP,
        ElfSecName::ARK_FUNCENTRY,
        ElfSecName::ARK_ASMSTUB,
        ElfSecName::ARK_MODULEINFO
    };
    return secNames;
}

void StubFileInfo::Dump() const
{
    uint64_t asmAddr = GetAsmStubAddr();
    uint64_t asmSize = GetAsmStubSize();

    LOG_COMPILER(ERROR) << "Stub file loading: ";
    LOG_COMPILER(ERROR) << " - asm stubs [0x" << std::hex << asmAddr << ", 0x" << std::hex << asmAddr + asmSize << "]";

    for (const ModuleSectionDes &d : des_) {
        for (const auto &s : d.GetSectionsInfo()) {
            std::string name = d.GetSecName(s.first);
            uint32_t size = d.GetSecSize(s.first);
            uint64_t addr = d.GetSecAddr(s.first);
            LOG_COMPILER(ERROR) << " - section = " << name << " [0x" << std::hex << addr << ", 0x" << std::hex
                                << addr + size << "]";
        }
    }
}
}  // namespace panda::ecmascript
