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

#include <sstream>

#include "ecmascript/arksteed/arch/arm64/arksteed_assembler_arm64-inl.h"
#include "ecmascript/arksteed/arksteed_assembler.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::arksteed {
#if defined(PANDA_TARGET_ARM64)
// =============================================================================
// Register Move Operations
// =============================================================================

void ArkSteedAssembler::Move(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Mov(dst, src);
}

void ArkSteedAssembler::Move(ArkSteedRegister dst, int32_t immediate)
{
    assembler_.Mov(dst, aarch64::Immediate(immediate));
}

void ArkSteedAssembler::Move(ArkSteedRegister dst, int64_t immediate)
{
    ArkSteedRegister arm64Dst = dst;
    if (immediate >= 0 && immediate <= 0xFFFF) {  // 0xFFFF: 16-bit immediate mask
        assembler_.Movz(arm64Dst, static_cast<uint64_t>(immediate), 0);
    } else {
        uint64_t uimm = static_cast<uint64_t>(immediate);
        assembler_.Movz(arm64Dst, uimm & 0xFFFF, 0);  // 0xFFFF: lower 16 bits mask
        if ((uimm >> 16) & 0xFFFF) {  // 16: shift to extract bits [16, 31]
            assembler_.Movk(arm64Dst, (uimm >> 16) & 0xFFFF, 16);  // 16: LSL shift amount for bits [16, 31]
        }
        if ((uimm >> 32) & 0xFFFF) {  // 32: shift to extract bits [32, 47]
            assembler_.Movk(arm64Dst, (uimm >> 32) & 0xFFFF, 32);  // 32: LSL shift amount for bits [32, 47]
        }
        if ((uimm >> 48) & 0xFFFF) {  // 48: shift to extract bits [48, 63]
            assembler_.Movk(arm64Dst, (uimm >> 48) & 0xFFFF, 48);  // 48: LSL shift amount for bits [48, 63]
        }
    }
}

void ArkSteedAssembler::Move(ArkSteedRegister dst, uint64_t immediate)
{
    Move(dst, static_cast<int64_t>(immediate));
}

void ArkSteedAssembler::Move(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Mov(dst, src);
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
    Move(scratch, static_cast<int64_t>(bits));
    // Move from GP register to V register using FMOV
    assembler_.Fmov(dst, scratch);
}

// =============================================================================
// Memory Operations
// =============================================================================

void ArkSteedAssembler::LoadField(ArkSteedRegister dst, ArkSteedRegister base, int32_t offset)
{
    aarch64::MemoryOperand operand(base, offset, aarch64::AddrMode::OFFSET);
    assembler_.Ldr(dst, operand);
}

void ArkSteedAssembler::StoreField(ArkSteedRegister src, ArkSteedRegister base, int32_t offset)
{
    aarch64::MemoryOperand operand(base, offset, aarch64::AddrMode::OFFSET);
    assembler_.Str(src, operand);
}

void ArkSteedAssembler::LoadActualArgc(ArkSteedRegister dst)
{
    // 2: argc is stored at fp + 2 * FRAME_SLOT_SIZE
    aarch64::MemoryOperand operand(kFramePointerRegister, 2 * FRAME_SLOT_SIZE, aarch64::AddrMode::OFFSET);
    assembler_.Ldr(dst, operand);
}

void ArkSteedAssembler::LoadFloat64(ArkSteedDoubleRegister dst, MemoryOperand srcOp)
{
    assembler_.Ldr(dst, srcOp);
}

// =============================================================================
// Arithmetic Operations
// =============================================================================

void ArkSteedAssembler::Add(ArkSteedRegister dst, ArkSteedRegister src)
{
    ArkSteedRegister arm64Dst = dst;
    assembler_.Add(arm64Dst, arm64Dst, aarch64::Operand(src));
}

void ArkSteedAssembler::Add(ArkSteedRegister dst, int32_t immediate)
{
    ArkSteedRegister arm64Dst = dst;
    assembler_.Add(arm64Dst, arm64Dst, aarch64::Operand(aarch64::Immediate(immediate)));
}

void ArkSteedAssembler::Sub(ArkSteedRegister dst, ArkSteedRegister src)
{
    ArkSteedRegister arm64Dst = dst;
    assembler_.Sub(arm64Dst, arm64Dst, aarch64::Operand(src));
}

void ArkSteedAssembler::Sub(ArkSteedRegister dst, int32_t immediate)
{
    ArkSteedRegister arm64Dst = dst;
    assembler_.Sub(arm64Dst, arm64Dst, aarch64::Operand(aarch64::Immediate(immediate)));
}

// =============================================================================
// Bitwise Operations
// =============================================================================

void ArkSteedAssembler::Or(ArkSteedRegister dst, int64_t immediate)
{
    ArkSteedRegister arm64Dst = dst;
    auto imm = aarch64::LogicalImmediate::Create(static_cast<uint64_t>(immediate),
                                                 arm64Dst.IsW() ? aarch64::W_REG_SIZE : aarch64::X_REG_SIZE);
    if (imm.IsValid()) {
        assembler_.Orr(arm64Dst, arm64Dst, imm);
        return;
    }

    ScratchRegisterScope scope;
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, immediate);
    assembler_.Orr(arm64Dst, arm64Dst, aarch64::Operand(scratch));
}

void ArkSteedAssembler::Or(ArkSteedRegister dst, ArkSteedRegister src)
{
    ArkSteedRegister arm64Dst = dst;
    assembler_.Orr(arm64Dst, arm64Dst, aarch64::Operand(src));
}

// =============================================================================
// Comparison Operations
// =============================================================================

void ArkSteedAssembler::Compare(ArkSteedRegister lhs, ArkSteedRegister rhs)
{
    assembler_.Cmp(lhs, aarch64::Operand(rhs));
}

void ArkSteedAssembler::Compare(ArkSteedRegister lhs, int32_t immediate)
{
    assembler_.Cmp(lhs, aarch64::Operand(aarch64::Immediate(immediate)));
}

// =============================================================================
// Control Flow
// =============================================================================

void ArkSteedAssembler::Jump(Label *target)
{
    assembler_.B(target);
}

void ArkSteedAssembler::JumpIf(Condition condition, Label *target)
{
    assembler_.B(ToPhysicalCondition(condition), target);
}

void ArkSteedAssembler::BranchIfNoPendingException(Label* target)
{
    ArkSteedRegister scratch = kScratchRegister;
    Move(aarch64::x0, static_cast<uint64_t>(entryThread_->GetGlueAddr()));
    LoadField(scratch, aarch64::x0, static_cast<int32_t>(JSThread::GlueData::GetExceptionOffset(false)));
    Move(aarch64::x0, JSTaggedValue::Hole().GetRawData());
    Compare(scratch, aarch64::x0);
    JumpIf(Condition::COND_EQUAL, target);
}

void ArkSteedAssembler::ReturnIfPendingException()
{
    Label noPendingException;
    // Keep SP 16-byte aligned on ARM64 while only preserving x0.
    Push(aarch64::xzr, aarch64::x0);
    BranchIfNoPendingException(&noPendingException);
    Move(aarch64::x0, JSTaggedValue::Exception().GetRawData());
    Epilogue();
    Return();
    Bind(&noPendingException);
    Pop(aarch64::xzr, aarch64::x0);
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
    assembler_.Blr(target);
}

void ArkSteedAssembler::Call(Label *target)
{
    assembler_.Bl(target);
}

void ArkSteedAssembler::Return()
{
    assembler_.Ret();
}

// =============================================================================
// Stack Operations
// =============================================================================

void ArkSteedAssembler::Push(ArkSteedRegister reg1, ArkSteedRegister reg2)
{
    // -16: space for two 64-bit registers (16 bytes)
    aarch64::MemoryOperand operand(aarch64::sp, -16, aarch64::AddrMode::PREINDEX);
    // Match two sequential pushes while keeping SP 16-byte aligned.
    assembler_.Stp(reg2, reg1, operand);
}

void ArkSteedAssembler::Pop(ArkSteedRegister reg1, ArkSteedRegister reg2)
{
    // 16: space for two 64-bit registers (16 bytes)
    aarch64::MemoryOperand operand(aarch64::sp, 16, aarch64::AddrMode::POSTINDEX);
    assembler_.Ldp(reg2, reg1, operand);
}

void ArkSteedAssembler::ReserveCallArgSlots(int32_t slotCount)
{
    if (slotCount > 0) {
        assembler_.Sub(aarch64::sp, aarch64::sp, aarch64::Operand(aarch64::Immediate(slotCount * FRAME_SLOT_SIZE)));
    }
}

void ArkSteedAssembler::FreeCallArgSlots(int32_t slotCount)
{
    if (slotCount > 0) {
        assembler_.Add(aarch64::sp, aarch64::sp, aarch64::Operand(aarch64::Immediate(slotCount * FRAME_SLOT_SIZE)));
    }
}

// =============================================================================
// Function Prologue/Epilogue
// =============================================================================

void ArkSteedAssembler::Prologue(Graph *graph)
{
    RecordComment("Prologue");

    // 1. Set up fp.
    // -16: space for fp and lr (two 64-bit registers, 16 bytes)
    aarch64::MemoryOperand saveFpLr(aarch64::sp, -16, aarch64::AddrMode::PREINDEX);
    assembler_.Stp(aarch64::fp, aarch64::lr, saveFpLr);
    assembler_.Mov(aarch64::fp, aarch64::sp);

    // 2. Reserve the aligned local frame once, then fill slots in push order.
    // [fp - 8] = frameType, [fp - 16] = jsFunc, [fp - 24] = lexicalEnv.
    ArkSteedRegister x19 = aarch64::x19;
    ArkSteedRegister x20 = aarch64::x20;
    ArkSteedRegister tmp = aarch64::x9;
    taggedStackSlots_ = graph->GetTaggedStackSlots();
    uint32_t taggedSlots = taggedStackSlots_ - 1;
    uint32_t untaggedSlots = graph->GetUntaggedStackSlots();
    // 3. Build the SteedFunctionFrame fixed header in push order.
    assembler_.Mov(tmp, aarch64::Immediate(static_cast<int>(FrameType::STEED_FUNCTION_FRAME)));
    Push(tmp, x20);
    assembler_.Mov(tmp, aarch64::Immediate(JSTaggedValue::VALUE_UNDEFINED));
    Push(x19, tmp);
    for (size_t i = 0; i < taggedSlots / 2; i++) {  // 2: push two slots at a time
        Push(tmp, tmp);
    }

    if (untaggedSlots > 0) {
        assembler_.Sub(aarch64::sp,
                       aarch64::sp,
                       aarch64::Operand(aarch64::Immediate(untaggedSlots * sizeof(uint64_t))));
    }
    SetHasFrame(true);
}

void ArkSteedAssembler::Epilogue()
{
    RecordComment("Epilogue");
    // LeaveFrame: restore sp, lr, fp
    assembler_.Mov(aarch64::sp, aarch64::fp);

    aarch64::MemoryOperand lrSlot(aarch64::sp, 8, aarch64::AddrMode::OFFSET);  // 8: lr is stored 8 bytes above fp
    assembler_.Ldr(aarch64::lr, lrSlot);

    // 16: space for fp and lr (two 64-bit registers, 16 bytes)
    aarch64::MemoryOperand postPop(aarch64::sp, 16, aarch64::AddrMode::POSTINDEX);
    assembler_.Ldr(aarch64::fp, postPop);
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

// =============================================================================
// Condition Code Mapping
// =============================================================================

aarch64::Condition ArkSteedAssembler::ToPhysicalCondition(Condition condition) const
{
    switch (condition) {
        case Condition::COND_EQUAL:
            return aarch64::Condition::EQ;
        case Condition::COND_NOT_EQUAL:
            return aarch64::Condition::NE;
        case Condition::COND_LESS_THAN:
            return aarch64::Condition::LT;
        case Condition::COND_LESS_THAN_OR_EQUAL:
            return aarch64::Condition::LE;
        case Condition::COND_GREATER_THAN:
            return aarch64::Condition::GT;
        case Condition::COND_GREATER_THAN_OR_EQUAL:
            return aarch64::Condition::GE;
        case Condition::COND_ABOVE:
            return aarch64::Condition::HI;
        case Condition::COND_BELOW:
            return aarch64::Condition::LO;
        case Condition::COND_ABOVE_OR_EQUAL:
            return aarch64::Condition::HS;
        case Condition::COND_BELOW_OR_EQUAL:
            return aarch64::Condition::LS;
        case Condition::COND_ZERO:
            return aarch64::Condition::EQ;
        case Condition::COND_NOT_ZERO:
            return aarch64::Condition::NE;
        case Condition::COND_OVERFLOW:
            return aarch64::Condition::VS;
        case Condition::COND_NOT_OVERFLOW:
            return aarch64::Condition::VC;
        default:
            UNREACHABLE();
    }
}

#endif
}  // namespace panda::ecmascript::arksteed
