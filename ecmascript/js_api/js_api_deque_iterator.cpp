/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/js_api/js_api_deque_iterator.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
// DequeIteratorPrototype%.next ( )
JSTaggedValue JSAPIDequeIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPIDequeIterator()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                            "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIDequeIterator> iter(input);
    JSHandle<JSTaggedValue> iteratorDeque(thread, iter->GetIteratedDeque(thread));
    if (iteratorDeque->IsUndefined()) {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }
    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(iteratorDeque);
    uint32_t index = iter->GetNextIndex();

    JSHandle<TaggedArray> elements(thread, deque->GetElements(thread));
    uint32_t capacity = elements->GetLength();
    uint32_t first = deque->GetFirst();
    uint32_t last = deque->GetLast();
    if (index == last) {
        JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
        iter->SetIteratedDeque(thread, undefinedHandle);
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }
    ASSERT(capacity != 0);
    iter->SetNextIndex((index + 1) % capacity);
    uint32_t elementIndex = (index + capacity - first) % capacity;
    JSHandle<JSTaggedValue> value(thread, JSHandle<JSAPIDeque>::Cast(iteratorDeque)->Get(thread, elementIndex));

    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}
} // namespace panda::ecmascript
