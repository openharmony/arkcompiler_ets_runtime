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

#include "ecmascript/element_accessor.h"

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
JSTaggedValue ElementAccessor::Get(JSHandle<JSObject> receiver, uint32_t idx)
{
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements());
    ASSERT(idx < elements->GetLength());
    ElementsKind kind = receiver->GetClass()->GetElementsKind();
    if (!elements->GetClass()->IsMutantTaggedArray()) {
        kind = ElementsKind::GENERIC;
    }
    // Note: Here we can't statically decide the element type is a primitive or heap object, especially for
    //       dynamically-typed languages like JavaScript. So we simply skip the read-barrier.
    size_t offset = JSTaggedValue::TaggedTypeSize() * idx;
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    JSTaggedType rawValue = Barriers::GetValue<JSTaggedType>(elements->GetData(), offset);
    return GetTaggedValueWithElementsKind(rawValue, kind);
}

JSTaggedValue ElementAccessor::Get(JSObject *receiver, uint32_t idx)
{
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements());
    ASSERT(idx < elements->GetLength());
    ElementsKind kind = receiver->GetClass()->GetElementsKind();
    if (!elements->GetClass()->IsMutantTaggedArray()) {
        kind = ElementsKind::GENERIC;
    }
    // Note: Here we can't statically decide the element type is a primitive or heap object, especially for
    //       dynamically-typed languages like JavaScript. So we simply skip the read-barrier.
    size_t offset = JSTaggedValue::TaggedTypeSize() * idx;
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    JSTaggedType rawValue = Barriers::GetValue<JSTaggedType>(elements->GetData(), offset);
    return GetTaggedValueWithElementsKind(rawValue, kind);
}

JSTaggedValue ElementAccessor::FastGet(JSHandle<TaggedArray> elements, uint32_t idx, ElementsKind kind)
{
    ASSERT(idx < elements->GetLength());
    size_t offset = JSTaggedValue::TaggedTypeSize() * idx;
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    JSTaggedType rawValue = Barriers::GetValue<JSTaggedType>(elements->GetData(), offset);
    return GetTaggedValueWithElementsKind(rawValue, kind);
}

bool ElementAccessor::IsDictionaryMode(JSHandle<JSObject> receiver)
{
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements());
    return elements->GetClass()->IsDictionary();
}

bool ElementAccessor::IsDictionaryMode(JSObject *receiver)
{
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements());
    return elements->GetClass()->IsDictionary();
}

uint32_t ElementAccessor::GetElementsLength(JSHandle<JSObject> receiver)
{
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements());
    return elements->GetLength();
}

uint32_t ElementAccessor::GetElementsLength(JSObject *receiver)
{
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements());
    return elements->GetLength();
}

JSTaggedValue ElementAccessor::GetTaggedValueWithElementsKind(JSTaggedType rawValue, ElementsKind kind)
{
    JSTaggedValue convertedValue = JSTaggedValue::Hole();
    if (rawValue == base::SPECIAL_HOLE) {
        return convertedValue;
    }
    switch (kind) {
        case ElementsKind::INT:
        case ElementsKind::HOLE_INT:
            convertedValue = JSTaggedValue(static_cast<int>(rawValue));
            break;
        case ElementsKind::NUMBER:
        case ElementsKind::HOLE_NUMBER:
            convertedValue = JSTaggedValue(base::bit_cast<double>(rawValue));
            break;
        case ElementsKind::HOLE:
        case ElementsKind::NONE:
        case ElementsKind::TAGGED:
        case ElementsKind::STRING:
        case ElementsKind::HOLE_TAGGED:
        case ElementsKind::HOLE_STRING:
            convertedValue = JSTaggedValue(rawValue);
            break;
        default:
            LOG_ECMA(FATAL) << "Trying to Get TaggedValue With Unknown ElementsKind";
            UNREACHABLE();
            break;
    }
    return convertedValue;
}

JSTaggedType ElementAccessor::ConvertTaggedValueWithElementsKind(JSTaggedValue rawValue, ElementsKind kind)
{
    JSTaggedType convertedValue = base::SPECIAL_HOLE;
    if (rawValue.IsHole() && Elements::IsInNumbers(kind)) {
        return convertedValue;
    }
    switch (kind) {
        case ElementsKind::INT:
        case ElementsKind::HOLE_INT:
            convertedValue = static_cast<JSTaggedType>(rawValue.GetInt());
            break;
        case ElementsKind::NUMBER:
        case ElementsKind::HOLE_NUMBER:
            if (rawValue.IsInt()) {
                int intValue = rawValue.GetInt();
                convertedValue = base::bit_cast<JSTaggedType>(static_cast<double>(intValue));
            } else {
                convertedValue = base::bit_cast<JSTaggedType>(rawValue.GetDouble());
            }
            break;
        case ElementsKind::HOLE:
        case ElementsKind::NONE:
        case ElementsKind::TAGGED:
        case ElementsKind::STRING:
        case ElementsKind::HOLE_TAGGED:
        case ElementsKind::HOLE_STRING:
            convertedValue = rawValue.GetRawData();
            break;
        default:
            LOG_ECMA(FATAL) << "Trying to Convert TaggedValue With Unknown ElementsKind";
            UNREACHABLE();
            break;
    }
    return convertedValue;
}

void ElementAccessor::CopyJSArrayObject(const JSThread *thread, JSHandle<JSObject>srcObj, JSHandle<JSObject>dstObj,
                                        uint32_t effectiveLength)
{
    ASSERT(effectiveLength <= GetElementsLength(srcObj));
    ASSERT(effectiveLength <= GetElementsLength(dstObj));
    for (uint32_t i = 0; i < effectiveLength; i++) {
        JSHandle<JSTaggedValue> value(thread, Get(srcObj, i));
        Set(thread, dstObj, i, value, true);
    }
}

void ElementAccessor::CopyJSArrayToTaggedArray(const JSThread *thread, JSHandle<JSObject>srcObj,
                                               JSHandle<TaggedArray>dstElements, uint32_t effectiveLength)
{
    ASSERT(effectiveLength <= GetElementsLength(srcObj));
    ASSERT(effectiveLength <= dstElements->GetLength());
    for (uint32_t i = 0; i < effectiveLength; i++) {
        JSHandle<JSTaggedValue> value(thread, Get(srcObj, i));
        dstElements->Set(thread, i, value);
    }
}
}  // namespace panda::ecmascript
