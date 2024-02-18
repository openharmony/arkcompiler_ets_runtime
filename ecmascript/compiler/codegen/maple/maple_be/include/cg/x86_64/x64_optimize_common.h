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

#ifndef MAPLEBE_INCLUDE_CG_X64_X64_OPTIMIZE_COMMON_H
#define MAPLEBE_INCLUDE_CG_X64_X64_OPTIMIZE_COMMON_H

#include "x64_isa.h"
#include "optimize_common.h"

namespace maplebe {
using namespace maple;

class X64InsnVisitor : public InsnVisitor {
public:
    explicit X64InsnVisitor(CGFunc &func) : InsnVisitor(func) {}

    ~X64InsnVisitor() = default;

    void ModifyJumpTarget(LabelIdx targetLabel, BB &bb) override;
    void ModifyJumpTarget(Operand &targetOperand, BB &bb) override;
    void ModifyJumpTarget(BB &newTarget, BB &bb) override;
    /* Check if it requires to add extra gotos when relocate bb */
    Insn *CloneInsn(Insn &originalInsn) override;
    LabelIdx GetJumpLabel(const Insn &insn) const override;
    bool IsCompareInsn(const Insn &insn) const override;
    bool IsCompareAndBranchInsn(const Insn &insn) const override;
    bool IsTestAndSetCCInsn(const Insn &insn) const override;
    bool IsTestAndBranchInsn(const Insn &insn) const override;
    bool IsAddOrSubInsn(const Insn &insn) const override;
    bool IsSimpleJumpInsn(const Insn &insn) const override;
    RegOperand *CreateVregFromReg(const RegOperand &pReg) override;
    void ReTargetSuccBB(BB &bb, LabelIdx newTarget) const override;
    void FlipIfBB(BB &bb, LabelIdx ftLabel) const override;
    BB *CreateGotoBBAfterCondBB(BB &bb, BB &fallthru, bool isTargetFallthru) const override;
    void ModifyFathruBBToGotoBB(BB &bb, LabelIdx labelIdx) const override;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_X64_X64_OPTIMIZE_COMMON_H */
