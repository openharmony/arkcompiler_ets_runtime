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

#ifndef ECMASCRIPT_IC_MEGA_IC_CACHE_H
#define ECMASCRIPT_IC_MEGA_IC_CACHE_H

#include <array>

#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"
#include "jsnapi_expo.h"

namespace panda::ecmascript {
class EcmaVM;
class MegaICCache {
public:
    enum MegaICKind {
        None,
        Load,
        Store,
    };
    inline JSTaggedValue Get(JSHClass *jsHclass, JSTaggedValue key)
    {
        int hash = Hash(jsHclass, key);
        PropertyKey &prop = keys_[hash];
        if ((prop.hclass_ == jsHclass) && (prop.key_ == key)) {
            return keys_[hash].results_;
        }
        return NOT_FOUND;
    }
    void Set(JSHClass *jsHclass, JSTaggedValue key, JSTaggedValue handler, JSThread* thread);
    inline void Clear()
    {
        for (auto &key : keys_) {
            key.hclass_ = nullptr;
            key.key_ = JSTaggedValue::Hole();
        }
    }
    inline bool IsCleared() const
    {
        for (auto &key : keys_) {
            if (key.hclass_ != nullptr) {
                return false;
            }
        }
        return true;
    }

    void Iterate(RootVisitor &v)
    {
        for (auto &key : keys_) {
            if (key.hclass_ != nullptr) {
                auto value = JSTaggedValue::Cast(key.hclass_);
                v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(value))));
            }
            v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(key.key_))));
            v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(key.results_))));
        }
    }
    constexpr static const JSTaggedValue NOT_FOUND = JSTaggedValue::Hole();
    static const uint32_t CACHE_LENGTH_BIT = 10;
    static const uint32_t CACHE_LENGTH = (1U << CACHE_LENGTH_BIT);
    static const uint32_t CACHE_LENGTH_MASK = CACHE_LENGTH - 1;
    static const uint32_t HCLASS_SHIFT = 3;
    struct PropertyKey : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                    base::AlignedPointer,
                                                    JSTaggedValue,
                                                    JSTaggedValue> {
        enum class Index : size_t {
            HclassIndex = 0,
            KeyIndex,
            ResultsIndex,
            NumOfMembers
        };
        static size_t GetHclassOffset(bool isArch32 = false)
        {
            return GetOffset<static_cast<size_t>(Index::HclassIndex)>(isArch32);
        }

        static size_t GetKeyOffset(bool isArch32 = false)
        {
            return GetOffset<static_cast<size_t>(Index::KeyIndex)>(isArch32);
        }

        static size_t GetResultsOffset(bool isArch32 = false)
        {
            return GetOffset<static_cast<size_t>(Index::ResultsIndex)>(isArch32);
        }

        static size_t GetPropertyKeySize()
        {
            return static_cast<size_t>(Index::NumOfMembers) * static_cast<size_t>(JSTaggedValue::TaggedTypeSize());
        }

        static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);
        alignas(EAS) JSHClass *hclass_ {nullptr};
        alignas(EAS) JSTaggedValue key_ {JSTaggedValue::Hole()};
        alignas(EAS) JSTaggedValue results_ {JSTaggedValue::Hole()};
    };

private:
    MegaICCache()
    {
        for (uint32_t i = 0; i < CACHE_LENGTH; ++i) {
            keys_[i].hclass_ = nullptr;
            keys_[i].key_ = JSTaggedValue::Hole();
            keys_[i].results_ = NOT_FOUND;
        }
    }
    ~MegaICCache() = default;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    static inline int Hash(JSHClass *cls, JSTaggedValue key)
    {
        uint32_t clsHash = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cls)) >> HCLASS_SHIFT; // skip 8bytes
        uint32_t keyHash = key.GetStringKeyHashCode();
        uint32_t hash = (clsHash * 31) ^ ((keyHash * 0x9e3779b9) ^ (keyHash >> 16));
        return static_cast<int>((hash) & CACHE_LENGTH_MASK);
    }
    PropertyKey keys_[CACHE_LENGTH];

    friend class JSThread;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_IC_MEGA_IC_CACHE_H
