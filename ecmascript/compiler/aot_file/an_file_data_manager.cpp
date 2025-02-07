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
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/js_file_path.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript {
AnFileDataManager *AnFileDataManager::GetInstance()
{
    static AnFileDataManager *anFileDataManager = new AnFileDataManager();
    return anFileDataManager;
}

AnFileDataManager::~AnFileDataManager()
{
    SafeDestroyAllData();
}

void AnFileDataManager::DestroyFileMapMem(MemMap &fileMapMem)
{
    if (fileMapMem.GetOriginAddr() != nullptr && fileMapMem.GetSize() > 0) {
        FileUnMap(fileMapMem);
        fileMapMem.Reset();
    }
}

void AnFileDataManager::SafeDestroyAllData()
{
    WriteLockHolder lock(lock_);
    if (loadedStub_ != nullptr) {
        loadedStub_->UnregisterFromDebugger();
        ExecutedMemoryAllocator::DestroyBuf(loadedStub_->GetStubsMem());
        DestroyFileMapMem(loadedStub_->GetFileMapMem());
        loadedStub_ = nullptr;
    }

    for (auto &iter : loadedAn_) {
        DestroyFileMapMem(iter->GetFileMapMem());
    }
    loadedAn_.clear();
    anFileNameVector_.clear();
}

void AnFileDataManager::SafeDestroyAnData(const std::string &fileName)
{
    WriteLockHolder lock(lock_);
    std::string anBasename = JSFilePath::GetBaseName(fileName);
    auto index = UnSafeGetFileInfoIndex(anBasename);
    if (index == INVALID_INDEX) {
        return;
    }
    auto info = UnSafeGetAnFileInfo(index);
    info->Destroy();
}

bool AnFileDataManager::SafeLoad(const std::string &fileName, Type type, [[maybe_unused]] std::function<bool
    (std::string fileName, uint8_t **buff, size_t *buffSize)> cb)
{
    WriteLockHolder lock(lock_);
    if (type == Type::STUB) {
        if (loadedStub_ != nullptr) {
            return true;
        }
        return UnsafeLoadFromStub(fileName);
    } else {
        const std::shared_ptr<const AOTFileInfo> aotFileInfo = UnsafeFind(fileName);
        if (aotFileInfo != nullptr) {
            return true;
        }
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
        return UnsafeLoadFromAOT(fileName, cb);
#else
        return UnsafeLoadFromAOT(fileName);
#endif
    }
}

std::shared_ptr<AnFileInfo> AnFileDataManager::UnsafeFind(const std::string &fileName) const
{
    // note: This method is not thread-safe
    // need to ensure that the instance of AnFileDataManager has been locked before use
    const auto iter = std::find(anFileNameVector_.begin(), anFileNameVector_.end(), fileName);
    if (iter == anFileNameVector_.end()) {
        return nullptr;
    }
    uint32_t index = std::distance(anFileNameVector_.begin(), iter);
    return loadedAn_.at(index);
}

bool AnFileDataManager::UnsafeLoadFromStub(const std::string &fileName)
{
    // note: This method is not thread-safe
    // need to ensure that the instance of AnFileDataManager has been locked before use
    loadedStub_ = std::make_shared<StubFileInfo>(StubFileInfo());
    if (!fileName.empty()) {
        return loadedStub_->MmapLoad(fileName);
    }
#if defined(PANDA_TARGET_OHOS)
    return loadedStub_->MmapLoad(fileName);
#else
    return loadedStub_->Load();
#endif
}

void AnFileDataManager::Dump() const
{
    loadedStub_->Dump();
    for (const auto& an : loadedAn_) {
        an->Dump();
    }
}

bool AnFileDataManager::UnsafeLoadFromAOTInternal(const std::string &fileName, std::shared_ptr<AnFileInfo> &info)
{
    std::string anBasename = JSFilePath::GetBaseName(fileName);
    anFileNameVector_.emplace_back(anBasename);
    loadedAn_.emplace_back(info);
    return true;
}

bool AnFileDataManager::UnsafeLoadFromAOT(const std::string &fileName)
{
    // note: This method is not thread-safe
    // need to ensure that the instance of AnFileDataManager has been locked before use
    std::shared_ptr<AnFileInfo> info = std::make_shared<AnFileInfo>(AnFileInfo());
    if (!info->Load(fileName)) {
        return false;
    }
    return UnsafeLoadFromAOTInternal(fileName, info);
}

#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
bool AnFileDataManager::UnsafeLoadFromAOT(const std::string &fileName, std::function<bool
    (std::string fileName, uint8_t **buff, size_t *buffSize)> cb)
{
    // note: This method is not thread-safe
    // need to ensure that the instance of AnFileDataManager has been locked before use
    std::shared_ptr<AnFileInfo> info = std::make_shared<AnFileInfo>(AnFileInfo());
    if (!info->Load(fileName, cb)) {
        return false;
    }
    return UnsafeLoadFromAOTInternal(fileName, info);
}
#endif

uint32_t AnFileDataManager::UnSafeGetFileInfoIndex(const std::string &fileName)
{
    const auto iter = std::find(anFileNameVector_.begin(), anFileNameVector_.end(), fileName);
    if (iter == anFileNameVector_.end()) {
        return INVALID_INDEX;
    }
    return std::distance(anFileNameVector_.begin(), iter);
}

uint32_t AnFileDataManager::SafeGetFileInfoIndex(const std::string &fileName)
{
    ReadLockHolder lock(lock_);
    return UnSafeGetFileInfoIndex(fileName);
}

std::shared_ptr<AnFileInfo> AnFileDataManager::SafeGetAnFileInfo(uint32_t index)
{
    ReadLockHolder lock(lock_);
    return UnSafeGetAnFileInfo(index);
}

std::shared_ptr<StubFileInfo> AnFileDataManager::SafeGetStubFileInfo()
{
    ReadLockHolder lock(lock_);
    return loadedStub_;
}

std::string AnFileDataManager::UnSafeGetAnFileNameNoSuffix(uint32_t index)
{
    // The anFileNames are stored in the order of loading. So we can get the an file name by index
    std::string fileName = anFileNameVector_.at(index);
    size_t pos = fileName.find_last_of(".");
    if (pos != std::string::npos) {
        return fileName.substr(0, pos);
    }
    return "";
}

std::string AnFileDataManager::SafeGetAnFileNameNoSuffix(uint32_t index)
{
    ReadLockHolder lock(lock_);
    return UnSafeGetAnFileNameNoSuffix(index);
}

void AnFileDataManager::SafeMergeChecksumInfo(const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap)
{
    WriteLockHolder lock(lock_);
    UnsafeMergeChecksumInfo(fileNameToChecksumMap);
}

// this method is not thread safe,please make sure have lock
void AnFileDataManager::UnsafeMergeChecksumInfo(const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap)
{
    if (fullFileNameToChecksumMap_.empty()) {
        fullFileNameToChecksumMap_ = fileNameToChecksumMap;
        return;
    }
    for (const auto &newPair : fileNameToChecksumMap) {
        bool alreadyHasChecksum = false;
        for (auto &fullPair : fullFileNameToChecksumMap_) {
            if (fullPair.first == newPair.first) {
                alreadyHasChecksum = true;
                VerifyChecksumOrSetToInvalid(newPair, fullPair);
                break;
            }
        }
        if (!alreadyHasChecksum) {
            fullFileNameToChecksumMap_.emplace(newPair.first, newPair.second);
        }
    }
}

void AnFileDataManager::VerifyChecksumOrSetToInvalid(const std::pair<CString, uint32_t> &newPair,
                                                     std::pair<const CString, uint32_t> &fullPair)
{
    if (fullPair.second == static_cast<uint32_t>(-1)) {
        LOG_ECMA(ERROR) << "MergeChecksumInfo fail because checksum is INVALID. "
                        << " file name: " << newPair.first;
        return;
    }
    if (newPair.second != fullPair.second) {
        LOG_ECMA(ERROR) << "MergeChecksumInfo fail because of different checksum, set to INVALID. "
                        << " file name: " << newPair.first << "old checksum: " << fullPair.second
                        << " new checksum: " << newPair.second;
        fullPair.second = static_cast<uint32_t>(-1);
    }
}

const std::unordered_map<CString, uint32_t> &AnFileDataManager::SafeGetfullFileNameToChecksumMap()
{
    ReadLockHolder lock(lock_);
    return UnsafeGetfullFileNameToChecksumMap();
}

const std::unordered_map<CString, uint32_t> &AnFileDataManager::UnsafeGetfullFileNameToChecksumMap() const
{
    return fullFileNameToChecksumMap_;
}

bool AnFileDataManager::SafeCheckFilenameToChecksum(const CString &fileName, uint32_t checksum)
{
    ReadLockHolder lock(lock_);
    return UnsafeCheckFilenameToChecksum(fileName, checksum);
}

bool AnFileDataManager::UnsafeCheckFilenameToChecksum(const CString &fileName, uint32_t checksum)
{
    for (const auto &fileNameToChecksumPair : fullFileNameToChecksumMap_) {
        if (fileName == fileNameToChecksumPair.first) {
            if (fileNameToChecksumPair.second == static_cast<uint32_t>(-1) ||
                fileNameToChecksumPair.second != checksum) {
                LOG_COMPILER(ERROR) << "an file verify checksum fail: checksum is invalid or different. FileName: "
                                    << fileName << " old checksum: " << fileNameToChecksumPair.second
                                    << " provided new checksum " << checksum;
                return false;
            } else {
                return true;
            }
        }
    }
    return true;
}

// Using for cpuprofiler to check if the ReadLock can be held in signal handler, to avoid the reentrancy deadlock.
bool AnFileDataManager::SafeTryReadLock()
{
    // Try to acquire the lock when the signal callback starts to execute. At this time, the vm thread is interrupted,
    // so the status of whether the lock is held by vm thread will not change until the signal callback ends. If the
    // attempt is successful, it means that the current vm thread does not hold the lock，the reentrancy problem will
    // not occur in the callback.
    if (lock_.TryReadLock()) {
        lock_.Unlock();
        return true;
    }
    return false;
}

bool AnFileDataManager::SafeInsideStub(uintptr_t pc)
{
    ReadLockHolder lock(lock_);
    if (loadedStub_ == nullptr) {
        LOG_COMPILER(ERROR) << "SafeInsideStub: The stub file is not loaded.";
        return false;
    }

    uint64_t stubStartAddr = loadedStub_->GetAsmStubAddr();
    uint64_t stubEndAddr = stubStartAddr + loadedStub_->GetAsmStubSize();
    if (pc >= stubStartAddr && pc <= stubEndAddr) {
        return true;
    }

    const std::vector<ModuleSectionDes> &des = loadedStub_->GetCodeUnits();
    for (const auto &curDes : des) {
        if (curDes.ContainCode(pc)) {
            return true;
        }
    }

    return false;
}

bool AnFileDataManager::SafeInsideAOT(uintptr_t pc)
{
    ReadLockHolder lock(lock_);
    for (auto &info : loadedAn_) {
        const std::vector<ModuleSectionDes> &des = info->GetCodeUnits();
        for (const auto &curDes : des) {
            if (curDes.ContainCode(pc)) {
                return true;
            }
        }
    }
    return false;
}

AOTFileInfo::CallSiteInfo AnFileDataManager::SafeCalCallSiteInfo(uintptr_t retAddr, bool isDeopt)
{
    ReadLockHolder lock(lock_);
    AOTFileInfo::CallSiteInfo callsiteInfo;

    bool ans = false;
    if (loadedStub_ != nullptr) {
        ans = loadedStub_->CalCallSiteInfo(retAddr, callsiteInfo, true, isDeopt);
    }
    if (ans) {
        return callsiteInfo;
    }
    // aot
    for (auto &info : loadedAn_) {
        ans = info->CalCallSiteInfo(retAddr, callsiteInfo, false, isDeopt);
        if (ans) {
            return callsiteInfo;
        }
    }
    return callsiteInfo;
}
}  // namespace panda::ecmascript
