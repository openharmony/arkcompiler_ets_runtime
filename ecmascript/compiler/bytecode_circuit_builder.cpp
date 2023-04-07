/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/bytecode_circuit_builder.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/ts_types/ts_manager.h"
#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript::kungfu {
void BytecodeCircuitBuilder::BytecodeToCircuit()
{
    ExceptionInfo exceptionInfo = {};

    // collect try catch block info
    CollectTryCatchBlockInfo(exceptionInfo);
    hasTryCatch_ = exceptionInfo.size() != 0;
    BuildRegionInfo();
    // Building the basic block diagram of bytecode
    BuildRegions(exceptionInfo);
}

void BytecodeCircuitBuilder::BuildRegionInfo()
{
    uint32_t size = pcOffsets_.size();
    uint32_t end = size - 1;  // 1: end
    BytecodeIterator iterator(this, 0, end);

    infoData_.resize(size);
    byteCodeToJSGate_.resize(size, Circuit::NullGate());
    regionsInfo_.InsertHead(0); // 0: start pc
    for (iterator.GotoStart(); !iterator.Done(); ++iterator) {
        auto index = iterator.Index();
        auto &info = infoData_[index];
        auto pc = pcOffsets_[index];
        info.metaData_ = bytecodes_->GetBytecodeMetaData(pc);
        BytecodeInfo::InitBytecodeInfo(this, info, pc);
        CollectRegionInfo(index);
    }
}

int32_t BytecodeCircuitBuilder::GetJumpOffset(uint32_t bcIndex) const
{
    auto pc = GetPCByIndex(bcIndex);
    auto &info = GetBytecodeInfo(bcIndex);
    int32_t offset = 0;
    switch (info.GetOpcode()) {
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JMP_IMM8:
            offset = static_cast<int8_t>(READ_INST_8_0());
            break;
        case EcmaOpcode::JNEZ_IMM16:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JMP_IMM16:
            offset = static_cast<int16_t>(READ_INST_16_0());
            break;
        case EcmaOpcode::JMP_IMM32:
        case EcmaOpcode::JNEZ_IMM32:
        case EcmaOpcode::JEQZ_IMM32:
            offset = static_cast<int32_t>(READ_INST_32_0());
            break;
        case EcmaOpcode::RETURN:
        case EcmaOpcode::RETURNUNDEFINED:
        case EcmaOpcode::SUSPENDGENERATOR_V8:
        case EcmaOpcode::DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8:
            offset = -(static_cast<int32_t>(pc - GetFirstPC()));
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    return offset;
}

void BytecodeCircuitBuilder::CollectRegionInfo(uint32_t bcIndex)
{
    auto pc = pcOffsets_[bcIndex];
    auto &info = infoData_[bcIndex];
    int32_t offset = 0;
    if (info.IsJump()) {
        switch (info.GetOpcode()) {
            case EcmaOpcode::JEQZ_IMM8:
            case EcmaOpcode::JNEZ_IMM8:
            case EcmaOpcode::JMP_IMM8:
                offset = static_cast<int8_t>(READ_INST_8_0());
                break;
            case EcmaOpcode::JNEZ_IMM16:
            case EcmaOpcode::JEQZ_IMM16:
            case EcmaOpcode::JMP_IMM16:
                offset = static_cast<int16_t>(READ_INST_16_0());
                break;
            case EcmaOpcode::JMP_IMM32:
            case EcmaOpcode::JNEZ_IMM32:
            case EcmaOpcode::JEQZ_IMM32:
                offset = static_cast<int32_t>(READ_INST_32_0());
                break;
            default:
                UNREACHABLE();
                break;
        }
        auto nextIndex = bcIndex + 1; // 1: next pc
        auto targetIndex = FindBcIndexByPc(pc + offset);
        // condition branch current basic block end
        if (info.IsCondJump()) {
            regionsInfo_.InsertSplit(nextIndex);
            regionsInfo_.InsertJump(targetIndex, bcIndex, false);
        } else {
            regionsInfo_.InsertHead(nextIndex);
            regionsInfo_.InsertJump(targetIndex, bcIndex, true);
        }
    } else if (info.IsReturn() || info.IsThrow()) {
        if (bcIndex != GetLastBcIndex()) {
            auto nextIndex = bcIndex + 1; // 1: next pc
            regionsInfo_.InsertHead(nextIndex);
        }
    }
}

void BytecodeCircuitBuilder::CollectTryCatchBlockInfo(ExceptionInfo &byteCodeException)
{
    panda_file::MethodDataAccessor mda(*pf_, method_->GetMethodId());
    panda_file::CodeDataAccessor cda(*pf_, mda.GetCodeId().value());

    cda.EnumerateTryBlocks([this, &byteCodeException](
        panda_file::CodeDataAccessor::TryBlock &tryBlock) {
        auto tryStartOffset = tryBlock.GetStartPc();
        auto tryEndOffset = tryBlock.GetStartPc() + tryBlock.GetLength();

        auto tryStartPc = const_cast<uint8_t *>(method_->GetBytecodeArray() + tryStartOffset);
        auto tryEndPc = const_cast<uint8_t *>(method_->GetBytecodeArray() + tryEndOffset);
        // skip try blocks with same pc in start and end label
        if (tryStartPc == tryEndPc) {
            return true;
        }

        auto tryStartBcIndex = FindBcIndexByPc(tryStartPc);
        regionsInfo_.InsertSplit(tryStartBcIndex);
        if (tryEndPc <= GetLastPC()) {
            auto tryEndBcIndex = FindBcIndexByPc(tryEndPc);
            regionsInfo_.InsertSplit(tryEndBcIndex);
        }
        byteCodeException.emplace_back(ExceptionItem { tryStartPc, tryEndPc, {} });
        tryBlock.EnumerateCatchBlocks([&](panda_file::CodeDataAccessor::CatchBlock &catchBlock) {
            auto pcOffset = catchBlock.GetHandlerPc();
            auto catchBlockPc = const_cast<uint8_t *>(method_->GetBytecodeArray() + pcOffset);
            auto catchBlockBcIndex = FindBcIndexByPc(catchBlockPc);
            regionsInfo_.InsertHead(catchBlockBcIndex);
            // try block associate catch block
            byteCodeException.back().catchs.emplace_back(catchBlockPc);
            return true;
        });
        return true;
    });
}

void BytecodeCircuitBuilder::BuildRegions(const ExceptionInfo &byteCodeException)
{
    auto &items = regionsInfo_.GetBlockItems();
    auto blockSize = items.size();
    graph_.resize(blockSize);
    // build basic block
    size_t blockId = 0;
    for (const auto &item : items) {
        auto &curBlock = GetBasicBlockById(blockId);
        curBlock.id = blockId;
        curBlock.start = item.GetStartBcIndex();
        if (blockId != 0) {
            auto &prevBlock = graph_[blockId - 1];
            prevBlock.end = curBlock.start - 1;
            prevBlock.bytecodeIterator_.Reset(this, prevBlock.start, prevBlock.end);
            // fall through
            if (!item.IsHeadBlock()) {
                curBlock.preds.emplace_back(&prevBlock);
                prevBlock.succs.emplace_back(&curBlock);
            }
        }
        blockId++;
    }
    auto &lastBlock = graph_[blockId - 1]; // 1: last block
    lastBlock.end = GetLastBcIndex();
    lastBlock.bytecodeIterator_.Reset(this, lastBlock.start, lastBlock.end);

    auto &splitItems = regionsInfo_.GetSplitItems();
    for (const auto &item : splitItems) {
        auto curIndex = regionsInfo_.FindBBIndexByBcIndex(item.startBcIndex);
        auto &curBlock = GetBasicBlockById(curIndex);
        auto predIndex = regionsInfo_.FindBBIndexByBcIndex(item.predBcIndex);
        auto &predBlock = GetBasicBlockById(predIndex);
        curBlock.preds.emplace_back(&predBlock);
        predBlock.succs.emplace_back(&curBlock);
    }

    if (byteCodeException.size() != 0) {
        BuildCatchBlocks(byteCodeException);
    }
    if (IsLogEnabled()) {
        PrintGraph("Build Basic Block");
    }
    ComputeDominatorTree();
}

void BytecodeCircuitBuilder::BuildCatchBlocks(const ExceptionInfo &byteCodeException)
{
    // try catch block associate
    for (size_t i = 0; i < graph_.size(); i++) {
        auto &bb = graph_[i];
        auto startIndex = bb.start;
        const auto pc = pcOffsets_[startIndex];
        for (auto it = byteCodeException.cbegin(); it != byteCodeException.cend(); it++) {
            if (pc < it->startPc || pc >= it->endPc) {
                continue;
            }
            // try block interval
            const auto &catchs = it->catchs; // catchs start pc
            for (size_t j = i + 1; j < graph_.size(); j++) {
                auto &catchBB = graph_[j];
                const auto catchStart = pcOffsets_[catchBB.start];
                if (std::find(catchs.cbegin(), catchs.cend(), catchStart) != catchs.cend()) {
                    bb.catchs.insert(bb.catchs.cbegin(), &catchBB);
                    bb.succs.emplace_back(&catchBB);
                    catchBB.preds.emplace_back(&bb);
                }
            }
        }

        // When there are multiple catch blocks in the current block, the set of catch blocks
        // needs to be sorted to satisfy the order of execution of catch blocks.
        bb.SortCatches();
    }
}

void BytecodeCircuitBuilder::ComputeDominatorTree()
{
    // Construct graph backward order
    std::unordered_map<size_t, size_t> bbIdToDfsTimestamp;
    std::unordered_map<size_t, size_t> dfsFatherIdx;
    std::unordered_map<size_t, size_t> bbDfsTimestampToIdx;
    std::vector<size_t> basicBlockList;
    size_t timestamp = 0;
    std::deque<size_t> pendingList;
    std::vector<size_t> visited(graph_.size(), 0);
    auto basicBlockId = graph_[0].id;
    visited[graph_[0].id] = 1;
    pendingList.emplace_back(basicBlockId);
    while (!pendingList.empty()) {
        size_t curBlockId = pendingList.back();
        pendingList.pop_back();
        basicBlockList.emplace_back(curBlockId);
        bbIdToDfsTimestamp[curBlockId] = timestamp++;
        for (const auto &succBlock: graph_[curBlockId].succs) {
            if (visited[succBlock->id] == 0) {
                visited[succBlock->id] = 1;
                pendingList.emplace_back(succBlock->id);
                dfsFatherIdx[succBlock->id] = bbIdToDfsTimestamp[curBlockId];
            }
        }
    }

    for (size_t idx = 0; idx < basicBlockList.size(); idx++) {
        bbDfsTimestampToIdx[basicBlockList[idx]] = idx;
    }
    RemoveDeadRegions(bbIdToDfsTimestamp);

    std::vector<size_t> immDom(basicBlockList.size()); // immediate dominator with dfs order index
    std::vector<size_t> semiDom(basicBlockList.size());
    std::vector<size_t> realImmDom(graph_.size()); // immediate dominator with real index
    std::vector<std::vector<size_t> > semiDomTree(basicBlockList.size());
    {
        std::vector<size_t> parent(basicBlockList.size());
        std::iota(parent.begin(), parent.end(), 0);
        std::vector<size_t> minIdx(basicBlockList.size());
        std::function<size_t(size_t)> unionFind = [&] (size_t idx) -> size_t {
            if (parent[idx] == idx) return idx;
            size_t unionFindSetRoot = unionFind(parent[idx]);
            if (semiDom[minIdx[idx]] > semiDom[minIdx[parent[idx]]]) {
                minIdx[idx] = minIdx[parent[idx]];
            }
            return parent[idx] = unionFindSetRoot;
        };
        auto merge = [&] (size_t fatherIdx, size_t sonIdx) -> void {
            size_t parentFatherIdx = unionFind(fatherIdx);
            size_t parentSonIdx = unionFind(sonIdx);
            parent[parentSonIdx] = parentFatherIdx;
        };
        std::iota(semiDom.begin(), semiDom.end(), 0);
        semiDom[0] = semiDom.size();
        for (size_t idx = basicBlockList.size() - 1; idx >= 1; idx--) {
            for (const auto &preBlock : graph_[basicBlockList[idx]].preds) {
                if (bbDfsTimestampToIdx[preBlock->id] < idx) {
                    semiDom[idx] = std::min(semiDom[idx], bbDfsTimestampToIdx[preBlock->id]);
                } else {
                    unionFind(bbDfsTimestampToIdx[preBlock->id]);
                    semiDom[idx] = std::min(semiDom[idx], semiDom[minIdx[bbDfsTimestampToIdx[preBlock->id]]]);
                }
            }
            for (const auto & succDomIdx : semiDomTree[idx]) {
                unionFind(succDomIdx);
                if (idx == semiDom[minIdx[succDomIdx]]) {
                    immDom[succDomIdx] = idx;
                } else {
                    immDom[succDomIdx] = minIdx[succDomIdx];
                }
            }
            minIdx[idx] = idx;
            merge(dfsFatherIdx[basicBlockList[idx]], idx);
            semiDomTree[semiDom[idx]].emplace_back(idx);
        }
        for (size_t idx = 1; idx < basicBlockList.size(); idx++) {
            if (immDom[idx] != semiDom[idx]) {
                immDom[idx] = immDom[immDom[idx]];
            }
            realImmDom[basicBlockList[idx]] = basicBlockList[immDom[idx]];
        }
        semiDom[0] = 0;
    }

    if (IsLogEnabled()) {
        PrintGraph("Computed Dom Trees");
    }

    BuildImmediateDominator(realImmDom);
}

void BytecodeCircuitBuilder::BuildImmediateDominator(const std::vector<size_t> &immDom)
{
    graph_[0].iDominator = &graph_[0];
    for (size_t i = 1; i < immDom.size(); i++) {
        auto dominatedBlock = &graph_[i];
        if (dominatedBlock->isDead) {
            continue;
        }
        auto immDomBlock = &graph_[immDom[i]];
        dominatedBlock->iDominator = immDomBlock;
    }

    for (auto &block : graph_) {
        if (block.isDead) {
            continue;
        }
        if (block.iDominator->id != block.id) {
            block.iDominator->immDomBlocks.emplace_back(&block);
        }
    }

    ComputeDomFrontiers(immDom);
    InsertPhi();
    UpdateCFG();
    BuildCircuit();
}

void BytecodeCircuitBuilder::ComputeDomFrontiers(const std::vector<size_t> &immDom)
{
    std::vector<std::set<BytecodeRegion *>> domFrontiers(immDom.size());
    for (auto &bb : graph_) {
        if (bb.isDead) {
            continue;
        }
        if (bb.preds.size() < 2) { // 2: pred num
            continue;
        }
        for (size_t i = 0; i < bb.preds.size(); i++) {
            auto runner = bb.preds[i];
            while (runner->id != immDom[bb.id]) {
                domFrontiers[runner->id].insert(&bb);
                runner = &graph_[immDom[runner->id]];
            }
        }
    }

    for (size_t i = 0; i < domFrontiers.size(); i++) {
        for (auto iter = domFrontiers[i].cbegin(); iter != domFrontiers[i].cend(); iter++) {
            graph_[i].domFrontiers.emplace_back(*iter);
        }
    }
}

void BytecodeCircuitBuilder::RemoveDeadRegions(const std::unordered_map<size_t, size_t> &bbIdToDfsTimestamp)
{
    for (auto &block: graph_) {
        std::vector<BytecodeRegion *> newPreds;
        for (auto &bb : block.preds) {
            if (bbIdToDfsTimestamp.count(bb->id)) {
                newPreds.emplace_back(bb);
            }
        }
        block.preds = newPreds;
    }

    for (auto &block : graph_) {
        block.isDead = !bbIdToDfsTimestamp.count(block.id);
        if (block.isDead) {
            block.succs.clear();
        }
    }
}

void BytecodeCircuitBuilder::InsertPhi()
{
    std::unordered_map<uint16_t, std::set<size_t>> defsitesInfo; // <vreg, bbs>
    for (auto &bb : graph_) {
        if (bb.isDead) {
            continue;
        }
        EnumerateBlock(bb, [this, &defsitesInfo, &bb]
            (const BytecodeInfo &bytecodeInfo) -> bool {
            if (bytecodeInfo.IsBc(EcmaOpcode::RESUMEGENERATOR)) {
                auto numVRegs = GetNumberVRegsWithEnv();
                for (size_t i = 0; i < numVRegs; i++) {
                    defsitesInfo[i].insert(bb.id);
                }
            }
            for (const auto &vreg: bytecodeInfo.vregOut) {
                defsitesInfo[vreg].insert(bb.id);
            }
            return true;
        });
    }

    // handle phi generated from multiple control flow in the same source block
    InsertExceptionPhi(defsitesInfo);

    for (const auto&[variable, defsites] : defsitesInfo) {
        std::queue<uint16_t> workList;
        for (auto blockId: defsites) {
            workList.push(blockId);
        }
        while (!workList.empty()) {
            auto currentId = workList.front();
            workList.pop();
            for (auto &block : graph_[currentId].domFrontiers) {
                if (!block->phi.count(variable)) {
                    block->phi.insert(variable);
                    if (!defsitesInfo[variable].count(block->id)) {
                        workList.push(block->id);
                    }
                }
            }
        }
    }

    if (IsLogEnabled()) {
        PrintGraph("Inserted Phis");
    }
}

void BytecodeCircuitBuilder::InsertExceptionPhi(std::unordered_map<uint16_t, std::set<size_t>> &defsitesInfo)
{
    // handle try catch defsite
    for (auto &bb : graph_) {
        if (bb.isDead) {
            continue;
        }
        if (bb.catchs.size() == 0) {
            continue;
        }
        std::set<size_t> vregs;
        EnumerateBlock(bb, [this, &vregs]
        (const BytecodeInfo &bytecodeInfo) -> bool {
            if (bytecodeInfo.IsBc(EcmaOpcode::RESUMEGENERATOR)) {
                auto numVRegs = GetNumberVRegsWithEnv();
                for (size_t i = 0; i < numVRegs; i++) {
                    vregs.insert(i);
                }
                return false;
            }
            for (const auto &vreg: bytecodeInfo.vregOut) {
                vregs.insert(vreg);
            }
            return true;
        });

        for (auto &vreg : vregs) {
            defsitesInfo[vreg].insert(bb.catchs.at(0)->id);
            bb.catchs.at(0)->phi.insert(vreg);
        }
    }
}

// Update CFG's predecessor, successor and try catch associations
void BytecodeCircuitBuilder::UpdateCFG()
{
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        bb.preds.clear();
        bb.trys.clear();
        std::vector<BytecodeRegion *> newSuccs;
        for (const auto &succ: bb.succs) {
            if (std::count(bb.catchs.cbegin(), bb.catchs.cend(), succ)) {
                continue;
            }
            newSuccs.emplace_back(succ);
        }
        bb.succs = newSuccs;
    }
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        for (auto &succ: bb.succs) {
            succ->preds.emplace_back(&bb);
        }
        for (auto &catchBlock: bb.catchs) {
            catchBlock->trys.emplace_back(&bb);
        }
    }
}

// build circuit
void BytecodeCircuitBuilder::BuildCircuitArgs()
{
    argAcc_.NewCommonArg(CommonArgIdx::GLUE, MachineType::I64, GateType::NJSValue());
    argAcc_.NewCommonArg(CommonArgIdx::LEXENV, MachineType::I64, GateType::TaggedValue());
    argAcc_.NewCommonArg(CommonArgIdx::ACTUAL_ARGC, MachineType::I64, GateType::NJSValue());
    auto funcIdx = static_cast<size_t>(CommonArgIdx::FUNC);
    const size_t actualNumArgs = argAcc_.GetActualNumArgs();
    // new actual argument gates
    for (size_t argIdx = funcIdx; argIdx < actualNumArgs; argIdx++) {
        argAcc_.NewArg(argIdx);
    }
    argAcc_.CollectArgs();
    if (HasTypes()) {
        argAcc_.FillArgsGateType(&typeRecorder_);
    }
}

bool BytecodeCircuitBuilder::ShouldBeDead(BytecodeRegion &curBlock)
{
    auto isDead = false;
    for (auto bbPred : curBlock.preds) {
        if (!bbPred->isDead) {
            return false;
        }
        isDead = true;
    }
    for (auto bbTry : curBlock.trys) {
        if (!bbTry->isDead) {
            return false;
        }
        isDead = true;
    }
    return isDead;
}

void BytecodeCircuitBuilder::CollectPredsInfo()
{
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        bb.numOfStatePreds = 0;
    }
    // get number of expanded state predicates of each block
    // one block-level try catch edge may correspond to multiple bytecode-level edges
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        if (ShouldBeDead(bb)) {
            bb.UpdateTryCatchInfoForDeadBlock();
            bb.isDead = true;
            continue;
        }
        bool noThrow = true;
        EnumerateBlock(bb, [&noThrow, &bb]
        (const BytecodeInfo &bytecodeInfo) -> bool {
            if (bytecodeInfo.IsGeneral()) {
                noThrow = false;
                if (!bb.catchs.empty()) {
                    bb.catchs.at(0)->numOfStatePreds++;
                }
            }
            if (bytecodeInfo.IsCondJump() && bb.succs.size() == 1) {
                ASSERT(bb.succs[0]->id == bb.id + 1);
                bb.succs[0]->numOfStatePreds++;
            }
            return true;
        });
        bb.UpdateRedundantTryCatchInfo(noThrow);
        bb.UpdateTryCatchInfoIfNoThrow(noThrow);
        for (auto &succ: bb.succs) {
            succ->numOfStatePreds++;
        }
    }
    // collect loopback edges
    std::vector<VisitState> visitState(graph_.size(), VisitState::UNVISITED);
    std::function<void(size_t)> dfs = [&](size_t bbId) -> void {
        visitState[bbId] = VisitState::PENDING;
        std::vector<BytecodeRegion *> merge;
        merge.insert(merge.end(), graph_[bbId].succs.begin(), graph_[bbId].succs.end());
        merge.insert(merge.end(), graph_[bbId].catchs.begin(), graph_[bbId].catchs.end());
        auto it = merge.crbegin();
        while (it != merge.crend()) {
            auto succBlock = *it;
            it++;
            if (visitState[succBlock->id] == VisitState::UNVISITED) {
                dfs(succBlock->id);
            } else {
                if (visitState[succBlock->id] == VisitState::PENDING) {
                    graph_[succBlock->id].loopbackBlocks.insert(bbId);
                }
            }
        }
        visitState[bbId] = VisitState::VISITED;
    };
    dfs(graph_[0].id);
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        bb.phiAcc = (bb.numOfStatePreds > 1) || (!bb.trys.empty());
        bb.numOfLoopBacks = bb.loopbackBlocks.size();
    }
}

void BytecodeCircuitBuilder::NewMerge(GateRef &state, GateRef &depend, size_t numOfIns)
{
    state = circuit_->NewGate(circuit_->Merge(numOfIns),
                              std::vector<GateRef>(numOfIns, Circuit::NullGate()));
    depend = circuit_->NewGate(circuit_->DependSelector(numOfIns),
                               std::vector<GateRef>(numOfIns + 1, Circuit::NullGate()));
    gateAcc_.NewIn(depend, 0, state);
}

void BytecodeCircuitBuilder::NewLoopBegin(BytecodeRegion &bb)
{
    if (bb.id == 0 && bb.numOfStatePreds == 1) {
        bb.mergeForwardEdges = circuit_->NewGate(circuit_->Merge(bb.numOfStatePreds),
            std::vector<GateRef>(bb.numOfStatePreds,
                                 circuit_->GetStateRoot()));
        bb.depForward = circuit_->NewGate(circuit_->DependSelector(bb.numOfStatePreds),
            std::vector<GateRef>(bb.numOfStatePreds + 1, Circuit::NullGate()));
        gateAcc_.NewIn(bb.depForward, 0, bb.mergeForwardEdges);
        gateAcc_.NewIn(bb.depForward, 1, circuit_->GetDependRoot());
    } else {
        NewMerge(bb.mergeForwardEdges, bb.depForward, bb.numOfStatePreds - bb.numOfLoopBacks);
    }
    NewMerge(bb.mergeLoopBackEdges, bb.depLoopBack, bb.numOfLoopBacks);
    auto loopBack = circuit_->NewGate(circuit_->LoopBack(),
        { bb.mergeLoopBackEdges });
    bb.stateStart = circuit_->NewGate(circuit_->LoopBegin(),
        { bb.mergeForwardEdges, loopBack });
    // 2: the number of depend inputs and it is in accord with LOOP_BEGIN
    bb.dependStart = circuit_->NewGate(circuit_->DependSelector(2),
        { bb.stateStart, bb.depForward, bb.depLoopBack });
}

void BytecodeCircuitBuilder::BuildBlockCircuitHead()
{
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        if (bb.numOfStatePreds == 0) {
            bb.stateStart = circuit_->GetStateRoot();
            bb.dependStart = circuit_->GetDependRoot();
        } else if (bb.numOfLoopBacks > 0) {
            NewLoopBegin(bb);
        } else {
            NewMerge(bb.stateStart, bb.dependStart, bb.numOfStatePreds);
        }
    }
}

std::vector<GateRef> BytecodeCircuitBuilder::CreateGateInList(
    const BytecodeInfo &info, const GateMetaData *meta)
{
    auto numValues = meta->GetNumIns();
    const size_t length = meta->GetInValueStarts();
    std::vector<GateRef> inList(numValues, Circuit::NullGate());
    auto inputSize = info.inputs.size();
    for (size_t i = 0; i < inputSize; i++) {
        auto &input = info.inputs[i];
        if (std::holds_alternative<ConstDataId>(input)) {
            if (std::get<ConstDataId>(input).IsStringId()) {
                inList[i + length] = circuit_->GetConstantDataGate(std::get<ConstDataId>(input).CaculateBitField(),
                                                                   GateType::StringType());
            } else {
                inList[i + length] = circuit_->GetConstantGate(MachineType::I64,
                                                               std::get<ConstDataId>(input).GetId(),
                                                               GateType::NJSValue());
            }
        } else if (std::holds_alternative<Immediate>(input)) {
            inList[i + length] = circuit_->GetConstantGate(MachineType::I64,
                                                           std::get<Immediate>(input).GetValue(),
                                                           GateType::NJSValue());
        } else if (std::holds_alternative<ICSlotId>(input)) {
            inList[i + length] = circuit_->GetConstantGate(MachineType::I16,
                                                           std::get<ICSlotId>(input).GetId(),
                                                           GateType::NJSValue());
        } else {
            ASSERT(std::holds_alternative<VirtualRegister>(input));
            continue;
        }
    }
    if (info.AccIn()) {
        inputSize++;
    }
    if (info.ThisObjectIn()) {
        inList[inputSize + length] = argAcc_.GetCommonArgGate(CommonArgIdx::THIS_OBJECT);
    }
    return inList;
}

void BytecodeCircuitBuilder::SetBlockPred(BytecodeRegion &bbNext, const GateRef &state,
                                          const GateRef &depend, bool isLoopBack)
{
    if (bbNext.numOfLoopBacks == 0) {
        gateAcc_.NewIn(bbNext.stateStart, bbNext.statePredIndex, state);
        gateAcc_.NewIn(bbNext.dependStart, bbNext.statePredIndex + 1, depend);
    } else {
        if (isLoopBack) {
            gateAcc_.NewIn(bbNext.mergeLoopBackEdges, bbNext.loopBackIndex, state);
            gateAcc_.NewIn(bbNext.depLoopBack, bbNext.loopBackIndex + 1, depend);
            bbNext.loopBackIndex++;
            ASSERT(bbNext.loopBackIndex <= bbNext.numOfLoopBacks);
        } else {
            gateAcc_.NewIn(bbNext.mergeForwardEdges, bbNext.forwardIndex, state);
            gateAcc_.NewIn(bbNext.depForward, bbNext.forwardIndex + 1, depend);
            bbNext.forwardIndex++;
            ASSERT(bbNext.forwardIndex <= bbNext.numOfStatePreds - bbNext.numOfLoopBacks);
        }
    }
    bbNext.statePredIndex++;
    ASSERT(bbNext.statePredIndex <= bbNext.numOfStatePreds);
}

GateRef BytecodeCircuitBuilder::NewConst(const BytecodeInfo &info)
{
    auto opcode = info.GetOpcode();
    GateRef gate = 0;
    switch (opcode) {
        case EcmaOpcode::LDNAN:
            gate = circuit_->GetConstantGate(MachineType::I64,
                                             base::NumberHelper::GetNaN(),
                                             GateType::NumberType());
            break;
        case EcmaOpcode::LDINFINITY:
            gate = circuit_->GetConstantGate(MachineType::I64,
                                             base::NumberHelper::GetPositiveInfinity(),
                                             GateType::NumberType());
            break;
        case EcmaOpcode::LDUNDEFINED:
            gate = circuit_->GetConstantGate(MachineType::I64,
                                             JSTaggedValue::VALUE_UNDEFINED,
                                             GateType::UndefinedType());
            break;
        case EcmaOpcode::LDNULL:
            gate = circuit_->GetConstantGate(MachineType::I64,
                                             JSTaggedValue::VALUE_NULL,
                                             GateType::NullType());
            break;
        case EcmaOpcode::LDTRUE:
            gate = circuit_->GetConstantGate(MachineType::I64,
                                             JSTaggedValue::VALUE_TRUE,
                                             GateType::BooleanType());
            break;
        case EcmaOpcode::LDFALSE:
            gate = circuit_->GetConstantGate(MachineType::I64,
                                             JSTaggedValue::VALUE_FALSE,
                                             GateType::BooleanType());
            break;
        case EcmaOpcode::LDHOLE:
            gate = circuit_->GetConstantGate(MachineType::I64,
                                             JSTaggedValue::VALUE_HOLE,
                                             GateType::TaggedValue());
            break;
        case EcmaOpcode::LDAI_IMM32:
            gate = circuit_->GetConstantGate(MachineType::I64,
                                             std::get<Immediate>(info.inputs[0]).ToJSTaggedValueInt(),
                                             GateType::IntType());
            break;
        case EcmaOpcode::FLDAI_IMM64:
            gate = circuit_->GetConstantGate(MachineType::I64,
                                             std::get<Immediate>(info.inputs.at(0)).ToJSTaggedValueDouble(),
                                             GateType::DoubleType());
            break;
        case EcmaOpcode::LDFUNCTION:
            gate = argAcc_.GetCommonArgGate(CommonArgIdx::FUNC);
            break;
        case EcmaOpcode::LDNEWTARGET:
            gate = argAcc_.GetCommonArgGate(CommonArgIdx::NEW_TARGET);
            break;
        case EcmaOpcode::LDTHIS:
            gate = argAcc_.GetCommonArgGate(CommonArgIdx::THIS_OBJECT);
            break;
        case EcmaOpcode::LDA_STR_ID16: {
            auto input = std::get<ConstDataId>(info.inputs.at(0));
            gate = circuit_->GetConstantDataGate(input.CaculateBitField(), GateType::StringType());
            break;
        }
        default:
            UNREACHABLE();
    }
    return gate;
}

void BytecodeCircuitBuilder::NewJSGate(BytecodeRegion &bb, GateRef &state, GateRef &depend)
{
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    size_t numValueInputs = bytecodeInfo.ComputeValueInputCount();
    GateRef gate = 0;
    bool writable = !bytecodeInfo.NoSideEffects();
    auto meta = circuit_->JSBytecode(numValueInputs,
        bytecodeInfo.GetOpcode(), iterator.Index(), writable);
    std::vector<GateRef> inList = CreateGateInList(bytecodeInfo, meta);
    if (bytecodeInfo.IsDef()) {
        gate = circuit_->NewGate(meta, MachineType::I64, inList.size(),
                                 inList.data(), GateType::AnyType());
    } else {
        gate = circuit_->NewGate(meta, MachineType::NOVALUE, inList.size(),
                                 inList.data(), GateType::Empty());
    }
    if (bytecodeInfo.IsSuspend()) {
        auto offsetGate = circuit_->GetConstantGate(MachineType::I32,
                                                    GetJumpOffset(iterator.Index()),
                                                    GateType::NJSValue());
        auto updateHotness = circuit_->NewGate(circuit_->UpdateHotness(), {state, depend, offsetGate});
        gateAcc_.NewIn(gate, 0, updateHotness);
        gateAcc_.NewIn(gate, 1, updateHotness);
    } else {
        gateAcc_.NewIn(gate, 0, state);
        gateAcc_.NewIn(gate, 1, depend);
    }
    state = gate;
    if (!bb.catchs.empty()) {
        auto ifSuccess = circuit_->NewGate(circuit_->IfSuccess(), {gate});
        auto ifException = circuit_->NewGate(circuit_->IfException(), {gate});

        auto &bbNext = bb.catchs.at(0);
        auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
        SetBlockPred(*bbNext, ifException, gate, isLoopBack);
        if (bytecodeInfo.GetOpcode() == EcmaOpcode::CREATEASYNCGENERATOROBJ_V8) {
            bbNext->expandedPreds.push_back({bb.id, iterator.Index() + 1, true}); // 1: next pc
        } else {
            bbNext->expandedPreds.push_back({bb.id, iterator.Index(), true});
        }
        state = ifSuccess;
    }
    byteCodeToJSGate_[iterator.Index()] = gate;
    if (bytecodeInfo.IsGeneratorRelative()) {
        if (bytecodeInfo.GetOpcode() == EcmaOpcode::SUSPENDGENERATOR_V8) {
            auto hole = circuit_->GetConstantGate(MachineType::I64,
                                                  JSTaggedValue::VALUE_HOLE,
                                                  GateType::TaggedValue());
            uint32_t numRegs = GetNumberVRegsWithEnv();
            std::vector<GateRef> vec(numRegs + 1, hole);
            vec[0] = depend;
            GateRef saveRegs =
                circuit_->NewGate(circuit_->SaveRegister(numRegs), vec);
            gateAcc_.ReplaceDependIn(gate, saveRegs);
        }
        suspendAndResumeGates_.emplace_back(gate);
    }
    depend = gate;
    if (bytecodeInfo.IsThrow()) {
        auto constant = circuit_->GetConstantGate(MachineType::I64,
                                                  JSTaggedValue::VALUE_EXCEPTION,
                                                  GateType::TaggedValue());
        circuit_->NewGate(circuit_->Return(),
            { state, depend, constant, circuit_->GetReturnRoot() });
        return;
    }
    if (iterator.Index() == bb.end) {
        auto &bbNext = graph_[bb.id + 1];
        auto isLoopBack = bbNext.loopbackBlocks.count(bb.id);
        SetBlockPred(bbNext, state, depend, isLoopBack);
        bbNext.expandedPreds.push_back({bb.id, iterator.Index(), false});
    }
}

void BytecodeCircuitBuilder::NewJump(BytecodeRegion &bb, GateRef &state, GateRef &depend)
{
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    size_t numValueInputs = bytecodeInfo.ComputeValueInputCount();
    auto offset = GetJumpOffset(iterator.Index());
    if (bytecodeInfo.IsCondJump()) {
        ASSERT(!bytecodeInfo.Deopt());
        auto meta = circuit_->JSBytecode(numValueInputs,
            bytecodeInfo.GetOpcode(), iterator.Index(), false);
        auto numValues = meta->GetNumIns();
        GateRef gate = circuit_->NewGate(meta, std::vector<GateRef>(numValues, Circuit::NullGate()));
        gateAcc_.NewIn(gate, 0, state);
        gateAcc_.NewIn(gate, 1, depend);
        auto ifTrue = circuit_->NewGate(circuit_->IfTrue(), {gate});
        auto trueRelay = circuit_->NewGate(circuit_->DependRelay(), {ifTrue, gate});
        if (offset < 0) {
            // place update hotness Gate when offset is negative.
            auto offsetGate = circuit_->GetConstantGate(MachineType::I32,
                                                        offset,
                                                        GateType::NJSValue());
            ifTrue = circuit_->NewGate(circuit_->UpdateHotness(), {ifTrue, trueRelay, offsetGate});
            trueRelay = ifTrue;
        }
        auto ifFalse = circuit_->NewGate(circuit_->IfFalse(), {gate});
        auto falseRelay = circuit_->NewGate(circuit_->DependRelay(), {ifFalse, gate});
        if (bb.succs.size() == 1) {
            auto &bbNext = bb.succs[0];
            ASSERT(bbNext->id == bb.id + 1);
            auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
            SetBlockPred(*bbNext, ifFalse, falseRelay, isLoopBack);
            SetBlockPred(*bbNext, ifTrue, trueRelay, isLoopBack);
            bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
            bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
        } else {
            ASSERT(bb.succs.size() == 2); // 2 : 2 num of successors
            [[maybe_unused]] uint32_t bitSet = 0;
            for (auto &bbNext: bb.succs) {
                if (bbNext->id == bb.id + 1) {
                    auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
                    SetBlockPred(*bbNext, ifFalse, falseRelay, isLoopBack);
                    bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
                    bitSet |= 1;
                } else {
                    auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
                    SetBlockPred(*bbNext, ifTrue, trueRelay, isLoopBack);
                    bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
                    bitSet |= 2; // 2:verify
                }
            }
            ASSERT(bitSet == 3); // 3:Verify the number of successor blocks
        }
        byteCodeToJSGate_[iterator.Index()] = gate;
    } else {
        ASSERT(bb.succs.size() == 1);
        auto &bbNext = bb.succs.at(0);
        auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
        if (offset < 0) {
            // place update hotness Gate when offset is negative.
            auto offsetGate = circuit_->GetConstantGate(MachineType::I32,
                                                        offset,
                                                        GateType::NJSValue());
            auto updateHotness = circuit_->NewGate(circuit_->UpdateHotness(), {state, depend, offsetGate});
            SetBlockPred(*bbNext, updateHotness, updateHotness, isLoopBack);
        } else {
            SetBlockPred(*bbNext, state, depend, isLoopBack);
        }
        bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
    }
}

void BytecodeCircuitBuilder::NewReturn(BytecodeRegion &bb, GateRef &state, GateRef &depend)
{
    ASSERT(bb.succs.empty());
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    auto offsetGate = circuit_->GetConstantGate(MachineType::I32,
                                                GetJumpOffset(iterator.Index()),
                                                GateType::NJSValue());
    auto updateHotness = circuit_->NewGate(circuit_->UpdateHotness(), {state, depend, offsetGate});
    if (bytecodeInfo.GetOpcode() == EcmaOpcode::RETURN) {
        // handle return.dyn bytecode
        auto gate = circuit_->NewGate(circuit_->Return(),
            { updateHotness, updateHotness, Circuit::NullGate(), circuit_->GetReturnRoot() });
        byteCodeToJSGate_[iterator.Index()] = gate;
    } else if (bytecodeInfo.GetOpcode() == EcmaOpcode::RETURNUNDEFINED) {
        // handle returnundefined bytecode
        auto constant = circuit_->GetConstantGate(MachineType::I64,
                                                  JSTaggedValue::VALUE_UNDEFINED,
                                                  GateType::TaggedValue());
        auto gate = circuit_->NewGate(circuit_->Return(),
            { updateHotness, updateHotness, constant, circuit_->GetReturnRoot() });
        byteCodeToJSGate_[iterator.Index()] = gate;
    }
}

void BytecodeCircuitBuilder::NewByteCode(BytecodeRegion &bb, GateRef &state, GateRef &depend)
{
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    if (bytecodeInfo.IsSetConstant()) {
        // handle bytecode command to get constants
        GateRef gate = NewConst(bytecodeInfo);
        byteCodeToJSGate_[iterator.Index()] = gate;
        if (iterator.Index() == bb.end) {
            auto &bbNext = graph_[bb.id + 1];
            auto isLoopBack = bbNext.loopbackBlocks.count(bb.id);
            SetBlockPred(bbNext, state, depend, isLoopBack);
            bbNext.expandedPreds.push_back({bb.id, iterator.Index(), false});
        }
    } else if (bytecodeInfo.IsGeneral()) {
        // handle general ecma.* bytecodes
        NewJSGate(bb, state, depend);
    } else if (bytecodeInfo.IsJump()) {
        // handle conditional jump and unconditional jump bytecodes
        NewJump(bb, state, depend);
    } else if (bytecodeInfo.IsReturn()) {
        // handle return.dyn and returnundefined bytecodes
        NewReturn(bb, state, depend);
    } else if (bytecodeInfo.IsMov()) {
        // handle mov.dyn lda.dyn sta.dyn bytecodes
        if (iterator.Index() == bb.end) {
            auto &bbNext = graph_[bb.id + 1];
            auto isLoopBack = bbNext.loopbackBlocks.count(bb.id);
            SetBlockPred(bbNext, state, depend, isLoopBack);
            bbNext.expandedPreds.push_back({bb.id, iterator.Index(), false});
        }
    } else if (bytecodeInfo.IsDiscarded()) {
        return;
    } else {
        UNREACHABLE();
    }
}

void BytecodeCircuitBuilder::BuildSubCircuit()
{
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        auto stateCur = bb.stateStart;
        auto dependCur = bb.dependStart;
        ASSERT(stateCur != Circuit::NullGate());
        ASSERT(dependCur != Circuit::NullGate());
        if (!bb.trys.empty()) {
            dependCur = circuit_->NewGate(circuit_->GetException(),
                MachineType::I64, {dependCur}, GateType::AnyType());
        }
        EnumerateBlock(bb, [this, &stateCur, &dependCur, &bb]
            (const BytecodeInfo &bytecodeInfo) -> bool {
            NewByteCode(bb, stateCur, dependCur);
            if (bytecodeInfo.IsJump() || bytecodeInfo.IsThrow()) {
                return false;
            }
            return true;
        });
    }
}

void BytecodeCircuitBuilder::NewPhi(BytecodeRegion &bb, uint16_t reg, bool acc, GateRef &currentPhi)
{
    if (bb.numOfLoopBacks == 0) {
        auto inList = std::vector<GateRef>(1 + bb.numOfStatePreds, Circuit::NullGate());
        currentPhi =
            circuit_->NewGate(circuit_->ValueSelector(bb.numOfStatePreds), MachineType::I64,
                              inList.size(), inList.data(), GateType::AnyType());
        gateAcc_.NewIn(currentPhi, 0, bb.stateStart);
        for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
            auto &[predId, predBcIdx, isException] = bb.expandedPreds.at(i);
            gateAcc_.NewIn(currentPhi, i + 1, ResolveDef(predId, predBcIdx, reg, acc));
        }
    } else {
        // 2: the number of value inputs and it is in accord with LOOP_BEGIN
        currentPhi = circuit_->NewGate(circuit_->ValueSelector(2), MachineType::I64,
                                      {bb.stateStart, Circuit::NullGate(), Circuit::NullGate()}, GateType::AnyType());
        auto inList = std::vector<GateRef>(1 + bb.numOfLoopBacks, Circuit::NullGate());
        auto loopBackValue = circuit_->NewGate(circuit_->ValueSelector(bb.numOfLoopBacks),
            MachineType::I64, inList.size(), inList.data(), GateType::AnyType());
        gateAcc_.NewIn(loopBackValue, 0, bb.mergeLoopBackEdges);
        size_t loopBackIndex = 1;  // 1: start index of value inputs
        for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
            auto &[predId, predBcIdx, isException] = bb.expandedPreds.at(i);
            if (bb.loopbackBlocks.count(predId)) {
                gateAcc_.NewIn(loopBackValue, loopBackIndex++, ResolveDef(predId, predBcIdx, reg, acc));
            }
        }
        inList = std::vector<GateRef>(1 + bb.numOfStatePreds - bb.numOfLoopBacks, Circuit::NullGate());
        auto forwardValue = circuit_->NewGate(
            circuit_->ValueSelector(bb.numOfStatePreds - bb.numOfLoopBacks), MachineType::I64,
            inList.size(), inList.data(), GateType::AnyType());
        gateAcc_.NewIn(forwardValue, 0, bb.mergeForwardEdges);
        size_t forwardIndex = 1;  // 1: start index of value inputs
        for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
            auto &[predId, predBcIdx, isException] = bb.expandedPreds.at(i);
            if (!bb.loopbackBlocks.count(predId)) {
                gateAcc_.NewIn(forwardValue, forwardIndex++, ResolveDef(predId, predBcIdx, reg, acc));
            }
        }
        gateAcc_.NewIn(currentPhi, 1, forwardValue);   // 1: index of forward value input
        gateAcc_.NewIn(currentPhi, 2, loopBackValue);  // 2: index of loop-back value input
    }
}

// recursive variables renaming algorithm
GateRef BytecodeCircuitBuilder::ResolveDef(const size_t bbId, int32_t bcId, const uint16_t reg, const bool acc)
{
    auto tmpReg = reg;
    // find def-site in bytecodes of basic block
    auto ans = Circuit::NullGate();
    auto &bb = graph_.at(bbId);
    GateType type = GateType::AnyType();
    auto tmpAcc = acc;

    BytecodeIterator iterator(this, bb.start, bcId);
    for (iterator.Goto(bcId); !iterator.Done(); --iterator) {
        const BytecodeInfo& curInfo = iterator.GetBytecodeInfo();
        // original bc use acc as input && current bc use acc as output
        bool isTransByAcc = tmpAcc && curInfo.AccOut();
        // 0 : the index in vreg-out list
        bool isTransByVreg = (!tmpAcc && curInfo.IsOut(tmpReg, 0));
        if (isTransByAcc || isTransByVreg) {
            if (curInfo.IsMov()) {
                tmpAcc = curInfo.AccIn();
                if (!curInfo.inputs.empty()) {
                    ASSERT(!tmpAcc);
                    ASSERT(curInfo.inputs.size() == 1);
                    tmpReg = std::get<VirtualRegister>(curInfo.inputs.at(0)).GetId();
                }
                if (HasTypes()) {
                    type = typeRecorder_.UpdateType(iterator.Index(), type);
                }
            } else {
                ans = byteCodeToJSGate_.at(iterator.Index());
                auto oldType = gateAcc_.GetGateType(ans);
                if (HasTypes() && !type.IsAnyType() && oldType.IsAnyType()) {
                    gateAcc_.SetGateType(ans, type);
                }
                break;
            }
        }
        if (curInfo.GetOpcode() != EcmaOpcode::RESUMEGENERATOR) {
            continue;
        }
        // New RESTORE_REGISTER HIR, used to restore the register content when processing resume instruction.
        // New SAVE_REGISTER HIR, used to save register content when processing suspend instruction.
        auto resumeGate = byteCodeToJSGate_.at(iterator.Index());
        ans = GetExistingRestore(resumeGate, tmpReg);
        if (ans != Circuit::NullGate()) {
            break;
        }
        GateRef resumeDependGate = gateAcc_.GetDep(resumeGate);
        ans = circuit_->NewGate(circuit_->RestoreRegister(tmpReg), MachineType::I64,
                                { resumeDependGate }, GateType::AnyType());
        SetExistingRestore(resumeGate, tmpReg, ans);
        gateAcc_.SetDep(resumeGate, ans);
        auto saveRegGate = ResolveDef(bbId, iterator.Index() - 1, tmpReg, tmpAcc);
        ASSERT(Bytecodes::GetOpcode(iterator.PeekPrevPc(2)) == EcmaOpcode::SUSPENDGENERATOR_V8); // 2: prev bc
        GateRef suspendGate = byteCodeToJSGate_.at(iterator.Index() - 2); // 2: prev bc
        GateRef saveRegs = gateAcc_.GetDep(suspendGate);
        gateAcc_.ReplaceValueIn(saveRegs, saveRegGate, tmpReg);
        break;
    }
    // find GET_EXCEPTION gate if this is a catch block
    if (ans == Circuit::NullGate() && tmpAcc) {
        if (!bb.trys.empty()) {
            std::vector<GateRef> outList;
            gateAcc_.GetOuts(bb.dependStart, outList);
            ASSERT(outList.size() == 1);
            const auto &getExceptionGate = outList.at(0);
            ASSERT(gateAcc_.GetOpCode(getExceptionGate) == OpCode::GET_EXCEPTION);
            ans = getExceptionGate;
        }
    }
    // find def-site in value selectors of vregs
    if (ans == Circuit::NullGate() && !tmpAcc && bb.phi.count(tmpReg)) {
        if (!bb.vregToValSelectorGate.count(tmpReg)) {
            NewPhi(bb, tmpReg, tmpAcc, bb.vregToValSelectorGate[tmpReg]);
        }
        ans = bb.vregToValSelectorGate.at(tmpReg);
    }
    // find def-site in value selectors of acc
    if (ans == Circuit::NullGate() && tmpAcc && bb.phiAcc) {
        if (bb.valueSelectorAccGate == Circuit::NullGate()) {
            NewPhi(bb, tmpReg, tmpAcc, bb.valueSelectorAccGate);
        }
        ans = bb.valueSelectorAccGate;
    }
    if (ans == Circuit::NullGate() && IsEntryBlock(bbId)) { // entry block
        // find def-site in function args
        ASSERT(!tmpAcc);
        if (tmpReg == GetEnvVregIdx()) {
            ans = argAcc_.GetCommonArgGate(CommonArgIdx::LEXENV);
        } else {
            ans = argAcc_.GetArgGate(tmpReg);
        }
        return ans;
    }
    if (ans == Circuit::NullGate()) {
        // recursively find def-site in dominator block
        return ResolveDef(bb.iDominator->id, bb.iDominator->end, tmpReg, tmpAcc);
    } else {
        // def-site already found
        return ans;
    }
}

void BytecodeCircuitBuilder::BuildCircuit()
{
    // create arg gates array
    BuildCircuitArgs();
    CollectPredsInfo();
    BuildBlockCircuitHead();
    // build states sub-circuit of each block
    BuildSubCircuit();
    // verification of soundness of CFG
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        ASSERT(bb.statePredIndex == bb.numOfStatePreds);
        ASSERT(bb.loopBackIndex == bb.numOfLoopBacks);
        if (bb.numOfLoopBacks) {
            ASSERT(bb.forwardIndex == bb.numOfStatePreds - bb.numOfLoopBacks);
        }
        // resolve def-site of virtual regs and set all value inputs
        EnumerateBlock(bb, [&](const BytecodeInfo &bytecodeInfo) -> bool {
            auto &iterator = bb.GetBytecodeIterator();
            const auto bcIndex = iterator.Index();
            const auto bbIndex = bb.id;
            GateRef gate = GetGateByBcIndex(bcIndex);
            if (gate == Circuit::NullGate()) {
                return true;
            }
            if (gateAcc_.IsConstant(gate)) {
                return true;
            }

            if (HasTypes()) {
                auto type = typeRecorder_.GetType(bcIndex);
                if (!type.IsAnyType()) {
                    gateAcc_.SetGateType(gate, type);
                }
            }
            auto valueCount = gateAcc_.GetInValueCount(gate);
            [[maybe_unused]] size_t numValueInputs = bytecodeInfo.ComputeValueInputCount();
            [[maybe_unused]] size_t numValueOutputs = bytecodeInfo.ComputeOutCount();
            ASSERT(numValueInputs == valueCount);
            ASSERT(numValueOutputs <= 1 + (bytecodeInfo.EnvOut() ? 1 : 0));
            auto valueStarts = gateAcc_.GetInValueStarts(gate);
            for (size_t valueIdx = 0; valueIdx < valueCount; valueIdx++) {
                auto inIdx = valueIdx + valueStarts;
                if (!gateAcc_.IsInGateNull(gate, inIdx)) {
                    continue;
                }
                if (valueIdx < bytecodeInfo.inputs.size()) {
                    auto vregId = std::get<VirtualRegister>(bytecodeInfo.inputs.at(valueIdx)).GetId();
                    GateRef defVreg = Circuit::NullGate();
                    if (IsFirstBCEnvIn(bbIndex, bcIndex, vregId)) {
                        defVreg = argAcc_.GetCommonArgGate(CommonArgIdx::LEXENV);
                    } else {
                        defVreg = ResolveDef(bbIndex, bcIndex - 1, vregId, false);
                    }
                    gateAcc_.NewIn(gate, inIdx, defVreg);
                } else {
                    GateRef defAcc = ResolveDef(bbIndex, bcIndex - 1, 0, true);
                    gateAcc_.NewIn(gate, inIdx, defAcc);
                }
            }
            return true;
        });
    }

    if (IsTypeLoweringEnabled()) {
        frameStateBuilder_.BuildFrameState();
    }

    if (IsLogEnabled()) {
        PrintGraph("Bytecode2Gate");
        LOG_COMPILER(INFO) << "\033[34m" << "============= "
                           << "After bytecode2circuit lowering ["
                           << methodName_ << "]"
                           << " =============" << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "=========================== End ===========================" << "\033[0m";
    }
}

GateRef BytecodeCircuitBuilder::GetExistingRestore(GateRef resumeGate, uint16_t tmpReg) const
{
    auto pr = std::make_pair(resumeGate, tmpReg);
    if (resumeRegToRestore_.count(pr)) {
        return resumeRegToRestore_.at(pr);
    }
    return Circuit::NullGate();
}

void BytecodeCircuitBuilder::SetExistingRestore(GateRef resumeGate, uint16_t tmpReg, GateRef restoreGate)
{
    auto pr = std::make_pair(resumeGate, tmpReg);
    resumeRegToRestore_[pr] = restoreGate;
}

void BytecodeCircuitBuilder::PrintGraph(const char* title)
{
    LOG_COMPILER(INFO) << "======================== " << title << " ========================";
    for (size_t i = 0; i < graph_.size(); i++) {
        BytecodeRegion& bb = graph_[i];
        if (bb.isDead) {
            LOG_COMPILER(INFO) << "B" << bb.id << ":                               ;preds= invalid BB";
            LOG_COMPILER(INFO) << "\tBytecodePC: [" << std::to_string(bb.start) << ", "
                               << std::to_string(bb.end) << ")";
            continue;
        }
        std::string log("B" + std::to_string(bb.id) + ":                               ;preds= ");
        for (size_t k = 0; k < bb.preds.size(); ++k) {
            log += std::to_string(bb.preds[k]->id) + ", ";
        }
        LOG_COMPILER(INFO) << log;
        LOG_COMPILER(INFO) << "\tBytecodePC: [" << std::to_string(bb.start) << ", "
                           << std::to_string(bb.end) << ")";

        std::string log1("\tSucces: ");
        for (size_t j = 0; j < bb.succs.size(); j++) {
            log1 += std::to_string(bb.succs[j]->id) + ", ";
        }
        LOG_COMPILER(INFO) << log1;

        for (size_t j = 0; j < bb.catchs.size(); j++) {
            LOG_COMPILER(INFO) << "\tcatch [: " << std::to_string(bb.catchs[j]->start) << ", "
                               << std::to_string(bb.catchs[j]->end) << ")";
        }

        std::string log2("\tTrys: ");
        for (auto tryBlock: bb.trys) {
            log2 += std::to_string(tryBlock->id) + " , ";
        }
        LOG_COMPILER(INFO) << log2;

        std::string log3 = "\tDom: ";
        for (size_t j = 0; j < bb.immDomBlocks.size(); j++) {
            log3 += "B" + std::to_string(bb.immDomBlocks[j]->id) + std::string(", ");
        }
        LOG_COMPILER(INFO) << log3;

        if (bb.iDominator) {
            LOG_COMPILER(INFO) << "\tIDom B" << bb.iDominator->id;
        }

        std::string log4("\tDom Frontiers: ");
        for (const auto &frontier: bb.domFrontiers) {
            log4 += std::to_string(frontier->id) + " , ";
        }
        LOG_COMPILER(INFO) << log4;

        std::string log5("\tPhi: ");
        for (auto variable: bb.phi) {
            log5 += std::to_string(variable) + " , ";
        }
        LOG_COMPILER(INFO) << log5;

        PrintBytecodeInfo(bb);
        LOG_COMPILER(INFO) << "";
    }
}

void BytecodeCircuitBuilder::PrintBytecodeInfo(BytecodeRegion& bb)
{
    if (bb.isDead) {
        return;
    }
    LOG_COMPILER(INFO) << "\tBytecode[] = ";
    EnumerateBlock(bb, [&](const BytecodeInfo &bytecodeInfo) -> bool {
        auto &iterator = bb.GetBytecodeIterator();
        std::string log;
        log += std::string("\t\t< ") + std::to_string(iterator.Index()) + ": ";
        log += GetEcmaOpcodeStr(iterator.GetBytecodeInfo().GetOpcode()) + ", " + "In=[";
        if (bytecodeInfo.AccIn()) {
            log += "acc,";
        }
        for (const auto &in: bytecodeInfo.inputs) {
            if (std::holds_alternative<VirtualRegister>(in)) {
                log += std::to_string(std::get<VirtualRegister>(in).GetId()) + ",";
            }
        }
        log += "], Out=[";
        if (bytecodeInfo.AccOut()) {
            log += "acc,";
        }
        for (const auto &out: bytecodeInfo.vregOut) {
            log +=  std::to_string(out) + ",";
        }
        log += "] >";
        LOG_COMPILER(INFO) << log;

        auto gate = byteCodeToJSGate_[iterator.Index()];
        if (gate != Circuit::NullGate()) {
            this->gateAcc_.ShortPrint(gate);
        }
        return true;
    });
}
}  // namespace panda::ecmascript::kungfu
