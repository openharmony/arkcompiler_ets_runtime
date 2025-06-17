/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SENDABLEENV_H
#define ECMASCRIPT_SENDABLEENV_H

#include "ecmascript/js_object.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
class SendableEnv : public TaggedArray {
public:
    static constexpr uint32_t SENDABLE_PARENT_ENV_INDEX = 0;
    static constexpr uint32_t SENDABLE_SCOPE_INFO_INDEX = 1;
    static constexpr uint32_t SENDABLE_RESERVED_ENV_LENGTH = 2;

    static SendableEnv *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<SendableEnv *>(object);
    }

    static size_t ComputeSize(uint32_t numSlots)
    {
        return TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), numSlots + SENDABLE_RESERVED_ENV_LENGTH);
    }

    void SetParentEnv(JSThread *thread, JSTaggedValue value)
    {
        Set(thread, SENDABLE_PARENT_ENV_INDEX, value);
    }

    JSTaggedValue GetParentEnv(JSThread *thread) const
    {
        return Get(thread, SENDABLE_PARENT_ENV_INDEX);
    }

    JSTaggedValue GetProperties(JSThread *thread, uint32_t index) const
    {
        return Get(thread, index + SENDABLE_RESERVED_ENV_LENGTH);
    }

    void SetProperties(JSThread *thread, uint32_t index, JSTaggedValue value)
    {
        Set(thread, index + SENDABLE_RESERVED_ENV_LENGTH, value);
    }

    JSTaggedValue GetScopeInfo(JSThread *thread) const
    {
        return Get(thread, SENDABLE_SCOPE_INFO_INDEX);
    }

    void SetScopeInfo(JSThread *thread, JSTaggedValue value)
    {
        Set(thread, SENDABLE_SCOPE_INFO_INDEX, value);
    }

    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SENDABLEENV_H
