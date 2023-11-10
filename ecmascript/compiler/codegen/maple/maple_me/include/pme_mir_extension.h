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

#ifndef MAPLE_ME_INCLUDE_PME_MIR_EXTENSION_H_
#define MAPLE_ME_INCLUDE_PME_MIR_EXTENSION_H_
#include "me_ir.h"
#include "mir_nodes.h"

namespace maple {
class PreMeMIRExtension {
public:
    BaseNode *parent;
    union {
        MeExpr *meexpr;
        MeStmt *mestmt;
    };

    explicit PreMeMIRExtension(BaseNode *p) : parent(p), meexpr(nullptr) {}
    PreMeMIRExtension(BaseNode *p, MeExpr *expr) : parent(p), meexpr(expr) {}
    PreMeMIRExtension(BaseNode *p, MeStmt *stmt) : parent(p), mestmt(stmt) {}
    virtual ~PreMeMIRExtension() = default;
    BaseNode *GetParent()
    {
        return parent;
    }
    MeExpr *GetMeExpr()
    {
        return meexpr;
    }
    MeStmt *GetMeStmt()
    {
        return mestmt;
    }
    void SetParent(BaseNode *p)
    {
        parent = p;
    }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_PME_MIR_EXTENSION_H_
