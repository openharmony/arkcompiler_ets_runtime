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

#include "ecmascript/compiler/aot_file/an_file_info.h"

#include <cerrno>
#include "ecmascript/compiler/aot_file/aot_version.h"
#include "ecmascript/compiler/aot_file/elf_builder.h"
#include "ecmascript/compiler/aot_file/elf_reader.h"
#include "ecmascript/compiler/binary_section.h"
#include "ecmascript/js_file_path.h"
#include "ecmascript/log.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript {
void AnFileInfo::Save(const std::string &filename, Triple triple)
{
    std::string realPath;
    if (!RealPath(filename, realPath, false)) {
        return;
    }
    const char *rawPath = realPath.c_str();
    TryRemoveAnFile(rawPath);

    std::ofstream file(rawPath, std::ofstream::binary);
    SetStubNum(entries_.size());
    AddFuncEntrySec();

    ElfBuilder builder(des_, GetDumpSectionNames());
    llvm::ELF::Elf64_Ehdr header;
    builder.PackELFHeader(header, base::FileHeaderBase::ToVersionNumber(AOTFileVersion::AN_VERSION), triple);
    file.write(reinterpret_cast<char *>(&header), sizeof(llvm::ELF::Elf64_Ehdr));
    builder.PackELFSections(file);
    builder.PackELFSegment(file);
    file.close();
}

bool AnFileInfo::Load(const std::string &filename)
{
    std::string realPath;
    if (!RealPath(filename, realPath, false)) {
        LOG_COMPILER(ERROR) << "Can not load aot file from path [ " << filename << " ], "
                            << "please execute ark_aot_compiler with options --aot-file.";
        return false;
    }

    fileMapMem_ = FileMap(realPath.c_str(), FILE_RDONLY, PAGE_PROT_READ);
    if (fileMapMem_.GetOriginAddr() == nullptr) {
        LOG_ECMA(ERROR) << "File mmap failed";
        return false;
    }
    PagePreRead(fileMapMem_.GetOriginAddr(), fileMapMem_.GetSize());

    moduleNum_ = 1;
    des_.resize(moduleNum_);
    ModuleSectionDes &des = des_[0];

    ElfReader reader(fileMapMem_);
    std::vector<ElfSecName> secs = GetDumpSectionNames();
    if (!reader.VerifyELFHeader(base::FileHeaderBase::ToVersionNumber(AOTFileVersion::AN_VERSION),
                                AOTFileVersion::AN_STRICT_MATCH)) {
        return false;
    }
    reader.ParseELFSections(des, secs);
    if (!reader.ParseELFSegment()) {
        LOG_ECMA(ERROR) << "modify mmap area permission failed";
        return false;
    }
    ParseFunctionEntrySection(des);

    UpdateFuncEntries();

    LOG_COMPILER(INFO) << "loaded an file: " << filename.c_str();
    isLoad_ = true;
    return true;
}

void AnFileInfo::TryRemoveAnFile(const char *filename)
{
    if (!FileExist(filename)) {
        return;
    }
    if (Unlink(filename) == -1) {
        LOG_COMPILER(ERROR) << "remove " << filename << " failed and errno is " << errno;
    }
}

void AnFileInfo::ParseFunctionEntrySection(ModuleSectionDes &des)
{
    uint64_t secAddr = des.GetSecAddr(ElfSecName::ARK_FUNCENTRY);
    uint32_t secSize = des.GetSecSize(ElfSecName::ARK_FUNCENTRY);
    FuncEntryDes *entryDes = reinterpret_cast<FuncEntryDes *>(secAddr);
    entryNum_ = secSize / sizeof(FuncEntryDes);
    entries_.assign(entryDes, entryDes + entryNum_);
    des.SetStartIndex(0);
    des.SetFuncCount(entryNum_);
}

void AnFileInfo::UpdateFuncEntries()
{
    ModuleSectionDes &des = des_[0];
    size_t len = entries_.size();
    for (size_t i = 0; i < len; i++) {
        FuncEntryDes &funcDes = entries_[i];
        funcDes.codeAddr_ += des.GetSecAddr(ElfSecName::TEXT);
        if (funcDes.isMainFunc_) {
            mainEntryMap_[funcDes.indexInKindOrMethodId_] = std::make_pair(funcDes.codeAddr_, funcDes.isFastCall_);
#ifndef NDEBUG
            LOG_COMPILER(INFO) << "AnFileInfo Load main method id: " << funcDes.indexInKindOrMethodId_
                               << " code addr: " << reinterpret_cast<void *>(funcDes.codeAddr_);
#endif
        }
    }
}

const std::vector<ElfSecName> &AnFileInfo::GetDumpSectionNames()
{
    static const std::vector<ElfSecName> secNames = {
        ElfSecName::TEXT,
        ElfSecName::STRTAB,
        ElfSecName::SYMTAB,
        ElfSecName::SHSTRTAB,
        ElfSecName::ARK_STACKMAP,
        ElfSecName::ARK_FUNCENTRY
    };
    return secNames;
}

void AnFileInfo::Destroy()
{
    mainEntryMap_.clear();
    isLoad_ = false;
    curTextSecOffset_ = 0;
    AOTFileInfo::Destroy();
}

void AnFileInfo::Dump() const
{
    LOG_COMPILER(INFO) << "An file loading: ";
    int i = 0;
    for (const ModuleSectionDes &d : des_) {
        i++;
        for (const auto &s : d.GetSectionsInfo()) {
            std::string name = d.GetSecName(s.first);
            uint32_t size = d.GetSecSize(s.first);
            uint64_t addr = d.GetSecAddr(s.first);
            LOG_COMPILER(INFO) << " - module-" << i << " <" << name << "> [0x" << std::hex << addr << ", 0x"
                                << std::hex << addr + size << "]";
        }
    }
}

bool AnFileInfo::IsLoadMain(const JSPandaFile *jsPandaFile, const CString &entry) const
{
    auto methodId = jsPandaFile->GetMainMethodIndex(entry);
#ifndef NDEBUG
    LOG_COMPILER(INFO) << "AnFileInfo IsLoadMain method id: " << methodId << " entry: " << entry;
#endif
    auto it = mainEntryMap_.find(methodId);
    return it != mainEntryMap_.end();
}

void AnFileInfo::AddFuncEntrySec()
{
    ModuleSectionDes &des = des_[ElfBuilder::FuncEntryModuleDesIndex];
    // add section
    uint64_t funcEntryAddr = reinterpret_cast<uint64_t>(entries_.data());
    uint32_t funcEntrySize = sizeof(FuncEntryDes) * entryNum_;
    des.SetSecAddrAndSize(ElfSecName::ARK_FUNCENTRY, funcEntryAddr, funcEntrySize);
}
}  // namespace panda::ecmascript
