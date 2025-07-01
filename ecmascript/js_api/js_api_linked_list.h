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

#ifndef ECMASCRIPT_JS_API_JS_API_LINKEDLIST_H
#define ECMASCRIPT_JS_API_JS_API_LINKEDLIST_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_list.h"

namespace panda::ecmascript {
class JSAPILinkedList : public JSObject {
public:
    static constexpr uint32_t DEFAULT_CAPACITY_LENGTH = 10;
    static JSAPILinkedList *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPILinkedList());
        return static_cast<JSAPILinkedList *>(object);
    }

    static void Add(JSThread *thread, const JSHandle<JSAPILinkedList> &list, const JSHandle<JSTaggedValue> &value);
    static JSHandle<JSAPILinkedList> Clone(JSThread *thread, const JSHandle<JSAPILinkedList> &list);
    static void AddFirst(JSThread *thread, const JSHandle<JSAPILinkedList> &list, const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue Insert(JSThread *thread, const JSHandle<JSAPILinkedList> &list,
                                const JSHandle<JSTaggedValue> &value, const int index);
    static JSTaggedValue PUBLIC_API Set(JSThread *thread, const JSHandle<JSAPILinkedList> &list,
                                        const int index, const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue ConvertToArray(const JSThread *thread, const JSHandle<JSAPILinkedList> &list);
    static JSHandle<TaggedArray> OwnKeys(JSThread *thread, const JSHandle<JSAPILinkedList> &list);
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSAPILinkedList> &list,
                               const JSHandle<JSTaggedValue> &key);
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSAPILinkedList> &list,
                                       const JSHandle<JSTaggedValue> &key);
    static bool SetProperty(JSThread *thread, const JSHandle<JSAPILinkedList> &obj,
                            const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue RemoveFirst(JSThread *thread, const JSHandle<JSAPILinkedList> &list);
    static JSTaggedValue RemoveLast(JSThread *thread, const JSHandle<JSAPILinkedList> &list);
    static JSTaggedValue RemoveByIndex(JSThread *thread, JSHandle<JSAPILinkedList> &list, const int index);
    static JSTaggedValue RemoveFirstFound(JSThread *thread, JSHandle<JSAPILinkedList> &list,
                                          const JSTaggedValue &element);
    static JSTaggedValue RemoveLastFound(JSThread *thread, JSHandle<JSAPILinkedList> &list,
                                         const JSTaggedValue &element);
    void Clear(JSThread *thread);
    bool Has(const JSThread *thread, const JSTaggedValue &element);
    JSTaggedValue GetFirst(const JSThread *thread);
    JSTaggedValue GetLast(const JSThread *thread);
    JSTaggedValue Get(const JSThread *thread, const int index);
    JSTaggedValue Remove(JSThread *thread, const JSTaggedValue &element);
    JSTaggedValue GetIndexOf(const JSThread *thread, const JSTaggedValue &element);
    JSTaggedValue GetLastIndexOf(const JSThread *thread, const JSTaggedValue &element);
    inline uint32_t Length(JSThread *thread)
    {
        return TaggedDoubleList::Cast(GetDoubleList(thread).GetTaggedObject())->Length();
    }
    static constexpr size_t DOUBLE_LIST_OFFSET = JSObject::SIZE;
    ACCESSORS(DoubleList, DOUBLE_LIST_OFFSET, SIZE);
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, DOUBLE_LIST_OFFSET, SIZE)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_API_JS_API_LinkedLIST_H
