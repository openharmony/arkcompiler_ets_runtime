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

#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"
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
        LegacyKind = BuiltinsId,
        MethodId,           // method offset of js function
        BuiltinFunctionId,  // function index of registered function
        LocalRecordId,
        ModuleRecordId,
        TotalKinds,
        UnknowId
    };

    static const ProfileType PROFILE_TYPE_NONE;

    static constexpr uint32_t ID_BITFIELD_NUM = 29;
    static constexpr uint32_t ABC_ID_BITFIELD_NUM = 20;
    static constexpr uint32_t KIND_BITFIELD_NUM = 15;
    using IdBits = BitField<uint32_t, 0, ID_BITFIELD_NUM>;
    using AbcIdBits = IdBits::NextField<uint32_t, ABC_ID_BITFIELD_NUM>;
    using KindBits = AbcIdBits::NextField<Kind, KIND_BITFIELD_NUM>;

    static_assert(KindBits::IsValid(Kind::TotalKinds));

    ProfileType() = default;
    explicit ProfileType(uint64_t rawType) : type_(rawType) {};
    ProfileType(PGOContext &context, ProfileTypeRef typeRef);
    ProfileType(ApEntityId abcId, uint32_t type, Kind kind = Kind::ClassId)
    {
        if (UNLIKELY(!IdBits::IsValid(type))) {
            type_ = 0;
        } else {
            UpdateAbcId(abcId);
            UpdateId(type);
            UpdateKind(kind);
        }
    }

    ProfileType &Remap(const PGOContext &context);

    bool IsNone() const
    {
        return type_ == PROFILE_TYPE_NONE.type_;
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

    bool IsMethodId() const
    {
        return GetKind() == Kind::MethodId;
    }

    bool IsBuiltinFunctionId() const
    {
        return GetKind() == Kind::BuiltinFunctionId;
    }

    uint32_t GetId() const
    {
        return IdBits::Decode(type_);
    }

    Kind GetKind() const
    {
        return KindBits::Decode(type_);
    }

    ApEntityId GetAbcId() const
    {
        return AbcIdBits::Decode(type_);
    }

    void UpdateAbcId(ApEntityId abcId)
    {
        type_ = AbcIdBits::Update(type_, abcId);
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
        std::stringstream stream;
        stream << "type: " << std::showbase << std::hex << type_ <<
                "(kind: " << std::showbase << std::dec << static_cast<uint32_t>(GetKind()) <<
                ", abcId: " << GetAbcId() <<
                ", id: " << GetId() << ")";
        return stream.str();
    }

    void UpdateId(uint32_t id)
    {
        type_ = IdBits::Update(type_, id);
    }

    void UpdateKind(Kind kind)
    {
        type_ = KindBits::Update(type_, kind);
    }

private:
    void UpdateId(uint64_t type)
    {
        type_ = IdBits::Update(type_, type);
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

    ProfileTypeRef &Remap(const PGOContext &context);

    bool IsNone() const
    {
        return typeId_ == 0;
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
        return std::to_string(typeId_);
    }

    void UpdateId(ApEntityId typeId)
    {
        typeId_ = typeId;
    }

private:
    ApEntityId typeId_ {0};
};
static_assert(sizeof(ProfileTypeRef) == sizeof(uint32_t));

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

    explicit ProfileTypeLegacy(ProfileTypeRef profileTypeRef) : type_(profileTypeRef.GetId()) {}

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
