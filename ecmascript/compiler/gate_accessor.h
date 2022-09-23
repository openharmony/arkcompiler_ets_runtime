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

namespace panda::ecmascript::kungfu {
class GateAccessor {
public:
    // do not create new gate or modify self during iteration
    struct ConstUseIterator {
        explicit ConstUseIterator(const Circuit* circuit, const Out* out) : circuit_(circuit), out_(out)
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

        friend bool operator== (const ConstUseIterator& a, const ConstUseIterator& b)
        {
            return a.out_ == b.out_;
        };
        friend bool operator!= (const ConstUseIterator& a, const ConstUseIterator& b)
        {
            return a.out_ != b.out_;
        };

    private:
        const Circuit* circuit_;
        const Out* out_;
    };

    // do not create new gate or modify self during iteration
    struct UseIterator {
        explicit UseIterator(Circuit* circuit, Out* out) : circuit_(circuit), out_(out)
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

        OpCode GetOpCode() const
        {
            ASSERT(out_ != nullptr);
            return out_->GetGateConst()->GetOpCode();
        }

        friend bool operator== (const UseIterator& a, const UseIterator& b)
        {
            return a.out_ == b.out_;
        };
        friend bool operator!= (const UseIterator& a, const UseIterator& b)
        {
            return a.out_ != b.out_;
        };

    private:
        Circuit* circuit_;
        Out* out_;
    };

    struct ConstInsIterator {
        explicit ConstInsIterator(const Circuit* circuit, const In* in) : circuit_(circuit), in_(in)
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
        explicit InsIterator(const Circuit* circuit, In* in) : circuit_(circuit), in_(in)
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

    InWrapper Ins(GateRef gate)
    {
        return { circuit_, gate };
    }

    ConstUseWrapper ConstUses(GateRef gate)
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

    void GetInVector(GateRef gate, std::vector<GateRef>& ins) const;
    void GetOutVector(GateRef gate, std::vector<GateRef>& outs) const;
    void GetOutStateVector(GateRef gate, std::vector<GateRef>& outStates) const;
    void GetAllGates(std::vector<GateRef>& gates) const;
    size_t GetNumIns(GateRef gate) const;
    OpCode GetOpCode(GateRef gate) const;
    bool IsGCRelated(GateRef gate) const;
    void SetOpCode(GateRef gate, OpCode::Op opcode);
    BitField GetBitField(GateRef gate) const;
    void SetBitField(GateRef gate, BitField bitField);
    void Print(GateRef gate) const;
    GateId GetId(GateRef gate) const;
    GateRef GetValueIn(GateRef gate, size_t idx = 0) const;
    size_t GetNumValueIn(GateRef gate) const;
    GateRef GetIn(GateRef gate, size_t idx) const;
    GateRef GetState(GateRef gate, size_t idx = 0) const;
    GateRef GetDep(GateRef gate, size_t idx = 0) const;
    size_t GetImmediateId(GateRef gate) const;
    void SetDep(GateRef gate, GateRef depGate, size_t idx = 0);
    UseIterator ReplaceIn(const UseIterator &useIt, GateRef replaceGate);
    // Add for lowering
    GateType GetGateType(GateRef gate) const;
    void SetGateType(GateRef gate, GateType gt);
    UseIterator DeleteExceptionDep(const UseIterator &useIt);
    void DeleteIn(GateRef gate, size_t idx);
    UseIterator DeleteGate(const UseIterator &useIt);
    void DecreaseIn(const UseIterator &useIt);
    void NewIn(GateRef gate, size_t idx, GateRef in);
    size_t GetStateCount(GateRef gate) const;
    size_t GetDependCount(GateRef gate) const;
    size_t GetInValueCount(GateRef gate) const;
    void UpdateAllUses(GateRef gate, GateRef replaceValueIn);
    void ReplaceIn(GateRef gate, size_t index, GateRef in);
    void ReplaceStateIn(GateRef gate, GateRef in, size_t index = 0);
    void ReplaceDependIn(GateRef gate, GateRef in, size_t index = 0);
    void ReplaceValueIn(GateRef gate, GateRef in, size_t index = 0);
    void DeleteGate(GateRef gate);
    MachineType GetMachineType(GateRef gate) const;
    void SetMachineType(GateRef gate, MachineType type);
    GateRef GetConstantGate(MachineType bitValue, BitField bitfield, GateType type) const;
    bool IsInGateNull(GateRef gate, size_t idx) const;
    bool IsSelector(GateRef g) const;
    bool IsControlCase(GateRef gate) const;
    bool IsLoopHead(GateRef gate) const;
    bool IsLoopBack(GateRef gate) const;
    bool IsState(GateRef gate) const;
    bool IsSchedulable(GateRef gate) const;
    bool IsTypedGate(GateRef gate) const;
    MarkCode GetMark(GateRef gate) const;
    void SetMark(GateRef gate, MarkCode mark);
    bool IsFinished(GateRef gate) const;
    bool IsVisited(GateRef gate) const;
    bool IsNotMarked(GateRef gate) const;
    void SetFinished(GateRef gate);
    void SetVisited(GateRef gate);
    bool IsStateIn(const UseIterator &useIt) const;
    bool IsDependIn(const UseIterator &useIt) const;
    bool IsValueIn(const UseIterator &useIt) const;
    bool IsExceptionState(const UseIterator &useIt) const;
    bool IsDependIn(GateRef gate, size_t index) const;
    bool IsValueIn(GateRef gate, size_t index) const;

private:
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

    Circuit *circuit_;
};
}
#endif  // ECMASCRIPT_COMPILER_GATE_ACCESSOR_H
