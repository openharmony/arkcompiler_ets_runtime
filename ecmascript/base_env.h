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

#ifndef ECMASCRIPT_BASE_ENV_H
#define ECMASCRIPT_BASE_ENV_H

#include "ecmascript/js_object.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
class BaseEnv : public TaggedArray {
public:
    static constexpr uint32_t GLOBAL_ENV_INDEX = 0;

    static BaseEnv* Cast(TaggedObject *obj)
    {
        ASSERT(JSTaggedValue(obj).IsLexicalEnv() || JSTaggedValue(obj).IsJSGlobalEnv() ||
               JSTaggedValue(obj).IsSFunctionEnv());
        return static_cast<BaseEnv*>(obj);
    }

    JSTaggedValue GetGlobalEnv() const
    {
        return Get(GLOBAL_ENV_INDEX);
    }

    void SetGlobalEnv(JSThread* thread, JSTaggedValue value)
    {
        Set(thread, GLOBAL_ENV_INDEX, value);
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_BASE_ENV_H
