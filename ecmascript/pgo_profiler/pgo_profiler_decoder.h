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
#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_type.h"
#include "ecmascript/platform/map.h"

namespace panda::ecmascript {
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

    bool PUBLIC_API Match(const CString &recordName, PGOMethodId methodId);

    bool PUBLIC_API LoadAndVerify(uint32_t checksum);
    bool PUBLIC_API LoadFull();
    void PUBLIC_API Clear();

    bool PUBLIC_API SaveAPTextFile(const std::string &outPath);

    void Merge(const PGOProfilerDecoder &decoder);

    bool InitMergeData();

    const std::string& GetInPath() const
    {
        return inPath_;
    }

    uint32_t GetHotnessThreshold() const
    {
        return hotnessThreshold_;
    }

    template <typename Callback>
    void Update(Callback callback)
    {
        if (!isLoaded_ || !isVerifySuccess_) {
            return;
        }
        recordSimpleInfos_->Update(callback);
    }

    template <typename Callback>
    void Update(const CString &recordName, Callback callback)
    {
        if (!isLoaded_ || !isVerifySuccess_) {
            return;
        }
        recordSimpleInfos_->Update(recordName, callback);
    }

    template <typename Callback>
    void GetTypeInfo(const JSPandaFile *jsPandaFile, const CString &recordName, const MethodLiteral *methodLiteral,
                     Callback callback)
    {
        if (!isLoaded_ || !isVerifySuccess_) {
            return;
        }
        const auto *methodName = MethodLiteral::GetMethodName(jsPandaFile, methodLiteral->GetMethodId());
        if (IsMethodMatchEnabled()) {
            auto checksum =
                PGOMethodInfo::CalcChecksum(methodName, methodLiteral->GetBytecodeArray(),
                                            MethodLiteral::GetCodeSize(jsPandaFile, methodLiteral->GetMethodId()));

            return recordSimpleInfos_->GetTypeInfo(recordName, methodName, checksum, callback);
        }
        recordSimpleInfos_->GetTypeInfo(recordName, methodName, callback);
    }

    void MatchAndMarkMethod(const CString &recordName, const char *methodName, EntityId methodId)
    {
        if (!isLoaded_ || !isVerifySuccess_) {
            return;
        }
        recordSimpleInfos_->MatchAndMarkMethod(recordName, methodName, methodId);
    }

    void GetMismatchResult(uint32_t &totalMethodCount, uint32_t &mismatchMethodCount,
                           std::set<std::pair<std::string, CString>> &mismatchMethodSet) const;

    bool IsMethodMatchEnabled() const
    {
        return header_->SupportMethodChecksum();
    }

    bool GetHClassLayoutDesc(PGOSampleType classType, PGOHClassLayoutDesc **desc) const;

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

private:
    bool Load();
    bool Verify(uint32_t checksum);

    bool LoadAPBinaryFile(int prot = PAGE_PROT_READ);
    void UnLoadAPBinaryFile();

    bool isLoaded_ {false};
    bool isVerifySuccess_ {false};
    std::string inPath_;
    uint32_t hotnessThreshold_ {0};
    PGOProfilerHeader *header_ {nullptr};
    PGOPandaFileInfos pandaFileInfos_;
    std::shared_ptr<PGORecordDetailInfos> recordDetailInfos_;
    std::unique_ptr<PGORecordSimpleInfos> recordSimpleInfos_;
    MemMap fileMapAddr_;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_PGO_PROFILE_DECODER_H
