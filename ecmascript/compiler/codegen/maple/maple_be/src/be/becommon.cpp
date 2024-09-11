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

#include "becommon.h"
#include "rt.h"
#include "cg_option.h"
#include "mir_builder.h"
#include "mpl_logging.h"
#include <cinttypes>
#include <list>

namespace maplebe {
using namespace maple;

BECommon::BECommon(MIRModule &mod)
    : mirModule(mod),
      typeSizeTable(GlobalTables::GetTypeTable().GetTypeTable().size(), 0, mirModule.GetMPAllocator().Adapter()),
      typeAlignTable(GlobalTables::GetTypeTable().GetTypeTable().size(), static_cast<uint8>(mirModule.IsCModule()),
                     mirModule.GetMPAllocator().Adapter()),
      typeHasFlexibleArray(GlobalTables::GetTypeTable().GetTypeTable().size(), 0, mirModule.GetMPAllocator().Adapter()),
      structFieldCountTable(GlobalTables::GetTypeTable().GetTypeTable().size(), 0,
                            mirModule.GetMPAllocator().Adapter()),
      funcReturnType(mirModule.GetMPAllocator().Adapter())
{
    for (uint32 i = 1; i < GlobalTables::GetTypeTable().GetTypeTable().size(); ++i) {
        MIRType *ty = GlobalTables::GetTypeTable().GetTypeTable()[i];
        ComputeTypeSizesAligns(*ty);
    }
}

void BECommon::AddNewTypeAfterBecommon(uint32 oldTypeTableSize, uint32 newTypeTableSize)
{
    for (auto i = oldTypeTableSize; i < newTypeTableSize; ++i) {
        MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(i);
        CHECK_NULL_FATAL(ty);
        typeSizeTable.emplace_back(0);
        typeAlignTable.emplace_back(static_cast<uint8>(mirModule.IsCModule()));
        typeHasFlexibleArray.emplace_back(0);
        structFieldCountTable.emplace_back(0);
        ComputeTypeSizesAligns(*ty);
    }
}

void BECommon::ComputeTypeSizesAligns(MIRType &ty, uint8 align)
{
    TyIdx tyIdx = ty.GetTypeIndex();
    if ((structFieldCountTable.size() > tyIdx) && (GetStructFieldCount(tyIdx) != 0)) {
        return; /* processed before */
    }

    if ((ty.GetPrimType() == PTY_ptr) || (ty.GetPrimType() == PTY_ref)) {
        ty.SetPrimType(GetLoweredPtrType());
    }

    switch (ty.GetKind()) {
        case kTypeScalar:
        case kTypePointer:
        case kTypeFunction:
            SetTypeSize(tyIdx, GetPrimTypeSize(ty.GetPrimType()));
            SetTypeAlign(tyIdx, GetTypeSize(tyIdx));
            break;
        case kTypeArray:
            CHECK_FATAL(false, "unsupported type");
            break;
        case kTypeByName:
        case kTypeVoid:
        default:
            SetTypeSize(tyIdx, 0);
            break;
    }
    /* there may be passed-in align attribute declared with the symbol */
    SetTypeAlign(tyIdx, std::max(GetTypeAlign(tyIdx), align));
}

bool BECommon::TyIsInSizeAlignTable(const MIRType &ty) const
{
    if (typeSizeTable.size() != typeAlignTable.size()) {
        return false;
    }
    return ty.GetTypeIndex() < typeSizeTable.size();
}

void BECommon::AddAndComputeSizeAlign(MIRType &ty)
{
    FinalizeTypeTable(ty);
    typeAlignTable.emplace_back(mirModule.IsCModule());
    typeSizeTable.emplace_back(0);
    ComputeTypeSizesAligns(ty);
}

void BECommon::AddElementToFuncReturnType(MIRFunction &func, const TyIdx tyIdx)
{
    funcReturnType[&func] = tyIdx;
}

MIRType *BECommon::BeGetOrCreatePointerType(const MIRType &pointedType)
{
    MIRType *newType = GlobalTables::GetTypeTable().GetOrCreatePointerType(pointedType, GetLoweredPtrType());
    if (TyIsInSizeAlignTable(*newType)) {
        return newType;
    }
    AddAndComputeSizeAlign(*newType);
    return newType;
}

MIRType *BECommon::BeGetOrCreateFunctionType(TyIdx tyIdx, const std::vector<TyIdx> &vecTy,
                                             const std::vector<TypeAttrs> &vecAt)
{
    MIRType *newType = GlobalTables::GetTypeTable().GetOrCreateFunctionType(tyIdx, vecTy, vecAt);
    if (TyIsInSizeAlignTable(*newType)) {
        return newType;
    }
    AddAndComputeSizeAlign(*newType);
    return newType;
}

void BECommon::FinalizeTypeTable(const MIRType &ty)
{
    if (ty.GetTypeIndex() > GetSizeOfTypeSizeTable()) {
        if (mirModule.GetSrcLang() == kSrcLangC) {
            for (uint32 i = GetSizeOfTypeSizeTable(); i < ty.GetTypeIndex(); ++i) {
                MIRType *tyTmp = GlobalTables::GetTypeTable().GetTypeFromTyIdx(i);
                AddAndComputeSizeAlign(*tyTmp);
            }
        } else {
            CHECK_FATAL(ty.GetTypeIndex() == typeSizeTable.size(), "make sure the ty idx is exactly the table size");
        }
    }
}
} /* namespace maplebe */
