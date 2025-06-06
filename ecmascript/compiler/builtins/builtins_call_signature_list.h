/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_LIST_H
#define ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_LIST_H

#include "ecmascript/base/config.h"

#define PADDING_BUILTINS_STUB_LIST(V)               \
    V(NONE)

// BUILTINS_STUB_LIST is shared both ASM Interpreter and AOT.
// AOT_BUILTINS_STUB_LIST is used in AOT only.
#define BUILTINS_STUB_LIST(V, D, C)                 \
    BUILTINS_METHOD_STUB_LIST(D, D, D, D)           \
    BUILTINS_CONTAINERS_STUB_BUILDER(D)             \
    AOT_AND_BUILTINS_STUB_LIST(V)                   \
    BUILTINS_CONSTRUCTOR_STUB_LIST(C)               \
    BUILTINS_ARKTOOLS_STUB_BUILDER(D)

#define BUILTINS_NOSTUB_LIST(V)                     \
    V(ObjectConstructor)                            \
    V(ErrorConstructor)                             \
    V(GlobalDecodeURIComponent)

#define BUILTINS_METHOD_STUB_LIST(V, T, D, K)       \
    BUILTINS_WITH_STRING_STUB_BUILDER(V)            \
    BUILTINS_WITH_OBJECT_STUB_BUILDER(T)            \
    BUILTINS_WITH_ARRAY_STUB_BUILDER(V)             \
    BUILTINS_WITH_SET_STUB_BUILDER(D)               \
    BUILTINS_WITH_MAP_STUB_BUILDER(D)               \
    BUILTINS_WITH_FUNCTION_STUB_BUILDER(V)          \
    BUILTINS_WITH_NUMBER_STUB_BUILDER(T)            \
    BUILTINS_WITH_TYPEDARRAY_STUB_BUILDER(V)        \
    BUILTINS_WITH_DATAVIEW_STUB_BUILDER(K)          \
    BUILTINS_WITH_REFLECT_STUB_BUILDER(T)           \
    BUILTINS_WITH_COLLATOR_STUB_BUILDER(V)          \
    BUILTINS_WITH_REGEXP_STUB_BUILDER(V)

#define BUILTINS_WITH_STRING_STUB_BUILDER(V)                                                            \
    V(CharAt,             String,   Hole(),                       STRING_CHARAT)                        \
    V(FromCharCode,       String,   Hole(),                       STRING_FROM_CHAR_CODE)                \
    V(CharCodeAt,         String,   Hole(),                       STRING_CHAR_CODE_AT)                  \
    V(CodePointAt,        String,   Undefined(),                  STRING_CODE_POINT_AT)                 \
    V(IndexOf,            String,   IntToTaggedPtr(Int32(-1)),    STRING_INDEX_OF)                      \
    V(Substring,          String,   IntToTaggedPtr(Int32(-1)),    STRING_SUB_STRING)                    \
    V(SubStr,             String,   Undefined(),                  STRING_SUB_STR)                       \
    V(Replace,            String,   Undefined(),                  STRING_REPLACE)                       \
    V(Trim,               String,   Undefined(),                  STRING_TRIM)                          \
    V(TrimStart,          String,   Undefined(),                  STRING_TRIM_START)                    \
    V(TrimEnd,            String,   Undefined(),                  STRING_TRIM_END)                      \
    V(TrimLeft,           String,   Undefined(),                  STRING_TRIM_LEFT)                     \
    V(TrimRight,          String,   Undefined(),                  STRING_TRIM_RIGHT)                    \
    V(PadStart,           String,   Undefined(),                  STRING_PAD_START)                     \
    V(PadEnd,             String,   Undefined(),                  STRING_PAD_END)                       \
    V(Concat,             String,   Undefined(),                  STRING_CONCAT)                        \
    V(Slice,              String,   Undefined(),                  STRING_SLICE)                         \
    V(ToLowerCase,        String,   Undefined(),                  STRING_TO_LOWER_CASE)                 \
    V(StartsWith,         String,   TaggedFalse(),                STRING_STARTS_WITH)                   \
    V(EndsWith,           String,   TaggedFalse(),                STRING_ENDS_WITH)                     \
    V(GetStringIterator,  String,   Undefined(),                  STRING_GET_STRING_ITERATOR)

#define BUILTINS_WITH_OBJECT_STUB_BUILDER(V)                                                            \
    V(ToStringFunc,                Object,   Undefined(),         OBJECT_TO_STRING_FUNC)                \
    V(Create,                      Object,   Undefined(),         OBJECT_CREATE)                        \
    V(Assign,                      Object,   Undefined(),         OBJECT_ASSIGN)                        \
    V(HasOwnProperty,              Object,   TaggedFalse(),       OBJECT_HAS_OWN_PROPERTY)              \
    V(Keys,                        Object,   Undefined(),         OBJECT_KEYS)                          \
    V(GetPrototypeOf,              Object,   Undefined(),         OBJECT_GET_PROTOTYPE_OF)              \
    V(GetOwnPropertyNames,         Object,   Undefined(),         OBJECT_GET_OWN_PROPERTY_NAMES)        \
    V(GetOwnPropertySymbols,       Object,   Undefined(),         OBJECT_GET_OWN_PROPERTY_SYMBOLS)      \
    V(Entries,                     Object,   Undefined(),         OBJECT_ENTRIES)                       \
    V(IsFrozen,                    Object,   Undefined(),         OBJECT_IS_FROZEN)                     \
    V(IsSealed,                    Object,   Undefined(),         OBJECT_IS_SEALED)                     \
    V(GetOwnPropertyDescriptors,   Object,   Undefined(),         OBJECT_GET_OWN_PROPERTY_DESCRIPTORS)  \
    V(SetPrototypeOf,              Object,   Undefined(),         OBJECT_SET_PROTOTYPE_OF)

#define BUILTINS_WITH_ARRAY_STUB_BUILDER(V)                                                      \
    V(With,                Array,   Undefined(),   ARRAY_WITH)                                   \
    V(Unshift,             Array,   Undefined(),   ARRAY_UNSHIFT)                                \
    V(Shift,               Array,   Undefined(),   ARRAY_SHIFT)                                  \
    V(Concat,              Array,   Undefined(),   ARRAY_CONCAT)                                 \
    V(Filter,              Array,   Undefined(),   ARRAY_FILTER)                                 \
    V(Find,                Array,   Undefined(),   ARRAY_FIND)                                   \
    V(FindIndex,           Array,   Undefined(),   ARRAY_FINDINDEX)                              \
    V(From,                Array,   Undefined(),   ARRAY_FROM)                                   \
    V(Splice,              Array,   Undefined(),   ARRAY_SPLICE)                                 \
    V(ToSpliced,           Array,   Undefined(),   ARRAY_TO_SPLICED)                             \
    V(ForEach,             Array,   Undefined(),   ARRAY_FOREACH)                                \
    V(IndexOf,             Array,   Undefined(),   ARRAY_INDEXOF)                                \
    V(LastIndexOf,         Array,   Undefined(),   ARRAY_LAST_INDEX_OF)                          \
    V(Pop,                 Array,   Undefined(),   ARRAY_POP)                                    \
    V(Slice,               Array,   Undefined(),   ARRAY_SLICE)                                  \
    V(Reduce,              Array,   Undefined(),   ARRAY_REDUCE)                                 \
    V(Reverse,             Array,   Undefined(),   ARRAY_REVERSE)                                \
    V(ToReversed,          Array,   Undefined(),   ARRAY_TO_REVERSED)                            \
    V(Push,                Array,   Undefined(),   ARRAY_PUSH)                                   \
    V(Values,              Array,   Undefined(),   ARRAY_VALUES)                                 \
    V(Includes,            Array,   Undefined(),   ARRAY_INCLUDES)                               \
    V(CopyWithin,          Array,   Undefined(),   ARRAY_COPY_WITHIN)                            \
    V(Some,                Array,   Undefined(),   ARRAY_SOME)                                   \
    V(Fill,                Array,   Undefined(),   ARRAY_FILL)                                   \
    V(Every,               Array,   Undefined(),   ARRAY_EVERY)                                  \
    V(FindLastIndex,       Array,   Undefined(),   ARRAY_FIND_LAST_INDEX)                        \
    V(FindLast,            Array,   Undefined(),   ARRAY_FIND_LAST)                              \
    V(ReduceRight,         Array,   Undefined(),   ARRAY_REDUCE_RIGHT)                           \
    V(Map,                 Array,   Undefined(),   ARRAY_MAP)                                    \
    V(FlatMap,             Array,   Undefined(),   ARRAY_FLAT_MAP)                               \
    V(ToSorted,            Array,   Undefined(),   ARRAY_TO_SORTED)                              \
    V(IsArray,             Array,   Undefined(),   ARRAY_IS_ARRAY)

#define BUILTINS_WITH_SET_STUB_BUILDER(V)                                                        \
    V(Clear,    Set,   Undefined(),    SET_CLEAR)                                                \
    V(Values,   Set,   Undefined(),    SET_VALUES)                                               \
    V(Entries,  Set,   Undefined(),    SET_ENTRIES)                                              \
    V(ForEach,  Set,   Undefined(),    SET_FOR_EACH)                                             \
    V(Add,      Set,   Undefined(),    SET_ADD)                                                  \
    V(Delete,   Set,   Undefined(),    SET_DELETE)                                               \
    V(Has,      Set,   Undefined(),    SET_HAS)

#define BUILTINS_WITH_MAP_STUB_BUILDER(V)                                                        \
    V(Clear,    Map,   Undefined(),    MAP_CLEAR)                                                \
    V(Values,   Map,   Undefined(),    MAP_VALUES)                                               \
    V(Entries,  Map,   Undefined(),    MAP_ENTRIES)                                              \
    V(Keys,     Map,   Undefined(),    MAP_KEYS)                                                 \
    V(ForEach,  Map,   Undefined(),    MAP_FOR_EACH)                                             \
    V(Set,      Map,   Undefined(),    MAP_SET)                                                  \
    V(Delete,   Map,   Undefined(),    MAP_DELETE)                                               \
    V(Has,      Map,   Undefined(),    MAP_HAS)                                                  \
    V(Get,      Map,   Undefined(),    MAP_GET)

#define BUILTINS_WITH_FUNCTION_STUB_BUILDER(V)                                                   \
    V(PrototypeApply,  Function,  Undefined(),  FUNCTION_PROTOTYPE_APPLY)                        \
    V(PrototypeBind,   Function,  Undefined(),  FUNCTION_PROTOTYPE_BIND)                         \
    V(PrototypeCall,   Function,  Undefined(),  FUNCTION_PROTOTYPE_CALL)

#define BUILTINS_WITH_NUMBER_STUB_BUILDER(V)                                                     \
    V(ParseFloat,      Number,    Undefined(),  NUMBER_PARSE_FLOAT)                              \
    V(ParseInt,        Number,    Undefined(),  NUMBER_PARSE_INT)                                \
    V(IsFinite,        Number,    Undefined(),  NUMBER_IS_FINITE)                                \
    V(IsNaN,           Number,    Undefined(),  NUMBER_IS_NAN)                                   \
    V(IsInteger,       Number,    Undefined(),  NUMBER_IS_INTEGER)                               \
    V(IsSafeInteger,   Number,    Undefined(),  NUMBER_IS_SAFEINTEGER)                           \
    V(ToStringFunc,    Number,    Undefined(),  NUMBER_TO_STRING_FUNC)

#if ENABLE_NEXT_OPTIMIZATION
#define BUILTINS_WITH_REFLECT_STUB_BUILDER(V)                                                                      \
    V(Get, Reflect, Undefined(), REFLECT_GET)                                                                      \
    V(Has, Reflect, Undefined(), REFLECT_HAS)                                                                      \
    V(Set, Reflect, Undefined(), REFLECT_SET)
#else
#define BUILTINS_WITH_REFLECT_STUB_BUILDER(V) V(Get, Reflect, Undefined(), REFLECT_GET)
#endif

#define BUILTINS_WITH_COLLATOR_STUB_BUILDER(V)                                                   \
    V(ResolvedOptions, Collator,   Undefined(),  COLLATOR_RESOLVED_OPTIONS)

#define BUILTINS_WITH_REGEXP_STUB_BUILDER(V)                                                     \
    V(GetFlags,        RegExp,   Undefined(),   REGEXP_GET_FLAGS)

#define BUILTINS_WITH_TYPEDARRAY_STUB_BUILDER(V)                                                 \
    V(Reverse,         TypedArray,  Undefined(),   TYPED_ARRAY_REVERSE)                          \
    V(LastIndexOf,     TypedArray,  Undefined(),   TYPED_ARRAY_LAST_INDEX_OF)                    \
    V(IndexOf,         TypedArray,  Undefined(),   TYPED_ARRAY_INDEX_OF)                         \
    V(Find,            TypedArray,  Undefined(),   TYPED_ARRAY_FIND)                             \
    V(Includes,        TypedArray,  Undefined(),   TYPED_ARRAY_INCLUDES)                         \
    V(CopyWithin,      TypedArray,  Undefined(),   TYPED_ARRAY_COPY_WITHIN)                      \
    V(ReduceRight,     TypedArray,  Undefined(),   TYPED_ARRAY_REDUCE_RIGHT)                     \
    V(Reduce,          TypedArray,  Undefined(),   TYPED_ARRAY_REDUCE)                           \
    V(Every,           TypedArray,  Undefined(),   TYPED_ARRAY_EVERY)                            \
    V(Some,            TypedArray,  Undefined(),   TYPED_ARRAY_SOME)                             \
    V(Filter,          TypedArray,  Undefined(),   TYPED_ARRAY_FILTER)                           \
    V(With,            TypedArray,  Undefined(),   TYPED_ARRAY_WITH)                             \
    V(Entries,         TypedArray,  Undefined(),   TYPED_ARRAY_ENTRIES)                          \
    V(Keys,            TypedArray,  Undefined(),   TYPED_ARRAY_KEYS)                             \
    V(Values,          TypedArray,  Undefined(),   TYPED_ARRAY_VALUES)                           \
    V(Slice,           TypedArray,  Undefined(),   TYPED_ARRAY_SLICE)                            \
    V(SubArray,        TypedArray,  Undefined(),   TYPED_ARRAY_SUB_ARRAY)                        \
    V(Sort,            TypedArray,  Undefined(),   TYPED_ARRAY_SORT)                             \
    V(Set,             TypedArray,  Undefined(),   TYPED_ARRAY_SET)                              \
    V(FindIndex,       TypedArray,  Undefined(),   TYPED_ARRAY_FIND_INDEX)                       \
    V(FindLastIndex,   TypedArray,  Undefined(),   TYPED_ARRAY_FIND_LAST_INDEX)                  \
    V(ToSorted,        TypedArray,  Undefined(),   TYPED_ARRAY_TO_SORTED)                        \
    V(Map,             TypedArray,  Undefined(),   TYPED_ARRAY_MAP)                              \
    V(ToReversed,      TypedArray,  Undefined(),   TYPED_ARRAY_TO_REVERSED)

#define BUILTINS_WITH_DATAVIEW_STUB_BUILDER(V)                                                   \
    V(SetInt32,     DataView,  INT32,     SetTypedValue,   Undefined(),   DATA_VIEW_SET_INT32)   \
    V(SetFloat32,   DataView,  FLOAT32,   SetTypedValue,   Undefined(),   DATA_VIEW_SET_FLOAT32) \
    V(SetFloat64,   DataView,  FLOAT64,   SetTypedValue,   Undefined(),   DATA_VIEW_SET_FLOAT64)

#define BUILTINS_CONTAINERS_STUB_BUILDER(V)                             \
    BUILTINS_WITH_CONTAINERS_ARRAYLIST_STUB_BUILDER(V)                  \
    BUILTINS_WITH_CONTAINERS_QUEUE_STUB_BUILDER(V)                      \
    BUILTINS_WITH_CONTAINERS_DEQUE_STUB_BUILDER(V)                      \
    BUILTINS_WITH_CONTAINERS_HASHMAP_STUB_BUILDER(V)                    \
    BUILTINS_WITH_CONTAINERS_HASHSET_STUB_BUILDER(V)                    \
    BUILTINS_WITH_CONTAINERS_LIGHTWEIGHTMAP_STUB_BUILDER(V)             \
    BUILTINS_WITH_CONTAINERS_LIGHTWEIGHTSET_STUB_BUILDER(V)             \
    BUILTINS_WITH_CONTAINERS_LINKEDLIST_STUB_BUILDER(V)                 \
    BUILTINS_WITH_CONTAINERS_LIST_STUB_BUILDER(V)                       \
    BUILTINS_WITH_CONTAINERS_PLAINARRAY_STUB_BUILDER(V)                 \
    BUILTINS_WITH_CONTAINERS_STACK_STUB_BUILDER(V)                      \
    BUILTINS_WITH_CONTAINERS_VECTOR_STUB_BUILDER(V)

#define BUILTINS_WITH_CONTAINERS_ARRAYLIST_STUB_BUILDER(V)              \
    V(ForEach,                  ArrayList,          Undefined())        \
    V(ReplaceAllElements,       ArrayList,          Undefined())

#define BUILTINS_WITH_CONTAINERS_QUEUE_STUB_BUILDER(V)                  \
    V(ForEach,                  Queue,              Undefined())

#define BUILTINS_WITH_CONTAINERS_DEQUE_STUB_BUILDER(V)                  \
    V(ForEach,                  Deque,              Undefined())

#define BUILTINS_WITH_CONTAINERS_HASHMAP_STUB_BUILDER(V)                \
    V(ForEach,                  HashMap,            Undefined())

#define BUILTINS_WITH_CONTAINERS_HASHSET_STUB_BUILDER(V)                \
    V(ForEach,                  HashSet,            Undefined())

#define BUILTINS_WITH_CONTAINERS_LIGHTWEIGHTMAP_STUB_BUILDER(V)         \
    V(ForEach,                  LightWeightMap,     Undefined())

#define BUILTINS_WITH_CONTAINERS_LIGHTWEIGHTSET_STUB_BUILDER(V)         \
    V(ForEach,                  LightWeightSet,     Undefined())

#define BUILTINS_WITH_CONTAINERS_LINKEDLIST_STUB_BUILDER(V)             \
    V(ForEach,                  LinkedList,         Undefined())

#define BUILTINS_WITH_CONTAINERS_LIST_STUB_BUILDER(V)                   \
    V(ForEach,                  List,               Undefined())

#define BUILTINS_WITH_CONTAINERS_PLAINARRAY_STUB_BUILDER(V)             \
    V(ForEach,                  PlainArray,         Undefined())

#define BUILTINS_WITH_CONTAINERS_STACK_STUB_BUILDER(V)                  \
    V(ForEach,                  Stack,              Undefined())

#define BUILTINS_WITH_CONTAINERS_VECTOR_STUB_BUILDER(V)                 \
    V(ForEach,                  Vector,             Undefined())        \
    V(ReplaceAllElements,       Vector,             Undefined())

#define BUILTINS_ARKTOOLS_STUB_BUILDER(V) \
    V(HashCode, ArkTools, Undefined())

#define BUILTINS_CONSTRUCTOR_STUB_LIST(V)           \
    V(BooleanConstructor)                           \
    V(NumberConstructor)                            \
    V(ProxyConstructor)                             \
    V(DateConstructor)                              \
    V(ArrayConstructor)                             \
    V(SetConstructor)                               \
    V(MapConstructor)                               \
    V(Int8ArrayConstructor)                         \
    V(Uint8ArrayConstructor)                        \
    V(Uint8ClampedArrayConstructor)                 \
    V(Int16ArrayConstructor)                        \
    V(Uint16ArrayConstructor)                       \
    V(Int32ArrayConstructor)                        \
    V(Uint32ArrayConstructor)                       \
    V(Float32ArrayConstructor)                      \
    V(Float64ArrayConstructor)                      \
    V(BigInt64ArrayConstructor)                     \
    V(BigUint64ArrayConstructor)

// NEXT_AOT_AND_BUILTINS_STUB_LIST: List of functions that is optimized for Next
#if ENABLE_NEXT_OPTIMIZATION
    #define NEXT_AOT_AND_BUILTINS_STUB_LIST(V)             \
        V(ArrayIteratorProtoNext)                          \
        V(MapIteratorProtoNext)                            \
        V(SetIteratorProtoNext)
    #define NEXT_AOT_BUILTINS_STUB_LIST(V)
#else
    #define NEXT_AOT_AND_BUILTINS_STUB_LIST(V)
    #define NEXT_AOT_BUILTINS_STUB_LIST(V)                 \
        V(ArrayIteratorProtoNext)                          \
        V(MapIteratorProtoNext)                            \
        V(SetIteratorProtoNext)
#endif

#define AOT_AND_BUILTINS_STUB_LIST(V)               \
    V(StringLocaleCompare)                          \
    V(StringIteratorProtoNext)                      \
    V(ArraySort)                                    \
    NEXT_AOT_AND_BUILTINS_STUB_LIST(V)

#define AOT_BUILTINS_STUB_LIST(V)                       \
    V(JsonStringify)                                    \
    V(MapProtoIterator)                                 \
    V(SetProtoIterator)                                 \
    V(StringProtoIterator)                              \
    V(ArrayProtoIterator)                               \
    V(TypedArrayProtoIterator)                          \
    V(IteratorProtoReturn)                              \
    NEXT_AOT_BUILTINS_STUB_LIST(V)

#if ENABLE_NEXT_OPTIMIZATION
#define CONDITION_REFLECT_HAS(V)
#else
#define CONDITION_REFLECT_HAS(V) V(ReflectHas)
#endif

// List of builtins which will try to be inlined in TypedNativeInlineLoweringPass
#define AOT_BUILTINS_INLINE_LIST(V)                 \
    V(MathAcos)                                     \
    V(MathAcosh)                                    \
    V(MathAsin)                                     \
    V(MathAsinh)                                    \
    V(MathAtan)                                     \
    V(MathAtan2)                                    \
    V(MathAtanh)                                    \
    V(MathCos)                                      \
    V(MathCosh)                                     \
    V(MathSign)                                     \
    V(MathSin)                                      \
    V(MathSinh)                                     \
    V(MathSqrt)                                     \
    V(MathTan)                                      \
    V(MathCbrt)                                     \
    V(MathTanh)                                     \
    V(MathLog)                                      \
    V(MathLog2)                                     \
    V(MathLog10)                                    \
    V(MathLog1p)                                    \
    V(MathExp)                                      \
    V(MathExpm1)                                    \
    V(MathClz32)                                    \
    V(MathPow)                                      \
    V(MathTrunc)                                    \
    V(MathCeil)                                     \
    V(MathFloor)                                    \
    V(MathAbs)                                      \
    V(MathRound)                                    \
    V(MathFRound)                                   \
    V(MathMin)                                      \
    V(MathMax)                                      \
    V(MathImul)                                     \
    V(DateGetTime)                                  \
    V(DateNow)                                      \
    V(GlobalIsFinite)                               \
    V(GlobalIsNan)                                  \
    V(BigIntConstructor)                            \
    V(ArrayBufferIsView)                            \
    V(BigIntAsIntN)                                 \
    V(BigIntAsUintN)                                \
    V(DataViewGetFloat32)                           \
    V(DataViewGetFloat64)                           \
    V(DataViewGetInt8)                              \
    V(DataViewGetInt16)                             \
    V(DataViewGetInt32)                             \
    V(DataViewGetUint16)                            \
    V(DataViewGetUint32)                            \
    V(DataViewGetUint8)                             \
    V(DataViewSetInt8)                              \
    V(DataViewSetInt16)                             \
    V(DataViewSetUint8)                             \
    V(DataViewSetUint16)                            \
    V(DataViewSetUint32)                            \
    V(ObjectIs)                                     \
    V(ObjectGetProto)                               \
    V(ObjectIsPrototypeOf)                          \
    V(ReflectGetPrototypeOf)                        \
    CONDITION_REFLECT_HAS(V)                        \
    V(ReflectConstruct)                             \
    V(ReflectApply)                                 \
    V(FunctionPrototypeHasInstance)                 \
    V(ArrayKeys)                                    \
    V(ArrayEntries)                                 \
    V(TYPED_BUILTINS_INLINE_FIRST = MathAcos)       \
    V(TYPED_BUILTINS_INLINE_LAST = ArrayEntries)

#endif  // ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_LIST_H
