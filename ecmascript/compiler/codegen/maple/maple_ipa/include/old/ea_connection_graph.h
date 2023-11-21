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

#ifndef MAPLEIPA_INCLUDE_ESCAPEANALYSIS_H
#define MAPLEIPA_INCLUDE_ESCAPEANALYSIS_H
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include "call_graph.h"
#include "me_ir.h"
#include "irmap.h"

namespace maple {
enum NodeKind { kObejectNode, kReferenceNode, kActualNode, kFieldNode, kPointerNode };

enum EAStatus { kNoEscape, kReturnEscape, kArgumentEscape, kGlobalEscape };

const inline std::string EscapeName(EAStatus esc)
{
    switch (esc) {
        case kNoEscape:
            return "NoEsc";
        case kReturnEscape:
            return "RetEsc";
        case kArgumentEscape:
            return "ArgEsc";
        case kGlobalEscape:
            return "GlobalEsc";
        default:
            return "";
    }
}

class Location {
public:
    Location(const std::string &modName, uint32 fileId, uint32 lineId)
        : modName(modName), fileId(fileId), lineId(lineId) {};
    ~Location() = default;

    const std::string &GetModName() const
    {
        return modName;
    }

    uint32 GetFileId() const
    {
        return fileId;
    }

    uint32 GetLineId() const
    {
        return lineId;
    }

private:
    std::string modName;
    uint32 fileId;
    uint32 lineId;
};

class EACGBaseNode;
class EACGObjectNode;
class EACGFieldNode;
class EACGRefNode;
class EACGActualNode;
class EACGPointerNode;

class EAConnectionGraph {
public:
    friend class BinaryMplExport;
    friend class BinaryMplImport;
    friend class EACGBaseNode;
    friend class EACGObjectNode;
    friend class EACGFieldNode;
    friend class EACGRefNode;
    friend class EACGPointerNode;
    // If import is false, need init globalNode.
    EAConnectionGraph(MIRModule *m, MapleAllocator *allocator, const GStrIdx &funcName, bool import = false)
        : mirModule(m),
          alloc(allocator),
          nodes(allocator->Adapter()),
          expr2Nodes(allocator->Adapter()),
          funcArgNodes(allocator->Adapter()),
          callSite2Nodes(allocator->Adapter()),
          funcStIdx(funcName),
          hasUpdated(false),
          needConv(false),
          imported(import),
          exprIdMax(0),
          globalObj(nullptr),
          globalRef(nullptr),
          globalField(nullptr) {};
    ~EAConnectionGraph() = default;

    EACGObjectNode *CreateObjectNode(MeExpr *expr, EAStatus initialEas, bool isPh, TyIdx tyIdx);
    EACGRefNode *CreateReferenceNode(MeExpr *expr, EAStatus initialEas, bool isStatic);
    EACGActualNode *CreateActualNode(EAStatus initialEas, bool isReurtn, bool isPh, uint8 argIdx, uint32 callSiteInfo);
    EACGFieldNode *CreateFieldNode(MeExpr *expr, EAStatus initialEas, FieldID fId, EACGObjectNode *belongTo, bool isPh);
    EACGPointerNode *CreatePointerNode(MeExpr *expr, EAStatus initialEas, int inderictL);
    EACGBaseNode *GetCGNodeFromExpr(MeExpr *me);
    EACGFieldNode *GetOrCreateFieldNodeFromIdx(EACGObjectNode &obj, int32 fieldID);
    EACGActualNode *GetReturnNode() const;
    const MapleVector<EACGBaseNode *> *GetFuncArgNodeVector() const;
    void TouchCallSite(uint32 callSiteInfo);
    MapleVector<EACGBaseNode *> *GetCallSiteArgNodeVector(uint32 callSite);
    bool ExprCanBeOptimized(MeExpr &expr);

    bool CGHasUpdated() const
    {
        return hasUpdated;
    }

    void UnSetCGUpdateFlag()
    {
        hasUpdated = false;
    }

    void SetCGHasUpdated()
    {
        hasUpdated = true;
    }

    void SetExprIdMax(int max)
    {
        exprIdMax = max;
    }

    void SetNeedConservation()
    {
        needConv = true;
    }

    bool GetNeedConservation() const
    {
        return needConv;
    }

    GStrIdx GetFuncNameStrIdx() const
    {
        return funcStIdx;
    }

    EACGObjectNode *GetGlobalObject()
    {
        return globalObj;
    }

    const EACGObjectNode *GetGlobalObject() const
    {
        return globalObj;
    }

    EACGRefNode *GetGlobalReference()
    {
        return globalRef;
    }

    const EACGRefNode *GetGlobalReference() const
    {
        return globalRef;
    }

    const MapleVector<EACGBaseNode *> &GetNodes() const
    {
        return nodes;
    }

    void ResizeNodes(size_t size, EACGBaseNode *val)
    {
        nodes.resize(size, val);
    }

    EACGBaseNode *GetNode(uint32 idx) const
    {
        CHECK_FATAL(idx < nodes.size(), "array check fail");
        return nodes[idx];
    }

    void SetNodeAt(size_t index, EACGBaseNode *val)
    {
        nodes[index] = val;
    }

    const MapleVector<EACGBaseNode *> &GetFuncArgNodes() const
    {
        return funcArgNodes;
    }

    const MapleMap<uint32, MapleVector<EACGBaseNode *> *> &GetCallSite2Nodes() const
    {
        return callSite2Nodes;
    }

    void InitGlobalNode();
    void AddMaps2Object(EACGObjectNode *caller, EACGObjectNode *callee);
    void UpdateExprOfNode(EACGBaseNode &node, MeExpr *me);
    void UpdateExprOfGlobalRef(MeExpr *me);
    void PropogateEAStatus();
    bool MergeCG(MapleVector<EACGBaseNode *> &caller, const MapleVector<EACGBaseNode *> *callee);
    void TrimGlobalNode() const;
    void UpdateEACGFromCaller(const MapleVector<EACGBaseNode *> &callerCallSiteArg,
                              const MapleVector<EACGBaseNode *> &calleeFuncArg);
    void DumpDotFile(const IRMap *irMap, bool dumpPt, MapleVector<EACGBaseNode *> *dumpVec = nullptr);
    void DeleteEACG() const;
    void RestoreStatus(bool old);
    void CountObjEAStatus() const;

    const std::string &GetFunctionName() const
    {
        return GlobalTables::GetStrTable().GetStringFromStrIdx(funcStIdx);
    }

private:
    MIRModule *mirModule;
    MapleAllocator *alloc;
    MapleVector<EACGBaseNode *> nodes;
    MapleMap<MeExpr *, MapleSet<EACGBaseNode *> *> expr2Nodes;
    // this vector contain func arg nodes first in declaration order and the last is return node
    MapleVector<EACGBaseNode *> funcArgNodes;
    MapleMap<uint32, MapleVector<EACGBaseNode *> *> callSite2Nodes;
    GStrIdx funcStIdx;
    bool hasUpdated;
    bool needConv;
    bool imported;
    int exprIdMax;
    EACGObjectNode *globalObj;
    EACGRefNode *globalRef;
    EACGFieldNode *globalField;
    // this is used as a tmp varible for merge cg
    std::map<EACGObjectNode *, std::set<EACGObjectNode *>> callee2Caller;
    void CheckArgNodeOrder(MapleVector<EACGBaseNode *> &funcArgV);
    void UpdateCallerNodes(const MapleVector<EACGBaseNode *> &caller, const MapleVector<EACGBaseNode *> &callee);
    void UpdateCallerRetNode(MapleVector<EACGBaseNode *> &caller, const MapleVector<EACGBaseNode *> &callee);
    void UpdateCallerEdges();
    void UpdateCallerEdgesInternal(EACGObjectNode *node1, int32 fieldID, EACGObjectNode *node2);
    void UpdateNodes(const EACGBaseNode &actualInCallee, EACGBaseNode &actualInCaller, bool firstTime);
    void UpdateCallerWithCallee(EACGObjectNode &objInCaller, const EACGObjectNode &objInCallee, bool firstTime);

    void SetCGUpdateFlag()
    {
        hasUpdated = true;
    }
};

class EACGBaseNode {
public:
    friend class BinaryMplExport;
    friend class BinaryMplImport;
    friend class EACGObjectNode;
    friend class EACGFieldNode;
    friend class EACGActualNode;
    friend class EACGRefNode;
    friend class EACGPointerNode;
    friend class EAConnectionGraph;

    EACGBaseNode(MIRModule *m, MapleAllocator *a, NodeKind nk, EAConnectionGraph *ec)
        : locInfo(nullptr), mirModule(m), alloc(a), kind(nk), meExpr(nullptr), eaStatus(kNoEscape), id(0), eaCG(ec)
    {
    }

    EACGBaseNode(MIRModule *m, MapleAllocator *a, NodeKind nk, EAConnectionGraph &ec, MeExpr *expr, EAStatus initialEas,
                 int i)
        : locInfo(nullptr), mirModule(m), alloc(a), kind(nk), meExpr(expr), eaStatus(initialEas), id(i), eaCG(&ec)
    {
        ec.SetCGUpdateFlag();
    }

    virtual ~EACGBaseNode() = default;

    virtual bool IsFieldNode() const
    {
        return kind == kFieldNode;
    }

    virtual bool IsObjectNode() const
    {
        return kind == kObejectNode;
    }

    virtual bool IsReferenceNode() const
    {
        return kind == kReferenceNode;
    }

    virtual bool IsActualNode() const
    {
        return kind == kActualNode;
    }

    virtual bool IsPointerNode() const
    {
        return kind == kPointerNode;
    }

    virtual const MeExpr *GetMeExpr() const
    {
        return meExpr;
    }

    virtual void SetMeExpr(MeExpr &newExpr)
    {
        if (IsFieldNode() && newExpr.GetMeOp() != kMeOpIvar && newExpr.GetMeOp() != kMeOpOp) {
            CHECK_FATAL(false, "must be kMeOpIvar or kMeOpOp");
        } else if (IsReferenceNode() == true && newExpr.GetMeOp() != kMeOpVar && newExpr.GetMeOp() != kMeOpReg &&
                   newExpr.GetMeOp() != kMeOpAddrof && newExpr.GetMeOp() != kMeOpConststr) {
            CHECK_FATAL(false, "must be kMeOpVar, kMeOpReg, kMeOpAddrof or kMeOpConststr");
        }
        meExpr = &newExpr;
    }

    const std::set<EACGObjectNode *> &GetPointsToSet() const
    {
        CHECK_FATAL(!IsPointerNode(), "must be pointer node");
        return pointsTo;
    };

    virtual bool AddOutNode(EACGBaseNode &newOut);

    virtual EAStatus GetEAStatus() const
    {
        return eaStatus;
    }

    virtual const std::set<EACGBaseNode *> &GetInSet() const
    {
        return in;
    }

    virtual void InsertInSet(EACGBaseNode *val)
    {
        (void)in.insert(val);
    }

    virtual const std::set<EACGBaseNode *> &GetOutSet() const
    {
        CHECK_FATAL(IsActualNode(), "must be actual node");
        return out;
    }

    virtual void InsertOutSet(EACGBaseNode *val)
    {
        (void)out.insert(val);
    }

    virtual bool UpdateEAStatus(EAStatus newEas)
    {
        if (newEas > eaStatus) {
            eaStatus = newEas;
            PropagateEAStatusForNode(this);
            eaCG->SetCGUpdateFlag();
            return true;
        }
        return false;
    }

    bool IsBelongTo(const EAConnectionGraph *cg) const
    {
        return this->eaCG == cg;
    }

    const EAConnectionGraph *GetEACG() const
    {
        return eaCG;
    }

    EAConnectionGraph *GetEACG()
    {
        return eaCG;
    }

    void SetEACG(EAConnectionGraph *cg)
    {
        this->eaCG = cg;
    }

    void SetID(int setId)
    {
        this->id = static_cast<size_t>(setId);
    }

    bool CanIgnoreRC() const;

protected:
    Location *locInfo;
    MIRModule *mirModule;
    MapleAllocator *alloc;
    NodeKind kind;
    MeExpr *meExpr;
    EAStatus eaStatus;
    size_t id;
    // OBJ<->Field will not in following Set
    std::set<EACGBaseNode *> in;
    std::set<EACGBaseNode *> out;
    std::set<EACGObjectNode *> pointsTo;
    EAConnectionGraph *eaCG;

    virtual void CheckAllConnectionInNodes();
    virtual std::string GetName(const IRMap *irMap) const;
    virtual void DumpDotFile(std::ostream &, std::map<EACGBaseNode *, bool> &, bool, const IRMap *irMap = nullptr) = 0;
    virtual void PropagateEAStatusForNode(const EACGBaseNode *subRoot) const;
    virtual void GetNodeFormatInDot(std::string &label, std::string &color) const;
    virtual bool UpdatePointsTo(const std::set<EACGObjectNode *> &cPointsTo);

    virtual void SetEAStatus(EAStatus status)
    {
        this->eaStatus = status;
    }

    virtual NodeKind GetNodeKind() const
    {
        return kind;
    }

private:
    virtual bool ReplaceByGlobalNode()
    {
        CHECK_FATAL(false, "impossible");
        return false;
    }
};

class EACGPointerNode : public EACGBaseNode {
public:
    friend class BinaryMplExport;
    friend class BinaryMplImport;
    EACGPointerNode(MIRModule *md, MapleAllocator *alloc, EAConnectionGraph *ec)
        : EACGBaseNode(md, alloc, kPointerNode, ec), indirectLevel(0)
    {
    }

    EACGPointerNode(MIRModule *md, MapleAllocator *alloc, EAConnectionGraph &ec, MeExpr *expr, EAStatus initialEas,
                    int i, int indirectL)
        : EACGBaseNode(md, alloc, kPointerNode, ec, expr, initialEas, i), indirectLevel(indirectL) {};
    ~EACGPointerNode() = default;

    void SetLocation(Location *loc)
    {
        this->locInfo = loc;
    }

    int GetIndirectLevel() const
    {
        return indirectLevel;
    }

    bool AddOutNode(EACGBaseNode &newOut) override
    {
        if (indirectLevel == 1) {
            CHECK_FATAL(!newOut.IsPointerNode(), "must be pointer node");
            (void)pointingTo.insert(&newOut);
            (void)out.insert(&newOut);
            (void)newOut.in.insert(this);
        } else {
            pointingTo.insert(&newOut);
            CHECK_FATAL(pointingTo.size() == 1, "the size must be one");
            CHECK_FATAL(newOut.IsPointerNode(), "must be pointer node");
            CHECK_FATAL((indirectLevel - static_cast<EACGPointerNode &>(newOut).GetIndirectLevel()) == 1,
                        "must be one");
            (void)out.insert(&newOut);
            (void)newOut.in.insert(this);
        }
        return false;
    }

    const std::set<EACGBaseNode *> &GetPointingTo() const
    {
        return pointingTo;
    }

    bool UpdatePointsTo(const std::set<EACGObjectNode *> &) override
    {
        CHECK_FATAL(false, "impossible to update PointsTo");
        return true;
    };

    void PropagateEAStatusForNode(const EACGBaseNode *) const override
    {
        CHECK_FATAL(false, "impossible to propagate EA status for node");
    }

    void DumpDotFile(std::ostream &fout, std::map<EACGBaseNode *, bool> &dumped, bool dumpPt,
                     const IRMap *irMap = nullptr) override;
    void CheckAllConnectionInNodes() override {}

private:
    int indirectLevel;
    std::set<EACGBaseNode *> pointingTo;
    bool ReplaceByGlobalNode() override
    {
        CHECK_FATAL(false, "impossible to replace by global node");
        return true;
    }
};

class EACGObjectNode : public EACGBaseNode {
public:
    friend class EACGFieldNode;
    friend class BinaryMplExport;
    friend class BinaryMplImport;
    EACGObjectNode(MIRModule *md, MapleAllocator *alloc, EAConnectionGraph *ec)
        : EACGBaseNode(md, alloc, kObejectNode, ec), rcOperations(0), ignorRC(false), isPhantom(false)
    {
    }

    EACGObjectNode(MIRModule *md, MapleAllocator *alloc, EAConnectionGraph &ec, MeExpr *expr, EAStatus initialEas,
                   int i, bool isPh)
        : EACGBaseNode(md, alloc, kObejectNode, ec, expr, initialEas, i),
          rcOperations(0),
          ignorRC(false),
          isPhantom(isPh)
    {
        (void)pointsBy.insert(this);
        (void)pointsTo.insert(this);
    };
    ~EACGObjectNode() = default;
    bool IsPhantom() const
    {
        return isPhantom == true;
    };

    void SetLocation(Location *loc)
    {
        this->locInfo = loc;
    }

    const std::map<FieldID, EACGFieldNode *> &GetFieldNodeMap() const
    {
        return fieldNodes;
    }

    EACGFieldNode *GetFieldNodeFromIdx(FieldID fId)
    {
        if (fieldNodes.find(-1) != fieldNodes.end()) {  // -1 expresses global
            return fieldNodes[-1];
        }
        if (fieldNodes.find(fId) == fieldNodes.end()) {
            return nullptr;
        }
        return fieldNodes[fId];
    }

    bool AddOutNode(EACGBaseNode &newOut) override;
    bool UpdatePointsTo(const std::set<EACGObjectNode *> &) override
    {
        CHECK_FATAL(false, "impossible");
        return true;
    };

    bool IsPointedByFieldNode() const;
    void PropagateEAStatusForNode(const EACGBaseNode *subRoot) const override;
    void DumpDotFile(std::ostream &fout, std::map<EACGBaseNode *, bool> &dumped, bool dumpPt,
                     const IRMap *irMap = nullptr) override;
    void CheckAllConnectionInNodes() override;

    void Insert2PointsBy(EACGBaseNode *node)
    {
        (void)pointsBy.insert(node);
    }

    void EraseNodeFromPointsBy(EACGBaseNode *node)
    {
        pointsBy.erase(node);
    }

    void IncresRCOperations()
    {
        ++rcOperations;
    }

    void IncresRCOperations(int num)
    {
        rcOperations += num;
    }

    int GetRCOperations() const
    {
        return rcOperations;
    }

    bool GetIgnorRC() const
    {
        return ignorRC;
    }

    void SetIgnorRC(bool ignore)
    {
        ignorRC = ignore;
    }

private:
    std::set<EACGBaseNode *> pointsBy;
    int rcOperations;
    bool ignorRC;
    bool isPhantom;
    std::map<FieldID, EACGFieldNode *> fieldNodes;
    bool ReplaceByGlobalNode() override;
};

class EACGRefNode : public EACGBaseNode {
public:
    friend class BinaryMplExport;
    friend class BinaryMplImport;
    EACGRefNode(MIRModule *md, MapleAllocator *alloc, EAConnectionGraph *ec)
        : EACGBaseNode(md, alloc, kReferenceNode, ec), isStaticField(false), sym(nullptr), version(0)
    {
    }

    EACGRefNode(MIRModule *md, MapleAllocator *alloc, EAConnectionGraph &ec, MeExpr *expr, EAStatus initialEas, int i,
                bool isS = false)
        : EACGBaseNode(md, alloc, kReferenceNode, ec, expr, initialEas, i),
          isStaticField(isS),
          sym(nullptr),
          version(0) {};
    ~EACGRefNode() = default;
    bool IsStaticRef() const
    {
        return isStaticField;
    };
    void SetSymbolAndVersion(MIRSymbol *mirSym, int versionIdx)
    {
        if (sym != nullptr) {
            CHECK_FATAL(sym == mirSym, "must be sym");
            CHECK_FATAL(versionIdx == version, "must be version ");
        }
        sym = mirSym;
        version = versionIdx;
    };

    void DumpDotFile(std::ostream &fout, std::map<EACGBaseNode *, bool> &dumped, bool dumpPt,
                     const IRMap *irMap = nullptr);

private:
    bool isStaticField;
    MIRSymbol *sym;
    int version;
    bool ReplaceByGlobalNode();
};
class EACGFieldNode : public EACGBaseNode {
public:
    friend class BinaryMplExport;
    friend class BinaryMplImport;
    friend class EACGObjectNode;
    EACGFieldNode(MIRModule *md, MapleAllocator *alloc, EAConnectionGraph *ec)
        : EACGBaseNode(md, alloc, kFieldNode, ec), fieldID(0), isPhantom(false), sym(nullptr), version(0), mirFieldId(0)
    {
    }

    EACGFieldNode(MIRModule *md, MapleAllocator *alloc, EAConnectionGraph &ec, MeExpr *expr, EAStatus initialEas, int i,
                  FieldID fId, EACGObjectNode *bt, bool isPh)
        : EACGBaseNode(md, alloc, kFieldNode, ec, expr, initialEas, i),
          fieldID(fId),
          isPhantom(isPh),
          sym(nullptr),
          version(0),
          mirFieldId(0)
    {
        bt->fieldNodes[fieldID] = this;
        (void)belongsTo.insert(bt);
    };

    ~EACGFieldNode() = default;

    FieldID GetFieldID() const
    {
        return fieldID;
    };

    void SetFieldID(FieldID id)
    {
        fieldID = id;
    }

    bool IsPhantom() const
    {
        return isPhantom;
    }

    const std::set<EACGObjectNode *> &GetBelongsToObj() const
    {
        return belongsTo;
    }

    void AddBelongTo(EACGObjectNode *newObj)
    {
        (void)belongsTo.insert(newObj);
    }

    void SetSymbolAndVersion(MIRSymbol *mirSym, int versionIdx, FieldID fID)
    {
        if (sym != nullptr) {
            CHECK_FATAL(sym == mirSym, "must be mirSym");
            CHECK_FATAL(version == versionIdx, "must be version");
            CHECK_FATAL(mirFieldId == fID, "must be  mir FieldId");
        }
        sym = mirSym;
        version = versionIdx;
        mirFieldId = fID;
    };

    void DumpDotFile(std::ostream &fout, std::map<EACGBaseNode *, bool> &dumped, bool dumpPt,
                     const IRMap *irMap = nullptr);

private:
    FieldID fieldID;
    std::set<EACGObjectNode *> belongsTo;
    bool isPhantom;
    MIRSymbol *sym;
    int version;
    FieldID mirFieldId;
    bool ReplaceByGlobalNode();
};

class EACGActualNode : public EACGBaseNode {
public:
    friend class BinaryMplExport;
    friend class BinaryMplImport;
    EACGActualNode(MIRModule *md, MapleAllocator *alloc, EAConnectionGraph *ec)
        : EACGBaseNode(md, alloc, kActualNode, ec), isReturn(false), isPhantom(false), argIdx(0), callSiteInfo(0) {};
    EACGActualNode(MIRModule *md, MapleAllocator *alloc, EAConnectionGraph &ec, MeExpr *expr, EAStatus initialEas,
                   int i, bool isR, bool isPh, uint8 aI, uint32 callSite)
        : EACGBaseNode(md, alloc, kActualNode, ec, expr, initialEas, i),
          isReturn(isR),
          isPhantom(isPh),
          argIdx(aI),
          callSiteInfo(callSite) {};
    ~EACGActualNode() = default;

    bool IsReturn() const
    {
        return isReturn;
    };

    bool IsPhantom() const
    {
        return isPhantom;
    };

    uint32 GetArgIndex() const
    {
        return argIdx;
    };

    uint32 GetCallSite() const
    {
        return callSiteInfo;
    }

    void DumpDotFile(std::ostream &fout, std::map<EACGBaseNode *, bool> &dumped, bool dumpPt,
                     const IRMap *irMap = nullptr);

private:
    bool isReturn;
    bool isPhantom;
    uint8 argIdx;
    uint32 callSiteInfo;
    bool ReplaceByGlobalNode();
};
}  // namespace maple
#endif
