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

bool MeCFG::FindUse(const StmtNode &stmt, StIdx stIdx) const
{
    Opcode opcode = stmt.GetOpCode();
    switch (opcode) {
        case OP_call:
        case OP_virtualcall:
        case OP_virtualicall:
        case OP_superclasscall:
        case OP_interfacecall:
        case OP_interfaceicall:
        case OP_customcall:
        case OP_polymorphiccall:
        case OP_icall:
        case OP_icallproto:
        case OP_intrinsiccall:
        case OP_xintrinsiccall:
        case OP_intrinsiccallwithtype:
        case OP_callassigned:
        case OP_virtualcallassigned:
        case OP_virtualicallassigned:
        case OP_superclasscallassigned:
        case OP_interfacecallassigned:
        case OP_interfaceicallassigned:
        case OP_customcallassigned:
        case OP_polymorphiccallassigned:
        case OP_icallassigned:
        case OP_icallprotoassigned:
        case OP_intrinsiccallassigned:
        case OP_xintrinsiccallassigned:
        case OP_intrinsiccallwithtypeassigned:
        case OP_asm:
        case OP_syncenter:
        case OP_syncexit: {
            for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
                BaseNode *argExpr = stmt.Opnd(i);
                if (FindExprUse(*argExpr, stIdx)) {
                    return true;
                }
            }
            break;
        }
        case OP_dassign: {
            const auto &dNode = static_cast<const DassignNode &>(stmt);
            return FindExprUse(*dNode.GetRHS(), stIdx);
        }
        case OP_regassign: {
            const auto &rNode = static_cast<const RegassignNode &>(stmt);
            if (rNode.GetRegIdx() < 0) {
                return false;
            }
            return FindExprUse(*rNode.Opnd(0), stIdx);
        }
        case OP_iassign: {
            const auto &iNode = static_cast<const IassignNode &>(stmt);
            if (FindExprUse(*iNode.Opnd(0), stIdx)) {
                return true;
            } else {
                return FindExprUse(*iNode.GetRHS(), stIdx);
            }
        }
            CASE_OP_ASSERT_NONNULL
        case OP_eval:
        case OP_free:
        case OP_switch: {
            BaseNode *argExpr = stmt.Opnd(0);
            return FindExprUse(*argExpr, stIdx);
        }
        default:
            break;
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

// Return true if there is no use or def of sym betweent from to to.
bool MeCFG::HasNoOccBetween(StmtNode &from, const StmtNode &to, StIdx stIdx) const
{
    for (StmtNode *stmt = &from; stmt && stmt != &to; stmt = stmt->GetNext()) {
        if (FindUse(*stmt, stIdx) || FindDef(*stmt, stIdx)) {
            return false;
        }
    }
    return true;
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

void MeCFG::CreateBasicBlocks()
{
    if (func.CurFunction()->IsEmpty()) {
        if (!MeOption::quiet) {
            LogInfo::MapleLogger() << "function is empty, cfg is nullptr\n";
        }
        return;
    }
    // create common_entry/exit bb first as bbVec[0] and bb_vec[1]
    auto *commonEntryBB = NewBasicBlock();
    commonEntryBB->SetAttributes(kBBAttrIsEntry);
    auto *commonExitBB = NewBasicBlock();
    commonExitBB->SetAttributes(kBBAttrIsExit);
    auto *firstBB = NewBasicBlock();
    firstBB->SetAttributes(kBBAttrIsEntry);
    StmtNode *nextStmt = func.CurFunction()->GetBody()->GetFirst();
    DEBUG_ASSERT(nextStmt != nullptr, "function has no statement");
    BB *curBB = firstBB;
    StmtNode *tryStmt = nullptr;  // record current try stmt for map<bb, try_stmt>
    BB *lastTryBB = nullptr;      // bb containing try_stmt
    do {
        StmtNode *stmt = nextStmt;
        nextStmt = stmt->GetNext();
        switch (stmt->GetOpCode()) {
            case OP_goto: {
                if (curBB->IsEmpty()) {
                    curBB->SetFirst(stmt);
                }
                curBB->SetLast(stmt);
                curBB->SetKind(kBBGoto);
                if (nextStmt != nullptr) {
                    BB *newBB = NewBasicBlock();
                    if (tryStmt != nullptr) {
                        SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
                    }
                    curBB = newBB;
                }
                break;
            }
            case OP_igoto: {
                if (curBB->IsEmpty()) {
                    curBB->SetFirst(stmt);
                }
                curBB->SetLast(stmt);
                curBB->SetKind(kBBIgoto);
                if (nextStmt != nullptr) {
                    curBB = NewBasicBlock();
                }
                break;
            }
            case OP_dassign: {
                DassignNode *dass = static_cast<DassignNode *>(stmt);
                // delete identity assignments inserted by LFO
                if (dass->GetRHS()->GetOpCode() == OP_dread) {
                    DreadNode *dread = static_cast<DreadNode *>(dass->GetRHS());
                    if (dass->GetStIdx() == dread->GetStIdx() && dass->GetFieldID() == dread->GetFieldID()) {
                        func.CurFunction()->GetBody()->RemoveStmt(stmt);
                        break;
                    }
                }
                if (curBB->IsEmpty()) {
                    curBB->SetFirst(stmt);
                }
                if ((nextStmt == nullptr) && to_ptr(curBB->GetStmtNodes().rbegin()) == nullptr) {
                    curBB->SetLast(stmt);
                }
                break;
            }
            case OP_brfalse:
            case OP_brtrue: {
                if (curBB->IsEmpty()) {
                    curBB->SetFirst(stmt);
                }
                curBB->SetLast(stmt);
                curBB->SetKind(kBBCondGoto);
                BB *newBB = NewBasicBlock();
                if (tryStmt != nullptr) {
                    SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
                }
                curBB = newBB;
                break;
            }
            case OP_if:
            case OP_doloop:
            case OP_dowhile:
            case OP_while: {
                DEBUG_ASSERT(false, "not yet implemented");
                break;
            }
            case OP_throw:
                if (tryStmt != nullptr) {
                    // handle as goto
                    if (curBB->IsEmpty()) {
                        curBB->SetFirst(stmt);
                    }
                    curBB->SetLast(stmt);
                    curBB->SetKind(kBBGoto);
                    if (nextStmt != nullptr) {
                        BB *newBB = NewBasicBlock();
                        SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
                        curBB = newBB;
                    }
                    break;
                }
                // fall thru to handle as return
                [[clang::fallthrough]];
            case OP_gosub:
            case OP_retsub:
            case OP_return: {
                if (curBB->IsEmpty()) {
                    curBB->SetFirst(stmt);
                }
                curBB->SetLast(stmt);
                curBB->SetKindReturn();
                if (nextStmt != nullptr) {
                    BB *newBB = NewBasicBlock();
                    if (tryStmt != nullptr) {
                        SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
                    }
                    curBB = newBB;
                    if (stmt->GetOpCode() == OP_gosub) {
                        curBB->SetAttributes(kBBAttrIsEntry);
                    }
                }
                break;
            }
            case OP_endtry:
                if (curBB->IsEmpty()) {
                    curBB->SetFirst(stmt);
                }
                if ((nextStmt == nullptr) && (curBB->GetStmtNodes().rbegin().base().d() == nullptr)) {
                    curBB->SetLast(stmt);
                }
                break;
            case OP_try: {
                // start a new bb or with a label
                if (!curBB->IsEmpty()) {
                    // prepare a new bb
                    StmtNode *lastStmt = stmt->GetPrev();
                    DEBUG_ASSERT(curBB->GetStmtNodes().rbegin().base().d() == nullptr ||
                                     curBB->GetStmtNodes().rbegin().base().d() == lastStmt,
                                 "something wrong building BB");
                    curBB->SetLast(lastStmt);
                    if (curBB->GetKind() == kBBUnknown) {
                        curBB->SetKind(kBBFallthru);
                    }
                    BB *newBB = NewBasicBlock();
                    // assume no nested try, so no need to call SetTryBlockInfo()
                    curBB = newBB;
                }
                curBB->SetFirst(stmt);
                tryStmt = stmt;
                lastTryBB = curBB;
                curBB->SetAttributes(kBBAttrIsTry);
                bbTryNodeMap[curBB] = tryStmt;
                // prepare a new bb that contains only a OP_try. It is needed for
                // dse to work correctly: assignments in a try block should not affect
                // assignments before the try block as exceptions might occur.
                curBB->SetLast(stmt);
                curBB->SetKind(kBBFallthru);
                BB *newBB = NewBasicBlock();
                SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
                curBB = newBB;
                break;
            }
            case OP_catch: {
                // start a new bb or with a label
                if (!curBB->IsEmpty()) {
                    // prepare a new bb
                    StmtNode *lastStmt = stmt->GetPrev();
                    DEBUG_ASSERT(curBB->GetStmtNodes().rbegin().base().d() == nullptr ||
                                     curBB->GetStmtNodes().rbegin().base().d() == lastStmt,
                                 "something wrong building BB");
                    curBB->SetLast(lastStmt);
                    if (curBB->GetKind() == kBBUnknown) {
                        curBB->SetKind(kBBFallthru);
                    }
                    BB *newBB = NewBasicBlock();
                    if (tryStmt != nullptr) {
                        SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
                    }
                    curBB = newBB;
                }
                curBB->SetFirst(stmt);
                curBB->SetAttributes(kBBAttrIsCatch);
                break;
            }
            case OP_label: {
                auto *labelNode = static_cast<LabelNode *>(stmt);
                LabelIdx labelIdx = labelNode->GetLabelIdx();
                if (func.IsPme() && curBB == firstBB && curBB->IsEmpty()) {
                    // when function starts with a label, need to insert dummy BB as entry
                    curBB = NewBasicBlock();
                }
                if (!curBB->IsEmpty() || curBB->GetBBLabel() != 0) {
                    // prepare a new bb
                    StmtNode *lastStmt = stmt->GetPrev();
                    DEBUG_ASSERT((curBB->GetStmtNodes().rbegin().base().d() == nullptr ||
                                  curBB->GetStmtNodes().rbegin().base().d() == lastStmt),
                                 "something wrong building BB");
                    if (curBB->GetStmtNodes().rbegin().base().d() == nullptr && (lastStmt->GetOpCode() != OP_label)) {
                        curBB->SetLast(lastStmt);
                    }
                    if (curBB->GetKind() == kBBUnknown) {
                        curBB->SetKind(kBBFallthru);
                    }
                    BB *newBB = NewBasicBlock();
                    if (tryStmt != nullptr) {
                        newBB->SetAttributes(kBBAttrIsTry);
                        SetBBTryNodeMap(*newBB, *tryStmt);
                    }
                    curBB = newBB;
                } else if (func.GetPreMeFunc() && (func.GetPreMeFunc()->label2WhileInfo.find(labelIdx) !=
                                                   func.GetPreMeFunc()->label2WhileInfo.end())) {
                    curBB->SetKind(kBBFallthru);
                    BB *newBB = NewBasicBlock();
                    if (tryStmt != nullptr) {
                        newBB->SetAttributes(kBBAttrIsTry);
                        SetBBTryNodeMap(*newBB, *tryStmt);
                    }
                    curBB = newBB;
                }
                labelBBIdMap[labelIdx] = curBB;
                curBB->SetBBLabel(labelIdx);
                // label node is not real node in bb, get frequency information to bb
                if (Options::profileUse && func.GetMirFunc()->GetFuncProfData()) {
                    auto freq = func.GetMirFunc()->GetFuncProfData()->GetStmtFreq(stmt->GetStmtID());
                    if (freq >= 0) {
                        curBB->SetFrequency(freq);
                    }
                }
                break;
            }
            case OP_jscatch: {
                if (curBB->IsEmpty()) {
                    curBB->SetFirst(stmt);
                }
                curBB->SetAttributes(kBBAttrIsEntry);
                curBB->SetAttributes(kBBAttrIsJSCatch);
                break;
            }
            case OP_finally: {
                DEBUG_ASSERT(curBB->GetStmtNodes().empty(), "runtime check error");
                curBB->SetFirst(stmt);
                curBB->SetAttributes(kBBAttrIsEntry);
                curBB->SetAttributes(kBBAttrIsJSFinally);
                break;
            }
            case OP_switch: {
                if (curBB->IsEmpty()) {
                    curBB->SetFirst(stmt);
                }
                curBB->SetLast(stmt);
                curBB->SetKind(kBBSwitch);
                BB *newBB = NewBasicBlock();
                if (tryStmt != nullptr) {
                    SetTryBlockInfo(nextStmt, tryStmt, lastTryBB, curBB, newBB);
                }
                curBB = newBB;
                break;
            }
            default: {
                if (curBB->IsEmpty()) {
                    curBB->SetFirst(stmt);
                }
                if ((nextStmt == nullptr) && (curBB->GetStmtNodes().rbegin().base().d() == nullptr)) {
                    curBB->SetLast(stmt);
                }
                break;
            }
        }
    } while (nextStmt != nullptr);
    DEBUG_ASSERT(tryStmt == nullptr, "unclosed try");      // tryandendtry should be one-one mapping
    DEBUG_ASSERT(lastTryBB == nullptr, "unclosed tryBB");  // tryandendtry should be one-one mapping
    auto *lastBB = curBB;
    if (lastBB->IsEmpty()) {
        // insert a return statement
        lastBB->SetFirst(func.GetMIRModule().GetMIRBuilder()->CreateStmtReturn(nullptr));
        lastBB->SetLast(lastBB->GetStmtNodes().begin().d());
        lastBB->SetKindReturn();
    } else if (lastBB->GetKind() == kBBUnknown) {
        lastBB->SetKindReturn();
        lastBB->SetAttributes(kBBAttrIsExit);
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
