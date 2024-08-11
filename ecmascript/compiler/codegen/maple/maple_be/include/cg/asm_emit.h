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

#ifndef MAPLEBE_INCLUDE_CG_ASM_EMIT_H
#define MAPLEBE_INCLUDE_CG_ASM_EMIT_H

#include "emit.h"

namespace maplebe {
class AsmFuncEmitInfo : public FuncEmitInfo {
public:
    explicit AsmFuncEmitInfo(CGFunc &func) : FuncEmitInfo(func) {}
    virtual ~AsmFuncEmitInfo() = default;
};

class AsmEmitter : public Emitter {
protected:
    AsmEmitter(CG &cg, const std::string &asmFileName) : Emitter(cg, asmFileName)
    {
        fileStream.open(asmFileName, std::ios::trunc);
    }

    virtual ~AsmEmitter() = default;

    virtual void EmitBBHeaderLabel(FuncEmitInfo &funcEmitInfo, const std::string &name, LabelIdx labIdx) = 0;
    virtual void Run(FuncEmitInfo &funcEmitInfo) = 0;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_ASM_EMIT_H */
