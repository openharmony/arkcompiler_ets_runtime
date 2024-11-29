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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_H
#define ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_H

#include "ecmascript/base/config.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/global_env_constants.h"

namespace panda::ecmascript::kungfu {

#define PADDING_BUILTINS_STUB_LIST(V)               \
    V(NONE)

// BUILTINS_STUB_LIST is shared both ASM Interpreter and AOT.
// AOT_BUILTINS_STUB_LIST is used in AOT only.
#define BUILTINS_STUB_LIST(V, D, C)                 \
    BUILTINS_METHOD_STUB_LIST(D, D, D, D)           \
    BUILTINS_WITH_CONTAINERS_STUB_BUILDER(D)        \
    AOT_AND_BUILTINS_STUB_LIST(V)                   \
    BUILTINS_CONSTRUCTOR_STUB_LIST(C)               \
    BUILTINS_ARKTOOLS_STUB_BUILDER(D)

#define BUILTINS_NOSTUB_LIST(V)                     \
    V(ObjectConstructor)                            \
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

#define BUILTINS_WITH_STRING_STUB_BUILDER(V)                                            \
    V(CharAt,             String,   Hole())                                             \
    V(FromCharCode,       String,   Hole())                                             \
    V(CharCodeAt,         String,   DoubleToTaggedDoublePtr(Double(base::NAN_VALUE)))   \
    V(CodePointAt,        String,   Undefined())                                        \
    V(IndexOf,            String,   IntToTaggedPtr(Int32(-1)))                          \
    V(Substring,          String,   IntToTaggedPtr(Int32(-1)))                          \
    V(SubStr,             String,   Undefined())                                        \
    V(Replace,            String,   Undefined())                                        \
    V(Trim,               String,   Undefined())                                        \
    V(TrimStart,          String,   Undefined())                                        \
    V(TrimEnd,            String,   Undefined())                                        \
    V(TrimLeft,           String,   Undefined())                                        \
    V(TrimRight,          String,   Undefined())                                        \
    V(PadStart,           String,   Undefined())                                        \
    V(PadEnd,             String,   Undefined())                                        \
    V(Concat,             String,   Undefined())                                        \
    V(Slice,              String,   Undefined())                                        \
    V(ToLowerCase,        String,   Undefined())                                        \
    V(StartsWith,         String,   TaggedFalse())                                      \
    V(EndsWith,           String,   TaggedFalse())                                      \
    V(GetStringIterator,  String,   Undefined())

#define BUILTINS_WITH_OBJECT_STUB_BUILDER(V)                                      \
    V(ToString,                    Object,   Undefined())                         \
    V(Create,                      Object,   Undefined())                         \
    V(Assign,                      Object,   Undefined())                         \
    V(HasOwnProperty,              Object,   TaggedFalse())                       \
    V(Keys,                        Object,   Undefined())                         \
    V(GetPrototypeOf,              Object,   Undefined())                         \
    V(GetOwnPropertyNames,         Object,   Undefined())                         \
    V(GetOwnPropertySymbols,       Object,   Undefined())                         \
    V(Entries,                     Object,   Undefined())                         \
    V(IsFrozen,                    Object,   Undefined())                         \
    V(IsSealed,                    Object,   Undefined())                         \
    V(GetOwnPropertyDescriptors,   Object,   Undefined())                         \
    V(SetPrototypeOf,              Object,   Undefined())

#define BUILTINS_WITH_ARRAY_STUB_BUILDER(V)         \
    V(With,          Array,   Undefined())          \
    V(Unshift,       Array,   Undefined())          \
    V(Shift,         Array,   Undefined())          \
    V(Concat,        Array,   Undefined())          \
    V(Filter,        Array,   Undefined())          \
    V(Find,          Array,   Undefined())          \
    V(FindIndex,     Array,   Undefined())          \
    V(From,          Array,   Undefined())          \
    V(Splice,        Array,   Undefined())          \
    V(ToSpliced,     Array,   Undefined())          \
    V(ForEach,       Array,   Undefined())          \
    V(IndexOf,       Array,   Undefined())          \
    V(LastIndexOf,   Array,   Undefined())          \
    V(Pop,           Array,   Undefined())          \
    V(Slice,         Array,   Undefined())          \
    V(Reduce,        Array,   Undefined())          \
    V(Reverse,       Array,   Undefined())          \
    V(ToReversed,    Array,   Undefined())          \
    V(Push,          Array,   Undefined())          \
    V(Values,        Array,   Undefined())          \
    V(Includes,      Array,   Undefined())          \
    V(CopyWithin,    Array,   Undefined())          \
    V(Some,          Array,   Undefined())          \
    V(Fill,          Array,   Undefined())          \
    V(Every,         Array,   Undefined())          \
    V(FindLastIndex, Array,   Undefined())          \
    V(FindLast,      Array,   Undefined())          \
    V(ReduceRight,   Array,   Undefined())          \
    V(Map,           Array,   Undefined())          \
    V(FlatMap,       Array,   Undefined())          \
    V(ToSorted,      Array,   Undefined())          \
    V(IsArray,       Array,   Undefined())

#define BUILTINS_WITH_SET_STUB_BUILDER(V)           \
    V(Clear,    Set,   Undefined())                 \
    V(Values,   Set,   Undefined())                 \
    V(Entries,  Set,   Undefined())                 \
    V(ForEach,  Set,   Undefined())                 \
    V(Add,      Set,   Undefined())                 \
    V(Delete,   Set,   Undefined())                 \
    V(Has,      Set,   Undefined())

#define BUILTINS_WITH_MAP_STUB_BUILDER(V)           \
    V(Clear,    Map,   Undefined())                 \
    V(Values,   Map,   Undefined())                 \
    V(Entries,  Map,   Undefined())                 \
    V(Keys,     Map,   Undefined())                 \
    V(ForEach,  Map,   Undefined())                 \
    V(Set,      Map,   Undefined())                 \
    V(Delete,   Map,   Undefined())                 \
    V(Has,      Map,   Undefined())                 \
    V(Get,      Map,   Undefined())

#define BUILTINS_WITH_FUNCTION_STUB_BUILDER(V)      \
    V(PrototypeApply,  Function,  Undefined())      \
    V(PrototypeBind,   Function,  Undefined())      \
    V(PrototypeCall,   Function,  Undefined())

#define BUILTINS_WITH_NUMBER_STUB_BUILDER(V)        \
    V(ParseFloat,      Number,    Undefined())      \
    V(ParseInt,        Number,    Undefined())      \
    V(IsFinite,        Number,    Undefined())      \
    V(IsNaN,           Number,    Undefined())      \
    V(IsInteger,       Number,    Undefined())      \
    V(IsSafeInteger,   Number,    Undefined())      \
    V(ToString,        Number,    Undefined())

#define BUILTINS_WITH_REFLECT_STUB_BUILDER(V)       \
    V(Get,             Reflect,   Undefined())

#define BUILTINS_WITH_COLLATOR_STUB_BUILDER(V)      \
    V(ResolvedOptions, Collator,   Undefined())

#define BUILTINS_WITH_REGEXP_STUB_BUILDER(V)        \
    V(GetFlags,        RegExp,   Undefined())

#define BUILTINS_WITH_TYPEDARRAY_STUB_BUILDER(V)    \
    V(Reverse,         TypedArray,  Undefined())    \
    V(LastIndexOf,     TypedArray,  Undefined())    \
    V(IndexOf,         TypedArray,  Undefined())    \
    V(Find,            TypedArray,  Undefined())    \
    V(Includes,        TypedArray,  Undefined())    \
    V(CopyWithin,      TypedArray,  Undefined())    \
    V(ReduceRight,     TypedArray,  Undefined())    \
    V(Reduce,          TypedArray,  Undefined())    \
    V(Every,           TypedArray,  Undefined())    \
    V(Some,            TypedArray,  Undefined())    \
    V(Filter,          TypedArray,  Undefined())    \
    V(With,            TypedArray,  Undefined())    \
    V(Entries,         TypedArray,  Undefined())    \
    V(Keys,            TypedArray,  Undefined())    \
    V(Values,          TypedArray,  Undefined())    \
    V(Slice,           TypedArray,  Undefined())    \
    V(SubArray,        TypedArray,  Undefined())    \
    V(Sort,            TypedArray,  Undefined())    \
    V(GetByteLength,   TypedArray,  Undefined())    \
    V(GetByteOffset,   TypedArray,  Undefined())    \
    V(Set,             TypedArray,  Undefined())    \
    V(FindIndex,       TypedArray,  Undefined())    \
    V(FindLastIndex,   TypedArray,  Undefined())    \
    V(ToSorted,        TypedArray,  Undefined())    \
    V(Of,              TypedArray,  Undefined())    \
    V(Map,             TypedArray,  Undefined())    \
    V(ToReversed,      TypedArray,  Undefined())

#define BUILTINS_WITH_DATAVIEW_STUB_BUILDER(V)                           \
    V(SetInt32,     DataView,  INT32,     SetTypedValue, Undefined())    \
    V(SetFloat32,   DataView,  FLOAT32,   SetTypedValue, Undefined())    \
    V(SetFloat64,   DataView,  FLOAT64,   SetTypedValue, Undefined())

#define BUILTINS_WITH_CONTAINERS_STUB_BUILDER(V)                                                               \
    V(ForEach,            ArrayList,      ContainersCommonFuncCall,  ARRAYLIST_FOREACH,            JS_POINTER) \
    V(ForEach,            Deque,          DequeCommonFuncCall,       DEQUE_FOREACH,                JS_POINTER) \
    V(ForEach,            HashMap,        ContainersHashCall,        HASHMAP_FOREACH,              JS_POINTER) \
    V(ForEach,            HashSet,        ContainersHashCall,        HASHSET_FOREACH,              JS_POINTER) \
    V(ForEach,            LightWeightMap, ContainersLightWeightCall, LIGHTWEIGHTMAP_FOREACH,       JS_POINTER) \
    V(ForEach,            LightWeightSet, ContainersLightWeightCall, LIGHTWEIGHTSET_FOREACH,       JS_POINTER) \
    V(ForEach,            LinkedList,     ContainersLinkedListCall,  LINKEDLIST_FOREACH,           JS_POINTER) \
    V(ForEach,            List,           ContainersLinkedListCall,  LIST_FOREACH,                 JS_POINTER) \
    V(ForEach,            PlainArray,     ContainersCommonFuncCall,  PLAINARRAY_FOREACH,           JS_POINTER) \
    V(ForEach,            Queue,          QueueCommonFuncCall,       QUEUE_FOREACH,                JS_POINTER) \
    V(ForEach,            Stack,          ContainersCommonFuncCall,  STACK_FOREACH,                JS_POINTER) \
    V(ForEach,            Vector,         ContainersCommonFuncCall,  VECTOR_FOREACH,               JS_POINTER) \
    V(ReplaceAllElements, ArrayList,      ContainersCommonFuncCall,  ARRAYLIST_REPLACEALLELEMENTS, JS_POINTER) \
    V(ReplaceAllElements, Vector,         ContainersCommonFuncCall,  VECTOR_REPLACEALLELEMENTS,    JS_POINTER)

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

#define AOT_AND_BUILTINS_STUB_LIST(V)  \
    V(StringLocaleCompare)             \
    V(StringIteratorProtoNext)         \
    V(ArraySort)

#define AOT_BUILTINS_STUB_LIST(V)                       \
    V(JsonStringify)                                    \
    V(MapProtoIterator)                                 \
    V(MapIteratorProtoNext)                             \
    V(SetProtoIterator)                                 \
    V(SetIteratorProtoNext)                             \
    V(StringProtoIterator)                              \
    V(ArrayProtoIterator)                               \
    V(ArrayIteratorProtoNext)                           \
    V(TypeArrayProtoIterator)                           \
    V(IteratorProtoReturn)

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
    V(ReflectHas)                                   \
    V(ReflectConstruct)                             \
    V(ReflectApply)                                 \
    V(FunctionPrototypeHasInstance)                 \
    V(ArrayKeys)                                    \
    V(ArrayEntries)                                 \
    V(TYPED_BUILTINS_INLINE_FIRST = MathAcos)       \
    V(TYPED_BUILTINS_INLINE_LAST = ArrayEntries)

class BuiltinsStubCSigns {
public:
    enum ID {
#define DEF_STUB_ID(name) name,
#define DEF_STUB_ID_DYN(name, type, ...) type##name,
        PADDING_BUILTINS_STUB_LIST(DEF_STUB_ID)
        BUILTINS_STUB_LIST(DEF_STUB_ID, DEF_STUB_ID_DYN, DEF_STUB_ID)
        NUM_OF_BUILTINS_STUBS,
        BUILTINS_NOSTUB_LIST(DEF_STUB_ID)
        AOT_BUILTINS_STUB_LIST(DEF_STUB_ID)
        AOT_BUILTINS_INLINE_LIST(DEF_STUB_ID)
#undef DEF_STUB_ID_DYN
#undef DEF_STUB_ID
        NUM_OF_BUILTINS_ID,
        BUILTINS_CONSTRUCTOR_STUB_FIRST = BooleanConstructor,
        TYPED_BUILTINS_FIRST = JsonStringify,
        TYPED_BUILTINS_LAST = IteratorProtoReturn,
        INVALID = 0xFFFF,
    };
    static_assert(ID::NONE == 0);

    static void Initialize();

    static void GetCSigns(std::vector<const CallSignature*>& callSigns);

    static const CallSignature *Get(size_t index)
    {
        ASSERT(index < NUM_OF_BUILTINS_STUBS);
        return &callSigns_[index];
    }

    static const std::string &GetName(int index)
    {
        ASSERT(index < NUM_OF_BUILTINS_STUBS);
        return callSigns_[index].GetName();
    }

    static const CallSignature* BuiltinsCSign()
    {
        return &builtinsCSign_;
    }

    static const CallSignature* BuiltinsWithArgvCSign()
    {
        return &builtinsWithArgvCSign_;
    }

    static bool IsFastBuiltin(ID builtinId)
    {
        return builtinId > NONE && builtinId < NUM_OF_BUILTINS_STUBS;
    }

    static bool IsTypedBuiltin(ID builtinId)
    {
        return (BuiltinsStubCSigns::ID::StringLocaleCompare == builtinId) ||
               (BuiltinsStubCSigns::ID::StringIteratorProtoNext == builtinId) ||
               (BuiltinsStubCSigns::ID::ArraySort == builtinId) ||
               (BuiltinsStubCSigns::ID::GlobalDecodeURIComponent == builtinId) ||
               ((BuiltinsStubCSigns::ID::TYPED_BUILTINS_FIRST <= builtinId) &&
               (builtinId <= BuiltinsStubCSigns::ID::TYPED_BUILTINS_LAST));
    }

    static bool IsTypedInlineBuiltin(ID builtinId)
    {
        if (TYPED_BUILTINS_INLINE_FIRST <= builtinId && builtinId <= TYPED_BUILTINS_INLINE_LAST) {
            return true;
        }
        if (BuiltinsStubCSigns::ID::DataViewSetInt32 <= builtinId &&
            builtinId <= BuiltinsStubCSigns::ID::DataViewSetFloat64) {
            return true;
        }
        // NOTE(schernykh): try to remove this switch and move StringFromCharCode to TYPED_BUILTINS_INLINE list
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::ArraySort:
            case BuiltinsStubCSigns::ID::StringFromCharCode:
            case BuiltinsStubCSigns::ID::StringCharCodeAt:
            case BuiltinsStubCSigns::ID::StringSubstring:
            case BuiltinsStubCSigns::ID::StringSubStr:
            case BuiltinsStubCSigns::ID::StringSlice:
            case BuiltinsStubCSigns::ID::MapGet:
            case BuiltinsStubCSigns::ID::MapHas:
            case BuiltinsStubCSigns::ID::MapKeys:
            case BuiltinsStubCSigns::ID::MapValues:
            case BuiltinsStubCSigns::ID::MapEntries:
            case BuiltinsStubCSigns::ID::SetHas:
            case BuiltinsStubCSigns::ID::MapDelete:
            case BuiltinsStubCSigns::ID::SetDelete:
            case BuiltinsStubCSigns::ID::TypedArrayEntries:
            case BuiltinsStubCSigns::ID::TypedArrayKeys:
            case BuiltinsStubCSigns::ID::TypedArrayValues:
            case BuiltinsStubCSigns::ID::SetValues:
            case BuiltinsStubCSigns::ID::SetEntries:
            case BuiltinsStubCSigns::ID::MapClear:
            case BuiltinsStubCSigns::ID::SetClear:
            case BuiltinsStubCSigns::ID::SetAdd:
            case BuiltinsStubCSigns::ID::NumberParseFloat:
            case BuiltinsStubCSigns::ID::NumberParseInt:
            case BuiltinsStubCSigns::ID::NumberIsFinite:
            case BuiltinsStubCSigns::ID::NumberIsNaN:
            case BuiltinsStubCSigns::ID::NumberIsInteger:
            case BuiltinsStubCSigns::ID::NumberIsSafeInteger:
            case BuiltinsStubCSigns::ID::ObjectIs:
            case BuiltinsStubCSigns::ID::ObjectGetPrototypeOf:
            case BuiltinsStubCSigns::ID::ObjectGetProto:
            case BuiltinsStubCSigns::ID::ObjectCreate:
            case BuiltinsStubCSigns::ID::ObjectIsPrototypeOf:
            case BuiltinsStubCSigns::ID::ObjectHasOwnProperty:
            case BuiltinsStubCSigns::ID::ReflectGetPrototypeOf:
            case BuiltinsStubCSigns::ID::ReflectGet:
            case BuiltinsStubCSigns::ID::ReflectHas:
            case BuiltinsStubCSigns::ID::ReflectConstruct:
            case BuiltinsStubCSigns::ID::ReflectApply:
            case BuiltinsStubCSigns::ID::FunctionPrototypeApply:
            case BuiltinsStubCSigns::ID::FunctionPrototypeBind:
            case BuiltinsStubCSigns::ID::FunctionPrototypeCall:
            case BuiltinsStubCSigns::ID::FunctionPrototypeHasInstance:
            case BuiltinsStubCSigns::ID::ArrayIndexOf:
            case BuiltinsStubCSigns::ID::ArrayIncludes:
            case BuiltinsStubCSigns::ID::ArrayValues:
            case BuiltinsStubCSigns::ID::ArrayForEach:
            case BuiltinsStubCSigns::ID::ArrayFind:
            case BuiltinsStubCSigns::ID::ArrayFindIndex:
            case BuiltinsStubCSigns::ID::ArrayFilter:
            case BuiltinsStubCSigns::ID::ArrayMap:
            case BuiltinsStubCSigns::ID::ArraySome:
            case BuiltinsStubCSigns::ID::ArrayEvery:
            case BuiltinsStubCSigns::ID::ArrayPop:
            case BuiltinsStubCSigns::ID::ArraySlice:
            case BuiltinsStubCSigns::ID::ArrayPush:
                return true;
            default:
                return false;
        }
        return false;
    }

    static bool IsTypedBuiltinNumber(ID builtinId)
    {
        return BuiltinsStubCSigns::ID::NumberConstructor == builtinId;
    }

    static bool IsTypedBuiltinGlobal(ID builtinId)
    {
        return BuiltinsStubCSigns::ID::GlobalDecodeURIComponent == builtinId;
    }

    static bool IsTypedBuiltinCallThis0(ID builtinId)
    {
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::MapIteratorProtoNext:
            case BuiltinsStubCSigns::ID::SetIteratorProtoNext:
            case BuiltinsStubCSigns::ID::StringIteratorProtoNext:
            case BuiltinsStubCSigns::ID::ArrayIteratorProtoNext:
            case BuiltinsStubCSigns::ID::IteratorProtoReturn:
            case BuiltinsStubCSigns::ID::ArraySort:
                return true;
            default:
                return false;
        }
    }

    static bool IsTypedBuiltinCallThis1(ID builtinId)
    {
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::JsonStringify:
            case BuiltinsStubCSigns::ID::StringLocaleCompare:
                return true;
            default:
                return false;
        }
    }

    static bool IsTypedBuiltinCallThis3(ID builtinId)
    {
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::StringLocaleCompare:
                return true;
            default:
                return false;
        }
    }

    static ConstantIndex GetConstantIndex(ID builtinId)
    {
        switch (builtinId) {
            case BuiltinsStubCSigns::ID::NONE:
                return ConstantIndex::INVALID;
            case BuiltinsStubCSigns::ID::MathAcos:
                return ConstantIndex::MATH_ACOS_INDEX;
            case BuiltinsStubCSigns::ID::MathAcosh:
                return ConstantIndex::MATH_ACOSH_INDEX;
            case BuiltinsStubCSigns::ID::MathAsin:
                return ConstantIndex::MATH_ASIN_INDEX;
            case BuiltinsStubCSigns::ID::MathAsinh:
                return ConstantIndex::MATH_ASINH_INDEX;
            case BuiltinsStubCSigns::ID::MathAtan:
                return ConstantIndex::MATH_ATAN_INDEX;
            case BuiltinsStubCSigns::ID::MathAtan2:
                return ConstantIndex::MATH_ATAN2_INDEX;
            case BuiltinsStubCSigns::ID::MathAtanh:
                return ConstantIndex::MATH_ATANH_INDEX;
            case BuiltinsStubCSigns::ID::MathCos:
                return ConstantIndex::MATH_COS_INDEX;
            case BuiltinsStubCSigns::ID::MathCosh:
                return ConstantIndex::MATH_COSH_INDEX;
            case BuiltinsStubCSigns::ID::MathSign:
                return ConstantIndex::MATH_SIGN_INDEX;
            case BuiltinsStubCSigns::ID::MathSin:
                return ConstantIndex::MATH_SIN_INDEX;
            case BuiltinsStubCSigns::ID::MathSinh:
                return ConstantIndex::MATH_SINH_INDEX;
            case BuiltinsStubCSigns::ID::MathTan:
                return ConstantIndex::MATH_TAN_INDEX;
            case BuiltinsStubCSigns::ID::MathTanh:
                return ConstantIndex::MATH_TANH_INDEX;
            case BuiltinsStubCSigns::ID::MathAbs:
                return ConstantIndex::MATH_ABS_INDEX;
            case BuiltinsStubCSigns::ID::MathLog:
                return ConstantIndex::MATH_LOG_INDEX;
            case BuiltinsStubCSigns::ID::MathLog2:
                return ConstantIndex::MATH_LOG2_INDEX;
            case BuiltinsStubCSigns::ID::MathLog10:
                return ConstantIndex::MATH_LOG10_INDEX;
            case BuiltinsStubCSigns::ID::MathLog1p:
                return ConstantIndex::MATH_LOG1P_INDEX;
            case BuiltinsStubCSigns::ID::MathExp:
                return ConstantIndex::MATH_EXP_INDEX;
            case BuiltinsStubCSigns::ID::MathExpm1:
                return ConstantIndex::MATH_EXPM1_INDEX;
            case BuiltinsStubCSigns::ID::MathClz32:
                return ConstantIndex::MATH_CLZ32_INDEX;
            case BuiltinsStubCSigns::ID::MathPow:
                return ConstantIndex::MATH_POW_INDEX;
            case BuiltinsStubCSigns::ID::MathCbrt:
                return ConstantIndex::MATH_CBRT_INDEX;
            case BuiltinsStubCSigns::ID::MathTrunc:
                return ConstantIndex::MATH_TRUNC_INDEX;
            case BuiltinsStubCSigns::ID::MathCeil:
                return ConstantIndex::MATH_CEIL_INDEX;
            case BuiltinsStubCSigns::ID::MathFloor:
                return ConstantIndex::MATH_FLOOR_INDEX;
            case BuiltinsStubCSigns::ID::MathMin:
                return ConstantIndex::MATH_MIN_INDEX;
            case BuiltinsStubCSigns::ID::MathMax:
                return ConstantIndex::MATH_MAX_INDEX;
            case BuiltinsStubCSigns::ID::MathSqrt:
                return ConstantIndex::MATH_SQRT_INDEX;
            case BuiltinsStubCSigns::ID::MathRound:
                return ConstantIndex::MATH_ROUND_INDEX;
            case BuiltinsStubCSigns::ID::MathFRound:
                return ConstantIndex::MATH_FROUND_INDEX;
            case BuiltinsStubCSigns::ID::MathImul:
                return ConstantIndex::MATH_IMUL_INDEX;
            case BuiltinsStubCSigns::ID::MapGet:
                return ConstantIndex::MAP_GET_INDEX;
            case BuiltinsStubCSigns::ID::MapHas:
                return ConstantIndex::MAP_HAS_INDEX;
            case BuiltinsStubCSigns::ID::MapKeys:
                return ConstantIndex::MAP_KEYS_INDEX;
            case BuiltinsStubCSigns::ID::MapValues:
                return ConstantIndex::MAP_VALUES_INDEX;
            case BuiltinsStubCSigns::ID::MapEntries:
                return ConstantIndex::MAP_ENTRIES_INDEX;
            case BuiltinsStubCSigns::ID::SetHas:
                return ConstantIndex::SET_HAS_INDEX;
            case BuiltinsStubCSigns::ID::MapDelete:
                return ConstantIndex::MAP_DELETE_INDEX;
            case BuiltinsStubCSigns::ID::SetDelete:
                return ConstantIndex::SET_DELETE_INDEX;
            case BuiltinsStubCSigns::ID::SetValues:
                return ConstantIndex::SET_VALUES_INDEX;
            case BuiltinsStubCSigns::ID::SetEntries:
                return ConstantIndex::SET_ENTRIES_INDEX;
            case BuiltinsStubCSigns::ID::MapClear:
                return ConstantIndex::MAP_CLEAR_INDEX;
            case BuiltinsStubCSigns::ID::SetClear:
                return ConstantIndex::SET_CLEAR_INDEX;
            case BuiltinsStubCSigns::ID::SetAdd:
                return ConstantIndex::SET_ADD_INDEX;
            case BuiltinsStubCSigns::ID::StringLocaleCompare:
                return ConstantIndex::LOCALE_COMPARE_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::StringCharCodeAt:
                return ConstantIndex::STRING_CHAR_CODE_AT_INDEX;
            case BuiltinsStubCSigns::ID::StringSubstring:
                return ConstantIndex::STRING_SUB_STRING_INDEX;
            case BuiltinsStubCSigns::ID::StringSubStr:
                return ConstantIndex::STRING_SUB_STR_INDEX;
            case BuiltinsStubCSigns::ID::StringSlice:
                return ConstantIndex::STRING_SLICE_INDEX;
            case BuiltinsStubCSigns::ID::ArraySort:
                return ConstantIndex::ARRAY_SORT_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::JsonStringify:
                return ConstantIndex::JSON_STRINGIFY_FUNCTION_INDEX;
            case BuiltinsStubCSigns::ID::MapIteratorProtoNext:
                return ConstantIndex::MAP_ITERATOR_PROTO_NEXT_INDEX;
            case BuiltinsStubCSigns::ID::SetIteratorProtoNext:
                return ConstantIndex::SET_ITERATOR_PROTO_NEXT_INDEX;
            case BuiltinsStubCSigns::ID::StringIteratorProtoNext:
                return ConstantIndex::STRING_ITERATOR_PROTO_NEXT_INDEX;
            case BuiltinsStubCSigns::ID::ArrayIteratorProtoNext:
                return ConstantIndex::ARRAY_ITERATOR_PROTO_NEXT_INDEX;
            case BuiltinsStubCSigns::ID::IteratorProtoReturn:
                return ConstantIndex::ITERATOR_PROTO_RETURN_INDEX;
            case BuiltinsStubCSigns::ID::StringFromCharCode:
                return ConstantIndex::STRING_FROM_CHAR_CODE_INDEX;
            case BuiltinsStubCSigns::ID::DateGetTime:
                return ConstantIndex::DATE_GET_TIME_INDEX;
            case BuiltinsStubCSigns::ID::DateNow:
                return ConstantIndex::DATE_NOW_INDEX;
            case BuiltinsStubCSigns::ID::TypedArrayEntries:
                return ConstantIndex::TYPED_ARRAY_ENTRIES_INDEX;
            case BuiltinsStubCSigns::ID::TypedArrayKeys:
                return ConstantIndex::TYPED_ARRAY_KEYS_INDEX;
            case BuiltinsStubCSigns::ID::TypedArrayValues:
                return ConstantIndex::TYPED_ARRAY_VALUES_INDEX;
            case BuiltinsStubCSigns::ID::GlobalIsFinite:
                return ConstantIndex::GLOBAL_IS_FINITE_INDEX;
            case BuiltinsStubCSigns::ID::GlobalIsNan:
                return ConstantIndex::GLOBAL_IS_NAN_INDEX;
            case BuiltinsStubCSigns::ID::ArrayBufferIsView:
                return ConstantIndex::ARRAY_BUFFER_IS_VIEW_INDEX;
            case BuiltinsStubCSigns::ID::DataViewGetFloat32:
                return ConstantIndex::DATA_VIEW_GET_FLOAT32_INDEX;
            case BuiltinsStubCSigns::ID::DataViewGetFloat64:
                return ConstantIndex::DATA_VIEW_GET_FLOAT64_INDEX;
            case BuiltinsStubCSigns::ID::DataViewGetInt8:
                return ConstantIndex::DATA_VIEW_GET_INT8_INDEX;
            case BuiltinsStubCSigns::ID::DataViewGetInt16:
                return ConstantIndex::DATA_VIEW_GET_INT16_INDEX;
            case BuiltinsStubCSigns::ID::DataViewGetInt32:
                return ConstantIndex::DATA_VIEW_GET_INT32_INDEX;
            case BuiltinsStubCSigns::ID::DataViewGetUint16:
                return ConstantIndex::DATA_VIEW_GET_UINT16_INDEX;
            case BuiltinsStubCSigns::ID::DataViewGetUint32:
                return ConstantIndex::DATA_VIEW_GET_UINT32_INDEX;
            case BuiltinsStubCSigns::ID::DataViewGetUint8:
                return ConstantIndex::DATA_VIEW_GET_UINT8_INDEX;
            case BuiltinsStubCSigns::ID::DataViewSetFloat32:
                return ConstantIndex::DATA_VIEW_SET_FLOAT32_INDEX;
            case BuiltinsStubCSigns::ID::DataViewSetFloat64:
                return ConstantIndex::DATA_VIEW_SET_FLOAT64_INDEX;
            case BuiltinsStubCSigns::ID::DataViewSetInt8:
                return ConstantIndex::DATA_VIEW_SET_INT8_INDEX;
            case BuiltinsStubCSigns::ID::DataViewSetInt16:
                return ConstantIndex::DATA_VIEW_SET_INT16_INDEX;
            case BuiltinsStubCSigns::ID::DataViewSetInt32:
                return ConstantIndex::DATA_VIEW_SET_INT32_INDEX;
            case BuiltinsStubCSigns::ID::DataViewSetUint8:
                return ConstantIndex::DATA_VIEW_SET_UINT8_INDEX;
            case BuiltinsStubCSigns::ID::DataViewSetUint16:
                return ConstantIndex::DATA_VIEW_SET_UINT16_INDEX;
            case BuiltinsStubCSigns::ID::DataViewSetUint32:
                return ConstantIndex::DATA_VIEW_SET_UINT32_INDEX;
            case BuiltinsStubCSigns::ID::BigIntAsIntN:
                return ConstantIndex::BIGINT_AS_INTN_INDEX;
            case BuiltinsStubCSigns::ID::BigIntAsUintN:
                return ConstantIndex::BIGINT_AS_UINTN_INDEX;
            case BuiltinsStubCSigns::ID::NumberIsFinite:
                return ConstantIndex::NUMBER_IS_FINITE_INDEX;
            case BuiltinsStubCSigns::ID::NumberIsInteger:
                return ConstantIndex::NUMBER_IS_INTEGER_INDEX;
            case BuiltinsStubCSigns::ID::NumberIsNaN:
                return ConstantIndex::NUMBER_IS_NAN_INDEX;
            case BuiltinsStubCSigns::ID::NumberIsSafeInteger:
                return ConstantIndex::NUMBER_IS_SAFEINTEGER_INDEX;
            case BuiltinsStubCSigns::ID::NumberParseFloat:
                return ConstantIndex::NUMBER_PARSE_FLOAT_INDEX;
            case BuiltinsStubCSigns::ID::NumberParseInt:
                return ConstantIndex::NUMBER_PARSE_INT_INDEX;
            case BuiltinsStubCSigns::ID::ObjectIs:
                return ConstantIndex::OBJECT_IS_INDEX;
            case BuiltinsStubCSigns::ID::ObjectGetPrototypeOf:
                return ConstantIndex::OBJECT_GET_PROTOTYPE_OF_INDEX;
            case BuiltinsStubCSigns::ID::ObjectGetProto:
                return ConstantIndex::OBJECT_GET_PROTO_INDEX;
            case BuiltinsStubCSigns::ID::ObjectCreate:
                return ConstantIndex::OBJECT_CREATE_INDEX;
            case BuiltinsStubCSigns::ID::ObjectIsPrototypeOf:
                return ConstantIndex::OBJECT_IS_PROTOTYPE_OF_INDEX;
            case BuiltinsStubCSigns::ID::ObjectHasOwnProperty:
                return ConstantIndex::OBJECT_HAS_OWN_PROPERTY_INDEX;
            case BuiltinsStubCSigns::ID::ReflectGetPrototypeOf:
                return ConstantIndex::REFLECT_GET_PROTOTYPE_OF_INDEX;
            case BuiltinsStubCSigns::ID::ReflectGet:
                return ConstantIndex::REFLECT_GET_INDEX;
            case BuiltinsStubCSigns::ID::ReflectHas:
                return ConstantIndex::REFLECT_HAS_INDEX;
            case BuiltinsStubCSigns::ID::ReflectConstruct:
                return ConstantIndex::REFLECT_CONSTRUCT_INDEX;
            case BuiltinsStubCSigns::ID::ReflectApply:
                return ConstantIndex::REFLECT_APPLY_INDEX;
            case BuiltinsStubCSigns::ID::FunctionPrototypeApply:
                return ConstantIndex::FUNCTION_PROTOTYPE_APPLY_INDEX;
            case BuiltinsStubCSigns::ID::FunctionPrototypeBind:
                return ConstantIndex::FUNCTION_PROTOTYPE_BIND_INDEX;
            case BuiltinsStubCSigns::ID::FunctionPrototypeCall:
                return ConstantIndex::FUNCTION_PROTOTYPE_CALL_INDEX;
            case BuiltinsStubCSigns::ID::FunctionPrototypeHasInstance:
                return ConstantIndex::FUNCTION_PROTOTYPE_HAS_INSTANCE_INDEX;
            case BuiltinsStubCSigns::ID::ArrayIndexOf:
                return ConstantIndex::ARRAY_INDEXOF_INDEX;
            case BuiltinsStubCSigns::ID::ArrayIncludes:
                return ConstantIndex::ARRAY_INCLUDES_INDEX;
            case BuiltinsStubCSigns::ID::ArrayValues:
                return ConstantIndex::ARRAY_VALUES_INDEX;
            case BuiltinsStubCSigns::ID::ArrayEntries:
                return ConstantIndex::ARRAY_ENTRIES_INDEX;
            case BuiltinsStubCSigns::ID::ArrayKeys:
                return ConstantIndex::ARRAY_KEYS_INDEX;
            case BuiltinsStubCSigns::ID::ArrayForEach:
                return ConstantIndex::ARRAY_FOREACH_INDEX;
            case BuiltinsStubCSigns::ID::ArrayFind:
                return ConstantIndex::ARRAY_FIND_INDEX;
            case BuiltinsStubCSigns::ID::ArrayFindIndex:
                return ConstantIndex::ARRAY_FINDINDEX_INDEX;
            case BuiltinsStubCSigns::ID::ArrayFilter:
                return ConstantIndex::ARRAY_FILTER_INDEX;
            case BuiltinsStubCSigns::ID::ArrayMap:
                return ConstantIndex::ARRAY_MAP_INDEX;
            case BuiltinsStubCSigns::ID::ArraySome:
                return ConstantIndex::ARRAY_SOME_INDEX;
            case BuiltinsStubCSigns::ID::ArrayEvery:
                return ConstantIndex::ARRAY_EVERY_INDEX;
            case BuiltinsStubCSigns::ID::ArrayPop:
                return ConstantIndex::ARRAY_POP_INDEX;
            case BuiltinsStubCSigns::ID::ArrayPush:
                return ConstantIndex::ARRAY_PUSH_INDEX;
            case BuiltinsStubCSigns::ID::ArraySlice:
                return ConstantIndex::ARRAY_SLICE_INDEX;
            case BuiltinsStubCSigns::ID::GlobalDecodeURIComponent:
                return ConstantIndex::GLOBAL_DECODE_URI_COMPONENT;
            default:
                LOG_COMPILER(DEBUG) << "GetConstantIndex Invalid Id:" << builtinId;
                return ConstantIndex::INVALID;
        }
    }

    static bool CheckBuiltinsIdInvalid(ID builtinId)
    {
        auto result = kungfu::BuiltinsStubCSigns::GetConstantIndex(builtinId);
        return result == ConstantIndex::INVALID;
    }

    static size_t GetGlobalEnvIndex(ID builtinId);

    static std::string GetBuiltinName(ID id)
    {
        static const std::map<BuiltinsStubCSigns::ID, const std::string> builtinId2Str = {
            {MathAcos, "Math.acos"},
            {MathAcosh, "Math.acosh"},
            {MathAsin, "Math.asin"},
            {MathAsinh, "Math.asinh"},
            {MathAtan, "Math.atan"},
            {MathAtan2, "Math.atan2"},
            {MathAtanh, "Math.atanh"},
            {MathCos, "Math.cos"},
            {MathCosh, "Math.cosh"},
            {MathSign, "Math.sign"},
            {MathSin, "Math.sin"},
            {MathSinh, "Math.sinh"},
            {MathTan, "Math.tan"},
            {MathTanh, "Math.tanh"},
            {MathLog, "Math.log"},
            {MathLog2, "Math.log2"},
            {MathLog10, "Math.log10"},
            {MathLog1p, "Math.log1p"},
            {MathExp, "Math.exp"},
            {MathExpm1, "Math.expm1"},
            {MathClz32, "Math.clz32"},
            {MathSqrt, "Math.sqrt"},
            {MathCbrt, "Math.cbrt"},
            {MathAbs, "Math.abs"},
            {MathPow, "Math.pow"},
            {MathTrunc, "Math.trunc"},
            {MathRound, "Math.round"},
            {MathFRound, "Math.fround"},
            {MathCeil, "Math.ceil"},
            {MathFloor, "Math.floor"},
            {MathImul, "Math.imul"},
            {MathMax, "Math.max"},
            {MathMin, "Math.min"},
            {DateGetTime, "Date.prototype.getTime"},
            {DateNow, "Date.now"},
            {TypedArrayEntries, "TypedArray.entries"},
            {TypedArrayKeys, "TypedArray.keys"},
            {TypedArrayValues, "TypedArray.values"},
            {GlobalIsFinite, "isFinite"},
            {GlobalIsNan, "isNan"},
            {BigIntAsIntN, "BigInt.asIntN"},
            {BigIntAsUintN, "BigInt.asUintN"},
            {MapGet, "Map.get"},
            {MapHas, "Map.has"},
            {MapKeys, "Map.keys"},
            {MapValues, "Map.values"},
            {MapEntries, "Map.entries"},
            {SetValues, "Set.values"},
            {SetEntries, "Set.entries"},
            {SetHas, "Set.has"},
            {MapDelete, "Map.delete"},
            {SetDelete, "Set.delete"},
            {MapClear, "Map.clear"},
            {SetClear, "Set.clear"},
            {SetAdd, "Set.add"},
            {BigIntConstructor, "BigInt"},
            {NumberParseFloat, "Number.parseFloat"},
            {NumberIsInteger, "Number.isInteger"},
            {NumberIsSafeInteger, "Number.isSafeInteger"},
            {NumberParseInt, "Number.parseInt"},
            {StringCharCodeAt, "String.prototype.charCodeAt"},
            {StringSubstring, "String.prototype.substring"},
            {StringSubStr, "String.prototype.substr"},
            {StringSlice, "String.prototype.slice"},
            {ObjectIs, "Object.is"},
            {ObjectGetPrototypeOf, "Object.getPrototypeOf"},
            {ObjectGetProto, "Object.prototype.getProto"},
            {ObjectCreate, "Object.create"},
            {ObjectIsPrototypeOf, "Object.prototype.isPrototypeOf"},
            {ObjectHasOwnProperty, "Object.prototype.hasOwnProperty"},
            {ReflectGetPrototypeOf, "Reflect.getPrototypeOf"},
            {ReflectGet, "Reflect.get"},
            {ReflectHas, "Reflect.has"},
            {ReflectConstruct, "Reflect.construct"},
            {ReflectApply, "Reflect.apply"},
            {FunctionPrototypeApply, "Function.prototype.apply"},
            {FunctionPrototypeBind, "Function.prototype.bind"},
            {FunctionPrototypeCall, "Function.prototype.call"},
            {FunctionPrototypeHasInstance, "Function.prototype.hasInstance"},
            {ArrayIncludes, "Array.prototype.includes"},
            {ArrayIndexOf, "Array.prototype.indexOf"},
            {ArrayEntries, "Array.prototype.entries"},
            {ArrayValues, "Array.prototype.values"},
            {ArrayKeys, "Array.prototype.keys"},
            {ArrayFind, "Array.prototype.find"},
            {ArrayFindIndex, "Array.prototype.findIndex"},
            {ArrayForEach, "Array.prototype.foreach"},
            {ArrayFilter, "Array.prototype.filter"},
            {ArraySome, "Array.prototype.some"},
            {ArrayMap, "Array.prototype.map"},
            {ArrayEvery, "Array.prototype.every"},
            {ArrayPop, "Array.prototype.pop"},
            {ArrayPush, "Array.prototype.push"},
            {ArraySlice, "Array.prototype.slice"},
            {DataViewGetInt8, "DataView.prototype.GetInt8"},
            {DataViewGetUint8, "DataView.prototype.GetUint8"},
            {DataViewGetInt16, "DataView.prototype.GetInt16"},
            {DataViewGetUint16, "DataView.prototype.GetUint16"},
            {DataViewGetInt32, "DataView.prototype.GetInt32"},
            {DataViewGetUint32, "DataView.prototype.GetUint32"},
            {DataViewGetFloat32, "DataView.prototype.GetFloat32"},
            {DataViewGetFloat64, "DataView.prototype.GetFloat64"},
            {DataViewSetInt8, "DataView.prototype.SetInt8"},
            {DataViewSetUint8, "DataView.prototype.SetUint8"},
            {DataViewSetInt16, "DataView.prototype.SetInt16"},
            {DataViewSetUint16, "DataView.prototype.SetUint16"},
            {DataViewSetInt32, "DataView.prototype.SetInt32"},
            {DataViewSetUint32, "DataView.prototype.SetUint32"},
            {DataViewSetFloat32, "DataView.prototype.SetFloat32"},
            {DataViewSetFloat64, "DataView.prototype.SetFloat64"},
        };
        if (builtinId2Str.count(id) > 0) {
            return builtinId2Str.at(id);
        }
        return "";
    }

    static ID GetBuiltinId(std::string idStr)
    {
        static const std::map<const std::string, BuiltinsStubCSigns::ID> str2BuiltinId = {
            {"acos", MathAcos},
            {"acosh", MathAcosh},
            {"asin", MathAsin},
            {"asinh", MathAsinh},
            {"atan", MathAtan},
            {"atan2", MathAtan2},
            {"atanh", MathAtanh},
            {"cos", MathCos},
            {"cosh", MathCosh},
            {"sign", MathSign},
            {"sin", MathSin},
            {"sinh", MathSinh},
            {"tan", MathTan},
            {"tanh", MathTanh},
            {"log", MathLog},
            {"log2", MathLog2},
            {"log10", MathLog10},
            {"log1p", MathLog1p},
            {"exp", MathExp},
            {"expm1", MathExpm1},
            {"clz32", MathClz32},
            {"sqrt", MathSqrt},
            {"cbrt", MathCbrt},
            {"abs", MathAbs},
            {"pow", MathPow},
            {"trunc", MathTrunc},
            {"round", MathRound},
            {"fround", MathFRound},
            {"ceil", MathCeil},
            {"floor", MathFloor},
            {"imul", MathImul},
            {"max", MathMax},
            {"min", MathMin},
            {"localeCompare", StringLocaleCompare},
            {"next", StringIteratorProtoNext},
            {"sort", ArraySort},
            {"stringify", JsonStringify},
            {"getTime", DateGetTime},
            {"now", DateNow},
            {"isFinite", GlobalIsFinite},
            {"isNan", GlobalIsNan},
            {"asIntN", BigIntAsIntN},
            {"asUintN", BigIntAsUintN},
            {"mapDelete", MapDelete},
            {"setDelete", SetDelete},
            {"BigInt", BigIntConstructor},
            {"charCodeAt", StringCharCodeAt},
            {"substring", StringSubstring},
            {"substr", StringSubStr},
            {"slice", StringSlice},
            {"is", ObjectIs},
            {"getPrototypeOf", ObjectGetPrototypeOf},
            {"create", ObjectCreate},
            {"isPrototypeOf", ObjectIsPrototypeOf},
            {"hasOwnProperty", ObjectHasOwnProperty},
            {"reflectGetPrototypeOf", ReflectGetPrototypeOf},
            {"get", ReflectGet},
            {"has", ReflectHas},
            {"construct", ReflectConstruct},
            {"ReflectApply", ReflectApply},
            {"apply", FunctionPrototypeApply},
            {"bind", FunctionPrototypeBind},
            {"call", FunctionPrototypeCall},
            {"hasInstance", FunctionPrototypeHasInstance},
            {"includes", ArrayIncludes},
            {"indexOf", ArrayIndexOf},
            {"entries", ArrayEntries},
            {"values", ArrayValues},
            {"keys", ArrayKeys},
            {"find", ArrayFind},
            {"findIndex", ArrayFindIndex},
            {"foreach", ArrayForEach},
            {"filter", ArrayFilter},
            {"some", ArraySome},
            {"map", ArrayMap},
            {"every", ArrayEvery},
            {"pop", ArrayPop},
            {"push", ArrayPush},
            {"slice", ArraySlice},
            {"getUint8", DataViewGetUint8},
            {"getInt8", DataViewGetInt8},
            {"getUint16", DataViewGetUint16},
            {"getInt16", DataViewGetInt16},
            {"getUint32", DataViewGetUint32},
            {"getInt32", DataViewGetInt32},
            {"getFloat32", DataViewGetFloat32},
            {"getFloat64", DataViewGetFloat64},
            {"setUint8", DataViewSetUint8},
            {"setInt8", DataViewSetInt8},
            {"setUint16", DataViewSetUint16},
            {"setInt16", DataViewSetInt16},
            {"setUint32", DataViewSetUint32},
            {"setInt32", DataViewSetInt32},
            {"setFloat32", DataViewSetFloat32},
            {"setFloat64", DataViewSetFloat64},
        };
        if (str2BuiltinId.count(idStr) > 0) {
            return str2BuiltinId.at(idStr);
        }
        return NONE;
    }

private:
    static CallSignature callSigns_[NUM_OF_BUILTINS_STUBS];
    static CallSignature builtinsCSign_;
    static CallSignature builtinsWithArgvCSign_;
};

enum class BuiltinsArgs : size_t {
    GLUE = 0,
    NATIVECODE,
    FUNC,
    NEWTARGET,
    THISVALUE,
    NUMARGS,
    ARG0_OR_ARGV,
    ARG1,
    ARG2,
    NUM_OF_INPUTS,
};

#define BUILTINS_STUB_ID(name) kungfu::BuiltinsStubCSigns::name
// to distinguish with the positive method offset of js function
#define PGO_BUILTINS_STUB_ID(name) ((-1) * kungfu::BuiltinsStubCSigns::name)
#define IS_TYPED_BUILTINS_ID(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltin(id)
#define IS_TYPED_INLINE_BUILTINS_ID(id) kungfu::BuiltinsStubCSigns::IsTypedInlineBuiltin(id)
#define IS_TYPED_BUILTINS_NUMBER_ID(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinNumber(id)
#define IS_TYPED_BUILTINS_GLOBAL_ID(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinGlobal(id)
#define IS_TYPED_BUILTINS_ID_CALL_THIS0(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinCallThis0(id)
#define IS_TYPED_BUILTINS_ID_CALL_THIS1(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinCallThis1(id)
#define IS_TYPED_BUILTINS_ID_CALL_THIS3(id) kungfu::BuiltinsStubCSigns::IsTypedBuiltinCallThis3(id)
#define GET_TYPED_CONSTANT_INDEX(id) kungfu::BuiltinsStubCSigns::GetConstantIndex(id)
#define IS_INVALID_ID(id) kungfu::BuiltinsStubCSigns::CheckBuiltinsIdInvalid(id)
#define GET_TYPED_GLOBAL_ENV_INDEX(id) kungfu::BuiltinsStubCSigns::GetGlobalEnvIndex(id)
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_H
