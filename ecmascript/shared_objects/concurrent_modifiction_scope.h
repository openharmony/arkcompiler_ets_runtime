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

#ifndef ECMASCRIPT_SHARED_OBJECTS_CONCURENT_MODICTION_SCOPE_H
#define ECMASCRIPT_SHARED_OBJECTS_CONCURENT_MODICTION_SCOPE_H

#include "ecmascript/shared_objects/js_shared_set.h"

namespace panda::ecmascript {
enum class ModType : uint8_t {
    READ = 0,
    WRITE = 1
};
template<typename Container, ModType modType = ModType::READ>
class ConcurrentModScope final {
public:
    ConcurrentModScope(JSThread *thread, const TaggedObject *obj)
        : thread_(thread), obj_(obj)
    {
        if constexpr (modType == ModType::READ) {
            CanRead();
        } else {
            CanWrite();
        }
    }

    ~ConcurrentModScope()
    {
        if constexpr (modType == ModType::READ) {
            ReadDone();
        } else {
            WriteDone();
        }
    }

    static constexpr uint32_t WRITE_MOD_MASK = 1 << 31;

private:
    inline uint32_t GetModRecord()
    {
        return reinterpret_cast<volatile std::atomic<uint32_t> *>(
            ToUintPtr(obj_) + Container::MOD_RECORD_OFFET)->load(std::memory_order_acquire);
    }

    inline void CanWrite()
    {
        // Set to ModType::WRITE, expect no writer and readers
        constexpr uint32_t expectedModRecord = 0;
        constexpr uint32_t desiredModRecord = WRITE_MOD_MASK;
        uint32_t ret = Barriers::AtomicSetPrimitive(const_cast<TaggedObject *>(obj_), Container::MOD_RECORD_OFFET,
            expectedModRecord, desiredModRecord);
        if (ret != expectedModRecord) {
            THROW_TYPE_ERROR(thread_, "Concurrent modification exception");
        }
    }

    inline void WriteDone()
    {
        constexpr uint32_t expectedModRecord = WRITE_MOD_MASK;
        constexpr uint32_t desiredModRecord = 0u;
        uint32_t ret = Barriers::AtomicSetPrimitive(const_cast<TaggedObject *>(obj_), Container::MOD_RECORD_OFFET,
            expectedModRecord, desiredModRecord);
        if (ret != expectedModRecord) {
            THROW_TYPE_ERROR(thread_, "Concurrent modification exception");
        }
    }

    inline void CanRead()
    {
        while (true) {
            // Expect no writers
            expectModRecord_ = GetModRecord();
            if ((expectModRecord_ & WRITE_MOD_MASK)) {
                THROW_TYPE_ERROR(thread_, "Concurrent modification exception");
            }
            // Increase readers by 1
            desiredModRecord_ = expectModRecord_ + 1;
            auto ret = Barriers::AtomicSetPrimitive(const_cast<TaggedObject *>(obj_), Container::MOD_RECORD_OFFET,
                expectModRecord_, desiredModRecord_);
            if (ret == expectModRecord_) {
                break;
            }
        }
    }

    inline void ReadDone()
    {
        std::swap(expectModRecord_, desiredModRecord_);
        while (true) {
            auto ret = Barriers::AtomicSetPrimitive(const_cast<TaggedObject *>(obj_), Container::MOD_RECORD_OFFET,
                expectModRecord_, desiredModRecord_);
            if (ret == expectModRecord_) {
                break;
            }
            expectModRecord_ = GetModRecord();
            if ((expectModRecord_ & WRITE_MOD_MASK) ||
                 expectModRecord_ == 0) {
                THROW_TYPE_ERROR(thread_, "Concurrent modification exception");
            }
            // Decrease readers by 1
            desiredModRecord_ = expectModRecord_ - 1;
        }
    }

    JSThread *thread_ {nullptr};
    const TaggedObject *obj_ {nullptr};
    // For readers
    uint32_t expectModRecord_ {0};
    uint32_t desiredModRecord_ {0};

    static_assert(std::is_same_v<Container, JSSharedSet>);
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_SHARED_OBJECTS_CONCURENT_MODICTION_SCOPE_H