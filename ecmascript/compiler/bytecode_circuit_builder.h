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

namespace panda::ecmascript::kungfu {
enum class SplitKind : uint8_t {
    DEFAULT,
    START,
    END
};

enum class VisitState : uint8_t {
    UNVISITED,
    PENDING,
    VISITED
};

struct CfgInfo {
    uint8_t *pc {nullptr};
    SplitKind splitKind {SplitKind::DEFAULT};
    std::vector<uint8_t *> succs {};
    CfgInfo(uint8_t *startOrEndPc, SplitKind kind, std::vector<uint8_t *> successors)
        : pc(startOrEndPc), splitKind(kind), succs(successors) {}

    bool operator<(const CfgInfo &rhs) const
    {
        if (this->pc != rhs.pc) {
            return this->pc < rhs.pc;
        } else {
            return this->splitKind < rhs.splitKind;
        }
    }

    bool operator==(const CfgInfo &rhs) const
    {
        return this->pc == rhs.pc && this->splitKind == rhs.splitKind;
    }
};

struct BytecodeRegion {
    size_t id {0};
    uint8_t *start {nullptr};
    uint8_t *end {nullptr};
    std::vector<BytecodeRegion *> preds {}; // List of predessesor blocks
    std::vector<BytecodeRegion *> succs {}; // List of successors blocks
    std::vector<BytecodeRegion *> trys {}; // List of trys blocks
    std::vector<BytecodeRegion *> catchs {}; // List of catches blocks
    std::vector<BytecodeRegion *> immDomBlocks {}; // List of dominated blocks
    BytecodeRegion *iDominator {nullptr}; // Block that dominates the current block
    std::vector<BytecodeRegion *> domFrontiers {}; // List of dominace frontiers
    std::set<size_t> loopbackBlocks {}; // List of loopback block ids
    bool isDead {false};
    std::set<uint16_t> phi {}; // phi node
    bool phiAcc {false};
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
    std::map<uint16_t, GateRef> vregToValSelectorGate {}; // corresponding ValueSelector gates of vregs
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
                                    MethodPcInfo &methodPCInfo,
                                    TSManager *tsManager,
                                    Bytecodes *bytecodes,
                                    const CompilationConfig* cconfig,
                                    bool hasTypes,
                                    bool enableLog,
                                    bool enableTypeLowering,
                                    std::string name,
                                    const CString &recordName)
        : tsManager_(tsManager), circuit_(cconfig->Is64Bit()), file_(jsPandaFile), pf_(jsPandaFile->GetPandaFile()),
          method_(methodLiteral), gateAcc_(&circuit_), argAcc_(&circuit_, method_),
          typeRecorder_(jsPandaFile, method_, tsManager), hasTypes_(hasTypes),
          enableLog_(enableLog), enableTypeLowering_(enableTypeLowering), pcToBCOffset_(methodPCInfo.pcToBCOffset),
          byteCodeCurPrePc_(methodPCInfo.byteCodeCurPrePc), bytecodeBlockInfos_(methodPCInfo.bytecodeBlockInfos),
          frameStateBuilder_(this, &circuit_, methodLiteral), methodName_(name), recordName_(recordName),
          bytecodes_(bytecodes)
    {
    }
    ~BytecodeCircuitBuilder() = default;
    NO_COPY_SEMANTIC(BytecodeCircuitBuilder);
    NO_MOVE_SEMANTIC(BytecodeCircuitBuilder);
    void PUBLIC_API BytecodeToCircuit();
    static void PUBLIC_API CollectBytecodeBlockInfo(uint8_t *pc, std::vector<CfgInfo> &bytecodeBlockInfos);

    [[nodiscard]] Circuit* GetCircuit()
    {
        return &circuit_;
    }

    [[nodiscard]] const std::map<GateRef, std::pair<size_t, size_t>>& GetGateToBytecode() const
    {
        return jsgateToBytecode_;
    }

    [[nodiscard]] const std::map<const uint8_t *, GateRef>& GetBytecodeToGate() const
    {
        return byteCodeToJSGate_;
    }

    [[nodiscard]] EcmaOpcode PcToOpcode(const uint8_t *pc) const
    {
        return Bytecodes::GetOpcode(pc);
    }

    [[nodiscard]] const uint8_t* GetJSBytecode(GateRef gate)
    {
        return GetByteCodeInfo(gate).GetPC();
    }

    [[nodiscard]] EcmaOpcode GetByteCodeOpcode(GateRef gate)
    {
        auto pc = GetJSBytecode(gate);
        return PcToOpcode(pc);
    }

    [[nodiscard]] std::string GetBytecodeStr(GateRef gate)
    {
        auto opcode = GetByteCodeOpcode(gate);
        return GetEcmaOpcodeStr(opcode);
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


    const BytecodeInfo &GetBytecodeInfo(size_t bbIndex, size_t bcIndex)
    {
        auto &bb = graph_[bbIndex];
        auto &iterator = bb.GetBytecodeIterator();
        iterator.Goto(static_cast<int32_t>(bcIndex));
        return iterator.GetBytecodeInfo();
    }
    // for external users, circuit must be built
    const BytecodeInfo &GetByteCodeInfo(const GateRef gate)
    {
        const auto &[bbIndex, bcIndex] = jsgateToBytecode_.at(gate);
        return GetBytecodeInfo(bbIndex, bcIndex);
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

    const std::map<const uint8_t *, int32_t> &GetPcToBCOffset() const
    {
        return pcToBCOffset_;
    }

    BytecodeRegion &GetBasicBlockById(size_t id)
    {
        return graph_[id];
    }

    size_t GetBasicBlockCount() const
    {
        return graph_.size();
    }

    size_t GetPcOffset(const uint8_t* pc) const
    {
        return static_cast<size_t>(pc - method_->GetBytecodeArray());
    }

    size_t GetNumberVRegs() const
    {
        return static_cast<size_t>(method_->GetNumberVRegs());
    }

    Bytecodes *GetBytecodes() const
    {
        return bytecodes_;
    }
private:
    void CollectTryCatchBlockInfo(std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &Exception);
    void CompleteBytecodeBlockInfo();
    void BuildBasicBlocks(std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &Exception);
    void ComputeDominatorTree();
    void BuildImmediateDominator(const std::vector<size_t> &immDom);
    void ComputeDomFrontiers(const std::vector<size_t> &immDom);
    void RemoveDeadRegions(const std::map<size_t, size_t> &bbIdToDfsTimestamp);
    void InsertPhi();
    void InsertExceptionPhi(std::map<uint16_t, std::set<size_t>> &defsitesInfo);
    void UpdateCFG();
    bool ShouldBeDead(BytecodeRegion &curBlock);
    // build circuit
    void BuildCircuitArgs();
    void CollectPredsInfo();
    void NewMerge(GateRef &state, GateRef &depend, size_t numOfIns);
    void NewLoopBegin(BytecodeRegion &bb);
    void BuildBlockCircuitHead();
    std::vector<GateRef> CreateGateInList(const BytecodeInfo &info);
    void SetBlockPred(BytecodeRegion &bbNext, const GateRef &state, const GateRef &depend, bool isLoopBack);
    GateRef NewConst(const BytecodeInfo &info);
    void NewJSGate(BytecodeRegion &bb, GateRef &state, GateRef &depend);
    void NewJump(BytecodeRegion &bb, GateRef &state, GateRef &depend);
    void NewReturn(BytecodeRegion &bb,  GateRef &state, GateRef &depend);
    void NewByteCode(BytecodeRegion &bb, GateRef &state, GateRef &depend);
    void AddBytecodeOffsetInfo(GateRef &gate, const BytecodeInfo &info, size_t bcOffsetIndex, uint8_t *pc);
    void BuildSubCircuit();
    void NewPhi(BytecodeRegion &bb, uint16_t reg, bool acc, GateRef &currentPhi);
    GateRef ResolveDef(const size_t bbId, int32_t bcId, const uint16_t reg, const bool acc);
    void BuildCircuit();
    GateRef GetExistingRestore(GateRef resumeGate, uint16_t tmpReg) const;
    void SetExistingRestore(GateRef resumeGate, uint16_t tmpReg, GateRef restoreGate);
    void PrintGraph();
    void PrintBBInfo();
    void PrintGraph(const char* title);
    void PrintBytecodeInfo(BytecodeRegion& region, const std::map<const uint8_t *, GateRef>& bcToGate);
    void PrintDefsitesInfo(const std::map<uint16_t, std::set<size_t>> &defsitesInfo);

    inline bool IsEntryBlock(const size_t bbId) const
    {
        return bbId == 0;
    }

    TSManager *tsManager_;
    Circuit circuit_;
    std::map<GateRef, std::pair<size_t, size_t>> jsgateToBytecode_;
    std::map<const uint8_t *, GateRef> byteCodeToJSGate_;
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
    const std::map<const uint8_t *, int32_t> &pcToBCOffset_;
    const std::map<uint8_t *, uint8_t *> &byteCodeCurPrePc_;
    std::vector<CfgInfo> &bytecodeBlockInfos_;
    std::map<std::pair<kungfu::GateRef, uint16_t>, kungfu::GateRef> resumeRegToRestore_;
    FrameStateBuilder frameStateBuilder_;
    std::string methodName_;
    const CString &recordName_;
    Bytecodes *bytecodes_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H
