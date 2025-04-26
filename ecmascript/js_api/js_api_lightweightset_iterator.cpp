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

#include "ecmascript/js_api/js_api_lightweightset_iterator.h"

#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_lightweightset.h"
#include "ecmascript/js_array.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
JSTaggedValue JSAPILightWeightSetIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    if (!input->IsJSAPILightWeightSetIterator()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                            "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPILightWeightSetIterator> iter(input);
    JSHandle<JSTaggedValue> lightWeightSet(thread, iter->GetIteratedLightWeightSet());
    uint32_t index = iter->GetNextIndex();
    IterationKind itemKind = IterationKind(iter->GetIterationKind());
    if (lightWeightSet->IsUndefined()) {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }
    uint32_t length = 0;
    if (lightWeightSet->IsJSAPILightWeightSet()) {
        length = JSHandle<JSAPILightWeightSet>(lightWeightSet)->GetLength();
    }
    if (index >= length) {
        JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
        iter->SetIteratedLightWeightSet(thread, undefinedHandle);
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }
    iter->SetNextIndex(index + 1);
    JSHandle<JSAPILightWeightSet> lightWeightSetHandle(lightWeightSet);
    JSAPILightWeightSet::CheckAndCopyValues(thread, lightWeightSetHandle);
    JSHandle<TaggedArray> valueArray(
        thread, TaggedArray::Cast(lightWeightSetHandle->GetValues().GetTaggedObject()));
    JSHandle<JSTaggedValue> value(thread, valueArray->Get(index));
    if (itemKind == IterationKind::VALUE) {
        return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array = factory->NewTaggedArray(2); // 2 means the length of array
    array->Set(thread, 0, value);
    array->Set(thread, 1, value);
    JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
    return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
}
} // namespace panda::ecmascript
