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

#ifndef ECMASCRIPT_JSSET_H
#define ECMASCRIPT_JSSET_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSSet : public JSObject {
public:
    CAST_CHECK(JSSet, IsJSSet);

    static bool Delete(const JSThread *thread, const JSHandle<JSSet> &set, const JSHandle<JSTaggedValue> &value);

    static void Add(JSThread *thread, const JSHandle<JSSet> &set, const JSHandle<JSTaggedValue> &value);

    static void Clear(const JSThread *thread, const JSHandle<JSSet> &set);

    bool Has(const JSThread *thread, JSTaggedValue value) const;

    uint32_t GetSize(const JSThread *thread) const;

    JSTaggedValue GetValue(const JSThread *thread, int entry) const;

    static constexpr size_t LINKED_SET_OFFSET = JSObject::SIZE;
    ACCESSORS(LinkedSet, LINKED_SET_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LINKED_SET_OFFSET, SIZE)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSSET_H
