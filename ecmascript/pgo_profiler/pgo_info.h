/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PGO_INFO_H
#define ECMASCRIPT_PGO_INFO_H

#include <cstdint>
#include <memory>

#include "libpandabase/macros.h"

#include "ecmascript/mem/c_string.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript::pgo {
class PGOProfilerHeader;
class PGOPandaFileInfos;
class PGOAbcFilePool;
class PGORecordDetailInfos;
class SaveTask;
class PGOProfilerDecoder;

class PGOInfo {
public:
    explicit PGOInfo(uint32_t hotnessThreshold = 0);
    ~PGOInfo();

    NO_COPY_SEMANTIC(PGOInfo);
    NO_MOVE_SEMANTIC(PGOInfo);

    PGOProfilerHeader& GetHeader() const;
    PGOProfilerHeader* GetHeaderPtr() const;
    void SetHeader(PGOProfilerHeader* header);
    PGOPandaFileInfos& GetPandaFileInfos() const;
    void SetPandaFileInfos(std::unique_ptr<PGOPandaFileInfos> pandaFileInfos);
    PGOAbcFilePool& GetAbcFilePool() const;
    std::shared_ptr<PGOAbcFilePool> GetAbcFilePoolPtr() const;
    void SetAbcFilePool(std::shared_ptr<PGOAbcFilePool> abcFilePool);
    PGORecordDetailInfos& GetRecordDetailInfos() const;
    std::shared_ptr<PGORecordDetailInfos> GetRecordDetailInfosPtr() const;
    void SetRecordDetailInfos(std::shared_ptr<PGORecordDetailInfos> recordDetailInfos);
    void Clear();
    void SetHotnessThreshold(uint32_t threshold);
    uint32_t GetHotnessThreshold() const;
    void SamplePandaFileInfoSafe(uint32_t checksum, const CString& abcName);
    void MergeWithExistProfile(PGOInfo& other, PGOProfilerDecoder& decoder, const SaveTask* task);
    bool GetPandaFileIdSafe(const CString& abcName, ApEntityId& entryId);
    bool GetPandaFileDescSafe(ApEntityId abcId, CString& desc);
    void MergeSafe(const PGORecordDetailInfos& recordInfos);
    void MergeSafe(const PGOPandaFileInfos& pandaFileInfos);
    void MergeSafe(const PGOInfo& other);
    bool VerifyPandaFileMatched(const PGOPandaFileInfos& pandaFileInfos,
                                const std::string& base,
                                const std::string& incoming) const;
    void ProcessToBinary(std::fstream& fileStream, PGOProfilerHeader* header);
    Mutex& GetSampleMutexLock();
    ConcurrentGuardValue& GetConcurrentGuardValue();

private:
    uint32_t hotnessThreshold_ {0};
    PGOProfilerHeader* header_;
    std::unique_ptr<PGOPandaFileInfos> pandaFileInfos_;
    std::shared_ptr<PGOAbcFilePool> abcFilePool_;
    std::shared_ptr<PGORecordDetailInfos> recordDetailInfos_;
    Mutex sampleMutexLock_;
    ConcurrentGuardValue v_;
};
} // namespace panda::ecmascript::pgo
#endif // ECMASCRIPT_PGO_INFO_H