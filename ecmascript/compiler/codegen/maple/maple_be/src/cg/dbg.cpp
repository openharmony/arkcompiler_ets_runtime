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

#include "dbg.h"
#include "emit.h"

namespace mpldbg {
using maplebe::CG;
using maplebe::Emitter;
using maplebe::MOperator;
using maplebe::Operand;
using maplebe::OpndDesc;

struct DbgDescr {
    const std::string name;
    uint32 opndCount;
    /* create 3 OperandType array to store dbg instruction's operand type */
    std::array<Operand::OperandType, 3> opndTypes;
};

static DbgDescr dbgDescrTable[kOpDbgLast + 1] = {
#define DBG_DEFINE(k, sub, n, o0, o1, o2) {#k, n, {Operand::kOpd##o0, Operand::kOpd##o1, Operand::kOpd##o2}},
#include "dbg.def"
#undef DBG_DEFINE
    {"undef", 0, {Operand::kOpdUndef, Operand::kOpdUndef, Operand::kOpdUndef}}};

void DbgInsn::Dump() const
{
    MOperator mOp = GetMachineOpcode();
    DbgDescr &dbgDescr = dbgDescrTable[mOp];
    LogInfo::MapleLogger() << "DBG " << dbgDescr.name;
    for (uint32 i = 0; i < dbgDescr.opndCount; ++i) {
        LogInfo::MapleLogger() << (i == 0 ? " : " : " ");
        Operand &curOperand = GetOperand(i);
        curOperand.Dump();
    }
    LogInfo::MapleLogger() << "\n";
}

bool DbgInsn::CheckMD() const
{
    DbgDescr &dbgDescr = dbgDescrTable[GetMachineOpcode()];
    /* dbg instruction's 3rd /4th/5th operand must be null */
    for (uint32 i = 0; i < dbgDescr.opndCount; ++i) {
        Operand &opnd = GetOperand(i);
        if (opnd.GetKind() != dbgDescr.opndTypes[i]) {
            return false;
        }
    }
    return true;
}

uint32 DbgInsn::GetLoc() const
{
    if (mOp != OP_DBG_loc) {
        return 0;
    }
    return static_cast<uint32>(static_cast<ImmOperand *>(opnds[0])->GetVal());
}

void ImmOperand::Dump() const
{
    LogInfo::MapleLogger() << " " << val;
}
void DBGOpndEmitVisitor::Visit(ImmOperand *v)
{
    emitter.Emit(v->GetVal());
}
}  // namespace mpldbg
