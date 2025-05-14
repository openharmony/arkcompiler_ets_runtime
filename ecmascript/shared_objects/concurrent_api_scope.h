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

#ifndef ECMASCRIPT_SHARED_OBJECTS_CONCURRENT_API_SCOPE_H
#define ECMASCRIPT_SHARED_OBJECTS_CONCURRENT_API_SCOPE_H

#include "ecmascript/js_object.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/shared_objects/js_shared_typed_array.h"

#include "ecmascript/containers/containers_errors.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
enum class ModType : uint8_t {
    READ = 0,
    WRITE = 1
};
template<typename Container, ModType modType = ModType::READ>
class ConcurrentApiScope final {
public:
    ConcurrentApiScope(JSThread *thread, const JSHandle<JSTaggedValue> &objHandle, SCheckMode mode = SCheckMode::CHECK)
        : thread_(thread), objHandle_(objHandle), checkMode_(mode)
    {
        if (checkMode_ == SCheckMode::SKIP) {
            return;
        }
        if constexpr (modType == ModType::READ) {
            CanRead();
        } else {
            CanWrite();
        }
    }

    ~ConcurrentApiScope()
    {
        if (checkMode_ == SCheckMode::SKIP) {
            return;
        }
        if constexpr (modType == ModType::READ) {
            ReadDone();
        } else {
            WriteDone();
        }
    }

    static constexpr uint32_t WRITE_MOD_MASK = 1 << 31;

private:
    NO_COPY_SEMANTIC(ConcurrentApiScope);
    NO_MOVE_SEMANTIC(ConcurrentApiScope);
    inline uint32_t GetModRecord()
    {
        return reinterpret_cast<volatile std::atomic<uint32_t> *>(
            ToUintPtr(objHandle_->GetTaggedObject()) +
            Container::MOD_RECORD_OFFSET)->load(std::memory_order_acquire);
    }

    inline void CanWrite()
    {
        // Set to ModType::WRITE, expect no writers and readers
        constexpr uint32_t expectedModRecord = 0;
        constexpr uint32_t desiredModRecord = WRITE_MOD_MASK;
        uint32_t ret = Barriers::AtomicSetPrimitive(objHandle_->GetTaggedObject(),
            Container::MOD_RECORD_OFFSET, expectedModRecord, desiredModRecord);
        if (ret != expectedModRecord) {
            auto error = containers::ContainerError::BusinessError(
                thread_, containers::ErrorFlag::CONCURRENT_MODIFICATION_ERROR, "Concurrent modification exception");
            THROW_NEW_ERROR_AND_RETURN(thread_, error);
        }
    }

    inline void WriteDone()
    {
        constexpr uint32_t expectedModRecord = WRITE_MOD_MASK;
        constexpr uint32_t desiredModRecord = 0u;
        uint32_t ret = Barriers::AtomicSetPrimitive(objHandle_->GetTaggedObject(),
            Container::MOD_RECORD_OFFSET, expectedModRecord, desiredModRecord);
        if (ret != expectedModRecord) {
            auto error = containers::ContainerError::BusinessError(
                thread_, containers::ErrorFlag::CONCURRENT_MODIFICATION_ERROR, "Concurrent modification exception");
            THROW_NEW_ERROR_AND_RETURN(thread_, error);
        }
    }

    inline void CanRead()
    {
        while (true) {
            // Expect no writers
            expectModRecord_ = GetModRecord();
            if ((expectModRecord_ & WRITE_MOD_MASK)) {
                auto error = containers::ContainerError::BusinessError(
                    thread_, containers::ErrorFlag::CONCURRENT_MODIFICATION_ERROR, "Concurrent modification exception");
                THROW_NEW_ERROR_AND_RETURN(thread_, error);
            }
            // Increase readers by 1
            desiredModRecord_ = expectModRecord_ + 1;
            auto ret = Barriers::AtomicSetPrimitive(objHandle_->GetTaggedObject(),
                Container::MOD_RECORD_OFFSET, expectModRecord_, desiredModRecord_);
            if (ret == expectModRecord_) {
                break;
            }
        }
    }

    inline void ReadDone()
    {
        std::swap(expectModRecord_, desiredModRecord_);
        while (true) {
            auto ret = Barriers::AtomicSetPrimitive(objHandle_->GetTaggedObject(),
                Container::MOD_RECORD_OFFSET, expectModRecord_, desiredModRecord_);
            if (ret == expectModRecord_) {
                break;
            }
            expectModRecord_ = GetModRecord();
            if ((expectModRecord_ & WRITE_MOD_MASK) ||
                 expectModRecord_ == 0) {
                auto error = containers::ContainerError::BusinessError(
                    thread_, containers::ErrorFlag::CONCURRENT_MODIFICATION_ERROR, "Concurrent modification exception");
                THROW_NEW_ERROR_AND_RETURN(thread_, error);
            }
            // Decrease readers by 1
            desiredModRecord_ = expectModRecord_ - 1;
        }
    }

    JSThread *thread_ {nullptr};
    JSHandle<JSTaggedValue> objHandle_;
    SCheckMode checkMode_ { SCheckMode::CHECK };
    // For readers
    uint32_t expectModRecord_ {0};
    uint32_t desiredModRecord_ {0};

    static_assert(std::is_same_v<Container, JSSharedSet> || std::is_same_v<Container, JSSharedMap> ||
                  std::is_same_v<Container, JSSharedArray> || std::is_same_v<Container, JSSharedTypedArray> ||
                  std::is_same_v<Container, JSAPIBitVector>);
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_SHARED_OBJECTS_CONCURRENT_API_SCOPE_H