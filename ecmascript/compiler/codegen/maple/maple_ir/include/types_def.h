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

#ifndef MAPLE_IR_INCLUDE_TYPES_DEF_H
#define MAPLE_IR_INCLUDE_TYPES_DEF_H

// NOTE: Since we already committed to -std=c++0x, we should eventually use the
// standard definitions in the <cstdint> and <limits> headers rather than
// reinventing our own primitive types.
#include <cstdint>
#include <cstddef>
#include <functional>
#include "mpl_number.h"

namespace maple {
// Let's keep the following definitions so that existing code will continue to work.
using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;
using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
typedef unsigned int uint;

class StIdx {  // scope nesting level + symbol table index
public:
    union un {
        struct {
            uint32 idx : 24;
            uint8 scope;  // scope level, with the global scope is at level 1
        } scopeIdx;

        uint32 fullIdx;
    };

    StIdx()
    {
        u.fullIdx = 0;
    }

    StIdx(uint32 level, uint32 i)
    {
        u.scopeIdx.scope = level;
        u.scopeIdx.idx = i;
    }

    StIdx(uint32 fidx)
    {
        u.fullIdx = fidx;
    }

    ~StIdx() = default;

    uint32 Idx() const
    {
        return u.scopeIdx.idx;
    }

    void SetIdx(uint32 idx)
    {
        u.scopeIdx.idx = idx;
    }

    uint32 Scope() const
    {
        return u.scopeIdx.scope;
    }

    uint32 FullIdx() const
    {
        return u.fullIdx;
    }

    void SetFullIdx(uint32 idx)
    {
        u.fullIdx = idx;
    }

    bool Islocal() const
    {
        return u.scopeIdx.scope > 1;
    }

    bool IsGlobal() const
    {
        return u.scopeIdx.scope == 1;
    }

    bool operator==(const StIdx &x) const
    {
        return u.fullIdx == x.u.fullIdx;
    }

    bool operator!=(const StIdx &x) const
    {
        return !(*this == x);
    }

    bool operator<(const StIdx &x) const
    {
        return u.fullIdx < x.u.fullIdx;
    }

private:
    un u;
};

using LabelIdx = uint32;
using phyRegIdx = uint64;
using OfstRegIdx = uint64;
using LabelIDOrder = uint32;
using PUIdx = uint32;
using PregIdx = int32;
using ExprIdx = int32;
using FieldID = int32;

class TypeTag;
using TyIdx = utils::Index<TypeTag, uint32>;  // global type table index

class GStrTag;
using GStrIdx = utils::Index<GStrTag, uint32>;  // global string table index

class UStrTag;
using UStrIdx = utils::Index<UStrTag, uint32>;  // user string table index (from the conststr opcode)

class U16StrTag;
using U16StrIdx = utils::Index<U16StrTag, uint32>;  // user string table index (from the conststr opcode)

const TyIdx kInitTyIdx = TyIdx(0);
const TyIdx kNoneTyIdx = TyIdx(UINT32_MAX);

enum SSALevel : uint8 {
    kSSAInvalid = 0x00,
    kSSATopLevel = 0x01,                        // ssa only for local top-level is valid
    kSSAAddrTaken = 0x02,                       // ssa only for addr-taken is valid
    kSSAMemory = kSSATopLevel | kSSAAddrTaken,  // ssa for both top-level and addr-taken is valid
    kSSAHSSA = 0x04,                            // hssa is valid
};

constexpr uint8 kOperandNumUnary = 1;
constexpr uint8 kOperandNumBinary = 2;
constexpr uint8 kOperandNumTernary = 3;
}  // namespace maple
namespace std {
template <>  // function-template-specialization
class hash<maple::StIdx> {
public:
    size_t operator()(const maple::StIdx &x) const
    {
        std::size_t seed = 0;
        hash_combine(seed, x.Scope());
        hash_combine(seed, x.Idx());
        return seed;
    }
};
}  // namespace std
#endif  // MAPLE_IR_INCLUDE_TYPES_DEF_H
