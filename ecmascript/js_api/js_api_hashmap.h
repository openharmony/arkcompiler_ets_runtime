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

#ifndef ECMASCRIPT_JS_API_JS_API_HASHMAP_H
#define ECMASCRIPT_JS_API_JS_API_HASHMAP_H

#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_node.h"

namespace panda::ecmascript {
class JSAPIHashMap : public JSObject {
public:
    static JSAPIHashMap *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPIHashMap());
        return static_cast<JSAPIHashMap *>(object);
    }

    static void Set(JSThread *thread, JSHandle<JSAPIHashMap> hashMap,
                    JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> value);
    static JSTaggedValue HasValue(JSThread *thread, JSHandle<JSAPIHashMap> hashMap,
                                  JSHandle<JSTaggedValue> value);
    static void SetAll(JSThread *thread, JSHandle<JSAPIHashMap> dst, JSHandle<JSAPIHashMap> src);
    static JSTaggedValue Remove(JSThread *thread, JSHandle<JSAPIHashMap> hashMap, JSTaggedValue key);
    JSTaggedValue IsEmpty();
    JSTaggedValue HasKey(JSThread *thread, JSTaggedValue key);
    bool Replace(JSThread *thread, JSTaggedValue key, JSTaggedValue newValue);
    void Clear(JSThread *thread);
    JSTaggedValue Get(JSThread *thread, JSTaggedValue key);
    inline JSTaggedValue GetLength()
    {
        return JSTaggedValue(GetSize());
    }

    static constexpr size_t HASHMAP_TABLE_INDEX = JSObject::SIZE;
    ACCESSORS(Table, HASHMAP_TABLE_INDEX, HASHMAP_SIZE_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Size, uint32_t, HASHMAP_SIZE_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, HASHMAP_TABLE_INDEX, HASHMAP_SIZE_OFFSET)
    DECL_DUMP()

private:
    static void SetAllLinkedNode(JSThread *thread, JSHandle<JSAPIHashMap> hashMap, JSMutableHandle<LinkedNode> node);
    static void SetAllRBTreeNode(JSThread *thread, JSHandle<JSAPIHashMap> hashMap, JSHandle<RBTreeNode> node);
    static bool HasValueLinkedNode(JSThread *thread, JSTaggedValue node, JSTaggedValue value);
    static bool HasValueRBTreeNode(JSThread *thread, JSTaggedValue node, JSTaggedValue value);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_API_JS_API_HASHMAP_H
