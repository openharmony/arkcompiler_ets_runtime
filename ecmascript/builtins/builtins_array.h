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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_ARRAY_H
#define ECMASCRIPT_BUILTINS_BUILTINS_ARRAY_H

#include "ecmascript/base/builtins_base.h"

// List of functions in Array, excluding the '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsArray::func refers to the native implementation of Array[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_ARRAY_FUNCTIONS(V)                          \
    /* Array.from ( items [ , mapfn [ , thisArg ] ] ) */    \
    V("from",    From,    1, ArrayFrom)                     \
    /* Array.isArray ( arg ) */                             \
    V("isArray", IsArray, 1, ArrayIsArray)                  \
    /* Array.of ( ...items ) */                             \
    V("of",      Of,      0, INVALID)

// List of functions in Array.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsArray::func refers to the native implementation of Array.prototype[name].
#define BUILTIN_ARRAY_PROTOTYPE_FUNCTIONS(V)                                \
    /* Array.prototype.at ( index ) */                                      \
    V("at",             At,               1, INVALID)                       \
    /* Array.prototype.concat ( ...items ) */                               \
    V("concat",         Concat,           1, ArrayConcat)                   \
    /* Array.prototype.copyWithin ( target, start [ , end ] ) */            \
    V("copyWithin",     CopyWithin,       2, ArrayCopyWithin)               \
    /* Array.prototype.entries ( ) */                                       \
    V("entries",        Entries,          0, ArrayEntries)                  \
    /* Array.prototype.every ( callbackfn [ , thisArg ] ) */                \
    V("every",          Every,            1, ArrayEvery)                    \
    /* Array.prototype.fill ( value [ , start [ , end ] ] ) */              \
    V("fill",           Fill,             1, ArrayFill)                     \
    /* Array.prototype.filter ( callbackfn [ , thisArg ] ) */               \
    V("filter",         Filter,           1, ArrayFilter)                   \
    /* Array.prototype.find ( predicate [ , thisArg ] ) */                  \
    V("find",           Find,             1, ArrayFind)                     \
    /* Array.prototype.findIndex ( predicate [ , thisArg ] ) */             \
    V("findIndex",      FindIndex,        1, ArrayFindIndex)                \
    /* Array.prototype.findLast ( predicate [ , thisArg ] ) */              \
    V("findLast",       FindLast,         1, ArrayFindLast)                 \
    /* Array.prototype.findLastIndex ( predicate [ , thisArg ] ) */         \
    V("findLastIndex",  FindLastIndex,    1, ArrayFindLastIndex)            \
    /* Array.prototype.flat ( [ depth ] ) */                                \
    V("flat",           Flat,             0, INVALID)                       \
    /* Array.prototype.flatMap ( mapperFunction [ , thisArg ] ) */          \
    V("flatMap",        FlatMap,          1, ArrayFlatMap)                  \
    /* Array.prototype.forEach ( callbackfn [ , thisArg ] ) */              \
    V("forEach",        ForEach,          1, ArrayForEach)                  \
    /* Array.prototype.includes ( searchElement [ , fromIndex ] ) */        \
    V("includes",       Includes,         1, ArrayIncludes)                 \
    /* Array.prototype.indexOf ( searchElement [ , fromIndex ] ) */         \
    V("indexOf",        IndexOf,          1, ArrayIndexOf)                  \
    /* Array.prototype.join ( separator ) */                                \
    V("join",           Join,             1, INVALID)                       \
    /* Array.prototype.keys ( ) */                                          \
    V("keys",           Keys,             0, ArrayKeys)                     \
    /* Array.prototype.lastIndexOf ( searchElement [ , fromIndex ] ) */     \
    V("lastIndexOf",    LastIndexOf,      1, ArrayLastIndexOf)              \
    /* Array.prototype.map ( callbackfn [ , thisArg ] ) */                  \
    V("map",            Map,              1, ArrayMap)                      \
    /* Array.prototype.pop ( ) */                                           \
    V("pop",            Pop,              0, ArrayPop)                      \
    /* Array.prototype.push ( ...items ) */                                 \
    V("push",           Push,             1, ArrayPush)                     \
    /* Array.prototype.reduce ( callbackfn [ , initialValue ] ) */          \
    V("reduce",         Reduce,           1, ArrayReduce)                   \
    /* Array.prototype.reduceRight ( callbackfn [ , initialValue ] ) */     \
    V("reduceRight",    ReduceRight,      1, ArrayReduceRight)              \
    /* Array.prototype.reverse ( ) */                                       \
    V("reverse",        Reverse,          0, ArrayReverse)                  \
    /* Array.prototype.shift ( ) */                                         \
    V("shift",          Shift,            0, ArrayShift)                    \
    /* Array.prototype.slice ( start, end ) */                              \
    V("slice",          Slice,            2, ArraySlice)                    \
    /* Array.prototype.some ( callbackfn [ , thisArg ] ) */                 \
    V("some",           Some,             1, ArraySome)                     \
    /* Array.prototype.sort ( comparefn ) */                                \
    V("sort",           Sort,             1, ArraySort)                     \
    /* Array.prototype.splice ( start, deleteCount, ...items ) */           \
    V("splice",         Splice,           2, ArraySplice)                   \
    /* Array.prototype.toLocaleString ( [ reserved1 [ , reserved2 ] ] ) */  \
    V("toLocaleString", ToLocaleString,   0, INVALID)                       \
    /* Array.prototype.toReversed ( ) */                                    \
    V("toReversed",     ToReversed,       0, ArrayToReversed)               \
    /* Array.prototype.toSorted ( comparefn ) */                            \
    V("toSorted",       ToSorted,         1, ArrayToSorted)                 \
    /* Array.prototype.toSpliced ( start, skipCount, ...items ) */          \
    V("toSpliced",      ToSpliced,        2, ArrayToSpliced)                \
    /* Array.prototype.toString ( ) */                                      \
    V("toString",       ToString,         0, INVALID)                       \
    /* Array.prototype.unshift ( ...items ) */                              \
    V("unshift",        Unshift,          1, ArrayUnshift)                  \
    /* Array.prototype.values ( ) */                                        \
    V("values",         Values,           0, ArrayValues)                   \
    /* Array.prototype.with ( index, value ) */                             \
    V("with",           With,             2, ArrayWith)

namespace panda::ecmascript::builtins {
static constexpr uint8_t INDEX_TWO = 2;
static constexpr uint8_t INDEX_THREE = 3;
class BuiltinsArray : public base::BuiltinsBase {
public:
    // 22.1.1
    static JSTaggedValue ArrayConstructor(EcmaRuntimeCallInfo *argv);

    // 22.1.2.1
    static JSTaggedValue From(EcmaRuntimeCallInfo *argv);
    // 22.1.2.2
    static JSTaggedValue IsArray(EcmaRuntimeCallInfo *argv);
    // 22.1.2.3
    static JSTaggedValue Of(EcmaRuntimeCallInfo *argv);
    // 22.1.2.5
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);

    // prototype
    // 22.1.3.1
    static JSTaggedValue Concat(EcmaRuntimeCallInfo *argv);
    // 22.1.3.3
    static JSTaggedValue CopyWithin(EcmaRuntimeCallInfo *argv);
    // 22.1.3.4
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    // 22.1.3.5
    static JSTaggedValue Every(EcmaRuntimeCallInfo *argv);
    // 22.1.3.6
    static JSTaggedValue Fill(EcmaRuntimeCallInfo *argv);
    // 22.1.3.7
    static JSTaggedValue Filter(EcmaRuntimeCallInfo *argv);
    // 22.1.3.8
    static JSTaggedValue Find(EcmaRuntimeCallInfo *argv);
    // 22.1.3.9
    static JSTaggedValue FindIndex(EcmaRuntimeCallInfo *argv);
    // 22.1.3.10
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    // 22.1.3.11
    static JSTaggedValue IndexOf(EcmaRuntimeCallInfo *argv);
    // 22.1.3.12
    static JSTaggedValue Join(EcmaRuntimeCallInfo *argv);
    // 22.1.3.13
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    // 22.1.3.14
    static JSTaggedValue LastIndexOf(EcmaRuntimeCallInfo *argv);
    // 22.1.3.15
    static JSTaggedValue Map(EcmaRuntimeCallInfo *argv);
    // 22.1.3.16
    static JSTaggedValue Pop(EcmaRuntimeCallInfo *argv);
    // 22.1.3.17
    static JSTaggedValue Push(EcmaRuntimeCallInfo *argv);
    // 22.1.3.18
    static JSTaggedValue Reduce(EcmaRuntimeCallInfo *argv);
    // 22.1.3.19
    static JSTaggedValue ReduceRight(EcmaRuntimeCallInfo *argv);
    // 22.1.3.20
    static JSTaggedValue Reverse(EcmaRuntimeCallInfo *argv);
    // 22.1.3.21
    static JSTaggedValue Shift(EcmaRuntimeCallInfo *argv);
    // 22.1.3.22
    static JSTaggedValue Slice(EcmaRuntimeCallInfo *argv);
    // 22.1.3.23
    static JSTaggedValue Some(EcmaRuntimeCallInfo *argv);
    // 22.1.3.24
    static JSTaggedValue Sort(EcmaRuntimeCallInfo *argv);
    // 22.1.3.25
    static JSTaggedValue Splice(EcmaRuntimeCallInfo *argv);
    // 22.1.3.26
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);
    // 22.1.3.27
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv); // no change
    // 22.1.3.28
    static JSTaggedValue Unshift(EcmaRuntimeCallInfo *argv); // done
    // 22.1.3.29
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv); // no change
    // es12 23.1.3.13
    static JSTaggedValue Includes(EcmaRuntimeCallInfo *argv); // no change
    // es12 23.1.3.10
    static JSTaggedValue Flat(EcmaRuntimeCallInfo *argv);
    // es12 23.1.3.11
    static JSTaggedValue FlatMap(EcmaRuntimeCallInfo *argv);
    // 23.1.3.1 Array.prototype.at ( index )
    static JSTaggedValue At(EcmaRuntimeCallInfo *argv); // no change
    // 23.1.3.33 Array.prototype.toReversed ( )
    static JSTaggedValue ToReversed(EcmaRuntimeCallInfo *argv); // no change
    // 23.1.3.39 Array.prototype.with ( index, value )
    static JSTaggedValue With(EcmaRuntimeCallInfo *argv); // done
    // 23.1.3.34 Array.prototype.toSorted ( comparefn )
    static JSTaggedValue ToSorted(EcmaRuntimeCallInfo *argv);
    // 23.1.3.11
    static JSTaggedValue FindLast(EcmaRuntimeCallInfo *argv); // no change
    // 23.1.3.12
    static JSTaggedValue FindLastIndex(EcmaRuntimeCallInfo *argv); // no change
    // 23.1.3.35 Array.prototype.toSpliced ( start, skipCount, ...items )
    static JSTaggedValue ToSpliced(EcmaRuntimeCallInfo *argv);

    // Excluding the '@@' internal properties
    static Span<const base::BuiltinFunctionEntry> GetArrayFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(ARRAY_FUNCTIONS);
    }

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetArrayPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(ARRAY_PROTOTYPE_FUNCTIONS);
    }

    static size_t GetNumPrototypeInlinedProperties()
    {
        // 4 : 4 More inlined entries in Array.prototype for the following functions/accessors:
        //   (1) 'length' accessor
        //   (2) Array.prototype.constructor, i.e. Array()
        //   (3) Array.prototype[@@iterator]()
        //   (4) Array.prototype[@@unscopables]()
        return GetArrayPrototypeFunctions().Size() + 4;
    }
    static JSTaggedValue ReduceUnStableJSArray(JSThread *thread, JSHandle<JSTaggedValue> &thisHandle,
        JSHandle<JSTaggedValue> &thisObjVal, int64_t k, int64_t len, JSMutableHandle<JSTaggedValue> &accumulator,
        JSHandle<JSTaggedValue> &callbackFnHandle);

    static JSTaggedValue FilterUnStableJSArray(JSThread *thread, JSHandle<JSTaggedValue> &thisArgHandle,
        JSHandle<JSTaggedValue> &thisObjVal, int64_t k, int64_t len, uint32_t toIndex,
        JSHandle<JSObject> newArrayHandle, JSHandle<JSTaggedValue> &callbackFnHandle);

    static JSTaggedValue MapUnStableJSArray(JSThread *thread, JSHandle<JSTaggedValue> &thisArgHandle,
        JSHandle<JSTaggedValue> &thisObjVal, int64_t k, int64_t len, JSHandle<JSObject> newArrayHandle,
        JSHandle<JSTaggedValue> &callbackFnHandle);

    static JSTaggedValue ReduceInner(EcmaRuntimeCallInfo *argv, int64_t len);

    static JSTaggedValue ReduceRightInner(EcmaRuntimeCallInfo *argv, int64_t len);
private:
#define BUILTIN_ARRAY_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsArray::method, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array ARRAY_FUNCTIONS  = {
        BUILTIN_ARRAY_FUNCTIONS(BUILTIN_ARRAY_FUNCTION_ENTRY)
    };
    static constexpr std::array ARRAY_PROTOTYPE_FUNCTIONS = {
        BUILTIN_ARRAY_PROTOTYPE_FUNCTIONS(BUILTIN_ARRAY_FUNCTION_ENTRY)
    };
#undef BUILTIN_ARRAY_FUNCTION_ENTRY

#if ENABLE_NEXT_OPTIMIZATION
    static JSTaggedValue IncludesStable(
        EcmaRuntimeCallInfo *argv, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue IncludesSlowPath(
        EcmaRuntimeCallInfo *argv, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue IncludesSlowPath(
        EcmaRuntimeCallInfo *argv, const JSHandle<JSTaggedValue> &thisObjVal, int64_t length, int64_t fromIndex);
#endif

    static JSTaggedValue IndexOfStable(
        EcmaRuntimeCallInfo *argv, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue IndexOfSlowPath(
        EcmaRuntimeCallInfo *argv, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue IndexOfSlowPath(
        EcmaRuntimeCallInfo *argv, const JSHandle<JSTaggedValue> &thisObjVal, int64_t length, int64_t fromIndex);

    static JSTaggedValue LastIndexOfStable(
        EcmaRuntimeCallInfo *argv, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue LastIndexOfSlowPath(
        EcmaRuntimeCallInfo *argv, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue LastIndexOfSlowPath(
        EcmaRuntimeCallInfo *argv, const JSHandle<JSTaggedValue> &thisObjVal, int64_t fromIndex);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_ARRAY_H
