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

#include "ecmascript/js_api/js_api_stack_iterator.h"

#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/containers/containers_errors.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
// StackIteratorPrototype%.next()
JSTaggedValue JSAPIStackIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPIStackIterator()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                            "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIStackIterator> iter(input);
    JSHandle<JSTaggedValue> stack(thread, iter->GetIteratedStack());
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    if (stack->IsUndefined()) {
        return globalConst->GetUndefinedIterResult();
    }
    uint32_t index = iter->GetNextIndex();

    uint32_t length = static_cast<uint32_t>((JSHandle<JSAPIStack>::Cast(stack))->GetSize() + 1);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    if (index + 1 > length) {
        JSHandle<JSTaggedValue> undefinedHandle = globalConst->GetHandledUndefined();
        iter->SetIteratedStack(thread, undefinedHandle);
        return globalConst->GetUndefinedIterResult();
    }
    iter->SetNextIndex(index + 1);
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(index));
    JSHandle<JSTaggedValue> value(thread, JSHandle<JSAPIStack>::Cast(stack)->Get(index));
    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}
}  // namespace panda::ecmascript
