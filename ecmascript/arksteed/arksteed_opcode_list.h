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
    V(Int64Constant)                  \
    V(Float64Constant)                \
    V(TaggedConstant)                 \
    V(HeapConstant)

#define VALUE_VERTEX_LIST(V)            \
    CONSTANT_VALUE_VERTEX_LIST(V)       \
    V(InitialValue)                     \
    V(ActualArgc)                       \
    V(Call)                             \
    V(CallRuntime)                      \
    V(CallCommonStub)                   \
    V(StringLoadElement)                \
    V(Phi)                              \
    V(LoadTaggedFromAddress)            \
    V(LoadI32FromAddress)               \
    V(LoadI64FromAddress)               \
    V(LoadF64FromAddress)               \
    V(LoadTaggedField)                  \
    V(LoadInt32Field)                   \
    V(LoadTaggedElement)                \
    V(LoadPrototypeFromObject)          \
    V(LoadPrototypeHolderByHClass)      \
    V(ConvertHoleToUndefined)           \
    V(LoadHClassAddress)                \
    V(FindPrototypeHolder)              \
    V(PrepareSharedStoreField)          \
    V(EnsurePropertiesCapacity)         \
    V(LoadException)                    \
    V(TaggedIntToI32)                   \
    V(CheckedTaggedIntToI32)            \
    V(CheckedTaggedString)              \
    V(I32ConditionCheck)                \
    V(F64ConditionCheck)                \
    V(TaggedEqual)                      \
    V(TaggedNotEqual)                   \
    V(StringEqual)                      \
    V(I32AddWithOverflow)               \
    V(I32SubWithOverflow)               \
    V(I32MulWithOverflow)               \
    V(I32DivWithOverflow)               \
    V(I32DivByConstWithCheck)           \
    V(I32Add)                           \
    V(I32Sub)                           \
    V(I32Mul)                           \
    V(I32Div)                           \
    V(CheckedI32Mod)                    \
    V(I32BitwiseBinary)                 \
    V(I32ToTaggedInt)                   \
    V(RawI64ToTagged)                   \
    V(TaggedToRawI64)                   \
    V(I64BitwiseBinary)                 \
    V(CheckedNonNegativeI32ToTaggedInt) \
    V(I32BNot)                          \
    V(I32NegWithOverflow)               \
    V(I32IncWithOverflow)               \
    V(I32DecWithOverflow)               \
    V(I32ToF64)                         \
    V(CheckedNumberToF64)               \
    V(F64ToI32Trunc)                    \
    V(F64ToTaggedDouble)                \
    V(F64Neg)                           \
    V(F64Add)                           \
    V(F64Sub)                           \
    V(F64Mul)                           \
    V(F64Div)                           \

#define NON_VALUE_VERTEX_LIST(V)    \
    V(DeoptIfHClassMismatch)        \
    V(DeoptIfHClassNotIn)           \
    V(DeoptIfPrototypeChanged)      \
    V(DeoptIfTaggedCondition)       \
    V(DeoptIfInt32Condition)        \
    V(DeoptIfNotNumber)             \
    V(StoreTaggedToAddress)         \
    V(StoreI32ToAddress)            \
    V(StoreI64ToAddress)            \
    V(StoreF64ToAddress)            \
    V(StoreTaggedField)             \
    V(StoreInt32Field)              \
    V(StoreDoubleField)             \
    V(StoreInt32FieldWithRep)       \
    V(StoreDoubleFieldWithRep)      \
    V(StoreTaggedFieldWithBarrier)  \
    V(StoreSharedFieldWithBarrier)  \
    V(TransitionHClassWithBarrier)  \
    V(StoreTaggedFieldByHClass)     \
    V(StoreEnvSlot)                 \
    V(SetValueWithBarrier)          \
    V(GapMove)                      \
    V(ConstantGapMove)

#define NON_CONTROL_VERTEX_LIST(V) \
    NON_VALUE_VERTEX_LIST(V)       \
    VALUE_VERTEX_LIST(V)

#define BRANCH_CONTROL_VERTEX_LIST(V)       \
    V(BranchIfTrue)                         \
    V(BranchIfTaggedString)                 \
    V(BranchIfHClassIn)                     \
    V(BranchIfInt32Compare)                 \
    V(BranchIfInt64Compare)                 \
    V(BranchIfFloat64Compare)               \
    V(BranchIfReferenceEqual)               \
    V(BranchIfObjectType)                   \
    V(BranchIfTaggedHeapObject)

#define UNCONDITIONAL_CONTROL_VERTEX_LIST(V) \
    V(Jump)                                  \
    V(JumpLoop)

#define TERMINAL_CONTROL_VERTEX_LIST(V) \
    V(Return)                           \
    V(Throw)                            \
    V(Deopt)

// List of control vertex types
#define CONTROL_VERTEX_LIST(V)              \
    BRANCH_CONTROL_VERTEX_LIST(V)           \
    UNCONDITIONAL_CONTROL_VERTEX_LIST(V)    \
    TERMINAL_CONTROL_VERTEX_LIST(V)

// List of all vertex types (both value and control vertices)
#define ALL_VERTEX_LIST(V)     \
    NON_CONTROL_VERTEX_LIST(V) \
    CONTROL_VERTEX_LIST(V)

#define VERTEX_LISTS_FOR_EACH(V)                                        \
    V(CONSTANT_VALUE_VERTEX_LIST,        IsConstantValueVertex)         \
    V(VALUE_VERTEX_LIST,                 IsValueVertex)                 \
    V(NON_VALUE_VERTEX_LIST,             IsNonValueVertex)              \
    V(NON_CONTROL_VERTEX_LIST,           IsNonControlVertex)            \
    V(BRANCH_CONTROL_VERTEX_LIST,        IsBranchControlVertex)         \
    V(UNCONDITIONAL_CONTROL_VERTEX_LIST, IsUnconditionalControlVertex)  \
    V(TERMINAL_CONTROL_VERTEX_LIST,      IsTerminalControlVertex)       \
    V(CONTROL_VERTEX_LIST,               IsControlVertex)

//==============================================================================
// Opcode Enum and Constants
//==============================================================================

// Define the opcode enum
#define DEF_OPCODE(type) type,
enum class VertexOpcode : uint16_t { INVALID, ALL_VERTEX_LIST(DEF_OPCODE) };
#undef DEF_OPCODE

// Count opcodes
#define PLUS_ONE(type) +1
static constexpr uint32_t OPCODE_COUNT = ALL_VERTEX_LIST(PLUS_ONE);
#undef PLUS_ONE

static constexpr VertexOpcode FIRST_OPCODE = static_cast<VertexOpcode>(1);
static constexpr VertexOpcode LAST_OPCODE = static_cast<VertexOpcode>(OPCODE_COUNT);

#define CASE(type) case VertexOpcode::type:
#define DEFINE_OPCODE_PREDICATE(CUR_OPCODE_LIST, FunctionName)  \
    constexpr bool FunctionName(VertexOpcode opcode)            \
    {                                                           \
        switch (opcode) {                                       \
            CUR_OPCODE_LIST(CASE)                               \
                return true;                                    \
            default:                                            \
                return false;                                   \
        }                                                       \
    }
VERTEX_LISTS_FOR_EACH(DEFINE_OPCODE_PREDICATE)
#undef DEFINE_OPCODE_PREDICATE
#undef CASE

inline const char *OpcodeToString(VertexOpcode opcode)
{
#define DEF_NAME(Name) #Name,
    static constexpr const char *names[] = {ALL_VERTEX_LIST(DEF_NAME)};
#undef DEF_NAME
    unsigned index = static_cast<unsigned>(opcode);
    ASSERT(index >= 1 && index <= sizeof(names));  // 1 : Opcode is 1-based. 0 is reserved for INVALID
    return names[index - 1];
}

//==============================================================================
// Forward Declaration & Opcode Mapping
//==============================================================================

#define DEF_FORWARD_DECLARATION(type) class type##Vertex;
ALL_VERTEX_LIST(DEF_FORWARD_DECLARATION)
#undef DEF_FORWARD_DECLARATION

template <typename T>
struct OpcodeTraits;

template <typename T>
constexpr VertexOpcode OpcodeOf = OpcodeTraits<T>::value;

#define DEF_OPCODE_OF_TRAITS(type)                                  \
    template <>                                                     \
    struct OpcodeTraits<type##Vertex> {                             \
        static constexpr VertexOpcode value = VertexOpcode::type;   \
    };
ALL_VERTEX_LIST(DEF_OPCODE_OF_TRAITS)
#undef DEF_OPCODE_HELPER

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_OPCODE_LIST_H
