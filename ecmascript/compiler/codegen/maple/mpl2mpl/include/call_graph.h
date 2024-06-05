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

#ifndef MPL2MPL_INCLUDE_CALL_GRAPH_H
#define MPL2MPL_INCLUDE_CALL_GRAPH_H
#include "class_hierarchy_phase.h"
#include "mir_builder.h"
#include "mir_nodes.h"
#include "scc.h"
#include "base_graph_node.h"
namespace maple {
enum CallType {
    kCallTypeInvalid,
    kCallTypeCall,
    kCallTypeVirtualCall,
    kCallTypeSuperCall,
    kCallTypeInterfaceCall,
    kCallTypeIcall,
    kCallTypeIntrinsicCall,
    kCallTypeXinitrinsicCall,
    kCallTypeIntrinsicCallWithType,
    kCallTypeCustomCall,
    kCallTypePolymorphicCall,
    kCallTypeFakeThreadStartRun
};

struct NodeComparator {
    bool operator()(const MIRFunction *lhs, const MIRFunction *rhs) const
    {
        return lhs->GetPuidx() < rhs->GetPuidx();
    }
};

template <typename T>
struct Comparator {
    bool operator()(const T *lhs, const T *rhs) const
    {
        return lhs->GetID() < rhs->GetID();
    }
};

// Information description of each callsite
class CallInfo {
public:
    CallInfo(CallType type, MIRFunction &call, StmtNode *node, uint32 ld, uint32 stmtId, bool local = false)
        : areAllArgsLocal(local), cType(type), mirFunc(&call), callStmt(node), loopDepth(ld), id(stmtId)
    {
    }
    // For fake callInfo, only id needed
    explicit CallInfo(uint32 stmtId) : id(stmtId) {}
    ~CallInfo() = default;

    uint32 GetID() const
    {
        return id;
    }

    const std::string GetCalleeName() const;
    CallType GetCallType() const
    {
        return cType;
    }

    uint32 GetLoopDepth() const
    {
        return loopDepth;
    }

    const std::string GetCallTypeName() const;
    const StmtNode *GetCallStmt() const
    {
        return callStmt;
    }
    StmtNode *GetCallStmt()
    {
        return callStmt;
    }

    void SetCallStmt(StmtNode *value)
    {
        callStmt = value;
    }

    MIRFunction *GetFunc()
    {
        return mirFunc;
    }

    bool AreAllArgsLocal() const
    {
        return areAllArgsLocal;
    }

    void SetAllArgsLocal()
    {
        areAllArgsLocal = true;
    }

private:
    bool areAllArgsLocal;
    CallType cType;        // Call type
    MIRFunction *mirFunc;  // Used to get signature
    StmtNode *callStmt;    // Call statement
    uint32 loopDepth;
    uint32 id;
};

// Node in callgraph
class CGNode : public BaseGraphNode {
public:
    void AddNumRefs()
    {
        ++numReferences;
    }

    void DecreaseNumRefs()
    {
        --numReferences;
    }

    uint32 NumReferences() const
    {
        return numReferences;
    }

    CGNode(MIRFunction *func, MapleAllocator &allocater, uint32 index)
        : alloc(&allocater),
          id(index),
          sccNode(nullptr),
          mirFunc(func),
          callees(alloc->Adapter()),
          vcallCandidates(alloc->Adapter()),
          isVcallCandidatesValid(false),
          icallCandidates(alloc->Adapter()),
          isIcallCandidatesValid(false),
          numReferences(0),
          callers(alloc->Adapter()),
          stmtCount(0),
          nodeCount(0),
          mustNotBeInlined(false),
          vcallCands(alloc->Adapter())
    {
    }

    ~CGNode() = default;

    void Dump(std::ofstream &fout) const;
    void DumpDetail() const;

    MIRFunction *GetMIRFunction() const
    {
        return mirFunc;
    }

    void AddCallsite(CallInfo &, CGNode *);
    void AddCallsite(CallInfo *, MapleSet<CGNode *, Comparator<CGNode>> *);
    void RemoveCallsite(const CallInfo *, CGNode *);

    uint32 GetID() const override
    {
        return id;
    }

    SCCNode<CGNode> *GetSCCNode()
    {
        return sccNode;
    }

    void SetSCCNode(SCCNode<CGNode> *node)
    {
        sccNode = node;
    }

    PUIdx GetPuIdx() const
    {
        return (mirFunc != nullptr) ? static_cast<int32>(mirFunc->GetPuidx()) : PUIdx(UINT32_MAX);  // invalid idx
    }

    const std::string &GetMIRFuncName() const
    {
        return (mirFunc != nullptr) ? mirFunc->GetName() : GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(0));
    }

    void AddCandsForCallNode(const KlassHierarchy &kh);
    void AddVCallCandidate(MIRFunction *func)
    {
        vcallCands.push_back(func);
    }

    bool HasSetVCallCandidates() const
    {
        return !vcallCands.empty();
    }

    MIRFunction *HasOneCandidate() const;
    const MapleVector<MIRFunction *> &GetVCallCandidates() const
    {
        return vcallCands;
    }

    // add caller to CGNode
    void AddCaller(CGNode *caller, StmtNode *stmt)
    {
        if (callers.find(caller) == callers.end()) {
            auto *callStmts = alloc->New<MapleSet<StmtNode *>>(alloc->Adapter());
            callers.emplace(caller, callStmts);
        }
        callers[caller]->emplace(stmt);
    }

    void DelCaller(CGNode *caller)
    {
        if (callers.find(caller) != callers.end()) {
            callers.erase(caller);
        }
    }

    void DelCallee(CallInfo *callInfo, CGNode *callee)
    {
        (void)callees[callInfo]->erase(callee);
    }

    bool HasCaller() const
    {
        return (!callers.empty());
    }

    uint32 NumberOfUses() const
    {
        return static_cast<uint32>(callers.size());
    }

    bool IsCalleeOf(CGNode *func) const;
    void IncrStmtCount()
    {
        ++stmtCount;
    }

    void IncrNodeCountBy(uint32 x)
    {
        nodeCount += x;
    }

    uint32 GetStmtCount() const
    {
        return stmtCount;
    }

    uint32 GetNodeCount() const
    {
        return nodeCount;
    }

    void Reset()
    {
        stmtCount = 0;
        nodeCount = 0;
        numReferences = 0;
        callees.clear();
        vcallCands.clear();
    }

    uint32 NumberOfCallSites() const
    {
        return static_cast<uint32>(callees.size());
    }

    const MapleMap<CallInfo *, MapleSet<CGNode *, Comparator<CGNode>> *, Comparator<CallInfo>> &GetCallee() const
    {
        return callees;
    }

    MapleMap<CallInfo *, MapleSet<CGNode *, Comparator<CGNode>> *, Comparator<CallInfo>> &GetCallee()
    {
        return callees;
    }

    const MapleMap<CGNode *, MapleSet<StmtNode *> *, Comparator<CGNode>> &GetCaller() const
    {
        return callers;
    }

    MapleMap<CGNode *, MapleSet<StmtNode *> *, Comparator<CGNode>> &GetCaller()
    {
        return callers;
    }

    const SCCNode<CGNode> *GetSCCNode() const
    {
        return sccNode;
    }

    bool IsMustNotBeInlined() const
    {
        return mustNotBeInlined;
    }

    void SetMustNotBeInlined()
    {
        mustNotBeInlined = true;
    }

    bool IsVcallCandidatesValid() const
    {
        return isVcallCandidatesValid;
    }

    void SetVcallCandidatesValid()
    {
        isVcallCandidatesValid = true;
    }

    void AddVcallCandidate(CGNode *item)
    {
        (void)vcallCandidates.insert(item);
    }

    MapleSet<CGNode *, Comparator<CGNode>> &GetVcallCandidates()
    {
        return vcallCandidates;
    }

    bool IsIcallCandidatesValid() const
    {
        return isIcallCandidatesValid;
    }

    void SetIcallCandidatesValid()
    {
        isIcallCandidatesValid = true;
    }

    void AddIcallCandidate(CGNode *item)
    {
        (void)icallCandidates.insert(item);
    }

    MapleSet<CGNode *, Comparator<CGNode>> &GetIcallCandidates()
    {
        return icallCandidates;
    }

    bool IsAddrTaken() const
    {
        return addrTaken;
    }

    void SetAddrTaken()
    {
        addrTaken = true;
    }

    void GetOutNodes(std::vector<BaseGraphNode *> &outNodes) final
    {
        for (auto &callSite : std::as_const(GetCallee())) {
            for (auto &cgIt : *callSite.second) {
                CGNode *calleeNode = cgIt;
                outNodes.emplace_back(calleeNode);
            }
        }
    }

    void GetOutNodes(std::vector<BaseGraphNode *> &outNodes) const final
    {
        for (auto &callSite : std::as_const(GetCallee())) {
            for (auto &cgIt : *callSite.second) {
                CGNode *calleeNode = cgIt;
                outNodes.emplace_back(calleeNode);
            }
        }
    }

    void GetInNodes(std::vector<BaseGraphNode *> &inNodes) final
    {
        for (auto pair : std::as_const(GetCaller())) {
            CGNode *callerNode = pair.first;
            inNodes.emplace_back(callerNode);
        }
    }

    void GetInNodes(std::vector<BaseGraphNode *> &inNodes) const final
    {
        for (auto pair : std::as_const(GetCaller())) {
            CGNode *callerNode = pair.first;
            inNodes.emplace_back(callerNode);
        }
    }

    const std::string GetIdentity() override
    {
        std::string sccIdentity;
        if (GetMIRFunction() != nullptr) {
            sccIdentity =
                "function(" + std::to_string(GetMIRFunction()->GetPuidx()) + "): " + GetMIRFunction()->GetName();
        } else {
            sccIdentity = "function: external";
        }
        return sccIdentity;
    }
    // check frequency
    int64_t GetFuncFrequency() const;
    int64_t GetCallsiteFrequency(const StmtNode *callstmt) const;

private:
    // mirFunc is generated from callStmt's puIdx from mpl instruction
    // mirFunc will be nullptr if CGNode represents an external/intrinsic call
    MapleAllocator *alloc;
    uint32 id;
    SCCNode<CGNode> *sccNode;  // the id of the scc where this cgnode belongs to
    MIRFunction *mirFunc;
    // Each callsite corresponds to one element
    MapleMap<CallInfo *, MapleSet<CGNode *, Comparator<CGNode>> *, Comparator<CallInfo>> callees;
    MapleSet<CGNode *, Comparator<CGNode>> vcallCandidates;  // vcall candidates of mirFunc
    bool isVcallCandidatesValid;
    MapleSet<CGNode *, Comparator<CGNode>> icallCandidates;  // icall candidates of mirFunc
    bool isIcallCandidatesValid;
    uint32 numReferences;  // The number of the node in this or other CGNode's callees
    // function candidate for virtual call
    // now the candidates would be same function name from base class to subclass
    // with type inference, the candidates would be reduced
    MapleMap<CGNode *, MapleSet<StmtNode *> *, Comparator<CGNode>> callers;
    uint32 stmtCount;  // count number of statements in the function, reuse this as callsite id
    uint32 nodeCount;  // count number of MIR nodes in the function/
    // this flag is used to mark the function which will read the current method invocation stack or something else,
    // so it cannot be inlined and all the parent nodes which contain this node should not be inlined, either.
    bool mustNotBeInlined;
    MapleVector<MIRFunction *> vcallCands;

    bool addrTaken = false;  // whether this function is taken address
};

using Callsite = std::pair<CallInfo *, MapleSet<CGNode *, Comparator<CGNode>> *>;
using CalleeIt = MapleMap<CallInfo *, MapleSet<CGNode *, Comparator<CGNode>> *, Comparator<CallInfo>>::iterator;
using Caller2Cands = std::pair<PUIdx, Callsite>;

class CallGraph : public AnalysisResult {
public:
    CallGraph(MIRModule &m, MemPool &memPool, MemPool &tempPool, KlassHierarchy &kh, const std::string &fn);
    ~CallGraph() = default;

    void InitCallExternal()
    {
        callExternal = cgAlloc.GetMemPool()->New<CGNode>(static_cast<MIRFunction *>(nullptr), cgAlloc, numOfNodes++);
    }

    const CGNode *CallExternal() const
    {
        return callExternal;
    }

    const CGNode *GetEntryNode() const
    {
        return entryNode;
    }

    const MapleVector<CGNode *> &GetRootNodes() const
    {
        return rootNodes;
    }

    const KlassHierarchy *GetKlassh() const
    {
        return klassh;
    }

    const MapleVector<SCCNode<CGNode> *> &GetSCCTopVec() const
    {
        return sccTopologicalVec;
    }

    const MapleMap<MIRFunction *, CGNode *, NodeComparator> &GetNodesMap() const
    {
        return nodesMap;
    }

    MIRType *GetFuncTypeFromFuncAddr(const BaseNode *);
    void RecordLocalConstValue(const StmtNode *stmt);
    CallNode *ReplaceIcallToCall(BlockNode &body, IcallNode *icall, PUIdx newPUIdx);
    void CollectAddroffuncFromExpr(const BaseNode *expr);
    void CollectAddroffuncFromStmt(const StmtNode *stmt);
    void CollectAddroffuncFromConst(MIRConst *mirConst);
    void Dump() const;
    CGNode *GetCGNode(MIRFunction *func) const;
    CGNode *GetCGNode(PUIdx puIdx) const;
    void UpdateCaleeCandidate(PUIdx callerPuIdx, IcallNode *icall, PUIdx calleePuidx, CallNode *call);
    void UpdateCaleeCandidate(PUIdx callerPuIdx, IcallNode *icall, std::set<PUIdx> &candidate);
    SCCNode<CGNode> *GetSCCNode(MIRFunction *func) const;
    bool IsRootNode(MIRFunction *func) const;
    MIRFunction *CurFunction() const
    {
        return mirModule->CurFunction();
    }

    bool IsInIPA() const
    {
        return mirModule->IsInIPA();
    }

    // iterator
    using iterator = MapleMap<MIRFunction *, CGNode *>::iterator;
    iterator Begin()
    {
        return nodesMap.begin();
    }

    iterator End()
    {
        return nodesMap.end();
    }

    MIRFunction *GetMIRFunction(const iterator &it) const
    {
        return (*it).first;
    }

    CGNode *GetCGNode(const iterator &it) const
    {
        return (*it).second;
    }

    void DelNode(CGNode &node);
    void ClearFunctionList();

    void SetDebugFlag(bool flag)
    {
        debugFlag = flag;
    }

    void GetNodes(std::vector<CGNode *> &nodes)
    {
        for (auto const &it : nodesMap) {
            CGNode *node = it.second;
            nodes.emplace_back(node);
        }
    }

private:
    CallType GetCallType(Opcode op) const;
    void FindRootNodes();
    void RemoveFileStaticRootNodes();  // file static and inline but not extern root nodes can be removed
    void RemoveFileStaticSCC();        // SCC can be removed if it has no caller and all its nodes is file static
    void SetCompilationFunclist() const;

    CallInfo *GenCallInfo(CallType type, MIRFunction *call, StmtNode *s, uint32 loopDepth, uint32 callsiteID)
    {
        return cgAlloc.GetMemPool()->New<CallInfo>(type, *call, s, loopDepth, callsiteID);
    }

    bool debugFlag = false;
    MIRModule *mirModule;
    MapleAllocator cgAlloc;
    MapleAllocator tempAlloc;
    MIRBuilder *mirBuilder;
    CGNode *entryNode;  // For main function, nullptr if there is multiple entries
    MapleVector<CGNode *> rootNodes;
    MapleString fileName;  // used for output dot file
    KlassHierarchy *klassh;
    MapleMap<MIRFunction *, CGNode *, NodeComparator> nodesMap;
    MapleVector<SCCNode<CGNode> *> sccTopologicalVec;
    MapleMap<StIdx, BaseNode *> localConstValueMap;  // used to record the local constant value
    MapleMap<TyIdx, MapleSet<Caller2Cands> *> icallToFix;
    MapleSet<PUIdx> addressTakenPuidxs;
    CGNode *callExternal = nullptr;  // Auxiliary node used in icall/intrinsic call
    uint32 numOfNodes;
    std::unordered_set<uint64> callsiteHash;
};

class IPODevirtulize {
public:
    IPODevirtulize(MIRModule *m, MemPool *memPool, KlassHierarchy *kh)
        : cgAlloc(memPool), mirBuilder(cgAlloc.GetMemPool()->New<MIRBuilder>(m)), klassh(kh)
    {
    }

    ~IPODevirtulize() = default;
    const KlassHierarchy *GetKlassh() const
    {
        return klassh;
    }

private:
    MapleAllocator cgAlloc;
    MIRBuilder *mirBuilder;
    KlassHierarchy *klassh;
};

}  // namespace maple
#endif  // MPL2MPL_INCLUDE_CALLGRAPH_H
