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

#include "peep.h"
#include "cg.h"
#include "mpl_logging.h"
#include "common_utils.h"
#if TARGAARCH64
#include "aarch64_peep.h"
#endif
#if TARGRISCV64
#include "riscv64_peep.h"
#endif
#if defined TARGX86_64
#include "x64_peep.h"
#endif
#if TARGARM32
#include "arm32_peep.h"
#endif

namespace maplebe {
#if TARGAARCH64
bool CGPeepPattern::IsCCRegCrossVersion(Insn &startInsn, Insn &endInsn, const RegOperand &ccReg) const
{
    if (startInsn.GetBB() != endInsn.GetBB()) {
        return true;
    }
    CHECK_FATAL(ssaInfo != nullptr, "must have ssaInfo");
    CHECK_FATAL(ccReg.IsSSAForm(), "cc reg must be ssa form");
    for (auto *curInsn = startInsn.GetNext(); curInsn != nullptr && curInsn != &endInsn; curInsn = curInsn->GetNext()) {
        if (!curInsn->IsMachineInstruction()) {
            continue;
        }
        if (curInsn->IsCall()) {
            return true;
        }
        uint32 opndNum = curInsn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = curInsn->GetOperand(i);
            if (!opnd.IsRegister()) {
                continue;
            }
            auto &regOpnd = static_cast<RegOperand &>(opnd);
            if (!curInsn->IsRegDefined(regOpnd.GetRegisterNumber())) {
                continue;
            }
            if (static_cast<RegOperand &>(opnd).IsOfCC()) {
                VRegVersion *ccVersion = ssaInfo->FindSSAVersion(ccReg.GetRegisterNumber());
                VRegVersion *curCCVersion = ssaInfo->FindSSAVersion(regOpnd.GetRegisterNumber());
                CHECK_FATAL(ccVersion != nullptr && curCCVersion != nullptr,
                            "RegVersion must not be null based on ssa");
                CHECK_FATAL(!ccVersion->IsDeleted() && !curCCVersion->IsDeleted(), "deleted version");
                if (ccVersion->GetVersionIdx() != curCCVersion->GetVersionIdx()) {
                    return true;
                }
            }
        }
    }
    return false;
}

int64 CGPeepPattern::GetLogValueAtBase2(int64 val) const
{
    return (__builtin_popcountll(static_cast<uint64>(val)) == 1) ? (__builtin_ffsll(val) - 1) : -1;
}

InsnSet CGPeepPattern::GetAllUseInsn(const RegOperand &defReg) const
{
    InsnSet allUseInsn;
    if ((ssaInfo != nullptr) && defReg.IsSSAForm()) {
        VRegVersion *defVersion = ssaInfo->FindSSAVersion(defReg.GetRegisterNumber());
        CHECK_FATAL(defVersion != nullptr, "useVRegVersion must not be null based on ssa");
        for (auto insnInfo : defVersion->GetAllUseInsns()) {
            Insn *secondInsn = insnInfo.second->GetInsn();
            allUseInsn.emplace(secondInsn);
        }
    }
    return allUseInsn;
}

Insn *CGPeepPattern::GetDefInsn(const RegOperand &useReg)
{
    if (!useReg.IsSSAForm()) {
        return nullptr;
    }
    regno_t useRegNO = useReg.GetRegisterNumber();
    VRegVersion *useVersion = ssaInfo->FindSSAVersion(useRegNO);
    DEBUG_ASSERT(useVersion != nullptr, "useVRegVersion must not be null based on ssa");
    CHECK_FATAL(!useVersion->IsDeleted(), "deleted version");
    DUInsnInfo *defInfo = useVersion->GetDefInsnInfo();
    return defInfo == nullptr ? nullptr : defInfo->GetInsn();
}

void CGPeepPattern::DumpAfterPattern(std::vector<Insn *> &prevInsns, const Insn *replacedInsn, const Insn *newInsn)
{
    LogInfo::MapleLogger() << ">>>>>>> In " << GetPatternName() << " : <<<<<<<\n";
    if (!prevInsns.empty()) {
        if ((replacedInsn == nullptr) && (newInsn == nullptr)) {
            LogInfo::MapleLogger() << "======= RemoveInsns : {\n";
        } else {
            LogInfo::MapleLogger() << "======= PrevInsns : {\n";
        }
        for (auto *prevInsn : prevInsns) {
            if (prevInsn != nullptr) {
                LogInfo::MapleLogger() << "[primal form] ";
                prevInsn->Dump();
                if (ssaInfo != nullptr) {
                    LogInfo::MapleLogger() << "[ssa form] ";
                    ssaInfo->DumpInsnInSSAForm(*prevInsn);
                }
            }
        }
        LogInfo::MapleLogger() << "}\n";
    }
    if (replacedInsn != nullptr) {
        LogInfo::MapleLogger() << "======= OldInsn :\n";
        LogInfo::MapleLogger() << "[primal form] ";
        replacedInsn->Dump();
        if (ssaInfo != nullptr) {
            LogInfo::MapleLogger() << "[ssa form] ";
            ssaInfo->DumpInsnInSSAForm(*replacedInsn);
        }
    }
    if (newInsn != nullptr) {
        LogInfo::MapleLogger() << "======= NewInsn :\n";
        LogInfo::MapleLogger() << "[primal form] ";
        newInsn->Dump();
        if (ssaInfo != nullptr) {
            LogInfo::MapleLogger() << "[ssa form] ";
            ssaInfo->DumpInsnInSSAForm(*newInsn);
        }
    }
}

/* Check if a regOpnd is live after insn. True if live, otherwise false. */
bool CGPeepPattern::IfOperandIsLiveAfterInsn(const RegOperand &regOpnd, Insn &insn)
{
    for (Insn *nextInsn = insn.GetNext(); nextInsn != nullptr; nextInsn = nextInsn->GetNext()) {
        if (!nextInsn->IsMachineInstruction()) {
            continue;
        }
        CHECK_FATAL(nextInsn->GetOperandSize() > 0, "must not be zero");
        int32 lastOpndId = static_cast<int32>(nextInsn->GetOperandSize() - 1);
        for (int32 i = lastOpndId; i >= 0; --i) {
            Operand &opnd = nextInsn->GetOperand(static_cast<uint32>(i));
            if (opnd.IsMemoryAccessOperand()) {
                auto &mem = static_cast<MemOperand &>(opnd);
                Operand *base = mem.GetBaseRegister();
                Operand *offset = mem.GetOffset();

                if (base != nullptr && base->IsRegister()) {
                    auto *tmpRegOpnd = static_cast<RegOperand *>(base);
                    if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return true;
                    }
                }
                if (offset != nullptr && offset->IsRegister()) {
                    auto *tmpRegOpnd = static_cast<RegOperand *>(offset);
                    if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return true;
                    }
                }
            } else if (opnd.IsList()) {
                auto &opndList = static_cast<ListOperand &>(opnd).GetOperands();
                if (find(opndList.begin(), opndList.end(), &regOpnd) != opndList.end()) {
                    return true;
                }
            }

            if (!opnd.IsRegister()) {
                continue;
            }
            auto &tmpRegOpnd = static_cast<RegOperand &>(opnd);
            if (opnd.IsRegister() && tmpRegOpnd.GetRegisterNumber() != regOpnd.GetRegisterNumber()) {
                continue;
            }
            const InsnDesc *md = nextInsn->GetDesc();
            auto *regProp = (md->opndMD[static_cast<uint32>(i)]);
            bool isUse = regProp->IsUse();
            /* if noUse Redefined, no need to check live-out. */
            return isUse;
        }
    }
    /* Check if it is live-out. */
    return FindRegLiveOut(regOpnd, *insn.GetBB());
}

/* entrance for find if a regOpnd is live-out. */
bool CGPeepPattern::FindRegLiveOut(const RegOperand &regOpnd, const BB &bb)
{
    /*
     * Each time use peephole, index is initialized by the constructor,
     * and the internal_flags3 should be cleared.
     */
    if (PeepOptimizer::index == 0) {
        FOR_ALL_BB(currbb, cgFunc)
        {
            currbb->SetInternalFlag3(0);
        }
    }
    /* before each invoke check function, increase index. */
    ++PeepOptimizer::index;
    return CheckOpndLiveinSuccs(regOpnd, bb);
}

/* Check regOpnd in succs/ehSuccs. True is live-out, otherwise false. */
bool CGPeepPattern::CheckOpndLiveinSuccs(const RegOperand &regOpnd, const BB &bb) const
{
    std::stack<BB *> bbStack;
    bbStack.push(const_cast<BB *>(&bb));
    while (!bbStack.empty()) {
        BB *currentBB = bbStack.top();
        bbStack.pop();
        if (CheckRegLiveinReturnBB(regOpnd, *currentBB)) {
            return true;
        }
        // The traversal order of sibling nodes in the iterative version
        // is reversed compared to the recursive version
        for (auto succ : currentBB->GetSuccs()) {
            DEBUG_ASSERT(succ->GetInternalFlag3() <= PeepOptimizer::index, "internal error.");
            if (succ->GetInternalFlag3() == PeepOptimizer::index) {
                continue;
            }
            succ->SetInternalFlag3(PeepOptimizer::index);
            ReturnType result = IsOpndLiveinBB(regOpnd, *succ);
            if (result == kResNotFind) {
                bbStack.push(succ);
            } else if (result == kResUseFirst) {
                return true;
            } else if (result == kResDefFirst) {
                // Do nothing, continue to process successors
            }
        }
    }
    return false;
}

/* Check if the reg is used in return BB */
bool CGPeepPattern::CheckRegLiveinReturnBB(const RegOperand &regOpnd, const BB &bb) const
{
#if TARGAARCH64 || TARGRISCV64
    if (bb.GetKind() == BB::kBBReturn) {
        regno_t regNO = regOpnd.GetRegisterNumber();
        RegType regType = regOpnd.GetRegisterType();
        if (regType == kRegTyVary) {
            return false;
        }
        PrimType returnType = cgFunc->GetFunction().GetReturnType()->GetPrimType();
        regno_t returnReg = R0;
        if (IsPrimitiveFloat(returnType)) {
            returnReg = V0;
        } else if (IsPrimitiveInteger(returnType)) {
            returnReg = R0;
        }
        if (regNO == returnReg) {
            return true;
        }
    }
#endif
    return false;
}

/*
 * Check regNO in current bb:
 * kResUseFirst:first find use point; kResDefFirst:first find define point;
 * kResNotFind:cannot find regNO, need to continue searching.
 */
ReturnType CGPeepPattern::IsOpndLiveinBB(const RegOperand &regOpnd, const BB &bb) const
{
    FOR_BB_INSNS_CONST(insn, &bb)
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        const InsnDesc *md = insn->GetDesc();
        int32 lastOpndId = static_cast<int32>(insn->GetOperandSize() - 1);
        for (int32 i = lastOpndId; i >= 0; --i) {
            Operand &opnd = insn->GetOperand(static_cast<uint32>(i));
            auto *regProp = (md->opndMD[static_cast<uint32>(i)]);
            if (opnd.IsConditionCode()) {
                if (regOpnd.GetRegisterNumber() == kRFLAG) {
                    bool isUse = regProp->IsUse();
                    if (isUse) {
                        return kResUseFirst;
                    }
                    DEBUG_ASSERT(regProp->IsDef(), "register should be redefined.");
                    return kResDefFirst;
                }
            } else if (opnd.IsList()) {
                auto &listOpnd = static_cast<ListOperand &>(opnd);
                if (insn->GetMachineOpcode() == MOP_asm) {
                    if (static_cast<uint32>(i) == kAsmOutputListOpnd || static_cast<uint32>(i) == kAsmClobberListOpnd) {
                        for (const auto op : listOpnd.GetOperands()) {
                            if (op->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                                return kResDefFirst;
                            }
                        }
                        continue;
                    } else if (static_cast<uint32>(i) != kAsmInputListOpnd) {
                        continue;
                    }
                    /* fall thru for kAsmInputListOpnd */
                }
                for (const auto op : listOpnd.GetOperands()) {
                    if (op->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return kResUseFirst;
                    }
                }
            } else if (opnd.IsMemoryAccessOperand()) {
                auto &mem = static_cast<MemOperand &>(opnd);
                Operand *base = mem.GetBaseRegister();
                Operand *offset = mem.GetOffset();

                if (base != nullptr) {
                    DEBUG_ASSERT(base->IsRegister(), "internal error.");
                    auto *tmpRegOpnd = static_cast<RegOperand *>(base);
                    if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return kResUseFirst;
                    }
                }
                if (offset != nullptr && offset->IsRegister()) {
                    auto *tmpRegOpnd = static_cast<RegOperand *>(offset);
                    if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return kResUseFirst;
                    }
                }
            } else if (opnd.IsRegister()) {
                auto &tmpRegOpnd = static_cast<RegOperand &>(opnd);
                if (tmpRegOpnd.GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                    bool isUse = regProp->IsUse();
                    if (isUse) {
                        return kResUseFirst;
                    }
                    DEBUG_ASSERT(regProp->IsDef(), "register should be redefined.");
                    return kResDefFirst;
                }
            }
        }
    }
    return kResNotFind;
}

int PeepPattern::LogValueAtBase2(int64 val) const
{
    return (__builtin_popcountll(static_cast<uint64>(val)) == 1) ? (__builtin_ffsll(val) - 1) : (-1);
}

/* Check if a regOpnd is live after insn. True if live, otherwise false. */
bool PeepPattern::IfOperandIsLiveAfterInsn(const RegOperand &regOpnd, Insn &insn)
{
    for (Insn *nextInsn = insn.GetNext(); nextInsn != nullptr; nextInsn = nextInsn->GetNext()) {
        if (!nextInsn->IsMachineInstruction()) {
            continue;
        }
        CHECK_FATAL(nextInsn->GetOperandSize() > 0, "must not be zero");
        int32 lastOpndId = static_cast<int32>(nextInsn->GetOperandSize() - 1);
        for (int32 i = lastOpndId; i >= 0; --i) {
            Operand &opnd = nextInsn->GetOperand(static_cast<uint32>(i));
            if (opnd.IsMemoryAccessOperand()) {
                auto &mem = static_cast<MemOperand &>(opnd);
                Operand *base = mem.GetBaseRegister();
                Operand *offset = mem.GetOffset();

                if (base != nullptr && base->IsRegister()) {
                    auto *tmpRegOpnd = static_cast<RegOperand *>(base);
                    if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return true;
                    }
                }
                if (offset != nullptr && offset->IsRegister()) {
                    auto *tmpRegOpnd = static_cast<RegOperand *>(offset);
                    if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return true;
                    }
                }
            } else if (opnd.IsList()) {
                auto &opndList = static_cast<ListOperand &>(opnd).GetOperands();
                if (find(opndList.begin(), opndList.end(), &regOpnd) != opndList.end()) {
                    return true;
                }
            }

            if (!opnd.IsRegister()) {
                continue;
            }
            auto &tmpRegOpnd = static_cast<RegOperand &>(opnd);
            if (opnd.IsRegister() && tmpRegOpnd.GetRegisterNumber() != regOpnd.GetRegisterNumber()) {
                continue;
            }
            const InsnDesc *md = nextInsn->GetDesc();
            auto *regProp = (md->opndMD[static_cast<uint64>(i)]);
            bool isUse = regProp->IsUse();
            /* if noUse Redefined, no need to check live-out. */
            return isUse;
        }
    }
    /* Check if it is live-out. */
    return FindRegLiveOut(regOpnd, *insn.GetBB());
}

/* entrance for find if a regOpnd is live-out. */
bool PeepPattern::FindRegLiveOut(const RegOperand &regOpnd, const BB &bb)
{
    /*
     * Each time use peephole, index is initialized by the constructor,
     * and the internal_flags3 should be cleared.
     */
    if (PeepOptimizer::index == 0) {
        FOR_ALL_BB(currbb, &cgFunc)
        {
            currbb->SetInternalFlag3(0);
        }
    }
    /* before each invoke check function, increase index. */
    ++PeepOptimizer::index;
    return CheckOpndLiveinSuccs(regOpnd, bb);
}

/* Check regOpnd in succs/ehSuccs. True is live-out, otherwise false. */
bool PeepPattern::CheckOpndLiveinSuccs(const RegOperand &regOpnd, const BB &bb) const
{
    std::stack<BB *> bbStack;
    bbStack.push(const_cast<BB *>(&bb));
    while (!bbStack.empty()) {
        BB *currentBB = bbStack.top();
        bbStack.pop();
        if (CheckRegLiveinReturnBB(regOpnd, *currentBB)) {
            return true;
        }
        // The traversal order of sibling nodes in the iterative version
        // is reversed compared to the recursive version
        for (auto succ : currentBB->GetSuccs()) {
            DEBUG_ASSERT(succ->GetInternalFlag3() <= PeepOptimizer::index, "internal error.");
            if (succ->GetInternalFlag3() == PeepOptimizer::index) {
                continue;
            }
            succ->SetInternalFlag3(PeepOptimizer::index);
            ReturnType result = IsOpndLiveinBB(regOpnd, *succ);
            if (result == kResNotFind) {
                bbStack.push(succ);
            } else if (result == kResUseFirst) {
                return true;
            } else if (result == kResDefFirst) {
                // Do nothing, continue to process successors
            }
        }
    }
    return false;
}

/* Check if the reg is used in return BB */
bool PeepPattern::CheckRegLiveinReturnBB(const RegOperand &regOpnd, const BB &bb) const
{
#if TARGAARCH64 || TARGRISCV64
    if (bb.GetKind() == BB::kBBReturn) {
        regno_t regNO = regOpnd.GetRegisterNumber();
        RegType regType = regOpnd.GetRegisterType();
        if (regType == kRegTyVary) {
            return false;
        }
        PrimType returnType = cgFunc.GetFunction().GetReturnType()->GetPrimType();
        regno_t returnReg = R0;
        if (IsPrimitiveFloat(returnType)) {
            returnReg = V0;
        } else if (IsPrimitiveInteger(returnType)) {
            returnReg = R0;
        }
        if (regNO == returnReg) {
            return true;
        }
    }
#endif
    return false;
}

/*
 * Check regNO in current bb:
 * kResUseFirst:first find use point; kResDefFirst:first find define point;
 * kResNotFind:cannot find regNO, need to continue searching.
 */
ReturnType PeepPattern::IsOpndLiveinBB(const RegOperand &regOpnd, const BB &bb) const
{
    FOR_BB_INSNS_CONST(insn, &bb)
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        const InsnDesc *md = insn->GetDesc();
        int32 lastOpndId = static_cast<int32>(insn->GetOperandSize() - 1);
        for (int32 i = lastOpndId; i >= 0; --i) {
            Operand &opnd = insn->GetOperand(static_cast<uint32>(i));
            auto *regProp = (md->opndMD[static_cast<uint32>(i)]);
            if (opnd.IsConditionCode()) {
                if (regOpnd.GetRegisterNumber() == kRFLAG) {
                    bool isUse = regProp->IsUse();
                    if (isUse) {
                        return kResUseFirst;
                    }
                    DEBUG_ASSERT(regProp->IsDef(), "register should be redefined.");
                    return kResDefFirst;
                }
            } else if (opnd.IsList()) {
                auto &listOpnd = static_cast<ListOperand &>(opnd);
                if (insn->GetMachineOpcode() == MOP_asm) {
                    if (static_cast<uint32>(i) == kAsmOutputListOpnd || static_cast<uint32>(i) == kAsmClobberListOpnd) {
                        for (const auto op : listOpnd.GetOperands()) {
                            if (op->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                                return kResDefFirst;
                            }
                        }
                        continue;
                    } else if (static_cast<uint32>(i) != kAsmInputListOpnd) {
                        continue;
                    }
                    /* fall thru for kAsmInputListOpnd */
                }
                for (const auto op : listOpnd.GetOperands()) {
                    if (op->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return kResUseFirst;
                    }
                }
            } else if (opnd.IsMemoryAccessOperand()) {
                auto &mem = static_cast<MemOperand &>(opnd);
                Operand *base = mem.GetBaseRegister();
                Operand *offset = mem.GetOffset();

                if (base != nullptr) {
                    DEBUG_ASSERT(base->IsRegister(), "internal error.");
                    auto *tmpRegOpnd = static_cast<RegOperand *>(base);
                    if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return kResUseFirst;
                    }
                }
                if (offset != nullptr && offset->IsRegister()) {
                    auto *tmpRegOpnd = static_cast<RegOperand *>(offset);
                    if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return kResUseFirst;
                    }
                }
            } else if (opnd.IsRegister()) {
                auto &tmpRegOpnd = static_cast<RegOperand &>(opnd);
                if (tmpRegOpnd.GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                    bool isUse = regProp->IsUse();
                    if (isUse) {
                        return kResUseFirst;
                    }
                    DEBUG_ASSERT(regProp->IsDef(), "register should be redefined.");
                    return kResDefFirst;
                }
            }
        }
    }
    return kResNotFind;
}

bool PeepPattern::IsMemOperandOptPattern(const Insn &insn, Insn &nextInsn)
{
    /* Check if base register of nextInsn and the dest operand of insn are identical. */
    auto *memOpnd = static_cast<MemOperand *>(nextInsn.GetMemOpnd());
    DEBUG_ASSERT(memOpnd != nullptr, "null ptr check");
    /* Only for AddrMode_B_OI addressing mode. */
    if (memOpnd->GetAddrMode() != MemOperand::kAddrModeBOi) {
        return false;
    }
    /* Only for immediate is  0. */
    if (memOpnd->GetOffsetImmediate()->GetOffsetValue() != 0) {
        return false;
    }
    /* Only for intact memory addressing. */
    if (!memOpnd->IsIntactIndexed()) {
        return false;
    }

    auto &oldBaseOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    /* Check if dest operand of insn is idential with base register of nextInsn. */
    if (memOpnd->GetBaseRegister() != &oldBaseOpnd) {
        return false;
    }

#ifdef USE_32BIT_REF
    if (nextInsn.IsAccessRefField() && nextInsn.GetOperand(kInsnFirstOpnd).GetSize() > k32BitSize) {
        return false;
    }
#endif
    /* Check if x0 is used after ldr insn, and if it is in live-out. */
    if (IfOperandIsLiveAfterInsn(oldBaseOpnd, nextInsn)) {
        return false;
    }
    return true;
}

template <typename T>
void PeepOptimizer::Run()
{
    auto *patterMatcher = peepOptMemPool->New<T>(cgFunc, peepOptMemPool);
    patterMatcher->InitOpts();
    FOR_ALL_BB(bb, &cgFunc)
    {
        FOR_BB_INSNS_SAFE(insn, bb, nextInsn)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            patterMatcher->Run(*bb, *insn);
        }
    }
}

int32 PeepOptimizer::index = 0;

void PeepHoleOptimizer::Peephole0()
{
    auto memPool = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "peepholeOptObj");
    PeepOptimizer peepOptimizer(*cgFunc, memPool.get());
#if TARGAARCH64 || TARGRISCV64
    peepOptimizer.Run<AArch64PeepHole0>();
#endif
#if TARGARM32
    peepOptimizer.Run<Arm32PeepHole0>();
#endif
}

void PeepHoleOptimizer::PeepholeOpt()
{
    auto memPool = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "peepholeOptObj");
    PeepOptimizer peepOptimizer(*cgFunc, memPool.get());
#if TARGAARCH64 || TARGRISCV64
    peepOptimizer.Run<AArch64PeepHole>();
#endif
#if TARGARM32
    peepOptimizer.Run<Arm32PeepHole>();
#endif
}

void PeepHoleOptimizer::PrePeepholeOpt()
{
    auto memPool = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "peepholeOptObj");
    PeepOptimizer peepOptimizer(*cgFunc, memPool.get());
#if TARGAARCH64 || TARGRISCV64
    peepOptimizer.Run<AArch64PrePeepHole>();
#endif
#if TARGARM32
    peepOptimizer.Run<Arm32PrePeepHole>();
#endif
}

void PeepHoleOptimizer::PrePeepholeOpt1()
{
    auto memPool = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "peepholeOptObj");
    PeepOptimizer peepOptimizer(*cgFunc, memPool.get());
#if TARGAARCH64 || TARGRISCV64
    peepOptimizer.Run<AArch64PrePeepHole1>();
#endif
#if TARGARM32
    peepOptimizer.Run<Arm32PrePeepHole1>();
#endif
}

/* === SSA form === */
bool CgPeepHole::PhaseRun(maplebe::CGFunc &f)
{
    CGSSAInfo *cgssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
    CHECK_FATAL((cgssaInfo != nullptr), "Get ssaInfo failed!");
    MemPool *mp = GetPhaseMemPool();
    auto *cgpeep = mp->New<AArch64CGPeepHole>(f, mp, cgssaInfo);
    CHECK_FATAL((cgpeep != nullptr), "Creat AArch64CGPeepHole failed!");
    cgpeep->Run();
    return false;
}

void CgPeepHole::GetAnalysisDependence(AnalysisDep &aDep) const
{
    aDep.AddRequired<CgSSAConstruct>();
    aDep.AddPreserved<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPeepHole, cgpeephole)
#endif
/* === Physical Pre Form === */
bool CgPrePeepHole::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *mp = GetPhaseMemPool();
    CGPeepHole *cgpeep = f.GetCG()->CreateCGPeepHole(*mp, f);
    CHECK_FATAL(cgpeep != nullptr, "PeepHoleOptimizer instance create failure");
    cgpeep->Run();
    return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPrePeepHole, cgprepeephole)

/* === Physical Post Form === */
bool CgPostPeepHole::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *mp = GetPhaseMemPool();
    CGPeepHole *cgpeep = f.GetCG()->CreateCGPeepHole(*mp, f);
    CHECK_FATAL(cgpeep != nullptr, "PeepHoleOptimizer instance create failure");
    cgpeep->Run();
    return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPostPeepHole, cgpostpeephole)

#if TARGAARCH64
bool CgPrePeepHole0::PhaseRun(maplebe::CGFunc &f)
{
    auto *peep = GetPhaseMemPool()->New<PeepHoleOptimizer>(&f);
    CHECK_FATAL(peep != nullptr, "PeepHoleOptimizer instance create failure");
    peep->PrePeepholeOpt();
    return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPrePeepHole0, prepeephole)

bool CgPrePeepHole1::PhaseRun(maplebe::CGFunc &f)
{
    auto *peep = GetPhaseMemPool()->New<PeepHoleOptimizer>(&f);
    CHECK_FATAL(peep != nullptr, "PeepHoleOptimizer instance create failure");
    peep->PrePeepholeOpt1();
    return false;
}

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPrePeepHole1, prepeephole1)

bool CgPeepHole0::PhaseRun(maplebe::CGFunc &f)
{
    ReachingDefinition *reachingDef = nullptr;
    if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2) {
        reachingDef = GET_ANALYSIS(CgReachingDefinition, f);
    }
    if (reachingDef == nullptr || !f.GetRDStatus()) {
        GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgReachingDefinition::id);
        return false;
    }

    auto *peep = GetPhaseMemPool()->New<PeepHoleOptimizer>(&f);
    CHECK_FATAL(peep != nullptr, "PeepHoleOptimizer instance create failure");
    peep->Peephole0();
    return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPeepHole0, peephole0)

bool CgPeepHole1::PhaseRun(maplebe::CGFunc &f)
{
    auto *peep = GetPhaseMemPool()->New<PeepHoleOptimizer>(&f);
    CHECK_FATAL(peep != nullptr, "PeepHoleOptimizer instance create failure");
    peep->PeepholeOpt();
    return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPeepHole1, peephole)
#endif

} /* namespace maplebe */
