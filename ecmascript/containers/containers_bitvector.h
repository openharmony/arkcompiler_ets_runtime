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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_BIT_VECTOR_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_BIT_VECTOR_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

#define BUILTIN_BITVECTOR_PROTOTYPE_FUNCTIONS(V)                                 \
    V("push", Push, 1, INVALID)                                                  \
    V("pop", Pop, 0, INVALID)                                                    \
    V("has", Has, 3, INVALID)                                                    \
    V("setBitsByRange", SetBitsByRange, 3, INVALID)                              \
    V("getBitsByRange", GetBitsByRange, 2, INVALID)                              \
    V("setAllBits", SetAllBits, 1, INVALID)                                      \
    V("resize", Resize, 1, INVALID)                                              \
    V("getBitCountByRange", GetBitCountByRange, 3, INVALID)                      \
    V("getIndexOf", GetIndexOf, 3, INVALID)                                      \
    V("getLastIndexOf", GetLastIndexOf, 3, INVALID)                              \
    V("flipBitByIndex", FlipBitByIndex, 1, INVALID)                              \
    V("flipBitsByRange", FlipBitsByRange, 2, INVALID)                            \
    V("values", GetIteratorObj, 0, INVALID)

namespace panda::ecmascript::containers {
class ContainersBitVector : public base::BuiltinsBase {
public:
    static JSTaggedValue BitVectorConstructor(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue Push(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue Pop(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue SetBitsByRange(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetBitsByRange(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue Resize(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetBitCountByRange(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetIndexOf(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetLastIndexOf(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue FlipBitByIndex(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue FlipBitsByRange(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo* argv);
    static JSTaggedValue SetAllBits(EcmaRuntimeCallInfo* argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetBitVectorPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(BITVECTOR_PROTOTYPE_FUNCTIONS);
    }

    static size_t GetNumPrototypeInlinedProperties()
    {
        // 4 : 4 more inline properties in BitVector.prototype
        //   (1) BitVector.prototype.constructor
        //   (2) BitVector.prototype [ @@toStringTag ]
        //   (3) BitVector.prototype [ @@iterator ]
        //   (4) get BitVector.prototype.size
        //   (5) get BitVector.prototype.length
        return GetBitVectorPrototypeFunctions().Size() + 5;
    }

    static Span<const std::pair<std::string_view, bool>> GetPrototypeProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(BITVECTOR_PROTOTYPE_PROPERTIES);
    }

    static Span<const std::pair<std::string_view, bool>> GetFunctionProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(BITVECTOR_FUNCTION_PROPERTIES);
    }
private:
    static void FreeBitsetVectorPointer([[maybe_unused]] void *env, void *pointer, void *data);

#define BUILTIN_BITVECTOR_FUNCTION_ENTRY(name, func, length, id) \
    base::BuiltinFunctionEntry::Create(name, ContainersBitVector::func, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array BITVECTOR_PROTOTYPE_FUNCTIONS = {
        BUILTIN_BITVECTOR_PROTOTYPE_FUNCTIONS(BUILTIN_BITVECTOR_FUNCTION_ENTRY)
    };
#undef BUILTIN_BITVECTOR_FUNCTION_ENTRY

#define BITVECTOR_PROPERTIES_PAIR(name, func, length, id) \
    std::pair<std::string_view, bool>(name, false),

    static constexpr std::array BITVECTOR_PROTOTYPE_PROPERTIES = {
        std::pair<std::string_view, bool>("constructor", false),
        BUILTIN_BITVECTOR_PROTOTYPE_FUNCTIONS(BITVECTOR_PROPERTIES_PAIR)
        std::pair<std::string_view, bool>("[Symbol.toStringTag]", false),
        std::pair<std::string_view, bool>("length", true),
        std::pair<std::string_view, bool>("size", true),
        std::pair<std::string_view, bool>("[Symbol.iterator]", false)
    };

    static constexpr std::array BITVECTOR_FUNCTION_PROPERTIES = {
        std::pair<std::string_view, bool>("length", false),
        std::pair<std::string_view, bool>("name", false),
        std::pair<std::string_view, bool>("prototype", false),
        std::pair<std::string_view, bool>("[Symbol.species]", true),
    };
#undef BITVECTOR_PROPERTIES_PAIR
};
} // namespace panda::ecmascript::containers
#endif // ECMASCRIPT_CONTAINERS_CONTAINERS_BIT_VECTOR_H
