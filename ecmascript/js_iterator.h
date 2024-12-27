/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_ITERATOR_H
#define ECMASCRIPT_JS_ITERATOR_H

#include "ecmascript/accessor_data.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
enum class IterationKind : uint8_t {
    KEY = 0,
    VALUE,
    KEY_AND_VALUE
};

class AsyncIteratorRecord final : public Record {
public:
    CAST_CHECK(AsyncIteratorRecord, IsAsyncIteratorRecord);

    static constexpr size_t ITERATOR_OFFSET = Record::SIZE;
    ACCESSORS(Iterator, ITERATOR_OFFSET, NEXTMETHOD_OFFSET);
    ACCESSORS(NextMethod, NEXTMETHOD_OFFSET, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    static constexpr size_t DONE_BITS = 1;
    FIRST_BIT_FIELD(BitField, Done, bool, DONE_BITS)

    DECL_VISIT_OBJECT(ITERATOR_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()
};

class JSIterator final {
public:
    static constexpr int VALUE_INLINE_PROPERTY_INDEX = 0;
    static constexpr int DONE_INLINE_PROPERTY_INDEX = 1;

    static constexpr size_t SIZE = JSObject::SIZE;
    
    static uint32_t GetInlinedPropertyOffset(uint32_t index)
    {
        return JSIterator::SIZE + index * JSTaggedValue::TaggedTypeSize();
    }

    static JSTaggedValue IteratorCloseAndReturn(JSThread *thread, const JSHandle<JSTaggedValue> &iter);
    // 7.4.1
    static JSHandle<JSTaggedValue> GetIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj);

    static JSHandle<JSTaggedValue> GetIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                               const JSHandle<JSTaggedValue> &method);

    static JSHandle<JSTaggedValue> GetAsyncIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    // 7.4.2
    static JSHandle<JSTaggedValue> IteratorNext(JSThread *thread, const JSHandle<JSTaggedValue> &iter,
                                           const JSHandle<JSTaggedValue> &value);

    static JSHandle<JSTaggedValue> IteratorNext(JSThread *thread, const JSHandle<AsyncIteratorRecord> &iter,
                                                 const JSHandle<JSTaggedValue> &value);

    static JSHandle<JSTaggedValue> IteratorNext(JSThread *thread, const JSHandle<AsyncIteratorRecord> &iter);

    static JSHandle<JSTaggedValue> IteratorNext(JSThread *thread, const JSHandle<JSTaggedValue> &iter);
    // 7.4.3
    static bool IteratorComplete(JSThread *thread, const JSHandle<JSTaggedValue> &iterResult);
    // 7.4.4
    static JSHandle<JSTaggedValue> IteratorValue(JSThread *thread, const JSHandle<JSTaggedValue> &iterResult);
    // 7.4.5
    static JSHandle<JSTaggedValue> IteratorStep(JSThread *thread, const JSHandle<JSTaggedValue> &iter);
    // 7.4.6
    static JSHandle<JSTaggedValue> IteratorClose(JSThread *thread, const JSHandle<JSTaggedValue> &iter,
                                                 const JSHandle<JSTaggedValue> &completion);
    // 7.4.7
    static JSHandle<JSObject> CreateIterResultObject(JSThread *thread, const JSHandle<JSTaggedValue> &value, bool done);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_ITERATOR_H
