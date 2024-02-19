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

#ifndef MAPLEBE_INCLUDE_AD_MAD_H
#define MAPLEBE_INCLUDE_AD_MAD_H
#include <vector>
#include <bitset>
#include "types_def.h"
#include "mpl_logging.h"
#include "insn.h"

namespace maplebe {
constexpr int kOccupyWidth = 32;
enum UnitId : maple::uint32 {
#include "mplad_unit_id.def"
    kUnitIdLast
};

enum UnitType : maple::uint8 { kUnitTypePrimart, kUnitTypeOr, kUnitTypeAnd, KUnitTypeNone };

enum RealUnitKind : maple::uint32 {
    kUnitKindUndef,
#include "mplad_unit_kind.def"
    kUnitKindLast = 13
};

enum SlotType : maple::uint8 {
    kSlotNone,
    kSlot0,
    kSlot1,
    kSlotAny,
    kSlots,
};

/* machine model */
enum LatencyType : maple::uint32 {
/* LT: latency */
#include "mplad_latency_type.def"
    kLtLast,
};

class Unit {
public:
    explicit Unit(enum UnitId theUnitId);
    Unit(enum UnitType theUnitType, enum UnitId theUnitId, int numOfUnits, ...);
    ~Unit() = default;

    enum UnitType GetUnitType() const
    {
        return unitType;
    }

    enum UnitId GetUnitId() const
    {
        return unitId;
    };

    const std::vector<Unit *> &GetCompositeUnits() const;

    std::string GetName() const;
    bool IsIdle(uint32 cycle) const;
    void Occupy(uint32 cycle);
    void Release();
    void AdvanceOneCycle();
    void Dump(int indent = 0) const;
    std::bitset<kOccupyWidth> GetOccupancyTable() const;

    void SetOccupancyTable(const std::bitset<kOccupyWidth> value)
    {
        occupancyTable = value;
    }

private:
    void PrintIndent(int indent) const;

    enum UnitId unitId;
    enum UnitType unitType;
    // using 32-bit vector simulating resource occupation, the LSB always indicates the current cycle by AdvanceOneCycle
    std::bitset<kOccupyWidth> occupancyTable;
    std::vector<Unit *> compositeUnits;
};

class Reservation {
public:
    Reservation(LatencyType t, int l, int n, ...);
    ~Reservation() = default;

    bool IsEqual(maple::uint32 typ) const
    {
        return typ == type;
    }

    int GetLatency() const
    {
        return latency;
    }

    uint32 GetUnitNum() const
    {
        return unitNum;
    }

    enum SlotType GetSlot() const
    {
        return slot;
    }

    const std::string &GetSlotName() const;

    Unit *const *GetUnit() const
    {
        return units;
    }

private:
    static const int kMaxUnit = 13;
    LatencyType type;
    int latency;
    uint32 unitNum;
    Unit *units[kMaxUnit];
    enum SlotType slot = kSlotNone;

    SlotType GetSlotType(UnitId unitID) const;
};

class Bypass {
public:
    Bypass(LatencyType d, LatencyType u, int l) : def(d), use(u), latency(l) {}
    virtual ~Bypass() = default;

    virtual bool CanBypass(const Insn &defInsn, const Insn &useInsn) const;

    int GetLatency() const
    {
        return latency;
    }

    LatencyType GetDefType() const
    {
        return def;
    }

    LatencyType GetUseType() const
    {
        return use;
    }

private:
    LatencyType def;
    LatencyType use;
    int latency;
};

class MAD {
public:
    MAD()
    {
        InitUnits();
        InitParallelism();
        InitReservation();
        InitBypass();
    }

    ~MAD();

    using BypassVector = std::vector<Bypass *>;

    void InitUnits() const;
    void InitParallelism() const;
    void InitReservation() const;
    void InitBypass() const;
    bool IsFullIssued() const;
    int GetLatency(const Insn &def, const Insn &use) const;
    int DefaultLatency(const Insn &insn) const;
    Reservation *FindReservation(const Insn &insn) const;
    void AdvanceOneCycleForAll() const;
    void ReleaseAllUnits() const;
    void SaveStates(std::vector<std::bitset<kOccupyWidth>> &occupyTable, int size) const;
    void RestoreStates(std::vector<std::bitset<kOccupyWidth>> &occupyTable, int size) const;

    int GetMaxParallelism() const
    {
        return parallelism;
    }

    const Unit *GetUnitByUnitId(enum UnitId uId) const
    {
        CHECK_FATAL(!allUnits.empty(), "CHECK_CONTAINER_EMPTY");
        return allUnits[uId];
    }

    static void AddUnit(Unit &u)
    {
        allUnits.emplace_back(&u);
    }

    static maple::uint32 GetAllUnitsSize()
    {
        return allUnits.size();
    }

    static void AddReservation(Reservation &rev)
    {
        allReservations.emplace_back(&rev);
    }

    static void AddBypass(Bypass &bp)
    {
        DEBUG_ASSERT(bp.GetDefType() < kLtLast, "out of range");
        DEBUG_ASSERT(bp.GetUseType() < kLtLast, "out of range");
        (bypassArrays[bp.GetDefType()][bp.GetUseType()]).push_back(&bp);
    }

protected:
    static void SetMaxParallelism(int num)
    {
        parallelism = num;
    }

    int BypassLatency(const Insn &def, const Insn &use) const;

private:
    static int parallelism;
    static std::vector<Unit *> allUnits;
    static std::vector<Reservation *> allReservations;
    static std::array<std::array<BypassVector, kLtLast>, kLtLast> bypassArrays;
};

class AluShiftBypass : public Bypass {
public:
    AluShiftBypass(LatencyType d, LatencyType u, int l) : Bypass(d, u, l) {}
    ~AluShiftBypass() override = default;

    bool CanBypass(const Insn &defInsn, const Insn &useInsn) const override;
};

class AccumulatorBypass : public Bypass {
public:
    AccumulatorBypass(LatencyType d, LatencyType u, int l) : Bypass(d, u, l) {}
    ~AccumulatorBypass() override = default;

    bool CanBypass(const Insn &defInsn, const Insn &useInsn) const override;
};

class StoreAddrBypass : public Bypass {
public:
    StoreAddrBypass(LatencyType d, LatencyType u, int l) : Bypass(d, u, l) {}
    ~StoreAddrBypass() override = default;

    bool CanBypass(const Insn &defInsn, const Insn &useInsn) const override;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_AD_MAD_H */
