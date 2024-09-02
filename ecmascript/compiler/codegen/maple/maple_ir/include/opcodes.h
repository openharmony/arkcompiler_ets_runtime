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

#ifndef MAPLE_IR_INCLUDE_OPCODES_H
#define MAPLE_IR_INCLUDE_OPCODES_H
#include "types_def.h"
#include "mpl_logging.h"

namespace maple {
enum Opcode : uint8 {
    OP_undef,
#define OPCODE(STR, YY, ZZ, SS) OP_##STR,
#include "opcodes.def"
#undef OPCODE
    OP_last,
};

inline constexpr bool IsDAssign(Opcode code)
{
    return (code == OP_dassign);
}

inline constexpr bool IsCallAssigned(Opcode code)
{
    return (code == OP_callassigned ||
            code == OP_icallassigned || code == OP_icallprotoassigned || code == OP_intrinsiccallassigned);
}

inline constexpr bool IsBranch(Opcode opcode)
{
    return (opcode == OP_goto || opcode == OP_brtrue || opcode == OP_brfalse || opcode == OP_switch);
}

inline constexpr bool IsLogicalShift(Opcode opcode)
{
    return (opcode == OP_lshr || opcode == OP_shl);
}

constexpr bool IsCommutative(Opcode opcode)
{
    switch (opcode) {
        case OP_add:
        case OP_mul:
        case OP_max:
        case OP_min:
        case OP_band:
        case OP_bior:
        case OP_bxor:
        case OP_eq:
        case OP_ne:
            return true;
        default:
            return false;
    }
}

constexpr bool IsStmtMustRequire(Opcode opcode)
{
    switch (opcode) {
        case OP_return:
        case OP_call:
        case OP_callassigned:
        case OP_icall:
        case OP_icallassigned:
        case OP_icallproto:
        case OP_icallprotoassigned:
        case OP_intrinsiccall:
        case OP_intrinsiccallassigned:
        case OP_intrinsiccallwithtype: {
            return true;
        }
        default:
            return false;
    }
}

// the result of these op is actually u1(may be set as other type, but its return value can only be zero or one)
// different from kOpcodeInfo.IsCompare(op) : cmp/cmpg/cmpl have no reverse op, and may return -1/0/1
constexpr bool IsCompareHasReverseOp(Opcode op)
{
    if (op == OP_eq || op == OP_ne || op == OP_ge || op == OP_gt || op == OP_le || op == OP_lt) {
        return true;
    }
    return false;
}

constexpr Opcode GetSwapCmpOp(Opcode op)
{
    switch (op) {
        case OP_eq:
            return OP_eq;
        case OP_ne:
            return OP_ne;
        case OP_ge:
            return OP_le;
        case OP_gt:
            return OP_lt;
        case OP_le:
            return OP_ge;
        case OP_lt:
            return OP_gt;
        default:
            CHECK_FATAL(false, "can't swap op");
            return op;
    }
}

constexpr Opcode GetReverseCmpOp(Opcode op)
{
    switch (op) {
        case OP_eq:
            return OP_ne;
        case OP_ne:
            return OP_eq;
        case OP_ge:
            return OP_lt;
        case OP_gt:
            return OP_le;
        case OP_le:
            return OP_gt;
        case OP_lt:
            return OP_ge;
        default:
            CHECK_FATAL(false, "opcode has no reverse op");
            return op;
    }
}

constexpr bool IsSupportedOpForCopyInPhasesLoopUnrollAndVRP(Opcode op)
{
    switch (op) {
        case OP_switch:
        case OP_comment:
        case OP_goto:
        case OP_dassign:
        case OP_regassign:
        case OP_brfalse:
        case OP_brtrue:
        case OP_iassign:
        case OP_call:
        case OP_callassigned:
        case OP_intrinsiccall:
        case OP_intrinsiccallassigned:
        case OP_intrinsiccallwithtype: {
            return true;
        }
        default:
            return false;
    }
}
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_OPCODES_H
