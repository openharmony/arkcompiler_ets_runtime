/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PGO_PROFILER_TYPES_PGO_TYPE_GENERATOR_H
#define ECMASCRIPT_PGO_PROFILER_TYPES_PGO_TYPE_GENERATOR_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/platform/mutex.h"
#include "utils/span.h"

namespace panda::ecmascript::pgo {
class PGOTypeGenerator {
public:
    ProfileType GenerateProfileType(ProfileType rootType, JSTaggedType hclass)
    {
        {
            LockHolder lock(mutex_);
            auto iter = exsitId_.find(hclass);
            if (iter != exsitId_.end()) {
                return iter->second;
            }
        }

        CString result = JSHClass::DumpToString(hclass);
        uint32_t traceId = ComputeHashCode(result);
        ProfileType type(rootType.GetAbcId(), traceId, rootType.GetKind());
        LockHolder lock(mutex_);
        exsitId_.emplace(hclass, type);
        return type;
    }

    ProfileType GetProfileType(JSTaggedType hclass)
    {
        LockHolder lock(mutex_);
        auto iter = exsitId_.find(hclass);
        if (iter != exsitId_.end()) {
            return iter->second;
        }
        return ProfileType::PROFILE_TYPE_NONE;
    }

    bool InsertProfileType(JSTaggedType hclass, ProfileType type)
    {
        LockHolder lock(mutex_);
        auto iter = exsitId_.find(hclass);
        if (iter != exsitId_.end()) {
            return false;
        }
        exsitId_.emplace(hclass, type);
        return true;
    }

    void UpdateProfileType(JSTaggedType oldHClass, JSTaggedType newHClass)
    {
        LockHolder lock(mutex_);
        auto iter = exsitId_.find(oldHClass);
        if (iter != exsitId_.end()) {
            auto profileType = iter->second;
            exsitId_.erase(oldHClass);
            exsitId_.emplace(newHClass, profileType);
        }
    }

    void ProcessReferences(const WeakRootVisitor &visitor)
    {
        for (auto iter = exsitId_.begin(); iter != exsitId_.end();) {
            JSTaggedType object = iter->first;
            auto fwd = visitor(reinterpret_cast<TaggedObject *>(object));
            if (fwd == nullptr) {
                iter = exsitId_.erase(iter);
                continue;
            }
            if (fwd != reinterpret_cast<TaggedObject *>(object)) {
                UNREACHABLE();
            }
            ++iter;
        }
    }

private:
    static constexpr uint32_t INVALID_ID = 0;

    uint32_t ComputeHashCode(const CString &string)
    {
        uint32_t hash = INVALID_ID;
        Span<const char> sp(string.c_str(), string.size());
        for (auto c : sp) {
            constexpr size_t SHIFT = 5;
            hash = (hash << SHIFT) - hash + c;
        }
        return hash;
    }

    Mutex mutex_;
    CMap<JSTaggedType, ProfileType> exsitId_;
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_TYPES_PGO_TYPE_GENERATOR_H
