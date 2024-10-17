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

#include "ecmascript/js_api/js_api_linked_list_iterator.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tagged_list.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
JSTaggedValue JSAPILinkedListIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    if (!input->IsJSAPILinkedListIterator()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                            "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedListIterator> iter(input);
    JSHandle<JSTaggedValue> linkedList(thread, iter->GetIteratedLinkedList());
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<TaggedDoubleList> list(linkedList);
    if (linkedList->IsUndefined()) {
        return globalConst->GetUndefinedIterResult();
    }
    int index = static_cast<int>(iter->GetNextIndex());
    int length = list->Length();
    if (index >= length) {
        JSHandle<JSTaggedValue> undefinedHandle = globalConst->GetHandledUndefined();
        iter->SetIteratedLinkedList(thread, undefinedHandle);
        return globalConst->GetUndefinedIterResult();
    }
    iter->SetNextIndex(index + 1);
    int dataIndex = static_cast<int>(iter->GetDataIndex());
    std::pair<int, JSTaggedValue> resultPair = list->GetByDataIndex(dataIndex);
    iter->SetDataIndex(resultPair.first);
    JSHandle<JSTaggedValue> value(thread, resultPair.second);
    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}

JSHandle<JSTaggedValue> JSAPILinkedListIterator::CreateLinkedListIterator(JSThread *thread,
                                                                          JSHandle<JSTaggedValue> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSAPILinkedList()) {
        if (obj->IsJSProxy() && JSHandle<JSProxy>::Cast(obj)->GetTarget().IsJSAPILinkedList()) {
            obj = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(obj)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The Symbol.iterator method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error,
                                             JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
        }
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSAPILinkedListIterator(JSHandle<JSAPILinkedList>(obj)));
    return iter;
}
} // namespace panda::ecmascript
