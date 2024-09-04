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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_INSN_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_INSN_H

#include "aarch64_isa.h"
#include "insn.h"
#include "string_utils.h"
#include "aarch64_operand.h"
#include "common_utils.h"
namespace maplebe {
class A64OpndEmitVisitor : public OpndEmitVisitor {
public:
    A64OpndEmitVisitor(Emitter &emitter, const OpndDesc *operandProp) : OpndEmitVisitor(emitter), opndProp(operandProp)
    {
    }
    ~A64OpndEmitVisitor() override
    {
        opndProp = nullptr;
    }

    void Visit(RegOperand *v) final;
    void Visit(ImmOperand *v) final;
    void Visit(MemOperand *v) final;
    void Visit(CondOperand *v) final;
    void Visit(StImmOperand *v) final;
    void Visit(BitShiftOperand *v) final;
    void Visit(ExtendShiftOperand *v) final;
    void Visit(LabelOperand *v) final;
    void Visit(FuncNameOperand *v) final;
    void Visit(CommentOperand *v) final;
    void Visit(OfstOperand *v) final;
    void Visit(ListOperand *v) final;

private:
    void EmitIntReg(const RegOperand &v, uint8 opndSz = kMaxSimm32);
    void Visit(const MIRSymbol &symbol, int64 offset);

    const OpndDesc *opndProp;
};

class A64OpndDumpVisitor : public OpndDumpVisitor {
public:
    A64OpndDumpVisitor(const OpndDesc &operandDesc) : OpndDumpVisitor(operandDesc) {}
    ~A64OpndDumpVisitor() override = default;

    void Visit(RegOperand *v) final;
    void Visit(ImmOperand *v) final;
    void Visit(MemOperand *v) final;
    void Visit(ListOperand *v) final;
    void Visit(CondOperand *v) final;
    void Visit(StImmOperand *v) final;
    void Visit(BitShiftOperand *v) final;
    void Visit(ExtendShiftOperand *v) final;
    void Visit(LabelOperand *v) final;
    void Visit(FuncNameOperand *v) final;
    void Visit(PhiOperand *v) final;
    void Visit(CommentOperand *v) final;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_INSN_H */
