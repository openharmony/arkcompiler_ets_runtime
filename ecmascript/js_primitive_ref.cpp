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

#include "ecmascript/js_primitive_ref.h"

#include "ecmascript/global_env.h"

namespace panda::ecmascript {
// ES6 9.4.3.4 StringCreate( value, prototype)
JSHandle<JSPrimitiveRef> JSPrimitiveRef::StringCreate(JSThread *thread, const JSHandle<JSTaggedValue> &value,
                                                      const JSHandle<JSTaggedValue> &newTarget)
{
    ASSERT(value->IsString());
    // 1. Let S be MakeBasicObject(<<[[Prototype]], [[Extensible]], [[StringData]]>>).
    // 2. Set S.[[Prototype]] to prototype.
    // 3. Set S.[[StringData]] to value.
    // 4. Set S.[[GetOwnProperty]] as specified.
    // 5. Set S.[[DefineOwnProperty]] as specified.
    // 6. Set S.[[OwnPropertyKeys]] as specified.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> str(factory->NewJSString(value, newTarget));
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSPrimitiveRef, thread);
    // 7. Let length be the number of code unit elements in value.
    JSHandle<JSTaggedValue> lengthStr = thread->GlobalConstants()->GetHandledLengthString();
    uint32_t length = EcmaStringAccessor(value->GetTaggedObject()).GetLength();
    // 8. Perform ! DefinePropertyOrThrow(S, "length", PropertyDescriptor { [[Value]]: F(length),
    //    [[Writable]]: false, [[Enumerable]]: false, [[Configurable]]: false }).
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(length)), false, false, false);
    [[maybe_unused]] bool status = JSTaggedValue::DefinePropertyOrThrow(thread, str, lengthStr, desc);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSPrimitiveRef, thread);
    ASSERT(status);
    // 9. Return S.
    return JSHandle<JSPrimitiveRef>(str);
}

bool JSPrimitiveRef::StringGetIndexProperty(const JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                            PropertyDescriptor *desc)
{
    uint16_t tmpChar = 0;
    {
        JSHandle<EcmaString> strHandle(thread, EcmaString::Cast(JSPrimitiveRef::Cast(*obj)->GetValue()));
        JSHandle<EcmaString> strFlat(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), strHandle));
        if (EcmaStringAccessor(strFlat).GetLength() <= index) {
            return false;
        }
        // 10. Let resultStr be a String value of length 1, containing one code unit from str, specifically the code
        // unit at index index
        tmpChar = EcmaStringAccessor(strFlat).Get(index);
    }
    JSHandle<JSTaggedValue> value(thread->GetEcmaVM()->GetFactory()->NewFromUtf16(&tmpChar, 1));
    // 11. Return a PropertyDescriptor{ [[Value]]: resultStr, [[Enumerable]]: true, [[Writable]]: false,
    // [[Configurable]]: false }.
    desc->SetValue(value);
    desc->SetEnumerable(true);
    desc->SetWritable(false);
    desc->SetConfigurable(false);
    return true;
}
// ES6 9.4.3.3 [[OwnPropertyKeys]] ( )
}  // namespace panda::ecmascript
