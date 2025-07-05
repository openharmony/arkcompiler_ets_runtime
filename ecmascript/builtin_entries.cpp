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

#include "ecmascript/builtin_entries.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
size_t BuiltinIndex::GetBuiltinIndex(const JSThread *thread, JSTaggedValue key) const
{
    auto ecmaString = EcmaString::Cast(key.GetTaggedObject());
    auto str = std::string(ConvertToString(thread, ecmaString));
    return GetBuiltinIndex(str);
}

size_t BuiltinIndex::GetBuiltinIndex(const std::string& key) const
{
    auto it = builtinIndex_.find(key);
    if (it == builtinIndex_.end()) {
        return NOT_FOUND;
    } else {
        return static_cast<size_t>(it->second);
    }
}
} // namespace panda::ecmascript
