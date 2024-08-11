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

#ifndef MPL2MPL_INCLUDE_MUID_REPLACEMENT_H
#define MPL2MPL_INCLUDE_MUID_REPLACEMENT_H
#include "phase_impl.h"
#include "version.h"
#include "maple_phase_manager.h"

namespace maple {
// For func def table.
constexpr uint32 kFuncDefAddrIndex = 0;
// For data def table.
constexpr uint32 kDataDefAddrIndex = 0;
// For func def info. table.
constexpr uint32 kFuncDefSizeIndex = 0;
constexpr uint32 kFuncDefNameIndex = 1;
constexpr uint32 kRangeBeginIndex = 0;
constexpr int32_t kDecoupleAndLazy = 3;
constexpr uint32_t kShiftBit16 = 16;
constexpr uint32_t kShiftBit15 = 15;

enum RangeIdx {
    // 0,1 entry is reserved for a stamp
    kVtabAndItab = 2,
    kItabConflict = 3,
    kVtabOffset = 4,
    kFieldOffset = 5,
    kValueOffset = 6,
    kLocalClassInfo = 7,
    kConststr = 8,
    kSuperclass = 9,
    kGlobalRootlist = 10,
    kClassmetaData = 11,
    kClassBucket = 12,
    kOldMaxNum = 16,  // Old num
    kDataSection = 17,
    kDecoupleStaticKey = 18,
    kDecoupleStaticValue = 19,
    kBssStart = 20,
    kLinkerSoHash = 21,
    kArrayClassCache = 22,
    kArrayClassCacheName = 23,
    kNewMaxNum = 24  // New num
};

struct SourceFileMethod {
    uint32 sourceFileIndex;
    uint32 sourceClassIndex;
    uint32 sourceMethodIndex;
    bool isVirtual;
};

struct SourceFileField {
    uint32 sourceFileIndex;
    uint32 sourceClassIndex;
    uint32 sourceFieldIndex;
};

}  // namespace maple
#endif  // MPL2MPL_INCLUDE_MUID_REPLACEMENT_H
