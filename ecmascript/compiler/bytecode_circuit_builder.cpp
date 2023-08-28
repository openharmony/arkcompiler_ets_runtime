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
    uint32_t end = size - 2;  // 1: end
    BytecodeIterator iterator(this, 0, end);

    infoData_.resize(size);
    byteCodeToJSGates_.resize(size, std::vector<GateRef>(0));
    regionsInfo_.InsertHead(0); // 0: start pc
    for (iterator.GotoStart(); !iterator.Done(); ++iterator) {
        auto index = iterator.Index();
        auto &info = infoData_[index];
        auto pc = pcOffsets_[index];
        info.metaData_ = bytecodes_->GetBytecodeMetaData(pc);
        ASSERT(!info.metaData_.IsInvalid());
        BytecodeInfo::InitBytecodeInfo(this, info, pc);
        CollectRegionInfo(index);
    }
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
                LOG_ECMA(FATAL) << "this branch is unreachable";
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
            if (bcIndex != GetLastBcIndex()) {
                regionsInfo_.InsertHead(nextIndex);
            }
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
    auto pf = file_->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pf, method_->GetMethodId());
    panda_file::CodeDataAccessor cda(*pf, mda.GetCodeId().value());

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

void BytecodeCircuitBuilder::BuildEntryBlock()
{
    BytecodeRegion &entryBlock = graph_[0];
    BytecodeRegion &nextBlock = graph_[1];
    entryBlock.succs.emplace_back(&nextBlock);
    nextBlock.preds.emplace_back(&entryBlock);
    entryBlock.bytecodeIterator_.Reset(this, INVALID_INDEX, INVALID_INDEX);
}

void BytecodeCircuitBuilder::BuildRegions(const ExceptionInfo &byteCodeException)
{
    auto &items = regionsInfo_.GetBlockItems();
    auto blockSize = items.size();

    // 1 : entry block. if the loop head is in the first bb block, the variables used in the head cannot correctly
    // generate Phi nodes through the dominator-tree algorithm, resulting in an infinite loop. Therefore, an empty
    // BB block is generated as an entry block
    graph_.resize(blockSize + 1);

    // build entry block
    BuildEntryBlock();

    // build basic block
    size_t blockId = 1;
    for (const auto &item : items) {
        auto &curBlock = GetBasicBlockById(blockId);
        curBlock.id = blockId;
        curBlock.start = item.GetStartBcIndex();
        if (blockId != 1) {
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
    if (!method_->IsFastCall()) {
        argAcc_.NewCommonArg(CommonArgIdx::ACTUAL_ARGC, MachineType::I64, GateType::NJSValue());
        auto funcIdx = static_cast<size_t>(CommonArgIdx::FUNC);
        const size_t actualNumArgs = argAcc_.GetActualNumArgs();
        // new actual argument gates
        for (size_t argIdx = funcIdx; argIdx < actualNumArgs; argIdx++) {
            argAcc_.NewArg(argIdx);
        }
    } else {
        auto funcIdx = static_cast<size_t>(FastCallArgIdx::FUNC);
        size_t actualNumArgs = static_cast<size_t>(FastCallArgIdx::NUM_OF_ARGS) + method_->GetNumArgsWithCallField();
        for (size_t argIdx = funcIdx; argIdx < actualNumArgs; argIdx++) {
            argAcc_.NewArg(argIdx);
        }
    }
    argAcc_.CollectArgs();
    if (HasTypes()) {
        argAcc_.FillArgsGateType(&typeRecorder_);
    }

    BuildFrameArgs();
}

void BytecodeCircuitBuilder::BuildFrameArgs()
{
    auto metaData = circuit_->FrameArgs();
    size_t numArgs = static_cast<size_t>(FrameArgIdx::NUM_OF_ARGS);
    std::vector<GateRef> args(numArgs, Circuit::NullGate());
    size_t idx = 0;
    args[idx++] = argAcc_.GetCommonArgGate(CommonArgIdx::FUNC);
    args[idx++] = argAcc_.GetCommonArgGate(CommonArgIdx::NEW_TARGET);
    args[idx++] = argAcc_.GetCommonArgGate(CommonArgIdx::THIS_OBJECT);
    args[idx++] = argAcc_.GetCommonArgGate(CommonArgIdx::ACTUAL_ARGC);
    GateRef frameArgs = circuit_->NewGate(metaData, args);
    argAcc_.SetFrameArgs(frameArgs);
}

bool BytecodeCircuitBuilder::ShouldBeDead(BytecodeRegion &curBlock)
{
    if (curBlock.iDominator->isDead) {
        return true;
    }
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
                if (!bb.catchs.empty() && !bytecodeInfo.NoThrow()) {
                    noThrow = false;
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

    CollectLoopBack();
    if (EnableLoopOptimization()) {
        for (auto &head : loopHeads_) {
            loopSize_ = 0;
            ComputeLoopDepth(head.second);
            head.first = loopSize_;
        }
        sort(loopHeads_.begin(), loopHeads_.end());
    }

    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        bb.phiAcc = (bb.numOfStatePreds > 1) || (!bb.trys.empty());
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

void BytecodeCircuitBuilder::NewLoopBegin(BytecodeRegion &bb, GateRef &state, GateRef &depend)
{
    if (bb.numOfLoopBacks > 1) {
        NewMerge(bb.loopBackStateMerge, bb.loopBackDependMerge, bb.numOfLoopBacks);
    }
    auto loopBack = circuit_->NewGate(circuit_->LoopBack(), { Circuit::NullGate() });
    auto loopBegin = circuit_->NewGate(circuit_->LoopBegin(), { Circuit::NullGate(), loopBack });
    // 2: the number of depend inputs and it is in accord with LOOP_BEGIN
    auto loopDepend = circuit_->NewGate(circuit_->DependSelector(2),
        { loopBegin, Circuit::NullGate(), Circuit::NullGate() });
    if (state == circuit_->GetStateRoot()) {
        ASSERT(depend == circuit_->GetDependRoot());
        gateAcc_.NewIn(loopBegin, 0, state);
        gateAcc_.NewIn(loopDepend, 1, depend);
    }
    state = loopBegin;
    depend = loopDepend;
}

void BytecodeCircuitBuilder::NewLoopExit(GateRef &state, GateRef &depend)
{
    auto loopExit = circuit_->NewGate(circuit_->LoopExit(),
        { state });
    depend = circuit_->NewGate(circuit_->LoopExitDepend(),
        { loopExit, depend });
    state = loopExit;
}

void BytecodeCircuitBuilder::TryInsertLoopExit(BytecodeRegion &bb, BytecodeRegion &bbNext,
                                               GateRef &state, GateRef &depend)
{
    size_t diff = LoopExitCount(bb.id, bbNext.id);
    for (size_t i = 0; i < diff; ++i) {
        NewLoopExit(state, depend);
    }
}

void BytecodeCircuitBuilder::BuildBlockCircuitHead(BytecodeRegion &bb, GateRef &state, GateRef &depend)
{
    auto mergeCount = bb.numOfStatePreds - bb.numOfLoopBacks;
    if (mergeCount == 0) {
        state = circuit_->GetStateRoot();
        depend = circuit_->GetDependRoot();
    }

    if (mergeCount > 1) {
        NewMerge(bb.stateMerge, bb.dependMerge, mergeCount);
        state = bb.stateMerge;
        depend = bb.dependMerge;
    }

    if (bb.numOfLoopBacks > 0) {
        NewLoopBegin(bb, state, depend);
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
            inList[i + length] = circuit_->GetConstantGate(MachineType::I64,
                                                           std::get<ConstDataId>(input).GetId(),
                                                           GateType::NJSValue());
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
    if (meta->HasFrameState()) {
        inList[inputSize + length] = GetFrameArgs();
    }
    return inList;
}

void BytecodeCircuitBuilder::SetLoopBlockPred(BytecodeRegion &bb, BytecodeRegion &bbNext,
                                              GateRef &state, GateRef &depend)
{
    ASSERT(bbNext.numOfLoopBacks > 0);
    ASSERT(gateAcc_.GetOpCode(bbNext.stateCurrent) == OpCode::LOOP_BEGIN);
    ASSERT(gateAcc_.GetOpCode(bbNext.dependCurrent) == OpCode::DEPEND_SELECTOR);
    // loop back
    if (bbNext.loopbackBlocks.count(bb.id)) {
        if (bbNext.loopBackStateMerge != Circuit::NullGate()) {
            ASSERT(bbNext.loopBackDependMerge != Circuit::NullGate());
            gateAcc_.NewIn(bbNext.loopBackStateMerge, bbNext.loopBackIndex, state);
            gateAcc_.NewIn(bbNext.loopBackDependMerge, bbNext.loopBackIndex + 1, depend);
            state = bbNext.loopBackStateMerge;
            depend = bbNext.loopBackDependMerge;
        }
        if (bbNext.loopBackIndex == 0) {
            auto loopBack = gateAcc_.GetState(bbNext.stateCurrent, 1); // 1: LoopBack
            gateAcc_.NewIn(loopBack, 0, state);
            gateAcc_.NewIn(bbNext.dependCurrent, 2, depend); // 2: loopback depend
        }
        bbNext.loopBackIndex++;
        ASSERT(bbNext.loopBackIndex <= bbNext.numOfLoopBacks);
    } else {
        if (bbNext.stateMerge != Circuit::NullGate()) {
            ASSERT(bbNext.dependMerge != Circuit::NullGate());
            gateAcc_.NewIn(bbNext.stateMerge, bbNext.forwardIndex, state);
            gateAcc_.NewIn(bbNext.dependMerge, bbNext.forwardIndex + 1, depend);
            state = bbNext.stateMerge;
            depend = bbNext.dependMerge;
        }
        if (bbNext.forwardIndex == 0) {
            gateAcc_.NewIn(bbNext.stateCurrent, 0, state);
            gateAcc_.NewIn(bbNext.dependCurrent, 1, depend); // 1: first depend
        }
        bbNext.forwardIndex++;
        ASSERT(bbNext.forwardIndex <= bbNext.numOfStatePreds - bbNext.numOfLoopBacks);
    }
}

void BytecodeCircuitBuilder::SetBlockPred(BytecodeRegion &bb, BytecodeRegion &bbNext,
                                          const GateRef &state, const GateRef &depend)
{
    auto stateCur = state;
    auto dependCur = depend;

    if (EnableLoopOptimization()) {
        TryInsertLoopExit(bb, bbNext, stateCur, dependCur);
    }

    // Init block head if not exists
    if (bbNext.stateCurrent == Circuit::NullGate()) {
        ASSERT(bbNext.dependCurrent == Circuit::NullGate());
        BuildBlockCircuitHead(bbNext, bbNext.stateCurrent, bbNext.dependCurrent);
        // no loop head, no merge bb
        if (bbNext.stateCurrent == Circuit::NullGate()) {
            ASSERT(bbNext.dependCurrent == Circuit::NullGate());
            bbNext.stateCurrent = stateCur;
            bbNext.dependCurrent = dependCur;
            bbNext.statePredIndex++;
            return;
        }
    }

    // loop bb
    if (bbNext.numOfLoopBacks > 0) {
        SetLoopBlockPred(bb, bbNext, stateCur, dependCur);
        bbNext.statePredIndex++;
        return;
    }

    // merge bb
    if (bbNext.stateMerge != Circuit::NullGate()) {
        ASSERT(bbNext.dependMerge != Circuit::NullGate());
        gateAcc_.NewIn(bbNext.stateMerge, bbNext.statePredIndex, stateCur);
        gateAcc_.NewIn(bbNext.dependMerge, bbNext.statePredIndex + 1, dependCur); // 1: skip state
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
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
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
    bool hasFrameState = bytecodeInfo.HasFrameArgs();
    size_t pcOffset = GetPcOffset(iterator.Index());
    auto meta = circuit_->JSBytecode(numValueInputs, bytecodeInfo.GetOpcode(), pcOffset, writable, hasFrameState);
    std::vector<GateRef> inList = CreateGateInList(bytecodeInfo, meta);
    if (bytecodeInfo.IsDef()) {
        gate = circuit_->NewGate(meta, MachineType::I64, inList.size(),
                                 inList.data(), GateType::AnyType());
    } else {
        gate = circuit_->NewGate(meta, MachineType::NOVALUE, inList.size(),
                                 inList.data(), GateType::Empty());
    }
    byteCodeToJSGates_[iterator.Index()].emplace_back(gate);
    jsGatesToByteCode_[gate] = iterator.Index();
    gateAcc_.NewIn(gate, 0, state);
    gateAcc_.NewIn(gate, 1, depend);
    state = gate;
    if (bytecodeInfo.IsThrow()) {
        depend = gate;

        if (!bb.catchs.empty()) {
            auto &bbNext = bb.catchs.at(0);
            SetBlockPred(bb, *bbNext, gate, gate);
            bbNext->expandedPreds.push_back({bb.id, iterator.Index(), true});
        } else {
            auto constant = circuit_->GetConstantGate(MachineType::I64,
                                                      JSTaggedValue::VALUE_EXCEPTION,
                                                      GateType::TaggedValue());
            circuit_->NewGate(circuit_->Return(),
                { state, depend, constant, circuit_->GetReturnRoot() });
        }
        return;
    }
    if (!bb.catchs.empty() && !bytecodeInfo.NoThrow()) {
        auto ifSuccess = circuit_->NewGate(circuit_->IfSuccess(), {gate});
        auto ifException = circuit_->NewGate(circuit_->IfException(), {gate, gate});

        auto &bbNext = bb.catchs.at(0);
        SetBlockPred(bb, *bbNext, ifException, ifException);
        if (bytecodeInfo.GetOpcode() == EcmaOpcode::CREATEASYNCGENERATOROBJ_V8) {
            bbNext->expandedPreds.push_back({bb.id, iterator.Index() + 1, true}); // 1: next pc
        } else {
            bbNext->expandedPreds.push_back({bb.id, iterator.Index(), true});
        }
        state = ifSuccess;
    }
    if (bytecodeInfo.IsGeneratorRelative()) {
        //exclude...
        if (bytecodeInfo.GetOpcode() == EcmaOpcode::SUSPENDGENERATOR_V8 ||
            bytecodeInfo.GetOpcode() == EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8 ||
            bytecodeInfo.GetOpcode() == EcmaOpcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8) {
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
}

void BytecodeCircuitBuilder::NewJump(BytecodeRegion &bb, GateRef &state, GateRef &depend)
{
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    size_t numValueInputs = bytecodeInfo.ComputeValueInputCount();
    if (bytecodeInfo.IsCondJump()) {
        size_t pcOffset = GetPcOffset(iterator.Index());
        auto meta = circuit_->JSBytecode(numValueInputs, bytecodeInfo.GetOpcode(), pcOffset, false, false);
        auto numValues = meta->GetNumIns();
        GateRef gate = circuit_->NewGate(meta, std::vector<GateRef>(numValues, Circuit::NullGate()));
        gateAcc_.NewIn(gate, 0, state);
        gateAcc_.NewIn(gate, 1, depend);
        auto ifTrue = circuit_->NewGate(circuit_->IfTrue(), {gate});
        auto trueRelay = circuit_->NewGate(circuit_->DependRelay(), {ifTrue, gate});
        auto ifFalse = circuit_->NewGate(circuit_->IfFalse(), {gate});
        auto falseRelay = circuit_->NewGate(circuit_->DependRelay(), {ifFalse, gate});
        if (bb.succs.size() == 1) {
            auto &bbNext = bb.succs[0];
            ASSERT(bbNext->id == bb.id + 1);
            SetBlockPred(bb, *bbNext, ifFalse, falseRelay);
            SetBlockPred(bb, *bbNext, ifTrue, trueRelay);
            bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
            bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
        } else {
            ASSERT(bb.succs.size() == 2); // 2 : 2 num of successors
            [[maybe_unused]] uint32_t bitSet = 0;
            for (auto &bbNext: bb.succs) {
                if (bbNext->id == bb.id + 1) {
                    SetBlockPred(bb, *bbNext, ifFalse, falseRelay);
                    bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
                    bitSet |= 1;
                } else {
                    SetBlockPred(bb, *bbNext, ifTrue, trueRelay);
                    bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
                    bitSet |= 2; // 2:verify
                }
            }
            ASSERT(bitSet == 3); // 3:Verify the number of successor blocks
        }
        byteCodeToJSGates_[iterator.Index()].emplace_back(gate);
        jsGatesToByteCode_[gate] = iterator.Index();
    } else {
        ASSERT(bb.succs.size() == 1);
        auto &bbNext = bb.succs.at(0);
        SetBlockPred(bb, *bbNext, state, depend);
        bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
    }
}

void BytecodeCircuitBuilder::NewReturn(BytecodeRegion &bb, GateRef &state, GateRef &depend)
{
    ASSERT(bb.succs.empty());
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    if (bytecodeInfo.GetOpcode() == EcmaOpcode::RETURN) {
        // handle return.dyn bytecode
        auto gate = circuit_->NewGate(circuit_->Return(),
            { state, depend, Circuit::NullGate(), circuit_->GetReturnRoot() });
        byteCodeToJSGates_[iterator.Index()].emplace_back(gate);
        jsGatesToByteCode_[gate] = iterator.Index();
    } else if (bytecodeInfo.GetOpcode() == EcmaOpcode::RETURNUNDEFINED) {
        // handle returnundefined bytecode
        auto constant = circuit_->GetConstantGate(MachineType::I64,
                                                  JSTaggedValue::VALUE_UNDEFINED,
                                                  GateType::TaggedValue());
        auto gate = circuit_->NewGate(circuit_->Return(),
            { state, depend, constant, circuit_->GetReturnRoot() });
        byteCodeToJSGates_[iterator.Index()].emplace_back(gate);
        jsGatesToByteCode_[gate] = iterator.Index();
    }
}

void BytecodeCircuitBuilder::NewByteCode(BytecodeRegion &bb, GateRef &state, GateRef &depend)
{
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    if (bytecodeInfo.IsSetConstant()) {
        // handle bytecode command to get constants
        GateRef gate = NewConst(bytecodeInfo);
        byteCodeToJSGates_[iterator.Index()].emplace_back(gate);
        jsGatesToByteCode_[gate] = iterator.Index();
    } else if (bytecodeInfo.IsGeneral()) {
        // handle general ecma.* bytecodes
        NewJSGate(bb, state, depend);
    } else if (bytecodeInfo.IsJump()) {
        // handle conditional jump and unconditional jump bytecodes
        NewJump(bb, state, depend);
    } else if (bytecodeInfo.IsReturn()) {
        // handle return.dyn and returnundefined bytecodes
        NewReturn(bb, state, depend);
    } else if (!bytecodeInfo.IsDiscarded() && !bytecodeInfo.IsMov()) {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void BytecodeCircuitBuilder::BuildSubCircuit()
{
    auto &entryBlock = graph_[0];
    BuildBlockCircuitHead(entryBlock, entryBlock.stateCurrent, entryBlock.dependCurrent);
    for (auto &bbId: dfsList_) {
        auto &bb = graph_[bbId];
        auto stateCur = bb.stateCurrent;
        auto dependCur = bb.dependCurrent;
        ASSERT(stateCur != Circuit::NullGate());
        ASSERT(dependCur != Circuit::NullGate());
        if (IsEntryBlock(bb.id)) {
            if (NeedCheckSafePointAndStackOver()) {
                stateCur = circuit_->NewGate(circuit_->CheckSafePointAndStackOver(), {stateCur, dependCur});
                dependCur = stateCur;
            }
            auto &bbNext = graph_[bb.id + 1];
            SetBlockPred(bb, bbNext, stateCur, dependCur);
            bbNext.expandedPreds.push_back({bb.id, bb.end, false});
            continue;
        }
        if (!bb.trys.empty()) {
            dependCur = circuit_->NewGate(circuit_->GetException(),
                MachineType::I64, {stateCur, dependCur}, GateType::AnyType());
            bb.dependCurrent = dependCur;
        }
        EnumerateBlock(bb, [this, &stateCur, &dependCur, &bb]
            (const BytecodeInfo &bytecodeInfo) -> bool {
            NewByteCode(bb, stateCur, dependCur);
            if (bytecodeInfo.IsJump() || bytecodeInfo.IsThrow()) {
                return false;
            }
            return true;
        });
        const BytecodeInfo& bytecodeInfo = GetBytecodeInfo(bb.end);
        if (bytecodeInfo.needFallThrough()) {
            auto &bbNext = graph_[bb.id + 1];
            SetBlockPred(bb, bbNext, stateCur, dependCur);
            bbNext.expandedPreds.push_back({bb.id, bb.end, false});
        }
    }
}

GateRef BytecodeCircuitBuilder::NewLoopBackPhi(BytecodeRegion &bb, uint16_t reg, bool acc)
{
    if (bb.numOfLoopBacks == 1) {
        for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
            auto &[predId, predBcIdx, isException] = bb.expandedPreds.at(i);
            if (bb.loopbackBlocks.count(predId)) {
                return NewValueFromPredBB(bb, i, gateAcc_.GetState(bb.stateCurrent, 1), reg, acc);
            }
        }
        UNREACHABLE();
        LOG_COMPILER(FATAL) << "this branch is unreachable";
    }
    auto inList = std::vector<GateRef>(1 + bb.numOfLoopBacks, Circuit::NullGate());
    auto loopBackValue = circuit_->NewGate(circuit_->ValueSelector(bb.numOfLoopBacks),
        MachineType::I64, inList.size(), inList.data(), GateType::AnyType());
    gateAcc_.NewIn(loopBackValue, 0, bb.loopBackStateMerge);
    size_t loopBackIndex = 1;  // 1: start index of value inputs
    for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
        auto &[predId, predBcIdx, isException] = bb.expandedPreds.at(i);
        if (bb.loopbackBlocks.count(predId)) {
            GateRef ans = NewValueFromPredBB(bb, i, gateAcc_.GetState(bb.loopBackStateMerge, loopBackIndex - 1),
                                             reg, acc);
            gateAcc_.NewIn(loopBackValue, loopBackIndex++, ans);
        }
    }
    return loopBackValue;
}

size_t BytecodeCircuitBuilder::LoopExitCount(size_t from, size_t to)
{
    if (!EnableLoopOptimization()) {
        return 0;
    }
    const auto &bb = GetBasicBlockById(from);
    const auto &bbNext = GetBasicBlockById(to);
    size_t headDep = ((bbNext.numOfLoopBacks > 0) && (bbNext.loopbackBlocks.count(bb.id) == 0)) ? 1 : 0;
    ASSERT(bbNext.loopDepth >= headDep);
    size_t nextDep = bbNext.loopDepth - headDep;
    ASSERT(bb.loopDepth >= nextDep);
    return bb.loopDepth - nextDep;
}

GateRef BytecodeCircuitBuilder::NewValueFromPredBB(BytecodeRegion &bb, size_t idx,
                                                   GateRef exit, uint16_t reg, bool acc)
{
    auto &[predId, predBcIdx, isException] = bb.expandedPreds.at(idx);
    if (LoopExitCount(predId, bb.id) == 0) {
        return ResolveDef(predId, predBcIdx, reg, acc);
    }
    while (gateAcc_.GetOpCode(exit) != OpCode::LOOP_EXIT) {
        exit = gateAcc_.GetState(exit);
    }
    if (IsLoopExitValueExists(exit, reg, acc)) {
        return GetLoopExitValue(exit, reg, acc);
    }
    GateRef res = ResolveDef(predId, predBcIdx, reg, acc);
    return NewLoopExitValue(exit, reg, acc, res);
}

GateRef BytecodeCircuitBuilder::NewLoopForwardPhi(BytecodeRegion &bb, uint16_t reg, bool acc)
{
    auto mergeCount = bb.numOfStatePreds - bb.numOfLoopBacks;
    if (mergeCount == 1) {
        for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
            auto &[predId, predBcIdx, isException] = bb.expandedPreds.at(i);
            if (!bb.loopbackBlocks.count(predId)) {
                return NewValueFromPredBB(bb, i, gateAcc_.GetState(bb.stateCurrent, 0), reg, acc);
            }
        }
        UNREACHABLE();
        LOG_COMPILER(FATAL) << "this branch is unreachable";
    }
    auto inList = std::vector<GateRef>(1 + mergeCount, Circuit::NullGate());
    auto forwardValue = circuit_->NewGate(
        circuit_->ValueSelector(mergeCount), MachineType::I64,
        inList.size(), inList.data(), GateType::AnyType());
    gateAcc_.NewIn(forwardValue, 0, bb.stateMerge);
    size_t forwardIndex = 1;  // 1: start index of value inputs
    for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
        auto &[predId, predBcIdx, isException] = bb.expandedPreds.at(i);
        if (!bb.loopbackBlocks.count(predId)) {
            GateRef ans = NewValueFromPredBB(bb, i, gateAcc_.GetState(bb.stateMerge, forwardIndex - 1), reg, acc);
            gateAcc_.NewIn(forwardValue, forwardIndex++, ans);
        }
    }
    return forwardValue;
}

void BytecodeCircuitBuilder::NewPhi(BytecodeRegion &bb, uint16_t reg, bool acc, GateRef &currentPhi)
{
    if (bb.numOfLoopBacks == 0) {
        if (bb.numOfStatePreds == 1) {
            currentPhi = NewValueFromPredBB(bb, 0, bb.stateCurrent, reg, acc);
            ASSERT(currentPhi != 0);
            return;
        }
        ASSERT(bb.stateMerge != Circuit::NullGate());
        auto inList = std::vector<GateRef>(1 + bb.numOfStatePreds, Circuit::NullGate());
        currentPhi =
            circuit_->NewGate(circuit_->ValueSelector(bb.numOfStatePreds), MachineType::I64,
                              inList.size(), inList.data(), GateType::AnyType());
        gateAcc_.NewIn(currentPhi, 0, bb.stateMerge);
        for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
            GateRef ans = NewValueFromPredBB(bb, i, gateAcc_.GetIn(bb.stateMerge, i), reg, acc);
            gateAcc_.NewIn(currentPhi, i + 1, ans);
        }
    } else {
        ASSERT(gateAcc_.GetOpCode(bb.stateCurrent) == OpCode::LOOP_BEGIN);
        // 2: the number of value inputs and it is in accord with LOOP_BEGIN
        currentPhi = circuit_->NewGate(circuit_->ValueSelector(2), MachineType::I64,
            {bb.stateCurrent, Circuit::NullGate(), Circuit::NullGate()}, GateType::AnyType());
        auto loopBackValue = NewLoopBackPhi(bb, reg, acc);
        auto forwardValue = NewLoopForwardPhi(bb, reg, acc);
        gateAcc_.NewIn(currentPhi, 1, forwardValue);   // 1: index of forward value input
        gateAcc_.NewIn(currentPhi, 2, loopBackValue);  // 2: index of loop-back value input
    }
    bb.phiGate.insert(currentPhi);
}

bool BytecodeCircuitBuilder::IsLoopExitValueExists(GateRef loopExit, uint16_t reg, bool acc)
{
    if (acc) {
        return loopExitToAccGate_.count(loopExit) > 0;
    } else {
        return loopExitToVregGate_.count(std::make_pair(loopExit, reg)) > 0;
    }
}

GateRef BytecodeCircuitBuilder::GetLoopExitValue(GateRef loopExit, uint16_t reg, bool acc)
{
    if (acc) {
        return loopExitToAccGate_.at(loopExit);
    } else {
        return loopExitToVregGate_.at(std::make_pair(loopExit, reg));
    }
}

GateRef BytecodeCircuitBuilder::CreateLoopExitValue(GateRef loopExit, uint16_t reg, bool acc, GateRef value)
{
    GateRef newPhi = circuit_->NewGate(circuit_->LoopExitValue(), gateAcc_.GetMachineType(value),
                                       {loopExit, value}, gateAcc_.GetGateType(value));
    if (acc) {
        return loopExitToAccGate_[loopExit] = newPhi;
    } else {
        auto key = std::make_pair(loopExit, reg);
        return loopExitToVregGate_[key] = newPhi;
    }
}

GateRef BytecodeCircuitBuilder::NewLoopExitValue(GateRef loopExit, uint16_t reg, bool acc, GateRef value)
{
    ASSERT(gateAcc_.GetOpCode(loopExit) == OpCode::LOOP_EXIT);
    ChunkVector<GateRef> exitList(circuit_->chunk());
    while (gateAcc_.GetOpCode(loopExit) == OpCode::LOOP_EXIT) {
        exitList.push_back(loopExit);
        loopExit = gateAcc_.GetState(loopExit);
    }
    while (!exitList.empty()) {
        GateRef exit = exitList.back();
        value = CreateLoopExitValue(exit, reg, acc, value);
        exitList.pop_back();
    }
    return value;
}

GateRef BytecodeCircuitBuilder::ResolveDef(const BytecodeRegion &bb, int32_t bcId, const uint16_t reg, const bool acc)
{
    // Ensure that bcId is not negative
    if (bcId == 0) {
        return ResolveDef(bb.id, bcId, reg, acc, false);
    }
    return ResolveDef(bb.id, bcId - 1, reg, acc);
}

// recursive variables renaming algorithm
GateRef BytecodeCircuitBuilder::ResolveDef(const size_t bbId, int32_t bcId,
    const uint16_t reg, const bool acc, bool needIter)
{
    auto tmpReg = reg;
    // find def-site in bytecodes of basic block
    auto ans = Circuit::NullGate();
    auto &bb = graph_.at(bbId);
    GateType type = GateType::AnyType();
    auto tmpAcc = acc;

    if (needIter) {
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
                    ans = byteCodeToJSGates_.at(iterator.Index()).at(0);
                    auto oldType = gateAcc_.GetGateType(ans);
                    if (!type.IsAnyType() && oldType.IsAnyType()) {
                        typeRecorder_.GetOrUpdatePGOType(tsManager_, gateAcc_.TryGetPcOffset(ans), type);
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
            auto resumeGate = byteCodeToJSGates_.at(iterator.Index()).at(0);
            ans = GetExistingRestore(resumeGate, tmpReg);
            if (ans != Circuit::NullGate()) {
                break;
            }
            ans = circuit_->NewGate(circuit_->RestoreRegister(tmpReg), MachineType::I64,
                                    { resumeGate }, GateType::AnyType());
            SetExistingRestore(resumeGate, tmpReg, ans);
            auto saveRegGate = ResolveDef(bbId, iterator.Index() - 1, tmpReg, tmpAcc);
            [[maybe_unused]] EcmaOpcode opcode = Bytecodes::GetOpcode(iterator.PeekPrevPc(2)); // 2: prev bc
            ASSERT(opcode == EcmaOpcode::SUSPENDGENERATOR_V8 || opcode == EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8);
            GateRef suspendGate = byteCodeToJSGates_.at(iterator.Index() - 2).at(0); // 2: prev bc
            GateRef saveRegs = gateAcc_.GetDep(suspendGate);
            gateAcc_.ReplaceValueIn(saveRegs, saveRegGate, tmpReg);
            break;
        }
    }
    // find GET_EXCEPTION gate if this is a catch block
    if (ans == Circuit::NullGate() && tmpAcc) {
        if (!bb.trys.empty()) {
            GateRef getExceptionGate = bb.dependCurrent;
            ASSERT(gateAcc_.GetOpCode(getExceptionGate) == OpCode::GET_EXCEPTION);
            ASSERT(getExceptionGate != Circuit::NullGate());
            ans = getExceptionGate;
        }
    }
    // find def-site in value selectors of vregs
    if (ans == Circuit::NullGate() && !tmpAcc && bb.phi.count(tmpReg)) {
        if (!bb.vregToValueGate.count(tmpReg)) {
            NewPhi(bb, tmpReg, tmpAcc, bb.vregToValueGate[tmpReg]);
        }
        ans = bb.vregToValueGate.at(tmpReg);
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
            ans = gateAcc_.GetInitialEnvGate(argAcc_.GetCommonArgGate(CommonArgIdx::FUNC));
        } else if (argAcc_.ArgGateNotExisted(tmpReg)) {
            // when GetArgGate fail, return hole
            ans = circuit_->GetConstantGate(MachineType::I64,
                                            JSTaggedValue::VALUE_HOLE,
                                            GateType::TaggedValue());
        } else {
            ans = argAcc_.GetArgGate(tmpReg);
        }
        return ans;
    }
    if (EnableLoopOptimization()) {
        // find def-site in value selectors of vregs
        if (ans == Circuit::NullGate() && !tmpAcc) {
            if (!bb.vregToValueGate.count(tmpReg)) {
                bb.vregToValueGate[tmpReg] = Circuit::NullGate();
                NewPhi(bb, tmpReg, tmpAcc, bb.vregToValueGate[tmpReg]);
            } else if (bb.vregToValueGate.at(tmpReg) == Circuit::NullGate()) {
                NewPhi(bb, tmpReg, tmpAcc, bb.vregToValueGate[tmpReg]);
            }
            ans = bb.vregToValueGate.at(tmpReg);
        }
        // find def-site in value selectors of acc
        if (ans == Circuit::NullGate() && tmpAcc) {
            if (bb.valueSelectorAccGate == Circuit::NullGate()) {
                NewPhi(bb, tmpReg, tmpAcc, bb.valueSelectorAccGate);
            }
            ans = bb.valueSelectorAccGate;
        }
    }
    if (ans == Circuit::NullGate()) {
        // recursively find def-site in dominator block
        GateRef res = ResolveDef(bb.iDominator->id, bb.iDominator->end, tmpReg, tmpAcc);
        return res;
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

            auto type = typeRecorder_.GetType(bcIndex);
            gateAcc_.SetGateType(gate, type);
            auto pgoType = typeRecorder_.GetOrUpdatePGOType(tsManager_, gateAcc_.TryGetPcOffset(gate), type);
            gateAcc_.TrySetPGOType(gate, pgoType);

            auto valueCount = gateAcc_.GetInValueCount(gate);
            [[maybe_unused]] size_t numValueInputs = bytecodeInfo.ComputeValueInputCount();
            [[maybe_unused]] size_t numValueOutputs = bytecodeInfo.ComputeOutCount();
            // RETURNUNDEFINED has value input, but not from acc
            ASSERT(numValueInputs == valueCount || bytecodeInfo.GetOpcode() == EcmaOpcode::RETURNUNDEFINED);
            ASSERT(numValueOutputs <= 1 + (bytecodeInfo.EnvOut() ? 1 : 0));
            auto valueStarts = gateAcc_.GetInValueStarts(gate);
            for (size_t valueIdx = 0; valueIdx < valueCount; valueIdx++) {
                auto inIdx = valueIdx + valueStarts;
                if (!gateAcc_.IsInGateNull(gate, inIdx)) {
                    continue;
                }
                if (bytecodeInfo.GetOpcode() == EcmaOpcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8) {
                    GateRef depIn = gateAcc_.GetDep(gate);
                    size_t depCount = gateAcc_.GetNumValueIn(depIn);
                    GateRef defVreg = Circuit::NullGate();
                    for (size_t idx = 0; idx < depCount; idx++) {
                        defVreg = ResolveDef(bb, bcIndex, idx, false);
                        gateAcc_.ReplaceValueIn(depIn, defVreg, idx);
                    }
                }
                if (valueIdx < bytecodeInfo.inputs.size()) {
                    auto vregId = std::get<VirtualRegister>(bytecodeInfo.inputs.at(valueIdx)).GetId();
                    GateRef defVreg = Circuit::NullGate();
                    if (IsFirstBCEnvIn(bbIndex, bcIndex, vregId)) {
                        defVreg = gateAcc_.GetInitialEnvGate(argAcc_.GetCommonArgGate(CommonArgIdx::FUNC));
                    } else {
                        defVreg = ResolveDef(bb, bcIndex, vregId, false);
                    }
                    gateAcc_.NewIn(gate, inIdx, defVreg);
                } else {
                    GateRef defAcc = ResolveDef(bb, bcIndex, 0, true);
                    if (!Bytecodes::IsCallOp(bytecodeInfo.GetOpcode())) {
                        gateAcc_.NewIn(gate, inIdx, defAcc);
                        continue;
                    }
                    auto oldGt = gateAcc_.GetGateType(defAcc).GetGTRef();
                    GateType callTargetType = typeRecorder_.GetCallTargetType(bcIndex);
                    if (!tsManager_->MethodOffsetIsVaild(oldGt) && !callTargetType.IsAnyType()) {
                        gateAcc_.SetGateType(defAcc, callTargetType);
                    }
                    gateAcc_.NewIn(gate, inIdx, defAcc);
                }
            }
            return true;
        });
    }

    if (IsTypeLoweringEnabled()) {
        frameStateBuilder_.BuildFrameState();
    }

    gateAcc_.EliminateRedundantPhi();

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

void BytecodeCircuitBuilder::CollectLoopBack()
{
    auto size = GetBasicBlockCount();
    ChunkVector<size_t> workList(circuit_->chunk());
    ChunkVector<VisitState> visitState(circuit_->chunk());
    visitState.resize(size, VisitState::UNVISITED);
    size_t entryId = 0; // entry id
    workList.emplace_back(entryId);
    while (!workList.empty()) {
        size_t bbId = workList.back();
        auto &bb = GetBasicBlockById(bbId);
        if (visitState[bbId] == VisitState::UNVISITED) {
            dfsList_.emplace_back(bbId);
            visitState[bbId] = VisitState::PENDING;
        }
        bool allVisited = true;

        for (const auto &succBlock: bb.succs) {
            size_t succId = succBlock->id;
            if (visitState[succId] == VisitState::UNVISITED) {
                // dfs
                workList.emplace_back(succId);
                allVisited = false;
                break;
            } else if (visitState[succId] == VisitState::PENDING) {
                // back edge
                CountLoopBackEdge(bbId, succId);
            }
        }

        for (const auto &succBlock: bb.catchs) {
            size_t succId = succBlock->id;
            if (visitState[succId] == VisitState::UNVISITED) {
                // dfs
                workList.emplace_back(succId);
                allVisited = false;
                break;
            } else if (visitState[succId] == VisitState::PENDING) {
                // back edge
                CountLoopBackEdge(bbId, succId);
            }
        }
        if (allVisited) {
            workList.pop_back();
            visitState[bbId] = VisitState::VISITED;
        }
    }
}

void BytecodeCircuitBuilder::CountLoopBackEdge(size_t fromId, size_t toId)
{
    auto &toBlock = GetBasicBlockById(toId);
    if (toBlock.numOfLoopBacks == 0) {
        loopHeads_.emplace_back(std::make_pair(0, toId));
    }
    toBlock.loopbackBlocks.insert(fromId);
    toBlock.numOfLoopBacks = toBlock.loopbackBlocks.size();
}

void BytecodeCircuitBuilder::ComputeLoopDepth(size_t loopHead)
{
    ChunkSet<size_t> visited (circuit_->chunk());
    ChunkQueue<size_t> workList (circuit_->chunk());
    visited.insert(loopHead);
    auto &headBB = GetBasicBlockById(loopHead);
    headBB.loopDepth++;
    for (auto loopBack : headBB.loopbackBlocks) {
        workList.push(loopBack);
    }
    while (!workList.empty()) {
        size_t cur = workList.front();
        workList.pop();
        if (visited.count(cur) > 0) {
            continue;
        }
        visited.insert(cur);
        auto &curBB = GetBasicBlockById(cur);
        curBB.loopDepth++;
        for (const auto& pred : curBB.preds) {
            workList.push(pred->id);
        }
        for (const auto& pred : curBB.trys) {
            workList.push(pred->id);
        }
    }
    loopSize_ = visited.size();
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
        if (IsEntryBlock(bb.id)) {
            LOG_COMPILER(INFO) << "\tBytecodePC: Empty";
        } else {
            LOG_COMPILER(INFO) << "\tBytecodePC: [" << std::to_string(bb.start) << ", "
                << std::to_string(bb.end) << ")";
        }

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

        std::string log6("\tLoop Depth: ");
        log6 += std::to_string(bb.loopDepth);
        LOG_COMPILER(INFO) << log6;

        PrintBytecodeInfo(bb);
        LOG_COMPILER(INFO) << "";
    }
}

void BytecodeCircuitBuilder::PrintBytecodeInfo(BytecodeRegion& bb)
{
    if (bb.isDead) {
        return;
    }
    if (IsEntryBlock(bb.id)) {
        LOG_COMPILER(INFO) << "\tBytecode[] = Empty";
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
            log += std::to_string(out) + ",";
        }
        log += "] >";
        LOG_COMPILER(INFO) << log;

        auto gate = GetGateByBcIndex(iterator.Index());
        if (gate != Circuit::NullGate()) {
            this->gateAcc_.ShortPrint(gate);
        }
        return true;
    });
}
}  // namespace panda::ecmascript::kungfu
