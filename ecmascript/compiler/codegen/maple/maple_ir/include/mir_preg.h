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

#ifndef MAPLE_IR_INCLUDE_MIR_PREG_H
#define MAPLE_IR_INCLUDE_MIR_PREG_H
#if MIR_FEATURE_FULL
#include <climits>
#include "mir_module.h"
#include "global_tables.h"
#endif  // MIR_FEATURE_FULL

namespace maple {
extern void PrintIndentation(int32 indent);

// these special registers are encoded by negating the enumeration
enum SpecialReg : signed int {
    kSregSp = 1,
    kSregFp = 2,
    kSregRetval0 = 3,
    kSregRetval1 = 4,
    kSregRetval2 = 5,
    kSregRetval3 = 6,
    kSregLast = 7,
};
#if MIR_FEATURE_FULL
class MIRPreg {
public:
    explicit MIRPreg(uint32 n = 0) : MIRPreg(n, kPtyInvalid, nullptr) {}

    MIRPreg(uint32 n, PrimType ptyp) : primType(ptyp), pregNo(n) {}

    MIRPreg(uint32 n, PrimType ptyp, MIRType *mType) : primType(ptyp), pregNo(n), mirType(mType) {}

    ~MIRPreg() = default;
    void SetNeedRC(bool newNeedRC = true)
    {
        this->needRC = newNeedRC;
    }

    bool NeedRC() const
    {
        return needRC;
    }

    bool IsRef() const
    {
        return mirType != nullptr && primType == PTY_ref;
    }

    PrimType GetPrimType() const
    {
        return primType;
    }

    void SetPrimType(PrimType pty)
    {
        primType = pty;
    }

    Opcode GetOp() const
    {
        return op;
    }

    void SetOp(Opcode o)
    {
        this->op = o;
    }

    int32 GetPregNo() const
    {
        return pregNo;
    }

    void SetPregNo(int32 newPregNo)
    {
        this->pregNo = newPregNo;
    }

    MIRType *GetMIRType() const
    {
        return mirType;
    }

    void SetMIRType(MIRType *newMirType)
    {
        this->mirType = newMirType;
    }

private:
    PrimType primType = kPtyInvalid;
    bool needRC = false;
    Opcode op = OP_undef;  // OP_constval, OP_addrof or OP_dread if rematerializable
    int32 pregNo;          // the number in maple IR after the %
    MIRType *mirType = nullptr;

public:
    union RematInfo {
        const MIRConst *mirConst;  // used only when op is OP_constval
        const MIRSymbol *sym;      // used only when op is OP_addrof or OP_dread
    } rematInfo;
    FieldID fieldID = 0;     // used only when op is OP_addrof or OP_dread
    bool addrUpper = false;  // used only when op is OP_addrof to indicate upper bits of address
};

class MIRPregTable {
public:
    explicit MIRPregTable(MapleAllocator *allocator)
        : pregNoToPregIdxMap(allocator->Adapter()), pregTable(allocator->Adapter()), mAllocator(allocator)
    {
        pregTable.push_back(nullptr);
        specPregTable[0].SetPregNo(0);
        specPregTable[kSregSp].SetPregNo(-kSregSp);
        specPregTable[kSregFp].SetPregNo(-kSregFp);
        specPregTable[kSregRetval0].SetPregNo(-kSregRetval0);
        specPregTable[kSregRetval1].SetPregNo(-kSregRetval1);
        for (uint32 i = 0; i < kSregLast; ++i) {
            specPregTable[i].SetPrimType(PTY_unknown);
        }
    }

    ~MIRPregTable();

    PregIdx CreatePreg(PrimType primType, MIRType *mtype = nullptr)
    {
        DEBUG_ASSERT(!mtype || mtype->GetPrimType() == PTY_ref || mtype->GetPrimType() == PTY_ptr, "ref or ptr type");
        uint32 index = ++maxPregNo;
        auto *preg = mAllocator->GetMemPool()->New<MIRPreg>(index, primType, mtype);
        return AddPreg(*preg);
    }

    PregIdx ClonePreg(const MIRPreg &rfpreg)
    {
        PregIdx idx = CreatePreg(rfpreg.GetPrimType(), rfpreg.GetMIRType());
        MIRPreg *preg = pregTable[static_cast<uint32>(idx)];
        preg->SetNeedRC(rfpreg.NeedRC());
        return idx;
    }

    MIRPreg *PregFromPregIdx(PregIdx pregidx)
    {
        if (pregidx < 0) {  // special register
            return &specPregTable[-pregidx];
        } else {
            return pregTable.at(static_cast<uint32>(pregidx));
        }
    }

    PregIdx GetPregIdxFromPregno(uint32 pregNo)
    {
        auto it = pregNoToPregIdxMap.find(pregNo);
        return (it == pregNoToPregIdxMap.end()) ? PregIdx(0) : it->second;
    }
#ifdef ARK_LITECG_DEBUG
    void DumpPregsWithTypes(int32 indent)
    {
        MapleVector<MIRPreg *> &pregtable = pregTable;
        for (uint32 i = 1; i < pregtable.size(); i++) {
            MIRPreg *mirpreg = pregtable[i];
            if (mirpreg->GetMIRType() == nullptr) {
                continue;
            }
            PrintIndentation(indent);
            LogInfo::MapleLogger() << "reg ";
            LogInfo::MapleLogger() << "%" << mirpreg->GetPregNo();
            LogInfo::MapleLogger() << " ";
            mirpreg->GetMIRType()->Dump(0);
            LogInfo::MapleLogger() << " " << (mirpreg->NeedRC() ? 1 : 0);
            LogInfo::MapleLogger() << "\n";
        }
    }
#endif
    size_t Size() const
    {
        return pregTable.size();
    }

    PregIdx AddPreg(MIRPreg &preg)
    {
        PregIdx idx = static_cast<PregIdx>(pregTable.size());
        pregTable.push_back(&preg);
        DEBUG_ASSERT(pregNoToPregIdxMap.find(preg.GetPregNo()) == pregNoToPregIdxMap.end(),
                     "The same pregno is already taken");
        pregNoToPregIdxMap[preg.GetPregNo()] = idx;
        return idx;
    }

    PregIdx EnterPregNo(uint32 pregNo, PrimType ptyp, MIRType *ty = nullptr)
    {
        PregIdx idx = GetPregIdxFromPregno(pregNo);
        if (idx == 0) {
            if (pregNo > maxPregNo) {
                maxPregNo = pregNo;
            }
            MIRPreg *preg = mAllocator->GetMemPool()->New<MIRPreg>(pregNo, ptyp, ty);
            return AddPreg(*preg);
        }
        return idx;
    }

    MapleVector<MIRPreg *> &GetPregTable()
    {
        return pregTable;
    }

    const MapleVector<MIRPreg *> &GetPregTable() const
    {
        return pregTable;
    }

    const MIRPreg *GetPregTableItem(const uint32 index) const
    {
        CHECK_FATAL(index < pregTable.size(), "array index out of range");
        return pregTable[index];
    }

    void SetPregNoToPregIdxMapItem(uint32 key, PregIdx value)
    {
        pregNoToPregIdxMap[key] = value;
    }

    uint32 GetMaxPregNo() const
    {
        return maxPregNo;
    }

    void SetMaxPregNo(uint32 index)
    {
        maxPregNo = index;
    }

    size_t SpecPregSize()
    {
        return kSregLast;
    }

private:
    uint32 maxPregNo = 0;                          //  the max pregNo that has been allocated
    MapleMap<uint32, PregIdx> pregNoToPregIdxMap;  // for quick lookup based on pregno
    MapleVector<MIRPreg *> pregTable;
    MIRPreg specPregTable[kSregLast];  // for the MIRPreg nodes corresponding to special registers
    MapleAllocator *mAllocator;
};

#endif  // MIR_FEATURE_FULL
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_PREG_H
