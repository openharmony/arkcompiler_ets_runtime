/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef MAPLEBE_INCLUDE_TARGET_SELECT_H
#define MAPLEBE_INCLUDE_TARGET_SELECT_H
#include "mempool.h"
#include "mempool_allocator.h"
 
extern "C" {
  // Declare all of the target-initialization functions that are available.
#define MAPLE_TARGET(TargetName) void MAPLEInitialize##TargetName##TargetInfo(maple::MemPool * m)
MAPLE_TARGET(AArch64);
MAPLE_TARGET(X64);
#undef MAPLE_TARGET

#define MAPLE_TARGET(TargetName) void MAPLEInitialize##TargetName##Target()
MAPLE_TARGET(AArch64);
MAPLE_TARGET(X64);
#undef MAPLE_TARGET
}

static inline void InitializeAllTargetInfos(maple::MemPool * m)
{
    #define MAPLE_TARGET(TargetName) MAPLEInitialize##TargetName##TargetInfo(m)
    MAPLE_TARGET(AArch64);
    MAPLE_TARGET(X64);
    #undef MAPLE_TARGET
}

static inline void InitializeAllTargets(void)
{
    #define MAPLE_TARGET(TargetName) MAPLEInitialize##TargetName##Target()
    MAPLE_TARGET(AArch64);
    MAPLE_TARGET(X64);
    #undef MAPLE_TARGET
}
#endif
