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
}  // namespace maple
