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

#include "ecmascript/js_api/js_api_vector_iterator.h"

#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_vector.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
// VectorIteratorPrototype%.next ( )
JSTaggedValue JSAPIVectorIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPIVectorIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an vector iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIVectorIterator> iter(input);
    // Let a be O.[[IteratedVectorLike]].
    JSHandle<JSTaggedValue> vector(thread, iter->GetIteratedVector());
    // If a is undefined, return an undefinedIteratorResult.
    if (vector->IsUndefined()) {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }
    // Let index be O.[[VectorLikeNextIndex]].
    uint32_t index = iter->GetNextIndex();
    // If a has a [[TypedVectorName]] internal slot, then
    // Let len be the value of O’s [[VectorLength]] internal slot.
    ASSERT(vector->IsJSAPIVector());
    const uint32_t length = static_cast<uint32_t>(JSHandle<JSAPIVector>::Cast(vector)->GetSize());
    // If index >= len, then
    if (index >= length) {
        // Set O.[[IteratedVectorLike]] to undefined.
        // Return undefinedIteratorResult.
        JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
        iter->SetIteratedVector(thread, undefinedHandle);
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }
    // Set O.[[VectorLikeNextIndex]] to index + 1.
    iter->SetNextIndex(index + 1);
    JSHandle<JSTaggedValue> value(thread, JSAPIVector::Get(thread, JSHandle<JSAPIVector>::Cast(vector),
                                    static_cast<int32_t>(index)));
    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}
} // namespace panda::ecmascript
