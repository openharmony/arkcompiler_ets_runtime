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

#include <algorithm>
#include <limits>
#include <sstream>

#include "ecmascript/arksteed/arch/arm64/arksteed_assembler_arm64-inl.h"

#include <limits>

#include "ecmascript/arksteed/arksteed_assembler.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/base/bit_helper.h"
#include "ecmascript/byte_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_tagged_value_wrapper.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/method.h"
#include "ecmascript/string/line_string.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript::arksteed {
#if defined(PANDA_TARGET_ARM64)
void ArkSteedAssembler::EmitAddSubImmediate(ArkSteedRegister dst, ArkSteedRegister src, uint64_t immediate,
                                            AddSubImmediateOp operation)
{
    ASSERT(dst.IsW() == src.IsW());
    ASSERT(FitsAddSubImmediate(immediate));

    constexpr uint32_t SF_BIT = 31U;
    constexpr uint32_t SET_FLAGS_BIT = 29U;
    constexpr uint32_t SHIFT_BIT = 22U;
    constexpr uint32_t IMMEDIATE_LOW_BIT = 10U;
    constexpr uint32_t SOURCE_LOW_BIT = 5U;
    constexpr uint32_t IMM12_MASK = (1U << 12U) - 1U;

    uint32_t opcode = 0U;
    switch (operation) {
        case AddSubImmediateOp::ADD:
            opcode = aarch64::ADD_Imm;
            break;
        case AddSubImmediateOp::SUB:
            opcode = aarch64::SUB_Imm;
            break;
        case AddSubImmediateOp::SUBS:
            opcode = aarch64::SUB_Imm | (1U << SET_FLAGS_BIT);
            break;
        default:
            UNREACHABLE();
    }

    uint32_t shift = immediate > IMM12_MASK ? 1U : 0U;
    uint32_t imm12 = static_cast<uint32_t>(shift == 0U ? immediate : immediate >> 12U);
    uint32_t size = dst.IsW() ? 0U : 1U;
    uint32_t code = opcode | (size << SF_BIT) | (shift << SHIFT_BIT) | (imm12 << IMMEDIATE_LOW_BIT) |
                    (static_cast<uint32_t>(src.GetId()) << SOURCE_LOW_BIT) | static_cast<uint32_t>(dst.GetId());
    assembler_.EmitU32(code);
}

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

void ArkSteedAssembler::MoveEmbeddedTagged(ArkSteedRegister dst, uint32_t handleIndex)
{
    // Keep both the final literal displacement and the unresolved-label link chain within imm19 range.
    CheckCodePools(true);

    size_t literalIndex = 0;
    auto it = embeddedLiteralIndexByHandle_.find(handleIndex);
    if (it == embeddedLiteralIndexByHandle_.end()) {
        literalIndex = pendingEmbeddedLiterals_.size();
        auto literal = std::make_unique<PendingEmbeddedLiteral>();
        literal->handleIndex = handleIndex;
        pendingEmbeddedLiterals_.push_back(std::move(literal));
        embeddedLiteralIndexByHandle_.emplace(handleIndex, literalIndex);
    } else {
        literalIndex = it->second;
    }

    PendingEmbeddedLiteral *literal = pendingEmbeddedLiterals_[literalIndex].get();
    uint32_t loadOffset = GetPcOffset();
    literal->loadOffsets.push_back(loadOffset);
    UpdateEmbeddedLiteralPoolCheck(loadOffset, literalIndex);
    assembler_.Ldr(dst, &literal->label);
}

void ArkSteedAssembler::FinalizeEmbeddedRefs()
{
    EmitEmbeddedLiteralPool(true);
}

void ArkSteedAssembler::Move(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Mov(dst, src);
}

void ArkSteedAssembler::Move(ArkSteedDoubleRegister dst, double immediate)
{
    uint64_t bits = base::bit_cast<uint64_t>(immediate);
    if (bits == 0) {
        assembler_.Fmov(dst, aarch64::xzr);
        return;
    }
    if (assembler_.TryFmov(dst, immediate)) {
        return;
    }
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, static_cast<int64_t>(bits));
    assembler_.Fmov(dst, scratch);
}

void ArkSteedAssembler::Move(ArkSteedDoubleRegister dst, double immediate, ArkSteedRegister scratch)
{
    uint64_t bits = base::bit_cast<uint64_t>(immediate);
    if (bits == 0) {
        assembler_.Fmov(dst, aarch64::xzr);
        return;
    }
    if (assembler_.TryFmov(dst, immediate)) {
        return;
    }
    Move(scratch, static_cast<int64_t>(bits));
    assembler_.Fmov(dst, scratch);
}

void ArkSteedAssembler::Move(ArkSteedDoubleRegister dst, ArkSteedRegister src)
{
    assembler_.Fmov(dst, src);
}

void ArkSteedAssembler::Move(ArkSteedRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Fmov(dst, src);
}

// =============================================================================
// Memory Operations
// =============================================================================

void ArkSteedAssembler::LoadField(ArkSteedRegister dst, ArkSteedRegister base, int32_t offset)
{
    aarch64::MemoryOperand operand(base, offset, aarch64::AddrMode::OFFSET);
    LoadRegisterWithOperand(dst, operand);
}

void ArkSteedAssembler::LoadInt32Field(ArkSteedRegister dst, ArkSteedRegister base, int32_t offset)
{
    aarch64::MemoryOperand operand(base, offset, aarch64::AddrMode::OFFSET);
    assembler_.Ldr(dst.W(), operand);
}

void ArkSteedAssembler::LoadTaggedElement(ArkSteedRegister dst, ArkSteedRegister elements, ArkSteedRegister index)
{
    constexpr uint8_t TAGGED_SIZE_SHIFT = 3;
    assembler_.Add(dst, elements, aarch64::Operand(index, aarch64::UXTW, TAGGED_SIZE_SHIFT));
    LoadField(dst, dst, static_cast<int32_t>(TaggedArray::DATA_OFFSET));
}

void ArkSteedAssembler::LoadLineStringCharCode(ArkSteedRegister dst, ArkSteedRegister string, ArkSteedRegister index,
                                               ArkSteedRegister lengthAndFlags)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister data = scope.AcquireScratch();
    Move(data, string);
    Add(data, static_cast<int32_t>(LineString::DATA_OFFSET));

    Label utf16;
    Label done;
    TestAndBranchIfNotZero(lengthAndFlags, BaseString::CompressedStatusBit::START_BIT, &utf16);
    assembler_.Ldrb(dst.W(), aarch64::MemoryOperand(data, index, aarch64::UXTW, 0));
    Jump(&done);
    Bind(&utf16);
    assembler_.Ldrh(dst.W(), aarch64::MemoryOperand(data, index, aarch64::UXTW, 1));
    Bind(&done);
}

void ArkSteedAssembler::LoadTypedArrayDataPointer(ArkSteedRegister dst, ArkSteedRegister receiver,
                                                  ArkSteedRegister storage, ArkSteedRegister scratch, bool isOnHeap)
{
    if (isOnHeap) {
        Move(dst, storage);
        Add(dst, static_cast<int32_t>(ByteArray::DATA_OFFSET));
        return;
    }
    LoadInt32Field(scratch, receiver, static_cast<int32_t>(JSTypedArray::BYTE_OFFSET_OFFSET));
    LoadField(dst, storage, static_cast<int32_t>(JSNativePointer::POINTER_OFFSET));
    Add(dst, scratch);
}

void ArkSteedAssembler::LoadTypedArrayIntElement(ArkSteedRegister dst, ArkSteedRegister data, ArkSteedRegister index,
                                                 JSType elementType)
{
    switch (elementType) {
        case JSType::JS_INT8_ARRAY:
            assembler_.Ldrb(dst.W(), aarch64::MemoryOperand(data, index, aarch64::UXTW, 0));
            Int32ShiftLeft(dst, 24U);
            Int32ShiftRightArithmetic(dst, 24U);
            break;
        case JSType::JS_UINT8_ARRAY:
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            assembler_.Ldrb(dst.W(), aarch64::MemoryOperand(data, index, aarch64::UXTW, 0));
            break;
        case JSType::JS_INT16_ARRAY:
            assembler_.Ldrh(dst.W(), aarch64::MemoryOperand(data, index, aarch64::UXTW, 1));
            Int32ShiftLeft(dst, 16U);
            Int32ShiftRightArithmetic(dst, 16U);
            break;
        case JSType::JS_UINT16_ARRAY:
            assembler_.Ldrh(dst.W(), aarch64::MemoryOperand(data, index, aarch64::UXTW, 1));
            break;
        case JSType::JS_INT32_ARRAY:
            assembler_.Ldr(dst.W(), aarch64::MemoryOperand(data, index, aarch64::UXTW, 2));
            break;
        default:
            UNREACHABLE();
    }
}

void ArkSteedAssembler::LoadTypedArrayDoubleElement(ArkSteedDoubleRegister dst, ArkSteedRegister data,
                                                    ArkSteedRegister index, ArkSteedRegister scratch,
                                                    JSType elementType)
{
    switch (elementType) {
        case JSType::JS_UINT32_ARRAY:
            assembler_.Ldr(scratch.W(), aarch64::MemoryOperand(data, index, aarch64::UXTW, 2));
            assembler_.Scvtf(dst, scratch.X());
            break;
        case JSType::JS_FLOAT32_ARRAY: {
            assembler_.Add(scratch, data, aarch64::Operand(index, aarch64::UXTW, 2));
            auto single = aarch64::VRegister::Create(dst.Code(), aarch64::S_REG_SIZE);
            assembler_.Ldr(single, aarch64::MemoryOperand(scratch, 0));
            assembler_.Fcvt(dst, single);
            break;
        }
        case JSType::JS_FLOAT64_ARRAY:
            assembler_.Add(scratch, data, aarch64::Operand(index, aarch64::UXTW, 3));
            assembler_.Ldr(dst, aarch64::MemoryOperand(scratch, 0));
            break;
        default:
            UNREACHABLE();
    }
}

void ArkSteedAssembler::StoreField(ArkSteedRegister src, ArkSteedRegister base, int32_t offset)
{
    aarch64::MemoryOperand operand(base, offset, aarch64::AddrMode::OFFSET);
    StoreRegisterWithOperand(src, operand);
}

void ArkSteedAssembler::StoreField(ArkSteedRegister src, ArkSteedRegister base, ArkSteedRegister offset)
{
    assembler_.Str(src, aarch64::MemoryOperand(base, offset, aarch64::UXTX));
}

void ArkSteedAssembler::StoreInt8Field(ArkSteedRegister src, ArkSteedRegister base, int32_t offset)
{
    assembler_.Strb(src.W(), aarch64::MemoryOperand(base, offset, aarch64::AddrMode::OFFSET));
}

void ArkSteedAssembler::StoreInt16Field(ArkSteedRegister src, ArkSteedRegister base, int32_t offset)
{
    assembler_.Strh(src.W(), aarch64::MemoryOperand(base, offset, aarch64::AddrMode::OFFSET));
}

void ArkSteedAssembler::StoreInt32Field(ArkSteedRegister src, ArkSteedRegister base, int32_t offset)
{
    aarch64::MemoryOperand operand(base, offset, aarch64::AddrMode::OFFSET);
    assembler_.Str(src.W(), operand);
}

void ArkSteedAssembler::StoreInt32FieldRelease(ArkSteedRegister src, ArkSteedRegister base, int32_t offset)
{
    aarch64::MemoryOperand operand(base, offset, aarch64::AddrMode::OFFSET);
    assembler_.Stlr(src.W(), operand);
}

void ArkSteedAssembler::StoreFloat64Field(ArkSteedDoubleRegister src, ArkSteedRegister base, int32_t offset)
{
    StoreFloat64(aarch64::MemoryOperand(base, offset, aarch64::AddrMode::OFFSET), src);
}

void ArkSteedAssembler::StoreFloat32Field(ArkSteedDoubleRegister src, ArkSteedDoubleRegister scratch,
                                          ArkSteedRegister base, int32_t offset)
{
    auto floatScratch = aarch64::VRegister::Create(scratch.Code(), 32);
    assembler_.FcvtFloat32(floatScratch, src);
    assembler_.StrFloat32(floatScratch, aarch64::MemoryOperand(base, offset, aarch64::AddrMode::OFFSET));
}

void ArkSteedAssembler::LoadActualArgc(ArkSteedRegister dst)
{
    // 2: argc is stored at fp + 2 * FRAME_SLOT_SIZE
    aarch64::MemoryOperand operand(kFramePointerRegister, 2 * FRAME_SLOT_SIZE, aarch64::AddrMode::OFFSET);
    assembler_.Ldr(dst, operand);
}

void ArkSteedAssembler::LoadFloat64(ArkSteedDoubleRegister dst, MemoryOperand srcOp)
{
    if (FitsScaledImmediateOffset(srcOp, true)) {
        assembler_.Ldr(dst, srcOp);
        return;
    }
    assembler_.Ldr(dst, MaterializeAddress(srcOp));
}

void ArkSteedAssembler::StoreFloat64(MemoryOperand dstOp, ArkSteedDoubleRegister src)
{
    if (FitsScaledImmediateOffset(dstOp, true)) {
        assembler_.Str(src, dstOp);
        return;
    }
    assembler_.Str(src, MaterializeAddress(dstOp));
}

void ArkSteedAssembler::StoreFloat64Constant(MemoryOperand dstOp, double immediate, ArkSteedRegister scratchGPR,
                                             ArkSteedDoubleRegister scratchFPR)
{
    uint64_t bits = base::bit_cast<uint64_t>(immediate);
    if (bits == 0) {
        assembler_.Fmov(scratchFPR, aarch64::xzr);
        StoreFloat64(dstOp, scratchFPR);
        return;
    }
    if (assembler_.TryFmov(scratchFPR, immediate)) {
        StoreFloat64(dstOp, scratchFPR);
        return;
    }
    Move(scratchGPR, bits);
    MoveRepr(MachineRepresentation::Word64, dstOp, scratchGPR);
}

void ArkSteedAssembler::NormalizeEagerDeoptOverflowLink()
{
    static_assert(ARKSTEED_EAGER_DEOPT_FIXED_EXIT_LINK_SIZE == 0U);
    static_assert(ARKSTEED_EAGER_DEOPT_VENEER_FRAME_SIZE == 2U * FRAME_SLOT_SIZE);
    assembler_.Add(aarch64::sp, aarch64::sp,
                   aarch64::Operand(aarch64::Immediate(ARKSTEED_EAGER_DEOPT_VENEER_FRAME_SIZE)));
}

void ArkSteedAssembler::CallArkSteedDeoptimizationEntry()
{
    ASSERT(temporaryRegisterScope_ == nullptr);
    ASSERT(entryThread_ != nullptr);

    ArkSteedRegister glue = ARKSTEED_EAGER_DEOPT_ENTRY_GLUE_REGISTER;
    ArkSteedRegister target = ARKSTEED_EAGER_DEOPT_ENTRY_TARGET_REGISTER;
    Move(glue, static_cast<uint64_t>(entryThread_->GetGlueAddr()));

    Address address = entryThread_->GetRTInterface(RTSTUB_ID(ArkSteedDeoptimizationEntry));
    Move(target, static_cast<uint64_t>(address));
    Push(aarch64::lr);
    Call(target);
}

void ArkSteedAssembler::ConvertInt32ToDouble(ArkSteedDoubleRegister dst, ArkSteedRegister src)
{
    assembler_.Scvtf(dst, src.W());
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
    int64_t value = immediate;
    if (value >= 0 && FitsAddSubImmediate(static_cast<uint64_t>(value))) {
        EmitAddSubImmediate(dst, dst, static_cast<uint64_t>(value), AddSubImmediateOp::ADD);
        return;
    }
    if (value < 0 && FitsAddSubImmediate(static_cast<uint64_t>(-value))) {
        EmitAddSubImmediate(dst, dst, static_cast<uint64_t>(-value), AddSubImmediateOp::SUB);
        return;
    }
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, value);
    if (dst.IsSp()) {
        assembler_.Add(dst, dst, aarch64::Operand(scratch, aarch64::UXTX, 0));
    } else {
        assembler_.Add(dst, dst, aarch64::Operand(scratch));
    }
}

void ArkSteedAssembler::Add(ArkSteedRegister dst, int64_t immediate)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, immediate);
    Add(dst, scratch);
}

void ArkSteedAssembler::Sub(ArkSteedRegister dst, ArkSteedRegister src)
{
    ArkSteedRegister arm64Dst = dst;
    assembler_.Sub(arm64Dst, arm64Dst, aarch64::Operand(src));
}

void ArkSteedAssembler::Sub(ArkSteedRegister dst, int32_t immediate)
{
    int64_t value = immediate;
    if (value >= 0 && FitsAddSubImmediate(static_cast<uint64_t>(value))) {
        EmitAddSubImmediate(dst, dst, static_cast<uint64_t>(value), AddSubImmediateOp::SUB);
        return;
    }
    if (value < 0 && FitsAddSubImmediate(static_cast<uint64_t>(-value))) {
        EmitAddSubImmediate(dst, dst, static_cast<uint64_t>(-value), AddSubImmediateOp::ADD);
        return;
    }
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, value);
    if (dst.IsSp()) {
        assembler_.Sub(dst, dst, aarch64::Operand(scratch, aarch64::UXTX, 0));
    } else {
        assembler_.Sub(dst, dst, aarch64::Operand(scratch));
    }
}

void ArkSteedAssembler::SignExtendInt32ToInt64(ArkSteedRegister dst, ArkSteedRegister src)
{
    // ADD (extended register) interprets register code 31 in Rn as SP, not
    // XZR. SBFM Xd, Xn, #0, #31 is the architectural SXTW alias and does not
    // accidentally add the current stack pointer to the Int32 payload.
    assembler_.Sbfm(dst, src, 0, 31);  // 0, 31: SXTW bit range.
}

void ArkSteedAssembler::Int32Add(ArkSteedRegister dst, ArkSteedRegister left, ArkSteedRegister right)
{
    assembler_.Adds(dst.W(), left.W(), aarch64::Operand(right.W()));
}

void ArkSteedAssembler::Int32Sub(ArkSteedRegister dst, ArkSteedRegister left, ArkSteedRegister right)
{
    assembler_.Subs(dst.W(), left.W(), aarch64::Operand(right.W()));
}

void ArkSteedAssembler::Int32Mul(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Mul(dst.W(), dst.W(), src.W());
}

void ArkSteedAssembler::Int32MulWide(ArkSteedRegister dst, ArkSteedRegister left, ArkSteedRegister right)
{
    assembler_.Smull(dst, left.W(), right.W());
}

void ArkSteedAssembler::Int32MulHigh(ArkSteedRegister dst, ArkSteedRegister left, ArkSteedRegister right)
{
    Int32MulWide(dst, left, right);
    assembler_.Asr(dst, dst, 32U);  // 32: signed high half of the 64-bit product.
}

void ArkSteedAssembler::Int32Div(ArkSteedRegister dst, ArkSteedRegister dividend, ArkSteedRegister divisor)
{
    assembler_.Sdiv(dst.W(), dividend.W(), divisor.W());
}

void ArkSteedAssembler::Int32DivAndRemainder(ArkSteedRegister quotient, ArkSteedRegister remainder,
                                             ArkSteedRegister dividend, ArkSteedRegister divisor)
{
    Int32Div(quotient, dividend, divisor);
    assembler_.Msub(remainder.W(), quotient.W(), divisor.W(), dividend.W());
}

void ArkSteedAssembler::PositiveInt32Mod(ArkSteedRegister dst, ArkSteedRegister dividend, ArkSteedRegister divisor)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister quotient = scope.AcquireScratch();
    assembler_.Sdiv(quotient.W(), dividend.W(), divisor.W());
    assembler_.Msub(dst.W(), quotient.W(), divisor.W(), dividend.W());
}

void ArkSteedAssembler::Int32ToFloat64(ArkSteedDoubleRegister dst, ArkSteedRegister src)
{
    assembler_.Scvtf(dst, src.W());
}

void ArkSteedAssembler::Float64Add(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Fadd(dst, dst, src);
}

void ArkSteedAssembler::Float64Sub(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Fsub(dst, dst, src);
}

void ArkSteedAssembler::Float64Mul(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Fmul(dst, dst, src);
}

void ArkSteedAssembler::Float64Div(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Fdiv(dst, dst, src);
}

void ArkSteedAssembler::Float64Neg(ArkSteedDoubleRegister dst, ArkSteedDoubleRegister src)
{
    assembler_.Fneg(dst, src);
}

void ArkSteedAssembler::CompareFloat64(ArkSteedDoubleRegister left, ArkSteedDoubleRegister right)
{
    assembler_.Fcmp(left, right);
}

void ArkSteedAssembler::TruncateFloat64ToInt32(ArkSteedRegister dst, ArkSteedDoubleRegister src)
{
    // JavaScript ToInt32 keeps the low 32 bits after truncation.  Converting
    // directly to a W register saturates values outside the signed int32
    // range on AArch64, losing those low bits.  Match the x64 path by first
    // truncating into 64 bits; subsequent W-register users naturally retain
    // the required low 32 bits.
    assembler_.Fcvtzs(dst, src);
}

// =============================================================================
// Bitwise Operations
// =============================================================================

void ArkSteedAssembler::Word64And(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.And(dst, dst, aarch64::Operand(src));
}

void ArkSteedAssembler::Or(ArkSteedRegister dst, int32_t immediate)
{
    Or(dst, static_cast<int64_t>(immediate));
}

void ArkSteedAssembler::Or(ArkSteedRegister dst, int64_t immediate)
{
    ArkSteedRegister arm64Dst = dst;
    auto imm = aarch64::LogicalImmediate::Create(static_cast<uint64_t>(immediate),
                                                 arm64Dst.IsW() ? aarch64::W_REG_SIZE : aarch64::X_REG_SIZE);
    if (imm.IsValid()) {
        assembler_.Orr(arm64Dst, arm64Dst, imm);
        return;
    }

    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, immediate);
    assembler_.Orr(arm64Dst, arm64Dst, aarch64::Operand(scratch));
}

void ArkSteedAssembler::Or(ArkSteedRegister dst, ArkSteedRegister src)
{
    ArkSteedRegister arm64Dst = dst;
    assembler_.Orr(arm64Dst, arm64Dst, aarch64::Operand(src));
}

void ArkSteedAssembler::And(ArkSteedRegister dst, int32_t immediate)
{
    And(dst, static_cast<int64_t>(immediate));
}

void ArkSteedAssembler::And(ArkSteedRegister dst, int64_t immediate)
{
    ArkSteedRegister arm64Dst = dst;
    auto imm = aarch64::LogicalImmediate::Create(static_cast<uint64_t>(immediate),
                                                 arm64Dst.IsW() ? aarch64::W_REG_SIZE : aarch64::X_REG_SIZE);
    if (imm.IsValid()) {
        assembler_.And(arm64Dst, arm64Dst, imm);
        return;
    }

    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, immediate);
    assembler_.And(arm64Dst, arm64Dst, aarch64::Operand(scratch));
}

void ArkSteedAssembler::And(ArkSteedRegister dst, ArkSteedRegister src)
{
    ArkSteedRegister arm64Dst = dst;
    assembler_.And(arm64Dst, arm64Dst, aarch64::Operand(src));
}

void ArkSteedAssembler::Lsr(ArkSteedRegister dst, uint32_t shift)
{
    ArkSteedRegister arm64Dst = dst;
    assembler_.Lsr(arm64Dst, arm64Dst, shift);
}

void ArkSteedAssembler::ShiftRightLogical(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Lsr(dst, dst, shift);
}

void ArkSteedAssembler::ShiftLeft(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Orr(dst, aarch64::xzr, aarch64::Operand(dst, aarch64::Shift::LSL, shift));
}

void ArkSteedAssembler::ShiftRightLogical32(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Lsr(dst.W(), dst.W(), shift);
}

void ArkSteedAssembler::MoveBitMask32(ArkSteedRegister dst, ArkSteedRegister bitIndex)
{
    Move(dst, 1);
    assembler_.Lsl(dst.W(), dst.W(), bitIndex.W());
}

void ArkSteedAssembler::Int32Neg(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Subs(dst.W(), aarch64::wzr, aarch64::Operand(src.W()));
}

void ArkSteedAssembler::Int32Inc(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Adds(dst.W(), src.W(), aarch64::Operand(aarch64::Immediate(1)));
}

void ArkSteedAssembler::Int32Dec(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Subs(dst.W(), src.W(), aarch64::Operand(aarch64::Immediate(1)));
}

void ArkSteedAssembler::Int32BNot(ArkSteedRegister dst)
{
    assembler_.Mvn(dst.W(), dst.W());
}

void ArkSteedAssembler::Int32And(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.And(dst.W(), dst.W(), aarch64::Operand(src.W()));
}

void ArkSteedAssembler::Int32And(ArkSteedRegister dst, int32_t immediate)
{
    auto imm = aarch64::LogicalImmediate::Create(static_cast<uint32_t>(immediate), aarch64::W_REG_SIZE);
    if (imm.IsValid()) {
        assembler_.And(dst.W(), dst.W(), imm);
        return;
    }
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, immediate);
    assembler_.And(dst.W(), dst.W(), aarch64::Operand(scratch.W()));
}

void ArkSteedAssembler::Int32Or(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Orr(dst.W(), dst.W(), aarch64::Operand(src.W()));
}

void ArkSteedAssembler::Int32Or(ArkSteedRegister dst, int32_t immediate)
{
    auto imm = aarch64::LogicalImmediate::Create(static_cast<uint32_t>(immediate), aarch64::W_REG_SIZE);
    if (imm.IsValid()) {
        assembler_.Orr(dst.W(), dst.W(), imm);
        return;
    }
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, immediate);
    assembler_.Orr(dst.W(), dst.W(), aarch64::Operand(scratch.W()));
}

void ArkSteedAssembler::Int32Xor(ArkSteedRegister dst, ArkSteedRegister src)
{
    assembler_.Eor(dst.W(), dst.W(), aarch64::Operand(src.W()));
}

void ArkSteedAssembler::Int32Xor(ArkSteedRegister dst, int32_t immediate)
{
    auto imm = aarch64::LogicalImmediate::Create(static_cast<uint32_t>(immediate), aarch64::W_REG_SIZE);
    if (imm.IsValid()) {
        assembler_.Eor(dst.W(), dst.W(), imm);
        return;
    }
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, immediate);
    assembler_.Eor(dst.W(), dst.W(), aarch64::Operand(scratch.W()));
}

void ArkSteedAssembler::Int32ShiftLeft(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Orr(dst.W(), aarch64::wzr, aarch64::Operand(dst.W(), aarch64::Shift::LSL, shift));
}

void ArkSteedAssembler::Int32ShiftLeftByRegister(ArkSteedRegister dst, ArkSteedRegister shift)
{
    assembler_.Lsl(dst.W(), dst.W(), shift.W());
}

void ArkSteedAssembler::Int32ShiftRightLogical(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Lsr(dst.W(), dst.W(), shift);
}

void ArkSteedAssembler::Int32ShiftRightLogicalByRegister(ArkSteedRegister dst, ArkSteedRegister shift)
{
    assembler_.Lsr(dst.W(), dst.W(), shift.W());
}

void ArkSteedAssembler::Int32ShiftRightArithmetic(ArkSteedRegister dst, uint32_t shift)
{
    assembler_.Asr(dst.W(), dst.W(), shift);
}

void ArkSteedAssembler::Int32ShiftRightArithmeticByRegister(ArkSteedRegister dst, ArkSteedRegister shift)
{
    assembler_.Asr(dst.W(), dst.W(), shift.W());
}

// =============================================================================
// Comparison Operations
// =============================================================================

void ArkSteedAssembler::Compare(ArkSteedRegister lhs, ArkSteedRegister rhs)
{
    assembler_.Cmp(lhs, aarch64::Operand(rhs));
}

void ArkSteedAssembler::CompareInt32(ArkSteedRegister lhs, ArkSteedRegister rhs)
{
    assembler_.Cmp(lhs.W(), aarch64::Operand(rhs.W()));
}

void ArkSteedAssembler::Compare(ArkSteedRegister lhs, int32_t immediate)
{
    if (immediate >= 0 && FitsAddSubImmediate(static_cast<uint64_t>(immediate))) {
        EmitAddSubImmediate(aarch64::xzr, lhs, static_cast<uint64_t>(immediate), AddSubImmediateOp::SUBS);
        return;
    }
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, static_cast<int64_t>(immediate));
    assembler_.Cmp(lhs, aarch64::Operand(scratch));
}

void ArkSteedAssembler::Compare(ArkSteedRegister lhs, int64_t immediate)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, immediate);
    Compare(lhs, scratch);
}

void ArkSteedAssembler::CompareInt32(ArkSteedRegister lhs, int32_t immediate)
{
    if (immediate >= 0 && FitsAddSubImmediate(static_cast<uint64_t>(immediate))) {
        EmitAddSubImmediate(aarch64::wzr, lhs.W(), static_cast<uint64_t>(immediate), AddSubImmediateOp::SUBS);
        return;
    }
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Move(scratch, static_cast<int64_t>(immediate));
    assembler_.Cmp(lhs.W(), aarch64::Operand(scratch.W()));
}

void ArkSteedAssembler::CompareField(ArkSteedRegister base, int32_t offset, ArkSteedRegister rhs)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    LoadField(scratch, base, offset);
    Compare(scratch, rhs);
}

// =============================================================================
// Control Flow
// =============================================================================

bool ArkSteedAssembler::IsVeneerBranchOrCall(uint32_t instruction)
{
    return (instruction & aarch64::BranchOpCode::BranchFMask) == aarch64::BranchOpCode::Branch;
}

bool ArkSteedAssembler::IsVeneerConditionOrCompareBranch(uint32_t instruction)
{
    return (instruction & aarch64::BranchOpCode::BranchCondFMask) == aarch64::BranchOpCode::BranchCond ||
           (instruction & aarch64::BranchOpCode::BranchCompareFMask) == aarch64::BranchOpCode::CBZ;
}

bool ArkSteedAssembler::IsVeneerTestBranch(uint32_t instruction)
{
    return (instruction & aarch64::BranchOpCode::BranchTestFMask) == aarch64::BranchOpCode::TBZ;
}

bool ArkSteedAssembler::IsVeneerBranchInRange(uint32_t instruction, int64_t displacement) const
{
    if ((displacement & (VENEER_INSTRUCTION_SIZE - 1U)) != 0) {
        return false;
    }
    if (IsVeneerBranchOrCall(instruction)) {
        constexpr int64_t minDisplacement = -(1LL << 27U);  // imm26 scaled by 4: -128 MiB.
        constexpr int64_t maxDisplacement = 1LL << 27U;  // imm26 scaled by 4: +128 MiB, exclusive.
        return displacement >= minDisplacement && displacement < maxDisplacement;
    }
    if (IsVeneerConditionOrCompareBranch(instruction)) {
        constexpr int64_t minDisplacement = -(1LL << 20U);  // imm19 scaled by 4: -1 MiB.
        constexpr int64_t maxDisplacement = 1LL << 20U;  // imm19 scaled by 4: +1 MiB, exclusive.
        return displacement >= minDisplacement && displacement < maxDisplacement;
    }
    if (IsVeneerTestBranch(instruction)) {
        constexpr int64_t minDisplacement = -(1LL << 15U);  // imm14 scaled by 4: -32 KiB.
        constexpr int64_t maxDisplacement = 1LL << 15U;  // imm14 scaled by 4: +32 KiB, exclusive.
        return displacement >= minDisplacement && displacement < maxDisplacement;
    }
    LOG_COMPILER(FATAL) << "Unsupported ARM64 veneer branch instruction: " << std::hex << instruction;
    UNREACHABLE();
}

uint32_t ArkSteedAssembler::GetVeneerBranchDeadline(uint32_t branchPc, uint32_t instruction) const
{
    uint64_t maxForwardDisplacement;
    if (IsVeneerBranchOrCall(instruction)) {
        maxForwardDisplacement = 1ULL << 27U;  // B/BL byte reach from signed imm26.
    } else if (IsVeneerConditionOrCompareBranch(instruction)) {
        maxForwardDisplacement = 1ULL << 20U;  // B.cond/CBZ/CBNZ byte reach from signed imm19.
    } else if (IsVeneerTestBranch(instruction)) {
        maxForwardDisplacement = 1ULL << 15U;  // TBZ/TBNZ byte reach from signed imm14.
    } else {
        LOG_COMPILER(FATAL) << "Unsupported ARM64 veneer branch instruction: " << std::hex << instruction;
        UNREACHABLE();
    }

    uint64_t poolEntryCount = static_cast<uint64_t>(veneerBranches_.size()) + 1U;  // 1: optional guard branch.
    uint64_t poolReserve = poolEntryCount * VENEER_INSTRUCTION_SIZE;
    uint64_t reservedDisplacement = VENEER_DISTANCE_MARGIN + poolReserve;
    if (maxForwardDisplacement <= reservedDisplacement) {
        return branchPc;
    }
    uint64_t deadline = static_cast<uint64_t>(branchPc) + maxForwardDisplacement - reservedDisplacement;
    return static_cast<uint32_t>(std::min(deadline, static_cast<uint64_t>(UINT32_MAX)));
}

uint64_t ArkSteedAssembler::GetPotentialVeneerPoolSize() const
{
    if (veneerBranches_.empty()) {
        return 0;
    }
    uint64_t entryCount = static_cast<uint64_t>(veneerBranches_.size()) + 1U;  // 1: optional guard branch.
    return entryCount * VENEER_INSTRUCTION_SIZE;
}

void ArkSteedAssembler::UpdateEmbeddedLiteralPoolCheck(uint32_t loadOffset, size_t literalIndex)
{
    // A literal's position is the pool prefix followed by its index in the current pool.
    uint64_t literalOffsetInPool = static_cast<uint64_t>(literalIndex) * EMBEDDED_LITERAL_SIZE;
    uint64_t reserve = EMBEDDED_LITERAL_DISTANCE_MARGIN + EMBEDDED_LITERAL_POOL_PREFIX_RESERVE +
                       literalOffsetInPool;
    uint64_t maximumPoolPosition = static_cast<uint64_t>(loadOffset) +
                                   EMBEDDED_LITERAL_MAX_FORWARD_DISPLACEMENT;
    uint64_t deadline = maximumPoolPosition > reserve ? maximumPoolPosition - reserve : 0;
    nextEmbeddedLiteralPoolCheck_ = std::min(
        nextEmbeddedLiteralPoolCheck_,
        static_cast<uint32_t>(std::min(deadline, static_cast<uint64_t>(UINT32_MAX))));
}

uint64_t ArkSteedAssembler::GetEmbeddedLiteralPoolMaxSize() const
{
    return EMBEDDED_LITERAL_POOL_PREFIX_RESERVE +
           static_cast<uint64_t>(pendingEmbeddedLiterals_.size()) * EMBEDDED_LITERAL_SIZE;
}

void ArkSteedAssembler::EmitEmbeddedLiteralPool(bool precedingCodeCanFallThrough)
{
    ASSERT(!emittingEmbeddedLiteralPool_);
    if (pendingEmbeddedLiterals_.empty()) {
        nextEmbeddedLiteralPoolCheck_ = UINT32_MAX;
        return;
    }

    emittingEmbeddedLiteralPool_ = true;
    uint32_t guardPc = UINT32_MAX;
    if (precedingCodeCanFallThrough) {
        guardPc = GetPcOffset();
        assembler_.B(0);  // 0: placeholder branch displacement.
    }

    while ((GetPcOffset() % EMBEDDED_LITERAL_SIZE) != 0) {
        assembler_.EmitU32(aarch64::Nop);
    }

    constexpr uint8_t RELOC_WIDTH = sizeof(JSTaggedType);
    for (const auto &literal : pendingEmbeddedLiterals_) {
        uint32_t literalOffset = GetPcOffset();
        ASSERT(literalOffset % EMBEDDED_LITERAL_SIZE == 0);
        ASSERT(std::all_of(literal->loadOffsets.begin(), literal->loadOffsets.end(),
                           [literalOffset](uint32_t loadOffset) {
                               int64_t displacement =
                                   static_cast<int64_t>(literalOffset) - static_cast<int64_t>(loadOffset);
                               return displacement >= 0 &&
                                      displacement <= EMBEDDED_LITERAL_MAX_FORWARD_DISPLACEMENT &&
                                      displacement % sizeof(uint32_t) == 0;
                           }));
        assembler_.Bind(&literal->label);
        embeddedRefRelocations_.push_back({literalOffset,
                                          literal->handleIndex,
                                          EmbeddedCodeRefRelocKind::ARM64_LITERAL64,
                                          RELOC_WIDTH});
        assembler_.EmitU64(JSTaggedValue::VALUE_HOLE);
    }

    if (precedingCodeCanFallThrough) {
        PatchVeneerBranchTarget(guardPc, GetPcOffset());
    }
    pendingEmbeddedLiterals_.clear();
    embeddedLiteralIndexByHandle_.clear();
    nextEmbeddedLiteralPoolCheck_ = UINT32_MAX;
    emittingEmbeddedLiteralPool_ = false;
}

void ArkSteedAssembler::CheckCodePools(bool precedingCodeCanFallThrough, size_t protectedCodeSize)
{
    ASSERT(!emittingVeneerPool_);
    ASSERT(!emittingEmbeddedLiteralPool_);

    uint64_t protectedCodeEnd = static_cast<uint64_t>(GetPcOffset()) + protectedCodeSize;
    uint64_t potentialVeneerPoolSize = GetPotentialVeneerPoolSize();
    bool emitLiteralPool = !pendingEmbeddedLiterals_.empty() &&
        protectedCodeEnd + potentialVeneerPoolSize >= nextEmbeddedLiteralPoolCheck_;
    if (emitLiteralPool) {
        // Veneers emitted before this pool also consume displacement from pending literal loads.
        uint64_t literalPoolSize = GetEmbeddedLiteralPoolMaxSize();
        ASSERT(literalPoolSize <= std::numeric_limits<size_t>::max() - protectedCodeSize);
        CheckVeneerPool(precedingCodeCanFallThrough,
                        protectedCodeSize + static_cast<size_t>(literalPoolSize));
        EmitEmbeddedLiteralPool(precedingCodeCanFallThrough);
    }
    CheckVeneerPool(precedingCodeCanFallThrough, protectedCodeSize);
}

void ArkSteedAssembler::UpdateVeneerPoolCheck()
{
    nextVeneerPoolCheck_ = UINT32_MAX;  // No deadline until an unresolved branch supplies one.
    for (const auto &[_, branches] : veneerBranches_) {
        for (uint32_t branchPc : branches) {
            uint32_t deadline = GetVeneerBranchDeadline(branchPc, assembler_.GetU32(branchPc));
            nextVeneerPoolCheck_ = std::min(nextVeneerPoolCheck_, deadline);
        }
    }
}

void ArkSteedAssembler::RecordVeneerBranch(uint32_t branchPc, Label *target)
{
    auto [iter, inserted] = veneerBranches_.try_emplace(target, chunk_);
    iter->second.push_back(branchPc);

    if (inserted && nextVeneerPoolCheck_ != UINT32_MAX) {
        uint32_t additionalReserve = VENEER_INSTRUCTION_SIZE;
        nextVeneerPoolCheck_ = nextVeneerPoolCheck_ > additionalReserve
            ? nextVeneerPoolCheck_ - additionalReserve
            : 0;  // 0: force a full check at the next safe codegen boundary.
    }
    uint32_t deadline = GetVeneerBranchDeadline(branchPc, assembler_.GetU32(branchPc));
    nextVeneerPoolCheck_ = std::min(nextVeneerPoolCheck_, deadline);
}

void ArkSteedAssembler::PatchVeneerBranchTarget(uint32_t branchPc, uint32_t targetPc)
{
    uint32_t instruction = assembler_.GetU32(branchPc);
    int64_t displacement = static_cast<int64_t>(targetPc) - static_cast<int64_t>(branchPc);
    if (!IsVeneerBranchInRange(instruction, displacement)) {
        LOG_COMPILER(FATAL) << "ARM64 veneer branch is out of range: branchPc=" << branchPc
                            << ", targetPc=" << targetPc;
    }

    uint32_t encodedDisplacement = static_cast<uint32_t>(displacement / VENEER_INSTRUCTION_SIZE);
    if (IsVeneerBranchOrCall(instruction)) {
        instruction &= ~aarch64::BRANCH_Imm26_MASK;
        instruction |= (encodedDisplacement << aarch64::BRANCH_Imm26_LOWBITS) & aarch64::BRANCH_Imm26_MASK;
    } else if (IsVeneerConditionOrCompareBranch(instruction)) {
        instruction &= ~aarch64::BRANCH_Imm19_MASK;
        instruction |= (encodedDisplacement << aarch64::BRANCH_Imm19_LOWBITS) & aarch64::BRANCH_Imm19_MASK;
    } else if (IsVeneerTestBranch(instruction)) {
        instruction &= ~aarch64::BRANCH_Imm14_MASK;
        instruction |= (encodedDisplacement << aarch64::BRANCH_Imm14_LOWBITS) & aarch64::BRANCH_Imm14_MASK;
    } else {
        UNREACHABLE();
    }
    assembler_.PutI32(branchPc, static_cast<int32_t>(instruction));
}

void ArkSteedAssembler::BindVeneerLabel(Label *label)
{
    ASSERT(!label->IsBound());
    ASSERT(!label->IsLinked());
    uint32_t targetPc = GetPcOffset();
    auto iter = veneerBranches_.find(label);
    if (iter != veneerBranches_.end()) {
        for (uint32_t branchPc : iter->second) {
            PatchVeneerBranchTarget(branchPc, targetPc);
        }
        veneerBranches_.erase(iter);
    }
    label->BindTo(static_cast<int32_t>(targetPc));
}

void ArkSteedAssembler::CheckVeneerPool(bool precedingCodeCanFallThrough, size_t protectedCodeSize)
{
    ASSERT(!emittingVeneerPool_);
    if (veneerBranches_.empty()) {
        nextVeneerPoolCheck_ = UINT32_MAX;  // No unresolved branch needs another pool check.
        return;
    }

    uint32_t currentPc = GetPcOffset();
    uint64_t protectedCodeEnd = static_cast<uint64_t>(currentPc) + protectedCodeSize;
    if (protectedCodeEnd < nextVeneerPoolCheck_) {
        return;
    }

    uint64_t poolEntryCount = static_cast<uint64_t>(veneerBranches_.size()) + 1U;  // 1: optional guard branch.
    uint64_t poolReserve = poolEntryCount * VENEER_INSTRUCTION_SIZE;
    // No pool check may occur inside protectedCodeSize. Test unresolved
    // branches against the far side of that range and emit their veneers now.
    uint64_t prospectivePoolEnd = protectedCodeEnd + VENEER_DISTANCE_MARGIN + poolReserve;
    ChunkVector<std::pair<uint32_t, Label *>> candidates(chunk_);
    for (const auto &[target, branches] : veneerBranches_) {
        uint32_t firstDeadline = UINT32_MAX;  // Sentinel until this target's first branch is examined.
        bool needsVeneer = false;
        for (uint32_t branchPc : branches) {
            uint32_t instruction = assembler_.GetU32(branchPc);
            firstDeadline = std::min(firstDeadline, GetVeneerBranchDeadline(branchPc, instruction));
            int64_t displacement = static_cast<int64_t>(prospectivePoolEnd) - static_cast<int64_t>(branchPc);
            needsVeneer = needsVeneer || !IsVeneerBranchInRange(instruction, displacement);
        }
        if (needsVeneer) {
            candidates.emplace_back(firstDeadline, target);
        }
    }

    if (candidates.empty()) {
        UpdateVeneerPoolCheck();
        return;
    }
    std::sort(candidates.begin(), candidates.end(), [](const auto &left, const auto &right) {
        return left.first < right.first;
    });

    emittingVeneerPool_ = true;
    uint32_t guardPc = UINT32_MAX;  // Sentinel used when fallthrough does not require a guard.
    if (precedingCodeCanFallThrough) {
        guardPc = GetPcOffset();
        assembler_.B(0);  // 0: placeholder branch displacement.
    }

    for (const auto &[_, target] : candidates) {
        auto iter = veneerBranches_.find(target);
        ASSERT(iter != veneerBranches_.end());
        uint32_t veneerPc = GetPcOffset();
        assembler_.B(0);  // 0: placeholder branch displacement.
        for (uint32_t branchPc : iter->second) {
            PatchVeneerBranchTarget(branchPc, veneerPc);
        }
        iter->second.clear();
        iter->second.push_back(veneerPc);
    }

    if (precedingCodeCanFallThrough) {
        PatchVeneerBranchTarget(guardPc, GetPcOffset());
    }
    emittingVeneerPool_ = false;
    UpdateVeneerPoolCheck();
}

void ArkSteedAssembler::FinalizeVeneers()
{
    if (!veneerBranches_.empty()) {
        LOG_COMPILER(FATAL) << "Unbound labels remain after ARM64 veneer finalization";
    }
}

void ArkSteedAssembler::Jump(Label *target)
{
    uint32_t branchPc = GetPcOffset();
    assembler_.B(0);  // 0: placeholder branch displacement.
    if (target->IsBound()) {
        PatchVeneerBranchTarget(branchPc, target->GetPos());
        return;
    }
    RecordVeneerBranch(branchPc, target);
}

void ArkSteedAssembler::JumpIf(Condition condition, Label *target)
{
    if (target->IsBound()) {
        int64_t displacement = static_cast<int64_t>(target->GetPos()) - static_cast<int64_t>(GetPcOffset());
        if (!IsVeneerBranchInRange(aarch64::BranchOpCode::BranchCond, displacement)) {
            assembler_.B(ToPhysicalCondition(NegateCondition(condition)), 2);  // 2: skip the following B.
            Jump(target);
            return;
        }
    }

    uint32_t branchPc = GetPcOffset();
    assembler_.B(ToPhysicalCondition(condition), 0);  // 0: placeholder branch displacement.
    if (target->IsBound()) {
        PatchVeneerBranchTarget(branchPc, target->GetPos());
        return;
    }
    RecordVeneerBranch(branchPc, target);
}

void ArkSteedAssembler::TestAndBranchIfZero(ArkSteedRegister value, int32_t bit, Label *target)
{
    if (target->IsBound()) {
        int64_t displacement = static_cast<int64_t>(target->GetPos()) - static_cast<int64_t>(GetPcOffset());
        if (!IsVeneerBranchInRange(aarch64::BranchOpCode::TBZ, displacement)) {
            assembler_.Tbnz(value, bit, 2);  // 2: skip the following B.
            Jump(target);
            return;
        }
    }

    uint32_t branchPc = GetPcOffset();
    assembler_.Tbz(value, bit, 0);  // 0: placeholder branch displacement.
    if (target->IsBound()) {
        PatchVeneerBranchTarget(branchPc, target->GetPos());
        return;
    }
    RecordVeneerBranch(branchPc, target);
}

void ArkSteedAssembler::TestAndBranchIfNotZero(ArkSteedRegister value, int32_t bit, Label *target)
{
    if (target->IsBound()) {
        int64_t displacement = static_cast<int64_t>(target->GetPos()) - static_cast<int64_t>(GetPcOffset());
        if (!IsVeneerBranchInRange(aarch64::BranchOpCode::TBNZ, displacement)) {
            assembler_.Tbz(value, bit, 2);  // 2: skip the following B.
            Jump(target);
            return;
        }
    }

    uint32_t branchPc = GetPcOffset();
    assembler_.Tbnz(value, bit, 0);  // 0: placeholder branch displacement.
    if (target->IsBound()) {
        PatchVeneerBranchTarget(branchPc, target->GetPos());
        return;
    }
    RecordVeneerBranch(branchPc, target);
}

void ArkSteedAssembler::JumpIfNotTaggedHeapObject(ArkSteedRegister value, Label *target)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.Acquire();
    Move(scratch, value);
    And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
    Compare(scratch, 0);
    JumpIf(Condition::NOT_EQUAL, target);
}

void ArkSteedAssembler::JumpIfNotJSFunction(ArkSteedRegister value, Label *target)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister hclass = scope.Acquire();
    ArkSteedRegister objectType = scope.Acquire();
    LoadField(hclass, value, TaggedObject::HCLASS_OFFSET);
    And(hclass, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    LoadField(objectType, hclass, JSHClass::BIT_FIELD_OFFSET);
    And(objectType, static_cast<int32_t>((1U << JSHClass::TYPE_BITFIELD_NUM) - 1));
    Compare(objectType, static_cast<int32_t>(JSType::JS_FUNCTION_FIRST));
    JumpIf(Condition::LESS_THAN, target);
    Compare(objectType, static_cast<int32_t>(JSType::JS_FUNCTION_LAST));
    JumpIf(Condition::GREATER_THAN, target);
}

void ArkSteedAssembler::JumpIfClassConstructor(ArkSteedRegister jsFunc, Label *target)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister hclass = scope.Acquire();
    ArkSteedRegister bitfield = scope.Acquire();
    Label notClassConstructor;
    LoadField(hclass, jsFunc, TaggedObject::HCLASS_OFFSET);
    And(hclass, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    LoadField(bitfield, hclass, JSHClass::BIT_FIELD_OFFSET);
    TestAndBranchIfZero(bitfield, JSHClass::IsClassConstructorOrPrototypeBit::START_BIT, &notClassConstructor);
    TestAndBranchIfNotZero(bitfield, JSHClass::ConstructorBit::START_BIT, target);
    Bind(&notClassConstructor);
}

void ArkSteedAssembler::JumpIfFunctionNotCompiled(ArkSteedRegister jsFunc, Label *target)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister bitfield = scope.Acquire();
    LoadField(bitfield, jsFunc, JSFunctionBase::BIT_FIELD_OFFSET);
    TestAndBranchIfZero(bitfield, JSFunctionBase::IsCompiledCodeBit::START_BIT, target);
}

void ArkSteedAssembler::BranchIfNoPendingException(Label* target)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    PushPair(aarch64::xzr, aarch64::x0);
    Move(aarch64::x0, static_cast<uint64_t>(entryThread_->GetGlueAddr()));
    LoadField(scratch, aarch64::x0, static_cast<int32_t>(JSThread::GlueData::GetExceptionOffset(false)));
    Move(aarch64::x0, JSTaggedValue::Hole().GetRawData());
    Compare(scratch, aarch64::x0);
    PopPair(aarch64::xzr, aarch64::x0);
    JumpIf(Condition::EQUAL, target);
}

void ArkSteedAssembler::ReturnWithPendingException()
{
    Move(aarch64::x0, JSTaggedValue::Exception().GetRawData());
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
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    size_t offset = JSThread::GlueData::GetExceptionOffset(false);  // false : isArch32 = false

    LoadField(dst, glue, static_cast<int32_t>(offset));
    Move(scratch, JSTaggedValue::Hole().GetRawData());
    StoreField(scratch, glue, static_cast<int32_t>(offset));
}

void ArkSteedAssembler::Bind(Label *label)
{
    BindVeneerLabel(label);
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
    uint32_t branchPc = GetPcOffset();
    assembler_.Bl(0);  // 0: placeholder branch displacement.
    if (target->IsBound()) {
        PatchVeneerBranchTarget(branchPc, target->GetPos());
        return;
    }
    RecordVeneerBranch(branchPc, target);
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
    PushPair(reg, aarch64::xzr);
}

void ArkSteedAssembler::Pop(ArkSteedRegister reg)
{
    PopPair(reg, aarch64::xzr);
}

void ArkSteedAssembler::PushPair(ArkSteedRegister reg1, ArkSteedRegister reg2)
{
    // -16: space for two 64-bit registers (16 bytes)
    aarch64::MemoryOperand operand(aarch64::sp, -16, aarch64::AddrMode::PREINDEX);
    // Match two sequential pushes while keeping SP 16-byte aligned.
    assembler_.Stp(reg2, reg1, operand);
}

void ArkSteedAssembler::PopPair(ArkSteedRegister reg1, ArkSteedRegister reg2)
{
    // 16: space for two 64-bit registers (16 bytes)
    aarch64::MemoryOperand operand(aarch64::sp, 16, aarch64::AddrMode::POSTINDEX);
    assembler_.Ldp(reg2, reg1, operand);
}

void ArkSteedAssembler::PushPair(ArkSteedDoubleRegister reg1, ArkSteedDoubleRegister reg2)
{
    aarch64::MemoryOperand operand(aarch64::sp, -2 * FRAME_SLOT_SIZE, aarch64::AddrMode::PREINDEX);
    assembler_.Stp(reg2, reg1, operand);
}

void ArkSteedAssembler::PopPair(ArkSteedDoubleRegister reg1, ArkSteedDoubleRegister reg2)
{
    aarch64::MemoryOperand operand(aarch64::sp, 2 * FRAME_SLOT_SIZE, aarch64::AddrMode::POSTINDEX);
    assembler_.Ldp(reg2, reg1, operand);
}

void ArkSteedAssembler::Push(ArkSteedDoubleRegister reg)
{
    aarch64::MemoryOperand operand(aarch64::sp, -2 * FRAME_SLOT_SIZE, aarch64::AddrMode::PREINDEX);
    assembler_.Str(reg, operand);
}

void ArkSteedAssembler::Pop(ArkSteedDoubleRegister reg)
{
    aarch64::MemoryOperand operand(aarch64::sp, 2 * FRAME_SLOT_SIZE, aarch64::AddrMode::POSTINDEX);
    assembler_.Ldr(reg, operand);
}

void ArkSteedAssembler::PushAll(const ArkSteedRegList &registerList)
{
    ArkSteedRegList registers = registerList;
    while (registers.Count() > 1U) {
        ArkSteedRegister reg1 = registers.PopFirst();
        ArkSteedRegister reg2 = registers.PopFirst();
        PushPair(reg1, reg2);
    }
    if (!registers.IsEmpty()) {
        Push(registers.PopFirst());
    }
}

void ArkSteedAssembler::PopAll(const ArkSteedRegList &registerList)
{
    ArkSteedRegList registers = registerList;
    if ((registers.Count() & 1U) != 0) {
        Pop(registers.PopLast());
    }
    while (!registers.IsEmpty()) {
        ArkSteedRegister reg2 = registers.PopLast();
        ArkSteedRegister reg1 = registers.PopLast();
        PopPair(reg1, reg2);
    }
}

void ArkSteedAssembler::PushAll(const ArkDoubleRegList &registerList)
{
    ArkDoubleRegList registers = registerList;
    while (registers.Count() > 1U) {
        ArkSteedDoubleRegister reg1 = registers.PopFirst();
        ArkSteedDoubleRegister reg2 = registers.PopFirst();
        PushPair(reg1, reg2);
    }
    if (!registers.IsEmpty()) {
        Push(registers.PopFirst());
    }
}

void ArkSteedAssembler::PopAll(const ArkDoubleRegList &registerList)
{
    ArkDoubleRegList registers = registerList;
    if ((registers.Count() & 1U) != 0) {
        Pop(registers.PopLast());
    }
    while (!registers.IsEmpty()) {
        ArkSteedDoubleRegister reg2 = registers.PopLast();
        ArkSteedDoubleRegister reg1 = registers.PopLast();
        PopPair(reg1, reg2);
    }
}

void ArkSteedAssembler::ReserveCallArgSlots(int32_t slotCount)
{
    if (slotCount > 0) {
        Sub(aarch64::sp, slotCount * FRAME_SLOT_SIZE);
    }
}

void ArkSteedAssembler::FreeCallArgSlots(int32_t slotCount)
{
    if (slotCount > 0) {
        Add(aarch64::sp, slotCount * FRAME_SLOT_SIZE);
    }
}

void ArkSteedAssembler::ReserveCallArgSlots(ArkSteedRegister slotCount)
{
    static constexpr int FRAME_SLOT_SIZE_LOG2 = 3;
    assembler_.Sub(aarch64::sp, aarch64::sp, aarch64::Operand(slotCount, aarch64::UXTW, FRAME_SLOT_SIZE_LOG2));
}

void ArkSteedAssembler::FreeCallArgSlots(ArkSteedRegister slotCount)
{
    static constexpr int FRAME_SLOT_SIZE_LOG2 = 3;
    assembler_.Add(aarch64::sp, aarch64::sp, aarch64::Operand(slotCount, aarch64::UXTW, FRAME_SLOT_SIZE_LOG2));
}

void ArkSteedAssembler::RestoreStackPointerToFrameBottom(Graph *graph)
{
    uint32_t taggedSlots = graph->GetTaggedStackSlots();
    uint32_t untaggedSlots = graph->GetUntaggedStackSlots();
    uint32_t frameSlots = 3 + taggedSlots + untaggedSlots;  // 3: frameType, jsFunc, lexicalEnv.
    if (((taggedSlots + untaggedSlots) & 1U) == 0) {
        frameSlots++;
    }
    int32_t offset = -static_cast<int32_t>(frameSlots * FRAME_SLOT_SIZE);
    Move(aarch64::sp, kFramePointerRegister);
    Add(aarch64::sp, offset);
}

void ArkSteedAssembler::PushUndefinedForSteedCall(ArkSteedRegister fillSlotCount, uint32_t userArgc)
{
    Label fillUndefined;
    Label fillDone;
    Compare(fillSlotCount, 0);
    JumpIf(Condition::LESS_THAN_OR_EQUAL, &fillDone);

    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    constexpr int32_t SLOT_INDEX_SHIFT = 3;
    const int32_t firstUndefinedArgBaseSlot = static_cast<int32_t>(NUM_MANDATORY_JSFUNC_ARGS + userArgc);
    Move(scratch, static_cast<int64_t>(JSTaggedValue::VALUE_UNDEFINED));
    Add(fillSlotCount, firstUndefinedArgBaseSlot);
    Bind(&fillUndefined);
    assembler_.Str(scratch, MemoryOperand(aarch64::sp, fillSlotCount, aarch64::UXTW, SLOT_INDEX_SHIFT));
    Sub(fillSlotCount, 1);
    Compare(fillSlotCount, firstUndefinedArgBaseSlot);
    JumpIf(Condition::GREATER_THAN, &fillUndefined);
    Bind(&fillDone);
}

void ArkSteedAssembler::PrepareSteedCalleeContext(ArkSteedRegister target, ArkSteedRegister codeEntry)
{
    Move(aarch64::x20, target);
    LoadField(aarch64::x19, aarch64::x20, JSFunction::LEXICAL_ENV_OFFSET);
    LoadField(codeEntry, aarch64::x20, JSFunction::CODE_ENTRY_OFFSET);
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
    PushPair(tmp, x20);
    assembler_.Mov(tmp, aarch64::Immediate(JSTaggedValue::VALUE_UNDEFINED));
    PushPair(x19, tmp);
    for (size_t i = 0; i < taggedSlots / 2; i++) {  // 2: push two slots at a time
        PushPair(tmp, tmp);
    }

    if (untaggedSlots > 0) {
        size_t slotSize = untaggedSlots * sizeof(uint64_t);
        ASSERT(slotSize <= static_cast<size_t>(std::numeric_limits<int32_t>::max()));
        Sub(aarch64::sp, static_cast<int32_t>(slotSize));
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
        case Condition::EQUAL:
            return aarch64::Condition::EQ;
        case Condition::NOT_EQUAL:
            return aarch64::Condition::NE;
        case Condition::LESS_THAN:
            return aarch64::Condition::LT;
        case Condition::LESS_THAN_OR_EQUAL:
            return aarch64::Condition::LE;
        case Condition::GREATER_THAN:
            return aarch64::Condition::GT;
        case Condition::GREATER_THAN_OR_EQUAL:
            return aarch64::Condition::GE;
        case Condition::ABOVE:
            return aarch64::Condition::HI;
        case Condition::BELOW:
            return aarch64::Condition::LO;
        case Condition::ABOVE_OR_EQUAL:
            return aarch64::Condition::HS;
        case Condition::BELOW_OR_EQUAL:
            return aarch64::Condition::LS;
        case Condition::ZERO:
            return aarch64::Condition::EQ;
        case Condition::NOT_ZERO:
            return aarch64::Condition::NE;
        case Condition::OVERFLOW:
            return aarch64::Condition::VS;
        case Condition::NOT_OVERFLOW:
            return aarch64::Condition::VC;
        case Condition::PARITY:
            return aarch64::Condition::VS;
        case Condition::NOT_PARITY:
            return aarch64::Condition::VC;
        default:
            UNREACHABLE();
    }
}

#endif
}  // namespace panda::ecmascript::arksteed
