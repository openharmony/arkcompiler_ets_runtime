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

void ConstantVertex::DoLoadToRegister(ArkSteedAssembler *masm, ArkSteedRegister reg) const
{
    __ Move(reg, GetValue().GetRawData());
}

void ConstantVertex::SetValueLocationConstraints()
{
    DefineAsConstant(this);
}

void ConstantVertex::Dump(std::ostream &output) const
{
    output << "  Constant: 0x" << std::hex << GetValue().GetRawData() << std::dec;
}

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

void RootConstantVertex::DoLoadToRegister(ArkSteedAssembler *masm, ArkSteedRegister reg) const
{
    JSTaggedType value = 0;
    switch (index_) {
        case RootIndex::UNDEFINED:
            value = JSTaggedValue::Undefined().GetRawData();
            break;
        case RootIndex::NULL_VALUE:
            value = JSTaggedValue::Null().GetRawData();
            break;
        case RootIndex::TRUE_VALUE:
            value = JSTaggedValue::True().GetRawData();
            break;
        case RootIndex::FALSE_VALUE:
            value = JSTaggedValue::False().GetRawData();
            break;
        default:
            UNREACHABLE();
            break;
    }
    __ Move(reg, value);
}

void RootConstantVertex::SetValueLocationConstraints()
{
    DefineAsConstant(this);
}

void RootConstantVertex::Dump(std::ostream &output) const
{
    const char *name = "Unknown";
    switch (index_) {
        case RootIndex::UNDEFINED:
            name = "Undefined";
            break;
        case RootIndex::NULL_VALUE:
            name = "Null";
            break;
        case RootIndex::TRUE_VALUE:
            name = "True";
            break;
        case RootIndex::FALSE_VALUE:
            name = "False";
            break;
    }
    output << "  RootConstant: " << name;
}

void BooleanConstantVertex::SetValueLocationConstraints()
{
    DefineAsConstant(this);
}

void BooleanConstantVertex::DoLoadToRegister(ArkSteedAssembler *masm, ArkSteedRegister reg) const
{
    __ Move(reg, GetValue());
}

void BooleanConstantVertex::Dump(std::ostream &output) const
{
    output << "  BooleanConstant: " << (GetValue() ? "true" : "false");
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

void DeoptVertex::SetValueLocationConstraints()
{
    for (int i = 0; i < GetInputCount(); i++) {
        UseAny(Arg(i));
    }
}

void DeoptVertex::Dump(std::ostream &output) const
{
    output << "  Deopt";
}

// ========================================= Slow Value Opcode =========================================

void LoadTaggedFieldVertex::SetValueLocationConstraints()
{
    DefineAsRegister(this);
    UseRegister(Arg(OBJECT_INDEX));
}

void LoadTaggedFieldVertex::Dump(std::ostream &output) const
{
    output << "  LoadTaggedField: offset=" << offset_;
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
}

void ToTaggedIntVertex::Dump(std::ostream &output) const
{
    output << "  ToTaggedInt";
}

}  // namespace panda::ecmascript::arksteed
