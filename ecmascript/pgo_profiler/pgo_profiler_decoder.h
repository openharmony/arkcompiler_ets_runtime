/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PGO_PROFILE_DECODER_H
#define ECMASCRIPT_PGO_PROFILE_DECODER_H

#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/platform/map.h"

namespace panda::ecmascript::pgo {
class PGOProfilerDecoder {
public:
    PGOProfilerDecoder() = default;
    PGOProfilerDecoder(const std::string &inPath, uint32_t hotnessThreshold)
        : inPath_(inPath), hotnessThreshold_(hotnessThreshold) {}

    virtual ~PGOProfilerDecoder()
    {
        Clear();
    };

    NO_COPY_SEMANTIC(PGOProfilerDecoder);
    NO_MOVE_SEMANTIC(PGOProfilerDecoder);

    bool PUBLIC_API Match(const JSPandaFile *jsPandaFile, const CString &recordName, PGOMethodId methodId);

    bool PUBLIC_API LoadAndVerify(const std::unordered_map<CString, uint32_t>& fileNameToChecksumMap,
                                  const std::shared_ptr<PGOAbcFilePool>& externalAbcFilePool = nullptr);
    bool PUBLIC_API LoadFull(const std::shared_ptr<PGOAbcFilePool> &externalAbcFilePool = nullptr);
    void PUBLIC_API Clear();

    bool PUBLIC_API SaveAPTextFile(const std::string &outPath);

    void Merge(const PGOProfilerDecoder &decoder);

    std::shared_ptr<PGOAbcFilePool> GetAbcFilePool() const
    {
        return abcFilePool_;
    }

    bool InitMergeData();

    void SetInPath(const std::string &inPath)
    {
        LOG_COMPILER(INFO) << "Set Profiler paths: " << inPath;
        inPath_ = inPath;
    }

    const std::string& GetInPath() const
    {
        return inPath_;
    }

    void SetHotnessThreshold(uint32_t hotnessThreshold)
    {
        hotnessThreshold_ = hotnessThreshold;
    }

    uint32_t GetHotnessThreshold() const
    {
        return hotnessThreshold_;
    }

    template <typename Callback>
    void Update(const JSPandaFile *jsPandaFile, Callback callback)
    {
        if (!isLoaded_ || !isVerifySuccess_) {
            return;
        }
        recordSimpleInfos_->Update(GetNormalizedFileDesc(jsPandaFile), callback);
    }

    template <typename Callback>
    void Update(const JSPandaFile *jsPandaFile, const CString &recordName, Callback callback)
    {
        if (!isLoaded_ || !isVerifySuccess_) {
            return;
        }
        recordSimpleInfos_->Update(GetNormalizedFileDesc(jsPandaFile), recordName, callback);
    }

    template <typename Callback>
    void GetTypeInfo(const JSPandaFile *jsPandaFile, const CString &recordName, const MethodLiteral *methodLiteral,
                     Callback callback) const
    {
        if (!isLoaded_ || !isVerifySuccess_ || methodLiteral == nullptr) {
            return;
        }
        const auto *methodName = MethodLiteral::GetMethodName(jsPandaFile, methodLiteral->GetMethodId());
        if (IsMethodMatchEnabled()) {
            auto checksum =
                PGOMethodInfo::CalcChecksum(methodName, methodLiteral->GetBytecodeArray(),
                                            MethodLiteral::GetCodeSize(jsPandaFile, methodLiteral->GetMethodId()));

            return recordSimpleInfos_->GetTypeInfo(GetNormalizedFileDesc(jsPandaFile), recordName, methodName, checksum,
                                                   callback);
        }
        recordSimpleInfos_->GetTypeInfo(GetNormalizedFileDesc(jsPandaFile), recordName, methodName, callback);
    }

    void MatchAndMarkMethod(const JSPandaFile *jsPandaFile, const CString &recordName, const char *methodName,
                            EntityId methodId)
    {
        if (!isLoaded_ || !isVerifySuccess_) {
            return;
        }
        recordSimpleInfos_->MatchAndMarkMethod(GetNormalizedFileDesc(jsPandaFile), recordName, methodName, methodId);
    }

    void PUBLIC_API GetMismatchResult(const JSPandaFile *jsPandaFile, uint32_t &totalMethodCount,
        uint32_t &mismatchMethodCount, std::set<std::pair<std::string, CString>> &mismatchMethodSet) const;

    bool IsMethodMatchEnabled() const
    {
        return header_->SupportMethodChecksum();
    }

    bool PUBLIC_API GetHClassTreeDesc(PGOSampleType profileType, PGOHClassTreeDesc **desc) const;

    template <typename Callback>
    bool IterateHClassTreeDesc(Callback callback) const
    {
        if (!isLoaded_ || !isVerifySuccess_) {
            return false;
        }
        return recordSimpleInfos_->IterateHClassTreeDesc(callback);
    }

    template <typename Callback>
    bool IterateProtoTransitionPool(Callback callback) const
    {
        if (!isLoaded_ || !isVerifySuccess_) {
            return false;
        }
        return recordSimpleInfos_->IterateProtoTransitionPool(callback);
    }

    bool IsLoaded() const
    {
        return isLoaded_;
    }

    PGORecordDetailInfos &GetRecordDetailInfos() const
    {
        return *recordDetailInfos_;
    }

    std::shared_ptr<PGORecordDetailInfos> GetRecordDetailInfosPtr() const
    {
        return recordDetailInfos_;
    }

    PGORecordSimpleInfos &GetRecordSimpleInfos() const
    {
        return *recordSimpleInfos_;
    }

    const PGOPandaFileInfos &GetPandaFileInfos() const
    {
        return pandaFileInfos_;
    }

    bool GetAbcNameById(ApEntityId abcId, CString &abcName) const
    {
        ASSERT(header_ != nullptr);
        if (!header_->SupportProfileTypeWithAbcId()) {
            return false;
        }
        ASSERT(abcFilePool_ != nullptr);
        ASSERT(abcFilePool_->GetPool() != nullptr);
        const auto *entry = abcFilePool_->GetPool()->GetEntry(abcId);
        if (entry == nullptr) {
            LOG_COMPILER(ERROR) << "Can not find abcId in pgo file. abcId: " << abcId;
            return false;
        }
        abcName = entry->GetData();
        return true;
    }

    bool GetABCIdByJSPandaFile(const JSPandaFile *jsPandaFile, ApEntityId &entryId) const
    {
        if (abcFilePool_ == nullptr) {
            return false;
        }
        CString name = jsPandaFile->GetNormalizedFileDesc();
        return abcFilePool_->GetEntryIdByNormalizedName(name, entryId);
    }

    bool IsCompatibleWithAOTFile()
    {
        // Can be null when AP file path is empty
        if (header_ == nullptr) {
            return false;
        }
        return header_->IsCompatibleWithAOTFile();
    }

    void MergeFileNameToChecksumMap(std::unordered_map<CString, uint32_t> &fileNameToChecksumMap) const;

private:
    bool Load(const std::shared_ptr<PGOAbcFilePool> &externalAbcFilePool);
    bool Verify(const std::unordered_map<CString, uint32_t>& fileNameToChecksumMap);

    bool LoadAPBinaryFile(int prot = PAGE_PROT_READ);
    void UnLoadAPBinaryFile();
    CString PUBLIC_API GetNormalizedFileDesc(const JSPandaFile *jsPandaFile) const;
    void LoadAbcIdPool(const std::shared_ptr<PGOAbcFilePool> &externalAbcFilePool, PGOContext &context,
                       void *addr);

    bool isLoaded_ {false};
    bool isVerifySuccess_ {false};
    std::string inPath_;
    uint32_t hotnessThreshold_ {0};
    PGOProfilerHeader *header_ {nullptr};
    PGOPandaFileInfos pandaFileInfos_;
    std::shared_ptr<PGOAbcFilePool> abcFilePool_;
    bool externalAbcFilePool_ {false};
    std::shared_ptr<PGORecordDetailInfos> recordDetailInfos_;
    std::unique_ptr<PGORecordSimpleInfos> recordSimpleInfos_;
    MemMap fileMapAddr_;
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILE_DECODER_H
