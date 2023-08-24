/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/gc_stats.h"

#include <iomanip>
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/mem.h"

constexpr int DESCRIPTION_LENGTH = 25;
constexpr int DATA_LENGTH = 8;

#define STATS_DESCRIPTION_FORMAT(description)    \
    std::left << std::setw(DESCRIPTION_LENGTH) << (description)

#define STATS_DATA_FORMAT(data)    \
    std::setw(DATA_LENGTH) << (data)

namespace panda::ecmascript {
void GCStats::PrintStatisticResult()
{
    LOG_GC(INFO) << "/******************* GCStats statistic: *******************/";
    PrintGCSummaryStatistic(GCType::STW_YOUNG_GC);
    PrintGCSummaryStatistic(GCType::PARTIAL_YOUNG_GC);
    PrintGCSummaryStatistic(GCType::PARTIAL_OLD_GC);
    PrintGCSummaryStatistic(GCType::COMPRESS_GC);
    PrintGCMemoryStatistic();
}

void GCStats::PrintGCStatistic()
{
    if (heap_->GetEcmaVM()->GetJSOptions().EnableGCTracer() || CheckIfLongTimePause()) {
        LOG_GC(INFO) << " [ " << GetGCTypeName() << " ] "
                        << sizeToMB(recordData_[(uint8_t)RecordData::START_OBJ_SIZE]) << " ("
                        << sizeToMB(recordData_[(uint8_t)RecordData::START_COMMIT_SIZE]) << ") -> "
                        << sizeToMB(recordData_[(uint8_t)RecordData::END_OBJ_SIZE]) << " ("
                        << sizeToMB(recordData_[(uint8_t)RecordData::END_COMMIT_SIZE]) << ") MB, "
                        << scopeDuration_[Scope::ScopeId::TotalGC] << "(+"
                        << GetConcurrrentMarkDuration()
                        << ")ms, " << GCReasonToString();

        // print verbose gc statsistics
        PrintVerboseGCStatistic();
    }
    InitializeRecordList();
}

const char *GCStats::GetGCTypeName()
{
    switch (gcType_) {
        case GCType::STW_YOUNG_GC:
            return "STWYoungGC";
        case GCType::PARTIAL_YOUNG_GC:
            return "HPP YoungGC";
        case GCType::PARTIAL_OLD_GC:
            return "HPP OldGC";
        case GCType::COMPRESS_GC:
            return "CompressGC";
        default:
            return "UnknownType";
    }
}

const char *GCStats::GCReasonToString()
{
    switch (reason_) {
        case GCReason::ALLOCATION_LIMIT:
            return "Memory reach limit";
        case GCReason::ALLOCATION_FAILED:
            return "Allocate object failed";
        case GCReason::IDLE:
            return "Idle time task";
        case GCReason::SWITCH_BACKGROUND:
            return "Switch to background";
        case GCReason::EXTERNAL_TRIGGER:
            return "Externally triggered";
        default:
            return "Other";
    }
}

float GCStats::GetConcurrrentMarkDuration()
{
    return concurrentMark_ ? scopeDuration_[Scope::ScopeId::ConcurrentMark] : 0;
}

void GCStats::PrintVerboseGCStatistic()
{
    PrintGCDurationStatistic();
    PrintGCMemoryStatistic();
    PrintGCSummaryStatistic();
}

void GCStats::PrintGCMemoryStatistic()
{
    NativeAreaAllocator *nativeAreaAllocator = heap_->GetNativeAreaAllocator();
    HeapRegionAllocator *heapRegionAllocator = heap_->GetHeapRegionAllocator();
    LOG_GC(INFO) << "/****************** GC Memory statistic: *****************/";
    LOG_GC(INFO) << "AllSpaces        used:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetHeapObjectSize())) << "KB"
                    << "     committed:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetCommittedSize())) << "KB\n"
                    << "ActiveSemiSpace  used:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetNewSpace()->GetHeapObjectSize())) << "KB"
                    << "     committed:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetNewSpace()->GetCommittedSize())) << "KB\n"
                    << "OldSpace         used:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetOldSpace()->GetHeapObjectSize())) << "KB"
                    << "     committed:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetOldSpace()->GetCommittedSize())) << "KB\n"
                    << "HugeObjectSpace  used:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetHugeObjectSpace()->GetHeapObjectSize())) << "KB"
                    << "     committed:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetHugeObjectSpace()->GetCommittedSize())) << "KB\n"
                    << "NonMovableSpace  used:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetNonMovableSpace()->GetHeapObjectSize())) << "KB"
                    << "     committed:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetNonMovableSpace()->GetCommittedSize())) << "KB\n"
                    << "MachineCodeSpace used:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetMachineCodeSpace()->GetHeapObjectSize())) << "KB"
                    << "     committed:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetMachineCodeSpace()->GetCommittedSize())) << "KB\n"
                    << "SnapshotSpace    used:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetSnapshotSpace()->GetHeapObjectSize())) << "KB"
                    << "     committed:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetSnapshotSpace()->GetCommittedSize())) << "KB\n"
                    << "AppSpawnSpace    used:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetAppSpawnSpace()->GetHeapObjectSize())) << "KB"
                    << "     committed:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetAppSpawnSpace()->GetCommittedSize())) << "KB";

    LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("Anno memory usage size:")
                    << STATS_DATA_FORMAT(sizeToMB(heapRegionAllocator->GetAnnoMemoryUsage())) << "MB\n"
                    << STATS_DESCRIPTION_FORMAT("Native memory usage size:")
                    << STATS_DATA_FORMAT(sizeToMB(nativeAreaAllocator->GetNativeMemoryUsage())) << "MB\n"
                    << STATS_DESCRIPTION_FORMAT("NativeBindingSize:")
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetNativeBindingSize())) << "KB";

    switch (gcType_) {
        case GCType::STW_YOUNG_GC: {
            double copiedRate = double(GetRecordData(RecordData::SEMI_ALIVE_SIZE)) /
                                GetRecordData(RecordData::SEMI_COMMIT_SIZE);
            double premotedRate = double(GetRecordData(RecordData::SEMI_PROMOTE_SIZE)) /
                                  GetRecordData(RecordData::SEMI_COMMIT_SIZE);
            double survivalRate = std::min(copiedRate + premotedRate, 1.0);
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("Young copied rate:") << STATS_DATA_FORMAT(copiedRate) << "\n"
                << STATS_DESCRIPTION_FORMAT("Young promoted rate:") << STATS_DATA_FORMAT(premotedRate) << "\n"
                << STATS_DESCRIPTION_FORMAT("Young survival rate:") << STATS_DATA_FORMAT(survivalRate);
            break;
        }
        case GCType::PARTIAL_YOUNG_GC: {
            double copiedRate = double(GetRecordData(RecordData::YOUNG_ALIVE_SIZE)) /
                                GetRecordData(RecordData::YOUNG_COMMIT_SIZE);
            double premotedRate = double(GetRecordData(RecordData::YOUNG_PROMOTE_SIZE)) /
                                  GetRecordData(RecordData::YOUNG_COMMIT_SIZE);
            double survivalRate = std::min(copiedRate + premotedRate, 1.0);
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("Young copied rate:") << STATS_DATA_FORMAT(copiedRate) << "\n"
                << STATS_DESCRIPTION_FORMAT("Young promoted rate:") << STATS_DATA_FORMAT(premotedRate) << "\n"
                << STATS_DESCRIPTION_FORMAT("Young survival rate:") << STATS_DATA_FORMAT(survivalRate);
            break;
        }
        case GCType::PARTIAL_OLD_GC: {
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("Heap alive rate:")
                << STATS_DATA_FORMAT(double(GetRecordData(RecordData::OLD_ALIVE_SIZE)) /
                                     GetRecordData(RecordData::OLD_COMMIT_SIZE));
            break;
        }
        case GCType::COMPRESS_GC: {
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("Heap alive rate:")
                << STATS_DATA_FORMAT(double(GetRecordData(RecordData::COMPRESS_ALIVE_SIZE)) /
                                     GetRecordData(RecordData::COMPRESS_COMMIT_SIZE));
            break;
        }
        default:
            break;
    }
}

void GCStats::PrintGCDurationStatistic()
{
    LOG_GC(INFO) << "/***************** GC Duration statistic: ****************/";
    switch (gcType_) {
        case GCType::STW_YOUNG_GC:
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("TotalGC:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::TotalGC]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Initialize:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Initialize]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Mark:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Mark]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("MarkRoots:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::MarkRoots]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("ProcessMarkStack:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ProcessMarkStack]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Sweep:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Sweep]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Finish:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Finish]) << "ms";
            break;
        case GCType::PARTIAL_YOUNG_GC:
        case GCType::PARTIAL_OLD_GC:
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("TotalGC:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::TotalGC]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Initialize:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Initialize]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Mark:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Mark]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("MarkRoots:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::MarkRoots]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("ConcurrentMark pause:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ConcurrentMark]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("WaitConcurrentMarkFinish:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::WaitConcurrentMarkFinished]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("ProcessMarkStack:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ProcessMarkStack]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("ReMark:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ReMark]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Sweep:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Sweep]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("ClearNativeObject:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ClearNativeObject]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Evacuate:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Evacuate]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("UpdateReference:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::UpdateReference]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("EvacuateSpace:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::EvacuateSpace]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Finish:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Finish]) << "ms";
            break;
        case GCType::COMPRESS_GC:
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("TotalGC:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::TotalGC]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Initialize:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Initialize]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Mark:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Mark]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("MarkRoots:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::MarkRoots]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("ProcessMarkStack:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ProcessMarkStack]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Sweep:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Sweep]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Finish:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Finish]) << "ms";
            break;
        default:
            break;
    }
}

bool GCStats::CheckIfNeedPrint(GCType type)
{
    uint32_t gcCount = 0;
    switch (type) {
        case GCType::STW_YOUNG_GC:
            gcCount = GetRecordData(RecordData::SEMI_COUNT);
            break;
        case GCType::PARTIAL_YOUNG_GC:
            gcCount = GetRecordData(RecordData::YOUNG_COUNT);
            break;
        case GCType::PARTIAL_OLD_GC:
            gcCount = GetRecordData(RecordData::OLD_COUNT);
            break;
        case GCType::COMPRESS_GC:
            gcCount = GetRecordData(RecordData::COMPRESS_COUNT);
            break;
        default:
            break;
    }

    if (gcCount > 0) {
        return true;
    }
    return false;
}

void GCStats::PrintGCSummaryStatistic(GCType type)
{
    if (type != GCType::START && !CheckIfNeedPrint(type)) {
        return;
    } else {
        type = type == GCType::START ? gcType_ : type;
    }
    LOG_GC(INFO) << "/***************** GC summary statistic: *****************/";
    switch (type) {
        case GCType::STW_YOUNG_GC: {
            double copiedRate = double(GetRecordData(RecordData::SEMI_TOTAL_ALIVE)) /
                                GetRecordData(RecordData::SEMI_TOTAL_COMMIT);
            double promotedRate = double(GetRecordData(RecordData::SEMI_TOTAL_PROMOTE)) /
                                  GetRecordData(RecordData::SEMI_TOTAL_COMMIT);
            double survivalRate = std::min(copiedRate + promotedRate, 1.0);
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("STWYoungGC occurs count")
                << STATS_DATA_FORMAT(GetRecordData(RecordData::SEMI_COUNT)) << "\n"
                << STATS_DESCRIPTION_FORMAT("STWYoungGC max pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::SEMI_MAX_PAUSE)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("STWYoungGC min pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::SEMI_MIN_PAUSE)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("STWYoungGC average pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::SEMI_TOTAL_PAUSE) /
                                     GetRecordData(RecordData::SEMI_COUNT)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("Young average copied rate:") << STATS_DATA_FORMAT(copiedRate) << "\n"
                << STATS_DESCRIPTION_FORMAT("Young average promoted rate:") << STATS_DATA_FORMAT(promotedRate) << "\n"
                << STATS_DESCRIPTION_FORMAT("Young average survival rate:") << STATS_DATA_FORMAT(survivalRate);
            break;
        }
        case GCType::PARTIAL_YOUNG_GC: {
            double copiedRate = double(GetRecordData(RecordData::YOUNG_TOTAL_ALIVE)) /
                                GetRecordData(RecordData::YOUNG_TOTAL_COMMIT);
            double promotedRate = double(GetRecordData(RecordData::YOUNG_TOTAL_PROMOTE)) /
                                  GetRecordData(RecordData::YOUNG_TOTAL_COMMIT);
            double survivalRate =  std::min(copiedRate + promotedRate, 1.0);
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("YoungGC occurs count")
                << STATS_DATA_FORMAT(GetRecordData(RecordData::YOUNG_COUNT)) << "\n"
                << STATS_DESCRIPTION_FORMAT("YoungGC max pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::YOUNG_MAX_PAUSE)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("YoungGC min pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::YOUNG_MIN_PAUSE)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("YoungGC average pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::YOUNG_TOTAL_PAUSE) /
                                     GetRecordData(RecordData::YOUNG_COUNT)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("Young average copied rate:") << STATS_DATA_FORMAT(copiedRate) << "\n"
                << STATS_DESCRIPTION_FORMAT("Young average promoted rate:") << STATS_DATA_FORMAT(promotedRate) << "\n"
                << STATS_DESCRIPTION_FORMAT("Young average survival rate:") << STATS_DATA_FORMAT(survivalRate);
            break;
        }
        case GCType::PARTIAL_OLD_GC: {
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("OldGC occurs count")
                << STATS_DATA_FORMAT(GetRecordData(RecordData::OLD_COUNT)) << "\n"
                << STATS_DESCRIPTION_FORMAT("OldGC max pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::OLD_MAX_PAUSE)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("OldGC min pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::OLD_MIN_PAUSE)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("OldGC average pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::OLD_TOTAL_PAUSE) /
                                     GetRecordData(RecordData::OLD_COUNT)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("Heap average alive rate:")
                << STATS_DATA_FORMAT(double(GetRecordData(RecordData::OLD_TOTAL_ALIVE)) /
                                     GetRecordData(RecordData::OLD_TOTAL_COMMIT));
            break;
        }
        case GCType::COMPRESS_GC: {
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("CompressGC occurs count")
                << STATS_DATA_FORMAT(GetRecordData(RecordData::COMPRESS_COUNT)) << "\n"
                << STATS_DESCRIPTION_FORMAT("CompressGC max pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::COMPRESS_MAX_PAUSE)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("CompressGC min pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::COMPRESS_MIN_PAUSE)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("CompressGC average pause:")
                << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::COMPRESS_TOTAL_PAUSE) /
                                     GetRecordData(RecordData::COMPRESS_COUNT)) << "ms\n"
                << STATS_DESCRIPTION_FORMAT("Heap average alive rate:")
                << STATS_DATA_FORMAT(double(GetRecordData(RecordData::COMPRESS_TOTAL_ALIVE)) /
                                     GetRecordData(RecordData::COMPRESS_TOTAL_COMMIT));
            break;
        }
        default:
            break;
    }
}

void GCStats::RecordStatisticBeforeGC(TriggerGCType gcType, GCReason reason)
{
    SetRecordData(RecordData::START_OBJ_SIZE, heap_->GetHeapObjectSize());
    SetRecordData(RecordData::START_COMMIT_SIZE, heap_->GetCommittedSize());
    SetRecordData(RecordData::START_YOUNG_OBJ_SIZE, heap_->GetNewSpace()->GetHeapObjectSize());
    SetRecordData(RecordData::START_NATIVE_POINTER_NUM, heap_->GetEcmaVM()->GetNativePointerListSize());
    gcType_ = GetGCType(gcType);
    reason_ = reason;

    switch (gcType_) {
        case GCType::STW_YOUNG_GC: {
            size_t semiCommitSize = heap_->GetNewSpace()->GetCommittedSize();
            SetRecordData(RecordData::SEMI_COMMIT_SIZE, semiCommitSize);
            IncreaseRecordData(RecordData::SEMI_TOTAL_COMMIT, semiCommitSize);
            break;
        }
        case GCType::PARTIAL_YOUNG_GC: {
            size_t youngCommitSize = heap_->GetNewSpace()->GetCommittedSize();
            SetRecordData(RecordData::YOUNG_COMMIT_SIZE, youngCommitSize);
            IncreaseRecordData(RecordData::YOUNG_TOTAL_COMMIT, youngCommitSize);
            break;
        }
        case GCType::PARTIAL_OLD_GC: {
            size_t oldCommitSize = heap_->GetCommittedSize();
            SetRecordData(RecordData::OLD_COMMIT_SIZE, oldCommitSize);
            IncreaseRecordData(RecordData::OLD_TOTAL_COMMIT, oldCommitSize);
            break;
        }
        case GCType::COMPRESS_GC: {
            size_t compressCommitSize = heap_->GetCommittedSize();
            SetRecordData(RecordData::COMPRESS_COMMIT_SIZE, compressCommitSize);
            IncreaseRecordData(RecordData::COMPRESS_TOTAL_COMMIT, compressCommitSize);
            break;
        }
        default:
            break;
    }
}

void GCStats::RecordStatisticAfterGC()
{
    SetRecordData(RecordData::END_OBJ_SIZE, heap_->GetHeapObjectSize());
    SetRecordData(RecordData::END_COMMIT_SIZE, heap_->GetCommittedSize());

    float duration = scopeDuration_[Scope::ScopeId::TotalGC];
    switch (gcType_) {
        case GCType::STW_YOUNG_GC: {
            if (GetRecordData(RecordData::SEMI_COUNT) == 0) {
                SetRecordDuration(RecordDuration::SEMI_MIN_PAUSE, duration);
                SetRecordDuration(RecordDuration::SEMI_MAX_PAUSE, duration);
            } else {
                SetRecordDuration(RecordDuration::SEMI_MIN_PAUSE,
                    std::min(GetRecordDuration(RecordDuration::SEMI_MIN_PAUSE), duration));
                SetRecordDuration(RecordDuration::SEMI_MAX_PAUSE,
                    std::max(GetRecordDuration(RecordDuration::SEMI_MAX_PAUSE), duration));
            }
            IncreaseRecordData(RecordData::SEMI_COUNT);
            IncreaseRecordDuration(RecordDuration::SEMI_TOTAL_PAUSE, duration);
            size_t semiAliveSize = heap_->GetNewSpace()->GetHeapObjectSize();
            SetRecordData(RecordData::SEMI_ALIVE_SIZE, semiAliveSize);
            IncreaseRecordData(RecordData::SEMI_TOTAL_ALIVE, semiAliveSize);
            size_t promotedSize = heap_->GetPromotedSize();
            SetRecordData(RecordData::SEMI_PROMOTE_SIZE, promotedSize);
            IncreaseRecordData(RecordData::SEMI_TOTAL_PROMOTE, promotedSize);
            break;
        }
        case GCType::PARTIAL_YOUNG_GC: {
            if (GetRecordData(RecordData::YOUNG_COUNT) == 0) {
                SetRecordDuration(RecordDuration::YOUNG_MIN_PAUSE, duration);
                SetRecordDuration(RecordDuration::YOUNG_MAX_PAUSE, duration);
            } else {
                SetRecordDuration(RecordDuration::YOUNG_MIN_PAUSE,
                    std::min(GetRecordDuration(RecordDuration::YOUNG_MIN_PAUSE), duration));
                SetRecordDuration(RecordDuration::YOUNG_MAX_PAUSE,
                    std::max(GetRecordDuration(RecordDuration::YOUNG_MAX_PAUSE), duration));
            }
            IncreaseRecordData(RecordData::YOUNG_COUNT);
            float concurrentMarkDuration = scopeDuration_[Scope::ScopeId::ConcurrentMark];
            IncreaseRecordDuration(RecordDuration::YOUNG_TOTAL_PAUSE, duration + concurrentMarkDuration);
            size_t youngAliveSize = heap_->GetNewSpace()->GetHeapObjectSize();
            SetRecordData(RecordData::YOUNG_ALIVE_SIZE, youngAliveSize);
            IncreaseRecordData(RecordData::YOUNG_TOTAL_ALIVE, youngAliveSize);
            size_t promotedSize = heap_->GetPromotedSize();
            SetRecordData(RecordData::YOUNG_PROMOTE_SIZE, promotedSize);
            IncreaseRecordData(RecordData::YOUNG_TOTAL_PROMOTE, promotedSize);
            break;
        }
        case GCType::PARTIAL_OLD_GC: {
            if (GetRecordData(RecordData::OLD_COUNT) == 0) {
                SetRecordDuration(RecordDuration::OLD_MIN_PAUSE, duration);
                SetRecordDuration(RecordDuration::OLD_MAX_PAUSE, duration);
            } else {
                SetRecordDuration(RecordDuration::OLD_MIN_PAUSE,
                    std::min(GetRecordDuration(RecordDuration::OLD_MIN_PAUSE), duration));
                SetRecordDuration(RecordDuration::OLD_MAX_PAUSE,
                    std::max(GetRecordDuration(RecordDuration::OLD_MAX_PAUSE), duration));
            }
            IncreaseRecordData(RecordData::OLD_COUNT);
            float concurrentMarkDuration = scopeDuration_[Scope::ScopeId::ConcurrentMark];
            IncreaseRecordDuration(RecordDuration::OLD_TOTAL_PAUSE, duration + concurrentMarkDuration);
            size_t oldAliveSize = heap_->GetHeapObjectSize();
            SetRecordData(RecordData::OLD_ALIVE_SIZE, oldAliveSize);
            IncreaseRecordData(RecordData::OLD_TOTAL_ALIVE, oldAliveSize);
            break;
        }
        case GCType::COMPRESS_GC: {
            if (GetRecordData(RecordData::COMPRESS_COUNT) == 0) {
                SetRecordDuration(RecordDuration::COMPRESS_MIN_PAUSE, duration);
                SetRecordDuration(RecordDuration::COMPRESS_MAX_PAUSE, duration);
            } else {
                SetRecordDuration(RecordDuration::COMPRESS_MIN_PAUSE,
                    std::min(GetRecordDuration(RecordDuration::COMPRESS_MIN_PAUSE), duration));
                SetRecordDuration(RecordDuration::COMPRESS_MAX_PAUSE,
                    std::max(GetRecordDuration(RecordDuration::COMPRESS_MAX_PAUSE), duration));
            }
            IncreaseRecordData(RecordData::COMPRESS_COUNT);
            IncreaseRecordDuration(RecordDuration::COMPRESS_TOTAL_PAUSE, duration);
            size_t compressAliveSize = heap_->GetHeapObjectSize();
            SetRecordData(RecordData::COMPRESS_ALIVE_SIZE, compressAliveSize);
            IncreaseRecordData(RecordData::COMPRESS_TOTAL_ALIVE, compressAliveSize);
            break;
        }
        default:
            break;
    }
    RecordGCSpeed();
}

void GCStats::RecordGCSpeed()
{
    double survivalRate = GetAvgSurvivalRate();
    size_t clearNativeSpeed = GetRecordData(RecordData::START_NATIVE_POINTER_NUM) /
                              scopeDuration_[Scope::ScopeId::ClearNativeObject];

    if (gcType_ == GCType::PARTIAL_YOUNG_GC) {
        gcSpeed_[(uint8_t)SpeedData::MARK_SPEED] =
            GetRecordData(RecordData::START_YOUNG_OBJ_SIZE) / scopeDuration_[Scope::ScopeId::Mark];
        size_t evacuateSpeed = survivalRate * GetRecordData(RecordData::START_YOUNG_OBJ_SIZE) /
                               scopeDuration_[Scope::ScopeId::EvacuateSpace];
        gcSpeed_[(uint8_t)SpeedData::YOUNG_EVACUATE_SPACE_SPEED] =
            (evacuateSpeed + gcSpeed_[(uint8_t)SpeedData::YOUNG_EVACUATE_SPACE_SPEED]) / 2;  // 2 means half
        gcSpeed_[(uint8_t)SpeedData::YOUNG_CLEAR_NATIVE_OBJ_SPEED] =
            (clearNativeSpeed + gcSpeed_[(uint8_t)SpeedData::YOUNG_CLEAR_NATIVE_OBJ_SPEED]) / 2;  // 2 means half
        size_t updateReferenceSpeed = GetRecordData(RecordData::START_OBJ_SIZE) /
                                      scopeDuration_[Scope::ScopeId::UpdateReference];
        gcSpeed_[(uint8_t)SpeedData::YOUNG_UPDATE_REFERENCE_SPEED] =
            (updateReferenceSpeed + gcSpeed_[(uint8_t)SpeedData::YOUNG_UPDATE_REFERENCE_SPEED]) / 2;  // 2 means half
    } else if (gcType_ == GCType::PARTIAL_OLD_GC) {
        gcSpeed_[(uint8_t)SpeedData::MARK_SPEED] =
            GetRecordData(RecordData::START_OBJ_SIZE) / scopeDuration_[Scope::ScopeId::Mark];
        size_t sweepSpeed = GetRecordData(RecordData::START_OBJ_SIZE) / scopeDuration_[Scope::ScopeId::Sweep];
        gcSpeed_[(uint8_t)SpeedData::SWEEP_SPEED] =
            (sweepSpeed + gcSpeed_[(uint8_t)SpeedData::SWEEP_SPEED]) / 2;  // 2 means half
        gcSpeed_[(uint8_t)SpeedData::OLD_CLEAR_NATIVE_OBJ_SPEED] =
            (clearNativeSpeed + gcSpeed_[(uint8_t)SpeedData::OLD_CLEAR_NATIVE_OBJ_SPEED]) / 2;  // 2 means half

        size_t evacuateSpaceSpeed = (survivalRate * GetRecordData(RecordData::START_YOUNG_OBJ_SIZE) +
            GetRecordData(RecordData::COLLECT_REGION_SET_SIZE)) / scopeDuration_[Scope::ScopeId::EvacuateSpace];
        gcSpeed_[(uint8_t)SpeedData::OLD_EVACUATE_SPACE_SPEED] =
            (evacuateSpaceSpeed + gcSpeed_[(uint8_t)SpeedData::OLD_EVACUATE_SPACE_SPEED]) / 2;  // 2 means half

        size_t updateReferenceSpeed = GetRecordData(RecordData::START_OBJ_SIZE) /
                                    scopeDuration_[Scope::ScopeId::UpdateReference];
        gcSpeed_[(uint8_t)SpeedData::UPDATE_REFERENCE_SPEED] =
            (updateReferenceSpeed + gcSpeed_[(uint8_t)SpeedData::UPDATE_REFERENCE_SPEED]) / 2;  // 2 means half
    }
}

GCType GCStats::GetGCType(TriggerGCType gcType)
{
    if (!heap_->GetJSThread()->IsReadyToMark()) {
        return heap_->IsFullMark() ? GCType::PARTIAL_OLD_GC : GCType::PARTIAL_YOUNG_GC;
    }
    switch (gcType) {
        case TriggerGCType::YOUNG_GC:
            return GCType::PARTIAL_YOUNG_GC;
        case TriggerGCType::OLD_GC:
            return GCType::PARTIAL_OLD_GC;
        case TriggerGCType::FULL_GC:
            return GCType::COMPRESS_GC;
        default:
            return GCType::OTHER;
    }
}

void GCStats::InitializeRecordList()
{
    for (float &duration : scopeDuration_) {
        duration = 0.0f;
    }
    for (int idx = 0; idx <= GetRecordDataIndex(RecordData::END_RECORD_OVERWRITE); idx++) {
        recordData_[idx] = 0;
    }
    concurrentMark_ = false;
}

bool GCStats::CheckIfLongTimePause()
{
    if (scopeDuration_[Scope::ScopeId::TotalGC] > longPauseTime_) {
        LOG_GC(INFO) << "Has checked a long time gc";
        return true;
    }
    return false;
}
}  // namespace panda::ecmascript
