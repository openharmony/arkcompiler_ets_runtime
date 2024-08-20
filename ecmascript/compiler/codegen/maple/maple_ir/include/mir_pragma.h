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

#ifndef MAPLE_IR_INCLUDE_MIR_PRAGMA_H
#define MAPLE_IR_INCLUDE_MIR_PRAGMA_H
#include "types_def.h"
#include "prim_types.h"
#include "mir_module.h"
#include "mpl_logging.h"
#include "mempool_allocator.h"

namespace maple {
class MIRModule;         // circular dependency exists, no other choice
class MIRType;           // circular dependency exists, no other choice
class MIRFunction;       // circular dependency exists, no other choice
class MIRSymbol;         // circular dependency exists, no other choice
class MIRSymbolTable;    // circular dependency exists, no other choice
class MIRTypeNameTable;  // circular dependency exists, no other choice
enum PragmaKind {
    kPragmaUnknown,
    kPragmaClass,
    kPragmaFunc,
    kPragmaField,
    kPragmaParam,
    kPragmaVar,
    kPragmaFuncExecptioni,
    kPragmaFuncVar
};

enum PragmaValueType {
    kValueByte = 0x00,          // (none; must be 0)  ubyte[1]
    kValueShort = 0x02,         // size - 1 (0…1)  ubyte[size]
    kValueChar = 0x03,          // size - 1 (0…1)  ubyte[size]
    kValueInt = 0x04,           // size - 1 (0…3)  ubyte[size]
    kValueLong = 0x06,          // size - 1 (0…7)  ubyte[size]
    kValueFloat = 0x10,         // size - 1 (0…3)  ubyte[size]
    kValueDouble = 0x11,        // size - 1 (0…7)  ubyte[size]
    kValueMethodType = 0x15,    // size - 1 (0…3)  ubyte[size]
    kValueMethodHandle = 0x16,  // size - 1 (0…3)  ubyte[size]
    kValueString = 0x17,        // size - 1 (0…3)  ubyte[size]
    kValueType = 0x18,          // size - 1 (0…3)  ubyte[size]
    kValueField = 0x19,         // size - 1 (0…3)  ubyte[size]
    kValueMethod = 0x1a,        // size - 1 (0…3)  ubyte[size]
    kValueEnum = 0x1b,          // size - 1 (0…3)  ubyte[size]
    kValueArray = 0x1c,         // (none; must be 0) encoded_array
    kValueAnnotation = 0x1d,    // (none; must be 0) encoded_annotation
    kValueNull = 0x1e,          // (none; must be 0) (none)
    kValueBoolean = 0x1f        // boolean (0…1)   (none)
};

class MIRPragmaElement {
public:
    explicit MIRPragmaElement(MIRModule &m) : MIRPragmaElement(m.GetPragmaMPAllocator())
    {
        d = 0;
    }

    explicit MIRPragmaElement(MapleAllocator &subElemAllocator) : subElemVec(subElemAllocator.Adapter())
    {
        subElemVec.clear();
        d = 0;
    }

    ~MIRPragmaElement() = default;
    void SubElemVecPushBack(MIRPragmaElement *elem)
    {
        subElemVec.push_back(elem);
    }

private:
    double d;
    MapleVector<MIRPragmaElement *> subElemVec;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_PRAGMA_H
