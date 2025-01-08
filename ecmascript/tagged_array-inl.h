/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TAGGED_ARRAY_INL_H
#define ECMASCRIPT_TAGGED_ARRAY_INL_H

#include "ecmascript/tagged_array.h"
#include "ecmascript/mem/barriers-inl.h"

namespace panda::ecmascript {
template<bool needBarrier, typename T>
inline void TaggedArray::Set(const JSThread *thread, uint32_t idx, const T &value)
{
    ASSERT(idx < GetLength());
    size_t offset = JSTaggedValue::TaggedTypeSize() * idx;

    if constexpr (std::is_same_v<T, JSTaggedValue>) {
        if (needBarrier && value.IsHeapObject()) {
            Barriers::SetObject<true>(thread, GetData(), offset, value.GetRawData());
        } else {
            Barriers::SetPrimitive<JSTaggedType>(GetData(), offset, value.GetRawData());
        }
    } else if constexpr (IsJSHandle<T>::value) {
        auto taggedValue = value.GetTaggedValue();
        if (taggedValue.IsHeapObject()) {
            Barriers::SetObject<true>(thread, GetData(), offset, taggedValue.GetRawData());
        } else {
            Barriers::SetPrimitive<JSTaggedType>(GetData(), offset, taggedValue.GetRawData());
        }
    } else {
        static_assert(!std::is_same_v<T, T>, "T must be either JSTaggedValue or JSHandle<>");
    }
}

#ifndef ECMASCRIPT_TAGGED_ARRAY_CPP
// `Get` is inlined when possible, including the case when it's used outside libark_jsruntime.
// For other cases `Get` is defined with external linkage in tagged_array.cpp
    #define MAYBE_INLINE inline
#else
    #define MAYBE_INLINE
#endif // ECMASCRIPT_TAGGED_ARRAY_CPP

MAYBE_INLINE JSTaggedValue TaggedArray::Get(uint32_t idx) const
{
    ASSERT(idx < GetLength());
    // Note: Here we can't statically decide the element type is a primitive or heap object, especially for
    //       dynamically-typed languages like JavaScript. So we simply skip the read-barrier.
    size_t offset = JSTaggedValue::TaggedTypeSize() * idx;
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    return JSTaggedValue(Barriers::GetValue<JSTaggedType>(GetData(), offset));
}

#undef MAYBE_INLINE

template <bool needBarrier, bool maybeOverlap>
inline void TaggedArray::Copy(const JSThread* thread, uint32_t dstStart, uint32_t srcStart,
                              const TaggedArray* srcArray, uint32_t count)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT((dstStart + count <= GetLength()) && "TaggedArray::Copy dst count is out of range");
    ASSERT((srcStart + count <= srcArray->GetLength()) && "TaggedArray::Copy src count is out of range");
    size_t taggedTypeSize = JSTaggedValue::TaggedTypeSize();
    JSTaggedValue* to = reinterpret_cast<JSTaggedValue*>(ToUintPtr(GetData()) + taggedTypeSize * dstStart);
    JSTaggedValue* from = reinterpret_cast<JSTaggedValue*>(ToUintPtr(srcArray->GetData()) + taggedTypeSize * srcStart);
    Barriers::CopyObject<needBarrier, maybeOverlap>(thread, this, to, from, count);
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_ARRAY_INL_H
