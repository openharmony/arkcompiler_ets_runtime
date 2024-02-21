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

#ifndef ECMASCRIPT_COMPILER_MCR_OPCODE_H
#define ECMASCRIPT_COMPILER_MCR_OPCODE_H

namespace panda::ecmascript::kungfu {

#define MCR_BINARY_GATE_META_DATA_CACHE_LIST(V)                                                      \
    V(Int32CheckRightIsZero, INT32_CHECK_RIGHT_IS_ZERO, GateFlags::CHECKABLE, 1, 1, 1)               \
    V(RemainderIsNegativeZero, REMAINDER_IS_NEGATIVE_ZERO, GateFlags::CHECKABLE, 1, 1, 2)            \
    V(Float64CheckRightIsZero, FLOAT64_CHECK_RIGHT_IS_ZERO, GateFlags::CHECKABLE, 1, 1, 1)           \
    V(ValueCheckNegOverflow, VALUE_CHECK_NEG_OVERFLOW, GateFlags::CHECKABLE, 1, 1, 1)                \
    V(OverflowCheck, OVERFLOW_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                  \
    V(Int32UnsignedUpperBoundCheck, INT32_UNSIGNED_UPPER_BOUND_CHECK, GateFlags::CHECKABLE, 1, 1, 2) \
    V(Int32DivWithCheck, INT32_DIV_WITH_CHECK, GateFlags::CHECKABLE, 1, 1, 2)                        \
    V(LexVarIsHoleCheck, LEX_VAR_IS_HOLE_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                       \
    V(TaggedIsHeapObject, TAGGED_IS_HEAP_OBJECT, GateFlags::NO_WRITE, 1, 1, 1)                       \
    V(IsMarkerCellValid, IS_MARKER_CELL_VALID, GateFlags::NO_WRITE, 1, 1, 1)                         \
    V(StringEqual, STRING_EQUAL, GateFlags::NO_WRITE, 1, 1, 2)

#define MCR_IMMUTABLE_META_DATA_CACHE_LIST(V)                                                   \
    V(ArrayGuardianCheck, ARRAY_GUARDIAN_CHECK, GateFlags::CHECKABLE, 1, 1, 0)                  \
    V(COWArrayCheck, COW_ARRAY_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                            \
    V(ConvertHoleAsUndefined, CONVERT_HOLE_AS_UNDEFINED, GateFlags::NO_WRITE, 1, 1, 1)          \
    V(EcmaStringCheck, ECMA_STRING_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                        \
    V(FinishAllocate, FINISH_ALLOCATE, GateFlags::NONE_FLAG, 0, 1, 1)                           \
    V(FlattenTreeStringCheck, FLATTEN_TREE_STRING_CHECK, GateFlags::CHECKABLE, 1, 1, 1)         \
    V(HeapObjectCheck, HEAP_OBJECT_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                        \
    V(ProtoChangeMarkerCheck, PROTO_CHANGE_MARKER_CHECK, GateFlags::CHECKABLE, 1, 1, 1)         \
    V(LookUpHolder, LOOK_UP_HOLDER, GateFlags::NO_WRITE, 1, 1, 3)                               \
    V(LoadGetter, LOAD_GETTER, GateFlags::CHECKABLE, 0, 1, 2)                                   \
    V(LoadSetter, LOAD_SETTER, GateFlags::CHECKABLE, 0, 1, 2)                                   \
    V(LoadArrayLength, LOAD_ARRAY_LENGTH, GateFlags::NO_WRITE, 1, 1, 1)                         \
    V(LoadStringLength, LOAD_STRING_LENGTH, GateFlags::NO_WRITE, 1, 1, 1)                       \
    V(StartAllocate, START_ALLOCATE, GateFlags::NONE_FLAG, 0, 1, 0)                             \
    V(StorePropertyNoBarrier, STORE_PROPERTY_NO_BARRIER, GateFlags::NONE_FLAG, 1, 1, 3)         \
    V(TypedCallCheck, TYPED_CALL_CHECK, GateFlags::CHECKABLE, 1, 1, 3)                          \
    V(TypedNewAllocateThis, TYPED_NEW_ALLOCATE_THIS, GateFlags::CHECKABLE, 1, 1, 2)             \
    V(TypedSuperAllocateThis, TYPED_SUPER_ALLOCATE_THIS, GateFlags::CHECKABLE, 1, 1, 2)         \
    V(ArrayConstructorCheck, ARRAY_CONSTRUCTOR_CHECK, GateFlags::CHECKABLE, 1, 1, 1)            \
    V(ObjectConstructorCheck, OBJECT_CONSTRUCTOR_CHECK, GateFlags::CHECKABLE, 1, 1, 1)          \
    V(IndexCheck, INDEX_CHECK, GateFlags::CHECKABLE, 1, 1, 2)                                   \
    V(MonoLoadPropertyOnProto, MONO_LOAD_PROPERTY_ON_PROTO, GateFlags::CHECKABLE, 1, 1, 4)      \
    V(StringFromSingleCharCode, STRING_FROM_SINGLE_CHAR_CODE, GateFlags::NO_WRITE, 1, 1, 1)     \
    MCR_BINARY_GATE_META_DATA_CACHE_LIST(V)

#define MCR_GATE_META_DATA_LIST_WITH_PC_OFFSET(V)                              \
    V(TypedCallBuiltin, TYPED_CALL_BUILTIN, GateFlags::CHECKABLE, 1, 1, value)

#define MCR_GATE_META_DATA_LIST_FOR_CALL(V)                                    \
    V(TypedCall, TYPEDCALL, GateFlags::HAS_FRAME_STATE, 1, 1, value)           \
    V(TypedFastCall, TYPEDFASTCALL, GateFlags::HAS_FRAME_STATE, 1, 1, value)

#define MCR_GATE_META_DATA_LIST_WITH_VALUE(V)                                                           \
    V(LoadConstOffset,             LOAD_CONST_OFFSET,              GateFlags::NO_WRITE,  0, 1, 1)       \
    V(LoadHClassFromConstpool,     LOAD_HCLASS_FROM_CONSTPOOL,     GateFlags::NO_WRITE,  0, 1, 1)       \
    V(StoreConstOffset,            STORE_CONST_OFFSET,             GateFlags::NONE_FLAG, 1, 1, 2)       \
    V(LoadElement,                 LOAD_ELEMENT,                   GateFlags::NO_WRITE,  1, 1, 2)       \
    V(StoreElement,                STORE_ELEMENT,                  GateFlags::NONE_FLAG, 1, 1, 3)       \
    V(StoreMemory,                 STORE_MEMORY,                   GateFlags::NONE_FLAG, 1, 1, 3)       \
    V(ObjectTypeCompare,           OBJECT_TYPE_COMPARE,            GateFlags::CHECKABLE, 1, 1, 2)       \
    V(ObjectTypeCheck,             OBJECT_TYPE_CHECK,              GateFlags::CHECKABLE, 1, 1, 2)       \
    V(StableArrayCheck,            STABLE_ARRAY_CHECK,             GateFlags::CHECKABLE, 1, 1, 1)       \
    V(StoreProperty,               STORE_PROPERTY,                 GateFlags::NONE_FLAG, 1, 1, 3)       \
    V(PrototypeCheck,              PROTOTYPE_CHECK,                GateFlags::CHECKABLE, 1, 1, 1)       \
    V(RangeGuard,                  RANGE_GUARD,                    GateFlags::NO_WRITE,  1, 1, 1)       \
    V(GetGlobalEnvObj,             GET_GLOBAL_ENV_OBJ,             GateFlags::NO_WRITE,  0, 1, 1)       \
    V(GetGlobalEnvObjHClass,       GET_GLOBAL_ENV_OBJ_HCLASS,      GateFlags::NO_WRITE,  0, 1, 1)       \
    V(GetGlobalConstantValue,      GET_GLOBAL_CONSTANT_VALUE,      GateFlags::NO_WRITE,  0, 1, 0)       \
    V(HClassStableArrayCheck,      HCLASS_STABLE_ARRAY_CHECK,      GateFlags::CHECKABLE, 1, 1, 1)       \
    V(HeapAlloc,                   HEAP_ALLOC,                     GateFlags::NONE_FLAG, 1, 1, 1)       \
    V(RangeCheckPredicate,         RANGE_CHECK_PREDICATE,          GateFlags::CHECKABLE, 1, 1, 2)       \
    V(BuiltinPrototypeHClassCheck, BUILTIN_PROTOTYPE_HCLASS_CHECK, GateFlags::CHECKABLE, 1, 1, 1)       \
    V(IsSpecificObjectType,        IS_SPECIFIC_OBJECT_TYPE,        GateFlags::NO_WRITE,  1, 1, 1)       \
    V(LoadBuiltinObject,           LOAD_BUILTIN_OBJECT,            GateFlags::CHECKABLE, 1, 1, 0)       \
    V(StringAdd,                   STRING_ADD,                     GateFlags::NO_WRITE,  1, 1, 2)

#define MCR_GATE_META_DATA_LIST_WITH_BOOL(V)                                                                 \
    V(LoadProperty, LOAD_PROPERTY, GateFlags::NO_WRITE, 1, 1, 2)                                             \
    V(MonoStorePropertyLookUpProto, MONO_STORE_PROPERTY_LOOK_UP_PROTO, GateFlags::HAS_FRAME_STATE, 1, 1, 5)  \
    V(MonoStoreProperty, MONO_STORE_PROPERTY, GateFlags::NONE_FLAG, 1, 1, 6)

#define MCR_GATE_META_DATA_LIST_WITH_GATE_TYPE(V)                                          \
    V(PrimitiveTypeCheck, PRIMITIVE_TYPE_CHECK, GateFlags::CHECKABLE, 1, 1, 1)             \
    V(TypedArrayCheck, TYPED_ARRAY_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                   \
    V(LoadTypedArrayLength, LOAD_TYPED_ARRAY_LENGTH, GateFlags::NO_WRITE, 1, 1, 1)         \
    V(TypedUnaryOp, TYPED_UNARY_OP, GateFlags::NO_WRITE, 1, 1, 1)                          \
    V(TypedConditionJump, TYPED_CONDITION_JUMP, GateFlags::NO_WRITE, 1, 1, 1)              \
    V(TypedConvert, TYPE_CONVERT, GateFlags::NO_WRITE, 1, 1, 1)                            \
    V(CheckAndConvert, CHECK_AND_CONVERT, GateFlags::CHECKABLE, 1, 1, 1)                   \
    V(Convert, CONVERT, GateFlags::NONE_FLAG, 0, 0, 1)                                     \
    V(JSInlineTargetTypeCheck, JSINLINETARGET_TYPE_CHECK, GateFlags::CHECKABLE, 1, 1, 2)   \
    V(TypeOfCheck, TYPE_OF_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                           \
    V(TypeOf, TYPE_OF, GateFlags::NO_WRITE, 1, 1, 0)

#define MCR_GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)         \
    MCR_GATE_META_DATA_LIST_WITH_VALUE(V)                     \
    MCR_GATE_META_DATA_LIST_WITH_GATE_TYPE(V)

#define MCR_GATE_OPCODE_LIST(V)     \
    V(TYPED_BINARY_OP)              \
    V(TYPED_CALLTARGETCHECK_OP)

}

#define MCR_GATE_META_DATA_LIST_WITH_VALUE_IN(V)                                                 \
    V(TypedCreateObjWithBuffer, TYPED_CREATE_OBJ_WITH_BUFFER, GateFlags::NONE_FLAG, 1, 1, value)

#define MCR_GATE_META_DATA_LIST_WITH_SIZE(V)                                       \
    MCR_GATE_META_DATA_LIST_WITH_VALUE_IN(V)
#endif  // ECMASCRIPT_COMPILER_MCR_OPCODE_H
