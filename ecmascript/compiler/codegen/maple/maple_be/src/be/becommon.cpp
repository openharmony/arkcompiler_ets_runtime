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

void BECommon::ComputeStructTypeSizesAligns(MIRType &ty, const TyIdx &tyIdx)
{
    auto &structType = static_cast<MIRStructType &>(ty);
    const FieldVector &fields = structType.GetFields();
    uint64 allocedSize = 0;
    uint64 allocedSizeInBits = 0;
    SetStructFieldCount(structType.GetTypeIndex(), fields.size());
    if (fields.size() == 0) {
        if (structType.IsCPlusPlus()) {
            SetTypeSize(tyIdx.GetIdx(), 1); /* empty struct in C++ has size 1 */
            SetTypeAlign(tyIdx.GetIdx(), 1);
        } else {
            SetTypeSize(tyIdx.GetIdx(), 0);
            SetTypeAlign(tyIdx.GetIdx(), k8ByteSize);
        }
        return;
    }
    auto structAttr = structType.GetTypeAttrs();
    auto structPack = static_cast<uint8>(structAttr.GetPack());
    for (uint32 j = 0; j < fields.size(); ++j) {
        TyIdx fieldTyIdx = fields[j].second.first;
        auto fieldAttr = fields[j].second.second;
        MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTyIdx);
        uint32 fieldTypeSize = GetTypeSize(fieldTyIdx);
        if (fieldTypeSize == 0) {
            ComputeTypeSizesAligns(*fieldType);
            fieldTypeSize = GetTypeSize(fieldTyIdx);
        }
        uint64 fieldSizeBits = fieldTypeSize * kBitsPerByte;
        auto attrAlign = static_cast<uint8>(fieldAttr.GetAlign());
        auto originAlign = std::max(attrAlign, GetTypeAlign(fieldTyIdx));
        uint8 fieldAlign = fieldAttr.IsPacked() ? 1 : std::min(originAlign, structPack);
        uint64 fieldAlignBits = fieldAlign * kBitsPerByte;
        CHECK_FATAL(fieldAlign != 0, "expect fieldAlign not equal 0");
        MIRStructType *subStructType = fieldType->EmbeddedStructType();
        if (subStructType != nullptr) {
            AppendStructFieldCount(structType.GetTypeIndex(), GetStructFieldCount(subStructType->GetTypeIndex()));
        }
        if (structType.GetKind() != kTypeUnion) {
            if (fieldType->GetKind() == kTypeBitField) {
                uint32 fieldSize = static_cast<MIRBitFieldType *>(fieldType)->GetFieldSize();
                /* is this field is crossing the align boundary of its base type? */
                if ((!structAttr.IsPacked() &&
                     ((allocedSizeInBits / fieldSizeBits) != ((allocedSizeInBits + fieldSize - 1u) / fieldSizeBits))) ||
                    fieldSize == 0) {
                    allocedSizeInBits = RoundUp(allocedSizeInBits, fieldSizeBits);
                }
                /* allocate the bitfield */
                allocedSizeInBits += fieldSize;
                allocedSize = std::max(allocedSize, RoundUp(allocedSizeInBits, fieldAlignBits) / kBitsPerByte);
            } else {
                bool leftoverbits = false;

                if (allocedSizeInBits == allocedSize * kBitsPerByte) {
                    allocedSize = RoundUp(allocedSize, fieldAlign);
                } else {
                    /* still some leftover bits on allocated words, we calculate things based on bits then. */
                    if (allocedSizeInBits / fieldAlignBits !=
                        (allocedSizeInBits + fieldSizeBits - 1) / fieldAlignBits) {
                        /* the field is crossing the align boundary of its base type */
                        allocedSizeInBits = RoundUp(allocedSizeInBits, fieldAlignBits);
                    }
                    leftoverbits = true;
                }
                if (leftoverbits) {
                    allocedSizeInBits += fieldSizeBits;
                    allocedSize = std::max(allocedSize, RoundUp(allocedSizeInBits, fieldAlignBits) / kBitsPerByte);
                } else {
                    /* pad alloced_size according to the field alignment */
                    allocedSize = RoundUp(allocedSize, fieldAlign);
                    allocedSize += fieldTypeSize;
                    allocedSizeInBits = allocedSize * kBitsPerByte;
                }
            }
        } else { /* for unions, bitfields are treated as non-bitfields */
            allocedSize = std::max(allocedSize, static_cast<uint64>(fieldTypeSize));
        }
        SetTypeAlign(tyIdx, std::max(GetTypeAlign(tyIdx), fieldAlign));
        /* C99
         * Last struct element of a struct with more than one member
         * is a flexible array if it is an array of size 0.
         */
        if ((j != 0) && ((j + 1) == fields.size()) && (fieldType->GetKind() == kTypeArray) &&
            (GetTypeSize(fieldTyIdx.GetIdx()) == 0)) {
            SetHasFlexibleArray(tyIdx.GetIdx(), true);
        }
    }
    SetTypeSize(tyIdx, RoundUp(allocedSize, GetTypeAlign(tyIdx.GetIdx())));
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
        case kTypeBitField:
        case kTypeFunction:
            SetTypeSize(tyIdx, GetPrimTypeSize(ty.GetPrimType()));
            SetTypeAlign(tyIdx, GetTypeSize(tyIdx));
            break;
        case kTypeUnion:
        case kTypeStruct: {
            ComputeStructTypeSizesAligns(ty, tyIdx);
            break;
        }
        case kTypeArray:
        case kTypeFArray:
        case kTypeJArray:
        case kTypeInterface:
        case kTypeClass: { /* cannot have union or bitfields */
            CHECK_FATAL(false, "unsupported type");
            break;
        }
        case kTypeByName:
        case kTypeVoid:
        default:
            SetTypeSize(tyIdx, 0);
            break;
    }
    /* there may be passed-in align attribute declared with the symbol */
    SetTypeAlign(tyIdx, std::max(GetTypeAlign(tyIdx), align));
}

bool BECommon::IsRefField(MIRStructType &structType, FieldID fieldID) const
{
    return false;
}

/*
 * compute the offset of the field given by fieldID within the structure type
 * structy; it returns the answer in the pair (byteoffset, bitoffset) such that
 * if it is a bitfield, byteoffset gives the offset of the container for
 * extracting the bitfield and bitoffset is with respect to the container
 */
std::pair<int32, int32> BECommon::GetFieldOffset(MIRStructType &structType, FieldID fieldID)
{
    CHECK_FATAL(fieldID <= GetStructFieldCount(structType.GetTypeIndex()), "GetFieldOFfset: fieldID too large");
    uint64 allocedSize = 0;
    uint64 allocedSizeInBits = 0;
    FieldID curFieldID = 1;
    if (fieldID == 0) {
        return std::pair<int32, int32>(0, 0);
    }
    DEBUG_ASSERT(structType.GetKind() != kTypeClass, "unsupported kTypeClass type");

    /* process the struct fields */
    FieldVector fields = structType.GetFields();
    auto structPack = static_cast<uint8>(structType.GetTypeAttrs().GetPack());
    for (uint32 j = 0; j < fields.size(); ++j) {
        TyIdx fieldTyIdx = fields[j].second.first;
        auto fieldAttr = fields[j].second.second;
        MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTyIdx);
        uint32 fieldTypeSize = GetTypeSize(fieldTyIdx);
        uint64 fieldSizeBits = fieldTypeSize * kBitsPerByte;
        auto originAlign = GetTypeAlign(fieldTyIdx);
        auto fieldAlign = fieldAttr.IsPacked() ? 1 : std::min(originAlign, structPack);
        uint64 fieldAlignBits = static_cast<uint64>(fieldAlign * kBitsPerByte);
        CHECK_FATAL(fieldAlign != 0, "fieldAlign should not equal 0");
        if (structType.GetKind() != kTypeUnion) {
            if (fieldType->GetKind() == kTypeBitField) {
                uint32 fieldSize = static_cast<MIRBitFieldType *>(fieldType)->GetFieldSize();
                /*
                 * Is this field is crossing the align boundary of its base type? Or,
                 * is field a zero-with bit field?
                 * Refer to C99 standard (§6.7.2.1) :
                 * > As a special case, a bit-field structure member with a width of 0 indicates that no further
                 * > bit-field is to be packed into the unit in which the previous bit-field, if any, was placed.
                 *
                 * We know that A zero-width bit field can cause the next field to be aligned on the next container
                 * boundary where the container is the same size as the underlying type of the bit field.
                 */
                CHECK_FATAL(allocedSizeInBits <= UINT64_MAX - fieldSize, "must not be zero");
                DEBUG_ASSERT(allocedSizeInBits + fieldSize >= 1, "allocedSizeInBits + fieldSize - 1u must be unsigned");
                if ((!structType.GetTypeAttrs().IsPacked() &&
                     ((allocedSizeInBits / fieldSizeBits) != ((allocedSizeInBits + fieldSize - 1u) / fieldSizeBits))) ||
                    fieldSize == 0) {
                    /*
                     * the field is crossing the align boundary of its base type;
                     * align alloced_size_in_bits to fieldAlign
                     */
                    allocedSizeInBits = RoundUp(allocedSizeInBits, fieldSizeBits);
                }
                /* allocate the bitfield */
                if (curFieldID == fieldID) {
                    return std::pair<int32, int32>((allocedSizeInBits / fieldAlignBits) * fieldAlign,
                                                   allocedSizeInBits % fieldAlignBits);
                } else {
                    ++curFieldID;
                }
                allocedSizeInBits += fieldSize;
                allocedSize = std::max(allocedSize, RoundUp(allocedSizeInBits, fieldAlignBits) / kBitsPerByte);
            } else {
                bool leftOverBits = false;
                uint64 offset = 0;

                if (allocedSizeInBits == allocedSize * k8BitSize) {
                    allocedSize = RoundUp(allocedSize, fieldAlign);
                    offset = allocedSize;
                } else {
                    /* still some leftover bits on allocated words, we calculate things based on bits then. */
                    if (allocedSizeInBits / fieldAlignBits !=
                        (allocedSizeInBits + fieldSizeBits - k1BitSize) / fieldAlignBits) {
                        /* the field is crossing the align boundary of its base type */
                        allocedSizeInBits = RoundUp(allocedSizeInBits, fieldAlignBits);
                    }
                    allocedSize = RoundUp(allocedSize, fieldAlign);
                    offset = static_cast<uint64>((allocedSizeInBits / fieldAlignBits) * fieldAlign);
                    leftOverBits = true;
                }

                if (curFieldID == fieldID) {
                    return std::pair<int32, int32>(offset, 0);
                } else {
                    MIRStructType *subStructType = fieldType->EmbeddedStructType();
                    if (subStructType == nullptr) {
                        ++curFieldID;
                    } else {
                        if ((curFieldID + GetStructFieldCount(subStructType->GetTypeIndex())) < fieldID) {
                            curFieldID += GetStructFieldCount(subStructType->GetTypeIndex()) + 1;
                        } else {
                            std::pair<int32, int32> result = GetFieldOffset(*subStructType, fieldID - curFieldID);
                            return std::pair<int32, int32>(result.first + allocedSize, result.second);
                        }
                    }
                }

                if (leftOverBits) {
                    allocedSizeInBits += fieldSizeBits;
                    allocedSize = std::max(allocedSize, RoundUp(allocedSizeInBits, fieldAlignBits) / kBitsPerByte);
                } else {
                    allocedSize += fieldTypeSize;
                    allocedSizeInBits = allocedSize * kBitsPerByte;
                }
            }
        } else { /* for unions, bitfields are treated as non-bitfields */
            if (curFieldID == fieldID) {
                return std::pair<int32, int32>(0, 0);
            } else {
                MIRStructType *subStructType = fieldType->EmbeddedStructType();
                if (subStructType == nullptr) {
                    curFieldID++;
                } else {
                    if ((curFieldID + GetStructFieldCount(subStructType->GetTypeIndex())) < fieldID) {
                        curFieldID += GetStructFieldCount(subStructType->GetTypeIndex()) + 1;
                    } else {
                        return GetFieldOffset(*subStructType, fieldID - curFieldID);
                    }
                }
            }
        }
    }
    CHECK_FATAL(false, "GetFieldOffset() fails to find field");
    return std::pair<int32, int32>(0, 0);
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

BaseNode *BECommon::GetAddressOfNode(const BaseNode &node)
{
    switch (node.GetOpCode()) {
        case OP_dread: {
            const DreadNode &dNode = static_cast<const DreadNode &>(node);
            const StIdx &index = dNode.GetStIdx();
            DEBUG_ASSERT(mirModule.CurFunction() != nullptr, "curFunction should not be nullptr");
            DEBUG_ASSERT(mirModule.CurFunction()->GetLocalOrGlobalSymbol(index) != nullptr, "nullptr check");
            return mirModule.GetMIRBuilder()->CreateAddrof(*mirModule.CurFunction()->GetLocalOrGlobalSymbol(index));
        }
        case OP_iread: {
            const IreadNode &iNode = static_cast<const IreadNode &>(node);
            if (iNode.GetFieldID() == 0) {
                return iNode.Opnd(0);
            }

            uint32 index = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeTable().at(iNode.GetTyIdx()))
                               ->GetPointedTyIdx();
            MIRType *pointedType = GlobalTables::GetTypeTable().GetTypeTable().at(index);
            std::pair<int32, int32> byteBitOffset =
                GetFieldOffset(static_cast<MIRStructType &>(*pointedType), iNode.GetFieldID());
            return mirModule.GetMIRBuilder()->CreateExprBinary(
                OP_add, *GlobalTables::GetTypeTable().GetPrimType(GetAddressPrimType()),
                static_cast<BaseNode *>(iNode.Opnd(0)),
                mirModule.GetMIRBuilder()->CreateIntConst(byteBitOffset.first, PTY_u32));
        }
        default:
            return nullptr;
    }
}
} /* namespace maplebe */
