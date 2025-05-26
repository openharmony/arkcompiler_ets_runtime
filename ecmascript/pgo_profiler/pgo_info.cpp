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

#include "ecmascript/pgo_profiler/pgo_info.h"

#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_trace.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"

namespace panda::ecmascript::pgo {

PGOInfo::PGOInfo(uint32_t hotnessThreshold): hotnessThreshold_(hotnessThreshold)
{
    abcFilePool_ = std::make_shared<PGOAbcFilePool>();
    pandaFileInfos_ = std::make_unique<PGOPandaFileInfos>();
    PGOProfilerHeader::Build(&header_, PGOProfilerHeader::LastSize());
    recordDetailInfos_ = std::make_shared<PGORecordDetailInfos>(hotnessThreshold_);
}

PGOInfo::~PGOInfo()
{
    Clear();
}

PGOProfilerHeader& PGOInfo::GetHeader() const
{
    return *header_;
}

PGOProfilerHeader* PGOInfo::GetHeaderPtr() const
{
    return header_;
}

void PGOInfo::SetHeader(PGOProfilerHeader* header)
{
    header_ = header;
}

PGOPandaFileInfos& PGOInfo::GetPandaFileInfos() const
{
    return *pandaFileInfos_;
}

void PGOInfo::SetPandaFileInfos(std::unique_ptr<PGOPandaFileInfos> pandaFileInfos)
{
    pandaFileInfos_ = std::move(pandaFileInfos);
}

PGOAbcFilePool& PGOInfo::GetAbcFilePool() const
{
    return *abcFilePool_;
}

std::shared_ptr<PGOAbcFilePool> PGOInfo::GetAbcFilePoolPtr() const
{
    return abcFilePool_;
}

void PGOInfo::SetAbcFilePool(std::shared_ptr<PGOAbcFilePool> abcFilePool)
{
    abcFilePool_ = abcFilePool;
}

PGORecordDetailInfos& PGOInfo::GetRecordDetailInfos() const
{
    return *recordDetailInfos_;
}

void PGOInfo::SetRecordDetailInfos(std::shared_ptr<PGORecordDetailInfos> recordDetailInfos)
{
    recordDetailInfos_ = recordDetailInfos;
}

std::shared_ptr<PGORecordDetailInfos> PGOInfo::GetRecordDetailInfosPtr() const
{
    return recordDetailInfos_;
}

void PGOInfo::SetHotnessThreshold(uint32_t threshold)
{
    hotnessThreshold_ = threshold;
}

uint32_t PGOInfo::GetHotnessThreshold() const
{
    return hotnessThreshold_;
}

void PGOInfo::SamplePandaFileInfoSafe(uint32_t checksum, const CString& abcName)
{
    LockHolder lock(sampleMutexLock_);
    ConcurrentGuard guard(v_, "SafeSamplePandaFileInfo");
    ApEntityId entryId(0);
    abcFilePool_->TryAddSafe(abcName, entryId);
    pandaFileInfos_->SampleSafe(checksum, entryId);
}

void PGOInfo::Clear()
{
    if (pandaFileInfos_) {
        pandaFileInfos_->ClearSafe();
    }
    if (abcFilePool_) {
        abcFilePool_->ClearSafe();
    }
    if (header_) {
        PGOProfilerHeader::Destroy(&header_);
    }
    if (recordDetailInfos_) {
        recordDetailInfos_->ClearSafe();
    }
}

bool PGOInfo::GetPandaFileIdSafe(const CString& abcName, ApEntityId& entryId)
{
    return abcFilePool_->GetEntryIdSafe(abcName, entryId);
}

bool PGOInfo::GetPandaFileDescSafe(ApEntityId abcId, CString& desc)
{
    return abcFilePool_->GetPandaFileDescSafe(abcId, desc);
}

void PGOInfo::MergeSafe(const PGORecordDetailInfos& recordInfos)
{
    recordDetailInfos_->MergeSafe(recordInfos);
}

void PGOInfo::MergeSafe(const PGOPandaFileInfos& pandaFileInfos)
{
    pandaFileInfos_->MergeSafe(pandaFileInfos);
}

void PGOInfo::MergeSafe(const PGOInfo& other)
{
    MergeSafe(other.GetPandaFileInfos());
    MergeSafe(other.GetRecordDetailInfos());
}

bool PGOInfo::VerifyPandaFileMatched(const PGOPandaFileInfos& pandaFileInfos,
                                     const std::string& base,
                                     const std::string& incoming) const
{
    return GetPandaFileInfos().VerifyChecksum(pandaFileInfos, base, incoming);
}

void PGOInfo::MergeWithExistProfile(PGOInfo& rtInfo, PGOProfilerDecoder& decoder, const SaveTask* task)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PGOInfo::MergeWithExistProfile");
    ClockScope start;
    // inherit some info from runtime encoder
    ASSERT(header_ != nullptr);
    ASSERT(rtInfo.header_ != nullptr);
    header_->SetVersion(rtInfo.header_->GetVersion());
    ASSERT(abcFilePool_->GetPool()->Empty());
    {
        // copy abcFilePool from runtime to temp merger.
        // Lock for avoid sampling fileInfos during merge, it will cause pandafile ApEntityId different in abcFilePool
        // and pandaFileInfos, which will cause checksum verification failed or an ApEntityId only in one of pools.
        // when PGOProfilerManager::MergeApFiles, ApEntityId in abcFilePool not exist will cause crash.
        LockHolder lock(rtInfo.GetSampleMutexLock());
        ConcurrentGuard guard(rtInfo.GetConcurrentGuardValue(), "MergeWithExistProfile");
        abcFilePool_->CopySafe(rtInfo.GetAbcFilePoolPtr());
        pandaFileInfos_->MergeSafe(rtInfo.GetPandaFileInfos());
    }
    if (task && task->IsTerminate()) {
        return;
    }
    if (decoder.LoadFull(abcFilePool_)) {
        MergeSafe(decoder.GetPandaFileInfos());
        SetRecordDetailInfos(decoder.GetRecordDetailInfosPtr());
    } else {
        LOG_PGO(ERROR) << "fail to load ap: " << decoder.GetInPath();
    }
    if (task && task->IsTerminate()) {
        return;
    }
    MergeSafe(rtInfo.GetRecordDetailInfos());
    if (PGOTrace::GetInstance()->IsEnable()) {
        PGOTrace::GetInstance()->SetMergeWithExistProfileTime(start.TotalSpentTime());
    }
}

Mutex& PGOInfo::GetSampleMutexLock()
{
    return sampleMutexLock_;
}

ConcurrentGuardValue& PGOInfo::GetConcurrentGuardValue()
{
    return v_;
}
} // namespace panda::ecmascript::pgo
