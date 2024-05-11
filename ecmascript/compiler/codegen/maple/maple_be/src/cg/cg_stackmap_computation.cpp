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

#include "cg_stackmap_computation.h"
#include "live.h"

namespace maplebe {
#define STACKMAP_DUMP (CG_DEBUG_FUNC(f))

void StackmapComputation::SetStackmapDerivedInfo()
{
    FOR_ALL_BB(bb, &cgFunc)
    {
        FOR_BB_INSNS(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            const InsnDesc *md = insn->GetDesc();
            uint32 opndNum = insn->GetOperandSize();
            for (uint32 i = 0; i < opndNum; ++i) {
                const OpndDesc *regProp = md->GetOpndDes(i);
                DEBUG_ASSERT(regProp != nullptr, "pointer is null in StackmapComputation::SetStackMapDerivedInfo");
                bool isDef = regProp->IsRegDef();
                Operand &opnd = insn->GetOperand(i);
                if (isDef) {
                    auto &regOpnd = static_cast<RegOperand &>(opnd);
                    uint32 regNO = regOpnd.GetRegisterNumber();
                    if (regOpnd.GetBaseRefOpnd() != nullptr) {
                        // set the base reference of derived reference for stackmap
                        derivedRef2Base[regNO] = regOpnd.GetBaseRefOpnd();
                    }
                }
            }
        }
    }
}

MemOperand *StackmapComputation::GetSpillMem(uint32 vRegNO, bool isDest, Insn &insn, regno_t regNO,
                                             bool &isOutOfRange, uint32 bitSize)
{
    MemOperand *memOpnd = regInfo->GetOrCreatSpillMem(vRegNO, bitSize);
    return regInfo->AdjustMemOperandIfOffsetOutOfRange(memOpnd, std::make_pair(vRegNO, regNO), isDest, insn,
                                                       isOutOfRange);
}

void StackmapComputation::SpillOperand(Insn &insn, regno_t regNO)
{
    bool isOutOfRange = false;
    uint32 regSize = GetPrimTypeSize(PTY_ref) * k8BitSize;
    PrimType spType = PTY_ref;
#if TARGX86_64
    RegOperand &srcOpnd = cgFunc.GetOpndBuilder()->CreateVReg(regNO, regSize, cgFunc.GetRegTyFromPrimTy(spType));
#else
    RegOperand &srcOpnd = cgFunc.GetOrCreateVirtualRegisterOperand(regNO);
#endif

    MemOperand *memOpnd = GetSpillMem(regNO, true, insn, kInvalidRegNO, isOutOfRange, regSize);
    Insn *stInsn = regInfo->BuildStrInsn(regSize, spType, srcOpnd, *memOpnd);
    insn.GetBB()->InsertInsnBefore(insn, *stInsn);
}

void StackmapComputation::LoadOperand(Insn &insn, regno_t regNO)
{
    bool isOutOfRange = false;
    uint32 regSize = GetPrimTypeSize(PTY_ref) * k8BitSize;
    PrimType spType = PTY_ref;

#if TARGX86_64
    RegOperand &resOpnd = cgFunc.GetOpndBuilder()->CreateVReg(regNO, regSize, cgFunc.GetRegTyFromPrimTy(spType));
#else
    RegOperand &resOpnd = cgFunc.GetOrCreateVirtualRegisterOperand(regNO);
#endif

    Insn *nextInsn = insn.GetNext();
    MemOperand *memOpnd = GetSpillMem(regNO, false, insn, kInvalidRegNO, isOutOfRange, regSize);
    Insn *ldInsn = regInfo->BuildLdrInsn(regSize, spType, resOpnd, *memOpnd);
    if (isOutOfRange && nextInsn != nullptr) {
        insn.GetBB()->InsertInsnBefore(*nextInsn, *ldInsn);
    } else if (isOutOfRange && nextInsn == nullptr) {
        insn.GetBB()->AppendInsn(*ldInsn);
    } else {
        insn.GetBB()->InsertInsnAfter(insn, *ldInsn);
    }
}

void StackmapComputation::RelocateStackmapInfo()
{
    const auto &referenceMapInsns = cgFunc.GetStackMapInsns();
    for (auto *insn : referenceMapInsns) {
        auto &deoptInfo = insn->GetStackMap()->GetDeoptInfo();
        const auto &deoptBundleInfo = deoptInfo.GetDeoptBundleInfo();
        for (const auto &item : deoptBundleInfo) {
            const auto *opnd = item.second;
            if (!opnd->IsRegister()) {
                continue;
            }
            regno_t regNO = (static_cast<const RegOperand *>(opnd))->GetRegisterNumber();
            SpillOperand(*insn, regNO);
            LoadOperand(*insn, regNO);
        }

        for (auto regNO : insn->GetStackMapLiveIn()) {
            if (!cgFunc.IsRegReference(regNO)) {
                continue;
            }
            auto itr = derivedRef2Base.find(regNO);
            if (itr != derivedRef2Base.end()) {
                SpillOperand(*insn, itr->second->GetRegisterNumber());
                LoadOperand(*insn, itr->second->GetRegisterNumber());
            }
            SpillOperand(*insn, regNO);
            LoadOperand(*insn, regNO);
        }
    }
}

void StackmapComputation::CollectReferenceMap()
{
    const auto &referenceMapInsns = cgFunc.GetStackMapInsns();
    if (needDump) {
        LogInfo::MapleLogger() << "===========reference map stack info================\n";
    }

    int insnNum = 0;

    for (auto *insn : referenceMapInsns) {
        insnNum++;
        for (auto regNO : insn->GetStackMapLiveIn()) {
            if (!cgFunc.IsRegReference(regNO)) {
                continue;
            }

            auto itr = derivedRef2Base.find(regNO);
            if (itr != derivedRef2Base.end()) {
                auto baseRegNum = (itr->second)->GetRegisterNumber();
                MemOperand *baseRegMemOpnd = cgFunc.GetOrCreatSpillMem(baseRegNum, k64BitSize);
                int64 baseRefMemoffset = baseRegMemOpnd->GetOffsetImmediate()->GetOffsetValue();
                insn->GetStackMap()->GetReferenceMap().ReocordStackRoots(baseRefMemoffset);
                if (needDump) {
                    LogInfo::MapleLogger() << "--------insn num: " << insnNum
                                           << " base regNO: " << baseRegNum << " offset: "
                                           << baseRefMemoffset << std::endl;
                }
            }
            MemOperand *memOperand = cgFunc.GetOrCreatSpillMem(regNO, k64BitSize);
            int64 offset = memOperand->GetOffsetImmediate()->GetOffsetValue();
            insn->GetStackMap()->GetReferenceMap().ReocordStackRoots(offset);
            if (itr == derivedRef2Base.end()) {
                insn->GetStackMap()->GetReferenceMap().ReocordStackRoots(offset);
            }
            if (needDump) {
                LogInfo::MapleLogger() << "--------insn num: " << insnNum << " regNO: " << regNO
                                       << " offset: " << offset << std::endl;
            }
        }
    }

    if (needDump) {
        LogInfo::MapleLogger() << "===========reference map stack info end================\n";
    }
    if (needDump) {
        LogInfo::MapleLogger() << "===========reference map info================\n";
        for (auto *insn : referenceMapInsns) {
            LogInfo::MapleLogger() << "  referenceMap insn: ";
            insn->Dump();
            insn->GetStackMap()->GetReferenceMap().Dump();
        }
    }
}

void StackmapComputation::SolveRegOpndDeoptInfo(const RegOperand &regOpnd, DeoptInfo &deoptInfo,
                                                int32 deoptVregNO) const
{
    if (!regOpnd.IsVirtualRegister()) {
        // Get Register No
        deoptInfo.RecordDeoptVreg2LocationInfo(deoptVregNO, LocationInfo({kInRegister, 0}));
        return;
    }
    // process virtual RegOperand
    regno_t vRegNO = regOpnd.GetRegisterNumber();
    MemOperand *memOpnd = cgFunc.GetOrCreatSpillMem(vRegNO, regOpnd.GetSize());
    SolveMemOpndDeoptInfo(*(static_cast<const MemOperand *>(memOpnd)), deoptInfo, deoptVregNO);
}

void StackmapComputation::SolveMemOpndDeoptInfo(const MemOperand &memOpnd, DeoptInfo &deoptInfo,
                                                int32 deoptVregNO) const
{
    int64 offset = memOpnd.GetOffsetImmediate()->GetOffsetValue();
    deoptInfo.RecordDeoptVreg2LocationInfo(deoptVregNO, LocationInfo({kOnStack, offset}));
}

void StackmapComputation::CollectDeoptInfo()
{
    const auto referenceMapInsns = cgFunc.GetStackMapInsns();
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
    if (needDump) {
        LogInfo::MapleLogger() << "===========deopt info================\n";
        for (auto *insn : referenceMapInsns) {
            LogInfo::MapleLogger() << "---- deoptInfo insn: ";
            insn->Dump();
            insn->GetStackMap()->GetDeoptInfo().Dump();
        }
    }
}

void StackmapComputation::run()
{
    if (needDump) {
        LogInfo::MapleLogger() << "===========StackmapComputation PhaseRun================\n";
    }
    SetStackmapDerivedInfo();
    RelocateStackmapInfo();
    CollectReferenceMap();
    CollectDeoptInfo();
}

bool CgStackmapComputation::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *phaseMp = GetPhaseMemPool();
    LiveAnalysis *live = nullptr;
    live = GET_ANALYSIS(CgLiveAnalysis, f);
    StackmapComputation *stackmapComputation = nullptr;
    stackmapComputation = GetPhaseAllocator()->New<StackmapComputation>(f, *phaseMp);
    stackmapComputation->SetNeedDump(STACKMAP_DUMP);
    stackmapComputation->run();

    if (live != nullptr) {
        live->ClearInOutDataInfo();
    }

    f.SetStackMapComputed();

    return true;
}

void CgStackmapComputation::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgLiveAnalysis>();
}

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgStackmapComputation, stackmapcomputation)
}  // namespace maplebe
