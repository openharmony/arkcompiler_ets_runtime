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

#ifndef MAPLEBE_INCLUDE_CG_DCE_H
#define MAPLEBE_INCLUDE_CG_DCE_H
#include "cgfunc.h"
#include "cg_ssa.h"

namespace maplebe {
/* dead code elimination*/
class CGDce {
public:
    CGDce(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo) : memPool(&mp), cgFunc(&f), ssaInfo(&sInfo) {}
    virtual ~CGDce() = default;

    void DoDce();
    /* provide public use in ssa opt */
    virtual bool RemoveUnuseDef(VRegVersion &defVersion) = 0;
    CGSSAInfo *GetSSAInfo()
    {
        return ssaInfo;
    }

protected:
    MemPool *memPool;
    CGFunc *cgFunc;
    CGSSAInfo *ssaInfo;
};

class DeleteRegUseVisitor : public OperandVisitorBase,
                            public OperandVisitors<RegOperand, ListOperand, MemOperand>,
                            public OperandVisitor<PhiOperand> {
public:
    DeleteRegUseVisitor(CGSSAInfo &cgSSAInfo, uint32 dInsnID) : deleteInsnId(dInsnID), ssaInfo(&cgSSAInfo) {}
    virtual ~DeleteRegUseVisitor() = default;

protected:
    CGSSAInfo *GetSSAInfo()
    {
        return ssaInfo;
    }
    uint32 deleteInsnId;

private:
    CGSSAInfo *ssaInfo;
};

MAPLE_FUNC_PHASE_DECLARE(CgDce, maplebe::CGFunc)
}  // namespace maplebe
#endif /* MAPLEBE_INCLUDE_CG_DCE_H */
