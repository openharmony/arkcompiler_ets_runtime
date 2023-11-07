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

#include "reg_alloc_lsra.h"
#include <iomanip>
#include <queue>
#include <chrono>
#include "loop.h"

namespace maplebe {
/*
 * ==================
 * = Linear Scan RA
 * ==================
 */
#define LSRA_DUMP (CG_DEBUG_FUNC(*cgFunc))
namespace {
constexpr uint32 kSpilled = 1;
constexpr uint32 kMinLiveIntervalLength = 20;
constexpr uint32 kPrintedActiveListLength = 10;
/* Here, kLoopWeight is a fine-tuned empirical parameter */
constexpr uint32 kLoopWeight = 4;
}  // namespace

#define IN_SPILL_RANGE                                                                           \
    (cgFunc->GetName().find(CGOptions::GetDumpFunc()) != std::string::npos && ++debugSpillCnt && \
     (CGOptions::GetSpillRangesBegin() < debugSpillCnt) && (debugSpillCnt < CGOptions::GetSpillRangesEnd()))

#ifdef RA_PERF_ANALYSIS
static long bfsUS = 0;
static long liveIntervalUS = 0;
static long holesUS = 0;
static long lsraUS = 0;
static long finalizeUS = 0;
static long totalUS = 0;

extern void printLSRATime()
{
    std::cout << "============================================================\n";
    std::cout << "               LSRA sub-phase time information              \n";
    std::cout << "============================================================\n";
    std::cout << "BFS BB sorting cost: " << bfsUS << "us \n";
    std::cout << "live interval computing cost: " << liveIntervalUS << "us \n";
    std::cout << "live range approximation cost: " << holesUS << "us \n";
    std::cout << "LSRA cost: " << lsraUS << "us \n";
    std::cout << "finalize cost: " << finalizeUS << "us \n";
    std::cout << "LSRA total cost: " << totalUS << "us \n";
    std::cout << "============================================================\n";
}
#endif

/*
 * This LSRA implementation is an interpretation of the [Poletto97] paper.
 * BFS BB ordering is used to order the instructions.  The live intervals are vased on
 * this instruction order.  All vreg defines should come before an use, else a warning is
 * given.
 * Live interval is traversed in order from lower instruction order to higher order.
 * When encountering a live interval for the first time, it is assumed to be live and placed
 * inside the 'active' structure until the vreg's last access.  During the time a vreg
 * is in 'active', the vreg occupies a physical register allocation and no other vreg can
 * be allocated the same physical register.
 */
void LSRALinearScanRegAllocator::PrintRegSet(const MapleSet<uint32> &set, const std::string &str) const
{
    LogInfo::MapleLogger() << str;
    for (auto reg : set) {
        LogInfo::MapleLogger() << " " << reg;
    }
    LogInfo::MapleLogger() << "\n";
}

bool LSRALinearScanRegAllocator::CheckForReg(Operand &opnd, const Insn &insn, const LiveInterval &li, regno_t regNO,
                                             bool isDef) const
{
    if (!opnd.IsRegister()) {
        return false;
    }
    auto &regOpnd = static_cast<RegOperand &>(opnd);
    if (regOpnd.GetRegisterType() == kRegTyCc || regOpnd.GetRegisterType() == kRegTyVary) {
        return false;
    }
    if (regOpnd.GetRegisterNumber() == regNO) {
        LogInfo::MapleLogger() << "set object circle at " << insn.GetId() << "," << li.GetRegNO()
                               << " size 5 fillcolor rgb \"";
        if (isDef) {
            LogInfo::MapleLogger() << "black\"\n";
        } else {
            LogInfo::MapleLogger() << "orange\"\n";
        }
    }
    return true;
}

void LSRALinearScanRegAllocator::PrintLiveRanges(const LiveInterval &li) const
{
    if (li.GetAssignedReg() != 0) {
        uint32 base = (li.GetRegType() == kRegTyInt) ? firstIntReg : firstFpReg;
        LogInfo::MapleLogger() << "(assigned R" << (li.GetAssignedReg() - base) << ")";
    }
    if (li.GetStackSlot() == kSpilled) {
        LogInfo::MapleLogger() << "(spill)";
    }
    for (auto range : li.GetRanges()) {
        LogInfo::MapleLogger() << "[" << range.GetStart() << ", " << range.GetEnd() << "]"
                               << " ";
    }
    if (li.GetSplitNext() != nullptr) {
        LogInfo::MapleLogger() << "### SPLIT ### ";
        PrintLiveRanges(*li.GetSplitNext());
    }
    LogInfo::MapleLogger() << "\n";
}

void LSRALinearScanRegAllocator::PrintAllLiveRanges() const
{
    LogInfo::MapleLogger() << "func: " << cgFunc->GetName() << "\n";
    for (auto *li : liveIntervalsArray) {
        if (li == nullptr || li->GetRegNO() == 0) {
            continue;
        }
        LogInfo::MapleLogger() << "vreg" << li->GetRegNO() << ": ";
        if (li->GetSplitParent() != nullptr) {
            PrintLiveRanges(*li->GetSplitParent());
        } else {
            PrintLiveRanges(*li);
        }
    }
}

/*
 * This is a support routine to compute the overlapping live intervals in graph form.
 * The output file can be viewed by gnuplot.
 * Despite the function name of LiveRanges, it is using live intervals.
 */
void LSRALinearScanRegAllocator::PrintLiveRangesGraph() const
{
    /* ================= Output to plot.pg =============== */
    std::ofstream out("plot.pg");
    CHECK_FATAL(out.is_open(), "Failed to open output file: plot.pg");
    std::streambuf *coutBuf = LogInfo::MapleLogger().rdbuf(); /* old buf */
    LogInfo::MapleLogger().rdbuf(out.rdbuf());                /* new buf */

    LogInfo::MapleLogger() << "#!/usr/bin/gnuplot\n";
    LogInfo::MapleLogger() << "#maxInsnNum " << maxInsnNum << "\n";
    LogInfo::MapleLogger() << "#minVregNum " << minVregNum << "\n";
    LogInfo::MapleLogger() << "#maxVregNum " << maxVregNum << "\n";
    LogInfo::MapleLogger() << "reset\nset terminal png\n";
    LogInfo::MapleLogger() << "set xrange [1:" << maxInsnNum << "]\n";
    LogInfo::MapleLogger() << "set grid\nset style data linespoints\n";
    LogInfo::MapleLogger() << "set datafile missing '0'\n";
    std::vector<std::vector<uint32>> graph(maxVregNum, std::vector<uint32>(maxInsnNum, 0));

    uint32 minY = 0xFFFFFFFF;
    uint32 maxY = 0;
    for (auto *li : liveIntervalsArray) {
        if (li == nullptr || li->GetRegNO() == 0) {
            continue;
        }
        uint32 regNO = li->GetRegNO();
        if ((li->GetLastUse() - li->GetFirstDef()) < kMinLiveIntervalLength) {
            continue;
        }
        if (regNO < minY) {
            minY = regNO;
        }
        if (regNO > maxY) {
            maxY = regNO;
        }
        uint32 n;
        for (n = 0; n <= (li->GetFirstDef() - 1); ++n) {
            graph[regNO - minVregNum][n] = 0;
        }
        if (li->GetLastUse() >= n) {
            for (; n <= (li->GetLastUse() - 1); ++n) {
                graph[regNO - minVregNum][n] = regNO;
            }
        }
        for (; n < maxInsnNum; ++n) {
            graph[regNO - minVregNum][n] = 0;
        }

        for (auto *bb : bfs->sortedBBs) {
            FOR_BB_INSNS(insn, bb) {
                const InsnDesc *md = insn->GetDesc();
                uint32 opndNum = insn->GetOperandSize();
                for (uint32 iSecond = 0; iSecond < opndNum; ++iSecond) {
                    Operand &opnd = insn->GetOperand(iSecond);
                    const OpndDesc *regProp = md->GetOpndDes(iSecond);
                    DEBUG_ASSERT(regProp != nullptr,
                                 "pointer is null in LSRALinearScanRegAllocator::PrintLiveRangesGraph");
                    bool isDef = regProp->IsRegDef();
                    if (opnd.IsList()) {
                        auto &listOpnd = static_cast<ListOperand &>(opnd);
                        for (auto op : listOpnd.GetOperands()) {
                            (void)CheckForReg(*op, *insn, *li, regNO, isDef);
                        }
                    } else if (opnd.IsMemoryAccessOperand()) {
                        auto &memOpnd = static_cast<MemOperand &>(opnd);
                        Operand *base = memOpnd.GetBaseRegister();
                        Operand *offset = memOpnd.GetIndexRegister();
                        if (base != nullptr && !CheckForReg(*base, *insn, *li, regNO, false)) {
                            continue;
                        }
                        if (offset != nullptr && !CheckForReg(*offset, *insn, *li, regNO, false)) {
                            continue;
                        }
                    } else {
                        (void)CheckForReg(opnd, *insn, *li, regNO, isDef);
                    }
                }
            }
        }
    }
    LogInfo::MapleLogger() << "set yrange [" << (minY - 1) << ":" << (maxY + 1) << "]\n";

    LogInfo::MapleLogger() << "plot \"plot.dat\" using 1:2 title \"R" << minVregNum << "\"";
    for (uint32 i = 1; i < ((maxVregNum - minVregNum) + 1); ++i) {
        LogInfo::MapleLogger() << ", \\\n\t\"\" using 1:" << (i + kDivide2) << " title \"R" << (minVregNum + i) << "\"";
    }
    LogInfo::MapleLogger() << ";\n";

    /* ================= Output to plot.dat =============== */
    std::ofstream out2("plot.dat");
    CHECK_FATAL(out2.is_open(), "Failed to open output file: plot.dat");
    LogInfo::MapleLogger().rdbuf(out2.rdbuf()); /* new buf */
    LogInfo::MapleLogger() << "##reg";
    for (uint32 i = minVregNum; i <= maxVregNum; ++i) {
        LogInfo::MapleLogger() << " R" << i;
    }
    LogInfo::MapleLogger() << "\n";
    for (uint32 n = 0; n < maxInsnNum; ++n) {
        LogInfo::MapleLogger() << (n + 1);
        for (uint32 i = minVregNum; i <= maxVregNum; ++i) {
            LogInfo::MapleLogger() << " " << graph[i - minVregNum][n];
        }
        LogInfo::MapleLogger() << "\n";
    }
    LogInfo::MapleLogger().rdbuf(coutBuf);
}

void LSRALinearScanRegAllocator::SpillStackMapInfo()
{
    const auto &referenceMapInsns = cgFunc->GetStackMapInsns();

    for (auto *li : liveIntervalsArray) {
        if (li == nullptr) {
            continue;
        }

        int32 regNO = li->GetRegNO();
        bool toSpill = false;
        for (auto *insn : referenceMapInsns) {
            auto &deoptInfo = insn->GetStackMap()->GetDeoptInfo();
            const auto &deoptBundleInfo = deoptInfo.GetDeoptBundleInfo();
            for (const auto &item : deoptBundleInfo) {
                const auto *opnd = item.second;
                if (opnd->IsRegister() && (regNO == static_cast<const RegOperand *>(opnd)->GetRegisterNumber())) {
                    li->SetStackSlot(kSpilled);
                    li->SetShouldSave(false);
                    toSpill = true;
                    break;
                }
            }
            if (toSpill) {
                break;
            }
            if (!cgFunc->IsRegReference(regNO)) {
                continue;
            }
            auto *stackMapLiveIn = insn->GetStackMapLiveIn();
            if (stackMapLiveIn->GetInfo().count(regNO) != 0) {
                li->SetStackSlot(kSpilled);
                li->SetShouldSave(false);
                break;
            }
        }
    }
}

void LSRALinearScanRegAllocator::PrintLiveInterval(const LiveInterval &li, const std::string &str) const
{
    LogInfo::MapleLogger() << str << "\n";
    if (li.GetIsCall() != nullptr) {
        LogInfo::MapleLogger() << " firstDef " << li.GetFirstDef();
        LogInfo::MapleLogger() << " isCall";
    } else if (li.GetPhysUse()) {
        LogInfo::MapleLogger() << "\tregNO " << li.GetRegNO();
        LogInfo::MapleLogger() << " firstDef " << li.GetFirstDef();
        LogInfo::MapleLogger() << " physUse " << li.GetPhysUse();
        LogInfo::MapleLogger() << " endByCall " << li.IsEndByCall();
    } else {
        /* show regno/firstDef/lastUse with 5/8/8 width respectively */
        LogInfo::MapleLogger() << "\tregNO " << std::setw(5) << li.GetRegNO();
        LogInfo::MapleLogger() << " firstDef " << std::setw(8) << li.GetFirstDef();
        LogInfo::MapleLogger() << " lastUse " << std::setw(8) << li.GetLastUse();
        LogInfo::MapleLogger() << " assigned " << li.GetAssignedReg();
        LogInfo::MapleLogger() << " refCount " << li.GetRefCount();
        LogInfo::MapleLogger() << " priority " << li.GetPriority();
    }
    LogInfo::MapleLogger() << " object_address 0x" << std::hex << &li << std::dec << "\n";
}

void LSRALinearScanRegAllocator::PrintParamQueue(const std::string &str)
{
    LogInfo::MapleLogger() << str << "\n";
    for (SingleQue &que : intParamQueue) {
        if (que.empty()) {
            continue;
        }
        LiveInterval *li = que.front();
        LiveInterval *last = que.back();
        PrintLiveInterval(*li, "");
        while (li != last) {
            que.pop_front();
            que.push_back(li);
            li = que.front();
            PrintLiveInterval(*li, "");
        }
        que.pop_front();
        que.push_back(li);
    }
}

void LSRALinearScanRegAllocator::PrintCallQueue(const std::string &str) const
{
    LogInfo::MapleLogger() << str << "\n";
    for (auto callInsnID : callQueue) {
        LogInfo::MapleLogger() << callInsnID << " ";
    }
    LogInfo::MapleLogger() << "\n";
}

void LSRALinearScanRegAllocator::PrintActiveList(const std::string &str, uint32 len) const
{
    uint32 count = 0;
    LogInfo::MapleLogger() << str << " " << active.size() << "\n";
    for (auto *li : active) {
        PrintLiveInterval(*li, "");
        ++count;
        if ((len != 0) && (count == len)) {
            break;
        }
    }
}

void LSRALinearScanRegAllocator::PrintActiveListSimple() const
{
    for (const auto *li : active) {
        uint32 assignedReg = li->GetAssignedReg();
        LogInfo::MapleLogger() << li->GetRegNO() << "(" << assignedReg << ", ";
        if (li->GetPhysUse()) {
            LogInfo::MapleLogger() << "p) ";
        } else {
            LogInfo::MapleLogger() << li->GetFirstAcrossedCall();
        }
        LogInfo::MapleLogger() << "<" << li->GetFirstDef() << "," << li->GetLastUse() << ">) ";
    }
    LogInfo::MapleLogger() << "\n";
}

void LSRALinearScanRegAllocator::PrintLiveIntervals() const
{
    /* vreg LogInfo */
    for (auto *li : liveIntervalsArray) {
        if (li == nullptr || li->GetRegNO() == 0) {
            continue;
        }
        PrintLiveInterval(*li, "");
    }
    LogInfo::MapleLogger() << "\n";
    /* preg LogInfo */
    for (auto param : intParamQueue) {
        for (auto *li : param) {
            if (li == nullptr || li->GetRegNO() == 0) {
                continue;
            }
            PrintLiveInterval(*li, "");
        }
    }
    LogInfo::MapleLogger() << "\n";
}

void LSRALinearScanRegAllocator::DebugCheckActiveList() const
{
    LiveInterval *prev = nullptr;
    for (auto *li : active) {
        if (prev != nullptr) {
            if ((li->GetRegNO() <= regInfo->GetLastParamsFpReg()) &&
                (prev->GetRegNO() > regInfo->GetLastParamsFpReg())) {
                if (li->GetFirstDef() < prev->GetFirstDef()) {
                    LogInfo::MapleLogger() << "ERRer: active list with out of order phys + vreg\n";
                    PrintLiveInterval(*prev, "prev");
                    PrintLiveInterval(*li, "current");
                    PrintActiveList("Active", kPrintedActiveListLength);
                }
            }
            if ((li->GetRegNO() <= regInfo->GetLastParamsFpReg()) &&
                (prev->GetRegNO() <= regInfo->GetLastParamsFpReg())) {
                if (li->GetFirstDef() < prev->GetFirstDef()) {
                    LogInfo::MapleLogger() << "ERRer: active list with out of order phys reg use\n";
                    PrintLiveInterval(*prev, "prev");
                    PrintLiveInterval(*li, "current");
                    PrintActiveList("Active", kPrintedActiveListLength);
                }
            }
        } else {
            prev = li;
        }
    }
}

/*
 * Prepare the free physical register pool for allocation.
 * When a physical register is allocated, it is removed from the pool.
 * The physical register is re-inserted into the pool when the associated live
 * interval has ended.
 */
void LSRALinearScanRegAllocator::InitFreeRegPool()
{
    for (regno_t regNO = regInfo->GetInvalidReg(); regNO < regInfo->GetAllRegNum(); ++regNO) {
        if (!regInfo->IsAvailableReg(regNO)) {
            continue;
        }
        if (regInfo->IsGPRegister(regNO)) {
            if (regInfo->IsYieldPointReg(regNO)) {
                continue;
            }
            /* ExtraSpillReg */
            if (regInfo->IsSpillRegInRA(regNO, needExtraSpillReg)) {
                intSpillRegSet.push_back(regNO - firstIntReg);
                continue;
            }
            if (regInfo->IsCalleeSavedReg(regNO)) {
                /* callee-saved registers */
                (void)intCalleeRegSet.insert(regNO - firstIntReg);
                intCalleeMask |= 1u << (regNO - firstIntReg);
            } else {
                /* caller-saved registers */
                (void)intCallerRegSet.insert(regNO - firstIntReg);
                intCallerMask |= 1u << (regNO - firstIntReg);
            }
        } else {
            /* fp ExtraSpillReg */
            if (regInfo->IsSpillRegInRA(regNO, needExtraSpillReg)) {
                fpSpillRegSet.push_back(regNO - firstFpReg);
                continue;
            }
            if (regInfo->IsCalleeSavedReg(regNO)) {
                /* fp callee-saved registers */
                (void)fpCalleeRegSet.insert(regNO - firstFpReg);
                fpCalleeMask |= 1u << (regNO - firstFpReg);
            } else {
                /* fp caller-saved registers */
                (void)fpCallerRegSet.insert(regNO - firstFpReg);
                fpCallerMask |= 1u << (regNO - firstFpReg);
            }
        }
    }

    if (LSRA_DUMP) {
        PrintRegSet(intCallerRegSet, "ALLOCATABLE_INT_CALLER");
        PrintRegSet(intCalleeRegSet, "ALLOCATABLE_INT_CALLEE");
        PrintRegSet(intParamRegSet, "ALLOCATABLE_INT_PARAM");
        PrintRegSet(fpCallerRegSet, "ALLOCATABLE_FP_CALLER");
        PrintRegSet(fpCalleeRegSet, "ALLOCATABLE_FP_CALLEE");
        PrintRegSet(fpParamRegSet, "ALLOCATABLE_FP_PARAM");
        LogInfo::MapleLogger() << "INT_SPILL_REGS";
        for (uint32 intSpillRegNO : intSpillRegSet) {
            LogInfo::MapleLogger() << " " << intSpillRegNO;
        }
        LogInfo::MapleLogger() << "\n";
        LogInfo::MapleLogger() << "FP_SPILL_REGS";
        for (uint32 fpSpillRegNO : fpSpillRegSet) {
            LogInfo::MapleLogger() << " " << fpSpillRegNO;
        }
        LogInfo::MapleLogger() << "\n";
        LogInfo::MapleLogger() << std::hex;
        LogInfo::MapleLogger() << "INT_CALLER_MASK " << intCallerMask << "\n";
        LogInfo::MapleLogger() << "INT_CALLEE_MASK " << intCalleeMask << "\n";
        LogInfo::MapleLogger() << "INT_PARAM_MASK " << intParamMask << "\n";
        LogInfo::MapleLogger() << "FP_CALLER_FP_MASK " << fpCallerMask << "\n";
        LogInfo::MapleLogger() << "FP_CALLEE_FP_MASK " << fpCalleeMask << "\n";
        LogInfo::MapleLogger() << "FP_PARAM_FP_MASK " << fpParamMask << "\n";
        LogInfo::MapleLogger() << std::dec;
    }
}

void LSRALinearScanRegAllocator::RecordPhysRegs(const RegOperand &regOpnd, uint32 insnNum, bool isDef)
{
    RegType regType = regOpnd.GetRegisterType();
    uint32 regNO = regOpnd.GetRegisterNumber();
    if (regType == kRegTyCc || regType == kRegTyVary) {
        return;
    }
    if (regInfo->IsUntouchableReg(regNO)) {
        return;
    }
    if (!regInfo->IsPreAssignedReg(regNO)) {
        return;
    }
    if (isDef) {
        /* parameter/return register def is assumed to be live until a call. */
        auto *li = memPool->New<LiveInterval>(*memPool);
        li->SetRegNO(regNO);
        li->SetRegType(regType);
        li->SetStackSlot(0xFFFFFFFF);
        li->SetFirstDef(insnNum);
        li->SetPhysUse(insnNum);
        li->SetAssignedReg(regNO);

        if (regType == kRegTyInt) {
            intParamQueue[regInfo->GetIntParamRegIdx(regNO)].push_back(li);
        } else {
            fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].push_back(li);
        }
    } else {
        if (regType == kRegTyInt) {
            CHECK_FATAL(!intParamQueue[regInfo->GetIntParamRegIdx(regNO)].empty(),
                        "was not defined before use, impossible");
            LiveInterval *li = intParamQueue[regInfo->GetIntParamRegIdx(regNO)].back();
            li->SetPhysUse(insnNum);
        } else {
            CHECK_FATAL(!fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].empty(),
                        "was not defined before use, impossible");
            LiveInterval *li = fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].back();
            li->SetPhysUse(insnNum);
        }
    }
}

void LSRALinearScanRegAllocator::UpdateLiveIntervalState(const BB &bb, LiveInterval &li) const
{
    if (bb.IsCatch()) {
        li.SetInCatchState();
    } else {
        li.SetNotInCatchState();
    }

    if (bb.GetInternalFlag1()) {
        li.SetInCleanupState();
    } else {
        li.SetNotInCleanupState(bb.GetId() == 1);
    }
}

void LSRALinearScanRegAllocator::UpdateRegUsedInfo(LiveInterval &li, regno_t regNO)
{
    uint32 index = regNO / (sizeof(uint64) * k8ByteSize);
    uint64 bit = regNO % (sizeof(uint64) * k8ByteSize);
    if ((regUsedInBB[index] & (static_cast<uint64>(1) << bit)) != 0) {
        li.SetMultiUseInBB(true);
    }
    regUsedInBB[index] |= (static_cast<uint64>(1) << bit);

    if (minVregNum > regNO) {
        minVregNum = regNO;
    }
    if (maxVregNum < regNO) {
        maxVregNum = regNO;
    }
}

/* main entry function for live interval computation. */
void LSRALinearScanRegAllocator::SetupLiveInterval(Operand &opnd, Insn &insn, bool isDef, uint32 &nUses)
{
    if (!opnd.IsRegister()) {
        return;
    }
    auto &regOpnd = static_cast<RegOperand &>(opnd);
    uint32 insnNum = insn.GetId();
    if (regOpnd.IsPhysicalRegister()) {
        RecordPhysRegs(regOpnd, insnNum, isDef);
        return;
    }
    RegType regType = regOpnd.GetRegisterType();
    if (regType == kRegTyCc || regType == kRegTyVary) {
        return;
    }

    LiveInterval *li = nullptr;
    uint32 regNO = regOpnd.GetRegisterNumber();
    if (liveIntervalsArray[regNO] == nullptr) {
        li = memPool->New<LiveInterval>(*memPool);
        li->SetRegNO(regNO);
        li->SetStackSlot(0xFFFFFFFF);
        liveIntervalsArray[regNO] = li;
        liQue.push_back(li);
    } else {
        li = liveIntervalsArray[regNO];
    }
    li->SetRegType(regType);

    BB *curBB = insn.GetBB();
    if (isDef) {
        /* never set to 0 before, why consider this condition ? */
        if (li->GetFirstDef() == 0) {
            li->SetFirstDef(insnNum);
            li->SetLastUse(insnNum + 1);
        } else if (!curBB->IsUnreachable()) {
            if (li->GetLastUse() < insnNum || li->IsUseBeforeDef()) {
                li->SetLastUse(insnNum + 1);
            }
        }
        /*
         * try-catch related
         *   Not set when extending live interval with bb's livein in ComputeLiveInterval.
         */
        li->SetResultCount(li->GetResultCount() + 1);
    } else {
        if (li->GetFirstDef() == 0) {
            DEBUG_ASSERT(false, "SetupLiveInterval: use before def");
        }
        /*
         * In ComputeLiveInterval when extending live interval using
         * live-out information, li created does not have a type.
         */
        if (!curBB->IsUnreachable()) {
            li->SetLastUse(insnNum);
        }
        ++nUses;
    }
    UpdateLiveIntervalState(*curBB, *li);

    li->SetRefCount(li->GetRefCount() + 1);
    li->AddUsePositions(insnNum);
    UpdateRegUsedInfo(*li, regNO);

    /* setup the def/use point for it */
    DEBUG_ASSERT(regNO < liveIntervalsArray.size(), "out of range of vector liveIntervalsArray");
}

/*
 * Support 'hole' in LSRA.
 * For a live interval, there might be multiple segments of live ranges,
 * and between these segments a 'hole'.
 * Some other short lived vreg can go into these 'holes'.
 *
 * from : starting instruction sequence id
 * to   : ending instruction sequence id
 */
void LSRALinearScanRegAllocator::LiveInterval::AddRange(uint32 from, uint32 to)
{
    if (ranges.empty()) {
        ranges.emplace_back(LinearRange(from, to));
        return;
    }
    /* create a new range */
    if (to < ranges.front().GetStart()) {
        (void)ranges.insert(ranges.begin(), LinearRange(from, to));
        return;
    }
    DEBUG_ASSERT(from <= ranges.front().GetEnd(), "No possible on reverse traverse.");
    if (to >= ranges.front().GetEnd() && from < ranges.front().GetStart()) {
        ranges.front().SetStart(from);
        ranges.front().SetEnd(to);
        return;
    }
    /* extend it's range forward. e.g. def-use opnd */
    if (to >= ranges.front().GetStart() && from < ranges.front().GetStart()) {
        ranges.front().SetStart(from);
        return;
    }
    return;
}

/* See if a vreg can fit in one of the holes of a longer live interval. */
uint32 LSRALinearScanRegAllocator::FillInHole(const LiveInterval &li)
{
    MapleSet<LiveInterval *, ActiveCmp>::iterator it;
    for (it = active.begin(); it != active.end(); ++it) {
        auto *ili = static_cast<LiveInterval *>(*it);

        /*
         * If ili is part in cleanup, the hole info will be not correct,
         * since cleanup bb do not have edge to normal func bb, and the
         * live-out info will not correct.
         */
        if (!ili->IsAllOutCleanup() || ili->IsAllInCatch()) {
            continue;
        }

        if (ili->GetRegType() != li.GetRegType() || ili->GetStackSlot() != 0xFFFFFFFF || ili->GetLiChild() != nullptr ||
            ili->GetAssignedReg() == 0) {
            continue;
        }
        /* todo: find available holes in ili->GetRanges() */
    }
    return 0;
}

uint32 LSRALinearScanRegAllocator::LiveInterval::GetUsePosAfter(uint32 pos) const
{
    for (auto usePos : usePositions) {
        if (usePos > pos) {
            return usePos;
        }
    }
    return 0;
}

MapleVector<LSRALinearScanRegAllocator::LinearRange>::iterator LSRALinearScanRegAllocator::LiveInterval::FindPosRange(
    uint32 pos)
{
    while (rangeFinder != ranges.end()) {
        if (rangeFinder->GetEnd() > pos) {
            break;
        }
        ++rangeFinder;
    }
    return rangeFinder;
}

void LSRALinearScanRegAllocator::SetupIntervalRangesByOperand(Operand &opnd, const Insn &insn, uint32 blockFrom,
                                                              bool isDef)
{
    auto &regOpnd = static_cast<RegOperand &>(opnd);
    RegType regType = regOpnd.GetRegisterType();
    if (regType == kRegTyCc || regType == kRegTyVary) {
        return;
    }
    regno_t regNO = regOpnd.GetRegisterNumber();
    if (regNO <= regInfo->GetAllRegNum()) {
        return;
    }
    if (!isDef) {
        liveIntervalsArray[regNO]->AddRange(blockFrom, insn.GetId());
        return;
    }
    if (liveIntervalsArray[regNO]->GetRanges().empty()) {
        liveIntervalsArray[regNO]->AddRange(insn.GetId(), insn.GetId());
    } else {
        liveIntervalsArray[regNO]->GetRanges().front().SetStart(insn.GetId());
    }
    liveIntervalsArray[regNO]->AddUsePositions(insn.GetId());
}

void LSRALinearScanRegAllocator::BuildIntervalRangesForEachOperand(const Insn &insn, uint32 blockFrom)
{
    const InsnDesc *md = insn.GetDesc();
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn.GetOperand(i);
        if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            Operand *base = memOpnd.GetBaseRegister();
            Operand *offset = memOpnd.GetIndexRegister();
            if (base != nullptr && base->IsRegister()) {
                SetupIntervalRangesByOperand(*base, insn, blockFrom, false);
            }
            if (offset != nullptr && offset->IsRegister()) {
                SetupIntervalRangesByOperand(*offset, insn, blockFrom, false);
            }
        } else if (opnd.IsRegister()) {
            const OpndDesc *regProp = md->GetOpndDes(i);
            DEBUG_ASSERT(regProp != nullptr,
                         "pointer is null in LSRALinearScanRegAllocator::BuildIntervalRangesForEachOperand");
            bool isDef = regProp->IsRegDef();
            SetupIntervalRangesByOperand(opnd, insn, blockFrom, isDef);
        }
    }
}

/* Support finding holes by searching for ranges where holes exist. */
void LSRALinearScanRegAllocator::BuildIntervalRanges()
{
    size_t bbIdx = bfs->sortedBBs.size();
    if (bbIdx == 0) {
        return;
    }

    do {
        --bbIdx;
        BB *bb = bfs->sortedBBs[bbIdx];
        if (bb->GetFirstInsn() == nullptr || bb->GetLastInsn() == nullptr) {
            continue;
        }
        uint32 blockFrom = bb->GetFirstInsn()->GetId();
        uint32 blockTo = bb->GetLastInsn()->GetId() + 1;

        for (auto regNO : bb->GetLiveOutRegNO()) {
            if (regNO < regInfo->GetAllRegNum()) {
                /* Do not consider physical regs. */
                continue;
            }
            liveIntervalsArray[regNO]->AddRange(blockFrom, blockTo);
        }

        FOR_BB_INSNS_REV(insn, bb) {
            BuildIntervalRangesForEachOperand(*insn, blockFrom);
        }
    } while (bbIdx != 0);
}

/* Extend live interval with live-in info */
void LSRALinearScanRegAllocator::UpdateLiveIntervalByLiveIn(const BB &bb, uint32 insnNum)
{
    for (const auto &regNO : bb.GetLiveInRegNO()) {
        if (!regInfo->IsVirtualRegister(regNO)) {
            /* Do not consider physical regs. */
            continue;
        }
        DEBUG_ASSERT(regNO < liveIntervalsArray.size(), "index out of range.");
        LiveInterval *liOuter = liveIntervalsArray[regNO];
        if (liOuter != nullptr || (bb.IsEmpty() && bb.GetId() != 1)) {
            continue;
        }
        /*
         * try-catch related
         *   Since it is livein but not seen before, its a use before def
         *   spill it temporarily
         */
        auto *li = memPool->New<LiveInterval>(*memPool);
        li->SetRegNO(regNO);
        li->SetStackSlot(kSpilled);
        liveIntervalsArray[regNO] = li;
        li->SetFirstDef(insnNum);
        liQue.push_back(li);

        li->SetUseBeforeDef(true);

        if (!bb.IsUnreachable()) {
            if (bb.GetId() != 1) {
                LogInfo::MapleLogger() << "ERROR: " << regNO << " use before def in bb " << bb.GetId() << " : "
                                       << cgFunc->GetName() << "\n";
                DEBUG_ASSERT(false, "There should only be [use before def in bb 1], temporarily.");
            }
            LogInfo::MapleLogger() << "WARNING: " << regNO << " use before def in bb " << bb.GetId() << " : "
                                   << cgFunc->GetName() << "\n";
        }
        UpdateLiveIntervalState(bb, *li);
    }
}

/* traverse live in regNO, for each live in regNO create a new liveinterval */
void LSRALinearScanRegAllocator::UpdateParamLiveIntervalByLiveIn(const BB &bb, uint32 insnNum)
{
    for (const auto &regNO : bb.GetLiveInRegNO()) {
        if (!regInfo->IsPreAssignedReg(regNO)) {
            continue;
        }
        auto *li = memPool->New<LiveInterval>(*memPool);
        li->SetRegNO(regNO);
        li->SetStackSlot(0xFFFFFFFF);
        li->SetFirstDef(insnNum);
        li->SetPhysUse(insnNum);
        li->SetAssignedReg(regNO);

        if (regInfo->IsGPRegister(regNO)) {
            li->SetRegType(kRegTyInt);
            intParamQueue[regInfo->GetIntParamRegIdx(regNO)].push_back(li);
        } else {
            li->SetRegType(kRegTyFloat);
            fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].push_back(li);
        }
        UpdateLiveIntervalState(bb, *li);
    }
}

void LSRALinearScanRegAllocator::ComputeLiveIn(BB &bb, uint32 insnNum)
{
    if (bb.IsEmpty() && bb.GetId() != 1) {
        return;
    }
    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "bb(" << bb.GetId() << ")LIVEOUT:";
        for (const auto &liveOutRegNO : bb.GetLiveOutRegNO()) {
            LogInfo::MapleLogger() << " " << liveOutRegNO;
        }
        LogInfo::MapleLogger() << ".\n";
        LogInfo::MapleLogger() << "bb(" << bb.GetId() << ")LIVEIN:";
        for (const auto &liveInRegNO : bb.GetLiveInRegNO()) {
            LogInfo::MapleLogger() << " " << liveInRegNO;
        }
        LogInfo::MapleLogger() << ".\n";
    }

    UpdateLiveIntervalByLiveIn(bb, insnNum);

    if (bb.GetFirstInsn() == nullptr) {
        return;
    }
    if (!bb.GetEhPreds().empty()) {
        bb.InsertLiveInRegNO(firstIntReg);
        bb.InsertLiveInRegNO(firstIntReg + 1);
    }
    UpdateParamLiveIntervalByLiveIn(bb, insnNum);
    if (!bb.GetEhPreds().empty()) {
        bb.EraseLiveInRegNO(firstIntReg);
        bb.EraseLiveInRegNO(firstIntReg + 1);
    }
}

void LSRALinearScanRegAllocator::ComputeLiveOut(BB &bb, uint32 insnNum)
{
    /*
     *  traverse live out regNO
     *  for each live out regNO if the last corresponding live interval is created within this bb
     *  update this lastUse of li to the end of BB
     */
    for (const auto &regNO : bb.GetLiveOutRegNO()) {
        if (regInfo->IsPreAssignedReg(static_cast<regno_t>(regNO))) {
            LiveInterval *liOut = nullptr;
            if (regInfo->IsGPRegister(regNO)) {
                if (intParamQueue[regInfo->GetIntParamRegIdx(regNO)].empty()) {
                    continue;
                }
                liOut = intParamQueue[regInfo->GetIntParamRegIdx(regNO)].back();
                if (bb.GetFirstInsn() && liOut->GetFirstDef() >= bb.GetFirstInsn()->GetId()) {
                    liOut->SetPhysUse(insnNum);
                }
            } else {
                if (fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].empty()) {
                    continue;
                }
                liOut = fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].back();
                if (bb.GetFirstInsn() && liOut->GetFirstDef() >= bb.GetFirstInsn()->GetId()) {
                    liOut->SetPhysUse(insnNum);
                }
            }
        }
        /* Extend live interval with live-out info */
        LiveInterval *li = liveIntervalsArray[regNO];
        if (li != nullptr && !bb.IsEmpty()) {
            li->SetLastUse(bb.GetLastInsn()->GetId());
            UpdateLiveIntervalState(bb, *li);
            if (bb.GetKind() == BB::kBBRangeGoto) {
                li->SetSplitForbid(true);
            }
        }
    }
}

void LSRALinearScanRegAllocator::ComputeLiveIntervalForEachOperand(Insn &insn)
{
    uint32 numUses = 0;
    const InsnDesc *md = insn.GetDesc();
    uint32 opndNum = insn.GetOperandSize();
    /*
     * we need to process src opnd first just in case the src/dest vreg are the same and the src vreg belongs to the
     * last interval.
     */
    for (int32 i = opndNum - 1; i >= 0; --i) {
        Operand &opnd = insn.GetOperand(static_cast<uint32>(i));
        const OpndDesc *opndDesc = md->GetOpndDes(i);
        DEBUG_ASSERT(opndDesc != nullptr, "ptr null check.");
        if (opnd.IsList()) {
            auto &listOpnd = static_cast<ListOperand &>(opnd);
            for (auto op : listOpnd.GetOperands()) {
                SetupLiveInterval(*op, insn, opndDesc->IsDef(), numUses);
            }
        } else if (opnd.IsMemoryAccessOperand()) {
            bool isDef = false;
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            Operand *base = memOpnd.GetBaseRegister();
            Operand *offset = memOpnd.GetIndexRegister();
            if (base != nullptr) {
                SetupLiveInterval(*base, insn, isDef, numUses);
            }
            if (offset != nullptr) {
                SetupLiveInterval(*offset, insn, isDef, numUses);
            }
        } else {
            /* Specifically, the "use-def" opnd is treated as a "use" opnd */
            bool isUse = opndDesc->IsRegUse();
            SetupLiveInterval(opnd, insn, !isUse, numUses);
        }
    }
    if (numUses >= regInfo->GetNormalUseOperandNum()) {
        needExtraSpillReg = true;
    }
}

void LSRALinearScanRegAllocator::ComputeLoopLiveIntervalPriority(const CGFuncLoops &loop)
{
    for (const auto *lp : loop.GetInnerLoops()) {
        /* handle nested Loops */
        ComputeLoopLiveIntervalPriority(*lp);
    }
    for (auto *bb : loop.GetLoopMembers()) {
        if (bb->IsEmpty()) {
            continue;
        }
        FOR_BB_INSNS(insn, bb) {
            ComputeLoopLiveIntervalPriorityInInsn(*insn);
        }
        loopBBRegSet.clear();
    }
}

void LSRALinearScanRegAllocator::ComputeLoopLiveIntervalPriorityInInsn(const Insn &insn)
{
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn.GetOperand(i);
        if (!opnd.IsRegister()) {
            continue;
        }
        auto &regOpnd = static_cast<RegOperand &>(opnd);
        if (regOpnd.IsPhysicalRegister()) {
            continue;
        }
        uint32 regNo = regOpnd.GetRegisterNumber();
        LiveInterval *li = liveIntervalsArray[regNo];
        if (li == nullptr || loopBBRegSet.find(regNo) != loopBBRegSet.end()) {
            continue;
        }
        li->SetPriority(kLoopWeight * li->GetPriority());
        (void)loopBBRegSet.insert(regNo);
    }
    return;
}

void LSRALinearScanRegAllocator::ComputeLiveInterval()
{
    liQue.clear();
    uint32 regUsedInBBSz = (cgFunc->GetMaxVReg() / (sizeof(uint64) * k8ByteSize) + 1);
    regUsedInBB.resize(regUsedInBBSz, 0);
    uint32 insnNum = 1;
    for (BB *bb : bfs->sortedBBs) {
        ComputeLiveIn(*bb, insnNum);
        FOR_BB_INSNS(insn, bb) {
            insn->SetId(insnNum);
            /* skip comment and debug insn */
            if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction()) {
                continue;
            }

            /* RecordCall, remember calls for caller/callee allocation. */
            if (insn->IsCall()) {
                if (!insn->GetIsThrow() || !bb->GetEhSuccs().empty()) {
                    callQueue.emplace_back(insn->GetId());
                }
            }

            ComputeLiveIntervalForEachOperand(*insn);

            /* handle return value for call insn */
            if (insn->IsCall()) {
                /* For all backend architectures so far, adopt all RetRegs as Def via this insn,
                 * and then their live begins.
                 * next optimization, you can determine which registers are actually used.
                 */
                RegOperand *retReg = nullptr;
                if (insn->GetRetType() == Insn::kRegInt) {
                    for (int i = 0; i < regInfo->GetIntRetRegsNum(); i++) {
                        retReg = regInfo->GetOrCreatePhyRegOperand(regInfo->GetIntRetReg(i), k64BitSize, kRegTyInt);
                        RecordPhysRegs(*retReg, insnNum, true);
                    }
                } else {
                    for (int i = 0; i < regInfo->GetFpRetRegsNum(); i++) {
                        retReg = regInfo->GetOrCreatePhyRegOperand(regInfo->GetFpRetReg(i), k64BitSize, kRegTyFloat);
                        RecordPhysRegs(*retReg, insnNum, true);
                    }
                }
            }
            ++insnNum;
        }

        ComputeLiveOut(*bb, insnNum);
    }

    maxInsnNum = insnNum - 1; /* insn_num started from 1 */
    regUsedInBB.clear();
    /* calculate Live Interval weight */
    for (auto *li : liveIntervalsArray) {
        if (li == nullptr || li->GetRegNO() == 0) {
            continue;
        }
        if (li->GetIsCall() != nullptr || li->GetPhysUse()) {
            continue;
        }
        if (li->GetLastUse() > li->GetFirstDef()) {
            li->SetPriority(static_cast<float>(li->GetRefCount()) /
                            static_cast<float>(li->GetLastUse() - li->GetFirstDef()));
        } else {
            li->SetPriority(static_cast<float>(li->GetRefCount()) /
                            static_cast<float>(li->GetFirstDef() - li->GetLastUse()));
        }
    }

    /* enhance loop Live Interval Priority */
    if (!cgFunc->GetLoops().empty()) {
        for (const auto *lp : cgFunc->GetLoops()) {
            ComputeLoopLiveIntervalPriority(*lp);
        }
    }

    if (LSRA_DUMP) {
        PrintLiveIntervals();
    }
}

/* Calculate the weight of a live interval for pre-spill and flexible spill */
void LSRALinearScanRegAllocator::LiveIntervalAnalysis()
{
    for (uint32 bbIdx = 0; bbIdx < bfs->sortedBBs.size(); ++bbIdx) {
        BB *bb = bfs->sortedBBs[bbIdx];

        FOR_BB_INSNS(insn, bb) {
            /* 1 calculate live interfere */
            if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction() || insn->GetId() == 0) {
                /* New instruction inserted by reg alloc (ie spill) */
                continue;
            }
            /* 1.1 simple retire from active */
            MapleSet<LiveInterval *, ActiveCmp>::iterator it;
            for (it = active.begin(); it != active.end(); /* erase will update */) {
                auto *li = static_cast<LiveInterval *>(*it);
                if (li->GetLastUse() > insn->GetId()) {
                    break;
                }
                it = active.erase(it);
            }
            const InsnDesc *md = insn->GetDesc();
            uint32 opndNum = insn->GetOperandSize();
            for (uint32 i = 0; i < opndNum; ++i) {
                const OpndDesc *regProp = md->GetOpndDes(i);
                DEBUG_ASSERT(regProp != nullptr, "pointer is null in LSRALinearScanRegAllocator::LiveIntervalAnalysis");
                bool isDef = regProp->IsRegDef();
                Operand &opnd = insn->GetOperand(i);
                if (isDef) {
                    auto &regOpnd = static_cast<RegOperand &>(opnd);
                    if (regOpnd.IsVirtualRegister() && regOpnd.GetRegisterType() != kRegTyCc) {
                        /* 1.2 simple insert to active */
                        uint32 regNO = regOpnd.GetRegisterNumber();
                        LiveInterval *li = liveIntervalsArray[regNO];
                        if (li->GetFirstDef() == insn->GetId()) {
                            (void)active.insert(li);
                        }
                    }
                }
            }

            /* 2 get interfere info, and analysis */
            uint32 interNum = active.size();
            if (LSRA_DUMP) {
                LogInfo::MapleLogger() << "In insn " << insn->GetId() << ", " << interNum
                                       << " overlap live intervals.\n";
                LogInfo::MapleLogger() << "\n";
            }

            /* 2.2 interfere with each other, analysis which to spill */
            while (interNum > CGOptions::GetOverlapNum()) {
                LiveInterval *lowestLi = nullptr;
                FindLowestPrioInActive(lowestLi);
                if (lowestLi != nullptr) {
                    if (LSRA_DUMP) {
                        PrintLiveInterval(*lowestLi, "Pre spilled: ");
                    }
                    lowestLi->SetStackSlot(kSpilled);
                    lowestLi->SetShouldSave(false);
                    active.erase(itFinded);
                    interNum = active.size();
                } else {
                    break;
                }
            }
        }
    }
    active.clear();
}

void LSRALinearScanRegAllocator::UpdateCallQueueAtRetirement(uint32 insnID)
{
    /*
     * active list is sorted based on increasing lastUse
     * any operand whose use is greater than current
     * instruction number is still in use.
     * If the use is less than or equal to instruction number
     * then it is possible to retire this live interval and
     * reclaim the physical register associated with it.
     */
    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "RetireActiveByInsn instr_num " << insnID << "\n";
    }
    /* Retire invalidated call from call queue */
    while (!callQueue.empty() && callQueue.front() <= insnID) {
        callQueue.pop_front();
    }
}

/* update allocate info by active queue */
void LSRALinearScanRegAllocator::UpdateActiveAllocateInfo(const LiveInterval &li)
{
    uint32 start = li.GetFirstDef();
    uint32 end = li.GetLastUse();
    if (li.GetSplitParent() != nullptr || li.IsUseBeforeDef()) {
        --start;
    }
    for (auto *activeLi : active) {
        uint32 regNO = activeLi->GetAssignedReg();
        uint32 rangeStartPos;
        auto posRange = activeLi->FindPosRange(start);
        if (posRange == activeLi->GetRanges().end()) {
            /* handle splited li */
            uint32 splitSafePos = activeLi->GetSplitPos();
            if (splitSafePos == li.GetFirstDef() && (li.GetSplitParent() != nullptr || li.IsUseBeforeDef())) {
                rangeStartPos = 0;
            } else if (splitSafePos > li.GetFirstDef()) {
                rangeStartPos = splitSafePos - 1;
            } else {
                rangeStartPos = 0XFFFFFFFUL;
            }
        } else if (posRange->GetEhStart() != 0 && posRange->GetEhStart() < posRange->GetStart()) {
            rangeStartPos = posRange->GetEhStart();
        } else {
            rangeStartPos = posRange->GetStart();
        }
        if (rangeStartPos > li.GetFirstDef()) {
            if (rangeStartPos < end) {
                blockForbiddenMask |= (1UL << activeLi->GetAssignedReg());
            }
            if (rangeStartPos < freeUntilPos[regNO]) {
                freeUntilPos[regNO] = rangeStartPos;
            }
        } else {
            freeUntilPos[regNO] = 0;
        }
    }
}

/* update allocate info by param queue */
void LSRALinearScanRegAllocator::UpdateParamAllocateInfo(const LiveInterval &li)
{
    bool isInt = (li.GetRegType() == kRegTyInt);
    MapleVector<SingleQue> &paramQueue = isInt ? intParamQueue : fpParamQueue;
    uint32 baseReg = isInt ? firstIntReg : firstFpReg;
    uint32 paramNum = isInt ? regInfo->GetIntRegs().size() : regInfo->GetFpRegs().size();
    uint32 start = li.GetFirstDef();
    uint32 end = li.GetLastUse();
    for (uint32 i = 0; i < paramNum; ++i) {
        while (!paramQueue[i].empty() && paramQueue[i].front()->GetPhysUse() <= start) {
            if (paramQueue[i].front()->GetPhysUse() == start && li.GetSplitParent() != nullptr) {
                break;
            }
            paramQueue[i].pop_front();
        }
        if (paramQueue[i].empty()) {
            continue;
        }
        auto regNo = paramQueue[i].front()->GetRegNO();
        uint32 startPos = paramQueue[i].front()->GetFirstDef();
        if (startPos <= start) {
            freeUntilPos[regNo] = 0;
        } else {
            if (startPos < end) {
                blockForbiddenMask |= (1UL << (i + baseReg));
            }
            if (startPos < freeUntilPos[regNo]) {
                freeUntilPos[regNo] = startPos;
            }
        }
    }
}

/* update active in retire */
void LSRALinearScanRegAllocator::RetireActive(LiveInterval &li, uint32 insnID)
{
    /* Retire live intervals from active list */
    MapleSet<LiveInterval *, ActiveCmp>::iterator it;
    for (it = active.begin(); it != active.end(); /* erase will update */) {
        auto *activeLi = static_cast<LiveInterval *>(*it);
        if (activeLi->GetLastUse() > insnID) {
            break;
        }
        if (activeLi->GetLastUse() == insnID) {
            if (li.GetSplitParent() != nullptr || activeLi->GetSplitNext() != nullptr) {
                ++it;
                continue;
            }
            if (activeLi->IsEndByMov() && activeLi->GetRegType() == li.GetRegType()) {
                li.SetPrefer(activeLi->GetAssignedReg());
            }
        }
        /* reserve split li in active */
        if (activeLi->GetSplitPos() >= insnID) {
            ++it;
            continue;
        }
        /*
         * live interval ended for this reg in active
         * release physical reg assigned to free reg pool
         */
        if (LSRA_DUMP) {
            LogInfo::MapleLogger() << "Removing "
                                   << "(" << activeLi->GetAssignedReg() << ")"
                                   << "from regmask\n";
            PrintLiveInterval(*activeLi, "\tRemoving virt_reg li\n");
        }
        it = active.erase(it);
    }
}

/* find the best physical reg by freeUntilPos */
uint32 LSRALinearScanRegAllocator::GetRegFromMask(uint32 mask, regno_t offset, const LiveInterval &li)
{
    uint32 prefer = li.GetPrefer();
    if (prefer != 0) {
        uint32 preg = li.GetPrefer() - offset;
        if ((mask & (1u << preg)) != 0 && freeUntilPos[prefer] == 0XFFFFFFFUL) {
            return prefer;
        }
    }
    uint32 bestReg = 0;
    uint32 maxFreeUntilPos = 0;
    for (uint32 preg = 0; preg < k32BitSize; ++preg) {
        if ((mask & (1u << preg)) == 0) {
            continue;
        }
        uint32 regNO = preg + offset;
        if (freeUntilPos[regNO] >= li.GetLastUse()) {
            return regNO;
        }
        if (freeUntilPos[regNO] > maxFreeUntilPos) {
            maxFreeUntilPos = freeUntilPos[regNO];
            bestReg = regNO;
        }
    }
    return bestReg;
}

/* Handle adrp register assignment. Use the same register for the next instruction. */
uint32 LSRALinearScanRegAllocator::GetSpecialPhysRegPattern(const LiveInterval &li)
{
    /* li's first def point */
    Insn *nInsn = nullptr;
    if (nInsn == nullptr || !nInsn->IsMachineInstruction() || nInsn->IsDMBInsn() || li.GetLastUse() > nInsn->GetId()) {
        return 0;
    }

    const InsnDesc *md = nInsn->GetDesc();
    if (!md->GetOpndDes(0)->IsRegDef()) {
        return 0;
    }
    Operand &opnd = nInsn->GetOperand(0);
    if (!opnd.IsRegister()) {
        return 0;
    }
    auto &regOpnd = static_cast<RegOperand &>(opnd);
    if (!regOpnd.IsPhysicalRegister()) {
        return 0;
    }
    uint32 regNO = regOpnd.GetRegisterNumber();
    if (!regInfo->IsPreAssignedReg(regNO)) {
        return 0;
    }

    /* next insn's dest is a physical param reg 'regNO'. return 'regNO' if dest of adrp is src of next insn */
    uint32 opndNum = nInsn->GetOperandSize();
    for (uint32 i = 1; i < opndNum; ++i) {
        Operand &src = nInsn->GetOperand(i);
        if (src.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(src);
            Operand *base = memOpnd.GetBaseRegister();
            if (base != nullptr) {
                auto *regSrc = static_cast<RegOperand *>(base);
                uint32 srcRegNO = regSrc->GetRegisterNumber();
                if (li.GetRegNO() == srcRegNO) {
                    return regNO;
                }
            }
            Operand *offset = memOpnd.GetIndexRegister();
            if (offset != nullptr) {
                auto *regSrc = static_cast<RegOperand *>(offset);
                uint32 srcRegNO = regSrc->GetRegisterNumber();
                if (li.GetRegNO() == srcRegNO) {
                    return regNO;
                }
            }
        } else if (src.IsRegister()) {
            auto &regSrc = static_cast<RegOperand &>(src);
            uint32 srcRegNO = regSrc.GetRegisterNumber();
            if (li.GetRegNO() == srcRegNO) {
                const OpndDesc *regProp = md->GetOpndDes(i);
                DEBUG_ASSERT(regProp != nullptr,
                             "pointer is null in LSRALinearScanRegAllocator::GetSpecialPhysRegPattern");
                bool srcIsDef = regProp->IsRegDef();
                if (srcIsDef) {
                    break;
                }
                return regNO;
            }
        }
    }
    return 0;
}

uint32 LSRALinearScanRegAllocator::FindAvailablePhyRegByFastAlloc(LiveInterval &li)
{
    uint32 regNO = 0;
    if (li.GetRegType() == kRegTyInt) {
        regNO = GetRegFromMask(intCalleeMask, firstIntReg, li);
        li.SetShouldSave(false);
        if (regNO == 0 || freeUntilPos[regNO] < li.GetLastUse()) {
            regNO = GetRegFromMask(intCallerMask, firstIntReg, li);
            li.SetShouldSave(true);
        }
    } else if (li.GetRegType() == kRegTyFloat) {
        regNO = GetRegFromMask(fpCalleeMask, firstFpReg, li);
        li.SetShouldSave(false);
        if (regNO == 0 || freeUntilPos[regNO] < li.GetLastUse()) {
            regNO = GetRegFromMask(fpCallerMask, firstFpReg, li);
            li.SetShouldSave(true);
        }
    }
    return regNO;
}

/* Determine if live interval crosses the call */
bool LSRALinearScanRegAllocator::NeedSaveAcrossCall(LiveInterval &li)
{
    bool saveAcrossCall = false;
    for (uint32 callInsnID : callQueue) {
        if (callInsnID > li.GetLastUse()) {
            break;
        }
        if (callInsnID < li.GetFirstDef()) {
            continue;
        }
        /* Need to spill/fill around this call */
        for (auto range : li.GetRanges()) {
            uint32 start;
            if (range.GetEhStart() != 0 && range.GetEhStart() < range.GetStart()) {
                start = range.GetEhStart();
            } else {
                start = range.GetStart();
            }
            if (callInsnID >= start && callInsnID < range.GetEnd()) {
                saveAcrossCall = true;
                break;
            }
        }
        if (saveAcrossCall) {
            break;
        }
    }
    if (LSRA_DUMP) {
        if (saveAcrossCall) {
            LogInfo::MapleLogger() << "\t\tlive interval crosses a call\n";
        } else {
            LogInfo::MapleLogger() << "\t\tlive interval does not cross a call\n";
        }
    }
    return saveAcrossCall;
}

/* Return a phys register number for the live interval. */
uint32 LSRALinearScanRegAllocator::FindAvailablePhyReg(LiveInterval &li)
{
    if (fastAlloc) {
        return FindAvailablePhyRegByFastAlloc(li);
    }
    uint32 regNO = 0;
    if (li.GetRegType() == kRegTyInt) {
        regNO = FindAvailablePhyReg(li, true);
    } else {
        DEBUG_ASSERT(li.GetRegType() == kRegTyFloat, "impossible register type");
        regNO = FindAvailablePhyReg(li, false);
    }
    return regNO;
}

/* Spill and reload for caller saved registers. */
void LSRALinearScanRegAllocator::InsertCallerSave(Insn &insn, Operand &opnd, bool isDef)
{
    auto &regOpnd = static_cast<RegOperand &>(opnd);
    uint32 vRegNO = regOpnd.GetRegisterNumber();
    if (vRegNO >= liveIntervalsArray.size()) {
        CHECK_FATAL(false, "index out of range in LSRALinearScanRegAllocator::InsertCallerSave");
    }
    LiveInterval *rli = liveIntervalsArray[vRegNO];
    RegType regType = regOpnd.GetRegisterType();

    isSpillZero = false;
    if (!isDef) {
        uint32 mask;
        uint32 regBase;
        if (regType == kRegTyInt) {
            mask = intBBDefMask;
            regBase = firstIntReg;
        } else {
            mask = fpBBDefMask;
            regBase = firstFpReg;
        }
        if (mask & (1u << (rli->GetAssignedReg() - regBase))) {
            if (LSRA_DUMP) {
                LogInfo::MapleLogger() << "InsertCallerSave " << rli->GetAssignedReg()
                                       << " skipping due to local def\n";
            }
            return;
        }
    }

    if (!rli->IsShouldSave()) {
        return;
    }

    uint32 regSize = regOpnd.GetSize();
    PrimType spType;

    if (regType == kRegTyInt) {
        spType = (regSize <= k32BitSize) ? PTY_i32 : PTY_i64;
        intBBDefMask |= (1u << (rli->GetAssignedReg() - firstIntReg));
    } else {
        spType = (regSize <= k32BitSize) ? PTY_f32 : PTY_f64;
        fpBBDefMask |= (1u << (rli->GetAssignedReg() - firstFpReg));
    }

    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "InsertCallerSave " << vRegNO << "\n";
    }

    if (!isDef && !rli->IsCallerSpilled()) {
        LogInfo::MapleLogger() << "WARNING: " << vRegNO << " caller restore without spill in bb "
                               << insn.GetBB()->GetId() << " : " << cgFunc->GetName() << "\n";
    }
    rli->SetIsCallerSpilled(true);

    MemOperand *memOpnd = nullptr;
    RegOperand *phyOpnd = nullptr;

    phyOpnd = regInfo->GetOrCreatePhyRegOperand(static_cast<regno_t>(rli->GetAssignedReg()), regSize, regType);
    std::string comment;
    bool isOutOfRange = false;
    if (isDef) {
        memOpnd = GetSpillMem(vRegNO, true, insn, static_cast<regno_t>(intSpillRegSet[0] + firstIntReg), isOutOfRange,
                              regSize);
        Insn *stInsn = regInfo->BuildStrInsn(regSize, spType, *phyOpnd, *memOpnd);
        comment = " SPILL for caller_save " + std::to_string(vRegNO);
        ++callerSaveSpillCount;
        if (rli->GetLastUse() == insn.GetId()) {
            regInfo->FreeSpillRegMem(vRegNO);
            comment += " end";
        }
        stInsn->SetComment(comment);
        if (isOutOfRange) {
            insn.GetBB()->InsertInsnAfter(*insn.GetNext(), *stInsn);
        } else {
            insn.GetBB()->InsertInsnAfter(insn, *stInsn);
        }
    } else {
        memOpnd = GetSpillMem(vRegNO, false, insn, static_cast<regno_t>(intSpillRegSet[0] + firstIntReg), isOutOfRange,
                              regSize);
        Insn *ldInsn = regInfo->BuildLdrInsn(regSize, spType, *phyOpnd, *memOpnd);
        comment = " RELOAD for caller_save " + std::to_string(vRegNO);
        ++callerSaveReloadCount;
        if (rli->GetLastUse() == insn.GetId()) {
            regInfo->FreeSpillRegMem(vRegNO);
            comment += " end";
        }
        ldInsn->SetComment(comment);
        insn.GetBB()->InsertInsnBefore(insn, *ldInsn);
    }
}

MemOperand *LSRALinearScanRegAllocator::GetSpillMem(uint32 vRegNO, bool isDest, Insn &insn, regno_t regNO,
                                                    bool &isOutOfRange, uint32 bitSize) const
{
    MemOperand *memOpnd = regInfo->GetOrCreatSpillMem(vRegNO, bitSize);
    return regInfo->AdjustMemOperandIfOffsetOutOfRange(memOpnd, vRegNO, isDest, insn, regNO, isOutOfRange);
}

/* Set a vreg in live interval as being marked for spill. */
void LSRALinearScanRegAllocator::SetOperandSpill(Operand &opnd)
{
    auto &regOpnd = static_cast<RegOperand &>(opnd);
    uint32 regNO = regOpnd.GetRegisterNumber();
    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "SetOperandSpill " << regNO;
        LogInfo::MapleLogger() << "(" << liveIntervalsArray[regNO]->GetFirstAcrossedCall();
        LogInfo::MapleLogger() << ", refCount " << liveIntervalsArray[regNO]->GetRefCount() << ")\n";
    }

    DEBUG_ASSERT(regNO < liveIntervalsArray.size(),
                 "index out of vector size in LSRALinearScanRegAllocator::SetOperandSpill");
    LiveInterval *li = liveIntervalsArray[regNO];
    li->SetStackSlot(kSpilled);
    li->SetShouldSave(false);
}

/*
 * Generate spill/reload for an operand.
 * spill_idx : one of 3 phys regs set aside for the purpose of spills.
 */
void LSRALinearScanRegAllocator::SpillOperand(Insn &insn, Operand &opnd, bool isDef, uint32 spillIdx)
{
    /*
     * Insert spill (def)  and fill (use)  instructions for the operand.
     *  Keep track of the 'slot' (base 0). The actual slot on the stack
     *  will be some 'base_slot_offset' + 'slot' off FP.
     *  For simplification, entire 64bit register is spilled/filled.
     *
     *  For example, a virtual register home 'slot' on the stack is location 5.
     *  This represents a 64bit slot (8bytes).  The base_slot_offset
     *  from the base 'slot' determined by whoever is added, off FP.
     *     stack address is  ( FP - (5 * 8) + base_slot_offset )
     *  So the algorithm is simple, for each virtual register that is not
     *  allocated, it has to have a home address on the stack (a slot).
     *  A class variable is used, start from 0, increment by 1.
     *  Since LiveInterval already represent unique regNO information,
     *  just add a slot number to it.  Subsequent reference to a regNO
     *  will either get an allocated physical register or a slot number
     *  for computing the stack location.
     *
     *  This function will also determine the operand to be a def or use.
     *  For def, spill instruction(s) is appended after the insn.
     *  For use, spill instruction(s) is prepended before the insn.
     *  Use FP - (slot# *8) for now.  Will recompute if base_slot_offset
     *  is not 0.
     *
     *  The total number of slots used will be used to compute the stack
     *  frame size.  This will require some interface external to LSRA.
     *
     *  For normal instruction, two spill regs should be enough.  The caller
     *  controls which ones to use.
     *  For more complex operations, need to break down the instruction.
     *    eg.  store  v1 -> [v2 + v3]  // 3 regs needed
     *         =>  p1 <- v2        // address part 1
     *             p2 <- v3        // address part 2
     *             p1 <- p1 + p2   // freeing up p2
     *             p2 <- v1
     *            store p2 -> [p1]
     *      or we can allocate more registers to the spill register set
     *  For store multiple, need to break it down into two or more instr.
     */
    auto &regOpnd = static_cast<RegOperand &>(opnd);
    uint32 regNO = regOpnd.GetRegisterNumber();
    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "SpillOperand " << regNO << "\n";
    }

    regno_t spReg;
    PrimType spType;
    CHECK_FATAL(regNO < liveIntervalsArray.size(), "index out of range in LSRALinearScanRegAllocator::SpillOperand");
    LiveInterval *li = liveIntervalsArray[regNO];
    DEBUG_ASSERT(!li->IsShouldSave(), "SpillOperand: Should not be caller");
    uint32 regSize = regOpnd.GetSize();
    RegType regType = regOpnd.GetRegisterType();

    if (li->GetRegType() == kRegTyInt) {
        DEBUG_ASSERT((spillIdx < intSpillRegSet.size()), "SpillOperand: ran out int spill reg");
        spReg = intSpillRegSet[spillIdx] + firstIntReg;
        spType = (regSize <= k32BitSize) ? PTY_i32 : PTY_i64;
    } else if (li->GetRegType() == kRegTyFloat) {
        DEBUG_ASSERT((spillIdx < fpSpillRegSet.size()), "SpillOperand: ran out fp spill reg");
        spReg = fpSpillRegSet[spillIdx] + firstFpReg;
        spType = (regSize <= k32BitSize) ? PTY_f32 : PTY_f64;
    } else {
        CHECK_FATAL(false, "SpillOperand: Should be int or float type");
    }

    bool isOutOfRange = false;
    RegOperand *phyOpnd = nullptr;
    if (isSpillZero) {
        phyOpnd = &cgFunc->GetZeroOpnd(regSize);
    } else {
        phyOpnd = regInfo->GetOrCreatePhyRegOperand(static_cast<regno_t>(spReg), regSize, regType);
    }
    li->SetAssignedReg(phyOpnd->GetRegisterNumber());

    MemOperand *memOpnd = nullptr;
    if (isDef) {
        /*
         * Need to assign spReg (one of the two spill reg) to the destination of the insn.
         *    spill_vreg <- opn1 op opn2
         * to
         *    spReg <- opn1 op opn2
         *    store spReg -> spillmem
         */
        li->SetStackSlot(kSpilled);

        ++spillCount;
        memOpnd = GetSpillMem(regNO, true, insn, static_cast<regno_t>(intSpillRegSet[spillIdx + 1] + firstIntReg),
                              isOutOfRange, regSize);
        Insn *stInsn = regInfo->BuildStrInsn(regSize, spType, *phyOpnd, *memOpnd);
        std::string comment = " SPILL vreg:" + std::to_string(regNO);
        if (li->GetLastUse() == insn.GetId()) {
            regInfo->FreeSpillRegMem(regNO);
            comment += " end";
        }
        stInsn->SetComment(comment);
        if (isOutOfRange) {
            insn.GetBB()->InsertInsnAfter(*insn.GetNext(), *stInsn);
        } else {
            insn.GetBB()->InsertInsnAfter(insn, *stInsn);
        }
    } else {
        /* Here, reverse of isDef, change either opn1 or opn2 to the spReg. */
        if (li->GetStackSlot() == 0xFFFFFFFF) {
            LogInfo::MapleLogger() << "WARNING: " << regNO << " assigned " << li->GetAssignedReg()
                                   << " restore without spill in bb " << insn.GetBB()->GetId() << " : "
                                   << cgFunc->GetName() << "\n";
        }
        ++reloadCount;
        memOpnd = GetSpillMem(regNO, false, insn, static_cast<regno_t>(intSpillRegSet[spillIdx] + firstIntReg),
                              isOutOfRange, regSize);
        Insn *ldInsn = regInfo->BuildLdrInsn(regSize, spType, *phyOpnd, *memOpnd);
        std::string comment = " RELOAD vreg" + std::to_string(regNO);
        if (li->GetLastUse() == insn.GetId()) {
            regInfo->FreeSpillRegMem(regNO);
            comment += " end";
        }
        ldInsn->SetComment(comment);
        insn.GetBB()->InsertInsnBefore(insn, *ldInsn);
    }
}

/* find the lowest li that meets the constraints related to li0 form current active */
void LSRALinearScanRegAllocator::FindLowestPrioInActive(LiveInterval *&targetLi, LiveInterval *li0, RegType regType)
{
    float lowestPrio = 1000.0;
    bool found = false;
    bool hintCalleeSavedReg = li0 && NeedSaveAcrossCall(*li0);
    MapleSet<LiveInterval *, ActiveCmp>::iterator it;
    MapleSet<LiveInterval *, ActiveCmp>::iterator lowestIt;
    for (it = active.begin(); it != active.end(); ++it) {
        LiveInterval *li = static_cast<LiveInterval *>(*it);
        regno_t regNO = li->GetAssignedReg();
        /* 1. Basic Constraints */
        if (li->GetPriority() >= lowestPrio || li->GetRegType() != regType || li->GetLiParent() || li->GetLiChild()) {
            continue;
        }
        /* 2. If li is pre-assigned to Physical register primitively, ignore it. */
        if (regInfo->IsPreAssignedReg(li->GetRegNO())) {
            continue;
        }
        /* 3. CalleeSavedReg is preferred here. If li is assigned to Non-CalleeSavedReg, ignore it. */
        if (hintCalleeSavedReg && !regInfo->IsCalleeSavedReg(regNO - firstIntReg)) {
            continue;
        }
        /* 4. Checkinterference. If li is assigned to li0's OverlapPhyReg, ignore it. */
        if (li0 && li0->IsOverlapPhyReg(regNO)) {
            continue;
        }
        lowestPrio = li->GetPriority();
        lowestIt = it;
        found = true;
    }
    if (found) {
        targetLi = *lowestIt;
        itFinded = lowestIt;
    }
    return;
}

/* Set a vreg in live interval as being marked for spill. */
void LSRALinearScanRegAllocator::SetLiSpill(LiveInterval &li)
{
    uint32 regNO = li.GetRegNO();
    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "SetLiSpill " << regNO;
        LogInfo::MapleLogger() << "(" << li.GetFirstAcrossedCall();
        LogInfo::MapleLogger() << ", refCount " << li.GetRefCount() << ")\n";
    }
    li.SetStackSlot(kSpilled);
    li.SetShouldSave(false);
}

uint32 LSRALinearScanRegAllocator::HandleSpillForLi(LiveInterval &li)
{
    /* choose the lowest priority li to spill */
    RegType regType = li.GetRegType();
    LiveInterval *spillLi = nullptr;
    FindLowestPrioInActive(spillLi, &li, regType);

    /*
     * compare spill_li with current li
     * spill_li is null and li->SetStackSlot(Spilled) when the li is spilled due to LiveIntervalAnalysis
     */
    if (!li.IsMustAllocate()) {
        if (spillLi == nullptr || li.GetStackSlot() == kSpilled || li.GetRefCount() <= spillLi->GetRefCount()) {
            /* spill current li */
            if (LSRA_DUMP) {
                LogInfo::MapleLogger() << "Flexible Spill: still spill " << li.GetRegNO() << ".\n";
            }
            SetLiSpill(li);
            return 0;
        }
    }
    DEBUG_ASSERT(spillLi != nullptr, "spillLi is null in LSRALinearScanRegAllocator::HandleSpillForLi");

    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "Flexible Spill: " << spillLi->GetRegNO() << " instead of " << li.GetRegNO() << ".\n";
        PrintLiveInterval(*spillLi, "TO spill: ");
        PrintLiveInterval(li, "Instead of: ");
    }

    uint32 newRegNO = spillLi->GetAssignedReg();
    li.SetAssignedReg(newRegNO);

    /* spill this live interval */
    (void)active.erase(itFinded);
    SetLiSpill(*spillLi);
    spillLi->SetAssignedReg(0);

    (void)active.insert(&li);
    return newRegNO;
}

uint32 LSRALinearScanRegAllocator::FindAvailablePhyReg(LiveInterval &li, bool isIntReg)
{
    uint32 &callerRegMask = isIntReg ? intCallerMask : fpCallerMask;
    uint32 &calleeRegMask = isIntReg ? intCalleeMask : fpCalleeMask;
    regno_t reg0 = isIntReg ? firstIntReg : firstFpReg;
    regno_t bestReg = 0;
    regno_t secondReg = 0;

    /* See if register is live accross a call */
    if (NeedSaveAcrossCall(li)) {
        if (!li.IsAllInCatch() && !li.IsAllInCleanupOrFirstBB()) {
            /* call in live interval, use callee if available */
            bestReg = GetRegFromMask(calleeRegMask, reg0, li);
            if (bestReg != 0 && freeUntilPos[bestReg] >= li.GetLastUse()) {
                li.SetShouldSave(false);
                return bestReg;
            }
        }
        /* can be optimize multi use between calls rather than in bb */
        if (bestReg == 0 || li.IsMultiUseInBB()) {
            secondReg = GetRegFromMask(callerRegMask, reg0, li);
            if (freeUntilPos[secondReg] >= li.GetLastUse()) {
                li.SetShouldSave(true);
                return secondReg;
            }
        }
    } else {
        /* Get forced register */
        uint32 forcedReg = GetSpecialPhysRegPattern(li);
        if (forcedReg != 0) {
            return forcedReg;
        }

        bestReg = GetRegFromMask(intCallerMask, reg0, li);
        if (bestReg == 0) {
            bestReg = GetRegFromMask(intCalleeMask, reg0, li);
        } else if (freeUntilPos[bestReg] < li.GetLastUse()) {
            secondReg = GetRegFromMask(intCalleeMask, reg0, li);
            if (secondReg != 0) {
                bestReg = (freeUntilPos[bestReg] > freeUntilPos[secondReg]) ? bestReg : secondReg;
            }
        }
    }
    if (bestReg != 0 && freeUntilPos[bestReg] < li.GetLastUse()) {
        DEBUG_ASSERT(freeUntilPos[bestReg] != 0, "impossible");
        bestReg = 0;
    }
    /* todo : try to fill in holes */
    /* todo : try first split if no hole exists */
    return bestReg;
}

/* Shell function to find a physical register for an operand. */
uint32 LSRALinearScanRegAllocator::AssignPhysRegs(LiveInterval &li)
{
    if (spillAll && !li.IsMustAllocate()) {
        return 0;
    }

    /* pre spilled: */
    if (li.GetStackSlot() != 0xFFFFFFFF && !li.IsMustAllocate()) {
        return 0;
    }

    if (LSRA_DUMP) {
        uint32 activeSz = active.size();
        LogInfo::MapleLogger() << "\tAssignPhysRegs-active_sz " << activeSz << "\n";
    }

    // aarch64 add fp lr to callee saved area;
    regInfo->Fini();

    uint32 regNO = FindAvailablePhyReg(li);
    if (regNO != 0) {
        li.SetAssignedReg(regNO);
        if (regInfo->IsCalleeSavedReg(regNO)) {
            if (!CGOptions::DoCalleeToSpill()) {
                if (LSRA_DUMP) {
                    LogInfo::MapleLogger()
                        << "\tCallee-save register for save/restore in prologue/epilogue: " << regNO << "\n";
                }
                cgFunc->AddtoCalleeSaved(regNO);
            }
            ++calleeUseCnt[regNO];
        }
    }
    return regNO;
}

void LSRALinearScanRegAllocator::AssignPhysRegsForLi(LiveInterval &li)
{
    uint32 newRegNO = AssignPhysRegs(li);
    if (newRegNO == 0) {
        newRegNO = HandleSpillForLi(li);
    }

    if (newRegNO != 0) {
        (void)active.insert(&li);
    }
}

/* Replace Use-Def Opnd */
RegOperand *LSRALinearScanRegAllocator::GetReplaceUdOpnd(Insn &insn, Operand &opnd, uint32 &spillIdx)
{
    if (!opnd.IsRegister()) {
        return nullptr;
    }
    const auto *regOpnd = static_cast<RegOperand *>(&opnd);

    uint32 vRegNO = regOpnd->GetRegisterNumber();
    RegType regType = regOpnd->GetRegisterType();
    if (regType == kRegTyCc || regType == kRegTyVary) {
        return nullptr;
    }
    if (regInfo->IsUntouchableReg(vRegNO)) {
        return nullptr;
    }
    if (regOpnd->IsPhysicalRegister()) {
        return nullptr;
    }

    DEBUG_ASSERT(vRegNO < liveIntervalsArray.size(),
                 "index out of range of MapleVector in LSRALinearScanRegAllocator::GetReplaceUdOpnd");
    LiveInterval *li = liveIntervalsArray[vRegNO];

    regno_t regNO = li->GetAssignedReg();
    if (regInfo->IsCalleeSavedReg(regNO)) {
        cgFunc->AddtoCalleeSaved(regNO);
    }

    if (li->IsShouldSave()) {
        InsertCallerSave(insn, opnd, false);
    } else if (li->GetStackSlot() == kSpilled) {
        SpillOperand(insn, opnd, false, spillIdx);
        SpillOperand(insn, opnd, true, spillIdx);
        ++spillIdx;
    }
    RegOperand *phyOpnd =
        regInfo->GetOrCreatePhyRegOperand(static_cast<regno_t>(li->GetAssignedReg()), opnd.GetSize(), regType);

    return phyOpnd;
}

/*
 * Create an operand with physical register assigned, or a spill register
 * in the case where a physical register cannot be assigned.
 */
RegOperand *LSRALinearScanRegAllocator::GetReplaceOpnd(Insn &insn, Operand &opnd, uint32 &spillIdx, bool isDef)
{
    if (!opnd.IsRegister()) {
        return nullptr;
    }
    const auto *regOpnd = static_cast<RegOperand *>(&opnd);

    uint32 vRegNO = regOpnd->GetRegisterNumber();
    RegType regType = regOpnd->GetRegisterType();
    if (regType == kRegTyCc || regType == kRegTyVary) {
        return nullptr;
    }
    if (regInfo->IsUntouchableReg(vRegNO)) {
        return nullptr;
    }
    if (regOpnd->IsPhysicalRegister()) {
        return nullptr;
    }

    DEBUG_ASSERT(vRegNO < liveIntervalsArray.size(),
                 "index out of range of MapleVector in LSRALinearScanRegAllocator::GetReplaceOpnd");
    LiveInterval *li = liveIntervalsArray[vRegNO];

    regno_t regNO = li->GetAssignedReg();
    if (regInfo->IsCalleeSavedReg(regNO)) {
        cgFunc->AddtoCalleeSaved(regNO);
    }

    if (li->IsShouldSave()) {
        InsertCallerSave(insn, opnd, isDef);
    } else if (li->GetStackSlot() == kSpilled) {
        spillIdx = isDef ? 0 : spillIdx;
        SpillOperand(insn, opnd, isDef, spillIdx);
        if (!isDef) {
            ++spillIdx;
        }
    }
    RegOperand *phyOpnd =
        regInfo->GetOrCreatePhyRegOperand(static_cast<regno_t>(li->GetAssignedReg()), opnd.GetSize(), regType);

    return phyOpnd;
}

/* Try to estimate if spill callee should be done based on even/odd for stp in prolog. */
void LSRALinearScanRegAllocator::CheckSpillCallee()
{
    if (CGOptions::DoCalleeToSpill()) {
        uint32 pairCnt = 0;
        for (size_t idx = 0; idx < sizeof(uint32); ++idx) {
            if ((intCalleeMask & (1ULL << idx)) != 0 && calleeUseCnt[idx] != 0) {
                ++pairCnt;
            }
        }
        if ((pairCnt & 0x01) != 0) {
            shouldOptIntCallee = true;
        }

        for (size_t idx = 0; idx < sizeof(uint32); ++idx) {
            if ((fpCalleeMask & (1ULL << idx)) != 0 && calleeUseCnt[idx] != 0) {
                ++pairCnt;
            }
        }
        if ((pairCnt & 0x01) != 0) {
            shouldOptFpCallee = true;
        }
    }
}

/* Iterate through all instructions and change the vreg to preg. */
void LSRALinearScanRegAllocator::FinalizeRegisters()
{
    CheckSpillCallee();
    for (BB *bb : bfs->sortedBBs) {
        intBBDefMask = 0;
        fpBBDefMask = 0;

        FOR_BB_INSNS(insn, bb) {
            if (insn->IsImmaterialInsn() || insn->GetId() == 0) {
                continue;
            }
            if (!insn->IsMachineInstruction()) {
                continue;
            }

            uint32 spillIdx = 0;
            const InsnDesc *md = insn->GetDesc();
            uint32 opndNum = insn->GetOperandSize();

            /* Handle source(use) opernads first */
            for (uint32 i = 0; i < opndNum; ++i) {
                const OpndDesc *regProp = md->GetOpndDes(i);
                DEBUG_ASSERT(regProp != nullptr, "pointer is null in LSRALinearScanRegAllocator::FinalizeRegisters");
                bool isDef = regProp->IsRegDef();
                if (isDef) {
                    continue;
                }
                Operand &opnd = insn->GetOperand(i);
                RegOperand *phyOpnd = nullptr;
                if (opnd.IsList()) {
                    /* For arm32, not arm64 */
                } else if (opnd.IsMemoryAccessOperand()) {
                    auto *memOpnd =
                        static_cast<MemOperand *>(static_cast<MemOperand &>(opnd).Clone(*cgFunc->GetMemoryPool()));
                    DEBUG_ASSERT(memOpnd != nullptr,
                                 "memopnd is null in LSRALinearScanRegAllocator::FinalizeRegisters");
                    insn->SetOperand(i, *memOpnd);
                    Operand *base = memOpnd->GetBaseRegister();
                    Operand *offset = memOpnd->GetIndexRegister();
                    if (base != nullptr) {
                        phyOpnd = GetReplaceOpnd(*insn, *base, spillIdx, false);
                        if (phyOpnd != nullptr) {
                            memOpnd->SetBaseRegister(*phyOpnd);
                        }
                    }
                    if (offset != nullptr) {
                        phyOpnd = GetReplaceOpnd(*insn, *offset, spillIdx, false);
                        if (phyOpnd != nullptr) {
                            memOpnd->SetIndexRegister(*phyOpnd);
                        }
                    }
                } else {
                    phyOpnd = GetReplaceOpnd(*insn, opnd, spillIdx, false);
                    if (phyOpnd != nullptr) {
                        insn->SetOperand(i, *phyOpnd);
                    }
                }
            }

            /* Handle ud(use-def) opernads */
            for (uint32 i = 0; i < opndNum; ++i) {
                const OpndDesc *regProp = md->GetOpndDes(i);
                DEBUG_ASSERT(regProp != nullptr, "pointer is null in LSRALinearScanRegAllocator::FinalizeRegisters");
                Operand &opnd = insn->GetOperand(i);
                bool isUseDef = regProp->IsRegDef() && regProp->IsRegUse();
                if (!isUseDef) {
                    continue;
                }
                RegOperand *phyOpnd = GetReplaceUdOpnd(*insn, opnd, spillIdx);
                if (phyOpnd != nullptr) {
                    insn->SetOperand(i, *phyOpnd);
                }
            }

            /* Handle dest(def) opernads last */
            for (uint32 i = 0; i < opndNum; ++i) {
                const OpndDesc *regProp = md->GetOpndDes(i);
                DEBUG_ASSERT(regProp != nullptr, "pointer is null in LSRALinearScanRegAllocator::FinalizeRegisters");
                Operand &opnd = insn->GetOperand(i);
                bool isUse = (regProp->IsRegUse()) || (opnd.IsMemoryAccessOperand());
                if (isUse) {
                    continue;
                }
                isSpillZero = false;
                RegOperand *phyOpnd = GetReplaceOpnd(*insn, opnd, spillIdx, true);
                if (phyOpnd != nullptr) {
                    insn->SetOperand(i, *phyOpnd);
                    if (isSpillZero) {
                        insn->GetBB()->RemoveInsn(*insn);
                    }
                }
            }

            if (insn->IsCall()) {
                intBBDefMask = 0;
                fpBBDefMask = 0;
            }
        }
    }
}

void LSRALinearScanRegAllocator::CollectReferenceMap()
{
    const auto &referenceMapInsns = cgFunc->GetStackMapInsns();
    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "===========reference map stack info================\n";
    }

    for (auto *insn : referenceMapInsns) {
        auto *stackMapLiveIn = insn->GetStackMapLiveIn();
        for (auto regNO : stackMapLiveIn->GetInfo()) {
            if (!cgFunc->IsRegReference(regNO)) {
                continue;
            }

            auto *li = liveIntervalsArray[regNO];
            if (li == nullptr) {
                continue;
            }

            if (li->IsShouldSave() || li->GetStackSlot() == kSpilled) {
                MemOperand *memOperand = cgFunc->GetOrCreatSpillMem(regNO, k64BitSize);
                int64 offset = memOperand->GetOffsetImmediate()->GetOffsetValue();
                insn->GetStackMap()->GetReferenceMap().ReocordStackRoots(offset);
                if (LSRA_DUMP) {
                    LogInfo::MapleLogger() << "--------insn id: " << insn->GetId() << " regNO: " << regNO << "offset: "
                                           << offset << std::endl;
                }
            } else {
                // TODO: li->GetAssignedReg - R0/RAX?
                CHECK_FATAL(false, "not support currently");
                insn->GetStackMap()->GetReferenceMap().ReocordRegisterRoots(li->GetAssignedReg());
            }
        }
    }

    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "===========reference map stack info end================\n";
    }
    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "===========reference map info================\n";
        for (auto *insn : referenceMapInsns) {
            LogInfo::MapleLogger() << "  referenceMap insn: ";
            insn->Dump();
            insn->GetStackMap()->GetReferenceMap().Dump();
        }
    }
}

void LSRALinearScanRegAllocator::SolveRegOpndDeoptInfo(const RegOperand &regOpnd, DeoptInfo &deoptInfo,
                                                       int32 deoptVregNO) const
{
    if (regOpnd.IsPhysicalRegister()) {
        // TODO: Get Register No
        deoptInfo.RecordDeoptVreg2LocationInfo(deoptVregNO, LocationInfo({kInRegister, 0}));
        return;
    }
    // process virtual RegOperand
    regno_t vRegNO = regOpnd.GetRegisterNumber();
    // TODO: LiveInterval *li = GetLiveIntervalAt(vRegNO, insn->GetId());
    LiveInterval *li = liveIntervalsArray[vRegNO];
    if (li->IsShouldSave() || li->GetStackSlot() == kSpilled) {
        MemOperand *memOpnd = cgFunc->GetOrCreatSpillMem(vRegNO, regOpnd.GetSize());
        SolveMemOpndDeoptInfo(*(static_cast<const MemOperand *>(memOpnd)), deoptInfo, deoptVregNO);
    } else {
        // TODO: Get Register NO
        deoptInfo.RecordDeoptVreg2LocationInfo(deoptVregNO, LocationInfo({kInRegister, li->GetAssignedReg()}));
    }
}

void LSRALinearScanRegAllocator::SolveMemOpndDeoptInfo(const MemOperand &memOpnd, DeoptInfo &deoptInfo,
                                                       int32 deoptVregNO) const
{
    int64 offset = memOpnd.GetOffsetImmediate()->GetOffsetValue();
    deoptInfo.RecordDeoptVreg2LocationInfo(deoptVregNO, LocationInfo({kOnStack, offset}));
}

void LSRALinearScanRegAllocator::CollectDeoptInfo()
{
    const auto referenceMapInsns = cgFunc->GetStackMapInsns();
    for (auto *insn : referenceMapInsns) {
        auto &deoptInfo = insn->GetStackMap()->GetDeoptInfo();
        const auto &deoptBundleInfo = deoptInfo.GetDeoptBundleInfo();
        if (deoptBundleInfo.empty()) {
            continue;
        }
        for (const auto &item : deoptBundleInfo) {
            const auto *opnd = item.second;
            if (opnd->IsRegister()) {
                SolveRegOpndDeoptInfo(*static_cast<const RegOperand *>(opnd), deoptInfo, item.first);
                continue;
            }
            if (opnd->IsImmediate()) {
                const auto *immOpnd = static_cast<const ImmOperand *>(opnd);
                deoptInfo.RecordDeoptVreg2LocationInfo(item.first, LocationInfo({kInConstValue, immOpnd->GetValue()}));
                continue;
            }
            if (opnd->IsMemoryAccessOperand()) {
                SolveMemOpndDeoptInfo(*(static_cast<const MemOperand *>(opnd)), deoptInfo, item.first);
                continue;
            }
            DEBUG_ASSERT(false, "can't reach here!");
        }
    }
    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "===========deopt info================\n";
        for (auto *insn : referenceMapInsns) {
            LogInfo::MapleLogger() << "---- deoptInfo insn: ";
            insn->Dump();
            insn->GetStackMap()->GetDeoptInfo().Dump();
        }
    }
}

void LSRALinearScanRegAllocator::SetAllocMode()
{
    if (CGOptions::IsFastAlloc()) {
        if (CGOptions::GetFastAllocMode() == 0) {
            fastAlloc = true;
        } else {
            spillAll = true;
        }
        /* In-Range spill range can still be specified (only works with --dump-func=). */
    } else if (cgFunc->NumBBs() > CGOptions::GetLSRABBOptSize()) {
        /* instruction size is checked in ComputeLieveInterval() */
        fastAlloc = true;
    }

    if (LSRA_DUMP) {
        if (fastAlloc) {
            LogInfo::MapleLogger() << "fastAlloc mode on\n";
        }
        if (spillAll) {
            LogInfo::MapleLogger() << "spillAll mode on\n";
        }
    }
}

void LSRALinearScanRegAllocator::LinearScanRegAllocator()
{
    if (LSRA_DUMP) {
        PrintParamQueue("Initial param queue");
        PrintCallQueue("Initial call queue");
    }
    freeUntilPos.resize(regInfo->GetAllRegNum(), 0XFFFFFFFUL);
    MapleVector<uint32> initialPosVec(freeUntilPos);
    uint32 curInsnID = 0;

    while (!liQue.empty()) {
        LiveInterval *li = liQue.front();
        liQue.pop_front();
        if (li->GetRangesSize() == 0) {
            /* range building has been skiped */
            li->AddRange(li->GetFirstDef(), li->GetLastUse());
        }
        li->InitRangeFinder();
        if (LSRA_DUMP) {
            LogInfo::MapleLogger() << "======Alloc R" << li->GetRegNO() << "======"
                                   << "\n";
        }
        blockForbiddenMask = 0;
        freeUntilPos = initialPosVec;
        DEBUG_ASSERT(li->GetFirstDef() >= curInsnID, "wrong li order");
        curInsnID = li->GetFirstDef();
        RetireActive(*li, curInsnID);
        UpdateCallQueueAtRetirement(curInsnID);
        UpdateActiveAllocateInfo(*li);
        UpdateParamAllocateInfo(*li);
        if (LSRA_DUMP) {
            DebugCheckActiveList();
        }
        AssignPhysRegsForLi(*li);
    }
}

/* Main entrance for the LSRA register allocator */
bool LSRALinearScanRegAllocator::AllocateRegisters()
{
    cgFunc->SetIsAfterRegAlloc();
    calleeUseCnt.resize(regInfo->GetAllRegNum());
    liveIntervalsArray.resize(cgFunc->GetMaxVReg());
    SetAllocMode();
#ifdef RA_PERF_ANALYSIS
    auto begin = std::chrono::system_clock::now();
#endif
    if (LSRA_DUMP) {
        const MIRModule &mirModule = cgFunc->GetMirModule();
        DotGenerator::GenerateDot("RA", *cgFunc, mirModule);
        DotGenerator::GenerateDot("RAe", *cgFunc, mirModule, true);
        LogInfo::MapleLogger() << "Entering LinearScanRegAllocator: " << cgFunc->GetName() << "\n";
    }
/* ================= LiveInterval =============== */
#ifdef RA_PERF_ANALYSIS
    start = std::chrono::system_clock::now();
#endif
    ComputeLiveInterval();

    if (LSRA_DUMP) {
        PrintLiveRangesGraph();
    }

    bool enableDoLSRAPreSpill = true;
    if (enableDoLSRAPreSpill) {
        LiveIntervalAnalysis();
    }

#ifdef RA_PERF_ANALYSIS
    end = std::chrono::system_clock::now();
    liveIntervalUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

/* ================= LiveRange =============== */
#ifdef RA_PERF_ANALYSIS
    start = std::chrono::system_clock::now();
#endif

    bool enableDoLSRAHole = true;
    if (enableDoLSRAHole) {
        BuildIntervalRanges();
    }

    SpillStackMapInfo();

    if (LSRA_DUMP) {
        PrintAllLiveRanges();
    }
#ifdef RA_PERF_ANALYSIS
    end = std::chrono::system_clock::now();
    holesUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif
    /* ================= InitFreeRegPool =============== */
    InitFreeRegPool();

/* ================= LinearScanRegAllocator =============== */
#ifdef RA_PERF_ANALYSIS
    start = std::chrono::system_clock::now();
#endif
    LinearScanRegAllocator();
#ifdef RA_PERF_ANALYSIS
    end = std::chrono::system_clock::now();
    lsraUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

    if (LSRA_DUMP) {
        PrintAllLiveRanges();
    }

#ifdef RA_PERF_ANALYSIS
    start = std::chrono::system_clock::now();
#endif
    FinalizeRegisters();
#ifdef RA_PERF_ANALYSIS
    end = std::chrono::system_clock::now();
    finalizeUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

    CollectReferenceMap();
    CollectDeoptInfo();

    if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "Total " << spillCount << " spillCount in " << cgFunc->GetName() << " \n";
        LogInfo::MapleLogger() << "Total " << reloadCount << " reloadCount\n";
        LogInfo::MapleLogger() << "Total "
                               << "(" << spillCount << "+ " << callerSaveSpillCount
                               << ") = " << (spillCount + callerSaveSpillCount) << " SPILL\n";
        LogInfo::MapleLogger() << "Total "
                               << "(" << reloadCount << "+ " << callerSaveReloadCount
                               << ") = " << (reloadCount + callerSaveReloadCount) << " RELOAD\n";
        uint32_t insertInsn = spillCount + callerSaveSpillCount + reloadCount + callerSaveReloadCount;
        float rate = (float(insertInsn) / float(maxInsnNum));
        LogInfo::MapleLogger() << "insn Num Befor RA:" << maxInsnNum << ", insert " << insertInsn << " insns: "
                               << ", insertInsn/insnNumBeforRA: " << rate << "\n";
    }

    bfs = nullptr; /* bfs is not utilized outside the function. */

#ifdef RA_PERF_ANALYSIS
    end = std::chrono::system_clock::now();
    totalUS += std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
#endif

    return true;
}

} /* namespace maplebe */
