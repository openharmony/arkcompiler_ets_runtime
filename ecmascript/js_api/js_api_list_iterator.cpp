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

#include "ecmascript/js_api/js_api_list_iterator.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/js_api/js_api_list.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
JSTaggedValue JSAPIListIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    if (!input->IsJSAPIListIterator()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                            "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIListIterator> iter(input);
    JSHandle<JSTaggedValue> list(thread, iter->GetIteratedList());
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<TaggedSingleList> singleList(list);
    if (list->IsUndefined()) {
        return globalConst->GetUndefinedIterResult();
    }
    int index = static_cast<int>(iter->GetNextIndex());
    int length = singleList->Length();
    if (index >= length) {
        JSHandle<JSTaggedValue> undefinedHandle = globalConst->GetHandledUndefined();
        iter->SetIteratedList(thread, undefinedHandle);
        return globalConst->GetUndefinedIterResult();
    }
    iter->SetNextIndex(index + 1);
    int dataIndex = static_cast<int>(iter->GetDataIndex());
    std::pair<int, JSTaggedValue> resultPair = singleList->GetByDataIndex(dataIndex);
    iter->SetDataIndex(resultPair.first);
    JSHandle<JSTaggedValue> value(thread, resultPair.second);
    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}

JSHandle<JSTaggedValue> JSAPIListIterator::CreateListIterator(JSThread *thread, JSHandle<JSTaggedValue> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSAPIList()) {
        if (obj->IsJSProxy() && JSHandle<JSProxy>::Cast(obj)->GetTarget().IsJSAPIList()) {
            obj = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(obj)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The Symbol.iterator method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error,
                                             JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
        }
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSAPIListIterator(JSHandle<JSAPIList>(obj)));
    return iter;
}
} // namespace panda::ecmascript
