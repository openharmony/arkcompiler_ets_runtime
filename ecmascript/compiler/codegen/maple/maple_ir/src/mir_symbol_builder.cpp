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

#include "mir_symbol_builder.h"

namespace maple {
MIRSymbol *MIRSymbolBuilder::GetLocalDecl(const MIRSymbolTable &symbolTable, const GStrIdx &strIdx) const
{
    if (strIdx != 0u) {
        const StIdx stIdx = symbolTable.GetStIdxFromStrIdx(strIdx);
        if (stIdx.FullIdx() != 0) {
            return symbolTable.GetSymbolFromStIdx(stIdx.Idx());
        }
    }
    return nullptr;
}

MIRSymbol *MIRSymbolBuilder::CreateLocalDecl(MIRSymbolTable &symbolTable, GStrIdx strIdx, const MIRType &type) const
{
    MIRSymbol *st = symbolTable.CreateSymbol(kScopeLocal);
    st->SetNameStrIdx(strIdx);
    st->SetTyIdx(type.GetTypeIndex());
    (void)symbolTable.AddToStringSymbolMap(*st);
    st->SetStorageClass(kScAuto);
    st->SetSKind(kStVar);
    return st;
}

MIRSymbol *MIRSymbolBuilder::GetGlobalDecl(GStrIdx strIdx) const
{
    if (strIdx != 0u) {
        const StIdx stIdx = GlobalTables::GetGsymTable().GetStIdxFromStrIdx(strIdx);
        if (stIdx.FullIdx() != 0) {
            return GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
        }
    }
    return nullptr;
}

MIRSymbol *MIRSymbolBuilder::CreateGlobalDecl(GStrIdx strIdx, const MIRType &type, MIRStorageClass sc) const
{
    MIRSymbol *st = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    st->SetNameStrIdx(strIdx);
    st->SetTyIdx(type.GetTypeIndex());
    (void)GlobalTables::GetGsymTable().AddToStringSymbolMap(*st);
    st->SetStorageClass(sc);
    st->SetSKind(kStVar);
    return st;
}

// when sametype is true, it means match everything the of the symbol
MIRSymbol *MIRSymbolBuilder::GetSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                                       bool sameType) const
{
    MIRSymbol *st = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
    if (st == nullptr || st->GetTyIdx() != tyIdx) {
        return nullptr;
    }

    if (sameType) {
        if (st->GetStorageClass() == sClass && st->GetSKind() == mClass) {
            return st;
        }
        return nullptr;
    }
    DEBUG_ASSERT(mClass == st->GetSKind(),
                 "trying to create a new symbol that has the same name and GtyIdx. might cause problem");
    DEBUG_ASSERT(sClass == st->GetStorageClass(),
                 "trying to create a new symbol that has the same name and tyIdx. might cause problem");
    return st;
}

// when func is null, create global symbol, otherwise create local symbol
MIRSymbol *MIRSymbolBuilder::CreateSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                                          MIRFunction *func, uint8 scpID) const
{
    MIRSymbol *st =
        (func != nullptr) ? func->GetSymTab()->CreateSymbol(scpID) : GlobalTables::GetGsymTable().CreateSymbol(scpID);
    CHECK_FATAL(st != nullptr, "Failed to create MIRSymbol");
    st->SetStorageClass(sClass);
    st->SetSKind(mClass);
    st->SetNameStrIdx(strIdx);
    st->SetTyIdx(tyIdx);
    if (func != nullptr) {
        (void)func->GetSymTab()->AddToStringSymbolMap(*st);
    } else {
        (void)GlobalTables::GetGsymTable().AddToStringSymbolMap(*st);
    }
    return st;
}

MIRSymbol *MIRSymbolBuilder::CreatePregFormalSymbol(TyIdx tyIdx, PregIdx pRegIdx, MIRFunction &func) const
{
    MIRSymbol *st = func.GetSymTab()->CreateSymbol(kScopeLocal);
    CHECK_FATAL(st != nullptr, "Failed to create MIRSymbol");
    st->SetStorageClass(kScFormal);
    st->SetSKind(kStPreg);
    st->SetTyIdx(tyIdx);
    MIRPregTable *pregTab = func.GetPregTab();
    st->SetPreg(pregTab->PregFromPregIdx(pRegIdx));
    return st;
}

size_t MIRSymbolBuilder::GetSymbolTableSize(const MIRFunction *func) const
{
    return (func == nullptr) ? GlobalTables::GetGsymTable().GetSymbolTableSize()
                             : func->GetSymTab()->GetSymbolTableSize();
}

const MIRSymbol *MIRSymbolBuilder::GetSymbolFromStIdx(uint32 idx, const MIRFunction *func) const
{
    if (func == nullptr) {
        auto &symTab = GlobalTables::GetGsymTable();
        return symTab.GetSymbolFromStidx(idx);
    } else {
        auto &symTab = *func->GetSymTab();
        return symTab.GetSymbolFromStIdx(idx);
    }
}
}  // namespace maple
