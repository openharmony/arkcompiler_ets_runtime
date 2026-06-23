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

#include "ecmascript/arksteed/arksteed_opcode.h"

#include "ecmascript/arksteed/arksteed_assembler-inl.h"
#include "ecmascript/arksteed/arksteed_bb.h"
#include "ecmascript/arksteed/arksteed_graph_processor.h"

namespace panda::ecmascript::arksteed {

namespace {
void UseDeoptFrameSlots(Vertex *vertex, const DeoptimizableMixin *deopt)
{
    for (uint32_t index = 0; index < deopt->DeoptInputCount(); ++index) {
        UseSlot(vertex->Arg(deopt->DeoptInputIndex(index)));
    }
}
}  // namespace

#define __ masm->

// Helper function for stub calls (CallRuntime, CallCommonStub)
// Uses C calling convention (CCallConv) in arksteed
void SetStubValueLocationConstraints(Vertex *vertex)
{
    size_t inputCount = static_cast<size_t>(vertex->GetInputCount());
    // For parameters that fit in argument registers, use fixed registers
    // For additional parameters, allow them to be in register or slot
    for (size_t paramIdx = 0; paramIdx < inputCount; paramIdx++) {
        if (paramIdx < static_cast<size_t>(ArkSteedAssembler::NUM_ARG_REGISTERS)) {
            ArkSteedRegister paramReg = ArkSteedAssembler::GetParameterRegister(static_cast<int>(paramIdx));
            UseFixed(vertex->Arg(paramIdx), static_cast<uint32_t>(paramReg.Code()));
        } else {
            UseAny(vertex->Arg(paramIdx));
        }
    }
}

inline ArkSteedRegister GetInputRegister(const Vertex *vertex, int index)
{
    const InputLocation *loc = vertex->GetInputLocation(index);
    ASSERT(loc->IsRegister());
    return loc->GetAssignedGeneralRegister();
}

inline ArkSteedRegister GetResultRegister(const ValueVertex *vertex)
{
    const ValueLocation &loc = vertex->Result();
    ASSERT(loc.IsRegister());
    return loc.GetAssignedGeneralRegister();
}

// ========================================= Common Value Opcode =========================================

void Int32ConstantVertex::DoLoadToRegister(ArkSteedAssembler *masm, ArkSteedRegister reg) const
{
    __ Move(reg, GetValue());
}

void Int32ConstantVertex::SetValueLocationConstraints()
{
    DefineAsConstant(this);
}

void Int32ConstantVertex::Dump(std::ostream &output) const
{
    output << "  Int32Constant: " << GetValue();
}

void IntPtrConstantVertex::DoLoadToRegister(ArkSteedAssembler *masm, ArkSteedRegister reg) const
{
    __ Move(reg, static_cast<int64_t>(GetValue()));
}

void IntPtrConstantVertex::SetValueLocationConstraints()
{
    DefineAsConstant(this);
}

void IntPtrConstantVertex::Dump(std::ostream &output) const
{
    output << "  IntPtrConstant: " << GetValue();
}

void Float64ConstantVertex::DoLoadToRegister(ArkSteedAssembler *masm, ArkSteedDoubleRegister reg) const
{
    __ Move(reg, GetValue());
}

void Float64ConstantVertex::SetValueLocationConstraints()
{
    DefineAsConstant(this);
}

void Float64ConstantVertex::Dump(std::ostream &output) const
{
    output << "  Float64Constant: " << GetValue();
}

void TaggedConstantVertex::SetValueLocationConstraints()
{
    DefineAsConstant(this);
}

void TaggedConstantVertex::DoLoadToRegister(ArkSteedAssembler *masm, ArkSteedRegister reg) const
{
    __ Move(reg, GetValue());
}

void TaggedConstantVertex::Dump(std::ostream &output) const
{
    output << "  TaggedConstant: 0x" << std::hex << GetValue() << std::dec;
}

void InitialValueVertex::SetValueLocationConstraints()
{
    // Convert fp-slot index (word units from fp) to tagged-slot index.
    // tagged_slot[0] is at fp-slot -4 (i.e. FP + 4n where n = pointer size).
    constexpr int32_t kTaggedSlot0FpSlotIndex = -4;
    int32_t taggedSlotIndex = kTaggedSlot0FpSlotIndex - frameSlotIndex_;
    Result().SetUnallocated(UnallocatedState::BasicPolicy::FIXED_SLOT, taggedSlotIndex);
}

void InitialValueVertex::Dump(std::ostream &output) const
{
    output << "  InitialValue frameSlot: " << frameSlotIndex_;
}

void ActualArgcVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
}

void ActualArgcVertex::Dump(std::ostream &output) const
{
    output << "  ActualArgc";
}

void CallRuntimeVertex::SetValueLocationConstraints()
{
    // Define return value in x0/rax (C calling convention)
    DefineAsFixed(this, 0);

    // Set parameter location constraints using helper function
    SetStubValueLocationConstraints(this);
}

void CallRuntimeVertex::Dump(std::ostream &output) const
{
    output << "  CallRuntime (id=" << static_cast<int>(runtimeId_) << ") with " << GetInputCount() << " args";
}

void CallVertex::SetValueLocationConstraints()
{
    DefineAsFixed(this, 0);
    SetTemporariesNeeded(1);
    UseRegister(Arg(TARGET_INDEX));
    for (int i = NEW_TARGET_INDEX; i < GetInputCount(); i++) {
        UseAny(Arg(i));
    }
}

void CallVertex::Dump(std::ostream &output) const
{
    output << "  Call actualArgc=" << actualArgc_;
}

void CallCommonStubVertex::SetValueLocationConstraints()
{
    // Define return value in x0/rax (C calling convention)
    DefineAsFixed(this, 0);

    // Set parameter location constraints using helper function
    SetStubValueLocationConstraints(this);
}

void CallCommonStubVertex::Dump(std::ostream &output) const
{
    output << "  CallCommonStub (id=" << stubId_ << ") with " << GetInputCount() << " args";
}

void DeoptIfHClassMismatchVertex::SetValueLocationConstraints()
{
    SetTemporariesNeeded(2);  // 2: actual hclass and expected hclass
    UseRegister(Arg(RECEIVER_INDEX));
    for (uint32_t i = RECEIVER_INDEX + 1, n = GetInputCount(); i < n; i++) {
        UseSlot(Arg(i));
    }
}

void DeoptIfHClassMismatchVertex::Dump(std::ostream &output) const
{
    output << "  DeoptIfHClassMismatch: expected=0x"
           << std::hex << reinterpret_cast<uintptr_t>(expectedHClass_) << std::dec
           << ", pc=" << GetBytecodeOffset();
}

void DeoptIfInt32ConditionVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
    UseDeoptFrameSlots(this, this);
}

void DeoptIfInt32ConditionVertex::Dump(std::ostream &output) const
{
    output << "  DeoptIfInt32Condition: condition=" << static_cast<uint32_t>(condition_)
           << ", type=" << static_cast<int>(deoptType_) << ", pc=" << GetBytecodeOffset();
}

void DeoptVertex::SetValueLocationConstraints()
{
    for (uint32_t i = 0, n = GetInputCount(); i < n; i++) {
        UseSlot(Arg(i));
    }
}

void DeoptVertex::Dump(std::ostream &output) const
{
    output << "  Deopt (type=" << static_cast<int>(deoptType_) << ", pc=" << GetBytecodeOffset() << ")";
}

// ========================================= Slow Value Opcode =========================================

void LoadFromAddressVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
}

void LoadFromAddressVertex::Dump(std::ostream &output) const
{
    output << "  LoadFromAddress: offset=" << offset_;
}

void LoadExceptionVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(GLUE_INDEX));
}

void LoadExceptionVertex::Dump(std::ostream &output) const
{
    output << "  LoadException";
}

void LoadTaggedFieldVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
}

void LoadTaggedFieldVertex::Dump(std::ostream &output) const
{
    output << "  LoadTaggedField: offset=" << offset_;
}

void StoreToAddressVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void StoreToAddressVertex::Dump(std::ostream &output) const
{
    output << "  StoreToAddress: offset=" << offset_;
}

void StoreTaggedFieldVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void StoreTaggedFieldVertex::Dump(std::ostream &output) const
{
    output << "  StoreTaggedField: offset=" << offset_;
}

void StoreEnvSlotVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(ENV_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void StoreEnvSlotVertex::Dump(std::ostream &output) const
{
    output << "  StoreEnvSlot: offset=" << offset_;
}

void SetValueWithBarrierVertex::SetValueLocationConstraints()
{
    UseFixed(Arg(GLUE_INDEX), static_cast<uint32_t>(ArkSteedAssembler::GetParameterRegister(0).Code()));
    UseFixed(Arg(OBJECT_INDEX), static_cast<uint32_t>(ArkSteedAssembler::GetParameterRegister(1).Code()));
    UseFixed(Arg(VALUE_INDEX), static_cast<uint32_t>(ArkSteedAssembler::GetParameterRegister(3).Code()));
}

void SetValueWithBarrierVertex::Dump(std::ostream &output) const
{
    output << "  SetValueWithBarrier: offset=" << offset_;
}

void TaggedIntToI32Vertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
}

void TaggedIntToI32Vertex::Dump(std::ostream &output) const
{
    output << "  TaggedIntToI32";
}

void CheckedTaggedIntToI32Vertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
    UseDeoptFrameSlots(this, this);
    SetTemporariesNeeded(1);
}

void CheckedTaggedIntToI32Vertex::Dump(std::ostream &output) const
{
    output << "  CheckedTaggedIntToI32";
}

void CheckedTaggedStringVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(INPUT_INDEX));
    UseDeoptFrameSlots(this, this);
    DefineSameAsFirst(this);
    SetTemporariesNeeded(1);
}

void CheckedTaggedStringVertex::Dump(std::ostream &output) const
{
    output << "  CheckedTaggedString";
}

template <class VertexT>
void VerifyI32BinaryDeoptInputs(const VertexT *vertex)
{
    ASSERT(vertex->FirstDeoptInputIndex() == VertexT::FIRST_DEOPT_INDEX);
    ASSERT(vertex->GetInputCount() == static_cast<int>(vertex->FirstDeoptInputIndex() + vertex->DeoptInputCount()));
    ASSERT(vertex->GetInput(VertexT::LEFT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
    ASSERT(vertex->GetInput(VertexT::RIGHT_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
}

void I32AddWithOverflowVertex::VerifyI32BinOpInputs() const
{
    VerifyI32BinaryDeoptInputs(this);
}

void I32SubWithOverflowVertex::VerifyI32BinOpInputs() const
{
    VerifyI32BinaryDeoptInputs(this);
}

#define DEFINE_I32_WITH_OVERFLOW_CONSTRAINTS(Name, DumpName)          \
    void I32##Name##WithOverflowVertex::SetValueLocationConstraints() \
    {                                                                   \
        UseRegister(Arg(LEFT_INDEX));                                   \
        UseRegister(Arg(RIGHT_INDEX));                                  \
        UseDeoptFrameSlots(this, this);                                \
        DefineSameAsFirst(this);                                        \
    }                                                                   \
                                                                        \
    void I32##Name##WithOverflowVertex::Dump(std::ostream &output) const \
    {                                                                   \
        output << "  I32" DumpName "WithOverflow";                      \
    }

DEFINE_I32_WITH_OVERFLOW_CONSTRAINTS(Add, "Add")
DEFINE_I32_WITH_OVERFLOW_CONSTRAINTS(Sub, "Sub")
#undef DEFINE_I32_WITH_OVERFLOW_CONSTRAINTS

void I32MulWithOverflowVertex::VerifyI32BinOpInputs() const
{
    VerifyI32BinaryDeoptInputs(this);
}

void I32DivWithOverflowVertex::VerifyI32BinOpInputs() const
{
    VerifyI32BinaryDeoptInputs(this);
}

void I32MulWithOverflowVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
    UseDeoptFrameSlots(this, this);
    DefineSameAsFirst(this);
    SetTemporariesNeeded(1);
}

void I32MulWithOverflowVertex::Dump(std::ostream &output) const
{
    output << "  I32MulWithOverflow";
}

void I32DivWithOverflowVertex::SetValueLocationConstraints()
{
#if defined(PANDA_TARGET_AMD64)
    DefineAsFixed(this, static_cast<uint32_t>(x64::rax.Code()));
    UseFixed(Arg(LEFT_INDEX), static_cast<uint32_t>(x64::rax.Code()));
    UseFixed(Arg(RIGHT_INDEX), static_cast<uint32_t>(x64::rcx.Code()));
    GetRegallocInfo()->AddGeneralTemporary(x64::rdx);
#else
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
#endif
    UseDeoptFrameSlots(this, this);
}

void I32DivWithOverflowVertex::Dump(std::ostream &output) const
{
    output << "  I32DivWithOverflow";
}

void I32DivByConstWithCheckVertex::SetValueLocationConstraints()
{
#if defined(PANDA_TARGET_AMD64)
    DefineAsFixed(this, static_cast<uint32_t>(x64::rax.Code()));
    UseFixed(Arg(INPUT_INDEX), static_cast<uint32_t>(x64::rax.Code()));
    GetRegallocInfo()->AddGeneralTemporary(x64::rdx);
    GetRegallocInfo()->AddGeneralTemporary(x64::rcx);
    GetRegallocInfo()->AddGeneralTemporary(x64::r8);
#else
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
    SetTemporariesNeeded(2);
#endif
    UseDeoptFrameSlots(this, this);
}

void I32DivByConstWithCheckVertex::Dump(std::ostream &output) const
{
    output << "  I32DivByConstWithCheck: divisor=" << divisor_ << ", magic=" << magic_ << ", shift=" << shift_;
}

void I32AddVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void I32AddVertex::Dump(std::ostream &output) const
{
    output << "  I32Add";
}

void I32SubVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void I32SubVertex::Dump(std::ostream &output) const
{
    output << "  I32Sub";
}

void I32MulVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void I32MulVertex::Dump(std::ostream &output) const
{
    output << "  I32Mul";
}

void I32DivVertex::SetValueLocationConstraints()
{
#if defined(PANDA_TARGET_AMD64)
    DefineAsFixed(this, static_cast<uint32_t>(x64::rax.Code()));
    UseFixed(Arg(LEFT_INDEX), static_cast<uint32_t>(x64::rax.Code()));
    UseFixed(Arg(RIGHT_INDEX), static_cast<uint32_t>(x64::rcx.Code()));
    GetRegallocInfo()->AddGeneralTemporary(x64::rdx);
#else
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
#endif
}

void I32DivVertex::Dump(std::ostream &output) const
{
    output << "  I32Div";
}

void PositiveI32ModVertex::SetValueLocationConstraints()
{
#if defined(PANDA_TARGET_AMD64)
    DefineAsRegister(this);
    UseFixed(Arg(LEFT_INDEX), static_cast<uint32_t>(x64::rax.Code()));
    UseFixed(Arg(RIGHT_INDEX), static_cast<uint32_t>(x64::rcx.Code()));
    GetRegallocInfo()->AddGeneralTemporary(x64::rdx);
#else
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
#endif
}

void PositiveI32ModVertex::Dump(std::ostream &output) const
{
    output << "  PositiveI32Mod";
}

void CheckedPositiveI32ModVertex::SetValueLocationConstraints()
{
#if defined(PANDA_TARGET_AMD64)
    DefineAsRegister(this);
    UseFixed(Arg(LEFT_INDEX), static_cast<uint32_t>(x64::rax.Code()));
    UseFixed(Arg(RIGHT_INDEX), static_cast<uint32_t>(x64::rcx.Code()));
    GetRegallocInfo()->AddGeneralTemporary(x64::rdx);
#else
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
#endif
    UseDeoptFrameSlots(this, this);
}

void CheckedPositiveI32ModVertex::Dump(std::ostream &output) const
{
    output << "  CheckedPositiveI32Mod";
}

void I32BitwiseBinaryVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    if (RightInputIsConstant()) {
        UseAny(Arg(RIGHT_INDEX));
    } else if (IsShift()) {
#if defined(PANDA_TARGET_AMD64)
        UseFixed(Arg(RIGHT_INDEX), static_cast<uint32_t>(x64::rcx.Code()));
#else
        UseRegister(Arg(RIGHT_INDEX));
#endif
    } else {
        UseRegister(Arg(RIGHT_INDEX));
    }
    DefineSameAsFirst(this);
}

void I32BitwiseBinaryVertex::Dump(std::ostream &output) const
{
    output << "  I32BitwiseBinary: kind=" << static_cast<uint32_t>(kind_);
}

void CheckedNonNegativeI32ToTaggedIntVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
    UseDeoptFrameSlots(this, this);
    SetTemporariesNeeded(1);
}

void CheckedNonNegativeI32ToTaggedIntVertex::Dump(std::ostream &output) const
{
    output << "  CheckedNonNegativeI32ToTaggedInt";
}

void I32ToF64Vertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
}

void I32ToF64Vertex::Dump(std::ostream &output) const
{
    output << "  I32ToF64";
}

void CheckedNumberToF64Vertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
    UseDeoptFrameSlots(this, this);
    SetTemporariesNeeded(2);
}

void CheckedNumberToF64Vertex::Dump(std::ostream &output) const
{
    output << "  CheckedNumberToF64";
}

void F64ToTaggedDoubleVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
    SetTemporariesNeeded(1);
}

void F64ToTaggedDoubleVertex::Dump(std::ostream &output) const
{
    output << "  F64ToTaggedDouble";
}

#define DEFINE_F64_BINOP_CONSTRAINTS(Name)       \
    void F64##Name##Vertex::SetValueLocationConstraints() \
    {                                           \
        DefineAsRegister(this);                 \
        UseRegister(Arg(LEFT_INDEX));           \
        UseRegister(Arg(RIGHT_INDEX));          \
    }                                           \
                                                \
    void F64##Name##Vertex::Dump(std::ostream &output) const \
    {                                           \
        output << "  F64" #Name;                \
    }

DEFINE_F64_BINOP_CONSTRAINTS(Add)
DEFINE_F64_BINOP_CONSTRAINTS(Sub)
DEFINE_F64_BINOP_CONSTRAINTS(Mul)
DEFINE_F64_BINOP_CONSTRAINTS(Div)
#undef DEFINE_F64_BINOP_CONSTRAINTS

// ========================================= Control Opcode =========================================

void JumpVertex::SetValueLocationConstraints() {}

void JumpLoopVertex::SetValueLocationConstraints() {}

void BranchIfTrueVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(0));
}

void BranchIfTrueVertex::Dump(std::ostream &output) const
{
    output << "  BranchIfTrue";
}

void ReturnVertex::SetValueLocationConstraints()
{
    UseFixed(Arg(0), 0);
}

void ReturnVertex::Dump(std::ostream &output) const
{
    output << "  Return";
}

void ThrowVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(EXCEPTION_INDEX));
    // to do: consider use any for dummy input
}

void ThrowVertex::Dump(std::ostream &output) const
{
    output << "  Throw (id=" << static_cast<int>(id_) << ")";
}

// ========================================= Non-Value Opcode =========================================

void ThrowIfSuperNotCorrectCallVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(INDEX_INDEX));
    UseRegister(Arg(THIS_VALUE_INDEX));
}

void ThrowIfSuperNotCorrectCallVertex::Dump(std::ostream &output) const
{
    output << "  ThrowIfSuperNotCorrectCall (id=" << static_cast<int>(id_) << ")";
}

void ThrowIfNotObjectVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(VALUE_INDEX));
}

void ThrowIfNotObjectVertex::Dump(std::ostream &output) const
{
    output << "  ThrowIfNotObject (id=" << static_cast<int>(runtimeId_) << ")";
}

void ThrowUndefinedIfHoleVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(HOLE_INDEX));
    UseRegister(Arg(OBJ_INDEX));
}

void ThrowUndefinedIfHoleVertex::Dump(std::ostream &output) const
{
    output << "  ThrowUndefinedIfHole (id=" << static_cast<int>(runtimeId_) << ")";
}

void ThrowUndefinedIfHoleWithNameVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(STRING_ID_INDEX));
    UseRegister(Arg(HOLE_INDEX));
}

void ThrowUndefinedIfHoleWithNameVertex::Dump(std::ostream &output) const
{
    output << "  ThrowUndefinedIfHoleWithName (id=" << static_cast<int>(runtimeId_) << ")";
}

void GapMoveVertex::SetValueLocationConstraints()
{
    UNREACHABLE();
}

void GapMoveVertex::Dump(std::ostream &output) const
{
    output << "  GapMove -> " << GetTarget().GetRegisterCode();
}

void ConstantGapMoveVertex::SetValueLocationConstraints()
{
    UNREACHABLE();
}

void ConstantGapMoveVertex::Dump(std::ostream &output) const
{
    output << "  ConstantGapMove -> " << GetTarget().GetRegisterCode();
}

void PhiVertex::SetValueLocationConstraints()
{
    DefineSameAsFirst(this);
}

void ToTaggedIntVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
    SetTemporariesNeeded(1);
}

void ToTaggedIntVertex::Dump(std::ostream &output) const
{
    output << "  ToTaggedInt";
}

}  // namespace panda::ecmascript::arksteed
