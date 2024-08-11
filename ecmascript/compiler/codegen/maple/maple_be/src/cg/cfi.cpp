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

#include "cfi.h"
#include "emit.h"

namespace cfi {
using maplebe::MOperator;
using maplebe::Operand;

struct CfiDescr {
    const std::string name;
    uint32 opndCount;
    /* create 3 OperandType array to store cfi instruction's operand type */
    std::array<Operand::OperandType, 3> opndTypes;
};

static CfiDescr cfiDescrTable[kOpCfiLast + 1] = {
#define CFI_DEFINE(k, sub, n, o0, o1, o2) {".cfi_" #k, n, {Operand::kOpd##o0, Operand::kOpd##o1, Operand::kOpd##o2}},
#define ARM_DIRECTIVES_DEFINE(k, sub, n, o0, o1, o2) \
    {"." #k, n, {Operand::kOpd##o0, Operand::kOpd##o1, Operand::kOpd##o2}},
#include "cfi.def"
#undef CFI_DEFINE
#undef ARM_DIRECTIVES_DEFINE
    {".cfi_undef", 0, {Operand::kOpdUndef, Operand::kOpdUndef, Operand::kOpdUndef}}};

void CfiInsn::Dump() const
{
    MOperator mOp = GetMachineOpcode();
    CfiDescr &cfiDescr = cfiDescrTable[mOp];
    LogInfo::MapleLogger() << "CFI " << cfiDescr.name;
    for (uint32 i = 0; i < static_cast<uint32>(cfiDescr.opndCount); ++i) {
        LogInfo::MapleLogger() << (i == 0 ? " : " : " ");
        Operand &curOperand = GetOperand(i);
        curOperand.Dump();
    }
    LogInfo::MapleLogger() << "\n";
}

bool CfiInsn::CheckMD() const
{
    CfiDescr &cfiDescr = cfiDescrTable[GetMachineOpcode()];
    /* cfi instruction's 3rd /4th/5th operand must be null */
    for (uint32 i = 0; i < static_cast<uint32>(cfiDescr.opndCount); ++i) {
        Operand &opnd = GetOperand(i);
        if (opnd.GetKind() != cfiDescr.opndTypes[i]) {
            return false;
        }
    }
    return true;
}

void RegOperand::Dump() const
{
    LogInfo::MapleLogger() << "reg: " << regNO << "[ size: " << GetSize() << "] ";
}

void ImmOperand::Dump() const
{
    LogInfo::MapleLogger() << "imm: " << val << "[ size: " << GetSize() << "] ";
}

void StrOperand::Dump() const
{
    LogInfo::MapleLogger() << str;
}

void LabelOperand::Dump() const
{
    LogInfo::MapleLogger() << "label:" << labelIndex;
}
void CFIOpndEmitVisitor::Visit(RegOperand *v)
{
    emitter.Emit(v->GetRegisterNO());
}
void CFIOpndEmitVisitor::Visit(ImmOperand *v)
{
    emitter.Emit(v->GetValue());
}
void CFIOpndEmitVisitor::Visit(SymbolOperand *v)
{
    CHECK_FATAL(false, "NIY");
}
void CFIOpndEmitVisitor::Visit(StrOperand *v)
{
    emitter.Emit(v->GetStr());
}
void CFIOpndEmitVisitor::Visit(LabelOperand *v)
{
    if (emitter.GetCG()->GetMIRModule()->IsCModule()) {
        CHECK_NULL_FATAL(emitter.GetCG()->GetMIRModule()->CurFunction());
        PUIdx pIdx = emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
        char *idx = strdup(std::to_string(pIdx).c_str());
        CHECK_FATAL(idx != nullptr, "strdup failed");
        emitter.Emit(".label.").Emit(idx).Emit("__").Emit(v->GetIabelIdx());
        free(idx);
        idx = nullptr;
    } else {
        emitter.Emit(".label.").Emit(v->GetParentFunc()).Emit(v->GetIabelIdx());
    }
}
} /* namespace cfi */
