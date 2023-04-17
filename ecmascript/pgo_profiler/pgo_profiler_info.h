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

#ifndef ECMASCRIPT_PGO_PROFILER_INFO_H
#define ECMASCRIPT_PGO_PROFILER_INFO_H

#include <memory>
#include <sstream>
#include <string.h>

#include "ecmascript/base/file_header.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/native_area_allocator.h"

namespace panda::ecmascript {
class SaveTask;

enum class SampleMode : uint8_t {
    HOTNESS_MODE,
    CALL_MODE,
};

struct SectionInfo {
    uint32_t offset_ {0};
    // reserve
    uint32_t size_ {0};
    uint32_t number_ {0};
};
static constexpr size_t ALIGN_SIZE = 4;

/**
 * |----PGOProfilerHeader
 * |--------MAGIC
 * |--------VERSION
 * |--------SECTION_NUMBER
 * |--------PANDA_FILE_INFO_SECTION_INFO
 * |------------offset
 * |------------size (reserve)
 * |------------number
 * |--------RECORD_INFO_SECTION_INFO
 * |------------offset
 * |------------size (reserve)
 * |------------number
 * |----PGOPandaFileInfos
 * |--------SIZE
 * |--------CHECK_SUM
 * |--------...
 * |----PGORecordDetailInfos
 * |--------PGOMethodInfoMap
 * |------------PGOMethodInfo
 * |----------------size
 * |----------------id
 * |----------------count
 * |----------------mode
 * |----------------methodName
 * |----------------...
 * |----PGORecordSimpleInfos
 * |--------PGOMethodIdSet
 * |------------id
 * |------------...
 */
class PGOProfilerHeader : public base::FileHeader {
public:
    static constexpr VersionType LAST_VERSION = {0, 0, 0, 1};
    static constexpr size_t SECTION_SIZE = 2;
    static constexpr size_t PANDA_FILE_SECTION_INDEX = 0;
    static constexpr size_t RECORD_INFO_SECTION_INDEX = 1;

    PGOProfilerHeader() : base::FileHeader(LAST_VERSION), sectionNumber_(SECTION_SIZE)
    {
        GetPandaInfoSection()->offset_ = Size();
    }

    static size_t LastSize()
    {
        return sizeof(PGOProfilerHeader) + (SECTION_SIZE - 1) * sizeof(SectionInfo);
    }

    size_t Size() const
    {
        return sizeof(PGOProfilerHeader) + (sectionNumber_ - 1) * sizeof(SectionInfo);
    }

    bool Verify() const
    {
        return VerifyInner("apPath file", LAST_VERSION);
    }

    static void Build(PGOProfilerHeader **header, size_t size)
    {
        *header = reinterpret_cast<PGOProfilerHeader *>(malloc(size));
        new (*header) PGOProfilerHeader();
    }

    static void Destroy(PGOProfilerHeader **header)
    {
        if (*header != nullptr) {
            free(*header);
            *header = nullptr;
        }
    }

    // Copy Header.
    static bool ParseFromBinary(void *buffer, PGOProfilerHeader **header);
    void ProcessToBinary(std::ofstream &fileStream) const;

    bool ParseFromText(std::ifstream &stream);
    bool ProcessToText(std::ofstream &stream) const;

    SectionInfo *GetPandaInfoSection() const
    {
        return GetSectionInfo(PANDA_FILE_SECTION_INDEX);
    }

    SectionInfo *GetRecordInfoSection() const
    {
        return GetSectionInfo(RECORD_INFO_SECTION_INDEX);
    }

    NO_COPY_SEMANTIC(PGOProfilerHeader);
    NO_MOVE_SEMANTIC(PGOProfilerHeader);

private:
    SectionInfo *GetSectionInfo(size_t index) const
    {
        if (index >= SECTION_SIZE) {
            return nullptr;
        }
        return const_cast<SectionInfo *>(&sectionInfos_) + index;
    }

    uint32_t sectionNumber_ {SECTION_SIZE};
    SectionInfo sectionInfos_;
};

class PGOPandaFileInfos {
public:
    void Sample(uint32_t checksum)
    {
        pandaFileInfos_.insert(checksum);
    }

    void Clear()
    {
        pandaFileInfos_.clear();
    }

    void ParseFromBinary(void *buffer, SectionInfo *const info);
    void ProcessToBinary(std::ofstream &fileStream, SectionInfo *info) const;

    void ProcessToText(std::ofstream &stream) const;
    bool ParseFromText(std::ifstream &stream);

    bool CheckSum(uint32_t checksum) const;

private:
    class PandaFileInfo {
    public:
        PandaFileInfo() = default;
        PandaFileInfo(uint32_t checksum) : size_(LastSize()), checksum_(checksum) {}

        static size_t LastSize()
        {
            return sizeof(PandaFileInfo);
        }

        size_t Size()
        {
            return static_cast<size_t>(size_);
        }

        bool operator<(const PandaFileInfo &right) const
        {
            return checksum_ < right.checksum_;
        }

        uint32_t GetChecksum() const
        {
            return checksum_;
        }
    private:
        // Support extended fields
        uint32_t size_;
        uint32_t checksum_;
    };

    std::set<PandaFileInfo> pandaFileInfos_;
};

class PGOMethodInfo {
public:
    static constexpr int METHOD_INFO_COUNT = 4;
    static constexpr int METHOD_ID_INDEX = 0;
    static constexpr int METHOD_COUNT_INDEX = 1;
    static constexpr int METHOD_MODE_INDEX = 2;
    static constexpr int METHOD_NAME_INDEX = 3;

    PGOMethodInfo(EntityId id) : id_(id) {}

    PGOMethodInfo(EntityId id, uint32_t count, SampleMode mode, const char *methodName)
        : id_(id), count_(count), mode_(mode)
    {
        size_t len = strlen(methodName);
        size_ = static_cast<uint32_t>(Size(len));
        if (len > 0 && memcpy_s(&methodName_, len, methodName, len) != EOK) {
            LOG_ECMA(ERROR) << "SetMethodName memcpy_s failed" << methodName << ", len = " << len;
            UNREACHABLE();
        }
        *(&methodName_ + len) = '\0';
    }

    static int32_t Size(uint32_t length)
    {
        return sizeof(PGOMethodInfo) + AlignUp(length, ALIGN_SIZE);
    }

    int32_t Size() const
    {
        return size_;
    }

    static bool GetSampleMode(std::string content, SampleMode &mode)
    {
        if (content == "HOTNESS_MODE") {
            mode = SampleMode::HOTNESS_MODE;
        } else if (content == "CALL_MODE") {
            mode = SampleMode::CALL_MODE;
        } else {
            return false;
        }
        return true;
    }

    void IncreaseCount()
    {
        count_++;
    }

    void ClearCount()
    {
        count_ = 0;
    }

    void Merge(const PGOMethodInfo *info)
    {
        if (!(id_ == info->GetMethodId())) {
            LOG_ECMA(ERROR) << "The method id must same for merging";
            return;
        }
        count_ += info->GetCount();
        SetSampleMode(info->GetSampleMode());
    }

    EntityId GetMethodId() const
    {
        return id_;
    }

    uint32_t GetCount() const
    {
        return count_;
    }

    const char *GetMethodName() const
    {
        return &methodName_;
    }

    void SetSampleMode(SampleMode mode)
    {
        if (mode_ == SampleMode::HOTNESS_MODE) {
            return;
        }
        mode_ = mode;
    }

    SampleMode GetSampleMode() const
    {
        return mode_;
    }

    std::string GetSampleModeToString() const
    {
        std::string result;
        switch (mode_) {
            case SampleMode::HOTNESS_MODE:
                result = "HOTNESS_MODE";
                break;
            case SampleMode::CALL_MODE:
                result = "CALL_MODE";
                break;
            default:
                LOG_ECMA(ERROR) << "mode error";
        }
        return result;
    }

    bool IsFilter(uint32_t threshold) const
    {
        if (count_ < threshold && mode_ == SampleMode::CALL_MODE) {
            return true;
        }
        return false;
    }

    void ParseFromBinary(void **buffer);
    void ProcessToBinary(std::ofstream &fileStream) const;

    static std::vector<std::string> ParseFromText(const std::string &infoString);
    void ProcessToText(std::string &text) const;

    NO_COPY_SEMANTIC(PGOMethodInfo);
    NO_MOVE_SEMANTIC(PGOMethodInfo);

private:
    uint32_t size_ {0};
    EntityId id_;
    uint32_t count_ {0};
    SampleMode mode_ {SampleMode::CALL_MODE};
    char methodName_;
};

class PGOMethodInfoMap {
public:
    PGOMethodInfoMap() = default;

    void Clear()
    {
        // PGOMethodInfo release by chunk
        methodInfos_.clear();
    }

    bool AddMethod(Chunk *chunk, EntityId methodId, const CString &methodName, SampleMode mode);
    void Merge(Chunk *chunk, PGOMethodInfoMap *methodInfos);

    bool ParseFromBinary(uint32_t threshold, void **buffer);
    bool ProcessToBinary(uint32_t threshold, const CString &recordName, const SaveTask *task,
        std::ofstream &fileStream) const;

    bool ParseFromText(Chunk *chunk, uint32_t threshold, const std::vector<std::string> &content);
    void ProcessToText(uint32_t threshold, const CString &recordName, std::ofstream &stream) const;

    NO_COPY_SEMANTIC(PGOMethodInfoMap);
    NO_MOVE_SEMANTIC(PGOMethodInfoMap);

private:
    CMap<EntityId, PGOMethodInfo *> methodInfos_;
};

class PGOMethodIdSet {
public:
    PGOMethodIdSet() = default;

    bool Match(EntityId methodId)
    {
        return methodIdSet_.find(methodId) != methodIdSet_.end();
    }

    template <typename Callback>
    bool Update(const CString &recordName, Callback callback)
    {
        std::unordered_set<EntityId> newIds = callback(recordName, methodIdSet_);
        if (!newIds.empty()) {
            methodIdSet_.insert(newIds.begin(), newIds.end());
            return true;
        }
        return false;
    }

    bool ParseFromBinary(uint32_t threshold, void **buffer);

    NO_COPY_SEMANTIC(PGOMethodIdSet);
    NO_MOVE_SEMANTIC(PGOMethodIdSet);
private:
    std::unordered_set<EntityId> methodIdSet_;
};

class PGORecordDetailInfos {
public:
    explicit PGORecordDetailInfos(uint32_t hotnessThreshold) : hotnessThreshold_(hotnessThreshold)
    {
        chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
    };

    ~PGORecordDetailInfos()
    {
        Clear();
    }

    void Clear()
    {
        for (auto iter : recordInfos_) {
            iter.second->Clear();
            nativeAreaAllocator_.Delete(iter.second);
        }
        recordInfos_.clear();
        chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
    }

    // If it is a new method, return true.
    bool AddMethod(const CString &recordName, EntityId methodId, const CString &methodName, SampleMode mode);
    void Merge(const PGORecordDetailInfos &recordInfos);

    void ParseFromBinary(void *buffer, SectionInfo *const info);
    void ProcessToBinary(const SaveTask *task, std::ofstream &fileStream, SectionInfo *info) const;

    bool ParseFromText(std::ifstream &stream);
    void ProcessToText(std::ofstream &stream) const;

    NO_COPY_SEMANTIC(PGORecordDetailInfos);
    NO_MOVE_SEMANTIC(PGORecordDetailInfos);
private:
    uint32_t hotnessThreshold_ {2};
    NativeAreaAllocator nativeAreaAllocator_;
    std::unique_ptr<Chunk> chunk_;
    CMap<CString, PGOMethodInfoMap *> recordInfos_;
};

class PGORecordSimpleInfos {
public:
    explicit PGORecordSimpleInfos(uint32_t threshold) : hotnessThreshold_(threshold) {}
    ~PGORecordSimpleInfos()
    {
        Clear();
    }

    void Clear()
    {
        for (auto iter : methodIds_) {
            nativeAreaAllocator_.Delete(iter.second);
        }
        methodIds_.clear();
    }

    bool Match(const CString &recordName, EntityId methodId);
    template <typename Callback>
    void Update(Callback callback)
    {
        for (auto iter = methodIds_.begin(); iter != methodIds_.end(); iter++) {
            auto recordName = iter->first;
            auto methodIds = iter->second;
            methodIds->Update(recordName, callback);
        }
    }

    template <typename Callback>
    void Update(const CString &recordName, Callback callback)
    {
        auto iter = methodIds_.find(recordName);
        if (iter != methodIds_.end()) {
            iter->second->Update(recordName, callback);
        } else {
            PGOMethodIdSet *methodIds = nativeAreaAllocator_.New<PGOMethodIdSet>();
            if (methodIds->Update(recordName, callback)) {
                methodIds_.emplace(recordName, methodIds);
            } else {
                nativeAreaAllocator_.Delete(methodIds);
            }
        }
    }

    void ParseFromBinary(void *buffer, SectionInfo *const info);

    NO_COPY_SEMANTIC(PGORecordSimpleInfos);
    NO_MOVE_SEMANTIC(PGORecordSimpleInfos);
private:
    uint32_t hotnessThreshold_ {2};
    NativeAreaAllocator nativeAreaAllocator_;
    CUnorderedMap<CString, PGOMethodIdSet *> methodIds_;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_PGO_PROFILER_INFO_H
