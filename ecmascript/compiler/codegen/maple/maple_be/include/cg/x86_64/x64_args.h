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

#ifndef MAPLEBE_INCLUDE_CG_X64_X64_ARGS_H
#define MAPLEBE_INCLUDE_CG_X64_X64_ARGS_H

#include "args.h"
#include "x64_isa.h"
#include "x64_cgfunc.h"
#include "x64_call_conv.h"

namespace maplebe {
using namespace maple;
using namespace x64;
struct X64ArgInfo {
    X64reg reg;
    MIRType *mirTy;
    uint32 symSize;
    uint32 stkSize;
    RegType regType;
    MIRSymbol *sym;
    const X64SymbolAlloc *symLoc;
    uint8 memPairSecondRegSize; /* struct arg requiring two regs, size of 2nd reg */
    bool doMemPairOpt;
    bool createTwoStores;
    bool isTwoRegParm;
};

class X64MoveRegArgs : public MoveRegArgs {
public:
    explicit X64MoveRegArgs(CGFunc &func) : MoveRegArgs(func) {}
    ~X64MoveRegArgs() override = default;
    void Run() override;

private:
    void CollectRegisterArgs(std::map<uint32, X64reg> &argsList, std::vector<uint32> &indexList,
                             std::map<uint32, X64reg> &pairReg, std::vector<uint32> &numFpRegs,
                             std::vector<uint32> &fpSize) const;
    X64ArgInfo GetArgInfo(std::map<uint32, X64reg> &argsList, uint32 argIndex, std::vector<uint32> &numFpRegs,
                          std::vector<uint32> &fpSize) const;
    void GenerateMovInsn(X64ArgInfo &argInfo, X64reg reg2);
    void MoveRegisterArgs();
    void MoveVRegisterArgs();
    void LoadStackArgsToVReg(MIRSymbol &mirSym);
    void MoveArgsToVReg(const CCLocInfo &ploc, MIRSymbol &mirSym);
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_X64_X64_ARGS_H */
