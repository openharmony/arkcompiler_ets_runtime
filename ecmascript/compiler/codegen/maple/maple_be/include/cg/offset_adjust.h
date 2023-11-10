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

#ifndef MAPLEBE_INCLUDE_CG_OFFSET_ADJUST_H
#define MAPLEBE_INCLUDE_CG_OFFSET_ADJUST_H

#include "cgfunc.h"
#include "cg_phase.h"

namespace maplebe {
class FrameFinalize {
public:
    explicit FrameFinalize(CGFunc &func) : cgFunc(&func) {}

    virtual ~FrameFinalize()
    {
        cgFunc = nullptr;
    }

    virtual void Run() {}

    std::string PhaseName() const
    {
        return "framefinalize";
    }

protected:
    CGFunc *cgFunc;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgFrameFinalize, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_OFFSET_ADJUST_H */
