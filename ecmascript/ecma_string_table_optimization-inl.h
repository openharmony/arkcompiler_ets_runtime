/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_STRING_TABLE_OPTIMIZ_INL_H
#define ECMASCRIPT_STRING_TABLE_OPTIMIZ_INL_H

#include "common_components/objects/string_table/hashtriemap-inl.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/runtime_lock.h"

namespace panda::ecmascript {
#if ENABLE_NEXT_OPTIMIZATION

template <typename LoaderCallback, typename EqualsCallback>
EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, uint32_t hashcode, LoaderCallback loaderCallback,
                                               EqualsCallback equalsCallback)
{
    BaseString *result = stringTable_.LoadOrStore<true>(vm->GetJSThread(), hashcode, loaderCallback, equalsCallback);
    ASSERT(result != nullptr);
    return EcmaString::FromBaseString(result);
}

inline void StringTableMutex::LockWithThreadState(JSThread* thread)
{
    return RuntimeLock(thread, mtx_);
}

#endif
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_STRING_TABLE_OPTIMIZ_INL_H
