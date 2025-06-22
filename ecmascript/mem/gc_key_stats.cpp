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

#include "ecmascript/mem/gc_key_stats.h"

#ifdef ENABLE_HISYSEVENT
#include "hisysevent.h"
#endif
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS) && !defined(PANDA_TARGET_IOS)
#include <sys/resource.h>
#endif

#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/platform/dfx_hisys_event.h"
#include "ecmascript/platform/os.h"

namespace panda::ecmascript {
using PGOProfilerManager = pgo::PGOProfilerManager;
using Clock = std::chrono::high_resolution_clock;
const std::string PARTITION_NAME = "/data";
const std::string COMPONENT_NAME = "ets_runtime";

bool GCKeyStats::CheckIfMainThread() const
{
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS) && !defined(PANDA_TARGET_IOS)
    return getpid() == syscall(SYS_gettid);
#else
    return true;
#endif
}

bool GCKeyStats::CheckIfKeyPauseTime() const
{
    return gcStats_->GetScopeDuration(GCStats::Scope::ScopeId::TotalGC) >= KEY_PAUSE_TIME;
}

void GCKeyStats::AddGCStatsToKey()
{
    LOG_GC(DEBUG) << "GCKeyStats AddGCStatsToKey!";

    recordCount_++;

    AddRecordDataStats(RecordKeyData::GC_TOTAL_MEM_USED,
        SizeToIntKB(gcStats_->GetRecordData(RecordData::END_OBJ_SIZE)));
    AddRecordDataStats(RecordKeyData::GC_TOTAL_MEM_COMMITTED,
        SizeToIntKB(gcStats_->GetRecordData(RecordData::END_COMMIT_SIZE)));
    AddRecordDataStats(RecordKeyData::GC_ACTIVE_MEM_USED,
        SizeToIntKB(gcStats_->GetRecordData(RecordData::YOUNG_ALIVE_SIZE)));
    AddRecordDataStats(RecordKeyData::GC_ACTIVE_MEM_COMMITTED,
        SizeToIntKB(gcStats_->GetRecordData(RecordData::YOUNG_COMMIT_SIZE)));
    AddRecordDataStats(RecordKeyData::GC_OLD_MEM_USED,
        SizeToIntKB(gcStats_->GetRecordData(RecordData::OLD_ALIVE_SIZE)));
    AddRecordDataStats(RecordKeyData::GC_OLD_MEM_COMMITTED,
        SizeToIntKB(gcStats_->GetRecordData(RecordData::OLD_COMMIT_SIZE)));

    AddRecordDataStats(RecordKeyData::GC_HUGE_MEM_USED,
        SizeToIntKB(heap_->GetHugeObjectSpace()->GetHeapObjectSize()));
    AddRecordDataStats(RecordKeyData::GC_HUGE_MEM_COMMITTED,
        SizeToIntKB(heap_->GetHugeObjectSpace()->GetCommittedSize()));

    AddRecordKeyDuration(RecordKeyDuration::GC_TOTAL_TIME,
        gcStats_->GetScopeDuration(GCStats::Scope::ScopeId::TotalGC));
    AddRecordKeyDuration(RecordKeyDuration::GC_MARK_TIME,
        gcStats_->GetScopeDuration(GCStats::Scope::ScopeId::Mark));
    AddRecordKeyDuration(RecordKeyDuration::GC_EVACUATE_TIME,
        gcStats_->GetScopeDuration(GCStats::Scope::ScopeId::Evacuate));

    if (CheckLastSendTimeIfSend() && CheckCountIfSend()) {
        SendSysEvent();
        PrintKeyStatisticResult();
        InitializeRecordList();
    }
}

void GCKeyStats::SendSysEvent() const
{
#ifdef ENABLE_HISYSEVENT
    int32_t ret = HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ARKTS_RUNTIME,
        "ARK_STATS_GC",
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "BUNDLE_NAME", PGOProfilerManager::GetInstance()->GetBundleName(),
        "PID", getpid(),
        "TID", syscall(SYS_gettid),
        "GC_TOTAL_COUNT", gcCount_,
        "GC_TOTAL_TIME", static_cast<int>(GetRecordKeyDuration(RecordKeyDuration::GC_TOTAL_TIME)),
        "GC_MARK_TIME", static_cast<int>(GetRecordKeyDuration(RecordKeyDuration::GC_MARK_TIME)),
        "GC_EVACUATE_TIME", static_cast<int>(GetRecordKeyDuration(RecordKeyDuration::GC_EVACUATE_TIME)),
        "GC_LONG_TIME", recordCount_,
        "GC_TOTAL_MEM_USED", GetRecordDataStats(RecordKeyData::GC_TOTAL_MEM_USED),
        "GC_TOTAL_MEM_COMMITTED", GetRecordDataStats(RecordKeyData::GC_TOTAL_MEM_COMMITTED),
        "GC_ACTIVE_MEM_USED", GetRecordDataStats(RecordKeyData::GC_ACTIVE_MEM_USED),
        "GC_ACTIVE_MEM_COMMITTED", GetRecordDataStats(RecordKeyData::GC_ACTIVE_MEM_COMMITTED),
        "GC_OLD_MEM_USED", GetRecordDataStats(RecordKeyData::GC_OLD_MEM_USED),
        "GC_OLD_MEM_COMMITTED", GetRecordDataStats(RecordKeyData::GC_OLD_MEM_COMMITTED),
        "GC_HUGE_MEM_USED", GetRecordDataStats(RecordKeyData::GC_HUGE_MEM_USED),
        "GC_HUGE_MEM_COMMITTED", GetRecordDataStats(RecordKeyData::GC_HUGE_MEM_COMMITTED));
    if (ret != 0) {
        LOG_GC(ERROR) << "GCKeyStats HiSysEventWrite Failed! ret = " << ret;
    }
#endif
}

void GCKeyStats::SendSysEventBeforeDump(std::string type, size_t limitSize, size_t activeMemory) const
{
#ifdef ENABLE_HISYSEVENT
    int32_t ret = HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::FRAMEWORK,
        "ARK_STATS_DUMP",
        OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
        "PID", getprocpid(),
        "TID", syscall(SYS_gettid),
        "PROCESS_NAME", PGOProfilerManager::GetInstance()->GetBundleName(),
        "LIMITSIZE", limitSize,
        "ACTIVE_MEMORY", activeMemory,
        "TYPE", type);
    if (ret != 0) {
        LOG_GC(ERROR) << "GCKeyStats SendSysEventBeforeDump Failed! ret = " << ret;
    }
#else
    LOG_GC(INFO) << "GCKeyStats type: " << type << ", limitSize: " << limitSize << ", activeMemory: " << activeMemory;
#endif
}

int32_t GCKeyStats::SendSysEventDataSize(std::vector<std::string> filePaths,  std::vector<uint64_t> fileSizes) const
{
#ifdef ENABLE_HISYSEVENT
    int32_t ret = HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::FILEMANAGEMENT,
        "USER_DATA_SIZE",
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "COMPONENT_NAME", COMPONENT_NAME,
        "PARTITION_NAME", PARTITION_NAME,
        "REMAIN_PARTITION_SIZE", panda::ecmascript::GetDeviceValidSize(PARTITION_NAME),
        "FILE_OR_FOLDER_PATH", filePaths,
        "FILE_OR_FOLDER_SIZE", fileSizes);
    if (ret != 0) {
        LOG_GC(ERROR) << "DataPartitionStats HiSysEventWrite Failed! ret = " << ret;
    }
    return ret;
#endif
    return 0;
}

void GCKeyStats::PrintKeyStatisticResult() const
{
    LOG_GC(INFO) << "/******************* GCKeyStats HiSysEvent statistic: *******************/";
}

void GCKeyStats::InitializeRecordList()
{
    gcCount_ = 0;
    recordCount_ = 0;
    std::fill(recordDurationStats_, recordDurationStats_ + (uint8_t)RecordKeyDuration::NUM_OF_KEY_DURATION, 0.0f);
    std::fill(recordDataStats_, recordDataStats_ + (uint8_t)RecordKeyData::NUM_OF_KEY_DATA, 0);
    lastSendTimestamp_ = Clock::now();
}

/*
| The Judgment criteria of Long GC
|  IsInBackground  |  IsSensitive or Idle |  GCTime |
| -----------------| ---------------------|---------|
|       false      |       Sensitive      |    33   |
|       false      |  !Sensitive & !Idle  |    33   |
|       true       |         Idle         |    200  |
|       true       |        !Idle         |    200  |
|       true       |         Idle         |    500  |
*/
void GCKeyStats::ProcessLongGCEvent()
{
    if (!DFXHiSysEvent::IsEnableDFXHiSysEvent() || !heap_->GetEcmaVM()->GetJSOptions().IsEnableDFXHiSysEvent()) {
        return;
    }
    LongGCStats *longGCStats = gcStats_->GetLongGCStats();
    GCReason gcReason = gcStats_->GetGCReason();
    bool gcIsSensitive = longGCStats->GetGCIsSensitive();
    bool gcIsInBackground = longGCStats->GetGCIsInBackground();
    float gcTotalTime = longGCStats->GetGCTotalTime();
    if (gcIsSensitive) {
        if (gcTotalTime > GC_SENSITIVE_LONG_TIME) {
            DFXHiSysEvent::SendLongGCEvent(longGCStats);
            longGCStats->Reset();
        }
    } else {
        if (IsIdle(gcReason)) {
            if (!gcIsInBackground && gcTotalTime > GC_IDLE_LONG_TIME) {
                longGCStats->SetCpuLoad(DFXHiSysEvent::GetCpuUsage());
                DFXHiSysEvent::SendLongGCEvent(longGCStats);
                longGCStats->Reset();
            } else if (gcIsInBackground && gcTotalTime > GC_BACKGROUD_IDLE_LONG_TIME) {
                longGCStats->SetCpuLoad(DFXHiSysEvent::GetCpuUsage());
                DFXHiSysEvent::SendLongGCEvent(longGCStats);
                longGCStats->Reset();
            }
        } else {
            if (!gcIsInBackground && gcTotalTime > GC_NOT_SENSITIVE_LONG_TIME) {
                DFXHiSysEvent::SendLongGCEvent(longGCStats);
                longGCStats->Reset();
            } else if (gcIsInBackground && gcTotalTime > GC_BACKGROUD_LONG_TIME) {
                longGCStats->SetCpuLoad(DFXHiSysEvent::GetCpuUsage());
                DFXHiSysEvent::SendLongGCEvent(longGCStats);
                longGCStats->Reset();
            }
        }
    }
}
} // namespace panda::ecmascript