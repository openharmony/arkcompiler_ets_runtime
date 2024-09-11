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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPTIMIZE_COMMON_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPTIMIZE_COMMON_H

#include "aarch64_isa.h"
#include "optimize_common.h"

namespace maplebe {
using namespace maple;

class AArch64InsnVisitor : public InsnVisitor {
public:
    explicit AArch64InsnVisitor(CGFunc &func) : InsnVisitor(func) {}

    ~AArch64InsnVisitor() override = default;

    void ModifyJumpTarget(maple::LabelIdx targetLabel, BB &bb) override;
    void ModifyJumpTarget(Operand &targetOperand, BB &bb) override;
    LabelIdx GetJumpLabel(const Insn &insn) const override;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPTIMIZE_COMMON_H */
