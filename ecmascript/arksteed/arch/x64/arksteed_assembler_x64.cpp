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
#include "ecmascript/js_function.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_tagged_value_wrapper.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/method.h"

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

void ArkSteedAssembler::Move(ArkSteedDoubleRegister dst, double immediate, ArkSteedRegister scratch)
{
    uint64_t bits = 0;
    static_assert(sizeof(double) == sizeof(uint64_t), "double must be 64 bits");
    if (memcpy_s(&bits, sizeof(double), &immediate, sizeof(double)) != EOK) {
        LOG_JIT(FATAL) << "memcpy failed in Move";
    }
    assembler_.Movabs(bits, scratch);
    assembler_.Movq(dst, scratch);
}

void ArkSteedAssembler::Move(ArkSteedDoubleRegister dst, ArkSteedRegister src)
{
    assembler_.Movq(dst, src);
}

void ArkSteedAssembler::Move(ArkSteedRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Movq(dst, src);
}

// =============================================================================
// Memory Operations
// =============================================================================

void ArkSteedAssembler::LoadField(ArkSteedRegister dst, ArkSteedRegister base, int32_t offset)
{
    x64::Operand operand(base, offset);
    assembler_.Movq(operand, dst);
}

void ArkSteedAssembler::LoadInt32Field(ArkSteedRegister dst, ArkSteedRegister base, int32_t offset)
{
    x64::Operand operand(base, offset);
    assembler_.Movl(operand, dst);
}

void ArkSteedAssembler::StoreField(ArkSteedRegister src, ArkSteedRegister base, int32_t offset)
{
    x64::Operand operand(base, offset);
    assembler_.Movq(src, operand);
}

void ArkSteedAssembler::StoreInt32Field(ArkSteedRegister src, ArkSteedRegister base, int32_t offset)
{
    x64::Operand operand(base, offset);
    assembler_.Movl(src, operand);
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

void ArkSteedAssembler::StoreFloat64(MemoryOperand dstOp, ArkSteedDoubleRegister src)
{
    assembler_.Movsd(dstOp, src);
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

void ArkSteedAssembler::SignExtendInt32ToInt64(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Movsxd(src, dst);
}

void ArkSteedAssembler::Int32Add(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Addl(src, dst);
}

void ArkSteedAssembler::Int32Sub(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Subl(src, dst);
}

void ArkSteedAssembler::Int32Mul(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Imull(src, dst);
}

void ArkSteedAssembler::Int32MulWide([[maybe_unused]] ArkSteedRegister dst,
                                     [[maybe_unused]] ArkSteedRegister left,
                                     [[maybe_unused]] ArkSteedRegister right)
{
    UNREACHABLE();
}

void ArkSteedAssembler::Int32MulHigh(ArkSteedRegister dst, ArkSteedRegister left, ArkSteedRegister right)
{
    ASSERT(dst == x64::rdx);
    ASSERT(left == x64::rax);
    assembler_.Imull(right);
}

void ArkSteedAssembler::Int32Div(ArkSteedRegister dst, ArkSteedRegister dividend, ArkSteedRegister divisor)
{
    Int32DivAndRemainder(dst, x64::rdx, dividend, divisor);
}

void ArkSteedAssembler::Int32DivAndRemainder(ArkSteedRegister quotient, ArkSteedRegister remainder,
                                             ArkSteedRegister dividend, ArkSteedRegister divisor)
{
    ASSERT(quotient == x64::rax);
    ASSERT(remainder == x64::rdx);
    ASSERT(dividend == x64::rax);
    ASSERT(divisor != x64::rax && divisor != x64::rdx);
    assembler_.Cdq();
    assembler_.Idivl(divisor);
}

void ArkSteedAssembler::PositiveInt32Mod(ArkSteedRegister dst, ArkSteedRegister dividend, ArkSteedRegister divisor)
{
    ASSERT(dividend == x64::rax);
    ASSERT(divisor != x64::rax && divisor != x64::rdx);
    if (dst != x64::rdx) {
        assembler_.Movl(dividend, dst);
    }
    Int32Div(dividend, dividend, divisor);
    if (dst != x64::rdx) {
        assembler_.Movl(dst, dividend);
    }
    if (dst != x64::rdx) {
        assembler_.Movsxd(x64::rdx, dst);
    }
}

void ArkSteedAssembler::Int32ToFloat64(ArkSteedDoubleRegister dst, ArkSteedRegister src)
{
    ScratchRegisterScope scope;
    ArkSteedRegister scratch = scope.AcquireScratch();
    SignExtendInt32ToInt64(scratch, src);
    assembler_.Cvtsi2sd(scratch, dst);
}

void ArkSteedAssembler::Float64Add(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Addsd(src, dst);
}

void ArkSteedAssembler::Float64Sub(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Subsd(src, dst);
}

void ArkSteedAssembler::Float64Mul(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Mulsd(src, dst);
}

void ArkSteedAssembler::Float64Div(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Divsd(src, dst);
}

void ArkSteedAssembler::Float64Neg(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    if (dst != src) {
        Move(dst, src);
    }
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratchGPR = scope.AcquireScratch();
    ArkSteedDoubleRegister scratchFPR = scope.AcquireDoubleScratch();
    Move(scratchGPR, static_cast<uint64_t>(1ULL << 63U));
    Move(scratchFPR, scratchGPR);
    assembler_.Xorpd(scratchFPR, dst);
}

void ArkSteedAssembler::CompareFloat64(ArkSteedDoubleRegister left, ArkSteedDoubleRegister right)
{
    assembler_.Ucomisd(right, left);
}

void ArkSteedAssembler::TruncateFloat64ToInt32(ArkSteedRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Cvttsd2si64(src, dst);
    assembler_.Andq(x64::Immediate(0xFFFFFFFF), dst);
}

void ArkSteedAssembler::Word64And(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.And(src, dst);
}

// =============================================================================
// Bitwise Operations
// =============================================================================

void ArkSteedAssembler::Int32Neg(ArkSteedRegister dst)
{
    assembler_.Negl(dst);
}

void ArkSteedAssembler::Int32Inc(ArkSteedRegister dst)
{
    assembler_.Incl(dst);
}

void ArkSteedAssembler::Int32Dec(ArkSteedRegister dst)
{
    assembler_.Decl(dst);
}

void ArkSteedAssembler::Int32BNot(ArkSteedRegister dst)
{
    assembler_.Notl(dst);
}

void ArkSteedAssembler::Or(ArkSteedRegister dst, int64_t immediate)
{
    // For 64-bit immediate, use movabs to a temp register then orq
    // For simplicity, handle common case where immediate fits in 32 bits
    if (immediate >= INT32_MIN && immediate <= INT32_MAX) {
        assembler_.Or(x64::Immediate(static_cast<int32_t>(immediate)), dst);
    } else {
        TemporaryRegisterScope scope(this);
        auto scratch = scope.AcquireScratch();
        assembler_.Movabs(static_cast<uint64_t>(immediate), scratch);
        assembler_.Orq(scratch, dst);
    }
}

void ArkSteedAssembler::Or(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Orq(src, dst);
}

void ArkSteedAssembler::And(ArkSteedRegister dst, int64_t immediate)
{
    if (immediate >= INT32_MIN && immediate <= INT32_MAX) {
        assembler_.Andq(x64::Immediate(static_cast<int32_t>(immediate)), dst);
    } else {
        TemporaryRegisterScope scope(this);
        auto scratch = scope.AcquireScratch();
        assembler_.Movabs(static_cast<uint64_t>(immediate), scratch);
        assembler_.And(scratch, dst);
    }
}

void ArkSteedAssembler::And(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.And(src, dst);
}

void ArkSteedAssembler::Lsr(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Shrq(x64::Immediate(static_cast<int32_t>(shift)), dst);
}

void ArkSteedAssembler::ShiftRightLogical(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Shrq(shift, dst);
}

void ArkSteedAssembler::ShiftRightLogical32(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Shrl(shift, dst);
}

void ArkSteedAssembler::MoveBitMask32(ArkSteedRegister dst, ArkSteedRegister bitIndex)
{
    Move(dst, 0);
    assembler_.Btsl(bitIndex, dst);
}

void ArkSteedAssembler::Int32And(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Andl(src, dst);
}

void ArkSteedAssembler::Int32And(ArkSteedRegister dst, int32_t immediate)
{
    assembler_.Andl(x64::Immediate(immediate), dst);
}

void ArkSteedAssembler::Int32Or(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Orl(src, dst);
}

void ArkSteedAssembler::Int32Or(ArkSteedRegister dst, int32_t immediate)
{
    assembler_.Orl(x64::Immediate(immediate), dst);
}

void ArkSteedAssembler::Int32Xor(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Xorl(src, dst);
}

void ArkSteedAssembler::Int32Xor(ArkSteedRegister dst, int32_t immediate)
{
    assembler_.Xorl(x64::Immediate(immediate), dst);
}

void ArkSteedAssembler::Int32ShiftLeft(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Shll(x64::Immediate(static_cast<int32_t>(shift)), dst);
}

void ArkSteedAssembler::Int32ShiftLeftByRegister(ArkSteedRegister dst, ArkSteedRegister shift)
{
    ASSERT(shift == x64::rcx);
    assembler_.ShllCl(dst);
}

void ArkSteedAssembler::Int32ShiftRightLogical(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Shrl(x64::Immediate(static_cast<int32_t>(shift)), dst);
}

void ArkSteedAssembler::Int32ShiftRightLogicalByRegister(ArkSteedRegister dst, ArkSteedRegister shift)
{
    ASSERT(shift == x64::rcx);
    assembler_.ShrlCl(dst);
}

void ArkSteedAssembler::Int32ShiftRightArithmetic(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Sarl(x64::Immediate(static_cast<int32_t>(shift)), dst);
}

void ArkSteedAssembler::Int32ShiftRightArithmeticByRegister(ArkSteedRegister dst, ArkSteedRegister shift)
{
    ASSERT(shift == x64::rcx);
    assembler_.SarlCl(dst);
}

// =============================================================================
// Comparison Operations
// =============================================================================

void ArkSteedAssembler::Compare(ArkSteedRegister lhs, ArkSteedRegister rhs)
{
    assembler_.Cmpq(rhs, lhs);
}

void ArkSteedAssembler::CompareInt32(ArkSteedRegister lhs, ArkSteedRegister rhs)
{
    assembler_.Cmpl(rhs, lhs);
}

void ArkSteedAssembler::Compare(ArkSteedRegister lhs, int32_t immediate)
{
    assembler_.Cmpq(x64::Immediate(immediate), lhs);
}

void ArkSteedAssembler::CompareInt32(ArkSteedRegister lhs, int32_t immediate)
{
    assembler_.Cmpl(x64::Immediate(immediate), lhs);
}

void ArkSteedAssembler::CompareField(ArkSteedRegister base, int32_t offset, ArkSteedRegister rhs)
{
    x64::Operand operand(base, offset);
    assembler_.Cmpq(rhs, operand);
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
            // COND_LESS_THAN is signed; unsigned comparisons must use a separate condition.
            assembler_.Jl(target);
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
            assembler_.Jo(target);
            break;
        case Condition::COND_NOT_OVERFLOW:
            assembler_.Jno(target);
            break;
        case Condition::COND_PARITY:
            assembler_.Jp(target);
            break;
        case Condition::COND_NOT_PARITY:
            assembler_.Jnp(target);
            break;
        default:
            UNREACHABLE();
    }
}

void ArkSteedAssembler::JumpIfNotTaggedHeapObject(ArkSteedRegister value, Label *target)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, value);
    assembler_.Shrq(x64::Immediate(static_cast<int32_t>(JSTaggedValue::TAG_BITS_SHIFT)), scratch);
    Compare(scratch, 0);
    JumpIf(Condition::COND_NOT_EQUAL, target);
    Move(scratch, value);
    And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_SPECIAL | JSTaggedValue::TAG_BOOLEAN));
    Compare(scratch, 0);
    JumpIf(Condition::COND_NOT_EQUAL, target);
}

void ArkSteedAssembler::JumpIfNotJSFunction(ArkSteedRegister value, Label *target)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    LoadField(scratch, value, TaggedObject::HCLASS_OFFSET);
    And(scratch, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    LoadField(scratch, scratch, JSHClass::BIT_FIELD_OFFSET);
    And(scratch, (1U << JSHClass::TYPE_BITFIELD_NUM) - 1);
    Compare(scratch, static_cast<int32_t>(JSType::JS_FUNCTION_FIRST));
    JumpIf(Condition::COND_LESS_THAN, target);
    Compare(scratch, static_cast<int32_t>(JSType::JS_FUNCTION_LAST));
    JumpIf(Condition::COND_GREATER_THAN, target);
}

void ArkSteedAssembler::JumpIfClassConstructor(ArkSteedRegister jsFunc, Label *target)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Label notClassConstructor;
    LoadField(scratch, jsFunc, TaggedObject::HCLASS_OFFSET);
    And(scratch, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    LoadField(scratch, scratch, JSHClass::BIT_FIELD_OFFSET);
    assembler_.Btq(x64::Immediate(JSHClass::IsClassConstructorOrPrototypeBit::START_BIT), scratch);
    assembler_.Jnb(&notClassConstructor);
    assembler_.Btq(x64::Immediate(JSHClass::ConstructorBit::START_BIT), scratch);
    assembler_.Jb(target);
    Bind(&notClassConstructor);
}

void ArkSteedAssembler::JumpIfFunctionNotCompiled(ArkSteedRegister jsFunc, Label *target)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister bitfield = scope.AcquireScratch();
    LoadField(bitfield, jsFunc, JSFunctionBase::BIT_FIELD_OFFSET);
    assembler_.Btq(x64::Immediate(JSFunctionBase::IsCompiledCodeBit::START_BIT), bitfield);
    assembler_.Jnb(target);
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

void ArkSteedAssembler::ReserveCallArgSlots(ArkSteedRegister slotCount)
{
    static constexpr int FRAME_SLOT_SIZE_LOG2 = 3;
    assembler_.Leaq(x64::Operand(slotCount, x64::Scale::Times8, 0), slotCount);
    assembler_.Subq(slotCount, x64::rsp);
    assembler_.Shrq(x64::Immediate(FRAME_SLOT_SIZE_LOG2), slotCount);
}

void ArkSteedAssembler::FreeCallArgSlots(ArkSteedRegister slotCount)
{
    assembler_.Leaq(x64::Operand(slotCount, x64::Scale::Times8, 0), slotCount);
    assembler_.Addq(slotCount, x64::rsp);
}

void ArkSteedAssembler::RestoreStackPointerToFrameBottom(Graph *graph)
{
    uint32_t taggedSlots = graph->GetTaggedStackSlots();
    uint32_t untaggedSlots = graph->GetUntaggedStackSlots();
    uint32_t frameSlots = 3 + taggedSlots + untaggedSlots;  // 3: frameType, jsFunc, lexicalEnv.
    if (((taggedSlots + untaggedSlots) & 1U) == 0) {
        frameSlots++;
    }
    assembler_.Leaq(x64::Operand(x64::rbp, -static_cast<int32_t>(frameSlots * FRAME_SLOT_SIZE)), x64::rsp);
}

void ArkSteedAssembler::PushUndefinedForSteedCall(ArkSteedRegister fillSlotCount, uint32_t userArgc)
{
    Label fillUndefined;
    Label fillDone;
    Compare(fillSlotCount, 0);
    JumpIf(Condition::COND_LESS_THAN_OR_EQUAL, &fillDone);

    assembler_.Bind(&fillUndefined);
    constexpr int32_t SLOT_BEFORE_FIRST_OPTIONAL_ARG = NUM_MANDATORY_JSFUNC_ARGS;
    int32_t firstUndefinedArgBaseOffset =
        static_cast<int32_t>((SLOT_BEFORE_FIRST_OPTIONAL_ARG + userArgc) * FRAME_SLOT_SIZE);
    assembler_.Movq(x64::Immediate(static_cast<int32_t>(JSTaggedValue::VALUE_UNDEFINED)),
                    x64::Operand(x64::rsp, fillSlotCount, x64::Scale::Times8, firstUndefinedArgBaseOffset));
    Sub(fillSlotCount, 1);
    Compare(fillSlotCount, 0);
    JumpIf(Condition::COND_GREATER_THAN, &fillUndefined);
    assembler_.Bind(&fillDone);
}

void ArkSteedAssembler::PrepareSteedCalleeContext(ArkSteedRegister target, ArkSteedRegister codeEntry)
{
    Move(x64::r12, target);
    LoadField(x64::rbx, x64::r12, JSFunction::LEXICAL_ENV_OFFSET);
    LoadField(codeEntry, x64::r12, JSFunction::CODE_ENTRY_OFFSET);
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
