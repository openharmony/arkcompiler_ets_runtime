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
#include <optional>
#include <string>
#include <variant>

#include "ecmascript/builtin_entries.h"
#include "ecmascript/elements.h"
#include "ecmascript/global_index.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/on_heap.h"
#include "libpandabase/utils/bit_field.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::pgo {
class ProfileTypeRef;
class PGOContext;

using ApEntityId = pgo::ApEntityId;

class ProfileType {
public:
    enum class Kind : uint8_t {
        ClassId,
        ObjectLiteralId,
        ArrayLiteralId,
        BuiltinsId,
        LegacyKind = BuiltinsId,
        MethodId,           // method offset of js function
        BuiltinFunctionId,  // function index of registered function
        RecordClassId,
        PrototypeId,
        ConstructorId,
        MegaStateKinds,
        TotalKinds,
        UnknowId,
        GlobalsId,
        JITClassId,
        TransitionClassId,      // function class id after set prototype
        TransitionPrototypeId,   // function prototype id after set prototype
        NapiId,
        InvalidId,
    };

    static constexpr uint32_t RECORD_ID_FOR_BUNDLE = 1;
    static constexpr uint32_t HUGE_ABC_ID = 500;

    static PUBLIC_API const ProfileType PROFILE_TYPE_NONE;

    static constexpr uint32_t ID_BITFIELD_NUM = 32;  // 0-31
    static constexpr uint32_t ABC_ID_BITFIELD_NUM = 10; // 32-41
    static constexpr uint32_t KIND_BITFIELD_NUM = 8;  // 42 - 49
    using IdBits = BitField<uint32_t, 0, ID_BITFIELD_NUM>;
    using AbcIdBits = IdBits::NextField<uint32_t, ABC_ID_BITFIELD_NUM>;
    using KindBits = AbcIdBits::NextField<Kind, KIND_BITFIELD_NUM>;
    using IsRootBits = KindBits::NextFlag;  // 50
    using EverOutOfBoundsBits = IsRootBits::NextFlag; // 51

    using StringMap = std::multimap<std::string, std::string>;
    using MapVector = std::vector<std::vector<StringMap>>;
    using VariantVector = std::vector<std::variant<StringMap, MapVector, std::vector<StringMap>>>;
    using VariantMap = std::multimap<std::string, std::variant<std::string, VariantVector>>;
    using jModuleType = std::multimap<std::string, std::variant<std::string, std::vector<VariantMap>>>;

    class BuiltinsId {
    public:
        static constexpr uint32_t BUILTINS_ID_NUM = 16;
        using BuiltinsIdBits = BitField<JSType, 0, BUILTINS_ID_NUM>;

        explicit BuiltinsId() = default;
        explicit BuiltinsId(uint32_t id) : id_(id) {}

        uint32_t GetId() const
        {
            return id_;
        }

        BuiltinsId SetBuiltinsId(JSType type)
        {
            id_ = BuiltinsIdBits::Update(id_, type);
            return *this;
        }

        JSType GetBuiltinsId() const
        {
            return BuiltinsIdBits::Decode(id_);
        }

        virtual std::string GetIdToString() const
        {
            std::stringstream ss;
            ss << "(";
            ss << GetId();
            ss << ")";
            return ss.str();
        }

    protected:
        uint32_t id_ { 0 };
    };

    class BuiltinsArrayId : public BuiltinsId {
    public:
        // BuilitinsArray second bit field
        static constexpr uint32_t OLD_ELEMENTS_KIND_BITFIELD_NUM = 5;
        static constexpr uint32_t NEW_ELEMENTS_KIND_BITFIELD_NUM = 5;
        using OldElementsKindBits = BuiltinsIdBits::NextField<ElementsKind, OLD_ELEMENTS_KIND_BITFIELD_NUM>;
        using NewElementsKindBits = OldElementsKindBits::NextField<ElementsKind, NEW_ELEMENTS_KIND_BITFIELD_NUM>;

        explicit BuiltinsArrayId() = default;
        explicit BuiltinsArrayId(uint32_t id) : BuiltinsId(id) {}

        BuiltinsArrayId UpdateElementsKind(ElementsKind kind)
        {
            id_ = OldElementsKindBits::Update(id_, kind);
            return *this;
        }

        ElementsKind GetElementsKind() const
        {
            return OldElementsKindBits::Decode(id_);
        }

        BuiltinsArrayId UpdateTransitionElementsKind(ElementsKind kind)
        {
            id_ = NewElementsKindBits::Update(id_, kind);
            return *this;
        }

        ElementsKind GetTransitionElementsKind() const
        {
            return NewElementsKindBits::Decode(id_);
        }

        std::string GetIdToString() const override
        {
            std::stringstream ss;
            ss << "(";
            ss << static_cast<uint32_t>(BuiltinsId::GetBuiltinsId()) << ", ";
            ss << static_cast<uint32_t>(GetElementsKind()) << ", ";
            ss << static_cast<uint32_t>(GetTransitionElementsKind());
            ss << ")";
            return ss.str();
        }
    };

    class BuiltinsTypedArrayId : public BuiltinsId {
    public:
        // BuilitinsTypedArray second bit field
        static constexpr uint8_t ON_HEAP_MODE_BITFIELD_NUM = 2;
        using OnHeapModeBits = BuiltinsArrayId::NewElementsKindBits::NextField<OnHeapMode, ON_HEAP_MODE_BITFIELD_NUM>;

        explicit BuiltinsTypedArrayId() = default;
        explicit BuiltinsTypedArrayId(uint32_t id) : BuiltinsId(id) {}

        BuiltinsTypedArrayId UpdateOnHeapMode(OnHeapMode onHeapMode)
        {
            id_ = OnHeapModeBits::Update(id_, onHeapMode);
            return *this;
        }

        OnHeapMode GetOnHeapMode() const
        {
            return OnHeapModeBits::Decode(id_);
        }

        std::string GetIdToString() const override
        {
            std::stringstream ss;
            ss << "(";
            ss << static_cast<uint32_t>(BuiltinsId::GetBuiltinsId()) << ", ";
            auto builtinsArrayId = BuiltinsArrayId(GetId());
            ss << static_cast<uint32_t>(builtinsArrayId.GetElementsKind()) << ", ";
            ss << static_cast<uint32_t>(builtinsArrayId.GetTransitionElementsKind()) << ", ";
            ss << static_cast<uint32_t>(GetOnHeapMode());
            ss << ")";
            return ss.str();
        }
    };

    static_assert(KindBits::IsValid(Kind::TotalKinds));

    ProfileType() = default;
    explicit ProfileType(uint64_t rawType) : type_(rawType) {};
    ProfileType(PGOContext &context, ProfileTypeRef typeRef);
    ProfileType(ApEntityId abcId, uint32_t type, Kind kind = Kind::ClassId, bool root = false,
                bool everOutOfBounds = false)
    {
        if (UNLIKELY(!IdBits::IsValid(type))) {
            type_ = 0;
        } else {
            UpdateAbcId(abcId);
            UpdateId(type);
            UpdateKind(kind);
            UpdateIsRootFlag(root);
            UpdateEverOutOfBounds(everOutOfBounds);
        }
    }

    static ProfileType CreateMegaType()
    {
        ProfileType type;
        type.UpdateKind(Kind::MegaStateKinds);
        return type;
    }

    static std::optional<ProfileType> CreateFromProfileTypeRef(PGOContext &context, ProfileTypeRef typeRef);

    static ProfileType CreateJITType()
    {
        ProfileType type;
        type.UpdateKind(Kind::JITClassId);
        return type;
    }

    static ProfileType CreateBuiltinsArray(ApEntityId abcId, JSType type, ElementsKind kind,
                                           ElementsKind transitionKind, bool everOutOfBounds)
    {
        auto id = BuiltinsArrayId().UpdateElementsKind(kind).UpdateTransitionElementsKind(transitionKind)
                  .SetBuiltinsId(type).GetId();
        return ProfileType(abcId, id, Kind::BuiltinsId, false, everOutOfBounds);
    }

    static ProfileType CreateBuiltinsTypedArray(ApEntityId abcId, JSType type, OnHeapMode onHeap,
                                                bool everOutOfBounds)
    {
        auto id = BuiltinsTypedArrayId().UpdateOnHeapMode(onHeap).SetBuiltinsId(type).GetId();
        return ProfileType(abcId, id, Kind::BuiltinsId, false, everOutOfBounds);
    }

    static ProfileType CreateBuiltins(ApEntityId abcId, JSType type)
    {
        auto id = BuiltinsId().SetBuiltinsId(type).GetId();
        return ProfileType(abcId, id, Kind::BuiltinsId);
    }

    static ProfileType CreateGlobals(ApEntityId abcId, GlobalIndex globalsId)
    {
        auto id = globalsId.GetGlobalIndex();
        return ProfileType(abcId, id, Kind::GlobalsId);
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

    bool IsRootType() const
    {
        return IsRootBits::Decode(type_);
    }

    bool IsEverOutOfBounds() const
    {
        return EverOutOfBoundsBits::Decode(type_);
    }

    bool IsGlobalsType() const
    {
        return GetKind() == Kind::GlobalsId;
    }

    bool IsBuiltinsType() const
    {
        return GetKind() == Kind::BuiltinsId;
    }

    bool IsClassType() const
    {
        return GetKind() == Kind::ClassId;
    }

    bool IsJITClassType() const
    {
        return GetKind() == Kind::JITClassId;
    }

    bool IsMethodId() const
    {
        return GetKind() == Kind::MethodId;
    }

    bool IsBuiltinFunctionId() const
    {
        return GetKind() == Kind::BuiltinFunctionId;
    }

    bool IsArrayLiteralType() const
    {
        return GetKind() == Kind::ArrayLiteralId;
    }

    bool IsObjectLiteralType() const
    {
        return GetKind() == Kind::ObjectLiteralId;
    }

    bool IsConstructor() const
    {
        return GetKind() == Kind::ConstructorId;
    }

    bool IsPrototype() const
    {
        return GetKind() == Kind::PrototypeId;
    }

    bool IsTransitionClassType() const
    {
        return GetKind() == Kind::TransitionClassId;
    }

    bool IsTransitionPrototype() const
    {
        return GetKind() == Kind::TransitionPrototypeId;
    }

    bool IsNapiType() const
    {
        return GetKind() == Kind::NapiId;
    }

    bool IsInvalidType() const
    {
        return GetKind() == Kind::InvalidId;
    }

    bool IsTransitionType() const
    {
        return IsTransitionClassType() || IsTransitionPrototype();
    }

    bool IsMegaStateType() const
    {
        return GetKind() == Kind::MegaStateKinds;
    }

    bool IsGeneralizedClassType() const
    {
        return IsClassType() || IsTransitionClassType();
    }

    bool IsGeneralizedPrototype() const
    {
        return IsPrototype() || IsTransitionPrototype();
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
        if (abcId > HUGE_ABC_ID) {
            LOG_ECMA(FATAL) << "huge abcId: " << abcId;  // only for debug purpose, do not merge to release version
        }
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
        stream << "Type: " << "(isRoot: " << IsRootType() <<
                ", ever out of bounds: " << IsEverOutOfBounds() <<
                ", kind: " << std::showbase << std::dec << static_cast<uint32_t>(GetKind()) <<
                ", abcId: " << GetAbcId() << ", id: " << GetId() << ")";
        return stream.str();
    }

    void GetTypeJson(StringMap &type) const
    {
        type.insert(std::make_pair(DumpJsonUtils::IS_ROOT, IsRootType() ? "true" : "false"));
        type.insert(std::make_pair(DumpJsonUtils::KIND, std::to_string(static_cast<double>(GetKind()))));
        type.insert(std::make_pair(DumpJsonUtils::ABC_ID, std::to_string(GetAbcId())));
        std::string strId;
        if (IsBuiltinsArray()) {
            auto arrayId = BuiltinsArrayId(GetId());
            strId = arrayId.GetIdToString();
        } else if (IsBuiltinsTypeArray()) {
            auto typedArrayId = BuiltinsTypedArrayId(GetId());
            strId = typedArrayId.GetIdToString();
        } else {
            auto builtinsId = BuiltinsId(GetId());
            strId = builtinsId.GetIdToString();
        }
        type.insert(std::make_pair(DumpJsonUtils::ID, strId));
    }

    friend std::ostream& operator<<(std::ostream& os, const ProfileType& type)
    {
        os << type.GetTypeString();
        return os;
    }

    void UpdateId(uint32_t id)
    {
        type_ = IdBits::Update(type_, id);
    }

    void UpdateKind(Kind kind)
    {
        type_ = KindBits::Update(type_, kind);
    }

    void UpdateIsRootFlag(bool root)
    {
        type_ = IsRootBits::Update(type_, root);
    }

    void UpdateEverOutOfBounds(bool val)
    {
        type_ = EverOutOfBoundsBits::Update(type_, val);
    }

    bool IsValidCallMethodId() const
    {
        return GetCallMethodId() > 0;
    }

    bool IsValidClassConstructorMethodId() const
    {
        return GetClassConstructorMethodId() > 0;
    }

    uint32_t GetCallMethodId() const
    {
        ASSERT(IsMethodId());
        return GetId();
    }

    uint32_t GetClassConstructorMethodId() const
    {
        ASSERT(IsClassType() || IsJITClassType());
        return GetId();
    }

    JSType GetBuiltinsType() const
    {
        ASSERT(IsBuiltinsType());
        auto builtinsId = BuiltinsId(GetId());
        return builtinsId.GetBuiltinsId();
    }

    GlobalIndex GetGlobalsId() const
    {
        ASSERT(IsGlobalsType());
        auto globalsId = GlobalIndex(GetId());
        return globalsId;
    }

    ElementsKind GetElementsKindBeforeTransition() const
    {
        ASSERT(IsBuiltinsArray());
        auto builtinsArrayId = BuiltinsArrayId(GetId());
        return builtinsArrayId.GetElementsKind();
    }

    ElementsKind GetElementsKindAfterTransition() const
    {
        ASSERT(IsBuiltinsArray());
        auto builtinsArrayId = BuiltinsArrayId(GetId());
        return builtinsArrayId.GetTransitionElementsKind();
    }

    bool IsBuiltinsMap() const
    {
        if (IsBuiltinsType()) {
            JSType type = GetBuiltinsType();
            return type == JSType::JS_MAP;
        }
        return false;
    }

    bool IsBuiltinsString() const
    {
        if (IsBuiltinsType()) {
            JSType type = GetBuiltinsType();
            return type >= JSType::STRING_FIRST && type <= JSType::STRING_LAST;
        }
        return false;
    }

    bool IsBuiltinsArray() const
    {
        if (IsBuiltinsType()) {
            JSType type = GetBuiltinsType();
            return type == JSType::JS_ARRAY;
        }
        return false;
    }

    bool IsBuiltinsTypeArray() const
    {
        if (IsBuiltinsType()) {
            JSType type = GetBuiltinsType();
            return type > JSType::JS_TYPED_ARRAY_FIRST && type <= JSType::JS_TYPED_ARRAY_LAST;
        }
        return false;
    }

private:
    void UpdateId(uint64_t type)
    {
        type_ = IdBits::Update(type_, type);
    }

    uint64_t type_ {0};
};

struct HashProfileType {
    uint64_t operator()(const ProfileType &profileType) const
    {
        return profileType.GetRaw();
    }
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

    bool IsBuiltinsArray() const
    {
        return false;
    }

    bool IsConstructor() const
    {
        return false;
    }

    bool IsMegaStateType() const
    {
        return false;
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

class TraProfileType {
public:
    TraProfileType(ProfileType root, ProfileType child) : root_(root), child_(child) {}

private:
    ProfileType root_;
    ProfileType child_;
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_TYPES_PGO_PROFILE_TYPE_H
