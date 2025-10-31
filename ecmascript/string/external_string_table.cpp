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

#include "ecmascript/string/external_string_table.h"

#include "ecmascript/string/external_string.h"


namespace panda::ecmascript {

void ExternalStringTable::AddString(CachedExternalString* string)
{
    std::lock_guard<std::mutex> lock(mutex_);
    externalStringList_.push_back(string);
}

static void SwapBackAndPop(CVector<CachedExternalString*>& vec, CVector<CachedExternalString*>::iterator& iter)
{
    *iter = vec.back();
    if (iter + 1 == vec.end()) {
        vec.pop_back();
        iter = vec.end();
    } else {
        vec.pop_back();
    }
}


static void ShrinkWithFactor(CVector<CachedExternalString*>& vec)
{
    constexpr size_t SHRINK_FACTOR = 2;
    if (vec.size() < vec.capacity() / SHRINK_FACTOR) {
        vec.shrink_to_fit();
    }
}

void ExternalStringTable::ProcessSharedExternalStringDelete(const WeakRootVisitor &visitor)
{
    LOG_GC(DEBUG) << "ExternalStringTable: ProcessSharedExternalStringDelete size=" << externalStringList_.size();
    auto sharedIter = externalStringList_.begin();
    while (sharedIter != externalStringList_.end()) {
        CachedExternalString* object = *sharedIter;
        auto fwd = visitor(reinterpret_cast<TaggedObject*>(object));
        if (fwd == nullptr) {
            ExternalNonMovableStringResource::FreeResource(nullptr,
                object->GetResource(), object->GetCachedResourceData());
            SwapBackAndPop(externalStringList_, sharedIter);
        } else {
            if (fwd != reinterpret_cast<TaggedObject*>(object)) {
                *sharedIter = reinterpret_cast<CachedExternalString*>(fwd);
            }
            ++sharedIter;
        }
    }
    ShrinkWithFactor(externalStringList_);
    LOG_GC(DEBUG) << "ExternalStringTable: ProcessSharedExternalStringDelete size=" << externalStringList_.size();
}

}
