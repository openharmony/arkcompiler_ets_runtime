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

#ifndef ECMASCRIPT_JS_SHARED_OBJECT_H
#define ECMASCRIPT_JS_SHARED_OBJECT_H

#include "ecmascript/js_object.h"

namespace panda::ecmascript {

class JSSharedObject : public JSObject {
public:
    CAST_CHECK(JSSharedObject, IsJSSharedObject);
    static constexpr size_t SIZE = JSObject::SIZE;
    static constexpr uint32_t MAX_INLINE = PropertyAttributes::MAX_FAST_PROPS_CAPACITY -
        SIZE / JSTaggedValue::TaggedTypeSize() + 1;
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, SIZE, SIZE)
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_JS_SHARED_OBJECT_H