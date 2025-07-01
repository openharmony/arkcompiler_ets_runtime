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

#ifndef ECMASCRIPT_JS_API_JS_API_LIST_H
#define ECMASCRIPT_JS_API_JS_API_LIST_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_list.h"

namespace panda::ecmascript {
class JSAPIList : public JSObject {
public:
    static constexpr uint32_t DEFAULT_CAPACITY_LENGTH = 10;
    static constexpr uint32_t UNUSED_BIT_FILEDS_NUM = 31;
    using IsOrderedListBit = BitField<uint32_t, 0, UNUSED_BIT_FILEDS_NUM>::NextFlag;

    static JSAPIList *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPIList());
        return static_cast<JSAPIList *>(object);
    }

    inline void SetIsOrderedList(bool flag) const
    {
        IsOrderedListBit::Set<uint32_t>(flag, GetBitFieldAddr());
    }

    inline bool IsOrderedList() const
    {
        uint32_t bits = GetBitField();
        return IsOrderedListBit::Decode(bits);
    }

    uint32_t *GetBitFieldAddr() const
    {
        return reinterpret_cast<uint32_t *>(ToUintPtr(this) + BIT_FIELD_OFFSET);
    }

    inline void ClearBitField()
    {
        SetBitField(0UL);
    }

    static void Add(JSThread *thread, const JSHandle<JSAPIList> &list, const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue Insert(JSThread *thread, const JSHandle<JSAPIList> &list,
                                const JSHandle<JSTaggedValue> &value, const int index);
    static JSTaggedValue PUBLIC_API Set(JSThread *thread, const JSHandle<JSAPIList> &list,
                                        const int index, const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue ReplaceAllElements(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                            const JSHandle<JSTaggedValue> &callbackFn,
                                            const JSHandle<JSTaggedValue> &thisArg);
    static JSTaggedValue Sort(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                              const JSHandle<JSTaggedValue> &callbackFn);
    static JSTaggedValue ConvertToArray(const JSThread *thread, const JSHandle<JSAPIList> &list);
    static JSTaggedValue GetSubList(JSThread *thread, const JSHandle<JSAPIList> &list,
                                    const int fromIndex, const int toIndex);
    static JSTaggedValue RemoveByIndex(JSThread *thread, const JSHandle<JSAPIList> &list, const int &index);
    static JSHandle<TaggedArray> OwnKeys(JSThread *thread, const JSHandle<JSAPIList> &list);
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSAPIList> &list,
                               const JSHandle<JSTaggedValue> &key);
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSAPIList> &list,
                                       const JSHandle<JSTaggedValue> &key);
    static bool SetProperty(JSThread *thread, const JSHandle<JSAPIList> &obj,
                            const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue FastGet(JSThread *thread, const int index, const JSHandle<JSAPIList> &list);

    JSTaggedValue GetFirst(const JSThread *thread);
    JSTaggedValue GetLast(const JSThread *thread);
    bool IsEmpty(const JSThread *thread);
    JSTaggedValue Get(const JSThread *thread, const int index);
    bool Has(const JSThread *thread, const JSTaggedValue &element);
    JSTaggedValue GetIndexOf(const JSThread *thread, const JSTaggedValue &element);
    JSTaggedValue GetLastIndexOf(const JSThread *thread, const JSTaggedValue &element);
    JSTaggedValue Equal(JSThread *thread, const JSHandle<JSAPIList> &list);
    void Clear(JSThread *thread);
    JSTaggedValue Remove(JSThread *thread, const JSTaggedValue &element);
    inline uint32_t Length(const JSThread *thread)
    {
        return TaggedSingleList::Cast(GetSingleList(thread).GetTaggedObject())->Length();
    }

    static constexpr size_t SINGLY_LIST_OFFSET = JSObject::SIZE;
    ACCESSORS(SingleList, SINGLY_LIST_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(BitField, uint32_t, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, SINGLY_LIST_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_API_JS_API_LIST_H
