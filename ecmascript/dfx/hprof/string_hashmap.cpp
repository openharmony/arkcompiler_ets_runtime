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

#include "ecmascript/dfx/hprof/string_hashmap.h"

#include "ecmascript/mem/native_area_allocator.h"

namespace panda::ecmascript {
CString *StringHashMap::FindOrInsertString(const CString *cstr)
{
    StringKey key = GenerateStringKey(cstr);
    auto it = hashmap_.find(key);
    if (it != hashmap_.end()) {
        return it->second;
    } else {  // NOLINT(readability-else-after-return)
        index_++;
        auto *newStr = const_cast<NativeAreaAllocator *>(
            vm_->GetNativeAreaAllocator())->New<CString>(cstr->c_str());
        hashmap_.emplace(key, newStr);
        orderedKey_.emplace_back(key);
        indexMap_.emplace(key, index_);
        return newStr;
    }
}

StringId StringHashMap::GetStringId(const CString *cstr) const
{
    auto it = indexMap_.find(GenerateStringKey(cstr));
    return it != indexMap_.end() ? it->second : 1;  // ""
}

CString *StringHashMap::GetStringByKey(StringKey key) const
{
    auto it = hashmap_.find(key);
    if (it != hashmap_.end()) {
        return it->second;
    }
    return nullptr;
}

std::pair<uint64_t, CString *> StringHashMap::GetStringAndIdPair(StringKey key) const
{
    auto it = hashmap_.find(key);
    if (it != hashmap_.end()) {
        return {indexMap_.at(key), it->second};
    }
    return {1, nullptr};  // 1 : invalid id
}

StringId StringHashMap::InsertStrAndGetStringId(const CString &cstrArg)
{
    CString *cstr = const_cast<CString *>(&cstrArg);
    StringKey key = GenerateStringKey(cstr);
    auto it = indexMap_.find(key);
    if (it != indexMap_.end()) {
        return it->second;
    } else {  // NOLINT(readability-else-after-return)
        index_++;
        auto *newStr = const_cast<NativeAreaAllocator *>(
            vm_->GetNativeAreaAllocator())->New<CString>(cstr->c_str());
        hashmap_.emplace(key, newStr);
        orderedKey_.emplace_back(key);
        indexMap_.emplace(key, index_);
        return index_;
    }
}

StringKey StringHashMap::GenerateStringKey(const CString *cstr) const
{
    return std::hash<CString>{} (*cstr);
}

CString *StringHashMap::GetString(const CString &cstr)
{
    return FindOrInsertString(&cstr);
}

void StringHashMap::Clear()
{
    auto *alloctor = const_cast<NativeAreaAllocator *>(vm_->GetNativeAreaAllocator());
    for (auto &[_, cstr] : hashmap_) {
        if (cstr != nullptr) {
            alloctor->Delete(cstr);
        }
    }
}
}  // namespace panda::ecmascript
