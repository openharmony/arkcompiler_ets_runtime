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
#ifndef ECMASCRIPT_COMPILER_AOT_FILE_AN_FILE_DATA_MANAGER_H
#define ECMASCRIPT_COMPILER_AOT_FILE_AN_FILE_DATA_MANAGER_H

#include "ecmascript/compiler/aot_file/an_file_info.h"
#include "ecmascript/compiler/aot_file/stub_file_info.h"
#include "ecmascript/platform/map.h"

namespace panda::ecmascript {
class AnFileDataManager {
public:
    enum class Type : uint8_t {
        STUB = 0,
        AOT,
    };

    static AnFileDataManager *GetInstance();
    ~AnFileDataManager();

    bool SafeLoad(const std::string &fileName, Type type);
    uint32_t SafeGetFileInfoIndex(const std::string &fileName);
    std::shared_ptr<AnFileInfo> SafeGetAnFileInfo(uint32_t index);
    std::shared_ptr<StubFileInfo> SafeGetStubFileInfo();
    bool SafeTryReadLock();
    bool SafeInsideStub(uintptr_t pc);
    bool SafeInsideAOT(uintptr_t pc);
    AOTFileInfo::CallSiteInfo SafeCalCallSiteInfo(uintptr_t retAddr);
    static void DestroyFileMapMem(MemMap &fileMapMem);
    void SafeDestroyAllData();
    void SafeDestroyAnData(const std::string &fileName);

    const std::string &GetDir() const
    {
        return anDir_;
    }

    bool IsEnable() const
    {
        return anEnable_;
    }

    void Dump() const DUMP_API_ATTR;

    // only main thread call this, only call once, no need to lock
    void SetDir(std::string dir)
    {
        anDir_ = std::move(dir);
    }

    void SetEnable(bool enable)
    {
        anEnable_ = enable;
    }

private:
    AnFileDataManager() = default;
    std::shared_ptr<AnFileInfo> UnsafeFind(const std::string &fileName) const;
    bool UnsafeLoadFromAOT(const std::string &fileName);
    bool UnsafeLoadFromStub();
    uint32_t UnSafeGetFileInfoIndex(const std::string &fileName);
    std::shared_ptr<AnFileInfo> UnSafeGetAnFileInfo(uint32_t index)
    {
        return loadedAn_.at(index);
    }

    os::memory::RWLock lock_ {};
    std::unordered_map<std::string, uint32_t> anFileNameToIndexMap_ {};
    std::vector<std::shared_ptr<AnFileInfo>> loadedAn_ {};
    std::shared_ptr<StubFileInfo> loadedStub_ {nullptr};
    std::string anDir_;
    bool anEnable_ {false};
    NO_COPY_SEMANTIC(AnFileDataManager);
    NO_MOVE_SEMANTIC(AnFileDataManager);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_AOT_FILE_AN_FILE_DATA_MANAGER_H
