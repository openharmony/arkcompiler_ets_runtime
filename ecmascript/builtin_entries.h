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

#ifndef ECMASCRIPT_BUILTIN_ENTRIES_H
#define ECMASCRIPT_BUILTIN_ENTRIES_H

#include <atomic>
#include <cstdint>
#include <sstream>
#include <string>

#include "ecmascript/ecma_string.h"
#include "ecmascript/js_tagged_value.h"

#define BUILTIN_LIST(V)                                                          \
    V("Function", BT_FUNCTION)                                                   \
    V("RangeError", BT_RANGEERROR)                                               \
    V("Error", BT_ERROR)                                                         \
    V("Object", BT_OBJECT)                                                       \
    V("SyntaxError", BT_SYNTAXERROR)                                             \
    V("TypeError", BT_TYPEERROR)                                                 \
    V("ReferenceError", BT_REFERENCEERROR)                                       \
    V("URIError", BT_URIERROR)                                                   \
    V("Symbol", BT_SYMBOL)                                                       \
    V("EvalError", BT_EVALERROR)                                                 \
    V("Number", BT_NUMBER)                                                       \
    V("parseFloat", BT_PARSEFLOAT)                                               \
    V("Date", BT_DATE)                                                           \
    V("Boolean", BT_BOOLEAN)                                                     \
    V("BigInt", BT_BIGINT)                                                       \
    V("parseInt", BT_PARSEINT)                                                   \
    V("WeakMap", BT_WEAKMAP)                                                     \
    V("RegExp", BT_REGEXP)                                                       \
    V("Set", BT_SET)                                                             \
    V("Map", BT_MAP)                                                             \
    V("WeakRef", BT_WEAKREF)                                                     \
    V("WeakSet", BT_WEAKSET)                                                     \
    V("FinalizationRegistry", BT_FINALIZATIONREGISTRY)                           \
    V("Array", BT_ARRAY)                                                         \
    V("Uint8ClampedArray", BT_UINT8CLAMPEDARRAY)                                 \
    V("Uint8Array", BT_UINT8ARRAY)                                               \
    V("TypedArray", BT_TYPEDARRAY)                                               \
    V("Int8Array", BT_INT8ARRAY)                                                 \
    V("Uint16Array", BT_UINT16ARRAY)                                             \
    V("Uint32Array", BT_UINT32ARRAY)                                             \
    V("Int16Array", BT_INT16ARRAY)                                               \
    V("Int32Array", BT_INT32ARRAY)                                               \
    V("Float32Array", BT_FLOAT32ARRAY)                                           \
    V("Float64Array", BT_FLOAT64ARRAY)                                           \
    V("BigInt64Array", BT_BIGINT64ARRAY)                                         \
    V("BigUint64Array", BT_BIGUINT64ARRAY)                                       \
    V("SharedArrayBuffer", BT_SHAREDARRAYBUFFER)                                 \
    V("DataView", BT_DATAVIEW)                                                   \
    V("String", BT_STRING)                                                       \
    V("ArrayBuffer", BT_ARRAYBUFFER)                                             \
    V("eval", BT_EVAL)                                                           \
    V("isFinite", BT_ISFINITE)                                                   \
    V("ArkPrivate", BT_ARKPRIVATE) /*Readonly*/                                  \
    V("print", BT_PRINT)                                                         \
    V("decodeURI", BT_DECODEURI)                                                 \
    V("decodeURIComponent", BT_DECODEURICOMPONENT)                               \
    V("isNaN", BT_ISNAN)                                                         \
    V("encodeURI", BT_ENCODEURI)                                                 \
    V("encodeURIComponent", BT_ENCODEURICOMPONENT)                               \
    V("Math", BT_MATH)                                                           \
    V("JSON", BT_JSON)                                                           \
    V("Atomics", BT_ATOMICS)                                                     \
    V("Reflect", BT_REFLECT)                                                     \
    V("Promise", BT_PROMISE)                                                     \
    V("Proxy", BT_PROXY)                                                         \
    V("GeneratorFunction", BT_GENERATORFUNCTION)                                 \
    V("Intl", BT_INTL)

namespace panda::ecmascript {
#define BUILTIN_TYPE(name, type) type,
enum class BuiltinType : int32_t {
    BUILTIN_LIST(BUILTIN_TYPE)
    NUMBER_OF_BUILTINS
};
#undef BUILTIN_TYPE

struct BuiltinEntries {
    static constexpr size_t COUNT = static_cast<size_t>(BuiltinType::NUMBER_OF_BUILTINS);
    struct {
        JSTaggedValue box_ {JSTaggedValue::Hole()};
        JSTaggedValue hClass_ {JSTaggedValue::Hole()};
    } builtin_[COUNT];

    uintptr_t Begin()
    {
        return reinterpret_cast<uintptr_t>(builtin_);
    }

    uintptr_t End()
    {
        return reinterpret_cast<uintptr_t>(builtin_ + COUNT);
    }

    static constexpr size_t SizeArch32 = sizeof(uint64_t) * 2 * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * 2 * COUNT;
};
STATIC_ASSERT_EQ_ARCH(sizeof(BuiltinEntries), BuiltinEntries::SizeArch32, BuiltinEntries::SizeArch64);

class BuiltinIndex {
public:
    static const int32_t NOT_FOUND = -1;

    size_t GetBuiltinBoxOffset(JSTaggedValue key) const
    {
        auto index = GetBuiltinIndex(key);
        ASSERT(index != NOT_FOUND);
        return sizeof(JSTaggedValue) * (index * 2); // 2 is size of BuiltinEntries
    }

    int32_t GetBuiltinIndex(JSTaggedValue key) const
    {
        auto ecmaString = EcmaString::Cast(key.GetTaggedObject());
        auto str = std::string(ConvertToString(ecmaString));
        return GetBuiltinIndex(str);
    }

    int32_t GetBuiltinIndex(const std::string& key) const
    {
        if (builtinIndex_.find(key) == builtinIndex_.end()) {
            return NOT_FOUND;
        } else {
            return static_cast<int32_t>(builtinIndex_.at(key));
        }
    }

private:
#define BUILTIN_MAP(name, type) {name, BuiltinType::type},
    std::unordered_map<std::string, BuiltinType> builtinIndex_{
        BUILTIN_LIST(BUILTIN_MAP)
    };
#undef BUILTIN_MAP
};

// #undef BUILTIN_LIST
}
#endif // ECMASCRIPT_BUILTIN_ENTRIES_H