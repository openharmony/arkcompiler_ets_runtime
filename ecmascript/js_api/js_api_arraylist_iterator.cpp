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

#include "ecmascript/js_api/js_api_arraylist_iterator.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_arraylist.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
// ArrayListIteratorPrototype%.next ( )
JSTaggedValue JSAPIArrayListIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    if (!input->IsJSAPIArrayListIterator()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                            "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIArrayListIterator> iter(input);
    JSHandle<JSTaggedValue> arrayList(thread, iter->GetIteratedArrayList(thread));

    if (arrayList->IsUndefined()) {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }

    uint32_t index = iter->GetNextIndex();
    uint32_t length = 0;

    if (arrayList->IsJSAPIArrayList()) {
        length = JSHandle<JSAPIArrayList>(arrayList)->GetLength(thread).GetArrayLength();
    }

    if (index >= length) {
        JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
        iter->SetIteratedArrayList(thread, undefinedHandle);
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }

    iter->SetNextIndex(index + 1);
    JSHandle<JSTaggedValue> value(thread, JSHandle<JSAPIArrayList>::Cast(arrayList)->Get(thread, index));

    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}
} // namespace panda::ecmascript
