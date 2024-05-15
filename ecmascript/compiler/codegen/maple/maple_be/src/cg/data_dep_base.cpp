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

#include "data_dep_base.h"

namespace maplebe {
void DataDepBase::ProcessNonMachineInsn(Insn &insn, MapleVector<Insn *> &comments, MapleVector<DepNode *> &dataNodes,
                                        const Insn *&locInsn)
{
    CHECK_FATAL(!insn.IsMachineInstruction(), "need non-machine-instruction");
    if (insn.IsImmaterialInsn()) {
        if (!insn.IsComment()) {
            locInsn = &insn;
        } else {
            (void)comments.emplace_back(&insn);
        }
    } else if (insn.IsCfiInsn()) {
        if (!dataNodes.empty()) {
            dataNodes.back()->AddCfiInsn(insn);
        }
    }
}

void DataDepBase::BuildDepsLastCallInsn(Insn &insn)
{
    Insn *lastCallInsn = curCDGNode->GetLastCallInsn();
    if (lastCallInsn != nullptr && !lastCallInsn->IsSpecialCall()) {
        AddDependence(*lastCallInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeControl);
    }
    if (insn.IsCall() || insn.IsTailCall()) {
        curCDGNode->SetLastCallInsn(&insn);
    }
}

void DataDepBase::BuildAmbiInsnDependency(Insn &insn)
{
    const auto &defRegnos = insn.GetDepNode()->GetDefRegnos();
    for (const auto &regNO : defRegnos) {
        if (IfInAmbiRegs(regNO)) {
            break;
        }
    }
}

// Build data dependence between control register and last call instruction.
// insn : instruction that with control register operand.
// isDest : if the control register operand is a destination operand.
void DataDepBase::BuildDepsBetweenControlRegAndCall(Insn &insn, bool isDest)
{
    Insn *lastCallInsn = curCDGNode->GetLastCallInsn();
    if (lastCallInsn == nullptr) {
        return;
    }
    if (isDest) {
        AddDependence(*lastCallInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeOutput);
        return;
    }
    AddDependence(*lastCallInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeAnti);
}

// Build control data dependence for branch/ret instructions
void DataDepBase::BuildDepsControlAll(Insn &insn, const MapleVector<DepNode *> &nodes)
{
    DepNode *depNode = insn.GetDepNode();
    // For rest instructions, we build control dependency
    for (auto *dataNode : nodes) {
        AddDependence(*dataNode, *depNode, kDependenceTypeControl);
    }
}

// Build data dependence of destination register operand
void DataDepBase::BuildDepsDefReg(Insn &insn, regno_t regNO)
{
    DepNode *node = insn.GetDepNode();
    node->AddDefReg(regNO);
    // 1. For building intra-block data dependence, only require the data flow info of the curBB(cur CDGNode)
    // 2. For building inter-block data dependence, require the data flow info of all BBs on the pred path in CFG
    // Build anti dependence
    if (isIntra || curRegion->GetRegionNodeSize() == 1 || curRegion->GetRegionRoot() == curCDGNode) {
        RegList *regList = curCDGNode->GetUseInsnChain(regNO);
        while (regList != nullptr) {
            CHECK_NULL_FATAL(regList->insn);
            AddDependence(*regList->insn->GetDepNode(), *node, kDependenceTypeAnti);
            regList = regList->next;
        }
    } else if (curRegion->GetRegionRoot() != curCDGNode) {
        BuildInterBlockDefUseDependency(*node, regNO, kDependenceTypeAnti, false);
    }

    // Build output dependence
    // Build intra block data dependence
    Insn *defInsn = curCDGNode->GetLatestDefInsn(regNO);
    if (defInsn != nullptr) {
        AddDependence(*defInsn->GetDepNode(), *node, kDependenceTypeOutput);
    } else if (!isIntra && curRegion->GetRegionRoot() != curCDGNode) {
        // Build inter block data dependence
        BuildInterBlockDefUseDependency(*node, regNO, kDependenceTypeOutput, true);
    }
}

// Build data dependence of source register operand
void DataDepBase::BuildDepsUseReg(Insn &insn, regno_t regNO)
{
    DepNode *node = insn.GetDepNode();
    node->AddUseReg(regNO);

    // Build intra block data dependence
    Insn *defInsn = curCDGNode->GetLatestDefInsn(regNO);
    if (defInsn != nullptr) {
        AddDependence(*defInsn->GetDepNode(), *node, kDependenceTypeTrue);
    } else if (!isIntra && curRegion->GetRegionRoot() != curCDGNode) {
        // Build inter block data dependence
        BuildInterBlockDefUseDependency(*node, regNO, kDependenceTypeTrue, true);
    }
}

// For inter data dependence analysis
void DataDepBase::BuildInterBlockDefUseDependency(DepNode &curDepNode, regno_t regNO, DepType depType, bool isDef)
{
    CHECK_FATAL(!isIntra, "must be inter block data dependence analysis");
    CHECK_FATAL(curRegion->GetRegionRoot() != curCDGNode, "for the root node, cross-BB search is not required");
    BB *curBB = curCDGNode->GetBB();
    CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
    std::vector<bool> visited(curRegion->GetMaxBBIdInRegion() + 1, false);
    if (isDef) {
        BuildPredPathDefDependencyDFS(*curBB, visited, curDepNode, regNO, depType);
    } else {
        BuildPredPathUseDependencyDFS(*curBB, visited, curDepNode, regNO, depType);
    }
}

void DataDepBase::BuildPredPathDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode, regno_t regNO,
                                                DepType depType)
{
    if (visited[curBB.GetId()]) {
        return;
    }
    CDGNode *cdgNode = curBB.GetCDGNode();
    CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
    CDGRegion *region = cdgNode->GetRegion();
    CHECK_FATAL(region != nullptr, "get region from cdgNode failed");
    if (region->GetRegionId() != curRegion->GetRegionId()) {
        return;
    }
    Insn *curDefInsn = cdgNode->GetLatestDefInsn(regNO);
    if (curDefInsn != nullptr) {
        visited[curBB.GetId()] = true;
        AddDependence(*curDefInsn->GetDepNode(), depNode, depType);
        return;
    }
    // Ignore back-edge
    if (cdgNode == curRegion->GetRegionRoot()) {
        return;
    }
    for (auto predIt = curBB.GetPredsBegin(); predIt != curBB.GetPredsEnd(); ++predIt) {
        // Ignore back-edge of self-loop
        if (*predIt != &curBB) {
            BuildPredPathDefDependencyDFS(**predIt, visited, depNode, regNO, depType);
        }
    }
}

void DataDepBase::BuildPredPathUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode, regno_t regNO,
                                                DepType depType)
{
    if (visited[curBB.GetId()]) {
        return;
    }
    CDGNode *cdgNode = curBB.GetCDGNode();
    CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
    CDGRegion *region = cdgNode->GetRegion();
    CHECK_FATAL(region != nullptr, "get region from cdgNode failed");
    if (region->GetRegionId() != curRegion->GetRegionId()) {
        return;
    }
    visited[curBB.GetId()] = true;
    RegList *useChain = cdgNode->GetUseInsnChain(regNO);
    while (useChain != nullptr) {
        Insn *useInsn = useChain->insn;
        CHECK_FATAL(useInsn != nullptr, "get useInsn failed");
        AddDependence(*useInsn->GetDepNode(), depNode, depType);
        useChain = useChain->next;
    }
    // Ignore back-edge
    if (cdgNode == curRegion->GetRegionRoot()) {
        return;
    }
    for (auto predIt = curBB.GetPredsBegin(); predIt != curBB.GetPredsEnd(); ++predIt) {
        // Ignore back-edge of self-loop
        if (*predIt != &curBB) {
            BuildPredPathUseDependencyDFS(**predIt, visited, depNode, regNO, depType);
        }
    }
}

void DataDepBase::BuildInterBlockSpecialDataInfoDependency(DepNode &curDepNode, bool needCmp, DepType depType,
                                                           DataDepBase::DataFlowInfoType infoType)
{
    CHECK_FATAL(!isIntra, "must be inter block data dependence analysis");
    CHECK_FATAL(curRegion->GetRegionRoot() != curCDGNode, "for the root node, cross-BB search is not required");
    BB *curBB = curCDGNode->GetBB();
    CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
    std::vector<bool> visited(curRegion->GetMaxBBIdInRegion() + 1, false);
    BuildPredPathSpecialDataInfoDependencyDFS(*curBB, visited, needCmp, curDepNode, depType, infoType);
}

// Generate a data depNode.
// insn  : create depNode for the instruction.
// nodes : a vector to store depNode.
// nodeSum  : the new depNode's index.
// comments : those comment insn between last no-comment's insn and insn.
DepNode *DataDepBase::GenerateDepNode(Insn &insn, MapleVector<DepNode *> &nodes, uint32 &nodeSum,
                                      MapleVector<Insn *> &comments)
{
    Reservation *rev = mad.FindReservation(insn);
    DEBUG_ASSERT(rev != nullptr, "get reservation of insn failed");
    auto *depNode = memPool.New<DepNode>(insn, alloc, rev->GetUnit(), rev->GetUnitNum(), *rev);
    depNode->SetIndex(nodeSum++);
    (void)nodes.emplace_back(depNode);
    insn.SetDepNode(*depNode);

    constexpr size_t vectorSize = 5;
    depNode->ReservePreds(vectorSize);
    depNode->ReserveSuccs(vectorSize);

    if (!comments.empty()) {
        depNode->SetComments(comments);
        comments.clear();
    }
    return depNode;
}

void DataDepBase::BuildPredPathSpecialDataInfoDependencyDFS(BB &curBB, std::vector<bool> &visited, bool needCmp,
                                                            DepNode &depNode, DepType depType,
                                                            DataDepBase::DataFlowInfoType infoType)
{
    if (visited[curBB.GetId()]) {
        return;
    }
    CDGNode *cdgNode = curBB.GetCDGNode();
    CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
    CDGRegion *region = cdgNode->GetRegion();
    CHECK_FATAL(region != nullptr, "get region from cdgNode failed");
    if (region != curCDGNode->GetRegion()) {
        return;
    }

    switch (infoType) {
        case kMembar: {
            Insn *membarInsn = cdgNode->GetMembarInsn();
            if (membarInsn != nullptr) {
                visited[curBB.GetId()] = true;
                AddDependence(*membarInsn->GetDepNode(), depNode, depType);
            }
            break;
        }
        case kLastCall: {
            Insn *lastCallInsn = cdgNode->GetLastCallInsn();
            if (lastCallInsn != nullptr) {
                visited[curBB.GetId()] = true;
                AddDependence(*lastCallInsn->GetDepNode(), depNode, depType);
            }
            break;
        }
        case kLastFrameDef: {
            Insn *lastFrameDef = cdgNode->GetLastFrameDefInsn();
            if (lastFrameDef != nullptr) {
                visited[curBB.GetId()] = true;
                AddDependence(*lastFrameDef->GetDepNode(), depNode, depType);
            }
            break;
        }
        case kStackUses: {
            visited[curBB.GetId()] = true;
            MapleVector<Insn *> stackUses = cdgNode->GetStackUseInsns();
            BuildForStackHeapDefUseInfoData(needCmp, stackUses, depNode, depType);
            break;
        }
        case kStackDefs: {
            visited[curBB.GetId()] = true;
            MapleVector<Insn *> stackDefs = cdgNode->GetStackDefInsns();
            BuildForStackHeapDefUseInfoData(needCmp, stackDefs, depNode, depType);
            break;
        }
        case kHeapUses: {
            visited[curBB.GetId()] = true;
            MapleVector<Insn *> &heapUses = cdgNode->GetHeapUseInsns();
            BuildForStackHeapDefUseInfoData(needCmp, heapUses, depNode, depType);
            break;
        }
        case kHeapDefs: {
            visited[curBB.GetId()] = true;
            MapleVector<Insn *> &heapDefs = cdgNode->GetHeapDefInsns();
            BuildForStackHeapDefUseInfoData(needCmp, heapDefs, depNode, depType);
            break;
        }
        default: {
            visited[curBB.GetId()] = true;
            break;
        }
    }
    // Ignore back-edge
    if (cdgNode == curRegion->GetRegionRoot()) {
        return;
    }
    for (auto predIt = curBB.GetPredsBegin(); predIt != curBB.GetPredsEnd(); ++predIt) {
        // Ignore back-edge of self-loop
        if (*predIt != &curBB) {
            BuildPredPathSpecialDataInfoDependencyDFS(**predIt, visited, needCmp, depNode, depType, infoType);
        }
    }
}

void DataDepBase::BuildForStackHeapDefUseInfoData(bool needCmp, MapleVector<Insn *> &insns, DepNode &depNode,
                                                  DepType depType)
{
    if (needCmp) {
        AddDependence4InsnInVectorByTypeAndCmp(insns, *depNode.GetInsn(), depType);
    } else {
        AddDependence4InsnInVectorByType(insns, *depNode.GetInsn(), depType);
    }
}

// Build data dependency edge from FromNode to ToNode:
//  Two data dependency node has a unique edge.
//  These two dependencies will set the latency on the dependency edge:
//   1. True data dependency overwrites other dependency;
//   2. Output data dependency overwrites other dependency except true dependency;
//  The latency of the other dependencies is 0.
void DataDepBase::AddDependence(DepNode &fromNode, DepNode &toNode, DepType depType)
{
    // Can not build a self loop dependence
    if (&fromNode == &toNode) {
        return;
    }
    // Check if exist edge between the two node
    if (!fromNode.GetSuccs().empty()) {
        DepLink *depLink = fromNode.GetSuccs().back();
        if (&(depLink->GetTo()) == &toNode) {
            // If there exists edges, only replace with the true or output dependency
            if (depLink->GetDepType() != kDependenceTypeTrue && depType == kDependenceTypeTrue) {
                depLink->SetDepType(kDependenceTypeTrue);
                // Set latency on true dependency edge:
                // 1. if there exists bypass between defInsn and useInsn, the latency of dependency is bypass default;
                // 2. otherwise, the latency of dependency is cost of defInsn
                depLink->SetLatency(static_cast<uint32>(mad.GetLatency(*fromNode.GetInsn(), *toNode.GetInsn())));
            } else if (depLink->GetDepType() != kDependenceTypeOutput && depType == kDependenceTypeOutput) {
                depLink->SetDepType(kDependenceTypeOutput);
                // Set latency on output dependency edge:
                // 1. set (fromInsn_default_latency - toInsn_default_latency), if it greater than 0;
                // 2. set 1 by default.
                int latency = (fromNode.GetReservation()->GetLatency() - toNode.GetReservation()->GetLatency()) > 0
                                  ? fromNode.GetReservation()->GetLatency() - toNode.GetReservation()->GetLatency()
                                  : 1;
                depLink->SetLatency(static_cast<uint32>(latency));
            }
            return;
        }
    }
    auto *depLink = memPool.New<DepLink>(fromNode, toNode, depType);
    if (depType == kDependenceTypeTrue) {
        depLink->SetLatency(static_cast<uint32>(mad.GetLatency(*fromNode.GetInsn(), *toNode.GetInsn())));
    } else if (depType == kDependenceTypeOutput) {
        int latency = (fromNode.GetReservation()->GetLatency() - toNode.GetReservation()->GetLatency()) > 0
                          ? fromNode.GetReservation()->GetLatency() - toNode.GetReservation()->GetLatency()
                          : 1;
        depLink->SetLatency(static_cast<uint32>(latency));
    }
    fromNode.AddSucc(*depLink);
    toNode.AddPred(*depLink);
}

void DataDepBase::AddDependence4InsnInVectorByType(MapleVector<Insn *> &insns, Insn &insn, const DepType &type)
{
    for (auto anyInsn : insns) {
        AddDependence(*anyInsn->GetDepNode(), *insn.GetDepNode(), type);
    }
}

void DataDepBase::AddDependence4InsnInVectorByTypeAndCmp(MapleVector<Insn *> &insns, Insn &insn, const DepType &type)
{
    for (auto anyInsn : insns) {
        if (anyInsn != &insn) {
            AddDependence(*anyInsn->GetDepNode(), *insn.GetDepNode(), type);
        }
    }
}

// Remove self data dependence (self loop) in data dependence graph.
void DataDepBase::RemoveSelfDeps(Insn &insn)
{
    DepNode *node = insn.GetDepNode();
    DEBUG_ASSERT(node->GetSuccs().back()->GetTo().GetInsn() == &insn, "Is not a self dependence.");
    DEBUG_ASSERT(node->GetPreds().back()->GetFrom().GetInsn() == &insn, "Is not a self dependence.");
    node->RemoveSucc();
    node->RemovePred();
}

// Check if regNO is in ehInRegs.
bool DataDepBase::IfInAmbiRegs(regno_t regNO) const
{
    return false;
}

// Return data dependence type name
const std::string &DataDepBase::GetDepTypeName(DepType depType) const
{
    DEBUG_ASSERT(depType <= kDependenceTypeNone, "array boundary check failed");
    return kDepTypeName[depType];
}
}  // namespace maplebe
