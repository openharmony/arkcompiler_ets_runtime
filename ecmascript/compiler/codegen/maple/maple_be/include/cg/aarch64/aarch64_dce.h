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

#ifndef MAPLEBE_INCLUDE_AARCH64_DCE_H
#define MAPLEBE_INCLUDE_AARCH64_DCE_H

#include "cg_dce.h"
namespace maplebe {
class AArch64Dce : public CGDce {
public:
    AArch64Dce(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo) : CGDce(mp, f, sInfo) {}
    ~AArch64Dce() override = default;

private:
    bool RemoveUnuseDef(VRegVersion &defVersion) override;
};

class A64DeleteRegUseVisitor : public DeleteRegUseVisitor {
public:
    A64DeleteRegUseVisitor(CGSSAInfo &cgSSAInfo, uint32 dInsnID) : DeleteRegUseVisitor(cgSSAInfo, dInsnID) {}
    ~A64DeleteRegUseVisitor() override = default;

private:
    void Visit(RegOperand *v) final;
    void Visit(ListOperand *v) final;
    void Visit(MemOperand *v) final;
    void Visit(PhiOperand *v) final;
};
}  // namespace maplebe
#endif /* MAPLEBE_INCLUDE_AARCH64_DCE_H */
