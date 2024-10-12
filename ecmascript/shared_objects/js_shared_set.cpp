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

#include "ecmascript/shared_objects/js_shared_set.h"

#include "ecmascript/linked_hash_table.h"
#include "ecmascript/shared_objects/concurrent_api_scope.h"

namespace panda::ecmascript {
void JSSharedSet::Add(JSThread *thread, const JSHandle<JSSharedSet> &set, const JSHandle<JSTaggedValue> &value)
{
    if (!value->IsSharedType()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::TYPE_ERROR,
                                                               "Parameter error. Only accept sendable value.");
        THROW_NEW_ERROR_AND_RETURN(thread, error);
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedSet, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(set));
    RETURN_IF_ABRUPT_COMPLETION(thread);

    JSHandle<LinkedHashSet> setHandle(thread, LinkedHashSet::Cast(set->GetLinkedSet().GetTaggedObject()));
    JSHandle<LinkedHashSet> newSet = LinkedHashSet::Add(thread, setHandle, value);
    set->SetLinkedSet(thread, newSet);
}

bool JSSharedSet::Delete(JSThread *thread, const JSHandle<JSSharedSet> &set, const JSHandle<JSTaggedValue> &value)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedSet, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(set));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    JSHandle<LinkedHashSet> setHandle(thread, LinkedHashSet::Cast(set->GetLinkedSet().GetTaggedObject()));
    int entry = setHandle->FindElement(thread, value.GetTaggedValue());
    if (entry == -1) {
        return false;
    }
    setHandle->RemoveEntry(thread, entry);
    return true;
}

void JSSharedSet::Clear(JSThread *thread, const JSHandle<JSSharedSet> &set)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedSet, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(set));
    RETURN_IF_ABRUPT_COMPLETION(thread);
    JSHandle<LinkedHashSet> setHandle(thread, LinkedHashSet::Cast(set->GetLinkedSet().GetTaggedObject()));
    JSHandle<LinkedHashSet> newSet = LinkedHashSet::Clear(thread, setHandle);
    set->SetLinkedSet(thread, newSet);
}

bool JSSharedSet::Has(JSThread *thread, const JSHandle<JSSharedSet> &set, JSTaggedValue value)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedSet> scope(thread, JSHandle<JSTaggedValue>::Cast(set));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    return LinkedHashSet::Cast(set->GetLinkedSet().GetTaggedObject())->Has(thread, value);
}

uint32_t JSSharedSet::GetSize(JSThread *thread, const JSHandle<JSSharedSet> &set)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedSet> scope(thread, JSHandle<JSTaggedValue>::Cast(set));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    return LinkedHashSet::Cast(set->GetLinkedSet().GetTaggedObject())->NumberOfElements();
}

JSTaggedValue JSSharedSet::GetValue(JSThread *thread, const JSHandle<JSSharedSet> &set, int entry)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedSet> scope(thread, JSHandle<JSTaggedValue>::Cast(set));
    ASSERT_PRINT(entry >= 0 && static_cast<uint32_t>(entry) < GetSize(thread, set),
        "entry must be non-negative integer less than capacity");
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());
    return LinkedHashSet::Cast(set->GetLinkedSet().GetTaggedObject())->GetValue(entry);
}
}  // namespace panda::ecmascript
