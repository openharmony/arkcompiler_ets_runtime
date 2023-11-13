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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_FIXSHORTBRANCH_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_FIXSHORTBRANCH_H

#include <vector>
#include "aarch64_cg.h"
#include "optimize_common.h"
#include "mir_builder.h"

namespace maplebe {
class AArch64FixShortBranch {
public:
    explicit AArch64FixShortBranch(CGFunc *cf) : cgFunc(cf) {}
    ~AArch64FixShortBranch() = default;
    void FixShortBranches();

private:
    CGFunc *cgFunc;
    uint32 CalculateAlignRange(const BB &bb, uint32 addr) const;
    void SetInsnId() const;
}; /* class AArch64ShortBranch */

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgFixShortBranch, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_FIXSHORTBRANCH_H */
