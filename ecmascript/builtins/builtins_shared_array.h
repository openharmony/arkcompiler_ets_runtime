/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SHARED_ARRAY_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SHARED_ARRAY_H

#include "ecmascript/base/builtins_base.h"

// List of functions in Shared Array, excluding the '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsSharedArray::func refers to the native implementation of SharedArray[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_SHARED_ARRAY_FUNCTIONS(V)                      \
    /* SharedArray.from ( items [ , mapfn [ , thisArg ] ] ) */ \
    V("from", From, 1, INVALID)
    // fixme(hzzhouzebin) Support later.
    // /* SharedArray.IsArray ( arg ) */                          \
    // V("IsArray", IsArray, 1, INVALID)                          \
    // /* SharedArray.of ( ...items ) */                          \
    // V("of", Of, 0, INVALID)

// List of functions in SharedArray.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsSharedArray::func refers to the native implementation of SharedArray.prototype[name].
#define BUILTIN_SHARED_ARRAY_PROTOTYPE_FUNCTIONS(V)                              \
    /* SharedArray.prototype.at ( index ) */                                     \
    V("at", At, 1, INVALID)                                                      \
    /* SharedArray.prototype.concat ( ...items ) */                              \
    V("concat", Concat, 1, INVALID)                                              \
    /* SharedArray.prototype.entries ( ) */                                      \
    V("entries", Entries, 0, INVALID)                                            \
    /* SharedArray.prototype.fill ( value [ , start [ , end ] ] ) */             \
    V("fill", Fill, 1, INVALID)                                                  \
    /* SharedArray.prototype.filter ( callbackfn [ , thisArg ] ) */              \
    V("filter", Filter, 1, INVALID)                                              \
    /* SharedArray.prototype.find ( predicate [ , thisArg ] ) */                 \
    V("find", Find, 1, INVALID)                                                  \
    /* SharedArray.prototype.findIndex ( predicate [ , thisArg ] ) */            \
    V("findIndex", FindIndex, 1, INVALID)                                        \
    /* SharedArray.prototype.flat ( [ depth ] ) */                               \
    V("flat", Flat, 0, INVALID)                                                  \
    /* SharedArray.prototype.flatMap ( mapperFunction [ , thisArg ] ) */         \
    V("flatMap", FlatMap, 1, INVALID)                                            \
    /* SharedArray.prototype.forEach ( callbackfn [ , thisArg ] ) */             \
    V("forEach", ForEach, 1, INVALID)                                            \
    /* SharedArray.prototype.includes ( searchElement [ , fromIndex ] ) */       \
    V("includes", Includes, 1, INVALID)                                          \
    /* SharedArray.prototype.indexOf ( searchElement [ , fromIndex ] ) */        \
    V("indexOf", IndexOf, 1, INVALID)                                            \
    /* SharedArray.prototype.join ( separator ) */                               \
    V("join", Join, 1, INVALID)                                                  \
    /* SharedArray.prototype.keys ( ) */                                         \
    V("keys", Keys, 0, INVALID)                                                  \
    /* SharedArray.prototype.map ( callbackfn [ , thisArg ] ) */                 \
    V("map", Map, 1, INVALID)                                                    \
    /* SharedArray.prototype.pop ( ) */                                          \
    V("pop", Pop, 0, INVALID)                                                    \
    /* SharedArray.prototype.push ( ...items ) */                                \
    V("push", Push, 1, INVALID)                                                  \
    /* SharedArray.prototype.reduce ( callbackfn [ , initialValue ] ) */         \
    V("reduce", Reduce, 1, INVALID)                                              \
    /* SharedArray.prototype.shift ( ) */                                        \
    V("shift", Shift, 0, INVALID)                                                \
    /* SharedArray.prototype.slice ( start, end ) */                             \
    V("slice", Slice, 2, INVALID)                                                \
    /* SharedArray.prototype.sort ( comparefn ) */                               \
    V("sort", Sort, 1, INVALID)                                                  \
    /* SharedArray.prototype.toString ( ) */                                     \
    V("toString", ToString, 0, INVALID)                                          \
    /* SharedArray.prototype.values ( ) */                                       \
    /* SharedArray.prototype.unshift ( ...items ) */                             \
    V("unshift", Unshift, 1, INVALID)                                            \
    V("values", Values, 0, INVALID)
    // fixme(hzzhouzebin) Support later.
    // /* SharedArray.prototype.with ( index, value ) */                            \
    // V("with", With, 2, INVALID)                                                  \
    // /* SharedArray.prototype.reduceRight ( callbackfn [ , initialValue ] ) */    \
    // V("reduceRight", ReduceRight, 1, INVALID)                                    \
    // /* SharedArray.prototype.reverse ( ) */                                      \
    // V("reverse", Reverse, 0, INVALID)                                            \
    // /* SharedArray.prototype.copyWithin ( target, start [ , end ] ) */           \
    // V("copyWithin", CopyWithin, 2, INVALID)                                      \
    // /* SharedArray.prototype.every ( callbackfn [ , thisArg ] ) */               \
    // V("every", Every, 1, INVALID)                                                \
    // /* SharedArray.prototype.findLast ( predicate [ , thisArg ] ) */             \
    // V("findLast", FindLast, 1, INVALID)                                          \
    // /* SharedArray.prototype.findLastIndex ( predicate [ , thisArg ] ) */        \
    // V("findLastIndex", FindLastIndex, 1, INVALID)                                \
    // /* SharedArray.prototype.lastIndexOf ( searchElement [ , fromIndex ] ) */    \
    // V("lastIndexOf", LastIndexOf, 1, INVALID)                                    \
    // /* SharedArray.prototype.some ( callbackfn [ , thisArg ] ) */                \
    // V("some", Some, 1, INVALID)                                                  \
    // /* SharedArray.prototype.splice ( start, deleteCount, ...items ) */          \
    // V("splice", Splice, 2, INVALID)                                              \
    // /* SharedArray.prototype.toLocaleString ( [ reserved1 [ , reserved2 ] ] ) */ \
    // V("toLocaleString", ToLocaleString, 0, INVALID)                              \
    // /* SharedArray.prototype.toReversed ( ) */                                   \
    // V("toReversed", ToReversed, 0, INVALID)                                      \
    // /* SharedArray.prototype.toSorted ( comparefn ) */                           \
    // V("toSorted", ToSorted, 1, INVALID)                                          \
    // /* SharedArray.prototype.toSpliced ( start, skipCount, ...items ) */         \
    // V("toSpliced", ToSpliced, 2, INVALID)

namespace panda::ecmascript::builtins {
class BuiltinsSharedArray : public base::BuiltinsBase {
public:
    // 22.1.1
    static JSTaggedValue ArrayConstructor(EcmaRuntimeCallInfo *argv);

    // 22.1.2.1
    static JSTaggedValue From(EcmaRuntimeCallInfo *argv);
    // 22.1.2.5
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);

    // prototype
    // 22.1.3.1
    static JSTaggedValue Concat(EcmaRuntimeCallInfo *argv);
    // 22.1.3.4
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
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
    // 22.1.3.15
    static JSTaggedValue Map(EcmaRuntimeCallInfo *argv);
    // 22.1.3.16
    static JSTaggedValue Pop(EcmaRuntimeCallInfo *argv);
    // 22.1.3.17
    static JSTaggedValue Push(EcmaRuntimeCallInfo *argv);
    // 22.1.3.18
    static JSTaggedValue Reduce(EcmaRuntimeCallInfo *argv);
    // 22.1.3.21
    static JSTaggedValue Shift(EcmaRuntimeCallInfo *argv);
    // 22.1.3.22
    static JSTaggedValue Slice(EcmaRuntimeCallInfo *argv);
    // 22.1.3.24
    static JSTaggedValue Sort(EcmaRuntimeCallInfo *argv);
    // 22.1.3.27
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv); // no change
    // 22.1.3.28
    static JSTaggedValue Unshift(EcmaRuntimeCallInfo *argv); // done
    // 22.1.3.29
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv); // no change
    // 22.1.3.31
    static JSTaggedValue Unscopables(EcmaRuntimeCallInfo *argv); // no change
    // es12 23.1.3.13
    static JSTaggedValue Includes(EcmaRuntimeCallInfo *argv); // no change
    // es12 23.1.3.10
    static JSTaggedValue Flat(EcmaRuntimeCallInfo *argv);
    // es12 23.1.3.11
    static JSTaggedValue FlatMap(EcmaRuntimeCallInfo *argv);
    // 23.1.3.1 Array.prototype.at ( index )
    static JSTaggedValue At(EcmaRuntimeCallInfo *argv); // no change

    // Excluding the '@@' internal properties
    static Span<const base::BuiltinFunctionEntry> GetSharedArrayFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(SENDABLE_ARRAY_FUNCTIONS);
    }

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetSharedArrayPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(SENDABLE_ARRAY_PROTOTYPE_FUNCTIONS);
    }

    static size_t GetNumPrototypeInlinedProperties()
    {
        // 4 : 4 More inlined entries in SharedArray.prototype for the following functions/accessors:
        //   (1) 'length' accessor
        //   (2) SharedArray.prototype.constructor, i.e. Array()
        //   (3) SharedArray.prototype[@@iterator]()
        //   (4) SharedArray.prototype[@@unscopables]()
        return GetSharedArrayPrototypeFunctions().Size() + 4;
    }
    static JSTaggedValue ReduceUnStableJSArray(JSThread *thread, JSHandle<JSTaggedValue> &thisHandle,
        JSHandle<JSTaggedValue> &thisObjVal, int64_t k, int64_t len, JSMutableHandle<JSTaggedValue> &accumulator,
        JSHandle<JSTaggedValue> &callbackFnHandle);

    static JSTaggedValue FilterUnStableJSArray(JSThread *thread, JSHandle<JSTaggedValue> &thisArgHandle,
        JSHandle<JSTaggedValue> &thisObjVal, int64_t k, int64_t len, uint32_t toIndex,
        JSHandle<JSObject> newArrayHandle, JSHandle<JSTaggedValue> &callbackFnHandle);

    static Span<const std::pair<std::string_view, bool>> GetPrototypeProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(ARRAY_PROTOTYPE_PROPERTIES);
    }
    static Span<const std::pair<std::string_view, bool>> GetFunctionProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(ARRAY_FUNCTION_PROPERTIES);
    }
private:
    static JSTaggedValue PopInner(EcmaRuntimeCallInfo *argv, JSHandle<JSTaggedValue> &thisHandle,
                                  JSHandle<JSObject> &thisObjHandle);
#define BUILTIN_SENDABLE_ARRAY_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsSharedArray::method, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array SENDABLE_ARRAY_FUNCTIONS  = {
        BUILTIN_SHARED_ARRAY_FUNCTIONS(BUILTIN_SENDABLE_ARRAY_FUNCTION_ENTRY)
    };
    static constexpr std::array SENDABLE_ARRAY_PROTOTYPE_FUNCTIONS = {
        BUILTIN_SHARED_ARRAY_PROTOTYPE_FUNCTIONS(BUILTIN_SENDABLE_ARRAY_FUNCTION_ENTRY)
    };
#undef BUILTIN_SENDABLE_ARRAY_FUNCTION_ENTRY

    static JSTaggedValue IndexOfStable(
        EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue IndexOfSlowPath(
        EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue IndexOfSlowPath(
        EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal,
        int64_t length, int64_t fromIndex);

    static JSTaggedValue LastIndexOfStable(
        EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue LastIndexOfSlowPath(
        EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue LastIndexOfSlowPath(
        EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal, int64_t fromIndex);
#define ARRAY_PROPERTIES_PAIR(name, func, length, id) \
    std::pair<std::string_view, bool>(name, false),
    static constexpr std::array ARRAY_PROTOTYPE_PROPERTIES = {
        std::pair<std::string_view, bool>("length", false),
        std::pair<std::string_view, bool>("constructor", false),
        BUILTIN_SHARED_ARRAY_PROTOTYPE_FUNCTIONS(ARRAY_PROPERTIES_PAIR)
        std::pair<std::string_view, bool>("[Symbol.iterator]", false),
        std::pair<std::string_view, bool>("[Symbol.unscopables]", false)
    };
    static constexpr std::array ARRAY_FUNCTION_PROPERTIES = {
        std::pair<std::string_view, bool>("length", false),
        std::pair<std::string_view, bool>("name", false),
        std::pair<std::string_view, bool>("prototype", false),
        BUILTIN_SHARED_ARRAY_FUNCTIONS(ARRAY_PROPERTIES_PAIR)
        std::pair<std::string_view, bool>("[Symbol.species]", true),
    };
#undef ARRAY_PROPERTIES_PAIR
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SHARED_ARRAY_H
