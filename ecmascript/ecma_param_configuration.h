/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ECMA_PARAM_CONFIGURATION_H
#define ECMASCRIPT_ECMA_PARAM_CONFIGURATION_H

#include "ecmascript/mem/mem_common.h"

#include "ecmascript/log_wrapper.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript {
static constexpr size_t DEFAULT_HEAP_SIZE = 448_MB;                 // Recommended range: 128-448MB
static constexpr size_t MAX_HEAP_SIZE = 1024_MB;
static constexpr size_t DEFAULT_WORKER_HEAP_SIZE = 768_MB;          // Recommended range: 128_MB, LargeHeap: 768_MB
static constexpr size_t MAX_WORKER_HEAP_SIZE = 1024_MB;
static constexpr size_t DEFAULT_SHARED_HEAP_SIZE = 778_MB;
static constexpr size_t MAX_SHARED_HEAP_SIZE = 2048_MB;
#ifdef PANDA_TARGET_64
static constexpr size_t LARGE_HEAP_POOL_SIZE = 4096_MB;
#else
static constexpr size_t LARGE_HEAP_POOL_SIZE = 1536_MB;
#endif

class EcmaParamConfiguration {
public:
    enum class HeapType : uint8_t {
        DEFAULT_HEAP,
        WORKER_HEAP,
        SHARED_HEAP
    };

    EcmaParamConfiguration() = default;
    EcmaParamConfiguration(HeapType heapType, size_t poolSize, size_t heapSize = 1_MB)
    {
        switch (heapType) {
            case HeapType::WORKER_HEAP:
                if (heapSize >= LOW_MEMORY && heapSize < MAX_WORKER_HEAP_SIZE) {
                    maxHeapSize_ = heapSize;
                } else if (heapSize >= MAX_WORKER_HEAP_SIZE) {
                    maxHeapSize_ = MAX_WORKER_HEAP_SIZE;
                } else {
                    maxHeapSize_ = DEFAULT_WORKER_HEAP_SIZE;
                }
                break;
            case HeapType::SHARED_HEAP:
                 if (heapSize >= LOW_MEMORY && heapSize < MAX_SHARED_HEAP_SIZE) {
                    maxHeapSize_ = heapSize;
                } else if (heapSize >= MAX_SHARED_HEAP_SIZE) {
                    maxHeapSize_ = MAX_SHARED_HEAP_SIZE;
                } else {
                    maxHeapSize_ = DEFAULT_SHARED_HEAP_SIZE;
                };
                break;
            default:
                if (poolSize >= DEFAULT_HEAP_SIZE) {
                    maxHeapSize_ = DEFAULT_HEAP_SIZE;
                } else {
                    maxHeapSize_ = poolSize; // pool is too small, no memory left for worker
                }
                if (heapSize >= LOW_MEMORY && heapSize <= MAX_HEAP_SIZE) {
                    maxHeapSize_ = heapSize;
                } else if (heapSize > MAX_HEAP_SIZE) {
                    maxHeapSize_ = MAX_HEAP_SIZE;
                }
        }
        Initialize();
    }

    void Initialize()
    {
        if (maxHeapSize_ < LOW_MEMORY) {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
        if (maxHeapSize_ < MEDIUM_MEMORY) { // 64_MB ~ 128_MB
            minSemiSpaceSize_ = 2_MB;
            maxSemiSpaceSize_ = 4_MB;
            defaultReadOnlySpaceSize_ = 256_KB;
            defaultNonMovableSpaceSize_ = 6_MB;
            defaultSnapshotSpaceSize_ = 512_KB;
            defaultMachineCodeSpaceSize_ = 2_MB;
            defaultGlobalAllocLimit_ = 20_MB;
            semiSpaceTriggerConcurrentMark_ = 1_MB;
            semiSpaceStepOvershootSize_ = 2_MB;
            oldSpaceStepOvershootSize_ = 4_MB;
            oldSpaceMaxOvershootSize_ = 8_MB;
            outOfMemoryOvershootSize_ = 2_MB;
            minAllocLimitGrowingStep_ = 2_MB;
            minNativeLimitGrowingStep_ = 16_MB;
            minGrowingStep_ = 4_MB;
            maxStackSize_ = 64_KB;
            maxJSSerializerSize_ = 8_MB;
            sharedHeapLimitGrowingFactor_ = 2; // 2: growing factor
            sharedHeapLimitGrowingStep_ = 20_MB;
            incObjSizeThresholdInSensitive_ = 40_MB;
            stepNativeSizeInc_ = 256_MB;
            nativeSizeOvershoot_ = 80_MB;
            maxNativeSizeInc_ = 768_MB;
            fragmentationLimitForSharedFullGC_ = 5_MB;
        } else if (maxHeapSize_ < HIGH_MEMORY) { // 128_MB ~ 256_MB
            minSemiSpaceSize_ = 2_MB;
            maxSemiSpaceSize_ = 8_MB;
            defaultReadOnlySpaceSize_ = 256_KB;
            defaultNonMovableSpaceSize_ = 6_MB;
            defaultSnapshotSpaceSize_ = 512_KB;
            defaultMachineCodeSpaceSize_ = 2_MB;
            defaultGlobalAllocLimit_ = 20_MB;
            semiSpaceTriggerConcurrentMark_ = 1.5_MB;
            semiSpaceStepOvershootSize_ = 2_MB;
            oldSpaceStepOvershootSize_ = 8_MB;
            oldSpaceMaxOvershootSize_ = 16_MB;
            outOfMemoryOvershootSize_ = 2_MB;
            minAllocLimitGrowingStep_ = 4_MB;
            minNativeLimitGrowingStep_ = 32_MB;
            minGrowingStep_ = 8_MB;
            maxStackSize_ = 128_KB;
            maxJSSerializerSize_ = 16_MB;
            sharedHeapLimitGrowingFactor_ = 2; // 2: growing factor
            sharedHeapLimitGrowingStep_ = 40_MB;
            incObjSizeThresholdInSensitive_ = 40_MB;
            stepNativeSizeInc_ = 256_MB;
            nativeSizeOvershoot_ = 80_MB;
            maxNativeSizeInc_ = 768_MB;
            fragmentationLimitForSharedFullGC_ = 5_MB;
        }  else { // 256_MB ~ 384_MB
            minSemiSpaceSize_ = 2_MB;
            maxSemiSpaceSize_ = 16_MB;
            defaultReadOnlySpaceSize_ = 256_KB;
            defaultNonMovableSpaceSize_ = 64_MB;
            defaultSnapshotSpaceSize_ = 4_MB;
            defaultMachineCodeSpaceSize_ = 8_MB;
            defaultGlobalAllocLimit_ = 20_MB;
            semiSpaceTriggerConcurrentMark_ = 1.5_MB;
            semiSpaceStepOvershootSize_ = 2_MB;
            oldSpaceStepOvershootSize_ = 8_MB;
            oldSpaceMaxOvershootSize_ = 16_MB;
            outOfMemoryOvershootSize_ = 2_MB;
            minAllocLimitGrowingStep_ = 8_MB;
            minNativeLimitGrowingStep_ = 64_MB;
            minGrowingStep_ = 16_MB;
            maxStackSize_ = 128_KB;
            maxJSSerializerSize_ = 16_MB;
            sharedHeapLimitGrowingFactor_ = 4; // 4: growing factor
            sharedHeapLimitGrowingStep_ = 80_MB;
            incObjSizeThresholdInSensitive_ = 80_MB;
            stepNativeSizeInc_ = 300_MB;
            nativeSizeOvershoot_ = 100_MB;
            asyncClearNativePointerThreshold_ = 500_MB;
            maxNativeSizeInc_ = 1_GB;
            fragmentationLimitForSharedFullGC_ = 5_MB;
        }
    }

    size_t GetMaxHeapSize() const
    {
        return maxHeapSize_;
    }

    size_t GetMinSemiSpaceSize() const
    {
        return minSemiSpaceSize_;
    }

    size_t GetMaxSemiSpaceSize() const
    {
        return maxSemiSpaceSize_;
    }

    size_t GetDefaultReadOnlySpaceSize() const
    {
        return defaultReadOnlySpaceSize_;
    }

    size_t GetDefaultNonMovableSpaceSize() const
    {
        return defaultNonMovableSpaceSize_;
    }

    size_t GetDefaultSnapshotSpaceSize() const
    {
        return defaultSnapshotSpaceSize_;
    }

    size_t GetDefaultMachineCodeSpaceSize() const
    {
        return defaultMachineCodeSpaceSize_;
    }

    size_t GetDefaultGlobalAllocLimit() const
    {
        return defaultGlobalAllocLimit_;
    }

    size_t GetSemiSpaceTriggerConcurrentMark() const
    {
        return semiSpaceTriggerConcurrentMark_;
    }

    size_t GetSemiSpaceStepOvershootSize() const
    {
        return semiSpaceStepOvershootSize_;
    }

    size_t GetOldSpaceStepOvershootSize() const
    {
        return oldSpaceStepOvershootSize_;
    }

    size_t GetOldSpaceMaxOvershootSize() const
    {
        return oldSpaceMaxOvershootSize_;
    }

    size_t GetOutOfMemoryOvershootSize() const
    {
        return outOfMemoryOvershootSize_;
    }

    size_t GetMinAllocLimitGrowingStep() const
    {
        return minAllocLimitGrowingStep_;
    }

    size_t GetMinNativeLimitGrowingStep() const
    {
        return minNativeLimitGrowingStep_;
    }

    size_t GetMinGrowingStep() const
    {
        return minGrowingStep_;
    }

    size_t GetSharedHeapLimitGrowingFactor() const
    {
        return sharedHeapLimitGrowingFactor_;
    }

    size_t GetSharedHeapLimitGrowingStep() const
    {
        return sharedHeapLimitGrowingStep_;
    }

    size_t GetIncObjSizeThresholdInSensitive() const
    {
        return incObjSizeThresholdInSensitive_;
    }

    size_t GetStepNativeSizeInc() const
    {
        return stepNativeSizeInc_;
    }

    size_t GetNativeSizeOvershoot() const
    {
        return nativeSizeOvershoot_;
    }

    size_t GetAsyncClearNativePointerThreshold() const
    {
        return asyncClearNativePointerThreshold_;
    }

    size_t GetMaxNativeSizeInc() const
    {
        return maxNativeSizeInc_;
    }

    size_t GetFragmentationLimitForSharedFullGC() const
    {
        return fragmentationLimitForSharedFullGC_;
    }

    uint32_t GetMaxStackSize() const
    {
        return maxStackSize_;
    }

    static size_t GetDefalutStackSize()
    {
        return DEFAULT_STACK_SIZE;
    }

    static size_t GetDefaultReservedStackSize()
    {
        return DEFAULT_RESERVED_STACK_SIZE;
    }

    static size_t GetAllowedUpperStackDiff()
    {
        return ALLOWED_UPPER_STACK_DIFF;
    }

    size_t GetMaxJSSerializerSize() const
    {
        return maxJSSerializerSize_;
    }

    void SetSharedHeapLimitGrowingFactor(size_t value)
    {
        sharedHeapLimitGrowingFactor_ = value;
    }

    void SetSharedHeapLimitGrowingStep(size_t value)
    {
        sharedHeapLimitGrowingStep_ = value;
    }

    void SetIncObjSizeThresholdInSensitive(size_t value)
    {
        incObjSizeThresholdInSensitive_ = value;
    }

    void SetStepNativeSizeInc(size_t value)
    {
        stepNativeSizeInc_ = value;
    }

    void SetDefaultGlobalAllocLimit(size_t value)
    {
        defaultGlobalAllocLimit_ = value;
    }

private:
    static constexpr size_t LOW_MEMORY = 64_MB;
    static constexpr size_t MEDIUM_MEMORY = 128_MB;
    static constexpr size_t HIGH_MEMORY = 256_MB;
    static constexpr size_t DEFAULT_STACK_SIZE = 992_KB;
    static constexpr size_t ALLOWED_UPPER_STACK_DIFF = 4_KB;
    static constexpr size_t DEFAULT_RESERVED_STACK_SIZE = 16_KB;

    size_t maxHeapSize_ {0};
    size_t minSemiSpaceSize_ {0};
    size_t maxSemiSpaceSize_ {0};
    size_t defaultReadOnlySpaceSize_ {0};
    size_t defaultNonMovableSpaceSize_ {0};
    size_t defaultSnapshotSpaceSize_ {0};
    size_t defaultMachineCodeSpaceSize_ {0};
    size_t defaultGlobalAllocLimit_ {0};
    size_t semiSpaceTriggerConcurrentMark_ {0};
    size_t semiSpaceStepOvershootSize_ {0};
    size_t oldSpaceStepOvershootSize_ {0};
    size_t oldSpaceMaxOvershootSize_ {0};
    size_t outOfMemoryOvershootSize_ {0};
    size_t minAllocLimitGrowingStep_ {0};
    size_t minNativeLimitGrowingStep_ {0};
    size_t minGrowingStep_ {0};
    size_t sharedHeapLimitGrowingFactor_ {0};
    size_t sharedHeapLimitGrowingStep_ {0};
    size_t incObjSizeThresholdInSensitive_ {0};
    size_t stepNativeSizeInc_ {0};
    size_t nativeSizeOvershoot_ {0};
    size_t asyncClearNativePointerThreshold_ {0};
    size_t maxNativeSizeInc_ {0};
    size_t maxJSSerializerSize_ {0};
    size_t fragmentationLimitForSharedFullGC_ {0};
    uint32_t maxStackSize_ {0};
};
} // namespace panda::ecmascript

#endif // ECMASCRIPT_ECMA_PARAM_CONFIGURATION_H
