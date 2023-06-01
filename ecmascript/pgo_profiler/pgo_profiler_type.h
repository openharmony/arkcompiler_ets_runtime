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

#include "ecmascript/property_attributes.h"

namespace panda::ecmascript {
class ClassType {
public:
    ClassType() = default;
    explicit ClassType(int32_t type) : type_(type) {}

    int32_t GetClassType() const
    {
        return type_;
    }

    bool operator<(const ClassType &right) const
    {
        return type_ < right.type_;
    }

    bool operator!=(const ClassType &right) const
    {
        return type_ != right.type_;
    }

private:
    int32_t type_ { 0 };
};

class PGOType {
public:
    enum class TypeKind : uint8_t {
        SAMPLE_TYPE,
        LAYOUT_DESC,
    };
    PGOType() = default;
    explicit PGOType(TypeKind kind) : kind_(kind) {}

    bool IsSampleType() const
    {
        return kind_ == TypeKind::SAMPLE_TYPE;
    }

    bool IsLayoutDesc() const
    {
        return kind_ == TypeKind::LAYOUT_DESC;
    }

private:
    TypeKind kind_ { TypeKind::SAMPLE_TYPE };
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
    };

    PGOSampleType() : type_(Type::NONE) {};

    explicit PGOSampleType(Type type) : type_(type) {};
    explicit PGOSampleType(uint32_t type) : type_(Type(type)) {};
    explicit PGOSampleType(ClassType type) : type_(type) {}

    static PGOSampleType CreateClassType(int32_t classType)
    {
        return PGOSampleType(ClassType(classType));
    }

    static PGOSampleType NoneType()
    {
        return PGOSampleType(Type::NONE);
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
        return static_cast<int32_t>(curType) | static_cast<int32_t>(newType);
    }

    PGOSampleType CombineType(PGOSampleType type)
    {
        if (type_.index() == 0) {
            type_ =
                Type(static_cast<uint32_t>(std::get<Type>(type_)) | static_cast<uint32_t>(std::get<Type>(type.type_)));
        } else {
            SetType(type);
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
            return std::to_string(std::get<ClassType>(type_).GetClassType());
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

    bool IsAny() const
    {
        return type_.index() == 0 && std::get<Type>(type_) == Type::ANY;
    }

    bool IsNone() const
    {
        return type_.index() == 0 && std::get<Type>(type_) == Type::NONE;
    }

    bool IsInt() const
    {
        return type_.index() == 0 && std::get<Type>(type_) == Type::INT;
    }

    bool IsIntOverFlow() const
    {
        return type_.index() == 0 && std::get<Type>(type_) == Type::INT_OVERFLOW;
    }

    bool IsDouble() const
    {
        return type_.index() == 0 && std::get<Type>(type_) == Type::DOUBLE;
    }

    bool IsNumber() const
    {
        if (type_.index() != 0) {
            return false;
        }
        switch (std::get<Type>(type_)) {
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

private:
    std::variant<Type, ClassType> type_;
};

class PGOSampleLayoutDesc : public PGOType {
public:
    PGOSampleLayoutDesc() = default;
    explicit PGOSampleLayoutDesc(ClassType type) : PGOType(TypeKind::LAYOUT_DESC), type_(type) {}

    void AddKeyAndDesc(const char *key, TrackType rep)
    {
        layoutDesc_.emplace(key, rep);
    }

    bool FindDescWithKey(const CString &key, TrackType &type) const
    {
        if (layoutDesc_.find(key) == layoutDesc_.end()) {
            return false;
        }
        type = layoutDesc_.at(key);
        return true;
    }

private:
    ClassType type_;
    std::unordered_map<CString, TrackType> layoutDesc_;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_PGO_PROFILER_TYPE_H
