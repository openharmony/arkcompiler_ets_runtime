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

#ifndef ECMASCRIPT_JS_ASYNC_FROM_SYNC_ITERATOR_H
#define ECMASCRIPT_JS_ASYNC_FROM_SYNC_ITERATOR_H

#include "ecmascript/ecma_string.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSAsyncFromSyncIterator : public JSObject {
public:
    static JSHandle<JSTaggedValue> CreateAsyncFromSyncIterator(JSThread *thread,
                                                               JSHandle<AsyncIteratorRecord> &syncIteratorRecord);
    static JSTaggedValue AsyncFromSyncIteratorContinuation(JSThread *thread, JSHandle<JSTaggedValue> &result,
                                                           JSHandle<PromiseCapability> &promiseCapability);
    static JSTaggedValue AsyncFromSyncIterUnwarpFunction(EcmaRuntimeCallInfo *argv);

    CAST_CHECK(JSAsyncFromSyncIterator, IsAsyncFromSyncIterator);

    static constexpr size_t SYNC_ITERATOR_RECORD = JSObject::SIZE;
    ACCESSORS(SyncIteratorRecord, SYNC_ITERATOR_RECORD, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, SYNC_ITERATOR_RECORD, SIZE);
    DECL_DUMP()
};
}
#endif  // ECMASCRIPT_JS_ASYNC_FROM_SYNC_ITERATOR_H
