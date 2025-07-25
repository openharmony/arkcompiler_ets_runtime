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

#ifndef ECMASCRIPT_LEXICALENV_H
#define ECMASCRIPT_LEXICALENV_H

#include "ecmascript/base_env.h"

namespace panda::ecmascript {
class LexicalEnv : public BaseEnv {
public:
    // the first field is used to store GlobalEnv
    static constexpr uint32_t PARENT_ENV_INDEX = 1;
    static constexpr uint32_t SCOPE_INFO_INDEX = 2;
    static constexpr uint32_t RESERVED_ENV_LENGTH = 3;

    static LexicalEnv *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsLexicalEnv());
        return static_cast<LexicalEnv *>(object);
    }

    static size_t ComputeSize(uint32_t numSlots)
    {
        return TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), numSlots + RESERVED_ENV_LENGTH);
    }

    void SetParentEnv(JSThread *thread, JSTaggedValue value)
    {
        Set(thread, PARENT_ENV_INDEX, value);
    }

    JSTaggedValue GetParentEnv(const JSThread *thread) const
    {
        return Get(thread, PARENT_ENV_INDEX);
    }

    JSTaggedValue GetProperties(const JSThread *thread, uint32_t index) const
    {
        return Get(thread, index + RESERVED_ENV_LENGTH);
    }

    void SetProperties(JSThread *thread, uint32_t index, JSTaggedValue value)
    {
        Set(thread, index + RESERVED_ENV_LENGTH, value);
    }

    JSTaggedValue GetScopeInfo(const JSThread *thread) const
    {
        return Get(thread, SCOPE_INFO_INDEX);
    }

    void SetScopeInfo(JSThread *thread, JSTaggedValue value)
    {
        Set(thread, SCOPE_INFO_INDEX, value);
    }

    DECL_DUMP()
};

class SFunctionEnv : public BaseEnv {
public:
    // the first field is used to store GlobalEnv
    static constexpr uint32_t CONSTRUCTOR_INDEX = 1;
    static constexpr uint32_t RESERVED_ENV_LENGTH = 2;

    static SFunctionEnv *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsSFunctionEnv());
        return static_cast<SFunctionEnv *>(object);
    }

    static size_t ComputeSize(uint32_t numSlots)
    {
        return TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), numSlots + RESERVED_ENV_LENGTH);
    }

    void SetConstructor(JSThread *thread, JSTaggedValue value)
    {
        Set(thread, CONSTRUCTOR_INDEX, value);
    }

    JSTaggedValue GetConstructor(const JSThread *thread) const
    {
        return Get(thread, CONSTRUCTOR_INDEX);
    }

    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_LEXICALENV_H
