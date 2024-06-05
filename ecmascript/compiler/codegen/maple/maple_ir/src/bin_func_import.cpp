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

#include "bin_mpl_export.h"
#include "bin_mpl_import.h"
#include "mir_function.h"
#include "opcode_info.h"
#include "mir_pragma.h"
#include "mir_builder.h"
using namespace std;

namespace maple {
void BinaryMplImport::ImportInfoVector(MIRInfoVector &infoVector, MapleVector<bool> &infoVectorIsString)
{
    int64 size = ReadNum();
    for (int64 i = 0; i < size; ++i) {
        GStrIdx gStrIdx = ImportStr();
        bool isstring = (ReadNum() != 0);
        infoVectorIsString.push_back(isstring);
        if (isstring) {
            GStrIdx fieldval = ImportStr();
            infoVector.emplace_back(MIRInfoPair(gStrIdx, fieldval.GetIdx()));
        } else {
            auto fieldval = static_cast<uint32>(ReadNum());
            infoVector.emplace_back(MIRInfoPair(gStrIdx, fieldval));
        }
    }
}

void BinaryMplImport::ImportFuncIdInfo(MIRFunction *func)
{
    int64 tag = ReadNum();
    CHECK_FATAL(tag == kBinFuncIdInfoStart, "kBinFuncIdInfoStart expected");
    func->SetPuidxOrigin(static_cast<PUIdx>(ReadNum()));
    ImportInfoVector(func->GetInfoVector(), func->InfoIsString());
    if (mod.GetFlavor() == kFlavorLmbc) {
        func->SetFrameSize(static_cast<uint32>(ReadNum()));
    }
}

void BinaryMplImport::ImportBaseNode(Opcode &o, PrimType &typ)
{
    o = static_cast<Opcode>(Read());
    typ = static_cast<PrimType>(Read());
}

MIRSymbol *BinaryMplImport::ImportLocalSymbol(MIRFunction *func)
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return nullptr;
    }
    if (tag < 0) {
        CHECK_FATAL(static_cast<size_t>(-tag) < localSymTab.size(), "index out of bounds");
        return localSymTab.at(static_cast<size_t>(-tag));
    }
    CHECK_FATAL(tag == kBinSymbol, "expecting kBinSymbol in ImportLocalSymbol()");
    MIRSymbol *sym = func->GetSymTab()->CreateSymbol(kScopeLocal);
    localSymTab.push_back(sym);
    sym->SetNameStrIdx(ImportStr());
    (void)func->GetSymTab()->AddToStringSymbolMap(*sym);
    sym->SetSKind(static_cast<MIRSymKind>(ReadNum()));
    sym->SetStorageClass(static_cast<MIRStorageClass>(ReadNum()));
    sym->SetAttrs(ImportTypeAttrs());
    sym->SetIsTmp(ReadNum() != 0);
    if (sym->GetSKind() == kStVar || sym->GetSKind() == kStFunc) {
        ImportSrcPos(sym->GetSrcPosition());
    }
    sym->SetTyIdx(ImportJType());
    if (sym->GetSKind() == kStPreg) {
        PregIdx pregidx = ImportPreg(func);
        MIRPreg *preg = func->GetPregTab()->PregFromPregIdx(pregidx);
        sym->SetPreg(preg);
    }
    return sym;
}

PregIdx BinaryMplImport::ImportPreg(MIRFunction *func)
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return 0;
    }
    if (tag == kBinSpecialReg) {
        return -Read();
    }
    if (tag < 0) {
        CHECK_FATAL(static_cast<size_t>(-tag) < localPregTab.size(), "index out of bounds");
        return localPregTab.at(static_cast<size_t>(-tag));
    }
    CHECK_FATAL(tag == kBinPreg, "expecting kBinPreg in ImportPreg()");

    PrimType primType = static_cast<PrimType>(Read());
    PregIdx pidx = func->GetPregTab()->CreatePreg(primType);
    localPregTab.push_back(pidx);
    return pidx;
}

LabelIdx BinaryMplImport::ImportLabel(MIRFunction *func)
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return 0;
    }
    if (tag < 0) {
        CHECK_FATAL(static_cast<size_t>(-tag) < localLabelTab.size(), "index out of bounds");
        return localLabelTab.at(static_cast<size_t>(-tag));
    }
    CHECK_FATAL(tag == kBinLabel, "kBinLabel expected in ImportLabel()");

    LabelIdx lidx = func->GetLabelTab()->CreateLabel();
    localLabelTab.push_back(lidx);
    return lidx;
}

void BinaryMplImport::ImportLocalTypeNameTable(MIRTypeNameTable *typeNameTab)
{
    int64 tag = ReadNum();
    CHECK_FATAL(tag == kBinTypenameStart, "kBinTypenameStart expected in ImportLocalTypeNameTable()");
    int64 size = ReadNum();
    for (int64 i = 0; i < size; ++i) {
        GStrIdx strIdx = ImportStr();
        TyIdx tyIdx = ImportJType();
        typeNameTab->SetGStrIdxToTyIdx(strIdx, tyIdx);
    }
}

void BinaryMplImport::ImportFormalsStIdx(MIRFunction *func)
{
    auto tag = ReadNum();
    CHECK_FATAL(tag == kBinFormalStart, "kBinFormalStart expected in ImportFormalsStIdx()");
    auto size = ReadNum();
    for (int64 i = 0; i < size; ++i) {
        func->GetFormalDefVec()[static_cast<uint64>(i)].formalSym = ImportLocalSymbol(func);
    }
}

void BinaryMplImport::ImportAliasMap(MIRFunction *func)
{
    int64 tag = ReadNum();
    CHECK_FATAL(tag == kBinAliasMapStart, "kBinAliasMapStart expected in ImportAliasMap()");
    int32 size = ReadInt();
    for (int32 i = 0; i < size; ++i) {
        MIRAliasVars aliasvars;
        GStrIdx strIdx = ImportStr();
        aliasvars.mplStrIdx = ImportStr();
        aliasvars.tyIdx = ImportJType();
        (void)ImportStr();  // not assigning to mimic parser
        func->GetAliasVarMap()[strIdx] = aliasvars;
    }
}

void BinaryMplImport::ImportSrcPos(SrcPosition &pos)
{
    if (!mod.IsWithDbgInfo()) {
        return;
    }
    pos.SetRawData(static_cast<uint32>(ReadNum()));
    pos.SetLineNum(static_cast<uint32>(ReadNum()));
}

void BinaryMplImport::ImportReturnValues(MIRFunction *func, CallReturnVector *retv)
{
    int64 tag = ReadNum();
    CHECK_FATAL(tag == kBinReturnvals, "expecting return values");
    auto size = static_cast<uint32>(ReadNum());
    for (uint32 i = 0; i < size; ++i) {
        RegFieldPair rfp;
        rfp.SetPregIdx(ImportPreg(func));
        if (rfp.IsReg()) {
            retv->push_back(std::make_pair(StIdx(), rfp));
            continue;
        }
        rfp.SetFieldID(static_cast<FieldID>(ReadNum()));
        MIRSymbol *lsym = ImportLocalSymbol(func);
        CHECK_FATAL(lsym != nullptr, "null ptr check");
        retv->push_back(std::make_pair(lsym->GetStIdx(), rfp));
        if (lsym->GetName().find("L_STR") == 0) {
            MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lsym->GetTyIdx());
            CHECK_FATAL(ty->GetKind() == kTypePointer, "Pointer type expected for L_STR prefix");
            MIRPtrType tempType(static_cast<MIRPtrType *>(ty)->GetPointedTyIdx(), PTY_ptr);
            TyIdx newTyidx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&tempType);
            lsym->SetTyIdx(newTyidx);
        }
    }
}
}  // namespace maple
