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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ARGS_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ARGS_H

#include "args.h"
#include "aarch64_cgfunc.h"

namespace maplebe {
using namespace maple;

class AArch64MoveRegArgs : public MoveRegArgs {
public:
    explicit AArch64MoveRegArgs(CGFunc &func) : MoveRegArgs(func)
    {
        aarFunc = static_cast<AArch64CGFunc *>(&func);
    }
    ~AArch64MoveRegArgs() override
    {
        aarFunc = nullptr;
    }
    void Run() override;

private:
    // gen param to stack
    // call foo(var $a) -> str X0, [memOpnd]
    void MoveRegisterArgs() const;
    // gen param to preg
    // call foo(%1) -> mov V201, X0
    void MoveVRegisterArgs();
    void MoveLocalRefVarToRefLocals(MIRSymbol &mirSym) const;
    void LoadStackArgsToVReg(MIRSymbol &mirSym) const;
    void MoveArgsToVReg(const CCLocInfo &ploc, MIRSymbol &mirSym) const;
    Insn &CreateMoveArgsToVRegInsn(MOperator mOp, RegOperand &destOpnd, RegOperand &srcOpnd, PrimType primType) const;

    AArch64CGFunc *aarFunc = nullptr;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ARGS_H */
