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

#ifndef MAPLE_ME_INCLUDE_IRMAP_H
#define MAPLE_ME_INCLUDE_IRMAP_H
#include "bb.h"
#include "ver_symbol.h"
#include "ssa_tab.h"
#include "me_ir.h"
#include "meexpr_use_info.h"

namespace maple {
class IRMapBuild;  // circular dependency exists, no other choice

class IRMap : public AnalysisResult {
    friend IRMapBuild;

public:
    IRMap(SSATab &ssaTab, MemPool &memPool, uint32 hashTableSize)
        : AnalysisResult(&memPool),
          ssaTab(ssaTab),
          mirModule(ssaTab.GetModule()),
          irMapAlloc(&memPool),
          mapHashLength(hashTableSize),
          hashTable(mapHashLength, nullptr, irMapAlloc.Adapter()),
          verst2MeExprTable(ssaTab.GetVersionStTableSize(), nullptr, irMapAlloc.Adapter()),
          lpreTmps(irMapAlloc.Adapter()),
          vst2Decrefs(irMapAlloc.Adapter()),
          exprUseInfo(&memPool)
    {
    }

    virtual ~IRMap() = default;
    virtual BB *GetBB(BBId id) = 0;
    virtual BB *GetBBForLabIdx(LabelIdx lidx, PUIdx pidx = 0) = 0;
    MeExpr *HashMeExpr(MeExpr &meExpr);
    IvarMeExpr *BuildLHSIvarFromIassMeStmt(IassignMeStmt &iassignMeStmt);
    IvarMeExpr *BuildLHSIvar(MeExpr &baseAddr, PrimType primType, const TyIdx &tyIdx, FieldID fieldID);
    IvarMeExpr *BuildLHSIvar(MeExpr &baseAddr, IassignMeStmt &iassignMeStmt, FieldID fieldID);
    MeExpr *CreateAddrofMeExpr(MeExpr &);
    MeExpr *CreateAddroffuncMeExpr(PUIdx);
    MeExpr *CreateAddrofMeExprFromSymbol(MIRSymbol &sym, PUIdx puIdx);
    MeExpr *CreateIaddrofMeExpr(FieldID fieldId, TyIdx tyIdx, MeExpr *base);
    MeExpr *CreateIvarMeExpr(MeExpr &expr, TyIdx tyIdx, MeExpr &base);

    // for creating VarMeExpr
    VarMeExpr *CreateVarMeExprVersion(OriginalSt *ost);
    VarMeExpr *CreateVarMeExprVersion(const VarMeExpr &varx)
    {
        return CreateVarMeExprVersion(varx.GetOst());
    }
    RegMeExpr *CreateRegRefMeExpr(const MeExpr &meExpr);
    VarMeExpr *CreateNewVar(GStrIdx strIdx, PrimType primType, bool isGlobal);
    VarMeExpr *CreateNewLocalRefVarTmp(GStrIdx strIdx, TyIdx tIdx);

    // for creating RegMeExpr
    RegMeExpr *CreateRegMeExprVersion(OriginalSt &);
    RegMeExpr *CreateRegMeExprVersion(const RegMeExpr &regx)
    {
        return CreateRegMeExprVersion(*regx.GetOst());
    }

    ScalarMeExpr *CreateRegOrVarMeExprVersion(OStIdx ostIdx);
    RegMeExpr *CreateRegMeExpr(PrimType);
    RegMeExpr *CreateRegMeExpr(MIRType &);
    RegMeExpr *CreateRegMeExpr(const MeExpr &meexpr)
    {
        MIRType *mirType = meexpr.GetType();
        if (mirType == nullptr || mirType->GetPrimType() == PTY_agg) {
            return CreateRegMeExpr(meexpr.GetPrimType());
        }
        if (meexpr.GetMeOp() == kMeOpIvar && mirType->GetPrimType() != PTY_ref) {
            return CreateRegMeExpr(meexpr.GetPrimType());
        }
        return CreateRegMeExpr(*mirType);
    }

    MeExpr *ReplaceMeExprExpr(MeExpr &, const MeExpr &, MeExpr &);
    MeExpr *GetMeExprByVerID(uint32 verid) const
    {
        return verst2MeExprTable[verid];
    }

    MeExpr *GetMeExpr(size_t index)
    {
        DEBUG_ASSERT(index < verst2MeExprTable.size(), "index out of range");
        MeExpr *meExpr = verst2MeExprTable.at(index);
        if (meExpr == nullptr || !meExpr->IsScalar()) {
            return nullptr;
        }
        return meExpr;
    }

    IassignMeStmt *CreateIassignMeStmt(TyIdx, IvarMeExpr &, MeExpr &, const MapleMap<OStIdx, ChiMeNode *> &);
    AssignMeStmt *CreateAssignMeStmt(ScalarMeExpr &, MeExpr &, BB &);
    void InsertMeStmtBefore(BB &, MeStmt &, MeStmt &);
    MePhiNode *CreateMePhi(ScalarMeExpr &);

    void DumpBB(const BB &bb)
    {
        int i = 0;
        for (const auto &meStmt : bb.GetMeStmts()) {
            if (GetDumpStmtNum()) {
                LogInfo::MapleLogger() << "(" << i++ << ") ";
            }
            meStmt.Dump(this);
        }
    }

    virtual void Dump() = 0;
    virtual void SetCurFunction(const BB &) {}

    MeExpr *CreateIntConstMeExpr(const IntVal &value, PrimType pType);
    MeExpr *CreateIntConstMeExpr(int64, PrimType);
    MeExpr *CreateConstMeExpr(PrimType, MIRConst &);
    MeExpr *CreateMeExprUnary(Opcode, PrimType, MeExpr &);
    MeExpr *CreateMeExprBinary(Opcode, PrimType, MeExpr &, MeExpr &);
    MeExpr *CreateMeExprCompare(Opcode, PrimType, PrimType, MeExpr &, MeExpr &);
    MeExpr *CreateMeExprSelect(PrimType, MeExpr &, MeExpr &, MeExpr &);
    MeExpr *CreateMeExprTypeCvt(PrimType, PrimType, MeExpr &);
    MeExpr *CreateMeExprRetype(PrimType, TyIdx, MeExpr &);
    MeExpr *CreateMeExprExt(Opcode, PrimType, uint32, MeExpr &);
    UnaryMeStmt *CreateUnaryMeStmt(Opcode op, MeExpr *opnd);
    UnaryMeStmt *CreateUnaryMeStmt(Opcode op, MeExpr *opnd, BB *bb, const SrcPosition *src);
    GotoMeStmt *CreateGotoMeStmt(uint32 offset, BB *bb, const SrcPosition *src = nullptr);
    IntrinsiccallMeStmt *CreateIntrinsicCallMeStmt(MIRIntrinsicID idx, std::vector<MeExpr *> &opnds,
                                                   TyIdx tyIdx = TyIdx());
    IntrinsiccallMeStmt *CreateIntrinsicCallAssignedMeStmt(MIRIntrinsicID idx, std::vector<MeExpr *> &opnds,
                                                           ScalarMeExpr *ret, TyIdx tyIdx = TyIdx());
    MeExpr *CreateCanonicalizedMeExpr(PrimType primType, Opcode opA, Opcode opB, MeExpr *opndA, MeExpr *opndB,
                                      MeExpr *opndC);
    MeExpr *CreateCanonicalizedMeExpr(PrimType primType, Opcode opA, MeExpr *opndA, Opcode opB, MeExpr *opndB,
                                      MeExpr *opndC);
    MeExpr *CreateCanonicalizedMeExpr(PrimType primType, Opcode opA, Opcode opB, MeExpr *opndA, MeExpr *opndB,
                                      Opcode opC, MeExpr *opndC, MeExpr *opndD);
    MeExpr *FoldConstExpr(PrimType primType, Opcode op, ConstMeExpr *opndA, ConstMeExpr *opndB);
    MeExpr *SimplifyBandExpr(const OpMeExpr *bandExpr);
    MeExpr *SimplifyLshrExpr(const OpMeExpr *shrExpr);
    MeExpr *SimplifySubExpr(const OpMeExpr *subExpr);
    MeExpr *SimplifyAddExpr(const OpMeExpr *addExpr);
    MeExpr *SimplifyMulExpr(const OpMeExpr *mulExpr);
    MeExpr *SimplifyCmpExpr(OpMeExpr *cmpExpr);
    MeExpr *SimplifySelExpr(OpMeExpr *selExpr);
    MeExpr *SimplifyOpMeExpr(OpMeExpr *opmeexpr);
    MeExpr *SimplifyOrMeExpr(OpMeExpr *opmeexpr);
    MeExpr *SimplifyAshrMeExpr(OpMeExpr *opmeexpr);
    MeExpr *SimplifyMeExpr(MeExpr *x);
    void SimplifyCastForAssign(MeStmt *assignStmt);
    void SimplifyAssign(AssignMeStmt *assignStmt);
    MeExpr *SimplifyCast(MeExpr *expr);
    MeExpr *SimplifyIvarWithConstOffset(IvarMeExpr *ivar, bool lhsIvar);
    MeExpr *SimplifyIvarWithAddrofBase(IvarMeExpr *ivar);
    MeExpr *SimplifyIvarWithIaddrofBase(IvarMeExpr *ivar, bool lhsIvar);
    MeExpr *SimplifyIvar(IvarMeExpr *ivar, bool lhsIvar);
    void UpdateIncDecAttr(MeStmt &meStmt);

    template <class T, typename... Arguments>
    T *NewInPool(Arguments &&... args)
    {
        return irMapAlloc.GetMemPool()->New<T>(&irMapAlloc, std::forward<Arguments>(args)...);
    }

    template <class T, typename... Arguments>
    T *New(Arguments &&... args)
    {
        return irMapAlloc.GetMemPool()->New<T>(std::forward<Arguments>(args)...);
    }

    SSATab &GetSSATab()
    {
        return ssaTab;
    }

    const SSATab &GetSSATab() const
    {
        return ssaTab;
    }

    MIRModule &GetMIRModule()
    {
        return mirModule;
    }

    const MIRModule &GetMIRModule() const
    {
        return mirModule;
    }

    MapleAllocator &GetIRMapAlloc()
    {
        return irMapAlloc;
    }

    int32 GetExprID() const
    {
        return exprID;
    }

    void SetExprID(int32 id)
    {
        exprID = id;
    }

    MapleVector<MeExpr *> &GetVerst2MeExprTable()
    {
        return verst2MeExprTable;
    }

    MeExpr *GetVerst2MeExprTableItem(uint32 i)
    {
        if (i >= verst2MeExprTable.size()) {
            return nullptr;
        }
        return verst2MeExprTable[i];
    }

    void SetVerst2MeExprTableItem(uint32 i, MeExpr *expr)
    {
        verst2MeExprTable[i] = expr;
    }

    MapleUnorderedMap<OStIdx, RegMeExpr *>::iterator GetLpreTmpsEnd()
    {
        return lpreTmps.end();
    }

    MapleUnorderedMap<OStIdx, RegMeExpr *>::iterator FindLpreTmpsItem(OStIdx idx)
    {
        return lpreTmps.find(idx);
    }

    void SetLpreTmps(OStIdx idx, RegMeExpr &expr)
    {
        lpreTmps[idx] = &expr;
    }

    MapleUnorderedMap<VarMeExpr *, MapleSet<MeStmt *> *> &GetVerst2DecrefsMap()
    {
        return vst2Decrefs;
    }

    MapleUnorderedMap<VarMeExpr *, MapleSet<MeStmt *> *>::iterator GetDecrefsEnd()
    {
        return vst2Decrefs.end();
    }

    MapleUnorderedMap<VarMeExpr *, MapleSet<MeStmt *> *>::iterator FindDecrefItem(VarMeExpr &var)
    {
        return vst2Decrefs.find(&var);
    }

    void SetDecrefs(VarMeExpr &var, MapleSet<MeStmt *> &set)
    {
        vst2Decrefs[&var] = &set;
    }

    void SetNeedAnotherPass(bool need)
    {
        needAnotherPass = need;
    }

    bool GetNeedAnotherPass() const
    {
        return needAnotherPass;
    }

    bool GetDumpStmtNum() const
    {
        return dumpStmtNum;
    }

    void SetDumpStmtNum(bool num)
    {
        dumpStmtNum = num;
    }
    const MeExprUseInfo &GetExprUseInfo() const
    {
        return exprUseInfo;
    }

    MeExprUseInfo &GetExprUseInfo()
    {
        return exprUseInfo;
    }

private:
    SSATab &ssaTab;
    MIRModule &mirModule;
    MapleAllocator irMapAlloc;
    int32 exprID = 0;                                                  // for allocating exprid_ in MeExpr
    uint32 mapHashLength;                                              // size of hashTable
    MapleVector<MeExpr *> hashTable;                                   // the value number hash table
    MapleVector<MeExpr *> verst2MeExprTable;                           // map versionst to MeExpr.
    MapleUnorderedMap<OStIdx, RegMeExpr *> lpreTmps;                   // for passing LPRE's temp usage to SPRE
    MapleUnorderedMap<VarMeExpr *, MapleSet<MeStmt *> *> vst2Decrefs;  // map versionst to decrefreset.
    MeExprUseInfo exprUseInfo;
    bool needAnotherPass = false;  // set to true if CFG has changed
    bool dumpStmtNum = false;
    BB *curBB = nullptr;  // current maple_me::BB being visited

    bool ReplaceMeExprStmtOpnd(uint32, MeStmt &, const MeExpr &, MeExpr &);
    void PutToBucket(uint32, MeExpr &);
    const BB *GetFalseBrBB(const CondGotoMeStmt &);
    MeExpr *ReplaceMeExprExpr(MeExpr &origExpr, MeExpr &newExpr, size_t opndsSize, const MeExpr &meExpr,
                              MeExpr &repExpr);
    MeExpr *SimplifyCompareSameExpr(OpMeExpr *opmeexpr);
    bool IfMeExprIsU1Type(const MeExpr *expr) const;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_IRMAP_H
