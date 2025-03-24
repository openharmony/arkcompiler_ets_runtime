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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SHARED_TYPEDARRAY_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SHARED_TYPEDARRAY_H

#include "ecmascript/base/builtins_base.h"

#define SHARED_TYPED_ARRAY_TYPES(V)     \
    V(SharedInt8Array)                  \
    V(SharedUint8Array)                 \
    V(SharedUint8ClampedArray)          \
    V(SharedInt16Array)                 \
    V(SharedUint16Array)                \
    V(SharedInt32Array)                 \
    V(SharedUint32Array)                \
    V(SharedFloat32Array)               \
    V(SharedFloat64Array)               \
    V(SharedBigInt64Array)              \
    V(SharedBigUint64Array)

// All types of %TypedArray%.
// V(Type, ctorName, TYPE, bytesPerElement) where JSType::JS_##TYPE is the type index.
// todo: remove shared* when refactor the following macro
#define BUILTIN_SHARED_TYPED_ARRAY_TYPES(V)                                                          \
    V(Int8Array,         SharedInt8Array,         INT8_ARRAY,          1, SendableInt8Array)         \
    V(Uint8Array,        SharedUint8Array,        UINT8_ARRAY,         1, SendableUint8Array)        \
    V(Uint8ClampedArray, SharedUint8ClampedArray, UINT8_CLAMPED_ARRAY, 1, SendableUint8ClampedArray) \
    V(Int16Array,        SharedInt16Array,        INT16_ARRAY,         2, SendableInt16Array)        \
    V(Uint16Array,       SharedUint16Array,       UINT16_ARRAY,        2, SendableUint16Array)       \
    V(Int32Array,        SharedInt32Array,        INT32_ARRAY,         4, SendableInt32Array)        \
    V(Uint32Array,       SharedUint32Array,       UINT32_ARRAY,        4, SendableUint32Array)       \
    V(Float32Array,      SharedFloat32Array,      FLOAT32_ARRAY,       4, SendableFloat32Array)      \
    V(Float64Array,      SharedFloat64Array,      FLOAT64_ARRAY,       8, SendableFloat64Array)      \
    V(BigInt64Array,     SharedBigInt64Array,     BIGINT64_ARRAY,      8, SendableBigInt64Array)     \
    V(BigUint64Array,    SharedBigUint64Array,    BIGUINT64_ARRAY,     8, SendableBigUint64Array)

// List of functions in %TypedArray%, excluding the '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsSharedTypedArray::func refers to the native implementation of %TypedArray%[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_SHARED_TYPED_ARRAY_FUNCTIONS(V)                     \
    /* %TypedArray%.from ( source [ , mapfn [ , thisArg ] ] ) */    \
    V("from", From, 1, INVALID)                                     \
    /* %TypedArray%.of ( ...items ) */                              \
    V("of",   Of,   0, INVALID)

// List of get accessors in %TypedArray%.prototype, excluding the '@@' properties.
// V(name, func, stubIndex)
// where BuiltinsSharedTypedArray::func refers to the native implementation.
#define BUILTIN_SHARED_TYPED_ARRAY_PROTOTYPE_GETTERS(V)                                                     \
    V("buffer",     GetBuffer,     INVALID) /* get %TypedArray%.prototype.buffer */                         \
    V("byteLength", GetByteLength, INVALID) /* get %TypedArray%.prototype.byteLength */     \
    V("byteOffset", GetByteOffset, INVALID) /* get %TypedArray%.prototype.byteOffset */     \
    V("length",     GetLength,     INVALID) /* get %TypedArray%.prototype.length */

// List of functions in %TypedArray%.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsSharedTypedArray::func refers to the native implementation of %TypedArray%.prototype[name].
#define BUILTIN_SHARED_TYPED_ARRAY_PROTOTYPE_FUNCTIONS(V)                               \
    /* %TypedArray%.prototype.at ( index ) */                                           \
    V("at",             At,             1, INVALID)                                     \
    /* %TypedArray%.prototype.copyWithin ( target, start [ , end ] ) */                 \
    V("copyWithin",     CopyWithin,     2, INVALID)                                     \
    /* %TypedArray%.prototype.entries ( ) */                                            \
    V("entries",        Entries,        0, INVALID)                                     \
    /* %TypedArray%.prototype.every ( callbackfn [ , thisArg ] ) */                     \
    V("every",          Every,          1, INVALID)                                     \
    /* %TypedArray%.prototype.fill ( value [ , start [ , end ] ] ) */                   \
    V("fill",           Fill,           1, INVALID)                                     \
    /* %TypedArray%.prototype.filter ( callbackfn [ , thisArg ] ) */                    \
    V("filter",         Filter,         1, INVALID)                                     \
    /* %TypedArray%.prototype.find ( predicate [ , thisArg ] ) */                       \
    V("find",           Find,           1, INVALID)                                     \
    /* %TypedArray%.prototype.findIndex ( predicate [ , thisArg ] ) */                  \
    V("findIndex",      FindIndex,      1, INVALID)                                     \
    /* %TypedArray%.prototype.forEach ( callbackfn [ , thisArg ] ) */                   \
    V("forEach",        ForEach,        1, INVALID)                                     \
    /* %TypedArray%.prototype.includes ( searchElement [ , fromIndex ] ) */             \
    V("includes",       Includes,       1, INVALID)                                     \
    /* %TypedArray%.prototype.indexOf ( searchElement [ , fromIndex ] ) */              \
    V("indexOf",        IndexOf,        1, INVALID)                                     \
    /* %TypedArray%.prototype.lastIndexOf ( searchElement [ , fromIndex ] ) */          \
    V("lastIndexOf",    LastIndexOf,    1, INVALID)                                     \
    /* %TypedArray%.prototype.join ( separator ) */                                     \
    V("join",           Join,           1, INVALID)                                     \
    /* %TypedArray%.prototype.keys ( ) */                                               \
    V("keys",           Keys,           0, INVALID)                                     \
    /* %TypedArray%.prototype.map ( callbackfn [ , thisArg ] ) */                       \
    V("map",            Map,            1, INVALID)                                     \
    /* %TypedArray%.prototype.reduce ( callbackfn [ , initialValue ] ) */               \
    V("reduce",         Reduce,         1, INVALID)                                     \
    /* %TypedArray%.prototype.reduceRight ( callbackfn [ , initialValue ] ) */          \
    V("reduceRight",    ReduceRight,    1, INVALID)                                     \
    /* %TypedArray%.prototype.reverse ( ) */                                            \
    V("reverse",        Reverse,        0, INVALID)                                     \
    /* %TypedArray%.prototype.set ( source [ , offset ] ) */                            \
    V("set",            Set,            1, INVALID)                                     \
    /* %TypedArray%.prototype.slice ( start, end ) */                                   \
    V("slice",          Slice,          2, INVALID)                                     \
    /* %TypedArray%.prototype.some ( callbackfn [ , thisArg ] ) */                      \
    V("some",           Some,           1, INVALID)                                     \
    /* %TypedArray%.prototype.sort ( comparefn ) */                                     \
    V("sort",           Sort,           1, INVALID)                                     \
    /* %TypedArray%.prototype.subarray ( begin, end ) */                                \
    V("subarray",       Subarray,       2, INVALID)                                     \
    /* %TypedArray%.prototype.toLocaleString ( [ reserved1 [ , reserved2 ] ] ) */       \
    V("toLocaleString", ToLocaleString, 0, INVALID)                                     \
    /* %TypedArray%.prototype.toString ( ) */                                           \
    V("toString",      ToString,        0, INVALID)                                     \
    /* %TypedArray%.prototype.values ( ) */                                             \
    V("values",         Values,         0, INVALID)

namespace panda::ecmascript::builtins {
class BuiltinsSharedTypedArray : public base::BuiltinsBase {
public:
    enum SeparatorFlag : int { MINUS_ONE = -1, MINUS_TWO = -2 };
    static JSTaggedValue TypedArrayBaseConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Int8ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Uint8ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Uint8ClampedArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Int16ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Uint16ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Int32ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Uint32ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Float32ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Float64ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue BigInt64ArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue BigUint64ArrayConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue AllocateTypedArray(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue From(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Of(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);

    // prototype
    static JSTaggedValue GetBuffer(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetByteLength(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetByteOffset(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue CopyWithin(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Every(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Fill(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Filter(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Find(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue FindIndex(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue LastIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Join(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLength(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Map(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Reduce(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReduceRight(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Reverse(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Set(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Slice(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Some(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Sort(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Subarray(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ToStringTag(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Includes(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue At(EcmaRuntimeCallInfo *argv);
    static const uint32_t MAX_ARRAY_INDEX = std::numeric_limits<uint32_t>::max();

    // Excluding the '@@' internal properties
    static Span<const base::BuiltinFunctionEntry> GetTypedArrayFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(TYPED_ARRAY_FUNCTIONS);
    }

    // Excluding the '@@' internal properties
    static Span<const base::BuiltinFunctionEntry> GetTypedArrayPrototypeAccessors()
    {
        return Span<const base::BuiltinFunctionEntry>(TYPED_ARRAY_PROTOTYPE_ACCESSORS);
    }

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetTypedArrayPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(TYPED_ARRAY_PROTOTYPE_FUNCTIONS);
    }

    static size_t GetNumPrototypeInlinedProperties()
    {
        // 3 : 3 more inline properties in %TypedArray%.prototype for the following functions/accessors:
        //   (1) %TypedArray%.prototype.constructor
        //   (2) %TypedArray%.prototype[@@iterator]
        //   (3) %TypedArray%.prototype[@@toStringTag]
        return GetTypedArrayPrototypeFunctions().Size() +
               GetTypedArrayPrototypeAccessors().Size() + 3;
    }

    static Span<const std::pair<std::string_view, bool>> GetPrototypeProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(TYPED_ARRAY_PROTOTYPE_PROPERTIES);
    }

    static Span<const std::pair<std::string_view, bool>> GetFunctionProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(TYPED_ARRAY_FUNCTION_PROPERTIES);
    }

    static Span<const std::pair<std::string_view, bool>> GetSpecificFunctionProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(SPECIFIC_TYPED_ARRAY_FUNCTION_PROPERTIES);
    }

    static Span<const std::pair<std::string_view, bool>> GetSpecificArrayPrototypeProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(SPECIFIC_TYPED_ARRAY_PROTOTYPE_PROPERTIES);
    }

private:
#define BUILTIN_TYPED_ARRAY_FUNCTION_ENTRY(name, func, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsSharedTypedArray::func, length, BUILTINS_STUB_ID(id)),
#define BUILTIN_TYPED_ARRAY_ACCESSOR_ENTRY(name, func, id)                          \
    base::BuiltinFunctionEntry::Create<base::BuiltinFunctionEntry::IsAccessorBit>(  \
        name, BuiltinsSharedTypedArray::func, 0, BUILTINS_STUB_ID(id)),

    static constexpr std::array TYPED_ARRAY_FUNCTIONS = {
        BUILTIN_SHARED_TYPED_ARRAY_FUNCTIONS(BUILTIN_TYPED_ARRAY_FUNCTION_ENTRY)
    };
    static constexpr std::array TYPED_ARRAY_PROTOTYPE_ACCESSORS = {
        BUILTIN_SHARED_TYPED_ARRAY_PROTOTYPE_GETTERS(BUILTIN_TYPED_ARRAY_ACCESSOR_ENTRY)
    };
    static constexpr std::array TYPED_ARRAY_PROTOTYPE_FUNCTIONS = {
        BUILTIN_SHARED_TYPED_ARRAY_PROTOTYPE_FUNCTIONS(BUILTIN_TYPED_ARRAY_FUNCTION_ENTRY)
    };

#define TYPED_ARRAY_PROPERTIES_PAIR(name, func, length, id) \
    std::pair<std::string_view, bool>(name, false),

    static constexpr std::array TYPED_ARRAY_PROTOTYPE_PROPERTIES = {
        std::pair<std::string_view, bool>("constructor", false),
        BUILTIN_SHARED_TYPED_ARRAY_PROTOTYPE_FUNCTIONS(TYPED_ARRAY_PROPERTIES_PAIR)
        std::pair<std::string_view, bool>("buffer", true),
        std::pair<std::string_view, bool>("byteLength", true),
        std::pair<std::string_view, bool>("byteOffset", true),
        std::pair<std::string_view, bool>("length", true),
        std::pair<std::string_view, bool>("[Symbol.iterator]", false),
        std::pair<std::string_view, bool>("[Symbol.toStringTag]", true),
    };

    static constexpr std::array TYPED_ARRAY_FUNCTION_PROPERTIES = {
        std::pair<std::string_view, bool>("length", false),
        std::pair<std::string_view, bool>("name", false),
        std::pair<std::string_view, bool>("prototype", false),
        BUILTIN_SHARED_TYPED_ARRAY_FUNCTIONS(TYPED_ARRAY_PROPERTIES_PAIR)
        std::pair<std::string_view, bool>("[Symbol.species]", true),
    };

    static constexpr std::array SPECIFIC_TYPED_ARRAY_FUNCTION_PROPERTIES = {
        std::pair<std::string_view, bool>("length", false),
        std::pair<std::string_view, bool>("name", false),
        std::pair<std::string_view, bool>("prototype", false),
        std::pair<std::string_view, bool>("BYTES_PER_ELEMENT", false),
    };

    static constexpr std::array SPECIFIC_TYPED_ARRAY_PROTOTYPE_PROPERTIES = {
        std::pair<std::string_view, bool>("constructor", false),
        std::pair<std::string_view, bool>("BYTES_PER_ELEMENT", false),
    };
#undef TYPED_ARRAY_PROPERTIES_PAIR

#undef BUILTIN_TYPED_ARRAY_FUNCTION_ENTRY
#undef BUILTIN_TYPED_ARRAY_ACCESSOR_ENTRY
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SHARED_TYPEDARRAY_H
