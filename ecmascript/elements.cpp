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

#include "ecmascript/elements.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
CMap<ElementsKind, ConstantIndex> Elements::InitializeHClassMap()
{
    CMap<ElementsKind, ConstantIndex> result;
    result.emplace(ElementsKind::NONE, ConstantIndex::ELEMENT_NONE_HCLASS_INDEX);
    result.emplace(ElementsKind::INT, ConstantIndex::ELEMENT_INT_HCLASS_INDEX);
    result.emplace(ElementsKind::DOUBLE, ConstantIndex::ELEMENT_DOUBLE_HCLASS_INDEX);
    result.emplace(ElementsKind::NUMBER, ConstantIndex::ELEMENT_NUMBER_HCLASS_INDEX);
    result.emplace(ElementsKind::STRING, ConstantIndex::ELEMENT_STRING_HCLASS_INDEX);
    result.emplace(ElementsKind::OBJECT, ConstantIndex::ELEMENT_OBJECT_HCLASS_INDEX);
    result.emplace(ElementsKind::TAGGED, ConstantIndex::ELEMENT_TAGGED_HCLASS_INDEX);
    result.emplace(ElementsKind::HOLE_INT, ConstantIndex::ELEMENT_HOLE_INT_HCLASS_INDEX);
    result.emplace(ElementsKind::HOLE_DOUBLE, ConstantIndex::ELEMENT_HOLE_DOUBLE_HCLASS_INDEX);
    result.emplace(ElementsKind::HOLE_NUMBER, ConstantIndex::ELEMENT_HOLE_NUMBER_HCLASS_INDEX);
    result.emplace(ElementsKind::HOLE_STRING, ConstantIndex::ELEMENT_HOLE_STRING_HCLASS_INDEX);
    result.emplace(ElementsKind::HOLE_OBJECT, ConstantIndex::ELEMENT_HOLE_OBJECT_HCLASS_INDEX);
    result.emplace(ElementsKind::HOLE_TAGGED, ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX);
    return result;
}

std::string Elements::GetString(ElementsKind kind)
{
    return std::to_string(static_cast<uint32_t>(kind));
}

bool Elements::IsInt(ElementsKind kind)
{
    return kind == ElementsKind::INT;
}

bool Elements::IsDouble(ElementsKind kind)
{
    return kind == ElementsKind::DOUBLE;
}

bool Elements::IsObject(ElementsKind kind)
{
    return kind == ElementsKind::OBJECT;
}

bool Elements::IsHole(ElementsKind kind)
{
    static constexpr uint8_t EVEN_NUMBER = 2;
    return static_cast<uint8_t>(kind) % EVEN_NUMBER == 1;
}

ElementsKind Elements::MergeElementsKind(ElementsKind curKind, ElementsKind newKind)
{
    auto result = ElementsKind(static_cast<uint8_t>(curKind) | static_cast<uint8_t>(newKind));
    ASSERT(result != ElementsKind::NONE);
    result = FixElementsKind(result);
    return result;
}

ElementsKind Elements::FixElementsKind(ElementsKind oldKind)
{
    auto result = oldKind;
    switch (result) {
        case ElementsKind::NONE:
        case ElementsKind::INT:
        case ElementsKind::DOUBLE:
        case ElementsKind::NUMBER:
        case ElementsKind::STRING:
        case ElementsKind::OBJECT:
        case ElementsKind::HOLE_INT:
        case ElementsKind::HOLE_DOUBLE:
        case ElementsKind::HOLE_NUMBER:
        case ElementsKind::HOLE_STRING:
        case ElementsKind::HOLE_OBJECT:
            break;
        default:
            if (IsHole(result)) {
                result = ElementsKind::HOLE_TAGGED;
            } else {
                result = ElementsKind::TAGGED;
            }
            break;
    }
    return result;
}

ElementsKind Elements::ToElementsKind(JSTaggedValue value, ElementsKind kind)
{
    ElementsKind valueKind = ElementsKind::NONE;
    if (value.IsInt()) {
        valueKind = ElementsKind::INT;
    } else if (value.IsDouble()) {
        valueKind = ElementsKind::DOUBLE;
    } else if (value.IsString()) {
        valueKind = ElementsKind::STRING;
    } else if (value.IsHeapObject()) {
        valueKind = ElementsKind::OBJECT;
    } else if (value.IsHole()) {
        valueKind = ElementsKind::HOLE;
    } else {
        valueKind = ElementsKind::TAGGED;
    }
    return MergeElementsKind(valueKind, kind);
}
}  // namespace panda::ecmascript
