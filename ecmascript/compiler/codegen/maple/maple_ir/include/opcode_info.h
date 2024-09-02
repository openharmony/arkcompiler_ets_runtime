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

#ifndef MAPLE_IR_INCLUDE_OPCODE_INFO_H
#define MAPLE_IR_INCLUDE_OPCODE_INFO_H
#include "types_def.h"
#include "opcodes.h"
#include "mpl_logging.h"

namespace maple {
enum OpcodeProp {
    kOpcodePropNone,
    kOpcodePropIsStmt,          // The instruction is a stmt, so has 2 stmt pointers
    kOpcodePropIsVarSize,       // The instruction size is not fixed
    kOpcodePropNotMMPL,         // The instruction is not allowed in Machine Maple IR
    kOpcodePropIsCompare,       // The instruction is one of the 6 comparison ops
    kOpcodePropIsTypeCvt,       // The instruction is a type conversion op
    kOpcodePropHasSSAUse,       // The instruction may incur a use in SSA form
    kOpcodePropHasSSADef,       // The instruction may incur a def in SSA form
    kOpcodePropIsCall,          // The instruction is among the call instructions
    kOpcodePropIsCallAssigned,  // The instruction is among the call instructions with implicit assignments of the
    // returned values
    kOpcodePropNotPure,  // The operation does not return same result with idential operands
    kOpcodePropMayThrowException,
    kOpcodePropIsAssertUpperBoundary,  // The operation check upper boundary
    kOpcodePropIsAssertLowerBoundary,  // The operation check lower boundary
};

constexpr unsigned long OPCODEISSTMT = 1ULL << kOpcodePropIsStmt;
constexpr unsigned long OPCODEISVARSIZE = 1ULL << kOpcodePropIsVarSize;
constexpr unsigned long OPCODENOTMMPL = 1ULL << kOpcodePropNotMMPL;
constexpr unsigned long OPCODEISCOMPARE = 1ULL << kOpcodePropIsCompare;
constexpr unsigned long OPCODEISTYPECVT = 1ULL << kOpcodePropIsTypeCvt;
constexpr unsigned long OPCODEHASSSAUSE = 1ULL << kOpcodePropHasSSAUse;
constexpr unsigned long OPCODEHASSSADEF = 1ULL << kOpcodePropHasSSADef;
constexpr unsigned long OPCODEISCALL = 1ULL << kOpcodePropIsCall;
constexpr unsigned long OPCODEISCALLASSIGNED = 1ULL << kOpcodePropIsCallAssigned;
constexpr unsigned long OPCODENOTPURE = 1ULL << kOpcodePropNotPure;
constexpr unsigned long OPCODEMAYTHROWEXCEPTION = 1ULL << kOpcodePropMayThrowException;
constexpr unsigned long OPCODEASSERTUPPERBOUNDARY = 1ULL << kOpcodePropIsAssertUpperBoundary;
constexpr unsigned long OPCODEASSERTLOWERBOUNDARY = 1ULL << kOpcodePropIsAssertLowerBoundary;

struct OpcodeDesc {
    uint8 instrucSize;  // size of instruction in bytes
    uint16 flag;        // stores the opcode property flags
    std::string name;
};

class OpcodeTable {
public:
    OpcodeTable();
    ~OpcodeTable() = default;

    OpcodeDesc GetTableItemAt(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o];
    }

    bool IsStmt(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODEISSTMT;
    }

    bool IsVarSize(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODEISVARSIZE;
    }

    bool NotMMPL(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODENOTMMPL;
    }

    bool IsCompare(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODEISCOMPARE;
    }

    bool IsTypeCvt(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODEISTYPECVT;
    }

    bool HasSSAUse(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODEHASSSAUSE;
    }

    bool HasSSADef(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODEHASSSADEF;
    }

    bool IsCall(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODEISCALL;
    }

    bool IsCallAssigned(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODEISCALLASSIGNED;
    }

    bool IsICall(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return o == OP_icall || o == OP_icallassigned || o == OP_icallproto || o == OP_icallprotoassigned;
    }

    bool NotPure(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODENOTPURE;
    }

    bool MayThrowException(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODEMAYTHROWEXCEPTION;
    }

    bool HasSideEffect(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return MayThrowException(o);
    }

    const std::string &GetName(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].name;
    }

    bool IsCondBr(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return o == OP_brtrue || o == OP_brfalse;
    }

    bool AssignActualVar(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return o == OP_dassign || o == OP_regassign;
    }

    bool IsAssertBoundary(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & (OPCODEASSERTUPPERBOUNDARY | OPCODEASSERTLOWERBOUNDARY);
    }

    bool IsAssertUpperBoundary(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODEASSERTUPPERBOUNDARY;
    }

    bool IsAssertLowerBoundary(Opcode o) const
    {
        DEBUG_ASSERT(o < OP_last, "invalid opcode");
        return table[o].flag & OPCODEASSERTLOWERBOUNDARY;
    }

private:
    OpcodeDesc table[OP_last];
};
extern const OpcodeTable kOpcodeInfo;
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_OPCODE_INFO_H
