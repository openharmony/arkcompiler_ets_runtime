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
    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> exceptionInfo;

    // collect try catch block info
    CollectTryCatchBlockInfo(exceptionInfo);

    // Complete bytecode block Information
    CompleteBytecodeBlockInfo();

    // Building the basic block diagram of bytecode
    BuildBasicBlocks(exceptionInfo);
}

void BytecodeCircuitBuilder::CollectBytecodeBlockInfo(uint8_t *pc, std::vector<CfgInfo> &bytecodeBlockInfos)
{
    BytecodeInstruction inst(pc);
    auto opcode = inst.GetOpcode();
    auto bytecodeOffset = BytecodeInstruction::Size(opcode);
    switch (static_cast<EcmaOpcode>(opcode)) {
        case EcmaOpcode::JMP_IMM8: {
            int8_t offset = static_cast<int8_t>(READ_INST_8_0());
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            // current basic block end
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + bytecodeOffset, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + bytecodeOffset));
            // jump basic block start
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JMP_IMM16: {
            int16_t offset = static_cast<int16_t>(READ_INST_16_0());
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + bytecodeOffset, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + bytecodeOffset));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JMP_IMM32: {
            int32_t offset = static_cast<int32_t>(READ_INST_32_0());
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + bytecodeOffset, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + bytecodeOffset));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JEQZ_IMM8: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + bytecodeOffset);   // first successor
            int8_t offset = static_cast<int8_t>(READ_INST_8_0());
            temp.emplace_back(pc + offset);  // second successor
            // condition branch current basic block end
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            // first branch basic block start
            bytecodeBlockInfos.emplace_back(pc + bytecodeOffset, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + bytecodeOffset));
            // second branch basic block start
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JEQZ_IMM16: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + bytecodeOffset);   // first successor
            int16_t offset = static_cast<int16_t>(READ_INST_16_0());
            temp.emplace_back(pc + offset);  // second successor
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp); // end
            bytecodeBlockInfos.emplace_back(pc + bytecodeOffset, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + bytecodeOffset));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JEQZ_IMM32: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + bytecodeOffset);   // first successor
            int16_t offset = static_cast<int16_t>(READ_INST_32_0());
            temp.emplace_back(pc + offset);  // second successor
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp); // end
            bytecodeBlockInfos.emplace_back(pc + bytecodeOffset, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + bytecodeOffset));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JNEZ_IMM8: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + bytecodeOffset); // first successor
            int8_t offset = static_cast<int8_t>(READ_INST_8_0());
            temp.emplace_back(pc + offset); // second successor
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + bytecodeOffset, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + bytecodeOffset));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JNEZ_IMM16: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + bytecodeOffset); // first successor
            int16_t offset = static_cast<int16_t>(READ_INST_16_0());
            temp.emplace_back(pc + offset); // second successor
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + bytecodeOffset, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + bytecodeOffset));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::JNEZ_IMM32: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + bytecodeOffset); // first successor
            int16_t offset = static_cast<int16_t>(READ_INST_32_0());
            temp.emplace_back(pc + offset); // second successor
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, temp);
            bytecodeBlockInfos.emplace_back(pc + bytecodeOffset, SplitKind::START,
                                            std::vector<uint8_t *>(1, pc + bytecodeOffset));
            bytecodeBlockInfos.emplace_back(pc + offset, SplitKind::START, std::vector<uint8_t *>(1, pc + offset));
        }
            break;
        case EcmaOpcode::RETURN:
        case EcmaOpcode::RETURNUNDEFINED:
        case EcmaOpcode::THROW_PREF_NONE:
        case EcmaOpcode::THROW_CONSTASSIGNMENT_PREF_V8:
        case EcmaOpcode::THROW_NOTEXISTS_PREF_NONE:
        case EcmaOpcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
        case EcmaOpcode::THROW_DELETESUPERPROPERTY_PREF_NONE: {
            bytecodeBlockInfos.emplace_back(pc, SplitKind::END, std::vector<uint8_t *>(1, pc));
            break;
        }
        default:
            break;
    }
}

void BytecodeCircuitBuilder::CollectTryCatchBlockInfo(std::map<std::pair<uint8_t *, uint8_t *>,
                                                      std::vector<uint8_t *>> &byteCodeException)
{
    // try contains many catch
    panda_file::MethodDataAccessor mda(*pf_, method_->GetMethodId());
    panda_file::CodeDataAccessor cda(*pf_, mda.GetCodeId().value());

    cda.EnumerateTryBlocks([this, &byteCodeException](
            panda_file::CodeDataAccessor::TryBlock &try_block) {
        auto tryStartOffset = try_block.GetStartPc();
        auto tryEndOffset = try_block.GetStartPc() + try_block.GetLength();
        auto tryStartPc = const_cast<uint8_t *>(method_->GetBytecodeArray() + tryStartOffset);
        auto tryEndPc = const_cast<uint8_t *>(method_->GetBytecodeArray() + tryEndOffset);
        // skip try blocks with same pc in start and end label
        if (tryStartPc == tryEndPc) {
            return true;
        }
        byteCodeException[std::make_pair(tryStartPc, tryEndPc)] = {};
        uint32_t pcOffset = panda_file::INVALID_OFFSET;
        try_block.EnumerateCatchBlocks([&](panda_file::CodeDataAccessor::CatchBlock &catch_block) {
            pcOffset = catch_block.GetHandlerPc();
            auto catchBlockPc = const_cast<uint8_t *>(method_->GetBytecodeArray() + pcOffset);
            // try block associate catch block
            byteCodeException[std::make_pair(tryStartPc, tryEndPc)].emplace_back(catchBlockPc);
            return true;
        });
        // Check whether the previous block of the try block exists.
        // If yes, add the current block; otherwise, create a new block.
        bool flag = false;
        for (size_t i = 0; i < bytecodeBlockInfos_.size(); i++) {
            if (bytecodeBlockInfos_[i].splitKind == SplitKind::START) {
                continue;
            }
            if (bytecodeBlockInfos_[i].pc == byteCodeCurPrePc_.at(tryStartPc)) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            // pre block
            if (byteCodeCurPrePc_.at(tryStartPc) != tryStartPc) {
                bytecodeBlockInfos_.emplace_back(byteCodeCurPrePc_.at(tryStartPc), SplitKind::END,
                                                 std::vector<uint8_t *>(1, tryStartPc));
            }
        }
        // try block
        bytecodeBlockInfos_.emplace_back(tryStartPc, SplitKind::START, std::vector<uint8_t *>(1, tryStartPc));
        flag = false;
        for (size_t i = 0; i < bytecodeBlockInfos_.size(); i++) {
            if (bytecodeBlockInfos_[i].splitKind == SplitKind::START) {
                continue;
            }
            if (bytecodeBlockInfos_[i].pc == byteCodeCurPrePc_.at(tryEndPc)) {
                auto &succs = bytecodeBlockInfos_[i].succs;
                auto iter = std::find(succs.cbegin(), succs.cend(), bytecodeBlockInfos_[i].pc);
                if (iter == succs.cend()) {
                    auto opcode = PcToOpcode(bytecodeBlockInfos_[i].pc);
                    switch (opcode) {
                        case EcmaOpcode::JMP_IMM8:
                        case EcmaOpcode::JMP_IMM16:
                        case EcmaOpcode::JMP_IMM32:
                        case EcmaOpcode::JEQZ_IMM8:
                        case EcmaOpcode::JEQZ_IMM16:
                        case EcmaOpcode::JEQZ_IMM32:
                        case EcmaOpcode::JNEZ_IMM8:
                        case EcmaOpcode::JNEZ_IMM16:
                        case EcmaOpcode::JNEZ_IMM32:
                        case EcmaOpcode::RETURN:
                        case EcmaOpcode::RETURNUNDEFINED:
                        case EcmaOpcode::THROW_PREF_NONE: {
                            break;
                        }
                        default: {
                            succs.emplace_back(tryEndPc);
                            break;
                        }
                    }
                }
                flag = true;
                break;
            }
        }
        if (!flag) {
            bytecodeBlockInfos_.emplace_back(byteCodeCurPrePc_.at(tryEndPc), SplitKind::END,
                                             std::vector<uint8_t *>(1, tryEndPc));
        }
        bytecodeBlockInfos_.emplace_back(tryEndPc, SplitKind::START, std::vector<uint8_t *>(1, tryEndPc)); // next block
        return true;
    });
}

void BytecodeCircuitBuilder::CompleteBytecodeBlockInfo()
{
    std::sort(bytecodeBlockInfos_.begin(), bytecodeBlockInfos_.end());

    // Deduplicate
    auto deduplicateIndex = std::unique(bytecodeBlockInfos_.begin(), bytecodeBlockInfos_.end());
    bytecodeBlockInfos_.erase(deduplicateIndex, bytecodeBlockInfos_.end());

    // Supplementary block information
    // endBlockPc: Pairs occur, with odd indexes indicating endPc, and even indexes indicating startPc.
    std::vector<uint8_t *> endBlockPc;
    std::vector<uint8_t *> startBlockPc; //
    for (size_t i = 0; i < bytecodeBlockInfos_.size() - 1; i++) {
        if (bytecodeBlockInfos_[i].splitKind == bytecodeBlockInfos_[i + 1].splitKind &&
            bytecodeBlockInfos_[i].splitKind == SplitKind::START) {
            auto prePc = byteCodeCurPrePc_.at(bytecodeBlockInfos_[i + 1].pc);
            endBlockPc.emplace_back(prePc); // Previous instruction of current instruction
            endBlockPc.emplace_back(bytecodeBlockInfos_[i + 1].pc); // current instruction
            continue;
        }
        if (bytecodeBlockInfos_[i].splitKind == bytecodeBlockInfos_[i + 1].splitKind &&
            bytecodeBlockInfos_[i].splitKind == SplitKind::END) {
            auto tempPc = bytecodeBlockInfos_[i].pc;
            auto findItem = std::find_if(byteCodeCurPrePc_.cbegin(), byteCodeCurPrePc_.cend(),
                                         [tempPc](const std::map<uint8_t *, uint8_t *>::value_type item) {
                                             return item.second == tempPc;
                                         });
            if (findItem != byteCodeCurPrePc_.cend()) {
                startBlockPc.emplace_back((*findItem).first);
            }
        }
    }

    // Supplementary end block info
    for (auto iter = endBlockPc.cbegin(); iter != endBlockPc.cend(); iter += 2) { // 2: index
        bytecodeBlockInfos_.emplace_back(*iter, SplitKind::END, std::vector<uint8_t *>(1, *(iter + 1)));
    }
    // Supplementary start block info
    for (auto iter = startBlockPc.cbegin(); iter != startBlockPc.cend(); iter++) {
        bytecodeBlockInfos_.emplace_back(*iter, SplitKind::START, std::vector<uint8_t *>(1, *iter));
    }

    // Deduplicate successor
    for (size_t i = 0; i < bytecodeBlockInfos_.size(); i++) {
        if (bytecodeBlockInfos_[i].splitKind == SplitKind::END) {
            std::set<uint8_t *> tempSet(bytecodeBlockInfos_[i].succs.cbegin(),
                                        bytecodeBlockInfos_[i].succs.cend());
            bytecodeBlockInfos_[i].succs.assign(tempSet.cbegin(), tempSet.cend());
        }
    }

    std::sort(bytecodeBlockInfos_.begin(), bytecodeBlockInfos_.end());

    // handling jumps to an empty block
    auto endPc = bytecodeBlockInfos_[bytecodeBlockInfos_.size() - 1].pc;
    auto iter = --byteCodeCurPrePc_.cend();
    if (endPc == iter->first) {
        bytecodeBlockInfos_.emplace_back(endPc, SplitKind::END, std::vector<uint8_t *>(1, endPc));
    }
    // Deduplicate
    deduplicateIndex = std::unique(bytecodeBlockInfos_.begin(), bytecodeBlockInfos_.end());
    bytecodeBlockInfos_.erase(deduplicateIndex, bytecodeBlockInfos_.end());
}

void BytecodeCircuitBuilder::BuildBasicBlocks(std::map<std::pair<uint8_t *, uint8_t *>,
                                              std::vector<uint8_t *>> &exception)
{
    std::map<uint8_t *, BytecodeRegion *> startPcToBB; // [start, bb]
    std::map<uint8_t *, BytecodeRegion *> endPcToBB; // [end, bb]
    graph_.resize(bytecodeBlockInfos_.size() / 2); // 2 : half size
    // build basic block
    int blockId = 0;
    int index = 0;
    for (size_t i = 0; i < bytecodeBlockInfos_.size() - 1; i += 2) { // 2:index
        auto startPc = bytecodeBlockInfos_[i].pc;
        auto endPc = bytecodeBlockInfos_[i + 1].pc;
        auto block = &graph_[index++];
        block->id = blockId++;
        block->start = startPc;
        block->end = endPc;
        block->preds = {};
        block->succs = {};
        startPcToBB[startPc] = block;
        endPcToBB[endPc] = block;
        block->bytecodeIterator_.Reset(this, startPc, endPc);
    }

    // add block associate
    for (size_t i = 0; i < bytecodeBlockInfos_.size(); i++) {
        if (bytecodeBlockInfos_[i].splitKind == SplitKind::START) {
            continue;
        }
        auto curPc = bytecodeBlockInfos_[i].pc;
        auto &successors = bytecodeBlockInfos_[i].succs;
        for (size_t j = 0; j < successors.size(); j++) {
            if (successors[j] == curPc) {
                continue;
            }
            auto curBlock = endPcToBB[curPc];
            auto succsBlock = startPcToBB[successors[j]];
            curBlock->succs.emplace_back(succsBlock);
            succsBlock->preds.emplace_back(curBlock);
        }
    }

    // try catch block associate
    for (size_t i = 0; i < graph_.size(); i++) {
        const auto pc = graph_[i].start;
        auto it = exception.cbegin();
        for (; it != exception.cend(); it++) {
            if (pc < it->first.first || pc >= it->first.second) { // try block interval
                continue;
            }
            auto catchs = exception[it->first]; // catchs start pc
            for (size_t j = i + 1; j < graph_.size(); j++) {
                if (std::find(catchs.cbegin(), catchs.cend(), graph_[j].start) != catchs.cend()) {
                    graph_[i].catchs.insert(graph_[i].catchs.cbegin(), &graph_[j]);
                    graph_[i].succs.emplace_back(&graph_[j]);
                    graph_[j].preds.emplace_back(&graph_[i]);
                }
            }
        }

        // When there are multiple catch blocks in the current block, the set of catch blocks
        // needs to be sorted to satisfy the order of execution of catch blocks.
        BytecodeRegion& bb = graph_[i];
        bb.SortCatches();
    }

    if (IsLogEnabled()) {
        PrintGraph("Build Basic Block");
    }
    ComputeDominatorTree();
}

void BytecodeCircuitBuilder::ComputeDominatorTree()
{
    // Construct graph backward order
    std::map<size_t, size_t> bbIdToDfsTimestamp;
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

void BytecodeCircuitBuilder::RemoveDeadRegions(const std::map<size_t, size_t> &bbIdToDfsTimestamp)
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
    std::map<uint16_t, std::set<size_t>> defsitesInfo; // <vreg, bbs>
    for (auto &bb : graph_) {
        if (bb.isDead) {
            continue;
        }
        EnumerateBlock(bb, [this, &defsitesInfo, &bb]
            (const BytecodeInfo &bytecodeInfo) -> bool {
            if (bytecodeInfo.IsBc(EcmaOpcode::RESUMEGENERATOR)) {
                auto numVRegs = method_->GetNumberVRegs();
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

void BytecodeCircuitBuilder::InsertExceptionPhi(std::map<uint16_t, std::set<size_t>> &defsitesInfo)
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
                auto numVRegs = method_->GetNumberVRegs();
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
    state = circuit_.NewGate(OpCode(OpCode::MERGE), numOfIns,
                             std::vector<GateRef>(numOfIns, Circuit::NullGate()),
                             GateType::Empty());
    depend = circuit_.NewGate(OpCode(OpCode::DEPEND_SELECTOR), numOfIns,
                              std::vector<GateRef>(numOfIns + 1, Circuit::NullGate()),
                              GateType::Empty());
    gateAcc_.NewIn(depend, 0, state);
}

void BytecodeCircuitBuilder::NewLoopBegin(BytecodeRegion &bb)
{
    if (bb.id == 0 && bb.numOfStatePreds == 1) {
        bb.mergeForwardEdges = circuit_.NewGate(OpCode(OpCode::MERGE),
            bb.numOfStatePreds, std::vector<GateRef>(bb.numOfStatePreds,
                                                     Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY))),
            GateType::Empty());
        bb.depForward = circuit_.NewGate(OpCode(OpCode::DEPEND_SELECTOR),
            bb.numOfStatePreds, std::vector<GateRef>(bb.numOfStatePreds + 1, Circuit::NullGate()), GateType::Empty());
        gateAcc_.NewIn(bb.depForward, 0, bb.mergeForwardEdges);
        gateAcc_.NewIn(bb.depForward, 1, Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)));
    } else {
        NewMerge(bb.mergeForwardEdges, bb.depForward, bb.numOfStatePreds - bb.numOfLoopBacks);
    }
    NewMerge(bb.mergeLoopBackEdges, bb.depLoopBack, bb.numOfLoopBacks);
    auto loopBack = circuit_.NewGate(OpCode(OpCode::LOOP_BACK), 0,
                                     {bb.mergeLoopBackEdges}, GateType::Empty());
    bb.stateStart = circuit_.NewGate(OpCode(OpCode::LOOP_BEGIN), 0,
                                     {bb.mergeForwardEdges, loopBack}, GateType::Empty());
    // 2: the number of depend inputs and it is in accord with LOOP_BEGIN
    bb.dependStart = circuit_.NewGate(OpCode(OpCode::DEPEND_SELECTOR), 2,
                                      {bb.stateStart, bb.depForward, bb.depLoopBack},
                                      GateType::Empty());
}

void BytecodeCircuitBuilder::BuildBlockCircuitHead()
{
    for (auto &bb: graph_) {
        if (bb.isDead) {
            continue;
        }
        if (bb.numOfStatePreds == 0) {
            bb.stateStart = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
            bb.dependStart = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
        } else if (bb.numOfLoopBacks > 0) {
            NewLoopBegin(bb);
        } else {
            NewMerge(bb.stateStart, bb.dependStart, bb.numOfStatePreds);
        }
    }
}

std::vector<GateRef> BytecodeCircuitBuilder::CreateGateInList(const BytecodeInfo &info)
{
    size_t numValueInputs = info.ComputeValueInputCount();
    const size_t length = 2; // 2: state and depend on input
    const size_t numBCOffsetInput = info.ComputeBCOffsetInputCount();
    std::vector<GateRef> inList(length + numValueInputs + numBCOffsetInput, Circuit::NullGate());
    for (size_t i = 0; i < info.inputs.size(); i++) {
        auto &input = info.inputs[i];
        if (std::holds_alternative<ConstDataId>(input)) {
            if (std::get<ConstDataId>(input).IsStringId()) {
                tsManager_->AddIndexOrSkippedMethodID(CacheType::STRING,
                    std::get<ConstDataId>(input).GetId());
                inList[i + length] = circuit_.GetConstantDataGate(std::get<ConstDataId>(input).CaculateBitField(),
                                                                  GateType::StringType());
                continue;
            } else if (std::get<ConstDataId>(input).IsMethodId()) {
                tsManager_->AddIndexOrSkippedMethodID(CacheType::METHOD,
                    std::get<ConstDataId>(input).GetId());
            } else if (std::get<ConstDataId>(input).IsClassLiteraId()) {
                tsManager_->AddIndexOrSkippedMethodID(CacheType::CLASS_LITERAL,
                    std::get<ConstDataId>(input).GetId(), recordName_);
            } else if (std::get<ConstDataId>(input).IsObjectLiteralID()) {
                tsManager_->AddIndexOrSkippedMethodID(CacheType::OBJECT_LITERAL,
                    std::get<ConstDataId>(input).GetId(), recordName_);
            } else if (std::get<ConstDataId>(input).IsArrayLiteralID()) {
                tsManager_->AddIndexOrSkippedMethodID(CacheType::ARRAY_LITERAL,
                    std::get<ConstDataId>(input).GetId(), recordName_);
            }
            inList[i + length] = circuit_.GetConstantGate(MachineType::I64,
                                                              std::get<ConstDataId>(input).GetId(),
                                                              GateType::NJSValue());
        } else if (std::holds_alternative<Immediate>(input)) {
            inList[i + length] = circuit_.GetConstantGate(MachineType::I64,
                                                          std::get<Immediate>(input).GetValue(),
                                                          GateType::NJSValue());
        } else if (std::holds_alternative<ICSlotId>(input)) {
            inList[i + length] = circuit_.GetConstantGate(MachineType::I16,
                                                          std::get<ICSlotId>(input).GetId(),
                                                          GateType::NJSValue());
        } else {
            ASSERT(std::holds_alternative<VirtualRegister>(input));
            continue;
        }
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
            gate = circuit_.GetConstantGate(MachineType::I64,
                                            base::NumberHelper::GetNaN(),
                                            GateType::TaggedValue());
            break;
        case EcmaOpcode::LDINFINITY:
            gate = circuit_.GetConstantGate(MachineType::I64,
                                            base::NumberHelper::GetPositiveInfinity(),
                                            GateType::TaggedValue());
            break;
        case EcmaOpcode::LDUNDEFINED:
            gate = circuit_.GetConstantGate(MachineType::I64,
                                            JSTaggedValue::VALUE_UNDEFINED,
                                            GateType::TaggedValue());
            break;
        case EcmaOpcode::LDNULL:
            gate = circuit_.GetConstantGate(MachineType::I64,
                                            JSTaggedValue::VALUE_NULL,
                                            GateType::TaggedValue());
            break;
        case EcmaOpcode::LDTRUE:
            gate = circuit_.GetConstantGate(MachineType::I64,
                                            JSTaggedValue::VALUE_TRUE,
                                            GateType::TaggedValue());
            break;
        case EcmaOpcode::LDFALSE:
            gate = circuit_.GetConstantGate(MachineType::I64,
                                            JSTaggedValue::VALUE_FALSE,
                                            GateType::TaggedValue());
            break;
        case EcmaOpcode::LDHOLE:
            gate = circuit_.GetConstantGate(MachineType::I64,
                                            JSTaggedValue::VALUE_HOLE,
                                            GateType::TaggedValue());
            break;
        case EcmaOpcode::LDAI_IMM32:
            gate = circuit_.GetConstantGate(MachineType::I64,
                                            std::get<Immediate>(info.inputs[0]).ToJSTaggedValueInt(),
                                            GateType::TaggedValue());
            break;
        case EcmaOpcode::FLDAI_IMM64:
            gate = circuit_.GetConstantGate(MachineType::I64,
                                            std::get<Immediate>(info.inputs.at(0)).ToJSTaggedValueDouble(),
                                            GateType::TaggedValue());
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
            if (input.IsStringId()) {
                tsManager_->AddIndexOrSkippedMethodID(CacheType::STRING, input.GetId());
            }
            gate = circuit_.GetConstantDataGate(input.CaculateBitField(), GateType::StringType());
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
    auto pc = bytecodeInfo.GetPC();
    size_t numValueInputs = bytecodeInfo.ComputeTotalValueCount();
    GateRef gate = 0;
    std::vector<GateRef> inList = CreateGateInList(bytecodeInfo);
    if (bytecodeInfo.IsDef()) {
        gate = circuit_.NewGate(OpCode(OpCode::JS_BYTECODE), MachineType::I64, numValueInputs,
                                inList, GateType::AnyType());
    } else {
        gate = circuit_.NewGate(OpCode(OpCode::JS_BYTECODE), MachineType::NOVALUE, numValueInputs,
                                inList, GateType::Empty());
    }
    // 1: store bcoffset in the end.
    AddBytecodeOffsetInfo(gate, bytecodeInfo, numValueInputs + 1, const_cast<uint8_t *>(pc));
    gateAcc_.NewIn(gate, 0, state);
    gateAcc_.NewIn(gate, 1, depend);
    auto ifSuccess = circuit_.NewGate(OpCode(OpCode::IF_SUCCESS), 0, {gate}, GateType::Empty());
    auto ifException = circuit_.NewGate(OpCode(OpCode::IF_EXCEPTION), 0, {gate}, GateType::Empty());
    if (!bb.catchs.empty()) {
        auto &bbNext = bb.catchs.at(0);
        auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
        SetBlockPred(*bbNext, ifException, gate, isLoopBack);
        if (bytecodeInfo.GetOpcode() == EcmaOpcode::CREATEASYNCGENERATOROBJ_V8) {
            bbNext->expandedPreds.push_back({bb.id, iterator.Index() + 1, true}); // 1: next pc
        } else {
            bbNext->expandedPreds.push_back({bb.id, iterator.Index(), true});
        }
    } else {
        auto constant = circuit_.GetConstantGate(MachineType::I64,
                                                 JSTaggedValue::VALUE_EXCEPTION,
                                                 GateType::TaggedValue());
        circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                         {ifException, gate, constant,
                         Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                         GateType::Empty());
    }
    jsgateToBytecode_[gate] = { bb.id, iterator.Index() };
    byteCodeToJSGate_[pc] = gate;
    if (bytecodeInfo.IsGeneratorRelative()) {
        suspendAndResumeGates_.emplace_back(gate);
    }
    if (bytecodeInfo.IsThrow()) {
        auto constant = circuit_.GetConstantGate(MachineType::I64,
                                                 JSTaggedValue::VALUE_EXCEPTION,
                                                 GateType::TaggedValue());
        circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                         {ifSuccess, gate, constant,
                         Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST))},
                         GateType::Empty());
        return;
    }
    state = ifSuccess;
    depend = gate;
    if (pc == bb.end) {
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
    auto pc = bytecodeInfo.GetPC();
    size_t numValueInputs = bytecodeInfo.ComputeValueInputCount();
    if (bytecodeInfo.IsCondJump()) {
        GateRef gate = 0;
        gate = circuit_.NewGate(OpCode(OpCode::JS_BYTECODE), MachineType::NOVALUE, numValueInputs,
                                std::vector<GateRef>(2 + numValueInputs, // 2: state and depend input
                                                     Circuit::NullGate()),
                                GateType::Empty());
        gateAcc_.NewIn(gate, 0, state);
        gateAcc_.NewIn(gate, 1, depend);
        auto ifTrue = circuit_.NewGate(OpCode(OpCode::IF_TRUE), 0, {gate}, GateType::Empty());
        auto trueRelay = circuit_.NewGate(OpCode(OpCode::DEPEND_RELAY), 0, {ifTrue, gate}, GateType::Empty());
        auto ifFalse = circuit_.NewGate(OpCode(OpCode::IF_FALSE), 0, {gate}, GateType::Empty());
        auto falseRelay = circuit_.NewGate(OpCode(OpCode::DEPEND_RELAY), 0, {ifFalse, gate}, GateType::Empty());
        if (bb.succs.size() == 1) {
            auto &bbNext = bb.succs[0];
            ASSERT(bbNext->id == bb.id + 1);
            auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
            SetBlockPred(*bbNext, ifFalse, trueRelay, isLoopBack);
            SetBlockPred(*bbNext, ifTrue, falseRelay, isLoopBack);
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
        jsgateToBytecode_[gate] = { bb.id, iterator.Index() };
        byteCodeToJSGate_[pc] = gate;
    } else {
        ASSERT(bb.succs.size() == 1);
        auto &bbNext = bb.succs.at(0);
        auto isLoopBack = bbNext->loopbackBlocks.count(bb.id);
        SetBlockPred(*bbNext, state, depend, isLoopBack);
        bbNext->expandedPreds.push_back({bb.id, iterator.Index(), false});
    }
}

void BytecodeCircuitBuilder::NewReturn(BytecodeRegion &bb, GateRef &state, GateRef &depend)
{
    ASSERT(bb.succs.empty());
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    auto pc = bytecodeInfo.GetPC();
    if (bytecodeInfo.GetOpcode() == EcmaOpcode::RETURN) {
        // handle return.dyn bytecode
        auto gate = circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                                     { state, depend, Circuit::NullGate(),
                                     Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST)) },
                                     GateType::Empty());
        jsgateToBytecode_[gate] = { bb.id, iterator.Index() };
        byteCodeToJSGate_[pc] = gate;
    } else if (bytecodeInfo.GetOpcode() == EcmaOpcode::RETURNUNDEFINED) {
        // handle returnundefined bytecode
        auto constant = circuit_.GetConstantGate(MachineType::I64,
                                                 JSTaggedValue::VALUE_UNDEFINED,
                                                 GateType::TaggedValue());
        auto gate = circuit_.NewGate(OpCode(OpCode::RETURN), 0,
                                     { state, depend, constant,
                                     Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST)) },
                                     GateType::Empty());
        jsgateToBytecode_[gate] = { bb.id, iterator.Index() };
        byteCodeToJSGate_[pc] = gate;
    }
}

void BytecodeCircuitBuilder::NewByteCode(BytecodeRegion &bb, GateRef &state, GateRef &depend)
{
    auto &iterator = bb.GetBytecodeIterator();
    const BytecodeInfo& bytecodeInfo = iterator.GetBytecodeInfo();
    auto pc = bytecodeInfo.GetPC();
    if (bytecodeInfo.IsSetConstant()) {
        // handle bytecode command to get constants
        GateRef gate = NewConst(bytecodeInfo);
        jsgateToBytecode_[gate] = { bb.id, iterator.Index() };
        byteCodeToJSGate_[pc] = gate;
        if (pc == bb.end) {
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
        if (pc == bb.end) {
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
            dependCur = circuit_.NewGate(OpCode(OpCode::GET_EXCEPTION), 0, {dependCur}, GateType::Empty());
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
        currentPhi =
            circuit_.NewGate(OpCode(OpCode::VALUE_SELECTOR), MachineType::I64, bb.numOfStatePreds,
                             std::vector<GateRef>(1 + bb.numOfStatePreds, Circuit::NullGate()), GateType::AnyType());
        gateAcc_.NewIn(currentPhi, 0, bb.stateStart);
        for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
            auto &[predId, predBcIdx, isException] = bb.expandedPreds.at(i);
            gateAcc_.NewIn(currentPhi, i + 1, ResolveDef(predId, predBcIdx, reg, acc));
        }
    } else {
        // 2: the number of value inputs and it is in accord with LOOP_BEGIN
        currentPhi = circuit_.NewGate(OpCode(OpCode::VALUE_SELECTOR), MachineType::I64, 2,
                                      {bb.stateStart, Circuit::NullGate(), Circuit::NullGate()}, GateType::AnyType());
        auto loopBackValue =
            circuit_.NewGate(OpCode(OpCode::VALUE_SELECTOR), MachineType::I64, bb.numOfLoopBacks,
                             std::vector<GateRef>(1 + bb.numOfLoopBacks, Circuit::NullGate()), GateType::AnyType());
        gateAcc_.NewIn(loopBackValue, 0, bb.mergeLoopBackEdges);
        size_t loopBackIndex = 1;  // 1: start index of value inputs
        for (size_t i = 0; i < bb.numOfStatePreds; ++i) {
            auto &[predId, predBcIdx, isException] = bb.expandedPreds.at(i);
            if (bb.loopbackBlocks.count(predId)) {
                gateAcc_.NewIn(loopBackValue, loopBackIndex++, ResolveDef(predId, predBcIdx, reg, acc));
            }
        }
        auto forwardValue = circuit_.NewGate(
            OpCode(OpCode::VALUE_SELECTOR), MachineType::I64, bb.numOfStatePreds - bb.numOfLoopBacks,
            std::vector<GateRef>(1 + bb.numOfStatePreds - bb.numOfLoopBacks, Circuit::NullGate()), GateType::AnyType());
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
GateRef BytecodeCircuitBuilder::ResolveDef(const size_t bbId, int32_t bcId,
                                               const uint16_t reg, const bool acc)
{
    auto tmpReg = reg;
    // find def-site in bytecodes of basic block
    auto ans = Circuit::NullGate();
    auto &bb = graph_.at(bbId);
    GateType type = GateType::AnyType();
    auto tmpAcc = acc;
    auto &iterator = bb.GetBytecodeIterator();
    for (iterator.Goto(bcId); !iterator.Done(); --iterator) {
        const BytecodeInfo& curInfo = iterator.GetBytecodeInfo();
        auto pcIter = curInfo.GetPC();
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
                    type = typeRecorder_.UpdateType(pcToBCOffset_.at(pcIter) - 1, type);
                }
            } else {
                ans = byteCodeToJSGate_.at(pcIter);
                if (HasTypes() && !type.IsAnyType()) {
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
        auto resumeGate = byteCodeToJSGate_.at(pcIter);
        ans = GetExistingRestore(resumeGate, tmpReg);
        if (ans != Circuit::NullGate()) {
            break;
        }
        GateRef resumeDependGate = gateAcc_.GetDep(resumeGate);
        ans = circuit_.NewGate(OpCode(OpCode::RESTORE_REGISTER), MachineType::I64, tmpReg,
                               {resumeDependGate}, GateType::AnyType());
        SetExistingRestore(resumeGate, tmpReg, ans);
        gateAcc_.SetDep(resumeGate, ans);
        bcId = iterator.Index();
        auto saveRegGate = ResolveDef(bbId, iterator.Index() - 1, tmpReg, tmpAcc);
        iterator.Goto(bcId);
        auto nextPcIter = iterator.PeekPrevPc(2); // 2: skip 2
        ASSERT(Bytecodes::GetOpcode(nextPcIter) == EcmaOpcode::SUSPENDGENERATOR_V8);
        GateRef suspendGate = byteCodeToJSGate_.at(nextPcIter);
        auto dependGate = gateAcc_.GetDep(suspendGate);
        auto newDependGate = circuit_.NewGate(OpCode(OpCode::SAVE_REGISTER), tmpReg, {dependGate, saveRegGate},
                                              GateType::Empty());
        gateAcc_.SetDep(suspendGate, newDependGate);
        break;
    }
    // find GET_EXCEPTION gate if this is a catch block
    if (ans == Circuit::NullGate() && tmpAcc) {
        if (!bb.trys.empty()) {
            std::vector<GateRef> outList;
            gateAcc_.GetOutVector(bb.dependStart, outList);
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
        ans = argAcc_.GetArgGate(tmpReg);
        return ans;
    }
    if (ans == Circuit::NullGate()) {
        // recursively find def-site in dominator block
        auto dom = bb.iDominator;
        auto &domIterator = dom->GetBytecodeIterator();
        return ResolveDef(dom->id, domIterator.GetEndBcIndex(), tmpReg, tmpAcc);
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
    }
    // resolve def-site of virtual regs and set all value inputs
    std::vector<GateRef> gates;
    circuit_.GetAllGates(gates);
    for (auto gate: gates) {
        auto valueCount = gateAcc_.GetInValueCount(gate);
        auto it = jsgateToBytecode_.find(gate);
        if (it == jsgateToBytecode_.cend()) {
            continue;
        }
        if (gateAcc_.IsConstant(gate)) {
            continue;
        }
        const auto &[bbIndex, bcIndex] = it->second;
        const BytecodeInfo& bytecodeInfo = GetBytecodeInfo(bbIndex, bcIndex);
        if (HasTypes()) {
            auto pc = bytecodeInfo.GetPC();
            auto type = typeRecorder_.GetType(pcToBCOffset_.at(pc) - 1);
            if (!type.IsAnyType()) {
                gateAcc_.SetGateType(gate, type);
            }
        }
        [[maybe_unused]] size_t numValueInputs = bytecodeInfo.ComputeTotalValueCount();
        [[maybe_unused]] size_t numValueOutputs = bytecodeInfo.ComputeOutCount();
        ASSERT(numValueInputs == valueCount);
        ASSERT(numValueOutputs <= 1);
        auto stateCount = gateAcc_.GetStateCount(gate);
        auto dependCount = gateAcc_.GetDependCount(gate);
        for (size_t valueIdx = 0; valueIdx < valueCount; valueIdx++) {
            auto inIdx = valueIdx + stateCount + dependCount;
            if (!gateAcc_.IsInGateNull(gate, inIdx)) {
                continue;
            }
            if (valueIdx < bytecodeInfo.inputs.size()) {
                auto vregId = std::get<VirtualRegister>(bytecodeInfo.inputs.at(valueIdx)).GetId();
                GateRef defVreg = ResolveDef(bbIndex, bcIndex - 1, vregId, false);
                gateAcc_.NewIn(gate, inIdx, defVreg);
            } else {
                GateRef defAcc = ResolveDef(bbIndex, bcIndex - 1, 0, true);
                gateAcc_.NewIn(gate, inIdx, defAcc);
            }
        }
    }
    if (HasTypes() && IsTypeLoweringEnabled()) {
        frameStateBuilder_.BuildFrameState();
    }

    if (IsLogEnabled()) {
        PrintGraph("Bytecode2Gate");
        LOG_COMPILER(INFO) << "\033[34m" << "============= "
                           << "After bytecode2circuit lowering ["
                           << methodName_ << "]"
                           << " =============" << "\033[0m";
        circuit_.PrintAllGates(*this);
        LOG_COMPILER(INFO) << "\033[34m" << "=========================== End ===========================" << "\033[0m";
    }
}

void BytecodeCircuitBuilder::AddBytecodeOffsetInfo(GateRef &gate, const BytecodeInfo &info, size_t bcOffsetIndex,
                                                   uint8_t *pc)
{
    if (info.IsCall()) {
        auto bcOffset = circuit_.GetConstantGate(MachineType::I64,
                                                 pcToBCOffset_.at(pc),
                                                 GateType::NJSValue());
        gateAcc_.NewIn(gate, bcOffsetIndex, bcOffset);
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
    std::map<const uint8_t *, GateRef> bcToGate;
    for (const auto &[key, value]: jsgateToBytecode_) {
        auto pc = GetBytecodeInfo(value.first, value.second).GetPC();
        bcToGate[pc] = key;
    }

    LOG_COMPILER(INFO) << "======================== " << title << " ========================";
    for (size_t i = 0; i < graph_.size(); i++) {
        BytecodeRegion& bb = graph_[i];
        if (bb.isDead) {
            LOG_COMPILER(INFO) << "B" << bb.id << ":                               ;preds= invalid BB";
            LOG_COMPILER(INFO) << "\tBytecodePC: [" << reinterpret_cast<void*>(bb.start) << ", "
                               << reinterpret_cast<void*>(bb.end) << ")";
            continue;
        }
        std::string log("B" + std::to_string(bb.id) + ":                               ;preds= ");
        for (size_t k = 0; k < bb.preds.size(); ++k) {
            log += std::to_string(bb.preds[k]->id) + ", ";
        }
        LOG_COMPILER(INFO) << log;
        LOG_COMPILER(INFO) << "\tBytecodePC: [" << reinterpret_cast<void*>(bb.start) << ", "
                           << reinterpret_cast<void*>(bb.end) << ")";

        std::string log1("\tSucces: ");
        for (size_t j = 0; j < bb.succs.size(); j++) {
            log1 += std::to_string(bb.succs[j]->id) + ", ";
        }
        LOG_COMPILER(INFO) << log1;

        for (size_t j = 0; j < bb.catchs.size(); j++) {
            LOG_COMPILER(INFO) << "\tcatch [: " << reinterpret_cast<void*>(bb.catchs[j]->start) << ", "
                               << reinterpret_cast<void*>(bb.catchs[j]->end) << ")";
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

        PrintBytecodeInfo(bb, bcToGate);
        LOG_COMPILER(INFO) << "";
    }
}

void BytecodeCircuitBuilder::PrintBytecodeInfo(BytecodeRegion& bb, const std::map<const uint8_t *, GateRef>& bcToGate)
{
    if (bb.isDead) {
        return;
    }
    LOG_COMPILER(INFO) << "\tBytecode[] = ";
    EnumerateBlock(bb, [&](const BytecodeInfo &bytecodeInfo) -> bool {
        auto &iterator = bb.GetBytecodeIterator();
        auto pc = bytecodeInfo.GetPC();
        std::string log;
        log += std::string("\t\t< ") + std::to_string(iterator.Index()) + ": ";
        log += GetEcmaOpcodeStr(static_cast<EcmaOpcode>(*pc)) + ", " + "In=[";
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

        auto r = bcToGate.find(pc);
        if (r != bcToGate.end()) {
            this->gateAcc_.ShortPrint(r->second);
        }
        return true;
    });
}
}  // namespace panda::ecmascript::kungfu
