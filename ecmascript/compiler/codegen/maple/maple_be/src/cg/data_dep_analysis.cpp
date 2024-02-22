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

#include "data_dep_analysis.h"
#include "control_dep_analysis.h"
#include "aarch64_cg.h"

namespace maplebe {
void DataDepAnalysis::Run(CDGRegion &region)
{
    MemPool *regionMp = memPoolCtrler.NewMemPool("inter-block dda mempool", true);
    auto *regionAlloc = new MapleAllocator(regionMp);

    MapleVector<Insn *> comments(interAlloc.Adapter());
    CDGNode *root = region.GetRegionRoot();
    CHECK_FATAL(root != nullptr, "the root of region must be computed first");
    InitInfoInRegion(*regionMp, *regionAlloc, region);

    // Visit CDGNodes in the region follow the topological order of CFG
    for (auto cdgNode : region.GetRegionNodes()) {
        BB *curBB = cdgNode->GetBB();
        CHECK_FATAL(curBB != nullptr, "get bb from CDGNode failed");
        // Init data dependence info for cur cdgNode
        InitInfoInCDGNode(*regionMp, *regionAlloc, *curBB, *cdgNode);
        const Insn *locInsn = curBB->GetFirstLoc();
        FOR_BB_INSNS(insn, curBB)
        {
            if (!insn->IsMachineInstruction()) {
                ddb.ProcessNonMachineInsn(*insn, comments, cdgNode->GetAllDataNodes(), locInsn);
                continue;
            }
            cdgNode->AccNodeSum();
            DepNode *ddgNode = ddb.GenerateDepNode(*insn, cdgNode->GetAllDataNodes(), cdgNode->GetNodeSum(), comments);
            ddb.BuildMayThrowInsnDependency(*ddgNode, *insn, *locInsn);
            ddb.BuildOpndDependency(*insn);
            BuildSpecialInsnDependency(*insn, *cdgNode, region, *regionAlloc);
            ddb.BuildAmbiInsnDependency(*insn);
            ddb.BuildAsmInsnDependency(*insn);
            ddb.BuildSpecialCallDeps(*insn);
            // For global schedule before RA, do not move the instruction before the call,
            // avoid unnecessary callee allocation and callee save instructions.
            if (!cgFunc.IsAfterRegAlloc()) {
                ddb.BuildDepsLastCallInsn(*insn);
            }
            if (insn->IsFrameDef()) {
                cdgNode->SetLastFrameDefInsn(insn);
            }
            UpdateRegUseAndDef(*insn, *ddgNode, *cdgNode);
        }
        cdgNode->CopyAndClearComments(comments);
        UpdateReadyNodesInfo(*cdgNode, *root);
    }
    ClearInfoInRegion(regionMp, regionAlloc, region);
}

void DataDepAnalysis::InitInfoInRegion(MemPool &regionMp, MapleAllocator &regionAlloc, CDGRegion &region)
{
    ddb.SetCDGRegion(&region);
    for (auto cdgNode : region.GetRegionNodes()) {
        cdgNode->InitTopoInRegionInfo(regionMp, regionAlloc);
    }
}

void DataDepAnalysis::InitInfoInCDGNode(MemPool &regionMp, MapleAllocator &regionAlloc, BB &bb, CDGNode &cdgNode)
{
    ddb.SetCDGNode(&cdgNode);
    ddb.InitCDGNodeDataInfo(regionMp, regionAlloc, cdgNode);
    if (cgFunc.GetMirModule().IsJavaModule()) {
        // Analysis live-in registers in catch BB
        ddb.AnalysisAmbiInsns(bb);
    }
}

void DataDepAnalysis::BuildDepsForPrevSeparator(CDGNode &cdgNode, DepNode &depNode, CDGRegion &curRegion)
{
    if (cdgNode.GetRegion() != &curRegion) {
        return;
    }
    DepNode *prevSepNode = nullptr;
    MapleVector<DepNode *> &dataNodes = cdgNode.GetAllDataNodes();
    for (auto i = static_cast<int32>(dataNodes.size() - 1); i >= 0; --i) {
        if (dataNodes[static_cast<uint32>(i)]->GetType() == kNodeTypeSeparator) {
            prevSepNode = dataNodes[static_cast<uint32>(i)];
            break;
        }
    }
    if (prevSepNode != nullptr) {
        ddb.AddDependence(*prevSepNode, depNode, kDependenceTypeSeparator);
        return;
    }
    BB *bb = cdgNode.GetBB();
    CHECK_FATAL(bb != nullptr, "get bb from cdgNode failed");
    for (auto predIt = bb->GetPredsBegin(); predIt != bb->GetPredsEnd(); ++predIt) {
        CDGNode *predNode = (*predIt)->GetCDGNode();
        CHECK_FATAL(predNode != nullptr, "get cdgNode from bb failed");
        BuildDepsForPrevSeparator(*predNode, depNode, curRegion);
    }
}

void DataDepAnalysis::BuildSpecialInsnDependency(Insn &insn, CDGNode &cdgNode, CDGRegion &region, MapleAllocator &alloc)
{
    MapleVector<DepNode *> dataNodes(alloc.Adapter());
    for (auto nodeId : cdgNode.GetTopoPredInRegion()) {
        CDGNode *predNode = region.GetCDGNodeById(nodeId);
        CHECK_FATAL(predNode != nullptr, "get cdgNode from region by id failed");
        for (auto depNode : predNode->GetAllDataNodes()) {
            dataNodes.emplace_back(depNode);
        }
    }
    for (auto depNode : cdgNode.GetAllDataNodes()) {
        if (depNode != insn.GetDepNode()) {
            dataNodes.emplace_back(depNode);
        }
    }
    ddb.BuildSpecialInsnDependency(insn, dataNodes);
}

void DataDepAnalysis::UpdateRegUseAndDef(Insn &insn, const DepNode &depNode, CDGNode &cdgNode)
{
    // Update reg use
    const auto &useRegNos = depNode.GetUseRegnos();
    for (auto regNO : useRegNos) {
        cdgNode.AppendUseInsnChain(regNO, &insn, interMp);
    }

    // Update reg def
    const auto &defRegNos = depNode.GetDefRegnos();
    for (const auto regNO : defRegNos) {
        // Update reg def for cur depInfo
        cdgNode.SetLatestDefInsn(regNO, &insn);
        cdgNode.ClearUseInsnChain(regNO);
    }
}

void DataDepAnalysis::UpdateReadyNodesInfo(CDGNode &cdgNode, const CDGNode &root) const
{
    BB *bb = cdgNode.GetBB();
    CHECK_FATAL(bb != nullptr, "get bb from cdgNode failed");
    for (auto succIt = bb->GetSuccsBegin(); succIt != bb->GetSuccsEnd(); ++succIt) {
        CDGNode *succNode = (*succIt)->GetCDGNode();
        CHECK_FATAL(succNode != nullptr, "get cdgNode from bb failed");
        if (succNode != &root && succNode->GetRegion() == cdgNode.GetRegion()) {
            succNode->SetNodeSum(std::max(cdgNode.GetNodeSum(), succNode->GetNodeSum()));
            // Successor nodes in region record nodeIds that have been visited in topology order
            for (const auto &nodeId : cdgNode.GetTopoPredInRegion()) {
                succNode->InsertVisitedTopoPredInRegion(nodeId);
            }
            succNode->InsertVisitedTopoPredInRegion(cdgNode.GetNodeId());
        }
    }
}

void DataDepAnalysis::ClearInfoInRegion(MemPool *regionMp, MapleAllocator *regionAlloc, CDGRegion &region) const
{
    delete regionAlloc;
    memPoolCtrler.DeleteMemPool(regionMp);
    for (auto cdgNode : region.GetRegionNodes()) {
        cdgNode->ClearDataDepInfo();
        cdgNode->ClearTopoInRegionInfo();
    }
}

void DataDepAnalysis::GenerateDataDepGraphDotOfRegion(CDGRegion &region)
{
    bool hasExceedMaximum = (region.GetRegionNodes().size() > kMaxDumpRegionNodeNum);
    std::streambuf *coutBuf = std::cout.rdbuf();
    std::ofstream iddgFile;
    std::streambuf *fileBuf = iddgFile.rdbuf();
    (void)std::cout.rdbuf(fileBuf);

    // Define the output file name
    std::string fileName;
    (void)fileName.append("interDDG_");
    (void)fileName.append(cgFunc.GetName());
    (void)fileName.append("_region");
    (void)fileName.append(std::to_string(region.GetRegionId()));
    (void)fileName.append(".dot");

    iddgFile.open(fileName, std::ios::trunc);
    if (!iddgFile.is_open()) {
        LogInfo::MapleLogger(kLlWarn) << "fileName:" << fileName << " open failed.\n";
        return;
    }
    iddgFile << "digraph InterDDG_" << cgFunc.GetName() << " {\n\n";
    if (hasExceedMaximum) {
        iddgFile << "newrank = true;\n";
    }
    iddgFile << "  node [shape=box];\n\n";

    for (auto cdgNode : region.GetRegionNodes()) {
        // Dump nodes style
        for (auto depNode : cdgNode->GetAllDataNodes()) {
            ddb.DumpNodeStyleInDot(iddgFile, *depNode);
        }
        iddgFile << "\n";

        /* Dump edges style */
        for (auto depNode : cdgNode->GetAllDataNodes()) {
            for (auto succ : depNode->GetSuccs()) {
                // Avoid overly complex data dependency graphs
                if (hasExceedMaximum && succ->GetDepType() == kDependenceTypeSeparator) {
                    continue;
                }
                iddgFile << "  insn_" << depNode->GetInsn() << " -> "
                         << "insn_" << succ->GetTo().GetInsn();
                iddgFile << " [";
                switch (succ->GetDepType()) {
                    case kDependenceTypeTrue:
                        iddgFile << "color=red,";
                        iddgFile << "label= \"" << succ->GetLatency() << "\"";
                        break;
                    case kDependenceTypeOutput:
                        iddgFile << "label= \""
                                 << "output"
                                 << "\"";
                        break;
                    case kDependenceTypeAnti:
                        iddgFile << "label= \""
                                 << "anti"
                                 << "\"";
                        break;
                    case kDependenceTypeControl:
                        iddgFile << "label= \""
                                 << "control"
                                 << "\"";
                        break;
                    case kDependenceTypeMembar:
                        iddgFile << "label= \""
                                 << "membar"
                                 << "\"";
                        break;
                    case kDependenceTypeThrow:
                        iddgFile << "label= \""
                                 << "throw"
                                 << "\"";
                        break;
                    case kDependenceTypeSeparator:
                        iddgFile << "label= \""
                                 << "separator"
                                 << "\"";
                        break;
                    case kDependenceTypeMemAccess:
                        iddgFile << "label= \""
                                 << "memAccess"
                                 << "\"";
                        break;
                    default:
                        CHECK_FATAL(false, "invalid depType");
                }
                iddgFile << "];\n";
            }
        }
        iddgFile << "\n";

        // Dump BB cluster
        BB *bb = cdgNode->GetBB();
        CHECK_FATAL(bb != nullptr, "get bb from cdgNode failed");
        iddgFile << "  subgraph cluster_" << bb->GetId() << " {\n";
        iddgFile << "    color=blue;\n";
        iddgFile << "    label = \"bb #" << bb->GetId() << "\";\n";
        for (auto depNode : cdgNode->GetAllDataNodes()) {
            iddgFile << "    insn_" << depNode->GetInsn() << ";\n";
        }
        iddgFile << "}\n\n";
    }

    iddgFile << "}\n";
    (void)iddgFile.flush();
    iddgFile.close();
    (void)std::cout.rdbuf(coutBuf);
}
}  // namespace maplebe
