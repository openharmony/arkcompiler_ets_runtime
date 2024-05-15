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

#ifndef ECMASCRIPT_JS_SHARED_TYPED_ARRAY_H
#define ECMASCRIPT_JS_SHARED_TYPED_ARRAY_H

#include "ecmascript/js_typed_array.h"

#include "ecmascript/js_dataview.h"
#include "ecmascript/js_object.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
class JSSharedTypedArray : public JSTypedArray {
public:
    static constexpr size_t MAX_ONHEAP_LENGTH = 512 * 8;
    static JSSharedTypedArray *Cast(TaggedObject *object)
    {
    #if ECMASCRIPT_ENABLE_CAST_CHECK
        if (!(JSTaggedValue(object).IsSharedTypedArray() || JSTaggedValue(object).IsJSSharedTypedArray())) {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    #else
        ASSERT(JSTaggedValue(object).IsSharedTypedArray() || JSTaggedValue(object).IsJSSharedTypedArray());
    #endif
        return static_cast<JSSharedTypedArray *>(object);
    }

    static JSTaggedValue GetSharedOffHeapBuffer(JSThread *thread, JSHandle<JSSharedTypedArray> typedArray);

    static constexpr size_t MOD_RECORD_OFFSET = JSTypedArray::SIZE;
    ACCESSORS_SYNCHRONIZED_PRIMITIVE_FIELD(ModRecord, uint32_t, MOD_RECORD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    static constexpr uint32_t MAX_INLINE = PropertyAttributes::MAX_FAST_PROPS_CAPACITY -
        SIZE / JSTaggedValue::TaggedTypeSize() + 1;
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSTypedArray, MOD_RECORD_OFFSET, MOD_RECORD_OFFSET)
    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_SHARED_TYPED_ARRAY_H
