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

#ifndef MAPLEIR_INCLUDE_MIRSYMBOLBUILDER_H
#define MAPLEIR_INCLUDE_MIRSYMBOLBUILDER_H
#include <string>
#include <utility>
#include <vector>
#include <map>
#include "opcodes.h"
#include "prim_types.h"
#include "mir_type.h"
#include "mir_const.h"
#include "mir_symbol.h"
#include "mir_nodes.h"
#include "mir_module.h"
#include "mir_preg.h"
#include "mir_function.h"
#include "printing.h"
#include "intrinsic_op.h"
#include "opcode_info.h"
#include "global_tables.h"

namespace maple {
class MIRSymbolBuilder {
public:
    static MIRSymbolBuilder &Instance()
    {
        static MIRSymbolBuilder builder;
        return builder;
    }

    MIRSymbol *GetLocalDecl(const MIRSymbolTable &symbolTable, const GStrIdx &strIdx) const;
    MIRSymbol *CreateSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass, MIRFunction *func,
                            uint8 scpID) const;
    MIRSymbol *CreatePregFormalSymbol(TyIdx tyIdx, PregIdx pRegIdx, MIRFunction &func) const;

private:
    MIRSymbolBuilder() = default;
    ~MIRSymbolBuilder() = default;
    MIRSymbolBuilder(const MIRSymbolBuilder &) = delete;
    MIRSymbolBuilder(const MIRSymbolBuilder &&) = delete;
    MIRSymbolBuilder &operator=(const MIRSymbolBuilder &) = delete;
    MIRSymbolBuilder &operator=(const MIRSymbolBuilder &&) = delete;
};
}  // namespace maple
#endif  // MAPLEIR_INCLUDE_MIRSYMBOLBUILDER_H
