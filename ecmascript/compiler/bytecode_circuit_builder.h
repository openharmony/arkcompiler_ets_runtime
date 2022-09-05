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
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/emca_bytecode.h"
#include "ecmascript/compiler/type_recorder.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/method_literal.h"

namespace panda::ecmascript::kungfu {
using VRegIDType = uint16_t;
using ImmValueType = uint64_t;
using StringIdType = uint32_t;
using MethodIdType = uint16_t;

class VirtualRegister {
public:
    explicit VirtualRegister(VRegIDType id) : id_(id)
    {
    }
    ~VirtualRegister() = default;

    void SetId(VRegIDType id)
    {
        id_ = id;
    }

    VRegIDType GetId() const
    {
        return id_;
    }

private:
    VRegIDType id_;
};

class Immediate {
public:
    explicit Immediate(ImmValueType value) : value_(value)
    {
    }
    ~Immediate() = default;

    void SetValue(ImmValueType value)
    {
        value_ = value;
    }

    ImmValueType ToJSTaggedValueInt() const
    {
        return value_ | JSTaggedValue::TAG_INT;
    }

    ImmValueType ToJSTaggedValueDouble() const
    {
        return JSTaggedValue(bit_cast<double>(value_)).GetRawData();
    }

    ImmValueType GetValue() const
    {
        return value_;
    }

private:
    ImmValueType value_;
};

class StringId {
public:
    explicit StringId(StringIdType id) : id_(id)
    {
    }
    ~StringId() = default;

    void SetId(StringIdType id)
    {
        id_ = id;
    }

    StringIdType GetId() const
    {
        return id_;
    }

private:
    StringIdType id_;
};

class MethodId {
public:
    explicit MethodId(MethodIdType id) : id_(id)
    {
    }
    ~MethodId() = default;

    void SetId(MethodIdType id)
    {
        id_ = id;
    }

    MethodIdType GetId() const
    {
        return id_;
    }

private:
    MethodIdType id_;
};

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
    std::vector<std::tuple<size_t, const uint8_t *, bool>> expandedPreds {};
    kungfu::GateRef stateStart {kungfu::Circuit::NullGate()};
    kungfu::GateRef dependStart {kungfu::Circuit::NullGate()};
    kungfu::GateRef mergeForwardEdges {kungfu::Circuit::NullGate()};
    kungfu::GateRef mergeLoopBackEdges {kungfu::Circuit::NullGate()};
    kungfu::GateRef depForward {kungfu::Circuit::NullGate()};
    kungfu::GateRef depLoopBack {kungfu::Circuit::NullGate()};
    std::map<uint16_t, kungfu::GateRef> vregToValSelectorGate {}; // corresponding ValueSelector gates of vregs
    kungfu::GateRef valueSelectorAccGate {kungfu::Circuit::NullGate()};

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

struct BytecodeInfo {
    // set of id, immediate and read register
    std::vector<std::variant<StringId, MethodId, Immediate, VirtualRegister>> inputs {};
    std::vector<VRegIDType> vregOut {}; // write register
    bool accIn {false}; // read acc
    bool accOut {false}; // write acc
    EcmaBytecode opcode { EcmaBytecode::NOP };
    uint16_t offset {0};

    bool IsOut(VRegIDType reg, uint32_t index) const
    {
        bool isDefined = (!vregOut.empty() && (reg == vregOut.at(index)));
        return isDefined;
    }

    bool IsMov() const
    {
        switch (opcode) {
            case EcmaBytecode::MOV:
            case EcmaBytecode::LDA:
            case EcmaBytecode::STA:
                return true;
            default:
                return false;
        }
    }

    bool IsJump() const
    {
        switch (opcode) {
            case EcmaBytecode::JMP:
            case EcmaBytecode::JEQZ:
            case EcmaBytecode::JNEZ:
                return true;
            default:
                return false;
        }
    }

    bool IsCondJump() const
    {
        switch (opcode) {
            case EcmaBytecode::JEQZ:
            case EcmaBytecode::JNEZ:
                return true;
            default:
                return false;
        }
    }

    bool IsReturn() const
    {
        switch (opcode) {
            case EcmaBytecode::RETURN:
            case EcmaBytecode::RETURNUNDEFINED:
                return true;
            default:
                return false;
        }
    }

    bool IsThrow() const
    {
        switch (opcode) {
            case EcmaBytecode::THROW:
            case EcmaBytecode::THROW_NOTEXISTS:
            case EcmaBytecode::THROW_PATTERNNONCOERCIBLE:
            case EcmaBytecode::THROW_DELETESUPERPROPERTY:
            case EcmaBytecode::THROW_CONSTASSIGNMENT:
                return true;
            default:
                return false;
        }
    }

    bool IsDiscarded() const
    {
        switch (opcode) {
            case EcmaBytecode::DEBUGGER:
            case EcmaBytecode::NOP:
                return true;
            default:
                return false;
        }
    }

    bool IsSetConstant() const
    {
        switch (opcode) {
            case EcmaBytecode::LDNAN:
            case EcmaBytecode::LDINFINITY:
            case EcmaBytecode::LDUNDEFINED:
            case EcmaBytecode::LDNULL:
            case EcmaBytecode::LDTRUE:
            case EcmaBytecode::LDFALSE:
            case EcmaBytecode::LDHOLE:
            case EcmaBytecode::LDAI:
            case EcmaBytecode::FLDAI:
            case EcmaBytecode::LDFUNCTION:
                return true;
            default:
                return false;
        }
    }

    bool IsGeneral() const
    {
        return !IsMov() && !IsJump() && !IsReturn() && !IsSetConstant() && !IsDiscarded();
    }

    bool IsCall() const
    {
        switch (opcode) {
            case EcmaBytecode::CALLARG0:
            case EcmaBytecode::CALLARG1:
            case EcmaBytecode::CALLARGS2:
            case EcmaBytecode::CALLARGS3:
            case EcmaBytecode::CALLTHISRANGE:
            case EcmaBytecode::CALLRANGE:
                return true;
            default:
                return false;
        }
    }

    size_t ComputeBCOffsetInputCount() const
    {
        return IsCall() ? 1 : 0;
    }

    size_t ComputeValueInputCount() const
    {
        return (accIn ? 1 : 0) + inputs.size();
    }

    size_t ComputeOutCount() const
    {
        return accOut ? 1 : 0;
    }

    size_t ComputeTotalValueCount() const
    {
        return ComputeValueInputCount() + ComputeBCOffsetInputCount();
    }

    bool IsGeneratorRelative() const
    {
        switch (opcode) {
            case EcmaBytecode::SUSPENDGENERATOR:
            case EcmaBytecode::RESUMEGENERATOR:
                return true;
            default:
                return false;
        }
    }

    bool IsBc(EcmaBytecode other) const
    {
        return opcode == other;
    }
};

class BytecodeCircuitBuilder {
public:
    explicit BytecodeCircuitBuilder(const JSPandaFile *jsPandaFile,
                                    JSHandle<JSTaggedValue> &constantPool,
                                    const MethodLiteral *methodLiteral,
                                    BytecodeInfoCollector::MethodPcInfo &methodPCInfo,
                                    TSManager *tsManager,
                                    const CompilationConfig* cconfig,
                                    bool enableLog)
        : circuit_(cconfig->Is64Bit()), file_(jsPandaFile), pf_(jsPandaFile->GetPandaFile()), method_(methodLiteral),
          constantPool_(constantPool), gateAcc_(&circuit_), argAcc_(&circuit_, method_, jsPandaFile),
          typeRecorder_(jsPandaFile, method_, tsManager), hasTypes_(file_->HasTSTypes()),
          enableLog_(enableLog), pcToBCOffset_(methodPCInfo.pcToBCOffset),
          byteCodeCurPrePc_(methodPCInfo.byteCodeCurPrePc), bytecodeBlockInfos_(methodPCInfo.bytecodeBlockInfos)
    {
    }
    ~BytecodeCircuitBuilder() = default;
    NO_COPY_SEMANTIC(BytecodeCircuitBuilder);
    NO_MOVE_SEMANTIC(BytecodeCircuitBuilder);
    void PUBLIC_API BytecodeToCircuit();
    static void PUBLIC_API CollectBytecodeBlockInfo(uint8_t *pc, std::vector<CfgInfo> &bytecodeBlockInfos);

    [[nodiscard]] kungfu::Circuit* GetCircuit()
    {
        return &circuit_;
    }

    [[nodiscard]] const std::map<kungfu::GateRef, std::pair<size_t, const uint8_t *>>& GetGateToBytecode() const
    {
        return jsgateToBytecode_;
    }

    [[nodiscard]] const std::map<const uint8_t *, kungfu::GateRef>& GetBytecodeToGate() const
    {
        return byteCodeToJSGate_;
    }

    [[nodiscard]] std::string GetBytecodeStr(kungfu::GateRef gate) const
    {
        return GetEcmaBytecodeStr(GetByteCodeOpcode(gate));
    }

    [[nodiscard]] EcmaBytecode GetByteCodeOpcode(kungfu::GateRef gate) const
    {
        auto pc = jsgateToBytecode_.at(gate).second;
        return static_cast<EcmaBytecode>(*pc);
    }

    [[nodiscard]] const uint8_t* GetJSBytecode(GateRef gate) const
    {
        return jsgateToBytecode_.at(gate).second;
    }

    [[nodiscard]] const MethodLiteral* GetMethod() const
    {
        return method_;
    }

    [[nodiscard]] const JSPandaFile* GetJSPandaFile() const
    {
        return file_;
    }

    BytecodeInfo GetBytecodeInfo(const uint8_t *pc);
    // for external users, circuit must be built
    BytecodeInfo GetByteCodeInfo(const GateRef gate)
    {
        auto pc = jsgateToBytecode_.at(gate).second;
        return GetBytecodeInfo(pc);
    }

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    [[nodiscard]] const std::vector<kungfu::GateRef>& GetAsyncRelatedGates() const
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
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto bytecodeInfo = GetBytecodeInfo(pc);
            bool ret = cb(pc, bytecodeInfo);
            if (!ret) {
                break;
            }
            pc += bytecodeInfo.offset;
        }
    }

    JSHandle<JSTaggedValue> GetConstantPool() const
    {
        return constantPool_;
    }

private:
    void CollectTryCatchBlockInfo(std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &Exception);
    void CompleteBytecodeBlockInfo();
    void BuildBasicBlocks(std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &Exception);
    void ComputeDominatorTree();
    void BuildImmediateDominator(const std::vector<size_t> &immDom);
    void ComputeDomFrontiers(const std::vector<size_t> &immDom);
    void RemoveDeadRegions(const std::map<size_t, size_t> &dfsTimestamp);
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
    void NewJSGate(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void NewJump(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void NewReturn(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void NewByteCode(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void AddBytecodeOffsetInfo(GateRef &gate, const BytecodeInfo &info, size_t bcOffsetIndex, uint8_t *pc);
    void BuildSubCircuit();
    void NewPhi(BytecodeRegion &bb, uint16_t reg, bool acc, GateRef &currentPhi);
    GateRef RenameVariable(const size_t bbId, const uint8_t *end, const uint16_t reg, const bool acc);
    void BuildCircuit();
    void PrintCollectBlockInfo(std::vector<CfgInfo> &bytecodeBlockInfos);
    void PrintGraph();
    void PrintBytecodeInfo();
    void PrintBBInfo();
    void PrintDefsitesInfo(const std::map<uint16_t, std::set<size_t>> &defsitesInfo);

    inline bool IsEntryBlock(const size_t bbId) const
    {
        return bbId == 0;
    }

    kungfu::Circuit circuit_;
    std::map<kungfu::GateRef, std::pair<size_t, const uint8_t *>> jsgateToBytecode_;
    std::map<const uint8_t *, kungfu::GateRef> byteCodeToJSGate_;
    BytecodeGraph graph_;
    const JSPandaFile *file_ {nullptr};
    const panda_file::File *pf_ {nullptr};
    const MethodLiteral *method_ {nullptr};
    JSHandle<JSTaggedValue> constantPool_;
    GateAccessor gateAcc_;
    ArgumentAccessor argAcc_;
    TypeRecorder typeRecorder_;
    bool hasTypes_ {false};
    bool enableLog_ {false};
    std::vector<kungfu::GateRef> suspendAndResumeGates_ {};
    const std::map<const uint8_t *, int32_t> &pcToBCOffset_;
    const std::map<uint8_t *, uint8_t *> &byteCodeCurPrePc_;
    std::vector<CfgInfo> &bytecodeBlockInfos_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H
