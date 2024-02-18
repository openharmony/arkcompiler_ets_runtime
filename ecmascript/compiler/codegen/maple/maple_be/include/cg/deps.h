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

#ifndef MAPLEBE_INCLUDE_CG_DEPS_H
#define MAPLEBE_INCLUDE_CG_DEPS_H

#include <array>
#include "mad.h"
#ifndef ONLY_C
#include "pressure.h"
#endif
#include "cgfunc.h"
namespace maplebe {
#define PRINT_STR_VAL(STR, VAL) (LogInfo::MapleLogger() << std::left << std::setw(12) << (STR) << (VAL) << " | ")
#define PRINT_VAL(VAL) (LogInfo::MapleLogger() << std::left << std::setw(12) << (VAL) << " | ")
enum DepType : uint8 {
    kDependenceTypeTrue,
    kDependenceTypeOutput,
    kDependenceTypeAnti,
    kDependenceTypeControl,
    kDependenceTypeMemAccess,  // for dependencies between insns of def-use memory operations (alternative true
                               // dependency)
    kDependenceTypeMembar,
    kDependenceTypeThrow,  // using in java
    kDependenceTypeSeparator,
    kDependenceTypeNone
};

inline const std::array<std::string, kDependenceTypeNone + 1> kDepTypeName = {
    "true-dep",   "output-dep", "anti-dep",      "control-dep", "memaccess-dep",
    "membar-dep", "throw-dep",  "separator-dep", "none-dep",
};

enum NodeType : uint8 { kNodeTypeNormal, kNodeTypeSeparator, kNodeTypeEmpty };

enum ScheduleState : uint8 {
    kNormal,
    kCandidate,
    kWaiting,
    kReady,
    kScheduled,
};

enum SimulateState : uint8 {
    kStateUndef,
    kRunning,  // the instruction is running
    kRetired,  // the instruction is executed and returns result
    kUndefined,
};

class DepNode;

class DepLink {
public:
    DepLink(DepNode &fromNode, DepNode &toNode, DepType typ) : from(fromNode), to(toNode), depType(typ), latency(0) {}
    virtual ~DepLink() = default;

    DepNode &GetFrom() const
    {
        return from;
    }
    DepNode &GetTo() const
    {
        return to;
    }
    void SetDepType(DepType dType)
    {
        depType = dType;
    }
    DepType GetDepType() const
    {
        return depType;
    }
    void SetLatency(uint32 lat)
    {
        latency = lat;
    }
    uint32 GetLatency() const
    {
        return latency;
    }

private:
    DepNode &from;
    DepNode &to;
    DepType depType;
    uint32 latency;
};

class RegPressure;
class DepNode {
public:
    DepNode(Insn &insn, MapleAllocator &alloc)
        : insn(&insn),
          units(nullptr),
          reservation(nullptr),
          unitNum(0),
          eStart(0),
          lStart(0),
          visit(0),
          type(kNodeTypeNormal),
          state(kNormal),
          index(0),
          simulateCycle(0),
          schedCycle(0),
          bruteForceSchedCycle(0),
          validPredsSize(0),
          validSuccsSize(0),
          topoPredsSize(0),
          preds(alloc.Adapter()),
          succs(alloc.Adapter()),
          comments(alloc.Adapter()),
          cfiInsns(alloc.Adapter()),
          clinitInsns(alloc.Adapter()),
          locInsn(nullptr),
          useRegnos(alloc.Adapter()),
          defRegnos(alloc.Adapter()),
          regPressure(nullptr)
    {
    }

    DepNode(Insn &insn, MapleAllocator &alloc, Unit *const *unit, uint32 num, Reservation &rev)
        : insn(&insn),
          units(unit),
          reservation(&rev),
          unitNum(num),
          eStart(0),
          lStart(0),
          visit(0),
          type(kNodeTypeNormal),
          state(kNormal),
          index(0),
          simulateCycle(0),
          schedCycle(0),
          bruteForceSchedCycle(0),
          validPredsSize(0),
          validSuccsSize(0),
          topoPredsSize(0),
          preds(alloc.Adapter()),
          succs(alloc.Adapter()),
          comments(alloc.Adapter()),
          cfiInsns(alloc.Adapter()),
          clinitInsns(alloc.Adapter()),
          locInsn(nullptr),
          useRegnos(alloc.Adapter()),
          defRegnos(alloc.Adapter()),
          regPressure(nullptr)
    {
    }

    virtual ~DepNode() = default;

    /* If the cpu units required by the reservation type of the instruction are
     * idle, return true.
     */
    bool IsResourceIdle() const
    {
        // The 'i' indicates cpu cycles, that 0 indicates the current cycle
        uint32 cycles = reservation->GetUnitNum();
        Unit *const *requiredUnits = reservation->GetUnit();
        for (uint32 i = 0; i < cycles; ++i) {
            Unit *unit = requiredUnits[i];
            if (unit != nullptr) {
                if (!unit->IsIdle(i)) {
                    return false;
                }
            }
        }
        return true;
    }

    /* When an instruction is issued, occupy all units required in the specific
     * cycle
     */
    void OccupyRequiredUnits() const
    {
        // The 'i' indicates cpu cycles, that 0 indicates the current cycle
        uint32 cycles = reservation->GetUnitNum();
        Unit *const *requiredUnits = reservation->GetUnit();
        for (uint32 i = 0; i < cycles; ++i) {
            Unit *unit = requiredUnits[i];
            if (unit != nullptr) {
                unit->Occupy(i);
            }
        }
    }

    /* Get unit kind of this node's units[0] */
    uint32 GetUnitKind() const
    {
        uint32 retValue = 0;
        if ((units == nullptr) || (units[0] == nullptr)) {
            return retValue;
        }

        switch (units[0]->GetUnitId()) {
            case kUnitIdSlotD:
                retValue |= kUnitKindSlot0;
                break;
            case kUnitIdAgen:
            case kUnitIdSlotSAgen:
                retValue |= kUnitKindAgen;
                break;
            case kUnitIdSlotDAgen:
                retValue |= kUnitKindAgen;
                retValue |= kUnitKindSlot0;
                break;
            case kUnitIdHazard:
            case kUnitIdSlotSHazard:
                retValue |= kUnitKindHazard;
                break;
            case kUnitIdCrypto:
                retValue |= kUnitKindCrypto;
                break;
            case kUnitIdMul:
            case kUnitIdSlotSMul:
                retValue |= kUnitKindMul;
                break;
            case kUnitIdDiv:
                retValue |= kUnitKindDiv;
                break;
            case kUnitIdBranch:
            case kUnitIdSlotSBranch:
                retValue |= kUnitKindBranch;
                break;
            case kUnitIdStAgu:
                retValue |= kUnitKindStAgu;
                break;
            case kUnitIdLdAgu:
                retValue |= kUnitKindLdAgu;
                break;
            case kUnitIdFpAluS:
            case kUnitIdFpAluD:
                retValue |= kUnitKindFpAlu;
                break;
            case kUnitIdFpMulS:
            case kUnitIdFpMulD:
                retValue |= kUnitKindFpMul;
                break;
            case kUnitIdFpDivS:
            case kUnitIdFpDivD:
                retValue |= kUnitKindFpDiv;
                break;
            case kUnitIdSlot0LdAgu:
                retValue |= kUnitKindSlot0;
                retValue |= kUnitKindLdAgu;
                break;
            case kUnitIdSlot0StAgu:
                retValue |= kUnitKindSlot0;
                retValue |= kUnitKindStAgu;
                break;
            default:
                break;
        }

        return retValue;
    }

    Insn *GetInsn() const
    {
        return insn;
    }
    void SetInsn(Insn &rvInsn)
    {
        insn = &rvInsn;
    }
    void SetUnits(Unit *const *unit)
    {
        units = unit;
    }
    const Unit *GetUnitByIndex(uint32 idx) const
    {
        DEBUG_ASSERT(idx < unitNum, "out of units");
        return units[idx];
    }
    Reservation *GetReservation() const
    {
        return reservation;
    }
    void SetReservation(Reservation &rev)
    {
        reservation = &rev;
    }
    uint32 GetUnitNum() const
    {
        return unitNum;
    }
    void SetUnitNum(uint32 num)
    {
        unitNum = num;
    }
    uint32 GetEStart() const
    {
        return eStart;
    }
    void SetEStart(uint32 start)
    {
        eStart = start;
    }
    uint32 GetLStart() const
    {
        return lStart;
    }
    void SetLStart(uint32 start)
    {
        lStart = start;
    }
    uint32 GetDelay() const
    {
        return delay;
    }
    void SetDelay(uint32 prio)
    {
        delay = prio;
    }
    uint32 GetVisit() const
    {
        return visit;
    }
    void SetVisit(uint32 visitVal)
    {
        visit = visitVal;
    }
    void IncreaseVisit()
    {
        ++visit;
    }
    NodeType GetType() const
    {
        return type;
    }
    void SetType(NodeType nodeType)
    {
        type = nodeType;
    }
    ScheduleState GetState() const
    {
        return state;
    }
    void SetState(ScheduleState scheduleState)
    {
        state = scheduleState;
    }
    uint32 GetIndex() const
    {
        return index;
    }
    void SetIndex(uint32 idx)
    {
        index = idx;
    }
    void SetSchedCycle(uint32 cycle)
    {
        schedCycle = cycle;
    }
    uint32 GetSchedCycle() const
    {
        return schedCycle;
    }
    void SetSimulateCycle(uint32 cycle)
    {
        simulateCycle = cycle;
    }
    uint32 GetSimulateCycle() const
    {
        return simulateCycle;
    }
    void SetBruteForceSchedCycle(uint32 cycle)
    {
        bruteForceSchedCycle = cycle;
    }
    uint32 GetBruteForceSchedCycle() const
    {
        return bruteForceSchedCycle;
    }
    void SetValidPredsSize(uint32 validSize)
    {
        validPredsSize = validSize;
    }
    uint32 GetValidPredsSize() const
    {
        return validPredsSize;
    }
    void DecreaseValidPredsSize()
    {
        --validPredsSize;
    }
    void IncreaseValidPredsSize()
    {
        ++validPredsSize;
    }
    uint32 GetValidSuccsSize() const
    {
        return validSuccsSize;
    }
    void SetValidSuccsSize(uint32 size)
    {
        validSuccsSize = size;
    }
    void DecreaseValidSuccsSize()
    {
        --validSuccsSize;
    }
    uint32 GetTopoPredsSize()
    {
        return topoPredsSize;
    }
    void SetTopoPredsSize(uint32 size)
    {
        topoPredsSize = size;
    }
    void IncreaseTopoPredsSize()
    {
        ++topoPredsSize;
    }
    void DecreaseTopoPredsSize()
    {
        --topoPredsSize;
    }
    const MapleVector<DepLink *> &GetPreds() const
    {
        return preds;
    }
    MapleVector<DepLink *>::iterator GetPredsBegin()
    {
        return preds.begin();
    }
    MapleVector<DepLink *>::iterator GetPredsEnd()
    {
        return preds.end();
    }
    void ReservePreds(size_t size)
    {
        preds.reserve(size);
    }
    void AddPred(DepLink &depLink)
    {
        preds.emplace_back(&depLink);
    }
    void RemovePred()
    {
        preds.pop_back();
    }
    /* For mock data dependency graph, do not use in normal process */
    void ErasePred(const DepLink &predLink)
    {
        for (auto iter = preds.begin(); iter != preds.end(); ++iter) {
            DepNode &predNode = (*iter)->GetFrom();
            if (predNode.GetInsn()->GetId() == predLink.GetFrom().GetInsn()->GetId()) {
                (void)preds.erase(iter);
                return;
            }
        }
    }
    const MapleVector<DepLink *> &GetSuccs() const
    {
        return succs;
    }
    MapleVector<DepLink *>::iterator GetSuccsBegin()
    {
        return succs.begin();
    }
    MapleVector<DepLink *>::iterator GetSuccsEnd()
    {
        return succs.end();
    }
    void ReserveSuccs(size_t size)
    {
        succs.reserve(size);
    }
    void AddSucc(DepLink &depLink)
    {
        succs.emplace_back(&depLink);
    }
    void RemoveSucc()
    {
        succs.pop_back();
    }
    /* For mock data dependency graph */
    MapleVector<DepLink *>::iterator EraseSucc(const MapleVector<DepLink *>::iterator iter)
    {
        return succs.erase(iter);
    }
    const MapleVector<Insn *> &GetComments() const
    {
        return comments;
    }
    void SetComments(MapleVector<Insn *> com)
    {
        comments = com;
    }
    void AddComments(Insn &addInsn)
    {
        comments.emplace_back(&addInsn);
    }
    void ClearComments()
    {
        comments.clear();
    }
    const MapleVector<Insn *> &GetCfiInsns() const
    {
        return cfiInsns;
    }
    void SetCfiInsns(MapleVector<Insn *> insns)
    {
        cfiInsns = insns;
    }
    void AddCfiInsn(Insn &curInsn)
    {
        (void)cfiInsns.emplace_back(&curInsn);
    }
    void ClearCfiInsns()
    {
        cfiInsns.clear();
    }
    const MapleVector<Insn *> &GetClinitInsns() const
    {
        return clinitInsns;
    }
    void SetClinitInsns(MapleVector<Insn *> insns)
    {
        clinitInsns = insns;
    }
    void AddClinitInsn(Insn &addInsn)
    {
        (void)clinitInsns.emplace_back(&addInsn);
    }
    const RegPressure *GetRegPressure() const
    {
        return regPressure;
    }
    void SetRegPressure(RegPressure &pressure)
    {
        regPressure = &pressure;
    }
    void DumpRegPressure() const
    {
        if (regPressure) {
            regPressure->DumpRegPressure();
        }
    }
    void InitPressure() const
    {
#ifndef ONLY_C
        regPressure->InitPressure();
#endif
    }
#ifndef ONLY_C
    const MapleVector<int32> &GetPressure() const
    {
        return regPressure->GetPressure();
    }
#endif

    void IncPressureByIndex(int32 idx) const
    {
#ifndef ONLY_C
        regPressure->IncPressureByIndex(static_cast<uint32>(idx));
#endif
    }
    void DecPressureByIndex(int32 idx) const
    {
#ifndef ONLY_C
        regPressure->DecPressureByIndex(static_cast<uint32>(idx));
#endif
    }
#ifndef ONLY_C
    const MapleVector<int32> &GetDeadDefNum() const
    {
        return regPressure->GetDeadDefNum();
    }
#endif
    void IncDeadDefByIndex(int32 idx) const
    {
#ifndef ONLY_C
        regPressure->IncDeadDefByIndex(static_cast<uint32>(idx));
#endif
    }

    void SetRegUses(RegList &regList) const
    {
#ifndef ONLY_C
        regPressure->SetRegUses(&regList);
#endif
    }
    void SetRegDefs(size_t idx, RegList *regList) const
    {
#ifndef ONLY_C
        regPressure->SetRegDefs(idx, regList);
#endif
    }

    bool GetIncPressure() const
    {
#ifndef ONLY_C
        return regPressure->GetIncPressure();
#else
        return false;
#endif
    }
    void SetIncPressure(bool value) const
    {
#ifndef ONLY_C
        regPressure->SetIncPressure(value);
#endif
    }
    int32 GetMaxDepth() const
    {
#ifndef ONLY_C
        return regPressure->GetMaxDepth();
#else
        return 0;
#endif
    }
    void SetMaxDepth(int32 value) const
    {
#ifndef ONLY_C
        regPressure->SetMaxDepth(value);
#endif
    }
    int32 GetNear() const
    {
#ifndef ONLY_C
        return regPressure->GetNear();
#else
        return 0;
#endif
    }
    void SetNear(int32 value) const
    {
#ifndef ONLY_C
        regPressure->SetNear(value);
#endif
    }
    int32 GetPriority() const
    {
#ifndef ONLY_C
        return regPressure->GetPriority();
#else
        return 0;
#endif
    }
    void SetPriority(int32 value) const
    {
#ifndef ONLY_C
        regPressure->SetPriority(value);
#endif
    }
    RegList *GetRegUses(size_t idx) const
    {
#ifndef ONLY_C
        return regPressure->GetRegUses(idx);
#else
        return nullptr;
#endif
    }
    void InitRegUsesSize(size_t size) const
    {
#ifndef ONLY_C
        regPressure->InitRegUsesSize(size);
#endif
    }
    RegList *GetRegDefs(size_t idx) const
    {
#ifndef ONLY_C
        return regPressure->GetRegDefs(idx);
#else
        return nullptr;
#endif
    }
    void InitRegDefsSize(size_t size) const
    {
#ifndef ONLY_C
        regPressure->InitRegDefsSize(size);
#endif
    }

    void SetNumCall(int32 value) const
    {
#ifndef ONLY_C
        regPressure->SetNumCall(value);
#endif
    }

    int32 GetNumCall() const
    {
#ifndef ONLY_C
        return regPressure->GetNumCall();
#else
        return 0;
#endif
    }

    void SetHasNativeCallRegister(bool value) const
    {
#ifndef ONLY_C
        regPressure->SetHasNativeCallRegister(value);
#endif
    }

    bool GetHasNativeCallRegister() const
    {
#ifndef ONLY_C
        return regPressure->GetHasNativeCallRegister();
#else
        return false;
#endif
    }

    const Insn *GetLocInsn() const
    {
        return locInsn;
    }
    void SetLocInsn(const Insn &locationInsn)
    {
        locInsn = &locationInsn;
    }

    /* printf dep-node's information of scheduling */
    void DumpSchedInfo() const
    {
        PRINT_STR_VAL("estart: ", eStart);
        PRINT_STR_VAL("lstart: ", lStart);
        PRINT_STR_VAL("visit: ", visit);
        PRINT_STR_VAL("state: ", state);
        PRINT_STR_VAL("index: ", index);
        PRINT_STR_VAL("validPredsSize: ", validPredsSize);
        PRINT_STR_VAL("validSuccsSize: ", validSuccsSize);
        LogInfo::MapleLogger() << '\n';

        constexpr int32 width = 12;
        LogInfo::MapleLogger() << std::left << std::setw(width) << "usereg: ";
        for (const auto &useReg : useRegnos) {
            LogInfo::MapleLogger() << "R" << useReg << " ";
        }
        LogInfo::MapleLogger() << "\n";
        LogInfo::MapleLogger() << std::left << std::setw(width) << "defreg: ";
        for (const auto &defReg : defRegnos) {
            LogInfo::MapleLogger() << "R" << defReg << " ";
        }
        LogInfo::MapleLogger() << "\n";
    }

    void SetHasPreg(bool value) const
    {
#ifndef ONLY_C
        regPressure->SetHasPreg(value);
#endif
    }

    bool GetHasPreg() const
    {
#ifndef ONLY_C
        return regPressure->GetHasPreg();
#else
        return false;
#endif
    }

    void AddUseReg(regno_t reg)
    {
        useRegnos.emplace_back(reg);
    }

    const MapleVector<regno_t> &GetUseRegnos() const
    {
        return useRegnos;
    }

    void AddDefReg(regno_t reg)
    {
        defRegnos.emplace_back(reg);
    }

    const MapleVector<regno_t> &GetDefRegnos() const
    {
        return defRegnos;
    }

    void SetSimulateState(SimulateState simulateState)
    {
        simuState = simulateState;
    }

    SimulateState GetSimulateState() const
    {
        return simuState;
    }

    void SetSimulateIssueCycle(uint32 cycle)
    {
        simulateIssueCycle = cycle;
    }

    uint32 GetSimulateIssueCycle() const
    {
        return simulateIssueCycle;
    }

private:
    Insn *insn;
    Unit *const *units;
    Reservation *reservation;
    uint32 unitNum;
    uint32 eStart;
    uint32 lStart;
    uint32 delay = 0;  // Identify the critical path priority
    uint32 visit;
    NodeType type;
    ScheduleState state;
    uint32 index;
    uint32 simulateCycle;
    uint32 schedCycle;
    uint32 bruteForceSchedCycle;

    /* For scheduling, denotes unscheduled preds/succs number. */
    uint32 validPredsSize;
    uint32 validSuccsSize;

    /* For compute eStart by topological order */
    uint32 topoPredsSize;

    /* Dependence links. */
    MapleVector<DepLink *> preds;
    MapleVector<DepLink *> succs;

    /* Non-machine instructions prior to insn, such as comments. */
    MapleVector<Insn *> comments;

    /* Non-machine instructions which follows insn, such as cfi instructions. */
    MapleVector<Insn *> cfiInsns;

    /* Special instructions which follows insn, such as clinit instructions. */
    MapleVector<Insn *> clinitInsns;

    /* loc insn which indicate insn location in source file */
    const Insn *locInsn;

    MapleVector<regno_t> useRegnos;
    MapleVector<regno_t> defRegnos;

    /* For register pressure analysis */
    RegPressure *regPressure;
    SimulateState simuState = kUndefined;  // For calculating original cycles of BB
    uint32 simulateIssueCycle = 0;  // For calculating original cycles of BB, record the cycle when the insn issuing
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_DEPS_H */
