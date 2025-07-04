/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JSPRIMITIVEREF_H
#define ECMASCRIPT_JSPRIMITIVEREF_H

#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
enum PrimitiveType : uint8_t {
    PRIMITIVE_TYPE_INVALID = 0,
    PRIMITIVE_BOOLEAN = 1,
    PRIMITIVE_NUMBER = 2,
    PRIMITIVE_STRING = 4,
    PRIMITIVE_SYMBOL = 8,
    PRIMITIVE_BIGINT = 16,
};

class JSPrimitiveRef : public JSObject {
public:
    CAST_CHECK(JSPrimitiveRef, IsJSPrimitiveRef);

    JSPrimitiveRef() = delete;

    bool IsNumber(const JSThread *thread) const
    {
        return GetValue(thread).IsNumber();
    }

    bool IsBigInt(const JSThread *thread) const
    {
        return GetValue(thread).IsBigInt();
    }

    bool IsInt(const JSThread *thread) const
    {
        return GetValue(thread).IsInt();
    }

    bool IsBoolean(const JSThread *thread) const
    {
        return GetValue(thread).IsBoolean();
    }

    bool IsString(const JSThread *thread) const
    {
        return GetValue(thread).IsString();
    }

    bool IsSymbol(const JSThread *thread) const
    {
        return GetValue(thread).IsSymbol();
    }

    uint32_t GetStringLength(const JSThread *thread) const
    {
        ASSERT(IsString(thread));
        return EcmaStringAccessor(GetValue(thread)).GetLength();
    }

    // ES6 9.4.3 String Exotic Objects
    // ES6 9.4.3.4 StringCreate( value, prototype)
    static JSHandle<JSPrimitiveRef> StringCreate(JSThread *thread, const JSHandle<JSTaggedValue> &value,
                                                 const JSHandle<JSTaggedValue> &newTarget);
    static bool StringGetIndexProperty(const JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                       PropertyDescriptor *desc);

    static constexpr size_t VALUE_OFFSET = JSObject::SIZE;
    ACCESSORS(Value, VALUE_OFFSET, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, VALUE_OFFSET, SIZE)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSPRIMITIVEREF_H
