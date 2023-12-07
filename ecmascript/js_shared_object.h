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

    static constexpr size_t OWNER_ID_OFFSET = JSObject::SIZE;
    ACCESSORS_PRIMITIVE_FIELD(OwnerID, uint64_t, OWNER_ID_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    DECL_CHECK_OWNERSHIP();
    DECL_SET_OWNERSHIP();
    DECL_DUMP()
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_JS_SHARED_OBJECT_H