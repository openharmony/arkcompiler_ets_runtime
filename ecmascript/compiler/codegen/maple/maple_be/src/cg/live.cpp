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

#include "live.h"
#include <set>
#include "cg.h"
#include "cg_option.h"
#include "cgfunc.h"

/*
 * This phase build two sets: liveOutRegno and liveInRegno of each BB.
 * This algorithm mainly include 3 parts:
 * 1. initialize and get def[]/use[] of each BB;
 * 2. build live_in and live_out based on this algorithm
 *   Out[B] = U In[S] //S means B's successor;
 *   In[B] = use[B] U (Out[B]-def[B]);
 * 3. deal with cleanup BB.
 */
namespace maplebe {
#define LIVE_ANALYZE_DUMP_NEWPM CG_DEBUG_FUNC(f)

void LiveAnalysis::InitAndGetDefUse()
{
    FOR_ALL_BB(bb, cgFunc)
    {
        InitBB(*bb);
        GetBBDefUse(*bb);
    }
}

bool LiveAnalysis::RemovePhiLiveInFromSuccNotFromThisBB(BB &curBB, BB &succBB) const
{
    if (succBB.GetPhiInsns().empty()) {
        return false;
    }
    LocalMapleAllocator allocator(cgFunc->GetStackMemPool());
    SparseDataInfo tempPhiIn(succBB.GetLiveIn()->GetMaxRegNum(), allocator);
    tempPhiIn.ResetAllBit();
    for (auto phiInsnIt : succBB.GetPhiInsns()) {
        auto &phiList = static_cast<PhiOperand &>(phiInsnIt.second->GetOperand(kInsnSecondOpnd));
        for (auto phiOpndIt : phiList.GetOperands()) {
            uint32 fBBId = phiOpndIt.first;
            DEBUG_ASSERT(fBBId != 0, "GetFromBBID = 0");
            if (fBBId != curBB.GetId()) {
                regno_t regNo = phiOpndIt.second->GetRegisterNumber();
                tempPhiIn.SetBit(regNo);
            }
        }
    }
    return curBB.GetLiveOut()->Difference(tempPhiIn);
}

/* Out[BB] = Union all of In[Succs(BB)]
 *
 * in ssa form
 * Out[BB] = Union all of In[Succs(BB)] Except Phi use reg dont from this BB
 */
bool LiveAnalysis::GenerateLiveOut(BB &bb) const
{
    bool isChanged = false;
    for (auto succBB : bb.GetSuccs()) {
        if (succBB->GetLiveInChange() && !succBB->GetLiveIn()->NoneBit()) {
            isChanged = bb.LiveOutOrBits(*succBB->GetLiveIn()) || isChanged;
            isChanged = RemovePhiLiveInFromSuccNotFromThisBB(bb, *succBB) || isChanged;
        }
    }
    return isChanged;
}

/* In[BB] = use[BB] Union (Out[BB]-def[BB]) */
bool LiveAnalysis::GenerateLiveIn(BB &bb)
{
    bool isChanged = false;
    LocalMapleAllocator allocator(cgFunc->GetStackMemPool());
    if (!bb.GetInsertUse()) {
        if (!bb.GetLiveIn()->IsEqual(*bb.GetUse())) {
            bb.SetLiveInInfo(*bb.GetUse());
            isChanged = true;
        }
        bb.SetInsertUse(true);
    }
    SparseDataInfo &bbLiveOut = bb.GetLiveOut()->Clone(allocator);
    if (!bbLiveOut.NoneBit()) {
        bbLiveOut.Difference(*bb.GetDef());
        isChanged = bb.LiveInOrBits(bbLiveOut) || isChanged;
    }
    return isChanged;
}

SparseDataInfo *LiveAnalysis::GenerateLiveInByDefUse(SparseDataInfo &liveOut, SparseDataInfo &use, SparseDataInfo &def)
{
    SparseDataInfo *liveIn = &use;
    LocalMapleAllocator allocator(cgFunc->GetStackMemPool());
    SparseDataInfo *tmpLiveOut = memPool->New<SparseDataInfo>(liveOut, allocator);
    if (!liveOut.NoneBit()) {
        tmpLiveOut->Difference(def);
        liveIn->OrBits(*tmpLiveOut);
    }
    return liveIn;
}

void LiveAnalysis::GenerateStackMapLiveIn()
{
    const auto &stackMapInsns = cgFunc->GetStackMapInsns();
    for (auto *insn : stackMapInsns) {
        BB *curBB = insn->GetBB();
        SparseDataInfo *liveIn =
            GenerateLiveInByDefUse(*curBB->GetLiveOut(), *insn->GetStackMapUse(), *insn->GetStackMapDef());
        insn->SetStackMapLiveIn(*liveIn);
    }
}
/* building liveIn and liveOut of each BB. */
void LiveAnalysis::BuildInOutforFunc()
{
    iteration = 0;
    bool hasChange;
    do {
        ++iteration;
        hasChange = false;
        FOR_ALL_BB_REV(bb, cgFunc)
        {
            if (!bb || bb->IsUnreachable() || !bb->GetLiveOut() || !bb->GetLiveIn()) {
                continue;
            }
            if (!GenerateLiveOut(*bb) && bb->GetInsertUse()) {
                continue;
            }
            if (GenerateLiveIn(*bb)) {
                bb->SetLiveInChange(true);
                hasChange = true;
            } else {
                bb->SetLiveInChange(false);
            }
        }
    } while (hasChange);
}

/*  reset to liveout/in_regno */
void LiveAnalysis::ResetLiveSet()
{
    FOR_ALL_BB(bb, cgFunc)
    {
        bb->GetLiveIn()->GetBitsOfInfo<MapleSet<uint32>>(bb->GetLiveInRegNO());
        bb->GetLiveOut()->GetBitsOfInfo<MapleSet<uint32>>(bb->GetLiveOutRegNO());
    }
}

/* entry function for LiveAnalysis */
void LiveAnalysis::AnalysisLive()
{
    InitAndGetDefUse();
    BuildInOutforFunc();
    if (!cgFunc->IsStackMapComputed()) {
        GenerateStackMapLiveIn();
    }
}

void LiveAnalysis::MarkStackMapInsn(Insn &insn, BB &bb) const
{
    if (insn.GetStackMap() != nullptr) {
        for (auto [deoptVreg, opnd] : insn.GetStackMap()->GetDeoptInfo().GetDeoptBundleInfo()) {
            CollectLiveInfo(bb, *opnd, false, true);
        }
    }
    insn.SetStackMapDef(*NewDef(*bb.GetDef()));
    insn.SetStackMapUse(*NewUse(*bb.GetUse()));
}

/*
 * entry of get def/use of bb.
 * getting the def or use info of each regopnd as parameters of CollectLiveInfo().
 */
void LiveAnalysis::GetBBDefUse(BB &bb) const
{
    if (bb.GetKind() == BB::kBBReturn) {
        GenerateReturnBBDefUse(bb);
    }
    if (!bb.HasMachineInsn()) {
        return;
    }
    bb.DefResetAllBit();
    bb.UseResetAllBit();

    FOR_BB_INSNS_REV(insn, &bb)
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }

        if (insn->IsCall() && !cgFunc->IsStackMapComputed()) {
            MarkStackMapInsn(*insn, bb);
        }

        bool isAsm = insn->IsAsmInsn();
        const InsnDesc *md = insn->GetDesc();
        if (insn->IsCall() || insn->IsTailCall()) {
            ProcessCallInsnParam(bb, *insn);
        }
        uint32 opndNum = insn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            const OpndDesc *opndDesc = md->GetOpndDes(i);
            DEBUG_ASSERT(opndDesc != nullptr, "null ptr check");
            Operand &opnd = insn->GetOperand(i);
            if (opnd.IsList()) {
                if (isAsm) {
                    ProcessAsmListOpnd(bb, opnd, i);
                } else {
                    ProcessListOpnd(bb, opnd, opndDesc->IsDef());
                }
            } else if (opnd.IsMemoryAccessOperand()) {
                ProcessMemOpnd(bb, opnd);
            } else if (opnd.IsConditionCode()) {
                ProcessCondOpnd(bb);
            } else if (opnd.IsPhi()) {
                auto &phiOpnd = static_cast<PhiOperand &>(opnd);
                for (auto opIt : phiOpnd.GetOperands()) {
                    CollectLiveInfo(bb, *opIt.second, false, true);
                }
            } else {
                bool isDef = opndDesc->IsRegDef();
                bool isUse = opndDesc->IsRegUse();
                CollectLiveInfo(bb, opnd, isDef, isUse);
            }
        }
    }
}

/* build use and def sets of each BB according to the type of regOpnd. */
void LiveAnalysis::CollectLiveInfo(BB &bb, const Operand &opnd, bool isDef, bool isUse) const
{
    if (!opnd.IsRegister()) {
        return;
    }
    auto &regOpnd = static_cast<const RegOperand &>(opnd);
    regno_t regNO = regOpnd.GetRegisterNumber();
    RegType regType = regOpnd.GetRegisterType();
    if (regType == kRegTyVary) {
        return;
    }
    if (isDef) {
        bb.SetDefBit(regNO);
        if (!isUse) {
            bb.UseResetBit(regNO);
        }
    }
    if (isUse) {
        bb.SetUseBit(regNO);
        bb.DefResetBit(regNO);
    }
    if (!cgFunc->IsStackMapComputed() && regOpnd.GetBaseRefOpnd() != nullptr) {
        const RegOperand &baseOpnd = *regOpnd.GetBaseRefOpnd();
        regno_t baseRegNO = baseOpnd.GetRegisterNumber();
        bb.SetUseBit(baseRegNO);
        bb.DefResetBit(baseRegNO);
    }
}

void LiveAnalysis::ProcessAsmListOpnd(BB &bb, Operand &opnd, uint32 idx) const
{
    bool isDef = false;
    bool isUse = false;
    switch (idx) {
        case kAsmOutputListOpnd:
        case kAsmClobberListOpnd: {
            isDef = true;
            break;
        }
        case kAsmInputListOpnd: {
            isUse = true;
            break;
        }
        default:
            return;
    }
    ListOperand &listOpnd = static_cast<ListOperand &>(opnd);
    for (auto op : listOpnd.GetOperands()) {
        CollectLiveInfo(bb, *op, isDef, isUse);
    }
}

void LiveAnalysis::ProcessListOpnd(BB &bb, Operand &opnd, bool isDef) const
{
    ListOperand &listOpnd = static_cast<ListOperand &>(opnd);
    for (auto op : listOpnd.GetOperands()) {
        CollectLiveInfo(bb, *op, isDef, !isDef);
    }
}

void LiveAnalysis::ProcessMemOpnd(BB &bb, Operand &opnd) const
{
    auto &memOpnd = static_cast<MemOperand &>(opnd);
    Operand *base = memOpnd.GetBaseRegister();
    Operand *offset = memOpnd.GetIndexRegister();
    if (base != nullptr) {
        CollectLiveInfo(bb, *base, !memOpnd.IsIntactIndexed(), true);
    }
    if (offset != nullptr) {
        CollectLiveInfo(bb, *offset, false, true);
    }
}

void LiveAnalysis::ProcessCondOpnd(BB &bb) const
{
    Operand &rflag = cgFunc->GetOrCreateRflag();
    CollectLiveInfo(bb, rflag, false, true);
}

/* dump the current info of def/use/livein/liveout */
void LiveAnalysis::Dump() const
{
    CHECK_FATAL(cgFunc != nullptr, "nullptr check");
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(cgFunc->GetFunction().GetStIdx().Idx());
    DEBUG_ASSERT(funcSt != nullptr, "null ptr check");
    LogInfo::MapleLogger() << "\n---------  liveness for " << funcSt->GetName() << "  iteration ";
    LogInfo::MapleLogger() << iteration << " ---------\n";
    FOR_ALL_BB(bb, cgFunc)
    {
        LogInfo::MapleLogger() << "  === BB_" << bb->GetId() << " (" << std::hex << bb << ") " << std::dec << " <"
                               << bb->GetKindName();
        if (bb->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
            LogInfo::MapleLogger() << "[labeled with " << bb->GetLabIdx() << "]";
        }
        LogInfo::MapleLogger() << "> idx " << bb->GetId() << " ===\n";

        if (!bb->GetPreds().empty()) {
            LogInfo::MapleLogger() << "    pred [ ";
            for (auto *pred : bb->GetPreds()) {
                LogInfo::MapleLogger() << pred->GetId() << " (" << std::hex << pred << ") " << std::dec << " ";
            }
            LogInfo::MapleLogger() << "]\n";
        }
        if (!bb->GetSuccs().empty()) {
            LogInfo::MapleLogger() << "    succ [ ";
            for (auto *succ : bb->GetSuccs()) {
                LogInfo::MapleLogger() << succ->GetId() << " (" << std::hex << succ << ") " << std::dec << " ";
            }
            LogInfo::MapleLogger() << "]\n";
        }

        const SparseDataInfo *infoDef = nullptr;
        LogInfo::MapleLogger() << "    DEF: ";
        infoDef = bb->GetDef();
        DumpInfo(*infoDef);

        const SparseDataInfo *infoUse = nullptr;
        LogInfo::MapleLogger() << "\n    USE: ";
        infoUse = bb->GetUse();
        DumpInfo(*infoUse);

        const SparseDataInfo *infoLiveIn = nullptr;
        LogInfo::MapleLogger() << "\n    Live IN: ";
        infoLiveIn = bb->GetLiveIn();
        DumpInfo(*infoLiveIn);

        const SparseDataInfo *infoLiveOut = nullptr;
        LogInfo::MapleLogger() << "\n    Live OUT: ";
        infoLiveOut = bb->GetLiveOut();
        DumpInfo(*infoLiveOut);
        LogInfo::MapleLogger() << "\n";
    }
    LogInfo::MapleLogger() << "---------------------------\n";
}

void LiveAnalysis::DumpInfo(const SparseDataInfo &info) const
{
    uint32 count = 1;
    std::set<uint32> res;
    info.GetInfo().ConvertToSet(res);
    for (uint32 x : res) {
        LogInfo::MapleLogger() << x << " ";
        ++count;
        /* 20 output one line */
        if ((count % 20) == 0) {
            LogInfo::MapleLogger() << "\n";
        }
    }
    LogInfo::MapleLogger() << '\n';
}

/* initialize dependent info and container of BB. */
void LiveAnalysis::InitBB(BB &bb)
{
    bb.SetLiveInChange(true);
    bb.SetInsertUse(false);
    bb.ClearLiveInRegNO();
    bb.ClearLiveOutRegNO();
    const uint32 maxRegCount = cgFunc->GetMaxVReg();
    bb.SetLiveIn(*NewLiveIn(maxRegCount));
    bb.SetLiveOut(*NewLiveOut(maxRegCount));
    bb.SetDef(*NewDef(maxRegCount));
    bb.SetUse(*NewUse(maxRegCount));
}

void CgLiveAnalysis::GetAnalysisDependence(AnalysisDep &aDep) const
{
#if TARGX86_64
    if (Triple::GetTriple().GetArch() == Triple::ArchType::x64) {
        aDep.AddRequired<CgHandleCFG>();
    }
#endif
    aDep.SetPreservedAll();
}

bool CgLiveAnalysis::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *liveMemPool = GetPhaseMemPool();
    live = f.GetCG()->CreateLiveAnalysis(*liveMemPool, f);
    CHECK_FATAL(live != nullptr, "NIY");
    live->AnalysisLive();
    if (LIVE_ANALYZE_DUMP_NEWPM) {
        live->Dump();
    }
    live->ResetLiveSet();
    return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgLiveAnalysis, liveanalysis)
} /* namespace maplebe */
