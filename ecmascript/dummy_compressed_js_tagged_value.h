/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DUMMY_COMPRESSED_JS_TAGGED_VALUE_H
#define ECMASCRIPT_DUMMY_COMPRESSED_JS_TAGGED_VALUE_H

#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
static_assert(std::is_same_v<JSTaggedType, CompressedJSTaggedType>);
using TemporaryJSTaggedValue = JSTaggedValue;
class CompressedJSTaggedValue : public JSTaggedValueInternals {
public:
    static_assert(std::is_unsigned<JSTaggedType>::value);

    static ARK_INLINE constexpr size_t CompressedTaggedTypeSize()
    {
        return JSTaggedValue::TaggedTypeSize();
    }

    static ARK_INLINE constexpr size_t CompressFactorToJSTaggedValue()
    {
        static_assert(JSTaggedValue::TaggedTypeSize() == CompressedTaggedTypeSize());
        return JSTaggedValue::TaggedTypeSize() / CompressedTaggedTypeSize();
    }

    static ARK_INLINE constexpr CompressedJSTaggedValue Hole()
    {
        return CompressedJSTaggedValue(JSTaggedValue::Hole().GetRawData());
    }

    static ARK_INLINE size_t AlignUpSizeToJSTaggedValue(size_t size)
    {
        ASSERT(size % CompressedTaggedTypeSize() == 0);
        return AlignUp(size, JSTaggedValue::TaggedTypeSize());
    }

    ARK_INLINE explicit constexpr CompressedJSTaggedValue(CompressedJSTaggedType v) : value_(v)
    {
    }

    ARK_INLINE CompressedJSTaggedType GetCompressedRawData() const
    {
        return value_;
    }

    static ARK_INLINE CompressedJSTaggedValue Compress(JSTaggedValue value)
    {
        return CompressedJSTaggedValue(value.GetRawData());
    }

    ARK_INLINE TemporaryJSTaggedValue Decompress([[maybe_unused]] JSTaggedValue helper)
    {
        return TemporaryJSTaggedValue(value_);
    }

    ARK_INLINE TemporaryJSTaggedValue Decompress([[maybe_unused]] uintptr_t helper)
    {
        return TemporaryJSTaggedValue(value_);
    }

private:
    CompressedJSTaggedType value_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DUMMY_COMPRESSED_JS_TAGGED_VALUE_H
