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

namespace panda::ecmascript {
/**
 * | INT    \          -> INT_OVERFLOW \
 * |           NUMBER                    NUMBER_HETEROE1
 * | DOUBLE /                          /
 */
class PGOSampleType {
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

    PGOSampleType(Type type) : type_(type) {};
    PGOSampleType(uint32_t type) : type_(Type(type)) {};

    static PGOSampleType NoneType()
    {
        return PGOSampleType(Type::NONE);
    }

    PGOSampleType CombineType(PGOSampleType type)
    {
        type_ = Type(static_cast<uint32_t>(type_) | static_cast<uint32_t>(type.type_));
        return *this;
    }

    void SetType(PGOSampleType type)
    {
        type_ = type.type_;
    }

    std::string GetTypeString()
    {
        return std::to_string(static_cast<uint32_t>(type_));
    }

    bool IsAny()
    {
        return type_ == Type::ANY;
    }

    bool IsNone()
    {
        return type_ == Type::NONE;
    }

    bool IsInt()
    {
        return type_ == Type::INT;
    }

    bool IsIntOverFlow()
    {
        return type_ == Type::INT_OVERFLOW;
    }

    bool IsDouble()
    {
        return type_ == Type::DOUBLE;
    }

    bool IsNumber()
    {
        switch (type_) {
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

private:
    Type type_ { Type::NONE };
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_PGO_PROFILER_TYPE_H
