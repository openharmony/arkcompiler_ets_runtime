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

#ifndef ECMASCRIPT_PGO_PROFILER_TYPES_PGO_PROFILER_TYPE_H
#define ECMASCRIPT_PGO_PROFILER_TYPES_PGO_PROFILER_TYPE_H

#include <stdint.h>
#include <string>
#include <variant>

#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "libpandabase/utils/bit_field.h"
#include "macros.h"

namespace panda::ecmascript::pgo {
class PGOContext;
class PGOType {
public:
    enum class TypeKind : uint8_t {
        SCALAR_OP_TYPE,
        RW_OP_TYPE,
    };
    PGOType() = default;
    explicit PGOType(TypeKind kind) : kind_(kind) {}

    bool IsScalarOpType() const
    {
        return kind_ == TypeKind::SCALAR_OP_TYPE;
    }

    bool IsRwOpType() const
    {
        return kind_ == TypeKind::RW_OP_TYPE;
    }

private:
    TypeKind kind_ {TypeKind::SCALAR_OP_TYPE};
};

/**
 * | INT    \          -> INT_OVERFLOW \
 * |           NUMBER                    NUMBER_HETEROE1
 * | DOUBLE /                          /
 */
template <typename PGOProfileType>
class PGOSampleTemplate : public PGOType {
public:
    static constexpr int WEIGHT_BITS = 11;
    static constexpr int WEIGHT_START_BIT = 10;
    static constexpr int WEIGHT_TRUE_START_BIT = WEIGHT_START_BIT + WEIGHT_BITS;
    static constexpr int WEIGHT_MASK = (1 << WEIGHT_BITS) - 1;

    enum class Type : uint32_t {
        NONE = 0x0ULL,
        INT = 0x1ULL,                        // 00000001
        INT_OVERFLOW = (0x1ULL << 1) | INT,  // 00000011
        DOUBLE = 0x1ULL << 2,                // 00000100
        NUMBER = INT | DOUBLE,               // 00000101
        NUMBER1 = INT_OVERFLOW | DOUBLE,     // 00000111
        BOOLEAN = 0x1ULL << 3,
        UNDEFINED_OR_NULL = 0x1ULL << 4,
        SPECIAL = 0x1ULL << 5,
        BOOLEAN_OR_SPECIAL = BOOLEAN | SPECIAL,
        STRING = 0x1ULL << 6,
        BIG_INT = 0x1ULL << 7,
        HEAP_OBJECT = 0x1ULL << 8,
        HEAP_OR_UNDEFINED_OR_NULL = HEAP_OBJECT | UNDEFINED_OR_NULL,
        ANY = (0x1ULL << WEIGHT_START_BIT) - 1,
    };

    PGOSampleTemplate() : type_(Type::NONE) {};

    explicit PGOSampleTemplate(Type type) : type_(type) {};
    explicit PGOSampleTemplate(uint32_t type) : type_(Type(type)) {};
    explicit PGOSampleTemplate(PGOProfileType type) : type_(type) {}

    template <typename FromType>
    static PGOSampleTemplate ConvertFrom(PGOContext &context, const FromType &from)
    {
        if (from.IsProfileType()) {
            return PGOSampleTemplate(PGOProfileType(context, from.GetProfileType()));
        }
        return PGOSampleTemplate(static_cast<PGOSampleTemplate::Type>(from.GetType()));
    }

    static PGOSampleTemplate CreateProfileType(int32_t profileType,
                                               typename ProfileType::Kind kind = ProfileType::Kind::ClassId)
    {
        return PGOSampleTemplate(PGOProfileType(profileType, kind));
    }

    static PGOSampleTemplate NoneType()
    {
        return PGOSampleTemplate(Type::NONE);
    }

    static int32_t None()
    {
        return static_cast<int32_t>(Type::NONE);
    }

    static int32_t AnyType()
    {
        return static_cast<int32_t>(Type::ANY);
    }

    static int32_t IntType()
    {
        return static_cast<int32_t>(Type::INT);
    }

    static int32_t IntOverFlowType()
    {
        return static_cast<int32_t>(Type::INT_OVERFLOW);
    }

    static int32_t DoubleType()
    {
        return static_cast<int32_t>(Type::DOUBLE);
    }

    static int32_t NumberType()
    {
        return static_cast<int32_t>(Type::NUMBER);
    }

    static int32_t HeapObjectType()
    {
        return static_cast<int32_t>(Type::HEAP_OBJECT);
    }

    static int32_t UndefineOrNullType()
    {
        return static_cast<int32_t>(Type::UNDEFINED_OR_NULL);
    }

    static int32_t BooleanType()
    {
        return static_cast<int32_t>(Type::BOOLEAN);
    }

    static int32_t StringType()
    {
        return static_cast<int32_t>(Type::STRING);
    }

    static int32_t BigIntType()
    {
        return static_cast<int32_t>(Type::BIG_INT);
    }

    static int32_t SpecialType()
    {
        return static_cast<int32_t>(Type::SPECIAL);
    }

    static int32_t CombineType(int32_t curType, int32_t newType)
    {
        return static_cast<int32_t>(static_cast<uint32_t>(curType) | static_cast<uint32_t>(newType));
    }

    static PGOSampleTemplate NoneProfileType()
    {
        return PGOSampleTemplate(PGOProfileType());
    }

    PGOSampleTemplate CombineType(PGOSampleTemplate type)
    {
        if (type_.index() == 0) {
            auto oldType = static_cast<uint32_t>(std::get<Type>(type_));
            oldType = oldType & AnyType();
            type_ =
                Type(oldType | static_cast<uint32_t>(std::get<Type>(type.type_)));
        } else {
            this->SetType(type);
        }
        return *this;
    }

    PGOSampleTemplate CombineCallTargetType(PGOSampleTemplate type)
    {
        ASSERT(type_.index() == 1);
        uint32_t oldMethodId = GetProfileType().GetId();
        uint32_t newMethodId = type.GetProfileType().GetId();
        // If we have recorded a valid method if before, invalidate it.
        if ((oldMethodId != newMethodId) && (oldMethodId != 0)) {
            type_ = PGOProfileType(0);
        }
        return *this;
    }

    void SetType(PGOSampleTemplate type)
    {
        type_ = type.type_;
    }

    std::string GetTypeString() const
    {
        if (type_.index() == 0) {
            return std::to_string(static_cast<uint32_t>(std::get<Type>(type_)));
        } else {
            return std::get<PGOProfileType>(type_).GetTypeString();
        }
    }

    bool IsProfileType() const
    {
        return type_.index() == 1;
    }

    PGOProfileType GetProfileType() const
    {
        ASSERT(IsProfileType());
        return std::get<PGOProfileType>(type_);
    }

    Type GetPrimitiveType() const
    {
        auto type = static_cast<uint32_t>(std::get<Type>(type_));
        return Type(type & AnyType());
    }

    uint32_t GetWeight() const
    {
        ASSERT(type_.index() == 0);
        auto type = static_cast<uint32_t>(std::get<Type>(type_));
        return type >> WEIGHT_START_BIT;
    }

    bool IsAny() const
    {
        return type_.index() == 0 && GetPrimitiveType() == Type::ANY;
    }

    bool IsNone() const
    {
        return type_.index() == 0 && GetPrimitiveType() == Type::NONE;
    }

    bool IsInt() const
    {
        return type_.index() == 0 && GetPrimitiveType() == Type::INT;
    }

    bool IsIntOverFlow() const
    {
        return type_.index() == 0 && GetPrimitiveType() == Type::INT_OVERFLOW;
    }

    bool IsDouble() const
    {
        return type_.index() == 0 && GetPrimitiveType() == Type::DOUBLE;
    }

    bool IsNumber() const
    {
        if (type_.index() != 0) {
            return false;
        }
        auto primType = GetPrimitiveType();
        switch (primType) {
            case Type::INT:
            case Type::INT_OVERFLOW:
            case Type::DOUBLE:
            case Type::NUMBER:
            case Type::NUMBER1:
                return true;
            default:
                return false;
        }
    }

    bool operator<(const PGOSampleTemplate &right) const
    {
        return type_ < right.type_;
    }

    bool operator!=(const PGOSampleTemplate &right) const
    {
        return type_ != right.type_;
    }

    bool operator==(const PGOSampleTemplate &right) const
    {
        return type_ == right.type_;
    }

    Type GetType() const
    {
        ASSERT(!IsProfileType());
        return std::get<Type>(type_);
    }

private:
    std::variant<Type, PGOProfileType> type_;
};

using PGOSampleType = PGOSampleTemplate<ProfileType>;
using PGOSampleTypeRef = PGOSampleTemplate<ProfileTypeRef>;

enum class PGOObjKind {
    LOCAL,
    PROTOTYPE,
    CONSTRUCTOR,
    ELEMENT,
};

template <typename PGOProfileType>
class PGOObjectTemplate {
public:
    PGOObjectTemplate() = default;
    PGOObjectTemplate(PGOProfileType type, PGOObjKind kind) : type_(type), objKind_(kind) {}

    template <typename FromType>
    void ConvertFrom(PGOContext &context, const FromType &from)
    {
        objKind_ = from.GetObjKind();
        type_ = PGOProfileType(context, from.GetProfileType());
    }

    std::string GetInfoString() const
    {
        std::string result = type_.GetTypeString();
        result += "(";
        if (objKind_ == PGOObjKind::CONSTRUCTOR) {
            result += "c";
        } else if (objKind_ == PGOObjKind::PROTOTYPE) {
            result += "p";
        } else if (objKind_ == PGOObjKind::ELEMENT) {
            result += "e";
        } else {
            result += "l";
        }
        result += ")";
        return result;
    }

    PGOProfileType GetProfileType() const
    {
        return type_;
    }

    PGOObjKind GetObjKind() const
    {
        return objKind_;
    }

    bool IsNone() const
    {
        return type_.IsNone();
    }

    bool InConstructor() const
    {
        return objKind_ == PGOObjKind::CONSTRUCTOR;
    }

    bool InElement() const
    {
        return objKind_ == PGOObjKind::ELEMENT;
    }

    bool operator<(const PGOObjectTemplate &right) const
    {
        return type_ < right.type_ || objKind_ < right.objKind_;
    }

    bool operator==(const PGOObjectTemplate &right) const
    {
        return type_ == right.type_ && objKind_ == right.objKind_;
    }

private:
    PGOProfileType type_ {PGOProfileType()};
    PGOObjKind objKind_ {PGOObjKind::LOCAL};
};
using PGOObjectInfo = PGOObjectTemplate<ProfileType>;
using PGOObjectInfoRef = PGOObjectTemplate<ProfileTypeRef>;

template <typename PGOObjectInfoType>
class PGORWOpTemplate : public PGOType {
public:
    PGORWOpTemplate() : PGOType(TypeKind::RW_OP_TYPE) {};

    template <typename FromType>
    void ConvertFrom(PGOContext &context, const FromType &from)
    {
        count_ = std::min(from.GetCount(), static_cast<uint32_t>(POLY_CASE_NUM));
        for (uint32_t index = 0; index < count_; index++) {
            infos_[index].ConvertFrom(context, from.GetObjectInfo(index));
        }
    }

    void Merge(const PGORWOpTemplate &type)
    {
        for (uint32_t i = 0; i < type.count_; i++) {
            AddObjectInfo(type.infos_[i]);
        }
    }

    void AddObjectInfo(const PGOObjectInfoType &info)
    {
        if (info.IsNone()) {
            return;
        }
        uint32_t count = 0;
        for (; count < count_; count++) {
            if (infos_[count] == info) {
                return;
            }
        }
        if (count < static_cast<uint32_t>(POLY_CASE_NUM)) {
            infos_[count] = info;
            count_++;
        } else {
            LOG_ECMA(DEBUG) << "Class type exceeds 4, discard";
        }
    }

    PGOObjectInfoType GetObjectInfo(uint32_t index) const
    {
        ASSERT(index < count_);
        return infos_[index];
    }

    uint32_t GetCount() const
    {
        return count_;
    }

private:
    static constexpr int POLY_CASE_NUM = 4;
    uint32_t count_ = 0;
    PGOObjectInfoType infos_[POLY_CASE_NUM];
};

using PGORWOpType = PGORWOpTemplate<PGOObjectInfo>;
using PGORWOpTypeRef = PGORWOpTemplate<PGOObjectInfoRef>;
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_TYPES_PGO_PROFILER_TYPE_H
