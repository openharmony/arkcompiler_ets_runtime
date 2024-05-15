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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_DATA_DEP_BASE_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_DATA_DEP_BASE_H

#include "aarch64_operand.h"
#include "cgfunc.h"
#include "data_dep_base.h"

namespace maplebe {
class AArch64DataDepBase : public DataDepBase {
public:
    AArch64DataDepBase(MemPool &mp, CGFunc &func, MAD &mad, bool isIntraAna) : DataDepBase(mp, func, mad, isIntraAna) {}
    ~AArch64DataDepBase() override = default;

    void InitCDGNodeDataInfo(MemPool &mp, MapleAllocator &alloc, CDGNode &cdgNode) override;
    bool IsFrameReg(const RegOperand &opnd) const override;
    Insn *GetMemBaseDefInsn(const Insn &memInsn) const;

    void BuildDepsForMemDefCommon(Insn &insn, CDGNode &cdgNode);
    void BuildDepsForMemUseCommon(Insn &insn, CDGNode &cdgNode);
    void BuildDepsAccessStImmMem(Insn &insn) override;
    void BuildCallerSavedDeps(Insn &insn) override;
    void BuildDepsDirtyStack(Insn &insn) override;
    void BuildDepsUseStack(Insn &insn) override;
    void BuildDepsDirtyHeap(Insn &insn) override;
    void BuildDepsMemBar(Insn &insn) override;
    void BuildDepsUseMem(Insn &insn, MemOperand &memOpnd) override;
    void BuildDepsDefMem(Insn &insn, MemOperand &memOpnd) override;
    void BuildMemOpndDependency(Insn &insn, Operand &opnd, const OpndDesc &regProp);

    void BuildOpndDependency(Insn &insn) override;
    void BuildSpecialInsnDependency(Insn &insn, const MapleVector<DepNode *> &nodes) override;
    void BuildSpecialCallDeps(Insn &insn) override;
    void BuildAsmInsnDependency(Insn &insn) override;

    void BuildInterBlockMemDefUseDependency(DepNode &depNode, bool isMemDef) override;
    void BuildPredPathMemDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode) override;
    void BuildPredPathMemUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode) override;

    void DumpNodeStyleInDot(std::ofstream &file, DepNode &depNode) override;
};
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_DATA_DEP_BASE_H
