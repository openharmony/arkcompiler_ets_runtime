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

struct AArch64ArgInfo {
    AArch64reg reg;
    MIRType *mirTy;
    uint32 symSize;
    uint32 stkSize;
    RegType regType;
    MIRSymbol *sym;
    const AArch64SymbolAlloc *symLoc;
    uint8 memPairSecondRegSize; /* struct arg requiring two regs, size of 2nd reg */
    bool doMemPairOpt;
    bool createTwoStores;
    bool isTwoRegParm;
};

class AArch64MoveRegArgs : public MoveRegArgs {
public:
    explicit AArch64MoveRegArgs(CGFunc &func) : MoveRegArgs(func) {}
    ~AArch64MoveRegArgs() override = default;
    void Run() override;

private:
    RegOperand *baseReg = nullptr;
    const MemSegment *lastSegment = nullptr;
    void CollectRegisterArgs(std::map<uint32, AArch64reg> &argsList, std::vector<uint32> &indexList,
                             std::map<uint32, AArch64reg> &pairReg, std::vector<uint32> &numFpRegs,
                             std::vector<uint32> &fpSize) const;
    AArch64ArgInfo GetArgInfo(std::map<uint32, AArch64reg> &argsList, std::vector<uint32> &numFpRegs,
                       std::vector<uint32> &fpSize, uint32 argIndex) const;
    bool IsInSameSegment(const AArch64ArgInfo &firstArgInfo, const AArch64ArgInfo &secondArgInfo) const;
    void GenOneInsn(const AArch64ArgInfo &argInfo, RegOperand &baseOpnd, uint32 stBitSize, AArch64reg dest,
                    int32 offset) const;
    void GenerateStpInsn(const AArch64ArgInfo &firstArgInfo, const AArch64ArgInfo &secondArgInfo);
    void GenerateStrInsn(const AArch64ArgInfo &argInfo, AArch64reg reg2, uint32 numFpRegs, uint32 fpSize);
    void MoveRegisterArgs();
    void MoveVRegisterArgs();
    void MoveLocalRefVarToRefLocals(MIRSymbol &mirSym) const;
    void LoadStackArgsToVReg(MIRSymbol &mirSym) const;
    void MoveArgsToVReg(const CCLocInfo &ploc, MIRSymbol &mirSym) const;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ARGS_H */
