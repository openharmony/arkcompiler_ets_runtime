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

#ifndef ECMASCRIPT_ARKSTEED_FRAMESTATE_H
#define ECMASCRIPT_ARKSTEED_FRAMESTATE_H

#include "ecmascript/arksteed/arksteed_bb.h"
#include "ecmascript/arksteed/arksteed_bytecode_liveness.h"
#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/arksteed/arksteed_vertex.h"
#include "ecmascript/arksteed/arksteed_vreg.h"
#include "ecmascript/mem/chunk_containers.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::arksteed {
class PhiVertex;
class Graph;
class BB;

template <typename T>
class VirtualRegisterArray {
    static_assert(std::is_default_constructible_v<T>);

public:
    VirtualRegisterArray(VRegIDType numLocal, VRegIDType numParams, Chunk *chunk)
    {
        constexpr size_t ELEMENT_SIZE = sizeof(T);  // NOLINT(bugprone-sizeof-expression)
        numLocal_ = numLocal;
        numParams_ = numParams;
        VRegIDType totalSize = arksteed::NumVRegs(numLocal, numParams);
        data_ = reinterpret_cast<T *>(chunk->Allocate(totalSize * ELEMENT_SIZE));
        ASSERT(data_ != nullptr);

        if constexpr (std::is_scalar_v<T>) {
            // Arithmetic types, enum types or pointers
            if (memset_s(data_, totalSize * ELEMENT_SIZE, 0x0, totalSize * ELEMENT_SIZE) != EOK) {
                LOG_COMPILER(FATAL) << "memset failed in VirtualRegisterArray";
            }
        } else {
            // Constructs one-by-one if needed
            for (VRegIDType i = 0; i < totalSize; i++) {
                new (data_ + i) T{};
            }
        }
    }

    ~VirtualRegisterArray() = default;

    NO_COPY_SEMANTIC(VirtualRegisterArray);
    NO_MOVE_SEMANTIC(VirtualRegisterArray);

#define VIRTUAL_REGISTER_GETTER_SETTER(Name, assertExpr, indexExpr, ...) \
    const T &Get##Name(__VA_ARGS__) const                                \
    {                                                                    \
        ASSERT(assertExpr);                                              \
        return data_[indexExpr];                                         \
    }                                                                    \
    void Set##Name(__VA_ARGS__ __VA_OPT__(, ) T newValue)                \
    {                                                                    \
        ASSERT(assertExpr);                                              \
        data_[indexExpr] = std::move(newValue);                          \
    }

    // Get(index), Set(index, newValue)
    // Get(reg),   Set(reg, newValue)
    VIRTUAL_REGISTER_GETTER_SETTER(/* Name */, index < NumVRegs(), index, VRegIDType index)
    VIRTUAL_REGISTER_GETTER_SETTER(/* Name */, reg.GetId() < NumVRegs(), reg.GetId(), VirtualRegister reg)
    // GetLocal(index), SetLocal(index, newValue)
    VIRTUAL_REGISTER_GETTER_SETTER(Local, index < numLocal_, VRegOfLocal(index).GetId(), VRegIDType index)
    // GetParam(index), SetParam(index, newValue)
    VIRTUAL_REGISTER_GETTER_SETTER(Param, index < numParams_, VRegOfParam(numLocal_, index).GetId(), VRegIDType index)
    // GetEnv(), SetEnv(newValue)
    VIRTUAL_REGISTER_GETTER_SETTER(Env, true, VRegOfEnv(numLocal_, numParams_).GetId())
    // GetAcc(), SetAcc(newValue)
    VIRTUAL_REGISTER_GETTER_SETTER(Acc, true, VRegOfAcc(numLocal_, numParams_).GetId())

#undef VIRTUAL_REGISTER_ARRAY_ACCESS_FN

    VRegIDType NumLocalVRegs() const
    {
        return numLocal_;
    }
    VRegIDType NumParamVRegs() const
    {
        return numParams_;
    }
    VRegIDType NumVRegs() const
    {
        return arksteed::NumVRegs(numLocal_, numParams_);
    }

private:
    T *data_ = nullptr;
    VRegIDType numLocal_ = 0;
    VRegIDType numParams_ = 0;
};

class InterpreterFrameState : public VirtualRegisterArray<ValueVertex *> {
public:
    using VirtualRegisterArray<ValueVertex *>::VirtualRegisterArray;
};

class CondensedFrameState {
public:
    // Layout: [Local (filtered out if dead)] [Params] [Env] [Acc (filtered out if dead)]
    CondensedFrameState(const LivenessBitSet *liveness, Chunk *chunk)
        : liveness_(liveness), liveRegisters_(chunk->NewArray<ValueVertex *>(liveness->NumLiveVRegs()))
    {
        numLive_ = liveness->NumLiveVRegs();
        std::fill_n(liveRegisters_, numLive_, nullptr);
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << "CondensedFrameState: numLive = " << numLive_ << " which are " << liveness_->Dump();
#endif
    }

    NO_COPY_SEMANTIC(CondensedFrameState);
    NO_MOVE_SEMANTIC(CondensedFrameState);

// Supported call signature for non-const overloads of ForEach*():
// (1) f(ValueVertex**, VirtualRegister), which supports in-place vertex replacement
// (2) f(ValueVertex*, VirtualRegister)
#define REQUIRES_SIGNATURE                                                                      \
    class = std::enable_if_t < std::is_invocable_v<Function, ValueVertex *, VirtualRegister> || \
            std::is_invocable_v < Function,                                                     \
    ValueVertex **, VirtualRegister >>

// Supported call signature for const overloads of ForEach*():
//     f(const ValueVertex*, VirtualRegister)
#define REQUIRES_SIGNATURE_CONST \
    class = std::enable_if_t<std::is_invocable_v<Function, const ValueVertex *, VirtualRegister>>

    template <typename Function, REQUIRES_SIGNATURE>
    void ForEach(Function &&f)
    {
        VRegIDType nextArrayIndex = 0;
        if constexpr (std::is_invocable_v<Function, ValueVertex **, VirtualRegister>) {
            // (1) f(ValueVertex**, VirtualRegister)
            for (VRegIDType i = 0, n = liveness_->NumVRegs(); i < n; i++) {
                if (liveness_->Test(i)) {
                    std::invoke(f, liveRegisters_ + (nextArrayIndex++), VirtualRegister(i));
                }
            }
        } else {
            // f(ValueVertex*, VirtualRegister)
            for (VRegIDType i = 0, n = liveness_->NumVRegs(); i < n; i++) {
                if (liveness_->Test(i)) {
                    std::invoke(f, liveRegisters_[nextArrayIndex++], VirtualRegister(i));
                }
            }
        }
        ASSERT(nextArrayIndex == numLive_);
    }

    template <typename Function, REQUIRES_SIGNATURE_CONST>
    void ForEach(Function &&f) const
    {
        VRegIDType nextArrayIndex = 0;
        // f(const ValueVertex*, VirtualRegister)
        for (VRegIDType i = 0, n = liveness_->NumVRegs(); i < n; i++) {
            if (liveness_->Test(i)) {
                std::invoke(f, liveRegisters_[nextArrayIndex++], VirtualRegister(i));
            }
        }
        ASSERT(nextArrayIndex == numLive_);
    }

#undef REQUIRES_SIGNATURE
#undef REQUIRES_SIGNATURE_CONST

    const LivenessBitSet *GetLivenessBitSet() const
    {
        return liveness_;
    }

    VRegIDType NumVRegs() const
    {
        return liveness_->NumVRegs();
    }
    VRegIDType NumLocalVRegs() const
    {
        return liveness_->NumLocalVRegs();
    }
    VRegIDType NumParamVRegs() const
    {
        return liveness_->NumParamVRegs();
    }
    VRegIDType NumLiveVRegs() const
    {
        return numLive_;
    }

    VirtualRegister VRegOfEnv() const
    {
        return arksteed::VRegOfEnv(liveness_->NumLocalVRegs(), liveness_->NumParamVRegs());
    }
    VirtualRegister VRegOfAcc() const
    {
        return arksteed::VRegOfAcc(liveness_->NumLocalVRegs(), liveness_->NumParamVRegs());
    }

    bool AccIsLive() const
    {
        return liveness_->TestAcc();
    }

    // GetAcc(), SetAcc(newVertex)
    ValueVertex *GetAcc()
    {
        return (AccIsLive()) ? liveRegisters_[numLive_ - 1] : nullptr;
    }
    const ValueVertex *GetAcc() const
    {
        return (AccIsLive()) ? liveRegisters_[numLive_ - 1] : nullptr;
    }
    void SetAcc(ValueVertex *newVertex)
    {
        ASSERT(AccIsLive());
        liveRegisters_[numLive_ - 1] = newVertex;
    }

    ValueVertex **RawData()
    {
        return liveRegisters_;
    }
    const ValueVertex *const *RawData() const
    {
        return liveRegisters_;
    }

private:
    VRegIDType numLive_ = 0;
    const LivenessBitSet *const liveness_;
    ValueVertex **const liveRegisters_;
};

// =============================================================================
// RegisterMergeInfo - Stores merge information for a register at a merge point
// =============================================================================
struct RegisterMergeInfo {
    // Get operands array pointer
    InstructionOperand *Operands()
    {
        return reinterpret_cast<InstructionOperand *>(this + 1);
    }

    InstructionOperand &Operand(size_t i)
    {
        return Operands()[i];
    }

    ValueVertex *vertex;
};

// =============================================================================
// RegisterState - Simple class with pointer and flags
// =============================================================================
class RegisterState {
public:
    RegisterState() : pointer_(0), isInitialized_(false), isMerge_(false) {}

    bool IsInitialized() const
    {
        return isInitialized_;
    }
    bool IsMerge() const
    {
        return isMerge_;
    }

    void SetValue(ValueVertex *value)
    {
        pointer_ = reinterpret_cast<uintptr_t>(value);
        isInitialized_ = true;
        isMerge_ = false;
    }

    void SetMerge(RegisterMergeInfo *mergeInfo)
    {
        pointer_ = reinterpret_cast<uintptr_t>(mergeInfo);
        isInitialized_ = true;
        isMerge_ = true;
    }

    bool LoadMergeState(ValueVertex **vertex, RegisterMergeInfo **mergeInfo)
    {
        ASSERT(isInitialized_);
        if (isMerge_) {
            *mergeInfo = reinterpret_cast<RegisterMergeInfo *>(pointer_);
            *vertex = (*mergeInfo)->vertex;
            return true;
        }
        *mergeInfo = nullptr;
        *vertex = reinterpret_cast<ValueVertex *>(pointer_);
        return false;
    }

private:
    uintptr_t pointer_;
    bool isInitialized_;
    bool isMerge_;
};

// =============================================================================
// RegisterMergeState - Stores register state at a merge point
// =============================================================================
class RegisterMergeState {
public:
    static constexpr int ALLOCATABLE_GENERAL_REGISTER_COUNT = GetAllocatableGeneralRegisters().Count();
    static constexpr int ALLOCATABLE_DOUBLE_REGISTER_COUNT = GetAllocatableDoubleRegisters().Count();

    bool IsInitialized() const
    {
        return values_[0].IsInitialized();
    }

    template <typename Function>
    void ForEachGeneralRegister(Function &&f)
    {
        RegisterState *currentValue = &values_[0];
        for (ArkSteedRegister reg : GetAllocatableGeneralRegisters()) {
            f(reg, *currentValue);
            ++currentValue;
        }
    }

    template <typename Function>
    void ForEachDoubleRegister(Function &&f)
    {
        RegisterState *currentValue = &doubleValues_[0];
        for (ArkSteedDoubleRegister reg : GetAllocatableDoubleRegisters()) {
            f(reg, *currentValue);
            ++currentValue;
        }
    }

private:
    RegisterState values_[ALLOCATABLE_GENERAL_REGISTER_COUNT];
    RegisterState doubleValues_[ALLOCATABLE_DOUBLE_REGISTER_COUNT];
};

struct LoopEffects {
    explicit LoopEffects(int loopHeader, Chunk *zone)
        : contextSlotWritten(zone), objectsWritten(zone), keysCleared(zone)
    {}
    ChunkVector<uint32_t> contextSlotWritten;
    ChunkVector<ValueVertex *> objectsWritten;
    ChunkVector<uint32_t> keysCleared;
    bool unstableAspectsCleared = false;
    bool mayHaveAliasingContexts = false;

    void Merge(const LoopEffects *other)
    {
        if (!unstableAspectsCleared) {
            unstableAspectsCleared = other->unstableAspectsCleared;
        }
        if (!mayHaveAliasingContexts) {
            mayHaveAliasingContexts = other->mayHaveAliasingContexts;
        }
        contextSlotWritten.insert(contextSlotWritten.end(),
                                  other->contextSlotWritten.begin(),
                                  other->contextSlotWritten.end());
        objectsWritten.insert(objectsWritten.end(), other->objectsWritten.begin(), other->objectsWritten.end());
        keysCleared.insert(keysCleared.end(), other->keysCleared.begin(), other->keysCleared.end());
    }
};

class MergePointFrameState {
public:
    enum class BasicBlockType {
        DEFAULT,
        LOOP_HEADER,
        EXCEPTION_HANDLER_START,
    };

    using BasicBlockTypeBits = common::BitField<BasicBlockType, 0, 2>;  // 2: BasicBlockType bit width
    using IsInlineBits = common::BitField<bool, 2, 1>;  // 2: start bit after BasicBlockType

    MergePointFrameState(uint32_t bcIndex, uint32_t predecessorCount, BasicBlockType type,
                         const LivenessBitSet *liveness, Chunk *chunk);

    static MergePointFrameState *New(uint32_t bcIndex, uint32_t predecessorCount, const LivenessBitSet *liveness,
                                     Chunk *chunk);

    static MergePointFrameState *NewForLoop(uint32_t bcIndex, uint32_t predecessorCount, const LivenessBitSet *liveness,
                                            const LoopInfo *loopInfo, Chunk *chunk);

    static MergePointFrameState *NewForCatchBlock(uint32_t bcIndex, const LivenessBitSet *liveness, Chunk *chunk);

    VRegIDType NumVRegs() const
    {
        return frameState_.NumVRegs();
    }
    VRegIDType NumLocalVRegs() const
    {
        return frameState_.NumLocalVRegs();
    }
    VRegIDType NumParamVRegs() const
    {
        return frameState_.NumParamVRegs();
    }
    VirtualRegister VRegOfEnv() const
    {
        return frameState_.VRegOfEnv();
    }
    VirtualRegister VRegOfAcc() const
    {
        return frameState_.VRegOfAcc();
    }

    void MergeFrom(InterpreterFrameState &srcState, BB *srcPredecessor);

    void ClearIsLoop()
    {
        bitfield_ = BasicBlockTypeBits::Update(bitfield_, BasicBlockType::DEFAULT);
    }

    bool HasPhi() const
    {
        return !phis_.empty();
    }
    const ChunkVector<PhiVertex *> &Phis() const
    {
        return phis_;
    }
    ChunkVector<PhiVertex *> &Phis()
    {
        return phis_;
    }

    uint32_t PredecessorCount() const
    {
        return static_cast<uint32_t>(predecessors_.size());
    }
    uint32_t PredecessorsSoFar() const
    {
        return predecessorsSoFar_;
    }

    Span<BB *> Predecessors()
    {
        return {predecessors_.data(), predecessors_.size()};
    }
    Span<const BB *const> Predecessors() const
    {
        return {predecessors_.data(), predecessors_.size()};
    }

    BB *PredecessorAt(int i) const
    {
        return predecessors_[i];
    }
    void SetPredecessorAt(int i, BB *val)
    {
        predecessors_[i] = val;
    }

    BasicBlockType GetBasicBlockType() const
    {
        return BasicBlockTypeBits::Decode(bitfield_);
    }

    bool IsLoopHeader() const
    {
        return GetBasicBlockType() == BasicBlockType::LOOP_HEADER;
    }

    bool IsExceptionHandler() const
    {
        return GetBasicBlockType() == BasicBlockType::EXCEPTION_HANDLER_START;
    }

    bool IsInline() const
    {
        return IsInlineBits::Decode(bitfield_);
    }

    uint32_t BytecodeIndex() const
    {
        return bcIndex_;
    }

    const LoopInfo *GetLoopInfo() const
    {
        return loopInfo_;
    }
    void ClearLoopInfo()
    {
        loopInfo_ = nullptr;
    }
    void SetLoopEffects(LoopEffects *loopEffects)
    {
        loopEffects_ = loopEffects;
    }
    const LoopEffects *GetLoopEffects() const
    {
        return loopEffects_;
    }

    const CondensedFrameState &FrameState() const
    {
        return frameState_;
    }
    CondensedFrameState &FrameState()
    {
        return frameState_;
    }

    RegisterMergeState &RegisterState()
    {
        return registerState_;
    }

private:
    void MergePhisFrom(InterpreterFrameState &srcState);
    // Merge a single value from the unmerged state
    ValueVertex *MergeValue(VirtualRegister reg, ValueVertex *destValue, ValueVertex *srcValue);

    ValueVertex *NewLoopPhi(VirtualRegister reg);
    ValueVertex *NewExceptionPhi(VirtualRegister reg);

    // to do: Debug only. To be removed
    std::string VRegDisplayString(VirtualRegister vreg) const
    {
        return arksteed::VRegDisplayString(vreg, NumLocalVRegs(), NumParamVRegs());
    }

    uint32_t bcIndex_;
    uint32_t predecessorsSoFar_;
    uint32_t bitfield_;
    ChunkVector<BB *> predecessors_;
    ChunkVector<PhiVertex *> phis_;
    CondensedFrameState frameState_;
    RegisterMergeState registerState_;
    const LoopInfo *loopInfo_ = nullptr;
    const LoopEffects *loopEffects_ = nullptr;
    Chunk *chunk_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_FRAMESTATE_H
