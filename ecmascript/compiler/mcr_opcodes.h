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

#define MCR_BINARY_GATE_META_DATA_CACHE_LIST(V)                                                             \
    V(Int32CheckRightIsZero, INT32_CHECK_RIGHT_IS_ZERO, GateFlags::CHECKABLE, 1, 1, 1)                      \
    V(RemainderIsNegativeZero, REMAINDER_IS_NEGATIVE_ZERO, GateFlags::CHECKABLE, 1, 1, 2)                   \
    V(Float64CheckRightIsZero, FLOAT64_CHECK_RIGHT_IS_ZERO, GateFlags::CHECKABLE, 1, 1, 1)                  \
    V(ValueCheckNegOverflow, VALUE_CHECK_NEG_OVERFLOW, GateFlags::CHECKABLE, 1, 1, 1)                       \
    V(OverflowCheck, OVERFLOW_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                         \
    V(Int32UnsignedUpperBoundCheck, INT32_UNSIGNED_UPPER_BOUND_CHECK, GateFlags::CHECKABLE, 1, 1, 2)        \
    V(Int32DivWithCheck, INT32_DIV_WITH_CHECK, GateFlags::CHECKABLE, 1, 1, 2)                               \
    V(LexVarIsHoleCheck, LEX_VAR_IS_HOLE_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                              \
    V(IsUndefinedOrHoleCheck, IS_UNDEFINED_OR_HOLE_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                    \
    V(IsNotUndefinedOrHoleCheck, IS_NOT_UNDEFINED_OR_HOLE_CHECK, GateFlags::CHECKABLE, 1, 1, 1)             \
    V(IsDataViewCheck, IS_DATA_VIEW_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                   \
    V(TaggedIsHeapObject, TAGGED_IS_HEAP_OBJECT, GateFlags::NO_WRITE, 1, 1, 1)                              \
    V(IsMarkerCellValid, IS_MARKER_CELL_VALID, GateFlags::NO_WRITE, 1, 1, 1)                                \
    V(StringEqual, STRING_EQUAL, GateFlags::NO_WRITE, 1, 1, 2)                                              \
    V(IsCallableCheck, IS_CALLABLE_CHECK, GateFlags::CHECKABLE, 1, 1, 1)

#define MCR_IMMUTABLE_META_DATA_CACHE_LIST(V)                                                                      \
    V(ArrayGuardianCheck, ARRAY_GUARDIAN_CHECK, GateFlags::CHECKABLE, 1, 1, 0)                                     \
    V(COWArrayCheck, COW_ARRAY_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                               \
    V(ConvertHoleAsUndefined, CONVERT_HOLE_AS_UNDEFINED, GateFlags::NO_WRITE, 1, 1, 1)                             \
    V(EcmaStringCheck, ECMA_STRING_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                           \
    V(StringKeyCheck, STRING_KEY_CHECK, GateFlags::CHECKABLE, 1, 1, 2)                                             \
    V(InternStringKeyCheck, INTERN_STRING_KEY_CHECK, GateFlags::CHECKABLE, 1, 1, 2)                                \
    V(InternStringCheck, INTERN_STRING_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                       \
    V(EcmaMapCheck, ECMA_MAP_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                                 \
    V(FinishAllocate, FINISH_ALLOCATE, GateFlags::NONE_FLAG, 0, 1, 1)                                              \
    V(FlattenTreeStringCheck, FLATTEN_TREE_STRING_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                            \
    V(HeapObjectCheck, HEAP_OBJECT_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                           \
    V(EcmaObjectCheck, ECMA_OBJECT_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                           \
    V(MathHClassConsistencyCheck, MATH_HCLASS_CONSISTENCY_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                    \
    V(ProtoChangeMarkerCheck, PROTO_CHANGE_MARKER_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                            \
    V(PrimTypeProtoChangeMarkerCheck, PRIMTYPE_PROTO_CHANGE_MARKER_CHECK, GateFlags::CHECKABLE, 1, 1, 1)           \
    V(LookUpHolder, LOOK_UP_HOLDER, GateFlags::CHECKABLE, 1, 1, 3)                                                 \
    V(LoadGetter, LOAD_GETTER, GateFlags::NO_WRITE, 0, 1, 2)                                                       \
    V(LoadSetter, LOAD_SETTER, GateFlags::NO_WRITE, 0, 1, 2)                                                       \
    V(LoadStringLength, LOAD_STRING_LENGTH, GateFlags::NO_WRITE, 1, 1, 1)                                          \
    V(LoadMapSize, LOAD_MAP_SIZE, GateFlags::NO_WRITE, 1, 1, 1)                                                    \
    V(StartAllocate, START_ALLOCATE, GateFlags::NONE_FLAG, 0, 1, 0)                                                \
    V(StorePropertyNoBarrier, STORE_PROPERTY_NO_BARRIER, GateFlags::NONE_FLAG, 1, 1, 3)                            \
    V(TypedNewAllocateThis, TYPED_NEW_ALLOCATE_THIS, GateFlags::CHECKABLE, 1, 1, 3)                                \
    V(TypedSuperAllocateThis, TYPED_SUPER_ALLOCATE_THIS, GateFlags::NO_WRITE, 1, 1, 2)                             \
    V(ArrayConstructorCheck, ARRAY_CONSTRUCTOR_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                               \
    V(ObjectConstructorCheck, OBJECT_CONSTRUCTOR_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                             \
    V(BooleanConstructorCheck, BOOLEAN_CONSTRUCTOR_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                           \
    V(IndexCheck, INDEX_CHECK, GateFlags::CHECKABLE, 1, 1, 2)                                                      \
    V(MonoLoadPropertyOnProto, MONO_LOAD_PROPERTY_ON_PROTO, GateFlags::CHECKABLE, 1, 1, 4)                         \
    V(StringFromSingleCharCode, STRING_FROM_SINGLE_CHAR_CODE, GateFlags::NO_WRITE, 1, 1, 1)                        \
    V(StringCharCodeAt, STRING_CHAR_CODE_AT, GateFlags::NO_WRITE, 1, 1, 2)                                         \
    V(StringSubStr, STRING_SUB_STR, GateFlags::NO_WRITE, 1, 1, 3)                                                  \
    V(TypedArrayEntries, TYPED_ARRAY_ENTRIES, GateFlags::NO_WRITE, 1, 1, 1)                                        \
    V(TypedArrayKeys, TYPED_ARRAY_KEYS, GateFlags::NO_WRITE, 1, 1, 1)                                              \
    V(TypedArrayValues, TYPED_ARRAY_VALUES, GateFlags::NO_WRITE, 1, 1, 1)                                          \
    V(Float32ArrayConstructorCheck, FLOAT32_ARRAY_CONSTRUCTOR_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                \
    V(MigrateFromRawValueToHeapValues, MIGRATE_FROM_RAWVALUE_TO_HEAPVALUES, GateFlags::NONE_FLAG, 1, 1, 3)         \
    V(MigrateFromHeapValueToRawValue, MIGRATE_FROM_HEAPVALUE_TO_RAWVALUE, GateFlags::NONE_FLAG, 1, 1, 3)           \
    V(MigrateFromHoleIntToHoleNumber, MIGRATE_FROM_HOLEINT_TO_HOLENUMBER, GateFlags::NONE_FLAG, 1, 1, 1)           \
    V(MigrateFromHoleNumberToHoleInt, MIGRATE_FROM_HOLENUMBER_TO_HOLEINT, GateFlags::NONE_FLAG, 1, 1, 1)           \
    V(NumberIsFinite, NUMBER_IS_FINITE, GateFlags::NO_WRITE, 1, 1, 1)                                              \
    V(NumberIsInteger, NUMBER_IS_INTEGER, GateFlags::NO_WRITE, 1, 1, 1)                                            \
    V(NumberIsNaN, NUMBER_IS_NAN, GateFlags::NO_WRITE, 1, 1, 1)                                                    \
    V(NumberIsSafeInteger, NUMBER_IS_SAFEINTEGER, GateFlags::NO_WRITE, 1, 1, 1)                                    \
    V(NumberParseFloat, NUMBER_PARSE_FLOAT, GateFlags::CHECKABLE, 1, 1, 1)                                         \
    V(NumberParseInt, NUMBER_PARSE_INT, GateFlags::NO_WRITE, 1, 1, 2)                                              \
    V(MathAcos, MATH_ACOS, GateFlags::NO_WRITE, 1, 1, 1)                                                           \
    V(MathAcosh, MATH_ACOSH, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(MathAsin, MATH_ASIN, GateFlags::NO_WRITE, 1, 1, 1)                                                           \
    V(MathAsinh, MATH_ASINH, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(MathAtan, MATH_ATAN, GateFlags::NO_WRITE, 1, 1, 1)                                                           \
    V(MathAtan2, MATH_ATAN2, GateFlags::NO_WRITE, 1, 1, 2)                                                         \
    V(MathAtanh, MATH_ATANH, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(MathCos, MATH_COS, GateFlags::NO_WRITE, 1, 1, 1)                                                             \
    V(MathCosh, MATH_COSH, GateFlags::NO_WRITE, 1, 1, 1)                                                           \
    V(MathSign, MATH_SIGN, GateFlags::NO_WRITE, 1, 1, 1)                                                           \
    V(MathSignTagged, MATH_SIGN_TAGGED, GateFlags::NO_WRITE, 1, 1, 1)                                              \
    V(MathSin, MATH_SIN, GateFlags::NO_WRITE, 1, 1, 1)                                                             \
    V(MathSinh, MATH_SINH, GateFlags::NO_WRITE, 1, 1, 1)                                                           \
    V(MathSqrt, MATH_SQRT, GateFlags::NO_WRITE, 1, 1, 1)                                                           \
    V(MathTan, MATH_TAN, GateFlags::NO_WRITE, 1, 1, 1)                                                             \
    V(MathTanh, MATH_TANH, GateFlags::NO_WRITE, 1, 1, 1)                                                           \
    V(MathTrunc, MATH_TRUNC, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(MathLog, MATH_LOG, GateFlags::NO_WRITE, 1, 1, 1)                                                             \
    V(MathLog2, MATH_LOG2, GateFlags::NO_WRITE, 1, 1, 1)                                                           \
    V(MathLog10, MATH_LOG10, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(MathLog1p, MATH_LOG1P, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(MathExp, MATH_EXP, GateFlags::NO_WRITE, 1, 1, 1)                                                             \
    V(MathExpm1, MATH_EXPM1, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(MathClz32, MATH_CLZ32, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(MathClz32Double, MATH_CLZ32_DOUBLE, GateFlags::NO_WRITE, 1, 1, 1)                                            \
    V(MathClz32Int32, MATH_CLZ32_INT32, GateFlags::NO_WRITE, 1, 1, 1)                                              \
    V(MathAbs, MATH_ABS, GateFlags::CHECKABLE, 1, 1, 1)                                                            \
    V(MathAbsInt32, MATH_ABS_INT32, GateFlags::CHECKABLE, 1, 1, 1)                                                 \
    V(MathAbsDouble, MATH_ABS_DOUBLE, GateFlags::CHECKABLE, 1, 1, 1)                                               \
    V(MathPow, MATH_POW, GateFlags::NO_WRITE, 1, 1, 2)                                                             \
    V(MathCbrt, MATH_CBRT, GateFlags::NO_WRITE, 1, 1, 1)                                                           \
    V(MathMin, MATH_MIN, GateFlags::NO_WRITE, 1, 1, 2)                                                             \
    V(MathMinInt32, MATH_MIN_INT32, GateFlags::NO_WRITE, 1, 1, 2)                                                  \
    V(MathMinDouble, MATH_MIN_DOUBLE, GateFlags::NO_WRITE, 1, 1, 2)                                                \
    V(MathMax, MATH_MAX, GateFlags::NO_WRITE, 1, 1, 2)                                                             \
    V(MathMaxInt32, MATH_MAX_INT32, GateFlags::NO_WRITE, 1, 1, 2)                                                  \
    V(MathMaxDouble, MATH_MAX_DOUBLE, GateFlags::NO_WRITE, 1, 1, 2)                                                \
    V(MathRound, MATH_ROUND, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(MathRoundDouble, MATH_ROUND_DOUBLE, GateFlags::NO_WRITE, 1, 1, 1)                                            \
    V(MathFRound, MATH_FROUND, GateFlags::NO_WRITE, 1, 1, 1)                                                       \
    V(MathCeil, MATH_CEIL, GateFlags::NO_WRITE, 1, 1, 1)                                                           \
    V(MathFloor, MATH_FLOOR, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(MathImul, MATH_IMUL, GateFlags::NO_WRITE, 1, 1, 2)                                                           \
    V(NewNumber, NEW_NUMBER, GateFlags::CHECKABLE, 1, 1, 2)                                                        \
    V(GlobalIsFinite, GLOBAL_IS_FINITE, GateFlags::NO_WRITE, 1, 1, 1)                                              \
    V(GlobalIsNan, GLOBAL_IS_NAN, GateFlags::NO_WRITE, 1, 1, 1)                                                    \
    V(BigIntConstructor, BIGINT_CONSTRUCTOR, GateFlags::NO_WRITE, 1, 1, 1)                                         \
    V(BigIntConstructorInt32, BIGINT_CONSTRUCTOR_INT32, GateFlags::NO_WRITE, 1, 1, 1)                              \
    V(BigIntConstructorUint32, BIGINT_CONSTRUCTOR_UINT32, GateFlags::NO_WRITE, 1, 1, 1)                            \
    V(ArrayBufferIsView, ARRAY_BUFFER_IS_VIEW, GateFlags::NO_WRITE, 1, 1, 1)                                       \
    V(DataViewGet, DATA_VIEW_GET, GateFlags::NO_WRITE, 1, 1, 5)                                                    \
    V(DataViewSet, DATA_VIEW_SET, GateFlags::NONE_FLAG, 1, 1, 6)                                                   \
    V(BigIntAsIntN, BIGINT_ASINTN, GateFlags::NO_WRITE, 1, 1, 3)                                                   \
    V(BigIntAsUintN, BIGINT_ASUINTN, GateFlags::NO_WRITE, 1, 1, 3)                                                 \
    V(MapGet, MAP_GET, GateFlags::NO_WRITE, 1, 1, 2)                                                               \
    V(DateGetTime, DATE_GET_TIME, GateFlags::NO_WRITE, 1, 1, 1)                                                    \
    V(MapHas, MAP_HAS, GateFlags::NO_WRITE, 1, 1, 2)                                                               \
    V(MapKeys, MAP_KEYS, GateFlags::NO_WRITE, 1, 1, 1)                                                             \
    V(MapValues, MAP_VALUES, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(MapEntries, MAP_ENTRIES, GateFlags::NO_WRITE, 1, 1, 1)                                                       \
    V(SetHas, SET_HAS, GateFlags::NO_WRITE, 1, 1, 2)                                                               \
    V(SetAdd, SET_ADD, GateFlags::NONE_FLAG, 1, 1, 2)                                                              \
    V(MapDelete, MAP_DELETE, GateFlags::NONE_FLAG, 1, 1, 2)                                                        \
    V(SetDelete, SET_DELETE, GateFlags::NONE_FLAG, 1, 1, 2)                                                        \
    V(DateNow, DATE_NOW, GateFlags::NONE_FLAG, 1, 1, 0)                                                            \
    V(SetValues, SET_VALUES, GateFlags::NO_WRITE, 1, 1, 1)                                                         \
    V(SetEntries, SET_ENTRIES, GateFlags::NO_WRITE, 1, 1, 1)                                                       \
    V(ObjectIs, OBJECT_IS, GateFlags::NO_WRITE, 1, 1, 2)                                                           \
    V(ObjectGetPrototypeOf, OBJECT_GET_PROTOTYPE_OF, GateFlags::NO_WRITE, 1, 1, 1)                                 \
    V(ObjectCreate, OBJECT_CREATE, GateFlags::NONE_FLAG, 1, 1, 1)                                                  \
    V(ObjectIsPrototypeOf, OBJECT_IS_PROTOTYPE_OF, GateFlags::NO_WRITE, 1, 1, 2)                                   \
    V(ObjectHasOwnProperty, OBJECT_HAS_OWN_PROPERTY, GateFlags::NO_WRITE, 1, 1, 2)                                 \
    V(ReflectGetPrototypeOf, REFLECT_GET_PROTOTYPE_OF, GateFlags::NO_WRITE, 1, 1, 1)                               \
    V(ReflectGet, REFLECT_GET, GateFlags::NONE_FLAG, 1, 1, 2)                                                      \
    V(ReflectHas, REFLECT_HAS, GateFlags::NO_WRITE, 1, 1, 2)                                                       \
    V(ReflectConstruct, REFLECT_CONSTRUCT, GateFlags::NONE_FLAG, 1, 1, 1)                                          \
    V(ReflectApply, REFLECT_APPLY, GateFlags::NONE_FLAG, 1, 1, 3)                                                  \
    V(FunctionPrototypeApply, FUNCTION_PROTOTYPE_APPLY, GateFlags::NONE_FLAG, 1, 1, 3)                             \
    V(FunctionPrototypeBind, FUNCTION_PROTOTYPE_BIND, GateFlags::NONE_FLAG, 1, 1, 2)                               \
    V(HeapObjectIsEcmaObjectCheck, HEAP_OBJECT_IS_ECMA_OBJECT, GateFlags::CHECKABLE, 1, 1, 1)                      \
    V(CallTargetIsCompiledCheck, CALL_TARGET_IS_COMPILED_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                     \
    V(ArrayIncludesIndexOf, ARRAY_INCLUDES_INDEXOF, GateFlags::NO_WRITE, 1, 1, 6)                                  \
    V(ArrayIteratorBuiltin, ARRAY_ITERATOR_BUILTIN, GateFlags::NONE_FLAG, 1, 1, 2)                                 \
    V(ArrayPop, ARRAY_POP, GateFlags::NONE_FLAG, 1, 1, 2)                                                          \
    V(ArrayPush, ARRAY_PUSH, GateFlags::NONE_FLAG, 1, 1, 2)                                                        \
    V(ArraySlice, ARRAY_SLICE, GateFlags::NONE_FLAG, 1, 1, 4)                                                      \
    V(ArraySort, ARRAY_SORT, GateFlags::NONE_FLAG, 1, 1, 2)                                                        \
    V(InlineSuperCtorCheck, INLINE_SUPER_CTOR_CHECK, GateFlags::CHECKABLE, 1, 1, 3)                                \
    V(CheckConstructor, CHECK_CONSTRUCTOR, GateFlags::NO_WRITE, 1, 1, 3)                                           \
    MCR_BINARY_GATE_META_DATA_CACHE_LIST(V)

#define MCR_GATE_META_DATA_LIST_WITH_PC_OFFSET(V)                                                            \
    V(TypedCallBuiltin, TYPED_CALL_BUILTIN, GateFlags::CHECKABLE, 1, 1, value)                               \
    V(TypedCallBuiltinSideEffect, TYPED_CALL_BUILTIN_SIDE_EFFECT, GateFlags::HAS_FRAME_STATE, 1, 1, value)   \
    V(ObjectTypeCheck, OBJECT_TYPE_CHECK, GateFlags::CHECKABLE, 1, 1, value)

#define MCR_GATE_META_DATA_LIST_FOR_CALL(V)                                    \
    V(TypedCall, TYPEDCALL, GateFlags::HAS_FRAME_STATE, 1, 1, value)           \
    V(TypedFastCall, TYPEDFASTCALL, GateFlags::HAS_FRAME_STATE, 1, 1, value)

#define MCR_GATE_META_DATA_LIST_WITH_VALUE(V)                                                           \
    V(LoadConstOffset,             LOAD_CONST_OFFSET,              GateFlags::NO_WRITE,  0, 1, 1)       \
    V(LoadHClassFromConstpool,     LOAD_HCLASS_FROM_CONSTPOOL,     GateFlags::NO_WRITE,  0, 1, 1)       \
    V(StoreConstOffset,            STORE_CONST_OFFSET,             GateFlags::NONE_FLAG, 0, 1, 2)       \
    V(LoadElement,                 LOAD_ELEMENT,                   GateFlags::NO_WRITE,  1, 1, 2)       \
    V(StoreElement,                STORE_ELEMENT,                  GateFlags::NONE_FLAG, 1, 1, 3)       \
    V(StoreMemory,                 STORE_MEMORY,                   GateFlags::NONE_FLAG, 1, 1, 3)       \
    V(StableArrayCheck,            STABLE_ARRAY_CHECK,             GateFlags::CHECKABLE, 1, 1, 1)       \
    V(LoadArrayLength,             LOAD_ARRAY_LENGTH,              GateFlags::NO_WRITE,  1, 1, 1)       \
    V(ElementsKindCheck,           ELEMENTSKIND_CHECK,             GateFlags::CHECKABLE, 1, 1, 1)       \
    V(StoreProperty,               STORE_PROPERTY,                 GateFlags::NONE_FLAG, 1, 1, 3)       \
    V(PrototypeCheck,              PROTOTYPE_CHECK,                GateFlags::CHECKABLE, 1, 1, 1)       \
    V(RangeGuard,                  RANGE_GUARD,                    GateFlags::NO_WRITE,  1, 1, 1)       \
    V(GetGlobalEnvObj,             GET_GLOBAL_ENV_OBJ,             GateFlags::NO_WRITE,  0, 1, 1)       \
    V(GetGlobalEnvObjHClass,       GET_GLOBAL_ENV_OBJ_HCLASS,      GateFlags::NO_WRITE,  0, 1, 1)       \
    V(GetGlobalConstantValue,      GET_GLOBAL_CONSTANT_VALUE,      GateFlags::NO_WRITE,  0, 1, 0)       \
    V(HClassStableArrayCheck,      HCLASS_STABLE_ARRAY_CHECK,      GateFlags::CHECKABLE, 1, 1, 1)       \
    V(HeapAlloc,                   HEAP_ALLOC,                     GateFlags::NONE_FLAG, 0, 1, 2)       \
    V(RangeCheckPredicate,         RANGE_CHECK_PREDICATE,          GateFlags::CHECKABLE, 1, 1, 2)       \
    V(BuiltinPrototypeHClassCheck, BUILTIN_PROTOTYPE_HCLASS_CHECK, GateFlags::CHECKABLE, 1, 1, 1)       \
    V(IsSpecificObjectType,        IS_SPECIFIC_OBJECT_TYPE,        GateFlags::NO_WRITE,  1, 1, 1)       \
    V(LoadBuiltinObject,           LOAD_BUILTIN_OBJECT,            GateFlags::CHECKABLE, 1, 1, 0)       \
    V(StringAdd,                   STRING_ADD,                     GateFlags::NO_WRITE,  1, 1, 2)       \
    V(ArrayForEach,                ARRAY_FOR_EACH,                 GateFlags::NONE_FLAG, 1, 1, 3)       \
    V(ArrayFilter,                 ARRAY_FILTER,                   GateFlags::NONE_FLAG, 1, 1, 4)       \
    V(ArrayFindOrFindIndex,        ARRAY_FIND_OR_FINDINDEX,        GateFlags::NONE_FLAG, 1, 1, 4)       \
    V(ArrayMap,                    ARRAY_MAP,                      GateFlags::NONE_FLAG, 1, 1, 4)       \
    V(ArraySome,                   ARRAY_SOME,                     GateFlags::NONE_FLAG, 1, 1, 3)       \
    V(ArrayEvery,                  ARRAY_EVERY,                    GateFlags::NONE_FLAG, 1, 1, 3)

#define MCR_GATE_META_DATA_LIST_WITH_BOOL(V)                                                                 \
    V(LoadProperty, LOAD_PROPERTY, GateFlags::NO_WRITE, 1, 1, 2)                                             \
    V(MonoStorePropertyLookUpProto, MONO_STORE_PROPERTY_LOOK_UP_PROTO, GateFlags::HAS_FRAME_STATE, 1, 1, 5)  \
    V(MonoStoreProperty, MONO_STORE_PROPERTY, GateFlags::HAS_FRAME_STATE, 1, 1, 7)

#define MCR_GATE_META_DATA_LIST_WITH_GATE_TYPE(V)                                                           \
    V(PrimitiveTypeCheck, PRIMITIVE_TYPE_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                              \
    V(TypedArrayCheck, TYPED_ARRAY_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                    \
    V(LoadTypedArrayLength, LOAD_TYPED_ARRAY_LENGTH, GateFlags::NO_WRITE, 1, 1, 1)                          \
    V(TypedBinaryOp, TYPED_BINARY_OP, GateFlags::NO_WRITE, 1, 1, 2)                                         \
    V(TypedUnaryOp, TYPED_UNARY_OP, GateFlags::NO_WRITE, 1, 1, 1)                                           \
    V(TypedConditionJump, TYPED_CONDITION_JUMP, GateFlags::NO_WRITE, 1, 1, 1)                               \
    V(TypedConvert, TYPE_CONVERT, GateFlags::NO_WRITE, 1, 1, 1)                                             \
    V(CheckAndConvert, CHECK_AND_CONVERT, GateFlags::CHECKABLE, 1, 1, 1)                                    \
    V(Convert, CONVERT, GateFlags::NONE_FLAG, 0, 0, 1)                                                      \
    V(JSInlineTargetTypeCheck, JSINLINETARGET_TYPE_CHECK, GateFlags::CHECKABLE, 1, 1, 2)                    \
    V(JSInlineTargetHeapConstantCheck, JSINLINETARGET_HEAPCONSTANT_CHECK, GateFlags::CHECKABLE, 1, 1, 2)    \
    V(TypeOfCheck, TYPE_OF_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                                            \
    V(TypeOf, TYPE_OF, GateFlags::NO_WRITE, 1, 1, 0)                                                        \
    V(TypedCallTargetCheckOp, TYPED_CALLTARGETCHECK_OP, GateFlags::CHECKABLE, 1, 1, 2)

// NOTICE-PGO: wx typedcalltargetcheckop can adopt different number of valueIn

#define MCR_GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)         \
    MCR_GATE_META_DATA_LIST_WITH_VALUE(V)                     \
    MCR_GATE_META_DATA_LIST_WITH_GATE_TYPE(V)

#define MCR_GATE_META_DATA_LIST_WITH_VALUE_IN(V)                                                                   \
    V(StringSlice, STRING_SLICE, GateFlags::NO_WRITE, 1, 1, value)                                                 \
    V(StringSubstring, STRING_SUB_STRING, GateFlags::NO_WRITE, 1, 1, value)                                        \
    V(TypedCreateObjWithBuffer, TYPED_CREATE_OBJ_WITH_BUFFER, GateFlags::CHECKABLE, 1, 1, value)                   \
    V(TypedCallCheck, TYPED_CALL_CHECK, GateFlags::CHECKABLE, 1, 1, value)                                         \
    V(FunctionPrototypeCall, FUNCTION_PROTOTYPE_CALL, GateFlags::NONE_FLAG, 1, 1, value)

#define MCR_GATE_META_DATA_LIST_WITH_SIZE(V)                                       \
    MCR_GATE_META_DATA_LIST_WITH_VALUE_IN(V)
}

#endif  // ECMASCRIPT_COMPILER_MCR_OPCODE_H
