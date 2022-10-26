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
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/ecma_opcode_des.h"
#include "ecmascript/compiler/frame_states.h"
#include "ecmascript/compiler/type_recorder.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/method_literal.h"

namespace panda::ecmascript::kungfu {
using VRegIDType = uint32_t;
using ICSlotIdType = uint16_t;
using ImmValueType = uint64_t;
using EcmaOpcode = BytecodeInstruction::Opcode;

enum class ConstDataIDType : uint8_t {
    StringIDType,
    MethodIDType,
    ArrayLiteralIDType,
    ObjectLiteralIDType,
    ClassLiteralIDType,
};

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


class ICSlotId {
public:
    explicit ICSlotId(ICSlotIdType id) : id_(id)
    {
    }
    ~ICSlotId() = default;

    void SetId(ICSlotIdType id)
    {
        id_ = id;
    }

    ICSlotIdType GetId() const
    {
        return id_;
    }

private:
    ICSlotIdType id_;
};

class ConstDataId {
public:
    explicit ConstDataId(ConstDataIDType type, uint16_t id)
        :type_(type), id_(id)
    {
    }

    explicit ConstDataId(BitField bitfield)
    {
        type_ = ConstDataIDType(bitfield >> TYPE_SHIFT);
        id_ = bitfield & ((1 << TYPE_SHIFT) - 1);
    }

    ~ConstDataId() = default;

    void SetId(uint16_t id)
    {
        id_ = id;
    }

    uint16_t GetId() const
    {
        return id_;
    }

    void SetType(ConstDataIDType type)
    {
        type_ = type;
    }

    ConstDataIDType GetType() const
    {
        return type_;
    }

    bool IsStringId() const
    {
        return type_ == ConstDataIDType::StringIDType;
    }

    bool IsMethodId() const
    {
        return type_ == ConstDataIDType::MethodIDType;
    }

    bool IsClassLiteraId() const
    {
        return type_ == ConstDataIDType::ClassLiteralIDType;
    }

    bool IsObjectLiteralID() const
    {
        return type_ == ConstDataIDType::ObjectLiteralIDType;
    }

    bool IsArrayLiteralID() const
    {
        return type_ == ConstDataIDType::ArrayLiteralIDType;
    }

    BitField CaculateBitField() const
    {
        return (static_cast<uint8_t>(type_) << TYPE_SHIFT) | id_;
    }

private:
    static constexpr int TYPE_SHIFT = 16;
    ConstDataIDType type_;
    uint16_t id_;
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
    GateRef stateStart {Circuit::NullGate()};
    GateRef dependStart {Circuit::NullGate()};
    GateRef mergeForwardEdges {Circuit::NullGate()};
    GateRef mergeLoopBackEdges {Circuit::NullGate()};
    GateRef depForward {Circuit::NullGate()};
    GateRef depLoopBack {Circuit::NullGate()};
    std::map<uint16_t, GateRef> vregToValSelectorGate {}; // corresponding ValueSelector gates of vregs
    GateRef valueSelectorAccGate {Circuit::NullGate()};

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
    std::vector<std::variant<ConstDataId, ICSlotId, Immediate, VirtualRegister>> inputs {};
    std::vector<VRegIDType> vregOut {}; // write register
    bool accIn {false}; // read acc
    bool accOut {false}; // write acc
    bool deopt {false}; // may trigger deopt
    EcmaOpcode opcode {0};
    uint16_t offset {0};
    uint32_t pcOffset {0};

    bool Deopt() const
    {
        return deopt;
    }

    bool IsDef() const
    {
        return (!vregOut.empty()) || accOut;
    }

    bool IsOut(VRegIDType reg, uint32_t index) const
    {
        bool isDefined = (!vregOut.empty() && (reg == vregOut.at(index)));
        return isDefined;
    }

    bool IsMov() const
    {
        switch (opcode) {
            case EcmaOpcode::MOV_V4_V4:
            case EcmaOpcode::MOV_V8_V8:
            case EcmaOpcode::MOV_V16_V16:
            case EcmaOpcode::LDA_V8:
            case EcmaOpcode::STA_V8:
                return true;
            default:
                return false;
        }
    }

    bool IsJump() const
    {
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
                return true;
            default:
                return false;
        }
    }

    bool IsCondJump() const
    {
        switch (opcode) {
            case EcmaOpcode::JEQZ_IMM8:
            case EcmaOpcode::JEQZ_IMM16:
            case EcmaOpcode::JEQZ_IMM32:
            case EcmaOpcode::JNEZ_IMM8:
            case EcmaOpcode::JNEZ_IMM16:
            case EcmaOpcode::JNEZ_IMM32:
                return true;
            default:
                return false;
        }
    }

    bool IsReturn() const
    {
        switch (opcode) {
            case EcmaOpcode::RETURN:
            case EcmaOpcode::RETURNUNDEFINED:
                return true;
            default:
                return false;
        }
    }

    bool IsThrow() const
    {
        switch (opcode) {
            case EcmaOpcode::THROW_PREF_NONE:
            case EcmaOpcode::THROW_NOTEXISTS_PREF_NONE:
            case EcmaOpcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
            case EcmaOpcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
            case EcmaOpcode::THROW_CONSTASSIGNMENT_PREF_V8:
                return true;
            default:
                return false;
        }
    }

    bool IsDiscarded() const
    {
        switch (opcode) {
            case EcmaOpcode::DEBUGGER:
                return true;
            default:
                return false;
        }
    }

    bool IsSetConstant() const
    {
        switch (opcode) {
            case EcmaOpcode::LDNAN:
            case EcmaOpcode::LDINFINITY:
            case EcmaOpcode::LDUNDEFINED:
            case EcmaOpcode::LDNULL:
            case EcmaOpcode::LDTRUE:
            case EcmaOpcode::LDFALSE:
            case EcmaOpcode::LDHOLE:
            case EcmaOpcode::LDAI_IMM32:
            case EcmaOpcode::FLDAI_IMM64:
            case EcmaOpcode::LDFUNCTION:
            case EcmaOpcode::LDA_STR_ID16:
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
            case EcmaOpcode::CALLARG0_IMM8:
            case EcmaOpcode::CALLARG1_IMM8_V8:
            case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
            case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
            case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
            case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
            case EcmaOpcode::CALLTHIS0_IMM8_V8:
            case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
            case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
            case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
            case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
            case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
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
            case EcmaOpcode::SUSPENDGENERATOR_V8:
            case EcmaOpcode::RESUMEGENERATOR:
                return true;
            default:
                return false;
        }
    }

    bool IsBc(EcmaOpcode ecmaOpcode) const
    {
        return opcode == ecmaOpcode;
    }
};

enum BytecodeOffset {
    ONE = 1,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    TEN
};

class BytecodeCircuitBuilder {
public:
    explicit BytecodeCircuitBuilder(const JSPandaFile *jsPandaFile,
                                    const MethodLiteral *methodLiteral,
                                    MethodPcInfo &methodPCInfo,
                                    TSManager *tsManager,
                                    const CompilationConfig* cconfig,
                                    bool hasTypes,
                                    bool enableLog,
                                    std::string name,
                                    const CString &recordName)
        : tsManager_(tsManager), circuit_(cconfig->Is64Bit()), file_(jsPandaFile), pf_(jsPandaFile->GetPandaFile()),
          method_(methodLiteral), gateAcc_(&circuit_), argAcc_(&circuit_, method_, jsPandaFile),
          typeRecorder_(jsPandaFile, method_, tsManager), hasTypes_(hasTypes),
          enableLog_(enableLog), pcToBCOffset_(methodPCInfo.pcToBCOffset),
          byteCodeCurPrePc_(methodPCInfo.byteCodeCurPrePc), bytecodeBlockInfos_(methodPCInfo.bytecodeBlockInfos),
          frameStateBuilder_(this, &circuit_, methodLiteral), methodName_(name), recordName_(recordName)
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

    [[nodiscard]] const std::map<GateRef, std::pair<size_t, const uint8_t *>>& GetGateToBytecode() const
    {
        return jsgateToBytecode_;
    }

    [[nodiscard]] const std::map<const uint8_t *, GateRef>& GetBytecodeToGate() const
    {
        return byteCodeToJSGate_;
    }

    [[nodiscard]] EcmaOpcode PcToOpcode(const uint8_t *pc) const
    {
        BytecodeInstruction inst(pc);
        return inst.GetOpcode();
    }

    [[nodiscard]] std::string GetBytecodeStr(GateRef gate) const
    {
        auto pc = jsgateToBytecode_.at(gate).second;
        auto opcode = PcToOpcode(pc);
        return GetEcmaOpcodeStr(opcode);
    }

    [[nodiscard]] EcmaOpcode GetByteCodeOpcode(GateRef gate) const
    {
        auto pc = jsgateToBytecode_.at(gate).second;
        return PcToOpcode(pc);
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

    const std::string& GetMethodName() const
    {
        return methodName_;
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

    const std::map<const uint8_t *, int32_t> &GetPcToBCOffset() const
    {
        return pcToBCOffset_;
    }

    const BytecodeRegion &GetBasicBlockById(size_t id) const
    {
        return graph_[id];
    }

    size_t GetBasicBlockCount() const
    {
        return graph_.size();
    }

    const std::map<uint8_t *, uint8_t *> &GetByteCodeCurPrePc() const
    {
        return byteCodeCurPrePc_;
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
    void NewJSGate(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void NewJump(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void NewReturn(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void NewByteCode(BytecodeRegion &bb, const uint8_t *pc, GateRef &state, GateRef &depend);
    void AddBytecodeOffsetInfo(GateRef &gate, const BytecodeInfo &info, size_t bcOffsetIndex, uint8_t *pc);
    void BuildSubCircuit();
    void NewPhi(BytecodeRegion &bb, uint16_t reg, bool acc, GateRef &currentPhi);
    GateRef RenameVariable(const size_t bbId, const uint8_t *end, const uint16_t reg, const bool acc);
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
    std::map<GateRef, std::pair<size_t, const uint8_t *>> jsgateToBytecode_;
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
    std::vector<GateRef> suspendAndResumeGates_ {};
    const std::map<const uint8_t *, int32_t> &pcToBCOffset_;
    const std::map<uint8_t *, uint8_t *> &byteCodeCurPrePc_;
    std::vector<CfgInfo> &bytecodeBlockInfos_;
    std::map<std::pair<kungfu::GateRef, uint16_t>, kungfu::GateRef> resumeRegToRestore_;
    FrameStateBuilder frameStateBuilder_;
    std::string methodName_;
    const CString &recordName_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_CLASS_LINKER_BYTECODE_CIRCUIT_IR_BUILDER_H
