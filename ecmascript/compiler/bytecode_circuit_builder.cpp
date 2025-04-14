/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/interpreter/interpreter-inl.h"


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
    ASSERT(size > 0);
    uint32_t end = size - 1;  // 1: end
    BytecodeIterator iterator(this, 0, end);

    infoData_.resize(size);
    byteCodeToJSGates_.resize(size, std::vector<GateRef>(0));
    regionsInfo_.InsertHead(0); // 0: start pc
    iterator.GotoStart();
    while (!iterator.Done()) {
        auto index = iterator.Index();
        auto &info = infoData_[index];
        auto pc = pcOffsets_[index];
        info.metaData_ = bytecodes_->GetBytecodeMetaData(pc);
        ASSERT(!info.metaData_.IsInvalid());
        BytecodeInfo::InitBytecodeInfo(this, info, pc);
        CollectRegionInfo(index);
        ++iterator;
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
    } else if (info.IsInsufficientProfile()) {
        if (bcIndex != GetLastBcIndex()) {
            auto nextIndex = bcIndex + 1; // 1: next pc
            regionsInfo_.InsertSplit(nextIndex);
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
            byteCodeException.back().catches.emplace_back(catchBlockPc);
            return true;
        });
        return true;
    });
}

bool BytecodeCircuitBuilder::IsAncestor(size_t nodeA, size_t nodeB)
{
    return timeIn_[bbIdToDfsTimestamp_[nodeA]] <= timeIn_[bbIdToDfsTimestamp_[nodeB]] &&
        timeOut_[bbIdToDfsTimestamp_[nodeA]] >= timeOut_[bbIdToDfsTimestamp_[nodeB]];
}

void BytecodeCircuitBuilder::PerformDFS(const std::vector<size_t> &immDom, size_t listSize)
{
    std::vector<std::vector<size_t>> sonList(listSize);
    for (size_t idx = 1; idx < immDom.size(); idx++) {
        sonList[immDom[idx]].push_back(idx);
    }

    {
        size_t timestamp = 0;
        struct DFSState {
            size_t cur;
            std::vector<size_t> &succList;
            size_t idx;
        };
        std::stack<DFSState> dfsStack;
        size_t root = 0;
        dfsStack.push({root, sonList[root], 0});
        timeIn_[root] = timestamp++;
        while (!dfsStack.empty()) {
            auto &curState = dfsStack.top();
            auto &cur = curState.cur;
            auto &succList = curState.succList;
            auto &idx = curState.idx;
            if (idx == succList.size()) {
                timeOut_[cur] = timestamp++;
                dfsStack.pop();
                continue;
            }
            const auto &succ = succList[idx];
            dfsStack.push({succ, sonList[succ], 0});
            timeIn_[succ] = timestamp++;
            idx++;
        }
    }
}

void BytecodeCircuitBuilder::ReducibilityCheck()
{
    std::vector<size_t> basicBlockList;
    std::vector<size_t> immDom;
    std::unordered_map<size_t, size_t> bbDfsTimestampToIdx;
    ComputeDominatorTree(basicBlockList, immDom, bbDfsTimestampToIdx);
    timeIn_.resize(basicBlockList.size());
    timeOut_.resize(basicBlockList.size());
    PerformDFS(immDom, basicBlockList.size());
}

void BytecodeCircuitBuilder::ComputeImmediateDominators(const std::vector<size_t> &basicBlockList,
                                                        std::unordered_map<size_t, size_t> &dfsFatherIdx,
                                                        std::vector<size_t> &immDom,
                                                        std::unordered_map<size_t, size_t> &bbDfsTimestampToIdx)
{
    std::vector<size_t> semiDom(basicBlockList.size());
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
        auto calculateSemiDom = [&](size_t idx, const ChunkVector<BytecodeRegion *> &blocks) {
            for (const auto &preBlock : blocks) {
                if (bbDfsTimestampToIdx[preBlock->id] < idx) {
                    semiDom[idx] = std::min(semiDom[idx], bbDfsTimestampToIdx[preBlock->id]);
                } else {
                    unionFind(bbDfsTimestampToIdx[preBlock->id]);
                    semiDom[idx] = std::min(semiDom[idx], semiDom[minIdx[bbDfsTimestampToIdx[preBlock->id]]]);
                }
            }
        };
        std::iota(semiDom.begin(), semiDom.end(), 0);
        semiDom[0] = semiDom.size();
        for (size_t idx = basicBlockList.size() - 1; idx >= 1; idx--) {
            calculateSemiDom(idx, graph_[basicBlockList[idx]]->preds);
            if (!graph_[basicBlockList[idx]]->trys.empty()) {
                calculateSemiDom(idx, graph_[basicBlockList[idx]]->trys);
            }
            for (const auto &succDomIdx : semiDomTree[idx]) {
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
        }
        semiDom[0] = 0;
    }
}

void BytecodeCircuitBuilder::ComputeDominatorTree(std::vector<size_t> &basicBlockList, std::vector<size_t> &immDom,
    std::unordered_map<size_t, size_t> &bbDfsTimestampToIdx)
{
    std::unordered_map<size_t, size_t> dfsFatherIdx;
    size_t timestamp = 0;
    std::deque<size_t> pendingList;
    std::vector<size_t> visited(graph_.size(), 0);
    auto basicBlockId = graph_[0]->id;
    visited[graph_[0]->id] = 1;
    pendingList.emplace_back(basicBlockId);

    auto visitConnectedBlocks = [&](const ChunkVector<BytecodeRegion *> &succs, size_t curBlockId) {
        for (const auto &succBlock : succs) {
            if (visited[succBlock->id] == 0) {
                visited[succBlock->id] = 1;
                pendingList.emplace_back(succBlock->id);
                dfsFatherIdx[succBlock->id] = bbIdToDfsTimestamp_[curBlockId];
            }
        }
    };

    while (!pendingList.empty()) {
        size_t curBlockId = pendingList.back();
        pendingList.pop_back();
        basicBlockList.emplace_back(curBlockId);
        bbIdToDfsTimestamp_[curBlockId] = timestamp++;
        visitConnectedBlocks(graph_[curBlockId]->succs, curBlockId);
        if (!graph_[curBlockId]->catches.empty()) {
            visitConnectedBlocks(graph_[curBlockId]->catches, curBlockId);
        }
    }
    for (size_t idx = 0; idx < basicBlockList.size(); idx++) {
        bbDfsTimestampToIdx[basicBlockList[idx]] = idx;
    }

    immDom.resize(basicBlockList.size());
    ComputeImmediateDominators(basicBlockList, dfsFatherIdx, immDom, bbDfsTimestampToIdx);
}

void BytecodeCircuitBuilder::BuildEntryBlock()
{
    BytecodeRegion &entryBlock = RegionAt(0);
    BytecodeRegion &nextBlock = RegionAt(1);
    entryBlock.succs.emplace_back(&nextBlock);
    nextBlock.preds.emplace_back(&entryBlock);
    entryBlock.bytecodeIterator_.Reset(this, 0, BytecodeIterator::INVALID_INDEX);
}

void BytecodeCircuitBuilder::BuildBasicBlock()
{
    auto &items = regionsInfo_.GetBlockItems();
    size_t blockId = 1;
    for (const auto &item : items) {
        auto &curBlock = GetBasicBlockById(blockId);
        curBlock.id = blockId;
        curBlock.start = item.GetStartBcIndex();
        if (blockId != 1) {
            auto &prevBlock = RegionAt(blockId - 1);
            ASSERT(curBlock.start >= 1);
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
    auto &lastBlock = RegionAt(blockId - 1); // 1: last block
    lastBlock.end = GetLastBcIndex();
    lastBlock.bytecodeIterator_.Reset(this, lastBlock.start, lastBlock.end);
}

void BytecodeCircuitBuilder::BuildRegions(const ExceptionInfo &byteCodeException)
{
    auto blockSize = regionsInfo_.GetBlockItems().size();

    // 1 : entry block. if the loop head is in the first bb block, the variables used in the head cannot correctly
    // generate Phi nodes through the dominator-tree algorithm, resulting in an infinite loop. Therefore, an empty
    // BB block is generated as an entry block
    graph_.resize(blockSize + 1, nullptr);
    for (size_t i = 0; i < graph_.size(); i++) {
        graph_[i] = circuit_->chunk()->New<BytecodeRegion>(circuit_->chunk());
    }

    // build entry block
    BuildEntryBlock();

    // build basic block
    BuildBasicBlock();

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
    UpdateCFG();
    if (HasTryCatch()) {
        CollectTryPredsInfo();
    }
    RemoveUnreachableRegion();
    if (IsLogEnabled() && !IsPreAnalysis()) {
        PrintGraph(std::string("Update CFG [" + methodName_ + "]").c_str());
    }
    frameStateBuilder_.AnalyzeLiveness();
    RemoveInsufficientProfileRegion();
    if (NeedIrreducibleLoopCheck()) {
        ReducibilityCheck();
    }
    if (IsLogEnabled() && !IsPreAnalysis()) {
        PrintGraph(std::string("Update CFG With Profile [" + methodName_ + "]").c_str());
    }
    BuildCircuit();
}

void BytecodeCircuitBuilder::BuildCatchBlocks(const ExceptionInfo &byteCodeException)
{
    // try catch block associate
    for (size_t i = 0; i < graph_.size(); i++) {
        auto &bb = RegionAt(i);
        auto startIndex = bb.start;
        bool noThrow = true;
        EnumerateBlock(bb, [&noThrow](const BytecodeInfo &bytecodeInfo) -> bool {
            if (bytecodeInfo.IsGeneral()) {
                if (!bytecodeInfo.NoThrow()) {
                    noThrow = false;
                    return false;
                }
            }
            return true;
        });
        if (noThrow) {
            continue;
        }
        const auto pc = pcOffsets_[startIndex];
        for (auto it = byteCodeException.cbegin(); it != byteCodeException.cend(); it++) {
            if (pc < it->startPc || pc >= it->endPc) {
                continue;
            }
            // try block interval
            const auto &catches = it->catches; // catches start pc
            for (size_t j = i + 1; j < graph_.size(); j++) {
                auto &catchBB = RegionAt(j);
                const auto catchStart = pcOffsets_[catchBB.start];
                if (std::find(catches.cbegin(), catches.cend(), catchStart) != catches.cend()) {
                    bb.catches.insert(bb.catches.cbegin(), &catchBB);
                }
            }
        }

        // When there are multiple catch blocks in the current block, the set of catch blocks
        // needs to be sorted to satisfy the order of execution of catch blocks.
        bb.SortCatches();
    }
}

void BytecodeCircuitBuilder::CollectTryPredsInfo()
{
    for (size_t i = 0; i < graph_.size(); i++) {
        auto &bb = RegionAt(i);
        if (bb.catches.empty()) {
            continue;
        } else if (bb.catches.size() > 1) { // 1: cache size
            for (auto it = bb.catches.begin() + 1; it != bb.catches.end();) { // 1: invalid catch bb
                bb.EraseThisBlock((*it)->trys);
                it = bb.catches.erase(it);
            }
        }

        EnumerateBlock(bb, [&bb](const BytecodeInfo &bytecodeInfo) -> bool {
            if (bytecodeInfo.IsGeneral()) {
                // if block which can throw exception has serval catchs block,
                // only the innermost catch block is useful
                ASSERT(bb.catches.size() == 1); // 1: cache size
                if (!bytecodeInfo.NoThrow()) {
                    bb.catches.at(0)->numOfStatePreds++;
                }
            }
            return true;
        });
    }
}

void BytecodeCircuitBuilder::RemoveUnusedPredsInfo(BytecodeRegion& bb, bool skipInsufficientProfile)
{
    EnumerateBlock(bb, [&bb, &skipInsufficientProfile](const BytecodeInfo &bytecodeInfo) -> bool {
        if (skipInsufficientProfile && bytecodeInfo.IsInsufficientProfile()) {
            return true;
        }
        if (bytecodeInfo.IsGeneral()) {
            ASSERT(bb.catches.size() == 1); // 1: cache size
            if (!bytecodeInfo.NoThrow()) {
                bb.catches.at(0)->numOfStatePreds--;
            }
        }
        return true;
    });
}

void BytecodeCircuitBuilder::ClearUnreachableRegion(ChunkVector<BytecodeRegion*>& pendingList,
                                                    bool skipInsufficientProfile)
{
    auto bb = pendingList.back();
    pendingList.pop_back();
    for (auto it = bb->preds.begin(); it != bb->preds.end(); it++) {
        ASSERT((*it)->numOfStatePreds >= 0);
        if ((*it)->numOfStatePreds != 0) {
            bb->EraseThisBlock((*it)->succs);
        }
    }
    for (auto it = bb->succs.begin(); it != bb->succs.end(); it++) {
        auto bbNext = *it;
        ASSERT(bbNext->numOfStatePreds >= 0);
        if (bbNext->numOfStatePreds != 0) {
            bb->EraseThisBlock(bbNext->preds);
            bbNext->numOfStatePreds--;
            if (bbNext->numOfStatePreds == 0) {
                pendingList.emplace_back(bbNext);
            }
        }
    }
    for (auto it = bb->trys.begin(); it != bb->trys.end(); it++) {
        ASSERT((*it)->numOfStatePreds >= 0);
        if ((*it)->numOfStatePreds != 0) {
            bb->EraseThisBlock((*it)->catches);
        }
    }
    for (auto it = bb->catches.begin(); it != bb->catches.end(); it++) {
        auto bbNext = *it;
        ASSERT(bbNext->numOfStatePreds >= 0);
        if (bbNext->numOfStatePreds != 0) {
            RemoveUnusedPredsInfo(*bb, skipInsufficientProfile);
            bb->EraseThisBlock(bbNext->trys);
            if (bbNext->numOfStatePreds == 0) {
                pendingList.emplace_back(bbNext);
            }
        }
    }
    bb->preds.clear();
    bb->succs.clear();
    bb->trys.clear();
    bb->catches.clear();
    numOfLiveBB_--;

    RemoveIfInRpoList(bb);
}

void BytecodeCircuitBuilder::RemoveIfInRpoList(BytecodeRegion *bb)
{
    auto& rpoList = frameStateBuilder_.GetRpoList();
    for (auto iter = rpoList.begin(); iter != rpoList.end(); iter++) {
        if (*iter == bb->id) {
            rpoList.erase(iter);
            break;
        }
    }
}

void BytecodeCircuitBuilder::RemoveUnreachableRegion()
{
    numOfLiveBB_ = graph_.size();
    ChunkVector<BytecodeRegion*> pendingList(circuit_->chunk());
    for (size_t i = 1; i < graph_.size(); i++) { // 1: skip entry bb
        auto &bb = RegionAt(i);
        ASSERT(bb.numOfStatePreds >= 0);
        if (bb.numOfStatePreds == 0) {
            pendingList.emplace_back(&bb);
        }
    }
    while (!pendingList.empty()) {
        ClearUnreachableRegion(pendingList);
    }
}

void BytecodeCircuitBuilder::RemoveInsufficientProfileRegion()
{
    ChunkVector<BytecodeRegion*> pendingList(circuit_->chunk());
    for (size_t i = 1; i < graph_.size(); i++) {
        auto &curBlock = RegionAt(i);
        const BytecodeInfo& lastBytecodeInfo = GetBytecodeInfo(curBlock.end);
        // We believe that bytecode with insufficient profile has never been executed.
        // If the last bytecode in the block has not been executed by the assembly interpreter,
        // the benefits of compiling its subsequent bytecode are relatively small, for the following reasons:
        //     1. Even if these bytecode are compiled, they are unlikely to be executed.
        //     2. These bytecode are missing profile and cannot obtain high-quality machine code.
        if (!lastBytecodeInfo.IsInsufficientProfile()) {
            continue;
        }
        // 1. Disconnect from the successor.
        // The last bytecode can only be one of JMP, THOW, or Unexecuted(InsufficientProfile), where JMP and THOW must
        // have been executed. The Block split from Unexecuted(InsufficientProfile) has only one successor or none.
        ASSERT(curBlock.succs.size() <= 1);
        for (auto it = curBlock.succs.begin(); it != curBlock.succs.end(); it++) {
            auto nextBlock = *it;
            ASSERT(nextBlock->numOfStatePreds >= 0);
            if (nextBlock->numOfStatePreds == 0) {
                continue;
            }
            curBlock.EraseThisBlock(nextBlock->preds);
            nextBlock->numOfStatePreds--;
            if (nextBlock->numOfStatePreds == 0) {
                pendingList.emplace_back(nextBlock);
            }
        }
        curBlock.succs.clear();
        // 2. Disconnect from the catch.
        // If the bytecode has not been executed(InsufficientProfile), a deopt is generated.
        // The assembly interpreter continues to handle the exception.
        if (curBlock.catches.size() == 0 || lastBytecodeInfo.NoThrow()) {
            continue;
        }
        ASSERT(curBlock.catches.size() == 1);
        curBlock.catches.at(0)->numOfStatePreds--;
        // Before lastBytecodeInfo, there may still be bytecode that throws an exception,
        // which will determine whether the catchBlock can be removed from the catchs list of curBlock.
        size_t numOfThrowBytecode = 0;
        EnumerateBlock(curBlock, [&numOfThrowBytecode](const BytecodeInfo &bytecodeInfo) -> bool {
            if (bytecodeInfo.IsGeneral() && !bytecodeInfo.NoThrow()) {
                numOfThrowBytecode++;
            }
            return true;
        });
        // no other throw
        if (numOfThrowBytecode == 1) {
            curBlock.EraseThisBlock(curBlock.catches.at(0)->trys);
            if (curBlock.catches.at(0)->numOfStatePreds == 0) {
                pendingList.emplace_back(curBlock.catches.at(0));
            }
            curBlock.catches.clear();
        }
    }
    while (!pendingList.empty()) {
        ClearUnreachableRegion(pendingList, true);
    }
    RemoveIsolatedRegion();
}

void MarkConnection(const BytecodeRegion& curBlock, std::vector<bool>& connectedToRoot)
{
    ASSERT(curBlock.id < connectedToRoot.size());
    if (connectedToRoot[curBlock.id]) {
        return;
    }
    connectedToRoot[curBlock.id] = true;
    for (auto it = curBlock.succs.begin(); it != curBlock.succs.end(); it++) {
        BytecodeRegion* nextBlock = *it;
        MarkConnection(*nextBlock, connectedToRoot);
    }
    if (curBlock.catches.size() == 0) {
        return;
    }
    ASSERT(curBlock.catches.size() == 1);
    MarkConnection(*(curBlock.catches.at(0)), connectedToRoot);
}

void BytecodeCircuitBuilder::RemoveIsolatedRegion()
{
    std::vector<bool> connectedToRoot(graph_.size(), false);
    MarkConnection(RegionAt(0), connectedToRoot);
    ChunkVector<BytecodeRegion*> pendingList(circuit_->chunk());
    for (size_t index = 0; index < connectedToRoot.size(); ++index) {
        auto& curBlock = RegionAt(index);
        if (!connectedToRoot[index] && curBlock.numOfStatePreds != 0) {
            curBlock.numOfStatePreds = 0;
            pendingList.emplace_back(&curBlock);
        }
    }
    while (!pendingList.empty()) {
        ClearUnreachableRegion(pendingList, true);
    }
}

void BytecodeCircuitBuilder::ComputeNumOfLoopBack()
{
    for (size_t i = 0; i < graph_.size(); i++) {
        auto &bb = RegionAt(i);
        if (!IsEntryBlock(bb.id) && bb.numOfStatePreds == 0) {
            continue;
        }
        for (auto &succ: bb.succs) {
            if (succ->IsLoopBack(bb)) {
                succ->numOfLoopBack++;
            }
        }
        if (bb.catches.empty()) {
            continue;
        }
        
        EnumerateBlock(bb, [&bb](const BytecodeInfo &bytecodeInfo) -> bool {
            if (bytecodeInfo.IsGeneral() && !bytecodeInfo.NoThrow() && bb.catches.at(0)->IsLoopBack(bb)) {
                // if block which can throw exception has serval catchs block,
                // only the innermost catch block is useful
                ASSERT(bb.catches.size() == 1); // 1: cache size
                bb.catches.at(0)->numOfLoopBack++;
            }
            return true;
        });
    }
}
// Update CFG's predecessor, successor and try catch associations
void BytecodeCircuitBuilder::UpdateCFG()
{
    for (size_t i = 0; i < graph_.size(); i++) {
        auto &bb = RegionAt(i);
        bb.preds.clear();
        bb.trys.clear();
    }
    for (size_t i = 0; i < graph_.size(); i++) {
        auto &bb = RegionAt(i);
        for (auto &succ: bb.succs) {
            succ->preds.emplace_back(&bb);
            succ->numOfStatePreds++;
        }
        for (auto &catchBlock: bb.catches) {
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
        argAcc_.NewCommonArg(CommonArgIdx::ACTUAL_ARGV, MachineType::ARCH, GateType::NJSValue());
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
    BuildFrameArgs();
}

void BytecodeCircuitBuilder::BuildFrameArgs()
{
    UInt32PairAccessor accessor(0, 0);
    auto metaData = circuit_->FrameArgs(accessor.ToValue());
    size_t numArgs = metaData->GetNumIns();
    std::vector<GateRef> args(numArgs, Circuit::NullGate());
    size_t idx = 0;
    args[idx++] = argAcc_.GetCommonArgGate(CommonArgIdx::FUNC);
    args[idx++] = argAcc_.GetCommonArgGate(CommonArgIdx::NEW_TARGET);
    args[idx++] = argAcc_.GetCommonArgGate(CommonArgIdx::THIS_OBJECT);
    args[idx++] = argAcc_.GetCommonArgGate(CommonArgIdx::ACTUAL_ARGC);
    args[idx++] = argAcc_.GetCommonArgGate(CommonArgIdx::ACTUAL_ARGV);
    GateRef sharedConstpool = Circuit::NullGate();
    GateRef unSharedConstpool = Circuit::NullGate();
    GetCurrentConstpool(argAcc_.GetCommonArgGate(CommonArgIdx::FUNC), sharedConstpool, unSharedConstpool);
    args[idx++] = sharedConstpool;
    args[idx++] = unSharedConstpool;
    args[idx++] = GetPreFrameArgs();
    GateRef frameArgs = circuit_->NewGate(metaData, args);
    argAcc_.SetFrameArgs(frameArgs);
}

void BytecodeCircuitBuilder::BuildOSRArgs()
{
    // offset -1 : glue
    (void)circuit_->NewGate(circuit_->GetMetaBuilder()->InitVreg(INIT_VRGE_GLUE), MachineType::I64,
                            {circuit_->GetArgRoot()}, GateType::NJSValue());
    // offset -2 : argc
    GateRef argc = method_->IsFastCall()
                       ? circuit_->GetConstantGate(MachineType::I64, 0, GateType::NJSValue())
                       : circuit_->NewGate(circuit_->GetMetaBuilder()->InitVreg(INIT_VRGE_ARGS), MachineType::I64,
                                           {circuit_->GetArgRoot()}, GateType::TaggedValue());
    // offset -3 : argv
    GateRef argv = method_->IsFastCall()
                       ? circuit_->GetConstantGate(MachineType::ARCH, 0, GateType::NJSValue())
                       : circuit_->NewGate(circuit_->GetMetaBuilder()->InitVreg(INIT_VRGE_ARGV), MachineType::I64,
                                           {circuit_->GetArgRoot()}, GateType::TaggedValue());
    // offset -4 : func
    (void)circuit_->NewGate(circuit_->GetMetaBuilder()->InitVreg(INIT_VRGE_FUNCTION), MachineType::I64,
                            {circuit_->GetArgRoot()}, GateType::TaggedValue());
    // offset -5 : new_target
    GateRef newTarget =
        method_->IsFastCall()
            ? circuit_->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_UNDEFINED, GateType::UndefinedType())
            : circuit_->NewGate(circuit_->GetMetaBuilder()->InitVreg(INIT_VRGE_NEW_TARGET), MachineType::I64,
                                {circuit_->GetArgRoot()}, GateType::TaggedValue());
    // offset -6 : this_object
    (void)circuit_->NewGate(circuit_->GetMetaBuilder()->InitVreg(INIT_VRGE_THIS_OBJECT), MachineType::I64,
                            {circuit_->GetArgRoot()}, GateType::TaggedValue());
    // offset -7 : numargs
    (void)circuit_->NewGate(circuit_->GetMetaBuilder()->InitVreg(INIT_VRGE_NUM_ARGS), MachineType::I64,
                            {circuit_->GetArgRoot()}, GateType::TaggedValue());
    for (size_t argIdx = 1; argIdx <= method_->GetNumArgsWithCallField(); argIdx++) {
        // common args
        argAcc_.NewArg(method_->IsFastCall() ? static_cast<size_t>(FastCallArgIdx::NUM_OF_ARGS)
                                             : static_cast<size_t>(CommonArgIdx::NUM_OF_ARGS) + argIdx);
    }

    auto &args = argAcc_.args_;
    if (args.size() == 0) {
        GateAccessor(circuit_).GetArgsOuts(args);
        std::reverse(args.begin(), args.end());
        if (method_->IsFastCall() && args.size() >= static_cast<uint8_t>(FastCallArgIdx::NUM_OF_ARGS)) {
            args.insert(args.begin() + static_cast<uint8_t>(CommonArgIdx::ACTUAL_ARGC), argc);
            args.insert(args.begin() + static_cast<uint8_t>(CommonArgIdx::ACTUAL_ARGV), argv);
            // 3: newtarget index
            args.insert(args.begin() + static_cast<uint8_t>(CommonArgIdx::NEW_TARGET), newTarget);
        }
    }

    BuildFrameArgs();
}

GateRef BytecodeCircuitBuilder::NewDeopt(BytecodeRegion &bb)
{
    ASSERT(bb.succs.empty());
    auto &iterator = bb.GetBytecodeIterator();
    GateRef state = frameStateBuilder_.GetCurrentState();
    GateRef depend = frameStateBuilder_.GetCurrentDepend();
    std::string comment = Deoptimizier::DisplayItems(DeoptType::INSUFFICIENTPROFILE);
    GateRef type = circuit_->GetConstantGate(MachineType::I64, static_cast<int64_t>(DeoptType::INSUFFICIENTPROFILE),
                                             GateType::NJSValue());
    GateRef condition = circuit_->GetConstantGate(MachineType::I1, 0, GateType::NJSValue());
    GateRef deopt = circuit_->NewGate(circuit_->DeoptCheck(), MachineType::I1,
                                      {state, depend, condition, gateAcc_.FindNearestFrameState(depend), type},
                                      GateType::NJSValue(), comment.c_str());
    GateRef dependRelay = circuit_->NewGate(circuit_->DependRelay(), {deopt, depend});
    GateRef undef =
        circuit_->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_UNDEFINED, GateType::TaggedValue());
    circuit_->NewGate(circuit_->Return(), {deopt, dependRelay, undef, circuit_->GetReturnRoot()});
    byteCodeToJSGates_[iterator.Index()].emplace_back(deopt);
    jsGatesToByteCode_[deopt] = iterator.Index();
    return deopt;
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

void BytecodeCircuitBuilder::MergeThrowGate(BytecodeRegion &bb, uint32_t bcIndex)
{
    auto state = frameStateBuilder_.GetCurrentState();
    auto depend = frameStateBuilder_.GetCurrentDepend();
    if (!bb.catches.empty()) {
        auto ifSuccess = circuit_->NewGate(circuit_->IfSuccess(), {state});
        auto dependRelay = circuit_->NewGate(circuit_->DependRelay(), {ifSuccess, depend});
        auto ifException = circuit_->NewGate(circuit_->IfException(), {state, depend});
        frameStateBuilder_.UpdateStateDepend(ifException, ifException);
        ASSERT(bb.catches.size() == 1); // 1: one catch
        auto bbNext = bb.catches.at(0);
        frameStateBuilder_.MergeIntoSuccessor(bb, *bbNext);
        bbNext->expandedPreds.push_back({bb.id, bcIndex, true});
        state = ifSuccess;
        depend = dependRelay;
    }
    auto constant = circuit_->GetConstantGate(MachineType::I64,
                                              JSTaggedValue::VALUE_EXCEPTION,
                                              GateType::TaggedValue());
    circuit_->NewGate(circuit_->Return(),
        { state, depend, constant, circuit_->GetReturnRoot() });
}

void BytecodeCircuitBuilder::MergeExceptionGete(BytecodeRegion &bb,
    const BytecodeInfo& bytecodeInfo, uint32_t bcIndex)
{
    auto state = frameStateBuilder_.GetCurrentState();
    auto depend = frameStateBuilder_.GetCurrentDepend();
    auto ifSuccess = circuit_->NewGate(circuit_->IfSuccess(), {state});
    auto dependRelay = circuit_->NewGate(circuit_->DependRelay(), {ifSuccess, depend});
    ASSERT(bb.catches.size() == 1); // 1: one catch
    auto bbNext = bb.catches.at(0);
    auto ifException = circuit_->NewGate(circuit_->IfException(), {state, depend});
    frameStateBuilder_.UpdateStateDepend(ifException, ifException);
    frameStateBuilder_.MergeIntoSuccessor(bb, *bbNext);
    if (bytecodeInfo.GetOpcode() == EcmaOpcode::CREATEASYNCGENERATOROBJ_V8) {
        bbNext->expandedPreds.push_back({bb.id, bcIndex + 1, true}); // 1: next pc
    } else {
        bbNext->expandedPreds.push_back({bb.id, bcIndex, true});
    }
    depend = dependRelay;
    frameStateBuilder_.UpdateStateDepend(ifSuccess, depend);
}

void BytecodeCircuitBuilder::NewJSGate(BytecodeRegion &bb)
{
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    GateRef state = frameStateBuilder_.GetCurrentState();
    GateRef depend = frameStateBuilder_.GetCurrentDepend();
    size_t numValueInputs = bytecodeInfo.ComputeValueInputCount();
    GateRef gate = 0;
    bool writable = !bytecodeInfo.NoSideEffects();
    bool hasFrameState = bytecodeInfo.HasFrameState();
    size_t pcOffset = GetPcOffset(iterator.Index());
    auto methodOffset = method_->GetMethodId().GetOffset();
    auto meta = circuit_->JSBytecode(
        numValueInputs, methodOffset, bytecodeInfo.GetOpcode(), pcOffset, iterator.Index(), writable, hasFrameState);
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
    frameStateBuilder_.UpdateStateDepend(gate, gate);
    frameStateBuilder_.UpdateFrameValues(bytecodeInfo, iterator.Index(), gate);
    if (bytecodeInfo.IsThrow()) {
        MergeThrowGate(bb, iterator.Index());
        return;
    }

    if (!bb.catches.empty() && !bytecodeInfo.NoThrow()) {
        MergeExceptionGete(bb, bytecodeInfo, iterator.Index());
    } else if (!bb.catches.empty()) {
        frameStateBuilder_.GetOrOCreateMergedContext(bb.catches.at(0)->id);
    }
    if (bytecodeInfo.IsGeneratorRelative()) {
        suspendAndResumeGates_.emplace_back(gate);
    }
}

void BytecodeCircuitBuilder::NewJump(BytecodeRegion &bb)
{
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    GateRef state = frameStateBuilder_.GetCurrentState();
    GateRef depend = frameStateBuilder_.GetCurrentDepend();
    size_t numValueInputs = bytecodeInfo.ComputeValueInputCount();
    if (bytecodeInfo.IsCondJump() && bb.succs.size() == 2) { // 2: two succ
        size_t pcOffset = GetPcOffset(iterator.Index());
        auto methodOffset = method_->GetMethodId().GetOffset();
        auto meta = circuit_->JSBytecode(
            numValueInputs, methodOffset, bytecodeInfo.GetOpcode(), pcOffset, iterator.Index(), false, false);
        auto numValues = meta->GetNumIns();
        GateRef gate = circuit_->NewGate(meta, std::vector<GateRef>(numValues, Circuit::NullGate()));
        gateAcc_.NewIn(gate, 0, state);
        gateAcc_.NewIn(gate, 1, depend);
        frameStateBuilder_.UpdateStateDepend(gate, gate);
        frameStateBuilder_.UpdateFrameValues(bytecodeInfo, iterator.Index(), gate);

        auto ifTrue = circuit_->NewGate(circuit_->IfTrue(), {gate});
        auto trueRelay = circuit_->NewGate(circuit_->DependRelay(), {ifTrue, gate});
        auto ifFalse = circuit_->NewGate(circuit_->IfFalse(), {gate});
        auto falseRelay = circuit_->NewGate(circuit_->DependRelay(), {ifFalse, gate});
        for (auto &bbNext: bb.succs) {
            if (iterator.Index() + 1 == bbNext->start) {
                frameStateBuilder_.UpdateStateDepend(ifFalse, falseRelay);
                frameStateBuilder_.MergeIntoSuccessor(bb, *bbNext);
                bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
            } else {
                frameStateBuilder_.UpdateStateDepend(ifTrue, trueRelay);
                frameStateBuilder_.MergeIntoSuccessor(bb, *bbNext);
                bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
            }
        }
        byteCodeToJSGates_[iterator.Index()].emplace_back(gate);
        jsGatesToByteCode_[gate] = iterator.Index();
    } else {
        ASSERT(bb.succs.size() == 1); // 1: only one succ if not condjump
        auto &bbNext = bb.succs.at(0);
        frameStateBuilder_.MergeIntoSuccessor(bb, *bbNext);
        bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
    }

    if (!bb.catches.empty()) {
        frameStateBuilder_.GetOrOCreateMergedContext(bb.catches.at(0)->id);
    }
}

GateRef BytecodeCircuitBuilder::NewReturn(BytecodeRegion &bb)
{
    ASSERT(bb.succs.empty());
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    GateRef state = frameStateBuilder_.GetCurrentState();
    GateRef depend = frameStateBuilder_.GetCurrentDepend();
    GateRef gate = Circuit::NullGate();
    if (bytecodeInfo.GetOpcode() == EcmaOpcode::RETURN) {
        // handle return.dyn bytecode
        gate = circuit_->NewGate(circuit_->Return(),
            { state, depend, Circuit::NullGate(), circuit_->GetReturnRoot() });
        byteCodeToJSGates_[iterator.Index()].emplace_back(gate);
        jsGatesToByteCode_[gate] = iterator.Index();
    } else if (bytecodeInfo.GetOpcode() == EcmaOpcode::RETURNUNDEFINED) {
        // handle returnundefined bytecode
        auto constant = circuit_->GetConstantGate(MachineType::I64,
                                                  JSTaggedValue::VALUE_UNDEFINED,
                                                  GateType::TaggedValue());
        gate = circuit_->NewGate(circuit_->Return(),
            { state, depend, constant, circuit_->GetReturnRoot() });
        byteCodeToJSGates_[iterator.Index()].emplace_back(gate);
        jsGatesToByteCode_[gate] = iterator.Index();
    }
    return gate;
}

void BytecodeCircuitBuilder::NewByteCode(BytecodeRegion &bb)
{
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    FrameLiveOut* liveout;
    auto bcId = iterator.Index();
    if (bcId != 0 && iterator.IsInRange(bcId - 1)) {
        liveout = frameStateBuilder_.GetOrOCreateBCEndLiveOut(bcId - 1);
    } else {
        liveout = frameStateBuilder_.GetOrOCreateBBLiveOut(bb.id);
    }
    frameStateBuilder_.AdvanceToNextBc(bytecodeInfo, liveout, bcId);
    GateRef gate = Circuit::NullGate();
    if (bytecodeInfo.IsInsufficientProfile()) {
        // handle general ecma.* bytecodes
        NewDeopt(bb);
    } else if (bytecodeInfo.IsSetConstant()) {
        // handle bytecode command to get constants
        gate = NewConst(bytecodeInfo);
        byteCodeToJSGates_[iterator.Index()].emplace_back(gate);
        jsGatesToByteCode_[gate] = iterator.Index();
    } else if (bytecodeInfo.IsGeneral()) {
        // handle general ecma.* bytecodes
        NewJSGate(bb);
    } else if (bytecodeInfo.IsJump()) {
        // handle conditional jump and unconditional jump bytecodes
        NewJump(bb);
    } else if (bytecodeInfo.IsReturn()) {
        // handle return.dyn and returnundefined bytecodes
        gate = NewReturn(bb);
    } else if (bytecodeInfo.IsMov()) {
        frameStateBuilder_.UpdateMoveValues(bytecodeInfo);
    } else if (!bytecodeInfo.IsDiscarded()) {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    if (gate != Circuit::NullGate()) {
        frameStateBuilder_.UpdateFrameValues(bytecodeInfo, iterator.Index(), gate);
    }
}

void BytecodeCircuitBuilder::BuildSubCircuit()
{
    auto &entryBlock = RegionAt(0);
    frameStateBuilder_.InitEntryBB(entryBlock);
    auto& rpoList = frameStateBuilder_.GetRpoList();
    for (auto &bbId: rpoList) {
        auto &bb = RegionAt(bbId);
        frameStateBuilder_.AdvanceToNextBB(bb);
        if (IsEntryBlock(bb.id)) {
            if (NeedCheckSafePointAndStackOver()) {
                GateRef state = frameStateBuilder_.GetCurrentState();
                GateRef depend = frameStateBuilder_.GetCurrentDepend();
                auto stackCheck = circuit_->NewGate(circuit_->CheckSafePointAndStackOver(), {state, depend});
                bb.dependCache = stackCheck;
                frameStateBuilder_.UpdateStateDepend(stackCheck, stackCheck);
            }
            auto &bbNext = RegionAt(bb.id + 1);
            frameStateBuilder_.MergeIntoSuccessor(bb, bbNext);
            bbNext.expandedPreds.push_back({bb.id, bb.end, false});
            continue;
        }
        if (!bb.trys.empty()) {
            GateRef state = frameStateBuilder_.GetCurrentState();
            GateRef depend = frameStateBuilder_.GetCurrentDepend();
            auto getException = circuit_->NewGate(circuit_->GetException(),
                MachineType::I64, {state, depend}, GateType::AnyType());
            frameStateBuilder_.UpdateAccumulator(getException);
            frameStateBuilder_.UpdateStateDepend(state, getException);
        }
        EnumerateBlock(bb, [this, &bb]
            (const BytecodeInfo &bytecodeInfo) -> bool {
            NewByteCode(bb);
            if (bytecodeInfo.IsJump() || bytecodeInfo.IsThrow()) {
                return false;
            }
            return true;
        });
        bool needFallThrough = true;
        if (!bb.IsEmptryBlock()) {
            const BytecodeInfo& bytecodeInfo = GetBytecodeInfo(bb.end);
            needFallThrough = bytecodeInfo.needFallThrough();
        }
        // fallThrough or empty merge bb
        if (needFallThrough) {
            ASSERT(bb.succs.size() == 1); // 1: fall through
            auto &bbNext = RegionAt(bb.succs[0]->id);
            frameStateBuilder_.MergeIntoSuccessor(bb, bbNext);
            bbNext.expandedPreds.push_back({bb.id, bb.end, false});
        }
    }
}

bool BytecodeCircuitBuilder::FindOsrLoopHeadBB()
{
    int32_t loopBackBcIndex {-1};
    for (size_t k = 0; k < pcOffsets_.size(); k++) {
        if (static_cast<int32_t>(pcOffsets_[k] - pcOffsets_[0]) == osrOffset_) {
            loopBackBcIndex = static_cast<int32_t>(k);
        }
    }
    if (loopBackBcIndex == -1) {
        LOG_COMPILER(ERROR) << "Unable to find the loop back of OSR.";
        return false;
    }
    auto &rpoList = frameStateBuilder_.GetRpoList();
    for (auto &bbId : rpoList) {
        auto &bb = RegionAt(bbId);
        if (bb.end == static_cast<uint32_t>(loopBackBcIndex)) {
            frameStateBuilder_.SetOsrLoopHeadBB(bb);
            return true;
        }
    }
    LOG_COMPILER(ERROR) << "Unable to find the loop head bb of OSR.";
    return false;
}

void BytecodeCircuitBuilder::GenDeoptAndReturnForOsrLoopExit(BytecodeRegion &osrLoopExitBB)
{
    frameStateBuilder_.AdvanceToNextBB(osrLoopExitBB, true);
    GateRef state = frameStateBuilder_.GetCurrentState();
    GateRef depend = frameStateBuilder_.GetCurrentDepend();
    std::string comment = Deoptimizier::DisplayItems(DeoptType::OSRLOOPEXIT);
    GateRef type =
        circuit_->GetConstantGate(MachineType::I64, static_cast<int64_t>(DeoptType::OSRLOOPEXIT), GateType::NJSValue());
    GateRef condition = circuit_->GetConstantGate(MachineType::I1, 0, GateType::NJSValue());
    GateRef deopt = circuit_->NewGate(circuit_->DeoptCheck(), MachineType::I1,
                                      {state, depend, condition, gateAcc_.GetFrameState(depend), type},
                                      GateType::NJSValue(), comment.c_str());
    GateRef dependRelay = circuit_->NewGate(circuit_->DependRelay(), {deopt, depend});
    GateRef undef =
        circuit_->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_UNDEFINED, GateType::TaggedValue());
    circuit_->NewGate(circuit_->Return(), {state, dependRelay, undef, circuit_->GetReturnRoot()});
}

void BytecodeCircuitBuilder::CollectCacheBBforOSRLoop(BytecodeRegion *bb)
{
    catchBBOfOSRLoop_.insert(bb);
    for (BytecodeRegion *succBB : bb->succs) {
        CollectCacheBBforOSRLoop(succBB);
    }
}

void BytecodeCircuitBuilder::HandleOsrLoopBody(BytecodeRegion &osrLoopBodyBB)
{
    if (!osrLoopBodyBB.trys.empty()) {
        GateRef state = frameStateBuilder_.GetCurrentState();
        GateRef depend = frameStateBuilder_.GetCurrentDepend();
        auto getException =
            circuit_->NewGate(circuit_->GetException(), MachineType::I64, {state, depend}, GateType::AnyType());
        frameStateBuilder_.UpdateAccumulator(getException);
        frameStateBuilder_.UpdateStateDepend(state, getException);
    }
    // collect catch BB.
    if (!osrLoopBodyBB.catches.empty()) {
        for (BytecodeRegion *targetBB : osrLoopBodyBB.catches) {
            CollectCacheBBforOSRLoop(targetBB);
        }
    }
    EnumerateBlock(osrLoopBodyBB, [this, &osrLoopBodyBB](const BytecodeInfo &bytecodeInfo) -> bool {
        NewByteCode(osrLoopBodyBB);
        if (bytecodeInfo.IsJump() || bytecodeInfo.IsThrow()) {
            return false;
        }
        return true;
    });
    bool needFallThrough = true;
    if (!osrLoopBodyBB.IsEmptryBlock()) {
        const BytecodeInfo &bytecodeInfo = GetBytecodeInfo(osrLoopBodyBB.end);
        needFallThrough = bytecodeInfo.needFallThrough();
    }
    // fallThrough or empty merge osrLoopBodyBB
    if (needFallThrough) {
        ASSERT(osrLoopBodyBB.succs.size() == 1);  // 1: fall through
        auto &bbNext = RegionAt(osrLoopBodyBB.succs[0]->id);
        frameStateBuilder_.MergeIntoSuccessor(osrLoopBodyBB, bbNext);
        bbNext.expandedPreds.push_back({osrLoopBodyBB.id, osrLoopBodyBB.end, false});
    }
}

void BytecodeCircuitBuilder::BuildOsrCircuit()
{
    if (!FindOsrLoopHeadBB()) {
        LOG_COMPILER(FATAL) << "invalid osr offset";
    }
    circuit_->SetIsOsr();
    auto &entryBlock = RegionAt(0);
    frameStateBuilder_.InitEntryBB(entryBlock);
    std::set<size_t> osrLoopExitBBIds;
    auto &rpoList = frameStateBuilder_.GetRpoList();
    for (auto &bbId : rpoList) {
        auto &bb = RegionAt(bbId);
        if (frameStateBuilder_.IsOsrLoopExit(bb)) {
            // The loop exit BB is in front of the loop head BB. At this time,
            // the loop exit BB does not have the context object, and the processing of the loop exit BB is delayed.
            if (frameStateBuilder_.IsContextExists(bb.id)) {
                GenDeoptAndReturnForOsrLoopExit(bb);
            } else {
                osrLoopExitBBIds.insert(bbId);
            }
            continue;
        }

        // Processes only the BBs related to the loop specified by the OSR.
        if (!IsEntryBlock(bb.id) && frameStateBuilder_.OutOfOsrLoop(bb) && !IsCacheBBOfOSRLoop(bb)) {
            continue;
        }

        frameStateBuilder_.AdvanceToNextBB(bb);
        if (IsEntryBlock(bb.id)) {
            if (NeedCheckSafePointAndStackOver()) {
                GateRef state = frameStateBuilder_.GetCurrentState();
                GateRef depend = frameStateBuilder_.GetCurrentDepend();
                auto stackCheck = circuit_->NewGate(circuit_->CheckSafePointAndStackOver(), {state, depend});
                bb.dependCache = stackCheck;
                frameStateBuilder_.UpdateStateDepend(stackCheck, stackCheck);
            }
            auto *bbNext = &RegionAt(bb.id + 1);
            while (!IsEntryBlock(bbNext->id) && frameStateBuilder_.OutOfOsrLoop(*bbNext)) {
                bbNext = &RegionAt(bbNext->id + 1);
            }
            frameStateBuilder_.MergeIntoSuccessor(bb, *bbNext);
            bbNext->expandedPreds.push_back({bb.id, bb.end, false});
            continue;
        }

        HandleOsrLoopBody(bb);
    }
    for (size_t bbId : osrLoopExitBBIds) {
        auto &bb = RegionAt(bbId);
        GenDeoptAndReturnForOsrLoopExit(bb);
    }
}

void BytecodeCircuitBuilder::BuildCircuit()
{
    if (IsOSR()) {
        // create osr arg gates array
        BuildOSRArgs();
        frameStateBuilder_.DoBytecodeAnalysis();
        if (TerminateAnalysis()) {
            return;
        }
        // build states sub-circuit of osr block
        BuildOsrCircuit();
    } else {
        // create arg gates array
        BuildCircuitArgs();
        frameStateBuilder_.DoBytecodeAnalysis();
        if (TerminateAnalysis()) {
            return;
        }
        // build states sub-circuit of each block
        BuildSubCircuit();
    }
    if (IsLogEnabled()) {
        PrintGraph(std::string("Bytecode2Gate [" + methodName_ + "]").c_str());
        LOG_COMPILER(INFO) << "\033[34m" << "============= "
                           << "After bytecode2circuit lowering ["
                           << methodName_ << "]"
                           << " =============" << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "=========================== End ===========================" << "\033[0m";
    }
}

void BytecodeCircuitBuilder::PrintGraph(const char* title)
{
    LOG_COMPILER(INFO) << "======================== " << title << " ========================";
    for (size_t i = 0; i < graph_.size(); i++) {
        BytecodeRegion& bb = RegionAt(i);
        if (!IsEntryBlock(bb.id) && bb.numOfStatePreds == 0) {
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

        for (size_t j = 0; j < bb.catches.size(); j++) {
            LOG_COMPILER(INFO) << "\tcatch [: " << std::to_string(bb.catches[j]->start) << ", "
                               << std::to_string(bb.catches[j]->end) << ")";
        }

        std::string log2("\tTrys: ");
        for (auto tryBlock: bb.trys) {
            log2 += std::to_string(tryBlock->id) + " , ";
        }
        LOG_COMPILER(INFO) << log2;

        PrintBytecodeInfo(bb);
        LOG_COMPILER(INFO) << "";
    }
}

void BytecodeCircuitBuilder::PrintBytecodeInfo(BytecodeRegion& bb)
{
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
