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

#ifndef MAPLEBE_INCLUDE_CG_PRESSURE_H
#define MAPLEBE_INCLUDE_CG_PRESSURE_H

#include "cgbb.h"
#include "cgfunc.h"

namespace maplebe {
struct RegList {
    Insn *insn;
    RegList *next;
};

#define FOR_ALL_REGCLASS(i) for (uint32 i = 0; i < static_cast<uint32>(RegPressure::GetMaxRegClassNum()); ++i)

class RegPressure {
public:
    explicit RegPressure(MapleAllocator &alloc)
        : regUses(alloc.Adapter()), regDefs(alloc.Adapter()), pressure(alloc.Adapter()), deadDefNum(alloc.Adapter())
    {
    }

    virtual ~RegPressure() = default;

    void DumpRegPressure() const;

    void SetRegUses(RegList *regList)
    {
        regUses.emplace_back(regList);
    }

    void SetRegDefs(size_t idx, RegList *regList)
    {
        if (idx < regDefs.size()) {
            regDefs[idx] = regList;
        } else {
            regDefs.emplace_back(regList);
        }
    }

    static void SetMaxRegClassNum(int32 maxClassNum)
    {
        maxRegClassNum = maxClassNum;
    }

    static int32 GetMaxRegClassNum()
    {
        return maxRegClassNum;
    }

    int32 GetPriority() const
    {
        return priority;
    }

    void SetPriority(int32 value)
    {
        priority = value;
    }

    int32 GetMaxDepth() const
    {
        return maxDepth;
    }

    void SetMaxDepth(int32 value)
    {
        maxDepth = value;
    }

    int32 GetNear() const
    {
        return near;
    }

    void SetNear(int32 value)
    {
        near = value;
    }

    int32 GetIncPressure() const
    {
        return incPressure;
    }

    void SetIncPressure(bool value)
    {
        incPressure = value;
    }
    const MapleVector<int32> &GetPressure() const
    {
        return pressure;
    }

    void IncPressureByIndex(uint32 index)
    {
        DEBUG_ASSERT(index < pressure.size(), "index out of range");
        ++pressure[index];
    }

    void DecPressureByIndex(uint32 index)
    {
        DEBUG_ASSERT(index < pressure.size(), "index out of range");
        --pressure[index];
    }

    void InitPressure()
    {
        pressure.resize(static_cast<uint64>(maxRegClassNum), 0);
        deadDefNum.resize(static_cast<uint64>(maxRegClassNum), 0);
        incPressure = false;
    }

    const MapleVector<int32> &GetDeadDefNum() const
    {
        return deadDefNum;
    }

    void IncDeadDefByIndex(uint32 index)
    {
        DEBUG_ASSERT(index < deadDefNum.size(), "index out of range");
        ++deadDefNum[index];
    }

    RegList *GetRegUses(size_t idx) const
    {
        return idx < regUses.size() ? regUses[idx] : nullptr;
    }

    void InitRegUsesSize(size_t size)
    {
        regUses.reserve(size);
    }

    RegList *GetRegDefs(size_t idx) const
    {
        return idx < regDefs.size() ? regDefs[idx] : nullptr;
    }

    void InitRegDefsSize(size_t size)
    {
        regDefs.reserve(size);
    }

    void SetHasPreg(bool value)
    {
        hasPreg = value;
    }

    bool GetHasPreg() const
    {
        return hasPreg;
    }

    void SetNumCall(int32 value)
    {
        callNum = value;
    }

    int32 GetNumCall() const
    {
        return callNum;
    }

    void SetHasNativeCallRegister(bool value)
    {
        hasNativeCallRegister = value;
    }

    bool GetHasNativeCallRegister() const
    {
        return hasNativeCallRegister;
    }

private:
    /* save reglist of every uses'register */
    MapleVector<RegList *> regUses;
    /* save reglist of every defs'register */
    MapleVector<RegList *> regDefs;

    /* the number of the node needs registers */
    MapleVector<int32> pressure;
    /* the count of dead define registers */
    MapleVector<int32> deadDefNum;
    /* max number of reg's class */
    static int32 maxRegClassNum;
    int32 priority = 0;
    int32 maxDepth = 0;
    int32 near = 0;
    /* the number of successor call */
    int32 callNum = 0;
    /* if a type register increase then set incPressure as true. */
    bool incPressure = false;
    /* if define physical register, set hasPreg as true */
    bool hasPreg = false;
    /* it is call native special register */
    bool hasNativeCallRegister = false;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_PRESSURE_H */
