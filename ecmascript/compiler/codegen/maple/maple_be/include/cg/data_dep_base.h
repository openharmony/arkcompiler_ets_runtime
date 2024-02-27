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
#ifndef MAPLEBE_INCLUDE_CG_DATA_DEP_BASE_H
#define MAPLEBE_INCLUDE_CG_DATA_DEP_BASE_H

#include "deps.h"
#include "cgbb.h"
#include "cg_cdg.h"

namespace maplebe {
using namespace maple;
constexpr maple::uint32 kMaxDependenceNum = 200;
constexpr maple::uint32 kMaxInsnNum = 220;

class DataDepBase {
public:
    DataDepBase(MemPool &memPool, CGFunc &func, MAD &mad, bool isIntraAna)
        : memPool(memPool),
          alloc(&memPool),
          cgFunc(func),
          mad(mad),
          beforeRA(!cgFunc.IsAfterRegAlloc()),
          isIntra(isIntraAna)
    {
    }
    virtual ~DataDepBase()
    {
        curRegion = nullptr;
        curCDGNode = nullptr;
    }

    enum DataFlowInfoType : uint8 {
        kDataFlowUndef,
        kMembar,
        kLastCall,
        kLastFrameDef,
        kStackUses,
        kStackDefs,
        kHeapUses,
        kHeapDefs,
        kMayThrows,
        kAmbiguous,
    };

    void SetCDGNode(CDGNode *cdgNode)
    {
        curCDGNode = cdgNode;
    }
    CDGNode *GetCDGNode()
    {
        return curCDGNode;
    }
    void SetCDGRegion(CDGRegion *region)
    {
        curRegion = region;
    }

    void ProcessNonMachineInsn(Insn &insn, MapleVector<Insn *> &comments, MapleVector<DepNode *> &dataNodes,
                               const Insn *&locInsn);

    void AddDependence4InsnInVectorByType(MapleVector<Insn *> &insns, Insn &insn, const DepType &type);
    void AddDependence4InsnInVectorByTypeAndCmp(MapleVector<Insn *> &insns, Insn &insn, const DepType &type);

    const std::string &GetDepTypeName(DepType depType) const;

    bool IfInAmbiRegs(regno_t regNO) const;
    void AddDependence(DepNode &fromNode, DepNode &toNode, DepType depType);
    DepNode *GenerateDepNode(Insn &insn, MapleVector<DepNode *> &nodes, uint32 &nodeSum, MapleVector<Insn *> &comments);
    void RemoveSelfDeps(Insn &insn);
    void BuildDepsUseReg(Insn &insn, regno_t regNO);
    void BuildDepsDefReg(Insn &insn, regno_t regNO);
    void BuildDepsAmbiInsn(Insn &insn);
    void BuildAmbiInsnDependency(Insn &insn);
    void BuildMayThrowInsnDependency(DepNode &depNode, Insn &insn, const Insn &locInsn);
    void BuildDepsControlAll(Insn &insn, const MapleVector<DepNode *> &nodes);
    void BuildDepsBetweenControlRegAndCall(Insn &insn, bool isDest);
    void BuildDepsLastCallInsn(Insn &insn);
    void BuildInterBlockDefUseDependency(DepNode &curDepNode, regno_t regNO, DepType depType, bool isDef);
    void BuildPredPathDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode, regno_t regNO,
                                       DepType depType);
    void BuildPredPathUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode, regno_t regNO,
                                       DepType depType);
    void BuildInterBlockSpecialDataInfoDependency(DepNode &curDepNode, bool needCmp, DepType depType,
                                                  DataDepBase::DataFlowInfoType infoType);
    void BuildPredPathSpecialDataInfoDependencyDFS(BB &curBB, std::vector<bool> &visited, bool needCmp,
                                                   DepNode &depNode, DepType depType,
                                                   DataDepBase::DataFlowInfoType infoType);

    virtual void InitCDGNodeDataInfo(MemPool &mp, MapleAllocator &alloc, CDGNode &cdgNode) = 0;
    virtual bool IsFrameReg(const RegOperand &) const = 0;
    virtual void BuildDepsMemBar(Insn &insn) = 0;
    virtual void BuildDepsUseMem(Insn &insn, MemOperand &memOpnd) = 0;
    virtual void BuildDepsDefMem(Insn &insn, MemOperand &memOpnd) = 0;
    virtual void BuildDepsAccessStImmMem(Insn &insn) = 0;
    virtual void BuildCallerSavedDeps(Insn &insn) = 0;
    virtual void BuildDepsDirtyStack(Insn &insn) = 0;
    virtual void BuildDepsUseStack(Insn &insn) = 0;
    virtual void BuildDepsDirtyHeap(Insn &insn) = 0;
    virtual void BuildOpndDependency(Insn &insn) = 0;
    virtual void BuildSpecialInsnDependency(Insn &insn, const MapleVector<DepNode *> &nodes) = 0;
    virtual void BuildSpecialCallDeps(Insn &insn) = 0;
    virtual void BuildAsmInsnDependency(Insn &insn) = 0;
    virtual void BuildInterBlockMemDefUseDependency(DepNode &depNode, bool isMemDef) = 0;
    virtual void BuildPredPathMemDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode) = 0;
    virtual void BuildPredPathMemUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode) = 0;
    virtual void DumpNodeStyleInDot(std::ofstream &file, DepNode &depNode) = 0;

private:
    // only called by BuildPredPathSpecialDataInfoDependencyDFS
    void BuildForStackHeapDefUseInfoData(bool needCmp, MapleVector<Insn *> &insns, DepNode &depNode, DepType depType);

protected:
    MemPool &memPool;
    MapleAllocator alloc;
    CGFunc &cgFunc;
    MAD &mad;
    bool beforeRA = false;
    bool isIntra = false;
    CDGNode *curCDGNode = nullptr;
    CDGRegion *curRegion = nullptr;
};
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_DATA_DEP_BASE_H
