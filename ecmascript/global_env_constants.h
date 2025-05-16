/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_GLOBAL_ENV_CONSTANTS_H
#define ECMASCRIPT_GLOBAL_ENV_CONSTANTS_H

#include <cstdint>
#include "ecmascript/compiler/builtins/builtins_call_signature_list.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/visitor.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
// Forward Declaration
template<typename T>
class JSHandle;
class JSHClass;
class JSThread;
class ObjectFactory;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SHARED_GLOBAL_ENV_CONSTANT_CLASS(V)                                                                           \
    /* GC Root */                                                                                                     \
    V(JSTaggedValue, HClassClass, HCLASS_CLASS_INDEX, ecma_roots_class)                                               \
    V(JSTaggedValue, FreeObjectWithNoneFieldClass, FREE_OBJECT_WITH_NONE_FIELD_CLASS_INDEX, ecma_roots_class)         \
    V(JSTaggedValue, FreeObjectWithOneFieldClass, FREE_OBJECT_WITH_ONE_FIELD_CLASS_INDEX, ecma_roots_class)           \
    V(JSTaggedValue, FreeObjectWithTwoFieldClass, FREE_OBJECT_WITH_TWO_FIELD_CLASS_INDEX, ecma_roots_class)           \
    V(JSTaggedValue, LineStringClass, LINE_STRING_CLASS_INDEX, ecma_roots_class)                                      \
    V(JSTaggedValue, SlicedStringClass, SLICED_STRING_CLASS_INDEX, ecma_roots_class)                                  \
    V(JSTaggedValue, TreeStringClass, TREE_STRING_CLASS_INDEX, ecma_roots_class)                                      \
    V(JSTaggedValue, TaggedArrayClass, TAGGED_ARRAY_CLASS_INDEX, ecma_roots_class)                                    \
    V(JSTaggedValue, ByteArrayClass, BYTE_ARRAY_CLASS_INDEX, ecma_roots_class)                                        \
    V(JSTaggedValue, ConstantPoolClass, CONSTANT_POOL_CLASS_INDEX, ecma_roots_class)                                  \
    V(JSTaggedValue, ProfileTypeInfoClass, PROFILE_TYPE_INFO_CLASS_INDEX, ecma_roots_class)                           \
    V(JSTaggedValue, DictionaryClass, DICTIONARY_CLASS_INDEX, ecma_roots_class)                                       \
    V(JSTaggedValue, COWArrayClass, COW_ARRAY_CLASS_INDEX, ecma_roots_class)                                          \
    V(JSTaggedValue, MutantTaggedArrayClass, MUTANT_TAGGED_ARRAY_CLASS_INDEX, ecma_roots_class)                       \
    V(JSTaggedValue, COWMutantTaggedArrayClass, COW_MUTANT_TAGGED_ARRAY_CLASS_INDEX, ecma_roots_class)                \
    V(JSTaggedValue, BigIntClass, BIGINT_CLASS_INDEX, ecma_roots_class)                                               \
    V(JSTaggedValue, JSNativePointerClass, JS_NATIVE_POINTER_CLASS_INDEX, ecma_roots_class)                           \
    V(JSTaggedValue, SJSNativePointerClass, SENDABLE_JS_NATIVE_POINTER_CLASS_INDEX, ecma_roots_class)                 \
    V(JSTaggedValue, LexicalEnvClass, LEXICAL_ENV_CLASS_INDEX, ecma_roots_class)                                      \
    V(JSTaggedValue, SFunctionEnvClass, SFUNCTION_ENV_CLASS_INDEX, ecma_roots_class)                                  \
    V(JSTaggedValue, SymbolClass, SYMBOL_CLASS_INDEX, ecma_roots_class)                                               \
    V(JSTaggedValue, AccessorDataClass, ACCESSOR_DATA_CLASS_INDEX, ecma_roots_class)                                  \
    V(JSTaggedValue, InternalAccessorClass, INTERNAL_ACCESSOR_CLASS_INDEX, ecma_roots_class)                          \
    V(JSTaggedValue, CompletionRecordClass, COMPLETION_RECORD_CLASS_INDEX, ecma_roots_class)                          \
    V(JSTaggedValue, GeneratorContextClass, GENERATOR_CONTEST_INDEX, ecma_roots_class)                                \
    V(JSTaggedValue, AsyncGeneratorRequestRecordClass, ASYNC_GENERATOR_REQUEST_CLASS_INDEX, ecma_roots_class)         \
    V(JSTaggedValue, AsyncIteratorRecordClass, ASYNC_ITERATOR_RECORD_CLASS_INDEX, ecma_roots_class)                   \
    V(JSTaggedValue, CapabilityRecordClass, CAPABILITY_RECORD_CLASS_INDEX, ecma_roots_class)                          \
    V(JSTaggedValue, ReactionsRecordClass, REACTIONS_RECORD_CLASS_INDEX, ecma_roots_class)                            \
    V(JSTaggedValue, PromiseIteratorRecordClass, PROMISE_ITERATOR_RECORD_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, PromiseRecordClass, PROMISE_RECORD_CLASS_INDEX, ecma_roots_class)                                \
    V(JSTaggedValue, PromiseResolvingFunctionsRecordClass, PROMISE_RESOLVING_FUNCTIONS_CLASS_INDEX, ecma_roots_class) \
    V(JSTaggedValue, MicroJobQueueClass, MICRO_JOB_QUEUE_CLASS_INDEX, ecma_roots_class)                               \
    V(JSTaggedValue, PendingJobClass, PENDING_JOB_CLASS_INDEX, ecma_roots_class)                                      \
    V(JSTaggedValue, EnumCacheClass, ENUM_CACHE_CLASS_INDEX, ecma_roots_class)                                        \
    V(JSTaggedValue, ProtoChangeMarkerClass, PROTO_CHANGE_MARKER_CLASS_INDEX, ecma_roots_class)                       \
    V(JSTaggedValue, ProtoChangeDetailsClass, PROTO_CHANGE_DETAILS_CLASS_INDEX, ecma_roots_class)                     \
    V(JSTaggedValue, MarkerCellClass, MARKER_CELL_CLASS_INDEX, ecma_roots_class)                                      \
    V(JSTaggedValue, TrackInfoClass, TRACK_INFO_CLASS_INDEX, ecma_roots_class)                                        \
    V(JSTaggedValue, PrototypeHandlerClass, PROTOTYPE_HANDLER_CLASS_INDEX, ecma_roots_class)                          \
    V(JSTaggedValue, TransitionHandlerClass, TRANSITION_HANDLER_CLASS_INDEX, ecma_roots_class)                        \
    V(JSTaggedValue, TransWithProtoHandlerClass, TRANS_WITH_PROTO_HANDLER_CLASS_INDEX, ecma_roots_class)              \
    V(JSTaggedValue, StoreAOTHandlerClass, STORE_TS_HANDLER_CLASS_INDEX, ecma_roots_class)                            \
    V(JSTaggedValue, PropertyBoxClass, PROPERTY_BOX_CLASS_INDEX, ecma_roots_class)                                    \
    V(JSTaggedValue, ProgramClass, PROGRAM_CLASS_INDEX, ecma_roots_class)                                             \
    V(JSTaggedValue, ImportEntryClass, IMPORT_ENTRY_CLASS_INDEX, ecma_roots_class)                                    \
    V(JSTaggedValue, LocalExportEntryClass, LOCAL_EXPORT_ENTRY_CLASS_INDEX, ecma_roots_class)                         \
    V(JSTaggedValue, IndirectExportEntryClass, INDIRECT_EXPORT_ENTRY_CLASS_INDEX, ecma_roots_class)                   \
    V(JSTaggedValue, StarExportEntryClass, STAR_EXPORT_ENTRY_CLASS_INDEX, ecma_roots_class)                           \
    V(JSTaggedValue, SourceTextModuleClass, SOURCE_TEXT_MODULE_CLASS_INDEX, ecma_roots_class)                         \
    V(JSTaggedValue, ResolvedBindingClass, RESOLVED_BINDING_CLASS_INDEX, ecma_roots_class)                            \
    V(JSTaggedValue, ResolvedIndexBindingClass, RESOLVED_INDEX_BINDING_CLASS_INDEX, ecma_roots_class)                 \
    V(JSTaggedValue, MachineCodeClass, MACHINE_CODE_CLASS_INDEX, ecma_roots_class)                                    \
    V(JSTaggedValue, ClassInfoExtractorHClass, CLASS_INFO_EXTRACTOR_HCLASS_INDEX, ecma_roots_class)                   \
    V(JSTaggedValue, CellRecordClass, CELL_RECORD_CLASS_INDEX, ecma_roots_class)                                      \
    V(JSTaggedValue, AOTLiteralInfoClass, AOT_LITERAL_INFO_CLASS_INDEX, ecma_roots_class)                             \
    V(JSTaggedValue, MethodClass, METHOD_CLASS_INDEX, ecma_roots_class)                                               \
    V(JSTaggedValue, LinkedNode, LINKED_NODE_CLASS_INDEX, ecma_roots_class)                                           \
    V(JSTaggedValue, RBTreeNode, RB_TREENODE_CLASS_INDEX, ecma_roots_class)                                           \
    V(JSTaggedValue, ClassLiteralClass, CLASS_LITERAL_HCLASS_INDEX, ecma_roots_class)                                 \
    V(JSTaggedValue, ExtraProfileTypeInfoClass, EXTRA_PROFILE_TYPE_INFO_CLASS_INDEX, ecma_roots_class)                \
    V(JSTaggedValue, ProfileTypeInfoCell0Class, PROFILE_TYPE_INFO_CELL_0_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, ProfileTypeInfoCell1Class, PROFILE_TYPE_INFO_CELL_1_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, ProfileTypeInfoCellNClass, PROFILE_TYPE_INFO_CELL_N_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, FunctionTemplateClass, FUNCTION_TEMPLATE_CLASS_INDEX, ecma_roots_class)                          \
    V(JSTaggedValue, VTableClass, VTABLE_CLASS_INDEX, ecma_roots_class)                                               \
    V(JSTaggedValue, ResolvedRecordIndexBindingClass, RESOLVED_RECORD_INEDX_BINDING_CLASS_INDEX, ecma_roots_class)    \
    V(JSTaggedValue, ResolvedRecordBindingClass, RESOLVED_RECORD_BINDING_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, SendableEnvClass, SENDABLE_ENV_CLASS_INDEX, ecma_roots_class)

#ifdef USE_CMC_GC
#define SHARED_GLOBAL_ENV_CONSTANT_SHARED_CLASS(V)                                                                    \
    V(JSTaggedValue, SharedTaggedArrayClass, SHARED_TAGGED_ARRAY_CLASS_INDEX, ecma_roots_class)                       \
    V(JSTaggedValue, SharedConstantPoolClass, SHARED_CONSTANT_POOL_CLASS_INDEX, ecma_roots_class)                     \
    /* ProfileTypeInfo only in local now */                                                                           \
    V(JSTaggedValue, SharedDictionaryClass, SHARED_DICTIONARY_CLASS_INDEX, ecma_roots_class)                          \
    V(JSTaggedValue, SharedCOWArrayClass, SHARED_COW_ARRAY_CLASS_INDEX, ecma_roots_class)                             \
    V(JSTaggedValue, SharedMutantTaggedArrayClass, SHARED_MUTANT_TAGGED_ARRAY_CLASS_INDEX, ecma_roots_class)          \
    /* COWMutantTaggedArray only in local now */                                                                      \
    V(JSTaggedValue, SharedAOTLiteralInfoClass, SHARED_AOT_LITERAL_INFO_CLASS_INDEX, ecma_roots_class)                \
    /* VTable only in local now */
#endif

#define GLOBAL_ENV_CONSTANT_CLASS(V)                                                                                  \
    V(JSTaggedValue, JSProxyCallableClass, JS_PROXY_CALLABLE_CLASS_INDEX, ecma_roots_class)                           \
    V(JSTaggedValue, JSProxyConstructClass, JS_PROXY_CONSTRUCT_CLASS_INDEX, ecma_roots_class)                         \
    V(JSTaggedValue, JSRealmClass, JS_REALM_CLASS_INDEX, ecma_roots_class)                                            \
    V(JSTaggedValue, JSProxyOrdinaryClass, JS_PROXY_ORDINARY_CLASS_INDEX, ecma_roots_class)                           \
    V(JSTaggedValue, JSSetIteratorClass, JS_SET_ITERATOR_CLASS_INDEX, ecma_roots_class)                               \
    V(JSTaggedValue, JSSharedSetIteratorClass, JS_SHARED_SET_ITERATOR_CLASS_INDEX, ecma_roots_class)                  \
    V(JSTaggedValue, JSRegExpIteratorClass, JS_REGEXP_ITERATOR_CLASS_INDEX, ecma_roots_class)                         \
    V(JSTaggedValue, JSMapIteratorClass, JS_MAP_ITERATOR_CLASS_INDEX, ecma_roots_class)                               \
    V(JSTaggedValue, JSSharedMapIteratorClass, JS_SHARED_MAP_ITERATOR_CLASS_INDEX, ecma_roots_class)                  \
    V(JSTaggedValue, JSArrayIteratorClass, JS_ARRAY_ITERATOR_CLASS_INDEX, ecma_roots_class)                           \
    V(JSTaggedValue, JSSharedArrayIteratorClass, JS_SHARED_ARRAY_ITERATOR_CLASS_INDEX, ecma_roots_class)              \
    V(JSTaggedValue, JSAPIArrayListIteratorClass, JS_API_ARRAYLIST_ITERATOR_CLASS_INDEX, ecma_roots_class)            \
    V(JSTaggedValue, JSAPIDequeIteratorClass, JS_API_DEQUE_ITERATOR_CLASS_INDEX, ecma_roots_class)                    \
    V(JSTaggedValue, JSAPILightWeightMapIteratorClass, JS_API_LIGHTWEIGHTMAP_ITERATOR_CLASS_INDEX, ecma_roots_class)  \
    V(JSTaggedValue, JSAPILightWeightSetIteratorClass, JS_API_LIGHTWEIGHTSET_ITERATOR_CLASS_INDEX, ecma_roots_class)  \
    V(JSTaggedValue, JSAPILinkedListIteratorClass, JS_API_LINKED_LIST_ITERATOR_CLASS_INDEX, ecma_roots_class)         \
    V(JSTaggedValue, JSAPIListIteratorClass, JS_API_LIST_ITERATOR_CLASS_INDEX, ecma_roots_class)                      \
    V(JSTaggedValue, JSAPIPlainArrayIteratorClass, JS_API_PLAIN_ARRAY_ITERATOR_CLASS_INDEX, ecma_roots_class)         \
    V(JSTaggedValue, JSAPIQueueIteratorClass, JS_API_QUEUE_ITERATOR_CLASS_INDEX, ecma_roots_class)                    \
    V(JSTaggedValue, JSAPIStackIteratorClass, JS_API_STACK_ITERATOR_CLASS_INDEX, ecma_roots_class)                    \
    V(JSTaggedValue, JSAPIVectorIteratorClass, JS_API_VECTOR_ITERATOR_CLASS_INDEX, ecma_roots_class)                  \
    V(JSTaggedValue, JSAPIBitVectorIteratorClass, JS_API_BITVECTOR_ITERATOR_CLASS_INDEX, ecma_roots_class)            \
    V(JSTaggedValue, JSAPIHashMapIteratorClass, JS_API_HASH_MAP_ITERATOR_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, JSAPIHashSetIteratorClass, JS_API_HASH_SET_ITERATOR_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, JSAPITreeMapIteratorClass, JS_API_TREE_MAP_ITERATOR_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, JSAPITreeSetIteratorClass, JS_API_TREE_SET_ITERATOR_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, ObjectClass, OBJECT_HCLASS_INDEX, initial_object_hclass)                                         \
    V(JSTaggedValue, ClassPrototypeClass, CLASS_PROTOTYPE_HCLASS_INDEX, ecma_roots_class)                             \
    V(JSTaggedValue, ClassConstructorClass, CLASS_CONSTRUCTOR_HCLASS_INDEX, ecma_roots_class)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLOBAL_ENV_CONSTANT_SPECIAL(V)                                                                 \
    V(JSTaggedValue, EmptyLayoutInfo, EMPTY_LAYOUT_INFO_OBJECT_INDEX, ecma_roots_special)              \
    V(JSTaggedValue, DefaultSupers, DEFAULT_SUPERS_INDEX, ecma_roots_special)                          \
    V(JSTaggedValue, EmptyTaggedQueue, EMPTY_TAGGED_QUEUE_OBJECT_INDEX, ecma_roots_special)

#if ENABLE_NEXT_OPTIMIZATION
#define CONDITION_GLOBAL_REFLECT_HAS(V)
#else
#define CONDITION_GLOBAL_REFLECT_HAS(V) V(JSTaggedValue, ReflectHas, REFLECT_HAS_INDEX, ecma_roots_special)
#endif

// Use for builtins inlining
#define GLOBAL_ENV_INLINED_BUILTINS(V)                                                                  \
    V(JSTaggedValue, MathSqrt, MATH_SQRT_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, MathAcos, MATH_ACOS_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, MathAcosh, MATH_ACOSH_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, MathAsin, MATH_ASIN_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, MathAsinh, MATH_ASINH_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, MathAtan, MATH_ATAN_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, MathAtan2, MATH_ATAN2_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, MathAtanh, MATH_ATANH_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, MathCos, MATH_COS_INDEX, ecma_roots_builtins)                                      \
    V(JSTaggedValue, MathCosh, MATH_COSH_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, MathSign, MATH_SIGN_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, MathSin, MATH_SIN_INDEX, ecma_roots_builtins)                                      \
    V(JSTaggedValue, MathSinh, MATH_SINH_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, MathTan, MATH_TAN_INDEX, ecma_roots_builtins)                                      \
    V(JSTaggedValue, MathTanh, MATH_TANH_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, MathTrunc, MATH_TRUNC_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, MathLog, MATH_LOG_INDEX, ecma_roots_builtins)                                      \
    V(JSTaggedValue, MathLog2, MATH_LOG2_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, MathLog10, MATH_LOG10_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, MathLog1p, MATH_LOG1P_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, MathExp, MATH_EXP_INDEX, ecma_roots_builtins)                                      \
    V(JSTaggedValue, MathExpm1, MATH_EXPM1_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, MathClz32, MATH_CLZ32_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, MathAbs, MATH_ABS_INDEX, ecma_roots_builtins)                                      \
    V(JSTaggedValue, MathPow, MATH_POW_INDEX, ecma_roots_builtins)                                      \
    V(JSTaggedValue, MathCbrt, MATH_CBRT_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, MathMin, MATH_MIN_INDEX, ecma_roots_builtins)                                      \
    V(JSTaggedValue, MathMax, MATH_MAX_INDEX, ecma_roots_builtins)                                      \
    V(JSTaggedValue, MathRound, MATH_ROUND_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, MathFRound, MATH_FROUND_INDEX, ecma_roots_builtins)                                \
    V(JSTaggedValue, MathCeil, MATH_CEIL_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, MathFloor, MATH_FLOOR_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, MathImul, MATH_IMUL_INDEX, ecma_roots_builtins)                                    \
    V(JSTaggedValue, BigIntAsIntN, BIGINT_AS_INTN_INDEX, ecma_roots_builtins)                           \
    V(JSTaggedValue, BigIntAsUintN, BIGINT_AS_UINTN_INDEX, ecma_roots_builtins)                         \
    V(JSTaggedValue, GlobalIsFinite, GLOBAL_IS_FINITE_INDEX, ecma_roots_builtins)                       \
    V(JSTaggedValue, GlobalIsNan, GLOBAL_IS_NAN_INDEX, ecma_roots_builtins)                             \
    V(JSTaggedValue, LocaleCompareFunction, LOCALE_COMPARE_FUNCTION_INDEX, ecma_roots_builtins)         \
    V(JSTaggedValue, ArraySortFunction, ARRAY_SORT_FUNCTION_INDEX, ecma_roots_builtins)                 \
    V(JSTaggedValue, JsonStringifyFunction, JSON_STRINGIFY_FUNCTION_INDEX, ecma_roots_builtins)         \
    V(JSTaggedValue, MapIteratorProtoNext, MAP_ITERATOR_PROTO_NEXT_INDEX, ecma_roots_builtins)          \
    V(JSTaggedValue, SetIteratorProtoNext, SET_ITERATOR_PROTO_NEXT_INDEX, ecma_roots_builtins)          \
    V(JSTaggedValue, StringIteratorProtoNext, STRING_ITERATOR_PROTO_NEXT_INDEX, ecma_roots_builtins)    \
    V(JSTaggedValue, ArrayIteratorProtoNext, ARRAY_ITERATOR_PROTO_NEXT_INDEX, ecma_roots_builtins)      \
    V(JSTaggedValue, IteratorProtoReturn, ITERATOR_PROTO_RETURN_INDEX, ecma_roots_builtins)             \
    V(JSTaggedValue, ArrayBufferIsView, ARRAY_BUFFER_IS_VIEW_INDEX, ecma_roots_builtins)                \
    V(JSTaggedValue, DataViewGetFloat32, DATA_VIEW_GET_FLOAT32_INDEX, ecma_roots_builtins)              \
    V(JSTaggedValue, DataViewGetFloat64, DATA_VIEW_GET_FLOAT64_INDEX, ecma_roots_builtins)              \
    V(JSTaggedValue, DataViewGetInt8, DATA_VIEW_GET_INT8_INDEX, ecma_roots_builtins)                    \
    V(JSTaggedValue, DataViewGetInt16, DATA_VIEW_GET_INT16_INDEX, ecma_roots_builtins)                  \
    V(JSTaggedValue, DataViewGetInt32, DATA_VIEW_GET_INT32_INDEX, ecma_roots_builtins)                  \
    V(JSTaggedValue, DataViewGetUint16, DATA_VIEW_GET_UINT16_INDEX, ecma_roots_builtins)                \
    V(JSTaggedValue, DataViewGetUint32, DATA_VIEW_GET_UINT32_INDEX, ecma_roots_builtins)                \
    V(JSTaggedValue, DataViewGetUint8, DATA_VIEW_GET_UINT8_INDEX, ecma_roots_builtins)                  \
    V(JSTaggedValue, DataViewSetInt8, DATA_VIEW_SET_INT8_INDEX, ecma_roots_builtins)                    \
    V(JSTaggedValue, DataViewSetInt16, DATA_VIEW_SET_INT16_INDEX, ecma_roots_builtins)                  \
    V(JSTaggedValue, DataViewSetUint8, DATA_VIEW_SET_UINT8_INDEX, ecma_roots_builtins)                  \
    V(JSTaggedValue, DataViewSetUint16, DATA_VIEW_SET_UINT16_INDEX, ecma_roots_builtins)                \
    V(JSTaggedValue, DataViewSetUint32, DATA_VIEW_SET_UINT32_INDEX, ecma_roots_builtins)                \
    V(JSTaggedValue, DateGetTime, DATE_GET_TIME_INDEX, ecma_roots_special)                              \
    V(JSTaggedValue, DateNow, DATE_NOW_INDEX, ecma_roots_special)                                       \
    V(JSTaggedValue, ObjectIs, OBJECT_IS_INDEX, ecma_roots_special)                                     \
    V(JSTaggedValue, ObjectGetPrototo, OBJECT_GET_PROTO_INDEX, ecma_roots_special)                      \
    V(JSTaggedValue, ObjectIsPrototypeOf, OBJECT_IS_PROTOTYPE_OF_INDEX, ecma_roots_special)             \
    V(JSTaggedValue, ReflectGetPrototypeOf, REFLECT_GET_PROTOTYPE_OF_INDEX, ecma_roots_special)         \
    CONDITION_GLOBAL_REFLECT_HAS(V)                                                                     \
    V(JSTaggedValue, ReflectConstruct, REFLECT_CONSTRUCT_INDEX, ecma_roots_special)                     \
    V(JSTaggedValue, ReflectApply, REFLECT_APPLY_INDEX, ecma_roots_special)                             \
    V(JSTaggedValue, FunctionHasInstance, FUNCTION_PROTOTYPE_HAS_INSTANCE_INDEX, ecma_roots_special)    \
    V(JSTaggedValue, ArrayKeys, ARRAY_KEYS_INDEX, ecma_roots_builtins)                                  \
    V(JSTaggedValue, ArrayEntries, ARRAY_ENTRIES_INDEX, ecma_roots_builtins)                            \
    V(JSTaggedValue, GloablDecodeURIComponent, GLOBAL_DECODE_URI_COMPONENT, ecma_roots_builtins)

// All of type JSTaggedValue
#define SHARED_GLOBAL_ENV_CONSTANT_STRING(V)                                                                \
    V(ConstructorString,              CONSTRUCTOR_STRING_INDEX,              "constructor")                 \
    V(PrototypeString,                PROTOTYPE_STRING_INDEX,                "prototype")                   \
    V(SizeString,                     SIZE_STRING_INDEX,                     "size")                        \
    V(LengthString,                   LENGTH_STRING_INDEX,                   "length")                      \
    V(ValueString,                    VALUE_STRING_INDEX,                    "value")                       \
    V(SetString,                      SET_STRING_INDEX,                      "set")                         \
    V(GetString,                      GET_STRING_INDEX,                      "get")                         \
    V(SetWithSpaceString,             SET_SPACE_STRING_INDEX,                "set ")                        \
    V(GetWithSpaceString,             GET_SPACE_STRING_INDEX,                "get ")                        \
    V(WritableString,                 WRITABLE_STRING_INDEX,                 "writable")                    \
    V(EnumerableString,               ENUMERABLE_STRING_INDEX,               "enumerable")                  \
    V(ConfigurableString,             CONFIGURABLE_STRING_INDEX,             "configurable")                \
    V(NameString,                     NAME_STRING_INDEX,                     "name")                        \
    V(SendableElementsSymbol,         SENDABLE_ELEMENTS_INDEX,               "sendableElements")            \
    /* SymbolTable * RegisterSymbols */                                                                     \
    V(GetPrototypeOfString,           GETPROTOTYPEOF_STRING_INDEX,           "getPrototypeOf")              \
    V(SetPrototypeOfString,           SETPROTOTYPEOF_STRING_INDEX,           "setPrototypeOf")              \
    V(IsExtensibleString,             ISEXTENSIBLE_STRING_INDEX,             "isExtensible")                \
    V(PreventExtensionsString,        PREVENTEXTENSIONS_STRING_INDEX,        "preventExtensions")           \
    V(GetOwnPropertyDescriptorString, GETOWNPROPERTYDESCRIPTOR_STRING_INDEX, "getOwnPropertyDescriptor")    \
    V(DefinePropertyString,           DEFINEPROPERTY_STRING_INDEX,           "defineProperty")              \
    V(HasString,                      HAS_STRING_INDEX,                      "has")                         \
    V(DeletePropertyString,           DELETEPROPERTY_STRING_INDEX,           "deleteProperty")              \
    V(EnumerateString,                ENUMERATE_STRING_INDEX,                "enumerate")                   \
    V(OwnKeysString,                  OWNKEYS_STRING_INDEX,                  "ownKeys")                     \
    V(ApplyString,                    APPLY_STRING_INDEX,                    "apply")                       \
    V(NegativeZeroString,             NEGATIVE_ZERO_STRING_INDEX,            "-0")                          \
    V(DoneString,                     DONE_STRING_INDEX,                     "done")                        \
    V(ProxyString,                    PROXY_STRING_INDEX,                    "proxy")                       \
    V(RevokeString,                   REVOKE_STRING_INDEX,                   "revoke")                      \
    V(NextString,                     NEXT_STRING_INDEX,                     "next")                        \
    V(ToStringString,                 TO_STRING_STRING_INDEX,                "toString")                    \
    V(ToLocaleStringString,           TO_LOCALE_STRING_STRING_INDEX,         "toLocaleString")              \
    V(ValueOfString,                  VALUE_OF_STRING_INDEX,                 "valueOf")                     \
    V(UndefinedString,                UNDEFINED_STRING_INDEX,                "undefined")                   \
    V(NullString,                     NULL_STRING_INDEX,                     "null")                        \
    V(BooleanString,                  BOOLEAN_STRING_INDEX,                  "boolean")                     \
    V(NumberString,                   NUMBER_STRING_INDEX,                   "number")                      \
    V(BigIntString,                   BIGINT_STRING_INDEX,                   "bigint")                      \
    V(FunctionString,                 FUNCTION_STRING_INDEX,                 "function")                    \
    V(StringString,                   STRING_STRING_INDEX,                   "string")                      \
    V(SymbolString,                   SYMBOL_STRING_INDEX,                   "symbol")                      \
    V(NativeModuleFailureInfoString,  NATIVE_MODULE_FAILURE_INFO_STRING_INDEX, "NativeModuleFailureInfo")   \
    V(ObjectString,                   OBJECT_STRING_INDEX,                   "object")                      \
    V(TrueString,                     TRUE_STRING_INDEX,                     "true")                        \
    V(FalseString,                    FALSE_STRING_INDEX,                    "false")                       \
    V(ReturnString,                   RETURN_STRING_INDEX,                   "return")                      \
    V(ProxyConstructString,           PROXY_CONSTRUCT_STRING_INDEX,          "construct")                   \
    V(ProxyCallString,                PROXY_CALL_STRING_INDEX,               "call")                        \
    V(PromiseThenString,              PROMISE_THEN_STRING_INDEX,             "then")                        \
    V(PromiseCatchString,             PROMISE_CATCH_STRING_INDEX,            "catch")                       \
    V(PromiseFinallyString,           PROMISE_FINALLY_STRING_INDEX,          "finally")                     \
    V(PromiseStatusString,            PROMISE_STATUS_STRING_INDEX,           "status")                      \
    V(PromiseFulfilledString,         PROMISE_FULFILLED_STRING_INDEX,        "fulfilled")                   \
    V(PromiseRejectedString,          PROMISE_REJECTED_STRING_INDEX,         "rejected")                    \
    V(PromiseReasonString,            PROMISE_REASON_STRING_INDEX,           "reason")                      \
    V(ScriptJobString,                SCRIPT_JOB_STRING_INDEX,               "ScriptJobs")                  \
    V(PromiseString,                  PROMISE_STRING_INDEX,                  "PrimiseJobs")                 \
    V(ThrowerString,                  THROWER_STRING_INDEX,                  "Thrower")                     \
    V(IdentityString,                 IDENTITY_STRING_INDEX,                 "Identity")                    \
    V(CallerString,                   CALLER_STRING_INDEX,                   "caller")                      \
    V(CalleeString,                   CALLEE_STRING_INDEX,                   "callee")                      \
    V(Int8ArrayString,                INT8_ARRAY_STRING_INDEX,               "Int8Array")                   \
    V(Uint8ArrayString,               UINT8_ARRAY_STRING_INDEX,              "Uint8Array")                  \
    V(Uint8ClampedArrayString,        UINT8_CLAMPED_ARRAY_STRING_INDEX,      "Uint8ClampedArray")           \
    V(Int16ArrayString,               INT16_ARRAY_STRING_INDEX,              "Int16Array")                  \
    V(Uint16ArrayString,              UINT16_ARRAY_STRING_INDEX,             "Uint16Array")                 \
    V(Int32ArrayString,               INT32_ARRAY_STRING_INDEX,              "Int32Array")                  \
    V(Uint32ArrayString,              UINT32_ARRAY_STRING_INDEX,             "Uint32Array")                 \
    V(Float32ArrayString,             FLOAT32_ARRAY_STRING_INDEX,            "Float32Array")                \
    V(Float64ArrayString,             FLOAT64_ARRAY_STRING_INDEX,            "Float64Array")                \
    V(BigInt64ArrayString,            BIGINT64_ARRAY_STRING_INDEX,           "BigInt64Array")               \
    V(BigUint64ArrayString,           BIGUINT64_ARRAY_STRING_INDEX,          "BigUint64Array")              \
    V(SharedInt8ArrayString,          SHARED_INT8_ARRAY_STRING_INDEX,        "SharedInt8Array")             \
    V(SharedUint8ArrayString,         SHARED_UINT8_ARRAY_STRING_INDEX,       "SharedUint8Array")            \
    V(SharedUint8ClampedArrayString,  SHARED_UINT8_CLAMPED_ARRAY_STRING_INDEX, "SharedUint8ClampedArray")   \
    V(SharedInt16ArrayString,         SHARED_INT16_ARRAY_STRING_INDEX,       "SharedInt16Array")            \
    V(SharedUint16ArrayString,        SHARED_UINT16_ARRAY_STRING_INDEX,      "SharedUint16Array")           \
    V(SharedInt32ArrayString,         SHARED_INT32_ARRAY_STRING_INDEX,       "SharedInt32Array")            \
    V(SharedUint32ArrayString,        SHARED_UINT32_ARRAY_STRING_INDEX,      "SharedUint32Array")           \
    V(SharedFloat32ArrayString,       SHARED_FLOAT32_ARRAY_STRING_INDEX,     "SharedFloat32Array")          \
    V(SharedFloat64ArrayString,       SHARED_FLOAT64_ARRAY_STRING_INDEX,     "SharedFloat64Array")          \
    V(SharedBigInt64ArrayString,      SHARED_BIGINT64_ARRAY_STRING_INDEX,    "SharedBigInt64Array")         \
    V(SharedBigUint64ArrayString,     SHARED_BIGUINT64_ARRAY_STRING_INDEX,   "SharedBigUint64Array")        \
    V(AsyncFunctionString,            ASYNC_FUNCTION_STRING_INDEX,           "AsyncFunction")               \
    V(PromiseResolveString,           PROMISE_RESOLVE_STRING_INDEX,          "resolve")                     \
    V(IdString,                       ID_STRING_INDEX,                       "id")                          \
    V(MethodString,                   METHOD_STRING_INDEX,                   "method")                      \
    V(ParamsString,                   PARAMS_STRING_INDEX,                   "params")                      \
    V(ResultString,                   RESULT_STRING_INDEX,                   "result")                      \
    V(ToJsonString,                   TO_JSON_STRING_INDEX,                  "toJSON")                      \
    V(GlobalString,                   GLOBAL_STRING_INDEX,                   "global")                      \
    V(MessageString,                  MESSAGE_STRING_INDEX,                  "message")                     \
    V(CauseString,                    CAUSE_STRING_INDEX,                    "cause")                       \
    V(ErrorString,                    ERROR_STRING_INDEX,                    "Error")                       \
    V(ErrorsString,                   ERRORS_STRING_INDEX,                   "errors")                      \
    V(AggregateErrorString,           AGGREGATE_ERROR_STRING_INDEX,          "AggregateError")              \
    V(RangeErrorString,               RANGE_ERROR_STRING_INDEX,              "RangeError")                  \
    V(ReferenceErrorString,           REFERENCE_ERROR_STRING_INDEX,          "ReferenceError")              \
    V(TypeErrorString,                TYPE_ERROR_STRING_INDEX,               "TypeError")                   \
    V(URIErrorString,                 URI_ERROR_STRING_INDEX,                "URIError")                    \
    V(SyntaxErrorString,              SYNTAX_ERROR_STRING_INDEX,             "SyntaxError")                 \
    V(EvalErrorString,                EVAL_ERROR_STRING_INDEX,               "EvalError")                   \
    V(OOMErrorString,                 OOM_ERROR_STRING_INDEX,                "OutOfMemoryError")            \
    V(TerminationErrorString,         TERMINATION_ERROR_STRING_INDEX,        "TerminationError")            \
    V(ErrorFuncString,                ERROR_FUNC_STRING_INDEX,               "errorfunc")                   \
    V(StackString,                    STACK_STRING_INDEX,                    "stack")                       \
    V(TopStackString,                 TOP_STACK_STRING_INDEX,                "topstack")                    \
    V(StackEmptyString,               STACK_EMPTY_STRING_INDEX,              "stackisempty")                \
    V(ObjNotCoercibleString,          OBJ_NOT_COERCIBLE_STRING_INDEX,        "objectnotcoercible")          \
    /* for Intl. */                                                                                         \
    V(LanguageString,                 LANGUAGE_STRING_CLASS_INDEX,           "language")                    \
    V(ScriptString,                   SCRIPT_STRING_CLASS_INDEX,             "script")                      \
    V(RegionString,                   REGION_STRING_CLASS_INDEX,             "region")                      \
    V(BaseNameString,                 BASE_NAME_STRING_CLASS_INDEX,          "baseName")                    \
    V(CalendarString,                 CALENDAR_STRING_CLASS_INDEX,           "calendar")                    \
    V(CollationString,                COLLATION_STRING_CLASS_INDEX,          "collation")                   \
    V(HourCycleString,                HOUR_CYCLE_STRING_CLASS_INDEX,         "hourCycle")                   \
    V(CaseFirstString,                CASE_FIRST_STRING_CLASS_INDEX,         "caseFirst")                   \
    V(NumericString,                  NUMERIC_STRING_CLASS_INDEX,            "numeric")                     \
    V(NumberingSystemString,          NUMBERING_SYSTEM_STRING_CLASS_INDEX,   "numberingSystem")             \
    V(TypeString,                     TYPE_STRING_INDEX,                     "type")                        \
    V(GranularityString,              GRANULARITY_STRING_INDEX,              "granularity")                 \
    V(GraphemeString,                 GRAPHEME_STRING_INDEX,                 "grapheme")                    \
    V(WordString,                     WORD_STRING_INDEX,                     "word")                        \
    V(SentenceString,                 SENTENCE_STRING_INDEX,                 "sentence")                    \
    V(SegmentString,                  SEGMENT_STRING_INDEX,                  "segment")                     \
    V(IsWordLikeString,               ISWORDLIKE_STRING_INDEX,               "isWordLike")                  \
    V(LocaleMatcherString,            LOCALE_MATCHER_STRING_INDEX,           "localeMatcher")               \
    V(LanguageDisplayString,          LAUGUAGE_DISPLAY_STRING_INDEX,         "languageDisplay")             \
    V(FormatMatcherString,            FORMAT_MATCHER_STRING_INDEX,           "formatMatcher")               \
    V(Hour12String,                   HOUR12_STRING_INDEX,                   "hour12")                      \
    V(H11String,                      H11_STRING_INDEX,                      "h11")                         \
    V(H12String,                      H12_STRING_INDEX,                      "h12")                         \
    V(H23String,                      H23_STRING_INDEX,                      "h23")                         \
    V(H24String,                      H24_STRING_INDEX,                      "h24")                         \
    V(WeekdayString,                  WEEK_DAY_STRING_INDEX,                 "weekday")                     \
    V(EraString,                      ERA_STRING_INDEX,                      "era")                         \
    V(YearString,                     YEAR_STRING_INDEX,                     "year")                        \
    V(QuarterString,                  QUARTER_STRING_INDEX,                  "quarter")                     \
    V(MonthString,                    MONTH_STRING_INDEX,                    "month")                       \
    V(DayString,                      DAY_STRING_INDEX,                      "day")                         \
    V(HourString,                     HOUR_STRING_INDEX,                     "hour")                        \
    V(MinuteString,                   MINUTE_STRING_INDEX,                   "minute")                      \
    V(SecondString,                   SECOND_STRING_INDEX,                   "second")                      \
    V(YearsString,                    YEARS_STRING_INDEX,                    "years")                       \
    V(QuartersString,                 QUARTERS_STRING_INDEX,                 "quarters")                    \
    V(MonthsString,                   MONTHS_STRING_INDEX,                   "months")                      \
    V(DaysString,                     DAYS_STRING_INDEX,                     "days")                        \
    V(HoursString,                    HOURS_STRING_INDEX,                    "hours")                       \
    V(MinutesString,                  MINUTES_STRING_INDEX,                  "minutes")                     \
    V(SecondsString,                  SECONDS_STRING_INDEX,                  "seconds")                     \
    V(TimeZoneNameString,             TIME_ZONE_NAME_STRING_INDEX,           "timeZoneName")                \
    V(LocaleString,                   LOCALE_STRING_INDEX,                   "locale")                      \
    V(TimeZoneString,                 TIME_ZONE_STRING_INDEX,                "timeZone")                    \
    V(LiteralString,                  LITERAL_STRING_INDEX,                  "literal")                     \
    V(YearNameString,                 YEAR_NAME_STRING_INDEX,                "yearName")                    \
    V(DayPeriodString,                DAY_PERIOD_STRING_INDEX,               "dayPeriod")                   \
    V(FractionalSecondDigitsString,   FRACTIONAL_SECOND_DIGITS_STRING_INDEX, "fractionalSecondDigits")      \
    V(FractionalSecondString,         FRACTIONAL_SECOND_STRING_INDEX,        "fractionalSecond")            \
    V(RelatedYearString,              RELATED_YEAR_STRING_INDEX,             "relatedYear")                 \
    V(LookUpString,                   LOOK_UP_STRING_INDEX,                  "lookup")                      \
    V(BestFitString,                  BEST_FIT_STRING_INDEX,                 "bestfit")                     \
    V(DateStyleString,                DATE_STYLE_STRING_INDEX,               "dateStyle")                   \
    V(TimeStyleString,                TIME_STYLE_STRING_INDEX,               "timeStyle")                   \
    V(UTCString,                      UTC_STRING_INDEX,                      "UTC")                         \
    V(WeekString,                     WEEK_STRING_INDEX,                     "week")                        \
    V(WeeksString,                    WEEKS_STRING_INDEX,                    "weeks")                       \
    V(SourceString,                   SOURCE_STRING_INDEX,                   "source")                      \
    V(FormatString,                   FORMAT_STRING_INDEX,                   "format")                      \
    V(EnUsString,                     EN_US_STRING_INDEX,                    "en-US")                       \
    V(UndString,                      UND_STRING_INDEX,                      "und")                         \
    V(LatnString,                     LATN_STRING_INDEX,                     "latn")                        \
    V(StyleString,                    STYLE_STRING_INDEX,                    "style")                       \
    V(UnitString,                     UNIT_STRING_INDEX,                     "unit")                        \
    V(IntegerString,                  INTEGER_STRING_INDEX,                  "integer")                     \
    V(NanString,                      NAN_STRING_INDEX,                      "nan")                         \
    V(InfinityString,                 INFINITY_STRING_INDEX,                 "infinity")                    \
    V(InfinityCapitalString,          INFINITY_CAP_STRING_INDEX,             "Infinity")                    \
    V(MinusInfinityCapitalString,     MINUS_INFINITY_CAP_STRING_INDEX,       "-Infinity")                   \
    V(FractionString,                 FRACTION_STRING_INDEX,                 "fraction")                    \
    V(DecimalString,                  DECIMAL_STRING_INDEX,                  "decimal")                     \
    V(GroupString,                    GROUP_STRING_INDEX,                    "group")                       \
    V(GroupsString,                   GROUPS_STRING_INDEX,                   "groups")                      \
    V(CurrencyString,                 CURRENCY_STRING_INDEX,                 "currency")                    \
    V(CurrencySignString,             CURRENCY_SIGN_STRING_INDEX,            "currencySign")                \
    V(CurrencyDisplayString,          CURRENCY_DISPLAY_STRING_INDEX,         "currencyDisplay")             \
    V(PercentSignString,              PERCENT_SIGN_STRING_INDEX,             "percentSign")                 \
    V(PercentString,                  PERCENT_STRING_INDEX,                  "percent")                     \
    V(MinusSignString,                MINUS_SIGN_STRING_INDEX,               "minusSign")                   \
    V(PlusSignString,                 PLUS_SIGN_STRING_INDEX,                "plusSign")                    \
    V(ExponentSeparatorString,        EXPONENT_SEPARATOR_STRING_INDEX,       "exponentSeparator")           \
    V(ExponentMinusSignString,        EXPONENT_MINUS_SIGN_INDEX,             "exponentMinusSign")           \
    V(ExponentIntegerString,          EXPONENT_INTEGER_STRING_INDEX,         "exponentInteger")             \
    V(LongString,                     LONG_STRING_INDEX,                     "long")                        \
    V(ShortString,                    SHORT_STRING_INDEX,                    "short")                       \
    V(FullString,                     FULL_STRING_INDEX,                     "full")                        \
    V(MediumString,                   MEDIUM_STRING_INDEX,                   "medium")                      \
    V(NarrowString,                   NARROW_STRING_INDEX,                   "narrow")                      \
    V(AlwaysString,                   ALWAYS_STRING_INDEX,                   "always")                      \
    V(AutoString,                     AUTO_STRING_INDEX,                     "auto")                        \
    V(ThrowString,                    THROW_STRING_INDEX,                    "throw")                       \
    V(UnitDisplayString,              UNIT_DISPLAY_INDEX,                    "unitDisplay")                 \
    V(NotationString,                 NOTATION_INDEX,                        "notation")                    \
    V(CompactDisplayString,           COMPACT_DISPALY_INDEX,                 "compactDisplay")              \
    V(UserGroupingString,             USER_GROUPING_INDEX,                   "useGrouping")                 \
    V(SignDisplayString,              SIGN_DISPLAY_INDEX,                    "signDisplay")                 \
    V(CodeString,                     CODE_INDEX,                            "code")                        \
    V(NarrowSymbolString,             NARROW_SYMBOL_INDEX,                   "narrowSymbol")                \
    V(StandardString,                 STANDARD_INDEX,                        "standard")                    \
    V(DialectString,                  DIALECT_INDEX,                         "dialect")                     \
    V(AccountingString,               ACCOUNTING_INDEX,                      "accounting")                  \
    V(ScientificString,               SCIENTIFIC_INDEX,                      "scientific")                  \
    V(EngineeringString,              ENGINEERING_INDEX,                     "engineering")                 \
    V(CompactString,                  COMPACT_STRING_INDEX,                  "compact")                     \
    V(NeverString,                    NEVER_INDEX,                           "never")                       \
    V(ExceptZeroString,               EXPECT_ZERO_INDEX,                     "exceptZero")                  \
    V(MinimumIntegerDigitsString,     MINIMUM_INTEGER_DIGITS_INDEX,          "minimumIntegerDigits")        \
    V(MinimumFractionDigitsString,    MINIMUM_FRACTIONDIGITS_INDEX,          "minimumFractionDigits")       \
    V(MaximumFractionDigitsString,    MAXIMUM_FRACTIONDIGITS_INDEX,          "maximumFractionDigits")       \
    V(MinimumSignificantDigitsString, MINIMUM_SIGNIFICANTDIGITS_INDEX,       "minimumSignificantDigits")    \
    V(MaximumSignificantDigitsString, MAXIMUM_SIGNIFICANTDIGITS_INDEX,       "maximumSignificantDigits")    \
    V(InvalidDateString,              INVALID_DATE_INDEX,                    "Invalid Date")                \
    V(UsageString,                    USAGE_INDEX,                           "usage")                       \
    V(CompareString,                  COMPARE_INDEX,                         "compare")                     \
    V(SensitivityString,              SENSITIVITY_INDEX,                     "sensitivity")                 \
    V(IgnorePunctuationString,        IGNORE_PUNCTUATION_INDEX,              "ignorePunctuation")           \
    V(CardinalString,                 CARDINAL_INDEX,                        "cardinal")                    \
    V(OrdinalString,                  ORDINAL_INDEX,                         "ordinal")                     \
    V(ExecString,                     EXEC_INDEX,                            "exec")                        \
    V(LastIndexString,                LAST_INDEX_INDEX,                      "lastIndex")                   \
    V(PluralCategoriesString,         PLURAL_CATEGORIES_INDEX,               "pluralCategories")            \
    V(SortString,                     SORT_INDEX,                            "sort")                        \
    V(SearchString,                   SEARCH_INDEX,                          "search")                      \
    V(BaseString,                     BASE_INDEX,                            "base")                        \
    V(AccentString,                   ACCENT_INDEX,                          "accent")                      \
    V(CaseString,                     CASE_INDEX,                            "case")                        \
    V(VariantString,                  VARIANT_INDEX,                         "variant")                     \
    V(EnUsPosixString,                EN_US_POSIX_STRING_INDEX,              "en-US-POSIX")                 \
    V(UpperString,                    UPPER_INDEX,                           "upper")                       \
    V(LowerString,                    LOWER_INDEX,                           "lower")                       \
    V(DefaultString,                  DEFAULT_INDEX,                         "default")                     \
    V(NotInitializedString,           NOT_INITIALIZED_INDEX,                 " is not initialized")         \
    V(SharedString,                   SHARED_INDEX,                          "shared")                      \
    V(StartRangeString,               START_RANGE_INDEX,                     "startRange")                  \
    V(EndRangeString,                 END_RANGE_INDEX,                       "endRange")                    \
    V(Iso8601String,                  ISO8601_INDEX,                         "iso8601")                     \
    V(GregoryString,                  GREGORY_INDEX,                         "gregory")                     \
    V(EthioaaString,                  ETHIOAA_INDEX,                         "ethioaa")                     \
    V(StickyString,                   STICKY_INDEX,                          "sticky")                      \
    V(HasIndicesString,               HAS_INDICES_INDEX,                     "hasIndices")                  \
    V(DotAllString,                   DOT_ALL_INDEX,                         "dotAll")                      \
    V(MultilineString,                MULTILINE_INDEX,                       "multiline")                   \
    V(IgnoreCaseString,               IGNORE_CASE_INDEX,                     "ignoreCase")                  \
    V(IndicesString,                  INDICES_INDEX,                         "indices")                     \
    V(UString,                        U_INDEX,                               "u")                           \
    V(IndexString,                    INDEX_INDEX,                           "index")                       \
    V(InputString,                    INPUT_INDEX,                           "input")                       \
    V(UnicodeString,                  UNICODE_INDEX,                         "unicode")                     \
    V(ZeroString,                     ZERO_INDEX,                            "0")                           \
    V(ValuesString,                   VALUES_INDEX,                          "values")                      \
    V(AddString,                      ADD_INDEX,                             "add")                         \
    V(AmbiguousString,                AMBIGUOUS_INDEX,                       "ambiguous")                   \
    V(ModuleString,                   MODULE_INDEX,                          "Module")                      \
    V(StarString,                     STAR_INDEX,                            "*")                           \
    V(DateTimeFieldString,            DATETIMEFIELD_INDEX,                   "dateTimeField")               \
    V(ConjunctionString,              CONJUNCTION_INDEX,                     "conjunction")                 \
    V(NoneString,                     NONE_INDEX,                            "none")                        \
    V(FallbackString,                 FALLBACK_INDEX,                        "fallback")                    \
    V(DisjunctionString,              DISJUNCTION_INDEX,                     "disjunction")                 \
    V(ElementString,                  ELEMENT_INDEX,                         "element")                     \
    V(FlagsString,                    FLAGS_INDEX,                           "flags")                       \
    V(GString,                        G_INDEX,                               "g")                           \
    V(NfcString,                      NFC_INDEX,                             "NFC")                         \
    V(NfdString,                      NFD_INDEX,                             "NFD")                         \
    V(NfkcString,                     NFKC_INDEX,                            "NFKC")                        \
    V(NfkdString,                     NFKD_INDEX,                            "NFKD")                        \
    V(EntriesString,                  ENTRIES_INDEX,                         "entries")                     \
    V(LeftSquareBracketString,        LEFT_SQUARE_BRACKET_INDEX,             "[")                           \
    V(RightSquareBracketString,       RIGHT_SQUARE_BRACKET_INDEX,            "]")                           \
    V(YString,                        Y_INDEX,                               "y")                           \
    V(DollarString,                   DOLLAR_INDEX,                          "$")                           \
    V(CommaString,                    COMMA_INDEX,                           ",")                           \
    V(JoinString,                     JOIN_INDEX,                            "join")                        \
    V(CopyWithinString,               COPY_WITHIN_INDEX,                     "copyWithin")                  \
    V(FillString,                     FILL_INDEX,                            "fill")                        \
    V(FindString,                     FIND_INDEX,                            "find")                        \
    V(FindIndexString,                FIND_INDEX_INDEX,                      "findIndex")                   \
    V(FlatString,                     FLAT_INDEX,                            "flat")                        \
    V(FlatMapString,                  FLATMAP_INDEX,                         "flatMap")                     \
    V(IncludesString,                 INCLUDES_INDEX,                        "includes")                    \
    V(KeysString,                     KEYS_INDEX,                            "keys")                        \
    V(BoundString,                    BOUND_INDEX,                           "bound")                       \
    V(TrimLeftString,                 TRIMLEFT_INDEX,                        "trimLeft")                    \
    V(TrimStartString,                TRIMSTART_INDEX,                       "trimStart")                   \
    V(TrimRightString,                TRIMRIGHT_INDEX,                       "trimRight")                   \
    V(TrimEndString,                  TRIMEND_INDEX,                         "trimEnd")                     \
    V(BackslashString,                BACKSLASH_INDEX,                       "/")                           \
    V(SpaceString,                    SPACE_INDEX,                           " ")                           \
    V(NanCapitalString,               NAN_INDEX,                             "NaN")                         \
    V(NotEqualString,                 NOT_EQUAL_INDEX,                       "not-equal")                   \
    V(OkString,                       OK_INDEX,                              "ok")                          \
    V(TimeoutString,                  TIMEOUT_INDEX,                         "timed-out")                   \
    V(CjsExportsString,               CJS_EXPORTS_INDEX,                     "exports")                     \
    V(CjsCacheString,                 CJS_CACHE_INDEX,                       "_cache")                      \
    V(NapiWrapperString,              NAPI_WRAPPER_INDEX,                    "_napiwrapper")                \
    /* for require native module */                                                                         \
    V(RequireNativeModuleString,      REQUIRE_NATIVE_MOUDULE_FUNC_INDEX,     "requireNativeModule")         \
    V(RequireNapiString,              REQUIRE_NAPI_FUNC_INDEX,               "requireNapi")                 \
    V(DollarStringOne,                DOLLAR_STRING_ONE_INDEX,               "$1")                          \
    V(DollarStringTwo,                DOLLAR_STRING_TWO_INDEX,               "$2")                          \
    V(DollarStringThree,              DOLLAR_STRING_THREE_INDEX,             "$3")                          \
    V(DollarStringFour,               DOLLAR_STRING_FOUR_INDEX,              "$4")                          \
    V(DollarStringFive,               DOLLAR_STRING_FIVE_INDEX,              "$5")                          \
    V(DollarStringSix,                DOLLAR_STRING_SIX_INDEX,               "$6")                          \
    V(DollarStringSeven,              DOLLAR_STRING_SEVEN_INDEX,             "$7")                          \
    V(DollarStringEight,              DOLLAR_STRING_EIGHT_INDEX,             "$8")                          \
    V(DollarStringNine,               DOLLAR_STRING_NINE_INDEX,              "$9")                          \
    /* for object to string */                                                                              \
    V(UndefinedToString,              UNDEFINED_TO_STRING_INDEX,             "[object Undefined]")          \
    V(NullToString,                   NULL_TO_STRING_INDEX,                  "[object Null]")               \
    V(ObjectToString,                 OBJECT_TO_STRING_INDEX,                "[object Object]")             \
    V(ArrayToString,                  ARRAY_TO_STRING_INDEX,                 "[object Array]")              \
    V(StringToString,                 STRING_TO_STRING_INDEX,                "[object String]")             \
    V(BooleanToString,                BOOLEAN_TO_STRING_INDEX,               "[object Boolean]")            \
    V(NumberToString,                 NUMBER_TO_STRING_INDEX,                "[object Number]")             \
    V(ArgumentsToString,              ARGUMENTS_TO_STRING_INDEX,             "[object Arguments]")          \
    V(FunctionToString,               FUNCTION_TO_STRING_INDEX,              "[object Function]")           \
    V(DateToString,                   DATE_TO_STRING_INDEX,                  "[object Date]")               \
    V(ErrorToString,                  ERROR_TO_STRING_INDEX,                 "[object Error]")              \
    V(RegExpToString,                 REGEXP_TO_STRING_INDEX,                "[object RegExp]")             \
    V(YoungGcCause,                   YOUNG_GC_CAUSE,                        "young")                       \
    V(OldGcCause,                     OLD_GC_CAUSE,                          "old")                         \
    V(FullGcCause,                    FULL_GC_CAUSE,                         "full")                        \
    V(AppSpawnFullGcCause,            APP_SPAWN_FULL_GC_CAUSE,               "app_spawn_full")              \
    V(SharedGcCause,                  SHARED_GC_CAUSE,                       "shared")                      \
    V(SharedPartialGcCause,           SHARED_PARTIAL_GC_CAUSE,               "shared_partial")              \
    V(SharedFullGcCause,              SHARED_FULL_GC_CAUSE,                  "shared_full")                 \
    V(AppSpawnSharedFullGcCause,      APP_SPAWN_SHARED_FULL_GC_CAUSE,        "app_spawn_shared_full")       \
    V(SymbolLeftParentheses,          SYMBOL_LEFT_PARENTHESES,               "Symbol(")

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SHARED_GLOBAL_ENV_CONSTANT_ACCESSOR(V)                                                    \
    V(JSTaggedValue, FunctionPrototypeAccessor, FUNCTION_PROTOTYPE_ACCESSOR, ecma_roots_accessor) \
    V(JSTaggedValue, FunctionNameAccessor, FUNCTION_NAME_ACCESSOR, ecma_roots_accessor)           \
    V(JSTaggedValue, FunctionLengthAccessor, FUNCTION_LENGTH_ACCESSOR, ecma_roots_accessor)       \
    V(JSTaggedValue, ArrayLengthAccessor, ARRAY_LENGTH_ACCESSOR, ecma_roots_accessor)             \
    V(JSTaggedValue, SharedArrayLengthAccessor, SHARED_ARRAY_LENGTH_ACCESSOR, ecma_roots_accessor)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SHARED_GLOBAL_ENV_CONSTANT_SPECIAL(V)                                                                   \
    V(JSTaggedValue, Undefined, UNDEFINED_INDEX, ecma_roots_special)                                            \
    V(JSTaggedValue, Null, NULL_INDEX, ecma_roots_special)                                                      \
    V(JSTaggedValue, Hole, HOLE_INDEX, ecma_roots_special)                                                      \
    V(JSTaggedValue, True, TRUE_INDEX, ecma_roots_special)                                                      \
    V(JSTaggedValue, False, FALSE_INDEX, ecma_roots_special)                                                    \
    V(JSTaggedValue, ProtoTransRootHClassSymbol, PROTO_TRANS_ROOT_HCLASS_SYMBOL_INDEX, ecma_roots_special)      \
    V(JSTaggedValue, EmptyString, EMPTY_STRING_OBJECT_INDEX, ecma_roots_special)                                \
    V(JSTaggedValue, SingleCharTable, SINGLE_CHAR_TABLE_INDEX, ecma_roots_special)                              \
    V(JSTaggedValue, EmptySLayoutInfo, EMPTY_SLAYOUT_INFO_OBJECT_INDEX, ecma_roots_special)                     \
    V(JSTaggedValue, EmptyArray, EMPTY_ARRAY_OBJECT_INDEX, ecma_roots_special)                                  \
    V(JSTaggedValue, EmptySFunctionEnv, EMPTY_SFUNCTION_ENV_INDEX, ecma_roots_special)                          \
    V(JSTaggedValue, EmptyMutantArray, EMPTY_MUTANT_ARRAY_OBJECT_INDEX, ecma_roots_special)                     \
    V(JSTaggedValue, ProxyMethod, PROXY_METHOD_INDEX, ecma_roots_special)                                       \
    V(JSTaggedValue, Uint64MaxBigInt, UINT64_MAX_BIGINT_INDEX, ecma_roots_special)                              \
    V(JSTaggedValue, Int64MaxBigInt, INT64_MAX_BIGINT_INDEX, ecma_roots_special)                                \
    V(JSTaggedValue, EmptyProfileTypeInfoCell, EMPTY_PROFILE_TYPE_INFO_CELL_INDEX, ecma_roots_special)          \
    V(JSTaggedValue, NormalFunctionMethod, NORMAL_FUNCTION_METHOD_INDEX, ecma_roots_special)                    \
    V(JSTaggedValue, GetterFunctionMethod, GETTER_FUNCTION_METHOD_INDEX, ecma_roots_special)                    \
    V(JSTaggedValue, SetterFunctionMethod, SETTER_FUNCTION_METHOD_INDEX, ecma_roots_special)                    \
    V(JSTaggedValue, ArrowFunctionMethod, ARROW_FUNCTION_METHOD_INDEX, ecma_roots_special)                      \
    V(JSTaggedValue, AsyncArrowFunctionMethod, ASYNC_ARROW_FUNCTION_METHOD_INDEX, ecma_roots_special)           \
    V(JSTaggedValue, ConcurrentFunctionMethod, CONCURRENT_FUNCTION_METHOD_INDEX, ecma_roots_special)            \
    V(JSTaggedValue, AsyncFunctionMethod, ASYNC_FUNCTION_METHOD_INDEX, ecma_roots_special)                      \
    V(JSTaggedValue, BaseConstructorMethod, BASE_CONSTRUCTOR_METHOD_INDEX, ecma_roots_special)                  \
    V(JSTaggedValue, ClassConstructorMethod, CLASS_CONSTRUCTOR_METHOD_INDEX, ecma_roots_special)                \
    V(JSTaggedValue, BuiltinProxyConstructorMethod, BUILTIN_PROXY_CONSTRUCTOR_METHOD_INDEX, ecma_roots_special) \
    V(JSTaggedValue, BuiltinConstructorMethod, BUILTIN_CONSTRUCTOR_METHOD_INDEX, ecma_roots_special)            \
    V(JSTaggedValue, DerivedConstructorMethod, DERIVED_CONSTRUCTOR_METHOD_INDEX, ecma_roots_special)            \
    V(JSTaggedValue, GeneratorFunctionMethod, GENERATOR_FUNCTION_METHOD_INDEX, ecma_roots_special)              \
    V(JSTaggedValue, NoneFunctionMethod, NONE_FUNCTION_METHOD_INDEX, ecma_roots_special)                        \
    V(JSTaggedValue, AsyncGeneratorFunctionMethod, ASYNC_GENERATOR_FUNCTION_METHOD_INDEX, ecma_roots_special)   \
    V(JSTaggedValue, BoundFunctionMethod, BOUND_FUNCTION_METHOD_INDEX, ecma_roots_special)

#define GLOBAL_ENV_CACHES(V)                \
    V(JSTaggedValue, CachedJSCollatorLocales, CACHED_JSCOLLATOR_LOCALES_INDEX, cachedCollatorLocales)

// ConstantIndex used for explicit visit each constant.
enum class ConstantIndex : size_t {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)

#define INDEX_FILTER_BUILTIN4(ARG1, ARG2, ARG3, Index)  Index##_INDEX,
#define INDEX_FILTER_BUILTIN6(ARG1, ARG2, ARG3, ARG4, ARG5, Index)  Index##_INDEX,
#define INDEX_FILTER_COMMON(Index) Index,
#define INDEX_FILTER_WITH_TYPE(Type, Name, Index, Desc) INDEX_FILTER_COMMON(Index)
#define INDEX_FILTER_STRING(Name, Index, Token) INDEX_FILTER_COMMON(Index)
    // Caveats: make sure shareds starts from 0 and placed before non-shareds.
    SHARED_GLOBAL_ENV_CONSTANT_CLASS(INDEX_FILTER_WITH_TYPE)
#ifdef USE_CMC_GC
    SHARED_GLOBAL_ENV_CONSTANT_SHARED_CLASS(INDEX_FILTER_WITH_TYPE)
#endif
    SHARED_GLOBAL_ENV_CONSTANT_STRING(INDEX_FILTER_STRING)
    SHARED_GLOBAL_ENV_CONSTANT_ACCESSOR(INDEX_FILTER_WITH_TYPE)
    SHARED_GLOBAL_ENV_CONSTANT_SPECIAL(INDEX_FILTER_WITH_TYPE)
    GLOBAL_ENV_CONSTANT_CLASS(INDEX_FILTER_WITH_TYPE)
    GLOBAL_ENV_CONSTANT_SPECIAL(INDEX_FILTER_WITH_TYPE)
    BUILTINS_METHOD_STUB_LIST(INDEX_FILTER_BUILTIN4, INDEX_FILTER_BUILTIN4,                       \
                              INDEX_FILTER_BUILTIN4, INDEX_FILTER_BUILTIN6)
    GLOBAL_ENV_INLINED_BUILTINS(INDEX_FILTER_WITH_TYPE)
    GLOBAL_ENV_CACHES(INDEX_FILTER_WITH_TYPE)
#undef INDEX_FILTER_STRING
#undef INDEX_FILTER_WITH_TYPE
#undef INDEX_FILTER_COMMON
#undef INDEX_FILTER_BUILTIN4
#undef INDEX_FILTER_BUILTIN6

    CONSTANT_COUNT,

    CONSTANT_BEGIN = 0,
    CONSTANT_END = CONSTANT_COUNT,

    SHARED_BEGIN = HCLASS_CLASS_INDEX,
    SHARED_END = BOUND_FUNCTION_METHOD_INDEX,

    SHARED_HCLASS_BEGIN = HCLASS_CLASS_INDEX,
    SHARED_HCLASS_END = VTABLE_CLASS_INDEX,
    NON_SHARED_HCLASS_BEGIN = JS_PROXY_CALLABLE_CLASS_INDEX,
    NON_SHARED_HCLASS_END = CLASS_CONSTRUCTOR_HCLASS_INDEX,
    READ_ONLY_CONSTANT_BEGIN = CONSTANT_BEGIN,
    READ_ONLY_CONSTANT_END = CONSTANT_END,

    INVALID = -1,
    // ...
};
// clang-format on

class GlobalEnvConstants {
public:
    GlobalEnvConstants() = default;
    ~GlobalEnvConstants() = default;

    DEFAULT_MOVE_SEMANTIC(GlobalEnvConstants);
    DEFAULT_COPY_SEMANTIC(GlobalEnvConstants);

    const JSTaggedValue *BeginSlot() const;

    const JSTaggedValue *EndSlot() const;

    void Init(JSThread *thread);

    void CopySharedConstantsFrom(const GlobalEnvConstants *src);

    void InitSpecialForSnapshot();

    void SetCachedLocales(JSTaggedValue value);

    void SetConstant(ConstantIndex index, JSTaggedValue value);

    template<typename T>
    void SetConstant(ConstantIndex index, JSHandle<T> value);

    uintptr_t GetGlobalConstantAddr(ConstantIndex index) const;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_GET_COMMON(Type, Name)                 \
    const Type Get##Name() const;                   \
    const JSHandle<Type> GetHandled##Name() const;  \
    static size_t GetOffsetOf##Name();
#define DECL_GET_BUILTIN(Method, Object, ...)  DECL_GET_COMMON(JSTaggedValue, Object##Method)
#define DECL_GET_WITH_TYPE(Type, Name, Index, Desc) DECL_GET_COMMON(Type, Name)
#define DECL_GET_STRING(Name, Index, Token) DECL_GET_COMMON(JSTaggedValue, Name)
    SHARED_GLOBAL_ENV_CONSTANT_CLASS(DECL_GET_WITH_TYPE)
#ifdef USE_CMC_GC
    SHARED_GLOBAL_ENV_CONSTANT_SHARED_CLASS(DECL_GET_WITH_TYPE)
#endif
    SHARED_GLOBAL_ENV_CONSTANT_STRING(DECL_GET_STRING)
    SHARED_GLOBAL_ENV_CONSTANT_ACCESSOR(DECL_GET_WITH_TYPE)
    SHARED_GLOBAL_ENV_CONSTANT_SPECIAL(DECL_GET_WITH_TYPE)
    GLOBAL_ENV_CONSTANT_CLASS(DECL_GET_WITH_TYPE)
    GLOBAL_ENV_CONSTANT_SPECIAL(DECL_GET_WITH_TYPE)
    BUILTINS_METHOD_STUB_LIST(DECL_GET_BUILTIN, DECL_GET_BUILTIN, DECL_GET_BUILTIN, DECL_GET_BUILTIN)
    GLOBAL_ENV_INLINED_BUILTINS(DECL_GET_WITH_TYPE)
    GLOBAL_ENV_CACHES(DECL_GET_WITH_TYPE)
#undef DECL_GET_STRING
#undef DECL_GET_WITH_TYPE
#undef DECL_GET_COMMON
#undef DECL_GET_BUILTIN

    void Iterate(RootVisitor &visitor)
    {
        visitor.VisitRangeRoot(Root::ROOT_VM, ObjectSlot(ToUintPtr(BeginSlot())), ObjectSlot(ToUintPtr(EndSlot())));
    }

    JSTaggedValue GetGlobalConstantObject(size_t index) const
    {
        ASSERT(static_cast<ConstantIndex>(index) < ConstantIndex::CONSTANT_COUNT);
        return constants_[index];
    }

    size_t GetConstantCount() const
    {
        return static_cast<size_t>(ConstantIndex::CONSTANT_COUNT);
    }

    size_t GetEmptyArrayIndex() const
    {
        return static_cast<size_t>(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    }

    size_t GetEmptyMutantArrayIndex() const
    {
        return static_cast<size_t>(ConstantIndex::EMPTY_MUTANT_ARRAY_OBJECT_INDEX);
    }

    size_t GetLineStringClassIndex() const
    {
        return static_cast<size_t>(ConstantIndex::LINE_STRING_CLASS_INDEX);
    }

    bool IsSpecialOrUndefined(size_t index) const
    {
        size_t specialBegin = static_cast<size_t>(ConstantIndex::UNDEFINED_INDEX);
        size_t specialEnd = static_cast<size_t>(ConstantIndex::NULL_INDEX);
        return (index >= specialBegin && index <= specialEnd);
    }

    static constexpr size_t SizeArch32 =
        JSTaggedValue::TaggedTypeSize() * static_cast<size_t>(ConstantIndex::CONSTANT_COUNT);
    static constexpr size_t SizeArch64 =
        JSTaggedValue::TaggedTypeSize() * static_cast<size_t>(ConstantIndex::CONSTANT_COUNT);

private:
    void InitSharedStrings(ObjectFactory *factory);
    void InitSharedRootsClasses(ObjectFactory *factory);
    void InitSharedMiscellaneous(JSThread *thread, ObjectFactory *factory);
    void InitRootsClasses(ObjectFactory *factory);
    void InitRootsClassesPartOne(JSHClass *hClass, ObjectFactory *factory);
    void InitRootsClassesPartTwo(JSHClass *hClass, ObjectFactory *factory);
    void InitMiscellaneous(JSThread *thread, ObjectFactory *factory);
    void InitGlobalCaches();

    JSTaggedValue constants_[static_cast<int>(ConstantIndex::CONSTANT_COUNT)];  // NOLINT(modernize-avoid-c-arrays)
};
STATIC_ASSERT_EQ_ARCH(sizeof(GlobalEnvConstants), GlobalEnvConstants::SizeArch32, GlobalEnvConstants::SizeArch64);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_GLOBAL_ENV_CONSTANTS_H
