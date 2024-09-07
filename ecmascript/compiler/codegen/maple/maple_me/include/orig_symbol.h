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

#ifndef MAPLE_ME_INCLUDE_ORIG_SYMBOL_H
#define MAPLE_ME_INCLUDE_ORIG_SYMBOL_H
#include "mir_module.h"
#include "mir_symbol.h"
#include "mir_preg.h"
#include "mir_function.h"
#include "mpl_number.h"
#include "scc.h"

// This file defines the data structure OriginalSt that represents a program
// symbol occurring in the code of the program being optimized.
namespace maple {
class OStTag;
using OStIdx = utils::Index<OStTag, uint32>;

constexpr int kInitVersion = 0;
class VarMeExpr;  // circular dependency exists, no other choice
class VersionSt;  // circular dependency exists, no other choice
class OriginalSt {
public:
    struct OriginalStPtrComparator {
        bool operator()(const OriginalSt *x, const OriginalSt *y) const
        {
            return x->GetIndex() < y->GetIndex();
        }
    };

    OriginalSt(uint32 index, PregIdx rIdx, PUIdx pIdx, MapleAllocator &alloc)
        : OriginalSt(OStIdx(index), alloc, true, false, 0, pIdx, kPregOst, false, {.pregIdx = rIdx})
    {
    }

    ~OriginalSt() = default;

    void Dump() const;
    PregIdx GetPregIdx() const
    {
        DEBUG_ASSERT(ostType == kPregOst, "OriginalSt must be PregOst");
        return symOrPreg.pregIdx;
    }

    MIRPreg *GetMIRPreg() const
    {
        DEBUG_ASSERT(ostType == kPregOst, "OriginalSt must be PregOst");
        return GlobalTables::GetGsymTable().GetModule()->CurFunction()->GetPregTab()->PregFromPregIdx(
            symOrPreg.pregIdx);
    }

    MIRSymbol *GetMIRSymbol() const
    {
        DEBUG_ASSERT(ostType == kSymbolOst, "OriginalSt must be SymbolOst");
        return symOrPreg.mirSt;
    }

    bool HasAttr(AttrKind attrKind) const
    {
        if (ostType == kSymbolOst) {
            TypeAttrs typeAttr = symOrPreg.mirSt->GetAttrs();
            if (typeAttr.GetAttr(attrKind)) {
                return true;
            }
        }
        return false;
    }

    bool HasOneElemSimdAttr() const
    {
        if (ostType == kSymbolOst) {
            TypeAttrs typeAttr = symOrPreg.mirSt->GetAttrs();
            if (typeAttr.GetAttr(ATTR_oneelem_simd)) {
                return true;
            }
        }
        return false;
    }

    bool IsLocal() const
    {
        return isLocal;
    }

    bool IsFormal() const
    {
        return isFormal;
    }
    void SetIsFormal(bool isFormal)
    {
        this->isFormal = isFormal;
    }

    bool IsFinal() const
    {
        return isFinal;
    }
    void SetIsFinal(bool isFinal = true)
    {
        this->isFinal = isFinal;
    }

    bool IsPrivate() const
    {
        return isPrivate;
    }
    void SetIsPrivate(bool isPrivate)
    {
        this->isPrivate = isPrivate;
    }

    bool IsVolatile() const
    {
        return (ostType == kSymbolOst) ? symOrPreg.mirSt->IsVolatile() : false;
    }

    bool IsVrNeeded() const
    {
        return (ostType == kSymbolOst) ? symOrPreg.mirSt->GetIsTmp() : false;
    }

    bool Equal(const OriginalSt &ost) const;

    bool IsRealSymbol() const
    {
        return (ostType == kSymbolOst || ostType == kPregOst);
    }

    bool IsSymbolOst() const
    {
        return ostType == kSymbolOst;
    }

    bool IsPregOst() const
    {
        return (ostType == kPregOst);
    }

    bool IsSpecialPreg() const
    {
        return ostType == kPregOst && symOrPreg.pregIdx < 0;
    }

    bool IsSameSymOrPreg(const OriginalSt *ost) const
    {
        if (ostType != ost->ostType) {
            return false;
        }
        if (IsSymbolOst()) {
            return symOrPreg.mirSt == ost->symOrPreg.mirSt;
        }
        return symOrPreg.pregIdx == ost->symOrPreg.pregIdx;
    }

    int8 GetIndirectLev() const
    {
        return indirectLev;
    }

    void SetIndirectLev(int8 level)
    {
        indirectLev = level;
    }

    OStIdx GetIndex() const
    {
        return index;
    }

    size_t GetVersionIndex(size_t version) const
    {
        DEBUG_ASSERT(version < versionsIndices.size(), "version out of range");
        return versionsIndices.at(version);
    }

    const MapleVector<size_t> &GetVersionsIndices() const
    {
        return versionsIndices;
    }
    void PushbackVersionsIndices(size_t versionIndex)
    {
        versionsIndices.push_back(versionIndex);
    }

    size_t GetZeroVersionIndex() const
    {
        return zeroVersionIndex;
    }

    void SetZeroVersionIndex(size_t zeroVersionIndexParam)
    {
        zeroVersionIndex = zeroVersionIndexParam;
    }

    TyIdx GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(TyIdx tyIdxPara)
    {
        tyIdx = tyIdxPara;
    }

    FieldID GetFieldID() const
    {
        return fieldID;
    }

    void SetFieldID(FieldID fieldID)
    {
        this->fieldID = fieldID;
    }

    const OffsetType &GetOffset() const
    {
        return offset;
    }

    void SetOffset(const OffsetType &offsetVal)
    {
        offset = offsetVal;
    }

    bool IsIgnoreRC() const
    {
        return ignoreRC;
    }

    bool IsAddressTaken() const
    {
        return addressTaken;
    }

    void SetAddressTaken(bool addrTaken = true)
    {
        addressTaken = addrTaken;
    }

    // current ost can not be define by MayDef, or used by mayUse
    bool IsTopLevelOst() const;

    bool IsEPreLocalRefVar() const
    {
        return epreLocalRefVar;
    }

    void SetEPreLocalRefVar(bool epreLocalrefvarPara = true)
    {
        epreLocalRefVar = epreLocalrefvarPara;
    }

    PUIdx GetPuIdx() const
    {
        return puIdx;
    }

    bool IsIVCandidate() const
    {
        if (indirectLev != 0 || (IsSymbolOst() && GetMIRSymbol()->GetName() == "__nads_dummysym__")) {
            return false;
        }
        MIRType *mirtype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        if (IsPrimitiveInteger(mirtype->GetPrimType())) {
            // additional check using MIRSymbol's tyIdx
            if (IsSymbolOst()) {
                return IsPrimitiveInteger(GetMIRSymbol()->GetType()->GetPrimType());
            }
            return true;
        } else {
            return false;
        }
    }

    MIRType *GetType() const
    {
        return GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    }

    const OriginalSt *GetPrevLevelOst() const
    {
        return prevLevOst;
    }

    OriginalSt *GetPrevLevelOst()
    {
        return prevLevOst;
    }

    size_t GetPointerVstIdx() const
    {
        return pointerVstIdx;
    }

    void SetPointerVst(const VersionSt *vst);

    uint32 NumSSAVersions() const
    {
        if (zeroVersionIndex == 0 || !IsPregOst()) {
            return static_cast<uint32>(versionsIndices.size());
        }
        return static_cast<uint32>(versionsIndices.size()) - 1;  // preg's zero version not counted
    }

private:
    enum OSTType { kUnkonwnOst, kSymbolOst, kPregOst };
    union SymOrPreg {
        PregIdx pregIdx;
        MIRSymbol *mirSt;
    };
    OriginalSt(OStIdx index, MapleAllocator &alloc, bool local, bool isFormal, FieldID fieldID, PUIdx pIdx,
               OSTType ostType, bool ignoreRC, SymOrPreg sysOrPreg)
        : ostType(ostType),
          index(index),
          versionsIndices(alloc.Adapter()),
          fieldID(fieldID),
          offset(kOffsetUnknown),
          isLocal(local),
          isFormal(isFormal),
          ignoreRC(ignoreRC),
          symOrPreg(sysOrPreg),
          puIdx(pIdx)
    {
    }

    OSTType ostType;
    OStIdx index;                         // index number in originalStVector
    MapleVector<size_t> versionsIndices;  // the i-th element refers the index of versionst in versionst table
    size_t zeroVersionIndex = 0;          // same as versionsIndices[0]
    TyIdx tyIdx {0};                      // type of this symbol at this level; 0 for unknown
    FieldID fieldID;                      // at each level of indirection
    OffsetType offset;                    // bit offset
    int8 indirectLev = 0;                 // level of indirection; -1 for address, 0 for itself
    bool isLocal;                         // get from defined stmt or use expr
    bool isFormal;  // it's from the formal parameters so the type must be kSymbolOst or kPregOst after rename2preg
    bool addressTaken = false;
    bool isFinal = false;          // if the field has final attribute, only when fieldID != 0
    bool isPrivate = false;        // if the field has private attribute, only when fieldID != 0
    bool ignoreRC = false;         // base on MIRSymbol's IgnoreRC()
    bool epreLocalRefVar = false;  // is a localrefvar temp created by epre phase
public:
    bool isPtrWithIncDec = false;    // is a pointer with self-increment/decrement
    bool storesIVInitValue = false;  // temp created to store IV's initial value
private:
    SymOrPreg symOrPreg;
    PUIdx puIdx;
    size_t pointerVstIdx = 0;
    OriginalSt *prevLevOst = nullptr;
};

class SymbolFieldPair {
public:
    SymbolFieldPair(const StIdx &stIdx, FieldID fld, const TyIdx &tyIdx,
                    const OffsetType &offset = OffsetType(kOffsetUnknown))
        : stIdx(stIdx), fldIDAndOffset((static_cast<int64>(offset.val) << 32U) + fld), tyIdx(tyIdx)
    {
    }
    ~SymbolFieldPair() = default;
    bool operator==(const SymbolFieldPair &pairA) const
    {
        return (pairA.stIdx == stIdx) && (pairA.fldIDAndOffset == fldIDAndOffset) && (tyIdx == pairA.tyIdx);
    }

    const StIdx &GetStIdx() const
    {
        return stIdx;
    }

    FieldID GetFieldID() const
    {
        return static_cast<FieldID>(fldIDAndOffset);
    }

private:
    StIdx stIdx;
    int64 fldIDAndOffset;
    TyIdx tyIdx;
};

struct HashSymbolFieldPair {
    size_t operator()(const SymbolFieldPair &symbolFldID) const
    {
        return symbolFldID.GetStIdx().FullIdx();
    }
};

// This Table is for original symobols only. There is no SSA info attached and SSA is built based on this table.
class OriginalStTable {
public:
    OriginalStTable(MemPool &memPool, MIRModule &mod);
    ~OriginalStTable() = default;

    using OriginalStContainer = MapleVector<OriginalSt *>;
    using VstIdx2NextLevelOsts = MapleVector<OriginalStContainer *>;
    using OriginalStIterator = OriginalStContainer::iterator;
    using ConstOriginalStIterator = OriginalStContainer::const_iterator;

    OriginalSt *FindOrCreateSymbolOriginalSt(MIRSymbol &mirSt, PUIdx puIdx, FieldID fld);
    std::pair<OriginalSt *, bool> FindOrCreateSymbolOriginalSt(MIRSymbol &mirSt, PUIdx puIdx, FieldID fld,
                                                               const TyIdx &tyIdx,
                                                               const OffsetType &offset = OffsetType(kOffsetUnknown));
    OriginalSt *CreateSymbolOriginalSt(MIRSymbol &mirSt, PUIdx puIdx, FieldID fld, const TyIdx &tyIdx,
                                       const OffsetType &offset = OffsetType(kOffsetUnknown));
    OriginalSt *FindOrCreatePregOriginalSt(PregIdx pregIdx, PUIdx puIdx);
    OriginalSt *CreateSymbolOriginalSt(MIRSymbol &mirSt, PUIdx pidx, FieldID fld);
    OriginalSt *CreatePregOriginalSt(PregIdx pregIdx, PUIdx puIdx);
    OriginalSt *FindSymbolOriginalSt(const MIRSymbol &mirSt, FieldID fld, const TyIdx &tyIdx, const OffsetType &offset);
    OriginalSt *FindSymbolOriginalSt(const MIRSymbol &mirSt);
    const OriginalSt *GetOriginalStFromID(OStIdx id, bool checkFirst = false) const
    {
        if (checkFirst && id >= originalStVector.size()) {
            return nullptr;
        }
        DEBUG_ASSERT(id < originalStVector.size(), "symbol table index out of range");
        return originalStVector[id];
    }
    OriginalSt *GetOriginalStFromID(OStIdx id, bool checkFirst = false)
    {
        return const_cast<OriginalSt *>(const_cast<const OriginalStTable *>(this)->GetOriginalStFromID(id, checkFirst));
    }

    size_t Size() const
    {
        return originalStVector.size();
    }

    const MIRSymbol *GetMIRSymbolFromOriginalSt(const OriginalSt &ost) const
    {
        DEBUG_ASSERT(ost.IsRealSymbol(), "runtime check error");
        return ost.GetMIRSymbol();
    }

    const MIRSymbol *GetMIRSymbolFromID(OStIdx id) const
    {
        return GetOriginalStFromID(id, false)->GetMIRSymbol();
    }
    MIRSymbol *GetMIRSymbolFromID(OStIdx id)
    {
        auto *originalSt = GetOriginalStFromID(id, false);
        CHECK_FATAL(originalSt, "originalSt is nullptr!");
        return originalSt->GetMIRSymbol();
    }

    MapleAllocator &GetAlloc()
    {
        return alloc;
    }

    MapleVector<OriginalSt *> &GetOriginalStVector()
    {
        return originalStVector;
    }

    void SetEPreLocalRefVar(const OStIdx &id, bool epreLocalrefvarPara = true)
    {
        DEBUG_ASSERT(id < originalStVector.size(), "symbol table index out of range");
        originalStVector[id]->SetEPreLocalRefVar(epreLocalrefvarPara);
    }

    void SetZeroVersionIndex(const OStIdx &id, size_t zeroVersionIndexParam)
    {
        DEBUG_ASSERT(id < originalStVector.size(), "symbol table index out of range");
        originalStVector[id]->SetZeroVersionIndex(zeroVersionIndexParam);
    }

    size_t GetVersionsIndicesSize(const OStIdx &id) const
    {
        DEBUG_ASSERT(id < originalStVector.size(), "symbol table index out of range");
        return originalStVector[id]->GetVersionsIndices().size();
    }

    void UpdateVarOstMap(const OStIdx &id, std::map<OStIdx, OriginalSt *> &varOstMap)
    {
        DEBUG_ASSERT(id < originalStVector.size(), "symbol table index out of range");
        varOstMap[id] = originalStVector[id];
    }

    ConstOriginalStIterator begin() const
    {
        auto it = originalStVector.begin();
        // self-inc resulting from the fact that the 1st element is reserved and set null.
        ++it;
        return it;
    }

    OriginalStIterator begin()
    {
        auto it = originalStVector.begin();
        // self-inc resulting from the fact that the 1st element is reserved and set null.
        ++it;
        return it;
    }

    ConstOriginalStIterator end() const
    {
        return originalStVector.end();
    }

    OriginalStIterator end()
    {
        return originalStVector.end();
    }

    void Dump();
    OriginalSt *FindOrCreateAddrofSymbolOriginalSt(OriginalSt *ost);
    OriginalSt *FindOrCreateExtraLevOriginalSt(const VersionSt *vst, TyIdx ptyidx, FieldID fld,
                                               const OffsetType &offset = OffsetType(kOffsetUnknown));
    OriginalSt *FindExtraLevOriginalSt(const MapleVector<OriginalSt *> &nextLevelOsts, const MIRType *type, FieldID fld,
                                       const OffsetType &offset = OffsetType(kOffsetUnknown)) const;
    OriginalSt *FindExtraLevOriginalSt(const VersionSt *vst, const MIRType *type, FieldID fld,
                                       const OffsetType &offset = OffsetType(kOffsetUnknown)) const;
    MapleVector<OriginalSt *> *GetNextLevelOstsOfVst(size_t vstIdx) const;
    MapleVector<OriginalSt *> *GetNextLevelOstsOfVst(const VersionSt *vst) const;
    void AddNextLevelOstOfVst(size_t vstIdx, OriginalSt *ost);
    void AddNextLevelOstOfVst(const VersionSt *vst, OriginalSt *ost);

private:
    MapleAllocator alloc;
    MIRModule &mirModule;
    MapleVector<OriginalSt *> originalStVector;  // the vector that map a OriginalSt's index to its pointer
    // mir symbol to original table, this only exists for no-original variables.
public:
    MapleUnorderedMap<SymbolFieldPair, OStIdx, HashSymbolFieldPair> mirSt2Ost;
    MapleUnorderedMap<StIdx, OStIdx> addrofSt2Ost;

private:
    MapleUnorderedMap<PregIdx, OStIdx> preg2Ost;
    // mir type to virtual variables in original table. this only exists for no-original variables.
    MapleMap<TyIdx, OStIdx> pType2Ost;
    // malloc info to virtual variables in original table. this only exists for no-original variables.
    MapleMap<std::pair<BaseNode *, uint32>, OStIdx> malloc2Ost;
    MapleMap<uint32, OStIdx> thisField2Ost;   // field of this_memory to virtual variables in original table.
    VstIdx2NextLevelOsts nextLevelOstsOfVst;  // index is VersionSt index.
    OStIdx virtuaLostUnkownMem {0};
    OStIdx virtuaLostConstMem {0};
};
}  // namespace maple

namespace std {
template <>
struct hash<maple::OStIdx> {
    size_t operator()(const maple::OStIdx &x) const
    {
        return static_cast<size_t>(x);
    }
};
}  // namespace std
#endif  // MAPLE_ME_INCLUDE_ORIG_SYMBOL_H
