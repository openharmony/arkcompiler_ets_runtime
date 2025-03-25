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
#include "ecmascript/mem/gc_key_stats.h"
#include "ecmascript/mem/heap-inl.h"

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
    PrintGCSummaryStatistic(GCType::PARTIAL_YOUNG_GC);
    PrintGCSummaryStatistic(GCType::PARTIAL_OLD_GC);
    PrintGCSummaryStatistic(GCType::COMPRESS_GC);
    PrintGCMemoryStatistic();
}

void GCStats::PrintGCStatistic()
{
    ASSERT(heap_ != nullptr);
    ASSERT(heap_->GetEcmaVM() != nullptr);
    if (heap_->GetEcmaVM()->GetJSOptions().EnableGCTracer() || CheckIfLongTimePause()) {
        LOG_GC(INFO) << " [ " << GetGCTypeName() << " ] "
                        << sizeToMB(recordData_[(uint8_t)RecordData::START_OBJ_SIZE]) << " ("
                        << sizeToMB(recordData_[(uint8_t)RecordData::START_COMMIT_SIZE]) << ") -> "
                        << sizeToMB(recordData_[(uint8_t)RecordData::END_OBJ_SIZE]) << " ("
                        << sizeToMB(recordData_[(uint8_t)RecordData::END_COMMIT_SIZE]) << ") MB, "
                        << scopeDuration_[Scope::ScopeId::TotalGC] << "(+"
                        << GetConcurrrentMarkDuration()
                        << ")ms, GCReason: " << GCReasonToString()
                        << ", MarkReason: " << MarkReasonToString();
        LOG_GC(INFO) << "IsInBackground: " << heap_->IsInBackground() << "; "
            << "SensitiveStatus: " << static_cast<int>(heap_->GetSensitiveStatus()) << "; "
            << "StartupStatus: " << std::to_string(static_cast<int>(heap_->GetStartupStatus())) << "; "
            << "BundleName: " << heap_->GetEcmaVM()->GetBundleName()
            << "; Young: " << std::to_string(heap_->GetNewSpace()->GetCommittedSize())
            << "; Old: " << std::to_string(heap_->GetOldSpace()->GetCommittedSize())
            << "; TotalCommit" << std::to_string(heap_->GetCommittedSize());
        // print verbose gc statsistics
        PrintVerboseGCStatistic();
    }
    GCFinishTrace();
    InitializeRecordList();
}

const char *GCStats::GCReasonToString()
{
    return GCReasonToString(gcReason_);
}

const char *GCStats::GCReasonToString(GCReason reason)
{
    switch (reason) {
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
        case GCReason::WORKER_DESTRUCTION:
            return "Worker Destruction";
        case GCReason::TRIGGER_BY_JS:
            return "Trigger by JS";
        case GCReason::HINT_GC:
            return "Trigger by hint";
        case GCReason::NATIVE_LIMIT:
            return "Native reach limit";
        case GCReason::SHARED_LIMIT:
            return "Shared reach limit";
        case GCReason::IDLE_NATIVE:
            return "Idle time task by native";
        case GCReason::HANDLE_MARKING_FINISHED:
            return "ConcurrentMark finished";
        case GCReason::TRIGGER_BY_ARKUI:
            return "Trigger by ArkUI";
        case GCReason::TRIGGER_BY_ABILITY:
            return "Trigger by AbilityRuntime";
        case GCReason::TRIGGER_BY_MEM_TOOLS:
            return "Trigger by Mem tools";
        case GCReason::TRIGGER_BY_TASKPOOL:
            return "Trigger by taskPool";
        default: // LCOV_EXCL_BR_LINE
            return "Other";
    }
}

const char *GCStats::MarkReasonToString()
{
    return concurrentMark_ ? MarkReasonToString(markReason_) : "Not ConcurrentMark";
}

const char *GCStats::MarkReasonToString(MarkReason reason)
{
    switch (reason) {
        case MarkReason::ALLOCATION_LIMIT:
            return "Memory reach limit";
        case MarkReason::OLD_GC_WITHOUT_FULLMARK:
            return "OldGC without concurrent full mark";
        case MarkReason::IDLE:
            return "Idle time task";
        case MarkReason::EXIT_HIGH_SENSITIVE:
            return "Exit high sensitive";
        case MarkReason::EXTERNAL_TRIGGER:
            return "Externally triggered";
        case MarkReason::WORKER_DESTRUCTION:
            return "Worker Destruction";
        case MarkReason::TRIGGER_BY_JS:
            return "Trigger by JS";
        case MarkReason::HINT_GC:
            return "Trigger by hint";
        case MarkReason::NATIVE_LIMIT:
            return "Native reach limit";
        case MarkReason::SHARED_LIMIT:
            return "Shared reach limit";
        case MarkReason::EXIT_SERIALIZE:
            return "Exit serialize";
        default: // LCOV_EXCL_BR_LINE
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

void GCStats::GCFinishTrace()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PartialGC::Finish" + std::to_string(heap_->IsConcurrentFullMark())
    + ";Reason" + std::to_string(static_cast<int>(gcReason_))
    + ";Sensitive" + std::to_string(static_cast<int>(heap_->GetSensitiveStatus()))
    + ";IsInBackground" + std::to_string(heap_->IsInBackground())
    + ";Startup" + std::to_string(heap_->OnStartupEvent())
    + ";ConMark" + std::to_string(static_cast<int>(heap_->GetJSThread()->GetMarkStatus()))
    + ";Young" + std::to_string(heap_->GetNewSpace()->GetCommittedSize())
    + ";Old" + std::to_string(heap_->GetOldSpace()->GetCommittedSize())
    + ";TotalCommit" + std::to_string(heap_->GetCommittedSize())
    + ";NativeBindingSize" + std::to_string(heap_->GetNativeBindingSize())
    + ";NativeLimitSize" + std::to_string(heap_->GetGlobalSpaceNativeLimit()));
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
                    << "AppSpawnSpace    used:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetAppSpawnSpace()->GetHeapObjectSize())) << "KB"
                    << "     committed:"
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetAppSpawnSpace()->GetCommittedSize())) << "KB";

    LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("Anno memory usage size:")
                    << STATS_DATA_FORMAT(sizeToMB(heapRegionAllocator->GetAnnoMemoryUsage())) << "MB\n"
                    << STATS_DESCRIPTION_FORMAT("Native memory usage size:")
                    << STATS_DATA_FORMAT(sizeToMB(nativeAreaAllocator->GetNativeMemoryUsage())) << "MB\n"
                    << STATS_DESCRIPTION_FORMAT("NativeBindingSize:")
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetNativeBindingSize())) << "KB\n"
                    << STATS_DESCRIPTION_FORMAT("NativeLimitSize:")
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetGlobalSpaceNativeLimit())) << "KB\n"
                    << STATS_DESCRIPTION_FORMAT("ArrayBufferNativeSize:")
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetNativeAreaAllocator()->GetArrayBufferNativeSize()))
                    << "KB\n"
                    << STATS_DESCRIPTION_FORMAT("ChunkNativeSize:")
                    << STATS_DATA_FORMAT(sizeToKB(heap_->GetNativeAreaAllocator()->GetChunkNativeSize())) << "KB";
    switch (gcType_) {
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
                         << STATS_DESCRIPTION_FORMAT("ReMark:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ReMark]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("ProcessSharedGCRSetWorkList:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ProcessSharedGCRSetWorkList]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Sweep:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Sweep]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("ClearNativeObject:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ClearNativeObject]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Evacuate:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Evacuate]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("UpdateReference:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::UpdateReference]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("UpdateWeekRef:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::UpdateWeekRef]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("UpdateRoot:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::UpdateRoot]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("ProceeWorkload:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ProceeWorkload]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("EvacuateSpace:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::EvacuateSpace]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("EvacuateRegion:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::EvacuateRegion]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("WaitFinish:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::WaitFinish]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Finish:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Finish]) << "ms";
            break;
        case GCType::COMPRESS_GC:
            LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("TotalGC:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::TotalGC]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("ProcessSharedGCRSetWorkList:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ProcessSharedGCRSetWorkList]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Initialize:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Initialize]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Mark:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Mark]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("MarkRoots:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::MarkRoots]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Sweep:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Sweep]) << "ms\n"
                         << STATS_DESCRIPTION_FORMAT("Finish:")
                         << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Finish]) << "ms";
            break;
        default: // LCOV_EXCL_BR_LINE
            break;
    }
}

bool GCStats::CheckIfNeedPrint(GCType type)
{
    uint32_t gcCount = 0;
    switch (type) {
        case GCType::PARTIAL_YOUNG_GC:
            gcCount = GetRecordData(RecordData::YOUNG_COUNT);
            break;
        case GCType::PARTIAL_OLD_GC:
            gcCount = GetRecordData(RecordData::OLD_COUNT);
            break;
        case GCType::COMPRESS_GC:
            gcCount = GetRecordData(RecordData::COMPRESS_COUNT);
            break;
        default: // LCOV_EXCL_BR_LINE
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
        default: // LCOV_EXCL_BR_LINE
            break;
    }
}

size_t GCStats::GetAccumulatedAllocateSize()
{
    return accumulatedFreeSize_ + heap_->GetHeapObjectSize();
}

void GCStats::RecordStatisticBeforeGC(TriggerGCType gcType, GCReason reason)
{
    SetRecordData(RecordData::START_OBJ_SIZE, heap_->GetHeapObjectSize());
    SetRecordData(RecordData::START_COMMIT_SIZE, heap_->GetCommittedSize());
    SetRecordData(RecordData::START_YOUNG_OBJ_SIZE, heap_->GetNewSpace()->GetHeapObjectSize());
    SetRecordData(RecordData::START_NATIVE_POINTER_NUM, heap_->GetNativePointerListSize());
    gcType_ = GetGCType(gcType);
    gcReason_ = reason;

    switch (gcType_) {
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
        default: // LCOV_EXCL_BR_LINE
            break;
    }
    ProcessBeforeLongGCStats();
}

void GCStats::RecordStatisticAfterGC()
{
    ASSERT(heap_ != nullptr);
    SetRecordData(RecordData::END_OBJ_SIZE, heap_->GetHeapObjectSize());
    SetRecordData(RecordData::END_COMMIT_SIZE, heap_->GetCommittedSize());

    float duration = scopeDuration_[Scope::ScopeId::TotalGC];
    switch (gcType_) {
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
            IncreaseRecordDuration(RecordDuration::YOUNG_TOTAL_PAUSE, duration);
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
            IncreaseRecordDuration(RecordDuration::OLD_TOTAL_PAUSE, duration);
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

    if (gcType_ == GCType::COMPRESS_GC && scopeDuration_[Scope::ScopeId::TotalGC] > longPauseTime_) {
        IncreaseFullGCLongTimeCount();
    }
    IncreaseTotalDuration(scopeDuration_[Scope::ScopeId::TotalGC]);
    IncreaseAccumulatedFreeSize(GetRecordData(RecordData::START_OBJ_SIZE) -
                                GetRecordData(RecordData::END_OBJ_SIZE));
    ProcessAfterLongGCStats();
}

void GCStats::ProcessAfterLongGCStats()
{
    LongGCStats *longGCStats = GetLongGCStats();
    float gcTotalTime = GetScopeDuration(GCStats::Scope::ScopeId::TotalGC);
    if (IsLongGC(gcReason_, heap_->InSensitiveStatus(), heap_->IsInBackground(), gcTotalTime)) {
        longGCStats->SetGCType(static_cast<int>(gcType_));
        longGCStats->SetGCReason(static_cast<int>(gcReason_));
        longGCStats->SetMarkReason(static_cast<int>(markReason_));
        longGCStats->SetGCIsSensitive(heap_->InSensitiveStatus());
        longGCStats->SetGCIsInBackground(heap_->IsInBackground());
        longGCStats->SetGCTotalTime(gcTotalTime);
        longGCStats->SetGCMarkTime(GetScopeDuration(GCStats::Scope::ScopeId::Mark));
        longGCStats->SetGCEvacuateTime(GetScopeDuration(GCStats::Scope::ScopeId::Evacuate));
        longGCStats->SetGCUpdateRootTime(GetScopeDuration(GCStats::Scope::ScopeId::UpdateRoot));
        longGCStats->SetGCUpdateWeekRefTime(GetScopeDuration(GCStats::Scope::ScopeId::UpdateWeekRef));
        longGCStats->SetGCUpdateReferenceTime(GetScopeDuration(GCStats::Scope::ScopeId::UpdateReference));
        longGCStats->SetGCSweepNewToOldTime(GetScopeDuration(GCStats::Scope::ScopeId::SweepNewToOldRegions));
        longGCStats->SetGCFinalizeTime(GetScopeDuration(GCStats::Scope::ScopeId::Finalize));
        longGCStats->SetGCInvokeCallbackTime(GetScopeDuration(GCStats::Scope::ScopeId::InvokeNativeFinalizeCallbacks));
        longGCStats->SetAfterGCTotalMemUsed(heap_->GetHeapObjectSize());
        longGCStats->SetAfterGCTotalMemCommitted(heap_->GetCommittedSize());
        longGCStats->SetAfterGCActiveMemUsed(heap_->GetNewSpace()->GetHeapObjectSize());
        longGCStats->SetAfterGCActiveMemCommitted(heap_->GetNewSpace()->GetCommittedSize());
        longGCStats->SetAfterGCOldMemUsed(heap_->GetOldSpace()->GetHeapObjectSize());
        longGCStats->SetAfterGCOldMemCommitted(heap_->GetOldSpace()->GetCommittedSize());
        longGCStats->SetAfterGCHugeMemUsed(heap_->GetHugeObjectSpace()->GetHeapObjectSize());
        longGCStats->SetAfterGCHugeMemCommitted(heap_->GetHugeObjectSpace()->GetCommittedSize());
        longGCStats->SetAfterGCNativeBindingSize(heap_->GetNativeBindingSize());
        longGCStats->SetAfterGCNativeLimit(heap_->GetGlobalSpaceNativeLimit());
    }
}

void GCStats::ProcessBeforeLongGCStats()
{
    LongGCStats *longGCStats = GetLongGCStats();
    longGCStats->SetBeforeGCTotalMemUsed(heap_->GetHeapObjectSize());
    longGCStats->SetBeforeGCTotalMemCommitted(heap_->GetCommittedSize());
    longGCStats->SetBeforeGCActiveMemUsed(heap_->GetNewSpace()->GetHeapObjectSize());
    longGCStats->SetBeforeGCActiveMemCommitted(heap_->GetNewSpace()->GetCommittedSize());
    longGCStats->SetBeforeGCOldMemUsed(heap_->GetOldSpace()->GetHeapObjectSize());
    longGCStats->SetBeforeGCOldMemCommitted(heap_->GetOldSpace()->GetCommittedSize());
    longGCStats->SetBeforeGCHugeMemUsed(heap_->GetHugeObjectSpace()->GetHeapObjectSize());
    longGCStats->SetBeforeGCHugeMemCommitted(heap_->GetHugeObjectSpace()->GetCommittedSize());
    longGCStats->SetBeforeGCNativeBindingSize(heap_->GetNativeBindingSize());
    longGCStats->SetBeforeGCNativeLimit(heap_->GetGlobalSpaceNativeLimit());
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
bool GCStats::IsLongGC(GCReason gcReason, bool gcIsSensitive, bool gcIsInBackground, float gcTotalTime)
{
    if (gcIsSensitive) {
        if (gcTotalTime > GCKeyStats::GC_SENSITIVE_LONG_TIME) {
            return true;
        }
    } else {
        if (GCKeyStats::IsIdle(gcReason)) {
            if (!gcIsInBackground && gcTotalTime > GCKeyStats::GC_IDLE_LONG_TIME) {
                return true;
            } else if (gcIsInBackground && gcTotalTime > GCKeyStats::GC_BACKGROUD_IDLE_LONG_TIME) {
                return true;
            }
        } else {
            if (!gcIsInBackground && gcTotalTime > GCKeyStats::GC_NOT_SENSITIVE_LONG_TIME) {
                return true;
            } else if (gcIsInBackground && gcTotalTime > GCKeyStats::GC_BACKGROUD_LONG_TIME) {
                return true;
            }
        }
    }
    return false;
}

void GCStats::RecordGCSpeed()
{
    double survivalRate = GetAvgSurvivalRate();
    size_t clearNativeSpeed = GetRecordData(RecordData::START_NATIVE_POINTER_NUM) /
                              scopeDuration_[Scope::ScopeId::ClearNativeObject];

    if (gcType_ == GCType::PARTIAL_YOUNG_GC) {
        size_t objSize = GetRecordData(RecordData::START_YOUNG_OBJ_SIZE);
        gcSpeed_[(uint8_t)SpeedData::MARK_SPEED] = objSize / scopeDuration_[Scope::ScopeId::Mark];
        size_t evacuateSpeed = survivalRate * objSize / scopeDuration_[Scope::ScopeId::EvacuateSpace];
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
    if (heap_ && !heap_->IsReadyToConcurrentMark()) {
        switch (heap_->GetMarkType()) {
            case MarkType::MARK_YOUNG:
                return GCType::PARTIAL_YOUNG_GC;
            case MarkType::MARK_FULL:
                return GCType::PARTIAL_OLD_GC;
            default: // LCOV_EXCL_BR_LINE
                return GCType::OTHER;
        }
    }
    switch (gcType) {
        case TriggerGCType::YOUNG_GC:
            return GCType::PARTIAL_YOUNG_GC;
        case TriggerGCType::OLD_GC:
            return GCType::PARTIAL_OLD_GC;
        case TriggerGCType::FULL_GC:
            return GCType::COMPRESS_GC;
        case TriggerGCType::SHARED_GC:
            return GCType::SHARED_GC;
        case TriggerGCType::SHARED_PARTIAL_GC:
            return GCType::SHARED_PARTIAL_GC;
        case TriggerGCType::SHARED_FULL_GC:
            return GCType::SHARED_FULL_GC;
        default:
            return GCType::OTHER;
    }
}

void GCStats::InitializeRecordList()
{
    for (float &duration : scopeDuration_) {
        duration = 0.0f;
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

void SharedGCStats::PrintStatisticResult()
{
    LOG_GC(INFO) << "/******************* SharedGCStats statistic: *******************/";
    PrintSharedGCSummaryStatistic();
    PrintGCMemoryStatistic();
}

void SharedGCStats::PrintGCStatistic()
{
    if (enableGCTracer_) {
        LOG_GC(INFO) << " [ " << GetGCTypeName() << " ] "
                     << sizeToMB(recordData_[(uint8_t)RecordData::START_OBJ_SIZE]) << " ("
                     << sizeToMB(recordData_[(uint8_t)RecordData::START_COMMIT_SIZE]) << ") -> "
                     << sizeToMB(recordData_[(uint8_t)RecordData::END_OBJ_SIZE]) << " ("
                     << sizeToMB(recordData_[(uint8_t)RecordData::END_COMMIT_SIZE]) << ") MB, "
                     << scopeDuration_[Scope::ScopeId::TotalGC]
                     << "ms, GCReason: " << GCReasonToString()
                     << ", MarkReason: " << MarkReasonToString();
        PrintSharedGCDuration();
        PrintGCMemoryStatistic();
        PrintSharedGCSummaryStatistic();
    }
    SharedGCFinishTrace();
    InitializeRecordList();
}

void SharedGCStats::SharedGCFinishTrace()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGC::Finish;Reason"
        + std::to_string(static_cast<int>(gcReason_))
        + ";Sensitive" + std::to_string(static_cast<int>(sHeap_->GetSensitiveStatus()))
        + ";IsInBackground" + std::to_string(sHeap_->IsInBackground())
        + ";Startup" + std::to_string(sHeap_->OnStartupEvent())
        + ";Old" + std::to_string(sHeap_->GetOldSpace()->GetCommittedSize())
        + ";huge" + std::to_string(sHeap_->GetHugeObjectSpace()->GetCommittedSize())
        + ";NonMov" + std::to_string(sHeap_->GetNonMovableSpace()->GetCommittedSize())
        + ";TotCommit" + std::to_string(sHeap_->GetCommittedSize())
        + ";NativeBindingSize" + std::to_string(sHeap_->GetNativeSizeAfterLastGC())
        + ";NativeLimitGC" + std::to_string(sHeap_->GetNativeSizeTriggerSharedGC())
        + ";NativeLimitCM" + std::to_string(sHeap_->GetNativeSizeTriggerSharedCM()));
}

void SharedGCStats::PrintSharedGCSummaryStatistic()
{
    LOG_GC(INFO) << "/***************** GC summary statistic: *****************/";
    LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("SharedGC occurs count")
                 << STATS_DATA_FORMAT(GetRecordData(RecordData::SHARED_COUNT)) << "\n"
                 << STATS_DESCRIPTION_FORMAT("SharedGC max pause:")
                 << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::SHARED_MAX_PAUSE)) << "ms\n"
                 << STATS_DESCRIPTION_FORMAT("SharedGC min pause:")
                 << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::SHARED_MIN_PAUSE)) << "ms\n"
                 << STATS_DESCRIPTION_FORMAT("SharedGC average pause:")
                 << STATS_DATA_FORMAT(GetRecordDuration(RecordDuration::SHARED_TOTAL_PAUSE) /
                                      GetRecordData(RecordData::SHARED_COUNT)) << "ms\n"
                 << STATS_DESCRIPTION_FORMAT("SharedHeap average alive rate:")
                 << STATS_DATA_FORMAT(double(GetRecordData(RecordData::SHARED_TOTAL_ALIVE)) /
                                      GetRecordData(RecordData::SHARED_TOTAL_COMMIT));
}

void SharedGCStats::PrintGCMemoryStatistic()
{
    NativeAreaAllocator *nativeAreaAllocator = sHeap_->GetNativeAreaAllocator();
    HeapRegionAllocator *heapRegionAllocator = sHeap_->GetHeapRegionAllocator();
    LOG_GC(INFO) << "/****************** GC Memory statistic: *****************/";
    LOG_GC(INFO) << "AllSpaces         used:"
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetHeapObjectSize())) << "KB"
                 << "     committed:"
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetCommittedSize())) << "KB\n"
                 << "SharedOldSpace         used:"
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetOldSpace()->GetHeapObjectSize())) << "KB"
                 << "     committed:"
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetOldSpace()->GetCommittedSize())) << "KB\n"
                 << "SharedNonMovableSpace  used:"
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetNonMovableSpace()->GetHeapObjectSize())) << "KB"
                 << "     committed:"
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetNonMovableSpace()->GetCommittedSize())) << "KB\n"
                 << "SharedHugeObjectSpace  used:"
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetHugeObjectSpace()->GetHeapObjectSize())) << "KB"
                 << "     committed:"
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetHugeObjectSpace()->GetCommittedSize())) << "KB\n"
                 << "SharedAppSpawnSpace    used:"
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetAppSpawnSpace()->GetHeapObjectSize())) << "KB"
                 << "     committed:"
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetAppSpawnSpace()->GetCommittedSize())) << "KB";

    LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("Anno memory usage size:")
                 << STATS_DATA_FORMAT(sizeToMB(heapRegionAllocator->GetAnnoMemoryUsage())) << "MB\n"
                 << STATS_DESCRIPTION_FORMAT("Native memory usage size:")
                 << STATS_DATA_FORMAT(sizeToMB(nativeAreaAllocator->GetNativeMemoryUsage())) << "MB\n"
                 << STATS_DESCRIPTION_FORMAT("NativeBindingSize:")
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetNativeSizeAfterLastGC())) << "KB\n"
                 << STATS_DESCRIPTION_FORMAT("NativeLimitGC:")
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetNativeSizeTriggerSharedGC())) << "KB\n"
                 << STATS_DESCRIPTION_FORMAT("NativeLimitCM:")
                 << STATS_DATA_FORMAT(sizeToKB(sHeap_->GetNativeSizeTriggerSharedCM())) << "KB\n";

    LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("Heap alive rate:")
        << STATS_DATA_FORMAT(double(GetRecordData(RecordData::SHARED_ALIVE_SIZE)) /
                                    GetRecordData(RecordData::SHARED_COMMIT_SIZE));
}

void SharedGCStats::PrintSharedGCDuration()
{
    LOG_GC(INFO) << STATS_DESCRIPTION_FORMAT("TotalGC:")
        << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::TotalGC]) << "ms\n"
        << STATS_DESCRIPTION_FORMAT("Initialize:")
        << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Initialize]) << "ms\n"
        << STATS_DESCRIPTION_FORMAT("Mark:")
        << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Mark]) << "ms\n"
        << STATS_DESCRIPTION_FORMAT("Evacuate:")
        << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Evacuate]) << "ms\n"
        << STATS_DESCRIPTION_FORMAT("UpdateReference:")
        << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::UpdateReference]) << "ms\n"
        << STATS_DESCRIPTION_FORMAT("Sweep:")
        << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Sweep]) << "ms\n"
        << STATS_DESCRIPTION_FORMAT("Finish:")
        << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::Finish]) << "ms\n"
        << STATS_DESCRIPTION_FORMAT("SuspendAll:")
        << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::SuspendAll]) << "ms\n"
        << STATS_DESCRIPTION_FORMAT("ResumeAll:")
        << STATS_DATA_FORMAT(scopeDuration_[Scope::ScopeId::ResumeAll]) << "ms";
}

size_t SharedGCStats::GetAccumulatedAllocateSize()
{
    return accumulatedFreeSize_ + sHeap_->GetHeapObjectSize();
}

void SharedGCStats::RecordStatisticBeforeGC(TriggerGCType gcType, GCReason reason)
{
    size_t commitSize = sHeap_->GetCommittedSize();
    SetRecordData(RecordData::START_OBJ_SIZE, sHeap_->GetHeapObjectSize());
    SetRecordData(RecordData::START_COMMIT_SIZE, commitSize);
    SetRecordData(RecordData::SHARED_COMMIT_SIZE, commitSize);
    IncreaseRecordData(RecordData::SHARED_TOTAL_COMMIT, commitSize);
    gcType_ = GetGCType(gcType);
    gcReason_ = reason;
    ProcessBeforeLongGCStats();
}

void SharedGCStats::RecordStatisticAfterGC()
{
    SetRecordData(RecordData::END_OBJ_SIZE, sHeap_->GetHeapObjectSize());
    SetRecordData(RecordData::END_COMMIT_SIZE, sHeap_->GetCommittedSize());

    float duration = scopeDuration_[Scope::ScopeId::TotalGC];
    if (GetRecordData(RecordData::SHARED_COUNT) == 0) {
        SetRecordDuration(RecordDuration::SHARED_MIN_PAUSE, duration);
        SetRecordDuration(RecordDuration::SHARED_MAX_PAUSE, duration);
    } else {
        SetRecordDuration(RecordDuration::SHARED_MIN_PAUSE,
            std::min(GetRecordDuration(RecordDuration::SHARED_MIN_PAUSE), duration));
        SetRecordDuration(RecordDuration::SHARED_MAX_PAUSE,
            std::max(GetRecordDuration(RecordDuration::SHARED_MAX_PAUSE), duration));
    }
    IncreaseRecordData(RecordData::SHARED_COUNT);
    IncreaseRecordDuration(RecordDuration::SHARED_TOTAL_PAUSE, duration);
    size_t heapAliveSize = sHeap_->GetHeapObjectSize();
    SetRecordData(RecordData::SHARED_ALIVE_SIZE, heapAliveSize);
    IncreaseRecordData(RecordData::SHARED_TOTAL_ALIVE, heapAliveSize);

    IncreaseTotalDuration(scopeDuration_[Scope::ScopeId::TotalGC]);
    IncreaseAccumulatedFreeSize(GetRecordData(RecordData::START_OBJ_SIZE) -
                                GetRecordData(RecordData::END_OBJ_SIZE));
    ProcessAfterLongGCStats();
}

void SharedGCStats::ProcessAfterLongGCStats()
{
    LongGCStats *longGCStats = GetLongGCStats();
    float gcTotalTime = GetScopeDuration(GCStats::Scope::ScopeId::TotalGC);
    if (IsLongGC(gcReason_, sHeap_->InSensitiveStatus(), sHeap_->IsInBackground(), gcTotalTime)) {
        longGCStats->SetGCType(static_cast<int>(gcType_));
        longGCStats->SetGCReason(static_cast<int>(gcReason_));
        longGCStats->SetMarkReason(static_cast<int>(markReason_));
        longGCStats->SetGCIsSensitive(sHeap_->InSensitiveStatus());
        longGCStats->SetGCIsInBackground(sHeap_->IsInBackground());
        longGCStats->SetGCTotalTime(gcTotalTime);
        longGCStats->SetGCMarkTime(GetScopeDuration(GCStats::Scope::ScopeId::Mark));
        longGCStats->SetAfterGCTotalMemUsed(sHeap_->GetHeapObjectSize());
        longGCStats->SetAfterGCTotalMemCommitted(sHeap_->GetCommittedSize());
        longGCStats->SetAfterGCOldMemUsed(sHeap_->GetOldSpace()->GetHeapObjectSize());
        longGCStats->SetAfterGCOldMemCommitted(sHeap_->GetOldSpace()->GetCommittedSize());
        longGCStats->SetAfterGCHugeMemUsed(sHeap_->GetHugeObjectSpace()->GetHeapObjectSize());
        longGCStats->SetAfterGCHugeMemCommitted(sHeap_->GetHugeObjectSpace()->GetCommittedSize());
        longGCStats->SetAfterGCNativeBindingSize(sHeap_->GetNativeSizeAfterLastGC());
        longGCStats->SetAfterGCNativeLimit(sHeap_->GetNativeSizeTriggerSharedGC());
    }
}

void SharedGCStats::ProcessBeforeLongGCStats()
{
    LongGCStats *longGCStats = GetLongGCStats();
    longGCStats->SetBeforeGCTotalMemUsed(sHeap_->GetHeapObjectSize());
    longGCStats->SetBeforeGCTotalMemCommitted(sHeap_->GetCommittedSize());
    longGCStats->SetBeforeGCOldMemUsed(sHeap_->GetOldSpace()->GetHeapObjectSize());
    longGCStats->SetBeforeGCOldMemCommitted(sHeap_->GetOldSpace()->GetCommittedSize());
    longGCStats->SetBeforeGCHugeMemUsed(sHeap_->GetHugeObjectSpace()->GetHeapObjectSize());
    longGCStats->SetBeforeGCHugeMemCommitted(sHeap_->GetHugeObjectSpace()->GetCommittedSize());
    longGCStats->SetBeforeGCNativeBindingSize(sHeap_->GetNativeSizeAfterLastGC());
    longGCStats->SetBeforeGCNativeLimit(sHeap_->GetNativeSizeTriggerSharedGC());
}
}  // namespace panda::ecmascript
