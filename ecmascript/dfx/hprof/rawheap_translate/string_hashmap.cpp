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

#include "string_hashmap.h"

namespace rawheap_translate {
std::string StringHashMap::GetStringByKey(StringKey key) const
{
    auto it = hashmap_.find(key);
    if (it != hashmap_.end()) {
        return it->second;
    }
    return "";
}

StringKey StringHashMap::GetKeyByStringId(StringId stringId) const
{
    // Bounds check: stringId must be >= CUSTOM_STRID_START and within the
    // inserted range. Out-of-range ids (e.g. a node whose strId was never
    // assigned) return 0, which GetStringByKey treats as "not found".
    if (stringId < CUSTOM_STRID_START ||
        static_cast<size_t>(stringId - CUSTOM_STRID_START) >= orderedKey_.size()) {
        return 0;
    }
    return orderedKey_[stringId - CUSTOM_STRID_START]; // 3: index_ start from 3
}

StringId StringHashMap::InsertStrAndGetStringId(const std::string &str)
{
    StringKey key = GenerateStringKey(str);
    auto it = indexMap_.find(key);
    if (it != indexMap_.end()) {
        return it->second;
    } else {  // NOLINT(readability-else-after-return)
        index_++;
        hashmap_.emplace(key, str);
        orderedKey_.emplace_back(key);
        indexMap_.emplace(key, index_);
        return index_;
    }
}

StringKey StringHashMap::GenerateStringKey(const std::string &str) const
{
    return std::hash<std::string>{} (str);
}
}  // namespace rawheap_translate
