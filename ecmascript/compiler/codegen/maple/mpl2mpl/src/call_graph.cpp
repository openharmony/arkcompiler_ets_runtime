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

#include "call_graph.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <fstream>

#include "option.h"
#include "string_utils.h"
#include "maple_phase_manager.h"

//                   Call Graph Analysis
// This phase is a foundation phase of compilation. This phase build
// the call graph not only for this module also for the modules it
// depends on when this phase is running for IPA.
// The main procedure shows as following.
// A. Devirtual virtual call of private final static and none-static
//    variable. This step aims to reduce the callee set for each call
//    which can benefit IPA analysis.
// B. Build Call Graph.
//    i)  For IPA, it rebuild all the call graph of the modules this
//        module depends on. All necessary information is stored in mplt.
//    ii) Analysis each function in this module. For each call statement
//        create a CGNode, and collect potential callee functions to
//        generate Call Graph.
// C. Find All Root Node for the Call Graph.
// D. Construct SCC based on Tarjan Algorithm
// E. Set compilation order as the bottom-up order of callgraph. So callee
//    is always compiled before caller. This benefits those optimizations
//    need interprocedure information like escape analysis.
namespace maple {
const std::string CallInfo::GetCallTypeName() const
{
    switch (cType) {
        case kCallTypeCall:
            return "c";
        case kCallTypeVirtualCall:
            return "v";
        case kCallTypeSuperCall:
            return "s";
        case kCallTypeInterfaceCall:
            return "i";
        case kCallTypeIcall:
            return "icall";
        case kCallTypeIntrinsicCall:
            return "intrinsiccall";
        case kCallTypeXinitrinsicCall:
            return "xintrinsiccall";
        case kCallTypeIntrinsicCallWithType:
            return "intrinsiccallwithtype";
        case kCallTypeFakeThreadStartRun:
            return "fakecallstartrun";
        case kCallTypeCustomCall:
            return "customcall";
        case kCallTypePolymorphicCall:
            return "polymorphiccall";
        default:
            CHECK_FATAL(false, "unsupport CALL type");
            return "";
    }
}

const std::string CallInfo::GetCalleeName() const
{
    if ((cType >= kCallTypeCall) && (cType <= kCallTypeInterfaceCall)) {
        MIRFunction &mirf = *mirFunc;
        return mirf.GetName();
    } else if (cType == kCallTypeIcall) {
        return "IcallUnknown";
    } else if ((cType >= kCallTypeIntrinsicCall) && (cType <= kCallTypeIntrinsicCallWithType)) {
        return "IntrinsicCall";
    } else if (cType == kCallTypeCustomCall) {
        return "CustomCall";
    } else if (cType == kCallTypePolymorphicCall) {
        return "PolymorphicCall";
    }
    CHECK_FATAL(false, "should not be here");
    return "";
}

void CGNode::DumpDetail() const
{
    LogInfo::MapleLogger() << "---CGNode  @" << this << ": " << mirFunc->GetName() << "\t";
    if (Options::profileUse && mirFunc->GetFuncProfData()) {
        LogInfo::MapleLogger() << " Ffreq " << GetFuncFrequency() << "\t";
    }
    if (HasOneCandidate() != nullptr) {
        LogInfo::MapleLogger() << "@One Candidate\n";
    } else {
        LogInfo::MapleLogger() << std::endl;
    }
    if (HasSetVCallCandidates()) {
        for (uint32 i = 0; i < vcallCands.size(); ++i) {
            LogInfo::MapleLogger() << "   virtual call candidates: " << vcallCands[i]->GetName() << "\n";
        }
    }
    for (auto &callSite : callees) {
        for (auto &cgIt : *callSite.second) {
            CallInfo *ci = callSite.first;
            CGNode *node = cgIt;
            MIRFunction *mf = node->GetMIRFunction();
            if (mf != nullptr) {
                LogInfo::MapleLogger() << "\tcallee in module : " << mf->GetName() << "  ";
            } else {
                LogInfo::MapleLogger() << "\tcallee external: " << ci->GetCalleeName();
            }
            if (Options::profileUse) {
                LogInfo::MapleLogger() << " callsite freq" << GetCallsiteFrequency(ci->GetCallStmt());
            }
            LogInfo::MapleLogger() << "\n";
        }
    }
    // dump caller
    for (auto const &callerNode : GetCaller()) {
        CHECK_NULL_FATAL(callerNode.first);
        CHECK_NULL_FATAL(callerNode.first->mirFunc);
        LogInfo::MapleLogger() << "\tcaller : " << callerNode.first->mirFunc->GetName() << std::endl;
    }
}

void CGNode::Dump(std::ofstream &fout) const
{
    // if dumpall == 1, dump whole call graph
    // else dump callgraph with function defined in same module
    CHECK_NULL_FATAL(mirFunc);
    constexpr size_t withoutRingNodeSize = 1;
    if (callees.empty()) {
        fout << "\"" << mirFunc->GetName() << "\";\n";
        return;
    }
    for (auto &callSite : callees) {
        for (auto &cgIt : *callSite.second) {
            CallInfo *ci = callSite.first;
            CGNode *node = cgIt;
            if (node == nullptr) {
                continue;
            }
            MIRFunction *func = node->GetMIRFunction();
            fout << "\"" << mirFunc->GetName() << "\" -> ";
            if (func != nullptr) {
                if (node->GetSCCNode() != nullptr && node->GetSCCNode()->GetNodes().size() > withoutRingNodeSize) {
                    fout << "\"" << func->GetName() << "\"[label=" << node->GetSCCNode()->GetID() << " color=red];\n";
                } else {
                    fout << "\"" << func->GetName() << "\"[label=" << 0 << " color=blue];\n";
                }
            } else {
                // unknown / external function with empty function body
                fout << "\"" << ci->GetCalleeName() << "\"[label=" << ci->GetCallTypeName() << " color=blue];\n";
            }
        }
    }
}

void CGNode::AddCallsite(CallInfo *ci, MapleSet<CGNode *, Comparator<CGNode>> *callee)
{
    (void)callees.insert(std::pair<CallInfo *, MapleSet<CGNode *, Comparator<CGNode>> *>(ci, callee));
}

void CGNode::AddCallsite(CallInfo &ci, CGNode *node)
{
    CHECK_FATAL(ci.GetCallType() != kCallTypeInterfaceCall, "must be true");
    CHECK_FATAL(ci.GetCallType() != kCallTypeVirtualCall, "must be true");
    auto *cgVector = alloc->GetMemPool()->New<MapleSet<CGNode *, Comparator<CGNode>>>(alloc->Adapter());
    if (node != nullptr) {
        node->AddNumRefs();
        cgVector->insert(node);
    }
    (void)callees.emplace(&ci, cgVector);
}

void CGNode::RemoveCallsite(const CallInfo *ci, CGNode *node)
{
    for (Callsite callSite : GetCallee()) {
        if (callSite.first == ci) {
            auto cgIt = callSite.second->find(node);
            if (cgIt != callSite.second->end()) {
                callSite.second->erase(cgIt);
                return;
            }
            CHECK_FATAL(false, "node isn't in ci");
        }
    }
}

bool CGNode::IsCalleeOf(CGNode *func) const
{
    return callers.find(func) != callers.end();
}

int64_t CGNode::GetCallsiteFrequency(const StmtNode *callstmt) const
{
    GcovFuncInfo *funcInfo = mirFunc->GetFuncProfData();
    if (funcInfo->stmtFreqs.count(callstmt->GetStmtID()) > 0) {
        return funcInfo->stmtFreqs[callstmt->GetStmtID()];
    }
    DEBUG_ASSERT(0, "should not be here");
    return -1;
}

int64_t CGNode::GetFuncFrequency() const
{
    GcovFuncInfo *funcInfo = mirFunc->GetFuncProfData();
    if (funcInfo) {
        return funcInfo->GetFuncFrequency();
    }
    DEBUG_ASSERT(0, "should not be here");
    return 0;
}

void CallGraph::ClearFunctionList()
{
    for (auto iter = mirModule->GetFunctionList().begin(); iter != mirModule->GetFunctionList().end();) {
        if (GlobalTables::GetFunctionTable().GetFunctionFromPuidx((*iter)->GetPuidx()) == nullptr) {
            (*iter)->ReleaseCodeMemory();
            (*iter)->ReleaseMemory();
            iter = mirModule->GetFunctionList().erase(iter);
        } else {
            ++iter;
        }
    }
}

void CallGraph::DelNode(CGNode &node)
{
    if (node.GetMIRFunction() == nullptr) {
        return;
    }
    for (auto &callSite : node.GetCallee()) {
        for (auto &cgIt : *callSite.second) {
            cgIt->DelCaller(&node);
            node.DelCallee(callSite.first, cgIt);
            if (!cgIt->HasCaller() && cgIt->GetMIRFunction()->IsStatic() && !cgIt->IsAddrTaken()) {
                DelNode(*cgIt);
            }
        }
    }
    MIRFunction *func = node.GetMIRFunction();
    // Delete the method of class info
    if (func->GetClassTyIdx() != 0u) {
        MIRType *classType = GlobalTables::GetTypeTable().GetTypeTable().at(func->GetClassTyIdx());
        auto *mirStructType = static_cast<MIRStructType *>(classType);
        size_t j = 0;
        for (; j < mirStructType->GetMethods().size(); ++j) {
            if (mirStructType->GetMethodsElement(j).first == func->GetStIdx()) {
                mirStructType->GetMethods().erase(mirStructType->GetMethods().begin() + static_cast<ssize_t>(j));
                break;
            }
        }
    }
    GlobalTables::GetFunctionTable().SetFunctionItem(func->GetPuidx(), nullptr);
    // func will be erased, so the coressponding symbol should be set as Deleted
    func->GetFuncSymbol()->SetIsDeleted();
    nodesMap.erase(func);
    // Update Klass info as it has been built
    if (klassh->GetKlassFromFunc(func) != nullptr) {
        klassh->GetKlassFromFunc(func)->DelMethod(*func);
    }
}

CallGraph::CallGraph(MIRModule &m, MemPool &memPool, MemPool &templPool, KlassHierarchy &kh, const std::string &fn)
    : AnalysisResult(&memPool),
      mirModule(&m),
      cgAlloc(&memPool),
      tempAlloc(&templPool),
      mirBuilder(cgAlloc.GetMemPool()->New<MIRBuilder>(&m)),
      entryNode(nullptr),
      rootNodes(cgAlloc.Adapter()),
      fileName(fn, &memPool),
      klassh(&kh),
      nodesMap(cgAlloc.Adapter()),
      sccTopologicalVec(cgAlloc.Adapter()),
      localConstValueMap(tempAlloc.Adapter()),
      icallToFix(tempAlloc.Adapter()),
      addressTakenPuidxs(tempAlloc.Adapter()),
      numOfNodes(0)
{
}

CallType CallGraph::GetCallType(Opcode op) const
{
    CallType typeTemp = kCallTypeInvalid;
    switch (op) {
        case OP_call:
        case OP_callassigned:
            typeTemp = kCallTypeCall;
            break;
        case OP_virtualcall:
        case OP_virtualcallassigned:
            typeTemp = kCallTypeVirtualCall;
            break;
        case OP_superclasscall:
        case OP_superclasscallassigned:
            typeTemp = kCallTypeSuperCall;
            break;
        case OP_interfacecall:
        case OP_interfacecallassigned:
            typeTemp = kCallTypeInterfaceCall;
            break;
        case OP_icall:
        case OP_icallassigned:
            typeTemp = kCallTypeIcall;
            break;
        case OP_intrinsiccall:
        case OP_intrinsiccallassigned:
            typeTemp = kCallTypeIntrinsicCall;
            break;
        case OP_xintrinsiccall:
        case OP_xintrinsiccallassigned:
            typeTemp = kCallTypeXinitrinsicCall;
            break;
        case OP_intrinsiccallwithtype:
        case OP_intrinsiccallwithtypeassigned:
            typeTemp = kCallTypeIntrinsicCallWithType;
            break;
        case OP_customcall:
        case OP_customcallassigned:
            typeTemp = kCallTypeCustomCall;
            break;
        case OP_polymorphiccall:
        case OP_polymorphiccallassigned:
            typeTemp = kCallTypePolymorphicCall;
            break;
        default:
            break;
    }
    return typeTemp;
}

CGNode *CallGraph::GetCGNode(MIRFunction *func) const
{
    if (nodesMap.find(func) != nodesMap.end()) {
        return nodesMap.at(func);
    }
    return nullptr;
}

CGNode *CallGraph::GetCGNode(PUIdx puIdx) const
{
    return GetCGNode(GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx));
}

SCCNode<CGNode> *CallGraph::GetSCCNode(MIRFunction *func) const
{
    CGNode *cgNode = GetCGNode(func);
    return (cgNode != nullptr) ? cgNode->GetSCCNode() : nullptr;
}

void CallGraph::UpdateCaleeCandidate(PUIdx callerPuIdx, IcallNode *icall, std::set<PUIdx> &candidate)
{
    CGNode *caller = GetCGNode(callerPuIdx);
    CHECK_FATAL(caller != nullptr, "caller is null");
    for (auto &pair : caller->GetCallee()) {
        auto *callsite = pair.first;
        if (callsite->GetCallStmt() == icall) {
            auto *calleeSet = pair.second;
            calleeSet->clear();
            std::for_each(candidate.begin(), candidate.end(), [this, &calleeSet](PUIdx idx) {
                CGNode *callee = GetCGNode(idx);
                calleeSet->insert(callee);
            });
            return;
        }
    }
}

void CallGraph::UpdateCaleeCandidate(PUIdx callerPuIdx, IcallNode *icall, PUIdx calleePuidx, CallNode *call)
{
    CGNode *caller = GetCGNode(callerPuIdx);
    CHECK_FATAL(caller != nullptr, "caller is null");
    for (auto &pair : caller->GetCallee()) {
        auto *callsite = pair.first;
        if (callsite->GetCallStmt() == icall) {
            callsite->SetCallStmt(call);
            auto *calleeSet = pair.second;
            calleeSet->clear();
            calleeSet->insert(GetCGNode(calleePuidx));
        }
    }
}

bool CallGraph::IsRootNode(MIRFunction *func) const
{
    if (GetCGNode(func) != nullptr) {
        return (!GetCGNode(func)->HasCaller());
    } else {
        return false;
    }
}

// if expr has addroffunc expr as its opnd, store all the addroffunc puidx into result
void CallGraph::CollectAddroffuncFromExpr(const BaseNode *expr)
{
    if (expr->GetOpCode() == OP_addroffunc) {
        addressTakenPuidxs.insert(static_cast<const AddroffuncNode *>(expr)->GetPUIdx());
        return;
    }
    for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
        CollectAddroffuncFromExpr(expr->Opnd(i));
    }
}

void CallGraph::CollectAddroffuncFromStmt(const StmtNode *stmt)
{
    for (size_t i = 0; i < stmt->NumOpnds(); ++i) {
        CollectAddroffuncFromExpr(stmt->Opnd(i));
    }
}

void CallGraph::CollectAddroffuncFromConst(MIRConst *mirConst)
{
    if (mirConst->GetKind() == kConstAddrofFunc) {
        addressTakenPuidxs.insert(static_cast<MIRAddroffuncConst *>(mirConst)->GetValue());
    } else if (mirConst->GetKind() == kConstAggConst) {
        auto &constVec = static_cast<MIRAggConst *>(mirConst)->GetConstVec();
        for (auto &cst : constVec) {
            CollectAddroffuncFromConst(cst);
        }
    }
}

void CallGraph::RecordLocalConstValue(const StmtNode *stmt)
{
    if (stmt->GetOpCode() != OP_dassign) {
        return;
    }
    auto *dassign = static_cast<const DassignNode *>(stmt);
    MIRSymbol *lhs = CurFunction()->GetLocalOrGlobalSymbol(dassign->GetStIdx());
    if (!lhs->IsLocal() || !lhs->GetAttr(ATTR_const) || dassign->GetFieldID() != 0) {
        return;
    }
    if (localConstValueMap.find(lhs->GetStIdx()) != localConstValueMap.end()) {
        // Multi def found, put nullptr to indicate that we cannot handle this.
        localConstValueMap[lhs->GetStIdx()] = nullptr;
        return;
    }
    localConstValueMap[lhs->GetStIdx()] = dassign->GetRHS();
}

CallNode *CallGraph::ReplaceIcallToCall(BlockNode &body, IcallNode *icall, PUIdx newPUIdx)
{
    MapleVector<BaseNode *> opnds(icall->GetNopnd().begin() + 1, icall->GetNopnd().end(),
                                  CurFunction()->GetCodeMPAllocator().Adapter());
    CallNode *newCall = nullptr;
    if (icall->GetOpCode() == OP_icall) {
        newCall = mirBuilder->CreateStmtCall(newPUIdx, opnds, OP_call);
    } else if (icall->GetOpCode() == OP_icallassigned) {
        newCall =
            mirBuilder->CreateStmtCallAssigned(newPUIdx, opnds, icall->GetCallReturnSymbol(mirBuilder->GetMirModule()),
                                               OP_callassigned, icall->GetRetTyIdx());
    } else {
        CHECK_FATAL(false, "NYI");
    }
    body.ReplaceStmt1WithStmt2(icall, newCall);
    newCall->SetSrcPos(icall->GetSrcPos());
    if (debugFlag) {
        icall->Dump(0);
        newCall->Dump(0);
        LogInfo::MapleLogger() << "replace icall successfully!\n";
    }
    return newCall;
}

MIRType *CallGraph::GetFuncTypeFromFuncAddr(const BaseNode *base)
{
    MIRType *funcType = nullptr;
    switch (base->GetOpCode()) {
        case OP_dread: {
            auto *dread = static_cast<const DreadNode *>(base);
            const MIRSymbol *st = CurFunction()->GetLocalOrGlobalSymbol(dread->GetStIdx());
            funcType = st->GetType();
            if (funcType->IsStructType()) {
                funcType = static_cast<MIRStructType *>(funcType)->GetFieldType(dread->GetFieldID());
            }
            break;
        }
        case OP_iread: {
            auto *iread = static_cast<const IreadNode *>(base);
            funcType = iread->GetType();
            break;
        }
        case OP_select: {
            auto *select = static_cast<const TernaryNode *>(base);
            funcType = GetFuncTypeFromFuncAddr(select->Opnd(kSecondOpnd));
            if (funcType == nullptr) {
                funcType = GetFuncTypeFromFuncAddr(select->Opnd(kThirdOpnd));
            }
            break;
        }
        case OP_addroffunc: {
            auto *funcNode = static_cast<const AddroffuncNode *>(base);
            auto *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(funcNode->GetPUIdx());
            funcType = func->GetMIRFuncType();
            break;
        }
        default: {
            CHECK_FATAL(false, "NYI");
            break;
        }
    }
    CHECK_FATAL(funcType != nullptr, "Error");
    return funcType;
}

void CallGraph::FindRootNodes()
{
    if (!rootNodes.empty()) {
        CHECK_FATAL(false, "rootNodes has already been set");
    }
    for (auto const &it : nodesMap) {
        CGNode *node = it.second;
        if (!node->HasCaller()) {
            rootNodes.push_back(node);
        }
    }
}

void CallGraph::RemoveFileStaticRootNodes()
{
    std::vector<CGNode *> staticRoots;
    std::copy_if(
        rootNodes.begin(), rootNodes.end(), std::inserter(staticRoots, staticRoots.begin()), [](const CGNode *root) {
            // root means no caller, we should also make sure that root is not be used in addroffunc
            auto mirFunc = root->GetMIRFunction();
            return root != nullptr &&
                   mirFunc != nullptr &&  // remove before
                                          // if static functions or inline but not extern modified functions are not
                                          // used anymore, they can be removed safely.
                   !root->IsAddrTaken() && (mirFunc->IsStatic() || (mirFunc->IsInline() && !mirFunc->IsExtern()));
        });
    for (auto *root : staticRoots) {
        // DFS delete root and its callee that is static and have no caller after root is deleted
        DelNode(*root);
    }
    ClearFunctionList();
    // rebuild rootNodes
    rootNodes.clear();
    FindRootNodes();
}

void CallGraph::RemoveFileStaticSCC()
{
    for (size_t idx = 0; idx < sccTopologicalVec.size();) {
        SCCNode<CGNode> *sccNode = sccTopologicalVec[idx];
        if (sccNode->HasInScc() || sccNode == nullptr) {
            ++idx;
            continue;
        }
        bool canBeDel = true;
        for (auto *node : sccNode->GetNodes()) {
            // If the function is not static, it may be referred in other module;
            // If the function is taken address, we should deal with this situation conservatively,
            // because we are not sure whether the func pointer may escape from this SCC
            if (!node->GetMIRFunction()->IsStatic() || node->IsAddrTaken()) {
                canBeDel = false;
                break;
            }
        }
        if (canBeDel) {
            sccTopologicalVec.erase(sccTopologicalVec.begin() + static_cast<ssize_t>(idx));
            for (auto *calleeSCC : sccNode->GetOutScc()) {
                calleeSCC->RemoveInScc(sccNode);
            }
            for (auto *cgnode : sccNode->GetNodes()) {
                DelNode(*cgnode);
            }
            // this sccnode is deleted from sccTopologicalVec, so we don't inc idx here
            continue;
        }
        ++idx;
    }
    ClearFunctionList();
}

void CallGraph::Dump() const
{
    for (auto const &it : nodesMap) {
        CGNode *node = it.second;
        node->DumpDetail();
    }
}

// Sort CGNode within an SCC. Try best to arrange callee appears before
// its (direct) caller, so that caller can benefit from summary info.
// If we have iterative inter-procedure analysis, then would not bother
// do this.
static bool CGNodeCompare(CGNode *left, CGNode *right)
{
    // special case: left calls right and right calls left, then compare by id
    if (left->IsCalleeOf(right) && right->IsCalleeOf(left)) {
        return left->GetID() < right->GetID();
    }
    // left is right's direct callee, then make left appears first
    if (left->IsCalleeOf(right)) {
        return true;
    } else if (right->IsCalleeOf(left)) {
        return false;
    }
    return left->GetID() < right->GetID();
}

// Set compilation order as the bottom-up order of callgraph. So callee
// is always compiled before caller. This benifits thoses optimizations
// need interprocedure information like escape analysis.
void CallGraph::SetCompilationFunclist() const
{
    mirModule->GetFunctionList().clear();
    const MapleVector<SCCNode<CGNode> *> &sccTopVec = GetSCCTopVec();
    for (MapleVector<SCCNode<CGNode> *>::const_reverse_iterator it = sccTopVec.rbegin(); it != sccTopVec.rend(); ++it) {
        SCCNode<CGNode> *sccNode = *it;
        std::sort(sccNode->GetNodes().begin(), sccNode->GetNodes().end(), CGNodeCompare);
        for (auto const kIt : sccNode->GetNodes()) {
            CGNode *node = kIt;
            MIRFunction *func = node->GetMIRFunction();
            if ((func != nullptr && func->GetBody() != nullptr && !IsInIPA()) ||
                (func != nullptr && !func->IsNative())) {
                mirModule->GetFunctionList().push_back(func);
            }
        }
    }
}

void CGNode::AddCandsForCallNode(const KlassHierarchy &kh)
{
    // already set vcall candidates information
    if (HasSetVCallCandidates()) {
        return;
    }
    CHECK_NULL_FATAL(mirFunc);
    Klass *klass = kh.GetKlassFromFunc(mirFunc);
    if (klass != nullptr) {
        MapleVector<MIRFunction *> *vec = klass->GetCandidates(mirFunc->GetBaseFuncNameWithTypeStrIdx());
        if (vec != nullptr) {
            vcallCands = *vec;  // Vector copy
        }
    }
}

MIRFunction *CGNode::HasOneCandidate() const
{
    int count = 0;
    MIRFunction *cand = nullptr;
    if (!mirFunc->IsEmpty()) {
        ++count;
        cand = mirFunc;
    }
    // scan candidates
    for (size_t i = 0; i < vcallCands.size(); ++i) {
        if (vcallCands[i] == nullptr) {
            CHECK_FATAL(false, "must not be nullptr");
        }
        if (!vcallCands[i]->IsEmpty()) {
            ++count;
            if (cand == nullptr) {
                cand = vcallCands[i];
            }
        }
    }
    return (count == 1) ? cand : nullptr;
}

}  // namespace maple
