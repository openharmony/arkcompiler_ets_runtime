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

#include "ecmascript/js_api/js_api_bitvector_iterator.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_bitvector.h"
#include "ecmascript/shared_objects/concurrent_api_scope.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
// BitVectorIteratorPrototype%.next ( )
JSTaggedValue JSAPIBitVectorIterator::Next(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPIBitVectorIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an bit vector iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIBitVectorIterator> iter(input);
    // Let a be O.[[IteratedBitVectorLike]].
    JSHandle<JSTaggedValue> bitVector(thread, iter->GetIteratedBitVector());
    // If a is undefined, return an undefinedIteratorResult.
    if (bitVector->IsUndefined()) {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }
    // Let index be O.[[BitVectorLikeNextIndex]].
    uint32_t index = iter->GetNextIndex();
    // If a has a [[TypedBitVectorName]] internal slot, then
    // Let len be the value of O’s [[BitVectorLength]] internal slot.
    ASSERT(bitVector->IsJSAPIBitVector());
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector> scope(thread, bitVector);
    const uint32_t length = static_cast<uint32_t>(JSHandle<JSAPIBitVector>::Cast(bitVector)->GetSize());
    // If index >= len, then
    if (index >= length) {
        // Set O.[[IteratedVectorLike]] to undefined.
        // Return undefinedIteratorResult.
        JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
        iter->SetIteratedBitVector(thread, undefinedHandle);
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }
    // Set O.[[VectorLikeNextIndex]] to index + 1.
    iter->SetNextIndex(index + 1);
    JSHandle<JSTaggedValue> value(thread, JSHandle<JSAPIBitVector>::Cast(bitVector)->Get(thread, index));

    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}
} // namespace panda::ecmascript
