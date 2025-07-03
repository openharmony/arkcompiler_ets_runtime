/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_OPCODE_H
#define ECMASCRIPT_COMPILER_OPCODE_H

#include <string>

#include "ecmascript/compiler/bytecodes.h"

#include "ecmascript/compiler/lcr_opcodes.h"
#include "ecmascript/compiler/mcr_opcodes.h"
#include "ecmascript/compiler/hcr_opcodes.h"

namespace panda::ecmascript::kungfu {

#define SHARE_IMMUTABLE_META_DATA_CACHE_LIST(V)                                                 \
    V(CircuitRoot, CIRCUIT_ROOT, GateFlags::NONE_FLAG, 0, 0, 0)                                 \
    V(StateEntry, STATE_ENTRY, GateFlags::ROOT, 0, 0, 0)                                        \
    V(DependEntry, DEPEND_ENTRY, GateFlags::ROOT, 0, 0, 0)                                      \
    V(OrdinaryBlock, ORDINARY_BLOCK, GateFlags::CONTROL, 1, 0, 0)                               \
    V(DefaultCase, DEFAULT_CASE, GateFlags::CONTROL, 1, 0, 0)                                   \
    V(ReturnList, RETURN_LIST, GateFlags::ROOT, 0, 0, 0)                                        \
    V(ArgList, ARG_LIST, GateFlags::ROOT, 0, 0, 0)                                              \
    V(Dead, DEAD, GateFlags::NONE_FLAG, 0, 0, 0)                                                \
    V(Throw, THROW, GateFlags::CONTROL, 1, 1, 1)                                                \
    V(LoopExit, LOOP_EXIT, GateFlags::CONTROL, 1, 0, 0)                                         \
    V(LoopExitDepend, LOOP_EXIT_DEPEND, GateFlags::FIXED, 1, 1, 0)                              \
    V(LoopExitValue, LOOP_EXIT_VALUE, GateFlags::FIXED, 1, 0, 1)                                \
    V(DependRelay, DEPEND_RELAY, GateFlags::FIXED, 1, 1, 0)                                     \
    V(IfTrue, IF_TRUE, GateFlags::CONTROL, 1, 0, 0)                                             \
    V(IfFalse, IF_FALSE, GateFlags::CONTROL, 1, 0, 0)                                           \
    V(IfSuccess, IF_SUCCESS, GateFlags::CONTROL, 1, 0, 0)                                       \
    V(IfException, IF_EXCEPTION, GateFlags::CONTROL, 1, 1, 0)                                   \
    V(GetException, GET_EXCEPTION, GateFlags::NONE_FLAG, 1, 1, 0)                               \
    V(GetUnsharedConstPool, GET_UNSHARED_CONSTPOOL, GateFlags::NO_WRITE, 0, 0, 1)               \
    V(GetGlobalEnv, GET_GLOBAL_ENV, GateFlags::NO_WRITE, 0, 1, 0)                               \
    V(GetSuperConstructor, GET_SUPER_CONSTRUCTOR, GateFlags::NO_WRITE, 1, 1, 1)                 \
    V(CheckSafePointAndStackOver, CHECK_SAFEPOINT_AND_STACKOVER, GateFlags::NO_WRITE, 1, 1, 0)  \
    V(DeoptCheck, DEOPT_CHECK, GateFlags::NO_WRITE, 1, 1, 3)                                    \
    V(LoopBack, LOOP_BACK, GateFlags::CONTROL, 1, 0, 0)                                         \
    V(Return, RETURN, GateFlags::HAS_ROOT, 1, 1, 1)                                             \
    V(ReturnVoid, RETURN_VOID, GateFlags::HAS_ROOT, 1, 1, 0)                                    \
    V(StateSplit, STATE_SPLIT, GateFlags::CHECKABLE, 1, 1, 0)                                   \
    V(GetEnv, GET_ENV, GateFlags::NO_WRITE, 0, 1, 1)                                            \
    V(GetGlobalEnvByFunc, GET_GLOBAL_ENV_BY_FUNC, GateFlags::NO_WRITE, 0, 1, 1)                 \
    V(GetGlobalEnvByLexicalEnv, GET_GLOBAL_ENV_BY_LEXICAL_ENV, GateFlags::NO_WRITE, 0, 1, 1)

#define SHARE_INTRINSIC_IMMUTABLE_META_DATA_CACHE_LIST(V)                                                       \
    V(TaggedIsHeapObjectIntrinsic, TAGGED_IS_HEAPOBJECT_INTRINSIC, GateFlags::NONE_FLAG, 0, 0, 2)               \
    V(IsStableElementsIntrinsic, IS_STABLE_ELEMENTS_INTRINSIC, GateFlags::NONE_FLAG, 0, 1, 3)                   \
    V(HasPendingExceptionIntrinsic, HAS_PENDING_EXCEPTION_INTRINSIC, GateFlags::NONE_FLAG, 0, 1, 3)             \
    V(CheckTaggedObjectIsString, CHECK_OBJECT_IS_STRING, GateFlags::NONE_FLAG, 0, 1, 5)                         \
    V(IsJsCOWArrayIntrinsic, IS_JS_COW_ARRAY_INTRINSIC, GateFlags::NONE_FLAG, 0, 1, 6)                          \

#define SHARE_GATE_META_DATA_LIST_WITH_VALUE_IN(V)                                       \
    V(FrameValues, FRAME_VALUES, GateFlags::NONE_FLAG, 0, 0, value)                      \
    V(ValueSelector, VALUE_SELECTOR, GateFlags::FIXED, 1, 0, value)

#define SHARE_GATE_META_DATA_LIST_WITH_SIZE(V)                                 \
    V(LoopBegin, LOOP_BEGIN, GateFlags::CONTROL, value, 0, 0)                  \
    V(Merge, MERGE, GateFlags::CONTROL, value, 0, 0)                           \
    V(DependSelector, DEPEND_SELECTOR, GateFlags::FIXED, 1, value, 0)          \
    SHARE_GATE_META_DATA_LIST_WITH_VALUE_IN(V)

#define SHARE_GATE_META_DATA_LIST_WITH_VALUE(V)                                         \
    V(Constant, CONSTANT, GateFlags::NONE_FLAG, 0, 0, 0)                                \
    V(HeapConstant, HEAP_CONSTANT, GateFlags::NONE_FLAG, 0, 0, 0)                       \
    V(FrameArgs, FRAME_ARGS, GateFlags::HAS_FRAME_STATE, 0, 0, 7)                       \
    V(FrameState, FRAME_STATE, GateFlags::HAS_FRAME_STATE, 0, 0, 2)                     \
    V(IfBranch, IF_BRANCH, GateFlags::CONTROL, 1, 0, 1)                                 \
    V(RelocatableData, RELOCATABLE_DATA, GateFlags::NONE_FLAG, 0, 0, 0)                 \
    V(SwitchBranch, SWITCH_BRANCH, GateFlags::CONTROL, 1, 0, 1)                         \
    V(SwitchCase, SWITCH_CASE, GateFlags::CONTROL, 1, 0, 0)                             \
    V(GetSharedConstPool, GET_SHARED_CONSTPOOL, GateFlags::NO_WRITE, 0, 0, 1)

#define SHARE_GATE_OPCODE_LIST(V)     \
    V(CONSTSTRING)

#define SHARE_GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)   \
    V(Arg, ARG, GateFlags::HAS_ROOT, 0, 0, 0)             \
    V(InitVreg, INITVREG, GateFlags::HAS_ROOT, 0, 0, 0)  \
    SHARE_GATE_META_DATA_LIST_WITH_VALUE(V)

#define IMMUTABLE_META_DATA_CACHE_LIST(V)                                                       \
    SHARE_IMMUTABLE_META_DATA_CACHE_LIST(V)                                                     \
    LCR_IMMUTABLE_META_DATA_CACHE_LIST(V)                                                       \
    MCR_IMMUTABLE_META_DATA_CACHE_LIST(V)                                                       \
    HCR_IMMUTABLE_META_DATA_CACHE_LIST(V)                                                       \
    SHARE_INTRINSIC_IMMUTABLE_META_DATA_CACHE_LIST(V)

#define GATE_META_DATA_LIST_WITH_VALUE_IN(V)                                             \
    SHARE_GATE_META_DATA_LIST_WITH_VALUE_IN(V)                                           \
    HCR_GATE_META_DATA_LIST_WITH_VALUE_IN(V)                                             \
    MCR_GATE_META_DATA_LIST_WITH_VALUE_IN(V)

#define GATE_META_DATA_LIST_WITH_PC_OFFSET(V)                                  \
    MCR_GATE_META_DATA_LIST_WITH_PC_OFFSET(V)                                  \
    HCR_GATE_META_DATA_LIST_WITH_PC_OFFSET(V)

#define GATE_META_DATA_LIST_FOR_CALL(V)                                        \
    MCR_GATE_META_DATA_LIST_FOR_CALL(V)

#define GATE_META_DATA_LIST_FOR_NEW(V)                                         \
    HCR_GATE_META_DATA_LIST_FOR_NEW(V)

#define GATE_META_DATA_LIST_WITH_PC_OFFSET_FIXED_VALUE(V)                      \
    HCR_GATE_META_DATA_LIST_WITH_PC_OFFSET_FIXED_VALUE(V)

#define GATE_META_DATA_LIST_WITH_SIZE(V)                                       \
    SHARE_GATE_META_DATA_LIST_WITH_SIZE(V)                                     \
    HCR_GATE_META_DATA_LIST_WITH_SIZE(V)                                       \
    MCR_GATE_META_DATA_LIST_WITH_SIZE(V)

#define GATE_META_DATA_LIST_WITH_GATE_TYPE(V)                                  \
    MCR_GATE_META_DATA_LIST_WITH_GATE_TYPE(V)

#define GATE_META_DATA_LIST_WITH_VALUE(V)                                               \
    SHARE_GATE_META_DATA_LIST_WITH_VALUE(V)                                             \
    LCR_GATE_META_DATA_LIST_WITH_VALUE(V)                                             \
    MCR_GATE_META_DATA_LIST_WITH_VALUE(V)                                             \
    HCR_GATE_META_DATA_LIST_WITH_VALUE(V)

#define GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)         \
    SHARE_GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)       \
    LCR_GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)       \
    MCR_GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)       \
    HCR_GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)

#define GATE_META_DATA_LIST_WITH_BOOL(V)                                           \
    MCR_GATE_META_DATA_LIST_WITH_BOOL(V)

#define GATE_META_DATA_LIST_WITH_BOOL_VALUE_IN(V)                                  \
    HCR_GATE_META_DATA_LIST_WITH_BOOL_VALUE_IN(V)

#define GATE_OPCODE_LIST(V)     \
    SHARE_GATE_OPCODE_LIST(V)   \
    HCR_GATE_OPCODE_LIST(V)

enum class OpCode : uint16_t {
    NOP = 0,
#define DECLARE_GATE_OPCODE(NAME, OP, R, S, D, V) OP,
    IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_SIZE(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_ONE_PARAMETER(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_PC_OFFSET(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_FOR_CALL(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_FOR_NEW(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_PC_OFFSET_FIXED_VALUE(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_BOOL(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_BOOL_VALUE_IN(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
#define DECLARE_GATE_OPCODE(NAME) NAME,
    GATE_OPCODE_LIST(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
};

// Special virtual register in the OSR.
static constexpr size_t INIT_VRGE_GLUE = -1;
static constexpr size_t INIT_VRGE_ARGS = -2;
static constexpr size_t INIT_VRGE_ARGV = -3;
static constexpr size_t INIT_VRGE_FUNCTION = -4;
static constexpr size_t INIT_VRGE_NEW_TARGET = -5;
static constexpr size_t INIT_VRGE_THIS_OBJECT = -6;
static constexpr size_t INIT_VRGE_NUM_ARGS = -7;
static constexpr size_t INIT_VRGE_ENV = -8;

}

#endif  // ECMASCRIPT_COMPILER_SHARE_GATE_META_DATA_H
