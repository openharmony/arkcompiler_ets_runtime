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

#ifndef MAPLEBE_INCLUDE_CG_VALIDBIT_OPT_H
#define MAPLEBE_INCLUDE_CG_VALIDBIT_OPT_H

#include "cg.h"
#include "cgfunc.h"
#include "bb.h"
#include "insn.h"
#include "cg_ssa.h"

namespace maplebe {
#define CG_VALIDBIT_OPT_DUMP CG_DEBUG_FUNC(*cgFunc)
class ValidBitPattern {
public:
    ValidBitPattern(CGFunc &f, CGSSAInfo &info) : cgFunc(&f), ssaInfo(&info) {}
    virtual ~ValidBitPattern()
    {
        cgFunc = nullptr;
        ssaInfo = nullptr;
    }
    std::string PhaseName() const
    {
        return "cgvalidbitopt";
    }

    virtual std::string GetPatternName() = 0;
    virtual bool CheckCondition(Insn &insn) = 0;
    virtual void Run(BB &bb, Insn &insn) = 0;
    Insn *GetDefInsn(const RegOperand &useReg);
    InsnSet GetAllUseInsn(const RegOperand &defReg);
    void DumpAfterPattern(std::vector<Insn *> &prevInsns, const Insn *replacedInsn, const Insn *newInsn);

protected:
    CGFunc *cgFunc;
    CGSSAInfo *ssaInfo;
};

class ValidBitOpt {
public:
    ValidBitOpt(CGFunc &f, CGSSAInfo &info) : cgFunc(&f), ssaInfo(&info) {}
    virtual ~ValidBitOpt()
    {
        cgFunc = nullptr;
        ssaInfo = nullptr;
    }
    void Run();
    static uint32 GetImmValidBit(int64 value, uint32 size)
    {
        if (value < 0) {
            return size;
        } else if (value == 0) {
            return k1BitSize;
        }
        uint32 pos = 0;
        constexpr int64 mask = 1;
        for (uint32 i = 0; i <= k8BitSize * sizeof(int64); ++i, value >>= 1) {
            if ((value & mask) == mask) {
                pos = i + 1;
            }
        }
        return pos;
    }

    static int64 GetLogValueAtBase2(int64 val)
    {
        return (__builtin_popcountll(static_cast<uint64>(val)) == 1) ? (__builtin_ffsll(val) - 1) : -1;
    }

    template <typename VBOpt>
    void Optimize(BB &bb, Insn &insn)
    {
        VBOpt opt(*cgFunc, *ssaInfo);
        opt.Run(bb, insn);
    }
    virtual void DoOpt(BB &bb, Insn &insn) = 0;
    void RectifyValidBitNum();
    void RecoverValidBitNum();
    virtual void SetValidBits(Insn &insn) = 0;
    virtual bool SetPhiValidBits(Insn &insn) = 0;

protected:
    CGFunc *cgFunc;
    CGSSAInfo *ssaInfo;
};
MAPLE_FUNC_PHASE_DECLARE(CgValidBitOpt, maplebe::CGFunc)
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_VALIDBIT_OPT_H */
