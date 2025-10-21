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

#ifndef ECMASCRIPT_STRING_BASE_STRING_CLASS_H
#define ECMASCRIPT_STRING_BASE_STRING_CLASS_H
#include <cstdint>
#include "base/bit_field.h"
#include "common_interfaces/objects/base_class.h"

namespace panda::ecmascript {

enum class EcmaStringType : uint8_t {
    INVALID = 0,
    FIRST_OBJECT_TYPE,

    LINE_STRING = FIRST_OBJECT_TYPE,
    SLICED_STRING,
    TREE_STRING,
    CACHED_EXTERNAL_STRING,

    LAST_OBJECT_TYPE = CACHED_EXTERNAL_STRING,

    STRING_FIRST = LINE_STRING,
    STRING_LAST = CACHED_EXTERNAL_STRING,
};

class BaseStringClass : public common::BaseClass {
public:
    BaseStringClass() = delete;
    NO_MOVE_SEMANTIC_CC(BaseStringClass);
    NO_COPY_SEMANTIC_CC(BaseStringClass);

    using HeaderType = uint64_t;

    static constexpr size_t TYPE_BITFIELD_NUM = common::BITS_PER_BYTE * sizeof(EcmaStringType);
    using ObjectTypeBits = common::BitField<EcmaStringType, 0, TYPE_BITFIELD_NUM>; // 8

    void SetEcmaStringType(EcmaStringType type)
    {
        bitfield_ = ObjectTypeBits::Update(bitfield_, type);
    }

    EcmaStringType GetEcmaStringType() const
    {
        return ObjectTypeBits::Decode(bitfield_);
    }

    bool IsString() const
    {
        return GetEcmaStringType() >= EcmaStringType::STRING_FIRST &&
            GetEcmaStringType() <= EcmaStringType::STRING_LAST;
    }

    bool IsLineString() const
    {
        return GetEcmaStringType() == EcmaStringType::LINE_STRING;
    }

    bool IsSlicedString() const
    {
        return GetEcmaStringType() == EcmaStringType::SLICED_STRING;
    }

    bool IsTreeString() const
    {
        return GetEcmaStringType() == EcmaStringType::TREE_STRING;
    }

    bool IsCachedExternalString() const
    {
        return GetEcmaStringType() == EcmaStringType::CACHED_EXTERNAL_STRING;
    }
};
}  // namespace panda::ecmascript
#endif //ECMASCRIPT_STRING_BASE_STRING_CLASS_H
