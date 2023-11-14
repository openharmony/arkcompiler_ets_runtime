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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OFFSET_ADJUST_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OFFSET_ADJUST_H

#include "offset_adjust.h"
#include "aarch64_cgfunc.h"

namespace maplebe {
using namespace maple;

class AArch64FPLROffsetAdjustment : public FrameFinalize {
public:
    explicit AArch64FPLROffsetAdjustment(CGFunc &func) : FrameFinalize(func) {}

    ~AArch64FPLROffsetAdjustment() override = default;

    void Run() override;

private:
    void AdjustmentOffsetForOpnd(Insn &insn, AArch64CGFunc &aarchCGFunc);
    void AdjustmentOffsetForImmOpnd(Insn &insn, uint32 index, AArch64CGFunc &aarchCGFunc) const;
    void AdjustmentOffsetForFPLR();
    /* frame pointer(x29) is available as a general-purpose register if useFP is set as false */
    void AdjustmentStackPointer(Insn &insn, AArch64CGFunc &aarchCGFunc);
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OFFSET_ADJUST_H */