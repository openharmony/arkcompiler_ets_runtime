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

#ifndef ECMASCRIPT_TS_TYPES_BUILTIN_TYPE_ID_H
#define ECMASCRIPT_TS_TYPES_BUILTIN_TYPE_ID_H

#include <cstdint>
#include <string_view>

#define TS_BUILTIN_TYPE_LIST(V)     \
    V(FUNCTION)                     \
    V(RANGE_ERROR)                  \
    V(ERROR)                        \
    V(OBJECT)                       \
    V(SYNTAX_ERROR)                 \
    V(TYPE_ERROR)                   \
    V(REFERENCE_ERROR)              \
    V(URI_ERROR)                    \
    V(SYMBOL)                       \
    V(EVAL_ERROR)                   \
    V(NUMBER)                       \
    V(PARSE_FLOAT)                  \
    V(DATE)                         \
    V(BOOLEAN)                      \
    V(BIG_INT)                      \
    V(PARSE_INT)                    \
    V(WEAK_MAP)                     \
    V(REG_EXP)                      \
    V(SET)                          \
    V(MAP)                          \
    V(WEAK_REF)                     \
    V(WEAK_SET)                     \
    V(FINALIZATION_REGISTRY)        \
    V(ARRAY)                        \
    V(UINT8_CLAMPED_ARRAY)          \
    V(UINT8_ARRAY)                  \
    V(TYPED_ARRAY)                  \
    V(INT8_ARRAY)                   \
    V(UINT16_ARRAY)                 \
    V(UINT32_ARRAY)                 \
    V(INT16_ARRAY)                  \
    V(INT32_ARRAY)                  \
    V(FLOAT32_ARRAY)                \
    V(FLOAT64_ARRAY)                \
    V(BIGINT64_ARRAY)               \
    V(BIGUINT64_ARRAY)              \
    V(SHARED_ARRAY_BUFFER)          \
    V(DATA_VIEW)                    \
    V(STRING)                       \
    V(ARRAY_BUFFER)                 \
    V(EVAL)                         \
    V(IS_FINITE)                    \
    V(ARK_PRIVATE)                  \
    V(PRINT)                        \
    V(DECODE_URI)                   \
    V(DECODE_URI_COMPONENT)         \
    V(IS_NAN)                       \
    V(ENCODE_URI)                   \
    V(JS_NAN)                       \
    V(GLOBAL_THIS)                  \
    V(ENCODE_URI_COMPONENT)         \
    V(JS_INFINITY)                  \
    V(MATH)                         \
    V(JSON)                         \
    V(ATOMICS)                      \
    V(UNDEFINED)                    \
    V(REFLECT)                      \
    V(PROMISE)                      \
    V(PROXY)                        \
    V(GENERATOR_FUNCTION)           \
    V(ITERATOR)                     \
    V(ARRAY_ITERATOR)               \
    V(INTL)

namespace panda::ecmascript {
#define TS_BUILTIN_TYPE_ENUM_ITEM(TYPE) TYPE,

/* Since AOT allows loading lib_ark_builtins.d.ts optionally, the localId of the GlobalTSTypeRef
 * (abbreviated as GT) of one builtin object will be different in different cases.
 *
 * In case where AOT does not load lib_ark_builtins.d.ts, builtin objects will be assigned localIds
 * according to the following enum order. For example, the GT of FUNCTION will be (1, 21), where 1
 * is the index of builtin TSTypeTable. Note that in this case, it is prohibited to get TSType from
 * builtin TSTypeTable.
 *
 * In case where AOT has loaded lib_ark_builtins.d.ts, builtin objects will be assigned localIds in
 * the order in which they appear in bytecodes. To identify types of builtin objects, the following
 * enum is required as a parameter to the TSManager::GetBuiltinOffset to help to get the GT of some
 * builtin object.
 */
enum class BuiltinTypeId : uint8_t {  // keep the same with enum BuiltinType in ets_frontend
    NUM_INDEX_IN_SUMMARY = 0,
    BUILTIN_OFFSET = 20,
    TS_BUILTIN_TYPE_LIST(TS_BUILTIN_TYPE_ENUM_ITEM)
    NUM_OF_BUILTIN_TYPES = INTL - BUILTIN_OFFSET,
    TYPED_ARRAY_FIRST = UINT8_CLAMPED_ARRAY,
    TYPED_ARRAY_LAST = BIGUINT64_ARRAY,
};
#undef TS_BUILTIN_TYPE_ENUM_ITEM

inline constexpr std::string_view ToString(BuiltinTypeId type)
{
#define TS_BUILTIN_TYPE_SWITCH_CASE(TYPE)   \
    case BuiltinTypeId::TYPE:               \
        return #TYPE;

    switch (type) {
        TS_BUILTIN_TYPE_LIST(TS_BUILTIN_TYPE_SWITCH_CASE)
        default:
            return "(Invalid BuiltinTypeId)";
    }
}

inline constexpr bool IsTypedArrayType(BuiltinTypeId type)
{
    size_t index = static_cast<size_t>(type);
    size_t first = static_cast<size_t>(BuiltinTypeId::TYPED_ARRAY_FIRST);
    size_t last  = static_cast<size_t>(BuiltinTypeId::TYPED_ARRAY_LAST);
    return first <= index && index <= last;
}
} // namespace panda::ecmascript
#endif // ECMASCRIPT_TS_TYPES_BUILTIN_TYPE_ID_H
