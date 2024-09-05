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

#ifndef MAPLE_IR_INCLUDE_PRIM_TYPES_H
#define MAPLE_IR_INCLUDE_PRIM_TYPES_H
#include "types_def.h"
#include "cfg_primitive_types.h"

namespace maple {
class PrimitiveType {
public:
    // we need implicit conversion from PrimType to PrimitiveType, so there is no explicit keyword here.
    PrimitiveType(PrimType type) : property(GetPrimitiveTypeProperty(type)) {}
    ~PrimitiveType() = default;

    PrimType GetType() const
    {
        return property.type;
    }

    bool IsInteger() const
    {
        return property.IsInteger();
    }
    bool IsUnsigned() const
    {
        return property.IsUnsigned();
    }
    bool IsAddress() const
    {
        return property.IsAddress();
    }
    bool IsFloat() const
    {
        return property.IsFloat();
    }
    bool IsPointer() const
    {
        return property.IsPointer();
    }
    bool IsSimple() const
    {
        return property.IsSimple();
    }
private:
    const PrimitiveTypeProperty &property;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_PRIM_TYPES_H
