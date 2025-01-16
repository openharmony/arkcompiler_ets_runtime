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
struct MainFuncEntry {
    uint64_t mainEntry {0};
    int32_t fpDelta {0};
    bool isFastCall {false};
};

class PUBLIC_API AnFileInfo : public AOTFileInfo {
public:
    using FuncEntryIndexKey = std::pair<std::string, uint32_t>; // (compilefileName, MethodID)
    AnFileInfo() = default;
    ~AnFileInfo() override = default;
    bool PUBLIC_API Save(const std::string &filename, Triple triple, size_t anFileMaxByteSize,
                         const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap);
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

    MainFuncEntry GetMainFuncEntry(uint32_t fileIndex, uint32_t methodId) const
    {
        auto it = mainEntryMap_.find(std::make_pair(fileIndex, methodId));
        if (it == mainEntryMap_.end()) {
            return MainFuncEntry { 0, 0, false };
        }
        return it->second;
    }

    bool TryRemoveAnFile(const char *filename);

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

    bool IsLoadMain(uint32_t fileIndex, const JSPandaFile *jsPandaFile, const CString &entry) const;

    bool IsLoad() const
    {
        return isLoad_;
    }

    void Destroy() override;

    void MappingEntryFuncsToAbcFiles(std::string curCompileFileName, uint32_t start, uint32_t end)
    {
        while (start < end) {
            entryIdxToFileNameMap_[start] = curCompileFileName;
            ++start;
        }
    }

    const CMap<FuncEntryIndexKey, uint32_t>& GetMethodToEntryIndexMap() const
    {
        return methodToEntryIndexMap_;
    }

    void PUBLIC_API GenerateMethodToEntryIndexMap();

    void Dump() const;
    static const std::vector<ElfSecName> &GetDumpSectionNames();

private:
    using EntryKey = std::pair<uint32_t, uint32_t>;
    bool LoadInternal(const std::string &filename);
    bool Load(const std::string &filename);
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
    bool Load(const std::string &filename, [[maybe_unused]] std::function<bool
        (std::string fileName, uint8_t **buff, size_t *buffSize)> ReadAOTCallBack);
#endif
    void ParseFunctionEntrySection(ModuleSectionDes &moduleDes);
    bool ParseChecksumInfo(ModuleSectionDes &moduleDes);
    void UpdateFuncEntries();
    void AddFuncEntrySec();
    void AddFileNameToChecksumSec(const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap);
    uint64_t curTextSecOffset_ {0};
    // Future work: add main entry mapping to ai file
    std::map<EntryKey, MainFuncEntry> mainEntryMap_ {};
    bool isLoad_ {false};
    CUnorderedMap<uint32_t, std::string> entryIdxToFileNameMap_ {};
    CMap<FuncEntryIndexKey, uint32_t> methodToEntryIndexMap_ {};

    friend class AnFileDataManager;
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_COMPILER_AOT_FILE_AN_FILE_INFO_H
