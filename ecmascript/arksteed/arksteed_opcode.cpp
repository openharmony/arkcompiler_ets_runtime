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

#include "ecmascript/arksteed/arksteed_assembler-inl.h"  // IWYU pragma: keep
#include "ecmascript/arksteed/arksteed_bb.h"
#include "ecmascript/arksteed/arksteed_condition_code.h"
#include "ecmascript/arksteed/arksteed_dump_helper.h"
#include "ecmascript/arksteed/arksteed_graph_processor.h"
#include "ecmascript/arksteed/arksteed_opcode_list.h"
#include "ecmascript/arksteed/arksteed_vertex.h"
#include "ecmascript/compiler/common_stub_csigns.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include <ios>
#include <sstream>
#include <type_traits>

namespace panda::ecmascript::arksteed {

namespace {
void UseDeoptFrameSlot(InputLocation *location)
{
    location->GetOperand() = UnallocatedState(UnallocatedState::ExtendedPolicy::MUST_HAVE_SLOT,
        UnallocatedState::LifetimeFlag::USED_AT_END, NO_VREG);
}

void UseEagerDeoptFrameSlots(EagerDeoptimizableMixin *deopt)
{
    for (uint32_t index = 0; index < deopt->GetDeoptFrameValueCount(); ++index) {
        UseDeoptFrameSlot(deopt->GetDeoptSourceLocation(index));
    }
}

void UseLazyDeoptFrameSlots(LazyDeoptimizableMixin *deopt)
{
    if (!deopt->HasLazyDeoptFrameState()) {
        return;
    }
    for (uint32_t index = 0, valueCount = deopt->GetDeoptFrameValueCount(); index < valueCount; ++index) {
        UseDeoptFrameSlot(deopt->GetDeoptSourceLocation(index));
    }
}

}  // namespace

#define __ masm->

// Helper function for stub calls (CallRuntime, CallCommonStub)
// Uses C calling convention (CCallConv) in arksteed
void SetStubValueLocationConstraints(Vertex *vertex, size_t inputCount)
{
    for (size_t paramIdx = 0; paramIdx < inputCount; paramIdx++) {
        if (paramIdx < static_cast<size_t>(ArkSteedAssembler::NUM_ARG_REGISTERS)) {
            ArkSteedRegister paramReg = ArkSteedAssembler::GetParameterRegister(static_cast<int>(paramIdx));
            UseAndClobberFixed(vertex->Arg(paramIdx), paramReg);
        } else {
            UseAny(vertex->Arg(paramIdx));
        }
    }
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

void Int64ConstantVertex::DoLoadToRegister(ArkSteedAssembler *masm, ArkSteedRegister reg) const
{
    __ Move(reg, static_cast<int64_t>(GetValue()));
}

void Int64ConstantVertex::SetValueLocationConstraints()
{
    DefineAsConstant(this);
}

void Float64ConstantVertex::DoLoadToRegister(ArkSteedAssembler *masm, ArkSteedDoubleRegister reg) const
{
    __ Move(reg, GetValue());
}

void Float64ConstantVertex::SetValueLocationConstraints()
{
    DefineAsConstant(this);
}

void TaggedConstantVertex::SetValueLocationConstraints()
{
    DefineAsConstant(this);
}

void TaggedConstantVertex::DoLoadToRegister(ArkSteedAssembler *masm, ArkSteedRegister reg) const
{
    __ Move(reg, GetValue());
}

void HeapConstantVertex::SetValueLocationConstraints()
{
    DefineAsConstant(this);
}

void HeapConstantVertex::DoLoadToRegister(ArkSteedAssembler *masm, ArkSteedRegister reg) const
{
    masm->MoveEmbeddedTagged(reg, GetHandleIndex());
}

void InitialValueVertex::SetValueLocationConstraints()
{
    // Convert fp-slot index (word units from fp) to tagged-slot index.
    // tagged_slot[0] is at fp-slot -4 (i.e. FP + 4n where n = pointer size).
    constexpr int32_t kTaggedSlot0FpSlotIndex = -4;
    int32_t taggedSlotIndex = kTaggedSlot0FpSlotIndex - frameSlotIndex_;
    Result().SetUnallocated(UnallocatedState::BasicPolicy::FIXED_SLOT, taggedSlotIndex);
}

void ActualArgcVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
}

void CallRuntimeVertex::SetValueLocationConstraints()
{
    // Define return value in x0/rax (C calling convention)
    DefineAsFixed(this, 0);

    // Set parameter location constraints using helper function
    SetStubValueLocationConstraints(this, GetArgCount());
    UseLazyDeoptFrameSlots(static_cast<LazyDeoptimizableMixin *>(this));
}

void CallVertex::SetValueLocationConstraints()
{
    DefineAsFixed(this, 0);
#if defined(PANDA_TARGET_AMD64)
    // 1 for the call scratch held across the type guards, 1 for the guard's HClass/value.
    SetTemporariesNeeded(2);
#elif defined(PANDA_TARGET_ARM64)
    // 1 for the call scratch, 2 for the type guard's HClass + objectType/bitfield.
    SetTemporariesNeeded(3);
#else
    SetTemporariesNeeded(1);
#endif
    UseRegister(Arg(TARGET_INDEX));
    for (uint32_t i = NEW_TARGET_INDEX; i < GetInputCount(); i++) {
        UseAny(Arg(i));
    }
    UseLazyDeoptFrameSlots(static_cast<LazyDeoptimizableMixin *>(this));
}

void CallCommonStubVertex::SetValueLocationConstraints()
{
    // Define return value in x0/rax (C calling convention)
    DefineAsFixed(this, 0);

    // Set parameter location constraints using helper function
    SetStubValueLocationConstraints(this, GetArgCount());
    UseLazyDeoptFrameSlots(static_cast<LazyDeoptimizableMixin *>(this));
}

void DeoptIfHClassMismatchVertex::SetValueLocationConstraints()
{
    SetTemporariesNeeded(2);  // 2: actual hclass and expected hclass
    UseRegister(Arg(RECEIVER_INDEX));
}

void DeoptIfHClassNotInVertex::SetValueLocationConstraints()
{
    SetTemporariesNeeded(2);  // 2: actual hclass and expected hclass
    UseRegister(Arg(RECEIVER_INDEX));
}

void DeoptIfPrototypeChangedVertex::SetValueLocationConstraints()
{
    SetTemporariesNeeded(2);  // 2: traversed heap object and mask/check value
    UseRegister(Arg(RECEIVER_INDEX));
    UseEagerDeoptFrameSlots(this);
}

void DeoptIfTaggedConditionVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
    UseEagerDeoptFrameSlots(this);
}

void DeoptIfInt32ConditionVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void DeoptIfNotNumberVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(VALUE_INDEX));
    SetTemporariesNeeded(1);
}

void DeoptVertex::SetValueLocationConstraints()
{
    for (uint32_t i = 0, n = GetInputCount(); i < n; i++) {
        UseSlot(Arg(i));
    }
    UseEagerDeoptFrameSlots(this);
}

// ========================================= Slow Value Opcode =========================================

void LoadTaggedFromAddressVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
}

void LoadI32FromAddressVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
}

void LoadI64FromAddressVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
}

void LoadF64FromAddressVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
}

void LoadExceptionVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(GLUE_INDEX));
}

void LoadTaggedFieldVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
}

void LoadInt32FieldVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
}

void LoadTaggedElementVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(ELEMENTS_INDEX));
    UseRegister(Arg(INDEX_INDEX));
}

void LoadPrototypeFromObjectVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
}

void LoadPrototypeHolderByHClassVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(RECEIVER_INDEX));
    SetTemporariesNeeded(2);  // 2: current hclass and expected hclass
}

void ConvertHoleToUndefinedVertex::SetValueLocationConstraints()
{
    DefineSameAsFirst(this);
    UseRegister(Arg(VALUE_INDEX));
}

void LoadHClassAddressVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
    SetTemporariesNeeded(1);
}

void FindPrototypeHolderVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    SetTemporariesNeeded(2);  // 2: current HClass and mask/expected HClass
    UseRegister(Arg(RECEIVER_INDEX));
    UseEagerDeoptFrameSlots(this);
}

void StoreTaggedToAddressVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(OBJECT_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void StoreI32ToAddressVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(OBJECT_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void StoreI64ToAddressVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(OBJECT_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void StoreF64ToAddressVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(OBJECT_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void StoreTaggedFieldVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(OBJECT_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void StoreTaggedFieldWithBarrierVertex::SetValueLocationConstraints()
{
    SetTemporariesNeeded(2);  // 2: object and value region scratch registers
    UseRegister(Arg(GLUE_INDEX));
    UseRegister(Arg(OBJECT_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void StoreSharedFieldWithBarrierVertex::SetValueLocationConstraints()
{
    SetTemporariesNeeded(2);  // 2: object and value region scratch registers
    UseRegister(Arg(GLUE_INDEX));
    UseRegister(Arg(OBJECT_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void TransitionHClassWithBarrierVertex::SetValueLocationConstraints()
{
    SetTemporariesNeeded(2);  // 2: object and HClass region scratch registers
    UseRegister(Arg(GLUE_INDEX));
    UseRegister(Arg(OBJECT_INDEX));
    UseRegister(Arg(HCLASS_INDEX));
}

void PrepareSharedStoreFieldVertex::SetValueLocationConstraints()
{
    DefineAsFixed(this, 0);
    SetTemporariesNeeded(1);
    SetStubValueLocationConstraints(this, GetArgCount());
    UseLazyDeoptFrameSlots(this);
}

void EnsurePropertiesCapacityVertex::SetValueLocationConstraints()
{
    DefineAsFixed(this, 0);
    SetTemporariesNeeded(2);  // 2: properties and length
    SetStubValueLocationConstraints(this, GetArgCount());
    UseLazyDeoptFrameSlots(this);
}

void StoreInt32FieldVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(STORE_TARGET_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void StoreDoubleFieldVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(STORE_TARGET_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void StoreInt32FieldWithRepVertex::SetValueLocationConstraints()
{
    SetTemporariesNeeded(2);  // 2: tag and expected tag
    UseRegister(Arg(STORE_TARGET_INDEX));
    UseRegister(Arg(VALUE_INDEX));
    UseEagerDeoptFrameSlots(this);
}

void StoreDoubleFieldWithRepVertex::SetValueLocationConstraints()
{
    SetTemporariesNeeded(2);  // 2: tag and expected tag
    UseRegister(Arg(STORE_TARGET_INDEX));
    UseRegister(Arg(VALUE_INDEX));
    UseEagerDeoptFrameSlots(this);
}

void StoreTaggedFieldByHClassVertex::SetValueLocationConstraints()
{
    SetTemporariesNeeded(3);  // 3: HClass dispatch and write-barrier scratch registers
    UseRegister(Arg(GLUE_INDEX));
    UseRegister(Arg(OBJECT_INDEX));
    UseRegister(Arg(VALUE_INDEX));
    UseEagerDeoptFrameSlots(this);
}

void StoreEnvSlotVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(ENV_INDEX));
    UseRegister(Arg(VALUE_INDEX));
}

void SetValueWithBarrierVertex::SetValueLocationConstraints()
{
    UseFixed(Arg(GLUE_INDEX), static_cast<uint32_t>(ArkSteedAssembler::GetParameterRegister(0).Code()));
    UseFixed(Arg(OBJECT_INDEX), static_cast<uint32_t>(ArkSteedAssembler::GetParameterRegister(1).Code()));
    UseFixed(Arg(VALUE_INDEX), static_cast<uint32_t>(ArkSteedAssembler::GetParameterRegister(3).Code()));
}

void TaggedIntToI32Vertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
}

void CheckedTaggedIntToI32Vertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
    SetTemporariesNeeded(1);
}

void CheckedTaggedStringVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(INPUT_INDEX));
    DefineSameAsFirst(this);
    SetTemporariesNeeded(2);
}

void I32ConditionCheckVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void F64ConditionCheckVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void TaggedEqualVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void TaggedNotEqualVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void StringEqualVertex::SetValueLocationConstraints()
{
    DefineAsFixed(this, 0);
    SetStubValueLocationConstraints(this, GetInputCount());
}

template <class VertexT>
void VerifyI32BinaryDeoptInputs(const VertexT *vertex)
{
    ASSERT(vertex->GetInputCount() == VertexT::NUM_INPUTS);
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

#if defined(PANDA_TARGET_AMD64)
#define DEFINE_I32_WITH_OVERFLOW_RESULT_CONSTRAINT() DefineSameAsFirst(this)
#else
#define DEFINE_I32_WITH_OVERFLOW_RESULT_CONSTRAINT() DefineAsRegister(this)
#endif

#define DEFINE_I32_WITH_OVERFLOW_CONSTRAINTS(Name)                     \
    void I32##Name##WithOverflowVertex::SetValueLocationConstraints()  \
    {                                                                  \
        UseRegister(Arg(LEFT_INDEX));                                  \
        UseRegister(Arg(RIGHT_INDEX));                                 \
        DEFINE_I32_WITH_OVERFLOW_RESULT_CONSTRAINT();                   \
    }

DEFINE_I32_WITH_OVERFLOW_CONSTRAINTS(Add)
DEFINE_I32_WITH_OVERFLOW_CONSTRAINTS(Sub)
#undef DEFINE_I32_WITH_OVERFLOW_CONSTRAINTS
#undef DEFINE_I32_WITH_OVERFLOW_RESULT_CONSTRAINT

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
#if defined(PANDA_TARGET_AMD64)
    DefineSameAsFirst(this);
#else
    DefineAsRegister(this);
#endif
    SetTemporariesNeeded(1);
}

void I32DivWithOverflowVertex::SetValueLocationConstraints()
{
#if defined(PANDA_TARGET_AMD64)
    DefineAsFixed(this, x64::rax);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
    RequireSpecificTemporary(this, x64::rax);
    RequireSpecificTemporary(this, x64::rdx);
#else
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
#endif
}

void I32DivByConstWithCheckVertex::SetValueLocationConstraints()
{
#if defined(PANDA_TARGET_AMD64)
    DefineAsFixed(this, x64::rax);
    UseRegister(Arg(INPUT_INDEX));
    RequireSpecificTemporary(this, x64::rax);
    RequireSpecificTemporary(this, x64::rdx);
    RequireSpecificTemporary(this, x64::rcx);
    RequireSpecificTemporary(this, x64::r8);
#else
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
    SetTemporariesNeeded(2);
#endif
}

void I32AddVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
    DefineSameAsFirst(this);
}

void I32SubVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
    DefineSameAsFirst(this);
}

void I32MulVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
    DefineSameAsFirst(this);
}

void I32DivVertex::SetValueLocationConstraints()
{
#if defined(PANDA_TARGET_AMD64)
    DefineAsFixed(this, x64::rax);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
    RequireSpecificTemporary(this, x64::rax);
    RequireSpecificTemporary(this, x64::rdx);
#else
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
#endif
}

void CheckedI32ModVertex::SetValueLocationConstraints()
{
#if defined(PANDA_TARGET_AMD64)
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
    RequireSpecificTemporary(this, x64::rax);
    RequireSpecificTemporary(this, x64::rdx);
#else
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
#endif
}

void I32BitwiseBinaryVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    if (RightInputIsConstant()) {
        UseAny(Arg(RIGHT_INDEX));
    } else if (IsShift()) {
#if defined(PANDA_TARGET_AMD64)
        UseFixed(Arg(RIGHT_INDEX), x64::rcx);
#else
        UseRegister(Arg(RIGHT_INDEX));
#endif
    } else {
        UseRegister(Arg(RIGHT_INDEX));
    }
    DefineSameAsFirst(this);
}

void CheckedNonNegativeI32ToTaggedIntVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
}

void I32BNotVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(VALUE_INDEX));
    DefineSameAsFirst(this);
}

template <class VertexT>
void VerifyI32UnaryDeoptInputs(const VertexT *vertex)
{
    ASSERT(vertex->GetInputCount() == VertexT::NUM_INPUTS);
    ASSERT(vertex->GetInput(VertexT::VALUE_INDEX)->GetValueRepresentation() == ValueRepresentation::INT32);
}

void I32NegWithOverflowVertex::VerifyI32UnaryOpInputs() const
{
    VerifyI32UnaryDeoptInputs(this);
}

void I32IncWithOverflowVertex::VerifyI32UnaryOpInputs() const
{
    VerifyI32UnaryDeoptInputs(this);
}

void I32DecWithOverflowVertex::VerifyI32UnaryOpInputs() const
{
    VerifyI32UnaryDeoptInputs(this);
}

#if defined(PANDA_TARGET_AMD64)
#define DEFINE_I32_UNARY_WITH_OVERFLOW_RESULT_CONSTRAINT() DefineSameAsFirst(this)
#else
#define DEFINE_I32_UNARY_WITH_OVERFLOW_RESULT_CONSTRAINT() DefineAsRegister(this)
#endif

#define DEFINE_I32_UNARY_WITH_OVERFLOW_CONSTRAINTS(Name)     \
    void I32##Name##WithOverflowVertex::SetValueLocationConstraints() \
    {                                                         \
        UseRegister(Arg(VALUE_INDEX));                       \
        DEFINE_I32_UNARY_WITH_OVERFLOW_RESULT_CONSTRAINT();  \
    }

DEFINE_I32_UNARY_WITH_OVERFLOW_CONSTRAINTS(Neg)
DEFINE_I32_UNARY_WITH_OVERFLOW_CONSTRAINTS(Inc)
DEFINE_I32_UNARY_WITH_OVERFLOW_CONSTRAINTS(Dec)
#undef DEFINE_I32_UNARY_WITH_OVERFLOW_CONSTRAINTS
#undef DEFINE_I32_UNARY_WITH_OVERFLOW_RESULT_CONSTRAINT

void I32ToF64Vertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
}

void CheckedNumberToF64Vertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
    SetTemporariesNeeded(2);
}

void F64ToI32TruncVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
}

void F64ToTaggedDoubleVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
}

void F64NegVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(VALUE_INDEX));
#if defined(PANDA_TARGET_AMD64)
    DefineSameAsFirst(this);
#else
    DefineAsRegister(this);
#endif
}

#define DEFINE_F64_BINOP_CONSTRAINTS(Name)       \
    void F64##Name##Vertex::SetValueLocationConstraints() \
    {                                           \
        UseRegister(Arg(LEFT_INDEX));           \
        UseRegister(Arg(RIGHT_INDEX));          \
        DefineSameAsFirst(this);                \
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

void BranchIfTaggedStringVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(VALUE_INDEX));
    SetTemporariesNeeded(2);
}

void BranchIfHClassInVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(RECEIVER_INDEX));
    // 3: actual hclass, expected hclass, and the tagged-heap-object guard scratch.
    SetTemporariesNeeded(3);
}

void BranchIfInt32CompareVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void BranchIfInt64CompareVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void BranchIfFloat64CompareVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void BranchIfReferenceEqualVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

void BranchIfObjectTypeVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(VALUE_INDEX));
    SetTemporariesNeeded(1);
}

void ReturnVertex::SetValueLocationConstraints()
{
    UseFixed(Arg(0), 0);
}

void ThrowVertex::SetValueLocationConstraints()
{
    SetStubValueLocationConstraints(this, GetArgCount());
}

// ========================================= Non-Value Opcode =========================================

void BranchIfTaggedHeapObjectVertex::SetValueLocationConstraints()
{
    UseRegister(Arg(VALUE_INDEX));
    SetTemporariesNeeded(1);
}

void GapMoveVertex::SetValueLocationConstraints()
{
    UNREACHABLE();
}

void ConstantGapMoveVertex::SetValueLocationConstraints()
{
    UNREACHABLE();
}

void PhiVertex::SetValueLocationConstraints()
{
    DefineSameAsFirst(this);
}

void I32ToTaggedIntVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
}

void RawI64ToTaggedVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
}

void TaggedToRawI64Vertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(INPUT_INDEX));
}

void I64BitwiseBinaryVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(LEFT_INDEX));
    UseRegister(Arg(RIGHT_INDEX));
}

namespace {
#define HAS_COMMON_MIXIN(Field, _) +std::is_base_of_v<Field##Mixin, VertexT>

template <class VertexT>
struct DumpCommonHelper {
    static constexpr int COMMON_COUNT = COMMON_MIXINS_LIST(HAS_COMMON_MIXIN)
        + std::is_base_of_v<EagerDeoptimizableMixin, VertexT>
        + std::is_base_of_v<LazyDeoptimizableMixin, VertexT>;

    static constexpr bool HAS_COMMON = static_cast<bool>(COMMON_COUNT);

    static void Dump(std::ostream &out, const VertexT *vertex)
    {
        if constexpr (std::is_base_of_v<CommonStubIDMixin, VertexT>) {
            out << "  " << kungfu::CommonStubCSigns::GetName(vertex->GetCommonStubID());
        }
        if constexpr (std::is_base_of_v<RuntimeStubIDMixin, VertexT>) {
            out << "  " << kungfu::RuntimeStubCSigns::GetRTName(vertex->GetRuntimeStubID());
        }
        if constexpr (std::is_base_of_v<ConditionMixin, VertexT>) {
            out << "  cc = " << ConditionName(vertex->GetCondition());
        }
        if constexpr (std::is_base_of_v<OffsetMixin, VertexT>) {
            out << "  offset = " << vertex->GetOffset();
        }
        if constexpr (std::is_base_of_v<EagerDeoptimizableMixin, VertexT>) {
            out << "  bpc = " << vertex->GetBytecodeOffset();
        }
        if constexpr (std::is_base_of_v<LazyDeoptimizableMixin, VertexT>) {
            if (vertex->HasLazyDeoptFrameState()) {
                out << "  bpc = " << vertex->GetBytecodeOffset();
            }
        }
    }
};

#undef HAS_COMMON_MIXIN

template <class VertexT>
struct DumpExtraHelper {
    static constexpr bool HAS_EXTRA = false;
};

#define DUMP_EXTRA(Type)                                                                        \
    template <>                                                                                 \
    struct DumpExtraHelper<Type##Vertex> {                                                      \
        static constexpr bool HAS_EXTRA = true;                                                 \
        static void Dump(std::ostream &out, const Type##Vertex *vertex);                        \
    };                                                                                          \
    void DumpExtraHelper<Type##Vertex>::Dump(std::ostream &out, const Type##Vertex *vertex)

DUMP_EXTRA(Int32Constant)
{
    int32_t value = vertex->GetValue();
    uint32_t uValue = static_cast<uint32_t>(value);
    out << "  value = " << value << " (0x" << std::hex << uValue << ')' << std::dec;
}

DUMP_EXTRA(Int64Constant)
{
    int64_t value = vertex->GetValue();
    uint64_t uValue = static_cast<uint64_t>(value);
    out << "  value = " << value << " (0x" << std::hex << uValue << ')' << std::dec;
}

DUMP_EXTRA(Float64Constant)
{
    double value = vertex->GetValue();
    out << "  value = " << value << " (0x" << std::hex << value << ')' << std::dec;
}

DUMP_EXTRA(TaggedConstant)
{
    JSTaggedType rawValue = vertex->GetValue();
    JSTaggedValue value(rawValue);
    out << "  value = 0x" << std::hex << rawValue << std::dec << " (";

    if (value.IsInt()) {
        out << "tagged int: " << value.GetInt() << ')';
        return;
    }
    if (value.IsDouble()) {
        out << "tagged double: " << value.GetDouble() << ')';
        return;
    }
    switch (rawValue) {
        case JSTaggedValue::VALUE_UNDEFINED:
            out << "tagged undefined)";
            return;
        case JSTaggedValue::VALUE_NULL:
            out << "tagged null)";
            return;
        case JSTaggedValue::VALUE_TRUE:
            out << "tagged true)";
            return;
        case JSTaggedValue::VALUE_FALSE:
            out << "tagged false)";
            return;
        case JSTaggedValue::VALUE_HOLE:
            out << "tagged hole)";
            return;
        case JSTaggedValue::VALUE_EXCEPTION:
            out << "tagged exception)";
            return;
        default:
            break;
    }
    if (value.IsObject()) {
        out << "tagged heap object)";
        return;
    }
    out << "unknown)";
}

DUMP_EXTRA(InitialValue)
{
    out << "  frameSlot = " << vertex->GetFrameSlotIndex();
}

DUMP_EXTRA(Call)
{
    out << "  actualArgc = " << vertex->GetActualArgc();
}

DUMP_EXTRA(DeoptIfHClassMismatch)
{
    uintptr_t expected = reinterpret_cast<uintptr_t>(vertex->GetExpectedHClass());
    out << "  expected = 0x" << std::hex << expected << std::dec;
}

DUMP_EXTRA(DeoptIfHClassNotIn)
{
    out << "  expected = [";
    const auto &expectedHClasses = vertex->GetExpectedHClasses();
    out << std::hex;
    for (size_t i = 0; i < expectedHClasses.size(); ++i) {
        if (i != 0) out << ", ";
        out << "0x" << reinterpret_cast<uintptr_t>(expectedHClasses[i]);
    }
    out << std::dec;
    out << ']';
}

DUMP_EXTRA(DeoptIfPrototypeChanged)
{
    out << std::boolalpha;
    out << "  checkProtoChangeMarker = " << vertex->ShouldCheckProtoChangeMarker()
        << ", checkNotPrototype = " << vertex->ShouldCheckNotPrototype();
    out << std::noboolalpha;
}

DUMP_EXTRA(DeoptIfInt32Condition)
{
    out << "  type = " << static_cast<int>(vertex->GetDeoptType());
}

DUMP_EXTRA(Deopt)
{
    out << "  type = " << static_cast<int>(vertex->GetDeoptType());
}

DUMP_EXTRA(LoadPrototypeHolderByHClass)
{
    uintptr_t holderHClass = reinterpret_cast<uintptr_t>(vertex->GetHolderHClass());
    out << "  hclass = 0x" << std::hex << holderHClass << std::dec
        << "  depth = " << vertex->GetHolderDepth()
        << "  prototypes = [";
    const auto &expectedHClasses = vertex->GetExpectedPrototypeHClasses();
    out << std::hex;
    for (size_t i = 0; i < expectedHClasses.size(); ++i) {
        if (i != 0) out << ", ";
        out << "0x" << reinterpret_cast<uintptr_t>(expectedHClasses[i]);
    }
    out << std::dec;
    out << "]";
}

DUMP_EXTRA(FindPrototypeHolder)
{
    uintptr_t holderHClass = reinterpret_cast<uintptr_t>(vertex->GetExpectedHolderHClass());
    out << "  expected = 0x" << std::hex << holderHClass << std::dec;
}

DUMP_EXTRA(PrepareSharedStoreField)
{
    out << "  handlerInfo = 0x" << std::hex << vertex->GetHandlerInfo() << std::dec;
}

DUMP_EXTRA(EnsurePropertiesCapacity)
{
    out << "  fieldIndex = " << vertex->GetFieldIndex();
}

DUMP_EXTRA(StoreTaggedFieldByHClass)
{
    out << "  numCases = " << vertex->GetCases().size()
        << "  value_kind = " << WriteBarrierValueKindName(vertex->GetValueKind());
}

DUMP_EXTRA(StoreTaggedFieldWithBarrier)
{
    out << "  value_kind = " << WriteBarrierValueKindName(vertex->GetValueKind());
}

DUMP_EXTRA(StoreSharedFieldWithBarrier)
{
    out << "  value_kind = " << WriteBarrierValueKindName(vertex->GetValueKind());
}

DUMP_EXTRA(I32DivByConstWithCheck)
{
    out << "  divisor = " << vertex->GetDivisor()
        << "  magic = " << vertex->GetMagic()
        << "  shift = " << vertex->GetShift();
}

DUMP_EXTRA(I32BitwiseBinary)
{
    out << "  kind = " << IntBitwiseKindName(vertex->GetKind());
}

DUMP_EXTRA(BranchIfHClassIn)
{
    out << "  expected = [";
    const auto &expectedHClasses = vertex->GetExpectedHClasses();
    for (size_t i = 0; i < expectedHClasses.size(); ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << "0x" << std::hex << reinterpret_cast<uintptr_t>(expectedHClasses[i]) << std::dec;
    }
    out << "]";
}

DUMP_EXTRA(BranchIfObjectType)
{
    CString name = JSHClass::DumpJSType(vertex->GetExpectedType());
    out << "  expected = " << name;
}

DUMP_EXTRA(GapMove)
{
    out << "-> " << vertex->GetTarget().GetRegisterCode();
}

DUMP_EXTRA(ConstantGapMove)
{
    out << "-> " << vertex->GetTarget().GetRegisterCode();
}

#undef DUMP_EXTRA
}  // namespace

void Vertex::Dump(std::ostream &out, bool withColors) const
{
    out << FormatVertexLabel(this) << ":  ";
    WITH_ANSI_COLOR_SCOPE(BrightRed(out, withColors)) {
        out << OpcodeToString(GetOpcode());
    }
    ValueRepresentation repr = GetValueRepresentation();
    if (repr != ValueRepresentation::NONE) {
        out << " [";
        WITH_ANSI_COLOR_SCOPE(BrightRed(out, withColors)) {
            out << ValueRepresentationName(repr);
        }
        out << ']';
    }
    uint32_t n = GetInputCount();
    if (n > 0) {
        out << "  (";
        for (uint32_t i = 0; i < n; i++) {
            if (i != 0) out << ", ";
            out << FormatVertexLabel(GetInput(i));
        }
        out << ")";
    }

    auto doDumpFields = [&out](const auto *self) {
        using VertexT = std::remove_const_t<std::remove_pointer_t<decltype(self)>>;
        if constexpr (DumpCommonHelper<VertexT>::HAS_COMMON) {
            DumpCommonHelper<VertexT>::Dump(out, self);
        }
        if constexpr (DumpExtraHelper<VertexT>::HAS_EXTRA) {
            DumpExtraHelper<VertexT>::Dump(out, self);
        }
    };
    WITH_ANSI_COLOR_SCOPE(BrightGreen(out, withColors)) {
        switch (GetOpcode()) {
#define CASE(Type)                                  \
            case VertexOpcode::Type:                \
                doDumpFields(Cast<Type##Vertex>()); \
                break;
            ALL_VERTEX_LIST(CASE)
#undef CASE
            default:
                break;  // No-op otherwise
        }
    }
}

std::string Vertex::Dump(bool withColors) const
{
    std::ostringstream out;
    Dump(out, withColors);
    return out.str();
}

}  // namespace panda::ecmascript::arksteed
