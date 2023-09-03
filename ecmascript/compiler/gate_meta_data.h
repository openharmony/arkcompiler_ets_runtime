/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_GATE_META_DATA_H
#define ECMASCRIPT_COMPILER_GATE_META_DATA_H

#include <string>

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/chunk_containers.h"

#include "ecmascript/pgo_profiler/pgo_profiler_type.h"
#include "ecmascript/elements.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::kungfu {
using GateRef = int32_t;
enum MachineType : uint8_t { // Bit width
    NOVALUE = 0,
    ANYVALUE,
    ARCH,
    FLEX,
    I1,
    I8,
    I16,
    I32,
    I64,
    F32,
    F64,
};

enum class TypedOpKind : uint8_t {
    TYPED_BIN_OP,
    TYPED_CALL_TARGET_CHECK_OP,
    TYPED_UN_OP,
    TYPED_JUMP_OP,
    TYPED_STORE_OP,
    TYPED_LOAD_OP,
};

enum class TypedBinOp : uint8_t {
    TYPED_ADD = 0,
    TYPED_SUB,
    TYPED_MUL,
    TYPED_DIV,
    TYPED_MOD,
    TYPED_LESS,
    TYPED_LESSEQ,
    TYPED_GREATER,
    TYPED_GREATEREQ,
    TYPED_EQ,
    TYPED_NOTEQ,
    TYPED_STRICTEQ,
    TYPED_SHL,
    TYPED_SHR,
    TYPED_ASHR,
    TYPED_AND,
    TYPED_OR,
    TYPED_XOR,
    TYPED_EXP,
};

enum class TypedCallTargetCheckOp : uint8_t {
    JSCALL_IMMEDIATE_AFTER_FUNC_DEF = 0,
    JSCALL,
    JSCALL_FAST,
    JSCALLTHIS,
    JSCALLTHIS_FAST,
    JSCALLTHIS_NOGC,
    JSCALLTHIS_FAST_NOGC,
};

enum class TypedUnOp : uint8_t {
    TYPED_NEG = 0,
    TYPED_NOT,
    TYPED_INC,
    TYPED_DEC,
    TYPED_ISFALSE,
    TYPED_ISTRUE,
};

enum class TypedJumpOp : uint8_t {
    TYPED_JEQZ = 0,
    TYPED_JNEZ,
};

enum class BranchKind : uint8_t {
    NORMAL_BRANCH = 0,
    TRUE_BRANCH,
    FALSE_BRANCH,
    STRONG_TRUE_BRANCH,
    STRONG_FALSE_BRANCH,
};

class BranchWeight {
public:
    static constexpr uint32_t ZERO_WEIGHT = 0;
    static constexpr uint32_t ONE_WEIGHT = 1;
    static constexpr uint32_t WEAK_WEIGHT = 10;
    static constexpr uint32_t STRONG_WEIGHT = 1000;
    static constexpr uint32_t DEOPT_WEIGHT = 2000;
};

#define GATE_META_DATA_DEOPT_REASON(V)        \
    V(NotInt, NOTINT)                         \
    V(NotDouble, NOTDOUBLE)                   \
    V(NotNumber, NOTNUMBER)                   \
    V(NotBool, NOTBOOL)                       \
    V(NotHeapObject, NOTHEAPOBJECT)           \
    V(NotStableArray, NOTSARRAY)              \
    V(NotArray, NOTARRAY)                     \
    V(NotOnHeap, NOTONHEAP)                   \
    V(InconsistentHClass, INCONSISTENTHCLASS) \
    V(NotNewObj, NOTNEWOBJ)                   \
    V(NotLegalIndex, NOTLEGALIDX)             \
    V(NotIncOverflow, NOTINCOV)               \
    V(NotDecOverflow, NOTDECOV)               \
    V(NotNegativeOverflow, NOTNEGOV)          \
    V(NotCallTarget, NOTCALLTGT)              \
    V(NotJSCallTarget, NOTJSCALLTGT)          \
    V(CowArray, COWARRAY)                     \
    V(DivideZero, DIVZERO)                    \
    V(InlineFail, INLINEFAIL)                 \
    V(NotJSFastCallTarget, NOTJSFASTCALLTGT)  \
    V(LexVarIsHole, LEXVARISHOLE)             \
    V(ModZero, MODZERO)                       \
    V(Int32Overflow, INT32OVERFLOW)           \
    V(NotString, NOTSTRING)

enum class DeoptType : uint8_t {
    NOTCHECK = 0,
#define DECLARE_DEOPT_TYPE(NAME, TYPE) TYPE,
    GATE_META_DATA_DEOPT_REASON(DECLARE_DEOPT_TYPE)
#undef DECLARE_DEOPT_TYPE
};

enum class ICmpCondition : uint8_t {
    EQ = 1,
    UGT,
    UGE,
    ULT,
    ULE,
    NE,
    SGT,
    SGE,
    SLT,
    SLE,
};

enum class FCmpCondition : uint8_t {
    ALW_FALSE = 0,
    OEQ,
    OGT,
    OGE,
    OLT,
    OLE,
    ONE,
    ORD,
    UNO,
    UEQ,
    UGT,
    UGE,
    ULT,
    ULE,
    UNE,
    ALW_TRUE,
};

enum class TypedStoreOp : uint8_t {
    ARRAY_STORE_ELEMENT = 0,
    INT8ARRAY_STORE_ELEMENT,
    UINT8ARRAY_STORE_ELEMENT,
    UINT8CLAMPEDARRAY_STORE_ELEMENT,
    INT16ARRAY_STORE_ELEMENT,
    UINT16ARRAY_STORE_ELEMENT,
    INT32ARRAY_STORE_ELEMENT,
    UINT32ARRAY_STORE_ELEMENT,
    FLOAT32ARRAY_STORE_ELEMENT,
    FLOAT64ARRAY_STORE_ELEMENT,

    TYPED_ARRAY_FIRST = INT8ARRAY_STORE_ELEMENT,
    TYPED_ARRAY_LAST = FLOAT64ARRAY_STORE_ELEMENT,
};

enum class MemoryType : uint8_t {
    ELEMENT_TYPE = 0,
};

enum class TypedLoadOp : uint8_t {
    ARRAY_LOAD_INT_ELEMENT = 0,
    ARRAY_LOAD_DOUBLE_ELEMENT,
    ARRAY_LOAD_OBJECT_ELEMENT,
    ARRAY_LOAD_TAGGED_ELEMENT,
    ARRAY_LOAD_HOLE_TAGGED_ELEMENT,
    INT8ARRAY_LOAD_ELEMENT,
    UINT8ARRAY_LOAD_ELEMENT,
    UINT8CLAMPEDARRAY_LOAD_ELEMENT,
    INT16ARRAY_LOAD_ELEMENT,
    UINT16ARRAY_LOAD_ELEMENT,
    INT32ARRAY_LOAD_ELEMENT,
    UINT32ARRAY_LOAD_ELEMENT,
    FLOAT32ARRAY_LOAD_ELEMENT,
    FLOAT64ARRAY_LOAD_ELEMENT,
    STRING_LOAD_ELEMENT,

    TYPED_ARRAY_FIRST = INT8ARRAY_LOAD_ELEMENT,
    TYPED_ARRAY_LAST = FLOAT64ARRAY_LOAD_ELEMENT,
};

std::string MachineTypeToStr(MachineType machineType);

#define BINARY_GATE_META_DATA_CACHE_LIST(V)                                                          \
    V(Add, ADD, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Sub, SUB, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Mul, MUL, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Exp, EXP, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Sdiv, SDIV, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(Smod, SMOD, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(Udiv, UDIV, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(Umod, UMOD, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(Fdiv, FDIV, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(Fmod, FMOD, GateFlags::NONE_FLAG, 0, 0, 2)                                                     \
    V(And, AND, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Xor, XOR, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Or, OR, GateFlags::NONE_FLAG, 0, 0, 2)                                                         \
    V(Lsl, LSL, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Lsr, LSR, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(Asr, ASR, GateFlags::NONE_FLAG, 0, 0, 2)                                                       \
    V(AddWithOverflow, ADD_WITH_OVERFLOW, GateFlags::NONE_FLAG, 0, 0, 2)                             \
    V(SubWithOverflow, SUB_WITH_OVERFLOW, GateFlags::NONE_FLAG, 0, 0, 2)                             \
    V(MulWithOverflow, MUL_WITH_OVERFLOW, GateFlags::NONE_FLAG, 0, 0, 2)                             \
    V(ExtractValue, EXTRACT_VALUE, GateFlags::NONE_FLAG, 0, 0, 2)                                    \
    V(Sqrt, SQRT, GateFlags::NO_WRITE, 0, 0, 1)                                                      \
    V(Int32CheckRightIsZero, INT32_CHECK_RIGHT_IS_ZERO, GateFlags::CHECKABLE, 1, 1, 1)               \
    V(Float64CheckRightIsZero, FLOAT64_CHECK_RIGHT_IS_ZERO, GateFlags::CHECKABLE, 1, 1, 1)           \
    V(ValueCheckNegOverflow, VALUE_CHECK_NEG_OVERFLOW, GateFlags::CHECKABLE, 1, 1, 1)                \
    V(OverflowCheck, OVERFLOW_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                  \
    V(Int32UnsignedUpperBoundCheck, INT32_UNSIGNED_UPPER_BOUND_CHECK, GateFlags::CHECKABLE, 1, 1, 2) \
    V(Int32DivWithCheck, INT32_DIV_WITH_CHECK, GateFlags::CHECKABLE, 1, 1, 2)                        \
    V(LexVarIsHoleCheck, LEX_VAR_IS_HOLE_CHECK, GateFlags::CHECKABLE, 1, 1, 1)

#define UNARY_GATE_META_DATA_CACHE_LIST(V)                                       \
    V(Zext, ZEXT, GateFlags::NONE_FLAG, 0, 0, 1)                                 \
    V(Sext, SEXT, GateFlags::NONE_FLAG, 0, 0, 1)                                 \
    V(Trunc, TRUNC, GateFlags::NONE_FLAG, 0, 0, 1)                               \
    V(Fext, FEXT, GateFlags::NONE_FLAG, 0, 0, 1)                                 \
    V(Ftrunc, FTRUNC, GateFlags::NONE_FLAG, 0, 0, 1)                             \
    V(Rev, REV, GateFlags::NONE_FLAG, 0, 0, 1)                                   \
    V(TruncFloatToInt64, TRUNC_FLOAT_TO_INT64, GateFlags::NONE_FLAG, 0, 0, 1)    \
    V(TruncFloatToInt32, TRUNC_FLOAT_TO_INT32, GateFlags::NONE_FLAG, 0, 0, 1)    \
    V(TaggedToInt64, TAGGED_TO_INT64, GateFlags::NONE_FLAG, 0, 0, 1)             \
    V(Int64ToTagged, INT64_TO_TAGGED, GateFlags::NONE_FLAG, 0, 0, 1)             \
    V(SignedIntToFloat, SIGNED_INT_TO_FLOAT, GateFlags::NONE_FLAG, 0, 0, 1)      \
    V(UnsignedIntToFloat, UNSIGNED_INT_TO_FLOAT, GateFlags::NONE_FLAG, 0, 0, 1)  \
    V(FloatToSignedInt, FLOAT_TO_SIGNED_INT, GateFlags::NONE_FLAG, 0, 0, 1)      \
    V(UnsignedFloatToInt, UNSIGNED_FLOAT_TO_INT, GateFlags::NONE_FLAG, 0, 0, 1)  \
    V(Bitcast, BITCAST, GateFlags::NONE_FLAG, 0, 0, 1)

#define IMMUTABLE_META_DATA_CACHE_LIST(V)                                                       \
    V(CircuitRoot, CIRCUIT_ROOT, GateFlags::NONE_FLAG, 0, 0, 0)                                 \
    V(StateEntry, STATE_ENTRY, GateFlags::ROOT, 0, 0, 0)                                        \
    V(DependEntry, DEPEND_ENTRY, GateFlags::ROOT, 0, 0, 0)                                      \
    V(ReturnList, RETURN_LIST, GateFlags::ROOT, 0, 0, 0)                                        \
    V(ArgList, ARG_LIST, GateFlags::ROOT, 0, 0, 0)                                              \
    V(Return, RETURN, GateFlags::HAS_ROOT, 1, 1, 1)                                             \
    V(ReturnVoid, RETURN_VOID, GateFlags::HAS_ROOT, 1, 1, 0)                                    \
    V(Throw, THROW, GateFlags::CONTROL, 1, 1, 1)                                                \
    V(OrdinaryBlock, ORDINARY_BLOCK, GateFlags::CONTROL, 1, 0, 0)                               \
    V(IfTrue, IF_TRUE, GateFlags::CONTROL, 1, 0, 0)                                             \
    V(IfFalse, IF_FALSE, GateFlags::CONTROL, 1, 0, 0)                                           \
    V(LoopBegin, LOOP_BEGIN, GateFlags::CONTROL, 2, 0, 0)                                       \
    V(LoopBack, LOOP_BACK, GateFlags::CONTROL, 1, 0, 0)                                         \
    V(LoopExit, LOOP_EXIT, GateFlags::CONTROL, 1, 0, 0)                                         \
    V(LoopExitDepend, LOOP_EXIT_DEPEND, GateFlags::FIXED, 1, 1, 0)                              \
    V(LoopExitValue, LOOP_EXIT_VALUE, GateFlags::FIXED, 1, 0, 1)                                \
    V(DependRelay, DEPEND_RELAY, GateFlags::FIXED, 1, 1, 0)                                     \
    V(IfSuccess, IF_SUCCESS, GateFlags::CONTROL, 1, 0, 0)                                       \
    V(IfException, IF_EXCEPTION, GateFlags::CONTROL, 1, 1, 0)                                   \
    V(GetException, GET_EXCEPTION, GateFlags::NONE_FLAG, 1, 1, 0)                               \
    V(GetConstPool, GET_CONSTPOOL, GateFlags::NO_WRITE, 0, 1, 1)                                \
    V(GetGlobalEnv, GET_GLOBAL_ENV, GateFlags::NO_WRITE, 0, 1, 0)                               \
    V(StateSplit, STATE_SPLIT, GateFlags::CHECKABLE, 1, 1, 0)                                   \
    V(Load, LOAD, GateFlags::NO_WRITE, 0, 1, 1)                                                 \
    V(Store, STORE, GateFlags::NONE_FLAG, 0, 1, 2)                                              \
    V(TypedCallCheck, TYPED_CALL_CHECK, GateFlags::CHECKABLE, 1, 1, 3)                          \
    V(HeapObjectCheck, HEAP_OBJECT_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                        \
    V(COWArrayCheck, COW_ARRAY_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                            \
    V(ArrayGuardianCheck, ARRAY_GUARDIAN_CHECK, GateFlags::CHECKABLE, 1, 1, 0)                  \
    V(EcmaStringCheck, ECMA_STRING_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                        \
    V(FlattenStringCheck, FLATTEN_STRING_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                  \
    V(DeoptCheck, DEOPT_CHECK, GateFlags::NO_WRITE, 1, 1, 3)                                    \
    V(StoreProperty, STORE_PROPERTY, GateFlags::NONE_FLAG, 1, 1, 3)                             \
    V(StorePropertyNoBarrier, STORE_PROPERTY_NO_BARRIER, GateFlags::NONE_FLAG, 1, 1, 3)         \
    V(ToLength, TO_LENGTH, GateFlags::NONE_FLAG, 1, 1, 1)                                       \
    V(DefaultCase, DEFAULT_CASE, GateFlags::CONTROL, 1, 0, 0)                                   \
    V(LoadArrayLength, LOAD_ARRAY_LENGTH, GateFlags::NO_WRITE, 1, 1, 1)                         \
    V(LoadStringLength, LOAD_STRING_LENGTH, GateFlags::NO_WRITE, 1, 1, 1)                       \
    V(TypedNewAllocateThis, TYPED_NEW_ALLOCATE_THIS, GateFlags::CHECKABLE, 1, 1, 2)             \
    V(TypedSuperAllocateThis, TYPED_SUPER_ALLOCATE_THIS, GateFlags::CHECKABLE, 1, 1, 2)         \
    V(GetSuperConstructor, GET_SUPER_CONSTRUCTOR, GateFlags::NO_WRITE, 1, 1, 1)                 \
    V(CheckSafePointAndStackOver, CHECK_SAFEPOINT_AND_STACKOVER, GateFlags::NO_WRITE, 1, 1, 0)  \
    V(Dead, DEAD, GateFlags::NONE_FLAG, 0, 0, 0)                                                \
    V(FrameArgs, FRAME_ARGS, GateFlags::NONE_FLAG, 0, 0, 4)                                     \
    V(GetEnv, GET_ENV, GateFlags::NONE_FLAG, 0, 0, 1)                                           \
    V(ConvertHoleAsUndefined, CONVERT_HOLE_AS_UNDEFINED, GateFlags::NO_WRITE, 1, 1, 1)          \
    V(StartAllocate, START_ALLOCATE, GateFlags::NONE_FLAG, 0, 1, 0)                             \
    V(FinishAllocate, FINISH_ALLOCATE, GateFlags::NONE_FLAG, 0, 1, 0)                           \
    V(LoadGetter, LOAD_GETTER, GateFlags::NO_WRITE, 0, 1, 2)                                    \
    V(LoadSetter, LOAD_SETTER, GateFlags::NO_WRITE, 0, 1, 2)                                    \
    V(ReadSp, READSP, GateFlags::NONE_FLAG, 0, 0, 0)                                            \
    BINARY_GATE_META_DATA_CACHE_LIST(V)                                                         \
    UNARY_GATE_META_DATA_CACHE_LIST(V)

#define GATE_META_DATA_LIST_WITH_VALUE_IN(V)                                             \
    V(ValueSelector, VALUE_SELECTOR, GateFlags::FIXED, 1, 0, value)                      \
    V(FrameValues, FRAME_VALUES, GateFlags::NONE_FLAG, 0, 0, value)                      \
    V(RuntimeCall, RUNTIME_CALL, GateFlags::NONE_FLAG, 0, 1, value)                      \
    V(RuntimeCallWithArgv, RUNTIME_CALL_WITH_ARGV, GateFlags::NONE_FLAG, 0, 1, value)    \
    V(NoGcRuntimeCall, NOGC_RUNTIME_CALL, GateFlags::NO_WRITE, 0, 1, value)              \
    V(Call, CALL, GateFlags::NONE_FLAG, 0, 1, value)                                     \
    V(BytecodeCall, BYTECODE_CALL, GateFlags::NONE_FLAG, 0, 1, value)                    \
    V(DebuggerBytecodeCall, DEBUGGER_BYTECODE_CALL, GateFlags::NONE_FLAG, 0, 1, value)   \
    V(BuiltinsCallWithArgv, BUILTINS_CALL_WITH_ARGV, GateFlags::NONE_FLAG, 0, 1, value)  \
    V(BuiltinsCall, BUILTINS_CALL, GateFlags::NONE_FLAG, 0, 1, value)                    \
    V(SaveRegister, SAVE_REGISTER, GateFlags::NONE_FLAG, 0, 1, value)

#define GATE_META_DATA_LIST_WITH_PC_OFFSET(V)                                  \
    V(TypedCallBuiltin, TYPED_CALL_BUILTIN, GateFlags::NO_WRITE, 1, 1, value)  \
    V(Construct, CONSTRUCT, GateFlags::NONE_FLAG, 1, 1, value)

#define GATE_META_DATA_LIST_FOR_CALL(V)                                        \
    V(TypedCall, TYPEDCALL, GateFlags::NONE_FLAG, 1, 1, value)                 \
    V(TypedFastCall, TYPEDFASTCALL, GateFlags::NONE_FLAG, 1, 1, value)

#define GATE_META_DATA_LIST_WITH_PC_OFFSET_FIXED_VALUE(V)                      \
    V(CallGetter, CALL_GETTER, GateFlags::NONE_FLAG, 1, 1, 2)                  \
    V(CallSetter, CALL_SETTER, GateFlags::NONE_FLAG, 1, 1, 3)

#define GATE_META_DATA_LIST_WITH_SIZE(V)                                       \
    V(Merge, MERGE, GateFlags::CONTROL, value, 0, 0)                           \
    V(DependSelector, DEPEND_SELECTOR, GateFlags::FIXED, 1, value, 0)          \
    GATE_META_DATA_LIST_WITH_VALUE_IN(V)

#define GATE_META_DATA_LIST_WITH_GATE_TYPE(V)                                              \
    V(PrimitiveTypeCheck, PRIMITIVE_TYPE_CHECK, GateFlags::CHECKABLE, 1, 1, 1)             \
    V(TypedArrayCheck, TYPED_ARRAY_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                   \
    V(LoadTypedArrayLength, LOAD_TYPED_ARRAY_LENGTH, GateFlags::NO_WRITE, 1, 1, 1)         \
    V(IndexCheck, INDEX_CHECK, GateFlags::CHECKABLE, 1, 1, 2)                              \
    V(TypedUnaryOp, TYPED_UNARY_OP, GateFlags::NO_WRITE, 1, 1, 1)                          \
    V(TypedConditionJump, TYPED_CONDITION_JUMP, GateFlags::NO_WRITE, 1, 1, 1)              \
    V(TypedConvert, TYPE_CONVERT, GateFlags::NO_WRITE, 1, 1, 1)                            \
    V(CheckAndConvert, CHECK_AND_CONVERT, GateFlags::CHECKABLE, 1, 1, 1)                   \
    V(Convert, CONVERT, GateFlags::NONE_FLAG, 0, 0, 1)                                     \
    V(JSInlineTargetTypeCheck, JSINLINETARGET_TYPE_CHECK, GateFlags::CHECKABLE, 1, 1, 2)   \
    V(InlineAccessorCheck, INLINE_ACCESSOR_CHECK, GateFlags::CHECKABLE, 1, 1, 2)

#define GATE_META_DATA_LIST_WITH_VALUE(V)                                               \
    V(Icmp, ICMP, GateFlags::NONE_FLAG, 0, 0, 2)                                        \
    V(Fcmp, FCMP, GateFlags::NONE_FLAG, 0, 0, 2)                                        \
    V(IfBranch, IF_BRANCH, GateFlags::CONTROL, 1, 0, 1)                                 \
    V(Alloca, ALLOCA, GateFlags::NONE_FLAG, 0, 0, 0)                                    \
    V(SwitchBranch, SWITCH_BRANCH, GateFlags::CONTROL, 1, 0, 1)                         \
    V(SwitchCase, SWITCH_CASE, GateFlags::CONTROL, 1, 0, 0)                             \
    V(HeapAlloc, HEAP_ALLOC, GateFlags::NONE_FLAG, 1, 1, 1)                             \
    V(LoadConstOffset, LOAD_CONST_OFFSET, GateFlags::NO_WRITE, 0, 1, 1)                 \
    V(StoreConstOffset, STORE_CONST_OFFSET, GateFlags::NONE_FLAG, 1, 1, 2)              \
    V(LoadElement, LOAD_ELEMENT, GateFlags::NO_WRITE, 1, 1, 2)                          \
    V(StoreElement, STORE_ELEMENT, GateFlags::NONE_FLAG, 1, 1, 3)                       \
    V(StoreMemory, STORE_MEMORY, GateFlags::NONE_FLAG, 1, 1, 3)                         \
    V(RestoreRegister, RESTORE_REGISTER, GateFlags::NONE_FLAG, 0, 0, 1)                 \
    V(Constant, CONSTANT, GateFlags::NONE_FLAG, 0, 0, 0)                                \
    V(RelocatableData, RELOCATABLE_DATA, GateFlags::NONE_FLAG, 0, 0, 0)                 \
    V(GetGlobalEnvObj, GET_GLOBAL_ENV_OBJ, GateFlags::NO_WRITE, 0, 1, 1)                \
    V(GetGlobalEnvObjHClass, GET_GLOBAL_ENV_OBJ_HCLASS, GateFlags::NO_WRITE, 0, 1, 1)   \
    V(GetGlobalConstantValue, GET_GLOBAL_CONSTANT_VALUE, GateFlags::NO_WRITE, 0, 1, 0)  \
    V(FrameState, FRAME_STATE, GateFlags::HAS_FRAME_STATE, 0, 0, 2)                     \
    V(CreateArray, CREATE_ARRAY, GateFlags::NONE_FLAG, 1, 1, 0)                         \
    V(CreateArrayWithBuffer, CREATE_ARRAY_WITH_BUFFER, GateFlags::CHECKABLE, 1, 1, 2)   \
    V(RangeGuard, RANGE_GUARD, GateFlags::NO_WRITE, 1, 1, 1)                            \
    V(StableArrayCheck, STABLE_ARRAY_CHECK, GateFlags::CHECKABLE, 1, 1, 1)              \
    V(HClassStableArrayCheck, HCLASS_STABLE_ARRAY_CHECK, GateFlags::CHECKABLE, 1, 1, 1) \
    V(ObjectTypeCheck, OBJECT_TYPE_CHECK, GateFlags::CHECKABLE, 1, 1, 2)                \
    V(ObjectTypeCompare, OBJECT_TYPE_COMPARE, GateFlags::CHECKABLE, 1, 1, 2)

#define GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)         \
    V(Arg, ARG, GateFlags::HAS_ROOT, 0, 0, 0)             \
    GATE_META_DATA_LIST_WITH_VALUE(V)                     \
    GATE_META_DATA_LIST_WITH_GATE_TYPE(V)

#define GATE_META_DATA_LIST_WITH_BOOL(V)                                           \
    V(LoadProperty, LOAD_PROPERTY, GateFlags::NO_WRITE, 1, 1, 2)                   \

#define GATE_META_DATA_LIST_WITH_BOOL_VALUE_IN(V)                                  \
    V(CallOptimized, CALL_OPTIMIZED, GateFlags::NONE_FLAG, 0, 1, value)            \
    V(FastCallOptimized, FAST_CALL_OPTIMIZED, GateFlags::NONE_FLAG, 0, 1, value)   \

#define GATE_OPCODE_LIST(V)     \
    V(JS_BYTECODE)              \
    V(TYPED_BINARY_OP)          \
    V(TYPED_CALLTARGETCHECK_OP) \
    V(CONSTSTRING)

enum class OpCode : uint8_t {
    NOP = 0,
#define DECLARE_GATE_OPCODE(NAME, OP, R, S, D, V) OP,
    IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_SIZE(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_ONE_PARAMETER(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_PC_OFFSET(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_FOR_CALL(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_PC_OFFSET_FIXED_VALUE(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_BOOL(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_BOOL_VALUE_IN(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
#define DECLARE_GATE_OPCODE(NAME) NAME,
    GATE_OPCODE_LIST(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
};

enum GateFlags : uint8_t {
    NONE_FLAG = 0,
    NO_WRITE = 1 << 0,
    HAS_ROOT = 1 << 1,
    HAS_FRAME_STATE = 1 << 2,
    CONTROL = NO_WRITE,
    CONTROL_ROOT = NO_WRITE | HAS_ROOT,
    CHECKABLE = NO_WRITE | HAS_FRAME_STATE,
    ROOT = NO_WRITE | HAS_ROOT,
    FIXED = NO_WRITE,
};

class GateMetaData : public ChunkObject {
public:
    enum class Kind : uint8_t {
        IMMUTABLE = 0,
        MUTABLE_WITH_SIZE,
        IMMUTABLE_ONE_PARAMETER,
        MUTABLE_ONE_PARAMETER,
        IMMUTABLE_BOOL,
        MUTABLE_STRING,
        JSBYTECODE,
        TYPED_BINARY_OP,
        TYPED_CALLTARGETCHECK_OP,
        TYPED_CALL,
    };
    GateMetaData() = default;
    GateMetaData(OpCode opcode, GateFlags flags,
        uint32_t statesIn, uint16_t dependsIn, uint32_t valuesIn)
        : opcode_(opcode), flags_(flags),
        statesIn_(statesIn), dependsIn_(dependsIn), valuesIn_(valuesIn) {}

    static TypedBinOp GetRevCompareOp(TypedBinOp op)
    {
        switch (op) {
            case TypedBinOp::TYPED_LESS:
                return TypedBinOp::TYPED_GREATEREQ;
            case TypedBinOp::TYPED_LESSEQ:
                return TypedBinOp::TYPED_GREATER;
            case TypedBinOp::TYPED_GREATER:
                return TypedBinOp::TYPED_LESSEQ;
            case TypedBinOp::TYPED_GREATEREQ:
                return TypedBinOp::TYPED_LESS;
            case TypedBinOp::TYPED_EQ:
                return TypedBinOp::TYPED_NOTEQ;
            case TypedBinOp::TYPED_NOTEQ:
                return TypedBinOp::TYPED_EQ;
            default:
                UNREACHABLE();
                return op;
        }
    }

    static TypedBinOp GetSwapCompareOp(TypedBinOp op)
    {
        switch (op) {
            case TypedBinOp::TYPED_LESS:
                return TypedBinOp::TYPED_GREATER;
            case TypedBinOp::TYPED_LESSEQ:
                return TypedBinOp::TYPED_GREATEREQ;
            case TypedBinOp::TYPED_GREATER:
                return TypedBinOp::TYPED_LESS;
            case TypedBinOp::TYPED_GREATEREQ:
                return TypedBinOp::TYPED_LESSEQ;
            case TypedBinOp::TYPED_EQ:
                return TypedBinOp::TYPED_EQ;
            case TypedBinOp::TYPED_NOTEQ:
                return TypedBinOp::TYPED_NOTEQ;
            default:
                UNREACHABLE();
                return op;
        }
    }

    size_t GetStateCount() const
    {
        return statesIn_;
    }

    size_t GetDependCount() const
    {
        return dependsIn_;
    }

    size_t GetInValueCount() const
    {
        return valuesIn_;
    }

    size_t GetRootCount() const
    {
        return HasRoot() ? 1 : 0;
    }

    size_t GetInFrameStateCount() const
    {
        return HasFrameState() ? 1 : 0;
    }

    size_t GetNumIns() const
    {
        return GetStateCount() + GetDependCount() + GetInValueCount()
            + GetInFrameStateCount() + GetRootCount();
    }

    size_t GetInValueStarts() const
    {
        return GetStateCount() + GetDependCount();
    }

    size_t GetInFrameStateStarts() const
    {
        return GetInValueStarts() + GetInValueCount();
    }

    OpCode GetOpCode() const
    {
        return opcode_;
    }

    Kind GetKind() const
    {
        return kind_;
    }

    void AssertKind([[maybe_unused]] Kind kind) const
    {
        ASSERT(GetKind() == kind);
    }

    bool IsOneParameterKind() const
    {
        return GetKind() == Kind::IMMUTABLE_ONE_PARAMETER || GetKind() == Kind::MUTABLE_ONE_PARAMETER ||
            GetKind() == Kind::TYPED_BINARY_OP || GetKind() == Kind::TYPED_CALLTARGETCHECK_OP;
    }

    bool IsStringType() const
    {
        return GetKind() == Kind::MUTABLE_STRING;
    }

    bool IsRoot() const;
    bool IsProlog() const;
    bool IsFixed() const;
    bool IsSchedulable() const;
    bool IsState() const;  // note: IsState(STATE_ENTRY) == false
    bool IsGeneralState() const;
    bool IsTerminalState() const;
    bool IsVirtualState() const;
    bool IsCFGMerge() const;
    bool IsControlCase() const;
    bool IsLoopHead() const;
    bool IsNop() const;
    bool IsConstant() const;
    bool IsDependSelector() const;
    bool IsTypedOperator() const;
    bool IsCheckWithOneIn() const;
    bool IsCheckWithTwoIns() const;
    bool HasFrameState() const
    {
        return HasFlag(GateFlags::HAS_FRAME_STATE);
    }

    bool IsNotWrite() const
    {
        return HasFlag(GateFlags::NO_WRITE);
    }

    ~GateMetaData() = default;

    std::string TypedBinOpToStr(TypedBinOp typedBinOp) const;
    static std::string Str(OpCode opcode);
    std::string Str() const
    {
        return Str(opcode_);
    }
protected:
    void SetKind(Kind kind)
    {
        kind_ = kind;
    }

    void SetFlags(GateFlags flags)
    {
        flags_ = flags;
    }

    void DecreaseIn(size_t idx)
    {
        ASSERT(GetKind() == Kind::MUTABLE_WITH_SIZE);
        if (idx < statesIn_) {
            statesIn_--;
        } else if (idx < statesIn_ + dependsIn_) {
            dependsIn_--;
        } else {
            valuesIn_--;
        }
    }

    bool HasRoot() const
    {
        return HasFlag(GateFlags::HAS_ROOT);
    }

    bool HasFlag(GateFlags flag) const
    {
        return (GetFlags() & flag) == flag;
    }

    GateFlags GetFlags() const
    {
        return flags_;
    }

private:
    friend class Gate;
    friend class Circuit;
    friend class GateMetaBuilder;

    OpCode opcode_ { OpCode::NOP };
    Kind kind_ { Kind::IMMUTABLE };
    GateFlags flags_ { GateFlags::NONE_FLAG };
    uint32_t statesIn_ { 0 };
    uint32_t dependsIn_ { 0 };
    uint32_t valuesIn_ { 0 };
};

inline std::ostream& operator<<(std::ostream& os, OpCode opcode)
{
    return os << GateMetaData::Str(opcode);
}

class BoolMetaData : public GateMetaData {
public:
    BoolMetaData(OpCode opcode, GateFlags flags, uint32_t statesIn,
        uint16_t dependsIn, uint32_t valuesIn, bool value)
        : GateMetaData(opcode, flags, statesIn, dependsIn, valuesIn), value_(value)
    {
        SetKind(GateMetaData::Kind::IMMUTABLE_BOOL);
    }

    static const BoolMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::IMMUTABLE_BOOL);
        return static_cast<const BoolMetaData*>(meta);
    }

    bool GetBool() const
    {
        return value_;
    }

private:
    bool value_ { false };
};

class JSBytecodeMetaData : public GateMetaData {
public:
    explicit JSBytecodeMetaData(size_t valuesIn, EcmaOpcode opcode, uint32_t pcOffset, GateFlags flags)
        : GateMetaData(OpCode::JS_BYTECODE, flags, 1, 1, valuesIn),
        opcode_(opcode), pcOffset_(pcOffset)
    {
        SetKind(GateMetaData::Kind::JSBYTECODE);
    }

    static const JSBytecodeMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::JSBYTECODE);
        return static_cast<const JSBytecodeMetaData*>(meta);
    }

    uint32_t GetPcOffset() const
    {
        return pcOffset_;
    }

    void SetType(PGOSampleType type)
    {
        type_ = type;
    }

    PGOSampleType GetType() const
    {
        return type_;
    }

    EcmaOpcode GetByteCodeOpcode() const
    {
        return opcode_;
    }

    void SetElementsKind(ElementsKind kind)
    {
        elementsKind_ = kind;
        elementsKinds_.emplace_back(kind);
    }

    ElementsKind GetElementsKind() const
    {
        return elementsKind_;
    }

    std::vector<ElementsKind> GetElementsKinds() const
    {
        return elementsKinds_;
    }

private:
    EcmaOpcode opcode_;
    uint32_t pcOffset_;
    PGOSampleType type_;
    ElementsKind elementsKind_ {ElementsKind::GENERIC};
    std::vector<ElementsKind> elementsKinds_ {};
};

class OneParameterMetaData : public GateMetaData {
public:
    OneParameterMetaData(OpCode opcode, GateFlags flags, uint32_t statesIn,
        uint16_t dependsIn, uint32_t valuesIn, uint64_t value)
        : GateMetaData(opcode, flags, statesIn, dependsIn, valuesIn), value_(value)
    {
        SetKind(GateMetaData::Kind::IMMUTABLE_ONE_PARAMETER);
    }

    static const OneParameterMetaData* Cast(const GateMetaData* meta)
    {
        ASSERT(meta->IsOneParameterKind());
        return static_cast<const OneParameterMetaData*>(meta);
    }

    uint64_t GetValue() const
    {
        return value_;
    }

    void SetValue(uint64_t value)
    {
        value_ = value;
    }

private:
    uint64_t value_ { 0 };
};

class TypedCallMetaData : public OneParameterMetaData {
public:
    TypedCallMetaData(OpCode opcode, GateFlags flags, uint32_t statesIn,
        uint16_t dependsIn, uint32_t valuesIn, uint64_t value, bool noGC)
        : OneParameterMetaData(opcode, flags, statesIn, dependsIn, valuesIn, value),
        noGC_(noGC)
    {
        SetKind(GateMetaData::Kind::TYPED_CALL);
    }

    static const TypedCallMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::TYPED_CALL);
        return static_cast<const TypedCallMetaData*>(meta);
    }

    bool IsNoGC() const
    {
        return noGC_;
    }
private:
    bool noGC_;
};

class TypedCallTargetCheckMetaData : public OneParameterMetaData {
public:
    TypedCallTargetCheckMetaData(uint32_t valuesIn, uint64_t value, TypedCallTargetCheckOp checkOp)
        : OneParameterMetaData(OpCode::TYPED_CALLTARGETCHECK_OP, GateFlags::CHECKABLE, 1, 1, valuesIn, value),
        checkOp_(checkOp)
    {
        SetKind(GateMetaData::Kind::TYPED_CALLTARGETCHECK_OP);
    }

    static const TypedCallTargetCheckMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::TYPED_CALLTARGETCHECK_OP);
        return static_cast<const TypedCallTargetCheckMetaData*>(meta);
    }

    TypedCallTargetCheckOp GetTypedCallTargetCheckOp() const
    {
        return checkOp_;
    }
private:
    TypedCallTargetCheckOp checkOp_;
};

class TypedBinaryMetaData : public OneParameterMetaData {
public:
    TypedBinaryMetaData(uint64_t value, TypedBinOp binOp, PGOSampleType type)
        : OneParameterMetaData(OpCode::TYPED_BINARY_OP, GateFlags::NO_WRITE, 1, 1, 2, value), // 2: valuesIn
        binOp_(binOp), type_(type)
    {
        SetKind(GateMetaData::Kind::TYPED_BINARY_OP);
    }

    static const TypedBinaryMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::TYPED_BINARY_OP);
        return static_cast<const TypedBinaryMetaData*>(meta);
    }

    TypedBinOp GetTypedBinaryOp() const
    {
        return binOp_;
    }

    PGOSampleType GetType() const
    {
        return type_;
    }
private:
    TypedBinOp binOp_;
    PGOSampleType type_;
};

class StringMetaData : public GateMetaData {
public:
    StringMetaData(Chunk* chunk, const std::string &str)
        : GateMetaData(OpCode::CONSTSTRING, GateFlags::NONE_FLAG, 0, 0, 0),
        stringData_(str.size() + 1, chunk)
    {
        auto srcLength = str.size();
        auto destlength = stringData_.size();
        auto dest = stringData_.data();
        auto src = str.c_str();
        if (destlength <= static_cast<size_t>(srcLength) || strcpy_s(dest, destlength, src) != EOK) {
            LOG_COMPILER(FATAL) << "StringMetaData strcpy_s failed";
        }
        SetKind(GateMetaData::Kind::MUTABLE_STRING);
    }

    const ChunkVector<char> &GetString() const
    {
        return stringData_;
    }

private:
    ChunkVector<char> stringData_;
};

class GateTypeAccessor {
public:
    explicit GateTypeAccessor(uint64_t value)
        : type_(static_cast<uint32_t>(value)) {}

    GateType GetGateType() const
    {
        return type_;
    }

    static uint64_t ToValue(GateType type)
    {
        return static_cast<uint64_t>(type.Value());
    }
private:
    GateType type_;
};

class ValuePairTypeAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 8;
    explicit ValuePairTypeAccessor(uint64_t value) : bitField_(value) {}

    ValueType GetSrcType() const
    {
        return static_cast<ValueType>(LeftBits::Get(bitField_));
    }

    ValueType GetDstType() const
    {
        return static_cast<ValueType>(RightBits::Get(bitField_));
    }

    bool IsConvertSupport() const
    {
        return ConvertSupportBits::Get(bitField_) == ConvertSupport::ENABLE;
    }

    static uint64_t ToValue(ValueType srcType, ValueType dstType, ConvertSupport support = ConvertSupport::ENABLE)
    {
        uint8_t srcVlaue = static_cast<uint8_t>(srcType);
        uint8_t dstVlaue = static_cast<uint8_t>(dstType);
        return LeftBits::Encode(srcVlaue) | RightBits::Encode(dstVlaue) | ConvertSupportBits::Encode(support);
    }

private:
    using LeftBits = panda::BitField<uint8_t, 0, OPRAND_TYPE_BITS>;
    using RightBits = LeftBits::NextField<uint8_t, OPRAND_TYPE_BITS>;
    using ConvertSupportBits = RightBits::NextField<ConvertSupport, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class GatePairTypeAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit GatePairTypeAccessor(uint64_t value) : bitField_(value) {}

    GateType GetLeftType() const
    {
        return GateType(LeftBits::Get(bitField_));
    }

    GateType GetRightType() const
    {
        return GateType(RightBits::Get(bitField_));
    }

    static uint64_t ToValue(GateType leftType, GateType rightType)
    {
        return LeftBits::Encode(leftType.Value()) | RightBits::Encode(rightType.Value());
    }

private:
    using LeftBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using RightBits = LeftBits::NextField<uint32_t, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class TypedUnaryAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit TypedUnaryAccessor(uint64_t value) : bitField_(value) {}

    GateType GetTypeValue() const
    {
        return GateType(TypedValueBits::Get(bitField_));
    }

    TypedUnOp GetTypedUnOp() const
    {
        return TypedUnOpBits::Get(bitField_);
    }

    static uint64_t ToValue(GateType typeValue, TypedUnOp unaryOp)
    {
        return TypedValueBits::Encode(typeValue.Value())
            | TypedUnOpBits::Encode(unaryOp);
    }

private:
    using TypedValueBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using TypedUnOpBits = TypedValueBits::NextField<TypedUnOp, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class BranchAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit BranchAccessor(uint64_t value) : bitField_(value) {}

    int32_t GetTrueWeight() const
    {
        return TrueWeightBits::Get(bitField_);
    }

    int32_t GetFalseWeight() const
    {
        return FalseWeightBits::Get(bitField_);
    }

    static uint64_t ToValue(uint32_t trueWeight, uint32_t falseWeight)
    {
        return TrueWeightBits::Encode(trueWeight)
            | FalseWeightBits::Encode(falseWeight);
    }
private:
    using TrueWeightBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using FalseWeightBits = TrueWeightBits::NextField<uint32_t, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class TypedJumpAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    static constexpr int JUMP_OP_BITS = 8;
    static constexpr int BRANCH_KIND_BITS = 8;
    explicit TypedJumpAccessor(uint64_t value) : bitField_(value) {}

    GateType GetTypeValue() const
    {
        return GateType(TypedValueBits::Get(bitField_));
    }

    TypedJumpOp GetTypedJumpOp() const
    {
        return TypedJumpOpBits::Get(bitField_);
    }

    BranchKind GetBranchKind() const
    {
        return BranchKindBits::Get(bitField_);
    }

    static uint64_t ToValue(GateType typeValue, TypedJumpOp jumpOp, BranchKind branchKind)
    {
        return TypedValueBits::Encode(typeValue.Value())
            | TypedJumpOpBits::Encode(jumpOp)
            | BranchKindBits::Encode(branchKind);
    }

private:
    using TypedValueBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using TypedJumpOpBits = TypedValueBits::NextField<TypedJumpOp, JUMP_OP_BITS>;
    using BranchKindBits = TypedJumpOpBits::NextField<BranchKind, BRANCH_KIND_BITS>;

    uint64_t bitField_;
};

class FrameStateOutput {
public:
    static constexpr uint32_t INVALID_INDEX = static_cast<uint32_t>(-1);
    explicit FrameStateOutput(uint32_t value) : index_(value) {}

    static FrameStateOutput Invalid()
    {
        return FrameStateOutput(INVALID_INDEX);
    }

    bool IsInvalid() const
    {
        return index_ == INVALID_INDEX;
    }

    uint32_t GetValue() const
    {
        return index_;
    }
private:
    uint32_t index_;
};

class UInt32PairAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit UInt32PairAccessor(uint64_t value) : bitField_(value) {}
    explicit UInt32PairAccessor(uint32_t first, uint32_t second)
    {
        bitField_ = FirstBits::Encode(first) | SecondBits::Encode(second);
    }

    uint32_t GetFirstValue() const
    {
        return FirstBits::Get(bitField_);
    }

    uint32_t GetSecondValue() const
    {
        return SecondBits::Get(bitField_);
    }

    uint64_t ToValue() const
    {
        return bitField_;
    }

private:
    using FirstBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using SecondBits = FirstBits::NextField<uint32_t, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class ArrayMetaDataAccessor {
public:
    enum Mode : uint8_t {
        CREATE = 0,
        LOAD_ELEMENT,
        STORE_ELEMENT,
        LOAD_LENGTH
    };

    static constexpr int BITS_SIZE = 8;
    static constexpr int ARRAY_LENGTH_BITS_SIZE = 32;
    explicit ArrayMetaDataAccessor(uint64_t value) : bitField_(value) {}
    explicit ArrayMetaDataAccessor(ElementsKind kind, Mode mode, uint32_t length = 0)
    {
        bitField_ = ElementsKindBits::Encode(kind) | ModeBits::Encode(mode) | ArrayLengthBits::Encode(length);
    }

    ElementsKind GetElementsKind() const
    {
        return ElementsKindBits::Get(bitField_);
    }

    void SetArrayLength(uint32_t length)
    {
        bitField_ = ArrayLengthBits::Update(bitField_, length);
    }

    uint32_t GetArrayLength() const
    {
        return ArrayLengthBits::Get(bitField_);
    }

    bool IsLoadElement() const
    {
        return GetMode() == Mode::LOAD_ELEMENT;
    }

    uint64_t ToValue() const
    {
        return bitField_;
    }

private:
    Mode GetMode() const
    {
        return ModeBits::Get(bitField_);
    }

    using ElementsKindBits = panda::BitField<ElementsKind, 0, BITS_SIZE>;
    using ModeBits = ElementsKindBits::NextField<Mode, BITS_SIZE>;
    using ArrayLengthBits = ModeBits::NextField<uint32_t, ARRAY_LENGTH_BITS_SIZE>;

    uint64_t bitField_;
};

class ObjectTypeAccessor {
public:
    static constexpr int TYPE_BITS_SIZE = 32;
    static constexpr int IS_HEAP_OBJECT_BIT_SIZE = 1;

    explicit ObjectTypeAccessor(uint64_t value) : bitField_(value) {}
    explicit ObjectTypeAccessor(GateType type, bool isHeapObject = false)
    {
        bitField_ = TypeBits::Encode(type.Value()) | IsHeapObjectBit::Encode(isHeapObject);
    }

    GateType GetType() const
    {
        return GateType(TypeBits::Get(bitField_));
    }

    bool IsHeapObject() const
    {
        return IsHeapObjectBit::Get(bitField_);
    }

    uint64_t ToValue() const
    {
        return bitField_;
    }

private:
    using TypeBits = panda::BitField<uint32_t, 0, TYPE_BITS_SIZE>;
    using IsHeapObjectBit = TypeBits::NextField<bool, IS_HEAP_OBJECT_BIT_SIZE>;

    uint64_t bitField_;
};
} // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_GATE_META_DATA_H
