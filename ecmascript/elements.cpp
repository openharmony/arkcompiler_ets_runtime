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
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
CMap<ElementsKind, ConstantIndex> Elements::InitializeHClassMap()
{
    CMap<ElementsKind, ConstantIndex> result;
    result.emplace(ElementsKind::NONE, ConstantIndex::ELEMENT_NONE_HCLASS_INDEX);
    result.emplace(ElementsKind::HOLE, ConstantIndex::ELEMENT_HOLE_HCLASS_INDEX);
    result.emplace(ElementsKind::INT, ConstantIndex::ELEMENT_INT_HCLASS_INDEX);
    result.emplace(ElementsKind::NUMBER, ConstantIndex::ELEMENT_NUMBER_HCLASS_INDEX);
    result.emplace(ElementsKind::STRING, ConstantIndex::ELEMENT_STRING_HCLASS_INDEX);
    result.emplace(ElementsKind::OBJECT, ConstantIndex::ELEMENT_OBJECT_HCLASS_INDEX);
    result.emplace(ElementsKind::TAGGED, ConstantIndex::ELEMENT_TAGGED_HCLASS_INDEX);
    result.emplace(ElementsKind::HOLE_INT, ConstantIndex::ELEMENT_HOLE_INT_HCLASS_INDEX);
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

bool Elements::IsNumber(ElementsKind kind)
{
    return kind == ElementsKind::NUMBER;
}

bool Elements::IsTagged(ElementsKind kind)
{
    return kind == ElementsKind::TAGGED;
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
        case ElementsKind::NUMBER:
        case ElementsKind::STRING:
        case ElementsKind::OBJECT:
        case ElementsKind::HOLE:
        case ElementsKind::HOLE_INT:
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
        valueKind = ElementsKind::NUMBER;
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

void Elements::MigrateArrayWithKind(const JSThread *thread, const JSHandle<JSObject> &object,
                                    const ElementsKind oldKind, const ElementsKind newKind)
{
    if (!thread->GetEcmaVM()->IsEnableElementsKind()) {
        return;
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (oldKind == newKind) {
        return;
    }
    // When create ArrayLiteral from constantPool, we need to preserve the COW property if necessary.
    // When transition happens to Array, e.g. arr.x = 1, we need to preserve the COW property if necessary.
    bool needCOW = object->GetElements().IsCOWArray();
    if ((oldKind == ElementsKind::INT && newKind == ElementsKind::HOLE_INT) ||
        (oldKind == ElementsKind::NUMBER && newKind == ElementsKind::HOLE_NUMBER)) {
        return;
    } else if (oldKind == ElementsKind::INT || oldKind == ElementsKind::HOLE_INT) {
        JSHandle<MutantTaggedArray> elements = JSHandle<MutantTaggedArray>(thread, object->GetElements());
        uint32_t length = elements->GetLength();
        if (static_cast<uint32_t>(newKind) >= static_cast<uint32_t>(ElementsKind::STRING)) {
            JSMutableHandle<TaggedArray> newElements(thread, JSTaggedValue::Undefined());
            if (needCOW) {
                newElements.Update(factory->NewCOWTaggedArray(length));
            } else {
                newElements.Update(factory->NewTaggedArray(length));
            }
            for (uint32_t i = 0; i < length; i++) {
                JSTaggedType value = elements->Get(i).GetRawData();
                if (value == base::SPECIAL_HOLE) {
                    newElements->Set(thread, i, JSTaggedValue::Hole());
                } else {
                    int convertedValue = static_cast<int>(value);
                    newElements->Set(thread, i, JSTaggedValue(convertedValue));
                }
            }
            object->SetElements(thread, newElements);
        } else {
            for (uint32_t i = 0; i < length; i++) {
                JSTaggedType value = elements->Get(i).GetRawData();
                if (value == base::SPECIAL_HOLE) {
                    continue;
                }
                int intValue = static_cast<int>(elements->Get(i).GetRawData());
                double convertedValue = static_cast<double>(intValue);
                elements->Set<false>(thread, i, JSTaggedValue(base::bit_cast<JSTaggedType>(convertedValue)));
            }
        }
    } else if (static_cast<uint32_t>(oldKind) >= static_cast<uint32_t>(ElementsKind::NUMBER) &&
               static_cast<uint32_t>(oldKind) <= static_cast<uint32_t>(ElementsKind::HOLE_NUMBER)) {
        JSHandle<MutantTaggedArray> elements = JSHandle<MutantTaggedArray>(thread, object->GetElements());
        uint32_t length = elements->GetLength();
        if (static_cast<uint32_t>(newKind) >= static_cast<uint32_t>(ElementsKind::STRING)) {
            JSMutableHandle<TaggedArray> newElements(thread, JSTaggedValue::Undefined());
            if (needCOW) {
                newElements.Update(factory->NewCOWTaggedArray(length));
            } else {
                newElements.Update(factory->NewTaggedArray(length));
            }
            for (uint32_t i = 0; i < length; i++) {
                JSTaggedType value = elements->Get(i).GetRawData();
                if (value == base::SPECIAL_HOLE) {
                    newElements->Set(thread, i, JSTaggedValue::Hole());
                } else {
                    double convertedValue = base::bit_cast<double>(value);
                    newElements->Set(thread, i, JSTaggedValue(convertedValue));
                }
            }
            object->SetElements(thread, newElements);
        } else {
            LOG_ECMA(FATAL) << "This Branch is Unreachable in ConvertArray" << static_cast<uint32_t>(newKind);
        }
    } else {
        JSHandle<TaggedArray> elements = JSHandle<TaggedArray>(thread, object->GetElements());
        uint32_t length = elements->GetLength();
        if (newKind == ElementsKind::INT || newKind == ElementsKind::HOLE_INT) {
            JSMutableHandle<MutantTaggedArray> newElements(thread, JSTaggedValue::Undefined());
            if (needCOW) {
                newElements.Update(factory->NewCOWMutantTaggedArray(length));
            } else {
                newElements.Update(factory->NewMutantTaggedArray(length));
            }
            for (uint32_t i = 0; i < length; i++) {
                JSTaggedValue value = elements->Get(i);
                JSTaggedType convertedValue = 0;
                // To distinguish Hole (0x5) in taggedvalue with Interger 5
                if (value.IsHole()) {
                    convertedValue = base::SPECIAL_HOLE;
                } else {
                    convertedValue = static_cast<JSTaggedType>(value.GetInt());
                }
                newElements->Set<false>(thread, i, JSTaggedValue(convertedValue));
            }
            object->SetElements(thread, newElements);
        } else if (static_cast<uint32_t>(newKind) >= static_cast<uint32_t>(ElementsKind::NUMBER) &&
                   static_cast<uint32_t>(newKind) <= static_cast<uint32_t>(ElementsKind::HOLE_NUMBER)) {
            JSMutableHandle<MutantTaggedArray> newElements(thread, JSTaggedValue::Undefined());
            if (needCOW) {
                newElements.Update(factory->NewCOWMutantTaggedArray(length));
            } else {
                newElements.Update(factory->NewMutantTaggedArray(length));
            }
            for (uint32_t i = 0; i < length; i++) {
                JSTaggedValue value = elements->Get(i);
                JSTaggedType convertedValue = 0;
                // To distinguish Hole (0x5) in taggedvalue with Interger 5
                if (value.IsHole()) {
                    convertedValue = base::SPECIAL_HOLE;
                } else {
                    if (value.IsInt()) {
                        int intValue = value.GetInt();
                        convertedValue = base::bit_cast<JSTaggedType>(static_cast<double>(intValue));
                    } else {
                        convertedValue = base::bit_cast<JSTaggedType>(value.GetDouble());
                    }
                }
                newElements->Set<false>(thread, i, JSTaggedValue(convertedValue));
            }
            object->SetElements(thread, newElements);
        }
    }
}
}  // namespace panda::ecmascript
