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

#include "mir_function.h"
#include <cstdio>
#include <iostream>
#include "mir_nodes.h"
#include "printing.h"
#include "string_utils.h"

namespace {
using namespace maple;
enum FuncProp : uint32_t {
    kFuncPropHasCall = 1U,                    // the function has call
#ifdef ARK_LITECG_DEBUG
    kFuncPropInfoPrinted = 1U << 3,           // to avoid printing frameSize/moduleid/funcSize info more
                                              // than once per function since they
                                              // can only be printed at the beginning of a block
#endif
};
}  // namespace

namespace maple {
const MIRSymbol *MIRFunction::GetFuncSymbol() const
{
    return GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolTableIdx.Idx());
}
MIRSymbol *MIRFunction::GetFuncSymbol()
{
    const MIRFunction *mirFunc = const_cast<const MIRFunction *>(this);
    DEBUG_ASSERT(mirFunc != nullptr, "null ptr check");
    return const_cast<MIRSymbol *>(mirFunc->GetFuncSymbol());
}

const std::string &MIRFunction::GetName() const
{
    MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolTableIdx.Idx());
    DEBUG_ASSERT(mirSymbol != nullptr, "null ptr check");
    return mirSymbol->GetName();
}

#ifdef ARK_LITECG_DEBUG
GStrIdx MIRFunction::GetNameStrIdx() const
{
    MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolTableIdx.Idx());
    DEBUG_ASSERT(mirSymbol != nullptr, "null ptr check");
    return mirSymbol->GetNameStrIdx();
}
#endif

const std::string &MIRFunction::GetBaseClassName() const
{
    return GlobalTables::GetStrTable().GetStringFromStrIdx(baseClassStrIdx);
}

const std::string &MIRFunction::GetBaseFuncName() const
{
    return GlobalTables::GetStrTable().GetStringFromStrIdx(baseFuncStrIdx);
}

const MIRType *MIRFunction::GetReturnType() const
{
    CHECK_FATAL(funcType != nullptr, "funcType should not be nullptr");
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcType->GetRetTyIdx());
}
MIRType *MIRFunction::GetReturnType()
{
    return const_cast<MIRType *>(const_cast<const MIRFunction *>(this)->GetReturnType());
}

const MIRType *MIRFunction::GetNthParamType(size_t i) const
{
    CHECK_FATAL(funcType != nullptr, "funcType should not be nullptr");
    DEBUG_ASSERT(i < funcType->GetParamTypeList().size(), "array index out of range");
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcType->GetParamTypeList()[i]);
}
MIRType *MIRFunction::GetNthParamType(size_t i)
{
    return const_cast<MIRType *>(const_cast<const MIRFunction *>(this)->GetNthParamType(i));
}

// reconstruct formals, and return a new MIRFuncType
MIRFuncType *MIRFunction::ReconstructFormals(const std::vector<MIRSymbol *> &symbols, bool clearOldArgs)
{
    auto *newFuncType = static_cast<MIRFuncType *>(funcType->CopyMIRTypeNode());
    if (clearOldArgs) {
        formalDefVec.clear();
        newFuncType->GetParamTypeList().clear();
        newFuncType->GetParamAttrsList().clear();
    }
    for (auto *symbol : symbols) {
        FormalDef formalDef(symbol->GetNameStrIdx(), symbol, symbol->GetTyIdx(), symbol->GetAttrs());
        formalDefVec.push_back(formalDef);
        newFuncType->GetParamTypeList().push_back(symbol->GetTyIdx());
        newFuncType->GetParamAttrsList().push_back(symbol->GetAttrs());
    }
    return newFuncType;
}

void MIRFunction::UpdateFuncTypeAndFormalsAndReturnType(const std::vector<MIRSymbol *> &symbols, const TyIdx &retTyIdx,
                                                        bool clearOldArgs)
{
    auto *newFuncType = ReconstructFormals(symbols, clearOldArgs);
    newFuncType->SetRetTyIdx(retTyIdx);
    auto newFuncTypeIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(newFuncType);
    funcType = static_cast<MIRFuncType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(newFuncTypeIdx));
    delete newFuncType;
}

LabelIdx MIRFunction::GetOrCreateLableIdxFromName(const std::string &name)
{
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
    LabelIdx labelIdx = GetLabelTab()->GetLabelIdxFromStrIdx(strIdx);
    if (labelIdx == 0) {
        labelIdx = GetLabelTab()->CreateLabel();
        GetLabelTab()->SetSymbolFromStIdx(labelIdx, strIdx);
        GetLabelTab()->AddToStringLabelMap(labelIdx);
    }
    return labelIdx;
}

bool MIRFunction::HasCall() const
{
    return flag & kFuncPropHasCall;
}
void MIRFunction::SetHasCall()
{
    flag |= kFuncPropHasCall;
}

#ifdef ARK_LITECG_DEBUG
bool MIRFunction::IsInfoPrinted() const
{
    return flag & kFuncPropInfoPrinted;
}

void MIRFunction::SetInfoPrinted()
{
    flag |= kFuncPropInfoPrinted;
}

void MIRFunction::ResetInfoPrinted()
{
    flag &= ~kFuncPropInfoPrinted;
}

void FuncAttrs::DumpAttributes() const
{
// parse no content of attr
#define STRING(s) #s
#define FUNC_ATTR
#define NOCONTENT_ATTR
#define ATTR(AT)                                     \
    if (GetAttr(FUNCATTR_##AT)) {                    \
        LogInfo::MapleLogger() << " " << STRING(AT); \
    }
#include "all_attributes.def"
#undef ATTR
#undef NOCONTENT_ATTR
#undef FUNC_ATTR
    // parse content of attr
    if (GetAttr(FUNCATTR_alias) && !GetAliasFuncName().empty()) {
        LogInfo::MapleLogger() << " alias ( \"" << GetAliasFuncName() << "\" )";
    }
    if (GetAttr(FUNCATTR_constructor_priority) && GetConstructorPriority() != -1) {
        LogInfo::MapleLogger() << " constructor_priority ( " << GetConstructorPriority() << " )";
    }
    if (GetAttr(FUNCATTR_destructor_priority) && GetDestructorPriority() != -1) {
        LogInfo::MapleLogger() << " destructor_priority ( " << GetDestructorPriority() << " )";
    }
    if (GetAttr(FUNCATTR_frame_pointer) && !framePointer.empty()) {
        LogInfo::MapleLogger() << " frame-pointer ( " << framePointer << " )";
    }
    if (GetAttr(FUNCATTR_frame_reserved_slots) && GetFrameResverdSlot() != 0) {
        LogInfo::MapleLogger() << " frame-reserved-slots ( " << GetFrameResverdSlot() << " )";
    }
}

void MIRFunction::DumpFlavorLoweredThanMmpl() const
{
    LogInfo::MapleLogger() << " (";

    // Dump arguments
    bool hasPrintedFormal = false;
    for (uint32 i = 0; i < formalDefVec.size(); i++) {
        MIRSymbol *symbol = formalDefVec[i].formalSym;
        if (symbol == nullptr &&
            (formalDefVec[i].formalStrIdx.GetIdx() == 0 ||
             GlobalTables::GetStrTable().GetStringFromStrIdx(formalDefVec[i].formalStrIdx).empty())) {
            break;
        }
        hasPrintedFormal = true;
        if (symbol == nullptr) {
            LogInfo::MapleLogger() << "var %"
                                   << GlobalTables::GetStrTable().GetStringFromStrIdx(formalDefVec[i].formalStrIdx)
                                   << " ";
        } else {
            if (symbol->GetSKind() != kStPreg) {
                LogInfo::MapleLogger() << "var %" << symbol->GetName() << " ";
            } else {
                LogInfo::MapleLogger() << "reg %" << symbol->GetPreg()->GetPregNo() << " ";
            }
        }
        MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(formalDefVec[i].formalTyIdx);
        constexpr uint8 indent = 2;
        ty->Dump(indent);
        if (symbol != nullptr) {
            symbol->GetAttrs().DumpAttributes();
        } else {
            formalDefVec[i].formalAttrs.DumpAttributes();
        }
        DEBUG_ASSERT(formalDefVec.size() > 0, "must not be zero");
        if (i != (formalDefVec.size() - 1)) {
            LogInfo::MapleLogger() << ", ";
        }
    }
    if (IsVarargs()) {
        if (!hasPrintedFormal) {
            LogInfo::MapleLogger() << "...";
        } else {
            LogInfo::MapleLogger() << ", ...";
        }
    }

    LogInfo::MapleLogger() << ") ";
    GetReturnType()->Dump(1);
}

void MIRFunction::Dump(bool withoutBody)
{
    // skip the functions that are added during process methods in
    // class and interface decls.  these has nothing in formals
    // they do have paramtypelist_. this can not skip ones without args
    // but for them at least the func decls are valid
    if (GetAttr(FUNCATTR_optimized)) {
        return;
    }

    // save the module's curFunction and set it to the one currently Dump()ing
    MIRFunction *savedFunc = module->CurFunction();
    module->SetCurFunction(this);

    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolTableIdx.Idx());
    DEBUG_ASSERT(symbol != nullptr, "symbol MIRSymbol is null");
    if (!withoutBody) {
        symbol->GetSrcPosition().DumpLoc(MIRSymbol::LastPrintedLineNumRef(), MIRSymbol::LastPrintedColumnNumRef());
    }
    LogInfo::MapleLogger() << "func "
                           << "&" << symbol->GetName();
    theMIRModule = module;
    funcAttrs.DumpAttributes();

    if (symbol->GetWeakrefAttr().first) {
        LogInfo::MapleLogger() << " weakref";
        if (symbol->GetWeakrefAttr().second != UStrIdx(0)) {
            LogInfo::MapleLogger() << " (";
            PrintString(GlobalTables::GetUStrTable().GetStringFromStrIdx(symbol->GetWeakrefAttr().second));
            LogInfo::MapleLogger() << " )";
        }
    }

    if (symbol->sectionAttr != UStrIdx(0)) {
        LogInfo::MapleLogger() << " section (";
        PrintString(GlobalTables::GetUStrTable().GetStringFromStrIdx(symbol->sectionAttr));
        LogInfo::MapleLogger() << " )";
    }

    if (module->GetFlavor() != kMmpl) {
        DumpFlavorLoweredThanMmpl();
    }

    // codeMemPool is nullptr, means maple_ir has been released for memory's sake
    if (codeMemPool == nullptr) {
        LogInfo::MapleLogger() << '\n';
    } else if (GetBody() != nullptr && !withoutBody && symbol->GetStorageClass() != kScExtern) {
        ResetInfoPrinted();  // this ensures funcinfo will be printed
        GetBody()->Dump(0, module->GetFlavor() == kMmpl ? nullptr : GetSymTab(),
                        module->GetFlavor() < kMmpl ? GetPregTab() : nullptr, false, true,
                        module->GetFlavor());  // Dump body
    } else {
        LogInfo::MapleLogger() << '\n';
    }

    // restore the curFunction
    module->SetCurFunction(savedFunc);
}

void MIRFunction::DumpUpFormal(int32 indent) const
{
    PrintIndentation(indent + 1);

    LogInfo::MapleLogger() << "upformalsize " << GetUpFormalSize() << '\n';
    if (localWordsTypeTagged != nullptr) {
        PrintIndentation(indent + 1);
        LogInfo::MapleLogger() << "formalWordsTypeTagged = [ ";
        const auto *p = reinterpret_cast<const uint32 *>(localWordsTypeTagged);
        LogInfo::MapleLogger() << std::hex;
        while (p <
               reinterpret_cast<const uint32 *>(localWordsTypeTagged + BlockSize2BitVectorSize(GetUpFormalSize()))) {
            LogInfo::MapleLogger() << std::hex << "0x" << *p << " ";
            ++p;
        }
        LogInfo::MapleLogger() << std::dec << "]\n";
    }

    if (formalWordsRefCounted != nullptr) {
        PrintIndentation(indent + 1);
        LogInfo::MapleLogger() << "formalWordsRefCounted = [ ";
        const uint32 *p = reinterpret_cast<const uint32 *>(formalWordsRefCounted);
        LogInfo::MapleLogger() << std::hex;
        while (p <
               reinterpret_cast<const uint32 *>(formalWordsRefCounted + BlockSize2BitVectorSize(GetUpFormalSize()))) {
            LogInfo::MapleLogger() << std::hex << "0x" << *p << " ";
            ++p;
        }
        LogInfo::MapleLogger() << std::dec << "]\n";
    }
}

void MIRFunction::DumpFrame(int32 indent) const
{
    PrintIndentation(indent + 1);

    LogInfo::MapleLogger() << "framesize " << static_cast<uint32>(GetFrameSize()) << '\n';
    if (localWordsTypeTagged != nullptr) {
        PrintIndentation(indent + 1);
        LogInfo::MapleLogger() << "localWordsTypeTagged = [ ";
        const uint32 *p = reinterpret_cast<const uint32 *>(localWordsTypeTagged);
        LogInfo::MapleLogger() << std::hex;
        while (p < reinterpret_cast<const uint32 *>(localWordsTypeTagged + BlockSize2BitVectorSize(GetFrameSize()))) {
            LogInfo::MapleLogger() << std::hex << "0x" << *p << " ";
            ++p;
        }
        LogInfo::MapleLogger() << std::dec << "]\n";
    }

    if (localWordsRefCounted != nullptr) {
        PrintIndentation(indent + 1);
        LogInfo::MapleLogger() << "localWordsRefCounted = [ ";
        const uint32 *p = reinterpret_cast<const uint32 *>(localWordsRefCounted);
        LogInfo::MapleLogger() << std::hex;
        while (p < reinterpret_cast<const uint32 *>(localWordsRefCounted + BlockSize2BitVectorSize(GetFrameSize()))) {
            LogInfo::MapleLogger() << std::hex << "0x" << *p << " ";
            ++p;
        }
        LogInfo::MapleLogger() << std::dec << "]\n";
    }
}

void MIRFunction::DumpFuncBody(int32 indent)
{
    LogInfo::MapleLogger() << "  funcid " << GetPuidxOrigin() << '\n';

    if (IsInfoPrinted()) {
        return;
    }

    SetInfoPrinted();

    if (GetUpFormalSize() > 0) {
        DumpUpFormal(indent);
    }

    if (GetFrameSize() > 0) {
        DumpFrame(indent);
    }

    if (GetOutParmSize() > 0) {
        PrintIndentation(indent + 1);
        LogInfo::MapleLogger() << "outparmsize " << GetOutParmSize() << '\n';
    }

    if (GetModuleId() > 0) {
        PrintIndentation(indent + 1);
        LogInfo::MapleLogger() << "moduleID " << static_cast<uint32>(GetModuleId()) << '\n';
    }

    if (GetFuncSize() > 0) {
        PrintIndentation(indent + 1);
        LogInfo::MapleLogger() << "funcSize " << GetFuncSize() << '\n';
    }

    if (GetInfoVector().empty()) {
        return;
    }

    const MIRInfoVector &funcInfo = GetInfoVector();
    const MapleVector<bool> &funcInfoIsString = InfoIsString();
    PrintIndentation(indent + 1);
    LogInfo::MapleLogger() << "funcinfo {\n";
    size_t size = funcInfo.size();
    constexpr int kIndentOffset = 2;
    for (size_t i = 0; i < size; ++i) {
        PrintIndentation(indent + kIndentOffset);
        LogInfo::MapleLogger() << "@" << GlobalTables::GetStrTable().GetStringFromStrIdx(funcInfo[i].first) << " ";
        if (!funcInfoIsString[i]) {
            LogInfo::MapleLogger() << funcInfo[i].second;
        } else {
            LogInfo::MapleLogger() << "\""
                                   << GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(funcInfo[i].second))
                                   << "\"";
        }
        if (i < size - 1) {
            LogInfo::MapleLogger() << ",\n";
        } else {
            LogInfo::MapleLogger() << "}\n";
        }
    }
    LogInfo::MapleLogger() << '\n';
}
#endif

const MIRSymbol *MIRFunction::GetLocalOrGlobalSymbol(const StIdx &idx, bool checkFirst) const
{
    return idx.Islocal() ? GetSymbolTabItem(idx.Idx(), checkFirst)
                         : GlobalTables::GetGsymTable().GetSymbolFromStidx(idx.Idx(), checkFirst);
}
MIRSymbol *MIRFunction::GetLocalOrGlobalSymbol(const StIdx &idx, bool checkFirst)
{
    DEBUG_ASSERT(const_cast<MIRSymbol *>(const_cast<const MIRFunction *>(this)->
                 GetLocalOrGlobalSymbol(idx, checkFirst)) != nullptr,
                 "this->GetLocalOrGlobalSymbol(idx, checkFirst) should not be nullptr");
    return const_cast<MIRSymbol *>(const_cast<const MIRFunction *>(this)->GetLocalOrGlobalSymbol(idx, checkFirst));
}

void MIRFunction::EnterFormals()
{
    for (auto &formalDef : formalDefVec) {
        formalDef.formalSym = symTab->CreateSymbol(kScopeLocal);
        formalDef.formalSym->SetStorageClass(kScFormal);
        formalDef.formalSym->SetNameStrIdx(formalDef.formalStrIdx);
        formalDef.formalSym->SetTyIdx(formalDef.formalTyIdx);
        formalDef.formalSym->SetAttrs(formalDef.formalAttrs);
        const std::string &formalName = GlobalTables::GetStrTable().GetStringFromStrIdx(formalDef.formalStrIdx);
        if (!isdigit(formalName.front())) {
            formalDef.formalSym->SetSKind(kStVar);
            (void)symTab->AddToStringSymbolMap(*formalDef.formalSym);
        } else {
            formalDef.formalSym->SetSKind(kStPreg);
            uint32 thepregno = static_cast<uint32>(std::stoi(formalName));
            MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(formalDef.formalTyIdx);
            PrimType pType = mirType->GetPrimType();
            // if mirType info is not needed, set mirType to nullptr
            if (pType != PTY_ref && pType != PTY_ptr) {
                mirType = nullptr;
            } else if (pType == PTY_ptr && mirType->IsMIRPtrType()) {
                MIRType *pointedType = static_cast<MIRPtrType *>(mirType)->GetPointedType();
                if (pointedType == nullptr || pointedType->GetKind() != kTypeFunction) {
                    mirType = nullptr;
                }
            }
            PregIdx pregIdx = pregTab->EnterPregNo(thepregno, pType, mirType);
            MIRPreg *preg = pregTab->PregFromPregIdx(pregIdx);
            formalDef.formalSym->SetPreg(preg);
        }
    }
}

void MIRFunction::NewBody()
{
    codeMemPool = GetCodeMemPool();
    SetBody(codeMemPool->New<BlockNode>());
    SetLastPosBody(codeMemPool->New<BlockNode>());
    // If mir_function.has been seen as a declaration, its symtab has to be moved
    // from module mempool to function mempool.
    MIRSymbolTable *oldSymTable = GetSymTab();
    MIRPregTable *oldPregTable = GetPregTab();
    MIRTypeNameTable *oldTypeNameTable = typeNameTab;
    MIRLabelTable *oldLabelTable = GetLabelTab();
    symTab = module->GetMemPool()->New<MIRSymbolTable>(module->GetMPAllocator());
    pregTab = module->GetMemPool()->New<MIRPregTable>(&module->GetMPAllocator());
    typeNameTab = module->GetMemPool()->New<MIRTypeNameTable>(module->GetMPAllocator());
    labelTab = module->GetMemPool()->New<MIRLabelTable>(module->GetMPAllocator());

    if (oldSymTable == nullptr) {
        // formals not yet entered into symTab; enter them now
        EnterFormals();
    } else {
        for (size_t i = 1; i < oldSymTable->GetSymbolTableSize(); ++i) {
            (void)GetSymTab()->AddStOutside(oldSymTable->GetSymbolFromStIdx(i));
        }
    }
    if (oldPregTable != nullptr) {
        for (size_t i = 1; i < oldPregTable->Size(); ++i) {
            (void)GetPregTab()->AddPreg(*oldPregTable->PregFromPregIdx(static_cast<PregIdx>(i)));
        }
    }
    if (oldTypeNameTable != nullptr) {
        DEBUG_ASSERT(oldTypeNameTable->Size() == typeNameTab->Size(),
                     "Does not expect to process typeNameTab in MIRFunction::NewBody");
    }
    if (oldLabelTable != nullptr) {
        DEBUG_ASSERT(oldLabelTable->Size() == GetLabelTab()->Size(),
                     "Does not expect to process labelTab in MIRFunction::NewBody");
    }
}
}  // namespace maple
