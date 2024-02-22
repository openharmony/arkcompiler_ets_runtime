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

#include "instrument.h"
#include "cgbb.h"
#include "mir_builder.h"
#include "mpl_logging.h"

namespace maple {
std::string GetProfCntSymbolName(const std::string &funcName)
{
    return funcName + "_counter";
}

static inline void RegisterInFuncInfo(MIRFunction &func, const MIRSymbol &counter, uint64 elemCnt, uint32 cfgHash)
{
    if (elemCnt == 0) {
        return;
    }
    auto funcStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(namemangler::kprefixProfFuncDesc + func.GetName());
    MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(funcStrIdx);
    CHECK_FATAL(sym, "function have not generated, please check in CreateFuncInfo");
    auto *funcInfoMirConst = static_cast<MIRAggConst *>(sym->GetKonst());
    MIRType *u32Ty = GlobalTables::GetTypeTable().GetUInt32();
    MIRIntConst *cfgHashConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(cfgHash, *u32Ty);
    MIRIntConst *eleCntMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(elemCnt, *u32Ty);
    auto *counterConst = func.GetModule()->GetMemPool()->New<MIRAddrofConst>(counter.GetStIdx(), 0,
                                                                             *GlobalTables::GetTypeTable().GetPtr());
    int num1 = 1;
    int num2 = 2;
    int num3 = 3;
    int num4 = 4;
    funcInfoMirConst->SetItem(num1, cfgHashConst, num2);
    funcInfoMirConst->SetItem(num2, eleCntMirConst, num3);
    funcInfoMirConst->SetItem(num3, counterConst, num4);
}

MIRSymbol *GetOrCreateFuncCounter(MIRFunction &func, uint32 elemCnt, uint32 cfgHash)
{
    auto *mirModule = func.GetModule();
    std::string name = GetProfCntSymbolName(func.GetName());
    auto nameStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(name);
    MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(nameStrIdx);
    if (sym != nullptr) {
        return sym;
    }
    auto *elemType = GlobalTables::GetTypeTable().GetUInt64();
    MIRArrayType &arrayType = *GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, elemCnt);
    sym = mirModule->GetMIRBuilder()->CreateGlobalDecl(name.c_str(), arrayType, kScFstatic);
    auto *profTab = mirModule->GetMemPool()->New<MIRAggConst>(*mirModule, arrayType);
    MIRIntConst *indexConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *elemType);
    for (uint32 i = 0; i < elemCnt; ++i) {
        profTab->AddItem(indexConst, i);
    }
    sym->SetKonst(profTab);
    sym->SetStorageClass(kScFstatic);
    sym->sectionAttr = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName("mpl_counter");
    RegisterInFuncInfo(func, *sym, elemCnt, cfgHash);
    return sym;
}

template <class IRBB, class Edge>
void PGOInstrumentTemplate<IRBB, Edge>::GetInstrumentBBs(std::vector<IRBB *> &bbs, IRBB *commonEntry) const
{
    std::vector<Edge *> iEdges;
    mst.GetInstrumentEdges(iEdges);
    std::unordered_set<IRBB *> visitedBBs;
    for (auto &edge : iEdges) {
        IRBB *src = edge->GetSrcBB();
        IRBB *dest = edge->GetDestBB();
        if (src->GetSuccs().size() <= 1) {
            if (src == commonEntry) {
                bbs.push_back(dest);
            } else {
                bbs.push_back(src);
            }
        } else if (!edge->IsCritical()) {
            bbs.push_back(dest);
        } else {
            if (src->GetKind() == IRBB::kBBIgoto) {
                if (visitedBBs.find(dest) == visitedBBs.end()) {
                    // In this case, we have to instrument it anyway
                    bbs.push_back(dest);
                    (void)visitedBBs.insert(dest);
                }
            } else {
                CHECK_FATAL(false, "Unexpected case %d -> %d", src->GetId(), dest->GetId());
            }
        }
    }
}

template <typename BB>
BBUseEdge<BB> *BBUseInfo<BB>::GetOnlyUnknownOutEdges()
{
    BBUseEdge<BB> *ouEdge = nullptr;
    for (auto *e : outEdges) {
        if (!e->GetStatus()) {
            CHECK_FATAL(!ouEdge, "have multiple unknown out edges");
            ouEdge = e;
        }
    }
    return ouEdge;
}

template <typename BB>
BBUseEdge<BB> *BBUseInfo<BB>::GetOnlyUnknownInEdges()
{
    BBUseEdge<BB> *ouEdge = nullptr;
    for (auto *e : inEdges) {
        if (!e->GetStatus()) {
            CHECK_FATAL(!ouEdge, "have multiple unknown in edges");
            ouEdge = e;
        }
    }
    return ouEdge;
}

template <typename BB>
void BBUseInfo<BB>::Dump()
{
    for (const auto &inE : inEdges) {
        if (inE->GetStatus()) {
            LogInfo::MapleLogger() << inE->GetSrcBB()->GetId() << "->" << inE->GetDestBB()->GetId()
                                   << " c : " << inE->GetCount() << "\n";
        }
    }
    for (const auto &outE : outEdges) {
        if (outE->GetStatus()) {
            LogInfo::MapleLogger() << outE->GetSrcBB()->GetId() << "->" << outE->GetDestBB()->GetId()
                                   << " c : " << outE->GetCount() << "\n";
        }
    }
}

template class PGOInstrumentTemplate<maplebe::BB, maple::BBEdge<maplebe::BB>>;
template class PGOInstrumentTemplate<maplebe::BB, maple::BBUseEdge<maplebe::BB>>;
template class BBUseInfo<maplebe::BB>;
} /* namespace maple */
