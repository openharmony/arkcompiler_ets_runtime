/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ARK_STACKMAP_H
#define ECMASCRIPT_ARK_STACKMAP_H

#include "ecmascript/stackmap/llvm/llvm_stackmap_type.h"

namespace panda::ecmascript::kungfu {
struct ARKConst {
    LLVMStackMapType::IntType offset;
};

struct ArkStackMapHeader {
    uint32_t secSize;
    uint32_t callsiteNum;
};

#pragma pack(1)
struct CallsiteHeader {
    uint32_t calliteOffsetInTxtSec {0}; // relative text start addr
    uint32_t stackmapOffsetInSMSec{0}; // relative stackmap start addr
    uint32_t deoptOffset {0};
    uint16_t stackmapNum {0};
    uint16_t deoptNum {0};
};
#pragma pack ()

/* <vregId and kind, value> */
struct ARKDeopt {
    LLVMStackMapType::VRegId id; // deoptid
    LocationTy::Kind kind;
    std::variant<LLVMStackMapType::IntType, LLVMStackMapType::LargeInt, LLVMStackMapType::DwarfRegAndOffsetType> value;
};

struct ARKCallsite {
    CallsiteHeader head;
    LLVMStackMapType::CallSiteInfo stackmaps;
    std::vector<kungfu::ARKDeopt> callsite2Deopt;
    bool operator < (const ARKCallsite & x) const
    {
        return head.calliteOffsetInTxtSec < x.head.calliteOffsetInTxtSec;
    }
    uint32_t CalHeadSize() const;
    uint32_t CalStackMapSize(Triple triple) const;
};

struct ARKCallsiteAOTFileInfo {
    ArkStackMapHeader secHead;
    std::vector<ARKCallsite> callsites;
};
using ArkStackMap = LLVMStackMapType::CallSiteInfo;
using CalleeRegAndOffsetVec = std::vector<LLVMStackMapType::DwarfRegAndOffsetType>;
// * ArkStackMap layout as the following:
//               +-----------------------------------------+ ---------
//               |                 secSize                 |         ^
//               |-----------------------------------------|   ArkStackMapHeader
//               |               callsiteNum               |         v
//               +-----------------------------------------+ ---------
//               |           calliteOffsetInTxtSec         |         ^
//               |-----------------------------------------|         |
//               |            stackmapOffsetInSMSec        |         |
//               |-----------------------------------------|         |
//               |               deoptOffset               |   CallsiteHeader[0]
//               |-----------------------------------------|         |
//               |               stackmapNum               |         |
//               |-----------------------------------------|         |
//               |                deoptNum                 |         v
//               |-----------------------------------------| ---------
//               |                . . . . .                |
//               +-----------------------------------------+ ---------
//               |          dwarfRegAndOff(sleb128)        |     StackMaps[0]
//               |-----------------------------------------| ---------
//               |                . . . . .                |
//               +-----------------------------------------+ ---------
//               |          vregIdAndKind(sleb128)         |         ^
//               |-----------------------------------------|     Deopts[0]
//               |              value(sleb128)             |         v
//               |-----------------------------------------| ---------
//               |                . . . . .                |
//               +-----------------------------------------+ ---------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_ARK_STACKMAP_H