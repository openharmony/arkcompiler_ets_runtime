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

#include "mad.h"
#include <string>
#include <algorithm>
#if TARGAARCH64
#include "aarch64_operand.h"
#elif defined(TARGRISCV64) && TARGRISCV64
#include "riscv64_operand.h"
#endif
#include "schedule.h"
#include "insn.h"

namespace maplebe {
const std::string kUnitName[] = {
#include "mplad_unit_name.def"
    "None",
};
/* Unit */
Unit::Unit(enum UnitId theUnitId) : unitId(theUnitId), unitType(kUnitTypePrimart), occupancyTable(), compositeUnits()
{
    MAD::AddUnit(*this);
}

Unit::Unit(enum UnitType theUnitType, enum UnitId theUnitId, int numOfUnits, ...)
    : unitId(theUnitId), unitType(theUnitType), occupancyTable()
{
    DEBUG_ASSERT(numOfUnits > 1, "CG internal error, composite unit with less than 2 unit elements.");
    va_list ap;
    va_start(ap, numOfUnits);

    for (int i = 0; i < numOfUnits; ++i) {
        compositeUnits.emplace_back(static_cast<Unit *>(va_arg(ap, Unit *)));
    }
    va_end(ap);

    MAD::AddUnit(*this);
}

/* return name of unit */
std::string Unit::GetName() const
{
    DEBUG_ASSERT(GetUnitId() <= kUnitIdLast, "Unexpected UnitID");
    return kUnitName[GetUnitId()];
}

/* If the unit is idle in the CYCLE, return true */
bool Unit::IsIdle(uint32 cycle) const
{
    if (unitType == kUnitTypeOr) {
        // For 'or'-unit, if one of them is idle, return true
        for (Unit *unit : compositeUnits) {
            if (unit->IsIdle(cycle)) {
                return true;
            }
        }
        return false;
    } else if (GetUnitType() == kUnitTypeAnd) {
        // For 'and'-unit, if all of them are idle, return true
        for (auto *unit : compositeUnits) {
            if (!unit->IsIdle(cycle)) {
                return false;
            }
        }
        return true;
    }
    return ((occupancyTable & std::bitset<kOccupyWidth>(1u << cycle)) == 0);
}

/* Occupy unit in the CYCLE */
void Unit::Occupy(uint32 cycle)
{
    if (GetUnitType() == kUnitTypeOr) {
        // For 'or'-unit, occupy the first idle unit
        for (auto unit : GetCompositeUnits()) {
            if (unit->IsIdle(cycle)) {
                unit->Occupy(cycle);
                return;
            }
        }
        // If there is no return before, there is an error.
        CHECK_FATAL(false, "when the instruction issue, all units it required must be idle");
    } else if (GetUnitType() == kUnitTypeAnd) {
        // For 'and'-unit, occupy all the unit
        for (auto unit : GetCompositeUnits()) {
            unit->Occupy(cycle);
        }
        return;
    }
    occupancyTable |= (1ULL << cycle);
}

/* Advance one cpu cycle, and update the occupation status of all units */
void Unit::AdvanceOneCycle()
{
    if (GetUnitType() != kUnitTypePrimart) {
        return;
    }
    occupancyTable = (occupancyTable >> 1);
}

/* Release all units. */
void Unit::Release()
{
    if (GetUnitType() != kUnitTypePrimart) {
        return;
    }
    occupancyTable = 0;
}

const std::vector<Unit *> &Unit::GetCompositeUnits() const
{
    return compositeUnits;
}

void Unit::PrintIndent(int indent) const
{
    for (int i = 0; i < indent; ++i) {
        LogInfo::MapleLogger() << " ";
    }
}

void Unit::Dump(int indent) const
{
    PrintIndent(indent);
    LogInfo::MapleLogger() << "Unit " << GetName() << " (ID " << GetUnitId() << "): ";
    LogInfo::MapleLogger() << "occupancyTable = " << occupancyTable << '\n';
}

std::bitset<kOccupyWidth> Unit::GetOccupancyTable() const
{
    return occupancyTable;
}

/* MAD */
int MAD::parallelism;
std::vector<Unit *> MAD::allUnits;
std::vector<Reservation *> MAD::allReservations;
std::array<std::array<MAD::BypassVector, kLtLast>, kLtLast> MAD::bypassArrays;

MAD::~MAD()
{
    for (auto unit : allUnits) {
        delete unit;
    }
    for (auto rev : allReservations) {
        delete rev;
    }
    for (auto &bypassArray : bypassArrays) {
        for (auto &bypassVector : bypassArray) {
            for (auto *bypass : bypassVector) {
                delete bypass;
            }
        }
    }
    allUnits.clear();
    allReservations.clear();
}

void MAD::InitUnits() const
{
#include "mplad_unit_define.def"
}

void MAD::InitReservation() const
{
#include "mplad_reservation_define.def"
}

void MAD::InitParallelism() const {
#include "mplad_arch_define.def"
}

/* Return the reservation by latencyType of the instruction */
Reservation *MAD::FindReservation(const Insn &insn) const
{
    uint32 insnType = insn.GetLatencyType();
    for (auto reservation : allReservations) {
        if (reservation->IsEqual(insnType)) {
            return reservation;
        }
    }
    return nullptr;
}

/* Return latency from producer instruction to consumer instruction */
int MAD::GetLatency(const Insn &def, const Insn &use) const
{
    int latency = BypassLatency(def, use);
    if (latency < 0) {
        latency = DefaultLatency(def);
    }
    return latency;
}

/* Return latency according to latency from producer instruction to consumer instruction */
int MAD::BypassLatency(const Insn &def, const Insn &use) const
{
    int latency = -1;
    DEBUG_ASSERT(def.GetLatencyType() < kLtLast, "out of range");
    DEBUG_ASSERT(use.GetLatencyType() < kLtLast, "out of range");
    BypassVector &bypassVec = bypassArrays[def.GetLatencyType()][use.GetLatencyType()];
    for (auto bypass : bypassVec) {
        if (bypass->CanBypass(def, use)) {
            latency = bypass->GetLatency();
            break;
        }
    }
    return latency;
}

/* Return default cost of the instruction */
int MAD::DefaultLatency(const Insn &insn) const
{
    Reservation *res = insn.GetDepNode()->GetReservation();
    return res != nullptr ? res->GetLatency() : 0;
}

/* In the dual-issue arch, if two slots are occupied in the current cycle,
 * return true
 */
bool MAD::IsFullIssued() const
{
    return !GetUnitByUnitId(kUnitIdSlot0)->IsIdle(0) && !GetUnitByUnitId(kUnitIdSlot1)->IsIdle(0);
}

void MAD::AdvanceOneCycleForAll() const
{
    for (auto unit : allUnits) {
        unit->AdvanceOneCycle();
    }
}

void MAD::ReleaseAllUnits() const
{
    for (auto unit : allUnits) {
        unit->Release();
    }
}

void MAD::SaveStates(std::vector<std::bitset<kOccupyWidth>> &occupyTable, int size) const
{
    int i = 0;
    for (auto unit : allUnits) {
        CHECK_FATAL(i < size, "unit number error");
        occupyTable[i] = unit->GetOccupancyTable();
        ++i;
    }
}

#define ADDBYPASS(DEFLTTY, USELTTY, LT) AddBypass(*(new Bypass(DEFLTTY, USELTTY, LT)))
#define ADDALUSHIFTBYPASS(DEFLTTY, USELTTY, LT) AddBypass(*(new AluShiftBypass(DEFLTTY, USELTTY, LT)))
#define ADDACCUMULATORBYPASS(DEFLTTY, USELTTY, LT) AddBypass(*(new AccumulatorBypass(DEFLTTY, USELTTY, LT)))
#define ADDSTOREADDRBYPASS(DEFLTTY, USELTTY, LT) AddBypass(*(new StoreAddrBypass(DEFLTTY, USELTTY, LT)))

void MAD::InitBypass() const
{
#include "mplad_bypass_define.def"
}

void MAD::RestoreStates(std::vector<std::bitset<kOccupyWidth>> &occupyTable, int size) const
{
    int i = 0;
    for (auto unit : allUnits) {
        CHECK_FATAL(i < size, "unit number error");
        unit->SetOccupancyTable(occupyTable[i]);
        ++i;
    }
}

bool Bypass::CanBypass(const Insn &defInsn, const Insn &useInsn) const
{
    (void)defInsn;
    (void)useInsn;
    return true;
}

/* Return true if the USEINSN is an arithmetic or logic or shift instruction,
 * and the defOpnd of DEFINSN is not used in shift operation of USEINSN.
 * e.g.
 * true: r3=r2+x1 -> r5=r4<<0x2+r3
 * false: r3=r2+x1 -> r5=r3<<0x2+r4
 */
bool AluShiftBypass::CanBypass(const Insn &defInsn, const Insn &useInsn) const
{
    RegOperand *productOpnd = nullptr;
    uint32 defOpndNum = defInsn.GetOperandSize();
    for (uint32 i = 0; i < defOpndNum; ++i) {
        if (defInsn.OpndIsDef(i)) {
            Operand &opnd = defInsn.GetOperand(i);
            CHECK_FATAL(opnd.IsRegister(), "invalid producer insn of type alu-shift");
            productOpnd = &static_cast<RegOperand &>(opnd);
            break;
        }
    }
    CHECK_NULL_FATAL(productOpnd);
    uint32 useOpndNum = useInsn.GetOperandSize();
    if (useOpndNum <= kInsnThirdOpnd) {  // operand_size <= 2
        return true;
    }
    Operand &lastUseOpnd = useInsn.GetOperand(useOpndNum - 1);
    if (lastUseOpnd.GetKind() != Operand::kOpdShift && lastUseOpnd.GetKind() == Operand::kOpdExtend &&
        lastUseOpnd.GetKind() == Operand::kOpdRegShift) {
        return true;
    }
    Operand &shiftOpnd = useInsn.GetOperand(useOpndNum - 2);
    if (shiftOpnd.GetKind() != Operand::kOpdRegister) {
        return true;
    }
    if (static_cast<RegOperand &>(shiftOpnd).GetRegisterNumber() == productOpnd->GetRegisterNumber()) {
        return false;
    }
    return true;
}

/* Return true if the defOpnd of DEFINSN is used in the accumulator
 * operation(MLA-like) of USEINSN. In aarch64, the MOPs are in {madd, msub,
 * fmadd, fmsub, fnmadd, fnmsub} e.g. The USEINSN is {madd|msub} and the defOpnd
 * is used in the following cases: true: r98=x0*x1 -> x0=x2*x3+r98
 * false:r98=x0*x1 -> x0=x2*r98+x3
 */
bool AccumulatorBypass::CanBypass(const Insn &defInsn, const Insn &useInsn) const
{
    RegOperand *productOpnd = nullptr;
    uint32 defOpndNum = defInsn.GetOperandSize();
    for (uint32 i = 0; i < defOpndNum; ++i) {
        if (defInsn.OpndIsDef(i)) {
            Operand &opnd = defInsn.GetOperand(i);
            CHECK_FATAL(opnd.IsRegister(), "invalid producer insn of type alu-shift");
            productOpnd = &static_cast<RegOperand &>(opnd);
            break;
        }
    }
    CHECK_NULL_FATAL(productOpnd);
    // Currently, the MOPs of valid USEINSN are in {madd, msub, fmadd, fmsub,
    // fnmadd, fnmsub}, that have four operands of LATENCYTYPE kLtMul and
    // kLtFpmac.
    uint32 useOpndNum = useInsn.GetOperandSize();
    if (useOpndNum != kInsnFifthOpnd) {  // operand_size != 4
        return false;
    }
    // Get accumulator operand
    Operand &accuOpnd = useInsn.GetOperand(useOpndNum - 1);
    CHECK_FATAL(accuOpnd.IsRegister(), "invalid consumer insn of type mul");
    if (static_cast<RegOperand &>(accuOpnd).GetRegisterNumber() == productOpnd->GetRegisterNumber()) {
        return true;
    }
    return false;
}

/* Return true if the USEINSN(an integer store) does not use the defOpnd of
 * DEFINSN to calculate the mem address. e.g. true: r96=r92+x2 -> str r96, [r92]
 * false: r96=r92+x2 -> str r92, [r96, #8]
 * false: r96=r92+x2 -> str r92, [r94, r96]
 */
bool StoreAddrBypass::CanBypass(const Insn &defInsn, const Insn &useInsn) const
{
    // Only for LATENCY-TYPE {kLtStore1, kLtStore2, kLtStore3plus}
    if (useInsn.GetLatencyType() != kLtStore1 && useInsn.GetLatencyType() != kLtStore2 &&
        useInsn.GetLatencyType() != kLtStore3plus) {
        return false;
    }
    RegOperand *productOpnd = nullptr;
    uint32 defOpndNum = defInsn.GetOperandSize();
    for (uint32 i = 0; i < defOpndNum; ++i) {
        if (defInsn.OpndIsDef(i)) {
            Operand &opnd = defInsn.GetOperand(i);
            CHECK_FATAL(opnd.IsRegister(), "invalid producer insn of type alu-shift");
            productOpnd = &static_cast<RegOperand &>(opnd);
            break;
        }
    }
    CHECK_NULL_FATAL(productOpnd);
    uint32 useOpndNum = useInsn.GetOperandSize();
    for (uint32 i = 0; i < useOpndNum; ++i) {
        Operand &opnd = useInsn.GetOperand(i);
        if (opnd.GetKind() == Operand::kOpdMem) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            RegOperand *baseOpnd = memOpnd.GetBaseRegister();
            RegOperand *indexOpnd = memOpnd.GetIndexRegister();
            if (baseOpnd != nullptr && baseOpnd->GetRegisterNumber() == productOpnd->GetRegisterNumber()) {
                return false;
            }
            if (indexOpnd != nullptr && indexOpnd->GetRegisterNumber() == productOpnd->GetRegisterNumber()) {
                return false;
            }
            break;
        }
    }
    return true;
}

/* Reservation */
Reservation::Reservation(LatencyType t, int l, int n, ...) : type(t), latency(l), unitNum(n)
{
    DEBUG_ASSERT(l >= 0, "CG internal error, latency and unitNum should not be less than 0.");
    DEBUG_ASSERT(n >= 0, "CG internal error, latency and unitNum should not be less than 0.");

    errno_t ret = memset_s(units, sizeof(Unit *) * kMaxUnit, 0, sizeof(Unit *) * kMaxUnit);
    CHECK_FATAL(ret == EOK, "call memset_s failed in Reservation");

    va_list ap;
    va_start(ap, n);
    for (uint32 i = 0; i < unitNum; ++i) {
        units[i] = static_cast<Unit *>(va_arg(ap, Unit *));
    }
    va_end(ap);

    MAD::AddReservation(*this);
    /* init slot */
    if (n > 0) {
        /* if there are units, init slot by units[0] */
        slot = GetSlotType(units[0]->GetUnitId());
    } else {
        slot = kSlotNone;
    }
}

const std::string kSlotName[] = {
    "SlotNone", "Slot0", "Slot1", "SlotAny", "Slots",
};

const std::string &Reservation::GetSlotName() const
{
    DEBUG_ASSERT(GetSlot() <= kSlots, "Unexpected slot");
    return kUnitName[GetSlot()];
}

/* Get slot type by unit id */
SlotType Reservation::GetSlotType(UnitId unitID) const
{
    switch (unitID) {
        case kUnitIdSlot0:
        case kUnitIdSlot0LdAgu:
        case kUnitIdSlot0StAgu:
            return kSlot0;

        case kUnitIdSlot1:
            return kSlot1;

        case kUnitIdSlotS:
        case kUnitIdSlotSHazard:
        case kUnitIdSlotSMul:
        case kUnitIdSlotSBranch:
        case kUnitIdSlotSAgen:
            return kSlotAny;

        case kUnitIdSlotD:
        case kUnitIdSlotDAgen:
            return kSlots;

        default:
            DEBUG_ASSERT(false, "unknown slot type!");
            return kSlotNone;
    }
}
} /* namespace maplebe */
