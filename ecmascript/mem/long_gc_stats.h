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

#ifndef ECMASCRIPT_MEM_LONG_GC_STATS_H
#define ECMASCRIPT_MEM_LONG_GC_STATS_H
#include <cstddef>
#include "ecmascript/mem/mem_common.h"

namespace panda::ecmascript {
class LongGCStats {
public:
    ~LongGCStats() = default;

    void Reset()
    {
        gcType_ = 0;
        gcReason_ = 0;
        markReason_ = 0;
        gcIsSensitive_ = false;
        gcIsInBackground_ = false;
        gcTotalTime_ = 0.0f;
        gcMarkTime_ = 0.0f;
        gcEvacuateTime_ = 0.0f;
        gcUpdateRootTime_ = 0.0f;
        gcUpdateWeekRefTime_ = 0.0f;
        gcUpdateReferenceTime_ = 0.0f;
        gcSweepNewToOldTime_ = 0.0f;
        gcFinalizeTime_ = 0.0f;
        gcInvokeCallbackTime_ = 0.0f;
        beforeGCTotalMemUsed_ = 0;
        beforeGCTotalMemCommitted_ = 0;
        beforeGCActiveMemUsed_ = 0;
        beforeGCActiveMemCommitted_ = 0;
        beforeGCOldMemUsed_ = 0;
        beforeGCOldMemCommitted_ = 0;
        beforeGCHugeMemUsed_ = 0;
        beforeGCHugeMemCommitted_ = 0;
        beforeGCNativeBindingSize_ = 0;
        beforeGCNativeLimit_ = 0;
        afterGCTotalMemUsed_ = 0;
        afterGCTotalMemCommitted_ = 0;
        afterGCActiveMemUsed_ = 0;
        afterGCActiveMemCommitted_ = 0;
        afterGCOldMemUsed_ = 0;
        afterGCOldMemCommitted_ = 0;
        afterGCHugeMemUsed_ = 0;
        afterGCHugeMemCommitted_ = 0;
        afterGCNativeBindingSize_ = 0;
        afterGCNativeLimit_ = 0;
        cpuLoad_ = 0.0;
    }
    static int SizeToIntKB(size_t size)
    {
        return static_cast<int>(size / 1_KB);
    }
    size_t GetGCType() const
    {
        return gcType_;
    }
    void SetGCType(size_t gcType)
    {
        gcType_ = gcType;
    }

    size_t GetGCReason() const
    {
        return gcReason_;
    }
    void SetGCReason(size_t gcReason)
    {
        gcReason_ = gcReason;
    }

    size_t GetMarkReason() const
    {
        return markReason_;
    }
    void SetMarkReason(size_t markReason)
    {
        markReason_ = markReason;
    }

    bool GetGCIsSensitive() const
    {
        return gcIsSensitive_;
    }
    void SetGCIsSensitive(bool gcIsSensitive)
    {
        gcIsSensitive_ = gcIsSensitive;
    }

    bool GetGCIsInBackground() const
    {
        return gcIsInBackground_;
    }
    void SetGCIsInBackground(bool gcIsInBackground)
    {
        gcIsInBackground_ = gcIsInBackground;
    }

    float GetGCTotalTime() const
    {
        return gcTotalTime_;
    }
    void SetGCTotalTime(float gcTotalTime)
    {
        gcTotalTime_ = gcTotalTime;
    }

    float GetGCMarkTime() const
    {
        return gcMarkTime_;
    }
    void SetGCMarkTime(float gcMarkTime)
    {
        gcMarkTime_ = gcMarkTime;
    }

    float GetGCEvacuateTime() const
    {
        return gcEvacuateTime_;
    }
    void SetGCEvacuateTime(float gcEvacuateTime)
    {
        gcEvacuateTime_ = gcEvacuateTime;
    }

    float GetGCUpdateRootTime() const
    {
        return gcUpdateRootTime_;
    }
    void SetGCUpdateRootTime(float gcUpdateRootTime)
    {
        gcUpdateRootTime_ = gcUpdateRootTime;
    }

    float GetGCUpdateWeekRefTime() const
    {
        return gcUpdateWeekRefTime_;
    }
    void SetGCUpdateWeekRefTime(float gcUpdateWeekRefTime)
    {
        gcUpdateWeekRefTime_ = gcUpdateWeekRefTime;
    }

    float GetGCUpdateReferenceTime() const
    {
        return gcUpdateReferenceTime_;
    }
    void SetGCUpdateReferenceTime(float gcUpdateReferenceTime)
    {
        gcUpdateReferenceTime_ = gcUpdateReferenceTime;
    }

    float GetGCSweepNewToOldTime() const
    {
        return gcSweepNewToOldTime_;
    }
    void SetGCSweepNewToOldTime(float gcSweepNewToOldTime)
    {
        gcSweepNewToOldTime_ = gcSweepNewToOldTime;
    }

    float GetGCFinalizeTime() const
    {
        return gcFinalizeTime_;
    }
    void SetGCFinalizeTime(float gcFinalizeTime)
    {
        gcFinalizeTime_ = gcFinalizeTime;
    }

    float GetGCInvokeCallbackTime() const
    {
        return gcInvokeCallbackTime_;
    }
    void SetGCInvokeCallbackTime(float gcInvokeCallbackTime)
    {
        gcInvokeCallbackTime_ = gcInvokeCallbackTime;
    }

    size_t GetBeforeGCTotalMemUsed() const
    {
        return beforeGCTotalMemUsed_;
    }
    void SetBeforeGCTotalMemUsed(size_t beforeGCTotalMemUsed)
    {
        beforeGCTotalMemUsed_ = beforeGCTotalMemUsed;
    }

    size_t GetBeforeGCTotalMemCommitted() const
    {
        return beforeGCTotalMemCommitted_;
    }
    void SetBeforeGCTotalMemCommitted(size_t beforeGCTotalMemCommitted)
    {
        beforeGCTotalMemCommitted_ = beforeGCTotalMemCommitted;
    }

    size_t GetBeforeGCActiveMemUsed() const
    {
        return beforeGCActiveMemUsed_;
    }
    void SetBeforeGCActiveMemUsed(size_t beforeGCActiveMemUsed)
    {
        beforeGCActiveMemUsed_ = beforeGCActiveMemUsed;
    }

    size_t GetBeforeGCActiveMemCommitted() const
    {
        return beforeGCActiveMemCommitted_;
    }
    void SetBeforeGCActiveMemCommitted(size_t beforeGCActiveMemCommitted)
    {
        beforeGCActiveMemCommitted_ = beforeGCActiveMemCommitted;
    }

    size_t GetBeforeGCOldMemUsed() const
    {
        return beforeGCOldMemUsed_;
    }
    void SetBeforeGCOldMemUsed(size_t beforeGCOldMemUsed)
    {
        beforeGCOldMemUsed_ = beforeGCOldMemUsed;
    }

    size_t GetBeforeGCOldMemCommitted() const
    {
        return beforeGCOldMemCommitted_;
    }
    void SetBeforeGCOldMemCommitted(size_t beforeGCOldMemCommitted)
    {
        beforeGCOldMemCommitted_ = beforeGCOldMemCommitted;
    }

    size_t GetBeforeGCHugeMemUsed() const
    {
        return beforeGCHugeMemUsed_;
    }
    void SetBeforeGCHugeMemUsed(size_t beforeGCHugeMemUsed)
    {
        beforeGCHugeMemUsed_ = beforeGCHugeMemUsed;
    }

    size_t GetBeforeGCHugeMemCommitted() const
    {
        return beforeGCHugeMemCommitted_;
    }
    void SetBeforeGCHugeMemCommitted(size_t beforeGCHugeMemCommitted)
    {
        beforeGCHugeMemCommitted_ = beforeGCHugeMemCommitted;
    }

    size_t GetBeforeGCNativeBindingSize() const
    {
        return beforeGCNativeBindingSize_;
    }
    void SetBeforeGCNativeBindingSize(size_t beforeGCNativeBindingSize)
    {
        beforeGCNativeBindingSize_ = beforeGCNativeBindingSize;
    }

    size_t GetBeforeGCNativeLimit() const
    {
        return beforeGCNativeLimit_;
    }
    void SetBeforeGCNativeLimit(size_t beforeGCNativeLimit)
    {
        beforeGCNativeLimit_ = beforeGCNativeLimit;
    }

    size_t GetAfterGCTotalMemUsed() const
    {
        return afterGCTotalMemUsed_;
    }
    void SetAfterGCTotalMemUsed(size_t afterGCTotalMemUsed)
    {
        afterGCTotalMemUsed_ = afterGCTotalMemUsed;
    }

    size_t GetAfterGCTotalMemCommitted() const
    {
        return afterGCTotalMemCommitted_;
    }
    void SetAfterGCTotalMemCommitted(size_t afterGCTotalMemCommitted)
    {
        afterGCTotalMemCommitted_ = afterGCTotalMemCommitted;
    }

    size_t GetAfterGCActiveMemUsed() const
    {
        return afterGCActiveMemUsed_;
    }
    void SetAfterGCActiveMemUsed(size_t afterGCActiveMemUsed)
    {
        afterGCActiveMemUsed_ = afterGCActiveMemUsed;
    }

    size_t GetAfterGCActiveMemCommitted() const
    {
        return afterGCActiveMemCommitted_;
    }
    void SetAfterGCActiveMemCommitted(size_t afterGCActiveMemCommitted)
    {
        afterGCActiveMemCommitted_ = afterGCActiveMemCommitted;
    }

    size_t GetAfterGCOldMemUsed() const
    {
        return afterGCOldMemUsed_;
    }
    void SetAfterGCOldMemUsed(size_t afterGCOldMemUsed)
    {
        afterGCOldMemUsed_ = afterGCOldMemUsed;
    }

    size_t GetAfterGCOldMemCommitted() const
    {
        return afterGCOldMemCommitted_;
    }
    void SetAfterGCOldMemCommitted(size_t afterGCOldMemCommitted)
    {
        afterGCOldMemCommitted_ = afterGCOldMemCommitted;
    }

    size_t GetAfterGCHugeMemUsed() const
    {
        return afterGCHugeMemUsed_;
    }
    void SetAfterGCHugeMemUsed(size_t afterGCHugeMemUsed)
    {
        afterGCHugeMemUsed_ = afterGCHugeMemUsed;
    }

    size_t GetAfterGCHugeMemCommitted() const
    {
        return afterGCHugeMemCommitted_;
    }
    void SetAfterGCHugeMemCommitted(size_t afterGCHugeMemCommitted)
    {
        afterGCHugeMemCommitted_ = afterGCHugeMemCommitted;
    }

    size_t GetAfterGCNativeBindingSize() const
    {
        return afterGCNativeBindingSize_;
    }
    void SetAfterGCNativeBindingSize(size_t afterGCNativeBindingSize)
    {
        afterGCNativeBindingSize_ = afterGCNativeBindingSize;
    }

    size_t GetAfterGCNativeLimit() const
    {
        return afterGCNativeLimit_;
    }
    void SetAfterGCNativeLimit(size_t afterGCNativeLimit)
    {
        afterGCNativeLimit_ = afterGCNativeLimit;
    }

    double GetCpuLoad() const
    {
        return cpuLoad_;
    }
    void SetCpuLoad(double cpuLoad)
    {
        cpuLoad_ = cpuLoad;
    }

private:
    size_t gcType_ = 0;
    size_t gcReason_ = 0;
    size_t markReason_ = 0;
    bool gcIsSensitive_ = false;
    bool gcIsInBackground_ = false;
    float gcTotalTime_ = 0.0f;
    float gcMarkTime_ = 0.0f;
    float gcEvacuateTime_ = 0.0f;
    float gcUpdateRootTime_ = 0.0f;
    float gcUpdateWeekRefTime_ = 0.0f;
    float gcUpdateReferenceTime_ = 0.0f;
    float gcSweepNewToOldTime_ = 0.0f;
    float gcFinalizeTime_ = 0.0f;
    float gcInvokeCallbackTime_ = 0.0f;
    size_t beforeGCTotalMemUsed_ = 0;
    size_t beforeGCTotalMemCommitted_ = 0;
    size_t beforeGCActiveMemUsed_ = 0;
    size_t beforeGCActiveMemCommitted_ = 0;
    size_t beforeGCOldMemUsed_ = 0;
    size_t beforeGCOldMemCommitted_ = 0;
    size_t beforeGCHugeMemUsed_ = 0;
    size_t beforeGCHugeMemCommitted_ = 0;
    size_t beforeGCNativeBindingSize_ = 0;
    size_t beforeGCNativeLimit_ = 0;
    size_t afterGCTotalMemUsed_ = 0;
    size_t afterGCTotalMemCommitted_ = 0;
    size_t afterGCActiveMemUsed_ = 0;
    size_t afterGCActiveMemCommitted_ = 0;
    size_t afterGCOldMemUsed_ = 0;
    size_t afterGCOldMemCommitted_ = 0;
    size_t afterGCHugeMemUsed_ = 0;
    size_t afterGCHugeMemCommitted_ = 0;
    size_t afterGCNativeBindingSize_ = 0;
    size_t afterGCNativeLimit_ = 0;
    double cpuLoad_ = 0.0;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_LONG_GC_STATS_H