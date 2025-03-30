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

#include "ecmascript/compiler/aot_file/aot_checksum_helper.h"
#include "ecmascript/compiler/aot_file/elf_builder.h"
#include "ecmascript/compiler/aot_file/elf_reader.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {
bool AnFileInfo::Save(const std::string &filename, Triple triple, size_t anFileMaxByteSize,
                      const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap)
{
    std::string realPath;
    if (!RealPath(filename, realPath, false)) {
        return false;
    }
    const char *rawPath = realPath.c_str();
    if (!TryRemoveAnFile(rawPath)) {
        LOG_COMPILER(ERROR) << "unlink an file fail";
        return false;
    };

    SetStubNum(entries_.size());
    AddFuncEntrySec();
    AddFileNameToChecksumSec(fileNameToChecksumMap);

    ElfBuilder builder(des_, GetDumpSectionNames(), false, triple);
    size_t anFileSize = builder.CalculateTotalFileSize();
    if (anFileMaxByteSize != 0) {
        if (anFileSize > anFileMaxByteSize) {
            LOG_COMPILER(ERROR) << "Expected AN file size " << anFileSize << " bytes ("
                                << (static_cast<double>(anFileSize) / 1_MB) << "MB) "
                                << "exceeds maximum allowed size of " << (static_cast<double>(anFileMaxByteSize) / 1_MB)
                                << "MB";
            return false;
        }
    }
    std::fstream file(rawPath, std::fstream::binary | std::fstream::out | std::fstream::trunc);
    llvm::ELF::Elf64_Ehdr header;
    builder.PackELFHeader(header, base::FileHeaderBase::ToVersionNumber(AOTFileVersion::AN_VERSION), triple);
    file.write(reinterpret_cast<char *>(&header), sizeof(llvm::ELF::Elf64_Ehdr));
    builder.PackELFSections(file);
    builder.PackELFSegment(file);
    if (static_cast<size_t>(file.tellp()) != anFileSize) {
        LOG_COMPILER(ERROR) << "Error to save an file: file size " << file.tellp()
                            << " not equal calculated size: " << anFileSize;
        file.close();
        TryRemoveAnFile(rawPath);
        return false;
    }
    file.close();
    return true;
}

bool AnFileInfo::LoadInternal(const std::string &filename)
{
    if (fileMapMem_.GetOriginAddr() == nullptr) {
        LOG_ECMA(ERROR) << "File mmap failed";
        return false;
    }

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
    if (!ParseChecksumInfo(des)) {
        LOG_ECMA(ERROR) << "Parse checksum info failed, stop load aot";
        return false;
    }
    ParseFunctionEntrySection(des);

    UpdateFuncEntries();

    LOG_COMPILER(INFO) << "loaded an file: " << filename.c_str();
    isLoad_ = true;
    return true;
}

bool AnFileInfo::Load(const std::string &filename)
{
    std::string realPath;
    if (!RealPath(filename, realPath, false)) {
        LOG_COMPILER(ERROR) << "Can not load aot file from path [ " << filename << " ], "
                            << "please execute ark_aot_compiler with options --aot-file.";
        return false;
    }
    if (!FileExist(realPath.c_str())) {
        LOG_ECMA(WARN) << "File not exist. file: " << realPath;
        return false;
    }

    fileMapMem_ = FileMap(realPath.c_str(), FILE_RDONLY, PAGE_PROT_READ);
    return LoadInternal(filename);
}

#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
bool AnFileInfo::Load(const std::string &filename, [[maybe_unused]] std::function<bool
    (std::string fileName, uint8_t **buff, size_t *buffSize)> ReadAOTCallBack)
{
    std::string fileName = filename;
    uint8_t *buff = nullptr;
    size_t buffSize = 0;
    size_t found = filename.find_last_of("/");
    if (found != std::string::npos) {
        fileName = filename.substr(found + 1);
    }
    
    LOG_ECMA(INFO) << "Call JsAotReader to load: " << fileName;
    if (ReadAOTCallBack(fileName, &buff, &buffSize)) {
        void* newBuff = nullptr;
        if (posix_memalign(&newBuff, sysconf(_SC_PAGESIZE), buffSize) != 0) {
            LOG_ECMA(ERROR) << "posix_memalign failed!";
            return false;
        }
        std::copy(reinterpret_cast<char*>(buff), reinterpret_cast<char*>(buff) + buffSize,
                  reinterpret_cast<char*>(newBuff));
        fileMapMem_ = MemMap(newBuff, buffSize);
    }

    return LoadInternal(filename);
}
#endif

bool AnFileInfo::TryRemoveAnFile(const char *filename)
{
    if (!FileExist(filename)) {
        return true;
    }
    if (Unlink(filename) == -1) {
        LOG_COMPILER(ERROR) << "remove " << filename << " failed and errno is " << errno;
        return false;
    }
    return true;
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

bool AnFileInfo::ParseChecksumInfo(ModuleSectionDes &des)
{
    uint64_t secAddr = des.GetSecAddr(ElfSecName::ARK_CHECKSUMINFO);
    uint32_t secSize = des.GetSecSize(ElfSecName::ARK_CHECKSUMINFO);
    const char *data = reinterpret_cast<const char *>(secAddr);
    std::unordered_map<CString, uint32_t> checksumInfoMap;
    if (!AOTChecksumHelper::DeserializeChecksumMapFromChar(data, secSize, checksumInfoMap)) {
        LOG_COMPILER(ERROR) << "Deserialize checksum info from .an file failed!";
        return false;
    }

    AnFileDataManager::GetInstance()->UnsafeMergeChecksumInfo(checksumInfoMap);
    return true;
}

void AnFileInfo::UpdateFuncEntries()
{
    ModuleSectionDes &des = des_[0];
    size_t len = entries_.size();
    for (size_t i = 0; i < len; i++) {
        FuncEntryDes &funcDes = entries_[i];
        funcDes.codeAddr_ += des.GetSecAddr(ElfSecName::TEXT);
        if (funcDes.isMainFunc_) {
            EntryKey key = std::make_pair(funcDes.abcIndexInAi_, funcDes.indexInKindOrMethodId_);
            mainEntryMap_[key] = MainFuncEntry { funcDes.codeAddr_, funcDes.fpDeltaPrevFrameSp_, funcDes.isFastCall_ };
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
        ElfSecName::ARK_FUNCENTRY,
        ElfSecName::ARK_CHECKSUMINFO
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

bool AnFileInfo::IsLoadMain(uint32_t fileIndex, const JSPandaFile *jsPandaFile, const CString &entry) const
{
    auto methodId = jsPandaFile->GetMainMethodIndex(entry);
#ifndef NDEBUG
    LOG_COMPILER(INFO) << "AnFileInfo IsLoadMain method id: " << methodId << " entry: " << entry;
#endif
    auto it = mainEntryMap_.find(std::make_pair(fileIndex, methodId));
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

void AnFileInfo::AddFileNameToChecksumSec(const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap)
{
    AOTChecksumHelper::SerializeChecksumMapToVector(fileNameToChecksumMap, checksumDataVector_);
    ModuleSectionDes &des = des_[ElfBuilder::FuncEntryModuleDesIndex];
    uint64_t checksumInfoAddr = reinterpret_cast<uint64_t>(checksumDataVector_.data());
    uint32_t checksumInfoSize = checksumDataVector_.size();
    des.SetSecAddrAndSize(ElfSecName::ARK_CHECKSUMINFO, checksumInfoAddr, checksumInfoSize);
}

void AnFileInfo::GenerateMethodToEntryIndexMap()
{
    const std::vector<AOTFileInfo::FuncEntryDes> &entries = GetStubs();
    uint32_t entriesSize = entries.size();
    for (uint32_t i = 0; i < entriesSize; ++i) {
        const AOTFileInfo::FuncEntryDes &entry = entries[i];
        std::string &fileName = entryIdxToFileNameMap_.at(i);
        auto key = std::make_pair(fileName, entry.indexInKindOrMethodId_);
        methodToEntryIndexMap_[key] = i;
    }
}
}  // namespace panda::ecmascript
