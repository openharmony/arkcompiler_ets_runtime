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
#ifndef MAPLEBE_INCLUDE_CG_CFI_GENERATOR_H
#define MAPLEBE_INCLUDE_CG_CFI_GENERATOR_H
#include "cg_phase.h"
#include "cfi.h"
#include "cgfunc.h"
#include "insn.h"
#include "aarch64_insn.h"
#include "aarch64_operand.h"

namespace maplebe {
class GenCfi {
public:
    explicit GenCfi(CGFunc &func) : cg(*func.GetCG()), cgFunc(func) {}
    virtual ~GenCfi() = default;

    void Run();

protected:
    Insn *InsertCFIDefCfaOffset(BB &bb, Insn &insn, int32 &cfiOffset); /* cfiOffset in-out */

    Insn *FindStackDefInsn(BB &bb) const;
    Insn *FinsStackRevertInsn(BB &bb) const;

    /* CFI related routines */
    int64 GetOffsetFromCFA() const
    {
        return offsetFromCfa;
    }

    /* add increment (can be negative) and return the new value */
    int64 AddtoOffsetFromCFA(int64 delta)
    {
        offsetFromCfa += delta;
        return offsetFromCfa;
    }

    CG &cg;
    CGFunc &cgFunc;
    /* SP offset from Call Frame Address */
    int64 offsetFromCfa = 0;
    bool useFP = true;

    static constexpr const int32 kOffset8MemPos = 8;

private:
    void GenerateStartDirective(BB &bb);
    void GenerateEndDirective(BB &bb);
    void GenerateRegisterStateDirective(BB &bb);
    virtual void GenerateRegisterSaveDirective(BB &bb, Insn &stackDefInsn) {}
    virtual void GenerateRegisterRestoreDirective(BB &bb, Insn &stackRevertInsn) {}
};
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_CFI_GENERATOR_H */
