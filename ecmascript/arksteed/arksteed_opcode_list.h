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

#ifndef ECMASCRIPT_ARKSTEED_OPCODE_LIST_H
#define ECMASCRIPT_ARKSTEED_OPCODE_LIST_H

#include <algorithm>

#include "libpandabase/macros.h"

namespace panda::ecmascript::arksteed {

using VertexId = uint32_t;
static constexpr VertexId INVALID_VERTEX_ID = static_cast<VertexId>(-1);

//==============================================================================
// Opcode List Macros
//==============================================================================

// List of constant value vertices (for constant folding, etc.)
#define CONSTANT_VALUE_VERTEX_LIST(V) \
    V(Int32Constant)                  \
    V(IntPtrConstant)                 \
    V(Float64Constant)                \
    V(TaggedConstant)

// List of value vertex types (must match the enum in arksteed_vertex.h)
#define COMMON_VALUE_VERTEX_LIST(V) \
    CONSTANT_VALUE_VERTEX_LIST(V)   \
    V(InitialValue)                 \
    V(ActualArgc)                   \
    V(Call)                         \
    V(CallRuntime)                  \
    V(CallCommonStub)               \
    V(Phi)

#define VALUE_VERTEX_LIST(V)    \
    V(LoadFromAddress)          \
    V(StoreToAddress)           \
    V(LoadTaggedField)          \
    V(StoreTaggedField)         \
    V(StoreEnvSlot)             \
    V(LoadException)            \
    V(TaggedIntToI32)           \
    V(CheckedTaggedIntToI32)    \
    V(CheckedTaggedString)      \
    V(I32ConditionCheck)        \
    V(F64ConditionCheck)        \
    V(TaggedEqual)              \
    V(TaggedNotEqual)           \
    V(StringEqual)              \
    V(I32AddWithOverflow)       \
    V(I32SubWithOverflow)       \
    V(I32MulWithOverflow)       \
    V(I32DivWithOverflow)       \
    V(I32DivByConstWithCheck)   \
    V(I32Add)                   \
    V(I32Sub)                   \
    V(I32Mul)                   \
    V(I32Div)                   \
    V(PositiveI32Mod)           \
    V(CheckedPositiveI32Mod)    \
    V(I32BitwiseBinary)         \
    V(CheckedNonNegativeI32ToTaggedInt) \
    V(I32BNot)                  \
    V(I32NegWithOverflow)       \
    V(I32IncWithOverflow)       \
    V(I32DecWithOverflow)       \
    V(I32ToF64)                 \
    V(CheckedNumberToF64)       \
    V(F64ToI32Trunc)            \
    V(F64ToTaggedDouble)        \
    V(F64Neg)                   \
    V(F64Add)                   \
    V(F64Sub)                   \
    V(F64Mul)                   \
    V(F64Div)                   \
    COMMON_VALUE_VERTEX_LIST(V) \
    CONVERSION_VERTEX_LIST(V)

#define NON_CONTROL_VERTEX_LIST(V) \
    NON_VALUE_VERTEX_LIST(V)       \
    VALUE_VERTEX_LIST(V)

#define NON_VALUE_VERTEX_LIST(V)    \
    V(ThrowIfSuperNotCorrectCall)   \
    V(ThrowIfNotObject)             \
    V(ThrowUndefinedIfHole)         \
    V(ThrowUndefinedIfHoleWithName) \
    V(DeoptIfHClassMismatch)        \
    V(DeoptIfInt32Condition)        \
    V(Deopt)                        \
    V(SetValueWithBarrier)          \
    V(GapMove)                      \
    V(ConstantGapMove)

#define CONVERSION_VERTEX_LIST(V) V(ToTaggedInt)

#define BRANCH_CONTROL_VERTEX_LIST(V) \
    V(BranchIfTrue)                   \
    V(BranchIfInt32Compare)           \
    V(BranchIfFloat64Compare)         \
    V(BranchIfReferenceEqual)

#define CONDITION_CONTROL_VERTEX_LIST(V) BRANCH_CONTROL_VERTEX_LIST(V)

#define UNCONDITIONAL_CONTROL_VERTEX_LIST(V) \
    V(Jump)                                  \
    V(JumpLoop)

#define TERMINAL_CONTROL_VERTEX_LIST(V) \
    V(Return)                           \
    V(Throw)

// List of control vertex types
#define CONTROL_VERTEX_LIST(V)       \
    TERMINAL_CONTROL_VERTEX_LIST(V)  \
    CONDITION_CONTROL_VERTEX_LIST(V) \
    UNCONDITIONAL_CONTROL_VERTEX_LIST(V)

// List of all vertex types (both value and control vertices)
#define ALL_VERTEX_LIST(V)     \
    NON_CONTROL_VERTEX_LIST(V) \
    CONTROL_VERTEX_LIST(V)

//==============================================================================
// Opcode Enum and Constants
//==============================================================================

// Define the opcode enum
#define DEF_OPCODE(type) type,
enum class VertexOpcode : uint16_t { ALL_VERTEX_LIST(DEF_OPCODE) INVALID };
#undef DEF_OPCODE

// Count opcodes
#define PLUS_ONE(type) +1
static constexpr int OPCODE_COUNT = ALL_VERTEX_LIST(PLUS_ONE);
#undef PLUS_ONE

static constexpr VertexOpcode FIRST_OPCODE = static_cast<VertexOpcode>(0);
static constexpr VertexOpcode LAST_OPCODE = static_cast<VertexOpcode>(OPCODE_COUNT - 1);

#define VERTEX_OPCODE_VALUE(type) VertexOpcode::type,

static constexpr VertexOpcode FIRST_UNCONDITIONAL_CONTROL_VERTEX_OPCODE =
    std::min({UNCONDITIONAL_CONTROL_VERTEX_LIST(VERTEX_OPCODE_VALUE)});
static constexpr VertexOpcode LAST_UNCONDITIONAL_CONTROL_VERTEX_OPCODE =
    std::max({UNCONDITIONAL_CONTROL_VERTEX_LIST(VERTEX_OPCODE_VALUE)});
#undef VERTEX_OPCODE_VALUE

inline const char *OpcodeToString(VertexOpcode opcode)
{
#define DEF_NAME(Name) #Name,
    static constexpr const char *names[] = {ALL_VERTEX_LIST(DEF_NAME)};
#undef DEF_NAME
    auto index = static_cast<int>(opcode);
    ASSERT(index >= 0 && index < static_cast<int>(sizeof(names)));
    return names[index];
}

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_OPCODE_LIST_H
