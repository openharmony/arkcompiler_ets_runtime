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
#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_thread.h"
#include "libpandabase/macros.h"

#if defined(PANDA_TARGET_AMD64)
#include "ecmascript/compiler/assembler/x64/assembler_x64.h"
#elif defined(PANDA_TARGET_ARM64)
#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"
#endif

namespace panda::ecmascript::arksteed {

class ArkSteedDisassembler;
class Graph;

using Label = panda::ecmascript::Label;

class ArkSteedAssembler;
class ScratchRegisterScope;

// =============================================================================
// Condition - Platform-agnostic condition codes
// =============================================================================

enum class Condition {
    COND_EQUAL,
    COND_NOT_EQUAL,
    COND_LESS_THAN,
    COND_LESS_THAN_OR_EQUAL,
    COND_GREATER_THAN,
    COND_GREATER_THAN_OR_EQUAL,
    COND_ABOVE,
    COND_BELOW,
    COND_ABOVE_OR_EQUAL,
    COND_BELOW_OR_EQUAL,
    COND_ZERO,
    COND_NOT_ZERO,
    COND_OVERFLOW,
    COND_NOT_OVERFLOW
};
inline Condition NegateCondition(Condition cond)
{
    return static_cast<Condition>(static_cast<int>(cond) ^ 1);
}

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

    static constexpr int FRAME_SLOT_SIZE = 8;

    inline MemoryOperand GetStackSlot(const AllocatedState &operand);
    inline MemoryOperand ToMemOperand(const InstructionOperand &operand);

    // Get memory operand for C calling convention stack argument slot (SP-relative)
    inline MemoryOperand GetCallArgSlot(int32_t slotIndex);
    void ReserveCallArgSlots(int32_t slotCount);
    void FreeCallArgSlots(int32_t slotCount);

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

    template <typename Dest, typename Source>
    inline void MoveRepr(MachineRepresentation repr, Dest dst, Source src);

    void LoadTaggedValue(ArkSteedRegister dst, uint64_t taggedValue);

    // =========================================================================
    // Memory Operations
    // =========================================================================

    void LoadField(ArkSteedRegister dst, ArkSteedRegister base, int32_t offset);
    void StoreField(ArkSteedRegister src, ArkSteedRegister base, int32_t offset);
    void LoadActualArgc(ArkSteedRegister dst);

    void LoadFloat64(ArkSteedDoubleRegister dst, MemoryOperand srcOp);

    // =========================================================================
    // Arithmetic Operations
    // =========================================================================

    void Add(ArkSteedRegister dst, ArkSteedRegister src);
    void Add(ArkSteedRegister dst, int32_t immediate);
    void Sub(ArkSteedRegister dst, ArkSteedRegister src);
    void Sub(ArkSteedRegister dst, int32_t immediate);

    // =========================================================================
    // Bitwise Operations
    // =========================================================================

    void Or(ArkSteedRegister dst, int64_t immediate);
    void Or(ArkSteedRegister dst, ArkSteedRegister src);

    // =========================================================================
    // Comparison Operations
    // =========================================================================

    void Compare(ArkSteedRegister lhs, ArkSteedRegister rhs);
    void Compare(ArkSteedRegister lhs, int32_t immediate);

    // =========================================================================
    // Control Flow
    // =========================================================================

    void Jump(Label *target);
    void JumpIf(Condition condition, Label *target);
    void Bind(Label *label);
    inline void Branch(Condition condition, Label *ifTrue, bool fallthroughWhenTrue, Label *ifFalse,
                       bool fallthroughWhenFalse);

    // =========================================================================
    // Call/Return
    // =========================================================================

    void Call(ArkSteedRegister target);
    void Call(Label *target);
    inline void CallRuntime(kungfu::RuntimeStubCSigns::ID runtimeId);
    inline void CallCommonStub(uint32_t stubId);
    void ReturnIfPendingException();
    void Return();

    // =========================================================================
    // Stack Operations
    // =========================================================================

#if defined(PANDA_TARGET_AMD64)
    void Push(ArkSteedRegister reg);
    void Pop(ArkSteedRegister reg);
#elif defined(PANDA_TARGET_ARM64)
    void Push(ArkSteedRegister reg1, ArkSteedRegister reg2);
    void Pop(ArkSteedRegister reg1, ArkSteedRegister reg2);
#endif

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
    void BranchIfNoPendingException(Label* target);

#if defined(PANDA_TARGET_AMD64)
    using PlatformAssembler = x64::AssemblerX64;
#elif defined(PANDA_TARGET_ARM64)
    using PlatformAssembler = aarch64::AssemblerAarch64;
    aarch64::Condition ToPhysicalCondition(Condition condition) const;
#endif

    PlatformAssembler assembler_;
    bool enableComments_ = false;
    bool hasFrame_ = false;
    uint32_t taggedStackSlots_ = 0;
    CommentList comments_;
    [[maybe_unused]] JSThread *compilerThread_;
    JSThread *entryThread_;

    NO_COPY_SEMANTIC(ArkSteedAssembler);
    NO_MOVE_SEMANTIC(ArkSteedAssembler);
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ASSEMBLER_H
