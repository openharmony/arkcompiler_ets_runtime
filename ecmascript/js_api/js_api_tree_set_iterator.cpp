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

#include "ecmascript/js_api/js_api_tree_set_iterator.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tagged_tree.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
JSTaggedValue JSAPITreeSetIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // Let input be the this value
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPITreeSetIterator()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                            "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPITreeSetIterator> iter(input);
    // Let it be [[IteratedSet]].
    JSHandle<JSTaggedValue> iteratedSet(thread, iter->GetIteratedSet(thread));

    // If it is undefined, return undefinedIteratorResult.
    if (iteratedSet->IsUndefined()) {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }
    JSHandle<TaggedTreeSet> set(thread, JSHandle<JSAPITreeSet>::Cast(iteratedSet)->GetTreeSet(thread));
    uint32_t elements = static_cast<uint32_t>(set->NumberOfElements());

    JSMutableHandle<TaggedArray> entries(thread, iter->GetEntries(thread));
    if ((iter->GetEntries(thread).IsHole()) || (elements != entries->GetLength())) {
        entries.Update(TaggedTreeSet::GetArrayFromSet(thread, set).GetTaggedValue());
        iter->SetEntries(thread, entries);
    }

    // Let index be Set.[[NextIndex]].
    uint32_t index = static_cast<uint32_t>(iter->GetNextIndex());
    if (index < elements) {
        IterationKind itemKind = IterationKind(iter->GetIterationKind());

        int keyIndex = entries->Get(thread, index).GetInt();
        iter->SetNextIndex(index + 1);

        JSHandle<JSTaggedValue> key(thread, set->GetKey(thread, keyIndex));
        // If itemKind is key or value, let result be e.[[Key]].
        if (itemKind == IterationKind::VALUE || itemKind == IterationKind::KEY) {
            return JSIterator::CreateIterResultObject(thread, key, false).GetTaggedValue();
        }
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> array = factory->NewTaggedArray(2);  // 2 means the length of array
        array->Set(thread, 0, key);
        array->Set(thread, 1, key);
        JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
        return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
    }

    // Set [[IteratedSet]] to undefined.
    iter->SetIteratedSet(thread, JSTaggedValue::Undefined());
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    return env->GetUndefinedIteratorResult().GetTaggedValue();
}

JSHandle<JSTaggedValue> JSAPITreeSetIterator::CreateTreeSetIterator(JSThread *thread,
                                                                    JSHandle<JSTaggedValue> &obj,
                                                                    IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSAPITreeSet()) {
        if (obj->IsJSProxy() && JSHandle<JSProxy>::Cast(obj)->GetTarget(thread).IsJSAPITreeSet()) {
            obj = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(obj)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The Symbol.iterator method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error,
                                             JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
        }
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSAPITreeSetIterator(JSHandle<JSAPITreeSet>(obj), kind));
    return iter;
}
}  // namespace panda::ecmascript
