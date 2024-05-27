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
    CHECK_FATAL(alignedVal > 0, "must not be zero");
    uint32 range = (alignedVal - (((addr - 1) * kInsnSize) & (alignedVal - 1))) / kInsnSize - 1;
    return range;
}

bool AArch64AlignAnalysis::IsInSameAlignedRegion(uint32 addr1, uint32 addr2, uint32 alignedRegionSize) const
{
    CHECK_FATAL(addr2 > 0, "must not be zero");
    CHECK_FATAL(addr1 > 0, "must not be zero");
    return (((addr1 - 1) * kInsnSize) / alignedRegionSize) == (((addr2 - 1) * kInsnSize) / alignedRegionSize);
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

/**
 * The insertion of nop affects the judgement of the addressing range of short branches,
 * and the splitting of short branches affects the calculation of the location and number of nop insertions.
 * In the iteration process of both, we only make some marks, wait for the fixed points, and fill in nop finally.
 */
void AArch64AlignAnalysis::ComputeCondBranchAlign()
{
    bool shortBrChange = false;
    while (true) {
        shortBrChange = MarkShortBranchSplit();
        if (!shortBrChange) {
            break;
        }
    }
}
} /* namespace maplebe */
