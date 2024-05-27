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

#ifndef ECMASCRIPT_ELEMENT_ACCESSOR_INL_H
#define ECMASCRIPT_ELEMENT_ACCESSOR_INL_H

#include "ecmascript/element_accessor.h"

#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {

template<typename T>
inline void ElementAccessor::Set(const JSThread *thread, JSHandle<JSObject> receiver, uint32_t idx,
                                 const JSHandle<T> &value, bool needTransition, ElementsKind extraKind)
{
    // Change elementsKind
    ElementsKind oldKind = receiver->GetClass()->GetElementsKind();
    if (needTransition && JSHClass::TransitToElementsKind(thread, receiver,
                                                          JSHandle<JSTaggedValue>(value), extraKind)) {
        ElementsKind newKind = receiver->GetClass()->GetElementsKind();
        // GC might happen when migrating Array
        Elements::MigrateArrayWithKind(thread, receiver, oldKind, newKind);
    }
    
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements());
    ASSERT(idx < elements->GetLength());
    size_t offset = JSTaggedValue::TaggedTypeSize() * idx;

    ElementsKind kind = receiver->GetClass()->GetElementsKind();
    if (!elements->GetClass()->IsMutantTaggedArray()) {
        kind = ElementsKind::GENERIC;
    }

    JSTaggedType convertedValue = ConvertTaggedValueWithElementsKind(value.GetTaggedValue(), kind);
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (value.GetTaggedValue().IsHeapObject()) {
        Barriers::SetObject<true>(thread, elements->GetData(), offset, convertedValue);
    } else {  // NOLINTNEXTLINE(readability-misleading-indentation)
        Barriers::SetPrimitive<JSTaggedType>(elements->GetData(), offset, convertedValue);
    }
}

template<typename T>
void ElementAccessor::FastSet(const JSThread *thread, JSHandle<TaggedArray> elements, uint32_t idx,
                              const JSHandle<T> &value, ElementsKind kind)
{
    ASSERT(idx < elements->GetLength());
    size_t offset = JSTaggedValue::TaggedTypeSize() * idx;
    JSTaggedValue rawValue = value.GetTaggedValue();
    switch (kind) {
        case ElementsKind::INT:
            Barriers::SetPrimitive<JSTaggedType>(elements->GetData(), offset,
                                                 static_cast<JSTaggedType>(rawValue.GetInt()));
            break;
        case ElementsKind::NUMBER:
            if (rawValue.IsInt()) {
                int intValue = rawValue.GetInt();
                Barriers::SetPrimitive<JSTaggedType>(elements->GetData(), offset,
                                                     base::bit_cast<JSTaggedType>(static_cast<double>(intValue)));
            } else {
                Barriers::SetPrimitive<JSTaggedType>(elements->GetData(), offset,
                                                     base::bit_cast<JSTaggedType>(rawValue.GetDouble()));
            }
            break;
        case ElementsKind::TAGGED:
            if (value.GetTaggedValue().IsHeapObject()) {
                Barriers::SetObject<true>(thread, elements->GetData(), offset, rawValue.GetRawData());
            } else {  // NOLINTNEXTLINE(readability-misleading-indentation)
                Barriers::SetPrimitive<JSTaggedType>(elements->GetData(), offset, rawValue.GetRawData());
            }
            break;
        default:
            LOG_ECMA(FATAL) << "Trying to Convert TaggedValue With Unknown ElementsKind";
            UNREACHABLE();
            break;
    }
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_ELEMENT_ACCESSOR_INL_H
