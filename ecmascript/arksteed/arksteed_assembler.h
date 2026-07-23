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

#ifndef ECMASCRIPT_ARKSTEED_ASSEMBLER_H
#define ECMASCRIPT_ARKSTEED_ASSEMBLER_H

#include "ecmascript/arksteed/arksteed_comment.h"
#include "ecmascript/arksteed/arksteed_condition_code.h"
#include "ecmascript/arksteed/arksteed_deopt_helper.h"
#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_thread.h"
#include "libpandabase/macros.h"

#if defined(PANDA_TARGET_AMD64)
#include "ecmascript/compiler/assembler/x64/assembler_x64.h"
#elif defined(PANDA_TARGET_ARM64)
#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"
#include "ecmascript/mem/chunk_containers.h"
#endif

namespace panda::ecmascript::arksteed {

class ArkSteedDisassembler;
class Graph;

using Label = panda::ecmascript::Label;

class ArkSteedAssembler;
class TemporaryRegisterScope;

#if defined(PANDA_TARGET_AMD64)
constexpr x64::Register X64_SCRATCH_REGISTER = ARKSTEED_EAGER_DEOPT_ENTRY_TARGET_REGISTER;
constexpr x64::DoubleRegister X64_SCRATCH_DOUBLE_REGISTER = x64::xmm15;
static_assert(!GetAllocatableGeneralRegisters().Has(X64_SCRATCH_REGISTER));
static_assert(!GetAllocatableDoubleRegisters().Has(X64_SCRATCH_DOUBLE_REGISTER));
#elif defined(PANDA_TARGET_ARM64)
constexpr aarch64::Register kScratchRegister = ARKSTEED_EAGER_DEOPT_ENTRY_TARGET_REGISTER;
constexpr aarch64::Register kScratchRegister2 = ARKSTEED_EAGER_DEOPT_ENTRY_GLUE_REGISTER;
constexpr aarch64::DoubleRegister kScratchDoubleRegister = aarch64::d30;
constexpr aarch64::DoubleRegister kScratchDoubleRegister2 = aarch64::d31;
// x16 carries the shared-entry address, x17 carries the current glue and lr carries the JIT continuation PC.
static_assert(!GetAllocatableGeneralRegisters().Has(kScratchRegister));
static_assert(!GetAllocatableGeneralRegisters().Has(kScratchRegister2));
static_assert(!GetAllocatableDoubleRegisters().Has(kScratchDoubleRegister));
static_assert(!GetAllocatableDoubleRegisters().Has(kScratchDoubleRegister2));
#endif

// =============================================================================
// ArkSteedAssembler - Platform-agnostic assembler interface
// =============================================================================

class ArkSteedAssembler {
public:
    ArkSteedAssembler(Chunk *chunk, JSThread *compilerThread, JSThread *entryThread);
    ~ArkSteedAssembler();

    // =========================================================================
    // Register Move Operations
    // =========================================================================

#if defined(PANDA_TARGET_AMD64)
    using MemoryOperand = x64::Operand;
    static constexpr int NUM_ARG_REGISTERS = 6;
#elif defined(PANDA_TARGET_ARM64)
    using MemoryOperand = aarch64::MemoryOperand;
    static constexpr int NUM_ARG_REGISTERS = 8;
#endif
    static constexpr ArkSteedRegister GetParameterRegister(int i);
    inline void LoadGlue(ArkSteedRegister dst);

    static constexpr int FRAME_SLOT_SIZE = 8;

    inline MemoryOperand GetStackSlot(const AllocatedState &operand);
    inline MemoryOperand ToMemOperand(const InstructionOperand &operand);

    // Get memory operand for C calling convention stack argument slot (SP-relative)
    inline MemoryOperand GetCallArgSlot(int32_t slotIndex);
    void ReserveCallArgSlots(int32_t slotCount);
    void FreeCallArgSlots(int32_t slotCount);
    void ReserveCallArgSlots(ArkSteedRegister slotCount);
    void FreeCallArgSlots(ArkSteedRegister slotCount);
    void RestoreStackPointerToFrameBottom(Graph *graph);
    void PushUndefinedForSteedCall(ArkSteedRegister fillSlotCount, uint32_t userArgc);
    void PrepareSteedCalleeContext(ArkSteedRegister target, ArkSteedRegister codeEntry);

    inline int32_t GetFramePointerOffsetForStackSlot(int32_t slotIndex, MachineRepresentation rep) const
    {
        // to do: refactor
        constexpr int32_t kTaggedSlot0OffsetFromFp = -4 * FRAME_SLOT_SIZE;  // -4: 4 slots below fp for frame header
        if (rep == MachineRepresentation::Tagged) {
            return kTaggedSlot0OffsetFromFp - slotIndex * FRAME_SLOT_SIZE;
        } else {
            const int32_t kUntaggedSlot0OffsetFromFp =
                kTaggedSlot0OffsetFromFp - static_cast<int32_t>(taggedStackSlots_) * FRAME_SLOT_SIZE;
            return kUntaggedSlot0OffsetFromFp - slotIndex * FRAME_SLOT_SIZE;
        }
    }

    void Move(ArkSteedRegister dst, ArkSteedRegister src);
    void Move(ArkSteedRegister dst, int32_t immediate);
    void Move(ArkSteedRegister dst, int64_t immediate);
    void Move(ArkSteedRegister dst, uint64_t immediate);
    void Move(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src);
    void Move(ArkSteedDoubleRegister dst, double immediate);
    void Move(ArkSteedDoubleRegister dst, double immediate, ArkSteedRegister scratch);
    void Move(ArkSteedDoubleRegister dst, ArkSteedRegister src);
    void Move(ArkSteedRegister dst, ArkSteedDoubleRegister src);

    template <typename Dest, typename Source>
    inline void MoveRepr(MachineRepresentation repr, Dest dst, Source src);

    void LoadTaggedValue(ArkSteedRegister dst, uint64_t taggedValue);

    // =========================================================================
    // Memory Operations
    // =========================================================================

    void LoadField(ArkSteedRegister dst, ArkSteedRegister base, int32_t offset);
    void LoadInt32Field(ArkSteedRegister dst, ArkSteedRegister base, int32_t offset);
    void StoreField(ArkSteedRegister src, ArkSteedRegister base, int32_t offset);
    void StoreInt32Field(ArkSteedRegister src, ArkSteedRegister base, int32_t offset);
    void StoreInt32FieldRelease(ArkSteedRegister src, ArkSteedRegister base, int32_t offset);
    void StoreFloat64Field(ArkSteedDoubleRegister src, ArkSteedRegister base, int32_t offset);
    void LoadActualArgc(ArkSteedRegister dst);

    void LoadFloat64(ArkSteedDoubleRegister dst, MemoryOperand srcOp);
    void StoreFloat64(MemoryOperand dstOp, ArkSteedDoubleRegister src);
    void StoreFloat64Constant(MemoryOperand dstOp, double immediate, ArkSteedRegister scratchGPR,
                              ArkSteedDoubleRegister scratchFPR);
    void ConvertInt32ToDouble(ArkSteedDoubleRegister dst, ArkSteedRegister src);

    // =========================================================================
    // Arithmetic Operations
    // =========================================================================

    void Add(ArkSteedRegister dst, ArkSteedRegister src);
    void Add(ArkSteedRegister dst, int32_t immediate);
    void Add(ArkSteedRegister dst, int64_t immediate);
    void Sub(ArkSteedRegister dst, ArkSteedRegister src);
    void Sub(ArkSteedRegister dst, int32_t immediate);
    void SignExtendInt32ToInt64(ArkSteedRegister dst, ArkSteedRegister src);
    void Int32Add(ArkSteedRegister dst, ArkSteedRegister left, ArkSteedRegister right);
    void Int32Sub(ArkSteedRegister dst, ArkSteedRegister left, ArkSteedRegister right);
    void Int32Mul(ArkSteedRegister dst, ArkSteedRegister src);
    void Int32MulWide(ArkSteedRegister dst, ArkSteedRegister left, ArkSteedRegister right);
    void Int32MulHigh(ArkSteedRegister dst, ArkSteedRegister left, ArkSteedRegister right);
    void Int32Div(ArkSteedRegister dst, ArkSteedRegister dividend, ArkSteedRegister divisor);
    void Int32DivAndRemainder(ArkSteedRegister quotient, ArkSteedRegister remainder,
                              ArkSteedRegister dividend, ArkSteedRegister divisor);
    void PositiveInt32Mod(ArkSteedRegister dst, ArkSteedRegister dividend, ArkSteedRegister divisor);
    void Int32ToFloat64(ArkSteedDoubleRegister dst, ArkSteedRegister src);
    void Float64Add(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src);
    void Float64Sub(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src);
    void Float64Mul(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src);
    void Float64Div(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src);
    void Float64Neg(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src);
    void CompareFloat64(ArkSteedDoubleRegister left, ArkSteedDoubleRegister right);
    void TruncateFloat64ToInt32(ArkSteedRegister dst, ArkSteedDoubleRegister src);
    void Word64And(ArkSteedRegister dst, ArkSteedRegister src);

    // =========================================================================
    // Bitwise Operations
    // =========================================================================

    void Or(ArkSteedRegister dst, int32_t immediate);
    void Or(ArkSteedRegister dst, int64_t immediate);
    void Or(ArkSteedRegister dst, ArkSteedRegister src);
    void And(ArkSteedRegister dst, int32_t immediate);
    void And(ArkSteedRegister dst, int64_t immediate);
    void And(ArkSteedRegister dst, ArkSteedRegister src);
    void Lsr(ArkSteedRegister dst, uint32_t shift);
    void ShiftRightLogical(ArkSteedRegister dst, uint32_t shift);
    void ShiftRightLogical32(ArkSteedRegister dst, uint32_t shift);
    void MoveBitMask32(ArkSteedRegister dst, ArkSteedRegister bitIndex);
    void Int32Neg(ArkSteedRegister dst, ArkSteedRegister src);
    void Int32Inc(ArkSteedRegister dst, ArkSteedRegister src);
    void Int32Dec(ArkSteedRegister dst, ArkSteedRegister src);
    void Int32BNot(ArkSteedRegister dst);
    void Int32And(ArkSteedRegister dst, ArkSteedRegister src);
    void Int32And(ArkSteedRegister dst, int32_t immediate);
    void Int32Or(ArkSteedRegister dst, ArkSteedRegister src);
    void Int32Or(ArkSteedRegister dst, int32_t immediate);
    void Int32Xor(ArkSteedRegister dst, ArkSteedRegister src);
    void Int32Xor(ArkSteedRegister dst, int32_t immediate);
    void Int32ShiftLeft(ArkSteedRegister dst, uint32_t shift);
    void Int32ShiftLeftByRegister(ArkSteedRegister dst, ArkSteedRegister shift);
    void Int32ShiftRightLogical(ArkSteedRegister dst, uint32_t shift);
    void Int32ShiftRightLogicalByRegister(ArkSteedRegister dst, ArkSteedRegister shift);
    void Int32ShiftRightArithmetic(ArkSteedRegister dst, uint32_t shift);
    void Int32ShiftRightArithmeticByRegister(ArkSteedRegister dst, ArkSteedRegister shift);

    // =========================================================================
    // Comparison Operations
    // =========================================================================

    void Compare(ArkSteedRegister lhs, ArkSteedRegister rhs);
    void CompareInt32(ArkSteedRegister lhs, ArkSteedRegister rhs);
    void Compare(ArkSteedRegister lhs, int32_t immediate);
    void Compare(ArkSteedRegister lhs, int64_t immediate);
    void CompareInt32(ArkSteedRegister lhs, int32_t immediate);
    void CompareField(ArkSteedRegister base, int32_t offset, ArkSteedRegister rhs);

    // =========================================================================
    // Control Flow
    // =========================================================================

    void Jump(Label *target);
#if defined(PANDA_TARGET_AMD64)
    void Jump(ArkSteedRegister target);
#endif
    void JumpIf(Condition condition, Label *target);
    void JumpIfNotTaggedHeapObject(ArkSteedRegister value, Label *target);
    void JumpIfNotJSFunction(ArkSteedRegister value, Label *target);
    void JumpIfClassConstructor(ArkSteedRegister jsFunc, Label *target);
    void JumpIfFunctionNotCompiled(ArkSteedRegister jsFunc, Label *target);
    void Bind(Label *label);
#if defined(PANDA_TARGET_ARM64)
    void CheckVeneerPool(bool precedingCodeCanFallThrough, size_t protectedCodeSize = 0U);
    void FinalizeVeneers();
#endif
    inline void Branch(Condition condition, Label *ifTrue, bool fallthroughWhenTrue, Label *ifFalse,
                       bool fallthroughWhenFalse);

    // =========================================================================
    // Call/Return
    // =========================================================================

    void Call(ArkSteedRegister target);
    void Call(Label *target);
    inline void CallRuntime(kungfu::RuntimeStubCSigns::ID runtimeId);
    inline void CallTrampoline(kungfu::RuntimeStubCSigns::ID stubId);
    inline void CallNGCRuntime(kungfu::RuntimeStubCSigns::ID runtimeId);
    inline void CallCommonStub(uint32_t stubId);
    void ReturnWithPendingException();
    void ReturnIfPendingException();
    // Branch to target if no pending exception exists in JSThread.
    void BranchIfNoPendingException(Label* target);
    void LoadAndClearPendingException(ArkSteedRegister dst, ArkSteedRegister glue);
    void Return();

    // =========================================================================
    // Deoptimization
    // =========================================================================

    void CallDeoptHandler(kungfu::DeoptType deoptType);
    // Restores the normal ArkSteed cold-block link/alignment state before the GC-capable overflow path.
    void NormalizeEagerDeoptOverflowLink();
    // Calls the global no-GC entry while preserving the fixed-exit return PC.
    void CallArkSteedDeoptimizationEntry();

    // =========================================================================
    // Stack Operations
    // =========================================================================

    void Push(ArkSteedRegister reg);
    void Pop(ArkSteedRegister reg);
#if defined(PANDA_TARGET_ARM64)
    void Push(ArkSteedRegister reg1, ArkSteedRegister reg2);
    void Pop(ArkSteedRegister reg1, ArkSteedRegister reg2);
#endif
    void Push(ArkSteedDoubleRegister reg);
    void Pop(ArkSteedDoubleRegister reg);

    // =========================================================================
    // Function Prologue/Epilogue
    // =========================================================================

    void Prologue(Graph *graph);
    void Epilogue();

    // =========================================================================
    // Code Access
    // =========================================================================

    uint8_t *GetCodeBuffer();
    size_t GetCodeSize();
    uint32_t GetPcOffset() const
    {
        return static_cast<uint32_t>(assembler_.GetCurrentPosition());
    }

    void EnableComments(bool enable)
    {
        enableComments_ = enable;
    }
    void RecordComment(const char *str);
    bool IsCommentEnabled() const
    {
        return enableComments_;
    }
    CommentList &GetCommentList()
    {
        return comments_;
    }

    void Disassemble(std::ostream &os, const uint8_t *commentsData, uint32_t commentsSize);

    void SetHasFrame(bool hasFrame)
    {
        hasFrame_ = hasFrame;
    }

    void EnableCodeSign()
    {
        assembler_.SetDoCodeSign();
    }

private:
    friend class TemporaryRegisterScope;

#if defined(PANDA_TARGET_AMD64)
    using PlatformAssembler = x64::AssemblerX64;
#elif defined(PANDA_TARGET_ARM64)
    using PlatformAssembler = aarch64::AssemblerAarch64;
    aarch64::Condition ToPhysicalCondition(Condition condition) const;
    aarch64::MemoryOperand MaterializeAddress(const aarch64::MemoryOperand &operand);
    void LoadRegisterWithOperand(const aarch64::Register &dst, const aarch64::MemoryOperand &src);
    void StoreRegisterWithOperand(const aarch64::Register &src, const aarch64::MemoryOperand &dst);
    static constexpr uint32_t VENEER_INSTRUCTION_SIZE = sizeof(uint32_t);  // One ARM64 instruction is 4 bytes.
    static constexpr uint32_t VENEER_DISTANCE_MARGIN = 4U * 1024U;  // Check 4 KiB before the encoding limit.
    static bool IsVeneerBranchOrCall(uint32_t instruction);
    static bool IsVeneerConditionOrCompareBranch(uint32_t instruction);
    static bool IsVeneerTestBranch(uint32_t instruction);
    bool IsVeneerBranchInRange(uint32_t instruction, int64_t displacement) const;
    uint32_t GetVeneerBranchDeadline(uint32_t branchPc, uint32_t instruction) const;
    void UpdateVeneerPoolCheck();
    void RecordVeneerBranch(uint32_t branchPc, Label *target);
    void PatchVeneerBranchTarget(uint32_t branchPc, uint32_t targetPc);
    void BindVeneerLabel(Label *label);
    void TestAndBranchIfZero(ArkSteedRegister value, int32_t bit, Label *target);
    void TestAndBranchIfNotZero(ArkSteedRegister value, int32_t bit, Label *target);
#endif

    PlatformAssembler assembler_;
#if defined(PANDA_TARGET_ARM64)
    Chunk *chunk_;
    ChunkMap<Label *, ChunkVector<uint32_t>> veneerBranches_;
    uint32_t nextVeneerPoolCheck_ {UINT32_MAX};  // UINT32_MAX means that no pool check is pending.
    bool emittingVeneerPool_ {false};
#endif
    bool enableComments_ = false;
    bool hasFrame_ = false;
    uint32_t taggedStackSlots_ = 0;
    TemporaryRegisterScope *temporaryRegisterScope_ = nullptr;
    CommentList comments_;
    [[maybe_unused]] JSThread *compilerThread_;
    JSThread *entryThread_;

    NO_COPY_SEMANTIC(ArkSteedAssembler);
    NO_MOVE_SEMANTIC(ArkSteedAssembler);
};

class TemporaryRegisterScope {
public:
    explicit TemporaryRegisterScope(ArkSteedAssembler *assembler) : assembler_(assembler)
    {
        previous_ = assembler_->temporaryRegisterScope_;
        if (previous_ != nullptr) {
            availableTemporaryGPRs_ = previous_->availableTemporaryGPRs_;
            availableTemporaryFPRs_ = previous_->availableTemporaryFPRs_;
            requiredSpecificGPRs_ = previous_->requiredSpecificGPRs_;
            requiredSpecificFPRs_ = previous_->requiredSpecificFPRs_;
            availableScratchGPRs_ = previous_->availableScratchGPRs_;
            availableScratchFPRs_ = previous_->availableScratchFPRs_;
        } else {
#if defined(PANDA_TARGET_AMD64)
            availableScratchGPRs_.Set(X64_SCRATCH_REGISTER);
            availableScratchFPRs_.Set(X64_SCRATCH_DOUBLE_REGISTER);
#elif defined(PANDA_TARGET_ARM64)
            availableScratchGPRs_.Set(kScratchRegister);
            availableScratchGPRs_.Set(kScratchRegister2);
            availableScratchFPRs_.Set(kScratchDoubleRegister);
            availableScratchFPRs_.Set(kScratchDoubleRegister2);
#endif
        }
        assembler_->temporaryRegisterScope_ = this;
    }

    ~TemporaryRegisterScope()
    {
        ASSERT(assembler_->temporaryRegisterScope_ == this);
        assembler_->temporaryRegisterScope_ = previous_;
    }

    NO_COPY_SEMANTIC(TemporaryRegisterScope);
    NO_MOVE_SEMANTIC(TemporaryRegisterScope);

    void Include(const ArkSteedRegList &registers)
    {
        ASSERT((registers - GetAllocatableGeneralRegisters()).IsEmpty());
        availableTemporaryGPRs_ |= registers;
    }

    void IncludeDouble(const ArkDoubleRegList &registers)
    {
        ASSERT((registers - GetAllocatableDoubleRegisters()).IsEmpty());
        availableTemporaryFPRs_ |= registers;
    }

    void IncludeSpecific(const ArkSteedRegList &registers)
    {
        ASSERT((registers - GetAllocatableGeneralRegisters()).IsEmpty());
        requiredSpecificGPRs_ |= registers;
    }

    void IncludeSpecificDouble(const ArkDoubleRegList &registers)
    {
        ASSERT((registers - GetAllocatableDoubleRegisters()).IsEmpty());
        requiredSpecificFPRs_ |= registers;
    }

    ArkSteedRegister Acquire()
    {
        if (availableTemporaryGPRs_.IsEmpty()) {
            LOG_JIT(FATAL) << "RA temporary GPR pool exhausted";
        }
        return availableTemporaryGPRs_.PopFirst();
    }

    ArkSteedDoubleRegister AcquireDouble()
    {
        if (availableTemporaryFPRs_.IsEmpty()) {
            LOG_JIT(FATAL) << "RA temporary FPR pool exhausted";
        }
        return availableTemporaryFPRs_.PopFirst();
    }

    ArkSteedRegister AcquireSpecific(ArkSteedRegister reg) const
    {
        if (!requiredSpecificGPRs_.Has(reg)) {
            LOG_JIT(FATAL) << "Undeclared specific GPR temporary: " << static_cast<int32_t>(reg.Code());
        }
        return reg;
    }

    ArkSteedDoubleRegister AcquireSpecificDouble(ArkSteedDoubleRegister reg) const
    {
        if (!requiredSpecificFPRs_.Has(reg)) {
            LOG_JIT(FATAL) << "Undeclared specific FPR temporary: " << static_cast<int32_t>(reg.Code());
        }
        return reg;
    }

    ArkSteedRegister AcquireScratch()
    {
        if (availableScratchGPRs_.IsEmpty()) {
            LOG_JIT(FATAL) << "Architecture scratch GPR pool exhausted";
        }
        return availableScratchGPRs_.PopFirst();
    }

    ArkSteedDoubleRegister AcquireDoubleScratch()
    {
        if (availableScratchFPRs_.IsEmpty()) {
            LOG_JIT(FATAL) << "Architecture scratch FPR pool exhausted";
        }
        return availableScratchFPRs_.PopFirst();
    }

private:
    ArkSteedAssembler *assembler_;
    TemporaryRegisterScope *previous_ = nullptr;
    ArkSteedRegList availableTemporaryGPRs_;
    ArkDoubleRegList availableTemporaryFPRs_;
    ArkSteedRegList requiredSpecificGPRs_;
    ArkDoubleRegList requiredSpecificFPRs_;
    ArkSteedRegList availableScratchGPRs_;
    ArkDoubleRegList availableScratchFPRs_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ASSEMBLER_H
