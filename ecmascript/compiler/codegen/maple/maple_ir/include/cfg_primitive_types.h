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

#ifndef MAPLE_IR_INCLUDE_CFG_PRIMITIVE_TYPES_H
#define MAPLE_IR_INCLUDE_CFG_PRIMITIVE_TYPES_H

namespace maple {
uint8 GetPointerSize();  // Circular include dependency with mir_type.h

// Declaration of enum PrimType
#define LOAD_ALGO_PRIMARY_TYPE
enum PrimType {
    PTY_begin,  // PrimType begin
#define PRIMTYPE(P) PTY_##P,
#include "prim_types.def"
    PTY_end,  // PrimType end
#undef PRIMTYPE
};

constexpr PrimType kPtyInvalid = PTY_begin;
// just for test, no primitive type for derived SIMD types to be defined
constexpr PrimType kPtyDerived = PTY_end;

struct PrimitiveTypeProperty {
    PrimType type;

    PrimitiveTypeProperty(PrimType type, bool isInteger, bool isUnsigned, bool isAddress, bool isFloat, bool isPointer,
                          bool isSimple)
        : type(type),
          isInteger(isInteger),
          isUnsigned(isUnsigned),
          isAddress(isAddress),
          isFloat(isFloat),
          isPointer(isPointer),
          isSimple(isSimple)
    {
    }

    bool IsInteger() const
    {
        return isInteger;
    }
    bool IsUnsigned() const
    {
        return isUnsigned;
    }

    bool IsAddress() const
    {
        if (type == PTY_u64 || type == PTY_u32) {
            if ((type == PTY_u64 && GetPointerSize() == 8) || // PTY_u64 with 8 byte pointer
                (type == PTY_u32 && GetPointerSize() == 4)) { // PTY_u32 with 4 byte pointer
                return true;
            } else {
                return false;
            }
        } else {
            return isAddress;
        }
    }

    bool IsFloat() const
    {
        return isFloat;
    }

    bool IsPointer() const
    {
        if (type == PTY_u64 || type == PTY_u32) {
            if ((type == PTY_u64 && GetPointerSize() == 8) || // PTY_u64 with 8 byte pointer
                (type == PTY_u32 && GetPointerSize() == 4)) { // PTY_u32 with 4 byte pointer
                return true;
            } else {
                return false;
            }
        } else {
            return isPointer;
        }
    }

    bool IsSimple() const
    {
        return isSimple;
    }

private:
    bool isInteger;
    bool isUnsigned;
    bool isAddress;
    bool isFloat;
    bool isPointer;
    bool isSimple;
};

const PrimitiveTypeProperty &GetPrimitiveTypeProperty(PrimType pType);
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_CFG_PRIMITIVE_TYPES_H
