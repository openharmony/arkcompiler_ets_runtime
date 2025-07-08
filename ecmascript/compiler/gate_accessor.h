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

#ifndef ECMASCRIPT_COMPILER_GATE_ACCESSOR_H
#define ECMASCRIPT_COMPILER_GATE_ACCESSOR_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/mcr_gate_meta_data.h"
#include "ecmascript/elements.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"

namespace panda::ecmascript::kungfu {

class StateDepend {
public:
    StateDepend()
        : state_(Circuit::NullGate()), depend_(Circuit::NullGate()) {}

    explicit StateDepend(GateRef state, GateRef depend)
        : state_(state), depend_(depend) {}

    GateRef State() const
    {
        return state_;
    }

    GateRef Depend() const
    {
        return depend_;
    }

    void SetState(GateRef state)
    {
        state_ = state;
    }

    void SetDepend(GateRef depend)
    {
        depend_ = depend;
    }

    void Reset()
    {
        state_ = Circuit::NullGate();
        depend_ = Circuit::NullGate();
    }

private:
    GateRef state_;
    GateRef depend_;
};

class Edge {
public:
    explicit Edge(GateRef gate, size_t index) : gate_(gate), index_(static_cast<uint32_t>(index)) {}

    GateRef GetGate() const
    {
        return gate_;
    }

    size_t GetIndex() const
    {
        return static_cast<size_t>(index_);
    }

    void SetIndex(size_t index)
    {
        index_ = static_cast<uint32_t>(index);
    }

private:
    GateRef gate_;
    uint32_t index_;
};

class GateAccessor {
public:
    constexpr static uint32_t INVALID_BC_INDEX = 0;

    // do not create new gate or modify self during iteration
    struct ConstUseIterator {
        ConstUseIterator(const Circuit* circuit, const Out* out) : circuit_(circuit), out_(out)
        {
        }

        GateRef operator*() const
        {
            if (out_ != nullptr) {
                return circuit_->GetGateRef(out_->GetGateConst());
            }
            return 0;
        }

        const ConstUseIterator operator++()
        {
            ASSERT(out_ != nullptr);
            if (!out_->IsNextOutNull()) {
                out_ = out_->GetNextOutConst();
                return *this;
            }
            out_ = nullptr;
            return *this;
        }
        const ConstUseIterator operator++(int)
        {
            ConstUseIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        size_t GetIndex() const
        {
            ASSERT(out_ != nullptr);
            return out_->GetIndex();
        }

        OpCode GetOpCode() const
        {
            ASSERT(out_ != nullptr);
            return out_->GetGateConst()->GetOpCode();
        }

        friend bool operator == (const ConstUseIterator& a, const ConstUseIterator& b)
        {
            return a.out_ == b.out_;
        };
        friend bool operator != (const ConstUseIterator& a, const ConstUseIterator& b)
        {
            return a.out_ != b.out_;
        };

    private:
        const Circuit* circuit_;
        const Out* out_;
    };

    // do not create new gate or modify self during iteration
    struct UseIterator {
        UseIterator(Circuit* circuit, Out* out) : circuit_(circuit), out_(out)
        {
        }

        GateRef operator*() const
        {
            if (out_ != nullptr) {
                return circuit_->GetGateRef(out_->GetGate());
            }
            return 0;
        }

        const UseIterator& operator++()
        {
            ASSERT(out_ != nullptr);
            out_ = out_->IsNextOutNull() ? nullptr
                                         : out_->GetNextOut();
            return *this;
        }

        UseIterator operator++(int)
        {
            UseIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        size_t GetIndex() const
        {
            ASSERT(out_ != nullptr);
            return out_->GetIndex();
        }

        Edge GetEdge()
        {
            ASSERT(out_ != nullptr);
            UseIterator it = *this;
            return Edge(*it, GetIndex());
        }

        OpCode GetOpCode() const
        {
            ASSERT(out_ != nullptr);
            return out_->GetGateConst()->GetOpCode();
        }

        friend bool operator == (const UseIterator& a, const UseIterator& b)
        {
            return a.out_ == b.out_;
        };
        friend bool operator != (const UseIterator& a, const UseIterator& b)
        {
            return a.out_ != b.out_;
        };

    private:
        Circuit* circuit_;
        Out* out_;
    };

    struct ConstInsIterator {
        ConstInsIterator(const Circuit* circuit, const In* in) : circuit_(circuit), in_(in)
        {
        }

        GateRef operator*() const
        {
            return circuit_->GetGateRef(in_->GetGateConst());
        }

        const ConstInsIterator& operator++()
        {
            in_++;
            return *this;
        }
        ConstInsIterator operator++(int)
        {
            ConstInsIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        OpCode GetOpCode() const
        {
            ASSERT(in_ != nullptr);
            return in_->GetGateConst()->GetOpCode();
        }

        friend bool operator== (const ConstInsIterator& a, const ConstInsIterator& b)
        {
            return a.in_ == b.in_;
        };
        friend bool operator!= (const ConstInsIterator& a, const ConstInsIterator& b)
        {
            return a.in_ != b.in_;
        };

    private:
        const Circuit* circuit_;
        const In* in_;
    };

    struct InsIterator {
        InsIterator(const Circuit* circuit, In* in) : circuit_(circuit), in_(in)
        {
        }

        GateRef operator*()
        {
            return circuit_->GetGateRef(in_->GetGate());
        }

        const InsIterator& operator++()
        {
            in_++;
            return *this;
        }
        InsIterator operator++(int)
        {
            InsIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        OpCode GetOpCode() const
        {
            ASSERT(in_ != nullptr);
            return in_->GetGateConst()->GetOpCode();
        }

        friend bool operator== (const InsIterator& a, const InsIterator& b)
        {
            return a.in_ == b.in_;
        };
        friend bool operator!= (const InsIterator& a, const InsIterator& b)
        {
            return a.in_ != b.in_;
        };

    private:
        const Circuit* circuit_;
        In* in_;
    };

    struct ConstUseWrapper {
        Circuit* circuit;
        GateRef gate;
        auto begin()
        {
            return GateAccessor(circuit).ConstUseBegin(gate);
        }
        auto end()
        {
            return GateAccessor(circuit).ConstUseEnd();
        }
    };

    struct UseWrapper {
        Circuit* circuit;
        GateRef gate;
        auto begin()
        {
            return GateAccessor(circuit).UseBegin(gate);
        }
        auto end()
        {
            return GateAccessor(circuit).UseEnd();
        }
    };

    struct ConstInWrapper {
        Circuit* circuit;
        const GateRef gate;
        auto begin()
        {
            return GateAccessor(circuit).ConstInBegin(gate);
        }
        auto end()
        {
            return GateAccessor(circuit).ConstInEnd(gate);
        }
    };

    struct InWrapper {
        Circuit* circuit;
        GateRef gate;
        auto begin()
        {
            return GateAccessor(circuit).InBegin(gate);
        }
        auto end()
        {
            return GateAccessor(circuit).InEnd(gate);
        }
    };

    ConstInWrapper ConstIns(GateRef gate) const
    {
        return { circuit_, gate };
    }

    InWrapper Ins(GateRef gate) const
    {
        return { circuit_, gate };
    }

    ConstUseWrapper ConstUses(GateRef gate) const
    {
        return { circuit_, gate };
    }

    UseWrapper Uses(GateRef gate)
    {
        return { circuit_, gate };
    }

    explicit GateAccessor(Circuit *circuit) : circuit_(circuit)
    {
    }

    Circuit *GetCircuit() const
    {
        return circuit_;
    }

    ~GateAccessor() = default;
    void GetAllGates(std::vector<GateRef>& gates) const;
    size_t GetNumIns(GateRef gate) const;
    OpCode GetOpCode(GateRef gate) const;
    bool IsGCRelated(GateRef gate) const;
    uint64_t TryGetValue(GateRef gate) const;
    ICmpCondition GetICmpCondition(GateRef gate) const;
    FCmpCondition GetFCmpCondition(GateRef gate) const;
    size_t GetOffset(GateRef gate) const;
    size_t GetInitOffset(GateRef gate) const;
    uint32_t GetTrueWeight(GateRef gate) const;
    uint32_t GetFalseWeight(GateRef gate) const;
    bool HasBranchWeight(GateRef gate) const;
    MemoryAttribute GetMemoryAttribute(GateRef gate) const;
    size_t GetIndex(GateRef gate) const;
    size_t GetJSType(GateRef gate) const;
    uint32_t GetArraySize(GateRef gate) const;
    void SetArraySize(GateRef gate, uint32_t size);
    uint32_t GetStringStatus(GateRef gate) const;
    void SetStringStatus(GateRef gate, uint32_t type);
    ElementsKind GetElementsKind(GateRef gate) const;
    void SetElementsKind(GateRef gate, ElementsKind kind);
    RegionSpaceFlag GetRegionSpaceFlag(GateRef gate) const;
    size_t GetVirtualRegisterIndex(GateRef gate) const;
    bool TypedOpIsTypedArray(GateRef gate, TypedOpKind kind) const;
    TypedLoadOp GetTypedLoadOp(GateRef gate) const;
    TypedStoreOp GetTypedStoreOp(GateRef gate) const;
    MemoryType GetMemoryType(GateRef gate) const;
    uint32_t GetHClassIndex(GateRef gate) const;
    TypedBinOp GetTypedBinaryOp(GateRef gate) const;
    TypedCallTargetCheckOp GetTypedCallTargetCheckOp(GateRef gate) const;
    bool HasNumberType(GateRef gate) const;
    bool HasStringType(GateRef gate, bool onlyInternString = false) const;
    bool IsInternStringType(GateRef gate) const;
    GlobalTSTypeRef GetFuncGT(GateRef gate) const;
    GateType GetParamGateType(GateRef gate) const;
    ParamType GetParamType(GateRef gate) const;
    TypedUnaryAccessor GetTypedUnAccessor(GateRef gate) const;
    TypedBinaryAccessor GetTypedBinaryAccessor(GateRef gate) const;
    TypedJumpAccessor GetTypedJumpAccessor(GateRef gate) const;
    ArrayMetaDataAccessor GetArrayMetaDataAccessor(GateRef gate) const;
    CreateArgumentsAccessor GetCreateArgumentsAccessor(GateRef gate) const;
    ObjectTypeAccessor GetObjectTypeAccessor(GateRef gate) const;
    BuiltinPrototypeHClassAccessor GetBuiltinHClassAccessor(GateRef gate) const;
    TypedArrayMetaDataAccessor GetTypedArrayMetaDataAccessor(GateRef gate) const;
    LoadElementAccessor GetLoadElementAccessor(GateRef gate) const;
    StoreElementAccessor GetStoreElementAccessor(GateRef gate) const;
    bool NeedPushArgv(GateRef gate) const;
    bool IsFastCall(GateRef gate) const;
    uint64_t GetConstantValue(GateRef gate) const;
    const ChunkVector<char>& GetConstantString(GateRef gate) const;
    bool IsVtable(GateRef gate) const;
    bool GetNoGCFlag(GateRef gate) const;
    bool TypedCallIsNoGC(GateRef gate) const;
    bool IsNoGC(GateRef gate) const;
    uint32_t TryGetPcOffset(GateRef gate) const;
    uint32_t TryGetBcIndex(GateRef gate) const;
    uint32_t TryGetMethodOffset(GateRef gate) const;
    GateRef GetFrameArgs(GateRef gate) const;
    void UpdateMethodOffset(GateRef gate, uint32_t methodOffset);
    PGOTypeRef TryGetPGOType(GateRef gate) const;
    void TrySetPGOType(GateRef gate, PGOTypeRef type);
    uint32_t TryGetArrayElementsLength(GateRef gate) const;
    void TrySetArrayElementsLength(GateRef gate, uint32_t length);
    RegionSpaceFlag TryGetRegionSpaceFlag(GateRef gate) const;
    void TrySetRegionSpaceFlag(GateRef gate, RegionSpaceFlag length);
    ElementsKind TryGetElementsKind(GateRef gate) const;
    ElementsKind TryGetArrayElementsKind(GateRef gate) const;
    ElementsKind TryGetArrayElementsKindAfterTransition(GateRef gate) const;
    void TrySetElementsKind(GateRef gate, ElementsKind kind);
    void TrySetTransitionElementsKind(GateRef gate, ElementsKind kind);
    void TrySetOnHeapMode(GateRef gate, OnHeapMode onHeapMode) const;
    OnHeapMode TryGetOnHeapMode(GateRef gate) const;
    EcmaOpcode GetByteCodeOpcode(GateRef gate) const;
    void Print(GateRef gate) const DUMP_API_ATTR;
    std::string ToString(GateRef gate) const DUMP_API_ATTR;
#ifndef NDEBUG
    void PrintById(size_t id) const DUMP_API_ATTR ;
#endif
    void PrintWithBytecode(GateRef gate) const DUMP_API_ATTR;
    void ShortPrint(GateRef gate) const;
    GateId GetId(GateRef gate) const;
    GateRef GetValueIn(GateRef gate, size_t idx = 0) const;
    size_t GetNumValueIn(GateRef gate) const;
    std::vector<GateRef> GetValueIns(GateRef gate) const;
    GateRef GetIn(GateRef gate, size_t idx) const;
    GateRef GetState(GateRef gate, size_t idx = 0) const;
    GateRef GetDep(GateRef gate, size_t idx = 0) const;
    size_t GetImmediateId(GateRef gate) const;
    void SetDep(GateRef gate, GateRef depGate, size_t idx = 0);
    UseIterator ReplaceIn(const UseIterator &useIt, GateRef replaceGate);
    // Add for lowering
    GateType GetGateType(GateRef gate) const;
    bool IsConvertSupport(GateRef gate) const;
    ValueType GetSrcType(GateRef gate) const;
    ValueType GetDstType(GateRef gate) const;
    void SetGateType(GateRef gate, GateType gt);
    void DeleteIn(GateRef gate, size_t idx);
    UseIterator DeleteGate(const UseIterator &useIt);
    void DecreaseIn(const UseIterator &useIt);
    void DecreaseIn(GateRef gate, size_t index);
    void NewIn(GateRef gate, size_t idx, GateRef in);
    size_t GetStateCount(GateRef gate) const;
    size_t GetDependCount(GateRef gate) const;
    size_t GetInValueCount(GateRef gate) const;
    size_t GetInValueStarts(GateRef gate) const;
    void UpdateAllUses(GateRef gate, GateRef replaceValueIn);
    void ReplaceControlGate(GateRef gate, GateRef newState);
    void ReplaceInAfterInsert(GateRef state, GateRef depend, GateRef newGate);
    void GetFrameStateDependIn(GateRef gate, GateRef &dependIn);
    void GetStateInAndDependIn(GateRef insertAfter, GateRef &stateIn, GateRef &dependIn);
    void ReplaceIn(GateRef gate, size_t index, GateRef in);
    void ReplaceStateIn(GateRef gate, GateRef in, size_t index = 0);
    void ReplaceDependIn(GateRef gate, GateRef in, size_t index = 0);
    void ReplaceOrNewDependIn(GateRef gate, GateRef in, size_t index = 0);
    void ReplaceValueIn(GateRef gate, GateRef in, size_t index = 0);
    void DeleteGate(GateRef gate);
    MachineType GetMachineType(GateRef gate) const;
    void SetMachineType(GateRef gate, MachineType type);
    GateRef GetConstantGate(MachineType bitValue, BitField bitfield, GateType type) const;
    GateRef GetInitialEnvGate(GateRef depend, GateRef jsFunc) const;
    double GetFloat64FromConstant(GateRef gate) const;
    int GetInt32FromConstant(GateRef gate) const;
    bool IsBoolType(GateRef gate) const;
    bool IsInGateNull(GateRef gate, size_t idx) const;
    bool IsSelector(GateRef g) const;
    bool IsSimpleState(GateRef g) const;
    bool IsValueSelector(GateRef g) const;
    bool IsFrameValues(GateRef g) const;
    bool IsControlCase(GateRef gate) const;
    bool IsLoopExit(GateRef gate) const;
    bool IsLoopExitRelated(GateRef gate) const;
    bool IsLoopHead(GateRef gate) const;
    bool IsLoopBack(GateRef gate) const;
    bool IsState(GateRef gate) const;
    bool IsConstant(GateRef gate) const;
    bool IsDependSelector(GateRef gate) const;
    bool IsConstantValue(GateRef gate, uint64_t value) const;
    bool IsConstantTaggedValue(GateRef gate, uint64_t value) const;
    bool IsConstantUndefined(GateRef gate) const;
    bool IsUndefinedOrNullOrHole(GateRef gate) const;
    bool IsConstantNumber(GateRef gate) const;
    bool IsTypedOperator(GateRef gate) const;
    bool IsNotWrite(GateRef gate) const;
    bool IsDead(GateRef gate) const;
    bool IsCheckWithOneIn(GateRef gate) const;
    bool IsCheckWithTwoIns(GateRef gate) const;
    bool IsSchedulable(GateRef gate) const;
    bool IsVirtualState(GateRef gate) const;
    bool IsGeneralState(GateRef gate) const;
    MarkCode GetMark(GateRef gate) const;
    void SetMark(GateRef gate, MarkCode mark);
    bool IsFinished(GateRef gate) const;
    bool IsVisited(GateRef gate) const;
    bool IsPrevisit(GateRef gate) const;
    bool IsNotMarked(GateRef gate) const;
    void SetFinished(GateRef gate);
    void SetPrevisit(GateRef gate);
    void SetVisited(GateRef gate);
    bool IsStateIn(const UseIterator &useIt) const;
    bool IsDependIn(const UseIterator &useIt) const;
    bool IsValueIn(const UseIterator &useIt) const;
    bool IsFrameStateIn(const UseIterator &useIt) const;
    bool IsStateIn(GateRef gate, size_t index) const;
    bool IsDependIn(GateRef gate, size_t index) const;
    bool IsValueIn(GateRef gate, size_t index) const;
    void GetStateUses(GateRef gate, std::vector<GateRef> &stateUses);
    void GetDependUses(GateRef gate, std::vector<GateRef> &dependUses);
    void GetValueUses(GateRef gate, std::vector<GateRef> &valueUses);
    size_t GetValueUsesCount(GateRef gate);
    bool IsFrameStateIn(GateRef gate, size_t index) const;
    void EliminateRedundantPhi();
    void ReplaceGate(GateRef gate, GateRef state, GateRef depend, GateRef value);
    void ReplaceGate(GateRef gate, StateDepend stateDepend, GateRef replacement);
    void ReplaceGate(GateRef gate, GateRef replacement);
    uint32_t GetFirstValue(GateRef gate) const;
    uint32_t GetSecondValue(GateRef gate) const;
    GateRef GetGlueFromArgList() const;
    void GetArgsOuts(std::vector<GateRef>& outs) const;
    void GetReturnOuts(std::vector<GateRef>& outs) const;
    bool IsFixed(GateRef g) const;
    bool IsProlog(GateRef g) const;
    bool IsCFGMerge(GateRef g) const;
    bool MetaDataEqu(GateRef g1, GateRef g2) const;
    bool MetaDataValueEqu(GateRef g1, GateRef g2) const;
    bool IsNop(GateRef g) const;
    bool IsRoot(GateRef g) const;
    bool HasOuts(GateRef gate) const;
    void DeleteGateIfNoUse(GateRef gate);
    GateRef GetDependSelectorFromMerge(GateRef gate);
    bool HasIfExceptionUse(GateRef gate) const;
    bool IsIn(GateRef g, GateRef in) const;
    bool IsHeapObjectFromElementsKind(GateRef gate);
    bool IsConstString(GateRef gate);
    bool IsSingleCharGate(GateRef gate);
    bool UseForTypeOpProfilerGate(GateRef gate) const;
    uint32_t GetStringIdFromLdaStrGate(GateRef gate);
    bool IsIfOrSwitchRelated(GateRef gate) const;
    uint32_t GetConstpoolId(GateRef gate) const;
    GateRef GetFrameValue(GateRef gate);

    GateRef GetCircuitRoot() const
    {
        return GetRoot(OpCode::CIRCUIT_ROOT);
    }

    GateRef GetStateRoot() const
    {
        return GetRoot(OpCode::STATE_ENTRY);
    }

    GateRef GetDependRoot() const
    {
        return GetRoot(OpCode::DEPEND_ENTRY);
    }

    GateRef GetArgRoot() const
    {
        return GetRoot(OpCode::ARG_LIST);
    }

    GateRef GetReturnRoot() const
    {
        return GetRoot(OpCode::RETURN_LIST);
    }

    inline bool IsStateRoot(GateRef gate) const
    {
        return gate == GetStateRoot();
    }

    size_t GetFrameDepth(GateRef gate, OpCode op);
    GateRef GetFrameState(GateRef gate) const;
    void ReplaceFrameStateIn(GateRef gate, GateRef in);
    bool HasFrameState(GateRef gate) const;
    GateRef FindNearestFrameState(GateRef gate) const;
    GateRef FindNearestStateSplit(GateRef gate) const;
    void SetMetaData(GateRef gate, const GateMetaData* meta);

    void ReplaceHirWithIfBranch(GateRef hirGate, StateDepend success, StateDepend exception, GateRef value);
    void ReplaceHirDirectly(GateRef hirGate, StateDepend replacement, GateRef value);
    void ReplaceHirAndReplaceDeadIfException(GateRef hirGate, StateDepend replacement, GateRef value);

    bool IsLoopBackUse(GateRef gate, const UseIterator &useIt) const;
    void GetOutStates(GateRef gate, std::vector<GateRef>& outStates) const;
    bool IsCreateArray(GateRef gate) const;
    void SetStoreNoBarrier(GateRef gate, bool isNoBarrier);
    bool IsNoBarrier(GateRef gate) const;
    void GetIns(GateRef gate, std::vector<GateRef>& ins) const;

    bool TryGetMegaProp(GateRef gate) const;
    TypedBinOp GetRevCompareOpForTypedBinOp(TypedBinOp op);
    TypedBinOp GetSwapCompareOpForTypedBinOp(TypedBinOp op);
    void UpdateOnHeapMode(GateRef gate, OnHeapMode onHeapMode);
    uint64_t GetMetaDataHash(GateRef gate) const;

private:
    const GateMetaData *GetMetaData(GateRef gate) const;
    UseIterator ReplaceHirIfSuccess(const UseIterator &useIt, GateRef state);
    UseIterator ReplaceHirIfException(const UseIterator &useIt, StateDepend replacement);
    void ExceptionReturn(GateRef state, GateRef depend);

    GateRef GetRoot(OpCode opcode) const;
    ConstUseIterator ConstUseBegin(GateRef gate) const
    {
        if (circuit_->LoadGatePtrConst(gate)->IsFirstOutNull()) {
            return ConstUseIterator(circuit_, nullptr);
        }
        auto use = circuit_->LoadGatePtrConst(gate)->GetFirstOutConst();
        return ConstUseIterator(circuit_, use);
    }

    ConstUseIterator ConstUseEnd() const
    {
        return ConstUseIterator(circuit_, nullptr);
    }

    UseIterator UseBegin(GateRef gate) const
    {
        if (circuit_->LoadGatePtrConst(gate)->IsFirstOutNull()) {
            return UseIterator(circuit_, nullptr);
        }
        auto use = circuit_->LoadGatePtr(gate)->GetFirstOut();
        return UseIterator(circuit_, use);
    }

    UseIterator UseEnd() const
    {
        return UseIterator(circuit_, nullptr);
    }

    ConstInsIterator ConstInBegin(GateRef gate) const
    {
        return ConstInsIterator(circuit_, &reinterpret_cast<const In *>(circuit_->LoadGatePtrConst(gate) + 1)[0]);
    }

    ConstInsIterator ConstInEnd(GateRef gate) const
    {
        auto endIndex = circuit_->LoadGatePtrConst(gate)->GetNumIns();
        return ConstInsIterator(circuit_,
                                &reinterpret_cast<const In *>(circuit_->LoadGatePtrConst(gate) + 1)[endIndex]);
    }

    InsIterator InBegin(GateRef gate)
    {
        return InsIterator(circuit_, &reinterpret_cast<In *>(circuit_->LoadGatePtr(gate) + 1)[0]);
    }

    InsIterator InEnd(GateRef gate)
    {
        auto endIndex = circuit_->LoadGatePtrConst(gate)->GetNumIns();
        return InsIterator(circuit_, &reinterpret_cast<In *>(circuit_->LoadGatePtr(gate) + 1)[endIndex]);
    }

    void GetOuts(GateRef gate, std::vector<GateRef>& outs) const;

    void GetInStates(GateRef gate, std::vector<GateRef>& ins) const;

    Circuit *circuit_;

    friend class Circuit;
    friend class CircuitBuilder;
    friend class LLVMIRBuilder;
    friend class LiteCGIRBuilder;
    friend class Scheduler;
    friend class LoopPeeling;
};

class ConstGateAccessor {
public:
    struct ConstInsIterator {
        ConstInsIterator(const Circuit* circuit, const In* in) : circuit_(circuit), in_(in)
        {
        }

        GateRef operator*() const
        {
            return circuit_->GetGateRef(in_->GetGateConst());
        }

        const ConstInsIterator& operator++()
        {
            in_++;
            return *this;
        }
        ConstInsIterator operator++(int)
        {
            ConstInsIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        OpCode GetOpCode() const
        {
            ASSERT(in_ != nullptr);
            return in_->GetGateConst()->GetOpCode();
        }

        friend bool operator== (const ConstInsIterator& a, const ConstInsIterator& b)
        {
            return a.in_ == b.in_;
        };
        friend bool operator!= (const ConstInsIterator& a, const ConstInsIterator& b)
        {
            return a.in_ != b.in_;
        };

    private:
        const Circuit* circuit_;
        const In* in_;
    };

    struct ConstInWrapper {
        const Circuit* circuit;
        const GateRef gate;
        auto begin()
        {
            return ConstGateAccessor(circuit).ConstInBegin(gate);
        }
        auto end()
        {
            return ConstGateAccessor(circuit).ConstInEnd(gate);
        }
    };

    ConstInWrapper Ins(GateRef gate) const
    {
        return { circuit_, gate };
    }

    explicit ConstGateAccessor(const Circuit *circuit) : circuit_(circuit)
    {
    }

    ~ConstGateAccessor() = default;

    bool IsFixed(GateRef g) const;
    bool IsProlog(GateRef g) const;
    bool IsSchedulable(GateRef g) const;

private:
    ConstInsIterator ConstInBegin(GateRef gate) const
    {
        return ConstInsIterator(circuit_, &reinterpret_cast<const In *>(circuit_->LoadGatePtrConst(gate) + 1)[0]);
    }

    ConstInsIterator ConstInEnd(GateRef gate) const
    {
        auto endIndex = circuit_->LoadGatePtrConst(gate)->GetNumIns();
        return ConstInsIterator(circuit_,
                                &reinterpret_cast<const In *>(circuit_->LoadGatePtrConst(gate) + 1)[endIndex]);
    }
    const GateMetaData *GetMetaData(GateRef g) const;

    const Circuit *circuit_;
    friend struct ConstInWrapper;
};
}
#endif  // ECMASCRIPT_COMPILER_GATE_ACCESSOR_H
