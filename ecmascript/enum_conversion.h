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

#ifndef ECMASCRIPT_ENUM_CONVERSION_H
#define ECMASCRIPT_ENUM_CONVERSION_H

#include <optional>
#include "ecmascript/global_env_fields.h"
#include "ecmascript/ts_types/builtin_type_id.h"

namespace panda::ecmascript {
inline constexpr std::optional<GlobalEnvField> ToGlobelEnvPrototypeField(BuiltinTypeId type)
{
    switch (type) {
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            return GlobalEnvField::UINT8_CLAMPED_ARRAY_FUNCTION_PROTO_INDEX;
        case BuiltinTypeId::UINT8_ARRAY:
            return GlobalEnvField::UINT8_ARRAY_FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::TYPED_ARRAY:
            return GlobalEnvField::TYPED_ARRAY_PROTOTYPE_INDEX;
        case BuiltinTypeId::INT8_ARRAY:
            return GlobalEnvField::INT8_ARRAY_FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::UINT16_ARRAY:
            return GlobalEnvField::UINT16_ARRAY_FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::UINT32_ARRAY:
            return GlobalEnvField::UINT32_ARRAY_FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::INT16_ARRAY:
            return GlobalEnvField::INT16_ARRAY_FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::INT32_ARRAY:
            return GlobalEnvField::INT32_ARRAY_FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::FLOAT32_ARRAY:
            return GlobalEnvField::FLOAT32_ARRAY_FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::FLOAT64_ARRAY:
            return GlobalEnvField::FLOAT64_ARRAY_FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::BIGINT64_ARRAY:
            return GlobalEnvField::BIGINT64_ARRAY_FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::BIGUINT64_ARRAY:
            return GlobalEnvField::BIGUINT64_ARRAY_FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::ARRAY:
            return GlobalEnvField::ARRAY_PROTOTYPE_INDEX;
        case BuiltinTypeId::DATA_VIEW:
            return GlobalEnvField::DATA_VIEW_PROTOTYPE_INDEX;
        case BuiltinTypeId::DATE:
            return GlobalEnvField::DATE_PROTOTYPE_INDEX;
        case BuiltinTypeId::FUNCTION:
            return GlobalEnvField::FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::GENERATOR_FUNCTION:
            return GlobalEnvField::GENERATOR_FUNCTION_PROTOTYPE_OFFSET;
        case BuiltinTypeId::MAP:
            return GlobalEnvField::MAP_PROTOTYPE_INDEX;
        case BuiltinTypeId::OBJECT:
            return GlobalEnvField::OBJECT_FUNCTION_PROTOTYPE_INDEX;
        case BuiltinTypeId::SET:
            return GlobalEnvField::SET_PROTOTYPE_INDEX;
        case BuiltinTypeId::STRING:
            return GlobalEnvField::STRING_PROTOTYPE_INDEX;
        case BuiltinTypeId::ITERATOR:
            return GlobalEnvField::ITERATOR_PROTOTYPE_INDEX;
        case BuiltinTypeId::ARRAY_ITERATOR:
            return GlobalEnvField::ARRAY_ITERATOR_PROTOTYPE_INDEX;
        default: // No corresponding entry in either BuiltinTypeId or GlobalEnvField
            return std::nullopt;
    }
}

constexpr std::optional<BuiltinTypeId> ToBuiltinsTypeId(JSType type)
{
    if (type >= JSType::STRING_FIRST && type <= JSType::STRING_LAST) {
        return BuiltinTypeId::STRING;
    }
    switch (type) {
        case JSType::JS_MAP:
            return BuiltinTypeId::MAP;
        case JSType::JS_SET:
            return BuiltinTypeId::SET;
        case JSType::JS_OBJECT:
            return BuiltinTypeId::OBJECT;
        case JSType::JS_ARRAY:
            return BuiltinTypeId::ARRAY;
        case JSType::JS_DATA_VIEW:
            return BuiltinTypeId::DATA_VIEW;
        case JSType::JS_DATE:
            return BuiltinTypeId::DATE;
        case JSType::JS_INT8_ARRAY:
            return BuiltinTypeId::INT8_ARRAY;
        case JSType::JS_UINT8_ARRAY:
            return BuiltinTypeId::UINT8_ARRAY;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return BuiltinTypeId::UINT8_CLAMPED_ARRAY;
        case JSType::JS_INT16_ARRAY:
            return BuiltinTypeId::INT16_ARRAY;
        case JSType::JS_UINT16_ARRAY:
            return BuiltinTypeId::UINT16_ARRAY;
        case JSType::JS_INT32_ARRAY:
            return BuiltinTypeId::INT32_ARRAY;
        case JSType::JS_UINT32_ARRAY:
            return BuiltinTypeId::UINT32_ARRAY;
        case JSType::JS_FLOAT32_ARRAY:
            return BuiltinTypeId::FLOAT32_ARRAY;
        case JSType::JS_FLOAT64_ARRAY:
            return BuiltinTypeId::FLOAT64_ARRAY;
        case JSType::JS_ARRAY_ITERATOR:
            return BuiltinTypeId::ARRAY_ITERATOR;
        default:
            return std::nullopt;
    }
}
} // namespace panda::ecmascript
#endif // ECMASCRIPT_ENUM_CONVERSION_H
