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
    os::memory::WriteLockHolder lock(lock_);
    if (loadedStub_ != nullptr) {
        ExecutedMemoryAllocator::DestroyBuf(loadedStub_->GetStubsMem());
        loadedStub_ = nullptr;
    }

    for (auto &iter : loadedAn_) {
        DestroyFileMapMem(iter->GetFileMapMem());
    }
    loadedAn_.clear();
    anFileNameToIndexMap_.clear();
}

void AnFileDataManager::SafeDestroyAnData(const std::string &fileName)
{
    os::memory::WriteLockHolder lock(lock_);
    std::string anBasename = JSFilePath::GetBaseName(fileName);
    auto index = UnSafeGetFileInfoIndex(anBasename);
    if (index == INVALID_INDEX) {
        return;
    }
    auto info = UnSafeGetAnFileInfo(index);
    info->Destroy();
}

bool AnFileDataManager::SafeLoad(const std::string &fileName, Type type)
{
    os::memory::WriteLockHolder lock(lock_);
    if (type == Type::STUB) {
        if (loadedStub_ != nullptr) {
            return true;
        }
        return UnsafeLoadFromStub();
    } else {
        const std::shared_ptr<const AOTFileInfo> aotFileInfo = UnsafeFind(fileName);
        if (aotFileInfo != nullptr) {
            return true;
        }
        return UnsafeLoadFromAOT(fileName);
    }
}

std::shared_ptr<AnFileInfo> AnFileDataManager::UnsafeFind(const std::string &fileName) const
{
    // note: This method is not thread-safe
    // need to ensure that the instance of AnFileDataManager has been locked before use
    const auto iter = anFileNameToIndexMap_.find(fileName);
    if (iter == anFileNameToIndexMap_.end()) {
        return nullptr;
    }
    uint32_t index = iter->second;
    return loadedAn_.at(index);
}

bool AnFileDataManager::UnsafeLoadFromStub()
{
    // note: This method is not thread-safe
    // need to ensure that the instance of AnFileDataManager has been locked before use
    loadedStub_ = std::make_shared<StubFileInfo>(StubFileInfo());
    return loadedStub_->Load();
}

void AnFileDataManager::Dump() const
{
    loadedStub_->Dump();
    for (const auto& an : loadedAn_) {
        an->Dump();
    }
}

bool AnFileDataManager::UnsafeLoadFromAOT(const std::string &fileName)
{
    // note: This method is not thread-safe
    // need to ensure that the instance of AnFileDataManager has been locked before use
    std::shared_ptr<AnFileInfo> info = std::make_shared<AnFileInfo>(AnFileInfo());
    if (!info->Load(fileName)) {
        return false;
    }
    std::string anBasename = JSFilePath::GetBaseName(fileName);
    anFileNameToIndexMap_.insert({anBasename, loadedAn_.size()});
    loadedAn_.emplace_back(info);
    return true;
}

uint32_t AnFileDataManager::UnSafeGetFileInfoIndex(const std::string &fileName)
{
    auto iter = anFileNameToIndexMap_.find(fileName);
    if (iter == anFileNameToIndexMap_.end()) {
        return INVALID_INDEX;
    }
    return anFileNameToIndexMap_.at(fileName);
}

uint32_t AnFileDataManager::SafeGetFileInfoIndex(const std::string &fileName)
{
    os::memory::ReadLockHolder lock(lock_);
    return UnSafeGetFileInfoIndex(fileName);
}

std::shared_ptr<AnFileInfo> AnFileDataManager::SafeGetAnFileInfo(uint32_t index)
{
    os::memory::ReadLockHolder lock(lock_);
    return UnSafeGetAnFileInfo(index);
}

std::shared_ptr<StubFileInfo> AnFileDataManager::SafeGetStubFileInfo()
{
    os::memory::ReadLockHolder lock(lock_);
    return loadedStub_;
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
    os::memory::ReadLockHolder lock(lock_);
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
    os::memory::ReadLockHolder lock(lock_);
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

AOTFileInfo::CallSiteInfo AnFileDataManager::SafeCalCallSiteInfo(uintptr_t retAddr)
{
    os::memory::ReadLockHolder lock(lock_);
    AOTFileInfo::CallSiteInfo callsiteInfo;

    bool ans = false;
    if (loadedStub_ != nullptr) {
        ans = loadedStub_->CalCallSiteInfo(retAddr, callsiteInfo);
    }
    if (ans) {
        return callsiteInfo;
    }
    // aot
    for (auto &info : loadedAn_) {
        ans = info->CalCallSiteInfo(retAddr, callsiteInfo);
        if (ans) {
            return callsiteInfo;
        }
    }
    return callsiteInfo;
}
}  // namespace panda::ecmascript
