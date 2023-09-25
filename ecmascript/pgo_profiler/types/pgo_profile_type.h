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

#ifndef ECMASCRIPT_PGO_PROFILER_TYPES_PGO_PROFILE_TYPE_H
#define ECMASCRIPT_PGO_PROFILER_TYPES_PGO_PROFILE_TYPE_H

#include <cstdint>
#include <string>
#include <variant>

#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "libpandabase/utils/bit_field.h"
#include "macros.h"

namespace panda::ecmascript::pgo {
class ProfileTypeRef;
class PGOContext;

using ApEntityId = pgo::ApEntityId;

class ProfileType {
public:
    enum class Kind : uint8_t {
        ClassId,
        LiteralId,
        ElementId,
        BuiltinsId,
        TotalKinds,
        UnknowId,
        LegacyKind = UnknowId
    };

    static constexpr uint32_t ID_BITFIELD_NUM = 29;
    static constexpr uint32_t RECORD_ID_BITFIELD_NUM = 20;
    static constexpr uint32_t KIND_BITFIELD_NUM = 15;
    using IdBits = BitField<uint32_t, 0, ID_BITFIELD_NUM>;
    using IdRecordBits = IdBits::NextField<uint32_t, RECORD_ID_BITFIELD_NUM>;
    using KindBits = IdRecordBits::NextField<Kind, KIND_BITFIELD_NUM>;

    static_assert(KindBits::IsValid(Kind::TotalKinds));

    ProfileType() = default;
    ProfileType(PGOContext &context, ProfileTypeRef typeRef);
    explicit ProfileType(uint32_t type, Kind kind = Kind::ClassId)
    {
        if (UNLIKELY(!IdBits::IsValid(type))) {
            type_ = 0;
        } else {
            UpdateId(type);
            UpdateKind(kind);
        }
    }

    bool IsNone() const
    {
        return type_ == 0;
    }

    uint64_t GetRaw() const
    {
        return type_;
    }

    bool IsBuiltinsType() const
    {
        return GetKind() == Kind::BuiltinsId;
    }

    bool IsClassType() const
    {
        return GetKind() == Kind::ClassId;
    }

    bool IsElementType() const
    {
        return GetKind() == Kind::ElementId;
    }

    uint32_t GetId() const
    {
        return IdBits::Decode(type_);
    }

    Kind GetKind() const
    {
        return KindBits::Decode(type_);
    }

    uint32_t GetRecordId() const
    {
        return IdRecordBits::Decode(type_);
    }

    bool operator<(const ProfileType &right) const
    {
        return type_ < right.type_;
    }

    bool operator!=(const ProfileType &right) const
    {
        return type_ != right.type_;
    }

    bool operator==(const ProfileType &right) const
    {
        return type_ == right.type_;
    }

    std::string GetTypeString() const
    {
        return std::to_string(type_);
    }

    void UpdateRecordId(uint32_t recordId)
    {
        type_ = IdRecordBits::Update(type_, recordId);
    }

private:
    void UpdateId(uint64_t type)
    {
        type_ = IdBits::Update(type_, type);
    }

    void UpdateKind(Kind kind)
    {
        type_ = KindBits::Update(type_, kind);
    }
    uint64_t type_ {0};
};

class ProfileTypeRef {
public:
    ProfileTypeRef() = default;
    explicit ProfileTypeRef(ApEntityId type)
    {
        UpdateId(type);
    }
    ProfileTypeRef(PGOContext &context, const ProfileType &type);

    bool IsNone() const
    {
        return typeId_.GetOffset() == 0;
    }

    ApEntityId GetId() const
    {
        return typeId_;
    }

    bool operator<(const ProfileTypeRef &right) const
    {
        return typeId_ < right.typeId_;
    }

    bool operator==(const ProfileTypeRef &right) const
    {
        return typeId_ == right.typeId_;
    }

    std::string GetTypeString() const
    {
        return std::to_string(typeId_.GetOffset());
    }

    void UpdateId(ApEntityId typeId)
    {
        typeId_ = typeId;
    }

private:
    ApEntityId typeId_ {0};
};

class ProfileTypeLegacy {
public:
    static constexpr uint32_t ID_BITFIELD_NUM = 29;
    static constexpr uint32_t KIND_BITFIELD_NUM = 3;
    using IdBits = BitField<uint32_t, 0, ID_BITFIELD_NUM>;
    using KindBits = IdBits::NextField<ProfileType::Kind, KIND_BITFIELD_NUM>;

    // legacy size check. for version lower than WIDE_CLASS_TYPE_MINI_VERSION, we should consider the legacy scenario.
    static_assert(ID_BITFIELD_NUM == ProfileType::ID_BITFIELD_NUM);
    static_assert(KindBits::IsValid(ProfileType::Kind::LegacyKind));

    explicit ProfileTypeLegacy(uint32_t type, ProfileType::Kind kind = ProfileType::Kind::ClassId)
    {
        if (!IdBits::IsValid(type)) {
            type_ = 0;
        } else {
            UpdateId(type);
            UpdateKind(kind);
        }
    }

    explicit ProfileTypeLegacy(ProfileTypeRef profileTypeRef)
    {
        type_ = profileTypeRef.GetId().GetOffset();
    }

    bool IsNone() const
    {
        return type_ == 0;
    }

    uint32_t GetRaw() const
    {
        return type_;
    }

    uint32_t GetId() const
    {
        return IdBits::Decode(type_);
    }

    ProfileType::Kind GetKind() const
    {
        return KindBits::Decode(type_);
    }

private:
    void UpdateId(uint32_t type)
    {
        type_ = IdBits::Update(type_, type);
    }

    void UpdateKind(ProfileType::Kind kind)
    {
        type_ = KindBits::Update(type_, kind);
    }
    uint32_t type_ {0};
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_TYPES_PGO_PROFILE_TYPE_H
