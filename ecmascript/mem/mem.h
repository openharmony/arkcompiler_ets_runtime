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

#ifndef ECMASCRIPT_MEM_MEM_H
#define ECMASCRIPT_MEM_MEM_H

#include <cstdint>

#include "ecmascript/base/math_helper.h"
#include "ecmascript/ecma_param_configuration.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/tagged_object.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage, bugprone-lambda-function-name)
#define LOG_ECMA_MEM(level) LOG_GC(level) << __func__ << ":" << __LINE__ << " "

namespace panda::ecmascript {

enum class MemAlignment : uint8_t {
    MEM_ALIGN_OBJECT = 8,
    MEM_ALIGN_REGION = 16,
};

enum class MemAlignmentLog2 : uint8_t {
    MEM_ALIGN_OBJECT_LOG2 = 3,
    MEM_ALIGN_REGION_LOG2 = 4,
};

static constexpr size_t INITIAL_REGULAR_OBJECT_CAPACITY = 1024_MB;
static constexpr size_t INITIAL_HUGE_OBJECT_CAPACITY = 1024_MB;
static constexpr size_t INITIAL_NONMOVALBE_OBJECT_CAPACITY = 1024_MB;
static constexpr size_t INCREMENT_HUGE_OBJECT_CAPACITY = 128_MB;
static constexpr size_t LARGE_POOL_SIZE = 480_MB;
static constexpr size_t MEDIUM_POOL_SIZE = 256_MB;
static constexpr size_t LOW_POOL_SIZE = 64_MB;
static constexpr size_t MIN_MEM_POOL_CAPACITY = 64_MB;
static constexpr size_t MAX_MEM_POOL_CAPACITY = 1536_MB;

#if defined(PANDA_TARGET_32)
    static constexpr size_t MAX_GLOBAL_NATIVE_LIMIT = 512_MB;
#else
    static constexpr size_t MAX_GLOBAL_NATIVE_LIMIT = 2048_MB;
#endif

static constexpr size_t MIN_OLD_SPACE_LIMIT = 2_MB;
static constexpr size_t MIN_BACKGROUNG_GC_LIMIT = 30_MB;

static constexpr size_t MAX_NONMOVABLE_LIVE_OBJ_SIZE = 60_MB;

static constexpr size_t REGION_SIZE_LOG2 = 18U;

static constexpr size_t MIN_HEAP_SIZE = 5_MB;

static constexpr size_t DEFAULT_REGION_SIZE = 1U << REGION_SIZE_LOG2;
static constexpr size_t DEFAULT_REGION_MASK = DEFAULT_REGION_SIZE - 1;

static constexpr size_t DEFAULT_MARK_STACK_SIZE = 4_KB;

static constexpr double MIN_OBJECT_SURVIVAL_RATE = 0.75;
static constexpr double STRICT_OBJECT_SURVIVAL_RATE = 0.9;
static constexpr double MIN_SENSITIVE_OBJECT_SURVIVAL_RATE = 0.9;
static constexpr double GROW_OBJECT_SURVIVAL_RATE = 0.8;
static constexpr double SHRINK_OBJECT_SURVIVAL_RATE = 0.2;
static constexpr double MIN_GC_INTERVAL_MS = 1000;
static constexpr double LOW_ALLOCATION_SPEED_PER_MS = 1000;
static constexpr double DEFAULT_CAPACITY_RATE = 0.6;
static constexpr double HPPGC_NEWSPACE_SIZE_RATIO = 0.5;
// Objects which are larger than half of the region size are huge objects.
// Regular objects will be allocated on regular regions and migrated on spaces.
// They will never be moved to huge object space. So we take half of a regular
// region as the border of regular objects.
static constexpr size_t MAX_32BIT_OBJECT_SPACE_SIZE = 1_GB;
static constexpr size_t MAX_REGULAR_HEAP_OBJECT_SIZE_FOR_CMC = 32_KB; // initialize from CMC-GC
static constexpr size_t MAX_REGULAR_HEAP_OBJECT_SIZE = DEFAULT_REGION_SIZE * 2 / 3;
inline size_t g_maxRegularHeapObjectSize = MAX_REGULAR_HEAP_OBJECT_SIZE;
inline uint64_t g_maxGarbageCacheSize = 16_MB;
// internal allocator
static constexpr size_t CHUNK_ALIGN_SIZE = 4_KB;
static constexpr size_t MIN_CHUNK_AREA_SIZE = 4_KB;
static constexpr size_t MAX_CACHED_CHUNK_AREA_SIZE = 16_KB;
static constexpr uint32_t WORKNODE_SPACE_SIZE = 8_KB;
static constexpr size_t MAX_CHUNK_AREA_SIZE = 1_MB;
static constexpr size_t MAX_REGEXP_CACHE_SIZE = 8_KB;
// taskpool
static constexpr double TRIGGER_OLDGC_OBJECT_LIMIT_RATE = 0.1;
static constexpr double TRIGGER_OLDGC_OBJECT_SIZE_LIMIT = 20_MB;
static constexpr double TRIGGER_OLDGC_NATIVE_LIMIT_RATE = 0.1;
static constexpr double TRIGGER_OLDGC_NATIVE_SIZE_LIMIT = 20_MB;
// idle gc
static constexpr size_t IDLE_GC_YOUNG_SPACE = 3_MB;

static constexpr double LOW_ALLOCATION_RATE_PER_MS = 10;
static constexpr double IDLE_PATIAL_GC_SPACE_SIZE_LIMIT_RATE = 0.65f;
static constexpr double IDLE_SPACE_SIZE_LIMIT_RATE = 0.8f;
static constexpr double IDLE_SPACE_SIZE_MIN_INC_RATIO = 1.1f;
static constexpr double IDLE_FRAGMENT_SIZE_RATIO = 0.1f;
static constexpr double IDLE_BINDING_NATIVE_MIN_INC_RATIO = 0.2f;
static constexpr size_t IDLE_SPACE_SIZE_MIN_INC_STEP = 5_MB;
static constexpr size_t IDLE_SPACE_SIZE_MIN_INC_STEP_FULL = 1_MB;
static constexpr size_t IDLE_MIN_EXPECT_RECLAIM_SIZE = 1_MB;
static constexpr size_t IDLE_BINDING_NATIVE_MIN_INC_SIZE = 10_MB;

// cold start
static constexpr uint64_t DEFAULT_STARTUP_DURATION_MS = 2000;
static constexpr uint64_t MIN_CONFIGURABLE_STARTUP_DURATION_MS = 2000;
static constexpr uint64_t MAX_CONFIGURABLE_STARTUP_DURATION_MS = 5000;
static constexpr uint64_t FINISH_STARTUP_TIMEPOINT_MS = 8000;
static constexpr double JUST_FINISH_STARTUP_LOCAL_THRESHOLD_RATIO = 0.25;
static constexpr double JUST_FINISH_STARTUP_SHARED_THRESHOLD_RATIO = 0.25;
static constexpr double JUST_FINISH_STARTUP_LOCAL_CONCURRENT_MARK_RATIO = 0.9;
static constexpr double JUST_FINISH_STARTUP_SHARED_CONCURRENT_MARK_RATIO = 0.75;

using TaggedType = uint64_t;
static constexpr uint32_t TAGGED_TYPE_SIZE = sizeof(TaggedType);
static constexpr uint32_t TAGGED_TYPE_SIZE_LOG = base::MathHelper::GetIntLog2(TAGGED_TYPE_SIZE);
constexpr size_t HEAD_SIZE = TaggedObject::TaggedObjectSize();

template<typename T>
constexpr inline bool IsAligned(T value, size_t alignment)
{
    return (value & (alignment - 1U)) == 0;
}

template<typename T>
inline T AlignDown(T x, size_t alignment)
{
    ASSERT(std::is_integral<T>::value);
    // alignment must be a power of two.
    ASSERT(alignment != 0 && ((alignment & (alignment - 1U)) == 0));
    return x & ~(alignment - 1U);
}

template<typename T>
inline T AlignUp(T x, size_t alignment)
{
    ASSERT(std::is_integral<T>::value && (x + alignment) > 0);
    return AlignDown<T>(static_cast<T>(x + alignment - 1U), alignment);
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_MEM_H
