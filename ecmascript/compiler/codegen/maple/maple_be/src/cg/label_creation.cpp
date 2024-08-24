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

#include "label_creation.h"
#include "cgfunc.h"
#include "cg.h"

namespace maplebe {
using namespace maple;

void LabelCreation::Run() const
{
    CreateStartEndLabel();
}

void LabelCreation::CreateStartEndLabel() const
{
    DEBUG_ASSERT(cgFunc != nullptr, "expect a cgfunc before CreateStartEndLabel");
    MIRBuilder *mirBuilder = cgFunc->GetFunction().GetModule()->GetMIRBuilder();
    DEBUG_ASSERT(mirBuilder != nullptr, "get mirbuilder failed in CreateStartEndLabel");
    /* create start label */
    LabelIdx startLblIdx = cgFunc->CreateLabel();
    LabelNode *startLabel = mirBuilder->CreateStmtLabel(startLblIdx);
    cgFunc->GetFunction().GetBody()->InsertFirst(startLabel);
    /* creat return label */
    LabelIdx returnLblIdx = cgFunc->CreateLabel();
    LabelNode *returnLabel = mirBuilder->CreateStmtLabel(returnLblIdx);
    cgFunc->GetFunction().GetBody()->InsertLast(returnLabel);

    /* create end label */
    LabelIdx endLblIdx = cgFunc->CreateLabel();
    LabelNode *endLabel = mirBuilder->CreateStmtLabel(endLblIdx);
    cgFunc->SetEndLabel(*endLabel);
    cgFunc->GetFunction().GetBody()->InsertLast(endLabel);
    DEBUG_ASSERT(cgFunc->GetFunction().GetBody()->GetLast() == endLabel, "last stmt must be a endLabel");

    /* create function's low/high pc if dwarf enabled */
    MIRFunction *func = &cgFunc->GetFunction();
    /* add start/end labels into the static map table in class cg */
    if (!CG::IsInFuncWrapLabels(func)) {
        CG::SetFuncWrapLabels(func, std::make_pair(startLblIdx, endLblIdx));
    }
}

bool CgCreateLabel::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *memPool = GetPhaseMemPool();
    LabelCreation *labelCreate = memPool->New<LabelCreation>(f);
    labelCreate->Run();
    return false;
}
} /* namespace maplebe */
