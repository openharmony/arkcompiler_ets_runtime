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

#include "ecmascript/stackmap/llvm_stackmap_type.h"

namespace panda::ecmascript::kungfu {
struct ARKConst {
    OffsetType offset;
};

struct StackMapSecHead {
    uint32_t totalSize;
    uint32_t callsiteNum;
    uint32_t callsitStart;
    uint32_t callsitEnd;
};
struct CallsiteHead {
    uint32_t calliteOffset {0}; // relative text start addr
    uint32_t stackmapOffset {0}; // relative stackmap start addr
    uint32_t arkStackMapNum {0};
    uint32_t deoptOffset {0};
    uint32_t deoptNum {0};
};

/* <deoptId, kind, value> */
struct ARKDeopt {
    OffsetType Id; // deoptid
    LocationTy::Kind kind;
    std::variant<OffsetType, DwarfRegAndOffsetType, LargeInt> value;
};

struct ARKCallsite {
    CallsiteHead head;
    CallSiteInfo stackmaps;
    std::vector<kungfu::ARKDeopt> callsite2Deopt;
    bool operator < (const ARKCallsite & x) const
    {
        return head.calliteOffset < x.head.calliteOffset;
    }
    uint32_t CalHeadSize() const;
    uint32_t CalStackMapSize() const;
};

struct ARKCallsiteAOTFileInfo {
    StackMapSecHead secHead;
    std::vector<ARKCallsite> callsites;
};
using ArkStackMap = CallSiteInfo;
using CalleeRegAndOffsetVec = std::vector<DwarfRegAndOffsetType>;
/*
totalSize  callsiteNum callsitStart  callsitEnd
-----head1--------
-----head2--------
      |
      |
-----head[N]------

-----stackmap1----
-----stackmap2----
      |
      |
-----stackmap2[N]-

-----deopt1-----
-<deoptid, LocationTy::Kind, LocationTy
-          CONSTANT,  <OffsetType OffsetOrSmallConstant>
-          INDIRECT   <loc.DwarfRegNum, loc.OffsetOrSmallConstant>
-          CONSTANTNDEX <LargeInt>
*/
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_ARK_STACKMAP_H