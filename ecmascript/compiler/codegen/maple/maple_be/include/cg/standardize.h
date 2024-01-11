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

#ifndef MAPLEBE_INCLUDE_STANDARDIZE_H
#define MAPLEBE_INCLUDE_STANDARDIZE_H

#include "cgfunc.h"
namespace maplebe {
class Standardize {
public:
    explicit Standardize(CGFunc &f) : cgFunc(&f) {}

    virtual ~Standardize()
    {
        cgFunc = nullptr;
    }

    /*
     * for cpu instruction contains different operands
     * maple provide a default implement from three address to two address
     * convertion rule is:
     * mop(dest, src1, src2) -> mov(dest, src1)
     *                          mop(dest, src2)
     * maple provide a default implement from two address to one address for unary op
     * convertion rule is:
     * mop(dest, src) -> mov(dest, src1)
     *                   mop(dest)
     */
    void AddressMapping(Insn &insn);

    void DoStandardize();

protected:
    void SetAddressMapping(bool needMapping)
    {
        needAddrMapping = needMapping;
    }
    bool NeedAddressMapping(const Insn &insn)
    {
        /* Operand number for two addressing mode is 2 */
        /* and 3 for three addressing mode */
        constexpr uint32 operandSizeNeedAddrMapping = 3;
        needAddrMapping = (insn.GetOperandSize() >= operandSizeNeedAddrMapping) || (insn.IsUnaryOp());
        return needAddrMapping;
    }

private:
    virtual void StdzMov(Insn &insn) = 0;
    virtual void StdzStrLdr(Insn &insn) = 0;
    virtual void StdzBasicOp(Insn &insn) = 0;
    virtual void StdzUnaryOp(Insn &insn, CGFunc &cgFunc) = 0;
    virtual void StdzCvtOp(Insn &insn, CGFunc &cgFunc) = 0;
    virtual void StdzShiftOp(Insn &insn, CGFunc &cgFunc) = 0;
    CGFunc *cgFunc;
    bool needAddrMapping = false;
};
MAPLE_FUNC_PHASE_DECLARE_BEGIN(InstructionStandardize, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maplebe
#endif /* MAPLEBE_INCLUDE_STANDARDIZE_H */
