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
 
#include "ecmascript/mem/assert_scope.h"

namespace panda::ecmascript {

using AssertGarbageCollectBit = panda::BitField<bool, 0, 1>;            // default enable
using AssertHeapAllocBit = AssertGarbageCollectBit::NextFlag;           // default enable
using AssertHandleAllocBit = AssertHeapAllocBit::NextFlag;              // default enable
using AssertDeRefHandleBit = AssertHandleAllocBit::NextFlag;            // default enable
using AssertLocalToShareWeakRefBit = AssertDeRefHandleBit::NextFlag;    // default disable

constexpr size_t INITIAL_ASSERT_DATA = AssertGarbageCollectBit::Mask() |
                                       AssertHeapAllocBit::Mask() |
                                       AssertHandleAllocBit::Mask() |
                                       AssertDeRefHandleBit::Mask();

thread_local size_t currentAssertData = INITIAL_ASSERT_DATA;

template<AssertType type, bool isAllow, bool IsDebug>
bool AssertScopeT<type, isAllow, IsDebug>::IsAllowed()
{
    return true;
};

template<AssertType type, bool isAllow>
AssertScopeT<type, isAllow, true>::AssertScopeT() : oldData_(currentAssertData)
{
    switch (type) {
        case AssertType::GARBAGE_COLLECTION_ASSERT:
            currentAssertData = AssertGarbageCollectBit::Update(oldData_.value(), isAllow);
            break;
        case AssertType::HEAP_ALLOC_ASSERT:
            currentAssertData = AssertHeapAllocBit::Update(oldData_.value(), isAllow);
            break;
        case AssertType::HANDLE_ALLOC_ASSERT:
            currentAssertData = AssertHandleAllocBit::Update(oldData_.value(), isAllow);
            break;
        case AssertType::DEREF_HANDLE_ASSERT:
            currentAssertData = AssertDeRefHandleBit::Update(oldData_.value(), isAllow);
            break;
        case AssertType::LOCAL_TO_SHARE_WEAK_REF_ASSERT:
            currentAssertData = AssertLocalToShareWeakRefBit::Update(oldData_.value(), isAllow);
            break;
        default:
            break;
    }
}

template<AssertType type, bool isAllow>
AssertScopeT<type, isAllow, true>::~AssertScopeT()
{
    if (!oldData_.has_value()) {
        return;
    }
    currentAssertData = oldData_.value();
    oldData_.reset();
}

template<AssertType type, bool isAllow>
bool AssertScopeT<type, isAllow, true>::IsAllowed()
{
    switch (type) {
        case AssertType::GARBAGE_COLLECTION_ASSERT:
            return AssertGarbageCollectBit::Decode(currentAssertData);
        case AssertType::HEAP_ALLOC_ASSERT:
            return AssertHeapAllocBit::Decode(currentAssertData);
        case AssertType::HANDLE_ALLOC_ASSERT:
            return AssertHandleAllocBit::Decode(currentAssertData);
        case AssertType::DEREF_HANDLE_ASSERT:
            return AssertDeRefHandleBit::Decode(currentAssertData);
        case AssertType::LOCAL_TO_SHARE_WEAK_REF_ASSERT:
            return AssertLocalToShareWeakRefBit::Decode(currentAssertData);
        default:
            return true;
    }
}

template class PUBLIC_API AssertScopeT<AssertType::GARBAGE_COLLECTION_ASSERT, false, IS_ALLOW_CHECK>;
template class PUBLIC_API AssertScopeT<AssertType::GARBAGE_COLLECTION_ASSERT, true, IS_ALLOW_CHECK>;
template class PUBLIC_API AssertScopeT<AssertType::HEAP_ALLOC_ASSERT, false, IS_ALLOW_CHECK>;
template class PUBLIC_API AssertScopeT<AssertType::HEAP_ALLOC_ASSERT, true, IS_ALLOW_CHECK>;
template class PUBLIC_API AssertScopeT<AssertType::HANDLE_ALLOC_ASSERT, false, IS_ALLOW_CHECK>;
template class PUBLIC_API AssertScopeT<AssertType::HANDLE_ALLOC_ASSERT, true, IS_ALLOW_CHECK>;
template class PUBLIC_API AssertScopeT<AssertType::DEREF_HANDLE_ASSERT, false, IS_ALLOW_CHECK>;
template class PUBLIC_API AssertScopeT<AssertType::DEREF_HANDLE_ASSERT, true, IS_ALLOW_CHECK>;
template class PUBLIC_API AssertScopeT<AssertType::LOCAL_TO_SHARE_WEAK_REF_ASSERT, false, IS_ALLOW_CHECK>;
template class PUBLIC_API AssertScopeT<AssertType::LOCAL_TO_SHARE_WEAK_REF_ASSERT, true, IS_ALLOW_CHECK>;

}  // namespace panda::ecmascript