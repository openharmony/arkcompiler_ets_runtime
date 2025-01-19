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
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
std::string Elements::GetString(ElementsKind kind)
{
    return std::to_string(ToUint(kind));
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
    return ToUint(kind) % EVEN_NUMBER == 1;
}

ConstantIndex Elements::GetGlobalContantIndexByKind(ElementsKind kind)
{
    switch (kind) {
        case ElementsKind::NONE:
            return ConstantIndex::ELEMENT_NONE_HCLASS_INDEX;
        case ElementsKind::INT:
            return ConstantIndex::ELEMENT_INT_HCLASS_INDEX;
        case ElementsKind::NUMBER:
            return ConstantIndex::ELEMENT_NUMBER_HCLASS_INDEX;
        case ElementsKind::STRING:
            return ConstantIndex::ELEMENT_STRING_HCLASS_INDEX;
        case ElementsKind::OBJECT:
            return ConstantIndex::ELEMENT_OBJECT_HCLASS_INDEX;
        case ElementsKind::TAGGED:
            return ConstantIndex::ELEMENT_TAGGED_HCLASS_INDEX;
        case ElementsKind::HOLE:
            return ConstantIndex::ELEMENT_HOLE_HCLASS_INDEX;
        case ElementsKind::HOLE_INT:
            return ConstantIndex::ELEMENT_HOLE_INT_HCLASS_INDEX;
        case ElementsKind::HOLE_NUMBER:
            return ConstantIndex::ELEMENT_HOLE_NUMBER_HCLASS_INDEX;
        case ElementsKind::HOLE_STRING:
            return ConstantIndex::ELEMENT_HOLE_STRING_HCLASS_INDEX;
        case ElementsKind::HOLE_OBJECT:
            return ConstantIndex::ELEMENT_HOLE_OBJECT_HCLASS_INDEX;
        case ElementsKind::HOLE_TAGGED:
            return ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX;
        default:
            LOG_ECMA(FATAL) << "Unknown elementsKind when getting constantIndx: " << ToUint(kind);
    }
}

ElementsKind Elements::MergeElementsKind(ElementsKind curKind, ElementsKind newKind)
{
    auto result = ElementsKind(static_cast<uint8_t>(curKind) | static_cast<uint8_t>(newKind));
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
    return MergeElementsKind(ToElementsKind(value), kind);
}

void Elements::HandleIntKindMigration(const JSThread *thread, const JSHandle<JSObject> &object,
                                      const ElementsKind newKind, bool needCOW)
{
    if (IsStringOrNoneOrHole(newKind)) {
        JSTaggedValue newElements = MigrateFromRawValueToHeapValue(thread, object, needCOW, true);
        object->SetElements(thread, newElements);
    } else if (newKind == ElementsKind::NUMBER || newKind == ElementsKind::HOLE_NUMBER) {
        MigrateFromHoleIntToHoleNumber(thread, object);
    }
}

bool Elements::IsNumberKind(const ElementsKind kind)
{
    return ToUint(kind) >= Elements::ToUint(ElementsKind::NUMBER) &&
           ToUint(kind) <= Elements::ToUint(ElementsKind::HOLE_NUMBER);
}

bool Elements::IsStringOrNoneOrHole(const ElementsKind kind)
{
    return ToUint(kind) >= ToUint(ElementsKind::STRING) ||
           kind == ElementsKind::NONE || kind == ElementsKind::HOLE;
}

void Elements::HandleNumberKindMigration(const JSThread *thread, const JSHandle<JSObject> &object,
                                         const ElementsKind newKind, bool needCOW)
{
    if (IsStringOrNoneOrHole(newKind)) {
        JSTaggedValue newElements = MigrateFromRawValueToHeapValue(thread, object, needCOW, false);
        object->SetElements(thread, newElements);
    } else if (newKind == ElementsKind::INT || newKind == ElementsKind::HOLE_INT) {
        MigrateFromHoleNumberToHoleInt(thread, object);
    }
}

void Elements::HandleOtherKindMigration(const JSThread *thread, const JSHandle<JSObject> &object,
                                        const ElementsKind newKind, bool needCOW)
{
    if (newKind == ElementsKind::INT || newKind == ElementsKind::HOLE_INT) {
        JSTaggedValue newElements = MigrateFromHeapValueToRawValue(thread, object, needCOW, true);
        object->SetElements(thread, newElements);
    } else if (IsNumberKind(newKind)) {
        JSTaggedValue newElements = MigrateFromHeapValueToRawValue(thread, object, needCOW, false);
        object->SetElements(thread, newElements);
    }
}

void Elements::MigrateArrayWithKind(const JSThread *thread, const JSHandle<JSObject> &object,
                                    const ElementsKind oldKind, const ElementsKind newKind)
{
    if (!thread->IsEnableMutantArray()) {
        return;
    }

    if (oldKind == newKind ||
        (oldKind == ElementsKind::INT && newKind == ElementsKind::HOLE_INT) ||
        (oldKind == ElementsKind::NUMBER && newKind == ElementsKind::HOLE_NUMBER)) {
        return;
    }

    bool needCOW = object->GetElements().IsCOWArray();

    if (oldKind == ElementsKind::INT || oldKind == ElementsKind::HOLE_INT) {
        HandleIntKindMigration(thread, object, newKind, needCOW);
    } else if ((IsNumberKind(oldKind))) {
        HandleNumberKindMigration(thread, object, newKind, needCOW);
    } else {
        HandleOtherKindMigration(thread, object, newKind, needCOW);
    }
}

JSTaggedValue Elements::MigrateFromRawValueToHeapValue(const JSThread *thread, const JSHandle<JSObject> object,
                                                       bool needCOW, bool isIntKind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<MutantTaggedArray> elements = JSHandle<MutantTaggedArray>(thread, object->GetElements());
    uint32_t length = elements->GetLength();
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
        } else if (isIntKind) {
            int convertedValue = static_cast<int>(value);
            newElements->Set(thread, i, JSTaggedValue(convertedValue));
        } else {
            double convertedValue = base::bit_cast<double>(value);
            newElements->Set(thread, i, JSTaggedValue(convertedValue));
        }
    }
    return newElements.GetTaggedValue();
}

JSTaggedValue Elements::MigrateFromHeapValueToRawValue(const JSThread *thread, const JSHandle<JSObject> object,
                                                       bool needCOW, bool isIntKind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> elements = JSHandle<TaggedArray>(thread, object->GetElements());
    uint32_t length = elements->GetLength();
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
        } else if (isIntKind) {
            convertedValue = static_cast<JSTaggedType>(value.GetInt());
        } else if (value.IsInt()) {
            int intValue = value.GetInt();
            convertedValue = base::bit_cast<JSTaggedType>(static_cast<double>(intValue));
        } else {
            convertedValue = base::bit_cast<JSTaggedType>(value.GetDouble());
        }
        newElements->Set<false>(thread, i, JSTaggedValue(convertedValue));
    }
    return newElements.GetTaggedValue();
}

void Elements::MigrateFromHoleIntToHoleNumber(const JSThread *thread, const JSHandle<JSObject> object)
{
    JSHandle<MutantTaggedArray> elements = JSHandle<MutantTaggedArray>(thread, object->GetElements());
    uint32_t length = elements->GetLength();
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

void Elements::MigrateFromHoleNumberToHoleInt(const JSThread *thread, const JSHandle<JSObject> object)
{
    JSHandle<MutantTaggedArray> elements = JSHandle<MutantTaggedArray>(thread, object->GetElements());
    uint32_t length = elements->GetLength();
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedType value = elements->Get(i).GetRawData();
        if (value == base::SPECIAL_HOLE) {
            continue;
        }
        double intValue = base::bit_cast<double>(elements->Get(i).GetRawData());
        int64_t convertedValue = static_cast<int64_t>(intValue);
        elements->Set<false>(thread, i, JSTaggedValue(base::bit_cast<JSTaggedType>(convertedValue)));
    }
}
}  // namespace panda::ecmascript
