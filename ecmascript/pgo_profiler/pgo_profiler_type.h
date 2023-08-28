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

#ifndef ECMASCRIPT_PGO_PROFILER_TYPE_H
#define ECMASCRIPT_PGO_PROFILER_TYPE_H

#include <stdint.h>
#include <string>
#include <variant>

#include "macros.h"

namespace panda::ecmascript {
class ClassType {
public:
    enum class Kind : uint8_t {
        ClassId,
        LiteralId,
        ElementId,
        BuiltinsId,
        TotalKinds
    };

    static constexpr uint32_t ID_BITFIELD_NUM = 29;
    static constexpr uint32_t KIND_BITFIELD_NUM = 3;
    using IdBits = BitField<uint32_t, 0, ID_BITFIELD_NUM>;
    using KindBits = IdBits::NextField<Kind, KIND_BITFIELD_NUM>;

    static_assert(static_cast<uint32_t>(Kind::TotalKinds) <= (1ul << KIND_BITFIELD_NUM));

    ClassType() = default;
    explicit ClassType(uint32_t type, Kind kind = Kind::ClassId)
    {
        if (UNLIKELY(type >= (1ul << ID_BITFIELD_NUM))) {
            type_ = 0;
        } else {
            type_ = (static_cast<uint32_t>(kind) << ID_BITFIELD_NUM) + type;
        }
    }

    bool IsNone() const
    {
        return type_ == 0;
    }

    uint32_t GetClassType() const
    {
        return type_;
    }

    bool IsBuiltinsType() const
    {
        return GetKind() == Kind::BuiltinsId;
    }

    uint32_t GetId() const
    {
        return IdBits::Decode(type_);
    }

    Kind GetKind() const
    {
        return KindBits::Decode(type_);
    }

    bool operator<(const ClassType &right) const
    {
        return type_ < right.type_;
    }

    bool operator!=(const ClassType &right) const
    {
        return type_ != right.type_;
    }

    bool operator==(const ClassType &right) const
    {
        return type_ == right.type_;
    }

    std::string GetTypeString() const
    {
        return std::to_string(type_);
    }

private:
    uint32_t type_ { 0 };
};

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
    TypeKind kind_ { TypeKind::SCALAR_OP_TYPE };
};

/**
 * | INT    \          -> INT_OVERFLOW \
 * |           NUMBER                    NUMBER_HETEROE1
 * | DOUBLE /                          /
 */
class PGOSampleType : public PGOType {
public:
    enum class Type : uint32_t {
        NONE = 0x0ULL,
        INT = 0x1ULL,                       // 00000001
        INT_OVERFLOW = (0x1ULL << 1) | INT, // 00000011
        DOUBLE = 0x1ULL << 2,               // 00000100
        NUMBER = INT | DOUBLE,              // 00000101
        NUMBER1 = INT_OVERFLOW | DOUBLE,    // 00000111
        BOOLEAN = 0x1ULL << 3,
        UNDEFINED_OR_NULL = 0x1ULL << 4,
        SPECIAL = 0x1ULL << 5,
        BOOLEAN_OR_SPECIAL = BOOLEAN | SPECIAL,
        STRING = 0x1ULL << 6,
        BIG_INT = 0x1ULL << 7,
        HEAP_OBJECT = 0x1ULL << 8,
        HEAP_OR_UNDEFINED_OR_NULL = HEAP_OBJECT | UNDEFINED_OR_NULL,
        ANY = 0x3FFULL,
        BRANCH_START = 0x1ULL << 10,
        LIKELY = 0x2ULL << 10,
        UNLIKELY = 0x3ULL << 10,
        STRONG_LIKELY = 0x4ULL << 10,
        STRONG_UNLIKELY = 0x5ULL << 10,
        NORMAL_BRANCH = 0x6ULL << 10,
        BRANCH_END = 0x7ULL << 10,  // leave higher bits for branch type or count
    };

    PGOSampleType() : type_(Type::NONE) {};

    explicit PGOSampleType(Type type) : type_(type) {};
    explicit PGOSampleType(uint32_t type) : type_(Type(type)) {};
    explicit PGOSampleType(ClassType type) : type_(type) {}

    static PGOSampleType CreateClassType(int32_t classType, ClassType::Kind kind = ClassType::Kind::ClassId)
    {
        return PGOSampleType(ClassType(classType, kind));
    }

    static PGOSampleType NoneType()
    {
        return PGOSampleType(Type::NONE);
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

    static int32_t Likely()
    {
        return static_cast<int32_t>(Type::LIKELY);
    }

    static int32_t Unlikely()
    {
        return static_cast<int32_t>(Type::UNLIKELY);
    }

    static int32_t StrongLikely()
    {
        return static_cast<int32_t>(Type::STRONG_LIKELY);
    }

    static int32_t StrongUnLikely()
    {
        return static_cast<int32_t>(Type::STRONG_UNLIKELY);
    }

    static int32_t NormalBranch()
    {
        return static_cast<int32_t>(Type::NORMAL_BRANCH);
    }

    static int32_t BranchEndRev()
    {
        return static_cast<int32_t>(UINT32_MAX & static_cast<uint32_t>(Type::BRANCH_END));
    }

    static int32_t CombineType(int32_t curType, int32_t newType)
    {
        return static_cast<int32_t>(static_cast<uint32_t>(curType) | static_cast<uint32_t>(newType));
    }

    bool IsBranchType() const
    {
        auto type = std::get<Type>(type_);
        return (type >= Type::BRANCH_START) && (type <= Type::BRANCH_END);
    }

    static PGOSampleType NoneClassType()
    {
        return PGOSampleType(ClassType());
    }

    PGOSampleType CombineType(PGOSampleType type)
    {
        if (type_.index() == 0) {
            if (type.IsBranchType()) {
                // clear branch type bit before update branch type
                type_ = Type(static_cast<uint32_t>(std::get<Type>(type_)) &
                    (UINT32_MAX ^ static_cast<uint32_t>(Type::BRANCH_END)));
            }
            type_ =
                Type(static_cast<uint32_t>(std::get<Type>(type_)) | static_cast<uint32_t>(std::get<Type>(type.type_)));
        } else {
            SetType(type);
        }
        return *this;
    }

    PGOSampleType CombineCallTargetType(PGOSampleType type)
    {
        ASSERT(type_.index() == 1);
        uint32_t oldMethodId = GetClassType().GetClassType();
        uint32_t newMethodId = type.GetClassType().GetClassType();
        // If we have recorded a valid method if before, invalidate it.
        if ((oldMethodId != newMethodId) && (oldMethodId != 0)) {
            type_ = ClassType(0);
        }
        return *this;
    }

    void SetType(PGOSampleType type)
    {
        type_ = type.type_;
    }

    std::string GetTypeString() const
    {
        if (type_.index() == 0) {
            return std::to_string(static_cast<uint32_t>(std::get<Type>(type_)));
        } else {
            return std::get<ClassType>(type_).GetTypeString();
        }
    }

    bool IsClassType() const
    {
        return type_.index() == 1;
    }

    ClassType GetClassType() const
    {
        ASSERT(IsClassType());
        return std::get<ClassType>(type_);
    }

    Type GetPrimitiveType() const
    {
        auto type = static_cast<uint32_t>(std::get<Type>(type_));
        return static_cast<Type>(type & (UINT32_MAX ^ static_cast<uint32_t>(Type::BRANCH_END)));
    }

    Type GetBranchType() const
    {
        auto type = static_cast<uint32_t>(std::get<Type>(type_));
        return static_cast<Type>(type & static_cast<uint32_t>(Type::BRANCH_END));
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

    bool IsLikely() const
    {
        return type_.index() == 0 && GetBranchType() == Type::LIKELY;
    }

    bool IsUnLikely() const
    {
        return type_.index() == 0 && GetBranchType() == Type::UNLIKELY;
    }

    bool IsStrongLikely() const
    {
        return type_.index() == 0 && GetBranchType() == Type::STRONG_LIKELY;
    }

    bool IsStrongUnLikely() const
    {
        return type_.index() == 0 && GetBranchType() == Type::STRONG_UNLIKELY;
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

    bool operator<(const PGOSampleType &right) const
    {
        return type_ < right.type_;
    }

    bool operator!=(const PGOSampleType &right) const
    {
        return type_ != right.type_;
    }

    bool operator==(const PGOSampleType &right) const
    {
        return type_ == right.type_;
    }

private:
    std::variant<Type, ClassType> type_;
};

enum class PGOObjKind {
    LOCAL,
    PROTOTYPE,
    CONSTRUCTOR,
    ELEMENT,
};

class PGOObjectInfo {
public:
    PGOObjectInfo() : type_(ClassType()), objKind_(PGOObjKind::LOCAL) {}
    PGOObjectInfo(ClassType type, PGOObjKind kind) : type_(type), objKind_(kind) {}

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

    ClassType GetClassType() const
    {
        return type_;
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

    bool operator<(const PGOObjectInfo &right) const
    {
        return type_ < right.type_ || objKind_ < right.objKind_;
    }

    bool operator==(const PGOObjectInfo &right) const
    {
        return type_ == right.type_ && objKind_ == right.objKind_;
    }

private:
    ClassType type_ { ClassType() };
    PGOObjKind objKind_ { PGOObjKind::LOCAL };
};

class PGORWOpType : public PGOType {
public:
    PGORWOpType() : PGOType(TypeKind::RW_OP_TYPE), count_(0) {};

    void Merge(const PGORWOpType &type)
    {
        for (uint32_t i = 0; i < type.count_; i++) {
            AddObjectInfo(type.infos_[i]);
        }
    }

    void AddObjectInfo(const PGOObjectInfo &info)
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

    PGOObjectInfo GetObjectInfo(uint32_t index) const
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
    PGOObjectInfo infos_[POLY_CASE_NUM];
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_PGO_PROFILER_TYPE_H
