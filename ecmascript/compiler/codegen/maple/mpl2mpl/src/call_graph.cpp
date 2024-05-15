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

CGNode *CallGraph::GetOrGenCGNode(PUIdx puIdx, bool isVcall, bool isIcall)
{
    CGNode *node = GetCGNode(puIdx);
    if (node == nullptr) {
        MIRFunction *mirFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
        node = cgAlloc.GetMemPool()->New<CGNode>(mirFunc, cgAlloc, numOfNodes++);
        (void)nodesMap.insert(std::make_pair(mirFunc, node));
    }
    if (isVcall && !node->IsVcallCandidatesValid()) {
        MIRFunction *mirFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
        Klass *klass = nullptr;
        CHECK_NULL_FATAL(mirFunc);
        klass = klassh->GetKlassFromStrIdx(mirFunc->GetBaseClassNameStrIdx());
        if (klass == nullptr) {  // Incomplete
            node->SetVcallCandidatesValid();
            return node;
        }
        // Traverse all subclasses
        std::vector<Klass *> klassVector;
        klassVector.push_back(klass);
        GStrIdx calleeFuncStrIdx = mirFunc->GetBaseFuncNameWithTypeStrIdx();
        for (Klass *currKlass : klassVector) {
            const MIRFunction *method = currKlass->GetMethod(calleeFuncStrIdx);
            if (method != nullptr) {
                node->AddVcallCandidate(GetOrGenCGNode(method->GetPuidx()));
            }
            // add subclass of currKlass into vector
            for (Klass *subKlass : currKlass->GetSubKlasses()) {
                klassVector.push_back(subKlass);
            }
        }
        if (klass->IsClass() && !klass->GetMIRClassType()->IsAbstract()) {
            // If klass.foo does not exist, search superclass and find the nearest one
            // klass.foo does not exist
            auto &klassMethods = klass->GetMethods();
            if (std::find(klassMethods.begin(), klassMethods.end(), mirFunc) == klassMethods.end()) {
                Klass *superKlass = klass->GetSuperKlass();
                while (superKlass != nullptr) {
                    const MIRFunction *method = superKlass->GetMethod(calleeFuncStrIdx);
                    if (method != nullptr) {
                        node->AddVcallCandidate(GetOrGenCGNode(method->GetPuidx()));
                        break;
                    }
                    superKlass = superKlass->GetSuperKlass();
                }
            }
        }
        node->SetVcallCandidatesValid();
    }
    if (isIcall && !node->IsIcallCandidatesValid()) {
        Klass *CallerKlass = nullptr;
        CallerKlass = klassh->GetKlassFromStrIdx(CurFunction()->GetBaseClassNameStrIdx());
        if (CallerKlass == nullptr) {  // Incomplete
            CHECK_FATAL(false, "class is incomplete, impossible.");
            return node;
        }
        MIRFunction *mirFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
        Klass *klass = nullptr;
        klass = klassh->GetKlassFromStrIdx(mirFunc->GetBaseClassNameStrIdx());
        if (klass == nullptr) {  // Incomplete
            node->SetIcallCandidatesValid();
            return node;
        }
        GStrIdx calleeFuncStrIdx = mirFunc->GetBaseFuncNameWithTypeStrIdx();
        // Traverse all classes which implement the interface
        for (Klass *implKlass : klass->GetImplKlasses()) {
            const MIRFunction *method = implKlass->GetMethod(calleeFuncStrIdx);
            if (method != nullptr) {
                node->AddIcallCandidate(GetOrGenCGNode(method->GetPuidx()));
            } else if (!implKlass->GetMIRClassType()->IsAbstract()) {
                // Search in its parent class
                Klass *superKlass = implKlass->GetSuperKlass();
                while (superKlass != nullptr) {
                    const MIRFunction *methodT = superKlass->GetMethod(calleeFuncStrIdx);
                    if (methodT != nullptr) {
                        node->AddIcallCandidate(GetOrGenCGNode(methodT->GetPuidx()));
                        break;
                    }
                    superKlass = superKlass->GetSuperKlass();
                }
            }
        }
        node->SetIcallCandidatesValid();
    }
    return node;
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

void CallGraph::HandleCall(CGNode &node, StmtNode *stmt, uint32 loopDepth)
{
    PUIdx calleePUIdx = (static_cast<CallNode *>(stmt))->GetPUIdx();
    MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(calleePUIdx);
    // Ignore clinit
    if (!calleeFunc->IsClinit()) {
        CallInfo *callInfo = GenCallInfo(kCallTypeCall, calleeFunc, stmt, loopDepth, stmt->GetStmtID());
        CGNode *calleeNode = GetOrGenCGNode(calleeFunc->GetPuidx());
        DEBUG_ASSERT(calleeNode != nullptr, "calleenode is null");
        calleeNode->AddCaller(&node, stmt);
        node.AddCallsite(*callInfo, calleeNode);
    }
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

void CallGraph::HandleICall(BlockNode &body, CGNode &node, StmtNode *stmt, uint32 loopDepth)
{
    IcallNode *icall = static_cast<IcallNode *>(stmt);
    auto *funcAddr = icall->GetNopndAt(0);
    MIRType *funcType = nullptr;
    CHECK_FATAL(IsPrimitivePoint(funcAddr->GetPrimType()), "Error");
    switch (funcAddr->GetOpCode()) {
        case OP_dread: {
            auto *dread = static_cast<DreadNode *>(funcAddr);
            MIRSymbol *symbol = CurFunction()->GetLocalOrGlobalSymbol(dread->GetStIdx());
            funcType = symbol->GetType();
            if (funcType->IsStructType()) {
                funcType = static_cast<MIRStructType *>(funcType)->GetFieldType(dread->GetFieldID());
            }
            if (symbol->IsGlobal()) {
                // Global symbol
                if (!symbol->GetAttr(ATTR_const) || !symbol->IsConst()) {
                    break;
                }
                if (symbol->GetKonst()->GetKind() == kConstAddrofFunc) {
                    auto *addrofFuncConst = static_cast<MIRAddroffuncConst *>(symbol->GetKonst());
                    stmt = ReplaceIcallToCall(body, icall, addrofFuncConst->GetValue());
                    HandleCall(node, stmt, loopDepth);
                    return;
                }
                if (symbol->GetKonst()->GetKind() == kConstAggConst) {
                    auto *aggConst = static_cast<MIRAggConst *>(symbol->GetKonst());
                    auto *elem = aggConst->GetAggConstElement(dread->GetFieldID());
                    CHECK_FATAL(elem != nullptr, "elem is null");
                    if (elem->GetKind() == kConstAddrofFunc) {
                        auto *addrofFuncConst = static_cast<MIRAddroffuncConst *>(elem);
                        CHECK_FATAL(addrofFuncConst != nullptr, "addrofFuncConst is null");
                        stmt = ReplaceIcallToCall(body, icall, addrofFuncConst->GetValue());
                        HandleCall(node, stmt, loopDepth);
                        return;
                    }
                }
            } else {
                // Local symbol
                if (!symbol->GetAttr(ATTR_const)) {
                    break;
                }
                if (localConstValueMap.find(symbol->GetStIdx()) != localConstValueMap.end()) {
                    auto *rhsNode = localConstValueMap[symbol->GetStIdx()];
                    if (rhsNode != nullptr && rhsNode->GetOpCode() == OP_addroffunc) {
                        auto *funcNode = static_cast<AddroffuncNode *>(rhsNode);
                        stmt = ReplaceIcallToCall(body, icall, funcNode->GetPUIdx());
                        HandleCall(node, stmt, loopDepth);
                        return;
                    }
                }
            }
            break;
        }
        case OP_iread: {
            auto *iread = static_cast<IreadNode *>(funcAddr);
            funcType = iread->GetType();
            if (iread->Opnd(0)->GetOpCode() != OP_array) {
                break;
            }
            auto *arrayNode = static_cast<ArrayNode *>(iread->Opnd(0));
            if (arrayNode->GetBase()->GetOpCode() != OP_addrof) {
                break;
            }
            bool hasVarIndex = false;
            for (size_t i = 1; i < arrayNode->numOpnds; ++i) {
                if (!arrayNode->GetNopndAt(i)->IsConstval() ||
                    static_cast<ConstvalNode *>(arrayNode->GetNopndAt(i))->GetConstVal()->GetKind() != kConstInt) {
                    hasVarIndex = true;
                    break;
                }
            }
            if (hasVarIndex) {
                break;
            }
            MIRSymbol *symbol =
                CurFunction()->GetLocalOrGlobalSymbol(static_cast<AddrofNode *>(arrayNode->GetBase())->GetStIdx());
            if (symbol->IsGlobal()) {
                // Global array.
                if (!symbol->GetAttr(ATTR_const) || !symbol->IsConst()) {
                    break;
                }
                // Solve multi-dim array.
                if (symbol->GetKonst()->GetKind() == kConstAggConst) {
                    auto *aggConst = static_cast<MIRAggConst *>(symbol->GetKonst());
                    MIRConst *result = aggConst;
                    for (size_t i = 1; i < arrayNode->GetNumOpnds(); ++i) {
                        auto *konst = static_cast<ConstvalNode *>(arrayNode->GetNopndAt(i))->GetConstVal();
                        auto index = static_cast<MIRIntConst *>(konst)->GetExtValue();
                        if (result->GetKind() == kConstAggConst) {
                            result = static_cast<MIRAggConst *>(result)->GetConstVecItem(index);
                        }
                    }
                    CHECK_FATAL(result->GetKind() == kConstAddrofFunc, "Must be");
                    auto *constValue = static_cast<MIRAddroffuncConst *>(result);
                    stmt = ReplaceIcallToCall(body, icall, constValue->GetValue());
                    HandleCall(node, stmt, loopDepth);
                    return;
                }
            }
            break;
        }
        case OP_select: {
            auto *select = static_cast<const TernaryNode *>(funcAddr);
            auto *leftValue = select->Opnd(1);
            auto *rightValue = select->Opnd(2);
            if (leftValue->GetOpCode() == OP_addroffunc && rightValue->GetOpCode() == OP_addroffunc) {
                auto *funcNode1 = static_cast<AddroffuncNode *>(leftValue);
                auto *funcNode2 = static_cast<AddroffuncNode *>(rightValue);
                CallInfo *callInfo = GenCallInfo(kCallTypeIcall, nullptr, stmt, loopDepth, stmt->GetStmtID());
                CGNode *calleeNode1 = GetOrGenCGNode(funcNode1->GetPUIdx());
                CGNode *calleeNode2 = GetOrGenCGNode(funcNode2->GetPUIdx());
                CHECK_FATAL(calleeNode1 != nullptr && calleeNode2 != nullptr, "calleenode is null");
                auto *cgVector = cgAlloc.GetMemPool()->New<MapleSet<CGNode *, Comparator<CGNode>>>(cgAlloc.Adapter());
                cgVector->insert(calleeNode1);
                cgVector->insert(calleeNode2);
                calleeNode1->AddCaller(&node, stmt);
                calleeNode2->AddCaller(&node, stmt);
                node.AddCallsite(callInfo, cgVector);
                return;
            } else {
                funcType = GetFuncTypeFromFuncAddr(funcAddr);
            }
            break;
        }
        case OP_addroffunc: {
            auto *funcNode = static_cast<AddroffuncNode *>(funcAddr);
            stmt = ReplaceIcallToCall(body, icall, funcNode->GetPUIdx());
            HandleCall(node, stmt, loopDepth);
            return;
        }
        default: {
            break;
        }
    }
    if (!Options::wpaa) {
        // Do not handle icall if Options::wpaa is false.
        return;
    }
    CHECK_FATAL(funcType != nullptr, "Failed to get the function type.");
    while (funcType != nullptr && funcType->IsMIRPtrType()) {
        funcType = static_cast<MIRPtrType *>(funcType)->GetPointedType();
    }

    // Add a fake callsite here, need to fix it after all the function is visited.
    CallInfo *callInfo = GenCallInfo(kCallTypeIcall, nullptr, stmt, loopDepth, stmt->GetStmtID());
    node.AddCallsite(*callInfo, nullptr);
    ASSERT_NOT_NULL(funcType);
    if (icallToFix.find(funcType->GetTypeIndex()) == icallToFix.end()) {
        auto *tempSet = tempAlloc.GetMemPool()->New<MapleSet<Caller2Cands>>(tempAlloc.Adapter());
        icallToFix.insert({funcType->GetTypeIndex(), tempSet});
    }
    CHECK_FATAL(CurFunction()->GetPuidx() == node.GetPuIdx(), "Error");
    Callsite callSite = {callInfo, node.GetCallee().at(callInfo)};
    icallToFix.at(funcType->GetTypeIndex())->insert({node.GetPuIdx(), callSite});
}

void CallGraph::HandleBody(MIRFunction &func, BlockNode &body, CGNode &node, uint32 loopDepth)
{
    StmtNode *stmtNext = nullptr;
    for (StmtNode *stmt = body.GetFirst(); stmt != nullptr; stmt = stmtNext) {
        CollectAddroffuncFromStmt(stmt);
        RecordLocalConstValue(stmt);
        stmtNext = static_cast<StmtNode *>(stmt)->GetNext();
        Opcode op = stmt->GetOpCode();
        if (op == OP_comment) {
            continue;
        } else if (op == OP_doloop) {
            DoloopNode *doloopNode = static_cast<DoloopNode *>(stmt);
            HandleBody(func, *doloopNode->GetDoBody(), node, loopDepth + 1);
        } else if (op == OP_dowhile || op == OP_while) {
            WhileStmtNode *whileStmt = static_cast<WhileStmtNode *>(stmt);
            HandleBody(func, *whileStmt->GetBody(), node, loopDepth + 1);
        } else if (op == OP_if) {
            IfStmtNode *n = static_cast<IfStmtNode *>(stmt);
            HandleBody(func, *n->GetThenPart(), node, loopDepth);
            if (n->GetElsePart() != nullptr) {
                HandleBody(func, *n->GetElsePart(), node, loopDepth);
            }
        } else {
            node.IncrStmtCount();
            CallType ct = GetCallType(op);
            switch (ct) {
                case kCallTypeVirtualCall: {
                    PUIdx calleePUIdx = (static_cast<CallNode *>(stmt))->GetPUIdx();
                    MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(calleePUIdx);
                    CallInfo *callInfo =
                        GenCallInfo(kCallTypeVirtualCall, calleeFunc, stmt, loopDepth, stmt->GetStmtID());
                    // Retype makes object type more inaccurate.
                    StmtNode *stmtPrev = static_cast<StmtNode *>(stmt)->GetPrev();
                    if (stmtPrev != nullptr && stmtPrev->GetOpCode() == OP_dassign) {
                        DassignNode *dassignNode = static_cast<DassignNode *>(stmtPrev);
                        if (dassignNode->GetRHS()->GetOpCode() == OP_retype) {
                            CallNode *callNode = static_cast<CallNode *>(stmt);
                            CHECK_FATAL(callNode->Opnd(0)->GetOpCode() == OP_dread, "Must be dread.");
                            AddrofNode *dread = static_cast<AddrofNode *>(callNode->Opnd(0));
                            if (dassignNode->GetStIdx() == dread->GetStIdx()) {
                                RetypeNode *retypeNode = static_cast<RetypeNode *>(dassignNode->GetRHS());
                                CHECK_FATAL(retypeNode->Opnd(0)->GetOpCode() == OP_dread, "Must be dread.");
                                AddrofNode *dreadT = static_cast<AddrofNode *>(retypeNode->Opnd(0));
                                MIRType *type = func.GetLocalOrGlobalSymbol(dreadT->GetStIdx())->GetType();
                                CHECK_FATAL(type->IsMIRPtrType(), "Must be ptr type.");
                                MIRPtrType *ptrType = static_cast<MIRPtrType *>(type);
                                MIRType *targetType = ptrType->GetPointedType();
                                MIRFunction *calleeFuncT =
                                    GlobalTables::GetFunctionTable().GetFunctionFromPuidx(calleePUIdx);
                                GStrIdx calleeFuncStrIdx = calleeFuncT->GetBaseFuncNameWithTypeStrIdx();
                                Klass *klass = klassh->GetKlassFromTyIdx(targetType->GetTypeIndex());
                                if (klass != nullptr) {
                                    const MIRFunction *method = klass->GetMethod(calleeFuncStrIdx);
                                    if (method != nullptr) {
                                        calleePUIdx = method->GetPuidx();
                                    } else {
                                        std::string funcName = klass->GetKlassName();
                                        funcName.append((namemangler::kNameSplitterStr));
                                        funcName.append(calleeFuncT->GetBaseFuncNameWithType());
                                        MIRFunction *methodT =
                                            mirBuilder->GetOrCreateFunction(funcName, static_cast<TyIdx>(PTY_void));
                                        methodT->SetBaseClassNameStrIdx(klass->GetKlassNameStrIdx());
                                        methodT->SetBaseFuncNameWithTypeStrIdx(calleeFuncStrIdx);
                                        calleePUIdx = methodT->GetPuidx();
                                    }
                                }
                            }
                        }
                    }
                    // Add a call node whether or not the calleeFunc has its body
                    CGNode *calleeNode = GetOrGenCGNode(calleePUIdx, true);
                    CHECK_FATAL(calleeNode != nullptr, "calleenode is null");
                    CHECK_FATAL(calleeNode->IsVcallCandidatesValid(), "vcall candidate must be valid");
                    node.AddCallsite(callInfo, &calleeNode->GetVcallCandidates());
                    for (auto &cgIt : calleeNode->GetVcallCandidates()) {
                        CGNode *calleeNodeT = cgIt;
                        calleeNodeT->AddCaller(&node, stmt);
                    }
                    break;
                }
                case kCallTypeInterfaceCall: {
                    PUIdx calleePUIdx = (static_cast<CallNode *>(stmt))->GetPUIdx();
                    MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(calleePUIdx);
                    CallInfo *callInfo =
                        GenCallInfo(kCallTypeInterfaceCall, calleeFunc, stmt, loopDepth, stmt->GetStmtID());
                    // Add a call node whether or not the calleeFunc has its body
                    CGNode *calleeNode = GetOrGenCGNode(calleeFunc->GetPuidx(), false, true);
                    CHECK_FATAL(calleeNode != nullptr, "calleenode is null");
                    CHECK_FATAL(calleeNode->IsIcallCandidatesValid(), "icall candidate must be valid");
                    node.AddCallsite(callInfo, &calleeNode->GetIcallCandidates());
                    for (auto &cgIt : calleeNode->GetIcallCandidates()) {
                        CGNode *calleeNodeT = cgIt;
                        calleeNodeT->AddCaller(&node, stmt);
                    }
                    break;
                }
                case kCallTypeIcall: {
                    if (mirModule->IsCModule()) {
                        HandleICall(body, node, stmt, loopDepth);
                    }
                    break;
                }
                case kCallTypeCall: {
                    HandleCall(node, stmt, loopDepth);
                    break;
                }
                case kCallTypeSuperCall: {
                    PUIdx calleePUIdx = (static_cast<CallNode *>(stmt))->GetPUIdx();
                    MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(calleePUIdx);
                    Klass *klass = klassh->GetKlassFromFunc(calleeFunc);
                    if (klass == nullptr) {  // Fix CI
                        continue;
                    }
                    MapleVector<MIRFunction *> *cands =
                        klass->GetCandidates(calleeFunc->GetBaseFuncNameWithTypeStrIdx());
                    // continue to search its implinterfaces
                    if (cands == nullptr) {
                        for (Klass *implInterface : klass->GetImplInterfaces()) {
                            cands = implInterface->GetCandidates(calleeFunc->GetBaseFuncNameWithTypeStrIdx());
                            if (cands != nullptr && !cands->empty()) {
                                break;
                            }
                        }
                    }
                    if (cands == nullptr || cands->empty()) {
                        continue;  // Fix CI
                    }
                    MIRFunction *actualMirfunc = cands->at(0);
                    CallInfo *callInfo = GenCallInfo(kCallTypeCall, actualMirfunc, stmt, loopDepth, stmt->GetStmtID());
                    CGNode *calleeNode = GetOrGenCGNode(actualMirfunc->GetPuidx());
                    DEBUG_ASSERT(calleeNode != nullptr, "calleenode is null");
                    calleeNode->AddCaller(&node, stmt);
                    (static_cast<CallNode *>(stmt))->SetPUIdx(actualMirfunc->GetPuidx());
                    node.AddCallsite(*callInfo, calleeNode);
                    break;
                }
                case kCallTypeIntrinsicCall:
                case kCallTypeIntrinsicCallWithType:
                case kCallTypeCustomCall:
                case kCallTypePolymorphicCall:
                case kCallTypeXinitrinsicCall:
                case kCallTypeInvalid: {
                    break;
                }
                default: {
                    CHECK_FATAL(false, "NYI::unsupport call type");
                }
            }
        }
    }
}

void CallGraph::UpdateCallGraphNode(CGNode &node)
{
    node.Reset();
    MIRFunction *func = node.GetMIRFunction();
    CHECK_NULL_FATAL(func);
    BlockNode *body = func->GetBody();
    HandleBody(*func, *body, node, 0);
}

static void ResetInferredType(std::vector<MIRSymbol *> &inferredSymbols)
{
    for (size_t i = 0; i < inferredSymbols.size(); ++i) {
        inferredSymbols[i]->SetInferredTyIdx(TyIdx());
    }
    inferredSymbols.clear();
}

static void ResetInferredType(std::vector<MIRSymbol *> &inferredSymbols, MIRSymbol *symbol)
{
    if (symbol == nullptr) {
        return;
    }
    if (symbol->GetInferredTyIdx() == kInitTyIdx || symbol->GetInferredTyIdx() == kNoneTyIdx) {
        return;
    }
    size_t i = 0;
    for (; i < inferredSymbols.size(); ++i) {
        if (inferredSymbols[i] == symbol) {
            symbol->SetInferredTyIdx(TyIdx());
            inferredSymbols.erase(inferredSymbols.begin() + static_cast<ssize_t>(i));
            break;
        }
    }
}

static void SetInferredType(std::vector<MIRSymbol *> &inferredSymbols, MIRSymbol &symbol, const TyIdx &idx)
{
    symbol.SetInferredTyIdx(idx);
    size_t i = 0;
    for (; i < inferredSymbols.size(); ++i) {
        if (inferredSymbols[i] == &symbol) {
            break;
        }
    }
    if (i == inferredSymbols.size()) {
        inferredSymbols.push_back(&symbol);
    }
}

void IPODevirtulize::SearchDefInClinit(const Klass &klass)
{
    MIRClassType *classType = static_cast<MIRClassType *>(klass.GetMIRStructType());
    std::vector<MIRSymbol *> staticFinalPrivateSymbols;
    for (uint32 i = 0; i < classType->GetStaticFields().size(); ++i) {
        FieldAttrs attribute = classType->GetStaticFieldsPair(i).second.second;
        if (attribute.GetAttr(FLDATTR_final)) {
            staticFinalPrivateSymbols.push_back(
                GlobalTables::GetGsymTable().GetSymbolFromStrIdx(classType->GetStaticFieldsGStrIdx(i)));
        }
    }
    std::string typeName = klass.GetKlassName();
    typeName.append(namemangler::kClinitSuffix);
    GStrIdx clinitFuncGstrIdx =
        GlobalTables::GetStrTable().GetStrIdxFromName(typeName);
    if (clinitFuncGstrIdx == 0u) {
        return;
    }
    MIRFunction *func = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(clinitFuncGstrIdx)->GetFunction();
    if (func->GetBody() == nullptr) {
        return;
    }
    StmtNode *stmtNext = nullptr;
    std::vector<MIRSymbol *> gcmallocSymbols;
    for (StmtNode *stmt = func->GetBody()->GetFirst(); stmt != nullptr; stmt = stmtNext) {
        stmtNext = stmt->GetNext();
        Opcode op = stmt->GetOpCode();
        switch (op) {
            case OP_comment:
                break;
            case OP_dassign: {
                DassignNode *dassignNode = static_cast<DassignNode *>(stmt);
                MIRSymbol *leftSymbol = func->GetLocalOrGlobalSymbol(dassignNode->GetStIdx());
                size_t i = 0;
                for (; i < staticFinalPrivateSymbols.size(); ++i) {
                    if (staticFinalPrivateSymbols[i] == leftSymbol) {
                        break;
                    }
                }
                if (i < staticFinalPrivateSymbols.size()) {
                    if (dassignNode->GetRHS()->GetOpCode() == OP_dread) {
                        DreadNode *dreadNode = static_cast<DreadNode *>(dassignNode->GetRHS());
                        MIRSymbol *rightSymbol = func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
                        if (rightSymbol->GetInferredTyIdx() != kInitTyIdx &&
                            rightSymbol->GetInferredTyIdx() != kNoneTyIdx &&
                            (staticFinalPrivateSymbols[i]->GetInferredTyIdx() == kInitTyIdx ||
                             (staticFinalPrivateSymbols[i]->GetInferredTyIdx() == rightSymbol->GetInferredTyIdx()))) {
                            staticFinalPrivateSymbols[i]->SetInferredTyIdx(rightSymbol->GetInferredTyIdx());
                        } else {
                            staticFinalPrivateSymbols[i]->SetInferredTyIdx(kInitTyIdx);
                            staticFinalPrivateSymbols.erase(staticFinalPrivateSymbols.begin() +
                                                            static_cast<ssize_t>(i));
                        }
                    } else {
                        staticFinalPrivateSymbols[i]->SetInferredTyIdx(kInitTyIdx);
                        staticFinalPrivateSymbols.erase(staticFinalPrivateSymbols.begin() + static_cast<ssize_t>(i));
                    }
                } else if (dassignNode->GetRHS()->GetOpCode() == OP_gcmalloc) {
                    GCMallocNode *gcmallocNode = static_cast<GCMallocNode *>(dassignNode->GetRHS());
                    TyIdx inferredTypeIdx = gcmallocNode->GetTyIdx();
                    SetInferredType(gcmallocSymbols, *leftSymbol, inferredTypeIdx);
                } else if (dassignNode->GetRHS()->GetOpCode() == OP_retype) {
                    if (dassignNode->GetRHS()->Opnd(0)->GetOpCode() == OP_dread) {
                        DreadNode *dreadNode = static_cast<DreadNode *>(dassignNode->GetRHS()->Opnd(0));
                        MIRSymbol *rightSymbol = func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
                        if (rightSymbol->GetInferredTyIdx() != kInitTyIdx &&
                            rightSymbol->GetInferredTyIdx() != kNoneTyIdx) {
                            SetInferredType(gcmallocSymbols, *leftSymbol, rightSymbol->GetInferredTyIdx());
                        }
                    }
                } else {
                    ResetInferredType(gcmallocSymbols, leftSymbol);
                }
                break;
            }
            case OP_call:
            case OP_callassigned: {
                CallNode *callNode = static_cast<CallNode *>(stmt);
                MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
                if (calleeFunc->GetName().find(namemangler::kClinitSubStr, 0) != std::string::npos ||
                    calleeFunc->GetName().find("MCC_", 0) == 0) {
                    // ignore all side effect of initizlizor
                    continue;
                }
                for (size_t i = 0; i < callNode->GetReturnVec().size(); ++i) {
                    StIdx stIdx = callNode->GetReturnPair(i).first;
                    MIRSymbol *tmpSymbol = func->GetLocalOrGlobalSymbol(stIdx);
                    ResetInferredType(gcmallocSymbols, tmpSymbol);
                }
                for (size_t i = 0; i < callNode->GetNopndSize(); ++i) {
                    BaseNode *node = callNode->GetNopndAt(i);
                    CHECK_NULL_FATAL(node);
                    if (node->GetOpCode() != OP_dread) {
                        continue;
                    }
                    DreadNode *dreadNode = static_cast<DreadNode *>(node);
                    MIRSymbol *tmpSymbol = func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
                    ResetInferredType(gcmallocSymbols, tmpSymbol);
                }
                break;
            }
            default:
                ResetInferredType(gcmallocSymbols);
                break;
        }
    }
}

void IPODevirtulize::SearchDefInMemberMethods(const Klass &klass)
{
    SearchDefInClinit(klass);
    MIRClassType *classType = static_cast<MIRClassType *>(klass.GetMIRStructType());
    std::vector<FieldID> finalPrivateFieldID;
    for (size_t i = 0; i < classType->GetFieldsSize(); ++i) {
        FieldAttrs attribute = classType->GetFieldsElemt(i).second.second;
        if (attribute.GetAttr(FLDATTR_final)) {
            FieldID id = mirBuilder->GetStructFieldIDFromFieldNameParentFirst(
                classType, GlobalTables::GetStrTable().GetStringFromStrIdx(classType->GetFieldsElemt(i).first));
            finalPrivateFieldID.push_back(id);
        }
    }
    std::vector<MIRFunction *> initMethods;
    std::string typeName = klass.GetKlassName();
    typeName.append(namemangler::kCinitStr);
    for (MIRFunction *const &method : klass.GetMethods()) {
        if (strncmp(method->GetName().c_str(), typeName.c_str(), typeName.length()) == 0) {
            initMethods.push_back(method);
        }
    }
    if (initMethods.empty()) {
        return;
    }
    DEBUG_ASSERT(!initMethods.empty(), "Must have initializor");
    StmtNode *stmtNext = nullptr;
    for (size_t i = 0; i < initMethods.size(); ++i) {
        MIRFunction *func = initMethods[i];
        if (func->GetBody() == nullptr) {
            continue;
        }
        std::vector<MIRSymbol *> gcmallocSymbols;
        for (StmtNode *stmt = func->GetBody()->GetFirst(); stmt != nullptr; stmt = stmtNext) {
            stmtNext = stmt->GetNext();
            Opcode op = stmt->GetOpCode();
            switch (op) {
                case OP_comment:
                    break;
                case OP_dassign: {
                    DassignNode *dassignNode = static_cast<DassignNode *>(stmt);
                    MIRSymbol *leftSymbol = func->GetLocalOrGlobalSymbol(dassignNode->GetStIdx());
                    if (dassignNode->GetRHS()->GetOpCode() == OP_gcmalloc) {
                        GCMallocNode *gcmallocNode = static_cast<GCMallocNode *>(dassignNode->GetRHS());
                        SetInferredType(gcmallocSymbols, *leftSymbol, gcmallocNode->GetTyIdx());
                    } else if (dassignNode->GetRHS()->GetOpCode() == OP_retype) {
                        RetypeNode *retyStmt = static_cast<RetypeNode *>(dassignNode->GetRHS());
                        BaseNode *fromNode = retyStmt->Opnd(0);
                        if (fromNode->GetOpCode() == OP_dread) {
                            DreadNode *dreadNode = static_cast<DreadNode *>(fromNode);
                            MIRSymbol *fromSymbol = func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
                            SetInferredType(gcmallocSymbols, *leftSymbol, fromSymbol->GetInferredTyIdx());
                        } else {
                            ResetInferredType(gcmallocSymbols, leftSymbol);
                        }
                    } else {
                        ResetInferredType(gcmallocSymbols, leftSymbol);
                    }
                    break;
                }
                case OP_call:
                case OP_callassigned: {
                    CallNode *callNode = static_cast<CallNode *>(stmt);
                    MIRFunction *calleeFunc =
                        GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
                    if (calleeFunc->GetName().find(namemangler::kClinitSubStr, 0) != std::string::npos) {
                        // ignore all side effect of initizlizor
                        continue;
                    }
                    for (size_t j = 0; j < callNode->GetReturnVec().size(); ++j) {
                        StIdx stIdx = callNode->GetReturnPair(j).first;
                        MIRSymbol *tmpSymbol = func->GetLocalOrGlobalSymbol(stIdx);
                        ResetInferredType(gcmallocSymbols, tmpSymbol);
                    }
                    for (size_t j = 0; j < callNode->GetNopndSize(); ++j) {
                        BaseNode *node = callNode->GetNopndAt(j);
                        if (node->GetOpCode() != OP_dread) {
                            continue;
                        }
                        DreadNode *dreadNode = static_cast<DreadNode *>(node);
                        MIRSymbol *tmpSymbol = func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
                        ResetInferredType(gcmallocSymbols, tmpSymbol);
                    }
                    break;
                }
                case OP_iassign: {
                    IassignNode *iassignNode = static_cast<IassignNode *>(stmt);
                    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iassignNode->GetTyIdx());
                    DEBUG_ASSERT(type->GetKind() == kTypePointer, "Must be pointer type");
                    MIRPtrType *pointedType = static_cast<MIRPtrType *>(type);
                    if (pointedType->GetPointedTyIdx() == classType->GetTypeIndex()) {
                        // set field of current class
                        FieldID fieldID = iassignNode->GetFieldID();
                        size_t j = 0;
                        for (; j < finalPrivateFieldID.size(); ++j) {
                            if (finalPrivateFieldID[j] == fieldID) {
                                break;
                            }
                        }
                        if (j < finalPrivateFieldID.size()) {
                            if (iassignNode->GetRHS()->GetOpCode() == OP_dread) {
                                DreadNode *dreadNode = static_cast<DreadNode *>(iassignNode->GetRHS());
                                CHECK_FATAL(dreadNode != nullptr, "Impossible");
                                MIRSymbol *rightSymbol = func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
                                if (rightSymbol->GetInferredTyIdx() != kInitTyIdx &&
                                    rightSymbol->GetInferredTyIdx() != kNoneTyIdx &&
                                    (classType->GetElemInferredTyIdx(fieldID) == kInitTyIdx ||
                                     (classType->GetElemInferredTyIdx(fieldID) == rightSymbol->GetInferredTyIdx()))) {
                                    classType->SetElemInferredTyIdx(fieldID, rightSymbol->GetInferredTyIdx());
                                } else {
                                    classType->SetElemInferredTyIdx(fieldID, kInitTyIdx);
                                    finalPrivateFieldID.erase(finalPrivateFieldID.begin() + static_cast<ssize_t>(j));
                                }
                            } else {
                                classType->SetElemInferredTyIdx(fieldID, kInitTyIdx);
                                finalPrivateFieldID.erase(finalPrivateFieldID.begin() + static_cast<ssize_t>(j));
                            }
                        }
                    }
                    break;
                }
                default:
                    ResetInferredType(gcmallocSymbols);
                    break;
            }
        }
    }
}

void DoDevirtual(const Klass &klass, const KlassHierarchy &klassh)
{
    MIRClassType *classType = static_cast<MIRClassType *>(klass.GetMIRStructType());
    for (auto &func : klass.GetMethods()) {
        if (func->GetBody() == nullptr) {
            continue;
        }
        StmtNode *stmtNext = nullptr;
        std::vector<MIRSymbol *> inferredSymbols;
        for (StmtNode *stmt = func->GetBody()->GetFirst(); stmt != nullptr; stmt = stmtNext) {
            stmtNext = stmt->GetNext();
            Opcode op = stmt->GetOpCode();
            switch (op) {
                case OP_comment:
                    CASE_OP_ASSERT_NONNULL
                case OP_brtrue:
                case OP_brfalse:
                case OP_try:
                case OP_endtry:
                    break;
                case OP_dassign: {
                    DassignNode *dassignNode = static_cast<DassignNode *>(stmt);
                    MIRSymbol *leftSymbol = func->GetLocalOrGlobalSymbol(dassignNode->GetStIdx());
                    if (dassignNode->GetRHS()->GetOpCode() == OP_dread) {
                        DreadNode *dreadNode = static_cast<DreadNode *>(dassignNode->GetRHS());
                        if (func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx())->GetInferredTyIdx() != kInitTyIdx) {
                            SetInferredType(inferredSymbols, *leftSymbol,
                                            func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx())->GetInferredTyIdx());
                        }
                    } else if (dassignNode->GetRHS()->GetOpCode() == OP_iread) {
                        IreadNode *ireadNode = static_cast<IreadNode *>(dassignNode->GetRHS());
                        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ireadNode->GetTyIdx());
                        DEBUG_ASSERT(type->GetKind() == kTypePointer, "Must be pointer type");
                        MIRPtrType *pointedType = static_cast<MIRPtrType *>(type);
                        if (pointedType->GetPointedTyIdx() == classType->GetTypeIndex()) {
                            FieldID fieldID = ireadNode->GetFieldID();
                            FieldID tmpID = fieldID;
                            TyIdx tmpTyIdx = classType->GetElemInferredTyIdx(static_cast<size_t>(tmpID));
                            if (tmpTyIdx != kInitTyIdx && tmpTyIdx != kNoneTyIdx) {
                                SetInferredType(inferredSymbols, *leftSymbol,
                                                classType->GetElemInferredTyIdx(static_cast<size_t>(fieldID)));
                            }
                        }
                    } else {
                        ResetInferredType(inferredSymbols, leftSymbol);
                    }
                    break;
                }
                case OP_interfacecall:
                case OP_interfacecallassigned:
                case OP_virtualcall:
                case OP_virtualcallassigned: {
                    CallNode *calleeNode = static_cast<CallNode *>(stmt);
                    MIRFunction *calleeFunc =
                        GlobalTables::GetFunctionTable().GetFunctionFromPuidx(calleeNode->GetPUIdx());
                    if (calleeNode->GetNopndAt(0)->GetOpCode() == OP_dread) {
                        DreadNode *dreadNode = static_cast<DreadNode *>(calleeNode->GetNopndAt(0));
                        MIRSymbol *rightSymbol = func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
                        if (rightSymbol->GetInferredTyIdx() != kInitTyIdx &&
                            rightSymbol->GetInferredTyIdx() != kNoneTyIdx) {
                            // Devirtual
                            Klass *currKlass = klassh.GetKlassFromTyIdx(rightSymbol->GetInferredTyIdx());
                            CHECK_FATAL(currKlass != nullptr, "null ptr check");
                            if ((op == OP_interfacecall || op == OP_interfacecallassigned || op == OP_virtualcall ||
                                op == OP_virtualcallassigned)) {
                                std::vector<Klass *> klassVector;
                                klassVector.push_back(currKlass);
                                bool hasDevirtualed = false;
                                for (size_t index = 0; index < klassVector.size(); ++index) {
                                    Klass *tmpKlass = klassVector[index];
                                    for (MIRFunction *const &method : tmpKlass->GetMethods()) {
                                        if (calleeFunc->GetBaseFuncNameWithTypeStrIdx() ==
                                            method->GetBaseFuncNameWithTypeStrIdx()) {
                                            calleeNode->SetPUIdx(method->GetPuidx());
                                            if (op == OP_virtualcall || op == OP_interfacecall) {
                                                calleeNode->SetOpCode(OP_call);
                                            }
                                            if (op == OP_virtualcallassigned || op == OP_interfacecallassigned) {
                                                calleeNode->SetOpCode(OP_callassigned);
                                            }
                                            hasDevirtualed = true;
                                            if (false) {
                                                LogInfo::MapleLogger()
                                                    << ("Devirtualize In function:" + func->GetName()) << '\n';
                                                LogInfo::MapleLogger() << calleeNode->GetOpCode() << '\n';
                                                LogInfo::MapleLogger() << "    From:" << calleeFunc->GetName() << '\n';
                                                LogInfo::MapleLogger() << "    To  :"
                                                    << GlobalTables::GetFunctionTable().GetFunctionFromPuidx(
                                                        calleeNode->GetPUIdx())->GetName()
                                                    << '\n';
                                            }
                                            break;
                                        }
                                    }
                                    if (hasDevirtualed) {
                                        break;
                                    }
                                    // add subclass of currKlass into vecotr
                                    for (Klass *superKlass : tmpKlass->GetSuperKlasses()) {
                                        klassVector.push_back(superKlass);
                                    }
                                }
                                if (hasDevirtualed) {
                                    for (size_t i = 0; i < calleeNode->GetNopndSize(); ++i) {
                                        BaseNode *node = calleeNode->GetNopndAt(i);
                                        CHECK_NULL_FATAL(node);
                                        if (node->GetOpCode() != OP_dread) {
                                            continue;
                                        }
                                        dreadNode = static_cast<DreadNode *>(node);
                                        MIRSymbol *tmpSymbol = func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
                                        ResetInferredType(inferredSymbols, tmpSymbol);
                                    }
                                    if (op == OP_interfacecallassigned || op == OP_virtualcallassigned) {
                                        CallNode *callNode = static_cast<CallNode *>(stmt);
                                        for (size_t i = 0; i < callNode->GetReturnVec().size(); ++i) {
                                            StIdx stIdx = callNode->GetReturnPair(i).first;
                                            MIRSymbol *tmpSymbol = func->GetLocalOrGlobalSymbol(stIdx);
                                            ResetInferredType(inferredSymbols, tmpSymbol);
                                        }
                                    }
                                    break;
                                }
                                // Search default function in interfaces
                                Klass *tmpInterface = nullptr;
                                MIRFunction *tmpMethod = nullptr;
                                for (Klass *iKlass : currKlass->GetImplInterfaces()) {
                                    for (MIRFunction *const &method : iKlass->GetMethods()) {
                                        if (calleeFunc->GetBaseFuncNameWithTypeStrIdx() ==
                                                method->GetBaseFuncNameWithTypeStrIdx() &&
                                            !method->GetFuncAttrs().GetAttr(FUNCATTR_abstract)) {
                                            if (tmpInterface == nullptr ||
                                                klassh.IsSuperKlassForInterface(tmpInterface, iKlass)) {
                                                tmpInterface = iKlass;
                                                tmpMethod = method;
                                            }
                                            break;
                                        }
                                    }
                                }
                                // Add this check for the thirdparty APP compile
                                if (tmpMethod == nullptr) {
                                    if (Options::deferredVisit) {
                                        return;
                                    }
                                    Klass *parentKlass = klassh.GetKlassFromName(calleeFunc->GetBaseClassName());
                                    CHECK_FATAL(parentKlass != nullptr, "null ptr check");
                                    bool flag = false;
                                    if (parentKlass->GetKlassName() == currKlass->GetKlassName()) {
                                        flag = true;
                                    } else {
                                        for (Klass *const &superclass : currKlass->GetSuperKlasses()) {
                                            if (parentKlass->GetKlassName() == superclass->GetKlassName()) {
                                                flag = true;
                                                break;
                                            }
                                        }
                                        if (!flag && parentKlass->IsInterface()) {
                                            for (Klass *const &implClass : currKlass->GetImplKlasses()) {
                                                if (parentKlass->GetKlassName() == implClass->GetKlassName()) {
                                                    flag = true;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    if (!flag) {
                                        LogInfo::MapleLogger() << "warning: func " << calleeFunc->GetName()
                                                               << " is not found in DeVirtual!" << std::endl;
                                        LogInfo::MapleLogger()
                                            << "warning: " << calleeFunc->GetBaseClassName() << " is not the parent of "
                                            << currKlass->GetKlassName() << std::endl;
                                    }
                                }
                                if (tmpMethod == nullptr) {  // SearchWithoutRettype, search only in current class now.
                                    MIRType *retType =
                                        GlobalTables::GetTypeTable().GetTypeFromTyIdx(calleeFunc->GetReturnTyIdx());
                                    Klass *targetKlass = nullptr;
                                    bool isCalleeScalar = false;
                                    if (retType->GetKind() == kTypePointer && retType->GetPrimType() == PTY_ref) {
                                        MIRType *ptrType = (static_cast<MIRPtrType *>(retType))->GetPointedType();
                                        targetKlass = klassh.GetKlassFromTyIdx(ptrType->GetTypeIndex());
                                    } else if (retType->GetKind() == kTypeScalar) {
                                        isCalleeScalar = true;
                                    } else {
                                        targetKlass = klassh.GetKlassFromTyIdx(retType->GetTypeIndex());
                                    }
                                    if (targetKlass == nullptr && !isCalleeScalar) {
                                        CHECK_FATAL(false, "null ptr check");
                                    }
                                    Klass *curRetKlass = nullptr;
                                    bool isCurrVtabScalar = false;
                                    bool isFindMethod = false;
                                    for (MIRFunction *const &method : currKlass->GetMethods()) {
                                        if (calleeFunc->GetBaseFuncSigStrIdx() == method->GetBaseFuncSigStrIdx()) {
                                            Klass *tmpKlass = nullptr;
                                            MIRType *tmpType =
                                                GlobalTables::GetTypeTable().GetTypeFromTyIdx(method->GetReturnTyIdx());
                                            if (tmpType->GetKind() == kTypePointer &&
                                                tmpType->GetPrimType() == PTY_ref) {
                                                MIRType *ptrType =
                                                    (static_cast<MIRPtrType *>(tmpType))->GetPointedType();
                                                tmpKlass = klassh.GetKlassFromTyIdx(ptrType->GetTypeIndex());
                                            } else if (tmpType->GetKind() == kTypeScalar) {
                                                isCurrVtabScalar = true;
                                            } else {
                                                tmpKlass = klassh.GetKlassFromTyIdx(tmpType->GetTypeIndex());
                                            }
                                            if (tmpKlass == nullptr && !isCurrVtabScalar) {
                                                CHECK_FATAL(false, "null ptr check");
                                            }
                                            if (isCalleeScalar || isCurrVtabScalar) {
                                                if (isFindMethod) {
                                                    LogInfo::MapleLogger()
                                                        << "warning: this " << currKlass->GetKlassName()
                                                        << " has mult methods with the same function name but with "
                                                           "different return type!"
                                                        << std::endl;
                                                    break;
                                                }
                                                tmpMethod = method;
                                                isFindMethod = true;
                                                continue;
                                            }
                                            if (targetKlass->IsClass() && klassh.IsSuperKlass(tmpKlass, targetKlass) &&
                                                (curRetKlass == nullptr ||
                                                 klassh.IsSuperKlass(curRetKlass, tmpKlass))) {
                                                curRetKlass = tmpKlass;
                                                tmpMethod = method;
                                            }
                                            if (targetKlass->IsClass() &&
                                                klassh.IsInterfaceImplemented(tmpKlass, targetKlass)) {
                                                tmpMethod = method;
                                                break;
                                            }
                                            if (!targetKlass->IsClass()) {
                                                CHECK_FATAL(tmpKlass != nullptr, "Klass null ptr check");
                                                if (tmpKlass->IsClass() &&
                                                    klassh.IsInterfaceImplemented(targetKlass, tmpKlass) &&
                                                    (curRetKlass == nullptr ||
                                                     klassh.IsSuperKlass(curRetKlass, tmpKlass))) {
                                                    curRetKlass = tmpKlass;
                                                    tmpMethod = method;
                                                }
                                                if (!tmpKlass->IsClass() &&
                                                    klassh.IsSuperKlassForInterface(tmpKlass, targetKlass) &&
                                                    (curRetKlass == nullptr ||
                                                     klassh.IsSuperKlass(curRetKlass, tmpKlass))) {
                                                    curRetKlass = tmpKlass;
                                                    tmpMethod = method;
                                                }
                                            }
                                        }
                                    }
                                }
                                if (tmpMethod == nullptr && (currKlass->IsClass() || currKlass->IsInterface())) {
                                    LogInfo::MapleLogger() << "warning: func " << calleeFunc->GetName()
                                                           << " is not found in DeVirtual!" << std::endl;
                                    stmt->SetOpCode(OP_callassigned);
                                    break;
                                } else if (tmpMethod == nullptr) {
                                    LogInfo::MapleLogger()
                                        << "Error: func " << calleeFunc->GetName() << " is not found!" << std::endl;
                                    DEBUG_ASSERT(tmpMethod != nullptr, "Must not be null");
                                }
                                ASSERT_NOT_NULL(tmpMethod);
                                calleeNode->SetPUIdx(tmpMethod->GetPuidx());
                                if (op == OP_virtualcall || op == OP_interfacecall) {
                                    calleeNode->SetOpCode(OP_call);
                                }
                                if (op == OP_virtualcallassigned || op == OP_interfacecallassigned) {
                                    calleeNode->SetOpCode(OP_callassigned);
                                }
                                if (false) {
                                    LogInfo::MapleLogger() << ("Devirtualize In function:" + func->GetName()) << '\n';
                                    LogInfo::MapleLogger() << calleeNode->GetOpCode() << '\n';
                                    LogInfo::MapleLogger() << "    From:" << calleeFunc->GetName() << '\n';
                                    LogInfo::MapleLogger() << "    To  :" << GlobalTables::GetFunctionTable()
                                        .GetFunctionFromPuidx(calleeNode->GetPUIdx())->GetName() << '\n';
                                }
                                for (size_t i = 0; i < calleeNode->GetNopndSize(); ++i) {
                                    BaseNode *node = calleeNode->GetNopndAt(i);
                                    if (node->GetOpCode() != OP_dread) {
                                        continue;
                                    }
                                    dreadNode = static_cast<DreadNode *>(node);
                                    MIRSymbol *tmpSymbol = func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
                                    ResetInferredType(inferredSymbols, tmpSymbol);
                                }
                                if (op == OP_interfacecallassigned || op == OP_virtualcallassigned) {
                                    CallNode *callNode = static_cast<CallNode *>(stmt);
                                    for (size_t i = 0; i < callNode->GetReturnVec().size(); ++i) {
                                        StIdx stIdx = callNode->GetReturnPair(i).first;
                                        MIRSymbol *tmpSymbol = func->GetLocalOrGlobalSymbol(stIdx);
                                        ResetInferredType(inferredSymbols, tmpSymbol);
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
                    [[clang::fallthrough]];
                case OP_call:
                case OP_callassigned: {
                    CallNode *callNode = static_cast<CallNode *>(stmt);
                    for (size_t i = 0; i < callNode->GetReturnVec().size(); ++i) {
                        StIdx stIdx = callNode->GetReturnPair(i).first;
                        MIRSymbol *tmpSymbol = func->GetLocalOrGlobalSymbol(stIdx);
                        ResetInferredType(inferredSymbols, tmpSymbol);
                    }
                    for (size_t i = 0; i < callNode->GetNopndSize(); ++i) {
                        BaseNode *node = callNode->GetNopndAt(i);
                        if (node->GetOpCode() != OP_dread) {
                            continue;
                        }
                        DreadNode *dreadNode = static_cast<DreadNode *>(node);
                        MIRSymbol *tmpSymbol = func->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
                        ResetInferredType(inferredSymbols, tmpSymbol);
                    }
                    break;
                }
                default:
                    ResetInferredType(inferredSymbols);
                    break;
            }
        }
    }
}

void IPODevirtulize::DevirtualFinal()
{
    // Search all klass in order to find final variables
    MapleMap<GStrIdx, Klass *>::const_iterator it = klassh->GetKlasses().begin();
    for (; it != klassh->GetKlasses().end(); ++it) {
        Klass *klass = it->second;
        if (klass->IsClass()) {
            MIRClassType *classType = static_cast<MIRClassType *>(klass->GetMIRStructType());
            // Initialize inferred type of member fileds as kInitTyidx
            for (size_t i = 0; i < classType->GetFieldsSize(); ++i) {  // Don't include parent's field
                classType->SetElemInferredTyIdx(i, kInitTyIdx);
            }
            SearchDefInMemberMethods(*klass);
            for (size_t i = 0; i < classType->GetFieldInferredTyIdx().size(); ++i) {
                if (classType->GetElemInferredTyIdx(i) != kInitTyIdx &&
                    classType->GetElemInferredTyIdx(i) != kNoneTyIdx && debugFlag) {
                    FieldID tmpID = static_cast<FieldID>(i);
                    FieldPair pair = classType->TraverseToFieldRef(tmpID);
                    LogInfo::MapleLogger() << ("Inferred Final Private None-Static Variable:" + klass->GetKlassName() +
                                               ":" + GlobalTables::GetStrTable().GetStringFromStrIdx(pair.first))
                                           << '\n';
                }
            }
            for (size_t i = 0; i < classType->GetStaticFields().size(); ++i) {
                FieldAttrs attribute = classType->GetStaticFieldsPair(i).second.second;
                if (GlobalTables::GetGsymTable().GetSymbolFromStrIdx(classType->GetStaticFieldsGStrIdx(i)) == nullptr) {
                    continue;
                }
                TyIdx tyIdx = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
                    classType->GetStaticFieldsPair(i).first)->GetInferredTyIdx();
                if (tyIdx != kInitTyIdx && tyIdx != kNoneTyIdx) {
                    CHECK_FATAL(attribute.GetAttr(FLDATTR_final), "Must be final private");
                    if (debugFlag) {
                        LogInfo::MapleLogger() << ("Final Private Static Variable:" +
                            GlobalTables::GetStrTable().GetStringFromStrIdx(classType->GetStaticFieldsPair(i).first))
                            << '\n';
                    }
                }
            }
            DoDevirtual(*klass, *GetKlassh());
        }
    }
}

void CallGraph::ReadCallGraphFromMplt()
{
    // Read existing call graph from mplt, std::map<PUIdx, std::vector<CallInfo*> >
    // caller_PUIdx and all call site info are needed. Rebuild all other info of CGNode using CHA
    for (auto const &it : mirModule->GetMethod2TargetMap()) {
        CGNode *node = GetOrGenCGNode(it.first);
        CHECK_FATAL(node != nullptr, "node is null");
        std::vector<CallInfo *> callees = it.second;
        for (auto itInner = callees.begin(); itInner != callees.end(); ++itInner) {
            CallInfo *info = *itInner;
            CGNode *calleeNode =
                GetOrGenCGNode(info->GetFunc()->GetPuidx(), info->GetCallType() == kCallTypeVirtualCall,
                               info->GetCallType() == kCallTypeInterfaceCall);
            CHECK_FATAL(calleeNode != nullptr, "calleeNode is null");
            if (info->GetCallType() == kCallTypeVirtualCall) {
                node->AddCallsite(*itInner, &calleeNode->GetVcallCandidates());
            } else if (info->GetCallType() == kCallTypeInterfaceCall) {
                node->AddCallsite(*itInner, &calleeNode->GetIcallCandidates());
            } else if (info->GetCallType() == kCallTypeCall) {
                node->AddCallsite(**itInner, calleeNode);
            } else if (info->GetCallType() == kCallTypeSuperCall) {
                const MIRFunction *calleeFunc = info->GetFunc();
                Klass *klass = klassh->GetKlassFromFunc(calleeFunc);
                if (klass == nullptr) {  // Fix CI
                    continue;
                }
                MapleVector<MIRFunction *> *cands = klass->GetCandidates(calleeFunc->GetBaseFuncNameWithTypeStrIdx());
                // continue to search its implinterfaces
                if (cands == nullptr) {
                    for (Klass *implInterface : klass->GetImplInterfaces()) {
                        cands = implInterface->GetCandidates(calleeFunc->GetBaseFuncNameWithTypeStrIdx());
                        if (cands != nullptr && !cands->empty()) {
                            break;
                        }
                    }
                }
                if (cands == nullptr || cands->empty()) {
                    continue;  // Fix CI
                }
                MIRFunction *actualMirFunc = cands->at(0);
                CGNode *tempNode = GetOrGenCGNode(actualMirFunc->GetPuidx());
                DEBUG_ASSERT(tempNode != nullptr, "calleenode is null in CallGraph::HandleBody");
                node->AddCallsite(*info, tempNode);
            }
            for (auto &callSite : node->GetCallee()) {
                if (callSite.first == info) {
                    for (auto &cgIt : *callSite.second) {
                        CGNode *tempNode = cgIt;
                        tempNode->AddCaller(node, info->GetCallStmt());
                    }
                    break;
                }
            }
        }
    }
}

void CallGraph::GetMatchedCGNode(TyIdx idx, std::vector<CGNode *> &result)
{
    auto *funcType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(idx);
    for (auto puidx : addressTakenPuidxs) {
        auto *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puidx);
        if (func == nullptr) {
            continue;
        }
        if (func->GetMIRFuncType()->CompatibleWith(*funcType)) {
            result.push_back(GetOrGenCGNode(puidx));
        }
    }
}

void CallGraph::FixIcallCallee()
{
    for (auto &pair : icallToFix) {
        std::vector<CGNode *> funcs;
        GetMatchedCGNode(pair.first, funcs);
        for (auto &callerCallee : *pair.second) {
            auto puidx = callerCallee.first;
            auto candidate = callerCallee.second;
            CHECK_FATAL(candidate.second->empty(), "Error");
            candidate.second->insert(funcs.begin(), funcs.end());
            StmtNode *stmt = candidate.first->GetCallStmt();
            std::for_each(funcs.begin(), funcs.end(), [puidx, stmt, this](CGNode *elem) {
                CGNode *callerNode = GetOrGenCGNode(puidx);
                elem->AddCaller(callerNode, stmt);
            });
        }
    }
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
