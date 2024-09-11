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

#ifndef MAPLEBE_INCLUDE_BE_BECOMMON_H
#define MAPLEBE_INCLUDE_BE_BECOMMON_H
/* C++ headers. */
#include <cstddef>
#include <utility>
/* Basic Maple-independent utility functions */
#include "common_utils.h"
/* MapleIR headers. */
#include "mir_nodes.h"  /* maple_ir/include, for BaseNode */
#include "mir_type.h"   /* maple_ir/include, for MIRType */
#include "mir_module.h" /* maple_ir/include, for mirModule */

namespace maplebe {
using namespace maple;

enum BitsPerByte : uint8 { kBitsPerByte = 8, kLog2BitsPerByte = 3 };

inline uint32 GetPointerBitSize()
{
    return GetPointerSize() * kBitsPerByte;
}

class BECommon {
public:
    explicit BECommon(MIRModule &mod);

    ~BECommon() = default;

    void ComputeTypeSizesAligns(MIRType &type, uint8 align = 0);

    void AddNewTypeAfterBecommon(uint32 oldTypeTableSize, uint32 newTypeTableSize);

    bool HasFuncReturnType(MIRFunction &func) const
    {
        return (funcReturnType.find(&func) != funcReturnType.end());
    }

    const TyIdx GetFuncReturnType(MIRFunction &func) const
    {
        return (funcReturnType.at(&func));
    }

    void AddElementToFuncReturnType(MIRFunction &func, const TyIdx tyIdx);

    MIRType *BeGetOrCreatePointerType(const MIRType &pointedType);

    MIRType *BeGetOrCreateFunctionType(TyIdx tyIdx, const std::vector<TyIdx> &vecTy,
                                       const std::vector<TypeAttrs> &vecAt);

    PrimType GetAddressPrimType() const
    {
        return GetLoweredPtrType();
    }

    /* update typeSizeTable and typeAlignTable when new type is created */
    void UpdateTypeTable(MIRType &ty)
    {
        if (!TyIsInSizeAlignTable(ty)) {
            AddAndComputeSizeAlign(ty);
        }
    }

    /* Global type table might be updated during lowering for C/C++. */
    void FinalizeTypeTable(const MIRType &ty);

    uint32 GetFieldIdxIncrement(const MIRType &ty) const
    {
        return 1;
    }

    MIRModule &GetMIRModule() const
    {
        return mirModule;
    }

    uint64 GetTypeSize(uint32 idx) const
    {
        return typeSizeTable.at(idx);
    }
    uint32 GetSizeOfTypeSizeTable() const
    {
        return typeSizeTable.size();
    }
    bool IsEmptyOfTypeSizeTable() const
    {
        return typeSizeTable.empty();
    }
    void SetTypeSize(uint32 idx, uint64 value)
    {
        typeSizeTable.at(idx) = value;
    }
    void AddTypeSize(uint64 value)
    {
        typeSizeTable.emplace_back(value);
    }

    void AddTypeSizeAndAlign(const TyIdx tyIdx, uint64 value)
    {
        if (typeSizeTable.size() == tyIdx) {
            typeSizeTable.emplace_back(value);
            typeAlignTable.emplace_back(value);
        } else {
            CHECK_FATAL(typeSizeTable.size() > tyIdx, "there are some types haven't set type size and align, %d");
        }
    }

    uint8 GetTypeAlign(uint32 idx) const
    {
        return typeAlignTable.at(idx);
    }
    size_t GetSizeOfTypeAlignTable() const
    {
        return typeAlignTable.size();
    }
    bool IsEmptyOfTypeAlignTable() const
    {
        return typeAlignTable.empty();
    }
    void SetTypeAlign(uint32 idx, uint8 value)
    {
        typeAlignTable.at(idx) = value;
    }
    void AddTypeAlign(uint8 value)
    {
        typeAlignTable.emplace_back(value);
    }

    bool GetHasFlexibleArray(uint32 idx) const
    {
        return typeHasFlexibleArray.at(idx);
    }
    void SetHasFlexibleArray(uint32 idx, bool value)
    {
        typeHasFlexibleArray.at(idx) = value;
    }

    FieldID GetStructFieldCount(uint32 idx) const
    {
        return structFieldCountTable.at(idx);
    }
    uint32 GetSizeOfStructFieldCountTable() const
    {
        return structFieldCountTable.size();
    }
    void SetStructFieldCount(uint32 idx, FieldID value)
    {
        structFieldCountTable.at(idx) = value;
    }
    void AppendStructFieldCount(uint32 idx, FieldID value)
    {
        structFieldCountTable.at(idx) += value;
    }

private:
    bool TyIsInSizeAlignTable(const MIRType &) const;
    void AddAndComputeSizeAlign(MIRType &);

    MIRModule &mirModule;
    MapleVector<uint64> typeSizeTable;      /* index is TyIdx */
    MapleVector<uint8> typeAlignTable;      /* index is TyIdx */
    MapleVector<bool> typeHasFlexibleArray; /* struct with flexible array */
    /*
     * gives number of fields inside
     * each struct inclusive of nested structs, for speeding up
     * traversal for locating the field for a given fieldID
     */
    MapleVector<FieldID> structFieldCountTable;
    MapleUnorderedMap<MIRFunction *, TyIdx> funcReturnType;
}; /* class BECommon */
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_BE_BECOMMON_H */