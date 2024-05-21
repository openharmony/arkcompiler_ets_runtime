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

#include "me_cfg.h"
#include <iostream>
#include <algorithm>
#include <string>
#include "bb.h"
#include "ssa_mir_nodes.h"
#include "me_irmap.h"
#include "mir_builder.h"
#include "mir_lower.h"
#include "maple_phase_manager.h"

namespace {
constexpr int kFuncNameLenLimit = 80;
}

namespace maple {
#define MATCH_STMT(stmt, kOpCode)                                        \
    do {                                                                 \
        while ((stmt) != nullptr && (stmt)->GetOpCode() == OP_comment) { \
            (stmt) = (stmt)->GetNext();                                  \
        }                                                                \
        if ((stmt) == nullptr || (stmt)->GetOpCode() != (kOpCode)) {     \
            return false;                                                \
        }                                                                \
    } while (0)  // END define

bool MeCFG::FindExprUse(const BaseNode &expr, StIdx stIdx) const
{
    if (expr.GetOpCode() == OP_addrof || expr.GetOpCode() == OP_dread) {
        const auto &addofNode = static_cast<const AddrofNode &>(expr);
        return addofNode.GetStIdx() == stIdx;
    } else if (expr.GetOpCode() == OP_iread) {
        return FindExprUse(*expr.Opnd(0), stIdx);
    } else {
        for (size_t i = 0; i < expr.NumOpnds(); ++i) {
            if (FindExprUse(*expr.Opnd(i), stIdx)) {
                return true;
            }
        }
    }
    return false;
}

bool MeCFG::FindDef(const StmtNode &stmt, StIdx stIdx) const
{
    if (stmt.GetOpCode() != OP_dassign && !kOpcodeInfo.IsCallAssigned(stmt.GetOpCode())) {
        return false;
    }
    if (stmt.GetOpCode() == OP_dassign) {
        const auto &dassStmt = static_cast<const DassignNode &>(stmt);
        return dassStmt.GetStIdx() == stIdx;
    }
    const auto &cNode = static_cast<const CallNode &>(stmt);
    const CallReturnVector &nRets = cNode.GetReturnVec();
    if (!nRets.empty()) {
        DEBUG_ASSERT(nRets.size() == 1, "Single Ret value for now.");
        StIdx idx = nRets[0].first;
        RegFieldPair regFieldPair = nRets[0].second;
        if (!regFieldPair.IsReg()) {
            return idx == stIdx;
        }
    }
    return false;
}

bool MeCFG::IsStartTryBB(maple::BB &meBB) const
{
    if (!meBB.GetAttributes(kBBAttrIsTry) || meBB.GetAttributes(kBBAttrIsTryEnd)) {
        return false;
    }
    return (!meBB.GetStmtNodes().empty() && meBB.GetStmtNodes().front().GetOpCode() == OP_try);
}

// CFG Verify
// Check bb-vec and bb-list are strictly consistent.
void MeCFG::Verify() const
{
    // Check every bb in bb-list.
    auto eIt = valid_end();
    for (auto bIt = valid_begin(); bIt != eIt; ++bIt) {
        if (bIt == common_entry() || bIt == common_exit()) {
            continue;
        }
        auto *bb = *bIt;
        DEBUG_ASSERT(bb->GetBBId() < bbVec.size(), "CFG Error!");
        DEBUG_ASSERT(bbVec.at(bb->GetBBId()) == bb, "CFG Error!");
        if (bb->IsEmpty()) {
            continue;
        }
        DEBUG_ASSERT(bb->GetKind() != kBBUnknown, "runtime check error");
        // verify succ[1] is goto bb
        if (bb->GetKind() == kBBCondGoto) {
            if (!bb->GetAttributes(kBBAttrIsTry) && !bb->GetAttributes(kBBAttrWontExit)) {
                DEBUG_ASSERT(bb->GetStmtNodes().rbegin().base().d() != nullptr, "runtime check error");
                DEBUG_ASSERT(bb->GetSucc().size() == kBBVectorInitialSize, "runtime check error");
            }
            DEBUG_ASSERT(bb->GetSucc(1)->GetBBLabel() ==
                             static_cast<CondGotoNode &>(bb->GetStmtNodes().back()).GetOffset(),
                         "runtime check error");
        } else if (bb->GetKind() == kBBGoto) {
            if (bb->GetStmtNodes().back().GetOpCode() == OP_throw) {
                continue;
            }
            if (!bb->GetAttributes(kBBAttrIsTry) && !bb->GetAttributes(kBBAttrWontExit)) {
                DEBUG_ASSERT(bb->GetStmtNodes().rbegin().base().d() != nullptr, "runtime check error");
                DEBUG_ASSERT(bb->GetSucc().size() == 1, "runtime check error");
            }
            DEBUG_ASSERT(bb->GetSucc(0)->GetBBLabel() == static_cast<GotoNode &>(bb->GetStmtNodes().back()).GetOffset(),
                         "runtime check error");
        }
    }
}

// check that all the target labels in jump statements are defined
void MeCFG::VerifyLabels() const
{
    auto eIt = valid_end();
    for (auto bIt = valid_begin(); bIt != eIt; ++bIt) {
        BB *mirBB = *bIt;
        auto &stmtNodes = mirBB->GetStmtNodes();
        if (stmtNodes.rbegin().base().d() == nullptr) {
            continue;
        }
        if (mirBB->GetKind() == kBBGoto) {
            if (stmtNodes.back().GetOpCode() == OP_throw) {
                continue;
            }
            DEBUG_ASSERT(GetLabelBBAt(static_cast<GotoNode &>(stmtNodes.back()).GetOffset())->GetBBLabel() ==
                             static_cast<GotoNode &>(stmtNodes.back()).GetOffset(),
                         "undefined label in goto");
        } else if (mirBB->GetKind() == kBBCondGoto) {
            DEBUG_ASSERT(GetLabelBBAt(static_cast<CondGotoNode &>(stmtNodes.back()).GetOffset())->GetBBLabel() ==
                             static_cast<CondGotoNode &>(stmtNodes.back()).GetOffset(),
                         "undefined label in conditional branch");
        } else if (mirBB->GetKind() == kBBSwitch) {
            auto &switchStmt = static_cast<SwitchNode &>(stmtNodes.back());
            LabelIdx targetLabIdx = switchStmt.GetDefaultLabel();
            [[maybe_unused]] BB *bb = GetLabelBBAt(targetLabIdx);
            DEBUG_ASSERT(bb->GetBBLabel() == targetLabIdx, "undefined label in switch");
            for (size_t j = 0; j < switchStmt.GetSwitchTable().size(); ++j) {
                targetLabIdx = switchStmt.GetCasePair(j).second;
                bb = GetLabelBBAt(targetLabIdx);
                DEBUG_ASSERT(bb->GetBBLabel() == targetLabIdx, "undefined switch target label");
            }
        }
    }
}

void MeCFG::Dump() const
{
    // BSF Dump the cfg
    LogInfo::MapleLogger() << "####### CFG Dump: ";
    DEBUG_ASSERT(NumBBs() != 0, "size to be allocated is 0");
    auto *visitedMap = static_cast<bool *>(calloc(NumBBs(), sizeof(bool)));
    if (visitedMap != nullptr) {
        std::queue<BB *> qu;
        qu.push(GetFirstBB());
        while (!qu.empty()) {
            BB *bb = qu.front();
            qu.pop();
            if (bb == nullptr) {
                continue;
            }
            BBId id = bb->GetBBId();
            if (visitedMap[static_cast<long>(id)]) {
                continue;
            }
            LogInfo::MapleLogger() << id << " ";
            visitedMap[static_cast<long>(id)] = true;
            auto it = bb->GetSucc().begin();
            while (it != bb->GetSucc().end()) {
                BB *kidBB = *it;
                if (!visitedMap[static_cast<long>(kidBB->GetBBId())]) {
                    qu.push(kidBB);
                }
                ++it;
            }
        }
        LogInfo::MapleLogger() << '\n';
        free(visitedMap);
    }
}

// replace special char in FunctionName for output file
static void ReplaceFilename(std::string &fileName)
{
    for (char &c : fileName) {
        if (c == ';' || c == '/' || c == '|') {
            c = '_';
        }
    }
}

std::string MeCFG::ConstructFileNameToDump(const std::string &prefix) const
{
    std::string fileName;
    if (!prefix.empty()) {
        fileName.append(prefix);
        fileName.append("-");
    }
    // the func name length may exceed OS's file name limit, so truncate after 80 chars
    if (func.GetName().size() <= kFuncNameLenLimit) {
        fileName.append(func.GetName());
    } else {
        fileName.append(func.GetName().c_str(), kFuncNameLenLimit);
    }
    ReplaceFilename(fileName);
    fileName.append(".dot");
    return fileName;
}

BB *MeCFG::NewBasicBlock()
{
    BB *newBB = memPool->New<BB>(&mecfgAlloc, &func.GetVersAlloc(), BBId(nextBBId++));
    bbVec.push_back(newBB);
    return newBB;
}

// new a basic block and insert before position or after position
BB &MeCFG::InsertNewBasicBlock(const BB &position, bool isInsertBefore)
{
    BB *newBB = memPool->New<BB>(&mecfgAlloc, &func.GetVersAlloc(), BBId(nextBBId++));

    auto bIt = std::find(begin(), end(), &position);
    auto idx = position.GetBBId();
    if (!isInsertBefore) {
        ++bIt;
        ++idx;
    }
    auto newIt = bbVec.insert(bIt, newBB);
    auto eIt = end();
    // update bb's idx
    for (auto it = newIt; it != eIt; ++it) {
        if ((*it) != nullptr) {
            (*it)->SetBBId(BBId(idx));
        }
        ++idx;
    }
    return *newBB;
}

void MeCFG::DeleteBasicBlock(const BB &bb)
{
    DEBUG_ASSERT(bbVec[bb.GetBBId()] == &bb, "runtime check error");
    /* update first_bb_ and last_bb if needed */
    bbVec.at(bb.GetBBId()) = nullptr;
}

/* get next bb in bbVec */
BB *MeCFG::NextBB(const BB *bb)
{
    auto bbIt = std::find(begin(), end(), bb);
    CHECK_FATAL(bbIt != end(), "bb must be inside bb_vec");
    for (auto it = ++bbIt; it != end(); ++it) {
        if (*it != nullptr) {
            return *it;
        }
    }
    return nullptr;
}

/* get prev bb in bbVec */
BB *MeCFG::PrevBB(const BB *bb)
{
    auto bbIt = std::find(rbegin(), rend(), bb);
    CHECK_FATAL(bbIt != rend(), "bb must be inside bb_vec");
    for (auto it = ++bbIt; it != rend(); ++it) {
        if (*it != nullptr) {
            return *it;
        }
    }
    return nullptr;
}

void MeCFG::SetTryBlockInfo(const StmtNode *nextStmt, StmtNode *tryStmt, BB *lastTryBB, BB *curBB, BB *newBB)
{
    if (nextStmt->GetOpCode() == OP_endtry) {
        curBB->SetAttributes(kBBAttrIsTryEnd);
        DEBUG_ASSERT(lastTryBB != nullptr, "null ptr check");
        endTryBB2TryBB[curBB] = lastTryBB;
    } else {
        newBB->SetAttributes(kBBAttrIsTry);
        bbTryNodeMap[newBB] = tryStmt;
    }
}

void MeCFG::BuildSCCDFS(BB &bb, uint32 &visitIndex, std::vector<SCCOfBBs *> &sccNodes,
                        std::vector<uint32> &visitedOrder, std::vector<uint32> &lowestOrder, std::vector<bool> &inStack,
                        std::stack<uint32> &visitStack)
{
    uint32 id = bb.UintID();
    visitedOrder[id] = visitIndex;
    lowestOrder[id] = visitIndex;
    ++visitIndex;
    visitStack.push(id);
    inStack[id] = true;

    for (BB *succ : bb.GetSucc()) {
        if (succ == nullptr) {
            continue;
        }
        uint32 succId = succ->UintID();
        if (!visitedOrder[succId]) {
            BuildSCCDFS(*succ, visitIndex, sccNodes, visitedOrder, lowestOrder, inStack, visitStack);
            if (lowestOrder[succId] < lowestOrder[id]) {
                lowestOrder[id] = lowestOrder[succId];
            }
        } else if (inStack[succId]) {
            (void)backEdges.emplace(std::pair<uint32, uint32>(id, succId));
            if (visitedOrder[succId] < lowestOrder[id]) {
                lowestOrder[id] = visitedOrder[succId];
            }
        }
    }

    if (visitedOrder.at(id) == lowestOrder.at(id)) {
        auto *sccNode = GetAlloc().GetMemPool()->New<SCCOfBBs>(numOfSCCs++, &bb, &GetAlloc());
        uint32 stackTopId;
        do {
            stackTopId = visitStack.top();
            visitStack.pop();
            inStack[stackTopId] = false;
            auto *topBB = static_cast<BB *>(GetAllBBs()[stackTopId]);
            sccNode->AddBBNode(topBB);
            sccOfBB[stackTopId] = sccNode;
        } while (stackTopId != id);

        sccNodes.push_back(sccNode);
    }
}

void MeCFG::VerifySCC()
{
    for (BB *bb : GetAllBBs()) {
        if (bb == nullptr || bb == GetCommonExitBB()) {
            continue;
        }
        SCCOfBBs *scc = sccOfBB.at(bb->UintID());
        CHECK_FATAL(scc != nullptr, "bb should belong to a scc");
    }
}

void MeCFG::SCCTopologicalSort(std::vector<SCCOfBBs *> &sccNodes)
{
    std::set<SCCOfBBs *> inQueue;
    for (SCCOfBBs *node : sccNodes) {
        if (!node->HasPred()) {
            sccTopologicalVec.push_back(node);
            (void)inQueue.insert(node);
        }
    }

    // Top-down iterates all nodes
    for (size_t i = 0; i < sccTopologicalVec.size(); ++i) {
        SCCOfBBs *sccBB = sccTopologicalVec[i];
        for (SCCOfBBs *succ : sccBB->GetSucc()) {
            if (inQueue.find(succ) == inQueue.end()) {
                // successor has not been visited
                bool predAllVisited = true;
                // check whether all predecessors of the current successor have been visited
                for (SCCOfBBs *pred : succ->GetPred()) {
                    if (inQueue.find(pred) == inQueue.end()) {
                        predAllVisited = false;
                        break;
                    }
                }
                if (predAllVisited) {
                    sccTopologicalVec.push_back(succ);
                    (void)inQueue.insert(succ);
                }
            }
        }
    }
}

// BBId is index of BB in the bbVec, so we should swap two BB's pos in bbVec, if we want to swap their BBId.
void MeCFG::SwapBBId(BB &bb1, BB &bb2)
{
    bbVec[bb1.GetBBId()] = &bb2;
    bbVec[bb2.GetBBId()] = &bb1;
    BBId tmp = bb1.GetBBId();
    bb1.SetBBId(bb2.GetBBId());
    bb2.SetBBId(tmp);
}

// set bb succ frequency from bb freq
// no critical edge is expected
void MeCFG::ConstructEdgeFreqFromBBFreq()
{
    // set succfreqs
    auto eIt = valid_end();
    for (auto bIt = valid_begin(); bIt != eIt; ++bIt) {
        auto *bb = *bIt;
        if (!bb)
            continue;
        if (bb->GetSucc().size() == 1) {
            bb->PushBackSuccFreq(bb->GetFrequency());
        } else if (bb->GetSucc().size() == 2) { // bb has 2 succeed
            auto *fallthru = bb->GetSucc(0);
            auto *targetBB = bb->GetSucc(1);
            if (fallthru->GetPred().size() == 1) {
                auto succ0Freq = fallthru->GetFrequency();
                bb->PushBackSuccFreq(succ0Freq);
                DEBUG_ASSERT(bb->GetFrequency() >= succ0Freq, "sanity check");
                bb->PushBackSuccFreq(bb->GetFrequency() - succ0Freq);
            } else if (targetBB->GetPred().size() == 1) {
                auto succ1Freq = targetBB->GetFrequency();
                DEBUG_ASSERT(bb->GetFrequency() >= succ1Freq, "sanity check");
                bb->PushBackSuccFreq(bb->GetFrequency() - succ1Freq);
                bb->PushBackSuccFreq(succ1Freq);
            } else {
                CHECK_FATAL(false, "ConstructEdgeFreqFromBBFreq::NYI critical edge");
            }
        } else if (bb->GetSucc().size() > 2) { // bb has 2 succeed
            // switch case, no critical edge is supposted
            for (size_t i = 0; i < bb->GetSucc().size(); ++i) {
                bb->PushBackSuccFreq(bb->GetSucc(i)->GetFrequency());
            }
        }
    }
}

// set bb frequency from stmt record
void MeCFG::ConstructBBFreqFromStmtFreq()
{
    GcovFuncInfo *funcData = func.GetMirFunc()->GetFuncProfData();
    if (!funcData)
        return;
    if (funcData->stmtFreqs.size() == 0)
        return;
    auto eIt = valid_end();
    for (auto bIt = valid_begin(); bIt != eIt; ++bIt) {
        if ((*bIt)->IsEmpty())
            continue;
        StmtNode &first = (*bIt)->GetFirst();
        if (funcData->stmtFreqs.count(first.GetStmtID()) > 0) {
            (*bIt)->SetFrequency(funcData->stmtFreqs[first.GetStmtID()]);
        } else if (funcData->stmtFreqs.count((*bIt)->GetLast().GetStmtID()) > 0) {
            (*bIt)->SetFrequency(funcData->stmtFreqs[(*bIt)->GetLast().GetStmtID()]);
        } else {
            LogInfo::MapleLogger() << "ERROR::  bb " << (*bIt)->GetBBId() << "frequency is not set"
                                   << "\n";
            DEBUG_ASSERT(0, "no freq set");
        }
    }
    // add common entry and common exit
    auto *bb = *common_entry();
    uint64_t freq = 0;
    for (size_t i = 0; i < bb->GetSucc().size(); ++i) {
        freq += bb->GetSucc(i)->GetFrequency();
    }
    bb->SetFrequency(freq);
    bb = *common_exit();
    freq = 0;
    for (size_t i = 0; i < bb->GetPred().size(); ++i) {
        freq += bb->GetPred(i)->GetFrequency();
    }
    bb->SetFrequency(freq);
    // set succfreqs
    ConstructEdgeFreqFromBBFreq();
    // clear stmtFreqs since cfg frequency is create
    funcData->stmtFreqs.clear();
}

void MeCFG::ConstructStmtFreq()
{
    GcovFuncInfo *funcData = func.GetMirFunc()->GetFuncProfData();
    if (!funcData)
        return;
    auto eIt = valid_end();
    // clear stmtFreqs
    funcData->stmtFreqs.clear();
    for (auto bIt = valid_begin(); bIt != eIt; ++bIt) {
        auto *bb = *bIt;
        if (bIt == common_entry()) {
            funcData->entry_freq = bb->GetFrequency();
            funcData->real_entryfreq = funcData->entry_freq;
        }
        for (auto &stmt : bb->GetStmtNodes()) {
            Opcode op = stmt.GetOpCode();
            // record bb start/end stmt
            if (stmt.GetStmtID() == bb->GetFirst().GetStmtID() || stmt.GetStmtID() == bb->GetLast().GetStmtID() ||
                IsCallAssigned(op) || op == OP_call) {
                funcData->stmtFreqs[stmt.GetStmtID()] = bb->GetFrequency();
            }
        }
    }
}

void MeCFG::VerifyBBFreq()
{
    for (size_t i = 2; i < bbVec.size(); ++i) {  // skip common entry and common exit
        auto *bb = bbVec[i];
        if (bb == nullptr || bb->GetAttributes(kBBAttrIsEntry) || bb->GetAttributes(kBBAttrIsExit)) {
            continue;
        }
        // wontexit bb may has wrong succ, skip it
        if (bb->GetSuccFreq().size() != bb->GetSucc().size() && !bb->GetAttributes(kBBAttrWontExit)) {
            CHECK_FATAL(false, "VerifyBBFreq: succFreq size != succ size");
        }
        // bb freq == sum(out edge freq)
        uint64 succSumFreq = 0;
        for (auto succFreq : bb->GetSuccFreq()) {
            succSumFreq += succFreq;
        }
        if (succSumFreq != bb->GetFrequency()) {
            LogInfo::MapleLogger() << "[VerifyFreq failure] BB" << bb->GetBBId() << " freq: " << bb->GetFrequency()
                                   << ", all succ edge freq sum: " << succSumFreq << std::endl;
            LogInfo::MapleLogger() << func.GetName() << std::endl;
            CHECK_FATAL(false, "VerifyFreq failure: bb freq != succ freq sum");
        }
    }
}
}  // namespace maple
