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

#include "ecmascript/arksteed/arch/x64/arksteed_assembler_x64-inl.h"
#include "ecmascript/arksteed/arksteed_assembler.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::arksteed {
#if defined(PANDA_TARGET_AMD64)
// =============================================================================
// Register Move Operations
// =============================================================================

void ArkSteedAssembler::Move(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Movq(src, dst);
}

void ArkSteedAssembler::Move(ArkSteedRegister dst, int32_t immediate)
{
    assembler_.Movq(x64::Immediate(immediate), dst);
}

void ArkSteedAssembler::Move(ArkSteedRegister dst, int64_t immediate)
{
    assembler_.Movabs(static_cast<uint64_t>(immediate), dst);
}

void ArkSteedAssembler::Move(ArkSteedRegister dst, uint64_t immediate)
{
    assembler_.Movabs(immediate, dst);
}

void ArkSteedAssembler::Move(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Movsd(dst, src);
}

void ArkSteedAssembler::Move(ArkSteedDoubleRegister dst, double immediate)
{
    // Convert double to its 64-bit bit representation
    uint64_t bits = 0;
    static_assert(sizeof(double) == sizeof(uint64_t), "double must be 64 bits");
    if (memcpy_s(&bits, sizeof(double), &immediate, sizeof(double)) != EOK) {
        LOG_JIT(FATAL) << "memcpy failed in Move";
    }
    // Use scratch register to hold the 64-bit value
    ScratchRegisterScope scope;
    ArkSteedRegister scratch = scope.AcquireScratch();
    // Load the 64-bit value to scratch register
    assembler_.Movabs(bits, scratch);
    // Move from GP register to XMM register
    assembler_.Movq(dst, scratch);
}

// =============================================================================
// Memory Operations
// =============================================================================

void ArkSteedAssembler::LoadField(ArkSteedRegister dst, ArkSteedRegister base, int32_t offset)
{
    x64::Operand operand(base, offset);
    assembler_.Movq(operand, dst);
}

void ArkSteedAssembler::StoreField(ArkSteedRegister src, ArkSteedRegister base, int32_t offset)
{
    x64::Operand operand(base, offset);
    assembler_.Movq(src, operand);
}

void ArkSteedAssembler::LoadActualArgc(ArkSteedRegister dst)
{
    // 2: argc is stored at fp + 2 * FRAME_SLOT_SIZE
    x64::Operand operand(kFramePointerRegister, 2 * FRAME_SLOT_SIZE);
    assembler_.Movq(operand, dst);
}

void ArkSteedAssembler::LoadFloat64(ArkSteedDoubleRegister dst, MemoryOperand srcOp)
{
    assembler_.Movsd(dst, srcOp);
}

// =============================================================================
// Arithmetic Operations
// =============================================================================

void ArkSteedAssembler::Add(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Addq(src, dst);
}

void ArkSteedAssembler::Add(ArkSteedRegister dst, int32_t immediate)
{
    assembler_.Addq(x64::Immediate(immediate), dst);
}

void ArkSteedAssembler::Sub(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Subq(src, dst);
}

void ArkSteedAssembler::Sub(ArkSteedRegister dst, int32_t immediate)
{
    assembler_.Subq(x64::Immediate(immediate), dst);
}

// =============================================================================
// Bitwise Operations
// =============================================================================

void ArkSteedAssembler::Or(ArkSteedRegister dst, int64_t immediate)
{
    // For 64-bit immediate, use movabs to a temp register then orq
    // For simplicity, handle common case where immediate fits in 32 bits
    if (immediate >= INT32_MIN && immediate <= INT32_MAX) {
        assembler_.Or(x64::Immediate(static_cast<int32_t>(immediate)), dst);
    } else {
        // Use temp register for 64-bit immediate
        ScratchRegisterScope scope;
        auto scratch = scope.AcquireScratch();
        assembler_.Movabs(static_cast<uint64_t>(immediate), scratch);
        assembler_.Orq(scratch, dst);
    }
}

void ArkSteedAssembler::Or(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Orq(src, dst);
}

// =============================================================================
// Comparison Operations
// =============================================================================

void ArkSteedAssembler::Compare(ArkSteedRegister lhs, ArkSteedRegister rhs)
{
    assembler_.Cmpq(rhs, lhs);
}

void ArkSteedAssembler::Compare(ArkSteedRegister lhs, int32_t immediate)
{
    assembler_.Cmpq(x64::Immediate(immediate), lhs);
}

// =============================================================================
// Control Flow
// =============================================================================

void ArkSteedAssembler::Jump(Label *target)
{
    assembler_.Jmp(target);
}

void ArkSteedAssembler::JumpIf(Condition condition, Label *target)
{
    switch (condition) {
        case Condition::COND_EQUAL:
            assembler_.Je(target);
            break;
        case Condition::COND_NOT_EQUAL:
            assembler_.Jne(target);
            break;
        case Condition::COND_LESS_THAN:
            assembler_.Jb(target);
            break;
        case Condition::COND_LESS_THAN_OR_EQUAL:
            assembler_.Jle(target);
            break;
        case Condition::COND_GREATER_THAN:
            assembler_.Jg(target);
            break;
        case Condition::COND_GREATER_THAN_OR_EQUAL:
            assembler_.Jge(target);
            break;
        case Condition::COND_ZERO:
            assembler_.Jz(target);
            break;
        case Condition::COND_NOT_ZERO:
            assembler_.Jnz(target);
            break;
        case Condition::COND_ABOVE:
            assembler_.Ja(target);
            break;
        case Condition::COND_BELOW:
            assembler_.Jb(target);
            break;
        case Condition::COND_ABOVE_OR_EQUAL:
            assembler_.Jae(target);
            break;
        case Condition::COND_BELOW_OR_EQUAL:
            assembler_.Jbe(target);
            break;
        case Condition::COND_OVERFLOW:
        case Condition::COND_NOT_OVERFLOW:
            UNREACHABLE();
            break;
        default:
            UNREACHABLE();
    }
}

void ArkSteedAssembler::BranchIfNoPendingException(Label *target)
{
    ArkSteedRegister scratch = X64_SCRATCH_REGISTER;
    assembler_.Push(x64::rax);
    assembler_.Movabs(static_cast<uint64_t>(entryThread_->GetGlueAddr()), x64::rax);
    LoadField(scratch, x64::rax, static_cast<int32_t>(JSThread::GlueData::GetExceptionOffset(false)));
    assembler_.Movabs(JSTaggedValue::Hole().GetRawData(), x64::rax);
    assembler_.Cmpq(scratch, x64::rax);
    assembler_.Pop(x64::rax);
    assembler_.Je(target);
}

void ArkSteedAssembler::ReturnWithPendingException()
{
    assembler_.Movabs(JSTaggedValue::Exception().GetRawData(), x64::rax);
    Epilogue();
    Return();
}

void ArkSteedAssembler::ReturnIfPendingException()
{
    Label noPendingException;
    BranchIfNoPendingException(&noPendingException);
    ReturnWithPendingException();
    Bind(&noPendingException);
}

void ArkSteedAssembler::LoadAndClearPendingException(ArkSteedRegister dst, ArkSteedRegister glue)
{
    ArkSteedRegister scratch = X64_SCRATCH_REGISTER;
    size_t offset = JSThread::GlueData::GetExceptionOffset(false);  // false : isArch32 = false

    LoadField(dst, glue, static_cast<int32_t>(offset));
    assembler_.Movabs(JSTaggedValue::Hole().GetRawData(), scratch);
    StoreField(scratch, glue, static_cast<int32_t>(offset));
}

void ArkSteedAssembler::Bind(Label *label)
{
    assembler_.Bind(label);
}

// =============================================================================
// Call/Return
// =============================================================================

void ArkSteedAssembler::Call(ArkSteedRegister target)
{
    assembler_.Callq(target);
}

void ArkSteedAssembler::Call(Label *target)
{
    assembler_.Callq(target);
}

void ArkSteedAssembler::Return()
{
    assembler_.Ret();
}

// =============================================================================
// Stack Operations
// =============================================================================

void ArkSteedAssembler::Push(ArkSteedRegister reg)
{
    assembler_.Pushq(reg);
}

void ArkSteedAssembler::Pop(ArkSteedRegister reg)
{
    assembler_.Popq(reg);
}

void ArkSteedAssembler::ReserveCallArgSlots(int32_t slotCount)
{
    if (slotCount > 0) {
        assembler_.Subq(x64::Immediate(slotCount * FRAME_SLOT_SIZE), x64::rsp);
    }
}

void ArkSteedAssembler::FreeCallArgSlots(int32_t slotCount)
{
    if (slotCount > 0) {
        assembler_.Addq(x64::Immediate(slotCount * FRAME_SLOT_SIZE), x64::rsp);
    }
}

// =============================================================================
// Function Prologue/Epilogue
// =============================================================================

void ArkSteedAssembler::Prologue(Graph *graph)
{
    RecordComment("Prologue");

    // 1. Set up fp.
    assembler_.Pushq(x64::rbp);
    assembler_.Movq(x64::rsp, x64::rbp);

    // 2. Build the SteedFunctionFrame fixed header:
    // [rbp - 8] = frameType, [rbp - 16] = jsFunc, [rbp - 24] = lexicalEnv.
    assembler_.Pushq(x64::Immediate(static_cast<int>(FrameType::STEED_FUNCTION_FRAME)));
    assembler_.Pushq(x64::r12);  // jsFunc
    assembler_.Pushq(x64::rbx);  // lexicalEnv

    // 3. Initialize tagged slots for GC safety.
    uint32_t taggedSlots = graph->GetTaggedStackSlots();
    taggedStackSlots_ = taggedSlots;
    if (taggedSlots > 0) {
        auto undefined = static_cast<int64_t>(JSTaggedValue::VALUE_UNDEFINED);
        assembler_.Movq(x64::Immediate(undefined), x64::rax);
        for (uint32_t i = 0; i < taggedSlots; ++i) {
            assembler_.Pushq(x64::rax);
        }
    }

    // 4. Allocate untagged slots.
    uint32_t untaggedSlots = graph->GetUntaggedStackSlots();
    if (untaggedSlots > 0) {
        assembler_.Subq(x64::Immediate(untaggedSlots * sizeof(uint64_t)), x64::rsp);
    }

    // SysV requires 16-byte alignment at call sites. The 3 fixed header slots
    // leave rsp misaligned when the number of local slots is even.
    if (((taggedSlots + untaggedSlots) & 1U) == 0) {
        assembler_.Subq(x64::Immediate(FRAME_SLOT_SIZE), x64::rsp);
    }

    SetHasFrame(true);
}

void ArkSteedAssembler::Epilogue()
{
    RecordComment("Epilogue");
    // LeaveFrame: mov rsp, rbp; pop rbp
    assembler_.Movq(x64::rbp, x64::rsp);
    assembler_.Popq(x64::rbp);
}

// =============================================================================
// Code Access
// =============================================================================

uint8_t *ArkSteedAssembler::GetCodeBuffer()
{
    return assembler_.GetBegin();
}

size_t ArkSteedAssembler::GetCodeSize()
{
    return assembler_.GetCurrentPosition();
}
#endif
}  // namespace panda::ecmascript::arksteed
