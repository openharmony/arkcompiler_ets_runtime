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

#include "debug_info.h"
#include "mir_builder.h"
#include "printing.h"
#include "maple_string.h"
#include "global_tables.h"
#include "mir_type.h"
#include <cstring>
#include "securec.h"
#include "mpl_logging.h"
#include "version.h"

namespace maple {
extern const char *GetDwTagName(unsigned n);
extern const char *GetDwFormName(unsigned n);
extern const char *GetDwAtName(unsigned n);
extern const char *GetDwOpName(unsigned n);
extern const char *GetDwAteName(unsigned n);
extern const char *GetDwCfaName(unsigned n);
extern DwAte GetAteFromPTY(PrimType pty);

constexpr uint32 kIndx2 = 2;
constexpr uint32 kStructDBGSize = 8888;

// DBGDie methods
DBGDie::DBGDie(MIRModule *m, DwTag tag)
    : module(m),
      tag(tag),
      id(m->GetDbgInfo()->GetMaxId()),
      withChildren(false),
      sibling(nullptr),
      firstChild(nullptr),
      abbrevId(0),
      tyIdx(0),
      offset(0),
      size(0),
      attrVec(m->GetMPAllocator().Adapter()),
      subDieVec(m->GetMPAllocator().Adapter())
{
    if (module->GetDbgInfo()->GetParentDieSize()) {
        parent = module->GetDbgInfo()->GetParentDie();
    } else {
        parent = nullptr;
    }
    m->GetDbgInfo()->SetIdDieMap(m->GetDbgInfo()->GetIncMaxId(), this);
    attrVec.clear();
    subDieVec.clear();
}

void DBGDie::ResetParentDie()
{
    module->GetDbgInfo()->ResetParentDie();
}

DBGDieAttr *DBGDie::AddAttr(DwAt at, DwForm form, uint64 val)
{
    // collect strps which need label
    if (form == DW_FORM_strp) {
        module->GetDbgInfo()->AddStrps(static_cast<uint32>(val));
    }
    DBGDieAttr *attr = module->GetDbgInfo()->CreateAttr(at, form, val);
    AddAttr(attr);
    return attr;
}

DBGDieAttr *DBGDie::AddSimpLocAttr(DwAt at, DwForm form, uint64 val)
{
    DBGExprLoc *p = module->GetMemPool()->New<DBGExprLoc>(module, DW_OP_fbreg);
    if (val != kDbgDefaultVal) {
        p->AddSimpLocOpnd(val);
    }
    DBGDieAttr *attr = module->GetDbgInfo()->CreateAttr(at, form, reinterpret_cast<uint64>(p));
    AddAttr(attr);
    return attr;
}

DBGDieAttr *DBGDie::AddGlobalLocAttr(DwAt at, DwForm form, uint64 val)
{
    DBGExprLoc *p = module->GetMemPool()->New<DBGExprLoc>(module, DW_OP_addr);
    p->SetGvarStridx(static_cast<int>(val));
    DBGDieAttr *attr = module->GetDbgInfo()->CreateAttr(at, form, reinterpret_cast<uint64>(p));
    AddAttr(attr);
    return attr;
}

DBGDieAttr *DBGDie::AddFrmBaseAttr(DwAt at, DwForm form)
{
    DBGExprLoc *p = module->GetMemPool()->New<DBGExprLoc>(module, DW_OP_call_frame_cfa);
    DBGDieAttr *attr = module->GetDbgInfo()->CreateAttr(at, form, reinterpret_cast<uint64>(p));
    AddAttr(attr);
    return attr;
}

DBGExprLoc *DBGDie::GetExprLoc()
{
    for (auto it : attrVec) {
        if (it->GetDwAt() == DW_AT_location) {
            return it->GetPtr();
        }
    }
    return nullptr;
}

bool DBGDie::SetAttr(DwAt attr, uint64 val)
{
    for (auto it : attrVec) {
        if (it->GetDwAt() == attr) {
            it->SetU(val);
            return true;
        }
    }
    return false;
}

bool DBGDie::SetAttr(DwAt attr, int val)
{
    for (auto it : attrVec) {
        if (it->GetDwAt() == attr) {
            it->SetI(val);
            return true;
        }
    }
    return false;
}

bool DBGDie::SetAttr(DwAt attr, uint32 val)
{
    for (auto it : attrVec) {
        if (it->GetDwAt() == attr) {
            it->SetId(val);
            return true;
        }
    }
    return false;
}

bool DBGDie::SetAttr(DwAt attr, int64 val)
{
    for (auto it : attrVec) {
        if (it->GetDwAt() == attr) {
            it->SetJ(val);
            return true;
        }
    }
    return false;
}

bool DBGDie::SetAttr(DwAt attr, float val)
{
    for (auto it : attrVec) {
        if (it->GetDwAt() == attr) {
            it->SetF(val);
            return true;
        }
    }
    return false;
}

bool DBGDie::SetAttr(DwAt attr, double val)
{
    for (auto it : attrVec) {
        if (it->GetDwAt() == attr) {
            it->SetD(val);
            return true;
        }
    }
    return false;
}

bool DBGDie::SetAttr(DwAt attr, DBGExprLoc *ptr)
{
    for (auto it : attrVec) {
        if (it->GetDwAt() == attr) {
            it->SetPtr(ptr);
            return true;
        }
    }
    return false;
}

void DBGDie::AddAttr(DBGDieAttr *attr)
{
    for (auto it : attrVec) {
        if (it->GetDwAt() == attr->GetDwAt()) {
            return;
        }
    }
    attrVec.push_back(attr);
}

void DBGDie::AddSubVec(DBGDie *die)
{
    if (!die)
        return;
    for (auto it : subDieVec) {
        if (it->GetId() == die->GetId()) {
            return;
        }
    }
    subDieVec.push_back(die);
    die->parent = this;
}

// DBGAbbrevEntry methods
DBGAbbrevEntry::DBGAbbrevEntry(MIRModule *m, DBGDie *die) : attrPairs(m->GetMPAllocator().Adapter())
{
    tag = die->GetTag();
    abbrevId = 0;
    withChildren = die->GetWithChildren();
    for (auto it : die->GetAttrVec()) {
        attrPairs.push_back(it->GetDwAt());
        attrPairs.push_back(it->GetDwForm());
    }
}

bool DBGAbbrevEntry::Equalto(DBGAbbrevEntry *entry)
{
    if (attrPairs.size() != entry->attrPairs.size()) {
        return false;
    }
    if (withChildren != entry->GetWithChildren()) {
        return false;
    }
    for (uint32 i = 0; i < attrPairs.size(); i++) {
        if (attrPairs[i] != entry->attrPairs[i]) {
            return false;
        }
    }
    return true;
}

// DebugInfo methods
void DebugInfo::Init()
{
    mplSrcIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(module->GetFileName());
    compUnit = module->GetMemPool()->New<DBGDie>(module, DW_TAG_compile_unit);
    module->SetWithDbgInfo(true);
    ResetParentDie();
    if (module->GetSrcLang() == kSrcLangC) {
        varPtrPrefix = "";
    }
}

void DebugInfo::SetupCU()
{
    compUnit->SetWithChildren(true);
    /* Add the Producer (Compiler) Information */
    std::string producer = std::string("Maple Version ") + Version::GetVersionStr();
    GStrIdx strIdx = module->GetMIRBuilder()->GetOrCreateStringIndex(producer.c_str());
    compUnit->AddAttr(DW_AT_producer, DW_FORM_strp, strIdx.GetIdx());

    /* Source Languate  */
    compUnit->AddAttr(DW_AT_language, DW_FORM_data4, DW_LANG_C99);

    /* Add the compiled source file information */
    compUnit->AddAttr(DW_AT_name, DW_FORM_strp, mplSrcIdx.GetIdx());
    strIdx = module->GetMIRBuilder()->GetOrCreateStringIndex("/to/be/done/current/path");
    compUnit->AddAttr(DW_AT_comp_dir, DW_FORM_strp, strIdx.GetIdx());

    compUnit->AddAttr(DW_AT_low_pc, DW_FORM_addr, kDbgDefaultVal);
    compUnit->AddAttr(DW_AT_high_pc, DW_FORM_data8, kDbgDefaultVal);

    compUnit->AddAttr(DW_AT_stmt_list, DW_FORM_sec_offset, kDbgDefaultVal);
}

void DebugInfo::AddScopeDie(MIRScope *scope)
{
    if (!scope->NeedEmitAliasInfo()) {
        return;
    }

    if (scope->GetLevel() != 0) {
        DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_lexical_block);
        die->AddAttr(DW_AT_low_pc, DW_FORM_addr, kDbgDefaultVal);
        die->AddAttr(DW_AT_high_pc, DW_FORM_data8, kDbgDefaultVal);

        // add die to parent
        GetParentDie()->AddSubVec(die);

        PushParentDie(die);
    }

    // process aliasVarMap
    AddAliasDies(scope->GetAliasVarMap());

    if (scope->GetSubScopes().size() > 0) {
        // process subScopes
        for (auto it : scope->GetSubScopes()) {
            AddScopeDie(it);
        }
    }

    if (scope->GetLevel() != 0) {
        PopParentDie();
    }
}

void DebugInfo::AddAliasDies(MapleMap<GStrIdx, MIRAliasVars> &aliasMap)
{
    MIRFunction *func = GetCurFunction();
    for (auto &i : aliasMap) {
        // maple var
        MIRSymbol *var = nullptr;
        if (i.second.isLocal) {
            var = func->GetSymTab()->GetSymbolFromStrIdx(i.second.mplStrIdx);
        } else {
            var = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(i.second.mplStrIdx);
        }
        DEBUG_ASSERT(var, "can not find symbol");

        // create alias die using maple var except name
        DBGDie *vdie = CreateVarDie(var, i.first);

        GetParentDie()->AddSubVec(vdie);

        // add alias var name to debug_str section
        strps.insert(i.first.GetIdx());
    }
}

void DebugInfo::Finish()
{
    SetupCU();
    FillTypeAttrWithDieId();
    // build tree from root DIE compUnit
    BuildDieTree();
    BuildAbbrev();
    ComputeSizeAndOffsets();
}

void DebugInfo::BuildDebugInfo()
{
    DEBUG_ASSERT(module->GetDbgInfo(), "null dbgInfo");

    Init();

    // containner types
    for (auto it : module->GetTypeNameTab()->GetGStrIdxToTyIdxMap()) {
        GStrIdx strIdx = it.first;
        TyIdx tyIdx = it.second;
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx.GetIdx());

        switch (type->GetKind()) {
            case kTypeClass:
            case kTypeClassIncomplete:
            case kTypeInterface:
            case kTypeInterfaceIncomplete:
            case kTypeStruct:
            case kTypeStructIncomplete:
            case kTypeUnion: {
                (void)GetOrCreateStructTypeDie(type);
                break;
            }
            default:
                LogInfo::MapleLogger() << "named type "
                                       << GlobalTables::GetStrTable().GetStringFromStrIdx(strIdx).c_str() << "\n";
                break;
        }
    }

    for (size_t i = 0; i < GlobalTables::GetGsymTable().GetSymbolTableSize(); ++i) {
        MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(static_cast<uint32>(i));
        if (mirSymbol == nullptr || mirSymbol->IsDeleted() || mirSymbol->GetStorageClass() == kScUnused ||
            mirSymbol->GetStorageClass() == kScExtern) {
            continue;
        }
        if (module->IsCModule() && mirSymbol->IsGlobal() && mirSymbol->IsVar()) {
            DBGDie *vdie = CreateVarDie(mirSymbol);
            compUnit->AddSubVec(vdie);
        }
    }

    // setup debug info for functions
    for (auto func : GlobalTables::GetFunctionTable().GetFuncTable()) {
        // the first one in funcTable is nullptr
        if (!func) {
            continue;
        }
        SetCurFunction(func);
        // function decl
        if (stridxDieIdMap.find(func->GetNameStrIdx().GetIdx()) == stridxDieIdMap.end()) {
            DBGDie *fdie = GetOrCreateFuncDeclDie(func);
            if (!func->GetClassTyIdx().GetIdx() && func->GetBody()) {
                compUnit->AddSubVec(fdie);
            }
        }
        // function def
        if (funcDefStrIdxDieIdMap.find(func->GetNameStrIdx().GetIdx()) == funcDefStrIdxDieIdMap.end()) {
            DBGDie *fdie = GetOrCreateFuncDefDie(func, 0);
            if (!func->GetClassTyIdx().GetIdx() && func->GetBody()) {
                compUnit->AddSubVec(fdie);
            }
        }
    }

    // finalize debug info
    Finish();
}

DBGDieAttr *DebugInfo::CreateAttr(DwAt at, DwForm form, uint64 val)
{
    DBGDieAttr *attr = module->GetMemPool()->New<DBGDieAttr>(kDwAt);
    attr->SetDwAt(at);
    attr->SetDwForm(form);
    attr->SetU(val);
    return attr;
}

void DebugInfo::SetLocalDie(MIRFunction *func, GStrIdx strIdx, const DBGDie *die)
{
    (funcLstrIdxDieIdMap[func])[strIdx.GetIdx()] = die->GetId();
}

DBGDie *DebugInfo::GetLocalDie(MIRFunction *func, GStrIdx strIdx)
{
    uint32 id = (funcLstrIdxDieIdMap[func])[strIdx.GetIdx()];
    return idDieMap[id];
}

void DebugInfo::SetLocalDie(GStrIdx strIdx, const DBGDie *die)
{
    (funcLstrIdxDieIdMap[GetCurFunction()])[strIdx.GetIdx()] = die->GetId();
}

DBGDie *DebugInfo::GetLocalDie(GStrIdx strIdx)
{
    uint32 id = (funcLstrIdxDieIdMap[GetCurFunction()])[strIdx.GetIdx()];
    return idDieMap[id];
}

void DebugInfo::SetLabelIdx(MIRFunction *func, GStrIdx strIdx, LabelIdx labidx)
{
    (funcLstrIdxLabIdxMap[func])[strIdx.GetIdx()] = labidx;
}

LabelIdx DebugInfo::GetLabelIdx(MIRFunction *func, GStrIdx strIdx)
{
    LabelIdx labidx = (funcLstrIdxLabIdxMap[func])[strIdx.GetIdx()];
    return labidx;
}

void DebugInfo::SetLabelIdx(GStrIdx strIdx, LabelIdx labidx)
{
    (funcLstrIdxLabIdxMap[GetCurFunction()])[strIdx.GetIdx()] = labidx;
}

LabelIdx DebugInfo::GetLabelIdx(GStrIdx strIdx)
{
    LabelIdx labidx = (funcLstrIdxLabIdxMap[GetCurFunction()])[strIdx.GetIdx()];
    return labidx;
}

DBGDie *DebugInfo::CreateFormalParaDie(MIRFunction *func, MIRType *type, MIRSymbol *sym)
{
    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_formal_parameter);

    (void)GetOrCreateTypeDie(type);
    ASSERT_NOT_NULL(type);
    die->AddAttr(DW_AT_type, DW_FORM_ref4, type->GetTypeIndex().GetIdx());

    /* var Name */
    if (sym) {
        die->AddAttr(DW_AT_name, DW_FORM_strp, sym->GetNameStrIdx().GetIdx());
        die->AddAttr(DW_AT_decl_file, DW_FORM_data4, sym->GetSrcPosition().FileNum());
        die->AddAttr(DW_AT_decl_line, DW_FORM_data4, sym->GetSrcPosition().LineNum());
        die->AddAttr(DW_AT_decl_column, DW_FORM_data4, sym->GetSrcPosition().Column());
        die->AddSimpLocAttr(DW_AT_location, DW_FORM_exprloc, kDbgDefaultVal);
        SetLocalDie(func, sym->GetNameStrIdx(), die);
    }
    return die;
}

DBGDie *DebugInfo::GetOrCreateLabelDie(LabelIdx labid)
{
    MIRFunction *func = GetCurFunction();
    CHECK(labid < func->GetLabelTab()->GetLabelTableSize(), "index out of range in DebugInfo::GetOrCreateLabelDie");
    GStrIdx strid = func->GetLabelTab()->GetSymbolFromStIdx(labid);
    if ((funcLstrIdxDieIdMap[func]).size() &&
        (funcLstrIdxDieIdMap[func]).find(strid.GetIdx()) != (funcLstrIdxDieIdMap[func]).end()) {
        return GetLocalDie(strid);
    }

    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_label);
    die->AddAttr(DW_AT_name, DW_FORM_strp, strid.GetIdx());
    die->AddAttr(DW_AT_decl_file, DW_FORM_data4, mplSrcIdx.GetIdx());
    die->AddAttr(DW_AT_decl_line, DW_FORM_data4, lexer->GetLineNum());
    die->AddAttr(DW_AT_low_pc, DW_FORM_addr, kDbgDefaultVal);
    GetParentDie()->AddSubVec(die);
    SetLocalDie(strid, die);
    SetLabelIdx(strid, labid);
    return die;
}

DBGDie *DebugInfo::CreateVarDie(MIRSymbol *sym)
{
    // filter vtab
    if (sym->GetName().find(VTAB_PREFIX_STR) == 0) {
        return nullptr;
    }

    if (sym->GetName().find(GCTIB_PREFIX_STR) == 0) {
        return nullptr;
    }

    if (sym->GetStorageClass() == kScFormal) {
        return nullptr;
    }

    bool isLocal = sym->IsLocal();
    GStrIdx strIdx = sym->GetNameStrIdx();

    if (isLocal) {
        MIRFunction *func = GetCurFunction();
        if ((funcLstrIdxDieIdMap[func]).size() &&
            (funcLstrIdxDieIdMap[func]).find(strIdx.GetIdx()) != (funcLstrIdxDieIdMap[func]).end()) {
            return GetLocalDie(strIdx);
        }
    } else {
        if (stridxDieIdMap.find(strIdx.GetIdx()) != stridxDieIdMap.end()) {
            uint32 id = stridxDieIdMap[strIdx.GetIdx()];
            return idDieMap[id];
        }
    }

    DBGDie *die = CreateVarDie(sym, strIdx);

    GetParentDie()->AddSubVec(die);
    if (isLocal) {
        SetLocalDie(strIdx, die);
    } else {
        stridxDieIdMap[strIdx.GetIdx()] = die->GetId();
    }

    return die;
}

DBGDie *DebugInfo::CreateVarDie(MIRSymbol *sym, GStrIdx strIdx)
{
    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_variable);

    /* var Name */
    die->AddAttr(DW_AT_name, DW_FORM_strp, strIdx.GetIdx());
    die->AddAttr(DW_AT_decl_file, DW_FORM_data4, sym->GetSrcPosition().FileNum());
    die->AddAttr(DW_AT_decl_line, DW_FORM_data4, sym->GetSrcPosition().LineNum());
    die->AddAttr(DW_AT_decl_column, DW_FORM_data4, sym->GetSrcPosition().Column());

    bool isLocal = sym->IsLocal();
    if (isLocal) {
        die->AddSimpLocAttr(DW_AT_location, DW_FORM_exprloc, kDbgDefaultVal);
    } else {
        // global var just use its name as address in .s
        uint64 idx = strIdx.GetIdx();
        if ((sym->IsReflectionClassInfo() && !sym->IsReflectionArrayClassInfo()) || sym->IsStatic()) {
            std::string ptrName = varPtrPrefix + sym->GetName();
            idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(ptrName).GetIdx();
        }
        die->AddGlobalLocAttr(DW_AT_location, DW_FORM_exprloc, idx);
    }

    MIRType *type = sym->GetType();
    (void)GetOrCreateTypeDie(type);
    ASSERT_NOT_NULL(type);
    die->AddAttr(DW_AT_type, DW_FORM_ref4, type->GetTypeIndex().GetIdx());

    return die;
}

DBGDie *DebugInfo::GetOrCreateFuncDeclDie(MIRFunction *func)
{
    uint32 funcnameidx = func->GetNameStrIdx().GetIdx();
    if (stridxDieIdMap.find(funcnameidx) != stridxDieIdMap.end()) {
        uint32 id = stridxDieIdMap[funcnameidx];
        return idDieMap[id];
    }

    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_subprogram);
    stridxDieIdMap[funcnameidx] = die->GetId();

    die->AddAttr(DW_AT_external, DW_FORM_flag_present, 1);

    // Function Name
    MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());

    die->AddAttr(DW_AT_name, DW_FORM_strp, funcnameidx);
    die->AddAttr(DW_AT_decl_file, DW_FORM_data4, sym->GetSrcPosition().FileNum());
    die->AddAttr(DW_AT_decl_line, DW_FORM_data4, sym->GetSrcPosition().LineNum());
    die->AddAttr(DW_AT_decl_column, DW_FORM_data4, sym->GetSrcPosition().Column());

    // Attributes for DW_AT_accessibility
    uint32 access = 0;
    if (func->IsPublic()) {
        access = DW_ACCESS_public;
    } else if (func->IsPrivate()) {
        access = DW_ACCESS_private;
    } else if (func->IsProtected()) {
        access = DW_ACCESS_protected;
    }
    if (access) {
        die->AddAttr(DW_AT_accessibility, DW_FORM_data4, access);
    }

    die->AddAttr(DW_AT_GNU_all_tail_call_sites, DW_FORM_flag_present, kDbgDefaultVal);

    PushParentDie(die);

    // formal parameter
    for (uint32 i = 0; i < func->GetFormalCount(); i++) {
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(func->GetFormalDefAt(i).formalTyIdx);
        DBGDie *param = CreateFormalParaDie(func, type, nullptr);
        die->AddSubVec(param);
    }

    PopParentDie();

    return die;
}

bool LIsCompilerGenerated(const MIRFunction *func)
{
    return ((func->GetName().c_str())[0] != 'L');
}

DBGDie *DebugInfo::GetOrCreateFuncDefDie(MIRFunction *func, uint32 lnum)
{
    uint32 funcnameidx = func->GetNameStrIdx().GetIdx();
    if (funcDefStrIdxDieIdMap.find(funcnameidx) != funcDefStrIdxDieIdMap.end()) {
        uint32 id = funcDefStrIdxDieIdMap[funcnameidx];
        return idDieMap[id];
    }

    DBGDie *funcdecldie = GetOrCreateFuncDeclDie(func);
    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_subprogram);
    // update funcDefStrIdxDieIdMap and leave stridxDieIdMap for the func decl
    funcDefStrIdxDieIdMap[funcnameidx] = die->GetId();

    die->AddAttr(DW_AT_specification, DW_FORM_ref4, funcdecldie->GetId());
    die->AddAttr(DW_AT_decl_line, DW_FORM_data4, lnum);

    if (!func->IsReturnVoid()) {
        auto returnType = func->GetReturnType();
        (void)GetOrCreateTypeDie(returnType);
        ASSERT_NOT_NULL(returnType);
        die->AddAttr(DW_AT_type, DW_FORM_ref4, returnType->GetTypeIndex().GetIdx());
    }

    die->AddAttr(DW_AT_low_pc, DW_FORM_addr, kDbgDefaultVal);
    die->AddAttr(DW_AT_high_pc, DW_FORM_data8, kDbgDefaultVal);
    die->AddFrmBaseAttr(DW_AT_frame_base, DW_FORM_exprloc);
    if (!func->IsStatic() && !LIsCompilerGenerated(func)) {
        die->AddAttr(DW_AT_object_pointer, DW_FORM_ref4, kDbgDefaultVal);
    }
    die->AddAttr(DW_AT_GNU_all_tail_call_sites, DW_FORM_flag_present, kDbgDefaultVal);

    PushParentDie(die);

    // formal parameter
    for (uint32 i = 0; i < func->GetFormalCount(); i++) {
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(func->GetFormalDefAt(i).formalTyIdx);
        DBGDie *pdie = CreateFormalParaDie(func, type, func->GetFormalDefAt(i).formalSym);
        die->AddSubVec(pdie);
    }

    if (func->GetSymTab()) {
        // local variables, start from 1
        for (uint32 i = 1; i < func->GetSymTab()->GetSymbolTableSize(); i++) {
            MIRSymbol *var = func->GetSymTab()->GetSymbolFromStIdx(i);
            DBGDie *vdie = CreateVarDie(var);
            die->AddSubVec(vdie);
        }
    }

    // add scope die
    AddScopeDie(func->GetScope());

    PopParentDie();

    return die;
}

DBGDie *DebugInfo::GetOrCreatePrimTypeDie(MIRType *ty)
{
    PrimType pty = ty->GetPrimType();
    uint32 tid = static_cast<uint32>(pty);
    if (tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end()) {
        uint32 id = tyIdxDieIdMap[tid];
        return idDieMap[id];
    }

    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_base_type);
    die->SetTyIdx(static_cast<uint32>(pty));

    if (ty->GetNameStrIdx().GetIdx() == 0) {
        const char *name = GetPrimTypeName(ty->GetPrimType());
        std::string pname = std::string(name);
        GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(pname);
        ty->SetNameStrIdx(strIdx);
    }

    die->AddAttr(DW_AT_byte_size, DW_FORM_data4, GetPrimTypeSize(pty));
    die->AddAttr(DW_AT_encoding, DW_FORM_data4, GetAteFromPTY(pty));
    die->AddAttr(DW_AT_name, DW_FORM_strp, ty->GetNameStrIdx().GetIdx());

    compUnit->AddSubVec(die);
    tyIdxDieIdMap[static_cast<uint32>(pty)] = die->GetId();
    return die;
}

DBGDie *DebugInfo::CreatePointedFuncTypeDie(MIRFuncType *ftype)
{
    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_subroutine_type);

    die->AddAttr(DW_AT_prototyped, DW_FORM_data4, static_cast<int>(ftype->GetParamTypeList().size() > 0));
    MIRType *rtype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ftype->GetRetTyIdx());
    (void)GetOrCreateTypeDie(rtype);
    die->AddAttr(DW_AT_type, DW_FORM_ref4, ftype->GetRetTyIdx().GetIdx());

    compUnit->AddSubVec(die);

    for (uint32 i = 0; i < ftype->GetParamTypeList().size(); i++) {
        DBGDie *paramdie = module->GetMemPool()->New<DBGDie>(module, DW_TAG_formal_parameter);
        MIRType *ptype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ftype->GetNthParamType(i));
        (void)GetOrCreateTypeDie(ptype);
        paramdie->AddAttr(DW_AT_type, DW_FORM_ref4, ftype->GetNthParamType(i).GetIdx());
        die->AddSubVec(paramdie);
    }

    tyIdxDieIdMap[ftype->GetTypeIndex().GetIdx()] = die->GetId();
    return die;
}

DBGDie *DebugInfo::GetOrCreateTypeDie(MIRType *type)
{
    if (type == nullptr) {
        return nullptr;
    }

    uint32 tid = type->GetTypeIndex().GetIdx();
    if (tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end()) {
        uint32 id = tyIdxDieIdMap[tid];
        return idDieMap[id];
    }

    uint32 sid = type->GetNameStrIdx().GetIdx();
    if (sid)
        if (stridxDieIdMap.find(sid) != stridxDieIdMap.end()) {
            uint32 id = stridxDieIdMap[sid];
            return idDieMap[id];
        }

    if (type->GetTypeIndex() == static_cast<uint32>(type->GetPrimType())) {
        return GetOrCreatePrimTypeDie(type);
    }

    DBGDie *die = nullptr;
    switch (type->GetKind()) {
        case kTypePointer: {
            MIRPtrType *ptype = static_cast<MIRPtrType *>(type);
            die = GetOrCreatePointTypeDie(ptype);
            break;
        }
        case kTypeFunction: {
            MIRFuncType *ftype = static_cast<MIRFuncType *>(type);
            die = CreatePointedFuncTypeDie(ftype);
            break;
        }
        case kTypeArray:
        case kTypeFArray:
        case kTypeJArray: {
            MIRArrayType *atype = static_cast<MIRArrayType *>(type);
            die = GetOrCreateArrayTypeDie(atype);
            break;
        }
        case kTypeUnion:
        case kTypeStruct:
        case kTypeStructIncomplete:
        case kTypeClass:
        case kTypeClassIncomplete:
        case kTypeInterface:
        case kTypeInterfaceIncomplete: {
            die = GetOrCreateStructTypeDie(type);
            break;
        }
        case kTypeBitField:
            break;
        default:
            CHECK_FATAL(false, "Not support type now");
            break;
    }

    return die;
}

DBGDie *DebugInfo::GetOrCreatePointTypeDie(const MIRPtrType *ptrtype)
{
    uint32 tid = ptrtype->GetTypeIndex().GetIdx();
    if (tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end()) {
        uint32 id = tyIdxDieIdMap[tid];
        return idDieMap[id];
    }

    MIRType *type = ptrtype->GetPointedType();
    // for <* void>
    if ((type != nullptr) && (type->GetPrimType() == PTY_void || type->GetKind() == kTypeFunction)) {
        DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_pointer_type);
        die->AddAttr(DW_AT_byte_size, DW_FORM_data4, k8BitSize);
        if (type->GetKind() == kTypeFunction) {
            DBGDie *pdie = GetOrCreateTypeDie(type);
            die->AddAttr(DW_AT_type, DW_FORM_ref4, type->GetTypeIndex().GetIdx());
            tyIdxDieIdMap[type->GetTypeIndex().GetIdx()] = pdie->GetId();
        }
        tyIdxDieIdMap[ptrtype->GetTypeIndex().GetIdx()] = die->GetId();
        compUnit->AddSubVec(die);
        return die;
    }

    (void)GetOrCreateTypeDie(type);
    ASSERT_NOT_NULL(type);
    if (typeDefTyIdxMap.find(type->GetTypeIndex().GetIdx()) != typeDefTyIdxMap.end()) {
        uint32 tyIdx = typeDefTyIdxMap[type->GetTypeIndex().GetIdx()];
        if (pointedPointerMap.find(tyIdx) != pointedPointerMap.end()) {
            uint32 tyid = pointedPointerMap[tyIdx];
            if (tyIdxDieIdMap.find(tyid) != tyIdxDieIdMap.end()) {
                uint32 dieid = tyIdxDieIdMap[tyid];
                DBGDie *die = idDieMap[dieid];
                return die;
            }
        }
    }

    // update incomplete type from stridxDieIdMap to tyIdxDieIdMap
    MIRStructType *stype = static_cast<MIRStructType *>(type);
    if ((stype != nullptr) && stype->IsIncomplete()) {
        uint32 sid = stype->GetNameStrIdx().GetIdx();
        if (stridxDieIdMap.find(sid) != stridxDieIdMap.end()) {
            uint32 dieid = stridxDieIdMap[sid];
            if (dieid) {
                tyIdxDieIdMap[stype->GetTypeIndex().GetIdx()] = dieid;
            }
        }
    }

    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_pointer_type);
    die->AddAttr(DW_AT_byte_size, DW_FORM_data4, k8BitSize);
    // fill with type idx instead of typedie->id to avoid nullptr typedie of
    // forward reference of class types
    die->AddAttr(DW_AT_type, DW_FORM_ref4, type->GetTypeIndex().GetIdx());
    tyIdxDieIdMap[ptrtype->GetTypeIndex().GetIdx()] = die->GetId();

    compUnit->AddSubVec(die);

    return die;
}

DBGDie *DebugInfo::GetOrCreateArrayTypeDie(const MIRArrayType *arraytype)
{
    uint32 tid = arraytype->GetTypeIndex().GetIdx();
    if (tyIdxDieIdMap.find(tid) != tyIdxDieIdMap.end()) {
        uint32 id = tyIdxDieIdMap[tid];
        return idDieMap[id];
    }

    MIRType *type = arraytype->GetElemType();
    (void)GetOrCreateTypeDie(type);

    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_array_type);
    die->AddAttr(DW_AT_byte_size, DW_FORM_data4, k8BitSize);
    // fill with type idx instead of typedie->id to avoid nullptr typedie of
    // forward reference of class types
    die->AddAttr(DW_AT_type, DW_FORM_ref4, type->GetTypeIndex().GetIdx());
    tyIdxDieIdMap[arraytype->GetTypeIndex().GetIdx()] = die->GetId();

    compUnit->AddSubVec(die);

    // maple uses array of 1D array to represent 2D array
    // so only one DW_TAG_subrange_type entry is needed
    DBGDie *rangedie = module->GetMemPool()->New<DBGDie>(module, DW_TAG_subrange_type);
    (void)GetOrCreatePrimTypeDie(GlobalTables::GetTypeTable().GetUInt32());
    rangedie->AddAttr(DW_AT_type, DW_FORM_ref4, PTY_u32);
    rangedie->AddAttr(DW_AT_upper_bound, DW_FORM_data4, arraytype->GetSizeArrayItem(0));

    die->AddSubVec(rangedie);

    return die;
}

DBGDie *DebugInfo::CreateFieldDie(maple::FieldPair pair, uint32 lnum)
{
    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_member);
    die->AddAttr(DW_AT_name, DW_FORM_strp, pair.first.GetIdx());
    die->AddAttr(DW_AT_decl_file, DW_FORM_data4, mplSrcIdx.GetIdx());
    die->AddAttr(DW_AT_decl_line, DW_FORM_data4, lnum);

    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pair.second.first);
    (void)GetOrCreateTypeDie(type);
    // fill with type idx instead of typedie->id to avoid nullptr typedie of
    // forward reference of class types
    ASSERT_NOT_NULL(type);
    die->AddAttr(DW_AT_type, DW_FORM_ref4, type->GetTypeIndex().GetIdx());

    die->AddAttr(DW_AT_data_member_location, DW_FORM_data4, kDbgDefaultVal);

    return die;
}

DBGDie *DebugInfo::CreateBitfieldDie(const MIRBitFieldType *type, GStrIdx sidx, uint32 prevBits)
{
    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_member);

    die->AddAttr(DW_AT_name, DW_FORM_strp, sidx.GetIdx());
    die->AddAttr(DW_AT_decl_file, DW_FORM_data4, mplSrcIdx.GetIdx());
    die->AddAttr(DW_AT_decl_line, DW_FORM_data4, 0);

    MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(type->GetPrimType());
    (void)GetOrCreateTypeDie(ty);
    ASSERT_NOT_NULL(ty);
    die->AddAttr(DW_AT_type, DW_FORM_ref4, ty->GetTypeIndex().GetIdx());

    die->AddAttr(DW_AT_byte_size, DW_FORM_data4, GetPrimTypeSize(type->GetPrimType()));
    die->AddAttr(DW_AT_bit_size, DW_FORM_data4, type->GetFieldSize());
    die->AddAttr(DW_AT_bit_offset, DW_FORM_data4,
                 GetPrimTypeSize(type->GetPrimType()) * k8BitSize - type->GetFieldSize() - prevBits);
    die->AddAttr(DW_AT_data_member_location, DW_FORM_data4, 0);

    return die;
}

DBGDie *DebugInfo::GetOrCreateStructTypeDie(const MIRType *type)
{
    DEBUG_ASSERT(type, "null struture type");
    GStrIdx strIdx = type->GetNameStrIdx();
    DEBUG_ASSERT(strIdx.GetIdx(), "struture type missing name");

    if (tyIdxDieIdMap.find(type->GetTypeIndex().GetIdx()) != tyIdxDieIdMap.end()) {
        uint32 id = tyIdxDieIdMap[type->GetTypeIndex().GetIdx()];
        return idDieMap[id];
    }

    DBGDie *die = nullptr;
    switch (type->GetKind()) {
        case kTypeClass:
        case kTypeClassIncomplete: {
            const MIRClassType *classtype = static_cast<const MIRClassType *>(type);
            die = CreateClassTypeDie(strIdx, classtype);
            break;
        }
        case kTypeInterface:
        case kTypeInterfaceIncomplete: {
            const MIRInterfaceType *interfacetype = static_cast<const MIRInterfaceType *>(type);
            die = CreateInterfaceTypeDie(strIdx, interfacetype);
            break;
        }
        case kTypeStruct:
        case kTypeStructIncomplete:
        case kTypeUnion: {
            const MIRStructType *stype = static_cast<const MIRStructType *>(type);
            die = CreateStructTypeDie(strIdx, stype, false);
            break;
        }
        default:
            LogInfo::MapleLogger() << "named type " << GlobalTables::GetStrTable().GetStringFromStrIdx(strIdx).c_str()
                                   << "\n";
            break;
    }

    GlobalTables::GetTypeNameTable().SetGStrIdxToTyIdx(strIdx, type->GetTypeIndex());
    return die;
}

// shared between struct and union
DBGDie *DebugInfo::CreateStructTypeDie(GStrIdx strIdx, const MIRStructType *structtype, bool update)
{
    DBGDie *die = nullptr;

    if (update) {
        uint32 id = tyIdxDieIdMap[structtype->GetTypeIndex().GetIdx()];
        die = idDieMap[id];
        DEBUG_ASSERT(die, "update type die not exist");
    } else {
        DwTag tag = structtype->GetKind() == kTypeStruct ? DW_TAG_structure_type : DW_TAG_union_type;
        die = module->GetMemPool()->New<DBGDie>(module, tag);
        tyIdxDieIdMap[structtype->GetTypeIndex().GetIdx()] = die->GetId();
    }

    if (strIdx.GetIdx()) {
        stridxDieIdMap[strIdx.GetIdx()] = die->GetId();
    }

    compUnit->AddSubVec(die);

    die->AddAttr(DW_AT_decl_line, DW_FORM_data4, kStructDBGSize);
    die->AddAttr(DW_AT_name, DW_FORM_strp, strIdx.GetIdx());
    die->AddAttr(DW_AT_byte_size, DW_FORM_data4, kDbgDefaultVal);
    die->AddAttr(DW_AT_decl_file, DW_FORM_data4, mplSrcIdx.GetIdx());

    PushParentDie(die);

    // fields
    uint32 prevBits = 0;
    for (size_t i = 0; i < structtype->GetFieldsSize(); i++) {
        MIRType *ety = structtype->GetElemType(static_cast<uint32>(i));
        FieldPair fp = structtype->GetFieldsElemt(i);
        if (ety->IsMIRBitFieldType()) {
            MIRBitFieldType *bfty = static_cast<MIRBitFieldType *>(ety);
            DBGDie *bfdie = CreateBitfieldDie(bfty, fp.first, prevBits);
            prevBits += bfty->GetFieldSize();
            die->AddSubVec(bfdie);
        } else {
            prevBits = 0;
            DBGDie *fdie = CreateFieldDie(fp, 0);
            die->AddSubVec(fdie);
        }
    }

    // parentFields
    for (size_t i = 0; i < structtype->GetParentFieldsSize(); i++) {
        FieldPair fp = structtype->GetParentFieldsElemt(i);
        DBGDie *fdie = CreateFieldDie(fp, 0);
        die->AddSubVec(fdie);
    }

    // member functions decl
    for (auto fp : structtype->GetMethods()) {
        MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(fp.first.Idx());
        DEBUG_ASSERT((symbol != nullptr) && symbol->GetSKind() == kStFunc, "member function symbol not exist");
        MIRFunction *func = symbol->GetValue().mirFunc;
        DEBUG_ASSERT(func, "member function not exist");
        DBGDie *fdie = GetOrCreateFuncDeclDie(func);
        die->AddSubVec(fdie);
    }

    PopParentDie();

    // member functions defination, these die are global
    for (auto fp : structtype->GetMethods()) {
        MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(fp.first.Idx());
        DEBUG_ASSERT(symbol && symbol->GetSKind() == kStFunc, "member function symbol not exist");
        MIRFunction *func = symbol->GetValue().mirFunc;
        if (!func->GetBody()) {
            continue;
        }
        DEBUG_ASSERT(func, "member function not exist");
        DBGDie *fdie = GetOrCreateFuncDefDie(func, 0);
        compUnit->AddSubVec(fdie);
    }

    return die;
}

DBGDie *DebugInfo::CreateClassTypeDie(GStrIdx strIdx, const MIRClassType *classtype)
{
    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_class_type);

    PushParentDie(die);

    // parent
    uint32 ptid = classtype->GetParentTyIdx().GetIdx();
    if (ptid) {
        MIRType *parenttype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(classtype->GetParentTyIdx());
        DBGDie *parentdie = GetOrCreateTypeDie(parenttype);
        if (parentdie) {
            parentdie = module->GetMemPool()->New<DBGDie>(module, DW_TAG_inheritance);
            parentdie->AddAttr(DW_AT_name, DW_FORM_strp, parenttype->GetNameStrIdx().GetIdx());
            parentdie->AddAttr(DW_AT_type, DW_FORM_ref4, ptid);

            // set to DW_ACCESS_public for now
            parentdie->AddAttr(DW_AT_accessibility, DW_FORM_data4, DW_ACCESS_public);
            die->AddSubVec(parentdie);
        }
    }

    PopParentDie();

    // update common fields
    tyIdxDieIdMap[classtype->GetTypeIndex().GetIdx()] = die->GetId();
    DBGDie *die1 = CreateStructTypeDie(strIdx, classtype, true);
    DEBUG_ASSERT(die == die1, "ClassTypeDie update wrong die");

    return die1;
}

DBGDie *DebugInfo::CreateInterfaceTypeDie(GStrIdx strIdx, const MIRInterfaceType *interfacetype)
{
    DBGDie *die = module->GetMemPool()->New<DBGDie>(module, DW_TAG_interface_type);

    PushParentDie(die);

    // parents
    for (auto it : interfacetype->GetParentsTyIdx()) {
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(it);
        DBGDie *parentdie = GetOrCreateTypeDie(type);
        if (parentdie) {
            continue;
        }
        parentdie = module->GetMemPool()->New<DBGDie>(module, DW_TAG_inheritance);
        parentdie->AddAttr(DW_AT_name, DW_FORM_strp, type->GetNameStrIdx().GetIdx());
        parentdie->AddAttr(DW_AT_type, DW_FORM_ref4, it.GetIdx());
        parentdie->AddAttr(DW_AT_data_member_location, DW_FORM_data4, kDbgDefaultVal);

        // set to DW_ACCESS_public for now
        parentdie->AddAttr(DW_AT_accessibility, DW_FORM_data4, DW_ACCESS_public);
        die->AddSubVec(parentdie);
    }

    PopParentDie();

    // update common fields
    tyIdxDieIdMap[interfacetype->GetTypeIndex().GetIdx()] = die->GetId();
    DBGDie *die1 = CreateStructTypeDie(strIdx, interfacetype, true);
    DEBUG_ASSERT(die == die1, "InterfaceTypeDie update wrong die");

    return die1;
}

uint32 DebugInfo::GetAbbrevId(DBGAbbrevEntryVec *vec, DBGAbbrevEntry *entry)
{
    for (auto it : vec->GetEntryvec()) {
        if (it->Equalto(entry)) {
            return it->GetAbbrevId();
            ;
        }
    }
    return 0;
}

void DebugInfo::BuildAbbrev()
{
    uint32 abbrevid = 1;
    for (uint32 i = 1; i < maxId; i++) {
        DBGDie *die = idDieMap[i];
        DBGAbbrevEntry *entry = module->GetMemPool()->New<DBGAbbrevEntry>(module, die);

        if (!tagAbbrevMap[die->GetTag()]) {
            tagAbbrevMap[die->GetTag()] = module->GetMemPool()->New<DBGAbbrevEntryVec>(module, die->GetTag());
        }

        uint32 id = GetAbbrevId(tagAbbrevMap[die->GetTag()], entry);
        if (id) {
            // using existing abbrev id
            die->SetAbbrevId(id);
        } else {
            // add entry to vector
            entry->SetAbbrevId(abbrevid++);
            tagAbbrevMap[die->GetTag()]->GetEntryvec().push_back(entry);
            abbrevVec.push_back(entry);
            // update abbrevid in die
            die->SetAbbrevId(entry->GetAbbrevId());
        }
    }
    for (uint32 i = 1; i < maxId; i++) {
        DBGDie *die = idDieMap[i];
        if (die->GetAbbrevId() == 0) {
            LogInfo::MapleLogger() << "0 abbrevId i = " << i << " die->id = " << die->GetId() << std::endl;
        }
    }
}

void DebugInfo::BuildDieTree()
{
    for (auto it : idDieMap) {
        if (!it.first) {
            continue;
        }
        DBGDie *die = it.second;
        uint32 size = die->GetSubDieVecSize();
        die->SetWithChildren(size > 0);
        if (size) {
            die->SetFirstChild(die->GetSubDieVecAt(0));
            for (uint32 i = 0; i < size - 1; i++) {
                DBGDie *it0 = die->GetSubDieVecAt(i);
                DBGDie *it1 = die->GetSubDieVecAt(i + 1);
                if (it0->GetSubDieVecSize()) {
                    it0->SetSibling(it1);
                    (void)it0->AddAttr(DW_AT_sibling, DW_FORM_ref4, it1->GetId());
                }
            }
        }
    }
}

void DebugInfo::FillTypeAttrWithDieId()
{
    for (auto it : idDieMap) {
        DBGDie *die = it.second;
        for (auto at : die->GetAttrVec()) {
            if (at->GetDwAt() == DW_AT_type) {
                uint32 tid = at->GetId();
                MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(tid));
                if (type) {
                    uint32 dieid = tyIdxDieIdMap[tid];
                    if (dieid) {
                        at->SetId(dieid);
                    } else {
                        LogInfo::MapleLogger() << "dieid not found, typeKind = " << type->GetKind()
                                               << " primType = " << type->GetPrimType()
                                               << " nameStrIdx = " << type->GetNameStrIdx().GetIdx() << std::endl;
                    }
                } else {
                    LogInfo::MapleLogger() << "type not found, tid = " << tid << std::endl;
                }
                break;
            }
        }
    }
}

DBGDie *DebugInfo::GetDie(const MIRFunction *func)
{
    uint32 id = stridxDieIdMap[func->GetNameStrIdx().GetIdx()];
    if (id) {
        return idDieMap[id];
    }
    return nullptr;
}

DBGDie *DebugInfo::GetFuncDie(const MIRFunction &func, bool isDeclDie)
{
    uint32 id = isDeclDie ? stridxDieIdMap[func.GetNameStrIdx().GetIdx()]
                          : funcDefStrIdxDieIdMap[func.GetNameStrIdx().GetIdx()];
    if (id != 0) {
        return idDieMap[id];
    }
    return nullptr;
}

// Methods for calculating Offset and Size of DW_AT_xxx
size_t DBGDieAttr::SizeOf(DBGDieAttr *attr)
{
    DwForm form = attr->dwForm;
    switch (form) {
        // case DW_FORM_implicitconst:
        case DW_FORM_flag_present:
            return 0;  // Not handled yet.
        case DW_FORM_flag:
        case DW_FORM_ref1:
        case DW_FORM_data1:
            return sizeof(int8);
        case DW_FORM_ref2:
        case DW_FORM_data2:
            return sizeof(int16);
        case DW_FORM_ref4:
        case DW_FORM_data4:
            return sizeof(int32);
        case DW_FORM_ref8:
        case DW_FORM_ref_sig8:
        case DW_FORM_data8:
            return sizeof(int64);
        case DW_FORM_addr:
            return sizeof(int64);
        case DW_FORM_sec_offset:
        case DW_FORM_ref_addr:
        case DW_FORM_strp:
        case DW_FORM_GNU_ref_alt:
            // case DW_FORM_codeLinestrp:
            // case DW_FORM_strp_sup:
            // case DW_FORM_ref_sup:
            return k4BitSize;  // DWARF32, 8 if DWARF64

        case DW_FORM_string: {
            GStrIdx stridx(attr->value.id);
            const std::string &str = GlobalTables::GetStrTable().GetStringFromStrIdx(stridx);
            return str.length() + 1 /* terminal null byte */;
        }
        case DW_FORM_exprloc: {
            DBGExprLoc *ptr = attr->value.ptr;
            CHECK_FATAL(ptr != (DBGExprLoc *)(0xdeadbeef), "wrong ptr");
            switch (ptr->GetOp()) {
                case DW_OP_call_frame_cfa:
                    return k2BitSize;  // size 1 byte + DW_OP_call_frame_cfa 1 byte
                case DW_OP_fbreg: {
                    // DW_OP_fbreg 1 byte
                    size_t size = 1 + namemangler::GetSleb128Size(ptr->GetFboffset());
                    return size + namemangler::GetUleb128Size(size);
                }
                case DW_OP_addr: {
                    return namemangler::GetUleb128Size(k9BitSize) + k9BitSize;
                }
                default:
                    return k4BitSize;
            }
        }
        default:
            CHECK_FATAL(maple::GetDwFormName(form) != nullptr,
                        "GetDwFormName return null in DebugInfo::FillTypeAttrWithDieId");
            LogInfo::MapleLogger() << "unhandled SizeOf: " << maple::GetDwFormName(form) << std::endl;
            return 0;
    }
}

void DebugInfo::ComputeSizeAndOffsets()
{
    // CU-relative offset is reset to 0 here.
    uint32 cuOffset = sizeof(int32_t)  // Length of Unit Info
                      + sizeof(int16)  // DWARF version number      : 0x0004
                      + sizeof(int32)  // Offset into Abbrev. Section : 0x0000
                      + sizeof(int8);  // Pointer Size (in bytes)       : 0x08

    // After returning from this function, the length value is the size
    // of the .debug_info section
    ComputeSizeAndOffset(compUnit, cuOffset);
    debugInfoLength = cuOffset - sizeof(int32_t);
}

// Compute the size and offset of a DIE. The Offset is relative to start of the CU.
// It returns the offset after laying out the DIE.
void DebugInfo::ComputeSizeAndOffset(DBGDie *die, uint32 &cuOffset)
{
    uint32 cuOffsetOrg = cuOffset;
    die->SetOffset(cuOffset);

    // Add the byte size of the abbreviation code
    cuOffset += static_cast<uint32>(namemangler::GetUleb128Size(uint64_t(die->GetAbbrevId())));

    // Add the byte size of all the DIE attributes.
    for (const auto &attr : die->GetAttrVec()) {
        cuOffset += static_cast<uint32>(attr->SizeOf(attr));
    }

    die->SetSize(cuOffset - cuOffsetOrg);

    // Let the children compute their offsets.
    if (die->GetWithChildren()) {
        uint32 size = die->GetSubDieVecSize();

        for (uint32 i = 0; i < size; i++) {
            DBGDie *childDie = die->GetSubDieVecAt(i);
            ComputeSizeAndOffset(childDie, cuOffset);
        }

        // Each child chain is terminated with a zero byte, adjust the offset.
        cuOffset += sizeof(int8);
    }
}

/* ///////////////
 * Dumps
 * ///////////////
 */
void DebugInfo::Dump(int indent)
{
    LogInfo::MapleLogger() << "\n" << std::endl;
    LogInfo::MapleLogger() << "maple_debug_information {"
                           << "  Length: " << HEX(debugInfoLength) << std::endl;
    compUnit->Dump(indent + 1);
    LogInfo::MapleLogger() << "}\n" << std::endl;
    LogInfo::MapleLogger() << "maple_debug_abbrev {" << std::endl;
    for (uint32 i = 1; i < abbrevVec.size(); i++) {
        abbrevVec[i]->Dump(indent + 1);
    }
    LogInfo::MapleLogger() << "}" << std::endl;
    return;
}

void DBGExprLoc::Dump()
{
    LogInfo::MapleLogger() << " " << HEX(GetOp());
    for (auto it : simpLoc->GetOpnd()) {
        LogInfo::MapleLogger() << " " << HEX(it);
    }
}

void DBGDieAttr::Dump(int indent)
{
    PrintIndentation(indent);
    CHECK_FATAL(GetDwFormName(dwForm) && GetDwAtName(dwAttr), "null ptr check");
    LogInfo::MapleLogger() << GetDwAtName(dwAttr) << " " << GetDwFormName(dwForm);
    if (dwForm == DW_FORM_string || dwForm == DW_FORM_strp) {
        GStrIdx idx(value.id);
        LogInfo::MapleLogger() << " 0x" << std::hex << value.u << std::dec;
        LogInfo::MapleLogger() << " \"" << GlobalTables::GetStrTable().GetStringFromStrIdx(idx).c_str() << "\"";
    } else if (dwForm == DW_FORM_ref4) {
        LogInfo::MapleLogger() << " <" << HEX(value.id) << ">";
    } else if (dwAttr == DW_AT_encoding) {
        CHECK_FATAL(GetDwAteName(static_cast<uint32>(value.u)), "null ptr check");
        LogInfo::MapleLogger() << " " << GetDwAteName(static_cast<uint32>(value.u));
    } else if (dwAttr == DW_AT_location) {
        value.ptr->Dump();
    } else {
        LogInfo::MapleLogger() << " 0x" << std::hex << value.u << std::dec;
    }
    LogInfo::MapleLogger() << std::endl;
}

void DBGDie::Dump(int indent)
{
    PrintIndentation(indent);
    LogInfo::MapleLogger() << "<" << HEX(id) << "><" << HEX(offset);
    LogInfo::MapleLogger() << "><" << HEX(size) << "><"
                           << "> abbrev id: " << HEX(abbrevId);
    CHECK_FATAL(GetDwTagName(tag), "null ptr check");
    LogInfo::MapleLogger() << " (" << GetDwTagName(tag) << ") ";
    if (parent) {
        LogInfo::MapleLogger() << "parent <" << HEX(parent->GetId());
    }
    LogInfo::MapleLogger() << "> {";
    if (tyIdx) {
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(tyIdx));
        if (type->GetKind() == kTypeStruct || type->GetKind() == kTypeClass || type->GetKind() == kTypeInterface) {
            MIRStructType *stype = static_cast<MIRStructType *>(type);
            LogInfo::MapleLogger() << "           # " << stype->GetName();
        } else {
            LogInfo::MapleLogger() << "           # " << GetPrimTypeName(type->GetPrimType());
        }
    }
    LogInfo::MapleLogger() << std::endl;
    ;
    for (auto it : attrVec) {
        it->Dump(indent + 1);
    }
    PrintIndentation(indent);
    LogInfo::MapleLogger() << "} ";
    if (subDieVec.size()) {
        LogInfo::MapleLogger() << " {" << std::endl;
        for (auto it : subDieVec) {
            it->Dump(indent + 1);
        }
        PrintIndentation(indent);
        LogInfo::MapleLogger() << "}";
    }
    LogInfo::MapleLogger() << std::endl;
    return;
}

void DBGAbbrevEntry::Dump(int indent)
{
    PrintIndentation(indent);
    CHECK_FATAL(GetDwTagName(tag), "null ptr check ");
    LogInfo::MapleLogger() << "<" << HEX(abbrevId) << "> " << GetDwTagName(tag);
    if (GetWithChildren()) {
        LogInfo::MapleLogger() << " [with children] {" << std::endl;
    } else {
        LogInfo::MapleLogger() << " [no children] {" << std::endl;
    }
    for (uint32 i = 0; i < attrPairs.size(); i += k2BitSize) {
        PrintIndentation(indent + 1);
        CHECK_FATAL(GetDwAtName(attrPairs[i]) && GetDwFormName(attrPairs[i + 1]), "NULLPTR CHECK");

        LogInfo::MapleLogger() << " " << GetDwAtName(attrPairs[i]) << " " << GetDwFormName(attrPairs[i + 1]) << " "
                               << std::endl;
    }
    PrintIndentation(indent);
    LogInfo::MapleLogger() << "}" << std::endl;
    return;
}

void DBGAbbrevEntryVec::Dump(int indent)
{
    for (auto it : entryVec) {
        PrintIndentation(indent);
        it->Dump(indent);
    }
    return;
}

// DBGCompileMsgInfo methods
void DBGCompileMsgInfo::ClearLine(uint32 n)
{
    errno_t eNum = memset_s(codeLine[n], MAXLINELEN, 0, MAXLINELEN);
    if (eNum) {
        FATAL(kLncFatal, "memset_s failed");
    }
}

DBGCompileMsgInfo::DBGCompileMsgInfo() : startLine(0), errPos(0)
{
    lineNum[0] = 0;
    lineNum[1] = 0;
    lineNum[kIndx2] = 0;
    ClearLine(0);
    ClearLine(1);
    ClearLine(kIndx2);
    errLNum = 0;
    errCNum = 0;
}

void DBGCompileMsgInfo::SetErrPos(uint32 lnum, uint32 cnum)
{
    errLNum = lnum;
    errCNum = cnum;
}

void DBGCompileMsgInfo::UpdateMsg(uint32 lnum, const char *line)
{
    size_t size = strlen(line);
    if (size > MAXLINELEN - 1) {
        size = MAXLINELEN - 1;
    }
    startLine = (startLine + k2BitSize) % k3BitSize;
    ClearLine(startLine);
    errno_t eNum = memcpy_s(codeLine[startLine], MAXLINELEN, line, size);
    if (eNum) {
        FATAL(kLncFatal, "memcpy_s failed");
    }
    codeLine[startLine][size] = '\0';
    lineNum[startLine] = lnum;
}

void DBGCompileMsgInfo::EmitMsg()
{
    char str[MAXLINELEN + 1];

    errPos = errCNum;
    errPos = (errPos < k2BitSize) ? k2BitSize : errPos;
    errPos = (errPos > MAXLINELEN) ? MAXLINELEN : errPos;
    for (uint32 i = 0; i < errPos - 1; i++) {
        str[i] = ' ';
    }
    str[errPos - 1] = '^';
    str[errPos] = '\0';

    fprintf(stderr, "\n===================================================================\n");
    fprintf(stderr, "==================");
    fprintf(stderr, BOLD YEL "  Compilation Error Diagnosis  " RESET);
    fprintf(stderr, "==================\n");
    fprintf(stderr, "===================================================================\n");
    fprintf(stderr, "line %4u %s\n", lineNum[(startLine + k2BitSize) % k3BitSize],
            reinterpret_cast<char *>(codeLine[(startLine + k2BitSize) % k3BitSize]));
    fprintf(stderr, "line %4u %s\n", lineNum[(startLine + 1) % k3BitSize],
            reinterpret_cast<char *>(codeLine[(startLine + 1) % k3BitSize]));
    fprintf(stderr, "line %4u %s\n", lineNum[(startLine) % k3BitSize],
            reinterpret_cast<char *>(codeLine[(startLine) % k3BitSize]));
    fprintf(stderr, BOLD RED "          %s\n" RESET, str);
    fprintf(stderr, "===================================================================\n");
}
}  // namespace maple
