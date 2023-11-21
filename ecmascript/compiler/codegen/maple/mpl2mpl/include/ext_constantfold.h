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

#ifndef MPL2MPL_INCLUDE_EXTCONSTANTFOLD_H
#define MPL2MPL_INCLUDE_EXTCONSTANTFOLD_H
#include "mir_nodes.h"
#include "phase_impl.h"

namespace maple {
class ExtConstantFold {
public:
    explicit ExtConstantFold(MIRModule *mod) : mirModule(mod) {}
    BaseNode *ExtFoldUnary(UnaryNode *node);
    BaseNode *ExtFoldBinary(BinaryNode *node);
    BaseNode *ExtFoldTernary(TernaryNode *node);
    StmtNode *ExtSimplify(StmtNode *node);
    BaseNode *ExtFold(BaseNode *node);
    BaseNode *ExtFoldIor(BinaryNode *node);
    BaseNode *ExtFoldXand(BinaryNode *node);
    StmtNode *ExtSimplifyBlock(BlockNode *node);
    StmtNode *ExtSimplifyIf(IfStmtNode *node);
    StmtNode *ExtSimplifyDassign(DassignNode *node);
    StmtNode *ExtSimplifyIassign(IassignNode *node);
    StmtNode *ExtSimplifyWhile(WhileStmtNode *node);
    BaseNode *DispatchFold(BaseNode *node);

    MIRModule *mirModule;
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_EXTCONSTANTFOLD_H
