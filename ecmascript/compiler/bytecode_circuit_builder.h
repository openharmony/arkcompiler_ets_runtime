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

#ifndef ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H
#define ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H

#include <algorithm>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>
#include <variant>

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/ecma_opcode_des.h"
#include "ecmascript/compiler/frame_states.h"
#include "ecmascript/compiler/type_recorder.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/compiler/compiler_log.h"

namespace panda::ecmascript::kungfu {

enum class VisitState : uint8_t {
    UNVISITED,
    PENDING,
    VISITED
};

struct ExceptionItem {
    uint8_t* startPc;
    uint8_t* endPc;
    std::vector<uint8_t*> catchs;

    ExceptionItem(uint8_t* startPc, uint8_t* endPc, std::vector<uint8_t*> catchs)
        : startPc(startPc), endPc(endPc), catchs(catchs) {}
};

using ExceptionInfo = std::vector<ExceptionItem>;

class RegionItem {
public:
    static constexpr uint32_t INVALID_BC_INDEX = static_cast<uint32_t>(-1);
    bool operator<(const RegionItem &rhs) const
    {
        return this->startBcIndex_ < rhs.startBcIndex_;
    }

    bool operator>(const RegionItem &rhs) const
    {
        return this->startBcIndex_ > rhs.startBcIndex_;
    }

    bool operator==(const RegionItem &rhs) const
    {
        return this->startBcIndex_ == rhs.startBcIndex_;
    }

    RegionItem(uint32_t startBcIndex, bool isHeadBlock)
        : startBcIndex_(startBcIndex), isHeadBlock_(isHeadBlock) {}

    uint32_t GetStartBcIndex() const
    {
        return startBcIndex_;
    }

    uint32_t IsHeadBlock() const
    {
        return isHeadBlock_;
    }
private:
    uint32_t startBcIndex_ { INVALID_BC_INDEX };
    bool isHeadBlock_ { false };
    friend class RegionsInfo;
};

struct BytecodeSplitItem {
    BytecodeSplitItem(uint32_t start, uint32_t pred)
        : startBcIndex(start), predBcIndex(pred) {}
    uint32_t startBcIndex { RegionItem::INVALID_BC_INDEX };
    uint32_t predBcIndex { RegionItem::INVALID_BC_INDEX };
};

class RegionsInfo {
public:
    void InsertJump(uint32_t bcIndex, uint32_t predBcIndex, bool isJumpImm)
    {
        InsertBlockItem(bcIndex, false);
        auto fallThrogth = bcIndex - 1; // 1: fall through
        // isJumpImm will not generate fall through
        if (isJumpImm || fallThrogth != predBcIndex) {
            InsertSplitItem(bcIndex, predBcIndex);
        }
    }

    void InsertHead(uint32_t bcIndex)
    {
        InsertBlockItem(bcIndex, true);
    }

    void InsertSplit(uint32_t bcIndex)
    {
        InsertBlockItem(bcIndex, false);
    }

    size_t FindBBIndexByBcIndex(uint32_t bcIndex) const
    {
        auto findFunc = [] (uint32_t value, const RegionItem &item) {
            return value < item.startBcIndex_;
        };
        const auto &it = std::upper_bound(blockItems_.begin(),
            blockItems_.end(), bcIndex, findFunc);
        if (it == blockItems_.end()) {
            return blockItems_.size() - 1; // 1: last bb
        }
        // blockItems_[0]'s value is 0, bcIndex must be: bcIndex > blockItems_.begin()
        return std::distance(blockItems_.begin(), it) - 1; // 1: -1 for bbIndx
    }

    const std::vector<BytecodeSplitItem> &GetSplitItems() const
    {
        return splitItems_;
    }

    const std::set<RegionItem> &GetBlockItems() const
    {
        return blockItems_;
    }
private:
    void InsertBlockItem(uint32_t bcIndex, bool isHeadBlock)
    {
        auto result = blockItems_.insert(RegionItem { bcIndex, isHeadBlock });
        if (!result.second && isHeadBlock) {
            blockItems_.erase(result.first);
            blockItems_.insert(RegionItem { bcIndex, isHeadBlock });
        }
    }

    void InsertSplitItem(uint32_t bcIndex, uint32_t predBcIndex)
    {
        splitItems_.emplace_back(BytecodeSplitItem { bcIndex, predBcIndex });
    }
    std::set<RegionItem> blockItems_ {};
    std::vector<BytecodeSplitItem> splitItems_ {};
};

struct BytecodeRegion {
    size_t id {0};
    uint32_t start {0};
    uint32_t end {0};
    std::vector<BytecodeRegion *> preds {}; // List of predessesor blocks
    std::vector<BytecodeRegion *> succs {}; // List of successors blocks
    std::vector<BytecodeRegion *> trys {}; // List of trys blocks
    std::vector<BytecodeRegion *> catchs {}; // List of catches blocks
    std::vector<BytecodeRegion *> immDomBlocks {}; // List of dominated blocks
    BytecodeRegion *iDominator {nullptr}; // Block that dominates the current block
    std::vector<BytecodeRegion *> domFrontiers {}; // List of dominace frontiers
    std::set<size_t> loopbackBlocks {}; // List of loopback block ids
    bool isDead {false};
    bool phiAcc {false};
    std::set<uint16_t> phi {}; // phi node
    size_t numOfStatePreds {0};
    size_t numOfLoopBacks {0};
    size_t statePredIndex {0};
    size_t forwardIndex {0};
    size_t loopBackIndex {0};
    std::vector<std::tuple<size_t, size_t, bool>> expandedPreds {};
    GateRef stateStart {Circuit::NullGate()};
    GateRef dependStart {Circuit::NullGate()};
    GateRef mergeForwardEdges {Circuit::NullGate()};
    GateRef mergeLoopBackEdges {Circuit::NullGate()};
    GateRef depForward {Circuit::NullGate()};
    GateRef depLoopBack {Circuit::NullGate()};
    std::unordered_map<uint16_t, GateRef> vregToValSelectorGate {}; // corresponding ValueSelector gates of vregs
    GateRef valueSelectorAccGate {Circuit::NullGate()};
    BytecodeIterator bytecodeIterator_ {};

    BytecodeIterator &GetBytecodeIterator() {
        return bytecodeIterator_;
    }

    bool operator <(const BytecodeRegion &target) const
    {
        return id < target.id;
    }

    void SortCatches()
    {
        if (catchs.size() > 1) {
            std::sort(catchs.begin(), catchs.end(), [](BytecodeRegion *first, BytecodeRegion *second) {
                return first->start < second->start;
            });
        }
    }

    void UpdateTryCatchInfoForDeadBlock()
    {
        // Try-Catch infos of dead block should be cleared
        UpdateTryCatchInfo();
        isDead = true;
    }

    void UpdateRedundantTryCatchInfo(bool noThrow)
    {
        // if block which can throw exception has serval catchs block, only the innermost catch block is useful
        if (!noThrow && catchs.size() > 1) {
            size_t innerMostIndex = 1;
            UpdateTryCatchInfo(innerMostIndex);
        }
    }

    void UpdateTryCatchInfoIfNoThrow(bool noThrow)
    {
        // if block has no general insts, try-catch infos of it should be cleared
        if (noThrow && !catchs.empty()) {
            UpdateTryCatchInfo();
        }
    }

private:
    void UpdateTryCatchInfo(size_t index = 0)
    {
        for (auto catchBlock = catchs.begin() + index; catchBlock != catchs.end(); catchBlock++) {
            auto tryBlock = std::find((*catchBlock)->trys.begin(), (*catchBlock)->trys.end(), this);
            if (tryBlock != (*catchBlock)->trys.end()) {
                (*catchBlock)->trys.erase(tryBlock);
            }
            if ((*catchBlock)->trys.size() == 0) {
                (*catchBlock)->isDead = true;
            }
        }
        catchs.erase(catchs.begin() + index, catchs.end());
    }
};

using BytecodeGraph = std::vector<BytecodeRegion>;

class BytecodeCircuitBuilder {
public:
    explicit BytecodeCircuitBuilder(const JSPandaFile *jsPandaFile,
                                    const MethodLiteral *methodLiteral,
                                    const MethodPcInfo &methodPCInfo,
                                    TSManager *tsManager,
                                    Circuit *circuit,
                                    Bytecodes *bytecodes,
                                    bool hasTypes,
                                    bool enableLog,
                                    bool enableTypeLowering,
                                    std::string name,
                                    const CString &recordName)
        : tsManager_(tsManager), circuit_(circuit), file_(jsPandaFile), pf_(jsPandaFile->GetPandaFile()),
          method_(methodLiteral), gateAcc_(circuit), argAcc_(circuit, method_),
          typeRecorder_(jsPandaFile, method_, tsManager, recordName), hasTypes_(hasTypes),
          enableLog_(enableLog), enableTypeLowering_(enableTypeLowering),
          pcOffsets_(methodPCInfo.pcOffsets),
          frameStateBuilder_(this, circuit, methodLiteral), methodName_(name), recordName_(recordName),
          bytecodes_(bytecodes)
    {
    }
    ~BytecodeCircuitBuilder() = default;
    NO_COPY_SEMANTIC(BytecodeCircuitBuilder);
    NO_MOVE_SEMANTIC(BytecodeCircuitBuilder);
    void PUBLIC_API BytecodeToCircuit();
    int32_t GetJumpOffset(uint32_t bcIndex) const;
    void CollectRegionInfo(uint32_t bcIndex);

    [[nodiscard]] Circuit* GetCircuit() const
    {
        return circuit_;
    }

    GateRef GetGateByBcIndex(uint32_t bcIndex) const
    {
        ASSERT(bcIndex < byteCodeToJSGate_.size());
        return byteCodeToJSGate_[bcIndex];
    }

    [[nodiscard]] const MethodLiteral* GetMethod() const
    {
        return method_;
    }

    [[nodiscard]] const JSPandaFile* GetJSPandaFile() const
    {
        return file_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    bool IsTypeLoweringEnabled() const
    {
        return enableTypeLowering_;
    }

    [[nodiscard]] const std::vector<GateRef>& GetAsyncRelatedGates() const
    {
        return suspendAndResumeGates_;
    }

    inline bool HasTypes() const
    {
        return hasTypes_;
    }

    template <class Callback>
    void EnumerateBlock(BytecodeRegion &bb, const Callback &cb)
    {
        auto &iterator = bb.GetBytecodeIterator();
        for (iterator.GotoStart(); !iterator.Done(); ++iterator) {
            auto &bytecodeInfo = iterator.GetBytecodeInfo();
            bool ret = cb(bytecodeInfo);
            if (!ret) {
                break;
            }
        }
    }

    BytecodeRegion &GetBasicBlockById(size_t id)
    {
        ASSERT(id < graph_.size());
        return graph_[id];
    }

    size_t GetBasicBlockCount() const
    {
        return graph_.size();
    }

    size_t GetPcOffset(const uint8_t *pc) const
    {
        return static_cast<size_t>(pc - method_->GetBytecodeArray());
    }

    size_t GetPcOffset(uint32_t bcIndex) const
    {
        const uint8_t* pc = GetPCByIndex(bcIndex);
        return GetPcOffset(pc);
    }

    size_t GetNumberVRegs() const
    {
        return static_cast<size_t>(method_->GetNumberVRegs());
    }

    size_t GetNumberVRegsWithEnv() const
    {
        return GetNumberVRegs() + 1; // 1: env variable
    }

    size_t GetEnvVregIdx() const
    {
        return GetNumberVRegs();
    }

    Bytecodes *GetBytecodes() const
    {
        return bytecodes_;
    }

    uint32_t GetLastBcIndex() const
    {
        return static_cast<uint32_t>(pcOffsets_.size() - 1);
    }

    const uint8_t *GetPCByIndex(uint32_t index) const
    {
        ASSERT(index <= GetLastBcIndex());
        return pcOffsets_[index];
    }

    const uint8_t *GetFirstPC() const
    {
        return GetPCByIndex(0);
    }

    const uint8_t *GetLastPC() const
    {
        return GetPCByIndex(GetLastBcIndex());
    }

    uint32_t FindBcIndexByPc(const uint8_t *pc) const
    {
        auto begin = pcOffsets_.begin();
        auto end = pcOffsets_.end();
        auto it = std::lower_bound(begin, end, pc);
        ASSERT(it != end);
        ASSERT(*it == pc);
        return std::distance(begin, it);
    }

    const BytecodeInfo &GetBytecodeInfo(uint32_t index) const
    {
        return infoData_[index];
    }

    bool HasTryCatch() const
    {
        return hasTryCatch_;
    }

private:
    void CollectTryCatchBlockInfo(ExceptionInfo &Exception);
    void BuildCatchBlocks(const ExceptionInfo &Exception);
    void BuildRegions(const ExceptionInfo &Exception);
    void ComputeDominatorTree();
    void BuildImmediateDominator(const std::vector<size_t> &immDom);
    void ComputeDomFrontiers(const std::vector<size_t> &immDom);
    void RemoveDeadRegions(const std::unordered_map<size_t, size_t> &bbIdToDfsTimestamp);
    void InsertPhi();
    void InsertExceptionPhi(std::unordered_map<uint16_t, std::set<size_t>> &defsitesInfo);
    void UpdateCFG();
    bool ShouldBeDead(BytecodeRegion &curBlock);
    // build circuit
    void BuildCircuitArgs();
    void CollectPredsInfo();
    void NewMerge(GateRef &state, GateRef &depend, size_t numOfIns);
    void NewLoopBegin(BytecodeRegion &bb);
    void BuildBlockCircuitHead();
    std::vector<GateRef> CreateGateInList(const BytecodeInfo &info, const GateMetaData *meta);
    void SetBlockPred(BytecodeRegion &bbNext, const GateRef &state, const GateRef &depend, bool isLoopBack);
    GateRef NewConst(const BytecodeInfo &info);
    void NewJSGate(BytecodeRegion &bb, GateRef &state, GateRef &depend);
    void NewJump(BytecodeRegion &bb, GateRef &state, GateRef &depend);
    void NewReturn(BytecodeRegion &bb,  GateRef &state, GateRef &depend);
    void NewByteCode(BytecodeRegion &bb, GateRef &state, GateRef &depend);
    void BuildSubCircuit();
    void NewPhi(BytecodeRegion &bb, uint16_t reg, bool acc, GateRef &currentPhi);
    GateRef ResolveDef(const size_t bbId, int32_t bcId, const uint16_t reg, const bool acc);
    void BuildCircuit();
    GateRef GetExistingRestore(GateRef resumeGate, uint16_t tmpReg) const;
    void SetExistingRestore(GateRef resumeGate, uint16_t tmpReg, GateRef restoreGate);
    void PrintGraph();
    void PrintBBInfo();
    void PrintGraph(const char* title);
    void PrintBytecodeInfo(BytecodeRegion& region);
    void PrintDefsitesInfo(const std::unordered_map<uint16_t, std::set<size_t>> &defsitesInfo);
    void BuildRegionInfo();

    inline bool IsEntryBlock(const size_t bbId) const
    {
        return bbId == 0;
    }

    inline bool IsFirstBCEnvIn(const size_t bbId, const size_t bcIndex, const uint16_t reg) const
    {
        return (IsEntryBlock(bbId) && bcIndex == 0 && reg == GetNumberVRegs());
    }

    TSManager *tsManager_;
    Circuit *circuit_;
    std::vector<GateRef> byteCodeToJSGate_;
    BytecodeGraph graph_;
    const JSPandaFile *file_ {nullptr};
    const panda_file::File *pf_ {nullptr};
    const MethodLiteral *method_ {nullptr};
    GateAccessor gateAcc_;
    ArgumentAccessor argAcc_;
    TypeRecorder typeRecorder_;
    bool hasTypes_ {false};
    bool enableLog_ {false};
    bool enableTypeLowering_ {false};
    std::vector<GateRef> suspendAndResumeGates_ {};
    std::vector<const uint8_t*> pcOffsets_;
    std::map<std::pair<kungfu::GateRef, uint16_t>, kungfu::GateRef> resumeRegToRestore_ {};
    FrameStateBuilder frameStateBuilder_;
    std::string methodName_;
    const CString &recordName_;
    Bytecodes *bytecodes_;
    RegionsInfo regionsInfo_{};
    std::vector<BytecodeInfo> infoData_ {};
    bool hasTryCatch_ {false};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H
