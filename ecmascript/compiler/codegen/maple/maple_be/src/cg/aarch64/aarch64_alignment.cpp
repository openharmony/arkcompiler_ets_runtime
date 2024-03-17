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

#include "aarch64_alignment.h"
#include "insn.h"
#include "loop.h"
#include "aarch64_cg.h"
#include "cg_option.h"
#include <unordered_map>

namespace maplebe {
void AArch64AlignAnalysis::FindLoopHeader()
{
    if (loopInfo.GetLoops().empty()) {
        return;
    }
    for (auto *loop : loopInfo.GetLoops()) {
        BB &header = loop->GetHeader();
        if (&header == aarFunc->GetFirstBB() || IsIncludeCall(header) || !IsInSizeRange(header)) {
            continue;
        }
        InsertLoopHeaderBBs(header);
    }
}

void AArch64AlignAnalysis::FindJumpTarget()
{
    MapleUnorderedMap<LabelIdx, BB *> label2BBMap = aarFunc->GetLab2BBMap();
    if (label2BBMap.empty()) {
        return;
    }
    for (auto &iter : label2BBMap) {
        BB *jumpBB = iter.second;
        if (jumpBB != nullptr) {
            InsertJumpTargetBBs(*jumpBB);
        }
    }
}

bool AArch64AlignAnalysis::IsIncludeCall(BB &bb)
{
    return bb.HasCall();
}

bool AArch64AlignAnalysis::IsInSizeRange(BB &bb)
{
    uint64 size = 0;
    FOR_BB_INSNS_CONST(insn, &bb) {
        if (!insn->IsMachineInstruction() || insn->GetMachineOpcode() == MOP_pseudo_ret_int ||
            insn->GetMachineOpcode() == MOP_pseudo_ret_float) {
            continue;
        }
        size += kAlignInsnLength;
    }
    BB *curBB = &bb;
    while (curBB->GetNext() != nullptr && curBB->GetNext()->GetLabIdx() == 0) {
        FOR_BB_INSNS_CONST(insn, curBB->GetNext()) {
            if (!insn->IsMachineInstruction() || insn->GetMachineOpcode() == MOP_pseudo_ret_int ||
                insn->GetMachineOpcode() == MOP_pseudo_ret_float) {
                continue;
            }
            size += kAlignInsnLength;
        }
        curBB = curBB->GetNext();
    }
    AArch64AlignInfo targetInfo;
    if (CGOptions::GetAlignMinBBSize() == 0 || CGOptions::GetAlignMaxBBSize() == 0) {
        return false;
    }
    constexpr uint32 defaultMinBBSize = 16;
    constexpr uint32 defaultMaxBBSize = 44;
    targetInfo.alignMinBBSize = (CGOptions::OptimizeForSize()) ? defaultMinBBSize : CGOptions::GetAlignMinBBSize();
    targetInfo.alignMaxBBSize = (CGOptions::OptimizeForSize()) ? defaultMaxBBSize : CGOptions::GetAlignMaxBBSize();
    if (size <= targetInfo.alignMinBBSize || size >= targetInfo.alignMaxBBSize) {
        return false;
    }
    return true;
}

bool AArch64AlignAnalysis::HasFallthruEdge(BB &bb)
{
    for (auto *iter : bb.GetPreds()) {
        if (iter == bb.GetPrev()) {
            return true;
        }
    }
    return false;
}

void AArch64AlignAnalysis::ComputeLoopAlign()
{
    if (loopHeaderBBs.empty()) {
        return;
    }
    for (BB *bb : loopHeaderBBs) {
        if (bb == cgFunc->GetFirstBB() || IsIncludeCall(*bb) || !IsInSizeRange(*bb)) {
            continue;
        }
        bb->SetNeedAlign(true);
        if (CGOptions::GetLoopAlignPow() == 0) {
            return;
        }
        AArch64AlignInfo targetInfo;
        targetInfo.loopAlign = CGOptions::GetLoopAlignPow();
        if (alignInfos.find(bb) == alignInfos.end()) {
            alignInfos[bb] = targetInfo.loopAlign;
        } else {
            uint32 curPower = alignInfos[bb];
            alignInfos[bb] = (targetInfo.loopAlign < curPower) ? targetInfo.loopAlign : curPower;
        }
        bb->SetAlignPower(alignInfos[bb]);
    }
}

void AArch64AlignAnalysis::ComputeJumpAlign()
{
    if (jumpTargetBBs.empty()) {
        return;
    }
    for (BB *bb : jumpTargetBBs) {
        if (bb == cgFunc->GetFirstBB() || !IsInSizeRange(*bb) || HasFallthruEdge(*bb)) {
            continue;
        }
        bb->SetNeedAlign(true);
        if (CGOptions::GetJumpAlignPow() == 0) {
            return;
        }
        AArch64AlignInfo targetInfo;
        targetInfo.jumpAlign = (CGOptions::OptimizeForSize()) ? kOffsetAlignmentOf64Bit : CGOptions::GetJumpAlignPow();
        if (alignInfos.find(bb) == alignInfos.end()) {
            alignInfos[bb] = targetInfo.jumpAlign;
        } else {
            uint32 curPower = alignInfos[bb];
            alignInfos[bb] = (targetInfo.jumpAlign < curPower) ? targetInfo.jumpAlign : curPower;
        }
        bb->SetAlignPower(alignInfos[bb]);
    }
}

uint32 AArch64AlignAnalysis::GetAlignRange(uint32 alignedVal, uint32 addr) const
{
    if (addr == 0) {
        return addr;
    }
    uint32 range = (alignedVal - (((addr - 1) * kInsnSize) & (alignedVal - 1))) / kInsnSize - 1;
    return range;
}

bool AArch64AlignAnalysis::IsInSameAlignedRegion(uint32 addr1, uint32 addr2, uint32 alignedRegionSize) const
{
    return (((addr1 - 1) * kInsnSize) / alignedRegionSize) == (((addr2 - 1) * kInsnSize) / alignedRegionSize);
}

bool AArch64AlignAnalysis::MarkCondBranchAlign()
{
    sameTargetBranches.clear();
    uint32 addr = 0;
    bool change = false;
    FOR_ALL_BB(bb, aarFunc) {
        if (bb != nullptr && bb->IsBBNeedAlign()) {
            uint32 alignedVal = (1U << bb->GetAlignPower());
            uint32 alignNopNum = GetAlignRange(alignedVal, addr);
            addr += alignNopNum;
            bb->SetAlignNopNum(alignNopNum);
        }
        FOR_BB_INSNS(insn, bb) {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            addr += insn->GetAtomicNum();
            MOperator mOp = insn->GetMachineOpcode();
            if ((mOp == MOP_wtbz || mOp == MOP_wtbnz || mOp == MOP_xtbz || mOp == MOP_xtbnz) && insn->IsNeedSplit()) {
                ++addr;
            }
            if (!insn->IsCondBranch() || insn->GetOperandSize() == 0) {
                insn->SetAddress(addr);
                continue;
            }
            Operand &opnd = insn->GetOperand(insn->GetOperandSize() - 1);
            if (!opnd.IsLabelOpnd()) {
                insn->SetAddress(addr);
                continue;
            }
            LabelIdx targetIdx = static_cast<LabelOperand &>(opnd).GetLabelIndex();
            if (sameTargetBranches.find(targetIdx) == sameTargetBranches.end()) {
                sameTargetBranches[targetIdx] = addr;
                insn->SetAddress(addr);
                continue;
            }
            uint32 sameTargetAddr = sameTargetBranches[targetIdx];
            uint32 alignedRegionSize = 1 << kAlignRegionPower;
            /**
             * if two branches jump to the same target and their addresses are within an 16byte aligned region,
             * add a certain number of [nop] to move them out of the region.
             */
            if (IsInSameAlignedRegion(sameTargetAddr, addr, alignedRegionSize)) {
                uint32 nopNum = GetAlignRange(alignedRegionSize, addr) + 1;
                nopNum = nopNum > kAlignMaxNopNum ? 0 : nopNum;
                if (nopNum == 0) {
                    break;
                }
                change = true;
                insn->SetNopNum(nopNum);
                for (uint32 i = 0; i < nopNum; i++) {
                    addr += insn->GetAtomicNum();
                }
            } else {
                insn->SetNopNum(0);
            }
            sameTargetBranches[targetIdx] = addr;
            insn->SetAddress(addr);
        }
    }
    return change;
}

void AArch64AlignAnalysis::UpdateInsnId()
{
    uint32 id = 0;
    FOR_ALL_BB(bb, aarFunc) {
        if (bb != nullptr && bb->IsBBNeedAlign()) {
            uint32 alignedVal = 1U << (bb->GetAlignPower());
            uint32 range = GetAlignRange(alignedVal, id);
            id = id + (range > kAlignPseudoSize ? range : kAlignPseudoSize);
        }
        FOR_BB_INSNS(insn, bb) {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            id += insn->GetAtomicNum();
            if (insn->IsCondBranch() && insn->GetNopNum() != 0) {
                id += insn->GetNopNum();
            }
            MOperator mOp = insn->GetMachineOpcode();
            if ((mOp == MOP_wtbz || mOp == MOP_wtbnz || mOp == MOP_xtbz || mOp == MOP_xtbnz) && insn->IsNeedSplit()) {
                ++id;
            }
            insn->SetId(id);
            if (insn->GetMachineOpcode() == MOP_adrp_ldr && CGOptions::IsLazyBinding() &&
                !aarFunc->GetCG()->IsLibcore()) {
                ++id;
            }
        }
    }
}

bool AArch64AlignAnalysis::MarkShortBranchSplit()
{
    bool change = false;
    bool split;
    do {
        split = false;
        UpdateInsnId();
        for (auto *bb = aarFunc->GetFirstBB(); bb != nullptr && !split; bb = bb->GetNext()) {
            for (auto *insn = bb->GetLastInsn(); insn != nullptr && !split; insn = insn->GetPrev()) {
                if (!insn->IsMachineInstruction()) {
                    continue;
                }
                MOperator mOp = insn->GetMachineOpcode();
                if (mOp != MOP_wtbz && mOp != MOP_wtbnz && mOp != MOP_xtbz && mOp != MOP_xtbnz) {
                    continue;
                }
                if (insn->IsNeedSplit()) {
                    continue;
                }
                auto &labelOpnd = static_cast<LabelOperand &>(insn->GetOperand(kInsnThirdOpnd));
                if (aarFunc->DistanceCheck(*bb, labelOpnd.GetLabelIndex(), insn->GetId(),
                                           AArch64Abi::kMaxInstrForTbnz)) {
                    continue;
                }
                split = true;
                change = true;
                insn->SetNeedSplit(split);
            }
        }
    } while (split);
    return change;
}

void AArch64AlignAnalysis::AddNopAfterMark()
{
    FOR_ALL_BB(bb, aarFunc) {
        FOR_BB_INSNS(insn, bb) {
            if (!insn->IsMachineInstruction() || !insn->IsCondBranch() || insn->GetNopNum() == 0) {
                continue;
            }
            /**
             * To minimize the performance loss of nop, we decided to place nop on an island before the current addr.
             * The island here is after [b, ret, br, blr].
             * To ensure correct insertion of the nop, the nop is inserted in the original position in the following
             * cases:
             * 1. A branch with the same target exists before it.
             * 2. A branch whose nopNum value is not 0 exists before it.
             * 3. no BBs need to be aligned between the original location and the island.
             */
            std::unordered_map<LabelIdx, Insn *> targetCondBrs;
            bool findIsland = false;
            Insn *detect = insn->GetPrev();
            BB *region = bb;
            while (detect != nullptr || region != aarFunc->GetFirstBB()) {
                while (detect == nullptr) {
                    DEBUG_ASSERT(region->GetPrev() != nullptr, "get region prev failed");
                    region = region->GetPrev();
                    detect = region->GetLastInsn();
                }
                if (detect->GetMachineOpcode() == MOP_xuncond || detect->GetMachineOpcode() == MOP_xret ||
                    detect->GetMachineOpcode() == MOP_xbr) {
                    findIsland = true;
                    break;
                }
                if (region->IsBBNeedAlign()) {
                    break;
                }
                if (!detect->IsMachineInstruction() || !detect->IsCondBranch() || detect->GetOperandSize() == 0) {
                    detect = detect->GetPrev();
                    continue;
                }
                if (detect->GetNopNum() != 0) {
                    break;
                }
                Operand &opnd = detect->GetOperand(detect->GetOperandSize() - 1);
                if (!opnd.IsLabelOpnd()) {
                    detect = detect->GetPrev();
                    continue;
                }
                LabelIdx targetIdx = static_cast<LabelOperand &>(opnd).GetLabelIndex();
                if (targetCondBrs.find(targetIdx) != targetCondBrs.end()) {
                    break;
                }
                targetCondBrs[targetIdx] = detect;
                detect = detect->GetPrev();
            }
            uint32 nopNum = insn->GetNopNum();
            if (findIsland) {
                for (uint32 i = 0; i < nopNum; i++) {
                    (void)bb->InsertInsnAfter(*detect, aarFunc->GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_nop));
                }
            } else {
                for (uint32 i = 0; i < nopNum; i++) {
                    (void)bb->InsertInsnBefore(*insn, aarFunc->GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_nop));
                }
            }
        }
    }
}

/**
 * The insertion of nop affects the judgement of the addressing range of short branches,
 * and the splitting of short branches affects the calculation of the location and number of nop insertions.
 * In the iteration process of both, we only make some marks, wait for the fixed points, and fill in nop finally.
 */
void AArch64AlignAnalysis::ComputeCondBranchAlign()
{
    bool condBrChange = false;
    bool shortBrChange = false;
    while (true) {
        condBrChange = MarkCondBranchAlign();
        if (!condBrChange) {
            break;
        }
        shortBrChange = MarkShortBranchSplit();
        if (!shortBrChange) {
            break;
        }
    }
    AddNopAfterMark();
}
} /* namespace maplebe */
