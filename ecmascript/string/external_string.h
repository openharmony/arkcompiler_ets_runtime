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

// NOLINTBEGIN(cppcoreguidelines-special-member-functions)

#ifndef ECMASCRIPT_CACHED_EXTERNAL_STRING_H
#define ECMASCRIPT_CACHED_EXTERNAL_STRING_H

#include "ecmascript/string/base_string.h"

namespace panda::ecmascript {

using ExternalStringResourceCallback = void (*)(void* data, void* hint);

class ExternalNonMovableStringResource {
public:
    ExternalNonMovableStringResource(void* hint, ExternalStringResourceCallback callback)
        : hint_(hint), callback_(callback) {}

    static void FreeResource([[maybe_unused]] void* env, void* rec, void* data)
    {
        auto* resource = reinterpret_cast<ExternalNonMovableStringResource*>(rec);
        if (resource->callback_ != nullptr) {
            std::invoke(resource->callback_, data, resource->hint_);
        }
        delete resource;
    }

private:
    void* hint_;
    ExternalStringResourceCallback callback_;
};

/*
 +--------------------------------------------+ <-- offset 0
 |           BaseObject fields                |
 +--------------------------------------------+ <-- offset = BaseObjectSize()
 | Padding (uint64_t)                         |
 +--------------------------------------------+
 | LengthAndFlags (uint32_t)                  |
 +--------------------------------------------+
 | RawHashcode (uint32_t)                     |
 +--------------------------------------------+ <-- offset = BaseString::SIZE
 | Resource (ExternalNonMovableStringResource)| <-- RESOURCE_OFFSET
 +--------------------------------------------+
 | CachedResourceData (uint8_t/uint16_t)*     | <-- CACHED_RESOURCE_DATA_OFFSET
 +--------------------------------------------+ <-- SIZE
 */
// The CachedExternalString abstract class captures external string values,
// and the external string data should be immutable and non-movable.

/**
 * @class CacheExternalString
 * @brief CacheExternalString represents the wrapper for external data.
 *
 * ExternalStringResource should be immutable and non-movable, so we can simply cache the data pointer.
 */
class CachedExternalString : public BaseString {
public:
    STRING_CAST_CHECK(CachedExternalString, IsCachedExternalString);
    NO_MOVE_SEMANTIC_CC(CachedExternalString);
    NO_COPY_SEMANTIC_CC(CachedExternalString);

    static constexpr size_t RESOURCE_OFFSET = BaseString::SIZE;
    
    PRIMITIVE_FIELD(Resource, ExternalNonMovableStringResource*, RESOURCE_OFFSET, CACHE_RESOURCE_DATA_OFFSET)
    PRIMITIVE_FIELD(CachedResourceData, void*, CACHE_RESOURCE_DATA_OFFSET, SIZE)

    /**
     * @brief Retrieve string's raw UTF-8 data.
     * @return Pointer to UTF-8 buffer.
     */
    const uint8_t *GetDataUtf8() const;

    /**
     * @brief Retrieve string's raw UTF-16 data.
     * @return Pointer to UTF-16 buffer.
     */
    const uint16_t *GetDataUtf16() const;

    /**
     * @brief Get character at specific index.
     * @tparam VERIFY Whether bounds checking should be performed.
     * @param index Index into the character buffer.
     * @return UTF-16 code unit at the given index.
     */
    template <bool VERIFY = true>
    uint16_t Get(int32_t index) const;

    /**
     * @brief Create a CachedExternalString instance.
     * @tparam Allocator Callable allocator.
     * @param allocator Allocator object.
     * @param length String length in code units.
     * @param compressed Whether to use UTF-8 compression.
     * @return CachedExternalString pointer.
     */
    template <typename Allocator, panda::objects_traits::enable_if_is_allocate<Allocator, BaseObject *> = 0>
    static CachedExternalString *Create(Allocator &&allocator, ExternalNonMovableStringResource* resource,
                                        void* data, size_t length, bool compressed);
};

inline const uint8_t *CachedExternalString::GetDataUtf8() const
{
    DCHECK_CC(IsUtf8() && "BaseString: Read data as utf8 for utf16 string");
    return reinterpret_cast<uint8_t *>(GetCachedResourceData());
}

inline const uint16_t *CachedExternalString::GetDataUtf16() const
{
    DCHECK_CC(IsUtf16() && "BaseString: Read data as utf16 for utf8 string");
    return reinterpret_cast<uint16_t *>(GetCachedResourceData());
}
}
#endif // ECMASCRIPT_CACHED_EXTERNAL_STRING_H
