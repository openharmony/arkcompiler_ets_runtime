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

#ifndef RAWHEAP_TRANSLATE_STRING_HASHMAP_H
#define RAWHEAP_TRANSLATE_STRING_HASHMAP_H

#include "ecmascript/dfx/hprof/rawheap_translate/utils.h"

namespace rawheap_translate {
using StringKey = uint32_t;
using StringId = uint32_t;

// An Implementation for Native StringTable without Auto Mem-Management
// To make sure when using String, it still stays where it was.
class StringHashMap {
public:
    explicit StringHashMap()
    {
    }

    ~StringHashMap()
    {
    }

    /*
     * Get all keys sorted by insert order
     */
    const std::vector<StringKey> &GetOrderedKeyStorage() const
    {
        return orderedKey_;
    }
    /*
     * Get string by its hash key
     */
    std::string GetStringByKey(StringKey key) const;
    StringId InsertStrAndGetStringId(const std::string &cstrArg);
    size_t GetCapcity() const
    {
        return orderedKey_.size();
    }

private:
    StringKey GenerateStringKey(const std::string &cstr) const;
    std::vector<StringKey> orderedKey_;  // Used for Serialize Order
    size_t index_ {2};  // 2: Offset the String-Table Header
    std::unordered_map<StringKey, StringId> indexMap_;
    std::unordered_map<StringKey, std::string> hashmap_;
};
}  // namespace rawheap_translate
#endif  // RAWHEAP_TRANSLATE_STRING_HASHMAP_H
