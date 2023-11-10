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

#ifndef MAPLEBE_INCLUDE_CG_STRLDR_H
#define MAPLEBE_INCLUDE_CG_STRLDR_H
#include "cg_phase.h"
#include "maple_phase.h"

namespace maplebe {
class StoreLoadOpt {
public:
    StoreLoadOpt(CGFunc &func, MemPool &memPool) : cgFunc(func), memPool(memPool) {}
    virtual ~StoreLoadOpt() = default;
    virtual void Run() = 0;
    std::string PhaseName() const
    {
        return "storeloadopt";
    }

protected:
    CGFunc &cgFunc;
    MemPool &memPool;
    /* if the number of bbs is more than 500 or the number of insns is more than 9000, don't optimize. */
    static constexpr uint32 kMaxBBNum = 500;
    static constexpr uint32 kMaxInsnNum = 9000;
};

MAPLE_FUNC_PHASE_DECLARE(CgStoreLoadOpt, maplebe::CGFunc)
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_STRLDR_H */