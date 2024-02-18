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

#ifndef MAPLEBE_INCLUDE_CG_PROEPILOG_H
#define MAPLEBE_INCLUDE_CG_PROEPILOG_H
#include "cg_phase.h"
#include "cgfunc.h"
#include "insn.h"

namespace maplebe {
class GenProEpilog {
public:
    explicit GenProEpilog(CGFunc &func) : cgFunc(func) {}

    virtual ~GenProEpilog() = default;

    virtual void Run() {}

    std::string PhaseName() const
    {
        return "generateproepilog";
    }

    virtual bool NeedProEpilog()
    {
        return true;
    }

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

protected:
    CGFunc &cgFunc;
    int64 offsetFromCfa = 0; /* SP offset from Call Frame Address */
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_PROEPILOG_H */
