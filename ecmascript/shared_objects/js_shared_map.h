/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_SHARED_MAP_H
#define ECMASCRIPT_JS_SHARED_MAP_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSSharedMap : public JSObject {
public:
    CAST_CHECK(JSSharedMap, IsJSSharedMap);

    static bool Delete(JSThread *thread, const JSHandle<JSSharedMap> &map, const JSHandle<JSTaggedValue> &key);

    static void PUBLIC_API Set(JSThread *thread, const JSHandle<JSSharedMap> &map, const JSHandle<JSTaggedValue> &key,
                               const JSHandle<JSTaggedValue> &value);
    static void Clear(JSThread *thread, const JSHandle<JSSharedMap> &map);

    static bool Has(JSThread *thread, const JSHandle<JSSharedMap> &map, JSTaggedValue key);

    static JSTaggedValue Get(JSThread *thread, const JSHandle<JSSharedMap> &map, JSTaggedValue key);

    static uint32_t GetSize(JSThread *thread, const JSHandle<JSSharedMap> &map);

    static JSTaggedValue GetKey(JSThread *thread, const JSHandle<JSSharedMap> &map, uint32_t entry);

    static JSTaggedValue GetValue(JSThread *thread, const JSHandle<JSSharedMap> &map, uint32_t entry);

    static constexpr size_t LINKED_MAP_OFFSET = JSObject::SIZE;
    ACCESSORS(LinkedMap, LINKED_MAP_OFFSET, MOD_RECORD_OFFSET)
    ACCESSORS_SYNCHRONIZED_PRIMITIVE_FIELD(ModRecord, uint32_t, MOD_RECORD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    static constexpr uint32_t MAX_INLINE = PropertyAttributes::MAX_FAST_PROPS_CAPACITY -
        SIZE / JSTaggedValue::TaggedTypeSize() + 1;
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LINKED_MAP_OFFSET, MOD_RECORD_OFFSET)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_SHARED_MAP_H
