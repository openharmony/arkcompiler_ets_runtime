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
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/ecma_opcode_des.h"
#include "ecmascript/compiler/frame_states.h"
#include "ecmascript/compiler/loop_analysis.h"
#include "ecmascript/compiler/type_recorder.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"

namespace panda::ecmascript::kungfu {
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
            return blockItems_.size();
        }
        // blockItems_[0]'s value is 0, bcIndex must be: bcIndex > blockItems_.begin()
        return std::distance(blockItems_.begin(), it);
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
    size_t loopDepth {0};
    std::set<uint16_t> phi {}; // phi node
    std::set<GateRef> phiGate {}; // phi gate
    size_t numOfStatePreds {0};
    size_t numOfLoopBacks {0};
    size_t statePredIndex {0};
    size_t forwardIndex {0};
    size_t loopBackIndex {0};
    std::vector<std::tuple<size_t, size_t, bool>> expandedPreds {};
    GateRef loopExitState {Circuit::NullGate()};
    GateRef loopExitDepend {Circuit::NullGate()};
    GateRef stateCurrent {Circuit::NullGate()};
    GateRef dependCurrent {Circuit::NullGate()};
    GateRef stateMerge {Circuit::NullGate()};
    GateRef dependMerge {Circuit::NullGate()};
    GateRef loopBackStateMerge {Circuit::NullGate()};
    GateRef loopBackDependMerge {Circuit::NullGate()};
    std::unordered_map<uint16_t, GateRef> vregToValueGate {}; // corresponding value gates of vregs
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
    BytecodeCircuitBuilder(const JSPandaFile *jsPandaFile,
                           const MethodLiteral *methodLiteral,
                           const MethodPcInfo &methodPCInfo,
                           TSManager *tsManager,
                           Circuit *circuit,
                           Bytecodes *bytecodes,
                           bool hasTypes,
                           bool enableLog,
                           bool enableTypeLowering,
                           std::string name,
                           const CString &recordName,
                           PGOProfilerDecoder *decoder,
                           bool isInline,
                           bool enableOptTrackField)
        : tsManager_(tsManager), circuit_(circuit), file_(jsPandaFile),
          method_(methodLiteral), gateAcc_(circuit), argAcc_(circuit, method_),
          typeRecorder_(jsPandaFile, method_, tsManager, recordName, decoder, methodPCInfo, bytecodes,
            enableOptTrackField),
          hasTypes_(hasTypes), enableLog_(enableLog), enableTypeLowering_(enableTypeLowering),
          pcOffsets_(methodPCInfo.pcOffsets),
          frameStateBuilder_(this, circuit, methodLiteral),
          methodName_(name), recordName_(recordName),
          bytecodes_(bytecodes),
          dfsList_(circuit->chunk()),
          loopExitToVregGate_(circuit->chunk()),
          loopExitToAccGate_(circuit->chunk()),
          preFrameState_(circuit_->GetRoot()),
          isInline_(isInline)
    {
    }
    ~BytecodeCircuitBuilder() = default;
    NO_COPY_SEMANTIC(BytecodeCircuitBuilder);
    NO_MOVE_SEMANTIC(BytecodeCircuitBuilder);
    void PUBLIC_API BytecodeToCircuit();
    void CollectRegionInfo(uint32_t bcIndex);
    GateRef ResolveDef(const size_t bbId, int32_t bcId, const uint16_t reg, const bool acc, bool needIter = true);
    GateRef ResolveDef(const BytecodeRegion &bb, int32_t bcId, const uint16_t reg, const bool acc);

    [[nodiscard]] Circuit* GetCircuit() const
    {
        return circuit_;
    }

    GateRef GetGateByBcIndex(uint32_t bcIndex) const
    {
        ASSERT(bcIndex < byteCodeToJSGates_.size());
        if (byteCodeToJSGates_[bcIndex].size() > 0) {
            ASSERT(byteCodeToJSGates_[bcIndex].size() == 1);
            return byteCodeToJSGates_[bcIndex].at(0);
        }
        return Circuit::NullGate();
    }

    const std::vector<GateRef>& GetGatesByBcIndex(uint32_t bcIndex) const
    {
        ASSERT(bcIndex < byteCodeToJSGates_.size());
        return byteCodeToJSGates_[bcIndex];
    }

    uint32_t GetBcIndexByGate(GateRef gate) const
    {
        return jsGatesToByteCode_.at(gate);
    }

    bool NeedCheckSafePointAndStackOver() const
    {
        return !isInline_ && !method_->IsNoGC();
    }

    void UpdateBcIndexGate(GateRef gate, uint32_t bcIndex)
    {
        ASSERT(gateAcc_.GetOpCode(gate) == OpCode::JS_BYTECODE);
        ASSERT(bcIndex < byteCodeToJSGates_.size());
        byteCodeToJSGates_[bcIndex].emplace_back(gate);
        jsGatesToByteCode_[gate] = bcIndex;
    }

    const std::vector<std::pair<size_t, GateRef>>& GetLoopHeads() const
    {
        return loopHeads_;
    }

    [[nodiscard]] const MethodLiteral* GetMethod() const
    {
        return method_;
    }

    [[nodiscard]] const JSPandaFile *GetJSPandaFile() const
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

    void UpdateAsyncRelatedGate(GateRef gate)
    {
        suspendAndResumeGates_.emplace_back(gate);
    };

    inline bool HasTypes() const
    {
        return hasTypes_;
    }

    template <class Callback>
    void EnumerateBlock(BytecodeRegion &bb, const Callback &cb)
    {
        // Entry block is a empty block
        if (IsEntryBlock(bb.id)) {
            return;
        }
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

    size_t GetPcOffsetByGate(GateRef gate) const
    {
        return GetPcOffset(jsGatesToByteCode_.at(gate));
    }

    PGORWOpType GetPGOType(GateRef gate) const
    {
        return typeRecorder_.GetRwOpType(GetPcOffsetByGate(gate));
    }

    ElementsKind GetElementsKind(GateRef gate) const
    {
        return typeRecorder_.GetElementsKind(GetPcOffsetByGate(gate));
    }

    ElementsKind GetArrayElementsKind(GateRef gate) const
    {
        auto type = typeRecorder_.GetType(GetBcIndexByGate(gate));
        auto pgoType = typeRecorder_.GetOrUpdatePGOType(tsManager_, gateAcc_.TryGetPcOffset(gate), type);
        return typeRecorder_.GetElementsKind(pgoType);
    }

    bool ShouldPGOTypeInfer(GateRef gate) const
    {
        return jsGatesToByteCode_.find(gate) != jsGatesToByteCode_.end();
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

    bool EnableLoopOptimization() const
    {
        return (!HasTryCatch()) && (loopHeads_.size() != 0);
    }

    size_t LoopExitCount(size_t from, size_t to);

    GateRef GetFrameArgs() const
    {
        return argAcc_.GetFrameArgs();
    }

    GateRef GetPreFrameState() const
    {
        return preFrameState_;
    }

    void SetPreFrameState(GateRef gate)
    {
        preFrameState_ = gate;
    }

    const ChunkVector<size_t>& GetDfsList() const
    {
        return dfsList_;
    }

    inline bool IsEntryBlock(const size_t bbId) const
    {
        return bbId == 0;
    }

    inline bool IsFirstBasicBlock(const size_t bbId) const
    {
        return bbId == 1;
    }

private:
    void CollectTryCatchBlockInfo(ExceptionInfo &Exception);
    void BuildCatchBlocks(const ExceptionInfo &Exception);
    void BuildEntryBlock();
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
    void NewLoopBegin(BytecodeRegion &bb, GateRef &state, GateRef &depend);
    void NewLoopExit(GateRef &state, GateRef &depend);
    void TryInsertLoopExit(BytecodeRegion &bb, BytecodeRegion &bbNext, GateRef &state, GateRef &depend);
    void BuildBlockCircuitHead(BytecodeRegion &bb, GateRef &state, GateRef &depend);
    std::vector<GateRef> CreateGateInList(const BytecodeInfo &info, const GateMetaData *meta);
    void SetBlockPred(BytecodeRegion &bb, BytecodeRegion &bbNext, const GateRef &state, const GateRef &depend);
    void SetLoopBlockPred(BytecodeRegion &bb, BytecodeRegion &bbNext,
                          GateRef &state, GateRef &depend);
    GateRef NewConst(const BytecodeInfo &info);
    void NewJSGate(BytecodeRegion &bb, GateRef &state, GateRef &depend);
    void NewJump(BytecodeRegion &bb, GateRef &state, GateRef &depend);
    void NewReturn(BytecodeRegion &bb,  GateRef &state, GateRef &depend);
    void NewByteCode(BytecodeRegion &bb, GateRef &state, GateRef &depend);
    void BuildSubCircuit();
    void NewPhi(BytecodeRegion &bb, uint16_t reg, bool acc, GateRef &currentPhi);
    GateRef NewLoopBackPhi(BytecodeRegion &bb, uint16_t reg, bool acc);
    GateRef NewLoopForwardPhi(BytecodeRegion &bb, uint16_t reg, bool acc);
    bool IsLoopExitValueExists(GateRef loopExit, uint16_t reg, bool acc);
    GateRef GetLoopExitValue(GateRef loopExit, uint16_t reg, bool acc);
    GateRef CreateLoopExitValue(GateRef loopExit, uint16_t reg, bool acc, GateRef value);
    GateRef NewLoopExitValue(GateRef loopExit, uint16_t reg, bool acc, GateRef value);
    GateRef NewValueFromPredBB(BytecodeRegion &bb, size_t idx, GateRef exit, uint16_t reg, bool acc);

    void BuildCircuit();
    GateRef GetExistingRestore(GateRef resumeGate, uint16_t tmpReg) const;
    void SetExistingRestore(GateRef resumeGate, uint16_t tmpReg, GateRef restoreGate);
    void PrintGraph();
    void PrintBBInfo();
    void PrintGraph(const char* title);
    void PrintBytecodeInfo(BytecodeRegion& region);
    void PrintDefsitesInfo(const std::unordered_map<uint16_t, std::set<size_t>> &defsitesInfo);
    void BuildRegionInfo();
    void BuildFrameArgs();
    void CollectLoopBack();
    void ComputeLoopDepth(size_t loopHead);
    void CountLoopBackEdge(size_t fromId, size_t toId);

    inline bool IsFirstBCEnvIn(const size_t bbId, const size_t bcIndex, const uint16_t reg) const
    {
        return (IsFirstBasicBlock(bbId) && bcIndex == 0 && reg == GetNumberVRegs());
    }

    TSManager *tsManager_;
    Circuit *circuit_;
    std::vector<std::vector<GateRef>> byteCodeToJSGates_;
    std::unordered_map<GateRef, size_t> jsGatesToByteCode_;
    BytecodeGraph graph_;
    const JSPandaFile *file_ {nullptr};
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
    std::vector<std::pair<size_t, GateRef>> loopHeads_;
    size_t loopSize_{0};
    ChunkVector<size_t> dfsList_;
    ChunkMap<std::pair<GateRef, uint16_t>, GateRef> loopExitToVregGate_;
    ChunkMap<GateRef, GateRef> loopExitToAccGate_;
    GateRef preFrameState_ {Circuit::NullGate()};
    bool isInline_ {false};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H
