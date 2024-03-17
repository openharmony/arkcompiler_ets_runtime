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
#include "aarch64_cg.h"
#ifndef ONLY_C
#include "pressure.h"
#endif
#include "cg_irbuilder.h"
#include "aarch64_mem_reference.h"
#include "aarch64_data_dep_base.h"

namespace maplebe {
/** It is a yield point if loading from a dedicated
 *  register holding polling page address:
 *  ldr  wzr, [RYP]
 */
static bool IsYieldPoint(const Insn &insn)
{
    if (insn.IsLoad() && !insn.IsLoadLabel()) {
        auto mem = static_cast<MemOperand *>(insn.GetMemOpnd());
        return (mem != nullptr && mem->GetBaseRegister() != nullptr &&
                mem->GetBaseRegister()->GetRegisterNumber() == RYP);
    }
    return false;
}

static bool IsLazyLoad(MOperator op)
{
    return (op == MOP_lazy_ldr) || (op == MOP_lazy_ldr_static) || (op == MOP_lazy_tail);
}

bool AArch64DataDepBase::IsFrameReg(const RegOperand &opnd) const
{
    return (opnd.GetRegisterNumber() == RFP) || (opnd.GetRegisterNumber() == RSP);
}

void AArch64DataDepBase::InitCDGNodeDataInfo(MemPool &mp, MapleAllocator &alloc, CDGNode &cdgNode)
{
    uint32 maxRegNum = (cgFunc.IsAfterRegAlloc() ? AArch64reg::kAllRegNum : cgFunc.GetMaxVReg());
    cdgNode.InitDataDepInfo(mp, alloc, maxRegNum);
}

Insn *AArch64DataDepBase::GetMemBaseDefInsn(const Insn &memInsn) const
{
    auto *memOpnd = static_cast<MemOperand *>(memInsn.GetMemOpnd());
    if (memOpnd == nullptr) {
        return nullptr;
    }
    RegOperand *baseOpnd = memOpnd->GetBaseRegister();
    if (baseOpnd == nullptr) {
        return nullptr;
    }
    return curCDGNode->GetLatestDefInsn(baseOpnd->GetRegisterNumber());
}

void AArch64DataDepBase::BuildDepsForMemDefCommon(Insn &insn, CDGNode &cdgNode)
{
    // Stack memory
    // Build anti dependency
    MapleVector<Insn *> &stackUses = cdgNode.GetStackUseInsns();
    for (auto *stackUse : stackUses) {
        // the insn may be stack memory or heap use memory
        if (AArch64MemReference::NeedBuildMemoryDependency(*stackUse, insn, GetMemBaseDefInsn(*stackUse),
                                                           GetMemBaseDefInsn(insn), kDependenceTypeAnti)) {
            AddDependence(*stackUse->GetDepNode(), *insn.GetDepNode(), kDependenceTypeAnti);
        }
    }
    // Build output dependency
    MapleVector<Insn *> &stackDefs = cdgNode.GetStackDefInsns();
    for (auto *stackDef : stackDefs) {
        // the insn may be stack memory or heap use memory
        if (AArch64MemReference::NeedBuildMemoryDependency(*stackDef, insn, GetMemBaseDefInsn(*stackDef),
                                                           GetMemBaseDefInsn(insn), kDependenceTypeOutput)) {
            AddDependence(*stackDef->GetDepNode(), *insn.GetDepNode(), kDependenceTypeOutput);
        }
    }
    // Heap memory
    // Build anti dependency
    MapleVector<Insn *> &heapUses = cdgNode.GetHeapUseInsns();
    for (auto *heapUse : heapUses) {
        if (AArch64MemReference::NeedBuildMemoryDependency(*heapUse, insn, GetMemBaseDefInsn(*heapUse),
                                                           GetMemBaseDefInsn(insn), kDependenceTypeAnti)) {
            AddDependence(*heapUse->GetDepNode(), *insn.GetDepNode(), kDependenceTypeAnti);
        }
    }
    // Build output dependency
    MapleVector<Insn *> &heapDefs = cdgNode.GetHeapDefInsns();
    for (auto *heapDef : heapDefs) {
        if (AArch64MemReference::NeedBuildMemoryDependency(*heapDef, insn, GetMemBaseDefInsn(*heapDef),
                                                           GetMemBaseDefInsn(insn), kDependenceTypeOutput)) {
            AddDependence(*heapDef->GetDepNode(), *insn.GetDepNode(), kDependenceTypeOutput);
        }
    }
}

void AArch64DataDepBase::BuildDepsForMemUseCommon(Insn &insn, CDGNode &cdgNode)
{
    // Build dependency for stack memory access
    MapleVector<Insn *> &stackDefs = cdgNode.GetStackDefInsns();
    for (auto *stackDef : stackDefs) {
        // The insn may be stack memory or heap memory
        if ((stackDef->IsCall() && stackDef->GetMachineOpcode() != MOP_tls_desc_call) ||
            AArch64MemReference::NeedBuildMemoryDependency(*stackDef, insn, GetMemBaseDefInsn(*stackDef),
                                                           GetMemBaseDefInsn(insn), kDependenceTypeTrue)) {
            AddDependence(*stackDef->GetDepNode(), *insn.GetDepNode(), kDependenceTypeTrue);
        }
    }
    // Build dependency for heap memory access
    MapleVector<Insn *> &heapDefs = cdgNode.GetHeapDefInsns();
    for (auto *heapDef : heapDefs) {
        if (AArch64MemReference::NeedBuildMemoryDependency(*heapDef, insn, GetMemBaseDefInsn(*heapDef),
                                                           GetMemBaseDefInsn(insn), kDependenceTypeTrue)) {
            AddDependence(*heapDef->GetDepNode(), *insn.GetDepNode(), kDependenceTypeTrue);
        }
    }
}

// Build data dependence of symbol memory access.
// Memory accesses with symbol must be a heap memory access.
void AArch64DataDepBase::BuildDepsAccessStImmMem(Insn &insn)
{
    for (auto *heapDef : curCDGNode->GetHeapDefInsns()) {
        if (AArch64MemReference::NeedBuildMemoryDependency(*heapDef, insn, GetMemBaseDefInsn(*heapDef),
                                                           GetMemBaseDefInsn(insn), kDependenceTypeTrue)) {
            AddDependence(*heapDef->GetDepNode(), *insn.GetDepNode(), kDependenceTypeMemAccess);
        }
    }

    curCDGNode->AddHeapUseInsn(&insn);

    // Build dependency for membar insn
    Insn *membarInsn = curCDGNode->GetMembarInsn();
    if (membarInsn != nullptr) {
        AddDependence(*membarInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeMembar);
    }
}

// Build data dependence of memory bars instructions
void AArch64DataDepBase::BuildDepsMemBar(Insn &insn)
{
    if (isIntra || curRegion->GetRegionNodeSize() == 1 || curRegion->GetRegionRoot() == curCDGNode) {
        AddDependence4InsnInVectorByTypeAndCmp(curCDGNode->GetStackUseInsns(), insn, kDependenceTypeMembar);
        AddDependence4InsnInVectorByTypeAndCmp(curCDGNode->GetHeapUseInsns(), insn, kDependenceTypeMembar);
        AddDependence4InsnInVectorByTypeAndCmp(curCDGNode->GetStackDefInsns(), insn, kDependenceTypeMembar);
        AddDependence4InsnInVectorByTypeAndCmp(curCDGNode->GetHeapDefInsns(), insn, kDependenceTypeMembar);
    } else if (curRegion->GetRegionRoot() != curCDGNode) {
        BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), true, kDependenceTypeMembar, kStackUses);
        BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), true, kDependenceTypeMembar, kHeapUses);
        BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), true, kDependenceTypeMembar, kStackDefs);
        BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), true, kDependenceTypeMembar, kHeapDefs);
    }
    curCDGNode->SetMembarInsn(&insn);
}

// Build data dependence of stack memory and heap memory read:
// for memOpnd, do not build the true dependency, and identify it by a special mem dependency.
void AArch64DataDepBase::BuildDepsUseMem(Insn &insn, MemOperand &memOpnd)
{
    memOpnd.SetAccessSize(insn.GetMemoryByteSize());

    if (isIntra || curRegion->GetRegionNodeSize() == 1 || curRegion->GetRegionRoot() == curCDGNode) {
        BuildDepsForMemUseCommon(insn, *curCDGNode);
    } else if (curRegion->GetRegionRoot() != curCDGNode) {
        BuildInterBlockMemDefUseDependency(*insn.GetDepNode(), false);
    }

    // Record mem insn
    RegOperand *baseRegister = memOpnd.GetBaseRegister();
    if ((baseRegister != nullptr && IsFrameReg(*baseRegister)) || memOpnd.IsStackMem()) {
        curCDGNode->AddStackUseInsn(&insn);
    } else {
        curCDGNode->AddHeapUseInsn(&insn);
    }

    // Build dependency for membar insn
    Insn *membarInsn = curCDGNode->GetMembarInsn();
    if (membarInsn != nullptr) {
        AddDependence(*membarInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeMembar);
    } else if (!isIntra && curRegion->GetRegionRoot() != curCDGNode) {
        BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), false, kDependenceTypeMembar, kMembar);
    }
}

// Build data dependency of stack memory and heap memory definitions
// We do not need build output dependence for write-write, because of transitivity:
// e.g.
//   write1 [mem1]  ---
//        | access    |
//   read   [mem1]    X   (transitivity)
//        | anti      |
//   write2 [mem1]  ---
void AArch64DataDepBase::BuildDepsDefMem(Insn &insn, MemOperand &memOpnd)
{
    RegOperand *baseRegister = memOpnd.GetBaseRegister();
    ASSERT_NOT_NULL(baseRegister);
    memOpnd.SetAccessSize(insn.GetMemoryByteSize());

    if (isIntra || curRegion->GetRegionNodeSize() == 1 || curRegion->GetRegionRoot() == curCDGNode) {
        BuildDepsForMemDefCommon(insn, *curCDGNode);

        // Memory definition can not across may-throw insns for java
        if (cgFunc.GetMirModule().IsJavaModule()) {
            MapleVector<Insn *> &mayThrows = curCDGNode->GetMayThrowInsns();
            AddDependence4InsnInVectorByType(mayThrows, insn, kDependenceTypeThrow);
        }
    } else if (curRegion->GetRegionRoot() != curCDGNode) {
        BuildInterBlockMemDefUseDependency(*insn.GetDepNode(), true);

        if (cgFunc.GetMirModule().IsJavaModule()) {
            BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), false, kDependenceTypeThrow, kMayThrows);
        }
    }

    if (baseRegister->GetRegisterNumber() == RSP) {
        Insn *lastCallInsn = curCDGNode->GetLastCallInsn();
        if (lastCallInsn != nullptr && lastCallInsn->GetMachineOpcode() != MOP_tls_desc_call) {
            // Build a dependence between stack passed arguments and call
            AddDependence(*lastCallInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeControl);
        } else if (!isIntra && curRegion->GetRegionRoot() != curCDGNode) {
            BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), false, kDependenceTypeControl, kLastCall);
        }
    }

    // Build membar dependence
    Insn *membarInsn = curCDGNode->GetMembarInsn();
    if (membarInsn != nullptr) {
        AddDependence(*membarInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeMembar);
    } else if (!isIntra && curRegion->GetRegionRoot() != curCDGNode) {
        BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), false, kDependenceTypeMembar, kMembar);
    }

    // Update cur cdgNode info of def-memory insn
    if (IsFrameReg(*baseRegister) || memOpnd.IsStackMem()) {
        curCDGNode->AddStackDefInsn(&insn);
    } else {
        curCDGNode->AddHeapDefInsn(&insn);
    }
}

// Build dependence of call instructions.
// caller-saved physical registers will be defined by a call instruction.
// also a conditional register may be modified by a call.
void AArch64DataDepBase::BuildCallerSavedDeps(Insn &insn)
{
    // Build anti dependence and output dependence
    for (uint32 i = R0; i <= R9; ++i) {
        BuildDepsDefReg(insn, i);
    }
    for (uint32 i = V0; i <= V7; ++i) {
        BuildDepsDefReg(insn, i);
    }
    if (!beforeRA) {
        for (uint32 i = R9; i <= R18; ++i) {
            BuildDepsDefReg(insn, i);
        }
        for (uint32 i = RLR; i <= RSP; ++i) {
            BuildDepsUseReg(insn, i);
        }
        for (uint32 i = V16; i <= V31; ++i) {
            BuildDepsDefReg(insn, i);
        }
    }
    /* For condition operand, such as NE, EQ, and so on. */
    if (cgFunc.GetRflag() != nullptr) {
        BuildDepsDefReg(insn, kRFLAG);
    }
}

// Some insns may dirty all stack memory, such as "bl MCC_InitializeLocalStackRef"
void AArch64DataDepBase::BuildDepsDirtyStack(Insn &insn)
{
    /* Build anti dependence */
    MapleVector<Insn *> &stackUses = curCDGNode->GetStackUseInsns();
    AddDependence4InsnInVectorByType(stackUses, insn, kDependenceTypeAnti);
    /* Build output dependence */
    MapleVector<Insn *> &stackDefs = curCDGNode->GetStackDefInsns();
    AddDependence4InsnInVectorByType(stackDefs, insn, kDependenceTypeOutput);
    curCDGNode->AddStackDefInsn(&insn);
}

// Some call insns may use all stack memory, such as "bl MCC_CleanupLocalStackRef_NaiveRCFast"
void AArch64DataDepBase::BuildDepsUseStack(Insn &insn)
{
    /* Build true dependence */
    MapleVector<Insn *> &stackDefs = curCDGNode->GetStackDefInsns();
    AddDependence4InsnInVectorByType(stackDefs, insn, kDependenceTypeTrue);
}

// Some insns may dirty all heap memory, such as a call insn
void AArch64DataDepBase::BuildDepsDirtyHeap(Insn &insn)
{
    // Build anti dependence
    MapleVector<Insn *> &heapUses = curCDGNode->GetHeapUseInsns();
    AddDependence4InsnInVectorByType(heapUses, insn, kDependenceTypeAnti);
    // Build output dependence
    MapleVector<Insn *> &heapDefs = curCDGNode->GetHeapDefInsns();
    AddDependence4InsnInVectorByType(heapDefs, insn, kDependenceTypeOutput);

    Insn *membarInsn = curCDGNode->GetMembarInsn();
    if (membarInsn != nullptr) {
        AddDependence(*membarInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeMembar);
    }
    curCDGNode->AddHeapDefInsn(&insn);
}

// Build data dependence of memory operand.
// insn : an instruction with the memory access operand.
// opnd : the memory access operand.
// regProp : operand property of the memory access operand.
void AArch64DataDepBase::BuildMemOpndDependency(Insn &insn, Operand &opnd, const OpndDesc &regProp)
{
    DEBUG_ASSERT(opnd.IsMemoryAccessOperand(), "opnd must be memory Operand");
    auto *memOpnd = static_cast<MemOperand *>(&opnd);

    // Build dependency for register of memOpnd
    RegOperand *baseRegister = memOpnd->GetBaseRegister();
    if (baseRegister != nullptr) {
        regno_t regNO = baseRegister->GetRegisterNumber();
        BuildDepsUseReg(insn, regNO);
        if (memOpnd->IsPostIndexed() || memOpnd->IsPreIndexed()) {
            // Base operand has redefined
            BuildDepsDefReg(insn, regNO);
        }
    }
    RegOperand *indexRegister = memOpnd->GetIndexRegister();
    if (indexRegister != nullptr) {
        regno_t regNO = indexRegister->GetRegisterNumber();
        BuildDepsUseReg(insn, regNO);
    }

    // Build dependency for mem access
    if (regProp.IsUse()) {
        BuildDepsUseMem(insn, *memOpnd);
    } else {
        BuildDepsDefMem(insn, *memOpnd);
        BuildDepsAmbiInsn(insn);
    }

    // Build dependency for yield point in java
    if (cgFunc.GetMirModule().IsJavaModule() && IsYieldPoint(insn)) {
        BuildDepsMemBar(insn);
        BuildDepsDefReg(insn, kRFLAG);
    }
}

// Build Dependency for each operand of insn
void AArch64DataDepBase::BuildOpndDependency(Insn &insn)
{
    const InsnDesc *md = insn.GetDesc();
    MOperator mOp = insn.GetMachineOpcode();
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn.GetOperand(i);
        const OpndDesc *regProp = md->opndMD[i];
        if (opnd.IsMemoryAccessOperand()) {
            BuildMemOpndDependency(insn, opnd, *regProp);
        } else if (opnd.IsStImmediate() && mOp != MOP_xadrpl12) {
            BuildDepsAccessStImmMem(insn);
        } else if (opnd.IsRegister()) {
            auto &regOpnd = static_cast<RegOperand &>(opnd);
            regno_t regNO = regOpnd.GetRegisterNumber();
            if (regProp->IsUse()) {
                BuildDepsUseReg(insn, regNO);
            }
            if (regProp->IsDef()) {
                BuildDepsDefReg(insn, regNO);
            }
        } else if (opnd.IsConditionCode()) {
            // For condition operand, such as NE, EQ, and so on.
            if (regProp->IsUse()) {
                BuildDepsUseReg(insn, kRFLAG);
                BuildDepsBetweenControlRegAndCall(insn, false);
            }
            if (regProp->IsDef()) {
                BuildDepsDefReg(insn, kRFLAG);
                BuildDepsBetweenControlRegAndCall(insn, true);
            }
        } else if (opnd.IsList()) {
            auto &listOpnd = static_cast<const ListOperand &>(opnd);
            for (auto &lst : listOpnd.GetOperands()) {
                regno_t regNO = lst->GetRegisterNumber();
                BuildDepsUseReg(insn, regNO);
            }
        }
    }
}

// Build dependencies in some special cases for MOP_xbl
void AArch64DataDepBase::BuildSpecialBLDepsForJava(Insn &insn)
{
    DEBUG_ASSERT(insn.GetMachineOpcode() == MOP_xbl, "invalid insn");
    auto &target = static_cast<FuncNameOperand &>(insn.GetOperand(0));
    if ((target.GetName() == "MCC_InitializeLocalStackRef") || (target.GetName() == "MCC_ClearLocalStackRef") ||
        (target.GetName() == "MCC_DecRefResetPair")) {
        // Write stack memory
        BuildDepsDirtyStack(insn);
    } else if ((target.GetName() == "MCC_CleanupLocalStackRef_NaiveRCFast") ||
               (target.GetName() == "MCC_CleanupLocalStackRefSkip_NaiveRCFast") ||
               (target.GetName() == "MCC_CleanupLocalStackRefSkip")) {
        // Use Stack Memory
        BuildDepsUseStack(insn);
    }
}

// Build dependencies for call insns which do not obey standard call procedure
void AArch64DataDepBase::BuildSpecialCallDeps(Insn &insn)
{
    if (insn.IsSpecialCall()) {
        // The runtime model uses to implement this originates in the IA-64 processor-specific ABI
        // It is not available everywhere !!!
        // for tls_desc_call, which clobber r0, r1, cc reg according to call convention rules,
        // and the blr will write the LR reg.
        if (insn.GetMachineOpcode() == MOP_tls_desc_call) {
            BuildDepsDefReg(insn, RLR);
            BuildDepsDefReg(insn, kRFLAG);
        }
    }
}

// Build dependencies in some special cases (stack/heap/throw/clinit/lazy binding/control flow)
void AArch64DataDepBase::BuildSpecialInsnDependency(Insn &insn, const MapleVector<DepNode *> &nodes)
{
    const InsnDesc *md = insn.GetDesc();
    MOperator mOp = insn.GetMachineOpcode();
    if (insn.IsCall() || insn.IsTailCall()) {
        // Build caller saved registers dependency
        BuildCallerSavedDeps(insn);
        BuildDepsDirtyStack(insn);
        BuildDepsDirtyHeap(insn);
        BuildDepsAmbiInsn(insn);
        if (mOp == MOP_xbl) {
            BuildSpecialBLDepsForJava(insn);
        }
        BuildDepsLastCallInsn(insn);
    } else if (insn.IsClinit() || IsLazyLoad(insn.GetMachineOpcode()) ||
               insn.GetMachineOpcode() == MOP_arrayclass_cache_ldr) {
        BuildDepsDirtyHeap(insn);
        BuildDepsDefReg(insn, kRFLAG);
        if (insn.GetMachineOpcode() != MOP_adrp_ldr) {
            BuildDepsDefReg(insn, R16);
            BuildDepsDefReg(insn, R17);
        }
    } else if (mOp == MOP_xret || md->IsBranch()) {
        BuildDepsControlAll(insn, nodes);
    } else if (insn.IsMemAccessBar()) {
        BuildDepsMemBar(insn);
    } else if (insn.IsSpecialIntrinsic()) {
        BuildDepsDirtyHeap(insn);
    }
}

void AArch64DataDepBase::BuildAsmInsnDependency(Insn &insn)
{
    Insn *asmInsn = curCDGNode->GetLastInlineAsmInsn();
    if (asmInsn != nullptr) {
        // Due to the possible undefined behavior of users, we conservatively restrict
        // the instructions under the asm-insn to be moved above this instruction,
        // by building dependency edges on asm-insn and all subsequent instructions.
        // e.g.
        // asm volatile ( "mov x2, %[a]\n\t"
        //                "sub x2, x2, %[a]\n\t"
        //                "orr x3, x2, %[a]\n\t"
        //                :
        //                : [a] "I" (1)
        //                : "x2"
        // It only identifies that clobber x2.
        AddDependence(*asmInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeControl);
    }

    if (insn.IsAsmInsn()) {
        DEBUG_ASSERT(insn.GetOperand(kInsnSecondOpnd).IsList(), "invalid opnd of asm insn");
        DEBUG_ASSERT(insn.GetOperand(kInsnThirdOpnd).IsList(), "invalid opnd of asm insn");
        DEBUG_ASSERT(insn.GetOperand(kInsnFourthOpnd).IsList(), "invalid opnd of asm insn");
        auto &outputList = static_cast<ListOperand &>(insn.GetOperand(kInsnSecondOpnd));
        auto &clobberList = static_cast<ListOperand &>(insn.GetOperand(kInsnThirdOpnd));
        auto &inputList = static_cast<ListOperand &>(insn.GetOperand(kInsnFourthOpnd));
        for (auto *defOpnd : outputList.GetOperands()) {
            if (defOpnd == nullptr) {
                continue;
            }
            BuildDepsDefReg(insn, defOpnd->GetRegisterNumber());
        }
        for (auto *defOpnd : clobberList.GetOperands()) {
            if (defOpnd == nullptr) {
                continue;
            }
            BuildDepsDefReg(insn, defOpnd->GetRegisterNumber());
        }
        for (auto *useOpnd : inputList.GetOperands()) {
            if (useOpnd == nullptr) {
                continue;
            }
            BuildDepsUseReg(insn, useOpnd->GetRegisterNumber());
        }
        curCDGNode->SetLastInlineAsmInsn(&insn);
    }
}

void AArch64DataDepBase::BuildInterBlockMemDefUseDependency(DepNode &depNode, bool isMemDef)
{
    CHECK_FATAL(!isIntra, "must be inter block data dependence analysis");
    CHECK_FATAL(curRegion->GetRegionRoot() != curCDGNode, "for the root node, cross-BB search is not required");
    BB *curBB = curCDGNode->GetBB();
    CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
    std::vector<bool> visited(curRegion->GetMaxBBIdInRegion() + 1, false);
    if (isMemDef) {
        BuildPredPathMemDefDependencyDFS(*curBB, visited, depNode);
    } else {
        BuildPredPathMemUseDependencyDFS(*curBB, visited, depNode);
    }
}

void AArch64DataDepBase::BuildPredPathMemDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode)
{
    if (visited[curBB.GetId()]) {
        return;
    }
    CDGNode *cdgNode = curBB.GetCDGNode();
    CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
    CDGRegion *region = cdgNode->GetRegion();
    CHECK_FATAL(region != nullptr, "get region from cdgNode failed");
    if (region->GetRegionId() != curRegion->GetRegionId()) {
        return;
    }

    visited[curBB.GetId()] = true;

    BuildDepsForMemDefCommon(*depNode.GetInsn(), *cdgNode);

    // Ignore back-edge
    if (cdgNode == curRegion->GetRegionRoot()) {
        return;
    }

    for (auto predIt = curBB.GetPredsBegin(); predIt != curBB.GetPredsEnd(); ++predIt) {
        // Ignore back-edge of self-loop
        if (*predIt != &curBB) {
            BuildPredPathMemDefDependencyDFS(**predIt, visited, depNode);
        }
    }
}

void AArch64DataDepBase::BuildPredPathMemUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode)
{
    if (visited[curBB.GetId()]) {
        return;
    }
    CDGNode *cdgNode = curBB.GetCDGNode();
    CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
    CDGRegion *region = cdgNode->GetRegion();
    CHECK_FATAL(region != nullptr, "get region from cdgNode failed");
    if (region->GetRegionId() != curRegion->GetRegionId()) {
        return;
    }
    visited[curBB.GetId()] = true;

    BuildDepsForMemUseCommon(*depNode.GetInsn(), *cdgNode);

    // Ignore back-edge
    if (cdgNode == curRegion->GetRegionRoot()) {
        return;
    }
    for (auto predIt = curBB.GetPredsBegin(); predIt != curBB.GetPredsEnd(); ++predIt) {
        // Ignore back-edge of self-loop
        if (*predIt != &curBB) {
            BuildPredPathMemUseDependencyDFS(**predIt, visited, depNode);
        }
    }
}

void AArch64DataDepBase::DumpNodeStyleInDot(std::ofstream &file, DepNode &depNode)
{
    MOperator mOp = depNode.GetInsn()->GetMachineOpcode();
    const InsnDesc *md = &AArch64CG::kMd[mOp];
    file << "  insn_" << depNode.GetInsn() << "[";
    file << "label = \"" << depNode.GetInsn()->GetId() << ":\n";
    file << "{ " << md->name << "}\"];\n";
}
}  // namespace maplebe
