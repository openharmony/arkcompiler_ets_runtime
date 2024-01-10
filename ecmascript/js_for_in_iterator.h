/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_FORIN_ITERATOR_H
#define ECMASCRIPT_JS_FORIN_ITERATOR_H

#include <utility>

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
class JSForInIterator : public JSObject {
public:
    CAST_CHECK(JSForInIterator, IsForinIterator);

    static JSTaggedValue NextInternal(JSThread *thread, const JSHandle<JSForInIterator> &it);
    static JSTaggedValue NextInternalSlowpath(JSThread *thread, const JSHandle<JSForInIterator> &it);

    static JSTaggedValue Next(EcmaRuntimeCallInfo *msg);

    static constexpr size_t OBJECT_OFFSET = JSObject::SIZE;
    ACCESSORS(Object, OBJECT_OFFSET, CACHED_HCLASS_OFFSET)
    ACCESSORS(CachedHclass, CACHED_HCLASS_OFFSET, KEYS_OFFSET)
    ACCESSORS(Keys, KEYS_OFFSET, INDEX_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(Index, uint32_t, INDEX_OFFSET, LENGTH_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(Length, uint32_t, LENGTH_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, OBJECT_OFFSET, INDEX_OFFSET)
    DECL_DUMP()

private:
    static bool IsEnumCacheValid(JSTaggedValue receiver, JSTaggedValue cachedHclass, EnumCacheKind kind);
    static bool NeedCheckProperty(JSTaggedValue receiver);
    static bool HasProperty(JSThread *thread, JSHandle<JSTaggedValue> receiver, JSHandle<JSTaggedValue> key);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_FORIN_ITERATOR_H
