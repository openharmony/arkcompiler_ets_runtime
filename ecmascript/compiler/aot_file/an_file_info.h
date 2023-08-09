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
#ifndef ECMASCRIPT_COMPILER_AOT_FILE_AN_FILE_INFO_H
#define ECMASCRIPT_COMPILER_AOT_FILE_AN_FILE_INFO_H

#include "ecmascript/compiler/aot_file/aot_file_info.h"
#include "ecmascript/compiler/assembler/assembler.h"

namespace panda::ecmascript {
class PUBLIC_API AnFileInfo : public AOTFileInfo {
public:
    AnFileInfo() = default;
    ~AnFileInfo() override = default;
    void Save(const std::string &filename, Triple triple);
    void AddModuleDes(ModuleSectionDes &moduleDes)
    {
        des_.emplace_back(moduleDes);
        for (auto &s : moduleDes.GetSectionsInfo()) {
            auto sec = ElfSection(s.first);
            if (sec.isSequentialAOTSec()) {
                accumulateTotalSize(s.second.second);
            }
        }
        accumulateTotalSize(moduleDes.GetArkStackMapSize());
    }

    std::pair<uint64_t, bool> GetMainFuncEntry(uint32_t methodId) const
    {
        auto it = mainEntryMap_.find(methodId);
        if (it == mainEntryMap_.end()) {
            return std::make_pair(0, false);
        }
        return it->second;
    }

    void TryRemoveAnFile(const char *filename);

    void AlignTextSec(uint32_t alignSize)
    {
        curTextSecOffset_ = AlignUp(curTextSecOffset_, alignSize);
    }

    void UpdateCurTextSecOffset(uint64_t size)
    {
        curTextSecOffset_ += size;
    }

    uint64_t GetCurTextSecOffset() const
    {
        return curTextSecOffset_;
    }

    bool IsLoadMain(const JSPandaFile *jsPandaFile, const CString &entry) const;

    bool IsLoad() const
    {
        return isLoad_;
    }

    void Destroy() override;

    void Dump() const;

private:
    static const std::vector<ElfSecName> &GetDumpSectionNames();
    bool Load(const std::string &filename);
    void ParseFunctionEntrySection(ModuleSectionDes &moduleDes);
    void UpdateFuncEntries();
    void AddFuncEntrySec();
    uint64_t curTextSecOffset_ {0};
    std::unordered_map<uint32_t, std::pair<uint64_t, bool>> mainEntryMap_ {};
    bool isLoad_ {false};

    friend class AnFileDataManager;
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_COMPILER_AOT_FILE_AN_FILE_INFO_H
