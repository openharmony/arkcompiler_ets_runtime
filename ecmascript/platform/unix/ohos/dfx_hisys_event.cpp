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

#include "ecmascript/platform/dfx_hisys_event.h"
#ifdef ENABLE_HISYSEVENT
#include "hisysevent.h"
#endif
#ifdef ENABLE_UCOLLECTION
#include "cpu_collector_client.h"
#endif
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/log.h"

namespace panda::ecmascript {
using PGOProfilerManager = pgo::PGOProfilerManager;

void DFXHiSysEvent::SendLongGCEvent([[maybe_unused]] LongGCStats *longGCStats)
{
#ifdef ENABLE_HISYSEVENT
    LOG_GC(DEBUG) << "SendLongGCEvent: GC_TYPE" << longGCStats->GetGCType()
        << ";GC_REASON" << longGCStats->GetGCReason()
        << ";GC_IS_SENSITIVE" << longGCStats->GetGCIsSensitive()
        << ";GC_TOTAL_TIME" << longGCStats->GetGCTotalTime();
    int32_t ret = HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ARKTS_RUNTIME,
        "ARK_GC_LONG_TIME", OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "BUNDLE_NAME", PGOProfilerManager::GetInstance()->GetBundleName(),
        "PID", getpid(),
        "TID", syscall(SYS_gettid),
        "GC_TYPE", longGCStats->GetGCType(),
        "GC_REASON", longGCStats->GetGCReason(),
        "GC_IS_SENSITIVE", static_cast<int>(longGCStats->GetGCIsSensitive()),
        "GC_IS_INBACKGROUD", static_cast<int>(longGCStats->GetGCIsInBackground()),
        "GC_TOTAL_TIME", longGCStats->GetGCTotalTime(),
        "GC_MARK_TIME", longGCStats->GetGCMarkTime(),
        "GC_EVACUATE_TIME", longGCStats->GetGCEvacuateTime(),
        "GC_UPDATE_ROOT_TIME", longGCStats->GetGCUpdateRootTime(),
        "GC_UPDATE_WEEKREF_TIME", longGCStats->GetGCUpdateWeekRefTime(),
        "GC_UPDATE_REFERENCE_TIME", longGCStats->GetGCUpdateReferenceTime(),
        "GC_SWEEPNEWTOOLD_TIME", longGCStats->GetGCSweepNewToOldTime(),
        "GC_FINALIZE_TIME", longGCStats->GetGCFinalizeTime(),
        "GC_INVOKE_CALLBACK_TIME", longGCStats->GetGCInvokeCallbackTime(),
        "BEFORE_GC_TOTAL_MEM_USED", LongGCStats::SizeToIntKB(longGCStats->GetBeforeGCTotalMemUsed()),
        "BEFORE_GC_TOTAL_MEM_COMMITTED", LongGCStats::SizeToIntKB(longGCStats->GetBeforeGCTotalMemCommitted()),
        "BEFORE_GC_ACTIVE_MEM_USED", LongGCStats::SizeToIntKB(longGCStats->GetBeforeGCActiveMemUsed()),
        "BEFORE_GC_ACTIVE_MEM_COMMITTED", LongGCStats::SizeToIntKB(longGCStats->GetBeforeGCActiveMemCommitted()),
        "BEFORE_GC_OLD_MEM_USED", LongGCStats::SizeToIntKB(longGCStats->GetBeforeGCOldMemUsed()),
        "BEFORE_GC_OLD_MEM_COMMITTED", LongGCStats::SizeToIntKB(longGCStats->GetBeforeGCOldMemCommitted()),
        "BEFORE_GC_HUGE_MEM_USED", LongGCStats::SizeToIntKB(longGCStats->GetBeforeGCHugeMemUsed()),
        "BEFORE_GC_HUGE_MEM_COMMITTED", LongGCStats::SizeToIntKB(longGCStats->GetBeforeGCHugeMemCommitted()),
        "BEFORE_GC_NATIVE_BINDING_SIZE", LongGCStats::SizeToIntKB(longGCStats->GetBeforeGCNativeBindingSize()),
        "BEFORE_GC_NATIVE_LIMIT", LongGCStats::SizeToIntKB(longGCStats->GetBeforeGCNativeLimit()),
        "AFTER_GC_TOTAL_MEM_USED", LongGCStats::SizeToIntKB(longGCStats->GetAfterGCTotalMemUsed()),
        "AFTER_GC_TOTAL_MEM_COMMITTED", LongGCStats::SizeToIntKB(longGCStats->GetAfterGCTotalMemCommitted()),
        "AFTER_GC_ACTIVE_MEM_USED", LongGCStats::SizeToIntKB(longGCStats->GetAfterGCActiveMemUsed()),
        "AFTER_GC_ACTIVE_MEM_COMMITTED", LongGCStats::SizeToIntKB(longGCStats->GetAfterGCActiveMemCommitted()),
        "AFTER_GC_OLD_MEM_USED", LongGCStats::SizeToIntKB(longGCStats->GetAfterGCOldMemUsed()),
        "AFTER_GC_OLD_MEM_COMMITTED", LongGCStats::SizeToIntKB(longGCStats->GetAfterGCOldMemCommitted()),
        "AFTER_GC_HUGE_MEM_USED", LongGCStats::SizeToIntKB(longGCStats->GetAfterGCHugeMemUsed()),
        "AFTER_GC_HUGE_MEM_COMMITTED", LongGCStats::SizeToIntKB(longGCStats->GetAfterGCHugeMemCommitted()),
        "AFTER_GC_NATIVE_BINDING_SIZE", LongGCStats::SizeToIntKB(longGCStats->GetAfterGCNativeBindingSize()),
        "AFTER_GC_NATIVE_LIMIT", LongGCStats::SizeToIntKB(longGCStats->GetAfterGCNativeLimit()),
        "CPU_LOAD", longGCStats->GetCpuLoad());
    if (ret != 0) {
        LOG_GC(ERROR) << "GCKeyStats HiSysEventWrite ARK_GC_LONG_TIME Failed! ret = " << ret;
    }
#endif
}

bool DFXHiSysEvent::IsEnableDFXHiSysEvent()
{
    return true;
}

double DFXHiSysEvent::GetCpuUsage()
{
#ifdef ENABLE_UCOLLECTION
    auto collector = OHOS::HiviewDFX::UCollectClient::CpuCollector::Create();
    auto collectResult = collector->GetSysCpuUsage();
    if (collectResult.retCode == OHOS::HiviewDFX::UCollect::UcError::SUCCESS) {
        LOG_GC(DEBUG) << "GCKeyStats cpu usage: " << collectResult.data;
        return collectResult.data;
    }
    LOG_GC(ERROR) << "GCKeyStats get cpu usage failed, error code: " << collectResult.retCode;
    return -1.0;
#else
    return -1.0;
#endif
}
}  // namespace panda::ecmascript
