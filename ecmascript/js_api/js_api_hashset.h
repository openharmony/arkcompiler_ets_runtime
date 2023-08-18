/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_API_JS_API_HASHSET_H
#define ECMASCRIPT_JS_API_JS_API_HASHSET_H

#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSAPIHashSet : public JSObject {
public:
    static JSAPIHashSet *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPIHashSet());
        return static_cast<JSAPIHashSet *>(object);
    }
    static void Add(JSThread *thread, JSHandle<JSAPIHashSet> hashSet, JSHandle<JSTaggedValue> value);
    static JSTaggedValue Remove(JSThread *thread, JSHandle<JSAPIHashSet> hashSet, JSTaggedValue key);

    JSTaggedValue IsEmpty();
    JSTaggedValue Has(JSThread *thread, JSTaggedValue value);
    void Clear(JSThread *thread);
    inline JSTaggedValue GetLength()
    {
        return JSTaggedValue(GetSize());
    }

    static constexpr size_t HASHSET_TABLE_INDEX = JSObject::SIZE;
    ACCESSORS(Table, HASHSET_TABLE_INDEX, HASHSET_SIZE_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Size, uint32_t, HASHSET_SIZE_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, HASHSET_TABLE_INDEX, HASHSET_SIZE_OFFSET)
    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_API_JS_API_HASHSET_H