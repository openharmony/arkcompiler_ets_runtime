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

#ifndef MAPLE_IR_INCLUDE_JAVA_EH_LOWER_H
#define MAPLE_IR_INCLUDE_JAVA_EH_LOWER_H
#include "phase_impl.h"
#include "class_hierarchy.h"
#include "maple_phase_manager.h"

namespace maple {
class JavaEHLowerer : public FuncOptimizeImpl {
public:
    JavaEHLowerer(MIRModule &mod, KlassHierarchy *kh, bool dump) : FuncOptimizeImpl(mod, kh, dump) {}
    ~JavaEHLowerer() = default;

    FuncOptimizeImpl *Clone() override
    {
        return new JavaEHLowerer(*this);
    }

    void ProcessFunc(MIRFunction *func) override;

private:
    BlockNode *DoLowerBlock(BlockNode &);
    BaseNode *DoLowerExpr(BaseNode &, BlockNode &);
    BaseNode *DoLowerDiv(BinaryNode &, BlockNode &);
    void DoLowerBoundaryCheck(IntrinsiccallNode &, BlockNode &);
    BaseNode *DoLowerRem(BinaryNode &expr, BlockNode &blkNode)
    {
        return DoLowerDiv(expr, blkNode);
    }

    uint32 divSTIndex = 0;              // The index of divide operand and result.
    bool useRegTmp = Options::usePreg;  // Use register to save temp variable or not.
};

MAPLE_MODULE_PHASE_DECLARE(M2MJavaEHLowerer)
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_JAVA_EH_LOWER_H
