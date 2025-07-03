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

#include <cstddef>
#include <cstdint>
#include <string>

#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
enum class BuiltinType : int32_t {
    BT_FUNCTION,
    BT_RANGEERROR,
    BT_ERROR,
    BT_OBJECT,
    BT_SYNTAXERROR,
    BT_TYPEERROR,
    BT_REFERENCEERROR,
    BT_URIERROR,
    BT_SYMBOL,
    BT_EVALERROR,
    BT_NUMBER,
    BT_PARSEFLOAT,
    BT_DATE,
    BT_BOOLEAN,
    BT_BIGINT,
    BT_PARSEINT,
    BT_WEAKMAP,
    BT_REGEXP,
    BT_SET,
    BT_MAP,
    BT_WEAKREF,
    BT_WEAKSET,
    BT_FINALIZATIONREGISTRY,
    BT_ARRAY,
    BT_UINT8CLAMPEDARRAY,
    BT_UINT8ARRAY,
    BT_TYPEDARRAY,
    BT_INT8ARRAY,
    BT_UINT16ARRAY,
    BT_UINT32ARRAY,
    BT_INT16ARRAY,
    BT_INT32ARRAY,
    BT_FLOAT32ARRAY,
    BT_FLOAT64ARRAY,
    BT_BIGINT64ARRAY,
    BT_BIGUINT64ARRAY,
    BT_SHAREDARRAYBUFFER,
    BT_DATAVIEW,
    BT_STRING,
    BT_ARRAYBUFFER,
    BT_EVAL,
    BT_ISFINITE,
    BT_ARKPRIVATE,
    BT_PRINT,
    BT_DECODEURI,
    BT_DECODEURICOMPONENT,
    BT_ISNAN,
    BT_ENCODEURI,
    BT_ENCODEURICOMPONENT,
    BT_MATH,
    BT_JSON,
    BT_ATOMICS,
    BT_REFLECT,
    BT_PROMISE,
    BT_PROXY,
    BT_GENERATORFUNCTION,
    BT_INTL,
    NUMBER_OF_BUILTINS
};

struct BuiltinEntries {
    static constexpr size_t COUNT = static_cast<size_t>(BuiltinType::NUMBER_OF_BUILTINS);
    struct BuiltinEntry {
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

    void ClearByIndex(size_t index, JSTaggedValue value)
    {
        builtin_[index].box_ = value;
        builtin_[index].hClass_ = JSTaggedValue::Hole();
    }

    static constexpr size_t SizeArch32 = sizeof(uint64_t) * 2 * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * 2 * COUNT;
};
STATIC_ASSERT_EQ_ARCH(sizeof(BuiltinEntries), BuiltinEntries::SizeArch32, BuiltinEntries::SizeArch64);

class BuiltinIndex {
public:
    BuiltinIndex(const BuiltinIndex&) = delete;
    BuiltinIndex& operator=(const BuiltinIndex&) = delete;

    static const size_t NOT_FOUND = -1;

    static BuiltinIndex& GetInstance()
    {
        static BuiltinIndex instance;
        return instance;
    }

    size_t GetBuiltinBoxOffset(const JSThread *thread, JSTaggedValue key) const
    {
        auto index = GetBuiltinIndex(thread, key);
        ASSERT(index != NOT_FOUND);
        return sizeof(JSTaggedValue) * (index * 2); // 2 is size of BuiltinEntries
    }

    size_t GetBuiltinBoxOffset(size_t index)
    {
        return sizeof(JSTaggedValue) * (index * 2); // 2 is size of BuiltinEntries
    }

    size_t PUBLIC_API GetBuiltinIndex(const JSThread *thread, JSTaggedValue key) const;
    size_t PUBLIC_API GetBuiltinIndex(const std::string& key) const;

    auto GetBuiltinKeys() -> std::vector<std::string> const
    {
        std::vector<std::string> keys;
        for (const auto& it: builtinIndex_) {
            keys.emplace_back(it.first);
        }
        return keys;
    }

private:
    std::unordered_map<std::string, BuiltinType> builtinIndex_ = {
        {"Function", BuiltinType::BT_FUNCTION},
        {"RangeError", BuiltinType::BT_RANGEERROR},
        {"Error", BuiltinType::BT_ERROR},
        {"Object", BuiltinType::BT_OBJECT},
        {"SyntaxError", BuiltinType::BT_SYNTAXERROR},
        {"TypeError", BuiltinType::BT_TYPEERROR},
        {"ReferenceError", BuiltinType::BT_REFERENCEERROR},
        {"URIError", BuiltinType::BT_URIERROR},
        {"Symbol", BuiltinType::BT_SYMBOL},
        {"EvalError", BuiltinType::BT_EVALERROR},
        {"Number", BuiltinType::BT_NUMBER},
        {"parseFloat", BuiltinType::BT_PARSEFLOAT},
        {"Date", BuiltinType::BT_DATE},
        {"Boolean", BuiltinType::BT_BOOLEAN},
        {"BigInt", BuiltinType::BT_BIGINT},
        {"parseInt", BuiltinType::BT_PARSEINT},
        {"WeakMap", BuiltinType::BT_WEAKMAP},
        {"RegExp", BuiltinType::BT_REGEXP},
        {"Set", BuiltinType::BT_SET},
        {"Map", BuiltinType::BT_MAP},
        {"WeakRef", BuiltinType::BT_WEAKREF},
        {"WeakSet", BuiltinType::BT_WEAKSET},
        {"FinalizationRegistry", BuiltinType::BT_FINALIZATIONREGISTRY},
        {"Array", BuiltinType::BT_ARRAY},
        {"Uint8ClampedArray", BuiltinType::BT_UINT8CLAMPEDARRAY},
        {"Uint8Array", BuiltinType::BT_UINT8ARRAY},
        {"TypedArray", BuiltinType::BT_TYPEDARRAY},
        {"Int8Array", BuiltinType::BT_INT8ARRAY},
        {"Uint16Array", BuiltinType::BT_UINT16ARRAY},
        {"Uint32Array", BuiltinType::BT_UINT32ARRAY},
        {"Int16Array", BuiltinType::BT_INT16ARRAY},
        {"Int32Array", BuiltinType::BT_INT32ARRAY},
        {"Float32Array", BuiltinType::BT_FLOAT32ARRAY},
        {"Float64Array", BuiltinType::BT_FLOAT64ARRAY},
        {"BigInt64Array", BuiltinType::BT_BIGINT64ARRAY},
        {"BigUint64Array", BuiltinType::BT_BIGUINT64ARRAY},
        {"SharedArrayBuffer", BuiltinType::BT_SHAREDARRAYBUFFER},
        {"DataView", BuiltinType::BT_DATAVIEW},
        {"String", BuiltinType::BT_STRING},
        {"ArrayBuffer", BuiltinType::BT_ARRAYBUFFER},
        {"eval", BuiltinType::BT_EVAL},
        {"isFinite", BuiltinType::BT_ISFINITE},
        {"ArkPrivate", BuiltinType::BT_ARKPRIVATE},
        {"print", BuiltinType::BT_PRINT},
        {"decodeURI", BuiltinType::BT_DECODEURI},
        {"decodeURIComponent", BuiltinType::BT_DECODEURICOMPONENT},
        {"isNaN", BuiltinType::BT_ISNAN},
        {"encodeURI", BuiltinType::BT_ENCODEURI},
        {"encodeURIComponent", BuiltinType::BT_ENCODEURICOMPONENT},
        {"Math", BuiltinType::BT_MATH},
        {"JSON", BuiltinType::BT_JSON},
        {"Atomics", BuiltinType::BT_ATOMICS},
        {"Reflect", BuiltinType::BT_REFLECT},
        {"Promise", BuiltinType::BT_PROMISE},
        {"Proxy", BuiltinType::BT_PROXY},
        {"GeneratorFunction", BuiltinType::BT_GENERATORFUNCTION},
        {"Intl", BuiltinType::BT_INTL},
    };

    BuiltinIndex() {}
}; // class BuiltinIndex
} // namespace panda::ecmascript
#endif // ECMASCRIPT_BUILTIN_ENTRIES_H
