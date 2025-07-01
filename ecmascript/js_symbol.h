/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JSSYMBOL_H
#define ECMASCRIPT_JSSYMBOL_H

#include "ecmascript/base/number_helper.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_object.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"

namespace panda {
namespace ecmascript {
using ProfileType = pgo::ProfileType;

class JSSymbol : public TaggedObject {
public:
    static constexpr uint32_t IS_PRIVATE = 1U << 0U;
    static constexpr uint32_t IS_WELL_KNOWN_SYMBOL = 1U << 1U;
    static constexpr uint32_t IS_IN_PUBLIC_SYMBOL_TABLE = 1U << 2U;
    static constexpr uint32_t IS_INTERESTING_SYMBOL = 1U << 3U;
    static constexpr uint32_t IS_PRIVATE_NAME = 1U << 4U;
    static constexpr uint32_t IS_PRIVATE_BRAND = 1U << 5U;
    static constexpr uint32_t HAS_ID = 1U << 6U;

    static constexpr int SYMBOL_HAS_INSTANCE_TYPE = 0;
    static constexpr int SYMBOL_TO_PRIMITIVE_TYPE = 1;
    static constexpr int SYMBOL_DEFAULT_TYPE = 2;

    static constexpr const uint32_t LINEAR_X = 1103515245U;
    static constexpr const uint32_t LINEAR_Y = 12345U;
    static constexpr const uint32_t LINEAR_SEED = 987654321U;

    // 48: high 16 bits need to be double encoded as a valid number tagged value
    static constexpr size_t SYMBOL_ID_BITFIELD_NUM = 48;
    static constexpr size_t ABC_ID_OFFSET_BIT = SYMBOL_ID_BITFIELD_NUM - ProfileType::ABC_ID_BITFIELD_NUM;
    static constexpr size_t LITERAL_ID_BITFIELD_NUM = 16;
    static constexpr size_t LITERAL_ID_OFFSET_BIT = ABC_ID_OFFSET_BIT - LITERAL_ID_BITFIELD_NUM;

public:
    CAST_CHECK(JSSymbol, IsSymbol);

    static inline uint32_t ComputeHash()
    {
        return static_cast<uint32_t>(base::RandomGenerator::GenerateIdentityHash());
    }

    bool HasId() const
    {
        return (GetFlags() & HAS_ID) != 0U;
    }

    void SetHasId()
    {
        SetFlags(GetFlags() | HAS_ID);
    }

    bool IsPrivate() const
    {
        return (GetFlags() & IS_PRIVATE) != 0U;
    }

    void SetPrivate()
    {
        SetFlags(GetFlags() | IS_PRIVATE);
    }

    bool IsWellKnownSymbol() const
    {
        return (GetFlags() & IS_WELL_KNOWN_SYMBOL) != 0U;
    }

    void SetWellKnownSymbol()
    {
        SetFlags(GetFlags() | IS_WELL_KNOWN_SYMBOL);
    }

    bool IsInPublicSymbolTable() const
    {
        return (GetFlags() & IS_IN_PUBLIC_SYMBOL_TABLE) != 0U;
    }

    void SetInPublicSymbolTable()
    {
        SetFlags(GetFlags() | IS_IN_PUBLIC_SYMBOL_TABLE);
    }

    bool IsInterestingSymbol() const
    {
        return (GetFlags() & IS_INTERESTING_SYMBOL) != 0U;
    }

    void SetInterestingSymbol()
    {
        SetFlags(GetFlags() | IS_INTERESTING_SYMBOL);
    }

    bool IsPrivateNameSymbol() const
    {
        return (GetFlags() & IS_PRIVATE_NAME) != 0U;
    }

    void SetPrivateNameSymbol()
    {
        SetFlags(GetFlags() | IS_PRIVATE_NAME);
    }

    static bool Equal(JSThread *thread, const JSSymbol &src, const JSSymbol &dst)
    {
        if (src.GetFlags() != dst.GetFlags()) {
            return false;
        }
        EcmaString *srcString = EcmaString::Cast(src.GetDescription(thread).GetTaggedObject());
        EcmaString *dstString = EcmaString::Cast(dst.GetDescription(thread).GetTaggedObject());
        return EcmaStringAccessor::StringsAreEqual(thread, srcString, dstString);
    }

    uint64_t GetPrivateId()
    {
        return GetId();
    }

    void SetPrivateId(uint64_t id)
    {
        SetHasId();
        SetId(id);
    }

    static uint64_t GeneratePrivateId(uint64_t abcId, uint64_t literalId, uint64_t slotIndex)
    {
        return JSTaggedValueInternals::DOUBLE_ENCODE_OFFSET | (abcId << ABC_ID_OFFSET_BIT) |
               (literalId << LITERAL_ID_OFFSET_BIT) | slotIndex;
    }

    static uint64_t GetSlotIndex(uint64_t id)
    {
        uint64_t mask = (1ULL << LITERAL_ID_OFFSET_BIT) - 1;
        return id & mask;
    }

    static uint64_t GetLiteralId(uint64_t id)
    {
        uint64_t mask = (1ULL << LITERAL_ID_BITFIELD_NUM) - 1;
        return (id >> LITERAL_ID_OFFSET_BIT) & mask;
    }

    static uint64_t GetAbcId(uint64_t id)
    {
        uint64_t mask = (1ULL << ProfileType::ABC_ID_BITFIELD_NUM) - 1;
        return (id >> ABC_ID_OFFSET_BIT) & mask;
    }

public:
    static constexpr size_t DESCRIPTION_OFFSET = TaggedObjectSize();
    ACCESSORS(Description, DESCRIPTION_OFFSET, HASHFIELD_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(HashField, uint32_t, HASHFIELD_OFFSET, FLAGS_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(Flags, uint32_t, FLAGS_OFFSET, ID_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(Id, uint64_t, ID_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_DUMP()

    DECL_VISIT_OBJECT(DESCRIPTION_OFFSET, HASHFIELD_OFFSET)
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_NAME_H
