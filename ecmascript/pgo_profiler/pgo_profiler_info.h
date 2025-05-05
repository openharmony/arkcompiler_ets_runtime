/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include <cstdint>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <string.h>

#include "ecmascript/common.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_method_type_set.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_profile_type_pool.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_proto_transition_type_pool.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_record_pool.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/pgo_profiler/pgo_profiler_layout.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/ts_types/global_type_info.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::pgo {
class SaveTask;
class PGOContext;

class PGOPandaFileInfos {
public:
    void Sample(uint32_t checksum, uint32_t abcId)
    {
        fileInfos_.emplace(checksum, abcId);
    }

    void Clear()
    {
        fileInfos_.clear();
    }

    uint32_t GetFileInfosSzie() const
    {
        return fileInfos_.size();
    }

    void SampleSafe(uint32_t checksum, uint32_t abcId)
    {
        WriteLockHolder lock(fileInfosLock_);
        Sample(checksum, abcId);
    }

    void ClearSafe()
    {
        WriteLockHolder lock(fileInfosLock_);
        Clear();
    }

    void ParseFromBinary(void *buffer, SectionInfo *const info);
    void ProcessToBinary(std::fstream &fileStream, SectionInfo *info) const;
    void Merge(const PGOPandaFileInfos &pandaFileInfos);
    void MergeSafe(const PGOPandaFileInfos& pandaFileInfos);
    bool VerifyChecksum(const PGOPandaFileInfos &pandaFileInfos, const std::string &base,
                        const std::string &incoming) const;

    void ProcessToText(std::ofstream &stream) const;

    bool Checksum(const std::unordered_map<CString, uint32_t>& fileNameToChecksumMap,
                  const std::shared_ptr<PGOAbcFilePool>& abcFilePool_) const;
    bool Checksum(const std::unordered_map<CString, uint32_t>& fileNameToChecksumMap) const;
    void UpdateFileInfosAbcID(const PGOContext &context);

    template <typename Callback>
    void ForEachFileInfo(Callback callback) const
    {
        for (const auto &fileInfo : fileInfos_) {
            callback(fileInfo.GetChecksum(), fileInfo.GetAbcId());
        }
    }

private:
    class FileInfo {
    public:
        FileInfo() = default;
        FileInfo(uint32_t checksum, uint32_t abcId) : abcId_(abcId), checksum_(checksum) {}
        static size_t Size()
        {
            return sizeof(FileInfo);
        }

        bool operator<(const FileInfo &right) const
        {
            return abcId_ < right.abcId_;
        }

        uint32_t GetChecksum() const
        {
            return checksum_;
        }

        uint32_t GetAbcId() const
        {
            return abcId_;
        }

    private:
        // Support extended fields
        uint32_t abcId_;
        uint32_t checksum_;
    };

    std::set<FileInfo> fileInfos_;
    RWLock fileInfosLock_;
};

class PGOMethodInfo {
public:
    static constexpr int METHOD_INFO_COUNT = 4;
    static constexpr int METHOD_ID_INDEX = 0;
    static constexpr int METHOD_COUNT_INDEX = 1;
    static constexpr int METHOD_MODE_INDEX = 2;
    static constexpr int METHOD_NAME_INDEX = 3;
    static constexpr uint32_t METHOD_MAX_HIT_COUNT = 10000U;

    explicit PGOMethodInfo(PGOMethodId id) : id_(id) {}

    PGOMethodInfo(PGOMethodId id, uint32_t count, SampleMode mode, const char *methodName)
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

    static uint32_t PUBLIC_API CalcChecksum(const char *name, const uint8_t *byteCodeArray, uint32_t byteCodeLength);

    static uint32_t CalcOpCodeChecksum(const uint8_t *byteCodeArray, uint32_t byteCodeLength);

    static int32_t Size(uint32_t length)
    {
        return sizeof(PGOMethodInfo) + AlignUp(length, GetAlignmentInBytes(ALIGN_SIZE));
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
        count_ = std::min(count_ + info->GetCount(), METHOD_MAX_HIT_COUNT);
        SetSampleMode(info->GetSampleMode());
    }

    PGOMethodId GetMethodId() const
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

    void ProcessToJson(ProfileType::VariantMap &function) const;

    NO_COPY_SEMANTIC(PGOMethodInfo);
    NO_MOVE_SEMANTIC(PGOMethodInfo);

private:
    uint32_t size_ {0};
    PGOMethodId id_;
    uint32_t count_ {0};
    SampleMode mode_ {SampleMode::CALL_MODE};
    char methodName_ {0};
};

class PGODecodeMethodInfo {
public:
    explicit PGODecodeMethodInfo(PGOMethodId id) : methodId_(id) {}

    PGOMethodId GetMethodId() const
    {
        return methodId_;
    }

    PGOMethodTypeSet &GetPGOMethodTypeSet()
    {
        return pgoMethodTypeSet_;
    }

    void Merge(const PGODecodeMethodInfo &from);

private:
    PGOMethodId methodId_ {0};
    PGOMethodTypeSet pgoMethodTypeSet_ {};
};

class PGOMethodInfoMap {
public:
    PGOMethodInfoMap() = default;

    void Clear()
    {
        // PGOMethodInfo release by chunk
        for (auto &entry : methodTypeInfos_) {
            entry.second->Clear();
        }
        methodInfos_.clear();
        methodTypeInfos_.clear();
    }

    bool AddMethod(Chunk *chunk, Method *jsMethod, SampleMode mode);
    bool AddType(Chunk *chunk, PGOMethodId methodId, int32_t offset, PGOSampleType type);
    bool AddCallTargetType(Chunk *chunk, PGOMethodId methodId, int32_t offset, PGOSampleType type);
    bool AddObjectInfo(Chunk *chunk, PGOMethodId methodId, int32_t offset, const PGOObjectInfo &info);
    bool AddDefine(Chunk *chunk, PGOMethodId methodId, int32_t offset, PGODefineOpType type);
    void Merge(Chunk *chunk, PGOMethodInfoMap *methodInfos);

    bool ParseFromBinary(Chunk *chunk, PGOContext &context, void **buffer);
    bool ProcessToBinary(PGOContext &context, ProfileTypeRef recordProfileRef, const SaveTask *task,
                         std::fstream &fileStream, PGOProfilerHeader *const header) const;

    bool ParseFromText(Chunk *chunk, uint32_t threshold, const std::vector<std::string> &content);
    void ProcessToText(uint32_t threshold, const CString &recordName, std::ofstream &stream) const;

    void ProcessToJson(uint32_t threshold, ProfileType::jModuleType &jModule) const;

    const CMap<PGOMethodId, PGOMethodInfo *> &GetMethodInfos() const
    {
        return methodInfos_;
    }

    NO_COPY_SEMANTIC(PGOMethodInfoMap);
    NO_MOVE_SEMANTIC(PGOMethodInfoMap);

private:
    PGOMethodTypeSet *GetOrInsertMethodTypeSet(Chunk *chunk, PGOMethodId methodId);

    CMap<PGOMethodId, PGOMethodInfo *> methodInfos_;
    CMap<PGOMethodId, PGOMethodTypeSet *> methodTypeInfos_;
    CMap<PGOMethodId, uint32_t> methodsChecksum_;
    CMap<PGOSampleType, CMap<CString, TrackType>> globalLayoutDescInfos_;
};

class PGOMethodIdSet {
public:
    explicit PGOMethodIdSet(Chunk* chunk): chunk_(chunk), methodInfoMap_(chunk) {};
    ~PGOMethodIdSet() = default;

    void Clear()
    {
        candidateSet_.clear();
        for (auto &methodNameSet : methodInfoMap_) {
            methodNameSet.second.Clear();
        }
        methodInfoMap_.clear();
    }

    bool Match(EntityId methodId)
    {
        return candidateSet_.find(methodId) != candidateSet_.end();
    }

    template <typename Callback>
    bool Update(const CString &recordName, Callback callback)
    {
        std::unordered_set<EntityId> newIds = callback(recordName, candidateSet_);
        if (!newIds.empty()) {
            candidateSet_.insert(newIds.begin(), newIds.end());
            return true;
        }
        return false;
    }

    template <typename Callback>
    void GetTypeInfo(const char *methodName, Callback callback)
    {
        // for no function checksum in ap file
        auto iter = methodInfoMap_.find(methodName);
        if ((iter != methodInfoMap_.end()) && (iter->second.GetFirstMethodInfo() != nullptr)) {
            iter->second.GetFirstMethodInfo()->GetPGOMethodTypeSet().GetTypeInfo(callback);
        }
    }

    template <typename Callback>
    void GetTypeInfo(const char *methodName, uint32_t checksum, Callback callback)
    {
        auto iter = methodInfoMap_.find(methodName);
        if ((iter != methodInfoMap_.end()) && (iter->second.GetMethodInfo(checksum) != nullptr)) {
            return iter->second.GetMethodInfo(checksum)->GetPGOMethodTypeSet().GetTypeInfo(callback);
        }
        LOG_ECMA(DEBUG) << "Method checksum mismatched, name: " << methodName;
    }

    void MatchAndMarkMethod(const char *methodName, EntityId methodId)
    {
        const auto &iter = methodInfoMap_.find(methodName);
        if (iter == methodInfoMap_.end()) {
            // no matching method in PGO file.
            return;
        }
        candidateSet_.emplace(methodId);
        iter->second.SetMatch();
    }

    bool ParseFromBinary(PGOContext &context, void **buffer);

    void GetMismatchResult(const CString &recordName, uint32_t &totalMethodCount, uint32_t &mismatchMethodCount,
                           std::set<std::pair<std::string, CString>> &mismatchMethodSet) const;

    void Merge(const PGOMethodIdSet &from);

    class PGOMethodNameSet {
    public:
        explicit PGOMethodNameSet(Chunk* chunk): methodMap_(chunk) {};
        void SetMatch()
        {
            methodNameMatch_ = true;
        }

        bool IsMatch() const
        {
            return methodNameMatch_;
        }

        PGODecodeMethodInfo& GetOrCreateMethodInfo(uint32_t checksum, PGOMethodId methodId)
        {
            auto methodIter = methodMap_.find(checksum);
            if (methodIter == methodMap_.end()) {
                auto ret = methodMap_.emplace(checksum, methodId);
                ASSERT(ret.second);
                methodIter = ret.first;
            }
            return methodIter->second;
        }

        void Merge(const PGOMethodNameSet &from)
        {
            for (const auto &method : from.methodMap_) {
                uint32_t checksum = method.first;
                auto methodInfo = methodMap_.find(checksum);
                if (methodInfo == methodMap_.end()) {
                    auto ret = methodMap_.emplace(checksum, method.second.GetMethodId());
                    ASSERT(ret.second);
                    methodInfo = ret.first;
                }
                methodInfo->second.Merge(method.second);
            }
        }

        PGODecodeMethodInfo *GetFirstMethodInfo()
        {
            if (methodMap_.empty()) {
                return nullptr;
            }
            return &(methodMap_.begin()->second);
        }

        PGODecodeMethodInfo *GetMethodInfo(uint32_t checksum)
        {
            auto methodInfo = methodMap_.find(checksum);
            if (methodInfo == methodMap_.end()) {
                return nullptr;
            }
            return &(methodInfo->second);
        }

        void Clear()
        {
            methodMap_.clear();
        }

    private:
        bool methodNameMatch_ {false};
        ChunkUnorderedMap<uint32_t, PGODecodeMethodInfo> methodMap_;
    };

    NO_COPY_SEMANTIC(PGOMethodIdSet);
    NO_MOVE_SEMANTIC(PGOMethodIdSet);

private:
    Chunk* chunk_;
    std::unordered_set<EntityId> candidateSet_; // methodId in abc file, DO NOT for pgo internal use
    ChunkUnorderedMap<CString, PGOMethodNameSet> methodInfoMap_;
};

class PGORecordDetailInfos : public PGOContext {
public:
    explicit PGORecordDetailInfos(uint32_t hotnessThreshold);

    ~PGORecordDetailInfos() override;

    void Clear();
    void ClearSafe();
    void InitSections();

    // If it is a new method, return true.
    bool AddMethod(ProfileType recordProfileType, Method *jsMethod, SampleMode mode);
    bool AddType(ProfileType recordProfileType, PGOMethodId methodId, int32_t offset, PGOSampleType type);
    bool AddCallTargetType(ProfileType recordProfileType, PGOMethodId methodId, int32_t offset, PGOSampleType type);
    bool AddObjectInfo(ProfileType recordProfileType, PGOMethodId methodId, int32_t offset, const PGOObjectInfo &info);
    bool AddDefine(ProfileType recordProfileType, PGOMethodId methodId, int32_t offset, PGODefineOpType type);

    bool AddRwUseInfo(ProfileType rootType);
    bool AddRootLayout(JSTaggedType hclass, ProfileType rootType);
    bool UpdateTransitionLayout(
        ProfileType rootType, JSTaggedType parent, ProfileType parentType, JSTaggedType child, ProfileType childType);
    bool UpdateLayout(ProfileType rootType, JSTaggedType hclass, ProfileType curType);
    void AddRootPtType(ProfileType rootType, ProfileType ptType);
    bool IsDumped(ProfileType rootType, ProfileType curType) const;

    void Merge(const PGORecordDetailInfos &recordInfos);
    void MergeSafe(const PGORecordDetailInfos& recordInfos);

    void UpdateLayout();

    bool ParseFromBinary(void *buffer, PGOProfilerHeader *const header);
    void ProcessToBinary(const SaveTask *task, std::fstream &fileStream, PGOProfilerHeader *const header);

    void ProcessToText(std::ofstream &stream) const;

    const CMap<ProfileType, PGOMethodInfoMap *> &GetRecordInfos() const
    {
        return recordInfos_;
    }

    std::shared_ptr<PGORecordPool> GetRecordPool() const
    {
        return recordPool_;
    }

    std::shared_ptr<PGOProfileTypePool> GetProfileTypePool() const override
    {
        return profileTypePool_;
    }

    std::shared_ptr<PGOProtoTransitionPool> GetProtoTransitionPool() const
    {
        return protoTransitionPool_;
    }

    uint32_t GetHotnessThreshold() const override
    {
        return hotnessThreshold_;
    }

    PGOProfilerHeader *GetHeader() const override
    {
        return header_;
    }

    bool SupportElementsKind() const override
    {
        ASSERT(header_ != nullptr);
        return header_->SupportElementsKind();
    }

    bool SupportElementsTrackInfo() const override
    {
        ASSERT(header_ != nullptr);
        return header_->SupportElementsTrackInfo();
    }

    void ResetAbcIdRemap() const override
    {
        abcIdRemap_.clear();
    }

    void AddAbcIdRemap(ApEntityId oldId, ApEntityId newId) const override
    {
        abcIdRemap_[oldId] = newId;
    }

    const std::map<ApEntityId, ApEntityId> &GetAbcIdRemap() const override
    {
        return abcIdRemap_;
    }

    NO_COPY_SEMANTIC(PGORecordDetailInfos);
    NO_MOVE_SEMANTIC(PGORecordDetailInfos);

private:
    PGOMethodInfoMap *GetMethodInfoMap(ProfileType recordProfileType);
    bool ParseFromBinaryForLayout(void **buffer);
    bool ProcessToBinaryForLayout(NativeAreaAllocator *allocator, const SaveTask *task, std::fstream &stream);

    uint32_t hotnessThreshold_ {2};
    NativeAreaAllocator nativeAreaAllocator_;
    std::unique_ptr<Chunk> chunk_;
    CMap<ProfileType, PGOMethodInfoMap *> recordInfos_;
    std::set<PGOHClassTreeDesc> hclassTreeDescInfos_;
    PGOProfilerHeader *header_ {nullptr};
    std::shared_ptr<PGORecordPool> recordPool_;
    std::shared_ptr<PGOProtoTransitionPool> protoTransitionPool_;
    std::shared_ptr<PGOProfileTypePool> profileTypePool_;
    mutable std::map<ApEntityId, ApEntityId> abcIdRemap_;
    Mutex mutex_;
};

class PGORecordSimpleInfos : public PGOContext {
public:
    explicit PGORecordSimpleInfos(uint32_t threshold);

    ~PGORecordSimpleInfos() override;

    void Clear();

    void InitSections();

    bool Match(const CString &abcNormalizedDesc, const CString &recordName, EntityId methodId);

    template <typename Callback>
    void Update(const CString &abcNormalizedDesc, Callback callback)
    {
        auto abcMethodIds = methodIds_.find(abcNormalizedDesc);
        if (abcMethodIds == methodIds_.end()) {
            return;
        }
        for (auto iter = abcMethodIds->second.begin(); iter != abcMethodIds->second.end(); iter++) {
            auto recordName = iter->first;
            auto methodIds = iter->second;
            methodIds->Update(recordName, callback);
        }
    }

    template <typename Callback>
    void Update(const CString &abcNormalizedDesc, const CString &recordName, Callback callback)
    {
        auto abcMethodIds = methodIds_.find(abcNormalizedDesc);
        if (abcMethodIds == methodIds_.end()) {
            return;
        }
        auto iter = abcMethodIds->second.find(recordName);
        if (iter != abcMethodIds->second.end()) {
            iter->second->Update(recordName, callback);
        } else {
            PGOMethodIdSet *methodIdSet = nativeAreaAllocator_.New<PGOMethodIdSet>(chunk_.get());
            if (methodIdSet->Update(recordName, callback)) {
                abcMethodIds->second.emplace(recordName, methodIdSet);
            } else {
                nativeAreaAllocator_.Delete(methodIdSet);
            }
        }
    }

    template <typename Callback>
    void GetTypeInfo(const CString &abcNormalizedDesc, const CString &recordName, const char *methodName,
                     Callback callback)
    {
        auto abcMethodIds = methodIds_.find(abcNormalizedDesc);
        if (abcMethodIds == methodIds_.end()) {
            return;
        }
        auto iter = abcMethodIds->second.find(recordName);
        if (iter != abcMethodIds->second.end()) {
            iter->second->GetTypeInfo(methodName, callback);
        }
    }

    template <typename Callback>
    void GetTypeInfo(const CString &abcNormalizedDesc, const CString &recordName, const char *methodName,
                     uint32_t checksum, Callback callback)
    {
        auto abcMethodIds = methodIds_.find(abcNormalizedDesc);
        if (abcMethodIds == methodIds_.end()) {
            return;
        }
        auto iter = abcMethodIds->second.find(recordName);
        if (iter != abcMethodIds->second.end()) {
            iter->second->GetTypeInfo(methodName, checksum, callback);
        }
    }

    std::shared_ptr<PGOProtoTransitionPool> GetProtoTransitionPool() const
    {
        return protoTransitionPool_;
    }

    bool GetHClassTreeDesc(PGOSampleType profileType, PGOHClassTreeDesc **desc) const
    {
        auto iter = hclassTreeDescInfos_.find(PGOHClassTreeDesc(profileType.GetProfileType()));
        if (iter != hclassTreeDescInfos_.end()) {
            *desc = &(const_cast<PGOHClassTreeDesc &>(*iter));
            return true;
        }
        return false;
    }

    template <typename Callback>
    bool IterateHClassTreeDesc(Callback callback) const
    {
        for (auto treeDescInfo : hclassTreeDescInfos_) {
            callback(&treeDescInfo);
        }
        return true;
    }

    template <typename Callback>
    bool IterateProtoTransitionPool(Callback callback) const
    {
        protoTransitionPool_->Iterate(callback);
        return true;
    }

    void MatchAndMarkMethod(const CString &abcNormalizedDesc, const CString &recordName, const char *methodName,
                            EntityId methodId)
    {
        auto abcMethodIds = methodIds_.find(abcNormalizedDesc);
        if (abcMethodIds == methodIds_.end()) {
            return;
        }
        auto iter = abcMethodIds->second.find(recordName);
        if (iter != abcMethodIds->second.end()) {
            return iter->second->MatchAndMarkMethod(methodName, methodId);
        }
    }

    void GetMismatchResult(const CString &abcNormalizedDesc, uint32_t &totalMethodCount, uint32_t &mismatchMethodCount,
                           std::set<std::pair<std::string, CString>> &mismatchMethodSet) const
    {
        auto abcMethodIds = methodIds_.find(abcNormalizedDesc);
        if (abcMethodIds == methodIds_.end()) {
            return;
        }
        for (const auto &methodId : abcMethodIds->second) {
            methodId.second->GetMismatchResult(methodId.first, totalMethodCount, mismatchMethodCount,
                                               mismatchMethodSet);
        }
    }

    void ParseFromBinary(void *buffer, PGOProfilerHeader *const header, std::shared_ptr<PGOAbcFilePool> &abcFilePool);

    void Merge(const PGORecordSimpleInfos &simpleInfos);

    std::shared_ptr<PGOProfileTypePool> GetProfileTypePool() const override
    {
        return profileTypePool_;
    }

    uint32_t GetHotnessThreshold() const override
    {
        return hotnessThreshold_;
    }

    PGOProfilerHeader *GetHeader() const override
    {
        return header_;
    }

    bool SupportElementsKind() const override
    {
        ASSERT(header_ != nullptr);
        return header_->SupportElementsKind();
    }

    bool SupportElementsTrackInfo() const override
    {
        ASSERT(header_ != nullptr);
        return header_->SupportElementsTrackInfo();
    }

    void ResetAbcIdRemap() const override
    {
        abcIdRemap_.clear();
    }

    const std::map<ApEntityId, ApEntityId> &GetAbcIdRemap() const override
    {
        return abcIdRemap_;
    }

    void AddAbcIdRemap(ApEntityId oldId, ApEntityId newId) const override
    {
        abcIdRemap_[oldId] = newId;
    }

    NO_COPY_SEMANTIC(PGORecordSimpleInfos);
    NO_MOVE_SEMANTIC(PGORecordSimpleInfos);

private:
    bool ParseFromBinaryForLayout(void **buffer);

    uint32_t hotnessThreshold_ {2};
    NativeAreaAllocator nativeAreaAllocator_;
    std::unique_ptr<Chunk> chunk_;
    CUnorderedMap<CString, CUnorderedMap<CString, PGOMethodIdSet *>> methodIds_;
    PGOProfilerHeader *header_ {nullptr};
    // std::list<std::weak_ptr<PGOFileSectionInterface>> apSectionList_;
    std::shared_ptr<PGORecordPool> recordPool_;
    std::shared_ptr<PGOProtoTransitionPool> protoTransitionPool_;
    std::shared_ptr<PGOProfileTypePool> profileTypePool_;
    std::set<PGOHClassTreeDesc> hclassTreeDescInfos_;
    mutable std::map<ApEntityId, ApEntityId> abcIdRemap_;
};
} // namespace panda::ecmascript::pgo
#endif // ECMASCRIPT_PGO_PROFILER_INFO_H
