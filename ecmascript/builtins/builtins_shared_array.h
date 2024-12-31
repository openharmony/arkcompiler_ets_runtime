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
    V("from", From, 1, INVALID)                                \
    V("create", Create, 2, INVALID)                            \
    /* SendableArray.isArray ( arg ) */                        \
    V("isArray", IsArray, 1, INVALID)                          \
    V("of", Of, 1, INVALID)                                    \
    // fixme(hzzhouzebin) Support later.
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
    /* SharedArray.prototype.toLocaleString ( [ reserved1 [ , reserved2 ] ] ) */ \
    V("toLocaleString", ToLocaleString, 0, INVALID)                              \
    /* SharedArray.prototype.values ( ) */                                       \
    /* SharedArray.prototype.unshift ( ...items ) */                             \
    V("unshift", Unshift, 1, INVALID)                                            \
    V("values", Values, 0, INVALID)                                              \
    V("shrinkTo", ShrinkTo, 0, INVALID)                                          \
    V("extendTo", ExtendTo, 0, INVALID)                                          \
    /* SharedArray.prototype.splice ( start, deleteCount, ...items ) */          \
    V("splice", Splice, 2, INVALID)                                              \
    /* SharedArray.prototype.every ( callbackfn [ , thisArg ] ) */               \
    V("every", Every, 1, INVALID)                                                \
    /* SharedArray.prototype.some ( callbackfn [ , thisArg ] ) */                \
    V("some", Some, 1, INVALID)                                                  \
    /* SendableArray.prototype.lastIndexOf ( searchElement [ , fromIndex ] ) */  \
    V("lastIndexOf", LastIndexOf, 1, INVALID)                                    \
	/* SharedArray.prototype.copyWithin ( target, start [ , end ] ) */           \
    V("copyWithin", CopyWithin, 2, INVALID)                                      \
	/* SharedArray.prototype.reduceRight ( callbackfn [ , initialValue ] ) */    \
    V("reduceRight", ReduceRight, 1, INVALID)                                    \
    /*  SharedArray.prototype.reverse ( )               */                       \
    V("reverse", Reverse, 0, INVALID)                                            \
	/* SharedArray.prototype.findLast ( predicate [ , thisArg ] ) */             \
    V("findLast", FindLast, 1, INVALID)                                          \
    /* SharedArray.prototype.findLastIndex ( predicate [ , thisArg ] ) */        \
    V("findLastIndex", FindLastIndex, 1, INVALID)                                \
    // fixme(hzzhouzebin) Support later.
    // /* SharedArray.prototype.with ( index, value ) */                            \
    // V("with", With, 2, INVALID)                                                  \
    // /* SharedArray.prototype.toReversed ( ) */                                   \
    // V("toReversed", ToReversed, 0, INVALID)                                      \
    // /* SharedArray.prototype.toSorted ( comparefn ) */                           \
    // V("toSorted", ToSorted, 1, INVALID)                                          \
    // /* SharedArray.prototype.toSpliced ( start, skipCount, ...items ) */         \
    // V("toSpliced", ToSpliced, 2, INVALID)

namespace panda::ecmascript::builtins {
class BuiltinsSharedArray : public base::BuiltinsBase {
public:
    static JSTaggedValue ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue From(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Create(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IsArray(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);

    // prototype
    static JSTaggedValue Concat(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Fill(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Filter(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Find(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue FindIndex(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Join(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Map(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Pop(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Push(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Reduce(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Shift(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Slice(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Splice(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Sort(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Unshift(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Unscopables(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Includes(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue At(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ShrinkTo(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ExtendTo(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Every(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Some(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue LastIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Of(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue CopyWithin(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue FindLast(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue FindLastIndex(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Reverse(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReduceRight(EcmaRuntimeCallInfo *argv);

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
        JSHandle<JSObject> &thisObjHandle, int64_t k, int64_t len, JSMutableHandle<JSTaggedValue> &accumulator,
        JSHandle<JSTaggedValue> &callbackFnHandle);

    static JSTaggedValue FilterArray(JSThread *thread, JSHandle<JSTaggedValue> &thisArgHandle,
        JSHandle<JSTaggedValue> &thisObjVal, JSHandle<JSObject> newArrayHandle,
        JSHandle<JSTaggedValue> &callbackFnHandle);

    static Span<const std::pair<std::string_view, bool>> GetPrototypeProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(ARRAY_PROTOTYPE_PROPERTIES);
    }
    static Span<const std::pair<std::string_view, bool>> GetFunctionProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(ARRAY_FUNCTION_PROPERTIES);
    }

    static inline uint64_t ConvertTagValueToInteger(JSThread *thread, JSHandle<JSTaggedValue>& number, int64_t len);
    static inline uint64_t GetNumberArgVal(JSThread *thread, EcmaRuntimeCallInfo *argv, uint32_t idx, int64_t len,
                                           int64_t defVal);
    static inline uint64_t GetNumberArgValThrow(JSThread *thread, EcmaRuntimeCallInfo *argv, uint32_t idx,
                                                int64_t len, const char* err);

    static inline void SetElementValue(JSThread *thread, JSHandle<JSObject> arrHandle, uint32_t key,
                                       const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue FastReverse(JSThread *thread, JSHandle<TaggedArray>& elements,
                                            uint32_t lower, uint32_t len, ElementsKind kind);

private:
    static inline JSTaggedValue GetElementByKey(JSThread *thread, JSHandle<JSObject>& thisObjHandle, uint32_t index);
    static JSTaggedValue PopInner(EcmaRuntimeCallInfo *argv, JSHandle<JSTaggedValue> &thisHandle,
                                  JSHandle<JSObject> &thisObjHandle);
    static int64_t FillNewTaggedArray(JSThread *thread, EcmaRuntimeCallInfo *argv, int argc,
                                      int64_t newArrayIdx, JSHandle<TaggedArray> &eleArray);
    static int64_t CalNewArrayLen(JSThread *thread, EcmaRuntimeCallInfo *argv, int argc);

#define BUILTIN_SENDABLE_ARRAY_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsSharedArray::method, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array SENDABLE_ARRAY_FUNCTIONS  = {
        BUILTIN_SHARED_ARRAY_FUNCTIONS(BUILTIN_SENDABLE_ARRAY_FUNCTION_ENTRY)
    };
    static constexpr std::array SENDABLE_ARRAY_PROTOTYPE_FUNCTIONS = {
        BUILTIN_SHARED_ARRAY_PROTOTYPE_FUNCTIONS(BUILTIN_SENDABLE_ARRAY_FUNCTION_ENTRY)
    };
#undef BUILTIN_SENDABLE_ARRAY_FUNCTION_ENTRY

    static JSTaggedValue IndexOfSlowPath(
        EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle);

    static JSTaggedValue IndexOfSlowPath(
        EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal,
        int64_t length, int64_t fromIndex);

    static JSTaggedValue LastIndexOfSlowPath(EcmaRuntimeCallInfo *argv, JSThread *thread,
                                             const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue LastIndexOfSlowPath(EcmaRuntimeCallInfo *argv, JSThread *thread,
                                             const JSHandle<JSTaggedValue> &thisObjVal, int64_t fromIndex);
    static JSTaggedValue ToLocaleStringInternalHandle(EcmaRuntimeCallInfo *argv, JSThread *thread,
            EcmaContext *context, ObjectFactory *factory, const JSHandle<JSTaggedValue> &thisHandle, int64_t len);
    static JSTaggedValue ReduceRightInternalHandle(EcmaRuntimeCallInfo *argv, JSThread *thread,
            const JSHandle<JSTaggedValue> &thisHandle, JSHandle<JSObject>& thisObjHandle, uint32_t argc, int64_t len);

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
    static JSTaggedValue CheckElementMeetReq(JSThread *thread,
                                             JSHandle<JSTaggedValue> &thisObjVal,
                                             JSHandle<JSTaggedValue> &callbackFnHandle, bool isSome);
#undef ARRAY_PROPERTIES_PAIR

#define ARRAY_CHECK_SHARED_ARRAY(errMsg)                                            \
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);                             \
    if (UNLIKELY(!thisHandle->IsJSSharedArray())) {                                 \
        auto error = ContainerError::BindError(thread, errMsg);                     \
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception()); \
    }
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SHARED_ARRAY_H
