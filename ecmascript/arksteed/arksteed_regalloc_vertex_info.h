/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_VERTEX_INFO_H
#define ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_VERTEX_INFO_H

#include <utility>

#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/mem/chunk.h"

namespace panda::ecmascript::arksteed {

// =============================================================================
// RegallocVertexInfo - Base info for register allocation on a vertex
// =============================================================================

class RegallocVertexInfo {
public:
    RegallocVertexInfo(Chunk *chunk, int inputCount) : inputCount_(inputCount)
    {
        inputLocations_ = chunk->NewArray<InputLocation>(inputCount);
        for (int i = 0; i < inputCount; i++) {
            new (&inputLocations_[i]) InputLocation();
        }
    }

    NO_COPY_SEMANTIC(RegallocVertexInfo);
    NO_MOVE_SEMANTIC(RegallocVertexInfo);

    VertexId GetId() const
    {
        return id_;
    }
    void SetId(VertexId id)
    {
        id_ = id;
    }
    bool HasId() const
    {
        return id_ != INVALID_VERTEX_ID;
    }

    InputLocation *GetInputLocation(int index)
    {
        ASSERT(index >= 0 && index < inputCount_);
        return &inputLocations_[index];
    }
    const InputLocation *GetInputLocation(int index) const
    {
        ASSERT(index >= 0 && index < inputCount_);
        return &inputLocations_[index];
    }
    int GetInputCount() const
    {
        return inputCount_;
    }

    void AddGeneralTemporary(ArkSteedRegister reg)
    {
        generalTemporaries_.Set(reg);
    }
    void AddDoubleTemporary(ArkSteedDoubleRegister reg)
    {
        doubleTemporaries_.Set(reg);
    }
    void ClearGeneralTemporaries()
    {
        generalTemporaries_.Reset();
    }
    void ClearDoubleTemporaries()
    {
        doubleTemporaries_.Reset();
    }
    const ArkSteedRegList &GetGeneralTemporaries() const
    {
        return generalTemporaries_;
    }
    const ArkDoubleRegList &GetDoubleTemporaries() const
    {
        return doubleTemporaries_;
    }
    ArkSteedRegList &GetGeneralTemporaries()
    {
        return generalTemporaries_;
    }
    ArkDoubleRegList &GetDoubleTemporaries()
    {
        return doubleTemporaries_;
    }
    bool HasTemporaries() const
    {
        return !generalTemporaries_.IsEmpty() || !doubleTemporaries_.IsEmpty();
    }

    template <size_t N>
    std::array<ArkSteedRegister, N> TakeGeneralTemporaries() const
    {
        return TakeTemporaries<ArkSteedRegister, N>();
    }
    template <size_t N>
    std::array<ArkSteedDoubleRegister, N> TakeDoubleTemporaries() const
    {
        return TakeTemporaries<ArkSteedDoubleRegister, N>();
    }

    template <typename RegisterT, size_t N>
    std::array<RegisterT, N> TakeTemporaries() const
    {
        RegListBase<RegisterT> temporaries = GetTemporaries<RegisterT>();
        ASSERT(temporaries.Count() <= N);

        std::array<RegisterT, N> res;
        for (size_t i = 0; i < N; i++) {
            res[i] = temporaries.First();
            temporaries.PopFirst();
        }
        return res;
    }

    // Template methods for temporaries
    template <typename RegisterT>
    RegListBase<RegisterT> &GetTemporaries()
    {
        if constexpr (std::is_same_v<RegisterT, ArkSteedRegister>) {
            return generalTemporaries_;
        } else {
            return doubleTemporaries_;
        }
    }

    template <typename RegisterT>
    const RegListBase<RegisterT> &GetTemporaries() const
    {
        if constexpr (std::is_same_v<RegisterT, ArkSteedRegister>) {
            return generalTemporaries_;
        } else {
            return doubleTemporaries_;
        }
    }

    template <typename RegisterT>
    void ClearTemporaries()
    {
        if constexpr (std::is_same_v<RegisterT, ArkSteedRegister>) {
            generalTemporaries_.Reset();
        } else {
            doubleTemporaries_.Reset();
        }
    }

protected:
    VertexId id_ = INVALID_VERTEX_ID;
    ArkSteedRegList generalTemporaries_;
    ArkDoubleRegList doubleTemporaries_;
    InputLocation *inputLocations_ = nullptr;
    int inputCount_ = 0;
};

// =============================================================================
// RegallocValueVertexInfo - Extended info for ValueVertices
// =============================================================================

class RegallocValueVertexInfo : public RegallocVertexInfo {
public:
    RegallocValueVertexInfo(Chunk *chunk, int inputCount, MachineRepresentation rep)
        : RegallocVertexInfo(chunk, inputCount), lastUsesNextUseId_(&nextUse_), representation_(rep)
    {
        if (IsDoubleRegister()) {
            doubleRegistersWithResult = ArkDoubleRegList();
        } else {
            registersWithResult = ArkSteedRegList();
        }
    }

    ~RegallocValueVertexInfo() = default;
    NO_COPY_SEMANTIC(RegallocValueVertexInfo);
    NO_MOVE_SEMANTIC(RegallocValueVertexInfo);

    // Next use tracking
    void RecordUse(VertexId useId, InputLocation *inputLocation)
    {
        endId_ = useId;
        *lastUsesNextUseId_ = useId;
        lastUsesNextUseId_ = inputLocation->GetNextUseIdAddress();
    }

    bool HasValidLiveRange() const
    {
        return endId_ != INVALID_VERTEX_ID;
    }

    LiveRange GetLiveRange()
    {
        return LiveRange{id_, endId_};
    }
    VertexId GetEndId() const
    {
        return endId_;
    }

    void AdvanceNextUse(VertexId use)
    {
        nextUse_ = use;
    }
    VertexId CurrentNextUse() const
    {
        return nextUse_;
    }
    bool HasNoMoreUses() const
    {
        return nextUse_ == INVALID_VERTEX_ID;
    }

    // Result location
    ValueLocation &GetResult()
    {
        return result_;
    }
    const ValueLocation &GetResult() const
    {
        return result_;
    }
    void SetResultUnallocated(const UnallocatedState &op)
    {
        result_.SetUnallocated(op);
    }
    void SetResultAllocated(const AllocatedState &op)
    {
        result_.SetAllocated(op);
    }

    // Spill slot
    void SetSpillSlot(const AllocatedState &slot)
    {
        spillSlot_ = slot;
    }
    const InstructionOperand &GetSpillSlot() const
    {
        return spillSlot_;
    }
    bool HasSpillSlot() const
    {
        return !spillSlot_.IsInvalid();
    }
    bool IsSpilled() const
    {
        return spillSlot_.IsAnyStackSlot();
    }
    bool IsLoadable() const
    {
        return IsSpilled() || spillSlot_.IsConstant();
    }
    void SetNoSpill()
    {
        spillSlot_ = AllocatedState();
    }  // to do
    void SetConstantLocation(VertexId vreg)
    {
        spillSlot_ = ConstantOperand(vreg);
    }  // to do
    void Spill(const AllocatedState &location)
    {
        spillSlot_ = location;
    }

    // Register result
    void AddRegister(ArkSteedRegister reg)
    {
        ASSERT(!IsDoubleRegister());
        registersWithResult.Set(reg);
    }
    void AddRegister(ArkSteedDoubleRegister reg)
    {
        ASSERT(IsDoubleRegister());
        doubleRegistersWithResult.Set(reg);
    }
    void RemoveRegister(ArkSteedRegister reg)
    {
        ASSERT(!IsDoubleRegister());
        registersWithResult.Clear(reg);
    }
    void RemoveRegister(ArkSteedDoubleRegister reg)
    {
        ASSERT(IsDoubleRegister());
        doubleRegistersWithResult.Clear(reg);
    }
    bool HasRegisterResult() const
    {
        if (IsDoubleRegister()) {
            return !doubleRegistersWithResult.IsEmpty();
        }
        return !registersWithResult.IsEmpty();
    }

    InstructionOperand GetAllocation() const
    {
        if (HasRegisterResult()) {
            if (IsDoubleRegister()) {
                auto reg = doubleRegistersWithResult.First();
                return AllocatedState(LocationState::LocationKind::REGISTER, GetRepresentation(), reg.Code());
            } else {
                auto reg = registersWithResult.First();
                return AllocatedState(LocationState::LocationKind::REGISTER, GetRepresentation(), reg.Code());
            }
        }
        ASSERT(IsLoadable());
        return spillSlot_;
    }

    bool IsInRegister(ArkSteedRegister reg) const
    {
        ASSERT(!IsDoubleRegister());
        return registersWithResult.Has(reg);
    }
    bool IsInRegister(ArkSteedDoubleRegister reg) const
    {
        ASSERT(IsDoubleRegister());
        return doubleRegistersWithResult.Has(reg);
    }
    const ArkSteedRegList &GetRegisterResult() const
    {
        return registersWithResult;
    }
    const ArkDoubleRegList &GetDoubleRegisterResult() const
    {
        return doubleRegistersWithResult;
    }
    ArkSteedRegList &GetRegisterResult()
    {
        return registersWithResult;
    }
    ArkDoubleRegList &GetDoubleRegisterResult()
    {
        return doubleRegistersWithResult;
    }
    int GetRegisterCount() const
    {
        if (IsDoubleRegister()) {
            return static_cast<int>(doubleRegistersWithResult.Count());
        }
        return static_cast<int>(registersWithResult.Count());
    }

    template <typename T>
    RegListBase<T> ClearRegisters()
    {
        if constexpr (std::is_same_v<T, ArkSteedRegister>) {
            ASSERT(!IsDoubleRegister());
            return std::exchange(registersWithResult, ArkSteedRegList());
        } else {
            ASSERT(IsDoubleRegister());
            return std::exchange(doubleRegistersWithResult, ArkDoubleRegList());
        }
    }

    // Hint
    bool HasHint() const
    {
        return !hint_.IsInvalid();
    }
    void SetHint(const InstructionOperand &hint)
    {
        hint_ = hint;
    }
    void ClearHint()
    {
        hint_ = InstructionOperand();
    }
    const InstructionOperand &GetHint() const
    {
        return hint_;
    }

    MachineRepresentation GetRepresentation() const
    {
        return representation_;
    }
    bool IsDoubleRegister() const
    {
        return representation_ == MachineRepresentation::Float64;
    }

private:
    VertexId endId_ = INVALID_VERTEX_ID;
    VertexId nextUse_ = INVALID_VERTEX_ID;
    // Pointer to the current last use's nextUseId field. Most of the time
    // this will be a pointer to an Input's nextUseId_ field, but it's
    // initialized to this vertex's nextUse_ to track the first use.
    VertexId *lastUsesNextUseId_;
    MachineRepresentation representation_;
    ValueLocation result_;
    InstructionOperand spillSlot_;
    union {
        ArkSteedRegList registersWithResult;
        ArkDoubleRegList doubleRegistersWithResult;
    };
    InstructionOperand hint_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_VERTEX_INFO_H
