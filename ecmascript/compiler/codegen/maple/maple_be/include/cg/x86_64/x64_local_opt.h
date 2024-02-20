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

#ifndef MAPLEBE_INCLUDE_X64_LOCALO_H
#define MAPLEBE_INCLUDE_X64_LOCALO_H

#include "local_opt.h"
namespace maplebe {
class X64LocalOpt : public LocalOpt {
public:
    X64LocalOpt(MemPool &memPool, CGFunc &func, ReachingDefinition &rd) : LocalOpt(memPool, func, rd) {}
    ~X64LocalOpt() = default;

private:
    void DoLocalCopyProp() override;
};

class LocalCopyRegProp  : public LocalPropOptimizePattern {
public:
    LocalCopyRegProp (CGFunc &cgFunc, ReachingDefinition &rd) : LocalPropOptimizePattern(cgFunc, rd) {}
    ~LocalCopyRegProp () override = default;
    bool CheckCondition(Insn &insn) final;
    void Optimize(BB &bb, Insn &insn) final;

private:
    bool propagateOperand(Insn &insn, RegOperand &oldOpnd, RegOperand &replaceOpnd);
};

class X64RedundantDefRemove : public RedundantDefRemove {
public:
    X64RedundantDefRemove(CGFunc &cgFunc, ReachingDefinition &rd) : RedundantDefRemove(cgFunc, rd) {}
    ~X64RedundantDefRemove() override = default;
    void Optimize(BB &bb, Insn &insn) final;
};

}  // namespace maplebe

#endif /* MAPLEBE_INCLUDE_X64_LOCALO_H */
